#pragma once

#include "servers/rendering/renderer_rd/pipeline_cache_rd.h"
// #include "servers/rendering/renderer_rd/storage_rd/render_scene_buffers_rd.h"
#include "servers/rendering/renderer_rd/shaders/portal.glsl.gen.h"
#include "servers/rendering/renderer_rd/layout_rules.h"
#include "servers/rendering/renderer_rd/storage_rd/portal_storage.h"

namespace RendererRD {

class PortalRender {
private:

	enum {
		MODE_DEPTH_STENCIL,
		MODE_CLEAR_DEPTH,
		MODE_MAP_DEPTH,
		NUM_SHADER_MODES
	};

	enum {
		PIPELINE_DEPTH,
		PIPELINE_STENCIL,
		PIPELINE_CLEAR_DEPTH,
		PIPELINE_MAP_DEPTH,

		NUM_PIPELINES,
	};

	struct PushConstant {
		std430_mat4 model_matrix;
	};

	struct MapDepthPushConstant : PushConstant {
		std430_mat4 target_projection;
	};

	PortalShaderRD shader;
	RID shader_version;

	PipelineCacheRD pipelines[NUM_PIPELINES];

	mutable int current_pipeline = -1;
	mutable int framebuffer_format = -1;
	mutable uint32_t reference_cache = RD::STENCIL_MASK_DEFAULT;
	mutable uint32_t write_mask_cache = RD::STENCIL_MASK_DEFAULT;

	mutable RID vertex_array_cache;
	mutable RID index_array_cache;

	const int uniform_set_binding = 1;	

	_FORCE_INLINE_ RD::DrawListID _start_draw(RID p_framebuffer);
	_FORCE_INLINE_ void _bind_pipeline(RD::DrawListID p_draw_list, int p_pipeline);
	_FORCE_INLINE_ void _draw_portal_common(RD::DrawListID p_draw_list, const PortalRenderInfo &p_portal);

	void _draw_portal_depth(RD::DrawListID p_draw_list, const PortalRenderInfo &p_portal);
	void _draw_portal_stencil(RD::DrawListID p_draw_list, const PortalRenderInfo &p_portal);
	void _clear_portal_depth(RD::DrawListID p_draw_list, const PortalRenderInfo &p_portal, bool p_revert_stencil = false);

	RID _get_uniform_set(RID p_scene_data, RID p_depth_texture = RID());

public:

	PortalRender();
	~PortalRender();
	
	void portal_depth_prepass(Span<PortalRenderInfo> p_portals, int p_parent_index, RID p_scene_data, RID p_depth_framebuffer);
	void portal_transparent_resolve(Span<PortalRenderInfo> p_portals, int p_parent_index, RID p_scene_data, RID p_depth_framebuffer);
};
	
};