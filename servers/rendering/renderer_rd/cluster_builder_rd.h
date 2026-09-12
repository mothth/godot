/**************************************************************************/
/*  cluster_builder_rd.h                                                  */
/**************************************************************************/
/*                         This file is part of:                          */
/*                             GODOT ENGINE                               */
/*                        https://godotengine.org                         */
/**************************************************************************/
/* Copyright (c) 2014-present Godot Engine contributors (see AUTHORS.md). */
/* Copyright (c) 2007-2014 Juan Linietsky, Ariel Manzur.                  */
/*                                                                        */
/* Permission is hereby granted, free of charge, to any person obtaining  */
/* a copy of this software and associated documentation files (the        */
/* "Software"), to deal in the Software without restriction, including    */
/* without limitation the rights to use, copy, modify, merge, publish,    */
/* distribute, sublicense, and/or sell copies of the Software, and to     */
/* permit persons to whom the Software is furnished to do so, subject to  */
/* the following conditions:                                              */
/*                                                                        */
/* The above copyright notice and this permission notice shall be         */
/* included in all copies or substantial portions of the Software.        */
/*                                                                        */
/* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,        */
/* EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF     */
/* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. */
/* IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY   */
/* CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,   */
/* TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE      */
/* SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.                 */
/**************************************************************************/

#pragma once

#include "servers/rendering/renderer_rd/shaders/cluster_debug.glsl.gen.h"
#include "servers/rendering/renderer_rd/shaders/cluster_render.glsl.gen.h"
#include "servers/rendering/renderer_rd/shaders/cluster_store.glsl.gen.h"
#include "servers/rendering/renderer_rd/storage_rd/material_storage.h"
#include "servers/rendering/renderer_rd/storage_rd/portal_storage.h"
#include "servers/rendering/renderer_rd/layout_rules.h"
#include "servers/rendering/renderer_rd/dynamic_buffer.h"

class ClusterBuilderSharedDataRD {
	friend class ClusterBuilderRD;

	RID sphere_vertex_buffer;
	RID sphere_vertex_array;
	RID sphere_index_buffer;
	RID sphere_index_array;
	float sphere_overfit = 0.0; // Because an icosphere is not a perfect sphere, we need to enlarge it to cover the sphere area.

	RID cone_vertex_buffer;
	RID cone_vertex_array;
	RID cone_index_buffer;
	RID cone_index_array;
	float cone_overfit = 0.0; // Because an cone mesh is not a perfect cone, we need to enlarge it to cover the actual cone area.

	RID box_vertex_buffer;
	RID box_vertex_array;
	RID box_index_buffer;
	RID box_index_array;

	enum Divisor {
		DIVISOR_1,
		DIVISOR_2,
		DIVISOR_4,
	};

	struct ClusterRender {
		struct PushConstant {
			std430_mat4 projection;

			uint32_t base_index;
			uint32_t base_offset;
			uint32_t dest_offset;

			uint32_t cluster_screen_width;
			uint32_t cluster_data_size; 
			uint32_t cluster_depth_offset;
			uint32_t screen_to_clusters_shift;
			
			float inv_z_far;
			std430_uvec2 cluster_screen_offset; 
		};

		ClusterRenderShaderRD cluster_render_shader;
		RID shader_version;
		RID shader;

		enum ShaderVariant {
			SHADER_NORMAL,
			SHADER_USE_ATTACHMENT,
			SHADER_NORMAL_MOLTENVK,
			SHADER_USE_ATTACHMENT_MOLTENVK,
			SHADER_NORMAL_NO_ATOMICS,
			SHADER_USE_ATTACHMENT_NO_ATOMICS,
		};

		enum PipelineVersion {
			PIPELINE_NORMAL,
			PIPELINE_MSAA,
			PIPELINE_MAX
		};

		RID shader_pipelines[PIPELINE_MAX];
	} cluster_render;

	struct ClusterStore {
		struct PushConstant {
			uint32_t cluster_render_data_size; // how much data for a single cluster takes
			uint32_t max_render_element_count_div_32; // divided by 32
			std430_uvec2 cluster_screen_size;
			uint32_t render_element_count_div_32; // divided by 32
			uint32_t max_cluster_element_count_div_32; // divided by 32

			uint32_t render_buffer_offset;
			uint32_t dest_buffer_offset;
			uint32_t element_buffer_offset;
			uint32_t pad[3];
		};

		ClusterStoreShaderRD cluster_store_shader;
		RID shader_version;
		RID shader;
		RID shader_pipeline;
	} cluster_store;

	struct ClusterDebug {
		struct PushConstant {
			uint32_t screen_size[2];
			uint32_t cluster_screen_size[2];

