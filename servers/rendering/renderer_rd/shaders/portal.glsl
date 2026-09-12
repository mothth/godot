#[vertex]

#version 450

#VERSION_DEFINES

#define MAX_VIEWS 2

#include "scene_data_inc.glsl"

// Vertex position
layout(location = 0) in vec3 vertex_attrib;

layout(push_constant, std430) uniform Params {
	mat4 model_matrix;
#ifdef VERSION_MAP_DEPTH
	mat4 target_projection;
#endif
}
params;

// Honestly just using this because lazy, don't want to create a separate buffer just for portals
// when this already contains our camera information
layout(set = 1, binding = 0, std140) uniform SceneDataBlock {
	SceneData data;
} scene_data_block;

#define scene_data scene_data_block.data

void main() {
	mat4 view_matrix = transpose(mat4(scene_data.view_matrix[0],
		scene_data.view_matrix[1],
		scene_data.view_matrix[2],
		vec4(0.0, 0.0, 0.0, 1.0)));
	gl_Position = scene_data.projection_matrix * view_matrix * params.model_matrix * vec4(vertex_attrib, 1.0);
}

#[fragment]

#version 450

#VERSION_DEFINES

#define MAX_VIEWS 2

#include "scene_data_inc.glsl"

#ifdef VERSION_MAP_DEPTH

layout(push_constant, std430) uniform Params {
	mat4 model_matrix;
	mat4 target_projection;
}
params;

layout(set = 1, binding = 0, std140) uniform SceneDataBlock {
	SceneData data;
} scene_data_block;

#define scene_data scene_data_block.data

layout(set = 1, binding = 1) uniform sampler2D source_depth;

#endif

void main() {

#ifdef VERSION_CLEAR_DEPTH

	// We have reverse depth in Godot, so clear to 0.
	gl_FragDepth = 0.0;

#endif // VERSION_CLEAR_DEPTH

#ifdef VERSION_MAP_DEPTH

	vec2 uv = gl_FragCoord.xy / scene_data.screen_pixel_size * 2.0 - 1.0;
	float depth_value = texelFetch(source_depth, ivec2(gl_FragCoord.xy), 0).r;
	vec4 clip_pos = vec4(uv, depth_value, 1.0);
	clip_pos = params.target_projection * scene_data.inv_projection_matrix * clip_pos;
	clip_pos.xyz /= clip_pos.w;
	gl_FragDepth = clip_pos.z;

#endif // VERSION_MAP_DEPTH

}