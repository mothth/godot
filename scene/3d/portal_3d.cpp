#include "portal_3d.h"

#include "servers/rendering/rendering_server.h"
#include "scene/3d/physics/physics_body_3d.h"
#include "scene/resources/mesh.h"

Portal3D::Portal3D() {

}

Portal3D::~Portal3D() {
	if (rendering_rid.is_valid()) {
		RenderingServer::get_singleton()->free_rid(rendering_rid);
		rendering_rid = RID();
	}
	
	if (physics_rid.is_valid()) {
		PhysicsServer3D::get_singleton()->free_rid(physics_rid);
		physics_rid = RID();
	}
}

void Portal3D::_bind_methods() {
	/* Setters */

	ClassDB::bind_method(D_METHOD("set_partner_path", "path"), &Portal3D::set_partner_path);
	ClassDB::bind_method(D_METHOD("set_teleport_path", "path"), &Portal3D::set_teleport_path);
	ClassDB::bind_method(D_METHOD("set_custom_environment", "environment"), &Portal3D::set_custom_environment);
	ClassDB::bind_method(D_METHOD("set_custom_mesh", "mesh"), &Portal3D::set_custom_mesh);
	ClassDB::bind_method(D_METHOD("set_enabled", "enabled"), &Portal3D::set_enabled);
	ClassDB::bind_method(D_METHOD("set_double_sided", "enabled"), &Portal3D::set_double_sided);
	ClassDB::bind_method(D_METHOD("set_physics_enabled", "enabled"), &Portal3D::set_physics_enabled);
	// ClassDB::bind_method(D_METHOD("set_scaling_enabled", "enabled"), &Portal3D::set_scaling_enabled);
	ClassDB::bind_method(D_METHOD("set_teleport_audio", "enabled"), &Portal3D::set_teleport_audio);
	ClassDB::bind_method(D_METHOD("set_teleport_light", "enabled"), &Portal3D::set_teleport_light);
	ClassDB::bind_method(D_METHOD("set_teleport_gi", "enabled"), &Portal3D::set_teleport_gi);
	ClassDB::bind_method(D_METHOD("set_collision_ghost_mode", "mode"), &Portal3D::set_collision_ghost_mode);
	ClassDB::bind_method(D_METHOD("set_mesh_ghost_mode", "mode"), &Portal3D::set_mesh_ghost_mode);
	ClassDB::bind_method(D_METHOD("set_cull_mask_operator", "operator"), &Portal3D::set_cull_mask_op);
	ClassDB::bind_method(D_METHOD("set_cull_mask", "mask"), &Portal3D::set_cull_mask);
	ClassDB::bind_method(D_METHOD("set_collision_layer", "layer"), &Portal3D::set_collision_layer);
	ClassDB::bind_method(D_METHOD("set_teleport_mask", "mask"), &Portal3D::set_teleport_mask);
	ClassDB::bind_method(D_METHOD("set_max_recursion_depth", "depth"), &Portal3D::set_max_recursion_depth);
	
	/* Getters */

	ClassDB::bind_method(D_METHOD("get_partner_path"), &Portal3D::get_partner_path);
	ClassDB::bind_method(D_METHOD("get_teleport_path"), &Portal3D::get_teleport_path);
	ClassDB::bind_method(D_METHOD("get_custom_environment"), &Portal3D::get_custom_environment);
	ClassDB::bind_method(D_METHOD("get_custom_mesh"), &Portal3D::get_custom_mesh);
	ClassDB::bind_method(D_METHOD("is_enabled"), &Portal3D::is_enabled);
	ClassDB::bind_method(D_METHOD("is_double_sided"), &Portal3D::is_double_sided);
	ClassDB::bind_method(D_METHOD("is_physics_enabled"), &Portal3D::is_physics_enabled);
	// ClassDB::bind_method(D_METHOD("is_scaling_enabled"), &Portal3D::is_scaling_enabled);
	ClassDB::bind_method(D_METHOD("teleports_audio"), &Portal3D::teleports_audio);
	ClassDB::bind_method(D_METHOD("teleports_light"), &Portal3D::teleports_light);
	ClassDB::bind_method(D_METHOD("teleports_gi"), &Portal3D::teleports_gi);
	ClassDB::bind_method(D_METHOD("get_collision_ghost_mode"), &Portal3D::get_collision_ghost_mode);
	ClassDB::bind_method(D_METHOD("get_mesh_ghost_mode"), &Portal3D::get_mesh_ghost_mode);
	ClassDB::bind_method(D_METHOD("get_cull_mask_operator"), &Portal3D::get_cull_mask_op);
	ClassDB::bind_method(D_METHOD("get_cull_mask"), &Portal3D::get_cull_mask);
	ClassDB::bind_method(D_METHOD("get_collision_layer"), &Portal3D::get_collision_layer);
	ClassDB::bind_method(D_METHOD("get_teleport_mask"), &Portal3D::get_teleport_mask);
	ClassDB::bind_method(D_METHOD("get_max_recursion_depth"), &Portal3D::get_max_recursion_depth);

	/* Utility */

	ClassDB::bind_method(D_METHOD("get_partner"), &Portal3D::get_partner);
	ClassDB::bind_method(D_METHOD("teleport_transform", "transform"), &Portal3D::teleport_transform);
	ClassDB::bind_method(D_METHOD("teleport_point", "point"), &Portal3D::teleport_point);
	ClassDB::bind_method(D_METHOD("teleport_direction", "direction"), &Portal3D::teleport_direction);
	ClassDB::bind_method(D_METHOD("teleport_node", "node"), &Portal3D::teleport_node);
	ClassDB::bind_method(D_METHOD("is_world_portal"), &Portal3D::is_world_portal);

	/* Properties */

	ADD_PROPERTY(PropertyInfo(Variant::NODE_PATH, "partner", PROPERTY_HINT_NODE_PATH_VALID_TYPES, "Portal3D"), "set_partner_path", "get_partner_path");
	ADD_PROPERTY(PropertyInfo(Variant::NODE_PATH, "teleport_path"), "set_teleport_path", "get_teleport_path");
	ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "custom_mesh", PROPERTY_HINT_RESOURCE_TYPE, "Mesh"), "set_custom_mesh", "get_custom_mesh");
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "enabled"), "set_enabled", "is_enabled");
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "double_sided"), "set_double_sided", "is_double_sided");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "max_recursion_depth", PROPERTY_HINT_RANGE, "-1,255,1,exp"), "set_max_recursion_depth", "get_max_recursion_depth");

	ADD_GROUP("Physics", "");
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "physics_enabled"), "set_physics_enabled", "is_physics_enabled");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "collision_ghost_mode", PROPERTY_HINT_ENUM, "None,Ghost,Ghost Slice"), "set_collision_ghost_mode", "get_collision_ghost_mode");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "collision_layer", PROPERTY_HINT_LAYERS_3D_PHYSICS), "set_collision_layer", "get_collision_layer");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "teleport_mask", PROPERTY_HINT_LAYERS_3D_PHYSICS), "set_teleport_mask", "get_teleport_mask");

	ADD_GROUP("Rendering", "");
	ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "custom_environment", PROPERTY_HINT_RESOURCE_TYPE, "Environment"), "set_custom_environment", "get_custom_environment");
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "teleport_light"), "set_teleport_light", "teleports_light");
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "teleport_gi"), "set_teleport_gi", "teleports_gi");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "mesh_ghost_mode", PROPERTY_HINT_ENUM, "None,Ghost,Ghost Slice"), "set_mesh_ghost_mode", "get_mesh_ghost_mode");

	ADD_SUBGROUP("Cull Mask", "");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "cull_mask_operator", PROPERTY_HINT_ENUM, "Keep,Replace,Bitwise AND,Bitwise OR,Bitwise XOR"), "set_cull_mask_operator", "get_cull_mask_operator");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "cull_mask", PROPERTY_HINT_LAYERS_3D_RENDER), "set_cull_mask", "get_cull_mask");

	ADD_GROUP("Audio", "");
	// ADD_PROPERTY(PropertyInfo(Variant::BOOL, "scaling_enabled"), "set_scaling_enabled", "is_scaling_enabled");
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "teleport_audio"), "set_teleport_audio", "teleports_audio");

	/* Enums */

	BIND_ENUM_CONSTANT(COLLISION_GHOST_NONE);
	BIND_ENUM_CONSTANT(COLLISION_GHOST);
	BIND_ENUM_CONSTANT(COLLISION_GHOST_SLICE);

	BIND_ENUM_CONSTANT(MESH_GHOST_NONE);
	BIND_ENUM_CONSTANT(MESH_GHOST);
	BIND_ENUM_CONSTANT(MESH_GHOST_SLICE);

	BIND_ENUM_CONSTANT(CULL_MASK_OP_KEEP);
	BIND_ENUM_CONSTANT(CULL_MASK_OP_REPLACE);
	BIND_ENUM_CONSTANT(CULL_MASK_OP_AND);
	BIND_ENUM_CONSTANT(CULL_MASK_OP_OR);
	BIND_ENUM_CONSTANT(CULL_MASK_OP_XOR);

	BIND_CONSTANT(NOTIFICATION_PORTAL_STATE_CHANGED);

	/* Signals */

	ADD_SIGNAL(MethodInfo("node_teleported_from", PropertyInfo(Variant::OBJECT, "node", PROPERTY_HINT_RESOURCE_TYPE, "Node3D")));
	ADD_SIGNAL(MethodInfo("node_teleported_to", PropertyInfo(Variant::OBJECT, "node", PROPERTY_HINT_RESOURCE_TYPE, "Node3D")));
	
	ADD_SIGNAL(MethodInfo("body_shape_entered", PropertyInfo(Variant::RID, "body_rid"), PropertyInfo(Variant::OBJECT, "body", PROPERTY_HINT_RESOURCE_TYPE, "PhysicsBody3D"), PropertyInfo(Variant::INT, "body_shape_index")));
	ADD_SIGNAL(MethodInfo("body_shape_exited", PropertyInfo(Variant::RID, "body_rid"), PropertyInfo(Variant::OBJECT, "body", PROPERTY_HINT_RESOURCE_TYPE, "PhysicsBody3D"), PropertyInfo(Variant::INT, "body_shape_index")));
	ADD_SIGNAL(MethodInfo("body_entered", PropertyInfo(Variant::OBJECT, "body", PROPERTY_HINT_RESOURCE_TYPE, "PhysicsBody3D")));
	ADD_SIGNAL(MethodInfo("body_exited", PropertyInfo(Variant::OBJECT, "body", PROPERTY_HINT_RESOURCE_TYPE, "PhysicsBody3D")));
}

