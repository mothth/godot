#pragma once

#include "core/config/project_settings.h"
#include "servers/rendering/rendering_server.h"

#define MAX_RENDERABLE_PORTALS	255

struct PortalRenderInfo;

class RendererPortalStorage {
public:
	virtual ~RendererPortalStorage() {}

	/* PORTAL API */

	virtual RID portal_allocate() = 0;
	virtual void portal_initialize(RID p_rid) = 0;
	
	virtual void portal_set_double_sided(RID p_portal, bool p_double_sided) = 0;
	virtual void portal_set_teleport_light(RID p_portal, bool p_teleport_light) = 0;
	virtual void portal_set_teleport_gi(RID p_portal, bool p_teleport_gi) = 0;
	virtual void portal_set_mesh(RID p_portal, RID p_mesh) = 0;
	virtual void portal_set_destination_transform(RID p_portal, const Transform3D &p_transform) = 0;
	virtual void portal_set_scenario_override(RID p_portal, RID p_scenario) = 0;
	virtual void portal_set_environment_override(RID p_portal, RID p_environment) = 0;
	virtual void portal_set_recursive_depth(RID p_portal, int p_depth) = 0;
	virtual void portal_set_cull_mask(RID p_portal, RenderingServer::PortalCullMaskOp p_op, uint32_t p_mask = 0xFFFFFFFF) = 0;
	virtual void portal_set_cull_partner(RID p_portal, RID p_partner) = 0;

	virtual bool portal_is_local_to_environment(RID p_portal) const = 0;
	virtual int portal_get_max_recursive_depth(RID p_portal) const = 0;
	virtual AABB portal_get_aabb(RID p_portal) const = 0;

	virtual Projection portal_get_oblique_projection(RID p_portal, const Projection &p_cam_projection, const Transform3D &p_cam_transform) const = 0;

	/* INSTANCE API */

	virtual RID portal_instance_create(RID p_portal) = 0;
	virtual void portal_instance_free(RID p_instance) = 0;
	virtual int get_instance_count() const { return 0; }
	
	virtual void portal_instance_set_transform(RID p_instance, const Transform3D &p_transform) = 0;
	virtual void portal_instance_set_aabb(RID p_instance, const AABB &p_aabb) = 0;

	virtual RID portal_instance_get_base_portal(RID p_instance) const = 0;
	virtual RID portal_instance_get_owner(RID p_instance) const = 0;

	virtual PortalRenderInfo portal_instance_make_render_info(RID p_instance, const Transform3D &p_cam_transform, const Projection &p_cam_projection) const = 0;

	virtual LocalVector<RID> get_instances() const = 0;

	/* OTHER */

	int max_portal_depth;

	void config_update() {
		max_portal_depth = GLOBAL_GET("rendering/portals/max_recursive_depth");
	}

	static Plane make_oblique_plane(const Transform3D &p_transform, const Vector3 &p_oblique_position, const Vector3 &p_oblique_normal, float p_oblique_offset = 0.0f) {
		int dot = int(p_oblique_normal.dot(p_oblique_position - p_transform.origin) >= 0.0f ? 1.0f : -1.0f);
		Vector3 pos = p_transform.xform_inv(p_oblique_position);
		Vector3 normal = p_transform.basis.xform_inv(p_oblique_normal) * dot;
		real_t dist = -pos.dot(normal) + p_oblique_offset;

		return Plane(normal, dist);
	}
};

// Stores data about what portal indices a given instance is visible in.
// This data is only valid during a scene render, as portal indices are assigned based on cull results.
// (Regular view outside of any portals is designated as bit 0)
struct PortalMaskData {
	typedef uint8_t Index;

	// Helper list for obtaining portal indices from a mask
	class List {
	protected:

		friend struct PortalMaskData;

		int count;
		int capacity;
		Index *list = nullptr;
		const PortalMaskData *last_mask = nullptr;

	public:

		List(int p_max_portals) : count(p_max_portals > 1 ? 0 : 1), capacity(p_max_portals) {
			// If our max portals is one (which means no actual portals), we just assume the first index is always present, so don't allocate anything
			list = p_max_portals > 1 ? (Index *) memalloc(p_max_portals * sizeof(Index)) : nullptr;
		}

		~List() {
			if (list) {
				memfree(list);
			}
		}

		_FORCE_INLINE_ int portal_count() const { return count; }
		_FORCE_INLINE_ int portal_capacity() const { return capacity; }
		_FORCE_INLINE_ bool is_evaluated() const { return last_mask != nullptr; }
		
