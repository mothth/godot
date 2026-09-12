/**************************************************************************/
/*  render_scene_data_rd.h                                                */
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

#include "servers/rendering/renderer_scene_render.h"
#include "servers/rendering/storage/render_scene_data.h"
#include "servers/rendering/renderer_rd/layout_rules.h"
#include "servers/rendering/renderer_rd/dynamic_buffer.h"

// This is a container for data related to rendering a single frame of a viewport where we load this data into a dynamic UBO
// that can be used by the main scene shader but also by various effects.

class RenderSceneDataRD : public RenderSceneData {
	GDCLASS(RenderSceneDataRD, RenderSceneData);

public:
	bool calculate_motion_vectors = false;
	bool shadow_pass = false;
	bool material_uv2_mode = false;

	float taa_frame_count = 0.0f;
	Vector2 taa_jitter;
	Vector2 prev_taa_jitter;

	// For billboards to cast correct shadows.
	Transform3D main_cam_transform;

	// For stereo rendering
	uint32_t view_count = 1;
	Vector3 view_eye_offset[RendererSceneRender::MAX_RENDER_VIEWS];	

	Size2 reflection_atlas_border_size;

	float emissive_exposure_normalization = 0.0;

	float time;
	float time_step;

	float lod_distance_multiplier = 0.0;
	float screen_mesh_lod_threshold = 0.0;

	// A specific "camera" of the scene - this defines where the camera matrices and the actual environment.
	// Primarily used by portals and shadows, but yes, the primary view uses this too.
	struct CameraData {
		Transform3D cam_transform;
		Transform3D prev_cam_transform;
		Projection cam_projection;
		Projection prev_cam_projection;

		uint32_t camera_visible_layers = 0xFFFFFFFF;
		
		int portal_depth = 0;

		bool cam_orthogonal = false;
		bool cam_frustum = false;
		bool flip_y = false;
		bool pancake_shadows = false;

		float z_near = 0.0;
		float z_far = 0.0;

		float dual_paraboloid_side = 0.0;

		Size2i screen_size;
		Size2i viewport_size;

		uint32_t cluster_width = 0;
		uint32_t cluster_type_size = 0;
		uint32_t cluster_base_offset = 0;
		uint32_t cluster_remap_length = 0;
		Size2i cluster_offset;

		/* Environment / scenario specific (same for most, but may still vary between views if we are rendering, say, world portals) */

		uint32_t directional_light_count = 0;
		uint32_t directional_light_offset = 0;

		float radiance_pixel_size = 0.0f;
		float radiance_border_size = 0.0f;

		float volumetric_fog_inv_length = 0.0f;
		float volumetric_fog_detail_spread = 0.0f;

		RID environment;

		Plane get_near_plane() const {
			Plane plane = Plane(-cam_transform.basis.get_column(Vector3::AXIS_Z), cam_transform.origin);
			plane.d += z_near;
			return plane;
		}
	};

	// Additional data for stereo rendering - separated from ViewData because its so dang large
	struct MultiCameraData {
		Projection view_projection[RendererSceneRender::MAX_RENDER_VIEWS];
		Projection prev_view_projection[RendererSceneRender::MAX_RENDER_VIEWS];
	};

	LocalVector<CameraData> cameras;
	LocalVector<MultiCameraData> multi_cameras;

	virtual Transform3D get_cam_transform() const override;
	virtual Projection get_cam_projection() const override;

	virtual uint32_t get_view_count() const override;
	virtual Vector3 get_view_eye_offset(uint32_t p_view) const override;
	virtual Projection get_view_projection(uint32_t p_view) const override;

	void update_ubo(RID p_uniform_buffer, RS::ViewportDebugDraw p_debug_mode = RS::VIEWPORT_DEBUG_DRAW_DISABLED, RID p_reflection_probe_instance = RID(), RID p_camera_attributes = RID(), const Color &p_default_bg_color = Color(), float p_luminance_multiplier = 1.0f);
	virtual RID get_uniform_buffer() const override;

	uint32_t get_uniform_buffer_frame_count() {
		return cameras.size();
	}

	uint32_t get_uniform_buffer_size_bytes() {
		return calculate_motion_vectors ?
			(view_count > 1 ? sizeof(UBOMultiviewMotion) : sizeof(UBOMotion)) :
			//(view_count > 1 ? sizeof(UBOMultiview) : sizeof(UBO));
			sizeof(UBOMultiview);
	}

	_FORCE_INLINE_ const Transform3D &get_camera_transform(uint32_t p_camera_index) const {
		return (p_camera_index < cameras.size()) ? cameras[p_camera_index].cam_transform : main_cam_transform;
	}

	_FORCE_INLINE_ const Projection &get_camera_view_projection(uint32_t p_camera_index, int p_view) const {
		if (multi_cameras.size() <= p_camera_index) {
			return cameras[p_camera_index].cam_projection;
		} else {
			return multi_cameras[p_camera_index].view_projection[p_view];
		}
	}

	_FORCE_INLINE_ RID get_camera_environment(uint32_t p_camera_index) const {
		return (p_camera_index < cameras.size()) ? cameras[p_camera_index].environment : RID();
	}
	
	_FORCE_INLINE_ int camera_count() {
		return cameras.size();
	};

	_FORCE_INLINE_ CameraData &new_camera() {
		int size = cameras.size();
		cameras.resize(size+1);
		if (view_count > 1) {
			multi_cameras.resize(size+1);
		}
		return cameras[size];
	}