void Portal3D::_notification(int p_what) {
	switch (p_what) {
		case NOTIFICATION_POST_ENTER_TREE: {
			_update_portal();
		} break;

		case NOTIFICATION_EXIT_TREE: {
			_update_portal();
		} break;

		case NOTIFICATION_ENTER_WORLD: {
			if (physics_rid.is_valid()) {
				PhysicsServer3D::get_singleton()->portal_set_space(physics_rid, get_world_3d()->get_space());
				PhysicsServer3D::get_singleton()->portal_set_transform(physics_rid, get_global_transform());
			}
		} break;

		case NOTIFICATION_EXIT_WORLD: {
			if (physics_rid.is_valid()) {
				PhysicsServer3D::get_singleton()->portal_set_space(physics_rid, RID());
			}
		} break;

		case NOTIFICATION_TRANSFORM_CHANGED: {

			if (partner) {
				_update_teleport_transform();
				partner->_update_teleport_transform();
				if (partner->rendering_rid.is_valid()) {
					RenderingServer::get_singleton()->portal_set_destination_transform(partner->rendering_rid, get_global_transform());
				}
			}

			if (physics_rid.is_valid()) {
				PhysicsServer3D::get_singleton()->portal_set_transform(physics_rid, get_global_transform());
			}

		} break;

	}
}

