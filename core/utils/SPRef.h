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

#ifndef STAPPLER_CORE_UTILS_SPREF_H_
#define STAPPLER_CORE_UTILS_SPREF_H_

#include "SPTime.h"

// enable Ref debug mode to track retain/release sources
#ifndef SP_REF_DEBUG
#define SP_REF_DEBUG 0
#endif

namespace STAPPLER_VERSIONIZED stappler {

struct SP_PUBLIC AtomicCounter {
	AtomicCounter() { _count.store(1); }

	void increment() { ++ _count; }
	bool decrement() { if (_count.fetch_sub(1) == 1) { return true; } return false; }
	uint32_t get() const { return _count.load(); }

	std::atomic<uint32_t> _count;
};

template <typename Interface>
class SP_PUBLIC RefBase : public Interface::AllocBaseType {
public:
	using InterfaceType = Interface;

#if SP_REF_DEBUG
	virtual uint64_t retain();
	virtual void release(uint64_t id);

	void foreachBacktrace(const Callback<void(uint64_t, Time, const std::vector<std::string> &)> &) const;

#else
	uint64_t retain() { _counter.increment(); return 0; }
	void release(uint64_t id) { if (_counter.decrement()) { delete this; } }
#endif

	uint32_t getReferenceCount() const { return _counter.get(); }

	virtual ~RefBase() { }

protected:
	RefBase() { }

#if SP_REF_DEBUG
	virtual bool isRetainTrackerEnabled() const {
		return false;
	}
#endif

	AtomicCounter _counter;
};

namespace memleak {

SP_PUBLIC uint64_t getNextRefId();

SP_PUBLIC uint64_t retainBacktrace(const RefBase<memory::StandartInterface> *);
SP_PUBLIC void releaseBacktrace(const RefBase<memory::StandartInterface> *, uint64_t);
SP_PUBLIC void foreachBacktrace(const RefBase<memory::StandartInterface> *,
		const Callback<void(uint64_t, Time, const std::vector<std::string> &)> &);

SP_PUBLIC uint64_t retainBacktrace(const RefBase<memory::PoolInterface> *);
SP_PUBLIC void releaseBacktrace(const RefBase<memory::PoolInterface> *, uint64_t);
SP_PUBLIC void foreachBacktrace(const RefBase<memory::PoolInterface> *,
		const Callback<void(uint64_t, Time, const std::vector<std::string> &)> &);
}

template <typename _Base, typename _Pointer>
class RcBase {
public:
	using Base = _Base;
	using Type = typename std::remove_cv<_Base>::type;
	using Pointer = _Pointer;

	inline RcBase() : _ptr(nullptr) { }
	inline RcBase(const nullptr_t &) : _ptr(nullptr) { }
	inline RcBase(const Pointer &value) : _ptr(value) { doRetain(); }
	inline RcBase(const RcBase<Base, Pointer> &v) { _ptr = v._ptr; doRetain(); }
	inline RcBase(RcBase<Base, Pointer> &&v) {
		_ptr = v._ptr; v._ptr = nullptr;
#if SP_REF_DEBUG
		_id = v._id; v._id = 0;
#endif
	}

	inline RcBase & operator = (const nullptr_t &) {
		doRelease();
		_ptr = nullptr;
#if SP_REF_DEBUG
		_id = 0;
#endif
		return *this;
	}

	inline RcBase & operator = (const Pointer &value) { set(value); return *this; }
	inline RcBase & operator = (const RcBase<Base, Pointer> &v) { set(v._ptr); return *this; }
	inline RcBase & operator = (RcBase<Base, Pointer> &&v) {
		doRelease();
		_ptr = v._ptr; v._ptr = nullptr;
#if SP_REF_DEBUG
		_id = v._id; v._id = 0;
#endif
		return *this;
	}

	inline ~RcBase() { doRelease(); _ptr = nullptr; }

	inline void set(const Pointer &value) {
		_ptr = doSwap(value);
	}

	inline void swap(RcBase<Base, Pointer> & v) { auto ptr = _ptr; _ptr = v._ptr; v._ptr = ptr; }

	inline bool operator == (const RcBase<Base, Pointer> & other) const { return _ptr == other._ptr; }
	inline bool operator == (const Base * & other) const { return _ptr == other; }
	inline bool operator == (typename std::remove_const<Base>::type * other) const { return _ptr == other; }
	inline bool operator == (const std::nullptr_t other) const { return _ptr == other; }

	inline bool operator != (const RcBase<Base, Pointer> & other) const { return _ptr != other._ptr; }
	inline bool operator != (const Base * & other) const { return _ptr != other; }
	inline bool operator != (typename std::remove_const<Base>::type * other) const { return _ptr != other; }
	inline bool operator != (const std::nullptr_t other) const { return _ptr != other; }

	inline bool operator > (const RcBase<Base, Pointer> & other) const { return _ptr > other._ptr; }
	inline bool operator > (const Base * other) const { return _ptr > other; }
	inline bool operator > (typename std::remove_const<Base>::type * other) const { return _ptr > other; }
	inline bool operator > (const std::nullptr_t other) const { return _ptr > other; }

