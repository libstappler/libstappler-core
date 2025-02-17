/**
Copyright (c) 2017-2022 Roman Katuntsev <sbkarr@stappler.org>
Copyright (c) 2023 Stappler LLC <admin@stappler.dev>

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
**/

#ifndef STAPPLER_CORE_MEMORY_SPMEMRBTREE_H_
#define STAPPLER_CORE_MEMORY_SPMEMRBTREE_H_

#include "SPMemAlloc.h"

#define SP_MEM_RBTREE_DEBUG 0

namespace STAPPLER_VERSIONIZED stappler::memory::rbtree {

enum NodeColor : uintptr_t {
	Red = 0,
	Black = 1
};

template <typename Value>
using Storage = memory::Storage<Value>;

struct SP_PUBLIC NodeBase : public AllocPool {
	struct Flag {
		uintptr_t color : 1;
		uintptr_t prealloc : 1;
		uintptr_t index : (sizeof(uintptr_t) / 2) * 8 - 2;
		uintptr_t size : (sizeof(uintptr_t) / 2) * 8;
	};

	NodeBase *parent = nullptr;
	NodeBase *left = nullptr;
	NodeBase *right = nullptr;
	Flag flag;

	NodeBase() noexcept : flag(Flag{0, 0, 0, 0}) { }
	NodeBase(NodeColor c) noexcept : flag(Flag{uintptr_t(c ? 1 : 0), 0, 0, 0}) { }

	inline void setColor(NodeColor c) { flag.color = c ? 1 : 0; }
	inline NodeColor getColor() const { return NodeColor(flag.color); }

	inline void setPrealloc(bool v) { flag.prealloc = v ? 1 : 0; }
	inline bool isPrealloc() const { return flag.prealloc != 0; }

	inline void setSize(uintptr_t s) { flag.size = s; }
	inline uintptr_t getSize() const { return flag.size; }

	inline void setIndex(uintptr_t s) { flag.index = s; }
	inline uintptr_t getIndex() const { return flag.index; }

	static inline NodeBase * min (NodeBase * x) {
		while (x->left != 0) x = x->left;
		return x;
	}

	static inline const NodeBase * min(const NodeBase * x) {
		while (x->left != 0) x = x->left;
		return x;
	}

	static inline NodeBase * max(NodeBase * x) {
		while (x->right != 0) x = x->right;
		return x;
	}

	static inline const NodeBase * max(const NodeBase * x) {
		while (x->right != 0) x = x->right;
		return x;
	}

	static NodeBase * increment(NodeBase *c);
	static const NodeBase * increment(const NodeBase *c);

	static NodeBase * decrement(NodeBase *c);
	static const NodeBase * decrement(const NodeBase *c);

	// replace node in it's place in tree with new one
	static NodeBase *replace(NodeBase *old, NodeBase *n);

	static void insert(NodeBase *head, NodeBase * n);
	static void remove(NodeBase *head, NodeBase * n);
};

template <typename Value>
struct Node : public NodeBase {
	Storage<Value> value;

	static inline Value *cast(NodeBase *n) { return static_cast<Node<Value> *>(n)->value.ptr(); }
	static inline const Value *cast(const NodeBase *n) { return static_cast<const Node<Value> *>(n)->value.ptr(); }
};

template<typename Value>
struct TreeIterator {
	using iterator_category = std::bidirectional_iterator_tag;

	using node_type = Node<Value>;
	using value_type = Value;
	using reference = Value &;
	using pointer = Value *;

	using difference_type = ptrdiff_t;

	using self = TreeIterator<Value>;
	using node_ptr = NodeBase *;
	using link_ptr = Node<Value> *;

	TreeIterator() noexcept : _node() {}

	explicit TreeIterator(node_ptr x) noexcept : _node(x) {}

	reference operator*() const noexcept {return *node_type::cast(_node);}
	pointer operator->() const noexcept {return node_type::cast(_node);}

	self & operator++() noexcept {_node = node_type::increment(_node); return *this;}
	self operator++(int) noexcept {self ret = *this; _node = node_type::increment(_node); return ret;}

	self & operator--() noexcept {_node = node_type::decrement(_node); return *this;}
	self operator--(int) noexcept {self ret = *this; _node = node_type::decrement(_node); return ret;}

	bool operator==(const self & other) const noexcept {return _node == other._node;}
	bool operator!=(const self & other) const noexcept {return _node != other._node;}

	node_ptr _node;
};

template<typename Value>
struct TreeConstIterator {
	using iterator_category = std::bidirectional_iterator_tag;

	using node_type = Node<Value>;
	using value_type = Value;
	using reference = const Value &;
	using pointer = const Value *;

	using iterator = TreeIterator<Value>;

	using difference_type = ptrdiff_t;

	using self = TreeConstIterator<Value>;
	using node_ptr = const NodeBase *;
	using link_ptr = const Node<Value> *;

	TreeConstIterator() noexcept : _node() {}

	explicit TreeConstIterator(node_ptr x) noexcept : _node(x) {}

	TreeConstIterator(const iterator& it) noexcept : _node(it._node) {}

	iterator constcast() const noexcept {return iterator(const_cast<typename iterator::node_ptr>(_node));}

	reference operator*() const noexcept {return *node_type::cast(_node); }
	pointer operator->() const noexcept {return node_type::cast(_node); }