void Portal3D::_update_portal(bool p_update_partner) {
	Portal3D *old_partner = partner;

	if (is_inside_tree()) {
		partner = !partner_path.is_empty() ? Object::cast_to<Portal3D>(get_node_or_null(partner_path)) : nullptr;
		teleport_to = !teleport_path.is_empty() ? get_node_or_null(partner->teleport_path) : nullptr;
	}
	else {
		partner = nullptr;
		teleport_to = nullptr;
	}

	update_configuration_warnings();

	if (is_world_portal() && teleport_path.is_empty()) {
		WARN_PRINT("This portal teleports between worlds but `teleport_path` is unset. Nodes will NOT be teleported from the partner!");
	}

	if (old_partner && old_partner != partner && p_update_partner) {
		old_partner->_update_portal(false);
	}

	if (partner) {
		if (p_update_partner) {
			partner->_update_portal(false);
		}

		if (partner->partner != this) {
			if (partner->partner != nullptr) [[unlikely]] {
				WARN_PRINT("This portal's partner does not refer back to this portal. If changing partner from code, reset the partner's own partner path before changing path.");
			}
			partner = nullptr;
			_update_render_portal();
			return;
		}

		if (double_sided != partner->double_sided) [[unlikely]] {
			WARN_PRINT("`double_sided` is not configured the same for both this portal and its partner, unexpected behaviour may occur.");
		}

		if (old_partner != partner) {
			_update_teleport_transform();
		}
	}

	_update_render_portal();
	_update_physics_portal();
	update_gizmos();

	notification(NOTIFICATION_PORTAL_STATE_CHANGED);
}