	_FORCE_INLINE_ CameraData &first_camera() { return cameras[0]; }
	_FORCE_INLINE_ MultiCameraData &first_multi_camera() { return multi_cameras[0]; }
	_FORCE_INLINE_ CameraData &last_camera() { return cameras[cameras.size()-1]; }
	_FORCE_INLINE_ MultiCameraData &last_multi_camera() { return multi_cameras[multi_cameras.size()-1]; }
	_FORCE_INLINE_ const CameraData &first_camera() const { return cameras[0]; }
	_FORCE_INLINE_ const MultiCameraData &first_multi_camera() const { return multi_cameras[0]; }
	_FORCE_INLINE_ const CameraData &last_camera() const { return cameras[cameras.size()-1]; }
	_FORCE_INLINE_ const MultiCameraData &last_multi_camera() const { return multi_cameras[multi_cameras.size()-1]; }

	_FORCE_INLINE_ void clear_cameras() {
		cameras.clear();
		multi_cameras.clear();
	}

protected:
	RID uniform_buffer; // loaded into this uniform buffer (supplied externally)

	enum SceneDataFlags {
		SCENE_DATA_FLAGS_USE_AMBIENT_LIGHT = 1 << 0,
		SCENE_DATA_FLAGS_USE_AMBIENT_CUBEMAP = 1 << 1,
		SCENE_DATA_FLAGS_USE_REFLECTION_CUBEMAP = 1 << 2,
		SCENE_DATA_FLAGS_USE_FOG = 1 << 3,
		SCENE_DATA_FLAGS_USE_UV2_MATERIAL = 1 << 4,
		SCENE_DATA_FLAGS_USE_PANCAKE_SHADOWS = 1 << 5,
		SCENE_DATA_FLAGS_IN_SHADOW_PASS = 1 << 6, // Only used by Forward+ renderer.
		SCENE_DATA_FLAGS_USE_VOLUMETRIC_FOG = 1 << 7,
		SCENE_DATA_FLAGS_MAX
	};

	// This struct is loaded into Set 1 - Binding 0, populated at start of rendering a frame, must match with shader code
	struct SceneDataUBO {
		std140_mat4 projection_matrix;
		std140_mat4 inv_projection_matrix;
		std140_mat3x4 inv_view_matrix;
		std140_mat3x4 view_matrix;

		std140_mat4 main_cam_inv_view_matrix;

		#ifdef REAL_T_IS_DOUBLE
			std140_vec4 inv_view_precision;
		#endif

		std140_mat3 radiance_inverse_xform;

		float radiance_pixel_size;
		float radiance_border_size;

		std140_vec2 taa_jitter; 
		float taa_frame_count;  // Used to add break up samples over multiple frames. Value is an integer from 0 to taa_phase_count -1.

		float fog_density;
		float fog_height;
		float fog_height_density;

		float fog_depth_curve;
		float fog_depth_begin;
		float fog_depth_end;
		float fog_sun_scatter;

		union {
			std140_vec3 fog_light_color;
			std140_vec3_trail fog_aerial_perspective;
		};

		std140_vec4 ambient_light_color_energy;
		float ambient_color_sky_mix;

		float time;

		uint directional_light_count;
		uint directional_light_offset;

		float z_far;
		float z_near;

		std140_vec2 viewport_size;
		std140_vec2 screen_pixel_size;

		uint32_t camera_visible_layers;
		uint32_t flags;

		float emissive_exposure_normalization; // Needed to normalize emissive when using physical units.
		float IBL_exposure_normalization; // Adjusts for baked exposure.

		float dual_paraboloid_side; // Needed in shadow pass

		float volumetric_fog_inv_length;
		float volumetric_fog_detail_spread;

		uint32_t portal_depth;

		uint32_t cluster_width;
		uint32_t cluster_type_size;
		uint32_t cluster_base_offset;
		uint32_t cluster_remap_length;
		std140_uvec2 cluster_offset;

		std140_mat4 projection_matrix_view[RendererSceneRender::MAX_RENDER_VIEWS];
		std140_mat4 inv_projection_matrix_view[RendererSceneRender::MAX_RENDER_VIEWS];
		std140_vec4 eye_offset[RendererSceneRender::MAX_RENDER_VIEWS];
	};

	// Extra data if we are rendering multi-view. Must match with shader code (requires USE_MULTIVIEW to be defined)
	struct SceneDataMultiviewUBO : public SceneDataUBO {
		/*
		std140_mat4 projection_matrix_view[RendererSceneRender::MAX_RENDER_VIEWS];
		std140_mat4 inv_projection_matrix_view[RendererSceneRender::MAX_RENDER_VIEWS];
		std140_vec4 eye_offset[RendererSceneRender::MAX_RENDER_VIEWS];
		*/
	};

	////

	struct UBO { SceneDataUBO ubo; };
	struct UBOMultiview { SceneDataMultiviewUBO ubo; };

	struct UBOMotion {
		SceneDataUBO ubo;
		SceneDataUBO prev_ubo;
	};

	struct UBOMultiviewMotion {
		SceneDataMultiviewUBO ubo;
		SceneDataMultiviewUBO prev_ubo;
	};

	void _update_ubo_common(SceneDataUBO &ubo, RS::ViewportDebugDraw p_debug_mode, RID p_reflection_probe_instance, RID p_camera_attributes, const Color &p_default_bg_color, float p_luminance_multiplier, int p_camera_index);
};
