// Helper types for std140-compatible and std430-compatible structures
#pragma once

#include "core/typedefs.h"
#include "core/math/vector2.h"
#include "core/math/vector3.h"
#include "core/math/vector4.h"
#include "core/math/vector2i.h"
#include "core/math/vector3i.h"
#include "core/math/vector4i.h"
#include "core/math/projection.h"
#include "core/math/transform_3d.h"
#include "core/math/color.h"

#define LAYOUT_IS_GLSL_SCALAR(T) (std::is_same_v<T, float> || std::is_same_v<T, int> || std::is_same_v<T, unsigned int> || std::is_same_v<T, double>)
#define LAYOUT_IS_GLSL_CONTAINER(T) (std::is_base_of_v<_glsl_base, T>)
#define LAYOUT_IS_GLSL_VEC(T) (std::is_base_of_v<_glsl_base_vec, T>)
#define LAYOUT_IS_GLSL_STRUCT(T) (std::is_base_of_v<_glsl_base_struct, T>)
#define LAYOUT_IS_GLSL_TYPE(T) (LAYOUT_IS_GLSL_SCALAR(T) || LAYOUT_IS_GLSL_CONTAINER(T))

#define LAYOUT_GLSL_TYPE_ASSERT(T) static_assert(LAYOUT_IS_GLSL_TYPE(T))
#define LAYOUT_VEC_TYPE_ASSERT(T) static_assert(LAYOUT_IS_GLSL_SCALAR(T))
#define LAYOUT_VEC_TYPE_ASSERT_4(T) static_assert(LAYOUT_IS_GLSL_SCALAR(T) && sizeof(T) == 4)

struct std140 {
	template <typename T> static constexpr std::size_t member_alignment = (std::alignment_of_v<T> < 16) ? (std::alignment_of_v<T> > 4) ? next_power_of_2(std::alignment_of_v<T>) : 4 : 16;
	template <typename T> static constexpr std::size_t element_alignment = 16;
	template <typename T> static constexpr std::size_t struct_alignment = 16;
	template <typename T, std::size_t length> static constexpr std::size_t vec_alignment = (sizeof(T[length]) < 16) ? (sizeof(T[length]) > 4) ? next_power_of_2(sizeof(T[length])) : 4 : 16;
};

struct std430 {
	template <typename T> static constexpr std::size_t member_alignment = std140::member_alignment<T>;
	template <typename T> static constexpr std::size_t element_alignment = member_alignment<T>;
	template <typename T> static constexpr std::size_t struct_alignment = member_alignment<T>;
	template <typename T, std::size_t length> static constexpr std::size_t vec_alignment = std140::vec_alignment<T, length>;
};

////

/* Type-erased bases */

struct _glsl_base {};
struct _glsl_base_vec : _glsl_base {};
struct _glsl_base_struct : _glsl_base {};

/* Templated bases */

template <typename layout>
struct layout_struct : _glsl_base_struct {
	typedef layout Layout;
};

template <typename T, int length, typename layout>
struct alignas(layout::template vec_alignment<T, length>) _layout_base_vec : _glsl_base_vec {
	LAYOUT_VEC_TYPE_ASSERT(T);
	typedef layout Layout;
};

/* Everything else */

template <typename T, int length, typename layout>
struct _layout_array : _glsl_base {
	LAYOUT_GLSL_TYPE_ASSERT(T);
	struct alignas(layout::template element_alignment<T>) Element { T value; };
	Element array[length];
};

template <int num_columns, int num_rows, typename T, typename layout>
struct alignas(16) _layout_mat : _glsl_base {
	static_assert(num_rows <= 4 && num_rows > 1);
	static_assert(num_columns <= 4 && num_columns > 1);
	LAYOUT_VEC_TYPE_ASSERT(T);

	struct alignas(layout::template vec_alignment<T, num_rows>) Column {
		T row[num_rows];
		T &operator[](int p_index) { return row[p_index]; }
		const T &operator[](int p_index) const { return row[p_index]; }
	};

	union {
		Column columns[num_columns];
		T raw[sizeof(columns) / (sizeof(T))];	// Brackets around sizeof(T) to silence a warning
	};

	void store_projection(const Projection &p_mtx) {
		for (int i = 0; i < num_columns; i++) {
			for (int j = 0; j < num_rows; j++) {
				columns[i][j] = p_mtx.columns[i][j];
			}
		}
	}

	void store_projection_transposed(const Projection &p_mtx) {
		for (int i = 0; i < num_columns; i++) {
			for (int j = 0; j < num_rows; j++) {
				columns[i][j] = p_mtx.columns[j][i];
			}
		}
	}