void Portal3D::_update_render_portal() {
	if (is_open()) {
		_ensure_rendering_rid();
		partner->_ensure_rendering_rid();

		if (get_base() != rendering_rid) {
			set_base(rendering_rid);
		}

		RenderingServer::get_singleton()->portal_set_scenario_override(rendering_rid, is_world_portal() ? partner->get_world_3d()->get_scenario() : RID());
		RenderingServer::get_singleton()->portal_set_destination_transform(rendering_rid, partner->get_global_transform());
		RenderingServer::get_singleton()->portal_set_cull_partner(rendering_rid, partner->rendering_rid);
	}
	else if (rendering_rid.is_valid() && get_base() == rendering_rid) {
		set_base(RID());
	}
}

void Portal3D::_update_physics_portal() {
	_ensure_physics_rid();
	if (physics_rid.is_null()) {
		return;
	}

	if (partner) {
		partner->_ensure_physics_rid();
		PhysicsServer3D::get_singleton()->portal_set_partner(physics_rid, partner->physics_rid);
	} else {
		PhysicsServer3D::get_singleton()->portal_set_partner(physics_rid, RID());
	}

	PhysicsServer3D::get_singleton()->portal_set_disabled(physics_rid, !enabled);
}

void Portal3D::_ensure_rendering_rid() {
	if (rendering_rid.is_null()) {
		rendering_rid = RenderingServer::get_singleton()->portal_create();

		RenderingServer::get_singleton()->portal_set_double_sided(rendering_rid, double_sided);
		RenderingServer::get_singleton()->portal_set_teleport_light(rendering_rid, teleport_light);
		RenderingServer::get_singleton()->portal_set_teleport_gi(rendering_rid, teleport_gi);
		RenderingServer::get_singleton()->portal_set_mesh(rendering_rid, custom_mesh.is_valid() ? custom_mesh->get_rid() : RID());
		RenderingServer::get_singleton()->portal_set_environment_override(rendering_rid, custom_environment.is_valid() ? custom_environment->get_rid() : RID());
		RenderingServer::get_singleton()->portal_set_recursive_depth(rendering_rid, max_recursion_depth);

		_update_cull_mask();
	}
}

