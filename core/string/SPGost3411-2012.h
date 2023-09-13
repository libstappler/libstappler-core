/**
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

#ifndef CORE_CORE_STRING_SPGOST3411_2012_H_
#define CORE_CORE_STRING_SPGOST3411_2012_H_

#include "SPSha.h"

namespace stappler::crypto {

union alignas(16) uint512_u {
    unsigned long long QWORD[8];
    unsigned char B[64];
};

/* GOST R 34.11-2012 hash context */
struct Gost3411_Ctx {
    uint512_u buffer;
    uint512_u h;
    uint512_u N;
    uint512_u Sigma;
    size_t bufsize;
    unsigned int digest_size;
};

struct Gost3411_512 {
	using _Ctx = Gost3411_Ctx;

	constexpr static uint32_t Length = 64;
	using Buf = std::array<uint8_t, Length>;

	template <typename ... Args>
	static Buf perform(Args && ... args);

	Gost3411_512();
	Gost3411_512 & init();

	Gost3411_512 & update(const uint8_t *, size_t);
	Gost3411_512 & update(const CoderSource &);

	template  <typename T, typename ... Args>
	void _update(T && t, Args && ... args);

	template  <typename T>
	void _update(T && t);

	Buf final();
	void final(uint8_t *);

	_Ctx ctx;
};

struct Gost3411_256 {
	using _Ctx = Gost3411_Ctx;

	constexpr static uint32_t Length = 32;
	using Buf = std::array<uint8_t, Length>;

	template <typename ... Args>
	static Buf perform(Args && ... args);

	Gost3411_256();
	Gost3411_256 & init();

	Gost3411_256 & update(const uint8_t *, size_t);
	Gost3411_256 & update(const CoderSource &);

	template  <typename T, typename ... Args>
	void _update(T && t, Args && ... args);

	template  <typename T>
	void _update(T && t);

	Buf final();
	void final(uint8_t *);

	_Ctx ctx;
};

template <typename ... Args>
inline Gost3411_512::Buf Gost3411_512::perform(Args && ... args) {
	Gost3411_512 c;
	c._update(std::forward<Args>(args)...);
	return c.final();
}

template  <typename T, typename ... Args>
inline void Gost3411_512::_update(T && t, Args && ... args) {
	update(std::forward<T>(t));
	_update(std::forward<Args>(args)...);
}

template  <typename T>
inline void Gost3411_512::_update(T && t) {
	update(std::forward<T>(t));
}

template <typename ... Args>
inline Gost3411_256::Buf Gost3411_256::perform(Args && ... args) {
	Gost3411_256 c;
	c._update(std::forward<Args>(args)...);
	return c.final();
}

template  <typename T, typename ... Args>
inline void Gost3411_256::_update(T && t, Args && ... args) {
	update(std::forward<T>(t));
	_update(std::forward<Args>(args)...);
}

template  <typename T>
inline void Gost3411_256::_update(T && t) {
	update(std::forward<T>(t));
}

}

#endif /* CORE_CORE_STRING_SPGOST3411_2012_H_ */
