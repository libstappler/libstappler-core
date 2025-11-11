/**
Copyright (c) 2025 Stappler Team <admin@stappler.org>

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

#include "SPSharedModule.h"

#ifdef STAPPLER_ABI_REQUIRED

#include <unicode/uchar.h>
#include <unicode/ustring.h>
#include <unicode/uidna.h>

namespace STAPPLER_VERSIONIZED stappler::abi {

static SharedSymbol s_abiIcuSharedSymbols[] = {
	SharedSymbol("u_tolower", &u_tolower),
	SharedSymbol("u_toupper", &u_toupper),
	SharedSymbol("u_totitle", &u_totitle),
	SharedSymbol("u_strToLower", &u_strToLower),
	SharedSymbol("u_strToUpper", &u_strToUpper),
	SharedSymbol("u_strToTitle", &u_strToTitle),
	SharedSymbol("u_strCompare", &u_strCompare),
	SharedSymbol("u_strCaseCompare", &u_strCaseCompare),
	SharedSymbol("u_errorName", &u_errorName),
	SharedSymbol("uidna_openUTS46", &uidna_openUTS46),
	SharedSymbol("uidna_close", &uidna_close),
	SharedSymbol("uidna_labelToASCII_UTF8", &uidna_labelToASCII_UTF8),
	SharedSymbol("uidna_labelToUnicodeUTF8", &uidna_labelToUnicodeUTF8),
	SharedSymbol("uidna_nameToASCII_UTF8", &uidna_nameToASCII_UTF8),
	SharedSymbol("uidna_nameToUnicodeUTF8", &uidna_nameToUnicodeUTF8),
};

SP_USED static SharedModule s_abiIcuSharedModule("__abi__:libicuuc.so", s_abiIcuSharedSymbols,
		sizeof(s_abiIcuSharedSymbols) / sizeof(SharedSymbol));

} // namespace stappler::abi

#endif