	self & operator++() noexcept {_node = node_type::increment(_node); return *this;}
	self operator++(int) noexcept {self tmp = *this; _node = node_type::increment(_node); return tmp;}

	self & operator--() noexcept {_node = node_type::decrement(_node); return *this;}
	self operator--(int) noexcept {self tmp = *this; _node = node_type::decrement(_node); return tmp;}

	bool operator==(const self & x) const noexcept {return _node == x._node;}
	bool operator!=(const self & x) const noexcept {return _node != x._node;}

	node_ptr _node;
};


template<typename Value> inline bool
operator==(const TreeIterator<Value> & l, const TreeConstIterator<Value> & r) noexcept { return l._node == r._node; }

template<typename Value> inline bool
operator!=(const TreeIterator<Value> & l, const TreeConstIterator<Value> & r) noexcept { return l._node != r._node; }


template <typename Key, typename Value>
struct TreeKeyExtractor;

template <typename Key>
struct TreeKeyExtractor<Key, Key> {
	static inline const Key & extract(const Key &k) noexcept { return k; }

	template <typename A, typename ... Args>
	static inline void construct(A &alloc, Node<Key> *node, const Key &key, Args && ... args) noexcept {
		alloc.construct(node->value.ptr(), key);
	}

	template <typename A, typename ... Args>
	static inline void construct(A &alloc, Node<Key> *node, Key &&key, Args && ... args) noexcept {
		alloc.construct(node->value.ptr(), sp::move_unsafe(key));
	}
};

template <typename Key, typename Value>
struct TreeKeyExtractor<Key, Pair<Key, Value>> {
	static inline const Key & extract(const Pair<Key, Value> &k) noexcept { return k.first; }

	template <typename A, typename ... Args>
	static inline void construct(A &alloc, Node<Pair<Key, Value>> *node, const Key &k, Args && ... args) noexcept {
		alloc.construct(node->value.ptr(),
				std::piecewise_construct,
				std::forward_as_tuple(k),
				std::forward_as_tuple(std::forward<Args>(args)...));
	}

	template <typename A, typename ... Args>
	static inline void construct(A &alloc, Node<Pair<Key, Value>> *node, Key &&k, Args && ... args) noexcept {
		alloc.construct(node->value.ptr(),
				std::piecewise_construct,
				std::forward_as_tuple(sp::move_unsafe(k)),
				std::forward_as_tuple(std::forward<Args>(args)...));
	}
};

template <typename Key, typename Value>
struct TreeKeyExtractor<Key, Pair<const Key, Value>> {
	static inline const Key & extract(const Pair<const Key, Value> &k) noexcept { return k.first; }

	template <typename A, typename ... Args>
	static inline void construct(A &alloc, Node<Pair<const Key, Value>> *node, const Key &k, Args && ... args) noexcept {
		alloc.construct(node->value.ptr(),
				std::piecewise_construct,
				std::forward_as_tuple(k),
				std::forward_as_tuple(std::forward<Args>(args)...));
	}

	template <typename A, typename ... Args>
	static inline void construct(A &alloc, Node<Pair<const Key, Value>> *node, Key &&k, Args && ... args) noexcept {
		alloc.construct(node->value.ptr(),
				std::piecewise_construct,
				std::forward_as_tuple(sp::move_unsafe(k)),
				std::forward_as_tuple(std::forward<Args>(args)...));
	}
};

namespace impl {

template<typename T, typename ...P>
struct dependent_type {
	using type = T;
};

template<typename A, typename ...B>
using void_type = typename dependent_type<void, A, B...>::type;

template<typename DummyVoid, template<typename ...> typename A, typename ...B>
struct is_detected : std::false_type {};

template<template<typename ...> typename A, typename ...B>
struct is_detected<void_type<A<B...>>, A, B...> : std::true_type {};

template <template <typename...> typename A, typename ...B>
inline constexpr bool is_detected_v = impl::is_detected<void, A, B...>::value;

}

template <typename T> using DetectTransparent = typename T::is_transparent;

template <typename Key, typename Value, typename Comp = std::less<>>
class Tree : public AllocPool {
public:
	using value_type = Value;
	using node_type = Node<Value>;
	using node_ptr = Node<Value> *;
	using base_type = NodeBase *;
	using const_node_ptr = const node_type *;

	using node_allocator_type = Allocator<node_type>;
	using value_allocator_type = Allocator<value_type>;
	using comparator_type = Comp;

	using iterator = TreeIterator<Value>;
	using const_iterator = TreeConstIterator<Value>;

	using reverse_iterator = std::reverse_iterator<iterator>;
	using const_reverse_iterator = std::reverse_iterator<const_iterator>;

public:
	Tree(const Comp &comp = Comp(), const value_allocator_type &alloc = value_allocator_type()) noexcept
	: _header(NodeColor::Black), _comp(comp), _allocator(alloc), _size(0) { }

	Tree(const Tree &other, const value_allocator_type &alloc = value_allocator_type()) noexcept
	: _header(NodeColor::Black), _comp(other._comp), _allocator(alloc), _size(0) {
		clone(other);
	}

	Tree(Tree &&other, const value_allocator_type &alloc = value_allocator_type()) noexcept
	: _header(NodeColor::Black), _comp(other._comp), _allocator(alloc), _size(0) {
		if (other.get_allocator() == _allocator) {
			_header = other._header;
			_size = other._size;
			_comp = sp::move_unsafe(other._comp);
			if (_header.left != nullptr) {
				_header.left->parent = &_header;
			}
			other._header = NodeBase(NodeColor::Black);
			other._size = 0;
		} else {
			clone(other);
		}
	}

