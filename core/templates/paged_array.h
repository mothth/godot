/**************************************************************************/
/*  paged_array.h                                                         */
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

#include "core/os/memory.h"
#include "core/os/spin_lock.h"
#include "core/typedefs.h"

#include <type_traits>

// PagedArray is used mainly for filling a very large array from multiple threads efficiently and without causing major fragmentation

// PageArrayPool manages central page allocation in a thread safe matter

template <typename T>
class PagedArrayPool {
	T **page_pool = nullptr;
	uint32_t pages_allocated = 0;

	uint32_t *available_page_pool = nullptr;
	uint32_t pages_available = 0;

	uint32_t page_size = 0;
	SpinLock spin_lock;

public:
	struct PageInfo {
		T *page = nullptr;
		uint32_t page_id = 0;
	};

	PageInfo alloc_page() {
		spin_lock.lock();
		if (unlikely(pages_available == 0)) {
			uint32_t pages_used = pages_allocated;

			pages_allocated++;
			page_pool = (T **)memrealloc(page_pool, sizeof(T *) * pages_allocated);
			available_page_pool = (uint32_t *)memrealloc(available_page_pool, sizeof(uint32_t) * pages_allocated);

			page_pool[pages_used] = (T *)memalloc(sizeof(T) * page_size);
			available_page_pool[0] = pages_used;

			pages_available++;
		}

		pages_available--;
		uint32_t page_id = available_page_pool[pages_available];
		T *page = page_pool[page_id];
		spin_lock.unlock();

		return PageInfo{ page, page_id };
	}

	void free_page(uint32_t p_page_id) {
		spin_lock.lock();
		available_page_pool[pages_available] = p_page_id;
		pages_available++;
		spin_lock.unlock();
	}

	uint32_t get_page_size_shift() const {
		return get_shift_from_power_of_2(page_size);
	}

	uint32_t get_page_size_mask() const {
		return page_size - 1;
	}

	void reset() {
		ERR_FAIL_COND(pages_available < pages_allocated);
		if (pages_allocated) {
			for (uint32_t i = 0; i < pages_allocated; i++) {
				memfree(page_pool[i]);
			}
			memfree(page_pool);
			memfree(available_page_pool);
			page_pool = nullptr;
			available_page_pool = nullptr;
			pages_allocated = 0;
			pages_available = 0;
		}
	}
	bool is_configured() const {
		return page_size > 0;
	}

	void configure(uint32_t p_page_size) {
		ERR_FAIL_COND(page_pool != nullptr); // Safety check.
		ERR_FAIL_COND(p_page_size == 0);
		page_size = nearest_power_of_2_templated(p_page_size);
	}

	PagedArrayPool(uint32_t p_page_size = 4096) { // power of 2 recommended because of alignment with OS page sizes. Even if element is bigger, its still a multiple and get rounded amount of pages
		configure(p_page_size);
	}

	~PagedArrayPool() {
		ERR_FAIL_COND_MSG(pages_available < pages_allocated, "Pages in use exist at exit in PagedArrayPool");
		reset();
	}
};

// PageArray is a local array that is optimized to grow in place, then be cleared often.
// It does so by allocating pages from a PagedArrayPool.
// It is safe to use multiple PagedArrays from different threads, sharing a single PagedArrayPool.

template <typename T>
class PagedArray {
protected:
	PagedArrayPool<T> *page_pool = nullptr;

	T **page_data = nullptr;
	uint32_t *page_ids = nullptr;
	uint64_t count = 0;
	uint32_t max_pages_used = 0;
	uint32_t page_size_shift = 0;
	uint32_t page_size_mask = 0;

	_FORCE_INLINE_ uint32_t _get_pages_in_use() const {
		if (count == 0) {
			return 0;
		} else {
			return ((count - 1) >> page_size_shift) + 1;
		}
	}

	void _grow_page_array() {
		//no more room in the page array to put the new page, make room
		if (max_pages_used == 0) {
			max_pages_used = 1;
		} else {
			max_pages_used *= 2; // increase in powers of 2 to keep allocations to minimum
		}
		page_data = (T **)memrealloc(page_data, sizeof(T *) * max_pages_used);
		page_ids = (uint32_t *)memrealloc(page_ids, sizeof(uint32_t) * max_pages_used);
	}

public:
	_FORCE_INLINE_ const T &operator[](uint64_t p_index) const {
		CRASH_BAD_UNSIGNED_INDEX(p_index, count);
		uint32_t page = p_index >> page_size_shift;
		uint32_t offset = p_index & page_size_mask;

		return page_data[page][offset];
	}
	_FORCE_INLINE_ T &operator[](uint64_t p_index) {
		CRASH_BAD_UNSIGNED_INDEX(p_index, count);
		uint32_t page = p_index >> page_size_shift;
		uint32_t offset = p_index & page_size_mask;

		return page_data[page][offset];
	}

