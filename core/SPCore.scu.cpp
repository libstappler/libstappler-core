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

#include "SPCommon.h"

#include "SPIO.cc"
#include "SPMemPoolAllocator.cc"
#include "SPMemPoolApr.cc"
#include "SPMemPoolHash.cc"
#include "SPMemPoolInterface.cc"
#include "SPMemPoolPool.cc"
#include "SPMemPoolUtils.cc"
#include "SPMemAlloc.cc"
#include "SPMemRbtree.cc"
#include "SPMemUserData.cc"
#include "SPMemUuid.cc"
#include "SPBase64.cc"
#include "SPCharGroup.cc"
#include "SPSha2.cc"
#include "SPGost3411-2012.cc"
#include "SPString.cc"
#include "SPUnicode.cc"
#include "SPCommon.h"
#include "SPHtmlParser.cc"
#include "SPLog.cc"
#include "SPRef.cc"
#include "SPTime.cc"
#include "SPDso.cc"

#include "platform/SPCore-linux.cc"
#include "platform/SPCore-android.cc"
#include "platform/SPCore-win32.cc"

#ifdef MODULE_STAPPLER_DATA
#include "SPData.cc"
#include "SPDataUrlencoded.cc"
#endif

#include "SPUrl.cc"
#include "SPValid.cc"

#ifdef MODULE_STAPPLER_SQL
#include "SPSql.cc"
#endif

namespace STAPPLER_VERSIONIZED stappler {

#define STRINGIFY(x) #x
#define TOSTRING(x) STRINGIFY(x)

const char * getStapplerVersionString() {
	return TOSTRING(STAPPLER_VERSION_NUMBER) "/" TOSTRING(STAPPLER_VERSION_BUILD);
}

uint32_t getStapplerVersionNumber() {
	return STAPPLER_VERSION_NUMBER;
}

uint32_t getStapplerVersionBuild() {
	return STAPPLER_VERSION_BUILD;
}

}
