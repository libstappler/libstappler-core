/**
 Copyright (c) 2024 Stappler LLC <admin@stappler.dev>

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
#include "SPBitmap.h"

namespace STAPPLER_VERSIONIZED stappler::bitmap {

static SharedSymbol s_bitmapSharedSymbols[] = {
	SharedSymbol{"detectFormat(StringView)",
		(void *)static_cast<Pair<FileFormat, StringView> (*) (StringView)>(detectFormat)},
	SharedSymbol{"detectFormat(io::Producer const&)",
		(void *)static_cast<Pair<FileFormat, StringView> (*) (io::Producer const&)>(detectFormat)},
	SharedSymbol{"detectFormat(uint8_t const*, size_t)",
		(void *)static_cast<Pair<FileFormat, StringView> (*) (uint8_t const*, size_t)>(detectFormat)},
	SharedSymbol{"getMimeType(FileFormat)",
		(void *)static_cast<StringView (*) (FileFormat)>(getMimeType)},
	SharedSymbol{"getMimeType(StringView)",
		(void *)static_cast<StringView (*) (StringView)>(getMimeType)},
};

static SharedModule s_bitmapSharedModule("bitmap", s_bitmapSharedSymbols, sizeof(s_bitmapSharedSymbols) / sizeof(SharedSymbol));

}
