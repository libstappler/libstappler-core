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
#include "SPCrypto.h"

#if __CDT_PARSER__
#define SP_CRYPTO_OPENSSL 1
#endif

#if SP_CRYPTO_OPENSSL

#include <openssl/pem.h>
#include <openssl/rsa.h>
#include <openssl/evp.h>
#include <openssl/err.h>
#include <openssl/ssl.h>

namespace stappler::crypto {

static constexpr size_t DATA_ALIGN_BOUNDARY ( 16 );

static bool isPemKey(BytesView data) {
	StringView str((const char *)data.data(), data.size());
	str.readChars<StringView::WhiteSpace>();
	if (str.is("-----")) {
		return true;
	}
	return false;
}

static void logOpenSSLErrors() {
    BIO *bio = BIO_new(BIO_s_mem());
    ERR_print_errors(bio);
    char *buf;
    size_t len = BIO_get_mem_data(bio, &buf);
    log::error("OpenSSL", StringView(buf, len));
    BIO_free(bio);
    ERR_clear_error();
}

bool encryptAes(const AesKey &key, BytesView d, const Callback<void(const uint8_t *, size_t)> &cb) {
	auto dataSize = d.size();
	auto blockSize = math::align<size_t>(dataSize, DATA_ALIGN_BOUNDARY);

	uint8_t output[blockSize + 16];
	memcpy(output, &dataSize, sizeof(dataSize));
	memcpy(output + 8, &key.version, sizeof(key.version));

	uint8_t iv[16] = { 0 };
	EVP_CIPHER_CTX *en = nullptr;

	auto finalize = [&] (bool value) {
		if (en) {
			EVP_CIPHER_CTX_free(en);
			en = nullptr;
		}
		return value;
	};

	en = EVP_CIPHER_CTX_new();
	if (!en) {
		return finalize(false);
	}

	if (!EVP_EncryptInit_ex(en, EVP_aes_256_cbc(), NULL, key.data.data(), iv)) {
		return finalize(false);
	}

	auto target = d.data();
	auto targetSize = d.size();
	auto out = output + 16;

	int outSize = 0;
	while (targetSize > 0) {
		if (!EVP_EncryptUpdate(en, out, &outSize,target, targetSize)) {
			return finalize(false);
		}

		out += outSize;
		target += outSize;
		if (int(targetSize) > outSize) {
			targetSize -= outSize;
		} else {
			targetSize = 0;
		}
	}

	if (!EVP_EncryptFinal(en, out, &outSize)) {
		return finalize(false);
	}

	cb(output, blockSize + 16);
	return finalize(true);
}

bool decryptAes(const AesKey &key, BytesView d, const Callback<void(const uint8_t *, size_t)> &cb) {
	auto dataSize = d.readUnsigned64();
	auto blockSize = math::align<size_t>(dataSize, DATA_ALIGN_BOUNDARY);

	d.offset(8); // skip version data

	uint8_t output[blockSize + 16];
	uint8_t iv[16] = { 0 };
	EVP_CIPHER_CTX *de = nullptr;

	auto finalize = [&] (bool value) {
		if (de) {
			EVP_CIPHER_CTX_free(de);
			de = nullptr;
		}
		return value;
	};

	de = EVP_CIPHER_CTX_new();
	if (!de) {
		return finalize(false);
	}

	if (!EVP_DecryptInit_ex(de, EVP_aes_256_cbc(), NULL, key.data.data(), iv)) {
		return finalize(false);
	}

	auto target = d.data();
	auto targetSize = d.size();
	auto out = output;

	int outSize = 0;
	while (targetSize > 0) {
		if (!EVP_DecryptUpdate(de, out, &outSize, target, targetSize)) {
			return finalize(false);
		}

		out += outSize;
		target += outSize;
		if (int(targetSize) > outSize) {
			targetSize -= outSize;
		} else {
			targetSize = 0;
		}
	}

	EVP_DecryptFinal(de, out, &outSize); // gives false-positive error

	cb(output, dataSize);
	return finalize(true);
}

PrivateKey::PrivateKey() : _valid(true), _key(nullptr) { }

PrivateKey::PrivateKey(BytesView data, const CoderSource &str) : _valid(true), _key(nullptr) {
	import(data, str);
}

PrivateKey::~PrivateKey() {
	if (_key) {
		EVP_PKEY_free( _key );
		_key = nullptr;
		_valid = false;
	}
}

bool PrivateKey::generate(KeyBits bits) {
	EVP_PKEY_CTX *kctx = nullptr;
	auto finalize = [&] (bool value) {
		if (kctx) {
			EVP_PKEY_CTX_free(kctx);
			kctx = nullptr;
		}
		if (!value) {
			_valid = false;
			_key = nullptr;
		}
		return value;
	};

	if (_valid && !_loaded) {
		kctx = EVP_PKEY_CTX_new_id(EVP_PKEY_RSA, NULL);
		if (!kctx) {
			return finalize(false);
		}

		if (!EVP_PKEY_keygen_init(kctx)) {
			return finalize(false);
		}

		switch (bits) {
		case KeyBits::_1024: if (!EVP_PKEY_CTX_set_rsa_keygen_bits(kctx, 1024)) { return finalize(false); } break;
		case KeyBits::_2048: if (!EVP_PKEY_CTX_set_rsa_keygen_bits(kctx, 2048)) { return finalize(false); } break;
		case KeyBits::_4096: if (!EVP_PKEY_CTX_set_rsa_keygen_bits(kctx, 4096)) { return finalize(false); } break;
		}

		if (EVP_PKEY_keygen(kctx, &_key)) {
			_loaded = true;
			return finalize(true);
		}
	}
	return finalize(false);
}

bool PrivateKey::import(BytesView data, const CoderSource &passwd) {
	BIO *bioData = nullptr;

	auto finalize = [&] (bool value) {
		if (bioData) {
			BIO_free(bioData);
			bioData = nullptr;
		}
		if (!value) {
			_valid = false;
			_key = nullptr;
			logOpenSSLErrors();
		}
		return value;
	};

	if (_valid && !_loaded) {
		auto bioData = BIO_new_mem_buf((void*)data.data(), data.size());
		if (!bioData) {
			return finalize(false);
		}

		if (isPemKey(data)) {
			_key = PEM_read_bio_PrivateKey(bioData, NULL, [] (char *buf, int size, int rwflag, void *userdata) -> int {
				auto passwd = (const CoderSource *)userdata;
			    int i = passwd->size();
			    i = (i > size) ? size : i;
		        memcpy(buf, passwd->data(), i);
			    return i;
			}, (void *)&passwd);

			if (_key) {
				_loaded = true;
				return finalize(true);
			}
		} else {
			if (!passwd.empty()) {
				_key = d2i_PKCS8PrivateKey_bio(bioData, NULL, [] (char *buf, int size, int rwflag, void *userdata) -> int {
					auto passwd = (const CoderSource *)userdata;
				    int i = passwd->size();
				    i = (i > size) ? size : i;
			        memcpy(buf, passwd->data(), i);
				    return i;
				}, (void *)&passwd);
			} else {
				_key = d2i_PrivateKey_bio(bioData, NULL);
			}

			if (_key) {
				_loaded = true;
				return finalize(true);
			}
		}
	}
	return finalize(false);
}

PublicKey PrivateKey::exportPublic() const {
	return PublicKey(*this);
}

bool PrivateKey::exportPem(const Callback<void(const uint8_t *, size_t)> &cb, KeyFormat fmt, CoderSource passPhrase) const {
	BIO *bp = nullptr;

	auto finalize = [&] (bool value) {
		if (bp) {
			BIO_free(bp);
			bp = nullptr;
		}
		return value;
	};

	bp = BIO_new(BIO_s_mem());
	if (!bp) {
		return finalize(false);
	}

	switch (fmt) {
	case KeyFormat::RSA:
		if (passPhrase.empty()) {
			if (!PEM_write_bio_PrivateKey_traditional(bp, _key, NULL, (unsigned char *)NULL, 0, 0, NULL)) {
				return finalize(false);
			}
		} else {
			if (!PEM_write_bio_PrivateKey_traditional(bp, _key, EVP_des_ede3_cbc(), (unsigned char *)passPhrase.data(), int(passPhrase.size()), 0, NULL)) {
				return finalize(false);
			}
		}
		break;
	case KeyFormat::PKCS8:
		if (passPhrase.empty()) {
			if (!PEM_write_bio_PKCS8PrivateKey(bp, _key, NULL, NULL, 0, 0, NULL)) {
				return finalize(false);
			}
		} else {
			if (!PEM_write_bio_PKCS8PrivateKey(bp, _key, EVP_des_ede3_cbc(), (char *)passPhrase.data(), int(passPhrase.size()), 0, NULL)) {
				return finalize(false);
			}
		}
		break;
	}

    char *buf = nullptr;
	size_t len = BIO_get_mem_data(bp, &buf);
	if (len > 0) {
		cb((uint8_t *)buf, len);
		return finalize(true);
	}

	return finalize(false);
}

bool PrivateKey::exportDer(const Callback<void(const uint8_t *, size_t)> &cb, KeyFormat fmt, CoderSource passPhrase) const {
	BIO *bp = nullptr;

	auto finalize = [&] (bool value) {
		if (bp) {
			BIO_free(bp);
			bp = nullptr;
		}
		return value;
	};

	bp = BIO_new(BIO_s_mem());
	if (!bp) {
		return finalize(false);
	}

	switch (fmt) {
	case KeyFormat::RSA:
		if (passPhrase.empty()) {
			if (!i2d_PrivateKey_bio(bp, _key)) {
				return finalize(false);
			}
		} else {
			log::error("PrivateKey", "exportDer: passPhrase is not supported for KeyFormat::RSA");
			return finalize(false);
		}
		break;
	case KeyFormat::PKCS8:
		if (passPhrase.empty()) {
			if (!i2d_PKCS8PrivateKey_bio(bp, _key, NULL, NULL, 0, 0, NULL)) {
				return finalize(false);
			}
		} else {
			if (!i2d_PKCS8PrivateKey_bio(bp, _key, EVP_des_ede3_cbc(), (char *)passPhrase.data(), int(passPhrase.size()), 0, NULL)) {
				return finalize(false);
			}
		}
		break;
	}

    char *buf = nullptr;
	size_t len = BIO_get_mem_data(bp, &buf);
	if (len > 0) {
		cb((uint8_t *)buf, len);
		return finalize(true);
	}

	return finalize(false);
}

bool PrivateKey::sign(const Callback<void(const uint8_t *, size_t)> &cb, CoderSource data, SignAlgorithm algo) const {
	EVP_MD_CTX *mdctx =  EVP_MD_CTX_create();
	unsigned char *sigdata = nullptr;
	size_t siglen = 0;

	auto cleanup = [&] (bool value) {
		if (mdctx) {
			EVP_MD_CTX_destroy(mdctx);
			mdctx = nullptr;
		}
		if (sigdata) {
			OPENSSL_free(sigdata);
			sigdata = nullptr;
		}
		return value;
	};

	if (!mdctx) {
		return cleanup(false);
	}

	switch (algo) {
	case SignAlgorithm::RSA_SHA256:
	case SignAlgorithm::ECDSA_SHA256:
		if (1 != EVP_DigestSignInit(mdctx, NULL, EVP_sha256(), NULL, _key)) {
			return cleanup(false);
		}
		break;
	case SignAlgorithm::RSA_SHA512:
	case SignAlgorithm::ECDSA_SHA512:
		if (1 != EVP_DigestSignInit(mdctx, NULL, EVP_sha512(), NULL, _key)) {
			return cleanup(false);
		}
		break;
	}

	/* Call update with the message */
	if (1 != EVP_DigestSignUpdate(mdctx, data.data(), data.size())) {
		return cleanup(false);
	}

	if (1 == EVP_DigestSignFinal(mdctx, (unsigned char *)NULL, &siglen)) {
		if (auto sigdata = (unsigned char *)OPENSSL_malloc(sizeof(unsigned char) * siglen)) {
			if (1 == EVP_DigestSignFinal(mdctx, sigdata, &siglen)) {
				cb(sigdata, siglen);
				return cleanup(true);
			}

		}
	}
	return cleanup(false);
}

bool PrivateKey::verify(CoderSource data, BytesView signature, SignAlgorithm algo) const {
	EVP_MD_CTX *mdctx =  EVP_MD_CTX_create();

	auto cleanup = [&] (bool value) {
		if (mdctx) {
			EVP_MD_CTX_destroy(mdctx);
			mdctx = nullptr;
		}
		return value;
	};

	if (!mdctx) {
		return cleanup(false);
	}

	switch (algo) {
	case SignAlgorithm::RSA_SHA256:
	case SignAlgorithm::ECDSA_SHA256:
		if (!EVP_DigestVerifyInit(mdctx, NULL, EVP_sha256(), NULL, _key)) {
			return cleanup(false);
		}
		break;
	case SignAlgorithm::RSA_SHA512:
	case SignAlgorithm::ECDSA_SHA512:
		if (!EVP_DigestVerifyInit(mdctx, NULL, EVP_sha512(), NULL, _key)) {
			return cleanup(false);
		}
		break;
	}

	/* Initialize `key` with a public key */
	if (!EVP_DigestVerifyUpdate(mdctx, data.data(), data.size())) {
		return cleanup(false);
	}

	if (EVP_DigestVerifyFinal(mdctx, signature.data(), signature.size())) {
		return cleanup(true);
	}

	return cleanup(false);
}

bool PrivateKey::isSupported(KeyFormat fmt) const {
	return true;
}

PublicKey::PublicKey() : _valid(true), _key(nullptr) { }

PublicKey::PublicKey(BytesView data) : _valid(true), _key(nullptr) {
	if (data.starts_with((const uint8_t *)"ssh-rsa", "ssh-rsa"_len)) {
		importOpenSSH(StringView((const char *)data.data(), data.size()));
	} else {
		import(data);
	}
}
PublicKey::PublicKey(const PrivateKey &priv) : _valid(true), _key(nullptr) {
	auto bp = BIO_new(BIO_s_mem());
	if (!bp) {
		return;
	}

	if (i2b_PublicKey_bio(bp, priv.getKey())) {
		_key = b2i_PublicKey_bio(bp);
		if (_key) {
			_loaded = true;
		}
	}

	BIO_free(bp);
}

PublicKey::~PublicKey() {
	if (_key) {
		EVP_PKEY_free( _key );
		_key = nullptr;
		_valid = false;
	}
}

bool PublicKey::import(BytesView data) {
	BIO *bioData = nullptr;

	auto finalize = [&] (bool value) {
		if (bioData) {
			BIO_free(bioData);
			bioData = nullptr;
		}
		if (!value) {
			_valid = false;
			_key = nullptr;
		}
		return value;
	};

	if (_valid && !_loaded) {
		bioData = BIO_new_mem_buf((void*)data.data(), data.size());
		if (isPemKey(data)) {
			_key = PEM_read_bio_PUBKEY(bioData, NULL, NULL, NULL);
			if (_key) {
				_loaded = true;
				return finalize(true);
			}
		} else {
			_key = d2i_PUBKEY_bio(bioData, NULL);
			if (_key) {
				_loaded = true;
				return finalize(true);
			}
		}
	}
	return finalize(false);
}

bool PublicKey::importOpenSSH(StringView r) {
	if (_valid && !_loaded) {
		auto origKeyType = r.readUntil<StringView::CharGroup<CharGroupId::WhiteSpace>>();
		r.skipChars<StringView::CharGroup<CharGroupId::WhiteSpace>>();
		auto dataBlock = r.readUntil<StringView::CharGroup<CharGroupId::WhiteSpace>>();
		dataBlock = dataBlock.readChars<StringView::CharGroup<CharGroupId::Base64>>();
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

			auto modulusBn = BN_bin2bn(modulus.data(), mlen, nullptr);
			auto expBn = BN_bin2bn(exp.data(), elen, nullptr);

			auto rsa = RSA_new();
			RSA_set0_key(rsa, expBn, modulusBn, NULL);
			_key = EVP_PKEY_new();
			if (_key) {
				EVP_PKEY_assign_RSA(_key, rsa);
				_loaded = true;
			}
		}
	}