		_FORCE_INLINE_ void clear() {
			count = list ? 0 : 1;
			last_mask = nullptr;
		}

		_FORCE_INLINE_ Index operator[](int p_index) const {
			return list ? list[p_index] : 0;
		}

	};

	union {
		// Maximum renderable portal count is 255 (stencil value limit), so we have a 256-bit mask
		uint8_t portals_8[32];
		uint16_t portals_16[16];
		uint32_t portals_32[8];
		uint64_t portals_64[4];
		struct {
			uint64_t mask_1 = 0;
			uint64_t mask_2 = 0;
			uint64_t mask_3 = 0;
			uint64_t mask_4 = 0;
		};
	};

	bool operator==(const PortalMaskData &p_other) const {
		return mask_1 == p_other.mask_1 && mask_2 == p_other.mask_2 && mask_3 == p_other.mask_3 && mask_4 == p_other.mask_4;
	}

	bool operator!=(const PortalMaskData &p_other) const {
		return mask_1 != p_other.mask_1 || mask_2 != p_other.mask_2 || mask_3 != p_other.mask_3 || mask_4 != p_other.mask_4;
	}

	PortalMaskData &operator&=(const PortalMaskData &p_other) {
		for (int i = 0; i < 4; i++) {
			portals_64[i] &= p_other.portals_64[i];
		}
		return *this;
	}

	PortalMaskData &operator|=(const PortalMaskData &p_other) {
		for (int i = 0; i < 4; i++) {
			portals_64[i] |= p_other.portals_64[i];
		}
		return *this;
	}

	PortalMaskData &operator^=(const PortalMaskData &p_other) {
		for (int i = 0; i < 4; i++) {
			portals_64[i] ^= p_other.portals_64[i];
		}
		return *this;
	}

	PortalMaskData operator&(const PortalMaskData &p_other) const {
		PortalMaskData result = *this;
		result &= p_other;
		return result;
	}

	PortalMaskData operator|(const PortalMaskData &p_other) const {
		PortalMaskData result = *this;
		result |= p_other;
		return result;
	}

	PortalMaskData operator^(const PortalMaskData &p_other) const {
		PortalMaskData result = *this;
		result ^= p_other;
		return result;
	}

	PortalMaskData operator~() const {
		PortalMaskData result = *this;
		for (int i = 0; i < 4; i++) {
			result.portals_64[i] = ~result.portals_64[i];
		}
		return result;
	}

	// Stores a list of at most `p_buffer_size` indices in `p_dest`.
	// No index greater than `p_max_portals` can be stored.
	// Returns the number of indices stored.
	int get_portals(int p_buffer_size, Index *p_dest, int p_max_portals = MAX_RENDERABLE_PORTALS+1) const {
		p_max_portals = MIN(p_max_portals, MAX_RENDERABLE_PORTALS+1);

		int count = 0;
		int index = 0;

		while (index < p_max_portals) {
			int remainder = index & 31;
			int mask_index = index >> 5;

			uint32_t bit_mask = 1 << remainder;
			if (portals_32[mask_index] & bit_mask) {
				p_dest[count++] = index;
				if (count >= p_buffer_size) {
					break;
				}
			}

			// Just to speed up iteration
			else if (portals_32[mask_index] == 0) {
				index += 32;
				continue;
			}
			else if (portals_8[index >> 3] == 0) {
				index += 8;
				continue;
			}

			index++;
		};

		return count;
	}

	_FORCE_INLINE_ int get_portals(List &p_list) const {
		if (p_list.capacity <= 1) {
			// Don't do anything, if there is only one portal then by definition index 0 is always set,
			// which our list already accounts for.
			return p_list.count;
		} 
		
		if (this == p_list.last_mask) {
			// We already evaluated this mask last, we can skip it (This helps because instances will try sharing portal masks if they're equivalent.)
			return p_list.count;
		}

		p_list.count = get_portals(p_list.capacity, p_list.list, p_list.capacity);
		p_list.last_mask = this;

		return p_list.count;
	}

	void set_portal(uint32_t p_index) {
		uint32_t remainder = p_index & 31u;
		uint32_t mask_index = p_index >> 5u;
		uint32_t bit_mask = 1 << remainder;
		portals_32[mask_index] |= bit_mask;
	}

	void remove_portal(uint32_t p_index) {
		uint32_t remainder = p_index & 31u;
		uint32_t mask_index = p_index >> 5u;
		uint32_t bit_mask = 1 << remainder;
		portals_32[mask_index] &= ~bit_mask;
	}

