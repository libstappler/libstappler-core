/**
Copyright (c) 2017-2022 Roman Katuntsev <sbkarr@stappler.org>
Copyright (c) 2023-2025 Stappler LLC <admin@stappler.dev>

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
#include "SPSharedModule.cc"

#include "platform/SPCore-posix.cc"
#include "platform/SPCore-linux.cc"
#include "platform/SPCore-android.cc"
#include "platform/SPCore-win32.cc"
#include "platform/SPCore-darwin.cc"

#include "SPUrl.cc"
#include "SPValid.cc"
#include "SPCommandLineParser.cc"

#define STAPPLER_VERSION_VARIANT 0

#ifndef STAPPLER_VERSION_API
#define STAPPLER_VERSION_API 0
#endif

#ifndef STAPPLER_VERSION_REV
#define STAPPLER_VERSION_REV 0
#endif

#ifndef STAPPLER_VERSION_BUILD
#define STAPPLER_VERSION_BUILD 0
#endif

namespace STAPPLER_VERSIONIZED stappler {

#define STRINGIFY(x) #x
#define TOSTRING(x) STRINGIFY(x)

const char * getStapplerVersionString() {
	return TOSTRING(STAPPLER_VERSION_VARIANT) "." TOSTRING(STAPPLER_VERSION_API) "." TOSTRING(STAPPLER_VERSION_REV) "." TOSTRING(STAPPLER_VERSION_BUILD);
}

uint32_t getStapplerVersionIndex() {
	return SP_MAKE_API_VERSION(STAPPLER_VERSION_VARIANT, STAPPLER_VERSION_API, STAPPLER_VERSION_REV, STAPPLER_VERSION_BUILD);
}

uint32_t getStapplerVersionVariant() {
	return STAPPLER_VERSION_VARIANT;
}

uint32_t getStapplerVersionApi() {
	return STAPPLER_VERSION_API;
}

uint32_t getStapplerVersionRev() {
	return STAPPLER_VERSION_REV;
}

uint32_t getStapplerVersionBuild() {
	return STAPPLER_VERSION_BUILD;
}

}
