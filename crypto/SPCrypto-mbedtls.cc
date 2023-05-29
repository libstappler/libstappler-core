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

#include "SPString.h"
#include "SPLog.h"
#include "SPValid.h"
#include "SPCrypto.h"

#if __CDT_PARSER__
#define SP_CRYPTO_MBEDTLS 1
#endif

#if SP_CRYPTO_MBEDTLS

#include "mbedtls/mbedtls_config.h"
#include "mbedtls/pk.h"
#include "mbedtls/error.h"
#include "mbedtls/pem.h"
#include "mbedtls/entropy.h"
#include "mbedtls/ctr_drbg.h"

sp_pk_context::operator mbedtls_pk_context() const {
	mbedtls_pk_context ret;
	memcpy(&ret, this, sizeof(mbedtls_pk_context));
	return ret;
}

namespace stappler::crypto {

static constexpr size_t DATA_ALIGN_BOUNDARY ( 16 );

static constexpr auto PERSONALIZATION_STRING = "SP_PERSONALIZATION_STRING";
static constexpr int PUBLIC_EXPONENT = 65537;

static bool isPemKey(BytesView data) {
	StringView str((const char *)data.data(), data.size());
	str.readChars<StringView::WhiteSpace>();
	if (str.is("-----")) {
		return true;
	}
	return false;
}

static void sp_mbedtls_pk_init(sp_pk_context *key) {
	mbedtls_pk_init((mbedtls_pk_context *)key);
}

static void sp_mbedtls_pk_free(sp_pk_context *key) {
	mbedtls_pk_free((mbedtls_pk_context *)key);
}

struct EntropyContext {
	bool valid = false;
	mbedtls_entropy_context entropy;
	mbedtls_ctr_drbg_context ctr_drbg;

	EntropyContext() {
		mbedtls_ctr_drbg_init(&ctr_drbg);
		mbedtls_entropy_init(&entropy);

		if (mbedtls_ctr_drbg_seed(&ctr_drbg, mbedtls_entropy_func, &entropy,
				(const unsigned char*) PERSONALIZATION_STRING, strlen(PERSONALIZATION_STRING)) == 0) {
			valid = true;
		}
	}

	~EntropyContext() {
		mbedtls_ctr_drbg_free(&ctr_drbg);
		mbedtls_entropy_free(&entropy);
	}

