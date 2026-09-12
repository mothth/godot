#pragma once

#include "servers/rendering/storage/portal_storage.h"
#include "servers/rendering/storage/utilities.h"

namespace RendererRD {

class PortalStorage : public RendererPortalStorage {
private:
	static PortalStorage *singleton;
	
	// This is just simple fast thread-locking for RID access caches.
	mutable bool cache_lock = false;

	/* PORTAL */

	struct Portal {
		int max_depth = INT_MAX;
		bool double_sided = false;
		bool teleport_light = false;
		bool teleport_gi = false;

		RenderingServer::PortalCullMaskOp cull_mask_op;
		uint32_t cull_mask = 0xFFFFFFFF;
		
		Transform3D destination;
		
		RID mesh;
		RID env_override;
		RID scenario_override;
		RID cull_partner;

		Dependency dependency;
	};

	mutable RID_Owner<Portal, true> portal_owner = RID_Owner<Portal, true>(8192, 4096);
	mutable RID last_portal_rid;
	mutable Portal *last_portal = nullptr;

	Portal *portal_get_or_null(RID p_portal) const {
		if (cache_lock) {
			return portal_owner.get_or_null(p_portal);
		}
		cache_lock = true;
		if (p_portal != last_portal_rid) { 
			last_portal = portal_owner.get_or_null(p_portal);
			last_portal_rid = p_portal;
		}
		Portal *result = last_portal;
		cache_lock = false;
		return result;
	}

	void portal_free(RID p_rid);

	/* PORTAL INSTANCE */

	struct PortalInstance {
		Transform3D transform;
		Transform3D inv_transform;
		AABB aabb;
		RID portal;
		RID owner;

		PortalInstance(RID p_portal) : portal(p_portal) {}
	};

	mutable RID_Owner<PortalInstance> instance_owner = RID_Owner<PortalInstance>(8192, 8192);
	mutable RID last_instance_rid;
	mutable PortalInstance *last_instance = nullptr;

	PortalInstance *portal_instance_get_or_null(RID p_instance) const {
		if (cache_lock) {
			return instance_owner.get_or_null(p_instance);
		}
		cache_lock = true;
		if (p_instance != last_instance_rid) { 
			last_instance_rid = p_instance;
			last_instance = instance_owner.get_or_null(p_instance);
		}
		PortalInstance *result = last_instance;
		cache_lock = false;
		return result;
	}
	
	/* BUFFERS */

	// Default portal mesh buffers
	RID default_vertex_buffer;	
	RID default_index_buffer;
	RID default_vertex_array;
	RID default_index_array;

	mutable RD::VertexFormatID vertex_format = -1;

	void _make_vertex_format() const;
	
public:
	static PortalStorage *get_singleton() { return singleton; }

	PortalStorage();
	virtual ~PortalStorage() override;

	bool free(RID p_rid);
	bool owns_portal(RID p_rid) { return portal_owner.owns(p_rid); }

	/* PORTAL API */

	virtual RID portal_allocate() override;
	virtual void portal_initialize(RID p_rid) override;
	
	virtual void portal_set_double_sided(RID p_portal, bool p_double_sided) override;
	virtual void portal_set_teleport_light(RID p_portal, bool p_teleport_light) override;
	virtual void portal_set_teleport_gi(RID p_portal, bool p_teleport_gi) override;
	virtual void portal_set_mesh(RID p_portal, RID p_mesh) override;
	virtual void portal_set_destination_transform(RID p_portal, const Transform3D &p_transform) override;
	virtual void portal_set_scenario_override(RID p_portal, RID p_scenario) override;
	virtual void portal_set_environment_override(RID p_portal, RID p_environment) override;
	virtual void portal_set_recursive_depth(RID p_portal, int p_depth) override;
	virtual void portal_set_cull_mask(RID p_portal, RenderingServer::PortalCullMaskOp p_op, uint32_t p_mask = 0xFFFFFFFF) override;
	virtual void portal_set_cull_partner(RID p_portal, RID p_partner) override;

	virtual bool portal_is_local_to_environment(RID p_portal) const override;
	virtual int portal_get_max_recursive_depth(RID p_portal) const override;
	virtual AABB portal_get_aabb(RID p_portal) const override;

	virtual Projection portal_get_oblique_projection(RID p_portal, const Projection &p_cam_projection, const Transform3D &p_cam_transform) const override;

	void portal_get_rd_arrays(RID p_portal, RID &p_vertex_array, RID &p_index_array) const;

	/* INSTANCE API */

	virtual RID portal_instance_create(RID p_portal) override;
	virtual void portal_instance_free(RID p_instance) override;

	virtual void portal_instance_set_transform(RID p_instance, const Transform3D &p_transform) override;
	virtual void portal_instance_set_aabb(RID p_instance, const AABB &p_aabb) override;

	virtual RID portal_instance_get_base_portal(RID p_instance) const override;
	virtual RID portal_instance_get_owner(RID p_instance) const override;

	virtual PortalRenderInfo portal_instance_make_render_info(RID p_instance, const Transform3D &p_cam_transform, const Projection &p_cam_projection) const override;
	
	virtual LocalVector<RID> get_instances() const override { return instance_owner.get_owned_list(); }

	/* OTHER */
	
	_FORCE_INLINE_ RD::VertexFormatID get_vertex_format() const {
		if (vertex_format < 0) {
			_make_vertex_format();
		}
		return vertex_format;
	}
};

}