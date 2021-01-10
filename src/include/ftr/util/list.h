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

#ifndef __ftr__util__list__
#define __ftr__util__list__

#include "ftr/util/util.h"
#include "ftr/util/iterator.h"
#include <initializer_list>

FX_NS(ftr)

/**
 * @class List 双向链表
 */
template<class Item, class ItemAllocator> 
class FX_EXPORT List: public Object {
 private:
	struct ItemWrap {
		Item _item;
		ItemWrap* _prev;
		ItemWrap* _next;
		static void* operator new (std::size_t size) {
			return ItemAllocator::alloc(size);
		}
		static void  operator delete(void* p) {
			return ItemAllocator::free(p);
		}
	};
	struct IteratorData {
	 public:
		typedef Item Value;
		const Item& value() const;
		Item& value();
	 private:
		IteratorData();
		IteratorData(List*, ItemWrap*);
		bool equals(const IteratorData& it) const;
		bool is_null() const;
		void prev();
		void next();
		List* _host;
		ItemWrap* _item;
		friend class List;
		friend class IteratorTemplateConst<IteratorData>;
		friend class IteratorTemplate<IteratorData>;
	};
	
 public:
	typedef IteratorTemplateConst<IteratorData> IteratorConst;
	typedef IteratorTemplate<IteratorData> Iterator;
	
	List();
	List(const List&);
	List(List&&);
	List(const std::initializer_list<Item>& list);
	virtual ~List();
	List& operator=(const List&);
	List& operator=(List&&);
	Iterator push(const Item& item);
	Iterator push(Item&& item);
	void push(const List& ls);
	void push(List&& ls);
	Iterator unshift(const Item& item);
	Iterator unshift(Item&& item);
	void unshift(const List& ls);
	void unshift(List&& ls);
	void pop();
	void shift();
	void pop(uint count);
	void shift(uint count);
	Iterator before(IteratorConst it, const Item& item);
	Iterator before(IteratorConst it, Item&& item);
	Iterator after(IteratorConst it, const Item& item);
	Iterator after(IteratorConst it, Item&& item);
	void del(IteratorConst it);
	void clear();
	const Item& first()const;
	const Item& last()const;
	Item& first();
	Item& last();
	IteratorConst find(uint offset) const;
	Iterator find(uint offset);
	IteratorConst find(IteratorConst it, int offset = 0) const;
	Iterator find(IteratorConst it, int offset = 0);
	String join(cString& sp) const;
	IteratorConst begin() const;
	IteratorConst end() const;
	Iterator begin();
	Iterator end();
	uint length() const;
	
 private:
	ItemWrap* _first;
	ItemWrap* _last;
	uint      _length;
};

FX_END

#include "list.inl"

#endif