	return _loaded;
}

bool PublicKey::exportPem(const Callback<void(const uint8_t *, size_t)> &cb) const {
	BIO *bp = nullptr;

	auto finalize = [&] (bool value) {
		if (bp) {
			BIO_free(bp);
			bp = nullptr;
		}
		return value;
	};

	bp = BIO_new(BIO_s_mem());
	if (!bp) {
		return finalize(false);
	}

	if (!PEM_write_bio_PUBKEY(bp, _key)) {
		return finalize(false);
	}

	auto len = BIO_pending(bp);
	if (len > 0) {
		uint8_t buf[len];

		if (BIO_read(bp, buf, len)) {
			cb(buf, len);
			return finalize(true);
		}
	}
	return finalize(false);
}

bool PublicKey::exportDer(const Callback<void(const uint8_t *, size_t)> &cb) const {
	BIO *bp = nullptr;

	auto finalize = [&] (bool value) {
		if (bp) {
			BIO_free(bp);
			bp = nullptr;
		}
		return value;
	};


	bp = BIO_new(BIO_s_mem());
	if (!bp) {
		return finalize(false);
	}

	if (!i2d_PUBKEY_bio(bp, _key)) {
		return finalize(false);
	}

	auto len = BIO_pending(bp);
	if (len > 0) {
		uint8_t buf[len];

		if (BIO_read(bp, buf, len)) {
			cb(buf, len);
			return finalize(true);
		}
	}
	return finalize(false);
}

