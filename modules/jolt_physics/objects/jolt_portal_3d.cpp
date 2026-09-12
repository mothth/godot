#include "jolt_portal_3d.h"

#include "jolt_body_3d.h"
#include "../jolt_project_settings.h"
#include "../misc/jolt_math_funcs.h"
#include "../misc/jolt_type_conversions.h"
#include "../shapes/jolt_shape_3d.h"
#include "../spaces/jolt_broad_phase_layer.h"
#include "../spaces/jolt_space_3d.h"
#include "jolt_group_filter.h"
#include "jolt_physics_direct_body_state_3d.h"

JoltPortal3D::JoltPortal3D() : JoltObject3D(OBJECT_TYPE_PORTAL), call_queries_element(this) {}

JPH::BroadPhaseLayer JoltPortal3D::_get_broad_phase_layer() const {
	return JoltBroadPhaseLayer::PORTAL;
}

JPH::ObjectLayer JoltPortal3D::_get_object_layer() const {
	ERR_FAIL_NULL_V(space, 0);

	if (jolt_shape == nullptr || jolt_shape->GetType() == JPH::EShapeType::Empty) {
		// No point doing collision checks against a shapeless object.
		return space->map_to_object_layer(_get_broad_phase_layer(), 0, 0);
	}

	return space->map_to_object_layer(_get_broad_phase_layer(), collision_layer, collision_mask);
}

void JoltPortal3D::_partner_changed() {

}

void JoltPortal3D::_shape_updated() {

}

void JoltPortal3D::_update_teleport_transform() const {
	ERR_FAIL_NULL(partner);

	Transform3D this_transform = get_transform();
	Transform3D partner_transform = partner->get_transform();

	this_transform = this_transform.affine_inverse();
	teleport_transform = partner_transform * this_transform;
	if (!double_sided) {
		teleport_transform.basis.rotate_local(Vector3::UP, Math::PI);
	}

	teleport_transform_dirty = false;
}

void JoltPortal3D::_enqueue_call_queries() {
	if (space != nullptr) {
		space->enqueue_call_queries(&call_queries_element);
	}
}

void JoltPortal3D::_dequeue_call_queries() {
	if (space != nullptr) {
		space->dequeue_call_queries(&call_queries_element);
	}
}

////

void JoltPortal3D::set_partner(JoltPortal3D *p_partner) {
	if (partner != p_partner) {
		if (partner) {
			partner->partner = nullptr;
			partner->_partner_changed();
		}

		partner = p_partner;
		_partner_changed();

		if (p_partner) {
			p_partner->partner = this;
			p_partner->_partner_changed();
		}
	}
}

void JoltPortal3D::set_teleport_mask(uint32_t p_mask) {
	teleport_mask = p_mask;
}

void JoltPortal3D::set_shape_type(ShapeType p_type) {
	if (shape_type != p_type) {
		shape_type = p_type;
		_shape_updated();
	}
}

void JoltPortal3D::set_ghost_mode(GhostMode p_mode) {
	if (ghost_mode != p_mode) {
		ghost_mode = p_mode;
	}
}

void JoltPortal3D::set_shape_data(const Variant &p_data) {
	if (shape_data != p_data) {
		shape_data = p_data;
		_shape_updated();
	}
}

void JoltPortal3D::set_monitor_callback(const Callable &p_callable) {
	monitor_callback = p_callable;
}

void JoltPortal3D::set_teleport_callback(const Callable &p_callable) {
	teleport_callback = p_callable;
}

void JoltPortal3D::set_transform(Transform3D p_transform) {
	teleport_transform_dirty = true;
	// TODO
}

Transform3D JoltPortal3D::get_transform() const {
	// TODO
	return Transform3D();
}

bool JoltPortal3D::can_interact_with(const JoltBody3D &p_other) const {
	return p_other.can_interact_with(*this);
}

void JoltPortal3D::teleport_body(JoltBody3D &p_body) {
	if (teleport_transform_dirty) {
		_update_teleport_transform();
	}

	p_body.teleport(teleport_transform);
	body_teleport_list.push_back(&p_body);

	if (_should_call_queries()) {
		_enqueue_call_queries();
	}
}

void JoltPortal3D::call_queries() {
	for (JoltBody3D *body : body_teleport_list) {
		const Variant body_rid = body->get_rid();
		const Variant *args[1] = { &body_rid };

		Callable::CallError ce;
		Variant ret;
		teleport_callback.callp(args, 1, ret, ce);

		if (unlikely(ce.error != Callable::CallError::CALL_OK)) {
			ERR_PRINT_ONCE(vformat("Failed to call teleport callback for '%s'. It returned the following error: '%s'.", to_string(), Variant::get_callable_error_text(teleport_callback, args, 1, ce)));
		}
	}

	body_teleport_list.clear();
}