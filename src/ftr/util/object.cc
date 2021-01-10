/* ***** BEGIN LICENSE BLOCK *****
 * Distributed under the BSD license:
 *
 * Copyright (c) 2015, xuewen.chu
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     * Neither the name of xuewen.chu nor the
 *       names of its contributors may be used to endorse or promote products
 *       derived from this software without specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL xuewen.chu BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 * 
 * ***** END LICENSE BLOCK ***** */

#include "ftr/util/string.h"
#include "ftr/util/object.h"
#include "ftr/util/loop.h"
#include "ftr/util/array.h"
#include <vector>

FX_NS(ftr)

void* DefaultAllocator::alloc(size_t size) {
	return ::malloc(size);
}
void* DefaultAllocator::realloc(void* ptr, size_t size) {
	return ::realloc(ptr, size);
}
void DefaultAllocator::free(void* ptr) {
	::free(ptr);
}

static void* default_object_alloc(size_t size) {
	return ::malloc(size);
}
static void default_object_release(Object* obj) {
	obj->~Object();
	::free(obj);
}
static void default_object_retain(Object* obj) {
	/* NOOP */
}

static ObjectAllocator object_allocator = {
	&default_object_alloc, &default_object_release, &default_object_retain,
};

String Object::to_string() const {
	static String str("[Object]");
	return str;
}

#if FX_MEMORY_TRACE_MARK

static int active_mark_objects_count_ = 0;
static Mutex mark_objects_mutex;
static Array<Object*> mark_objects_([](){
	Array<Object*> rv;
	return rv;
}());

int Object::initialize_mark_() {
	if ( mark_index_ == 123456 ) {
		ScopeLock scope(mark_objects_mutex);
		uint index = mark_objects_.length();
		mark_objects_.push(this);
		active_mark_objects_count_++;
		return index;
	}
	return -1;
}

Object::Object(): mark_index_(initialize_mark_()) {

}

Object::~Object() {
	if ( mark_index_ > -1 ) {
		ScopeLock scope(mark_objects_mutex);
		mark_objects_[mark_index_] = nullptr;
		ASSERT(active_mark_objects_count_);
		active_mark_objects_count_--;
	}
}

std::vector<Object*> Object::mark_objects() {
	ScopeLock scope(mark_objects_mutex);
	Array<Object*> new_mark_objects;
	std::vector<Object*> rv;
	
	for ( auto& i : mark_objects_ ) {
		Object* obj = i.value();
		if ( i.value() ) {
			obj->mark_index_ = new_mark_objects.length();
			new_mark_objects.push(obj);
			rv.push_back(obj);
		}
	}
	
	ASSERT( new_mark_objects.length() == active_mark_objects_count_ );
	
	mark_objects_ = move(new_mark_objects);
	return rv;
}

int Object::mark_objects_count() {
	return active_mark_objects_count_;
}

#endif

bool Object::is_reference() const {
	return false;
}

bool Object::retain() {
	return 0;
}

void Object::release() {
	object_allocator.release(this);
}

void* Object::operator new(std::size_t size) {
	
#if FX_MEMORY_TRACE_MARK
	void* p = object_allocator.alloc(size);
	((Object*)p)->mark_index_ = 123456;
	return p;
#else 
	return object_allocator.alloc(size);
#endif 
}

void* Object::operator new(std::size_t size, void* p) {
	// std::size_t s;
	return p;
}

void Object::operator delete(void* p) {
	FX_UNREACHABLE();
}

void set_object_allocator(ObjectAllocator* alloc) {
	if ( alloc ) {
		if (alloc->alloc) {
			object_allocator.alloc = alloc->alloc;
		} else {
			object_allocator.alloc = &default_object_alloc;
		}
		if (alloc->release) {
			object_allocator.release = alloc->release;
		} else {
			object_allocator.release = &default_object_release;
		}
		if (alloc->retain) {
			object_allocator.retain = alloc->retain;
		} else {
			object_allocator.retain = &default_object_retain;
		}
	} else {
		object_allocator.alloc = &default_object_alloc;
		object_allocator.release = &default_object_release;
		object_allocator.retain = &default_object_retain;
	}
}

bool Retain(Object* obj) {
	return obj ? obj->retain() : false;
}

void Release(Object* obj) {
	if ( obj )
		obj->release();
}

Reference::~Reference() {
	ASSERT( m_ref_count <= 0 );
}

bool Reference::retain() {
	ASSERT(m_ref_count >= 0);
	if ( m_ref_count++ == 0 ) {
		object_allocator.retain(this);
	}
	return true;
}

void Reference::release() {
	ASSERT(m_ref_count >= 0);
	if ( --m_ref_count <= 0 ) { // 当引用记数小宇等于0释放
		object_allocator.release(this);
	}
}

bool Reference::is_reference() const {
	return true;
}

FX_END
