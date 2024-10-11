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

#ifndef STAPPLER_FILESYSTEM_SPFILEPATH_H_
#define STAPPLER_FILESYSTEM_SPFILEPATH_H_

#include "SPStringView.h"
#include "SPSpanView.h"

namespace STAPPLER_VERSIONIZED stappler {

using FilePath = ValueWrapper<StringView, class FilePathTag>;

}

namespace STAPPLER_VERSIONIZED stappler::filepath {

// check if filepath is absolute
SP_PUBLIC bool isAbsolute(StringView path);

SP_PUBLIC bool isCanonical(StringView path);

// check if filepath is in application bundle
SP_PUBLIC bool isBundled(StringView path);

// check if filepath above it's current root
SP_PUBLIC bool isAboveRoot(StringView path);

// check for ".", ".." and double slashes in path
SP_PUBLIC bool validatePath(StringView path);

// remove any ".", ".." and double slashes from path
template <typename Interface>
SP_PUBLIC auto reconstructPath(StringView path) -> typename Interface::StringType;

// returns current absolute path for file (canonical prefix will be decoded), this path should not be cached
// if writable flag is false, platform path will be returned with canonical prefix %PLATFORM%
// if writable flag is true, system should resolve platform prefix to absolute path, if possible
// if platform path can not be resolved (etc, it's in archive or another FS), empty string will be returned
template <typename Interface>
SP_PUBLIC auto absolute(StringView, bool writable = false) -> typename Interface::StringType;

// encodes path for long-term storage (default application dirs will be replaced with canonical prefix,
// like %CACHE%/dir)
template <typename Interface>
SP_PUBLIC auto canonical(StringView path) -> typename Interface::StringType;

// extract root from path by removing last component (/dir/file.tar.bz -> /dir)
SP_PUBLIC StringView root(StringView path);

// extract last component (/dir/file.tar.bz -> file.tar.bz)
SP_PUBLIC StringView lastComponent(StringView path);
SP_PUBLIC StringView lastComponent(StringView path, size_t allowedComponents);

// extract full filename extension (/dir/file.tar.gz -> tar.gz)
SP_PUBLIC StringView fullExtension(StringView path);

// extract last filename extension (/dir/file.tar.gz -> gz)
SP_PUBLIC StringView lastExtension(StringView path);

// /dir/file.tar.bz -> file
SP_PUBLIC StringView name(StringView path);

// /dir/file.tar.bz -> 2
SP_PUBLIC size_t extensionCount(StringView path);

template <typename Interface>
SP_PUBLIC auto split(StringView) -> typename Interface::template VectorType<StringView>;

SP_PUBLIC void split(StringView, const Callback<void(StringView)> &);

// merges two path component, removes or adds '/' where needed
template <typename Interface>
SP_PUBLIC auto _merge(StringView root, StringView path) -> typename Interface::StringType;

template <typename Interface>
SP_PUBLIC auto merge(SpanView<std::string>) -> typename Interface::StringType;

template <typename Interface>
SP_PUBLIC auto merge(SpanView<memory::string>) -> typename Interface::StringType;

template <typename Interface>
SP_PUBLIC auto merge(SpanView<StringView>) -> typename Interface::StringType;

template <typename Interface>
SP_PUBLIC auto merge(stappler::memory::StandartInterface::StringType &&str) -> typename Interface::StringType;

template <typename Interface>
SP_PUBLIC auto merge(stappler::memory::PoolInterface::StringType &&str) -> typename Interface::StringType;

template <typename Interface, class... Args>
SP_PUBLIC inline auto merge(StringView root, StringView path, Args&&... args) -> typename Interface::StringType {
	return merge<Interface>(_merge<Interface>(root, path), std::forward<Args>(args)...);
}

// translate some MIME Content-Type to common extensions
SP_PUBLIC StringView extensionForContentType(StringView type);

// replace root path component in filepath
// replace(/my/dir/first/file, /my/dir/first, /your/dir/second)
// [/my/dir/first -> /your/dir/second] /file
// /my/dir/first/file -> /your/dir/second/file
template <typename Interface>
SP_PUBLIC auto replace(StringView path, StringView source, StringView dest) -> typename Interface::StringType;

// Implementation

template <typename Interface>
SP_PUBLIC auto reconstructPath(StringView path) -> typename Interface::StringType {
	typename Interface::StringType ret; ret.reserve(path.size());
	bool start = (path.front() == '/');
	bool end = (path.back() == '/');

	typename Interface::template VectorType<StringView> retVec;
	StringView r(path);
	while (!r.empty()) {
		auto str = r.readUntil<StringView::Chars<'/'>>();
		if (str == ".." && str.size() == 2) {
			if (!retVec.empty()) {
				retVec.pop_back();
			}
		} else if ((str == "." && str.size() == 1) || str.size() == 0) {
		} else if (!str.empty()) {
			retVec.emplace_back(str);
		}
		if (r.is('/')) {
			++ r;
		}
	}

	if (start) {
		ret.push_back('/');
	}

	bool f = false;
	for (auto &it : retVec) {
		if (f) {
			ret.push_back('/');
		} else {
			f = true;
		}
		ret.append(it.data(), it.size());
	}
	if (end) {
		ret.push_back('/');
	}
	return ret;
}

template <typename Interface>
SP_PUBLIC inline auto split(StringView str) -> typename Interface::template VectorType<StringView> {
	typename Interface::template VectorType<StringView> ret;
	StringView s(str);
	do {
		if (s.is('/')) {
			s ++;
		}
		auto path = s.readUntil<StringView::Chars<'/', '?', ';', '&', '#'>>();
		ret.push_back(path);
	} while (!s.empty() && s.is('/'));
	return ret;
}

inline void split(StringView str, const Callback<void(StringView)> &cb) {
	StringView s(str);
	do {
		if (s.is('/')) {
			s ++;
		}
		auto path = s.readUntil<StringView::Chars<'/', '?', ';', '&', '#'>>();
		cb(path);
	} while (!s.empty() && s.is('/'));
}

template <typename Interface>
SP_PUBLIC inline auto replace(StringView path, StringView source, StringView dest) -> typename Interface::StringType {
	if (path.starts_with(source)) {
		if (dest.empty()) {
			return path.sub(source.size()).str<Interface>();
		} else {
			return filepath::merge<Interface>(dest, path.sub(source.size()));
		}
	}
	return path.str<Interface>();
}


}

#endif /* STAPPLER_FILESYSTEM_SPFILEPATH_H_ */