bool PublicKey::verify(CoderSource data, BytesView signature, SignAlgorithm algo) const {
	EVP_MD_CTX *mdctx =  EVP_MD_CTX_create();

	auto cleanup = [&] (bool value) {
		if (mdctx) {
			EVP_MD_CTX_destroy(mdctx);
			mdctx = nullptr;
		}
		return value;
	};

	if (!mdctx) {
		return cleanup(false);
	}

	switch (algo) {
	case SignAlgorithm::RSA_SHA256:
	case SignAlgorithm::ECDSA_SHA256:
		if (!EVP_DigestVerifyInit(mdctx, NULL, EVP_sha256(), NULL, _key)) {
			return cleanup(false);
		}
		break;
	case SignAlgorithm::RSA_SHA512:
	case SignAlgorithm::ECDSA_SHA512:
		if (!EVP_DigestVerifyInit(mdctx, NULL, EVP_sha512(), NULL, _key)) {
			return cleanup(false);
		}
		break;
	}

	/* Initialize `key` with a public key */
	if (!EVP_DigestVerifyUpdate(mdctx, data.data(), data.size())) {
		return cleanup(false);
	}

	if (EVP_DigestVerifyFinal(mdctx, signature.data(), signature.size())) {
		return cleanup(true);
	}

	return cleanup(false);
}

}

#endif
