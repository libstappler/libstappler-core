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

#include "SPDocEpub.h"
#include "SPDocFormat.h"
#include "SPHtmlParser.h"

namespace STAPPLER_VERSIONIZED stappler::document {

SP_USED static Format s_epubFormat([](memory::pool_t *, FileInfo str, StringView ct) -> bool {
	return DocumentEpub::isEpub(str);
}, [](memory::pool_t *p, FileInfo str, StringView ct) -> Rc<Document> {
	return Rc<DocumentEpub>::create(p, str, ct);
}, [](memory::pool_t *, BytesView str, StringView ct) -> bool {
	return DocumentEpub::isEpub(str);
}, [](memory::pool_t *p, BytesView str, StringView ct) -> Rc<Document> {
	return Rc<DocumentEpub>::create(p, str, ct);
}, 0);

bool DocumentEpub::isEpub(BytesView data) {
	ZipArchive<memory::StandartInterface> zip(data, true);
	if (!zip) {
		return false;
	}

	bool success = false;
	zip.readFile("mimetype", [&](BytesView data) {
		auto str = data.toStringView();
		if (str.equals(StringView("application/epub+zip"))) {
			success = true;
		}
	});
	return success;
}

bool DocumentEpub::isEpub(FileInfo path) {
	auto mem = filesystem::readIntoMemory<mem_std::Interface>(path);
	isEpub(mem);

	ZipArchive<memory::StandartInterface> zip(path);
	if (!zip) {
		return false;
	}

	bool success = false;
	zip.readFile("mimetype", [&](BytesView data) {
		auto str = data.toStringView();
		if (str.equals(StringView("application/epub+zip"))) {
			success = true;
		}
	});
	return success;
}

bool DocumentEpub::init(FileInfo info, StringView ct) {
	if (!Document::init(memory::app_root_pool, [&](memory::pool_t *pool) -> DocumentData * {
		return new (pool) EpubData(pool, info, ct);
	})) {
		return false;
	}

	if (_data) {
		return memory::pool::perform([&] { return static_cast<EpubData *>(_data)->init(); }, _pool);
	}
	return false;
}

bool DocumentEpub::init(BytesView data, StringView ct) {
	if (!Document::init(memory::app_root_pool, [&](memory::pool_t *pool) -> DocumentData * {
		return new (pool) EpubData(pool, data, ct);
	})) {
		return false;
	}

	if (_data) {
		return memory::pool::perform([&] { return static_cast<EpubData *>(_data)->init(); }, _pool);
	}
	return false;
}

bool DocumentEpub::init(memory::pool_t *pool, FileInfo info, StringView ct) {
	if (!Document::init(pool, [&](memory::pool_t *pool) -> DocumentData * {
		return new (pool) EpubData(pool, info, ct);
	})) {
		return false;
	}

	if (_data) {
		return memory::pool::perform([&] { return static_cast<EpubData *>(_data)->init(); }, _pool);
	}
	return false;
}

bool DocumentEpub::init(memory::pool_t *pool, BytesView data, StringView ct) {
	if (!Document::init(pool, [&](memory::pool_t *pool) -> DocumentData * {
		return new (pool) EpubData(pool, data, ct);
	})) {
		return false;
	}

	if (_data) {
		return memory::pool::perform([&] { return static_cast<EpubData *>(_data)->init(); }, _pool);
	}
	return false;
}

EpubData::~EpubData() { }

EpubData::EpubData(memory::pool_t *p, FileInfo info, StringView ct)
: DocumentData(p), archive(info) {
	type = ct.pdup(p);
}

EpubData::EpubData(memory::pool_t *p, BytesView data, StringView ct)
: DocumentData(p), archive(data, true) {
	type = ct.pdup(p);
}

static StringView _readEpubRootPath(StringView container) {
	struct EpubXmlContentReader {
		using Parser = html::Parser<EpubXmlContentReader>;
		using Tag = Parser::Tag;
		using StringReader = Parser::StringReader;

		inline void onTagAttribute(Parser &p, Tag &tag, StringReader &name, StringReader &value) {
			if (tag.name.equals("rootfile") && name.equals("full-path")) {
				result = value;
				p.cancel();
			}
		}

		StringView result;
	} r;

	html::parse(r, StringViewUtf8((const char *)container.data(), container.size()));
	return r.result;
};

static void _processRootEpubPublication(EpubData *data, StringView content) {
	/*struct Reader {
		using Parser = html::Parser<Reader>;
		using Tag = Parser::Tag;
		using StringReader = Parser::StringReader;

		struct Item {
			StringView id;
			StringView href;
			StringView type;
			StringView props;

			void clear() {
				id.clear();
				href.clear();
				type.clear();
				props.clear();
			}
		} item;

		enum Section {
			None,
			Package,
			Metadata,
			Manifest,
			Spine
		} section = None;

		Reader(EpubData *d) : data(d) { }

		inline void onBeginTag(Parser &p, Tag &tag) {
			switch (section) {
			case Metadata:
				if (tag.name.is("dc:")) {
					metaProps.emplace_back();
					auto &prop = metaProps.back();
					prop.name = String(tag.name.data() + 3, tag.name.size() - 3);
				} else if (tag.name.compare("meta") || tag.name.compare("opf:meta")) {
					metaProps.emplace_back();
				}
				break;
			case Manifest:
				if (tag.name.compare("item") || tag.name.compare("opf:item")) {
					item.clear();
				}
				break;
			case Spine:
				if (tag.name.compare("itemref") || tag.name.compare("opf:itemref")) {
					spineVec->emplace_back();
					SpineFile &spine = spineVec->back();
					spine.linear = true;
				}
				break;
			default: break;
			}
		}

		inline void onTagAttribute(Parser &p, Tag &tag, StringReader &name, StringReader &value) {
			switch (section) {
			case None:
				if (tag.name.compare("package") || tag.name.compare("opf:package")) {
					if (name.compare("version")) {
						version = value.str();
					} else if (name.compare("unique-identifier")) {
						uid = value.str();
					}
				}
				break;
			case Package:
				if ((tag.name.compare("spine") || tag.name.compare("opf:spine"))
						&& (name.compare("toc") || name.compare("opf:toc"))) {
					auto it = manifestIds.find(value.str());
					if (it != manifestIds.end()) {
						tocFile = it->second->path;
					}
				}
				break;
			case Metadata:
				if (tag.name.is("dc:")) {
					auto &prop = metaProps.back();
					if (name.compare("id")) {
						prop.id = value.str();
					} else if (name.compare("lang") || name.compare("xml-lang")) {
						prop.lang = locale::common(value.str());
					} else if (name.compare("scheme") || name.compare("opf:scheme")) {
						prop.scheme = value.str();
					}
				} else if (tag.name.compare("meta") || tag.name.compare("opf:meta")) {
					MetaProp &meta = metaProps.back();
					if (name.compare("id")) {
						meta.id = value.str();
					} else if (name.compare("property") || name.compare("name")) {
						meta.name = value.str();
					} else if (name.compare("scheme") || name.compare("opf:scheme")) {
						meta.scheme = value.str();
					} else if (name.compare("xml:lang")) {
						meta.lang = value.str();
					} else if (name.compare("content")) {
						meta.value = value.str();
					} else if (name.compare("refines")) {
						meta.refines = value.str();
					}
				}
				break;
			case Manifest:
				if (tag.name.compare("item") || tag.name.compare("opf:item")) {
					if (name.compare("id")) {
						item.id = value.str();
					} else if (name.compare("href")) {
						item.href = value.str();
					} else if (name.compare("media-type")) {
						item.type = value.str();
					} else if (name.compare("properties")) {
						item.props = value.str();
					}
				}
				break;
			case Spine:
				if (tag.name.compare("itemref") || tag.name.compare("opf:itemref")) {
					SpineFile &spine = spineVec->back();
					if (name.compare("idref")) {
						auto itemIt = manifestIds.find(value.str());
						if (itemIt != manifestIds.end()) {
							spine.entry = itemIt->second;
						}
					} else if (name.compare("linear") && value.compare("no")) {
						spine.linear = false;
					} else if (name.compare("properties")) {
						string::split(value, " ",
								[&](const StringView &str) { spine.props.emplace(str.str()); });
					}
				}
				break;
			default: break;
			}
		}

		inline void onPushTag(Parser &p, Tag &tag) {
			switch (section) {
			case None:
				if (tag.name.compare("package") || tag.name.compare("opf:package")) {
					section = Package;
				}
				break;
			case Package:
				if (tag.name.compare("metadata") || tag.name.compare("opf:metadata")) {
					section = Metadata;
				} else if (tag.name.compare("manifest") || tag.name.compare("opf:manifest")) {
					section = Manifest;
				} else if (tag.name.compare("spine") || tag.name.compare("opf:spine")) {
					section = Spine;
				}
				break;
			default: break;
			}
		}

		void updateMeta(MetaProp &prop, const String &key) {
			auto metaIt = metaIds.find(key);
			if (metaIt != metaIds.end()) {
				MetaProp *refined = nullptr;
				Vector<size_t> path = metaIt->second;
				for (auto &it : path) {
					if (refined == nullptr) {
						refined = &metaProps.at(it);
					} else {
						refined = &refined->extra.at(it);
					}
				}
				if (refined) {
					refined->extra.emplace_back(std::move(prop));
					auto &ref = refined->extra.back();
					if (!ref.id.empty()) {
						path.push_back(refined->extra.size() - 1);
						metaIds.emplace(ref.id, std::move(path));
					}
				}
			}
		}

		inline void popMetaTag() {
			MetaProp &prop = metaProps.back();
			if (!prop.refines.empty()) {
				updateMeta(prop, prop.refines.substr(1));
				metaProps.pop_back();
			} else {
				if (!prop.id.empty()) {
					metaIds.emplace(prop.id, Vector<size_t>{metaProps.size() - 1});
				}
			}
		}

		inline void onPopTag(Parser &p, Tag &tag) {
			switch (section) {
			case Package:
				if (tag.name.compare("package") || tag.name.compare("opf:package")) {
					section = None;
				}
				break;
			case Metadata:
				if (tag.name.compare("metadata") || tag.name.compare("opf:metadata")) {
					section = Package;
				} else if (tag.name.is("dc:") || tag.name.compare("meta")
						|| tag.name.compare("opf:meta")) {
					popMetaTag();
				}
				break;
			case Manifest:
				if (tag.name.compare("manifest") || tag.name.compare("opf:manifest")) {
					section = Package;
				}
				break;
			case Spine:
				if (tag.name.compare("spine") || tag.name.compare("opf:spine")) {
					section = Package;
				}
				break;
			default: break;
			}
		}

		inline void onManifestTag() {
			if (!item.id.empty() && !item.href.empty() && !item.type.empty()) {
				String path(*rootPath);
				path.append("/").append(item.href);
				auto fileIt = manifest->find(path);
				if (fileIt != manifest->end()) {
					fileIt->second.id = std::move(item.id);
					fileIt->second.mime = std::move(item.type);
					if (!item.props.empty()) {
						string::split(item.props, " ", [&fileIt](const StringView &r) {
							fileIt->second.props.insert(r.str());
						});
					}
					manifestIds.emplace(fileIt->second.id, &fileIt->second);
					auto &m = fileIt->second.mime;
					if (m == "text/html"
							|| m.compare(0, "application/xhtml"_len, "application/xhtml") == 0) {
						fileIt->second.type = ManifestFile::Source;
					} else if (m == "text/css") {
						fileIt->second.type = ManifestFile::Css;
					}
				}
			}
		}

		inline void onInlineTag(Parser &p, Tag &tag) {
			switch (section) {
			case Metadata:
				if (tag.name.is("dc:") || tag.name.compare("meta")
						|| tag.name.compare("opf:meta")) {
					popMetaTag();
				}
				break;
			case Manifest:
				if (tag.name.compare("item") || tag.name.compare("opf:item")) {
					onManifestTag();
				}
				break;
			case Spine:
				if (tag.name.compare("itemref") || tag.name.compare("opf:itemref")) {
					SpineFile &spine = spineVec->back();
					if (!spine.entry) {
						spineVec->pop_back();
					}
				}
				break;
			default: break;
			}
		}

		inline void onTagContent(Parser &p, Tag &tag, StringReader &s) {
			switch (section) {
			case Metadata:
				if (tag.name.is("dc:") || tag.name.compare("meta")
						|| tag.name.compare("opf:meta")) {
					auto &prop = metaProps.back();
					prop.value = s.str();
					string::trim(prop.value);
				}
				break;
			default: break;
			}
		}

		StringView version;
		StringView uid;
		StringView tocFile;
		EpubData *data = nullptr;

		Vector<MetaProp> metaProps;
		mem_pool::Map<StringView, mem_pool::Vector<size_t>> metaIds;
		mem_pool::Map<StringView, const EpubManifestFile *> manifestIds;

		mem_pool::String *rootPath = nullptr;
		mem_pool::Map<StringView, EpubManifestFile> *manifest = nullptr;
		Vector<SpineFile> *spineVec = nullptr;
	} r(&data);

	html::parse(r, StringViewUtf8((const char *)opf.data(), opf.size()));*/

	/*_meta.meta = std::move(r.metaProps);
	for (auto &it : _meta.meta) {
		if (it.id == r.uid) {
			_uniqueId = it.value;
		}

		if (it.name == "title") {
			_meta.titles.emplace_back(TitleMeta{it.value});
			TitleMeta &title = _meta.titles.back();

			if (!it.lang.empty()) {
				title.localizedTitle.emplace(it.lang, it.value);
			}

			for (auto &eit : it.extra) {
				if (eit.name == "alternate-script") {
					title.localizedTitle.emplace(eit.lang, it.value);
				} else if (eit.name == "display-seq") {
					title.sequence = StringToNumber<int64_t>(eit.value.c_str(), nullptr, 0);
				} else if (eit.name == "title-type") {
					if (eit.value == "main") {
						title.type = TitleMeta::Main;
					} else if (eit.value == "subtitle") {
						title.type = TitleMeta::Subtitle;
					} else if (eit.value == "short") {
						title.type = TitleMeta::Short;
					} else if (eit.value == "collection") {
						title.type = TitleMeta::Collection;
					} else if (eit.value == "edition") {
						title.type = TitleMeta::Edition;
					} else if (eit.value == "expanded") {
						title.type = TitleMeta::Expanded;
					}
				}
			}
		} else if (it.name == "contributor" || it.name == "creator") {
			_meta.authors.emplace_back(AuthorMeta{it.value,
				(it.name == "contributor") ? AuthorMeta::Contributor : AuthorMeta::Creator});

			AuthorMeta &author = _meta.authors.back();

			if (!it.lang.empty()) {
				author.localizedName.emplace(it.lang, it.value);
			}

			for (auto &eit : it.extra) {
				if (eit.name == "alternate-script") {
					author.localizedName.emplace(eit.lang, it.value);
				} else if (eit.name == "role") {
					author.role = eit.value;
					author.roleScheme = eit.scheme;
				}
			}
		} else if (it.name == "dcterms:modified") {
			_modified = it.value;
		} else if (it.name == "belongs-to-collection") {
			_meta.collections.emplace_back(CollectionMeta{it.value});
			auto &col = _meta.collections.back();
			if (!it.lang.empty()) {
				col.localizedTitle.emplace(it.lang, it.value);
			}

			for (auto &eit : it.extra) {
				if (eit.name == "collection-type") {
					col.type = eit.value;
				} else if (eit.name == "group-position") {
					col.position = eit.value;
				} else if (eit.name == "alternate-script") {
					col.localizedTitle.emplace(eit.lang, eit.value);
				} else if (eit.name == "dcterms:identifier") {
					col.uid = eit.value;
				}
			}
		}
	}

	String coverFile, tocFile;
	for (auto &it : _manifest) {
		if (it.second.props.find("cover-image") != it.second.props.end()) {
			coverFile = it.second.path;
		}
		if (it.second.props.find("nav") != it.second.props.end()) {
			tocFile = it.second.path;
		}
		if (!coverFile.empty() && !tocFile.empty()) {
			break;
		}
	}

	if (!coverFile.empty()) {
		_coverFile = coverFile;
	}

	if (!tocFile.empty()) {
		_tocFile = tocFile;
	} else {
		_tocFile = r.tocFile;
	}*/
}

bool EpubData::init() {
	if (!archive) {
		return false;
	}

	StringView rootPath;

	archive.ftw([&](uint64_t index, StringView path, size_t size, Time time) {
		if (path == "META-INF/container.xml") {
			archive.readFile(index, [&](BytesView data) {
				rootPath = _readEpubRootPath(data.readString());
				rootPath = rootPath.pdup(pool);
			});
		}

		path = path.pdup(pool);
		manifestFiles.emplace(path,
				EpubManifestFile{
					index,
					path,
					size,
				});
	});

	if (rootPath.empty()) {
		return false;
	}

	auto fIt = manifestFiles.find(rootPath);
	if (fIt == manifestFiles.end()) {
		return false;
	}

	archive.readFile(fIt->second.index,
			[&](BytesView data) { _processRootEpubPublication(this, data.toStringView()); });

	return true;
}

} // namespace stappler::document