	Tree & operator = (const Tree &other) noexcept {
		clone(other);
		return *this;
	}

	Tree & operator = (Tree &&other) noexcept {
		if (other.get_allocator() == _allocator) {
			clear();
			_header = other._header;
			_size = other._size;
			_comp = sp::move_unsafe(other._comp);
			if (_header.left != nullptr) {
				_header.left->parent = &_header;
			}
			other._header = NodeBase(NodeColor::Black);
			other._size = 0;
		} else {
			clone(other);
		}
		return *this;
	}

	~Tree() noexcept {
		if (_size > 0) {
			clear();
		}
		releaseTmp();
	}

	const value_allocator_type & get_allocator() const noexcept { return _allocator; }

	template <typename ... Args>
	Pair<iterator,bool> emplace(Args && ... args) {
		auto ret = insertNodeUnique(std::forward<Args>(args)...);
		return pair(iterator(ret.first), ret.second);
	}

	template <typename ... Args>
	iterator emplace_hint(const_iterator hint, Args && ... args) {
		return iterator(insertNodeUniqueHint(hint, std::forward<Args>(args)...));
	}

	template <typename K, typename ... Args>
	Pair<iterator,bool> try_emplace(K &&k, Args && ... args) {
		auto ret = tryInsertNodeUnique(std::forward<K>(k), std::forward<Args>(args)...);
		return pair(iterator(ret.first), ret.second);
	}

	template <typename K, typename ... Args>
	iterator try_emplace(const_iterator hint, K &&k, Args && ... args) {
		return iterator(tryInsertNodeUniqueHint(hint, std::forward<K>(k), std::forward<Args>(args)...));
	}

	template <typename K, class M>
	Pair<iterator, bool> insert_or_assign(K&& k, M&& m) {
		auto ret = tryAssignNodeUnique(std::forward<K>(k), std::forward<M>(m));
		return pair(iterator(ret.first), ret.second);
	}

	template <typename K, class M>
	iterator insert_or_assign(const_iterator hint, K&& k, M&& m) {
		return iterator(tryAssignNodeUniqueHint(hint, std::forward<K>(k), std::forward<M>(m)));
	}

	iterator erase( const_iterator pos ) {
		if (pos._node != &_header) {
			auto next = NodeBase::increment(pos.constcast()._node);
			deleteNode(const_cast<NodeBase *>(pos._node));
			return iterator(next);
		}
		return pos.constcast();
	}

	iterator erase( const_iterator first, const_iterator last ) {
		for (auto it = first; it != last; it ++) {
			deleteNode(const_cast<NodeBase *>(it._node));
		}
		return last;
	}

	size_t erase_unique(const Key &key) {
		auto node = find_impl(key);
		if (node) {
			deleteNode(node);
			return 1;
		}
		return 0;
	}

	iterator begin() noexcept { return iterator(_header.left?left():&_header); }
	iterator end() noexcept { return iterator(&_header); }

	const_iterator begin() const noexcept { return const_iterator(_header.left?left():&_header); }
	const_iterator end() const noexcept { return const_iterator(&_header); }

	reverse_iterator rbegin() noexcept { return reverse_iterator(end()); }
	reverse_iterator rend() noexcept { return reverse_iterator(begin()); }

	const_reverse_iterator rbegin() const noexcept { return const_reverse_iterator(end()); }
	const_reverse_iterator rend() const noexcept { return const_reverse_iterator(begin()); }

	const_iterator cbegin() const noexcept { return const_iterator(_header.left?left():&_header); }
	const_iterator cend() const noexcept { return const_iterator(&_header); }

	const_reverse_iterator crbegin() const noexcept { return const_reverse_iterator(cend()); }
	const_reverse_iterator crend() const noexcept { return const_reverse_iterator(cbegin()); }

	void clear() noexcept {
		if (_header.left) {
			clear_visit(static_cast<Node<Value> *>(_header.left));
		}
		_header.left = nullptr;
		_header.right = nullptr;
		_header.parent = nullptr;
		_size = 0;
	}

	void shrink_to_fit() noexcept {
		releaseTmp();
	}

	size_t capacity() const noexcept {
		return _size + _header.flag.size;
	}

	size_t size() const noexcept {
		return _size;
	}

	bool empty() const noexcept {
		return _header.left == nullptr;
	}

	void set_memory_persistent(bool value) noexcept {
		_header.flag.prealloc = value ? 1 : 0;
	}

	bool memory_persistent() const noexcept {
		return _header.flag.prealloc;
	}

	void swap(Tree &other) noexcept {
		std::swap(_header, other._header);
		std::swap(_allocator, other._allocator);
		std::swap(_size, other._size);
		std::swap(_comp, other._comp);
		std::swap(_tmp, other._tmp);
	}

	template< class K > iterator find( const K& x ) {
		auto ptr = find_impl(x);
		return (ptr) ? iterator(ptr) : end();
	}

	template< class K > const_iterator find( const K& x ) const {
		auto ptr = find_impl(x);
		return (ptr) ? const_iterator(ptr) : end();
	}