	_FORCE_INLINE_ void push_back(const T &p_value) {
		uint32_t remainder = count & page_size_mask;
		if (unlikely(remainder == 0)) {
			// at 0, so time to request a new page
			uint32_t page_count = _get_pages_in_use();
			uint32_t new_page_count = page_count + 1;

			if (unlikely(new_page_count > max_pages_used)) {
				ERR_FAIL_NULL(page_pool); // Safety check.

				_grow_page_array(); //keep out of inline
			}

			typename PagedArrayPool<T>::PageInfo page_info = page_pool->alloc_page();
			page_data[page_count] = page_info.page;
			page_ids[page_count] = page_info.page_id;
		}

		// place the new value
		uint32_t page = count >> page_size_shift;
		uint32_t offset = count & page_size_mask;

		if constexpr (!std::is_trivially_constructible_v<T>) {
			memnew_placement(&page_data[page][offset], T(p_value));
		} else {
			page_data[page][offset] = p_value;
		}

		count++;
	}

	_FORCE_INLINE_ void pop_back() {
		ERR_FAIL_COND(count == 0);

		if constexpr (!std::is_trivially_destructible_v<T>) {
			uint32_t page = (count - 1) >> page_size_shift;
			uint32_t offset = (count - 1) & page_size_mask;
			page_data[page][offset].~T();
		}

		uint32_t remainder = count & page_size_mask;
		if (unlikely(remainder == 1)) {
			// one element remained, so page must be freed.
			uint32_t last_page = _get_pages_in_use() - 1;
			page_pool->free_page(page_ids[last_page]);
		}
		count--;
	}

	void remove_at_unordered(uint64_t p_index) {
		ERR_FAIL_UNSIGNED_INDEX(p_index, count);
		(*this)[p_index] = (*this)[count - 1];
		pop_back();
	}

	void clear() {
		//destruct if needed
		if constexpr (!std::is_trivially_destructible_v<T>) {
			for (uint64_t i = 0; i < count; i++) {
				uint32_t page = i >> page_size_shift;
				uint32_t offset = i & page_size_mask;
				page_data[page][offset].~T();
			}
		}

		//return the pages to the pagepool, so they can be used by another array eventually
		uint32_t pages_used = _get_pages_in_use();
		for (uint32_t i = 0; i < pages_used; i++) {
			page_pool->free_page(page_ids[i]);
		}

		count = 0;

		//note we leave page_data and page_indices intact for next use. If you really want to clear them call reset()
	}

	void reset() {
		clear();
		if (page_data) {
			memfree(page_data);
			memfree(page_ids);
			page_data = nullptr;
			page_ids = nullptr;
			max_pages_used = 0;
		}
	}

	// This takes the pages from a source array and merges them to this one
	// resulting order is undefined, but content is merged very efficiently,
	// making it ideal to fill content on several threads to later join it.

