#pragma once

#include "servers/rendering/rendering_server.h"

struct BaseBufferHandler {
	RID buffer;

	_FORCE_INLINE_ void deallocate() {
		if (buffer.is_valid()) {
			RD::get_singleton()->free_rid(buffer);
			buffer = RID();
		}
	}

	_FORCE_INLINE_ RID get_buffer() {
		return buffer;
	}
	
	~BaseBufferHandler() {
		deallocate();
	}
};

struct UniformBufferHandler : BaseBufferHandler {
	void allocate_buffer(uint32_t p_size, uint32_t p_frames) {
		deallocate();
		buffer = RD::get_singleton()->uniform_buffer_create(p_size, Span<uint8_t>(), 0, p_frames);
	}
};

struct StorageBufferHandler : BaseBufferHandler {
	void allocate_buffer(uint32_t p_size, uint32_t p_frames) {
		deallocate();
		buffer = RD::get_singleton()->storage_buffer_create(p_size, Span<uint8_t>(), 0, 0, p_frames);
	}
};

// A utility container for "multi-frame" buffers that only resizes when needed, used with dynamic buffer bindings.
// NOTE: Any resize will **clear** the data of the buffer, as this is intended to be used for streaming data.
// If a persistent version is needed, it can implemented via a custom class derived from `BaseBufferHandler`.
template <typename Handler, bool tight = false>
class DynamicBuffer {
protected:
	uint32_t size = 0;		// Size of each frame in the buffer.
	uint32_t capacity = 0;	// Number of frames the buffer has allocated - this grows by power of twos if tight is false
	mutable Handler handler;

	// The actual size of each frame in the buffer, this is just so we don't have to reallocate the buffer if
	// we are switching between multiple sizes constantly for some reason. This will, however, correct itself
	// to the actual current size if capacity changes.
	mutable uint32_t true_size = 0;
	mutable bool dirty = false;

public:
	void set_size(uint32_t p_size) {
		if (p_size != size) {
			size = p_size;
			if (tight || p_size > true_size) {
				dirty = true;
			}
		}
	}

	void set_frame_count(uint32_t p_frames) {
		if (!tight && !is_power_of_2(p_frames)) {
			p_frames = next_power_of_2(p_frames);
		}
		if (p_frames > capacity) {
			capacity = p_frames;
			dirty = true;
		}
	}

	void reallocate() const {
		handler.allocate_buffer(size, capacity);
		true_size = size;
		dirty = false;
	}

	void deallocate() {
		if (handler.get_buffer().is_valid()) {
			handler.deallocate();
			size = 0;
			capacity = 0;
			dirty = false;
		}
	}
	
	_FORCE_INLINE_ bool needs_reallocation() const {
		return dirty;
	}

	// Get the buffer RID, ensuring it is correctly allocated.
	_FORCE_INLINE_ RID get_buffer() const {
		if (dirty) { reallocate(); }
		return handler.get_buffer();
	}

	_FORCE_INLINE_ bool is_null() const {
		return handler.get_buffer().is_null() && dirty;
	}

	_FORCE_INLINE_ bool is_valid() const {
		return handler.get_buffer().is_valid() || dirty;
	}

	_FORCE_INLINE_ operator RID() { return get_buffer(); }
	_FORCE_INLINE_ operator RID() const { return get_buffer(); }
};

template <bool tight = false>
using DynamicUniformBuffer = DynamicBuffer<UniformBufferHandler, tight>;

template <bool tight = false>
using DynamicStorageBuffer = DynamicBuffer<StorageBufferHandler, tight>;