void Portal3D::_ensure_physics_rid() {
	if (physics_rid.is_null() && is_open()) {
		physics_rid = PhysicsServer3D::get_singleton()->portal_create();

		PhysicsServer3D::get_singleton()->portal_attach_object_instance_id(physics_rid, get_instance_id());
		PhysicsServer3D::get_singleton()->portal_set_space(physics_rid, get_world_3d()->get_space());
		PhysicsServer3D::get_singleton()->portal_set_transform(physics_rid, get_global_transform());

		PhysicsServer3D::get_singleton()->portal_set_ghost_mode(physics_rid, (PhysicsServer3D::PortalGhostMode) collision_ghost_mode);
		PhysicsServer3D::get_singleton()->portal_set_collision_layer(physics_rid, collision_layer);
		PhysicsServer3D::get_singleton()->portal_set_teleport_mask(physics_rid, teleport_mask);

		PhysicsServer3D::get_singleton()->portal_set_shape_type(physics_rid, (PhysicsServer3D::PortalShapeType) collision_shape);
		PhysicsServer3D::get_singleton()->portal_set_shape_data(physics_rid, collision_shape_data);

		// TODO: callbacks
	}
}

void _physics_monitor_callback() {

}

void _physics_teleport_callback(RID p_body) {
	ObjectID instance_id = PhysicsServer3D::get_singleton()->body_get_object_instance_id(p_body);
	Object *obj = ObjectDB::get_instance(instance_id);
	PhysicsBody3D *physics_body = Object::cast_to<PhysicsBody3D>(obj);
	// TODO
}

void Portal3D::_update_teleport_transform() {
	Transform3D this_transform = get_global_transform();
	Transform3D partner_transform = partner->get_global_transform();

	/*
	if (!scaling_enabled) {
		Vector3 this_scale = this_transform.get_basis().get_scale_abs();
		Vector3 partner_scale = partner_transform.get_basis().get_scale_abs();
		if (!this_scale.is_equal_approx(partner_scale)) [[unlikely]] {
			WARN_PRINT("Scaling is disabled, but this portal and its partner's scales are not equivalent. Please make the scales equivalent, or turn on scaling.");
		}

		this_transform.orthonormalize();
		partner_transform.orthonormalize();
	}
	*/

	this_transform = this_transform.affine_inverse();
	teleport_xform = partner_transform * this_transform;
	if (!double_sided) {
		teleport_xform.basis.rotate_local(Vector3::UP, Math::PI);
	}
}

void Portal3D::_update_cull_mask() {
	RenderingServer::PortalCullMaskOp op;
	switch (cull_mask_op) {
		case CULL_MASK_OP_KEEP: op = RenderingServer::PORTAL_CULL_MASK_OP_KEEP; break;
		case CULL_MASK_OP_REPLACE: op = RenderingServer::PORTAL_CULL_MASK_OP_REPLACE; break;
		case CULL_MASK_OP_AND: op = RenderingServer::PORTAL_CULL_MASK_OP_AND; break;
		case CULL_MASK_OP_OR: op = RenderingServer::PORTAL_CULL_MASK_OP_OR; break;
		case CULL_MASK_OP_XOR: op = RenderingServer::PORTAL_CULL_MASK_OP_XOR; break;
		default: op = RenderingServer::PORTAL_CULL_MASK_OP_KEEP; break;
	}
	RenderingServer::get_singleton()->portal_set_cull_mask(rendering_rid, op, cull_mask);
}

void Portal3D::_teleport_node(Node3D &p_node, bool p_physics_handled) {
	ERR_FAIL_COND(is_world_portal() && partner->teleport_path.is_empty());
	ERR_FAIL_COND(!partner->teleport_path.is_empty() && partner->teleport_to == nullptr);

	if (!is_inside_world() || !p_node.is_inside_world() || p_node.get_world_3d() != get_world_3d()) [[unlikely]] {
		WARN_PRINT("`Node3D` is not in the same `World3D` as `Portal3D`!");
	}

	if (!p_physics_handled) {
		Transform3D transform = p_node.get_global_transform();

		if (partner->teleport_to) {
			p_node.reparent(partner->teleport_to, false);
		}

		transform = teleport_xform * transform;
		p_node.set_global_transform(transform);
	}
	else if (partner->teleport_to) {
		p_node.reparent(partner->teleport_to, true);
	}
	
	emit_signal(SNAME("node_teleported_from"), &p_node);
	partner->emit_signal(SNAME("node_teleported_to"), &p_node);
}

