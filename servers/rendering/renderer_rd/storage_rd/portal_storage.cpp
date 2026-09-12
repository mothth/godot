#include "portal_storage.h"
#include "servers/rendering/renderer_rd/renderer_scene_render_rd.h"
#include "servers/rendering/renderer_scene_render.h"

using namespace RendererRD;

PortalStorage *PortalStorage::singleton = nullptr;

PortalStorage::PortalStorage() {
	singleton = this;
	config_update();
	// ProjectSettings::get_singleton()->connect(SNAME("settings_changed"), callable_mp((RendererPortalStorage *)this, &RendererPortalStorage::config_update));

	/* Create default mesh buffers */

	{
		float vertices[] = {
			 1.0,  1.0,  0.0,
			-1.0,  1.0,  0.0,
			-1.0, -1.0,  0.0,
			 1.0,  -1.0,  0.0
		};
		default_vertex_buffer = RD::get_singleton()->vertex_buffer_create(sizeof(vertices), Span<uint8_t>((uint8_t *)vertices, sizeof(vertices)));
	}

	{
		uint16_t indices[6] = { 0, 1, 2, 0, 2, 3 };
		default_index_buffer = RD::get_singleton()->index_buffer_create(6, RD::INDEX_BUFFER_FORMAT_UINT16, Span<uint8_t>((uint8_t *)indices, sizeof(indices)), false);
	}

	/* Create default mesh arrays */

	{
		{
			Vector<RD::VertexAttribute> attributes;
			{
				RD::VertexAttribute va;
				va.format = RD::DATA_FORMAT_R32G32B32_SFLOAT;
				va.stride = sizeof(float) * 3;
				attributes.push_back(va);
			}
			vertex_format = RD::get_singleton()->vertex_format_create(attributes);
		}

		Vector<RID> buffers;
		buffers.push_back(default_vertex_buffer);

		default_vertex_array = RD::get_singleton()->vertex_array_create(4, vertex_format, buffers);
	}

	default_index_array = RD::get_singleton()->index_array_create(default_index_buffer, 0, 6);
}

PortalStorage::~PortalStorage() {
	if (singleton == this) {
		singleton = nullptr;
	}
	RD::get_singleton()->free_rid(default_vertex_buffer);
	RD::get_singleton()->free_rid(default_index_buffer);
	// RD::get_singleton()->free_rid(default_vertex_array);
	// RD::get_singleton()->free_rid(default_index_array);
}

bool PortalStorage::free(RID p_rid) {
	if (portal_owner.owns(p_rid)) {
		portal_free(p_rid);
		return true;
	}
	return false;
}

/* PORTAL API */

RID PortalStorage::portal_allocate() {
	return portal_owner.allocate_rid();
}

void PortalStorage::portal_initialize(RID p_rid) {
	portal_owner.initialize_rid(p_rid, Portal());
}

void PortalStorage::portal_free(RID p_rid) {
	Portal *portal = portal_get_or_null(p_rid);
	portal->dependency.deleted_notify(p_rid);
	if (last_portal_rid == p_rid) {
		last_portal_rid = RID();
		last_portal = nullptr;
	}
	portal_owner.free(p_rid);
}

void PortalStorage::portal_set_double_sided(RID p_portal, bool p_double_sided) {
	Portal *portal = portal_get_or_null(p_portal);
	ERR_FAIL_NULL(portal);

	portal->double_sided = p_double_sided;
}

void PortalStorage::portal_set_teleport_light(RID p_portal, bool p_teleport_light) {
	Portal *portal = portal_get_or_null(p_portal);
	ERR_FAIL_NULL(portal);

	portal->teleport_light = p_teleport_light;
}

void PortalStorage::portal_set_teleport_gi(RID p_portal, bool p_teleport_gi) {
	Portal *portal = portal_get_or_null(p_portal);
	ERR_FAIL_NULL(portal);

	portal->teleport_gi = p_teleport_gi;
}

void PortalStorage::portal_set_mesh(RID p_portal, RID p_mesh) {
	Portal *portal = portal_get_or_null(p_portal);
	ERR_FAIL_NULL(portal);

	portal->mesh = p_mesh;
}

void PortalStorage::portal_set_destination_transform(RID p_portal, const Transform3D &p_transform) {
	Portal *portal = portal_get_or_null(p_portal);
	ERR_FAIL_NULL(portal);

	portal->destination = p_transform;
}

void PortalStorage::portal_set_scenario_override(RID p_portal, RID p_scenario) {
	Portal *portal = portal_get_or_null(p_portal);
	ERR_FAIL_NULL(portal);

	portal->scenario_override = p_scenario;
}

void PortalStorage::portal_set_environment_override(RID p_portal, RID p_environment) {
	Portal *portal = portal_get_or_null(p_portal);
	ERR_FAIL_NULL(portal);

	portal->env_override = p_environment;
}

void PortalStorage::portal_set_recursive_depth(RID p_portal, int p_depth) {
	Portal *portal = portal_get_or_null(p_portal);
	ERR_FAIL_NULL(portal);

	portal->max_depth = p_depth;
}