	void merge_unordered(PagedArray<T> &p_array) {
		ERR_FAIL_COND(page_pool != p_array.page_pool);

		uint32_t remainder = count & page_size_mask;

		T *remainder_page = nullptr;
		uint32_t remainder_page_id = 0;

		if (remainder > 0) {
			uint32_t last_page = _get_pages_in_use() - 1;
			remainder_page = page_data[last_page];
			remainder_page_id = page_ids[last_page];
		}

		count -= remainder;

		uint32_t src_page_index = 0;
		uint32_t page_size = page_size_mask + 1;

		while (p_array.count > 0) {
			uint32_t page_count = _get_pages_in_use();
			uint32_t new_page_count = page_count + 1;

			if (unlikely(new_page_count > max_pages_used)) {
				_grow_page_array(); //keep out of inline
			}

			page_data[page_count] = p_array.page_data[src_page_index];
			page_ids[page_count] = p_array.page_ids[src_page_index];

			uint32_t take = MIN(p_array.count, page_size); //pages to take away
			p_array.count -= take;
			count += take;
			src_page_index++;
		}

		//handle the remainder page if exists
		if (remainder_page) {
			uint32_t new_remainder = count & page_size_mask;

			if (new_remainder > 0) {
				//must merge old remainder with new remainder

				T *dst_page = page_data[_get_pages_in_use() - 1];
				uint32_t to_copy = MIN(page_size - new_remainder, remainder);

				for (uint32_t i = 0; i < to_copy; i++) {
					if constexpr (!std::is_trivially_constructible_v<T>) {
						memnew_placement(&dst_page[i + new_remainder], T(remainder_page[i + remainder - to_copy]));
					} else {
						dst_page[i + new_remainder] = remainder_page[i + remainder - to_copy];
					}

					if constexpr (!std::is_trivially_destructible_v<T>) {
						remainder_page[i + remainder - to_copy].~T();
					}
				}

				remainder -= to_copy; //subtract what was copied from remainder
				count += to_copy; //add what was copied to the count

				if (remainder == 0) {
					//entire remainder copied, let go of remainder page
					page_pool->free_page(remainder_page_id);
					remainder_page = nullptr;
				}
			}

			if (remainder > 0) {
				//there is still remainder, append it
				uint32_t page_count = _get_pages_in_use();
				uint32_t new_page_count = page_count + 1;

				if (unlikely(new_page_count > max_pages_used)) {
					_grow_page_array(); //keep out of inline
				}

				page_data[page_count] = remainder_page;
				page_ids[page_count] = remainder_page_id;

				count += remainder;
			}
		}
	}

	_FORCE_INLINE_ uint64_t size() const {
		return count;
	}

	void set_page_pool(PagedArrayPool<T> *p_page_pool) {
		ERR_FAIL_COND(max_pages_used > 0); // Safety check.

		page_pool = p_page_pool;
		page_size_mask = page_pool->get_page_size_mask();
		page_size_shift = page_pool->get_page_size_shift();
	}

	~PagedArray() {
		reset();
	}
};

// A version of PagedArray that preserves pointers to its elements, even after merges. (In other words, it is pointer-stable.)
// As a drawback, there may be internal discontinuities in the array after merges that make it slower to index into.
// It is also no longer guaranteed that elements will be inserted at the end with `push_back`, which means you cannot use `size()-1` to access that element,
// so `push_back` has been replaced with `push_unordered`.

template <typename T>
class PersistentPagedArray {
protected:
	PagedArrayPool<T> *page_pool = nullptr;

	T **page_data = nullptr;
	uint32_t *page_ids = nullptr;

	// Stores the first available page-global index of each remainder page.
	// If we only have one remainder page, this isn't used. (The first available index is equal to the remainder of `count & page_size_mask` in that case.)
	// This is stored backwards, so the first remainder page has the last index in this array.
	// (This means we dont have to move the entire array by one element when a remainder page is used up.)
	uint64_t *remainder_page_ends = nullptr;

	uint64_t count = 0;
	uint32_t pages_used = 0;
	uint32_t max_pages_used = 0;

	uint32_t remainder_pages_used = 0;
	uint32_t max_remainder_pages_used = 0;

	uint32_t page_size_shift = 0;
	uint32_t page_size_mask = 0;

	void _grow_page_array(uint32_t p_to) {
		//no more room in the page array to put the new page, make room
		this->max_pages_used = next_power_of_2(p_to);
		this->page_data = (T **)memrealloc(this->page_data, sizeof(T *) * this->max_pages_used);
		this->page_ids = (uint32_t *)memrealloc(this->page_ids, sizeof(uint32_t) * this->max_pages_used);
	}

	void _grow_remainder_page_array(uint32_t p_to) {
		max_remainder_pages_used = next_power_of_2(p_to);
		this->remainder_page_ends = (uint64_t *) memrealloc(this->remainder_page_ends, sizeof(uint64_t) * max_remainder_pages_used);
	}

	_FORCE_INLINE_ uint64_t &free_remainder_index(uint32_t p_remainder_page = 0) {
		// We store remainder page ends backwards
		return remainder_page_ends[remainder_pages_used - p_remainder_page - 1];
	}

