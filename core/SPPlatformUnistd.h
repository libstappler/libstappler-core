/**
 Copyright (c) 2023-2024 Stappler LLC <admin@stappler.dev>

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

#ifndef CORE_CORE_SPPLATFORMUNISTD_H_
#define CORE_CORE_SPPLATFORMUNISTD_H_

#include "SPPlatformInit.h"

#if WIN32

// Suppress windows warnings for cross-compilation with clang
#if XWIN
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wnonportable-include-path"
#pragma clang diagnostic ignored "-Wignored-attributes"
#pragma clang diagnostic ignored "-Wmicrosoft-include"
#pragma clang diagnostic ignored "-Wignored-pragma-intrinsic"
#pragma clang diagnostic ignored "-Wpragma-pack"
#pragma clang diagnostic ignored "-Wunknown-pragmas"
#pragma clang diagnostic ignored "-Wunused-local-typedef"
#pragma clang diagnostic ignored "-Wmacro-redefined"
#pragma clang diagnostic ignored "-Wcomment"
#pragma clang diagnostic ignored "-Wunused-value"
#endif

#ifndef _MSC_VER
#warning Not MSC compiler
#endif

#include <intrin.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/utime.h>
#include <io.h>
#include <direct.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

#include <strsafe.h>
#include <esent.h>

#include <windows.h>
#include <windowsx.h>
#include <wincrypt.h>

#ifdef interface
#undef interface
#endif

#ifdef DELETE
#undef DELETE
#endif

using pid_t = DWORD;

#define PATH_MAX MAX_PATH

#else

#include <dirent.h>
#include <utime.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/stat.h>

#if LINUX
#include <sys/mman.h>
#endif

#endif

#if XWIN
#pragma clang diagnostic pop
#endif

#endif /* CORE_CORE_SPPLATFORMUNISTD_H_ */