	template< class K >
	iterator lower_bound(const K& x) {
		auto ptr = lower_bound_ptr(x);
		return (ptr) ? iterator(ptr) : end();
	}

	template< class K >
	const_iterator lower_bound(const K& x) const {
		auto ptr = lower_bound_ptr(x);
		return (ptr) ? const_iterator(ptr) : end();
	}

	template< class K >
	iterator upper_bound( const K& x ) {
		auto ptr = upper_bound_ptr(x);
		return (ptr) ? iterator(ptr) : end();
	}

	template< class K >
	const_iterator upper_bound( const K& x ) const {
		auto ptr = upper_bound_ptr(x);
		return (ptr) ? const_iterator(ptr) : end();
	}
	template< class K >
	Pair<iterator,iterator> equal_range( const K& x ) {
		return pair(lower_bound(x), upper_bound(x));
	}

	template< class K >
	Pair<const_iterator,const_iterator> equal_range( const K& x ) const {
		return pair(lower_bound(x), upper_bound(x));
	}

	template< class K >
	size_t count( const K& x ) const {
		return count_impl(x);
	}

	template< class K >
	size_t count_unique( const K& x ) const {
		return findNode(x)?1:0;
	}

	void reserve(size_t c) {
		// requested count is greater then size + pending preallocated nodes
		if (c > _size + _header.flag.size) {
			allocateTmp(c - _size);
		}
	}

protected:
	friend class TreeDebug;

	// header values has a special meanings:
	// _header.parent - left node from root (first node of iteration)
	// _header.right - right node from root
	// _header.left - root node (actual root of the tree), nullptr if tree is not defined
	// &_header (pointer to the header) - last node in iteration
	// _header.size - extra capacity, available via _tmp
	// _header.index - count of preallocated blocks in use
	// _header.prealloc - flag of persistent mode (enabled if 1, disabled by default)

	NodeBase _header; // root is _header.left
	comparator_type _comp;
	value_allocator_type _allocator;
	size_t _size = 0;
	Node<Value> *_tmp = nullptr;

	inline node_ptr root() { return static_cast<node_ptr>(_header.left); }
	inline const_node_ptr root() const { return static_cast<const_node_ptr>(_header.left); }
	inline void setroot(base_type n) { _header.left = n; n->parent = &_header; }

	inline node_ptr left() { return static_cast<node_ptr>(_header.parent); }
	inline const_node_ptr left() const { return static_cast<const_node_ptr>(_header.parent); }
	inline void setleft(base_type n) { _header.parent = (n == &_header)?nullptr:n; }

	inline node_ptr right() { return static_cast<node_ptr>(_header.right); }
	inline const_node_ptr right() const { return static_cast<const_node_ptr>(_header.right); }
	inline void setright(base_type n) { _header.right = (n == &_header)?nullptr:n; }


	inline const Key & extract(const Value &val) const { return TreeKeyExtractor<Key, Value>::extract(val); }
	inline const Key & extract(const Storage<Value> &s) const { return extract(s.ref()); }
	inline const Key & extract(const NodeBase *s) const { return extract(static_cast<const Node<Value> *>(s)->value.ref()); }

	inline bool compareLtKey(const Key &l, const Key &r) const { return _comp(l, r); }
	inline bool compareEqKey(const Key &l, const Key &r) const { return !compareLtKey(l, r) && !compareLtKey(r, l); }

	template <typename A, typename B>
	inline bool compareLtTransparent(const A &l, const B &r) const {
		if constexpr (impl::is_detected_v<DetectTransparent, Comp>) {
			return _comp(l, r);
		} else if constexpr (std::is_same_v<A, B>) {
			return compareLtKey(l, r);
		} else {
			static_assert("Comparator should be transparent or search key and stored key types must be the same");
			return false;
		}
	}

	inline bool compareEqValue(const Value &l, const Value &r) const { return compareEqKey(extract(l), extract(r)); }
	inline bool compareLtValue(const Value &l, const Value &r) const { return compareLtKey(extract(l), extract(r)); }

	struct InsertData {
		const Key *key;
		Node<Value> *val;
		NodeBase * current;
		NodeBase * parent;
		bool isLeft;
	};

	template <typename ... Args> InsertData
	constructNode(Args && ... args) {
		Node<Value> * ret = allocateNode();
		ret->parent = nullptr;
		ret->left = nullptr;
		ret->right = nullptr;
		ret->setColor(NodeColor::Red);
		_allocator.construct(ret->value.ptr(), std::forward<Args>(args)...);

		return InsertData{&extract(ret->value), ret, nullptr, nullptr, false};
	}

	InsertData constructKey(const Key &k) {
		return InsertData{&k, nullptr, nullptr, nullptr, false};
	}

	InsertData constructKey(Key &&k) {
		return InsertData{&k, nullptr, nullptr, nullptr, false};
	}

	template <typename K, typename ... Args>
	Node<Value> *constructEmplace(K &&k, Args && ... args) {
		Node<Value> * ret = allocateNode();
		ret->parent = nullptr;
		ret->left = nullptr;
		ret->right = nullptr;
		ret->setColor(NodeColor::Red);

		TreeKeyExtractor<Key, Value>::construct(_allocator, ret, std::forward<K>(k), std::forward<Args>(args)...);
		return ret;
	}

	template <typename M>
	void constructAssign(Node<Value> *n, const M &m) {
		n->value.ref() = m;
	}