	inline bool operator < (const RcBase<Base, Pointer> & other) const { return _ptr < other._ptr; }
	inline bool operator < (const Base * other) const { return _ptr < other; }
	inline bool operator < (typename std::remove_const<Base>::type * other) const { return _ptr < other; }
	inline bool operator < (const std::nullptr_t other) const { return _ptr < other; }

	inline bool operator >= (const RcBase<Base, Pointer> & other) const { return _ptr >= other._ptr; }
	inline bool operator >= (const Base * other) const { return _ptr >= other; }
	inline bool operator >= (typename std::remove_const<Base>::type * other) const { return _ptr >= other; }
	inline bool operator >= (const std::nullptr_t other) const { return _ptr >= other; }

	inline bool operator <= (const RcBase<Base, Pointer> & other) const { return _ptr <= other._ptr; }
	inline bool operator <= (const Base * other) const { return _ptr <= other; }
	inline bool operator <= (typename std::remove_const<Base>::type * other) const { return _ptr <= other; }
	inline bool operator <= (const std::nullptr_t other) const { return _ptr <= other; }

#if SP_REF_DEBUG
	uint64_t getId() const { return _id; }
#endif
protected:
	inline void doRetain() {
#if SP_REF_DEBUG
		if (_ptr) { _id = _ptr->retain(); }
#else
		if (_ptr) { _ptr->retain(); }
#endif
	}

	inline void doRelease() {
#if SP_REF_DEBUG
		if (_ptr) { _ptr->release(_id); }
#else
		if (_ptr) { _ptr->release(0); }
#endif
	}

	inline Pointer doSwap(Pointer value) {
#if SP_REF_DEBUG
		uint64_t id = 0;
		if (value) { id = value->retain(); }
		if (_ptr) { _ptr->release(_id); }
		_id = id;
		return value;
#else
		if (value) { value->retain(); }
		if (_ptr) { _ptr->release(0); }
		return value;
#endif
	}

	// unsafe
	inline RcBase(Pointer value, bool v) : _ptr(value) { }

	Pointer _ptr = nullptr;
#if SP_REF_DEBUG
	uint64_t _id = 0;
#endif
};

template <typename _Base>
class Rc final : public RcBase<_Base, typename std::remove_cv<_Base>::type *> {
public:
	using Parent = RcBase<_Base, typename std::remove_cv<_Base>::type *>;
	using Self = Rc<_Base>;
	using Type = typename std::remove_cv<_Base>::type;

	template <class... Args>
	static inline Self create(Args && ... args) {
		auto pRet = new Type();
	    if (pRet->init(std::forward<Args>(args)...)) {
	    	return Self(pRet, true); // unsafe assignment
		} else {
			delete pRet;
			return Self(nullptr);
		}
	}

	static inline Self alloc() {
		return Self(new Type(), true);
	}

	template <class... Args>
	static inline Self alloc(Args && ... args) {
		return Self(new Type(std::forward<Args>(args)...), true);
	}

	using Parent::Parent;
	using Parent::operator =;

	template <typename B, typename std::enable_if<std::is_convertible<B*, _Base *>{}>::type* = nullptr>
	inline Rc & operator = (const Rc<B> &value) { this->set(value); return *this; }

	// Direct call of `get` should not be on empty storage
	_Base *get() const {
#if SP_REF_DEBUG
		assert(_ptr);
#endif
		return this->_ptr;
	}

	inline operator _Base * () const { return get(); }

	inline _Base * operator->() const { return this->_ptr; }

	inline explicit operator bool () const { return this->_ptr != nullptr; }

	template <typename B, typename std::enable_if<std::is_convertible<_Base *, B*>{}>::type* = nullptr>
	inline operator Rc<B> () { return Rc<B>(static_cast<B *>(get())); }

	inline void swap(Self & v) { auto ptr = this->_ptr; this->_ptr = v._ptr; v._ptr = ptr; }

	template <typename Target>
	inline Rc<Target> cast() const {
		if (auto v = dynamic_cast<Target *>(this->_ptr)) {
			return Rc<Target>(v);
		}
		return Rc<Target>(nullptr);
	}
};

#if SP_REF_DEBUG

template <typename Interface>
uint64_t RefBase<Interface>::retain() {
	_counter.increment();
	if (isRetainTrackerEnabled()) {
		return memleak::retainBacktrace(this);
	}
	return 0;
}

template <typename Interface>
void RefBase<Interface>::release(uint64_t v) {
	if (isRetainTrackerEnabled()) {
		memleak::releaseBacktrace(this, v);
	}
	if (_counter.decrement()) {
		delete this;
	}
}

template <typename Interface>
void RefBase<Interface>::foreachBacktrace(const Callback<void(uint64_t, Time, const std::vector<std::string> &)> &cb) const {
	memleak::foreachBacktrace(this, cb);
}

#endif

}

namespace STAPPLER_VERSIONIZED stappler::mem_std {

using Ref = RefBase<memory::StandartInterface>;

}

namespace STAPPLER_VERSIONIZED stappler::mem_pool {

using Ref = RefBase<memory::PoolInterface>;

}

#endif /* STAPPLER_CORE_UTILS_REF_SPREF_H_ */