	void store_transform(const Transform3D &p_mtx) {
		int column_count = MIN(num_columns, 3);
		int row_count = MIN(num_rows, 3);
		for (int i = 0; i < column_count; i++) {
			for (int j = 0; j < row_count; j++) {
				columns[i][j] = p_mtx.basis.rows[j][i];
			}
		}
		if (num_columns > 3) {
			for (int j = 0; j < row_count; j++) {
				columns[3][j] = p_mtx.origin[j];
			}
		}
		if (num_rows > 3) {
			for (int i = 0; i < num_columns; i++) {
				columns[i][3] = (i < 3) ? 0.0f : 1.0f;
			}
		}
	}

	void store_transform_transposed(const Transform3D &p_mtx) {
		int column_count = MIN(num_columns, 3);
		int row_count = MIN(num_rows, 3);
		for (int i = 0; i < column_count; i++) {
			for (int j = 0; j < row_count; j++) {
				columns[i][j] = p_mtx.basis.rows[i][j];
			}
		}
		if (num_rows > 3) {
			for (int i = 0; i < column_count; i++) {
				columns[i][3] = p_mtx.origin[i];
			}
		}
		if (num_columns > 3) {
			for (int j = 0; j < num_rows; j++) {
				columns[3][j] = (j < 3) ? 0.0f : 1.0f;
			}
		}
	}
	
	void operator=(const Projection &p_mtx) { store_projection(p_mtx); }
	void operator=(const Transform3D &p_mtx) { store_transform(p_mtx); }

	T &operator[](int p_index) { return raw[p_index]; }
	const T &operator[](int p_index) const { return raw[p_index]; }
};

template <typename T, typename layout>
struct _layout_vec2 : _layout_base_vec<T, 2, layout> {
	union {
		T raw[2];
		struct { T x, y; };
	};

	_layout_vec2() = default;
	_layout_vec2(T p_v) : x(p_v), y(p_v) {}
	_layout_vec2(T p_x, T p_y) : x(p_x), y(p_y) {}
	_layout_vec2(Vector2 p_vec) : x(p_vec.x), y(p_vec.y) {}
	_layout_vec2(Vector2i p_vec) : x(p_vec.x), y(p_vec.y) {}

	T &operator[](int p_index) { return raw[p_index]; }
	const T &operator[](int p_index) const { return raw[p_index]; }
};

template <typename T, typename layout>
struct _layout_vec3 : _layout_base_vec<T, 3, layout> {
	union {
		T raw[3];
		struct { T x, y, z; };
		struct { T r, g, b; };
	};

	_layout_vec3() = default;
	_layout_vec3(T p_x, T p_y, T p_z) : x(p_x), y(p_y), z(p_z) {}
	_layout_vec3(Vector3 p_vec) : x(p_vec.x), y(p_vec.y), z(p_vec.z) {}
	_layout_vec3(Vector3i p_vec) : x(p_vec.x), y(p_vec.y), z(p_vec.z) {}
	_layout_vec3(Color p_color) : r(p_color.r), g(p_color.g), b(p_color.b) {}

	T &operator[](int p_index) { return raw[p_index]; }
	const T &operator[](int p_index) const { return raw[p_index]; }
};

// This is a helper - Allows to batch an additional 4-byte type in the trailing padding of _layout_vec3 when used in a union with it
template <typename T, typename layout>
struct alignas(layout::template vec_alignment<T, 3>) _layout_vec3_trail {
	LAYOUT_VEC_TYPE_ASSERT_4(T);
	static_assert(layout::template vec_alignment<T, 3> >= sizeof(T[4])); // We must be able to actually fit the extra element

private:
	T vec3[3];
public:
	T value;

	_layout_vec3_trail() = default;
	_layout_vec3_trail(T p_value) : value(p_value) {}
	
	T &operator=(T p_value) {
		value = p_value;
		return value;
	}
};

template <typename T, typename layout>
struct _layout_vec4 : _layout_base_vec<T, 4, layout> {
	union {
		T raw[4];
		struct { T x, y, z, w; };
		struct { T r, g, b, a; };
	};

	_layout_vec4() = default;
	_layout_vec4(T p_x, T p_y, T p_z, T p_w) : x(p_x), y(p_y), z(p_z), w(p_w) {}
	_layout_vec4(Vector4 p_vec) : x(p_vec.x), y(p_vec.y), z(p_vec.z), w(p_vec.w) {}
	_layout_vec4(Vector4i p_vec) : x(p_vec.x), y(p_vec.y), z(p_vec.z), w(p_vec.w) {}
	_layout_vec4(Color p_color) : r(p_color.r), g(p_color.g), b(p_color.b), a(p_color.a) {}

	T &operator[](int p_index) { return raw[p_index]; }
	const T &operator[](int p_index) const { return raw[p_index]; }
};

