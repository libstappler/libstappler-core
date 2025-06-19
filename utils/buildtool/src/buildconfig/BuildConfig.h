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

#ifndef CORE_UTILS_BUILDTOOL_SRC_BUILDCONFIG_BUILDCONFIG_H_
#define CORE_UTILS_BUILDTOOL_SRC_BUILDCONFIG_BUILDCONFIG_H_

#include "SPMakefile.h"
#include "SPStringView.h"

namespace stappler::buildtool {

makefile::String getExpression(makefile::Makefile *make, StringView str);
makefile::String getVariable(makefile::Makefile *make, StringView str);

void makeBuildConfigHeader(makefile::Makefile *, const CallbackStream &);
void makeAppConfigHeader(makefile::Makefile *, const CallbackStream &);
void makeAppConfigSource(makefile::Makefile *, const CallbackStream &);

void makeMergedAppConfigSource(makefile::Makefile *release, makefile::Makefile *debug, const CallbackStream &);

} // namespace stappler::buildtool

#endif /* CORE_UTILS_BUILDTOOL_SRC_BUILDCONFIG_BUILDCONFIG_H_ */