void PortalStorage::portal_set_cull_mask(RID p_portal, RenderingServer::PortalCullMaskOp p_op, uint32_t p_mask) {
	Portal *portal = portal_get_or_null(p_portal);
	ERR_FAIL_NULL(portal);

	portal->cull_mask_op = p_op;
	if (p_op != RenderingServer::PORTAL_CULL_MASK_OP_KEEP) {
		portal->cull_mask = p_mask;
	}
}

void PortalStorage::portal_set_cull_partner(RID p_portal, RID p_partner) {
	Portal *portal = portal_get_or_null(p_portal);
	ERR_FAIL_NULL(portal);

	portal->cull_partner = p_partner;
}

bool PortalStorage::portal_is_local_to_environment(RID p_portal) const {
	const Portal *portal = portal_get_or_null(p_portal);
	ERR_FAIL_NULL_V(portal, false);

	return portal->env_override.is_null() && portal->scenario_override.is_null();
}

int PortalStorage::portal_get_max_recursive_depth(RID p_portal) const {
	const Portal *portal = portal_get_or_null(p_portal);
	ERR_FAIL_NULL_V(portal, max_portal_depth);
	return portal->max_depth >= 0 ? MIN(portal->max_depth, max_portal_depth) : max_portal_depth;
}

AABB PortalStorage::portal_get_aabb(RID p_portal) const {
	const Portal *portal = portal_get_or_null(p_portal);
	ERR_FAIL_NULL_V(portal, AABB());

	if (portal->mesh.is_null()) {
		// Default AABB
		return AABB(Vector3(-1.0, -1.0, 0.0), Vector3(2.0, 2.0, 0.0));
	} else {
		// TODO
		return AABB();
	}
}

Projection PortalStorage::portal_get_oblique_projection(RID p_portal, const Projection &p_cam_projection, const Transform3D &p_cam_transform) const {
	const Portal *portal = portal_get_or_null(p_portal);
	ERR_FAIL_NULL_V(portal, Projection());

	Projection result = p_cam_projection;

	Vector3 cam_position = p_cam_transform.get_origin();
	Vector3 normal = portal->destination.basis.get_column(2);
	Plane portal_plane = Plane(normal, portal->destination.get_origin().dot(normal));

	float camera_distance = portal_plane.distance_to(cam_position);
	float z_near = p_cam_projection.get_z_near();
	float epsilon = 0.005f;

	// Buggy behaviour arises when we have an oblique plane too close to the camera, so we have a little threshold.
	// This is fine in most cases as the oblique plane is to cut off content behind the portal,
	// so this wouldn't matter if we're close to the portal. However, it could still be problematic...
	// We may consider making it a parameter...
	if (camera_distance >= z_near + epsilon) {
		float camera_side = portal_plane.is_point_over(cam_position) ? 1.0f : -1.0f;

		// We have an epilson as a little headroom between the near plane and the portal, as sometimes a fine gap can be seen
		Vector3 oblique_position = portal->destination.xform(Vector3(0.0f, 0.0f, epsilon * camera_side));
		Vector3 oblique_normal = normal * -camera_side;
		Plane plane = make_oblique_plane(p_cam_transform, oblique_position, oblique_normal);
		result.apply_oblique_plane(plane);
	}

	return result;
}

void PortalStorage::portal_get_rd_arrays(RID p_portal, RID &p_vertex_array, RID &p_index_array) const {
	const Portal *portal = portal_get_or_null(p_portal);
	ERR_FAIL_NULL(portal);

	if (portal->mesh.is_null()) {
		p_vertex_array = default_vertex_array;
		p_index_array = default_index_array;
	}
	else {
		// TODO: get custom mesh vertex and index array
	}
}

/* INSTANCE API */

RID PortalStorage::portal_instance_create(RID p_portal) {
	return instance_owner.make_rid(PortalInstance(p_portal));
}

void PortalStorage::portal_instance_free(RID p_instance) {
	if (last_instance_rid == p_instance) {
		last_instance_rid = RID();
		last_instance = nullptr;
	}
	instance_owner.free(p_instance);
}

void PortalStorage::portal_instance_set_transform(RID p_instance, const Transform3D &p_transform) {
	PortalInstance *instance = portal_instance_get_or_null(p_instance);
	ERR_FAIL_NULL(instance);
	instance->transform = p_transform;
	instance->inv_transform = p_transform.affine_inverse();
}

void PortalStorage::portal_instance_set_aabb(RID p_instance, const AABB &p_aabb) {
	PortalInstance *instance = portal_instance_get_or_null(p_instance);
	ERR_FAIL_NULL(instance);
	instance->aabb = p_aabb;
}

RID PortalStorage::portal_instance_get_base_portal(RID p_instance) const {
	const PortalInstance *instance = portal_instance_get_or_null(p_instance);
	ERR_FAIL_NULL_V(instance, RID());
	return instance->portal;
}

RID PortalStorage::portal_instance_get_owner(RID p_instance) const {
	const PortalInstance *instance = portal_instance_get_or_null(p_instance);
	ERR_FAIL_NULL_V(instance, RID());
	return instance->owner;
}