template <typename layout>
struct alignas(layout::template member_alignment<bool>) _layout_bool : _glsl_base {
	bool value;
};

////

template <typename T, int length> using _std140_array = _layout_array<T, length, std140>;
template <typename T, int length> using _std430_array = _layout_array<T, length, std430>;
template <int num_columns, int num_rows = num_columns, typename T = float> using _std140_mat = _layout_mat<num_columns, num_rows, T, std140>;
template <int num_columns, int num_rows = num_columns, typename T = float> using _std430_mat = _layout_mat<num_columns, num_rows, T, std430>;

template <typename T> using _std140_vec2 = _layout_vec2<T, std140>;
template <typename T> using _std430_vec2 = _layout_vec2<T, std430>;
template <typename T> using _std140_vec3 = _layout_vec3<T, std140>;
template <typename T> using _std430_vec3 = _layout_vec3<T, std430>;
template <typename T> using _std140_vec4 = _layout_vec4<T, std140>;
template <typename T> using _std430_vec4 = _layout_vec4<T, std430>;
template <typename T> using _std140_vec3_trail = _layout_vec3_trail<T, std140>;
template <typename T> using _std430_vec3_trail = _layout_vec3_trail<T, std430>;

////

typedef _std140_vec2<float> std140_vec2;
typedef _std140_vec3<float> std140_vec3;
typedef _std140_vec4<float> std140_vec4;

typedef _std140_vec2<int> std140_ivec2;
typedef _std140_vec3<int> std140_ivec3;
typedef _std140_vec4<int> std140_ivec4;

typedef _std140_vec2<unsigned int> std140_uvec2;
typedef _std140_vec3<unsigned int> std140_uvec3;
typedef _std140_vec4<unsigned int> std140_uvec4;

typedef _std140_vec2<double> std140_dvec2;
typedef _std140_vec3<double> std140_dvec3;
typedef _std140_vec4<double> std140_dvec4;

typedef _std140_vec3_trail<float> std140_vec3_trail;
typedef _std140_vec3_trail<int> std140_ivec3_trail;
typedef _std140_vec3_trail<unsigned int> std140_uvec3_trail;

typedef _std140_mat<2> std140_mat2;
typedef _std140_mat<3> std140_mat3;
typedef _std140_mat<4> std140_mat4;

typedef _std140_mat<2, 2, int> std140_imat2;
typedef _std140_mat<3, 3, int> std140_imat3;
typedef _std140_mat<4, 4, int> std140_imat4;

typedef _std140_mat<2, 2, unsigned int> std140_umat2;
typedef _std140_mat<3, 3, unsigned int> std140_umat3;
typedef _std140_mat<4, 4, unsigned int> std140_umat4;

typedef _std140_mat<2, 2, double> std140_dmat2;
typedef _std140_mat<3, 3, double> std140_dmat3;
typedef _std140_mat<4, 4, double> std140_dmat4;

typedef _std140_mat<3, 4> std140_mat3x4;

////

typedef _std430_vec2<float> std430_vec2;
typedef _std430_vec3<float> std430_vec3;
typedef _std430_vec4<float> std430_vec4;

typedef _std430_vec2<int> std430_ivec2;
typedef _std430_vec3<int> std430_ivec3;
typedef _std430_vec4<int> std430_ivec4;

typedef _std430_vec2<unsigned int> std430_uvec2;
typedef _std430_vec3<unsigned int> std430_uvec3;
typedef _std430_vec4<unsigned int> std430_uvec4;

typedef _std430_vec2<double> std430_dvec2;
typedef _std430_vec3<double> std430_dvec3;
typedef _std430_vec4<double> std430_dvec4;

typedef _std430_vec3_trail<float> std430_vec3_trail;
typedef _std430_vec3_trail<int> std430_ivec3_trail;
typedef _std430_vec3_trail<unsigned int> std430_uvec3_trail;

typedef _std430_mat<2> std430_mat2;
typedef _std430_mat<3> std430_mat3;
typedef _std430_mat<4> std430_mat4;

typedef _std430_mat<2, 2, int> std430_imat2;
typedef _std430_mat<3, 3, int> std430_imat3;
typedef _std430_mat<4, 4, int> std430_imat4;

typedef _std430_mat<2, 2, unsigned int> std430_umat2;
typedef _std430_mat<3, 3, unsigned int> std430_umat3;
typedef _std430_mat<4, 4, unsigned int> std430_umat4;

typedef _std430_mat<2, 2, double> std430_dmat2;
typedef _std430_mat<3, 3, double> std430_dmat3;
typedef _std430_mat<4, 4, double> std430_dmat4;

typedef _std430_mat<3, 4> std430_mat3x4;