	template <typename M>
	void constructAssign(Node<Value> *n, M &&m) {
		n->value.ref() = sp::move_unsafe(m);
	}

	bool getInsertPositionUnique_search(InsertData &d) {
	    while (d.current != nullptr) {
	    	d.parent = d.current;
	        if (compareLtKey(*(d.key), extract(d.current))) {
	        	d.isLeft = true;
	        	d.current = static_cast<Node<Value> *> (d.current->left);
	        } else {
	        	if (!compareLtKey(extract(d.current), *(d.key))) { // equality check
	        		return false;
	        	}
	        	d.isLeft = false;
	        	d.current = static_cast<Node<Value> *> (d.current->right);
	        }
	    }
	    return true;
	}

	bool getInsertPosition_tryRoot(InsertData &d) {
		if (_size == 0) {
			d.parent = nullptr;
			d.isLeft = true;
			d.current = nullptr;
			return true;
		}
		return false;
	}

	bool getInsertPositionUnique_tryHint(InsertData &d) {
		if (d.current == nullptr) {
			return false; // no hint provided
		}

		// check if hint is special value (begin or end)
		if (d.current == left() || static_cast<NodeBase *>(d.current) == &_header) {
			d.current = nullptr;
			return false; // this cases served by next two functions
		}

		// check if we can insert just before hint
		auto h = static_cast<Node<Value> *>(d.current);
		if (compareLtKey(*(d.key), extract(h->value))) {
			// check for bound
			auto p = static_cast<node_ptr>(NodeBase::decrement(d.current));
			if (compareLtKey(extract(p->value), *(d.key))) {
				d.parent = d.current;
				d.current = d.current->left;
				d.isLeft = true;
				if (!getInsertPositionUnique_search(d)) {
					return true; // stop searching, non-unique position, current is not null
				}
				return true;
			}
		} else if (compareLtKey(extract(h->value), *(d.key))) {
			auto p = static_cast<node_ptr>(NodeBase::increment(d.current));
			if (static_cast<NodeBase *>(p) == &_header) { // insert back case
				d.parent = d.current;
				d.current = d.current->right;
				d.isLeft = false;
				return true;
			} else if (compareLtKey(*(d.key), extract(p->value))) {
				d.parent = d.current;
				d.current = d.current->right;
				d.isLeft = false;
				if (!getInsertPositionUnique_search(d)) {
					return true; // stop searching, non-unique position, current is not null
				}
				return true;
			}
		} else { // hint and new one is equal
			return true; // current is hint, non-unique cancel
		}

		d.current = nullptr;
		return false;
	}

	bool getInsertPositionUnique_tryLeft(InsertData &d) {
		if (auto l = left()) {
			if (compareLtKey(*(d.key), extract(l->value))) {
				d.current = nullptr; d.parent = l; d.isLeft = true;
				return true;
			} else if (!compareLtKey(extract(l->value), *(d.key))) {
				d.current = l;
				return true;
			}
		}
		return false;
	}

	bool getInsertPositionUnique_tryRight(InsertData &d) {
		if (auto r = right()) {
			if (compareLtKey(extract(r->value), *(d.key))) {
				d.current = nullptr; d.parent = r; d.isLeft = false;
				return true;
			} else if (!compareLtKey(*(d.key), extract(r->value))) {
				d.current = r;
				return true;
			}
		}
		return false;
	}

	bool getInsertPositionUnique(InsertData &d) {
		// *_try* functions should return true with non-nullptr d.current
		// if new node is not unique to stop search process
		if (getInsertPosition_tryRoot(d)
				|| getInsertPositionUnique_tryHint(d)
				|| getInsertPositionUnique_tryLeft(d)
				|| getInsertPositionUnique_tryRight(d)) {
			return d.current == nullptr;
		}

		// full-scan
		if (!d.current) { d.current = root(); }
	    /* find where node belongs */
	    return getInsertPositionUnique_search(d);
	}

	template <typename ... Args> Pair<Node<Value> *,bool>
	insertNodeUnique(Args && ... args) {
		InsertData d = constructNode(std::forward<Args>(args)...);
		if (!getInsertPositionUnique(d)) {
			destroyNode(d.val);
			return pair(static_cast<Node<Value> *>(d.current), false);
		}

		return pair(makeInsert(d.val, d.parent, d.isLeft), true);
	}

	template <typename ... Args> Node<Value> *
	insertNodeUniqueHint(const_iterator hint, Args && ... args) {
		InsertData d = constructNode(std::forward<Args>(args)...);
		d.current = hint.constcast()._node;
		if (!getInsertPositionUnique(d)) {
			destroyNode(d.val);
			return static_cast<Node<Value> *>(d.current);
		}

		return makeInsert(d.val, d.parent, d.isLeft);
	}

	template <typename K, typename ... Args> Pair<Node<Value> *,bool>
	tryInsertNodeUnique(K &&k, Args && ... args) {
		InsertData d = constructKey(std::forward<K>(k));
		if (!getInsertPositionUnique(d)) {
			return pair(static_cast<Node<Value> *>(d.current), false);
		}

		return pair(makeInsert(
				constructEmplace(std::forward<K>(k), std::forward<Args>(args)...),
				d.parent, d.isLeft), true);
	}

