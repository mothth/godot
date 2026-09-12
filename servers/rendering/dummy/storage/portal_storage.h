#pragma once

#include "servers/rendering/storage/portal_storage.h"

namespace RendererDummy {

class PortalStorage : public RendererPortalStorage {
public:
	virtual ~PortalStorage() override {}

	/* PORTAL API */

	virtual RID portal_allocate() override { return RID(); }
	virtual void portal_initialize(RID p_rid) override {}
	
	virtual void portal_set_double_sided(RID p_portal, bool p_double_sided) override {}
	virtual void portal_set_teleport_light(RID p_portal, bool p_teleport_light) override {}
	virtual void portal_set_teleport_gi(RID p_portal, bool p_teleport_gi) override {}
	virtual void portal_set_mesh(RID p_portal, RID p_mesh) override {}
	virtual void portal_set_destination_transform(RID p_portal, const Transform3D &p_transform) override {}
	virtual void portal_set_scenario_override(RID p_portal, RID p_scenario) override {}
	virtual void portal_set_environment_override(RID p_portal, RID p_environment) override {}
	virtual void portal_set_recursive_depth(RID p_portal, int p_depth) override {}
	virtual void portal_set_cull_mask(RID p_portal, RenderingServer::PortalCullMaskOp p_op, uint32_t p_mask = 0xFFFFFFFF) override {}
	virtual void portal_set_cull_partner(RID p_portal, RID p_partner) override {}

	virtual bool portal_is_local_to_environment(RID p_portal) const override { return false; }
	virtual int portal_get_max_recursive_depth(RID p_portal) const override { return 0; }
	virtual AABB portal_get_aabb(RID p_portal) const override { return AABB(); }

	virtual Projection portal_get_oblique_projection(RID p_portal, const Projection &p_cam_projection, const Transform3D &p_cam_transform) const override { return Projection(); }

	/* INSTANCE API */

	virtual RID portal_instance_create(RID p_portal) override { return RID(); }
	virtual void portal_instance_free(RID p_instance) override {}
	virtual int get_instance_count() const override { return 0; }
	
	virtual void portal_instance_set_transform(RID p_instance, const Transform3D &p_transform) override {}
	virtual void portal_instance_set_aabb(RID p_instance, const AABB &p_aabb) override {}

	virtual RID portal_instance_get_base_portal(RID p_instance) const override { return RID(); }
	virtual RID portal_instance_get_owner(RID p_instance) const override { return RID(); }

	virtual PortalRenderInfo portal_instance_make_render_info(RID p_instance, const Transform3D &p_cam_transform, const Projection &p_cam_projection) const override { return PortalRenderInfo(); }
	virtual LocalVector<RID> get_instances() const override { return LocalVector<RID>(); }
};

}