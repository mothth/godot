#pragma once

#include "scene/3d/visual_instance_3d.h"

class Portal3D : public VisualInstance3D {
	GDCLASS(Portal3D, VisualInstance3D);

public:

	enum CollisionGhostMode {
		COLLISION_GHOST_NONE,
		COLLISION_GHOST,
		COLLISION_GHOST_SLICE
	};

	enum MeshGhostMode {
		MESH_GHOST_NONE,
		MESH_GHOST,
		MESH_GHOST_SLICE
	};

	enum CullMaskOp {
		CULL_MASK_OP_KEEP,
		CULL_MASK_OP_REPLACE,
		CULL_MASK_OP_AND,
		CULL_MASK_OP_OR,
		CULL_MASK_OP_XOR,
		NUM_CULL_MASK_OPS
	};

	enum CollisionShape {
		COLLISION_SHAPE_RECTANGLE, ///< vec3:"extents"
		COLLISION_SHAPE_CIRCLE, ///< float:"radius"
		COLLISION_SHAPE_CAPSULE,
		COLLISION_SHAPE_CONVEX_POLYGON, ///< array of planes:"planes"
		COLLISION_SHAPE_CONCAVE_POLYGON
	};

	enum {
		// Emitted when the state of the portal changes, either when the portal is enabled / disabled, or some property that causes an update is changed
		NOTIFICATION_PORTAL_STATE_CHANGED = 50
	};

protected:

	Portal3D *partner = nullptr;
	Node *teleport_to = nullptr;

	Transform3D teleport_xform;

	NodePath partner_path;
	NodePath teleport_path;
	Ref<Environment> custom_environment;
	Ref<Mesh> custom_mesh;
	
	bool enabled = true;
	bool double_sided = false;
	bool physics_enabled = true;
	bool solid = false;
	// bool scaling_enabled = false;
	bool teleport_audio = true;
	bool teleport_light = false;
	bool teleport_gi = false;

	int max_recursion_depth = -1;
	
	CollisionGhostMode collision_ghost_mode = COLLISION_GHOST;
	MeshGhostMode mesh_ghost_mode = MESH_GHOST;
	CullMaskOp cull_mask_op = CULL_MASK_OP_KEEP;
	CollisionShape collision_shape = COLLISION_SHAPE_RECTANGLE;

	uint32_t cull_mask = 0xFFFFF;
	uint32_t collision_layer = 1;
	uint32_t teleport_mask = UINT32_MAX;

	Variant collision_shape_data;

	RID rendering_rid;
	RID physics_rid;

	////

	static void _bind_methods();

	void _notification(int p_what);
	void _update_portal(bool p_update_partner = true);

	void _update_render_portal();
	void _update_physics_portal();
	void _ensure_rendering_rid();
	void _ensure_physics_rid();

	void _physics_monitor_callback();
	void _physics_teleport_callback(RID p_body);

	void _update_teleport_transform();
	void _update_cull_mask();

	void _teleport_node(Node3D &p_node, bool p_physics_handled = false);

public:
	/* Setters */

	void set_partner_path(const NodePath &p_path);
	void set_teleport_path(const NodePath &p_path);
	void set_custom_environment(const Ref<Environment> &p_environment);
	void set_custom_mesh(const Ref<Mesh> &p_mesh);

	void set_enabled(bool p_enabled = true);
	void set_double_sided(bool p_enabled = true);
	void set_physics_enabled(bool p_enabled = true);
	// void set_scaling_enabled(bool p_enabled = true);
	void set_teleport_audio(bool p_enabled = true);
	void set_teleport_light(bool p_enabled = true);
	void set_teleport_gi(bool p_enabled = true);

	void set_collision_ghost_mode(CollisionGhostMode p_mode);
	void set_mesh_ghost_mode(MeshGhostMode p_mode);
	void set_cull_mask_op(CullMaskOp p_op);

	void set_cull_mask(uint32_t p_mask);
	void set_collision_layer(uint32_t p_collision_layer);
	void set_teleport_mask(uint32_t p_teleport_mask);

	void set_max_recursion_depth(int p_depth);

	/* Getters */

	NodePath get_partner_path() const;
	NodePath get_teleport_path() const;
	Ref<Environment> get_custom_environment() const;
	Ref<Mesh> get_custom_mesh() const;

	bool is_enabled() const;
	bool is_double_sided() const;
	bool is_physics_enabled() const;
	// bool is_scaling_enabled() const;
	bool teleports_audio() const;
	bool teleports_light() const;
	bool teleports_gi() const;

	CollisionGhostMode get_collision_ghost_mode() const;
	MeshGhostMode get_mesh_ghost_mode() const;
	CullMaskOp get_cull_mask_op() const;

	uint32_t get_cull_mask() const;
	uint32_t get_collision_layer() const;
	uint32_t get_teleport_mask() const;

	int get_max_recursion_depth() const;

	/* Utility */

	Portal3D *get_partner() const;

	Transform3D teleport_transform(const Transform3D &p_transform) const;
	Vector3 teleport_point(const Vector3 &p_point) const;
	Vector3 teleport_direction(const Vector3 &p_direction) const;
	
	void teleport_node(Node3D *p_node);

	_FORCE_INLINE_ bool is_world_portal() const {
		return partner && is_inside_world() && partner->is_inside_world() && get_world_3d() != partner->get_world_3d();
	}

	_FORCE_INLINE_ bool is_open() const {
		return is_inside_world() && enabled && partner && partner->enabled;
	}

	/* Other */

	PackedStringArray get_configuration_warnings() const override;

	Portal3D();
	~Portal3D();
};

VARIANT_ENUM_CAST(Portal3D::CollisionGhostMode);
VARIANT_ENUM_CAST(Portal3D::MeshGhostMode);
VARIANT_ENUM_CAST(Portal3D::CullMaskOp);