	_FORCE_INLINE_ void _get_page_offset(uint64_t p_index, uint32_t &page, uint32_t &offset) const {
		page = p_index >> page_size_shift;
		if (page >= pages_used - remainder_pages_used && remainder_pages_used > 1) {
			// Account for remainder pages
			// TODO: We could turn this into a binary search since our array is already sorted, though for our use-cases it doesn't really matter
			page = pages_used - remainder_pages_used;
			uint32_t page_size = page_size_mask+1;
			for (int i = 0; i < remainder_pages_used; i++) {
				uint64_t free_index = free_remainder_index(i);
				if (p_index < free_index) {
					break;
				}
				p_index += page_size - (free_index & page_size_mask);
				page++;
			}
		}
		offset = p_index & page_size_mask;
	}

public:

	const T &operator[](uint64_t p_index) const {
		CRASH_BAD_UNSIGNED_INDEX(p_index, count);
		uint32_t page;
		uint32_t offset;
		_get_page_offset(p_index, page, offset);
		return page_data[page][offset];
	}
	
	T &operator[](uint64_t p_index) {
		CRASH_BAD_UNSIGNED_INDEX(p_index, count);
		uint32_t page;
		uint32_t offset;
		_get_page_offset(p_index, page, offset);
		return page_data[page][offset];
	}

	T *push_unordered(const T &p_value) {
		uint64_t index;

		if (remainder_pages_used > 0) [[likely]] {
			// We have a free index we can use already
			index = (remainder_pages_used > 1) ? free_remainder_index() : count;

			if (((index+1) & page_size_mask) == 0) {
				// Next index will be the end of the remainder page, which means we can say that this remainder page is used up
				remainder_pages_used--;
			}
			else if (remainder_pages_used > 1) {
				// Otherwise, if we have multiple remainder pages, increment our next free index for this page
				free_remainder_index()++;
			}
		}
		else {
			// Request a new page
			uint32_t new_page_count = pages_used + 1;

			if (unlikely(new_page_count > max_pages_used)) {
				ERR_FAIL_NULL_V(page_pool, nullptr); // Safety check.
				_grow_page_array(next_power_of_2(new_page_count)); //keep out of inline
			}

			typename PagedArrayPool<T>::PageInfo page_info = page_pool->alloc_page();
			page_data[pages_used] = page_info.page;
			page_ids[pages_used] = page_info.page_id;
			pages_used++;
			remainder_pages_used++; // We will now have a remainder page to use

			index = count;
		}

		// place the new value
		uint32_t page = index >> page_size_shift;
		uint32_t offset = index & page_size_mask;

		if constexpr (!std::is_trivially_constructible_v<T>) {
			memnew_placement(&page_data[page][offset], T(p_value));
		} else {
			page_data[page][offset] = p_value;
		}

		count++;
		return &page_data[page][offset];
	}

	_FORCE_INLINE_ void pop_back() {
		ERR_FAIL_COND(count == 0);

		uint32_t page;
		uint32_t offset;
		_get_page_offset(count - 1, page, offset);

		if constexpr (!std::is_trivially_destructible_v<T>) {
			page_data[page][offset].~T();
		}

		if (unlikely(offset == 1)) {
			// one element remained, so page must be freed.
			page_pool->free_page(page_ids[page]);
			pages_used--;
			remainder_pages_used--;
		}

		count--;
	}

	void remove_at_unordered(uint64_t p_index) {
		ERR_FAIL_UNSIGNED_INDEX(p_index, count);
		(*this)[p_index] = (*this)[count - 1];
		pop_back();
	}

	void erase(const T *p_element) {
		uint32_t page_size = page_size_mask + 1;
		uint32_t full_pages_used = pages_used - remainder_pages_used;
		for (int i = 0; i < pages_used; i++) {
			if (p_element > page_data[i] && p_element < page_data[i] + page_size) {
				// Found element page
				
				uint64_t offset = p_element - page_data[i];

				if (i >= full_pages_used && offset >= (free_remainder_index(i - pages_used) & page_size_mask)) [[unlikely]] {
					// Make sure we don't accidentally erase an element that doesn't exist
					continue;
				}

				if constexpr (!std::is_trivially_destructible_v<T>) {
					page_data[i][offset].~T();
				}

				if (unlikely(offset == 1)) {
					// one element remained, so page must be freed.
					page_pool->free_page(page_ids[i]);
					pages_used--;
					remainder_pages_used--;
				}

				count--;
				break;
			}
		}
	}