////

/* Setters */

void Portal3D::set_partner_path(const NodePath &p_path) {
	if (partner_path == p_path) {
		return;
	}

	partner_path = p_path;
	if (partner != nullptr) {
		// Ensure our old partner knows
		partner->_update_portal(false);
	}

	_update_portal();
}

void Portal3D::set_teleport_path(const NodePath &p_path) {
	if (teleport_path == p_path) {
		return;
	}

	_update_portal(false);
}

void Portal3D::set_custom_environment(const Ref<Environment> &p_environment) {
	if (custom_environment == p_environment) {
		return;
	}

	custom_environment = p_environment;
	if (rendering_rid.is_valid()) {
		RenderingServer::get_singleton()->portal_set_environment_override(rendering_rid, custom_environment.is_valid() ? custom_environment->get_rid() : RID());
	}
}

void Portal3D::set_custom_mesh(const Ref<Mesh> &p_mesh) {
	if (custom_mesh == p_mesh) {
		return;
	}

	custom_mesh = p_mesh;
	if (rendering_rid.is_valid()) {
		RenderingServer::get_singleton()->portal_set_mesh(rendering_rid, custom_mesh.is_valid() ? custom_mesh->get_rid() : RID());
	}
}

void Portal3D::set_enabled(bool p_enabled) {
	if (enabled == p_enabled) {
		return;
	}

	enabled = p_enabled;
	_update_portal();
}

void Portal3D::set_double_sided(bool p_enabled) {
	if (double_sided == p_enabled) {
		return;
	}

	double_sided = p_enabled;
	if (rendering_rid.is_valid()) {
		RenderingServer::get_singleton()->portal_set_double_sided(rendering_rid, double_sided);
	}
}

void Portal3D::set_physics_enabled(bool p_enabled) {
	if (physics_enabled == p_enabled) {
		return;
	}

	physics_enabled = p_enabled;
	_update_portal();
}

void Portal3D::set_teleport_audio(bool p_enabled) {
	if (teleport_audio == p_enabled) {
		return;
	}

	teleport_audio = p_enabled;
	// TODO: audio stuff
}

void Portal3D::set_teleport_light(bool p_enabled) {
	if (teleport_light == p_enabled) {
		return;
	}

	teleport_light = p_enabled;
	if (rendering_rid.is_valid()) {
		RenderingServer::get_singleton()->portal_set_teleport_light(rendering_rid, teleport_light);
	}
}

void Portal3D::set_teleport_gi(bool p_enabled) {
	if (teleport_gi == p_enabled) {
		return;
	}

	teleport_gi = p_enabled;
	if (rendering_rid.is_valid()) {
		RenderingServer::get_singleton()->portal_set_teleport_gi(rendering_rid, teleport_gi);
	}
}

void Portal3D::set_collision_ghost_mode(CollisionGhostMode p_mode) {
	if (collision_ghost_mode == p_mode) {
		return;
	}

	collision_ghost_mode = p_mode;
	// TODO
}

void Portal3D::set_mesh_ghost_mode(MeshGhostMode p_mode) {
	if (mesh_ghost_mode == p_mode) {
		return;
	}

	mesh_ghost_mode = p_mode;
	// TODO
}

void Portal3D::set_cull_mask_op(CullMaskOp p_op) {
	if (cull_mask_op == p_op) {
		return;
	}

	cull_mask_op = p_op;
	if (rendering_rid.is_valid()) {
		_update_cull_mask();
	}
}

void Portal3D::set_cull_mask(uint32_t p_mask) {
	if (cull_mask == p_mask) {
		return;
	}

	cull_mask = p_mask;
	if (rendering_rid.is_valid() && cull_mask_op != CULL_MASK_OP_KEEP) {
		_update_cull_mask();
	}
}