	template <typename K, typename ... Args> Node<Value> *
	tryInsertNodeUniqueHint(const_iterator hint, K &&k, Args && ... args) {
		InsertData d = constructKey(std::forward<K>(k));
		d.current = hint.constcast()._node;
		if (!getInsertPositionUnique(d)) {
			return static_cast<Node<Value> *>(d.current);
		}

		return makeInsert(constructEmplace(std::forward<K>(k), std::forward<Args>(args)...),
				d.parent, d.isLeft);
	}

	template <typename K, typename M> Pair<Node<Value> *,bool>
	tryAssignNodeUnique(K &&k, M &&m) {
		InsertData d = constructKey(std::forward<K>(k));
		if (!getInsertPositionUnique(d)) {
			constructAssign(d.current, std::forward<M>(m));
			return pair(static_cast<Node<Value> *>(d.current), false);
		}

		return pair(makeInsert(constructEmplace(std::forward<K>(k), std::forward<M>(m)),
				d.parent, d.isLeft), true);
	}

	template <typename K, typename M> Node<Value> *
	tryAssignNodeUniqueHint(const_iterator hint, K &&k, M &&m) {
		InsertData d = constructKey(std::forward<K>(k));
		d.current = hint.constcast()._node;
		if (!getInsertPositionUnique(d)) {
			constructAssign(d.current, std::forward<M>(m));
			return static_cast<Node<Value> *>(d.current);
		}

		return makeInsert(constructEmplace(std::forward<K>(k), std::forward<M>(m)),
				d.parent, d.isLeft);
	}

	Node<Value> * makeInsert(Node<Value> *n, NodeBase *parent, bool isLeft) {
		n->parent = parent;
		if (parent) {
			if (isLeft) {
				if (parent == left())
					setleft(n);
				parent->left = n;
			} else {
				if (parent == right())
					setright(n);
				parent->right = n;
			}
		} else {
			setleft(n);
			setright(n);
			setroot(n);
		}

		NodeBase::insert(&_header, n);
		++ _size;
		return n;
	}

	void deleteNode(NodeBase * z) {
		NodeBase * x = nullptr;
		NodeBase * y = nullptr;

	    if (!z) return;

	    if (!z->left || !z->right) {
	        /* y has a leaf node as a child */
	        y = z;

			if (z == right()) {
				setright((z == left())?nullptr:NodeBase::decrement(z));
			}
			if (z == left()) {
				setleft(NodeBase::increment(z));
			}
	    } else {
	        y = z->left;
	        while (y->right) y = y->right;
	    }

	    /* x is y's only child */
	    if (y->left) {
	        x = y->left;
	    } else {
	        x = y->right;
	    }

	    if (!x) {
	    	// if there is no replacement (we use empty leaf node as new Z),
	    	// we run rebalance with phantom Y node, then swap data and remove links
	    	// to phantom
			if (y->getColor() == NodeColor::Black) {
				NodeBase::remove(&_header, y);
			}

			if (y == y->parent->left)
				y->parent->left = nullptr;
			else
				y->parent->right = nullptr;


			if (y != z) {
				// copy node's data may be expensive, so, we keep data and replace node
				NodeBase::replace(z, y);
			}

	    } else {
	    	// if we have replacement, we insert it at proper place then call rebalance
			x->parent = y->parent;

			if (y == y->parent->left)
				y->parent->left = x;
			else
				y->parent->right = x;

			if (y != z) {
				NodeBase::replace(z, y);
			} else {
				y->setColor(NodeColor::Red);
			}

			if (y->getColor() == NodeColor::Black) {
				NodeBase::remove(&_header, x);
			} else {
				x->setColor(NodeColor::Black);
			}
	    }

	    destroyNode(static_cast<Node<Value> *>(z));
		-- _size;
	}

	void clear_visit(Node<Value> *target) {
 		if (target->left) {
			clear_visit(static_cast<Node<Value> *>(target->left));
		}
		if (target->right) {
			clear_visit(static_cast<Node<Value> *>(target->right));
		}
		destroyNode(target);
	}

	void clone_visit(const Node<Value> *source, Node<Value> *target) {
		_allocator.construct(target->value.ptr(), source->value.ref());
		target->setColor(source->getColor());
		if (source->left) {
			target->left = allocateNode();
			target->left->parent = target;
			clone_visit(static_cast<Node<Value> *>(source->left), static_cast<Node<Value> *>(target->left));
			if (_header.parent == source->left) { // check for leftmost node
				_header.parent = target->left;
			}
		} else {
			target->left = nullptr;
		}

		if (source->right) {
			target->right = allocateNode();
			target->right->parent = target;
			clone_visit(static_cast<Node<Value> *>(source->right), static_cast<Node<Value> *>(target->right));
			if (_header.right == source->right) { // check for rightmost node
				_header.right = target->right;
			}
		} else {
			target->right = nullptr;
		}
	}

	void clone(const Tree &other) {
		clear();

		_size = other._size;
		if (_size) {
			allocateTmp(_size);
		}

		_comp = other._comp;
		_header = other._header;
		if (other._header.left) {
			_header.left = allocateNode();
			_header.left->parent = &_header;
			if (other._header.left == other._header.parent) {
				_header.parent = _header.left;
			}
			if (other._header.left == other._header.right) {
				_header.right = _header.left;
			}
			clone_visit(static_cast<Node<Value> *>(other._header.left), static_cast<Node<Value> *>(_header.left));
		}
	}