			uint32_t cluster_shift;
			uint32_t cluster_type;
			float z_near;
			float z_far;

			uint32_t orthogonal;
			uint32_t max_cluster_element_count_div_32;

			uint32_t pad1;
			uint32_t pad2;
		};

		ClusterDebugShaderRD cluster_debug_shader;
		RID shader_version;
		RID shader;
		RID shader_pipeline;
	} cluster_debug;

public:
	ClusterBuilderSharedDataRD();
	~ClusterBuilderSharedDataRD();
};

class ClusterBuilderRD {
private:

	struct RenderElementData {
		uint32_t type; // 0-4
		uint32_t touches_near;
		uint32_t touches_far;
		uint32_t original_index;
		float transform_inv[12]; // Transposed transform for less space.
		float scale[3];
		uint32_t has_wide_spot_angle;

		_FORCE_INLINE_ float sort_value() const {
			return (type != ELEMENT_TYPE_REFLECTION_PROBE) ? -transform_inv[11] : scale_sort_size();
		}

		_FORCE_INLINE_ float scale_sort_size() const {
			return scale[0] * scale[0] + scale[1] * scale[1] + scale[2] * scale[2];
		}

		// For portals, so we can optimally sort them.
		// This is specifically for choosing new values for the index remap.
		struct SortComparator {
			_ALWAYS_INLINE_ bool operator()(const RenderElementData &p_a, const RenderElementData &p_b) const {
				return p_a.type == p_b.type && p_a.sort_value() < p_b.sort_value();
			}
		};
	}; // Keep aligned to 32 bytes.
	// ??? Doesn't seem very aligned to 32 bytes

public:
	static constexpr float WIDE_SPOT_ANGLE_THRESHOLD_DEG = 60.0f;

	enum LightType {
		LIGHT_TYPE_OMNI,
		LIGHT_TYPE_SPOT
	};

	enum BoxType {
		BOX_TYPE_REFLECTION_PROBE,
		BOX_TYPE_DECAL,
	};

	enum ElementType {
		ELEMENT_TYPE_OMNI_LIGHT,
		ELEMENT_TYPE_SPOT_LIGHT,
		ELEMENT_TYPE_DECAL,
		ELEMENT_TYPE_REFLECTION_PROBE,
		ELEMENT_TYPE_MAX,
	};

	struct ClusterPortal {
		Size2i cluster_screen_offset;
		Size2i cluster_screen_size;
		Rect2 render_region;
		uint32_t cluster_type_size = 0;
		uint32_t buffer_offset = 0;

		// Information about the index remap, used specifically when portals are sharing clustered items but from different views,
		// leading to a sub-optimal cluster buffer ordering. This is just to remap indices into something more optimal (usually depth-based).
		// Portals are free to not use this remap if the ordering of items is close enough to the normal ordering, which helps tremendously for recursive portals.
		// Portals will also not use this remap if they have their own unique set of clustered items or only uses some sub-section that is already contiguous,
		// in which the cluster buffer will just offset into those indices directly.
		// The index map is placed at the end of the cluster data for that portal, and this is used to offset into the maps for different types.

		uint32_t remap_length = 0;

	protected:

		friend class ClusterBuilderRD;

		LocalVector<uint32_t> remap;
		LocalVector<RenderElementData> render_elements;
		Transform3D view_xform;

		uint32_t render_element_offset;
		uint32_t render_buffer_offset;
		uint32_t render_element_max;

		_FORCE_INLINE_ RenderElementData &new_render_element() {
			render_elements.push_back(RenderElementData());
			return render_elements[render_elements.size()-1];
		}
	};

private:
	ClusterBuilderSharedDataRD *shared = nullptr;

	uint32_t cluster_count_by_type[ELEMENT_TYPE_MAX] = {};
	uint32_t max_elements_by_type = 0;

	// RenderElementData *render_elements = nullptr;
	LocalVector<RenderElementData> render_elements;
	uint32_t render_element_max = 0;

	uint32_t portal_count = 0;
	LocalVector<ClusterPortal> cluster_portals;

	Transform3D view_xform;
	Projection adjusted_projection;
	Projection projection;
	float z_far = 0;
	float z_near = 0;
	bool camera_orthogonal = false;

	enum Divisor {
		DIVISOR_1,
		DIVISOR_2,
		DIVISOR_4,
	};

	uint32_t cluster_size = 32;
#if defined(MACOS_ENABLED) || defined(APPLE_EMBEDDED_ENABLED)
	// Results in visual artifacts on macOS and iOS/visionOS when using MSAA and subgroups.
	// Using subgroups and disabling MSAA is the optimal solution for now and also works
	// with MoltenVK.
	bool use_msaa = false;
#else
	bool use_msaa = true;
#endif
	Divisor divisor = DIVISOR_4;