void Portal3D::set_collision_layer(uint32_t p_collision_layer) {
	if (collision_layer == p_collision_layer) {
		return;
	}

	collision_layer = p_collision_layer;
	// TODO
}

void Portal3D::set_teleport_mask(uint32_t p_teleport_mask) {
	if (teleport_mask == p_teleport_mask) {
		return;
	}

	teleport_mask = p_teleport_mask;
	// TODO
}

void Portal3D::set_max_recursion_depth(int p_depth) {
	if (max_recursion_depth == p_depth) {
		return;
	}

	max_recursion_depth = p_depth;
	if (rendering_rid.is_valid()) {
		RenderingServer::get_singleton()->portal_set_recursive_depth(rendering_rid, max_recursion_depth);
	}
}

/* Getters */

NodePath Portal3D::get_partner_path() const {
	return partner_path;
}

NodePath Portal3D::get_teleport_path() const {
	return teleport_path;
}

Ref<Environment> Portal3D::get_custom_environment() const {
	return custom_environment;
}

Ref<Mesh> Portal3D::get_custom_mesh() const {
	return custom_mesh;
}

bool Portal3D::is_enabled() const {
	return enabled;
}

bool Portal3D::is_double_sided() const {
	return double_sided;
}

bool Portal3D::is_physics_enabled() const {
	return physics_enabled;
}

bool Portal3D::teleports_audio() const {
	return teleport_audio;
}

bool Portal3D::teleports_light() const {
	return teleport_light;
}

bool Portal3D::teleports_gi() const {
	return teleport_gi;
}

Portal3D::CollisionGhostMode Portal3D::get_collision_ghost_mode() const {
	return collision_ghost_mode;
}

Portal3D::MeshGhostMode Portal3D::get_mesh_ghost_mode() const {
	return mesh_ghost_mode;
}

Portal3D::CullMaskOp Portal3D::get_cull_mask_op() const {
	return cull_mask_op;
}

uint32_t Portal3D::get_cull_mask() const {
	return cull_mask;
}

uint32_t Portal3D::get_collision_layer() const {
	return collision_layer;
}

uint32_t Portal3D::get_teleport_mask() const {
	return teleport_mask;
}

int Portal3D::get_max_recursion_depth() const {
	return max_recursion_depth;
}

/* Utility */

Portal3D *Portal3D::get_partner() const {
	return partner;
}

Transform3D Portal3D::teleport_transform(const Transform3D &p_transform) const {
	ERR_FAIL_COND_V(partner == nullptr, p_transform);
	return teleport_xform * p_transform;
}

Vector3 Portal3D::teleport_point(const Vector3 &p_point) const {
	ERR_FAIL_COND_V(partner == nullptr, p_point);
	return teleport_xform.xform(p_point);
}

Vector3 Portal3D::teleport_direction(const Vector3 &p_direction) const {
	ERR_FAIL_COND_V(partner == nullptr, p_direction);
	return teleport_xform.basis.xform(p_direction);
}

void Portal3D::teleport_node(Node3D *p_node) {
	ERR_FAIL_COND(p_node == nullptr);
	ERR_FAIL_COND(partner == nullptr);
	ERR_FAIL_COND(!p_node->is_inside_tree());
	_teleport_node(*p_node);
}

/* Other */

PackedStringArray Portal3D::get_configuration_warnings() const {
	PackedStringArray warnings = Node3D::get_configuration_warnings();

	if (partner && partner->partner != this) {
		warnings.push_back(RTR("This portal has a partner that does not refer to this portal.\nPlease ensure both portals have each other as a partner."));
	}

	if (is_world_portal() && teleport_path.is_empty()) {
		warnings.push_back(RTR("This portal teleports between worlds, but does not set a path for teleport nodes to end up.\nPlease set a path for `teleport_path` for nodes to teleport to when teleporting from the partner."));
	}

	return warnings;
}