	template< class K >
	node_ptr find_impl(const K & x) const {
		const_node_ptr current = root();
	    while(current) {
			auto &key = extract(current);
	        if (compareLtTransparent(x, key)) {
	        	current = static_cast<const_node_ptr> (current->left);
	        } else {
	        	if (!compareLtTransparent(key, x)) { // equality check
	        		return const_cast<node_ptr>(current);
	        	}
	        	current = static_cast<const_node_ptr> (current->right);
	        }
	    }
	    return nullptr;
	}

	template< class K >
	node_ptr lower_bound_ptr(const K& x) const {
		const_node_ptr current = root();
		const_node_ptr saved = nullptr;
	    while (current) {
	        if (!compareLtTransparent(extract(current), x)) {
	        	saved = current;
	        	current = static_cast<const_node_ptr> (current->left);
	        } else {
	        	current = static_cast<const_node_ptr> (current->right);
	        }
	    }
	    return const_cast<node_ptr>(saved);
	}

	template< class K >
	node_ptr upper_bound_ptr(const K& x) const {
		const_node_ptr current = root();
		const_node_ptr saved = current;
	    while (current) {
	        if (compareLtTransparent(x, extract(current))) {
	        	saved = current;
	        	current = static_cast<const_node_ptr> (current->left);
	        } else {
	        	current = static_cast<const_node_ptr> (current->right);
	        }
	    }
	    return const_cast<node_ptr>(saved);
	}

	template< class K >
	size_t count_impl(const K& x) const {
		auto c = find_impl(x);
		if (!c) {
			return 0;
		} else {
			size_t ret = 1;
			const_node_ptr next, current;

			current = c;
    		next = static_cast<const_node_ptr> (NodeBase::decrement(current));
    		while (next && !compareLtTransparent(extract(next), extract(current))) {
    			current = next;
    			next = static_cast<const_node_ptr> (NodeBase::decrement(current));
    			ret ++;
    		}

			current = c;
    		next = static_cast<const_node_ptr> (NodeBase::increment(current));
    		while (next && next != &_header && !compareLtTransparent(extract(current), extract(next))) {
    			current = next;
    			next = static_cast<const_node_ptr> (NodeBase::increment(current));
    			ret ++;
    		}
    		return ret;
		}
	}

	void destroyNode(Node<Value> *n) {
		_allocator.destroy(n->value.ptr());
		if (!_tmp) {
			// no saved node - hold one
			n->parent = nullptr;
			_tmp = n;
			++ _header.flag.size; // increment capacity counter
		} else if (n->isPrealloc() || _header.flag.prealloc) {
			// node was preallocated - hold it in chain
			n->parent = _tmp;
			_tmp = n;
			++ _header.flag.size; // increment capacity counter
		} else {
			// deallocate node
			node_allocator_type(_allocator).__deallocate(n, 1, n->getSize());
		}
	}

	Node<Value> * allocateNode() {
		if (_tmp) {
			auto ret = _tmp;
			_tmp = (Node<Value> *)ret->parent;
			-- _header.flag.size; // decrement capacity counter
			return ret;
		} else {
			size_t s;
			auto ret = node_allocator_type(_allocator).__allocate(1, s);
			ret->setSize(s);
			ret->setPrealloc(false);
			return ret;
		}
	}

	void allocateTmp(size_t count) {
		// preallocate new n nodes

		uintptr_t preallocIdx = ++ _header.flag.index;
		_header.flag.size += count; // increment capacity counter

		size_t s;
		auto ret = node_allocator_type(_allocator).__allocate(count, s);
		auto n = ret;

		for (size_t i = 0; i < count; ++ i) {
			NodeBase *tmpN = n;
			tmpN->parent = n + 1;
			tmpN->setPrealloc(true);
			tmpN->setIndex(preallocIdx);
			if (i < count - 1) {
				tmpN->parent = n + 1;
				tmpN->setSize(sizeof(Node<Value>));
				s -= sizeof(Node<Value>);
			} else {
				tmpN->parent = _tmp;
				n->setSize(s);
			}
			++ n;
		}
		_tmp = ret;
	}