	Size2i screen_size;
	Size2i cluster_screen_size;

	RID framebuffer;
	RID cluster_render_buffer; // Used for creating.
	RID cluster_buffer; // Used for rendering.
	RID element_buffer; // Used for storing, to hint element touches far plane or near plane.
	uint32_t cluster_render_buffer_size = 0;
	uint32_t cluster_buffer_size = 0;
	uint32_t cluster_buffer_base_size = 0;
	uint32_t cluster_buffer_capacity = 0;
	uint32_t cluster_render_buffer_capacity = 0;
	uint32_t element_buffer_capacity = 0;

	RID cluster_render_uniform_set;
	RID cluster_store_uniform_set;

	// Persistent data.

	void _clear();
	void _process_portals();
	void _make_uniform_sets(RID p_depth_buffer = RID(), RID p_depth_buffer_sampler = RID(), RID p_color_buffer = RID());

	_FORCE_INLINE_ RenderElementData &new_render_element() {
		render_elements.push_back(RenderElementData());
		return render_elements[render_elements.size()-1];
	}

	struct StateUniform {
		std140_mat4 projection;
		/*
		float projection[16];
		float inv_z_far;
		uint32_t screen_to_clusters_shift; // Shift to obtain coordinates in block indices.
		uint32_t cluster_screen_width;
		uint32_t cluster_data_size; // How much data is needed for a single cluster.
		uint32_t cluster_depth_offset;

		uint32_t pad0;
		uint32_t pad1;
		uint32_t pad2;
		*/
	};

	// RID state_uniform;

	RID debug_uniform_set;

public:
	void setup(Size2i p_screen_size, uint32_t p_max_elements, RID p_depth_buffer, RID p_depth_buffer_sampler, RID p_color_buffer);

	// Note - when portal rendering, the non-oblique camera projection should be passed. In other words, *always* pass the projection of the first camera.
	void begin(const Transform3D &p_view_transform, const Projection &p_cam_projection, bool p_flip_y, Span<PortalRenderInfo> p_portals = Span<PortalRenderInfo>());

	_FORCE_INLINE_ void add_light(LightType p_type, const Transform3D &p_transform, float p_radius, float p_spot_aperture, uint32_t p_index, int p_portal_index = 0) {
		if (p_index >= max_elements_by_type) {
			return; // Max number elements reached.
		}

		RenderElementData &e = (p_portal_index == 0) ? new_render_element() : cluster_portals[p_portal_index-1].new_render_element();
		const Transform3D &view = (p_portal_index == 0) ? view_xform : cluster_portals[p_portal_index-1].view_xform;
		Transform3D xform = view * p_transform;
		
		float radius = xform.basis.get_uniform_scale();
		if (radius < 0.98 || radius > 1.02) {
			xform.basis.orthonormalize();
		}

		radius *= p_radius;

		// Spotlights with wide angle are treated as Omni lights.
		// If the spot angle is above the threshold, we need a sphere instead of a cone for building the clusters
		// since the cone gets too flat/large (spot angle close to 90 degrees) or
		// can't even cover the affected area of the light (spot angle above 90 degrees).
		if (p_type == LIGHT_TYPE_OMNI || (p_type == LIGHT_TYPE_SPOT && p_spot_aperture > WIDE_SPOT_ANGLE_THRESHOLD_DEG)) {
			radius *= shared->sphere_overfit; // Overfit icosphere.

			float depth = -xform.origin.z;
			if (camera_orthogonal) {
				e.touches_near = (depth - radius) < z_near;
			} else {
				// Contains camera inside light.
				float radius2 = radius * shared->sphere_overfit; // Overfit again for outer size (camera may be outside actual sphere but behind an icosphere vertex)
				e.touches_near = xform.origin.length_squared() < radius2 * radius2;
			}

			e.touches_far = (depth + radius) > z_far;
			e.scale[0] = radius;
			e.scale[1] = radius;
			e.scale[2] = radius;
			if (p_type == LIGHT_TYPE_OMNI) {
				e.type = ELEMENT_TYPE_OMNI_LIGHT;
				cluster_count_by_type[ELEMENT_TYPE_OMNI_LIGHT] = MAX(cluster_count_by_type[ELEMENT_TYPE_OMNI_LIGHT], p_index+1);
			}
			else { // LIGHT_TYPE_SPOT with wide angle.
				e.type = ELEMENT_TYPE_SPOT_LIGHT;
				e.has_wide_spot_angle = true;
				cluster_count_by_type[ELEMENT_TYPE_SPOT_LIGHT] = MAX(cluster_count_by_type[ELEMENT_TYPE_SPOT_LIGHT], p_index+1);
			}

			e.original_index = p_index;

			RendererRD::MaterialStorage::store_transform_transposed_3x4(xform, e.transform_inv);

		} else /*LIGHT_TYPE_SPOT with no wide angle*/ {
			radius *= shared->cone_overfit; // Overfit cone.

			real_t len = Math::tan(Math::deg_to_rad(p_spot_aperture)) * radius;
			// Approximate, probably better to use a cone support function.
			float max_d = -1e20;
			float min_d = 1e20;
#define CONE_MINMAX(m_x, m_y)                                             \
	{                                                                     \
		float d = -xform.xform(Vector3(len * m_x, len * m_y, -radius)).z; \
		min_d = MIN(d, min_d);                                            \
		max_d = MAX(d, max_d);                                            \
	}

			CONE_MINMAX(1, 1);
			CONE_MINMAX(-1, 1);
			CONE_MINMAX(-1, -1);
			CONE_MINMAX(1, -1);

			if (camera_orthogonal) {
				e.touches_near = min_d < z_near;
			} else {
				Plane base_plane(-xform.basis.get_column(Vector3::AXIS_Z), xform.origin);
				float dist = base_plane.distance_to(Vector3());
				if (dist >= 0 && dist < radius) {
					// Contains camera inside light, check angle.
					float angle = Math::rad_to_deg(Math::acos((-xform.origin.normalized()).dot(-xform.basis.get_column(Vector3::AXIS_Z))));
					e.touches_near = angle < p_spot_aperture * 1.05; //overfit aperture a little due to cone overfit
				} else {
					e.touches_near = false;
				}
			}

			e.touches_far = max_d > z_far;
			e.scale[0] = len * shared->cone_overfit;
			e.scale[1] = len * shared->cone_overfit;
			e.scale[2] = radius;
			e.has_wide_spot_angle = false;
			e.type = ELEMENT_TYPE_SPOT_LIGHT;
			e.original_index = p_index;
			
			cluster_count_by_type[ELEMENT_TYPE_SPOT_LIGHT] = MAX(cluster_count_by_type[ELEMENT_TYPE_SPOT_LIGHT], p_index+1);

			RendererRD::MaterialStorage::store_transform_transposed_3x4(xform, e.transform_inv);
		}
	}

