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
#define MODULE_STAPPLER_CRYPTO_OPENSSL 1
#endif

#if MODULE_STAPPLER_CRYPTO_OPENSSL

#include <openssl/pem.h>
#include <openssl/rsa.h>
#include <openssl/evp.h>
#include <openssl/err.h>
#include <openssl/ssl.h>
#include <openssl/engine.h>

extern "C" {

#include <gost-engine.h>

}

namespace stappler::crypto {

// For interoperability with GnuTLS
static constexpr int OPENSSL_PK_ENCRYPT_PADDING = RSA_PKCS1_PADDING;

static void logOpenSSLErrors() {
    BIO *bio = BIO_new(BIO_s_mem());
    ERR_print_errors(bio);
    char *buf;
    size_t len = BIO_get_mem_data(bio, &buf);
    log::error("OpenSSL", StringView(buf, len));
    BIO_free(bio);
    ERR_clear_error();
}

static KeyType getOpenSSLKeyType(int id) {
	if (id == EVP_PKEY_RSA) {
		return KeyType::RSA;
	} else if (id == EVP_PKEY_DSA) {
		return KeyType::DSA;
	} else if (id == EVP_PKEY_EC) {
		return KeyType::ECDSA;
	} else if (id == EVP_PKEY_ED448) {
		return KeyType::EDDSA_ED448;
	} else if (id == NID_id_GostR3410_2012_256) {
		return KeyType::GOST3410_2012_256;
	} else if (id == NID_id_GostR3410_2012_512) {
		return KeyType::GOST3410_2012_512;
	}
	return KeyType::Unknown;
}

static const EVP_CIPHER *getOpenSSLCipher(BlockCipher b) {
	switch (b) {
	case BlockCipher::AES_CBC:
		return EVP_aes_256_cbc();
		break;
	case BlockCipher::AES_CFB8:
		return EVP_aes_256_cfb8();
		break;
	case BlockCipher::Gost3412_2015_CTR_ACPKM:
		if (auto m = EVP_get_cipherbyname("kuznyechik-ctr-acpkm")) {
			return m;
		}
		break;
	}
	return EVP_aes_256_cbc();
}

static BackendCtx s_openSSLCtx = {
	.name = Backend::OpenSSL,
	.title = StringView("OpenSSL"),
	.flags = BackendFlags::SupportsPKCS1 | BackendFlags::SupportsPKCS8 | BackendFlags::SupportsAes | BackendFlags::SecureLibrary
			| BackendFlags::SupportsGost3410_2012,
	.initialize = [] () {
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
		ENGINE_load_gost();
	    ENGINE_register_all_complete();
		OPENSSL_init_ssl(OPENSSL_INIT_SSL_DEFAULT, NULL);
#pragma GCC diagnostic pop
		log::verbose("Crypto", "OpenSSL+gost backend loaded");
	},
	.finalize = [] () { },
	.encryptBlock = [] (const BlockKey256 &key, BytesView d, const Callback<void(const uint8_t *, size_t)> &cb) -> bool {
		auto cipherBlockSize = getBlockSize(key.cipher);
		auto cipher = getOpenSSLCipher(key.cipher);

		uint64_t dataSize = d.size();
		auto blockSize = math::align<size_t>(dataSize, cipherBlockSize)
				+ cipherBlockSize; // allocate space for possible padding

		uint8_t output[blockSize + sizeof(CryptoBlockHeader)];

		fillCryptoBlockHeader(output, key, d);

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

		if (!EVP_EncryptInit_ex(en, cipher, NULL, key.data.data(), iv)) {
			return finalize(false);
		}

		auto perform = [] (EVP_CIPHER_CTX *en, const uint8_t *target, size_t targetSize, uint8_t *out) {
			int outSize = 0;
			while (targetSize > 0) {
				if (!EVP_EncryptUpdate(en, out, &outSize, target, targetSize)) {
					return false;
				}

				out += outSize;
				target += outSize;
				if (int(targetSize) >= outSize) {
					targetSize -= outSize;
				} else {
					targetSize = 0;
				}
			}

			if (!EVP_EncryptFinal(en, out, &outSize)) {
				return false;
			}

			return true;
		};

		fillCryptoBlockHeader(output, key, d);

		if constexpr (SafeBlockEncoding) {
			uint8_t tmp[blockSize];
			memset(tmp, 0, blockSize);
			memcpy(tmp, d.data(), d.size());

			if (!perform(en, tmp, blockSize - cipherBlockSize, output + sizeof(CryptoBlockHeader))) {
				return finalize(false);
			}
		} else {
			if (!perform(en, d.data(), d.size(), output + sizeof(CryptoBlockHeader))) {
				return finalize(false);
			}
		}

		cb(output, blockSize + sizeof(CryptoBlockHeader) - cipherBlockSize);
		return finalize(true);
	},
	.decryptBlock = [] (const BlockKey256 &key, BytesView b, const Callback<void(const uint8_t *, size_t)> &cb) -> bool {
		auto info = getBlockInfo(b);
		auto cipherBlockSize = getBlockSize(info.cipher);
		auto cipher = getOpenSSLCipher(info.cipher);

		auto blockSize = math::align<size_t>(info.dataSize, cipherBlockSize) + cipherBlockSize;
		b.offset(sizeof(CryptoBlockHeader));

		uint8_t output[blockSize];

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

		if (!EVP_DecryptInit_ex(de, cipher, NULL, key.data.data(), iv)) {
			return finalize(false);
		}

		auto target = b.data();
		auto targetSize = b.size();
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

		cb(output, info.dataSize);
		return finalize(true);
	},
	.hash256 = [] (Sha256::Buf &buf, const Callback<void( const HashCoderCallback &upd )> &cb, HashFunction func) -> bool {
		bool success = true;
		EVP_MD_CTX *mdctx = EVP_MD_CTX_new();
		EVP_MD_CTX_init(mdctx);

		switch (func) {
		case HashFunction::SHA_2:
			if (EVP_DigestInit(mdctx, EVP_sha256()) == 0) {
				success = false;
			}
			break;
		case HashFunction::GOST_3411:
			if (auto md = EVP_get_digestbyname("md_gost12_256")) {
				if (EVP_DigestInit(mdctx, md) == 0) {
					success = false;
				}
			} else {
				success = false;
			}
			break;
		}

		if (success) {
			cb([&] (const CoderSource &data) {
				if (success && EVP_DigestUpdate(mdctx, data.data(), data.size()) == 0) {
					success = false;
					return false;
				}
				return true;
			});

			if (success && EVP_DigestFinal(mdctx, buf.data(), nullptr) == 0) {
				success = false;
			}
		}

		EVP_MD_CTX_free(mdctx);
		return success;
	},
	.hash512 = [] (Sha512::Buf &buf, const Callback<void( const HashCoderCallback &upd )> &cb, HashFunction func) -> bool {
		bool success = true;
		EVP_MD_CTX *mdctx = EVP_MD_CTX_new();
		EVP_MD_CTX_init(mdctx);

		switch (func) {
		case HashFunction::SHA_2:
			if (EVP_DigestInit(mdctx, EVP_sha512()) == 0) {
				success = false;
			}
			break;
		case HashFunction::GOST_3411:
			if (auto md = EVP_get_digestbyname("md_gost12_512")) {
				if (EVP_DigestInit(mdctx, md) == 0) {
					success = false;
				}
			} else {
				success = false;
			}
			break;
		}

		if (success) {
			cb([&] (const CoderSource &data) {
				if (success && EVP_DigestUpdate(mdctx, data.data(), data.size()) == 0) {
					success = false;
					return false;
				}
				return true;
			});

			if (success && EVP_DigestFinal(mdctx, buf.data(), nullptr) == 0) {
				success = false;
			}
		}

		EVP_MD_CTX_free(mdctx);
		return success;
	},
	.privInit = [] (KeyContext &ctx) -> bool {
		ctx.keyCtx = nullptr;
		return true;
	},
	.privFree = [] (KeyContext &ctx) {
		if (ctx.keyCtx) {
			EVP_PKEY_free( static_cast<EVP_PKEY *>(ctx.keyCtx) );
			ctx.keyCtx = nullptr;
		}
	},
	.privGen = [] (KeyContext &ctx, KeyBits bits) -> bool {
		EVP_PKEY_CTX *kctx = nullptr;
		auto finalize = [&] (bool value) {
			if (kctx) {
				EVP_PKEY_CTX_free(kctx);
				kctx = nullptr;
			}
			if (!value) {
				ctx.keyCtx = nullptr;
				logOpenSSLErrors();
			}
			return value;
		};

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

		EVP_PKEY *ret = nullptr;
		if (EVP_PKEY_keygen(kctx, &ret)) {
			ctx.keyCtx = ret;
			ctx.type = getOpenSSLKeyType(EVP_PKEY_get_id(ret));
			return finalize(true);
		}
		return finalize(false);
	},
	.privImport = [] (KeyContext &ctx, BytesView data, const CoderSource &passwd) {
		BIO *bioData = nullptr;

		auto finalize = [&] (bool value) {
			if (bioData) {
				BIO_free(bioData);
				bioData = nullptr;
			}
			if (!value) {
				ctx.keyCtx = nullptr;
				logOpenSSLErrors();
			} else {
				ctx.type = getOpenSSLKeyType(EVP_PKEY_get_id(static_cast<EVP_PKEY *>(ctx.keyCtx)));
			}
			return value;
		};

		bioData = BIO_new_mem_buf(data.data(), data.size());
		if (!bioData) {
			return finalize(false);
		}

		if (isPemKey(data)) {
			ctx.keyCtx = PEM_read_bio_PrivateKey(bioData, NULL, [] (char *buf, int size, int rwflag, void *userdata) -> int {
				auto passwd = static_cast<const CoderSource *>(userdata);
				int i = passwd->size();
				i = (i > size) ? size : i;
				memcpy(buf, passwd->data(), i);
				return i;
			}, const_cast<CoderSource *>(&passwd));

			if (ctx.keyCtx) {
				return finalize(true);
			}
		} else {
			if (!passwd.empty()) {
				ctx.keyCtx = d2i_PKCS8PrivateKey_bio(bioData, NULL, [] (char *buf, int size, int rwflag, void *userdata) -> int {
					auto passwd = static_cast<const CoderSource *>(userdata);
					int i = passwd->size();
					i = (i > size) ? size : i;
					memcpy(buf, passwd->data(), i);
					return i;
				}, const_cast<CoderSource *>(&passwd));
			} else {
				ctx.keyCtx = d2i_PrivateKey_bio(bioData, NULL);
			}

			if (ctx.keyCtx) {
				return finalize(true);
			}
		}
		return finalize(false);
	},
	.privExportPem = [] (const KeyContext &ctx, const Callback<void(const uint8_t *, size_t)> &cb, KeyFormat fmt, const CoderSource &passPhrase) {
		BIO *bp = nullptr;
		auto key = static_cast<const EVP_PKEY *>(ctx.keyCtx);

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
				if (!PEM_write_bio_PrivateKey_traditional(bp, key, NULL, nullptr, 0, 0, NULL)) {
					return finalize(false);
				}
			} else {
				if (!PEM_write_bio_PrivateKey_traditional(bp, key, EVP_des_ede3_cbc(), const_cast<unsigned char *>(passPhrase.data()), int(passPhrase.size()), 0, NULL)) {
					return finalize(false);
				}
			}
			break;
		case KeyFormat::PKCS8:
			if (passPhrase.empty()) {
				if (!PEM_write_bio_PKCS8PrivateKey(bp, key, NULL, NULL, 0, 0, NULL)) {
					return finalize(false);
				}
			} else {
				if (!PEM_write_bio_PKCS8PrivateKey(bp, key, EVP_des_ede3_cbc(), reinterpret_cast<const char *>(passPhrase.data()), int(passPhrase.size()), 0, NULL)) {
					return finalize(false);
				}
			}
			break;
		}