	operator bool() const {
		return valid;
	}
};

bool encryptAes(const AesKey &key, BytesView d, const Callback<void(const uint8_t *, size_t)> &cb) {
	bool success = false;
	auto dataSize = d.size();
	auto blockSize = math::align<size_t>(dataSize, DATA_ALIGN_BOUNDARY);

	uint8_t output[blockSize + 16];
	memcpy(output, &dataSize, sizeof(dataSize));
	memcpy(output + 8, &key.version, sizeof(key.version));

	mbedtls_aes_context aes;
	unsigned char iv[16] = { 0 };

	mbedtls_aes_init( &aes );
	if (mbedtls_aes_setkey_enc( &aes, key.data.data(), 256 ) == 0) {
		if (mbedtls_aes_crypt_cbc( &aes, MBEDTLS_AES_ENCRYPT, blockSize, iv, d.data(), output + 16 ) == 0) {
			success = true;
			cb(output, blockSize + 16);
		}
	}
	mbedtls_aes_free( &aes );
	return success;
}

bool decryptAes(const AesKey &key, BytesView d, const Callback<void(const uint8_t *, size_t)> &cb) {
	bool success = false;
	auto dataSize = d.readUnsigned64();
	auto blockSize = math::align<size_t>(dataSize, DATA_ALIGN_BOUNDARY);

	d.offset(8); // skip version data

	uint8_t output[blockSize + 16];

	mbedtls_aes_context aes;
	unsigned char iv[16] = { 0 };

	mbedtls_aes_init( &aes );
	if (mbedtls_aes_setkey_dec( &aes, key.data.data(), 256 ) == 0) {
		if (mbedtls_aes_crypt_cbc( &aes, MBEDTLS_AES_DECRYPT, blockSize, iv, d.data(), output ) == 0) {
			success = true;
			cb(output, dataSize);
		}
	}
	mbedtls_aes_free( &aes );
	return success;
}

PrivateKey::PrivateKey() : _valid(true), _key() {
	sp_mbedtls_pk_init(&_key);
}

PrivateKey::PrivateKey(BytesView data, const CoderSource &str) : _valid(true), _key() {
	import(data, str);
}

PrivateKey::~PrivateKey() {
	if (_valid) {
		sp_mbedtls_pk_free( &_key );
		_valid = false;
	}
}

bool PrivateKey::generate(KeyBits bits) {
	int nbits = 0;

	switch (bits) {
	case KeyBits::_1024: nbits = 1024; break;
	case KeyBits::_2048: nbits = 2048; break;
	case KeyBits::_4096: nbits = 4096; break;
	}

	if (nbits == 0) {
		return false;
	}

	EntropyContext entropy;
	if (!entropy) {
		return false;
	}

	auto finalize = [&] (bool value) {
		if (!value) {
			sp_mbedtls_pk_free( &_key );
			_valid = false;
		}
		return value;
	};

	if (_valid && !_loaded) {
		sp_mbedtls_pk_init(&_key);

		int ret = -1;
		if ((ret = mbedtls_pk_setup((mbedtls_pk_context *)&_key, mbedtls_pk_info_from_type(mbedtls_pk_type_t(MBEDTLS_PK_RSA)))) != 0) {
			return finalize(false);
		}

		if ((ret = mbedtls_rsa_gen_key(mbedtls_pk_rsa(_key), mbedtls_ctr_drbg_random, &entropy.ctr_drbg, nbits, PUBLIC_EXPONENT)) != 0) {
			return finalize(false);
		}

		if (mbedtls_rsa_check_pubkey(mbedtls_pk_rsa(_key)) != 0) {
			return finalize(false);
		}

		if (mbedtls_rsa_check_privkey(mbedtls_pk_rsa(_key)) != 0) {
			return finalize(false);
		}

		_loaded = true;
		return finalize(true);
	}
	return finalize(false);
}

bool PrivateKey::import(BytesView data, const CoderSource &passwd) {
	if (_loaded || !_valid || data.empty()) {
		return false;
	}

	EntropyContext entropy;

	if (isPemKey(data)) {
		if (data.data()[data.size() - 1] != 0) {
			if (data.data()[data.size()] == 0) {
				auto err = mbedtls_pk_parse_key((mbedtls_pk_context *)&_key, data.data(), data.size() + 1,
						passwd.data(), passwd.size(), mbedtls_ctr_drbg_random, &entropy.ctr_drbg);
				if (err != 0) {
					sp_mbedtls_pk_free(&_key);
					_valid = false;
					return false;
				}
			} else {
				uint8_t buf[data.size() + 1];
				memcpy(buf, data.data(), data.size());
				buf[data.size()] = 0;

				auto err = mbedtls_pk_parse_key((mbedtls_pk_context *)&_key, buf, data.size() + 1,
						passwd.data(), passwd.size(), mbedtls_ctr_drbg_random, &entropy.ctr_drbg);
				if (err != 0) {
					sp_mbedtls_pk_free(&_key);
					_valid = false;
					return false;
				}
			}
		} else {
			auto err = mbedtls_pk_parse_key((mbedtls_pk_context *)&_key, data.data(), data.size(),
					passwd.data(), passwd.size(), mbedtls_ctr_drbg_random, &entropy.ctr_drbg);
			if (err != 0) {
				sp_mbedtls_pk_free(&_key);
				_valid = false;
				return false;
			}
		}
	} else {
		auto err = mbedtls_pk_parse_key((mbedtls_pk_context *)&_key, data.data(), data.size(),
				passwd.data(), passwd.size(), mbedtls_ctr_drbg_random, &entropy.ctr_drbg);
		if (err != 0) {
			sp_mbedtls_pk_free(&_key);
			_valid = false;
			return false;
		}
	}

	_loaded = true;
	return true;
}

PublicKey PrivateKey::exportPublic() const {
	uint8_t buf[10_KiB];
	int ret = -1;

	ret = mbedtls_pk_write_pubkey_der((mbedtls_pk_context *)&_key, buf, 10_KiB);
	if (ret > 0) {
		return PublicKey(BytesView(buf + 10_KiB - ret, ret));
	}

	return PublicKey();
}

bool PrivateKey::exportPem(const Callback<void(const uint8_t *, size_t)> &cb, KeyFormat fmt, CoderSource passPhrase) const {
	if (!_loaded || !_valid) {
		return false;
	}

	uint8_t buf[10_KiB];
	int ret = -1;

	switch (fmt) {
	case KeyFormat::PKCS1:
		if (!passPhrase.empty()) {
			log::text("Crypto", "Password-encoding is not supported for PKCS1");
		}
		ret = mbedtls_pk_write_key_pem((mbedtls_pk_context *)&_key, buf, 10_KiB);
		if (ret > 0) {
			cb(buf + 10_KiB - ret, ret);
			return true;
		} else if (ret == 0) {
			auto len = strlen((char *)buf);
			if (len < 10_KiB) {
				cb(buf, len);
				return true;
			}
		}
		break;
	case KeyFormat::PKCS8:
		log::text("Crypto", "KeyFormat::PKCS8 is not supported by mbedtls backend, Fallback to PKCS1");
		ret = mbedtls_pk_write_key_pem((mbedtls_pk_context *)&_key, buf, 10_KiB);
		if (ret > 0) {
			cb(buf + 10_KiB - ret, ret);
			return true;
		}
		break;
	}

	return false;
}

bool PrivateKey::exportDer(const Callback<void(const uint8_t *, size_t)> &cb, KeyFormat fmt, CoderSource passPhrase) const {
	if (!_loaded || !_valid) {
		return false;
	}

	uint8_t buf[10_KiB];
	int ret = -1;

	switch (fmt) {
	case KeyFormat::PKCS1:
		if (!passPhrase.empty()) {
			log::text("Crypto", "Password-encoding is not supported for PKCS1");
		}
		ret = mbedtls_pk_write_key_der((mbedtls_pk_context *)&_key, buf, 10_KiB);
		if (ret > 0) {
			cb(buf + 10_KiB - ret, ret);
			return true;
		}
		break;
	case KeyFormat::PKCS8:
		log::text("Crypto", "KeyFormat::PKCS8 is not supported by mbedtls backend, Fallback to PKCS1");
		ret = mbedtls_pk_write_key_der((mbedtls_pk_context *)&_key, buf, 10_KiB);
		if (ret > 0) {
			cb(buf + 10_KiB - ret, ret);
			return true;
		}
		break;
	}

	return false;
}

bool PrivateKey::sign(const Callback<void(const uint8_t *, size_t)> &cb, CoderSource data, SignAlgorithm algo) const {
	if (!_loaded || !_valid) {
		return false;
	}

	EntropyContext entropy;
	if (!entropy) {
		return false;
	}

	size_t writeSize = 0;
	uint8_t buf[MBEDTLS_PK_SIGNATURE_MAX_SIZE];

	switch (algo) {
	case SignAlgorithm::ECDSA_SHA256:
	case SignAlgorithm::RSA_SHA256: {
		auto hash = Sha256().update(data).final();

		if (mbedtls_pk_sign((mbedtls_pk_context *)&_key, MBEDTLS_MD_SHA256, hash.data(), hash.size(),
				buf, MBEDTLS_PK_SIGNATURE_MAX_SIZE, &writeSize, mbedtls_ctr_drbg_random, &entropy.ctr_drbg) == 0) {
			cb(buf, writeSize);
			return true;
		}

		break;
	}
	case SignAlgorithm::ECDSA_SHA512:
	case SignAlgorithm::RSA_SHA512: {
		auto hash = Sha512().update(data).final();

		if (mbedtls_pk_sign((mbedtls_pk_context *)&_key, MBEDTLS_MD_SHA512, hash.data(), hash.size(),
				buf, MBEDTLS_PK_SIGNATURE_MAX_SIZE, &writeSize, mbedtls_ctr_drbg_random, &entropy.ctr_drbg) == 0) {
			cb(buf, writeSize);
			return true;
		}

		break;
	}
	}

	return false;
}

bool PrivateKey::verify(CoderSource data, BytesView signature, SignAlgorithm algo) const {
	switch (algo) {
	case SignAlgorithm::ECDSA_SHA256:
	case SignAlgorithm::RSA_SHA256: {
		auto hash = string::Sha256().update(data).final();
		if (mbedtls_pk_verify((mbedtls_pk_context *)&_key, MBEDTLS_MD_SHA256, hash.data(), hash.size(),
				signature.data(), signature.size()) == 0) {
			return true;
		}
		break;
	}
	case SignAlgorithm::ECDSA_SHA512:
	case SignAlgorithm::RSA_SHA512: {
		auto hash = string::Sha512().update(data).final();
		if (mbedtls_pk_verify((mbedtls_pk_context *)&_key, MBEDTLS_MD_SHA512, hash.data(), hash.size(),
				signature.data(), signature.size()) == 0) {
			return true;
		}
		break;
	}
	}
	return false;
}

bool PrivateKey::isSupported(KeyFormat fmt) const {
	switch (fmt) {
	case KeyFormat::PKCS1: return true; break;
	case KeyFormat::PKCS8: return false; break;
	}
	return false;
}

PublicKey::PublicKey() : _valid(true), _key() {
	sp_mbedtls_pk_init(&_key);
}

PublicKey::PublicKey(BytesView data) : _valid(true), _key() {
	sp_mbedtls_pk_init(&_key);
	if (data.starts_with((const uint8_t *)"ssh-rsa", "ssh-rsa"_len)) {
		importOpenSSH(StringView((const char *)data.data(), data.size()));
	} else {
		import(data);
	}
}

PublicKey::PublicKey(const PrivateKey &priv) : _valid(true), _key() {
	sp_mbedtls_pk_init(&_key);

	uint8_t buf[10_KiB];
	int ret = -1;

	auto key = priv.getKey();
	ret = mbedtls_pk_write_pubkey_der((mbedtls_pk_context *)&key, buf, 10_KiB);
	if (ret > 0) {
		import(BytesView(buf + 10_KiB - ret, ret));
	}
}

PublicKey::~PublicKey() {
	if (_valid) {
		sp_mbedtls_pk_free(&_key);
		_valid = false;
	}
}

bool PublicKey::import(BytesView data) {
	if (_loaded || !_valid || data.empty()) {
		return false;
	}

	EntropyContext entropy;

	if (isPemKey(data)) {
		if (data.data()[data.size() - 1] != 0) {
			if (data.data()[data.size()] == 0) {
				auto err = mbedtls_pk_parse_public_key((mbedtls_pk_context *)&_key, data.data(), data.size() + 1);
				if (err != 0) {
					sp_mbedtls_pk_free(&_key);
					_valid = false;
					return false;
				}
			} else {
				uint8_t buf[data.size() + 1];
				memcpy(buf, data.data(), data.size());
				buf[data.size()] = 0;

				auto err = mbedtls_pk_parse_public_key((mbedtls_pk_context *)&_key, buf, data.size() + 1);
				if (err != 0) {
					sp_mbedtls_pk_free(&_key);
					_valid = false;
					return false;
				}
			}
		} else {
			auto err = mbedtls_pk_parse_public_key((mbedtls_pk_context *)&_key, data.data(), data.size());
			if (err != 0) {
				sp_mbedtls_pk_free(&_key);
				_valid = false;
				return false;
			}
		}
	} else {
		auto err = mbedtls_pk_parse_public_key((mbedtls_pk_context *)&_key, data.data(), data.size());
		if (err != 0) {
			sp_mbedtls_pk_free(&_key);
			_valid = false;
			return false;
		}
	}

	_loaded = true;
	return true;
}

bool PublicKey::importOpenSSH(StringView r) {
	if (!_valid || _loaded) {
		return false;
	}

	uint8_t out[2_KiB];
	uint8_t *buf = out;

	auto origKeyType = r.readUntil<StringView::CharGroup<CharGroupId::WhiteSpace>>();
	r.skipChars<StringView::CharGroup<CharGroupId::WhiteSpace>>();
	auto dataBlock = r.readUntil<StringView::CharGroup<CharGroupId::WhiteSpace>>();
	if (valid::validateBase64(dataBlock)) {
		uint8_t bytes[base64::decodeSize(dataBlock.size())];
		uint8_t *target = bytes;
		base64::decode([&] (uint8_t c) {
			*target++ = c;
		}, dataBlock);

		BytesViewNetwork dataView(bytes, target - bytes);
		auto len = dataView.readUnsigned32();
		auto keyType = dataView.readString(len);

		if (origKeyType != keyType || keyType != "ssh-rsa") {
			return false;
		}

		auto mlen = dataView.readUnsigned32();
		auto modulus = dataView.readBytes(mlen);

		auto elen = dataView.readUnsigned32();
		auto exp = dataView.readBytes(elen);

		size_t modSize = 1;
		size_t expSize = 1;

		auto readSize = [&] (size_t s) {
			if (s < 128) {
				return 1;
			} else if (s < 256) {
				return 2;
			} else {
				return 3;
			}
		};

		auto writeSize = [&] (size_t s) {
			if (s < 128) {
				*buf = uint8_t(s); ++ buf;
			} else if (s < 256) {
				*buf = uint8_t(0x81); ++ buf;
				*buf = uint8_t(s); ++ buf;
			} else {
				*buf = uint8_t(0x82); ++ buf;
				*buf = uint8_t((s >> 8) & 0xFF); ++ buf;
				*buf = uint8_t(s & 0xFF); ++ buf;
			}
		};

		modSize += readSize(mlen) + modulus.size();
		expSize += readSize(elen) + exp.size();

		*buf = uint8_t(0x30); ++ buf;
		writeSize(modSize + expSize);

		*buf = uint8_t(0x02); ++ buf;
		writeSize(exp.size());
		for (size_t i = 0; i < exp.size(); ++ i) {
			*buf = exp.at(i); ++ buf;
		}

		*buf = uint8_t(0x02); ++ buf;
		writeSize(modulus.size());
		for (size_t i = 0; i < modulus.size(); ++ i) {
			*buf = modulus.at(i); ++ buf;
		}

		if (mbedtls_pk_parse_public_key((mbedtls_pk_context *)&_key, (const uint8_t *)out, buf - out) != 0) {
			return false;
		}

		_loaded = true;
		return true;
	}

	return false;
}

bool PublicKey::exportPem(const Callback<void(const uint8_t *, size_t)> &cb) const {
	uint8_t buf[10_KiB];
	int ret = -1;

	ret = mbedtls_pk_write_pubkey_pem((mbedtls_pk_context *)&_key, buf, 10_KiB);
	if (ret > 0) {
		cb(buf + 10_KiB - ret, ret);
		return true;
	} else if (ret == 0) {
		auto len = strlen((char *)buf);
		if (len < 10_KiB) {
			cb(buf, len);
			return true;
		}
	}

	return false;
}

bool PublicKey::exportDer(const Callback<void(const uint8_t *, size_t)> &cb) const {
	uint8_t buf[10_KiB];
	int ret = -1;

	ret = mbedtls_pk_write_pubkey_der((mbedtls_pk_context *)&_key, buf, 10_KiB);
	if (ret > 0) {
		cb(buf + 10_KiB - ret, ret);
		return true;
	}

	return false;
}

bool PublicKey::verify(CoderSource data, BytesView signature, SignAlgorithm algo) const {
	switch (algo) {
	case SignAlgorithm::ECDSA_SHA256:
	case SignAlgorithm::RSA_SHA256: {
		auto hash = string::Sha256().update(data).final();
		if (mbedtls_pk_verify((mbedtls_pk_context *)&_key, MBEDTLS_MD_SHA256, hash.data(), hash.size(),
				signature.data(), signature.size()) == 0) {
			return true;
		}
		break;
	}
	case SignAlgorithm::ECDSA_SHA512:
	case SignAlgorithm::RSA_SHA512: {
		auto hash = string::Sha512().update(data).final();
		if (mbedtls_pk_verify((mbedtls_pk_context *)&_key, MBEDTLS_MD_SHA512, hash.data(), hash.size(),
				signature.data(), signature.size()) == 0) {
			return true;
		}
		break;
	}
	}
	return false;
}

}

#endif
