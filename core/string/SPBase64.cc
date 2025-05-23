/**
Copyright (c) 2016-2022 Roman Katuntsev <sbkarr@stappler.org>
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

namespace STAPPLER_VERSIONIZED stappler::base64 {

using Reader = BytesViewNetwork;

// Mapping from 6 bit pattern to ASCII character.
static const char *base64EncodeLookup =
		"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

// Definition for "masked-out" areas of the base64DecodeLookup mapping
#define xx 65

static unsigned char base64DecodeLookup[256] = {
	xx,
	xx,
	xx,
	xx,
	xx,
	xx,
	xx,
	xx,
	xx,
	xx,
	xx,
	xx,
	xx,
	xx,
	xx,
	xx,
	xx,
	xx,
	xx,
	xx,
	xx,
	xx,
	xx,
	xx,
	xx,
	xx,
	xx,
	xx,
	xx,
	xx,
	xx,
	xx,
	xx,
	xx,
	xx,
	xx,
	xx,
	xx,
	xx,
	xx,
	xx,
	xx,
	xx,
	62,
	xx,
	62,
	xx,
	63,
	52,
	53,
	54,
	55,
	56,
	57,
	58,
	59,
	60,
	61,
	xx,
	xx,
	xx,
	xx,
	xx,
	xx,
	xx,
	0,
	1,
	2,
	3,
	4,
	5,
	6,
	7,
	8,
	9,
	10,
	11,
	12,
	13,
	14,
	15,
	16,
	17,
	18,
	19,
	20,
	21,
	22,
	23,
	24,
	25,
	xx,
	xx,
	xx,
	xx,
	63,
	xx,
	26,
	27,
	28,
	29,
	30,
	31,
	32,
	33,
	34,
	35,
	36,
	37,
	38,
	39,
	40,
	41,
	42,
	43,
	44,
	45,
	46,
	47,
	48,
	49,
	50,
	51,
	xx,
	xx,
	xx,
	xx,
	xx,
	xx,
	xx,
	xx,
	xx,
	xx,
	xx,
	xx,
	xx,
	xx,
	xx,
	xx,
	xx,
	xx,
	xx,
	xx,
	xx,
	xx,
	xx,
	xx,
	xx,
	xx,
	xx,
	xx,
	xx,
	xx,
	xx,
	xx,
	xx,
	xx,
	xx,
	xx,
	xx,
	xx,
	xx,
	xx,
	xx,
	xx,
	xx,
	xx,
	xx,
	xx,
	xx,
	xx,
	xx,
	xx,
	xx,
	xx,
	xx,
	xx,
	xx,
	xx,
	xx,
	xx,
	xx,
	xx,
	xx,
	xx,
	xx,
	xx,
	xx,
	xx,
	xx,
	xx,
	xx,
	xx,
	xx,
	xx,
	xx,
	xx,
	xx,
	xx,
	xx,
	xx,
	xx,
	xx,
	xx,
	xx,
	xx,
	xx,
	xx,
	xx,
	xx,
	xx,
	xx,
	xx,
	xx,
	xx,
	xx,
	xx,
	xx,
	xx,
	xx,
	xx,
	xx,
	xx,
	xx,
	xx,
	xx,
	xx,
	xx,
	xx,
	xx,
	xx,
	xx,
	xx,
	xx,
	xx,
	xx,
	xx,
	xx,
	xx,
	xx,
	xx,
	xx,
	xx,
	xx,
	xx,
	xx,
	xx,
	xx,
	xx,
	xx,
	xx,
	xx,
	xx,
	xx,
	xx,
	xx,
};

// Fundamental sizes of the binary and base64 encode/decode units in bytes
constexpr int BinaryUnit = 3;
constexpr int Base64Unit = 4;

size_t encodeSize(size_t l) { return ((l / BinaryUnit) + ((l % BinaryUnit) ? 1 : 0)) * Base64Unit; }
size_t decodeSize(size_t l) { return ((l + Base64Unit - 1) / Base64Unit) * BinaryUnit; }

template <typename Callback>
static void make_encode(const CoderSource &data, const Callback &cb) {
	Reader inputBuffer(data.data(), data.size());
	auto length = inputBuffer.size();

	size_t i = 0;
	for (; i + BinaryUnit - 1 < length; i += BinaryUnit) {
		// Inner loop: turn 48 bytes into 64 base64 characters
		cb(base64EncodeLookup[(inputBuffer[i] & 0xFC) >> 2]);
		cb(base64EncodeLookup[((inputBuffer[i] & 0x03) << 4) | ((inputBuffer[i + 1] & 0xF0) >> 4)]);
		cb(base64EncodeLookup[((inputBuffer[i + 1] & 0x0F) << 2)
				| ((inputBuffer[i + 2] & 0xC0) >> 6)]);
		cb(base64EncodeLookup[inputBuffer[i + 2] & 0x3F]);
	}

	if (i + 1 < length) {
		// Handle the single '=' case
		cb(base64EncodeLookup[(inputBuffer[i] & 0xFC) >> 2]);
		cb(base64EncodeLookup[((inputBuffer[i] & 0x03) << 4) | ((inputBuffer[i + 1] & 0xF0) >> 4)]);
		cb(base64EncodeLookup[(inputBuffer[i + 1] & 0x0F) << 2]);
		cb('=');
	} else if (i < length) {
		// Handle the double '=' case
		cb(base64EncodeLookup[(inputBuffer[i] & 0xFC) >> 2]);
		cb(base64EncodeLookup[(inputBuffer[i] & 0x03) << 4]);
		cb('=');
		cb('=');
	}
}

template <typename Callback>
static void make_decode(const CoderSource &data, const Callback &cb) {
	StringView inputBuffer((char *)data.data(), data.size());
	auto length = inputBuffer.size();

	size_t i = 0;
	while (i < length) {
		// Accumulate 4 valid characters (ignore everything else)
		unsigned char accumulated[Base64Unit];
		size_t accumulateIndex = 0;
		while (i < length) {
			unsigned char decode = base64DecodeLookup[(unsigned char)inputBuffer[i++]];
			if (decode != xx) {
				accumulated[accumulateIndex] = decode;
				accumulateIndex++;
				if (accumulateIndex == Base64Unit) {
					break;
				}
			}
		}

		if (accumulateIndex >= 2) {
			cb((accumulated[0] << 2) | (accumulated[1] >> 4));
		}
		if (accumulateIndex >= 3) {
			cb((accumulated[1] << 4) | (accumulated[2] >> 2));
		}
		if (accumulateIndex >= 4) {
			cb((accumulated[2] << 6) | accumulated[3]);
		}
	}
}

#undef xx

typename memory::PoolInterface::StringType __encode_pool(const CoderSource &source) {
	typename memory::PoolInterface::StringType output;
	output.reserve(encodeSize(source.size()));
	make_encode(source, [&](const char &c) { output.push_back(c); });
	return output;
}
typename memory::StandartInterface::StringType __encode_std(const CoderSource &source) {
	typename memory::StandartInterface::StringType output;
	output.reserve(encodeSize(source.size()));
	make_encode(source, [&](const char &c) { output.push_back(c); });
	return output;
}

template <>
auto encode<memory::PoolInterface>(const CoderSource &source) ->
		typename memory::PoolInterface::StringType {
	return __encode_pool(source);
}

template <>
auto encode<memory::StandartInterface>(const CoderSource &source) ->
		typename memory::StandartInterface::StringType {
	return __encode_std(source);
}

void encode(std::basic_ostream<char> &stream, const CoderSource &source) {
	make_encode(source, [&](const char &c) { stream << c; });
}

void encode(const Callback<void(char)> &cb, const CoderSource &source) { make_encode(source, cb); }

size_t encode(char *buf, size_t bsize, const CoderSource &source) {
	size_t accum = 0;
	if (bsize < encodeSize(source.size())) {
		make_encode(source, [&](const char &c) {
			if (accum < bsize) {
				buf[accum++] = c;
			}
		});
	} else {
		make_encode(source, [&](const char &c) { buf[accum++] = c; });
	}
	return accum;
}

typename memory::PoolInterface::BytesType __decode_pool(const CoderSource &source) {
	typename memory::PoolInterface::BytesType output;
	output.reserve(encodeSize(source.size()));
	make_decode(source, [&](const uint8_t &c) { output.emplace_back(c); });
	return output;
}
typename memory::StandartInterface::BytesType __decode_std(const CoderSource &source) {
	typename memory::StandartInterface::BytesType output;
	output.reserve(encodeSize(source.size()));
	make_decode(source, [&](const uint8_t &c) { output.emplace_back(c); });
	return output;
}
void decode(std::basic_ostream<char> &stream, const CoderSource &source) {
	make_decode(source, [&](const uint8_t &c) { stream << char(c); });
}

void decode(const Callback<void(uint8_t)> &cb, const CoderSource &source) {
	make_decode(source, cb);
}

size_t decode(uint8_t *buf, size_t bsize, const CoderSource &source) {
	size_t accum = 0;
	if (bsize < decodeSize(source.size())) {
		make_decode(source, [&](const uint8_t &c) {
			if (accum < bsize) {
				buf[accum++] = c;
			}
		});
	} else {
		make_decode(source, [&](const uint8_t &c) { buf[accum++] = c; });
	}
	return accum;
}

} // namespace stappler::base64

namespace STAPPLER_VERSIONIZED stappler::base64url {

using Reader = BytesViewNetwork;

// Mapping from 6 bit pattern to ASCII character.
static const char *base64EncodeLookup =
		"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-_";

// Fundamental sizes of the binary and base64 encode/decode units in bytes
constexpr int BinaryUnit = 3;
constexpr int Base64Unit = 4;

template <typename Callback>
static void make_encode(const CoderSource &data, const Callback &cb) {
	Reader inputBuffer(data.data(), data.size());
	auto length = inputBuffer.size();

	size_t i = 0;
	for (; i + BinaryUnit - 1 < length; i += BinaryUnit) {
		// Inner loop: turn 48 bytes into 64 base64 characters
		cb(base64EncodeLookup[(inputBuffer[i] & 0xFC) >> 2]);
		cb(base64EncodeLookup[((inputBuffer[i] & 0x03) << 4) | ((inputBuffer[i + 1] & 0xF0) >> 4)]);
		cb(base64EncodeLookup[((inputBuffer[i + 1] & 0x0F) << 2)
				| ((inputBuffer[i + 2] & 0xC0) >> 6)]);
		cb(base64EncodeLookup[inputBuffer[i + 2] & 0x3F]);
	}

	if (i + 1 < length) {
		// Handle the single '=' case
		cb(base64EncodeLookup[(inputBuffer[i] & 0xFC) >> 2]);
		cb(base64EncodeLookup[((inputBuffer[i] & 0x03) << 4) | ((inputBuffer[i + 1] & 0xF0) >> 4)]);
		cb(base64EncodeLookup[(inputBuffer[i + 1] & 0x0F) << 2]);
	} else if (i < length) {
		// Handle the double '=' case
		cb(base64EncodeLookup[(inputBuffer[i] & 0xFC) >> 2]);
		cb(base64EncodeLookup[(inputBuffer[i] & 0x03) << 4]);
	}
}

typename memory::PoolInterface::StringType __encode_pool(const CoderSource &source) {
	typename memory::PoolInterface::StringType output;
	output.reserve(encodeSize(source.size()));
	make_encode(source, [&](const char &c) { output.push_back(c); });
	return output;
}
typename memory::StandartInterface::StringType __encode_std(const CoderSource &source) {
	typename memory::StandartInterface::StringType output;
	output.reserve(encodeSize(source.size()));
	make_encode(source, [&](const char &c) { output.push_back(c); });
	return output;
}

template <>
auto encode<memory::PoolInterface>(const CoderSource &source) ->
		typename memory::PoolInterface::StringType {
	return __encode_pool(source);
}

template <>
auto encode<memory::StandartInterface>(const CoderSource &source) ->
		typename memory::StandartInterface::StringType {
	return __encode_std(source);
}

void encode(std::basic_ostream<char> &stream, const CoderSource &source) {
	make_encode(source, [&](const char &c) { stream << c; });
}

void encode(const Callback<void(char)> &cb, const CoderSource &source) { make_encode(source, cb); }

size_t encode(char *buf, size_t bsize, const CoderSource &source) {
	size_t accum = 0;
	if (bsize < encodeSize(source.size())) {
		make_encode(source, [&](const char &c) {
			if (accum < bsize) {
				buf[accum++] = c;
			}
		});
	} else {
		make_encode(source, [&](const char &c) { buf[accum++] = c; });
	}
	return accum;
}

} // namespace stappler::base64url

namespace STAPPLER_VERSIONIZED stappler::base16 {

using Reader = BytesViewNetwork;

static const char *s_hexTable_lower[256] = {"00", "01", "02", "03", "04", "05", "06", "07", "08",
	"09", "0a", "0b", "0c", "0d", "0e", "0f", "10", "11", "12", "13", "14", "15", "16", "17", "18",
	"19", "1a", "1b", "1c", "1d", "1e", "1f", "20", "21", "22", "23", "24", "25", "26", "27", "28",
	"29", "2a", "2b", "2c", "2d", "2e", "2f", "30", "31", "32", "33", "34", "35", "36", "37", "38",
	"39", "3a", "3b", "3c", "3d", "3e", "3f", "40", "41", "42", "43", "44", "45", "46", "47", "48",
	"49", "4a", "4b", "4c", "4d", "4e", "4f", "50", "51", "52", "53", "54", "55", "56", "57", "58",
	"59", "5a", "5b", "5c", "5d", "5e", "5f", "60", "61", "62", "63", "64", "65", "66", "67", "68",
	"69", "6a", "6b", "6c", "6d", "6e", "6f", "70", "71", "72", "73", "74", "75", "76", "77", "78",
	"79", "7a", "7b", "7c", "7d", "7e", "7f", "80", "81", "82", "83", "84", "85", "86", "87", "88",
	"89", "8a", "8b", "8c", "8d", "8e", "8f", "90", "91", "92", "93", "94", "95", "96", "97", "98",
	"99", "9a", "9b", "9c", "9d", "9e", "9f", "a0", "a1", "a2", "a3", "a4", "a5", "a6", "a7", "a8",
	"a9", "aa", "ab", "ac", "ad", "ae", "af", "b0", "b1", "b2", "b3", "b4", "b5", "b6", "b7", "b8",
	"b9", "ba", "bb", "bc", "bd", "be", "bf", "c0", "c1", "c2", "c3", "c4", "c5", "c6", "c7", "c8",
	"c9", "ca", "cb", "cc", "cd", "ce", "cf", "d0", "d1", "d2", "d3", "d4", "d5", "d6", "d7", "d8",
	"d9", "da", "db", "dc", "dd", "de", "df", "e0", "e1", "e2", "e3", "e4", "e5", "e6", "e7", "e8",
	"e9", "ea", "eb", "ec", "ed", "ee", "ef", "f0", "f1", "f2", "f3", "f4", "f5", "f6", "f7", "f8",
	"f9", "fa", "fb", "fc", "fd", "fe", "ff"};

static const char *s_hexTable_upper[256] = {"00", "01", "02", "03", "04", "05", "06", "07", "08",
	"09", "0A", "0B", "0C", "0D", "0E", "0F", "10", "11", "12", "13", "14", "15", "16", "17", "18",
	"19", "1A", "1B", "1C", "1D", "1E", "1F", "20", "21", "22", "23", "24", "25", "26", "27", "28",
	"29", "2A", "2B", "2C", "2D", "2E", "2F", "30", "31", "32", "33", "34", "35", "36", "37", "38",
	"39", "3A", "3B", "3C", "3D", "3E", "3F", "40", "41", "42", "43", "44", "45", "46", "47", "48",
	"49", "4A", "4B", "4C", "4D", "4E", "4F", "50", "51", "52", "53", "54", "55", "56", "57", "58",
	"59", "5A", "5B", "5C", "5D", "5E", "5F", "60", "61", "62", "63", "64", "65", "66", "67", "68",
	"69", "6A", "6B", "6C", "6D", "6E", "6F", "70", "71", "72", "73", "74", "75", "76", "77", "78",
	"79", "7A", "7B", "7C", "7D", "7E", "7F", "80", "81", "82", "83", "84", "85", "86", "87", "88",
	"89", "8A", "8B", "8C", "8D", "8E", "8F", "90", "91", "92", "93", "94", "95", "96", "97", "98",
	"99", "9A", "9B", "9C", "9D", "9E", "9F", "A0", "A1", "A2", "A3", "A4", "A5", "A6", "A7", "A8",
	"A9", "AA", "AB", "AC", "AD", "AE", "AF", "B0", "B1", "B2", "B3", "B4", "B5", "B6", "B7", "B8",
	"B9", "BA", "BB", "BC", "BD", "BE", "BF", "C0", "C1", "C2", "C3", "C4", "C5", "C6", "C7", "C8",
	"C9", "CA", "CB", "CC", "CD", "CE", "CF", "D0", "D1", "D2", "D3", "D4", "D5", "D6", "D7", "D8",
	"D9", "DA", "DB", "DC", "DD", "DE", "DF", "E0", "E1", "E2", "E3", "E4", "E5", "E6", "E7", "E8",
	"E9", "EA", "EB", "EC", "ED", "EE", "EF", "F0", "F1", "F2", "F3", "F4", "F5", "F6", "F7", "F8",
	"F9", "FA", "FB", "FC", "FD", "FE", "FF"};

static uint8_t s_decTable[256] = {
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	1,
	2,
	3,
	4,
	5,
	6,
	7,
	8,
	9,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	10,
	11,
	12,
	13,
	14,
	15,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	10,
	11,
	12,
	13,
	14,
	15,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
};

size_t encodeSize(size_t length) { return length * 2; }
size_t decodeSize(size_t length) { return length / 2; }

const char *charToHex(const char &c, bool upper) {
	return upper ? s_hexTable_upper[reinterpret_cast<const uint8_t &>(c)]
				 : s_hexTable_lower[reinterpret_cast<const uint8_t &>(c)];
}

uint8_t hexToChar(const char &c) { return s_decTable[reinterpret_cast<const uint8_t &>(c)]; }

uint8_t hexToChar(const char &c, const char &d) { return (hexToChar(c) << 4) | hexToChar(d); }


template <>
auto encode<memory::PoolInterface>(const CoderSource &source, bool upper) ->
		typename memory::PoolInterface::StringType {
	Reader inputBuffer(source.data(), source.size());
	const auto length = inputBuffer.size();

	memory::PoolInterface::StringType output;
	output.resize(length * 2);
	for (size_t i = 0; i < length; ++i) {
		memcpy(output.data() + i * 2,
				upper ? s_hexTable_upper[inputBuffer[i]] : s_hexTable_lower[inputBuffer[i]], 2);
	}
	return output;
}

template <>
auto encode<memory::StandartInterface>(const CoderSource &source, bool upper) ->
		typename memory::StandartInterface::StringType {
	Reader inputBuffer(source.data(), source.size());
	const auto length = inputBuffer.size();

	memory::StandartInterface::StringType output;
	output.reserve(length * 2);
	for (size_t i = 0; i < length; ++i) {
		output.append(upper ? s_hexTable_upper[inputBuffer[i]] : s_hexTable_lower[inputBuffer[i]],
				2);
	}

	return output;
}

void encode(std::basic_ostream<char> &stream, const CoderSource &source, bool upper) {
	const auto length = source.size();
	for (size_t i = 0; i < length; ++i) {
		stream << (upper ? s_hexTable_upper[source[i]] : s_hexTable_lower[source[i]]);
	}
}
void encode(const Callback<void(char)> &cb, const CoderSource &source, bool upper) {
	const auto length = source.size();
	for (size_t i = 0; i < length; ++i) {
		auto &t = upper ? s_hexTable_upper[source[i]] : s_hexTable_lower[source[i]];
		cb(t[0]);
		cb(t[1]);
	}
}
size_t encode(char *buf, size_t bsize, const CoderSource &source, bool upper) {
	Reader inputBuffer(source.data(), source.size());
	const auto length = inputBuffer.size();

	size_t bytes = 0;
	for (size_t i = 0; i < length; ++i) {
		if (bytes + 2 <= bsize) {
			memcpy(buf + i * 2,
					upper ? s_hexTable_upper[inputBuffer[i]] : s_hexTable_lower[inputBuffer[i]], 2);
			bytes += 2;
		} else {
			break;
		}
	}
	return bytes;
}

template <>
auto decode<memory::PoolInterface>(const CoderSource &source) ->
		typename memory::PoolInterface::BytesType {
	const auto length = source.size();
	memory::PoolInterface::BytesType outputBuffer;
	outputBuffer.reserve(length / 2);
	for (size_t i = 0; i < length; i += 2) {
		outputBuffer.push_back((s_decTable[source[i]] << 4) | s_decTable[source[i + 1]]);
	}
	return outputBuffer;
}

template <>
auto decode<memory::StandartInterface>(const CoderSource &source) ->
		typename memory::StandartInterface::BytesType {
	const auto length = source.size();
	memory::StandartInterface::BytesType outputBuffer;
	outputBuffer.reserve(length / 2);
	for (size_t i = 0; i < length; i += 2) {
		outputBuffer.push_back((s_decTable[source[i]] << 4) | s_decTable[source[i + 1]]);
	}
	return outputBuffer;
}

void decode(std::basic_ostream<char> &stream, const CoderSource &source) {
	const auto length = source.size();

	for (size_t i = 0; i < length; i += 2) {
		stream << char((s_decTable[source[i]] << 4) | s_decTable[source[i + 1]]);
	}
}
void decode(const Callback<void(uint8_t)> &cb, const CoderSource &source) {
	const auto length = source.size();

	for (size_t i = 0; i < length; i += 2) {
		cb(uint8_t((s_decTable[source[i]] << 4) | s_decTable[source[i + 1]]));
	}
}
size_t decode(uint8_t *buf, size_t bsize, const CoderSource &source) {
	const auto length = source.size();

	size_t bytes = 0;
	for (size_t i = 0; i < length; i += 2) {
		if (bytes + 1 <= bsize) {
			buf[bytes] = uint8_t((s_decTable[source[i]] << 4) | s_decTable[source[i + 1]]);
			++bytes;
		} else {
			break;
		}
	}
	return bytes;
}

} // namespace stappler::base16
