/**
Copyright (c) 2021-2022 Roman Katuntsev <sbkarr@stappler.org>
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

#ifndef STAPPLER_CRYPTO_SPCRYPTO_H_
#define STAPPLER_CRYPTO_SPCRYPTO_H_

#include "SPIO.h"
#include "SPBytesView.h"
#include "SPSha.h"

#if __CDT_PARSER__

typedef struct mbedtls_pk_context mbedtls_pk_context;

struct sp_pk_context {
	void *info = nullptr;
	void *ctx = nullptr;

	operator mbedtls_pk_context() const;
};

using sp_pubkey_t = sp_pk_context;
using sp_privkey_t = sp_pk_context;
#endif

#if SP_CRYPTO_GNUTLS
struct gnutls_pubkey_st;
typedef struct gnutls_pubkey_st *sp_pubkey_t;

struct gnutls_privkey_st;
typedef struct gnutls_privkey_st *sp_privkey_t;

#elif SP_CRYPTO_OPENSSL

typedef struct evp_pkey_st *sp_pubkey_t;
typedef struct evp_pkey_st *sp_privkey_t;

#elif SP_CRYPTO_MBEDTLS

typedef struct mbedtls_pk_context mbedtls_pk_context;

struct sp_pk_context {
	void *info = nullptr;
	void *ctx = nullptr;

	operator mbedtls_pk_context() const;
};

using sp_pubkey_t = sp_pk_context;
using sp_privkey_t = sp_pk_context;

#endif

namespace stappler::crypto {

class PublicKey;

enum class SignAlgorithm {
	RSA_SHA256,
	RSA_SHA512,
	ECDSA_SHA256,
	ECDSA_SHA512,
};

enum class KeyBits {
	_1024,
	_2048,
	_4096
};

enum class KeyFormat {
	PKCS1,
	PKCS8,
	RSA = PKCS1,
};

struct AesKey {
	std::array<uint8_t, 32> data;
	uint32_t version; // keygen version
};

class PrivateKey {
public:
	PrivateKey();
	PrivateKey(BytesView, const CoderSource & passwd = CoderSource());
	~PrivateKey();

	PrivateKey(const PrivateKey &) = delete;
	PrivateKey& operator=(const PrivateKey &) = delete;

	bool generate(KeyBits = KeyBits::_2048);

	bool import(BytesView, const CoderSource & passwd = CoderSource());

	PublicKey exportPublic() const;

	sp_privkey_t getKey() const { return _key; }

	operator bool () const { return _valid && _loaded; }

	bool exportPem(const Callback<void(const uint8_t *, size_t)> &, KeyFormat = KeyFormat::PKCS8, CoderSource passPhrase = StringView()) const;
	bool exportDer(const Callback<void(const uint8_t *, size_t)> &, KeyFormat = KeyFormat::PKCS8, CoderSource passPhrase = StringView()) const;

	bool sign(const Callback<void(const uint8_t *, size_t)> &, CoderSource, SignAlgorithm = SignAlgorithm::RSA_SHA512) const;
	bool verify(CoderSource data, BytesView signature, SignAlgorithm) const;

	bool isSupported(KeyFormat) const;

protected:
	bool _loaded = false;
	bool _valid = false;
	sp_privkey_t _key;
};

class PublicKey {
public:
	PublicKey();
	PublicKey(BytesView);
	PublicKey(const PrivateKey &);
	~PublicKey();

	PublicKey(const PublicKey &) = delete;
	PublicKey& operator=(const PublicKey &) = delete;

	bool import(BytesView);
	bool importOpenSSH(StringView);

	sp_pubkey_t getKey() const { return _key; }

	operator bool () const { return _valid && _loaded; }

	bool exportPem(const Callback<void(const uint8_t *, size_t)> &) const; // only pkcs8
	bool exportDer(const Callback<void(const uint8_t *, size_t)> &) const; // only pkcs8

	bool verify(CoderSource data, BytesView signature, SignAlgorithm) const;

protected:
	bool _loaded = false;
	bool _valid = false;
	sp_pubkey_t _key;
};

bool encryptAes(const AesKey &, BytesView, const Callback<void(const uint8_t *, size_t)> &);
bool decryptAes(const AesKey &, BytesView, const Callback<void(const uint8_t *, size_t)> &);

AesKey makeAesKey(BytesView pkey, BytesView hash, uint32_t version = 1);
AesKey makeAesKey(const PrivateKey &pkey, BytesView hash, uint32_t version = 1);

// get keygen version from encrypted block
uint32_t getAesVersion(BytesView);

}

#endif /* STAPPLER_CRYPTO_SPCRYPTO_H_ */
