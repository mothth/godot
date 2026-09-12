#include "portal.h"

#include "servers/rendering/renderer_rd/uniform_set_cache_rd.h"
#include "servers/rendering/renderer_rd/storage_rd/material_storage.h"

using namespace RendererRD;

/*
#[versions]

depth_stencil = "#define VERSION_DEPTH_STENCIL";
clear_depth = "#define VERSION_CLEAR_DEPTH";
map_depth = "#define VERSION_MAP_DEPTH";
*/

PortalRender::PortalRender() {
	Vector<String> portal_modes;
	portal_modes.push_back("\n#define VERSION_DEPTH_STENCIL\n");
	portal_modes.push_back("\n#define VERSION_CLEAR_DEPTH\n");
	portal_modes.push_back("\n#define VERSION_MAP_DEPTH\n");

	Vector<uint64_t> dynamic_buffers;
	dynamic_buffers.push_back(ShaderRD::DynamicBuffer::encode(uniform_set_binding, 0));
	shader.initialize(portal_modes, "", Vector<RD::PipelineImmutableSampler>(), dynamic_buffers);
	shader_version = shader.version_create();

	/* Depth Pipeline */

	RD::PipelineDepthStencilState depth_stencil;
	depth_stencil.enable_depth_test = true;
	depth_stencil.enable_depth_write = true;
	depth_stencil.depth_compare_operator = RD::COMPARE_OP_GREATER_OR_EQUAL;
	depth_stencil.enable_stencil = true;
	depth_stencil.front_op.pass = RD::STENCIL_OP_KEEP;
	depth_stencil.front_op.fail = RD::STENCIL_OP_KEEP;
	depth_stencil.front_op.depth_fail = RD::STENCIL_OP_KEEP;
	depth_stencil.front_op.compare = RD::COMPARE_OP_EQUAL;
	depth_stencil.front_op.compare_mask = 0xFF;
	depth_stencil.front_op.write_mask = 0;
	depth_stencil.back_op = depth_stencil.front_op;

	pipelines[PIPELINE_DEPTH].setup(
		shader.version_get_shader(shader_version, MODE_DEPTH_STENCIL),
		RD::RENDER_PRIMITIVE_TRIANGLES,
		RD::PipelineRasterizationState(),
		RD::PipelineMultisampleState(),
		depth_stencil,
		RD::PipelineColorBlendState::create_disabled(),
		RD::DYNAMIC_STATE_STENCIL_REFERENCE
	);


	/* Stencil Pipeline */

	// We use invert operator and write mask to set our exact stencil value exactly,
	// since we are confining ourself to only draw on one stencil value anyway.

	depth_stencil = RD::PipelineDepthStencilState();
	depth_stencil.enable_depth_test = true;
	depth_stencil.enable_depth_write = false;
	depth_stencil.depth_compare_operator = RD::COMPARE_OP_GREATER_OR_EQUAL;
	depth_stencil.enable_stencil = true;
	depth_stencil.front_op.pass = RD::STENCIL_OP_INVERT;
	depth_stencil.front_op.fail = RD::STENCIL_OP_KEEP;
	depth_stencil.front_op.depth_fail = RD::STENCIL_OP_KEEP;
	depth_stencil.front_op.compare = RD::COMPARE_OP_EQUAL;
	depth_stencil.front_op.compare_mask = 0xFF;
	depth_stencil.front_op.write_mask = 0;
	depth_stencil.back_op = depth_stencil.front_op;

	pipelines[PIPELINE_STENCIL].setup(
		shader.version_get_shader(shader_version, MODE_DEPTH_STENCIL),
		RD::RENDER_PRIMITIVE_TRIANGLES,
		RD::PipelineRasterizationState(),
		RD::PipelineMultisampleState(),
		depth_stencil,
		RD::PipelineColorBlendState::create_disabled(),
		RD::DYNAMIC_STATE_STENCIL_REFERENCE | RD::DYNAMIC_STATE_STENCIL_WRITE_MASK
	);

	/* Clear Depth Pipeline */

	depth_stencil = RD::PipelineDepthStencilState();
	depth_stencil.enable_depth_test = true;
	depth_stencil.enable_depth_write = true;
	depth_stencil.depth_compare_operator = RD::COMPARE_OP_ALWAYS;
	depth_stencil.enable_stencil = true;
	depth_stencil.front_op.pass = RD::STENCIL_OP_INVERT;
	depth_stencil.front_op.fail = RD::STENCIL_OP_KEEP;
	depth_stencil.front_op.depth_fail = RD::STENCIL_OP_KEEP;
	depth_stencil.front_op.compare = RD::COMPARE_OP_EQUAL;
	depth_stencil.front_op.compare_mask = 0xFF;
	depth_stencil.front_op.write_mask = 0;
	depth_stencil.back_op = depth_stencil.front_op;

	pipelines[PIPELINE_CLEAR_DEPTH].setup(
		shader.version_get_shader(shader_version, MODE_CLEAR_DEPTH),
		RD::RENDER_PRIMITIVE_TRIANGLES,
		RD::PipelineRasterizationState(),
		RD::PipelineMultisampleState(),
		depth_stencil,
		RD::PipelineColorBlendState::create_disabled(),
		RD::DYNAMIC_STATE_STENCIL_REFERENCE | RD::DYNAMIC_STATE_STENCIL_WRITE_MASK
	);
}

