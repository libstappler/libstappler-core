/**
Copyright (c) 2019 Roman Katuntsev <sbkarr@stappler.org>
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

#ifndef STAPPLER_CORE_UTILS_SPVALID_H_
#define STAPPLER_CORE_UTILS_SPVALID_H_

#include "SPStringView.h"
#include "SPBytesView.h"

namespace STAPPLER_VERSIONIZED stappler::valid {

/** Identifier starts with [a-zA-Z_] and can contain [a-zA-Z0-9_\-.@] */
bool validateIdentifier(StringView str);

/** Text can contain all characters above 0x1F and \t, \r, \n, \b, \f */
bool validateText(StringView str);

bool validateEmailWithoutNormalization(StringView str);
bool validateEmail(memory::PoolInterface::StringType &str);
bool validateEmail(memory::StandartInterface::StringType &str);

bool validateUrl(memory::PoolInterface::StringType &str);
bool validateUrl(memory::StandartInterface::StringType &str);

bool validateNumber(const StringView &str);
bool validateHexadecimial(const StringView &str);
bool validateBase64(const StringView &str);

void makeRandomBytes(uint8_t *, size_t);

template <typename Interface>
auto makeRandomBytes(size_t) -> typename Interface::BytesType;

template <typename Interface>
auto makePassword(const StringView &str, const StringView &key = StringView()) -> typename Interface::BytesType;

bool validatePassord(const StringView &str, const BytesView &passwd, const StringView &key = StringView());

static constexpr size_t MIN_GENPASSWORD_LENGTH = 6;

// Minimal length is 6
template <typename Interface>
auto generatePassword(size_t len) -> typename Interface::StringType;

uint32_t readIp(StringView r);
uint32_t readIp(StringView r, bool &err);
Pair<uint32_t, uint32_t> readIpRange(StringView r);

}

#endif /* STAPPLER_CORE_UTILS_SPVALID_H_ */