	_FORCE_INLINE_ void add_box(BoxType p_box_type, const Transform3D &p_transform, const Vector3 &p_half_size, uint32_t p_index, int p_portal_index = 0) {
		if (p_index >= max_elements_by_type) {
			return; // Max number elements reached.
		}

		RenderElementData &e = (p_portal_index == 0) ? new_render_element() : cluster_portals[p_portal_index-1].new_render_element();
		const Transform3D &view = (p_portal_index == 0) ? view_xform : cluster_portals[p_portal_index-1].view_xform;
		Transform3D xform = view * p_transform;

		// Extract scale and scale the matrix by it, makes things simpler.
		Vector3 scale = p_half_size;
		for (uint32_t i = 0; i < 3; i++) {
			float s = xform.basis.rows[i].length();
			scale[i] *= s;
			xform.basis.rows[i] /= s;
		};

		float box_depth = Math::abs(xform.basis.xform_inv(Vector3(0, 0, -1)).dot(scale));
		float depth = -xform.origin.z;

		if (camera_orthogonal) {
			e.touches_near = depth - box_depth < z_near;
		} else {
			// Contains camera inside box.
			Vector3 inside = xform.xform_inv(Vector3(0, 0, 0)).abs();
			e.touches_near = inside.x < scale.x && inside.y < scale.y && inside.z < scale.z;
		}

		e.touches_far = depth + box_depth > z_far;

		e.scale[0] = scale.x;
		e.scale[1] = scale.y;
		e.scale[2] = scale.z;

		e.type = (p_box_type == BOX_TYPE_DECAL) ? ELEMENT_TYPE_DECAL : ELEMENT_TYPE_REFLECTION_PROBE;
		e.original_index = p_index;
		cluster_count_by_type[e.type] = MAX(cluster_count_by_type[e.type], p_index+1);

		RendererRD::MaterialStorage::store_transform_transposed_3x4(xform, e.transform_inv);
	}

	_FORCE_INLINE_ const ClusterPortal &get_cluster_portal(int p_index) const {
		return cluster_portals[p_index];
	}

	void bake_cluster();
	void debug(ElementType p_element);

	RID get_cluster_buffer() const;
	uint32_t get_cluster_size() const;
	uint32_t get_max_cluster_elements() const;

	void set_shared(ClusterBuilderSharedDataRD *p_shared);

	ClusterBuilderRD();
	~ClusterBuilderRD();
};