PortalRenderInfo PortalStorage::portal_instance_make_render_info(RID p_instance, const Transform3D &p_cam_transform, const Projection &p_cam_projection) const {
	const PortalInstance *instance = portal_instance_get_or_null(p_instance);
	ERR_FAIL_NULL_V(instance, PortalRenderInfo());

	const Portal *portal = portal_get_or_null(instance->portal);
	ERR_FAIL_NULL_V(portal, PortalRenderInfo());

	PortalRenderInfo info;

	/* Set general info */

	Vector3 cam_position = instance->transform.origin - p_cam_transform.origin;
	Vector3 portal_direction = -instance->transform.basis.get_column(2);

	info.front_facing = portal_direction.dot(cam_position) >= 0.0f;
	info.cull_mask_op = portal->cull_mask_op;
	info.cull_mask = portal->cull_mask;

	info.portal = instance->portal;
	info.partner_portal = portal->cull_partner;
	info.environment = portal->env_override;
	info.scenario = portal->scenario_override;

	/* Calculate transforms */

	info.source_transform = instance->transform;
	info.dest_transform = portal->destination;

	if (!portal->double_sided) {
		// Our destination transform is flipped if we're a double-sided portal
		info.dest_transform.basis.rotate_local(Vector3::UP, Math::PI);
	}
	else if (!info.front_facing) {
		// Correct transforms so they're always facing the camera
		info.front_facing = true;
		info.source_transform.basis.rotate_local(Vector3::UP, Math::PI);
		info.dest_transform.basis.rotate_local(Vector3::UP, Math::PI);
	}

	info.portal_transform = info.dest_transform * info.source_transform.affine_inverse();
	info.cam_transform = info.portal_transform * p_cam_transform;

	/* Get region on screen */

	Point2 min = Point2(1.0, 1.0);
	Point2 max = Point2(0.0, 0.0);
	Transform3D inv_cam_transform = p_cam_transform.affine_inverse();

	for (int i = 0; i < 8; i++) {
		Vector3 position = instance->aabb.get_endpoint(i);
		Plane plane(inv_cam_transform.xform(position), 1.0);
		plane = p_cam_projection.xform4(plane);

		// Prevent divide by zero.
		if (plane.d == 0) {
			continue;
		}

		plane.normal /= plane.d;

		Point2 point;
		point.x = plane.normal.x * 0.5 + 0.5;
		point.y = -plane.normal.y * 0.5 + 0.5;

		min = min.min(point).max(Point2(0.0, 0.0));
		max = max.max(point).min(Point2(1.0, 1.0));
	}
	
	info.region = Rect2(min, max - min);

	return info;
}

/* OTHER */

void PortalStorage::_make_vertex_format() const {
	Vector<RD::VertexAttribute> attributes;
	{
		RD::VertexAttribute va;
		va.format = RD::DATA_FORMAT_R32G32B32_SFLOAT;
		va.stride = sizeof(float) * 3;
		attributes.push_back(va);
	}
	vertex_format = RD::get_singleton()->vertex_format_create(attributes);
} 

/*
RendererSceneRender::CameraData PortalStorage::portal_instance_make_camera_data(RID p_instance, const RendererSceneRender::CameraData *p_camera_data) const {
	RendererSceneRender::CameraData data = *p_camera_data;

	PortalInstance *instance = portal_instance_get_or_null(p_instance);
	ERR_FAIL_NULL_V(instance, data);
	
	Portal *portal = portal_get_or_null(instance->portal);
	ERR_FAIL_NULL_V(portal, data);	

	Transform3D local = instance->inv_transform * data.main_transform;
	data.main_transform = portal->destination * local;

	if (data.view_count == 1) {
		// Buggy behaviour arises when we have an oblique plane so close to the camera, so we have a little threshold.
		// This is fine in most cases as the oblique plane is to cut off content behind the portal,
		// so this wouldn't matter if we're close to the portal. However, it could still be problematic...
		// This 0.05 value was the result of testing from when portals were implemented in The Story Machine rather than the engine,
		// any smaller and buggy behaviour seems to arise.
		// We may consider making it a parameter...
		Vector3 camera_origin = local.get_origin();
		if (std::abs(camera_origin.z) >= 0.05f) {
			// We have an epilson as a little headroom between the near plane and the portal, a fine gap can be seen otherwise
			Vector3 oblique_position = portal->destination.xform(Vector3(0.0f, 0.0f, 0.01f) * SIGN(camera_origin.z));
			Vector3 oblique_normal = portal->destination.basis.get_column(2) * SIGN(camera_origin.z);
			Vector4 plane = make_oblique_plane(data.main_transform, oblique_position, oblique_normal);

			// We base our projections off the shadow projection, as that will be untouched by any already applied oblique projections
			data.main_projection = data.shadow_projection;
			data.main_projection.apply_oblique_plane(plane);
			data.view_projection[0] = data.main_projection;
		}
	}
	else {
		// Multi-view is currently unsupported
	}

	data.portal_owner = p_instance;

	return data;
}
*/