		char *buf = nullptr;
		size_t len = BIO_get_mem_data(bp, &buf);
		if (len > 0) {
			cb(reinterpret_cast<uint8_t *>(buf), len);
			return finalize(true);
		}

		return finalize(false);
	},
	.privExportDer = [] (const KeyContext &ctx, const Callback<void(const uint8_t *, size_t)> &cb, KeyFormat fmt, const CoderSource &passPhrase) {
		BIO *bp = nullptr;
		auto key = static_cast<const EVP_PKEY *>(ctx.keyCtx);

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
				if (!i2d_PrivateKey_bio(bp, key)) {
					return finalize(false);
				}
			} else {
				log::error("PrivateKey", "exportDer: passPhrase is not supported for KeyFormat::RSA");
				return finalize(false);
			}
			break;
		case KeyFormat::PKCS8:
			if (passPhrase.empty()) {
				if (!i2d_PKCS8PrivateKey_bio(bp, key, NULL, NULL, 0, 0, NULL)) {
					return finalize(false);
				}
			} else {
				if (!i2d_PKCS8PrivateKey_bio(bp, key, EVP_des_ede3_cbc(), reinterpret_cast<const char *>(passPhrase.data()), int(passPhrase.size()), 0, NULL)) {
					return finalize(false);
				}
			}
			break;
		}

		char *buf = nullptr;
		size_t len = BIO_get_mem_data(bp, &buf);
		if (len > 0) {
			cb(reinterpret_cast<uint8_t *>(buf), len);
			return finalize(true);
		}

		return finalize(false);
	},
	.privExportPublic = [] (KeyContext &target, const KeyContext &privKey) {
		auto bp = BIO_new(BIO_s_mem());
		if (!bp) {
			return false;
		}

		if (i2b_PublicKey_bio(bp, static_cast<const EVP_PKEY *>(privKey.keyCtx))) {
			target.keyCtx = b2i_PublicKey_bio(bp);
			target.type = getOpenSSLKeyType(EVP_PKEY_get_id(static_cast<EVP_PKEY *>(target.keyCtx)));
		}

		BIO_free(bp);
		return target.keyCtx != nullptr;
	},
	.privSign = [] (const KeyContext &ctx, const Callback<void(const uint8_t *, size_t)> &cb, const CoderSource &data, SignAlgorithm algo) {
		auto key = static_cast<EVP_PKEY *>(ctx.keyCtx);
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
			if (1 != EVP_DigestSignInit(mdctx, NULL, EVP_sha256(), NULL, key)) {
				return cleanup(false);
			}
			break;
		case SignAlgorithm::RSA_SHA512:
		case SignAlgorithm::ECDSA_SHA512:
			if (1 != EVP_DigestSignInit(mdctx, NULL, EVP_sha512(), NULL, key)) {
				return cleanup(false);
			}
			break;
		case SignAlgorithm::GOST_256:
			if (auto md = EVP_get_digestbyname("md_gost12_256")) {
				if (1 != EVP_DigestSignInit(mdctx, NULL, md, NULL, key)) {
					return cleanup(false);
				}
			} else {
				return cleanup(false);
			}
			break;
		case SignAlgorithm::GOST_512:
			if (auto md = EVP_get_digestbyname("md_gost12_512")) {
				if (1 != EVP_DigestSignInit(mdctx, NULL, md, NULL, key)) {
					return cleanup(false);
				}
			} else {
				return cleanup(false);
			}
			break;
		}

		/* Call update with the message */
		if (1 != EVP_DigestSignUpdate(mdctx, data.data(), data.size())) {
			return cleanup(false);
		}

		if (1 == EVP_DigestSignFinal(mdctx, NULL, &siglen)) {
			sigdata = static_cast<unsigned char *>(OPENSSL_malloc(sizeof(unsigned char) * siglen));
			if (sigdata) {
				if (1 == EVP_DigestSignFinal(mdctx, sigdata, &siglen)) {
					cb(sigdata, siglen);
					return cleanup(true);
				}

			}
		}
		return cleanup(false);
	},
	.privVerify = [] (const KeyContext &ctx, const CoderSource &data, BytesView signature, SignAlgorithm algo) {
		auto key = static_cast<EVP_PKEY *>(ctx.keyCtx);
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
			if (!EVP_DigestVerifyInit(mdctx, NULL, EVP_sha256(), NULL, key)) {
				return cleanup(false);
			}
			break;
		case SignAlgorithm::RSA_SHA512:
		case SignAlgorithm::ECDSA_SHA512:
			if (!EVP_DigestVerifyInit(mdctx, NULL, EVP_sha512(), NULL, key)) {
				return cleanup(false);
			}
			break;
		case SignAlgorithm::GOST_256:
			if (auto md = EVP_get_digestbyname("md_gost12_256")) {
				if (1 != EVP_DigestVerifyInit(mdctx, NULL, md, NULL, key)) {
					return cleanup(false);
				}
			} else {
				return cleanup(false);
			}
			break;
		case SignAlgorithm::GOST_512:
			if (auto md = EVP_get_digestbyname("md_gost12_512")) {
				if (1 != EVP_DigestVerifyInit(mdctx, NULL, md, NULL, key)) {
					return cleanup(false);
				}
			} else {
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
	},
	.privEncrypt = [] (const KeyContext &ctx, const Callback<void(const uint8_t *, size_t)> &cb, const CoderSource &data) {
		auto key = static_cast<EVP_PKEY *>(ctx.keyCtx);
		auto pctx = EVP_PKEY_CTX_new(key, nullptr);

		void *out = nullptr;

		auto cleanup = [&] (bool value) {
			if (out) {
				OPENSSL_free(out);
				out = nullptr;
			}
			if (pctx) {
				EVP_PKEY_CTX_free(pctx);
				pctx = nullptr;
			}
			return value;
		};

		if (EVP_PKEY_encrypt_init(pctx) <= 0) {
			return cleanup(false);
		}

		switch (ctx.type) {
		case KeyType::RSA:
			if (EVP_PKEY_CTX_set_rsa_padding(pctx, OPENSSL_PK_ENCRYPT_PADDING) <= 0) {
				return cleanup(false);
			}
			break;
		default:
			break;
		}

		/* Determine buffer length */
		size_t outlen;
		if (EVP_PKEY_encrypt(pctx, NULL, &outlen, data.data(), data.size()) <= 0) {
			return cleanup(false);
		}

		out = OPENSSL_malloc(outlen);

		if (!out) {
			return cleanup(false);
		}

		if (EVP_PKEY_encrypt(pctx, static_cast<uint8_t *>(out), &outlen, data.data(), data.size()) <= 0) {
			return cleanup(false);
		}

		cb(static_cast<uint8_t *>(out), outlen);

		return cleanup(true);
	},
	.privDecrypt = [] (const KeyContext &ctx, const Callback<void(const uint8_t *, size_t)> &cb, const CoderSource &data) {
		auto key = static_cast<EVP_PKEY *>(ctx.keyCtx);
		auto pctx = EVP_PKEY_CTX_new(key, nullptr);

		void *out = nullptr;

		auto cleanup = [&] (bool value) {
			if (out) {
				OPENSSL_free(out);
				out = nullptr;
			}
			if (pctx) {
				EVP_PKEY_CTX_free(pctx);
				pctx = nullptr;
			}
			return value;
		};

		if (EVP_PKEY_decrypt_init(pctx) <= 0) {
			return cleanup(false);
		}

		switch (ctx.type) {
		case KeyType::RSA:
			if (EVP_PKEY_CTX_set_rsa_padding(pctx, OPENSSL_PK_ENCRYPT_PADDING) <= 0) {
				return cleanup(false);
			}
			break;
		default:
			break;
		}

		/* Determine buffer length */
		size_t outlen;
		if (EVP_PKEY_decrypt(pctx, NULL, &outlen, data.data(), data.size()) <= 0) {
			return cleanup(false);
		}

		out = OPENSSL_malloc(outlen);

		if (!out) {
			return cleanup(false);
		}

		if (EVP_PKEY_decrypt(pctx, static_cast<uint8_t *>(out), &outlen, data.data(), data.size()) <= 0) {
			return cleanup(false);
		}

		cb(static_cast<uint8_t *>(out), outlen);

		return cleanup(true);
	},
	.pubInit = [] (KeyContext &ctx) -> bool {
		ctx.keyCtx = nullptr;
		return true;
	},
	.pubFree = [] (KeyContext &ctx) {
		if (ctx.keyCtx) {
			EVP_PKEY_free( static_cast<EVP_PKEY *>(ctx.keyCtx) );
			ctx.keyCtx = nullptr;
		}
	},
	.pubImport = [] (KeyContext &ctx, BytesView data) {
		BIO *bioData = nullptr;

		auto finalize = [&] (bool value) {
			if (bioData) {
				BIO_free(bioData);
				bioData = nullptr;
			}
			if (!value) {
				ctx.keyCtx = nullptr;
			} else {
				ctx.type = getOpenSSLKeyType(EVP_PKEY_get_id(static_cast<EVP_PKEY *>(ctx.keyCtx)));
			}
			return value;
		};

		bioData = BIO_new_mem_buf((void*)data.data(), data.size());
		if (isPemKey(data)) {
			ctx.keyCtx = PEM_read_bio_PUBKEY(bioData, NULL, NULL, NULL);
			if (ctx.keyCtx) {
				return finalize(true);
			}
		} else {
			ctx.keyCtx = d2i_PUBKEY_bio(bioData, NULL);
			if (ctx.keyCtx) {
				return finalize(true);
			}
		}
		return finalize(false);
	},
	.pubImportOpenSSH = [] (KeyContext &ctx, StringView r) {
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

			auto elen = dataView.readUnsigned32();
			auto exp = dataView.readBytes(elen);

			auto mlen = dataView.readUnsigned32();
			auto modulus = dataView.readBytes(mlen);

			uint8_t out[12_KiB];
			uint8_t *buf = out;
			buf = writeRSAKey(buf, modulus, exp);

			std::stringstream stream;
			stream << "-----BEGIN RSA PUBLIC KEY-----\n";
			base64::encode([&] (char c) {
				stream << c;
			}, CoderSource(out, buf - out));
			stream << "\n-----END RSA PUBLIC KEY-----\n";

			BIO *bioData = nullptr;

			auto finalize = [&] (bool value) {
				if (bioData) {
					BIO_free(bioData);
					bioData = nullptr;
				}
				if (!value) {
					ctx.keyCtx = nullptr;
				} else {
					ctx.type = getOpenSSLKeyType(EVP_PKEY_get_id(static_cast<EVP_PKEY *>(ctx.keyCtx)));
				}
				return value;
			};

			std::string tmp = stream.str();

			bioData = BIO_new_mem_buf(tmp.data(), tmp.size());
			ctx.keyCtx = PEM_read_bio_PUBKEY(bioData, NULL, NULL, NULL);
			if (ctx.keyCtx) {
				return finalize(true);
			}
			return finalize(false);
		}
		return false;
	},
	.pubExportPem = [] (const KeyContext &ctx, const Callback<void(const uint8_t *, size_t)> &cb) {
		auto key = static_cast<EVP_PKEY *>(ctx.keyCtx);
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

		if (!PEM_write_bio_PUBKEY(bp, key)) {
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
	},
	.pubExportDer = [] (const KeyContext &ctx, const Callback<void(const uint8_t *, size_t)> &cb) {
		auto key = static_cast<EVP_PKEY *>(ctx.keyCtx);
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

		if (!i2d_PUBKEY_bio(bp, key)) {
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
	},
	.pubVerify = [] (const KeyContext &ctx, const CoderSource &data, BytesView signature, SignAlgorithm algo) {
		auto key = static_cast<EVP_PKEY *>(ctx.keyCtx);
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
			if (!EVP_DigestVerifyInit(mdctx, NULL, EVP_sha256(), NULL, key)) {
				return cleanup(false);
			}
			break;
		case SignAlgorithm::RSA_SHA512:
		case SignAlgorithm::ECDSA_SHA512:
			if (!EVP_DigestVerifyInit(mdctx, NULL, EVP_sha512(), NULL, key)) {
				return cleanup(false);
			}
			break;
		case SignAlgorithm::GOST_256:
			if (auto md = EVP_get_digestbyname("md_gost12_256")) {
				if (1 != EVP_DigestVerifyInit(mdctx, NULL, md, NULL, key)) {
					return cleanup(false);
				}
			} else {
				return cleanup(false);
			}
			break;
		case SignAlgorithm::GOST_512:
			if (auto md = EVP_get_digestbyname("md_gost12_512")) {
				if (1 != EVP_DigestVerifyInit(mdctx, NULL, md, NULL, key)) {
					return cleanup(false);
				}
			} else {
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
	},
	.pubEncrypt = [] (const KeyContext &ctx, const Callback<void(const uint8_t *, size_t)> &cb, const CoderSource &data) {
		auto key = static_cast<EVP_PKEY *>(ctx.keyCtx);
		auto pctx = EVP_PKEY_CTX_new(key, nullptr);

		void *out = nullptr;

		auto cleanup = [&] (bool value) {
			if (out) {
				OPENSSL_free(out);
				out = nullptr;
			}
			if (pctx) {
				EVP_PKEY_CTX_free(pctx);
				pctx = nullptr;
			}
			return value;
		};

		if (EVP_PKEY_encrypt_init(pctx) <= 0) {
			return cleanup(false);
		}

		switch (ctx.type) {
		case KeyType::RSA:
			if (EVP_PKEY_CTX_set_rsa_padding(pctx, OPENSSL_PK_ENCRYPT_PADDING) <= 0) {
				return cleanup(false);
			}
			break;
		default:
			break;
		}

		/* Determine buffer length */
		size_t outlen;
		if (EVP_PKEY_encrypt(pctx, NULL, &outlen, data.data(), data.size()) <= 0) {
			return cleanup(false);
		}

		out = OPENSSL_malloc(outlen);

		if (!out) {
			return cleanup(false);
		}

		if (EVP_PKEY_encrypt(pctx, static_cast<uint8_t *>(out), &outlen, data.data(), data.size()) <= 0) {
			return cleanup(false);
		}

		cb(static_cast<uint8_t *>(out), outlen);

		return cleanup(true);
	}
};

BackendCtxRef s_openSSLRef(&s_openSSLCtx);

}

#endif
