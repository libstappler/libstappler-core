/**
Copyright (c) 2022 Roman Katuntsev <sbkarr@stappler.org>
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

#include "SPCrypto.h"
#include "SPString.h"
#include "SPValid.h"

namespace stappler::crypto {

AesKey makeAesKey(BytesView pkey, BytesView hash, uint32_t version) {
	crypto::PrivateKey pk(pkey);
	if (pk && version > 0) {
		auto ret = makeAesKey(pk, hash, version);
		if (ret.version == 0) {
			ret.data = string::Sha256().update(hash).update(pkey).final();
		}
		return ret;
	} else {
		return AesKey { string::Sha256().update(hash).update(pkey).final(), 0 };
	}
}

AesKey makeAesKey(const PrivateKey &pkey, BytesView hash, uint32_t version) {
	AesKey ret;
	if (version == 2) {
		if (!pkey.sign([&] (const uint8_t *ptr, size_t s) {
			ret.data = string::Sha256().update(BytesView(ptr, s)).final();
			ret.version = version;
		}, hash, crypto::SignAlgorithm::RSA_SHA512)) {
			ret.version = 0;
		}
	} else if (version == 1) {
		if (!pkey.sign([&] (const uint8_t *ptr, size_t s) {
			s = std::min(s, size_t(256));
			ret.data = string::Sha256().update(BytesView(ptr, s)).final();
			ret.version = version;
		}, hash, crypto::SignAlgorithm::RSA_SHA512)) {
			ret.version = 0;
		}
	} else {
		ret.version = 0;
	}
	return ret;
}

uint32_t getAesVersion(BytesView val) {
	val.offset(8);
	return val.readUnsigned32();
}

}