PortalRender::~PortalRender() {
	shader.version_free(shader_version);
}

RID PortalRender::_get_uniform_set(RID p_scene_data, RID p_depth_texture) {
	thread_local LocalVector<RD::Uniform> uniforms;
	uniforms.clear();

	{
		RD::Uniform u;
		u.binding = 0;
		u.uniform_type = RD::UNIFORM_TYPE_UNIFORM_BUFFER_DYNAMIC;
		u.append_id(p_scene_data);
		uniforms.push_back(u);
	}

	if (p_depth_texture.is_valid()) {
		RD::Uniform u;
		u.binding = 1;
		u.uniform_type = RD::UNIFORM_TYPE_SAMPLER_WITH_TEXTURE;
		u.append_id(MaterialStorage::get_singleton()->sampler_rd_get_default(RS::CANVAS_ITEM_TEXTURE_FILTER_LINEAR, RS::CANVAS_ITEM_TEXTURE_REPEAT_DISABLED));
		u.append_id(p_depth_texture);
	}

	RID uniform_set = UniformSetCacheRD::get_singleton()->get_cache_vec(
		// As far as we know, this is safe(?)
		shader.version_get_shader(shader_version, p_depth_texture.is_null() ? MODE_DEPTH_STENCIL : MODE_MAP_DEPTH),
		uniform_set_binding,
		uniforms
	);

	// TODO
	// ????? TODO what?

	return uniform_set;
}

void PortalRender::portal_depth_prepass(Span<PortalRenderInfo> p_portals, int p_parent_index, RID p_scene_data, RID p_depth_framebuffer) {
	ERR_FAIL_COND(p_portals.is_empty());
	ERR_FAIL_COND(p_scene_data.is_null());
	ERR_FAIL_COND(p_depth_framebuffer.is_null());
	ERR_FAIL_COND(p_parent_index < p_portals[0].parent_index);
	ERR_FAIL_COND(p_parent_index - p_portals[0].index >= (int)p_portals.size());

	// Normally, we should be passing the entire portal list to this function, but just in case...
	const int index_offset = p_portals[0].index; 
	const PortalRenderInfo *parent = (p_parent_index > 0) ? &p_portals[p_parent_index - index_offset] : nullptr;

	if (parent && parent->first_child < 0) {
		// Do nothing, there are no child portals
		return;
	}

	RD::get_singleton()->draw_command_begin_label("Draw Portal Stencil");
	RD::DrawListID draw_list = _start_draw(p_depth_framebuffer);

	// Bind uniform set
	RID uniform_set = _get_uniform_set(p_scene_data);
	RD::get_singleton()->draw_list_bind_uniform_set(draw_list, uniform_set, uniform_set_binding, p_parent_index);

	// Count children
	int start = parent ? parent->first_child - index_offset : 0;
	int end = start + 1;
	while (end < (int)p_portals.size() && p_portals[end].parent_index == p_parent_index) {
		end++;
	}

	// Draw portal depths
	for (int i = start; i < end; i++) {
		_draw_portal_depth(draw_list, p_portals[i]);
	}

	// Draw portal stencils
	for (int i = start; i < end; i++) {
		_draw_portal_stencil(draw_list, p_portals[i]);
	}

	// Clear portal depths
	for (int i = start; i < end; i++) {
		_clear_portal_depth(draw_list, p_portals[i], false);
	}

	RD::get_singleton()->draw_list_end();
	RD::get_singleton()->draw_command_end_label();
}

void PortalRender::portal_transparent_resolve(Span<PortalRenderInfo> p_portals, int p_parent_index, RID p_scene_data, RID p_depth_framebuffer) {
	ERR_FAIL_COND(p_portals.is_empty());
	ERR_FAIL_COND(p_scene_data.is_null());
	ERR_FAIL_COND(p_depth_framebuffer.is_null());
	ERR_FAIL_COND(p_parent_index < p_portals[0].parent_index);
	ERR_FAIL_COND(p_parent_index - p_portals[0].index >= (int)p_portals.size());

	// Normally, we should be passing the entire portal list to this function, but just in case...
	const int index_offset = p_portals[0].index; 
	const PortalRenderInfo *parent = (p_parent_index > 0) ? &p_portals[p_parent_index - index_offset] : nullptr;

	if (parent && parent->first_child < 0) {
		// Do nothing, there are no child portals
		return;
	}
	
	RD::get_singleton()->draw_command_begin_label("Portal Transparent Resolve");
	RD::DrawListID draw_list = _start_draw(p_depth_framebuffer);

	// Bind uniform set
	RID uniform_set = _get_uniform_set(p_scene_data);
	RD::get_singleton()->draw_list_bind_uniform_set(draw_list, uniform_set, uniform_set_binding, p_parent_index);

	// Count children
	int start = parent ? parent->first_child - index_offset : 0;
	int end = start + 1;
	while (end < (int)p_portals.size() && p_portals[end].parent_index == p_parent_index) {
		end++;
	}

	// Clear portal depths and stencils
	for (int i = start; i < end; i++) {
		_clear_portal_depth(draw_list, p_portals[i], true);
	}

	// Draw portal depths
	for (int i = start; i < end; i++) {
		_draw_portal_depth(draw_list, p_portals[i]);
	}

	RD::get_singleton()->draw_list_end();
	RD::get_singleton()->draw_command_end_label();
}