	void releaseTmp() {
		// release any tmp nodes if possible
		// preallocated nodes released in batch as acquired, only if tree is empty

		struct PreallocatedData {
			Node<Value> *head = (Node<Value> *)maxOf<uintptr_t>();
			size_t count = 0;
			size_t size = 0;
		};

		if (_size != 0) {
			// release only free tmps;
			auto ptr = &_tmp;
			while (*ptr) {
				if (!(*ptr)->isPrealloc()) {
					node_allocator_type(_allocator).__deallocate(*ptr, 1, (*ptr)->getSize());
					*ptr = (Node<Value> *)(*ptr)->parent;
					-- _header.flag.size;
				} else {
					ptr = (Node<Value> **)&((*ptr)->parent);
				}
			}
			return;
		}

		if (_header.flag.index == 0) {
			// no preallocated blocks
			while (_tmp) {
				auto ptr = _tmp;
				_tmp = (Node<Value> *)_tmp->parent;
				node_allocator_type(_allocator).__deallocate(ptr, 1, ptr->getSize());
				-- _header.flag.size;
			}
			_tmp = nullptr;
		} else if (_header.flag.index == 1) {
			// all preallocated nodes should be in tmp chain, only one preallocated block
			PreallocatedData data;

			while (_tmp) {
				auto ptr = _tmp;
				_tmp = (Node<Value> *)_tmp->parent;

				if (ptr->isPrealloc()) {
					if (ptr < data.head) {
						data.head = ptr;
					}
					++ data.count;
					data.size += ptr->getSize();
				} else {
					node_allocator_type(_allocator).__deallocate(ptr, 1, ptr->getSize());
					-- _header.flag.size;
				}
			}
			if (data.head != (Node<Value> *)maxOf<uintptr_t>()) {
				_header.flag.size -= data.count;
				node_allocator_type(_allocator).__deallocate(data.head, data.count, data.size);
			}
		} else {
			// multiple preallocated blocks make things complicated
			// VLA, compilers should support C99,
			// MSVC - fuck off, use new[] or std::vector=)
			PreallocatedData data[_header.flag.index];

			while (_tmp) {
				auto ptr = _tmp;
				_tmp = (Node<Value> *)_tmp->parent;

				if (ptr->isPrealloc()) {
					if (ptr < data[ptr->getIndex() - 1].head) {
						data[ptr->getIndex() - 1].head = ptr;
					}
					++ data[ptr->getIndex() - 1].count;
					data[ptr->getIndex() - 1].size += ptr->getSize();
				} else {
					node_allocator_type(_allocator).__deallocate(ptr, 1, ptr->getSize());
					-- _header.flag.size;
				}
			}
			for (size_t i = 0; i < _header.flag.index; ++ i) {
				if (data[i].head != (Node<Value> *)maxOf<uintptr_t>()) {
					_header.flag.size -= data[i].count;
					node_allocator_type(_allocator).__deallocate(data[i].head, data[i].count, data[i].size);
				}
			}
		}
		_tmp = nullptr;
	}
};

}

#if SP_MEM_RBTREE_DEBUG

namespace STAPPLER_VERSIONIZED stappler::memory::rbtree {

class TreeDebug {
public:
	enum class Validation {
		Valid,
		RootIsNotBlack,
		RedChildIntoRedNode,
		DifferentBlackNodeCount,
	};

	template <class T>
	static void visit(const T & tree, std::ostream &stream) {
		typename T::const_node_ptr r = tree.root();
		stream << "visit " << (void *)r << "  header: " << tree._header.left << " | " << tree._header.right << " | " << tree._header.parent;
		stream << "\n";
		if (r) {
			visit(tree, stream, static_cast<typename T::const_node_ptr>(r), 0);
		}
	}

	template <class T>
	static Validation validate(const T & tree) {
		if (tree._header.left && tree._header.left->flag.color == NodeColor::Red) {
			return Validation::RootIsNotBlack;
		} else {
			auto counter = 0;
			auto root = tree._header.left;
			while (root) {
				if (root->flag.color == NodeColor::Black) ++counter;
				root = root->left;
			}
			return validate(counter, tree._header.left, 0);
		}
	}

	//static bool make_test(std::ostream &stream, int size = 128);
	//static bool make_test(std::ostream &stream, const apr::array<int> &insert, const apr::array<int> &erase);

	static bool make_hint_test(std::ostream &stream, int size = 128);
protected:
	template <class T>
	static void visit(const T & tree, std::ostream &stream, typename T::const_node_ptr node, int depth) {
		if (node->left) {
			visit(tree, stream, static_cast<typename T::const_node_ptr>(node->left), depth + 1);
		}
		for (int i = 0; i < depth; i++) {
			stream << "--";
		}
		stream << (void *)node << " l:" << (void *)node->left << " r:" << (void *)node->right	<< " p:"
				<< (void *)node->parent << " v:" << *(node->value.ptr()) << (node->flag.color?" black":" red") << "\n";
		if (node->right) {
			visit(tree, stream, static_cast<typename T::const_node_ptr>(node->right), depth + 1);
		}
	}

	static Validation validate(int counter, NodeBase *node, int path) {
		if (node == nullptr) {
			if (counter != path) {
				return Validation::DifferentBlackNodeCount;
			}
			return Validation::Valid;
		} else {
			if (node->flag.color == NodeColor::Black) {
				auto res = validate(counter, node->left, path + 1);
				if (res == Validation::Valid) {
					res = validate(counter, node->right, path + 1);
				}
				return res;
			} else {
				if ((node->left && node->left->flag.color == NodeColor::Red)
						|| (node->right && node->right->flag.color == NodeColor::Red)) {
					return Validation::RedChildIntoRedNode;
				} else {
					auto res = validate(counter, node->left, path);
					if (res == Validation::Valid) {
						res = validate(counter, node->right, path);
					}
					return res;
				}
			}
		}
	}
};

template<typename CharType> inline std::basic_ostream<CharType> &
operator << (std::basic_ostream<CharType> & os, const TreeDebug::Validation & v) {
	switch(v) {
	case TreeDebug::Validation::Valid: os << "Valid"; break;
	case TreeDebug::Validation::DifferentBlackNodeCount: os << "DifferentBlackNodeCount"; break;
	case TreeDebug::Validation::RedChildIntoRedNode: os << "RedChildIntoRedNode"; break;
	case TreeDebug::Validation::RootIsNotBlack: os << "RootIsNotBlack"; break;
	}
	return os;
}

}

#endif // SP_MEM_RBTREE_DEBUG

#endif /* STAPPLER_CORE_MEMORY_SPMEMRBTREE_H_ */