	// Create a new mask using a range of indices from `p_from` to `p_to` (exclusive) from this mask
	PortalMaskData sub_mask(uint64_t p_from, uint64_t p_to) const {
		PortalMaskData result = *this;
		for (int i = 0; i < 4; i++) {
			result.portals_64[i] &= make_range_mask_64(p_from, p_to, i);
		}
		return result;
	}

	_FORCE_INLINE_ static uint64_t make_range_mask_64(uint64_t p_from, uint64_t p_to, uint32_t mask_index) {
		uint64_t shift_from = MAX(p_from - mask_index * 64, 0U);
		uint64_t shift_to = MAX(p_to - mask_index * 64, 0U);
		return ~((1 << shift_from) - 1) & ((1 << shift_to) - 1);
	}

	PortalMaskData() {}
	PortalMaskData(const PortalMaskData &p_other) : mask_1(p_other.mask_1), mask_2(p_other.mask_2), mask_3(p_other.mask_3), mask_4(p_other.mask_4) {}
	
	// Create a mask with a defined range of set indices from `p_from` to `p_to` (exclusive)
	PortalMaskData(uint32_t p_from, uint32_t p_to) :
		mask_1(make_range_mask_64(p_from, p_to, 0)),
		mask_2(make_range_mask_64(p_from, p_to, 1)),
		mask_3(make_range_mask_64(p_from, p_to, 2)),
		mask_4(make_range_mask_64(p_from, p_to, 3))
	{}
};

// Information about how we should render a given portal (generated in cull stage)
struct PortalRenderInfo {
	typedef RenderingServer::PortalCullMaskOp CullMaskOp;

	int index = 0;
	int parent_index = 0;
	int depth = 0;

	int env_index = 0;
	int first_child = -1;

	// Always true for double-sided portals. This is mostly for for culling,
	// so you likely won't find this being false for portals actually in the render lists.
	bool front_facing = false;

	// Whether or not we have our cull mask cached
	mutable bool cull_mask_cached = false;

	// Cull mask
	CullMaskOp cull_mask_op = CullMaskOp::PORTAL_CULL_MASK_OP_KEEP;
	uint32_t cull_mask = 0xFFFFFFFF;
	mutable uint32_t cull_mask_cache = 0xFFFFFFFF;

	// Region on screen that encompasses the portal in normalised screen coordinates.
	Rect2 region;

	RID portal;
	RID partner_portal;
	RID environment;
	RID scenario;

	Transform3D source_transform;
	Transform3D dest_transform;
	Transform3D portal_transform;	// dest_transform * source_transform.affine_inverse()
	Transform3D cam_transform;

	PortalRenderInfo() = default;
	PortalRenderInfo(int p_index, int p_parent_index, int p_depth, const Transform3D &p_source_transform, const Transform3D &p_dest_transform, const Transform3D &p_parent_cam_transform) :
		index(p_index), parent_index(p_parent_index), depth(p_depth),
		source_transform(p_source_transform),
		dest_transform(p_dest_transform),
		portal_transform(p_dest_transform * p_source_transform.affine_inverse()),
		cam_transform(portal_transform * p_parent_cam_transform)
	{}

	uint32_t calculate_cull_mask(uint32_t p_cull_mask) const {
		switch (cull_mask_op) {
			case CullMaskOp::PORTAL_CULL_MASK_OP_KEEP: return p_cull_mask;
			case CullMaskOp::PORTAL_CULL_MASK_OP_REPLACE: return cull_mask;
			case CullMaskOp::PORTAL_CULL_MASK_OP_AND: return cull_mask & p_cull_mask;
			case CullMaskOp::PORTAL_CULL_MASK_OP_OR: return cull_mask | p_cull_mask;
			case CullMaskOp::PORTAL_CULL_MASK_OP_XOR: return cull_mask ^ p_cull_mask;
			default: return p_cull_mask;
		}
	}

	uint32_t get_cull_mask(Span<PortalRenderInfo> p_portals, uint32_t p_cull_mask) const {
		if (cull_mask_cached) {
			return cull_mask_cache;
		} else if (parent_index == 0 || cull_mask_op == CullMaskOp::PORTAL_CULL_MASK_OP_REPLACE) {
			cull_mask_cache = calculate_cull_mask(p_cull_mask);
		} else if (parent_index >= p_portals[0].index) {
			const PortalRenderInfo &parent = p_portals[parent_index - p_portals[0].index];
			cull_mask_cache = calculate_cull_mask(parent.get_cull_mask(p_portals, p_cull_mask));
		}

		cull_mask_cached = true;
		return cull_mask_cache;
	}
};