////

void PortalRender::_draw_portal_depth(RD::DrawListID p_draw_list, const PortalRenderInfo &p_portal) {
	_bind_pipeline(p_draw_list, PIPELINE_DEPTH);

	uint32_t reference = p_portal.parent_index;
	if (reference != reference_cache) {
		reference_cache = reference;
		RD::get_singleton()->draw_list_set_stencil_masks(p_draw_list, RD::STENCIL_FACE_FRONT_AND_BACK, reference);
	}
	
	_draw_portal_common(p_draw_list, p_portal);
}

void PortalRender::_draw_portal_stencil(RD::DrawListID p_draw_list, const PortalRenderInfo &p_portal) {
	_bind_pipeline(p_draw_list, PIPELINE_STENCIL);

	uint32_t reference = p_portal.parent_index;
	if (reference == reference_cache) { reference = RD::STENCIL_MASK_DEFAULT; }
	else { reference_cache = reference; }

	// Since we use invert operator, we do a bitwise XOR with our two indices to set our desired value
	uint32_t write_mask = (uint8_t)p_portal.index ^ (uint8_t)p_portal.parent_index;
	if (write_mask == write_mask_cache) { write_mask = RD::STENCIL_MASK_DEFAULT; }
	else { write_mask_cache = write_mask; }

	RD::get_singleton()->draw_list_set_stencil_masks(p_draw_list, RD::STENCIL_FACE_FRONT_AND_BACK, reference, RD::STENCIL_MASK_DEFAULT, write_mask);
	
	_draw_portal_common(p_draw_list, p_portal);
}

void PortalRender::_clear_portal_depth(RD::DrawListID p_draw_list, const PortalRenderInfo &p_portal, bool p_revert_stencil) {
	_bind_pipeline(p_draw_list, PIPELINE_CLEAR_DEPTH);

	uint32_t reference = p_portal.index;
	if (reference == reference_cache) { reference = RD::STENCIL_MASK_DEFAULT; }
	else { reference_cache = reference; }

	uint32_t write_mask = p_revert_stencil ? (uint8_t)p_portal.index ^ (uint8_t)p_portal.parent_index : 0;
	if (write_mask == write_mask_cache) { write_mask = RD::STENCIL_MASK_DEFAULT; }
	else { write_mask_cache = write_mask; }

	if (reference != RD::STENCIL_MASK_DEFAULT || write_mask != RD::STENCIL_MASK_DEFAULT) {
		RD::get_singleton()->draw_list_set_stencil_masks(p_draw_list, RD::STENCIL_FACE_FRONT_AND_BACK, reference, RD::STENCIL_MASK_DEFAULT, write_mask);
	}

	_draw_portal_common(p_draw_list, p_portal);
}

void PortalRender::_draw_portal_common(RD::DrawListID p_draw_list, const PortalRenderInfo &p_portal) {
	RID vertex_array;
	RID index_array;
	PortalStorage::get_singleton()->portal_get_rd_arrays(p_portal.portal, vertex_array, index_array);
	
	if (vertex_array != vertex_array_cache) {
		vertex_array_cache = vertex_array;
		RD::get_singleton()->draw_list_bind_vertex_array(p_draw_list, vertex_array);
	}

	if (index_array != index_array_cache) {
		index_array_cache = index_array;
		RD::get_singleton()->draw_list_bind_index_array(p_draw_list, index_array);
	}
	
	PushConstant push_constant;
	push_constant.model_matrix = p_portal.source_transform;

	RD::get_singleton()->draw_list_set_push_constant(p_draw_list, &push_constant, sizeof(push_constant));
	RD::get_singleton()->draw_list_draw(p_draw_list, true);
}

////

RD::DrawListID PortalRender::_start_draw(RID p_framebuffer) {
	// Reset our draw list state cache
	current_pipeline = -1;
	write_mask_cache = RD::STENCIL_MASK_DEFAULT;
	reference_cache = RD::STENCIL_MASK_DEFAULT;
	vertex_array_cache = RID();
	index_array_cache = RID();
	framebuffer_format = RD::get_singleton()->framebuffer_get_format(p_framebuffer);
	
	return RD::get_singleton()->draw_list_begin(p_framebuffer);
}

void PortalRender::_bind_pipeline(RD::DrawListID p_draw_list, int p_pipeline) {
	if (current_pipeline != p_pipeline) {
		RID pipeline = pipelines[p_pipeline].get_render_pipeline(PortalStorage::get_singleton()->get_vertex_format(), framebuffer_format);
		RD::get_singleton()->draw_list_bind_render_pipeline(p_draw_list, pipeline);
		current_pipeline = p_pipeline;
	}
}