	void clear() {
		uint32_t full_pages_used = pages_used - remainder_pages_used;

		//destruct if needed
		if constexpr (!std::is_trivially_destructible_v<T>) {
			uint64_t index = 0;
			for (uint64_t i = 0; i < count; i++) {
				uint32_t page = index >> page_size_shift;
				uint32_t offset = index & page_size_mask;
				if (remainder_pages_used > 1 && page >= full_pages_used) {
					// We're in one of several remainder pages - check if we need to jump
					if (index >= free_remainder_index(page - full_pages_used)) {
						page += 1;
						offset = 0;
						index = page << page_size_shift;
					}
				}
				
				page_data[page][offset].~T();
				index++;
			}
		}

		//return the pages to the pagepool, so they can be used by another array eventually
		for (uint32_t i = 0; i < pages_used; i++) {
			page_pool->free_page(page_ids[i]);
		}

		count = 0;
		pages_used = 0;
		remainder_pages_used = 0;

		//note we leave page_data and page_indices intact for next use. If you really want to clear them call reset()
	}

	void reset() {
		clear();
		if (page_data) {
			memfree(page_data);
			memfree(page_ids);
			memfree(remainder_page_ends);
			page_data = nullptr;
			page_ids = nullptr;
			remainder_page_ends = nullptr;
			max_pages_used = 0;
			max_remainder_pages_used = 0;
		}
	}

	void merge_unordered(PersistentPagedArray<T> &p_array) {
		ERR_FAIL_COND(page_pool != p_array.page_pool);

		uint32_t src_page_index = 0;
		uint32_t dest_page_index = 0;
		uint32_t page_size = page_size_mask+1;

		uint32_t new_page_count = pages_used + p_array.pages_used;
		if (new_page_count > max_pages_used) {
			_grow_page_array(new_page_count);
		}

		// Grow remainder page data if needed
		uint32_t new_remainder_pages_count = remainder_pages_used + p_array.remainder_pages_used;
		if (new_remainder_pages_count > 1 && new_remainder_pages_count > max_remainder_pages_used) {
			_grow_remainder_page_array(new_remainder_pages_count);
		}

		// Move our remainder pages to the end
		src_page_index = pages_used - remainder_pages_used;
		dest_page_index = new_page_count - remainder_pages_used;
		for (uint32_t i = 0; i < remainder_pages_used; i++) {
			page_data[dest_page_index] = page_data[src_page_index];
			page_ids[dest_page_index] = page_ids[src_page_index];
			src_page_index++;
			dest_page_index++;
		}
		
		// Correct our free remainder indices if needed
		if (new_remainder_pages_count > 1) {
			if (remainder_pages_used == 1) {
				// Our original lone remainder page now needs to store its first free index
				remainder_page_ends[0] = count + p_array.pages_used * page_size;
			} else {
				for (uint32_t i = 0; i < remainder_pages_used; i++) {
					remainder_page_ends[i] += p_array.pages_used * page_size;
				}
			}
		}

		// Move pages from other array
		src_page_index = 0;
		dest_page_index = pages_used - remainder_pages_used;
		for (uint32_t i = 0; i < p_array.pages_used; i++) {
			page_data[dest_page_index] = p_array.page_data[src_page_index];
			page_ids[dest_page_index] = p_array.page_ids[src_page_index];
			src_page_index++;
			dest_page_index++;
		}

		// Copy remainder index data from other array
		if (p_array.remainder_pages_used > 1) {
			for (uint32_t i = 0; i < p_array.remainder_pages_used; i++) {
				remainder_page_ends[i + remainder_pages_used] = p_array.remainder_page_ends[i] + (pages_used - remainder_pages_used) * page_size;
			}
		}
		else if (p_array.remainder_pages_used == 1 && new_remainder_pages_count > 1) {
			remainder_page_ends[new_remainder_pages_count - 1] = p_array.count + (pages_used - remainder_pages_used) * page_size;
		}

		count += p_array.count;
		pages_used = new_page_count;
		remainder_pages_used = new_remainder_pages_count;

		p_array.count = 0;
		p_array.pages_used = 0;
		p_array.remainder_pages_used = 0;
	}

	_FORCE_INLINE_ uint64_t size() const {
		return count;
	}

	void set_page_pool(PagedArrayPool<T> *p_page_pool) {
		ERR_FAIL_COND(max_pages_used > 0); // Safety check.
		page_pool = p_page_pool;
		page_size_mask = page_pool->get_page_size_mask();
		page_size_shift = page_pool->get_page_size_shift();
	}

	~PersistentPagedArray() {
		reset();
	}
};
