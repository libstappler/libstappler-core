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

#include "SPFilepath.h"
#include "SPFilesystem.h"

namespace STAPPLER_VERSIONIZED stappler::filepath {

SPUNUSED static bool inAppBundle(StringView path) {
	if (filepath::isAbsolute(path)) {
		return false;
	}
	if (filepath::isBundled(path) ||
			(!filepath::isAboveRoot(path) && filesystem::platform::_exists(path))) {
		return true;
	}
	return false;
}

// check if filepath is absolute
bool isAbsolute(StringView path) {
	if (path.empty()) {
		return true;
	}
	return path[0] == '/';
}

bool isCanonical(StringView path) {
	if (path.empty()) {
		return false;
	}
	return path[0] == '%';
}

// check if filepath is in application bundle
bool isBundled(StringView path) {
	if (path.size() >= "%PLATFORM%:"_len) {
		return path.starts_with("%PLATFORM%:");
	}
	return false;
}

// check if filepath above it's current root
bool isAboveRoot(StringView path) {
	size_t components = 0;
	StringView r(path);
	while (!r.empty()) {
		auto str = r.readUntil<StringView::Chars<'/'>>();
		if (str == ".." && str.size() == 2) {
			if (components == 0) {
				return true;
			}
			-- components;
		} else if ((str == "." && str.size() == 1) || str.size() == 0) {return false;
		} else {
			++ components;
		}
		if (r.is('/')) {
			++ r;
		}
	}
	return false;
}

// check for ".", ".." and double slashes in path
bool validatePath(StringView path) {
	StringView r(path);
	if (r.is('/')) {
		++ r;
	}
	while (!r.empty()) {
		auto str = r.readUntil<StringView::Chars<'/'>>();
		if ((str == ".." && str.size() == 2) || (str == "." && str.size() == 1) || str.size() == 0) {
			return false;
		}
		if (r.is('/')) {
			++ r;
		}
	}
	return true;
}

template <typename Interface>
auto absolute_fn(StringView path, bool writable) -> typename Interface::StringType {
	if (path.empty()) {
		return typename Interface::StringType();
	}
	if (path.front() == '%') {
		if (path.starts_with("%CACHE%")) {
			return filesystem::cachesPath<Interface>(path.sub(7), true);
		} else if (path.starts_with("%DOCUMENTS%")) {
			return filesystem::documentsPath<Interface>(path.sub(11), true);
		} else if (path.starts_with("%WRITEABLE%")) {
			return filesystem::writablePath<Interface>(path.sub(11), true);
		} else if (path.starts_with("%CURRENT%")) {
			return filesystem::currentDir<Interface>(path.sub(9), true);
		} else if (path.starts_with("%PLATFORM%:")) {
			writable = false;
		}
	}

	if (isAbsolute(path)) {
		return validatePath(path)?path.str<Interface>():reconstructPath<Interface>(path);
	}

	if (!writable && !isAboveRoot(path)) {
		if (validatePath(path)) {
			return filesystem::platform::_exists(path)?path.str<Interface>():filesystem::writablePath<Interface>(path);
		} else {
			auto ret = reconstructPath<Interface>(path);
			return filesystem::platform::_exists(ret)?ret:filesystem::writablePath<Interface>(ret);
		}
	}

	return validatePath(path)
			? filesystem::writablePath<Interface>(path)
			: reconstructPath<Interface>(filesystem::writablePath<Interface>(path));
}

template<>
auto absolute<memory::StandartInterface>(StringView path, bool writable) -> memory::StandartInterface::StringType {
	return absolute_fn<memory::StandartInterface>(path, writable);
}

template<>
auto absolute<memory::PoolInterface>(StringView path, bool writable) -> memory::PoolInterface::StringType {
	return absolute_fn<memory::PoolInterface>(path, writable);
}

template <typename Interface>
auto canonical_fn(StringView path) -> typename Interface::StringType {
	if (path.empty()) {
		return typename Interface::StringType();
	}
	if (path.front() == '%') {
		return path.str<Interface>();
	}

	bool isPlatform = filepath::isBundled(path);
	if (!isPlatform && inAppBundle(path)) {
		return StringView::merge<Interface>("%PLATFORM%:", path);
	} else if (isPlatform) {
		return path.str<Interface>();
	}

	auto cachePath = filesystem::cachesPath<Interface>();
	if (path.starts_with(StringView(cachePath))) {
		return merge<Interface>("%CACHE%", path.sub(cachePath.size()));
	}

	auto documentsPath = filesystem::documentsPath<Interface>();
	if (path.starts_with(StringView(documentsPath)) == 0) {
		return merge<Interface>("%DOCUMENTS%", path.sub(documentsPath.size()));
	}

	auto writablePath = filesystem::writablePath<Interface>();
	if (path.starts_with(StringView(writablePath)) == 0) {
		return merge<Interface>("%WRITEABLE%", path.sub(writablePath.size()));
	}

	auto currentDir = filesystem::currentDir<Interface>();
	if (path.starts_with(StringView(currentDir)) == 0) {
		return merge<Interface>("%CURRENT%", path.sub(currentDir.size()));
	}

	return path.str<Interface>();
}

template<>
auto canonical<memory::StandartInterface>(StringView path) -> memory::StandartInterface::StringType {
	return canonical_fn<memory::StandartInterface>(path);
}

template<>
auto canonical<memory::PoolInterface>(StringView path) -> memory::PoolInterface::StringType {
	return canonical_fn<memory::PoolInterface>(path);
}

StringView root(StringView path) {
	size_t pos = path.rfind('/');
	if (pos == maxOf<size_t>()) {
		return StringView();
	} else {
		if (pos == 0) {
			return StringView("/");
		} else {
			return path.sub(0, pos);
		}
	}
}

StringView lastComponent(StringView path) {
	size_t pos = path.rfind('/');
	if (pos != maxOf<size_t>()) {
		return path.sub(pos + 1);
	} else {
		return path;
	}
}

StringView lastComponent(StringView path, size_t allowedComponents) {
	if (allowedComponents == 0) {
		return "";
	}
	size_t pos = path.rfind('/');
	allowedComponents --;
	if (pos == 0) {
		pos = maxOf<size_t>();
	}

	while (pos != maxOf<size_t>() && allowedComponents > 0) {
		pos = path.rfind('/', pos - 1);
		allowedComponents --;
		if (pos == 0) {
			pos = maxOf<size_t>();
		}
	}

	if (pos != maxOf<size_t>()) {
		return path.sub(pos + 1);
	} else {
		return path;
	}
}

StringView fullExtension(StringView path) {
	auto cmp = lastComponent(path);

	size_t pos = cmp.find('.');
	if (pos == maxOf<size_t>()) {
		return StringView();
	} else {
		return cmp.sub(pos + 1);
	}
}

StringView lastExtension(StringView path) {
	auto cmp = lastComponent(path);

	size_t pos = cmp.rfind('.');
	if (pos == maxOf<size_t>()) {
		return "";
	} else {
		return cmp.sub(pos + 1);
	}
}

StringView name(StringView path) {
	auto cmp = lastComponent(path);

	size_t pos = cmp.find('.');
	if (pos == maxOf<size_t>()) {
		return cmp;
	} else {
		return cmp.sub(0, pos);
	}
}

template <typename Interface>
auto do_merge(StringView root, StringView path) -> typename Interface::StringType {
	if (path.empty()) {
		return root.str<Interface>();
	}
	return StringView::merge<Interface, '/'>(root, path);
}


template <typename SourceVector>
static size_t getMergeSize(const SourceVector &vec) {
	size_t ret = vec.size();
	for (auto it = vec.begin(); it != vec.end(); it ++) {
		ret += it->size();
	}
	return ret;
}

template <typename Buf, typename SourceVector>
static void doMerge(Buf &out, const SourceVector &vec) {
	auto stripSeps = [] (auto str) {
		StringView tmp(str);
		tmp.trimChars<StringView::Chars<'/'>>();
		return tmp;
	};

	bool hasSeparator = true;
	auto it = vec.begin();
	for (; it != vec.end(); it ++) {
		if (it->empty()) {
			continue;
		}

		if (!hasSeparator) {
			out.push_back('/');
		} else {
			hasSeparator = false;
		}

		auto tmp = stripSeps(*it);
		out.append(tmp.data(), tmp.size());
	}
}

template <>
auto _merge<memory::StandartInterface>(StringView root, StringView path) -> memory::StandartInterface::StringType {
	return do_merge<memory::StandartInterface>(root, path);
}

template <>
auto _merge<memory::PoolInterface>(StringView root, StringView path) -> memory::PoolInterface::StringType {
	return do_merge<memory::PoolInterface>(root, path);
}

template <>
auto merge<memory::StandartInterface>(SpanView<std::string> vec) -> memory::StandartInterface::StringType {
	memory::StandartInterface::StringType ret; ret.reserve(getMergeSize(vec));
	doMerge(ret, vec);
	return ret;
}

template <>
auto merge<memory::PoolInterface>(SpanView<std::string> vec) -> memory::PoolInterface::StringType {
	memory::PoolInterface::StringType ret; ret.reserve(getMergeSize(vec));
	doMerge(ret, vec);
	return ret;
}


template <>
auto merge<memory::StandartInterface>(SpanView<memory::string> vec) -> memory::StandartInterface::StringType {
	memory::StandartInterface::StringType ret; ret.reserve(getMergeSize(vec));
	doMerge(ret, vec);
	return ret;
}

template <>
auto merge<memory::PoolInterface>(SpanView<memory::string> vec) -> memory::PoolInterface::StringType {
	memory::PoolInterface::StringType ret; ret.reserve(getMergeSize(vec));
	doMerge(ret, vec);
	return ret;
}


template <>
auto merge<memory::StandartInterface>(SpanView<StringView> vec) -> memory::StandartInterface::StringType {
	memory::StandartInterface::StringType ret; ret.reserve(getMergeSize(vec));
	doMerge(ret, vec);
	return ret;
}

template <>
auto merge<memory::PoolInterface>(SpanView<StringView> vec) -> memory::PoolInterface::StringType {
	memory::PoolInterface::StringType ret; ret.reserve(getMergeSize(vec));
	doMerge(ret, vec);
	return ret;
}


template <>
auto merge<memory::StandartInterface>(stappler::memory::StandartInterface::StringType &&str) -> memory::StandartInterface::StringType {
	return str;
}

template <>
auto merge<memory::PoolInterface>(stappler::memory::StandartInterface::StringType &&str) -> memory::PoolInterface::StringType {
	return StringView(str).str<memory::PoolInterface>();
}


template <>
auto merge<memory::StandartInterface>(stappler::memory::PoolInterface::StringType &&str) -> memory::StandartInterface::StringType {
	return StringView(str).str<memory::StandartInterface>();
}

template <>
auto merge<memory::PoolInterface>(stappler::memory::PoolInterface::StringType &&str) -> memory::PoolInterface::StringType {
	return str;
}

size_t extensionCount(StringView path) {
	size_t ret = 0;
	auto cmp = lastComponent(path);
	for (auto c : cmp) {
		if (c == '.') { ret ++; }
	}
	return ret;
}

StringView extensionForContentType(StringView ct) {
	// TODO: reimplement with list from Serenity
	if (ct.compare("application/pdf") == 0 || ct.compare("application/x-pdf") == 0) {
		return ".pdf";
	} else if (ct.compare("image/jpeg") == 0 || ct.compare("image/pjpeg") == 0) {
		return ".jpeg";
	} else if (ct.compare("image/png") == 0) {
		return ".png";
	} else if (ct.compare("image/gif") == 0) {
		return ".gif";
	} else if (ct.compare("image/tiff") == 0) {
		return ".tiff";
	} else if (ct.compare("application/json") == 0) {
		return ".json";
	} else if (ct.compare("application/zip") == 0) {
		return ".zip";
	}
	return StringView();
}

}
