#pragma once

#include "servers/physics_3d/physics_server_3d.h"
#include "modules/jolt_physics/objects/jolt_physics_direct_body_state_3d.h"
#include "modules/jolt_physics/objects/jolt_object_3d.h"
#include "modules/jolt_physics/spaces/jolt_space_3d.h"

class JoltBodyGhost3D;

class JoltPortal3D final : public JoltObject3D {
public:
	typedef PhysicsServer3D::PortalShapeType ShapeType;
	typedef PhysicsServer3D::PortalGhostMode GhostMode;

protected:
	struct BodyIDHasher {
		static uint32_t hash(const JPH::BodyID &p_id) { return hash_fmix32(p_id.GetIndexAndSequenceNumber()); }
	};

	struct ShapeOverlap {
		JPH::SubShapeID id;
		bool origin_in_portal = false;
	};

	struct Overlap {
		LocalVector<ShapeOverlap> shapes;
		RID rid;
		ObjectID instance_id;
		JoltBodyGhost3D *ghost = nullptr;
	};

	SelfList<JoltPortal3D> call_queries_element;

	// This includes only bodies that are currently on the same side as the portal.
	// For example, when bodies teleport, overlap data is transferred to the partner.
	HashMap<JPH::BodyID, Overlap, BodyIDHasher> overlapping_bodies;

	// List of bodies we are teleporting on a given step
	LocalVector<JoltBody3D *> body_teleport_list;

	uint32_t teleport_mask = UINT32_MAX;
	JoltPortal3D *partner = nullptr;

	ShapeType shape_type = ShapeType::PORTAL_SHAPE_RECTANGLE;
	GhostMode ghost_mode = GhostMode::PORTAL_GHOST;

	Vector2 size = Vector2(1, 1);
	Variant shape_data;

	JPH::ShapeRefC jolt_shape;

	mutable Transform3D teleport_transform;
	mutable bool teleport_transform_dirty = true;

	bool double_sided = false;

	Callable monitor_callback;
	Callable teleport_callback;

	virtual JPH::BroadPhaseLayer _get_broad_phase_layer() const override;
	virtual JPH::ObjectLayer _get_object_layer() const override;

	virtual void _add_to_space() override {}

	virtual void _collision_layer_changed() override {}
	virtual void _collision_mask_changed() override {}

	void _partner_changed();
	void _shape_updated();

	void _update_teleport_transform() const;

	bool _should_call_queries() const { return monitor_callback.is_valid() || teleport_callback.is_valid(); }
	void _enqueue_call_queries();
	void _dequeue_call_queries();

public:
	JoltPortal3D();
	virtual ~JoltPortal3D() override {}

	JoltPortal3D *get_partner() const { return partner; }
	void set_partner(JoltPortal3D *p_partner);

	uint32_t get_teleport_mask() const { return teleport_mask; }
	void set_teleport_mask(uint32_t p_mask);

	ShapeType get_shape_type() const { return shape_type; }
	void set_shape_type(ShapeType p_type);

	GhostMode get_ghost_mode() const { return ghost_mode; }
	void set_ghost_mode(GhostMode p_mode);

	Variant get_shape_data() const { return shape_data; }
	void set_shape_data(const Variant &p_data);

	Callable get_monitor_callback() const { return monitor_callback; }
	void set_monitor_callback(const Callable &p_callable);

	Callable get_teleport_callback() const { return teleport_callback; }
	void set_teleport_callback(const Callable &p_callable);

	Basis get_basis() const;
	Vector3 get_position() const;
	AABB get_aabb() const;

	void set_transform(Transform3D p_transform);
	Transform3D get_transform() const;

	void teleport_body(JoltBody3D &p_body);
	
	void call_queries();

	////

	const JPH::Shape *get_jolt_shape() const { return jolt_shape; }

	// Eventually we will handle velocity for when we are attached to a body
	virtual Vector3 get_velocity_at_position(const Vector3 &p_position) const override { return Vector3(); }

	virtual bool can_interact_with(const JoltBody3D &p_other) const override;
	virtual bool can_interact_with(const JoltSoftBody3D &p_other) const override { return false; }
	virtual bool can_interact_with(const JoltArea3D &p_other) const override { return false; }

	bool can_teleport(const JoltObject3D &p_other) const {
		return (teleport_mask & p_other.get_collision_layer()) != 0;
	}

	const Transform3D &get_teleport_transform() const {
		if (teleport_transform_dirty) {
			_update_teleport_transform();
		}
		return teleport_transform;
	}
};