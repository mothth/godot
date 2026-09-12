// Scene data stores all our 3D rendering globals for a frame such as our matrices
// where this information is independent of the different RD implementations.
// This enables us to use this UBO in our main scene render shaders but also in
// effects that need access to this data.

#define SCENE_DATA_FLAGS_USE_AMBIENT_LIGHT (1 << 0)
#define SCENE_DATA_FLAGS_USE_AMBIENT_CUBEMAP (1 << 1)
#define SCENE_DATA_FLAGS_USE_REFLECTION_CUBEMAP (1 << 2)
#define SCENE_DATA_FLAGS_USE_FOG (1 << 3)
#define SCENE_DATA_FLAGS_USE_UV2_MATERIAL (1 << 4)
#define SCENE_DATA_FLAGS_USE_PANCAKE_SHADOWS (1 << 5)
#define SCENE_DATA_FLAGS_IN_SHADOW_PASS (1 << 6)
#define SCENE_DATA_FLAGS_USE_VOLUMETRIC_FOG (1 << 7)

struct SceneData {
	mat4 projection_matrix;
	mat4 inv_projection_matrix;
	mat3x4 inv_view_matrix;
	mat3x4 view_matrix;

	// Used for billboards to cast correct shadows.
	// Incandescence - Did some digging about this. This is used specifically in generated GDShaders, not the internal shaders.
	// This will change between portal views as well. It is only for shadow rendering which only happens once and not per-portal,
	// which theoretically could be problematic for portal views if we are rendering billboard shadows... It's possible at a later date
	// we may do per-portal shadow rendering specifically for materials that use MAIN_CAM_INV_VIEW_MATRIX.
	// The Story Machine doesn't even really use billboards anyway, or at least not ones that cast shadows. So its implementation is NOT a priority.
	mat4 main_cam_inv_view_matrix;

	#ifdef USE_DOUBLE_PRECISION
		vec4 inv_view_precision;
	#endif

	mat3 radiance_inverse_xform;

	float radiance_pixel_size;
	float radiance_border_size;

	vec2 taa_jitter;
	float taa_frame_count;

	float fog_density;
	float fog_height;
	float fog_height_density;

	float fog_depth_curve;
	float fog_depth_begin;
	float fog_depth_end;
	float fog_sun_scatter;

	vec3 fog_light_color;
	float fog_aerial_perspective;

	vec4 ambient_light_color_energy;
	float ambient_color_sky_mix;

	float time;

	uint directional_light_count;
	uint directional_light_offset;

	// These are the same for each portal, even if their projection matrices are oblique.
	float z_far;
	float z_near;

	vec2 viewport_size;
	vec2 screen_pixel_size;

	uint camera_visible_layers;
	uint flags;

	float emissive_exposure_normalization;
	float IBL_exposure_normalization;

	float dual_paraboloid_side; // Needed in shadow pass

	float volumetric_fog_inv_length;
	float volumetric_fog_detail_spread;

	uint portal_depth;
	
	uint cluster_width;
	uint cluster_type_size;
	uint cluster_base_offset;
	uint cluster_remap_length;
	uvec2 cluster_offset;

	/* The following are tagged on from class inheritance, so don't move these anywhere above. */

	//#ifdef USE_MULTIVIEW
		// Only used for multiview
		mat4 projection_matrix_view[MAX_VIEWS];
		mat4 inv_projection_matrix_view[MAX_VIEWS];
		vec4 eye_offset[MAX_VIEWS];
	//#endif
};