/**
Copyright (c) 2016-2022 Roman Katuntsev <sbkarr@stappler.org>
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

#ifndef STAPPLER_DB_SPDBFIELD_H_
#define STAPPLER_DB_SPDBFIELD_H_

#include "SPDbBackendInterface.h"
#include "SPDbQueryList.h"

namespace STAPPLER_VERSIONIZED stappler::db {

template <typename F, typename V>
struct FieldOption;

enum class Type {
	None,
	Integer, // 64-bit signed integer
	Float, // 64-bit float
	Boolean, // true/false
	Text, // simple text
	Bytes, // binary data
	Data, // raw Value
	Extra, // raw binary object
	Object, // linked object (one-to-one connection or endpoint for many-to-one)
	Set, // linked set of objects (one-to-many)
	Array, // set of raw Value
	File,
	Image,
	View, // immutable predicate-based reference set of objects
	FullTextView, // full-text search resource
	Virtual,

	Custom
};

enum class Flags : uint32_t {
	/** empty flag */
	None = 0,

	Required = 1
			<< 0, /** field is required to create a new object, this field can not be removed from object */
	Protected = 1 << 1, /** field does not appear in client's direct output */
	ReadOnly = 1 << 2, /** field can not be modified by client's edit request */

	Reference = 1 << 3, /** object or set stored by reference */
	Unique = 1 << 4, /** field or array should contain only unique values */
	// deprecated: AutoNamed = 1 << 5, /** field will be automatically filled with new UUID */
	AutoCTime = 1 << 6, /** Property will be automatically set to object creation time */
	AutoMTime = 1 << 7, /** Property will be automatically update to modification time */
	AutoUser = 1 << 8, /** Property will be automatically set to current user (if available) */
	Indexed = 1 << 9, /** Create index, that allows select queries on that field */
	Admin = 1 << 10, /** Field can be accessed by administrative queries only */
	ForceInclude = 1
			<< 11, /** field will be internally included in all queries (useful for access control) */
	ForceExclude = 1 << 12, /** field will be excluded, if not requested directly */
	Composed = 1
			<< 13, /** propagate modification events from objects in that field (for object and set fields) */
	Compressed = 1 << 14, /** Try to compress data field with lz-hc (incompatible with pg-cbor) */
	Enum = 1
			<< 15, /** Value is enumeration with fixed (or low-distributed) number of values (enables more effective index in MDB) */
	PatternIndexed = (1 << 9)
			| (1 << 16), /** Create index, that allows select queries with textual patterns (also enables normal index) */
	TrigramIndexed = (1 << 9)
			| (1 << 17), /** enable trigram index on this field (also enables normal index) */
};

SP_DEFINE_ENUM_AS_MASK(Flags)

inline bool checkIfComparationIsValid(Type t, Comparation c, Flags f) {
	switch (t) {
	case Type::Integer:
	case Type::Object:
		switch (c) {
		case Comparation::Includes:
		case Comparation::Prefix:
		case Comparation::Suffix:
		case Comparation::WordPart: return false; break;
		default: return true; break;
		}
		break;
	case Type::Float:
		switch (c) {
		case Comparation::Includes:
		case Comparation::In:
		case Comparation::NotIn:
		case Comparation::Prefix:
		case Comparation::Suffix:
		case Comparation::WordPart: return false; break;
		default: return true; break;
		}
		break;
	case Type::Bytes:
	case Type::Boolean:
		switch (c) {
		case Comparation::Equal:
		case Comparation::NotEqual:
		case Comparation::IsNull:
		case Comparation::IsNotNull: return true; break;
		default: return false; break;
		}
		break;
	case Type::Data:
	case Type::Extra:
	case Type::FullTextView:
		switch (c) {
		case Comparation::IsNull:
		case Comparation::IsNotNull: return true; break;
		default: return false; break;
		}
		break;
	case Type::Text:
		switch (c) {
		case Comparation::Equal:
		case Comparation::NotEqual:
		case Comparation::IsNull:
		case Comparation::IsNotNull:
		case Comparation::In:
		case Comparation::NotIn: return true; break;
		case Comparation::Prefix:
		case Comparation::Suffix:
		case Comparation::WordPart:
			return ((f & Flags::PatternIndexed) != Flags::None)
					|| ((f & Flags::TrigramIndexed) != Flags::None);
			break;
		default: return false; break;
		}
		break;
	default: return false; break;
	}
	return false;
}

enum class Transform {
	None,

	// Text
	Text,
	Identifier,
	Alias,
	Url,
	Email,
	Number,
	Hexadecimial,
	Base64,

	// Bytes
	Uuid,
	PublicKey,

	// Extra
	Array, // handle extra field as array

	Password, // deprecated
};

enum class ValidationLevel {
	NamesAndTypes,
	Slots,
	Full,
};

enum class Linkage {
	Auto,
	Manual,
	None,
};

using MinLength = stappler::ValueWrapper<size_t, class MinLengthTag>; // min utf8 length for string
using MaxLength = stappler::ValueWrapper<size_t, class MaxLengthTag>; // max utf8 length for string
using PasswordSalt =
		stappler::ValueWrapper<StringView, class PasswordSaltTag>; // hashing salt for password
using ForeignLink =
		stappler::ValueWrapper<StringView, class ForeignLinkTag>; // name for foreign linked field
using Documentation = stappler::ValueWrapper<StringView,
		class DocumentationTag>; // tag for field documentation text

// policy for images, that do not match bounds
enum class ImagePolicy {
	Resize, // resize to match bounds
	Reject, // reject input field
};

// max size for files
using MaxFileSize = stappler::ValueWrapper<size_t, class MaxFileSizeTag>;

struct SP_PUBLIC MaxImageSize {
	size_t width = 128;
	size_t height = 128;
	ImagePolicy policy = ImagePolicy::Resize;

	MaxImageSize() : width(128), height(128), policy(ImagePolicy::Resize) { }
	MaxImageSize(size_t w, size_t h, ImagePolicy p = ImagePolicy::Resize)
	: width(w), height(h), policy(p) { }
};

struct SP_PUBLIC MinImageSize {
	size_t width = 0;
	size_t height = 0;
	ImagePolicy policy = ImagePolicy::Reject;

	MinImageSize() : width(0), height(0), policy(ImagePolicy::Reject) { }
	MinImageSize(size_t w, size_t h, ImagePolicy p = ImagePolicy::Reject)
	: width(w), height(h), policy(p) { }
};

struct SP_PUBLIC Thumbnail {
	size_t width;
	size_t height;
	String name;

	Thumbnail(String &&name, size_t w, size_t h) : width(w), height(h), name(sp::move(name)) { }
};

// what to do if object is removed
enum class RemovePolicy {
	Cascade, // remove object in set or field
	Restrict, // reject request, if object or set is not empty
	Reference, // no linkage action, object is reference
	StrongReference, // only for Set: no linkage action, objects will be owned
	Null, // set object to null
};

// old-fashion filter fn (use WriteFilterFn instead)
using FilterFn = Function<bool(const Scheme &, Value &)>;

// function to deduce default value from object data
using DefaultFn = Function<Value(const Value &)>;

// function to modify out value of object's field to return it to users
using ReadFilterFn = Function<bool(const Scheme &, const Value &obj, Value &value)>;

// function to modify input value of object's field to write it into storage
using WriteFilterFn =
		Function<bool(const Scheme &, const Value &patch, Value &value, bool isCreate)>;

// function to replace previous value of field with another
using ReplaceFilterFn =
		Function<bool(const Scheme &, const Value &obj, const Value &oldValue, Value &newValue)>;

// function to deduce root object ids list from object of external scheme
// Used by:
// - View field: to deduce id of root object id from external objects
// - AutoField: to deduce id of object with auto field from external objects
using ViewLinkageFn = Function<Vector<uint64_t>(const Scheme &targetScheme, const Scheme &objScheme,
		const Value &obj)>;

// function to deduce view data from object of external scheme
using ViewFn = Function<bool(const Scheme &objScheme, const Value &obj)>;

// function to extract fulltext search data from object
using FullTextViewFn = Function<FullTextVector(const Scheme &objScheme, const Value &obj)>;

// function to prepare fulltext query from input string
using FullTextQueryFn = Function<FullTextQuery(const Value &searchData)>;

using VirtualReadFn = Function<Value(const Scheme &objScheme, const Value &)>;

using VirtualWriteFn = Function<bool(const Scheme &objScheme, const Value &, Value &)>;

struct SP_PUBLIC AutoFieldScheme : AllocBase {
	using ReqVec = Vector<String>;

	const Scheme &scheme;
	ReqVec requiresForAuto; // fields, that should be updated to trigger auto field update

	ViewLinkageFn linkage;
	ReqVec requiresForLinking;

	AutoFieldScheme(const Scheme &, ReqVec && = ReqVec(), ViewLinkageFn && = nullptr,
			ReqVec && = ReqVec());
	AutoFieldScheme(const Scheme &, ReqVec &&, ReqVec &&);
};

struct SP_PUBLIC AutoFieldDef {
	Vector<AutoFieldScheme> schemes;
	DefaultFn defaultFn;
	Vector<String>
			requireFields; // fields to acquire from field's scheme object when defaultFn is called
};

// definition for scheme's unique constraints
struct SP_PUBLIC UniqueConstraintDef {
	StringView name;
	Vector<StringView> fields;
};

struct SP_PUBLIC CustomFieldInfo {
	bool isIndexable = false;
	String typeName;

	Function<Value(const FieldCustom &, const ResultCursor &, size_t field)> readFromStorage;
	Function<bool(const FieldCustom &, QueryInterface &, StringStream &, const Value &)>
			writeToStorage;

	Function<String(const FieldCustom &)> getIndexName;
	Function<String(const FieldCustom &)> getIndexDefinition;

	Function<bool(const FieldCustom &, Comparation)> isComparationAllowed;

	Function<void(const FieldCustom &, const Scheme &,
			stappler::sql::Query<db::Binder, Interface>::WhereContinue &, Operator,
			const StringView &, Comparation, const Value &, const Value &)>
			writeQuery;

	Function<void(const FieldCustom &, const Scheme &,
			stappler::sql::Query<db::Binder, Interface>::SelectFrom &, Comparation cmp,
			const Value &val, const Value &)>
			writeFrom;
};

struct FieldCustom;

class SP_PUBLIC Field : public AllocBase {
public:
	template <typename... Args>
	static Field Data(String &&name, Args &&...args);
	template <typename... Args>
	static Field Integer(String &&name, Args &&...args);
	template <typename... Args>
	static Field Float(String &&name, Args &&...args);
	template <typename... Args>
	static Field Boolean(String &&name, Args &&...args);
	template <typename... Args>
	static Field Text(String &&name, Args &&...args);
	template <typename... Args>
	static Field Bytes(String &&name, Args &&...args);
	template <typename... Args>
	static Field Password(String &&name, Args &&...args);
	template <typename... Args>
	static Field Extra(String &&name, Args &&...args);
	template <typename... Args>
	static Field Extra(String &&name, stappler::InitializerList<Field> &&, Args &&...args);
	template <typename... Args>
	static Field File(String &&name, Args &&...args);
	template <typename... Args>
	static Field Image(String &&name, Args &&...args);
	template <typename... Args>
	static Field Object(String &&name, Args &&...args);
	template <typename... Args>
	static Field Set(String &&name, Args &&...args);
	template <typename... Args>
	static Field Array(String &&name, Args &&...args);
	template <typename... Args>
	static Field View(String &&name, Args &&...args);
	template <typename... Args>
	static Field FullTextView(String &&name, Args &&...args);
	template <typename... Args>
	static Field Virtual(String &&name, Args &&...args);
	template <typename... Args>
	static Field Custom(FieldCustom *);

	struct Slot : public AllocBase {
	public:
		virtual ~Slot() { }

		template <typename F, typename T>
		static void setOptions(F &f, T &&t) {
			FieldOption<F, typename std::remove_reference<T>::type>::assign(f, std::forward<T>(t));
		}

		template <typename F, typename T, typename... Args>
		static void setOptions(F &f, T &&t, Args &&...args) {
			setOptions(f, std::forward<T>(t));
			setOptions(f, std::forward<Args>(args)...);
		}

		template <typename F>
		static void init(F &f) { };

		template <typename F, typename... Args>
		static void init(F &f, Args &&...args) {
			setOptions(f, std::forward<Args>(args)...);
		};

		Slot(String &&n, Type t) : name(n), type(t) { }

		StringView getName() const { return name; }
		bool hasFlag(Flags f) const { return ((flags & f) != Flags::None); }
		Type getType() const { return type; }
		bool isProtected() const;
		Transform getTransform() const { return transform; }

		virtual bool isSimpleLayout() const {
			return type == Type::Integer || type == Type::Float || type == Type::Boolean
					|| type == Type::Text || type == Type::Bytes || type == Type::Data
					|| type == Type::Extra || type == Type::Virtual;
		}

		virtual bool isDataLayout() const { return type == Type::Data || type == Type::Extra; }

		bool isIndexed() const {
			return hasFlag(Flags::Indexed) || transform == Transform::Alias || type == Type::Object;
		}
		bool isFile() const { return type == Type::File || type == Type::Image; }

		virtual bool hasDefault() const;
		virtual Value getDefault(const Value &patch) const;

		virtual bool transformValue(const Scheme &, const Value &, Value &, bool isCreate) const;
		virtual void hash(StringStream &stream, ValidationLevel l) const;

		Value def;
		String name;
		String documentation;
		Flags flags = Flags::None;
		Type type = Type::None;
		Transform transform = Transform::None;
		DefaultFn defaultFn;

		ReadFilterFn readFilterFn;
		WriteFilterFn writeFilterFn;
		ReplaceFilterFn replaceFilterFn;

		AutoFieldDef autoField;

		size_t inputSizeHint = 0;

		const Scheme *owner = nullptr;
		const Field::Slot *root = nullptr;
	};

	StringView getName() const { return slot->getName(); }
	Type getType() const { return slot->getType(); }
	Flags getFlags() const { return slot->flags; }
	Transform getTransform() const { return slot->getTransform(); }
	Value getDefault(const Value &patch) const { return slot->getDefault(patch); }

	bool hasFlag(Flags f) const { return slot->hasFlag(f); }
	bool hasDefault() const { return slot->hasDefault(); }

	bool isProtected() const { return slot->isProtected(); }
	bool isSimpleLayout() const { return slot->isSimpleLayout(); }
	bool isDataLayout() const { return slot->isDataLayout(); }
	bool isIndexed() const { return slot->isIndexed(); }
	bool isFile() const { return slot->isFile(); }
	bool isReference() const;

	const Scheme *getForeignScheme() const;

	void hash(StringStream &stream, ValidationLevel l) const { slot->hash(stream, l); }

	bool transform(const Scheme &, int64_t, Value &, bool isCreate = false) const;
	bool transform(const Scheme &, const Value &, Value &, bool isCreate = false) const;

	explicit operator bool() const { return slot != nullptr; }

	template <typename SlotType = Slot>
	auto getSlot() const -> const SlotType * {
		return static_cast<const SlotType *>(slot);
	}

	Value getTypeDesc() const;

	Field(const Slot *s) : slot(s) { }

	Field(const Field &s) = default;
	Field &operator=(const Field &s) = default;

	Field(Field &&s) = default;
	Field &operator=(Field &&s) = default;

protected:
	const Slot *slot;
};


struct SP_PUBLIC FieldText : Field::Slot {
	virtual ~FieldText() { }

	template <typename... Args>
	FieldText(String &&n, Type t, Args &&...args) : Field::Slot(sp::move(n), t) {
		init<FieldText, Args...>(*this, std::forward<Args>(args)...);
	}

	virtual bool transformValue(const Scheme &, const Value &, Value &,
			bool isCreate) const override;
	virtual void hash(StringStream &stream, ValidationLevel l) const override;

	size_t minLength = config::FIELD_TEXT_DEFAULT_MIN_SIZE,
		   maxLength = config::FIELD_TEXT_DEFAULT_MAX_SIZE;
};

struct SP_PUBLIC FieldPassword : Field::Slot {
	virtual ~FieldPassword() { }

	template <typename... Args>
	FieldPassword(String &&n, Args &&...args) : Field::Slot(sp::move(n), Type::Bytes) {
		init<FieldPassword, Args...>(*this, std::forward<Args>(args)...);
		transform = Transform::Password;
	}

	virtual bool transformValue(const Scheme &, const Value &, Value &,
			bool isCreate) const override;
	virtual void hash(StringStream &stream, ValidationLevel l) const override;

	size_t minLength = config::FIELD_TEXT_DEFAULT_MIN_SIZE,
		   maxLength = config::FIELD_TEXT_DEFAULT_MAX_SIZE;
	StringView salt = config::FIELD_PASSWORD_DEFAULT_SALT;
};

struct SP_PUBLIC FieldExtra : Field::Slot {
	virtual ~FieldExtra() { }

	template <typename... Args>
	FieldExtra(String &&n, Args &&...args) : Field::Slot(sp::move(n), Type::Extra) {
		init<FieldExtra, Args...>(*this, std::forward<Args>(args)...);
	}

	virtual bool hasDefault() const override;
	virtual Value getDefault(const Value &) const override;

	virtual bool transformValue(const Scheme &, const Value &, Value &,
			bool isCreate) const override;
	virtual void hash(StringStream &stream, ValidationLevel l) const override;

	Map<String, Field> fields;
};

struct SP_PUBLIC FieldFile : Field::Slot {
	virtual ~FieldFile() { }

	template <typename... Args>
	FieldFile(String &&n, Args &&...args) : Field::Slot(sp::move(n), Type::File) {
		init<FieldFile, Args...>(*this, std::forward<Args>(args)...);
	}

	virtual void hash(StringStream &stream, ValidationLevel l) const override;

	size_t maxSize = config::FIELD_FILE_DEFAULT_MAX_SIZE;
	Vector<String> allowedTypes;
};

struct SP_PUBLIC FieldImage : Field::Slot {
	virtual ~FieldImage() { }

	template <typename... Args>
	FieldImage(String &&n, Args &&...args) : Field::Slot(sp::move(n), Type::Image) {
		init<FieldImage, Args...>(*this, std::forward<Args>(args)...);
	}

	virtual void hash(StringStream &stream, ValidationLevel l) const override;

	size_t maxSize = config::FIELD_FILE_DEFAULT_MAX_SIZE;
	Vector<String> allowedTypes;
	MaxImageSize maxImageSize;
	MinImageSize minImageSize;
	Vector<Thumbnail> thumbnails;
	bool primary = true;
};

struct SP_PUBLIC FieldObject : Field::Slot {
	virtual ~FieldObject() { }

	template <typename... Args>
	FieldObject(String &&n, Type t, Args &&...args) : Field::Slot(sp::move(n), t) {
		init<FieldObject, Args...>(*this, std::forward<Args>(args)...);
		if (t == Type::Set && (stappler::toInt(flags) & stappler::toInt(Flags::Reference))) {
			if (onRemove != RemovePolicy::Reference && onRemove != RemovePolicy::StrongReference) {
				onRemove = RemovePolicy::Reference;
			}
		}
		if (t == Type::Set
				&& (onRemove == RemovePolicy::Reference
						|| onRemove == RemovePolicy::StrongReference)) {
			flags |= Flags::Reference;
		}
	}

	virtual bool transformValue(const Scheme &, const Value &, Value &,
			bool isCreate) const override;
	virtual void hash(StringStream &stream, ValidationLevel l) const override;

	const Scheme *scheme = nullptr;
	RemovePolicy onRemove = RemovePolicy::Null;
	Linkage linkage = Linkage::Auto;
	StringView link;
};

struct SP_PUBLIC FieldArray : Field::Slot {
	virtual ~FieldArray() { }

	template <typename... Args>
	FieldArray(String &&n, Args &&...args)
	: Field::Slot(sp::move(n), Type::Array), tfield(new (std::nothrow) FieldText("", Type::Text)) {
		init<FieldArray, Args...>(*this, std::forward<Args>(args)...);
	}

	virtual bool transformValue(const Scheme &, const Value &, Value &,
			bool isCreate) const override;
	virtual void hash(StringStream &stream, ValidationLevel l) const override;

	Field tfield;
};

struct SP_PUBLIC FieldView : Field::Slot {
	enum DeltaOptions {
		Delta
	};

	virtual ~FieldView() { }

	template <typename... Args>
	FieldView(String &&n, Args &&...args) : Field::Slot(sp::move(n), Type::View) {
		init<FieldView, Args...>(*this, std::forward<Args>(args)...);
	}

	virtual bool transformValue(const Scheme &, const Value &, Value &,
			bool isCreate) const override {
		return false;
	}

	const Scheme *scheme = nullptr;
	Vector<String> requireFields;
	ViewLinkageFn linkage;
	ViewFn viewFn;
	bool delta = false;
};

struct SP_PUBLIC FieldFullTextView : Field::Slot {
	virtual ~FieldFullTextView() { }

	template <typename... Args>
	FieldFullTextView(String &&n, Args &&...args) : Field::Slot(sp::move(n), Type::FullTextView) {
		init<FieldFullTextView, Args...>(*this, std::forward<Args>(args)...);
	}

	virtual bool transformValue(const Scheme &, const Value &, Value &,
			bool isCreate) const override {
		return false;
	}

	FullTextQuery parseQuery(const Value &) const;

	Vector<String> requireFields;
	FullTextViewFn viewFn;
	FullTextQueryFn queryFn;

	search::Normalization normalization = search::Normalization::Default;
	const search::Configuration *searchConfiguration = nullptr;
};

struct SP_PUBLIC FieldCustom : Field::Slot {
	virtual ~FieldCustom() { }

	template <typename... Args>
	FieldCustom(String &&n, Args &&...args) : Field::Slot(sp::move(n), Type::Custom) {
		init<FieldCustom, Args...>(*this, std::forward<Args>(args)...);
	}

	virtual StringView getDriverTypeName() const = 0;
};

struct SP_PUBLIC FieldVirtual : Field::Slot {
	virtual ~FieldVirtual() { }

	template <typename... Args>
	FieldVirtual(String &&n, Args &&...args) : Field::Slot(sp::move(n), Type::Virtual) {
		init<FieldVirtual, Args...>(*this, std::forward<Args>(args)...);
	}

	virtual void hash(StringStream &stream, ValidationLevel l) const override { }
	virtual bool transformValue(const Scheme &, const Value &, Value &,
			bool isCreate) const override;

	Vector<String> requireFields;
	VirtualReadFn readFn;
	VirtualWriteFn writeFn;
};

template <typename... Args>
Field Field::Data(String &&name, Args &&...args) {
	auto newSlot = new (std::nothrow) Field::Slot(sp::move(name), Type::Data);
	Slot::init<Field::Slot>(*newSlot, std::forward<Args>(args)...);
	newSlot->inputSizeHint = config::FIELD_EXTRA_DEFAULT_HINT_SIZE;
	return Field(newSlot);
}

template <typename... Args>
Field Field::Integer(String &&name, Args &&...args) {
	auto newSlot = new (std::nothrow) Field::Slot(sp::move(name), Type::Integer);
	Slot::init<Field::Slot>(*newSlot, std::forward<Args>(args)...);
	return Field(newSlot);
}

template <typename... Args>
Field Field::Float(String &&name, Args &&...args) {
	auto newSlot = new (std::nothrow) Field::Slot(sp::move(name), Type::Float);
	Slot::init<Field::Slot>(*newSlot, std::forward<Args>(args)...);
	return Field(newSlot);
}

template <typename... Args>
Field Field::Boolean(String &&name, Args &&...args) {
	auto newSlot = new (std::nothrow) Field::Slot(sp::move(name), Type::Boolean);
	Slot::init<Field::Slot>(*newSlot, std::forward<Args>(args)...);
	return Field(newSlot);
}

template <typename... Args>
Field Field::Text(String &&name, Args &&...args) {
	return Field(
			new (std::nothrow) FieldText(sp::move(name), Type::Text, std::forward<Args>(args)...));
}

template <typename... Args>
Field Field::Bytes(String &&name, Args &&...args) {
	return Field(
			new (std::nothrow) FieldText(sp::move(name), Type::Bytes, std::forward<Args>(args)...));
}

template <typename... Args>
Field Field::Password(String &&name, Args &&...args) {
	return Field(new (std::nothrow) FieldPassword(sp::move(name), std::forward<Args>(args)...));
}

template <typename... Args>
Field Field::Extra(String &&name, Args &&...args) {
	auto newSlot = new (std::nothrow) FieldExtra(sp::move(name), std::forward<Args>(args)...);
	newSlot->inputSizeHint = config::FIELD_EXTRA_DEFAULT_HINT_SIZE;
	return Field(newSlot);
}

template <typename... Args>
Field Field::Extra(String &&name, stappler::InitializerList<Field> &&f, Args &&...args) {
	auto newSlot =
			new (std::nothrow) FieldExtra(sp::move(name), sp::move(f), std::forward<Args>(args)...);
	newSlot->inputSizeHint = config::FIELD_EXTRA_DEFAULT_HINT_SIZE;
	return Field(newSlot);
}

template <typename... Args>
Field Field::File(String &&name, Args &&...args) {
	return Field(new (std::nothrow) FieldFile(sp::move(name), std::forward<Args>(args)...));
}

template <typename... Args>
Field Field::Image(String &&name, Args &&...args) {
	return Field(new (std::nothrow) FieldImage(sp::move(name), std::forward<Args>(args)...));
}

template <typename... Args>
Field Field::Object(String &&name, Args &&...args) {
	return Field(new (std::nothrow)
					FieldObject(sp::move(name), Type::Object, std::forward<Args>(args)...));
}

template <typename... Args>
Field Field::Set(String &&name, Args &&...args) {
	return Field(
			new (std::nothrow) FieldObject(sp::move(name), Type::Set, std::forward<Args>(args)...));
}

template <typename... Args>
Field Field::Array(String &&name, Args &&...args) {
	return Field(new (std::nothrow) FieldArray(sp::move(name), std::forward<Args>(args)...));
}

template <typename... Args>
Field Field::View(String &&name, Args &&...args) {
	return Field(new (std::nothrow) FieldView(sp::move(name), std::forward<Args>(args)...));
}

template <typename... Args>
Field Field::FullTextView(String &&name, Args &&...args) {
	return Field(new (std::nothrow) FieldFullTextView(sp::move(name), std::forward<Args>(args)...));
}

template <typename... Args>
Field Field::Virtual(String &&name, Args &&...args) {
	return Field(new (std::nothrow) FieldVirtual(sp::move(name), std::forward<Args>(args)...));
}

template <typename... Args>
Field Field::Custom(FieldCustom *custom) {
	return Field(custom);
}

template <typename F>
struct FieldOption<F, Flags> {
	static inline void assign(F &f, Flags flags) { f.flags |= flags; }
};

template <typename F>
struct FieldOption<F, FilterFn> {
	static inline void assign(F &f, const FilterFn &fn) {
		f.writeFilterFn =
				WriteFilterFn([fn](const Scheme &scheme, const Value &patch, Value &value,
									  bool isCreate) -> bool { return fn(scheme, value); });
	}
};

template <typename F>
struct FieldOption<F, WriteFilterFn> {
	static inline void assign(F &f, const WriteFilterFn &fn) { f.writeFilterFn = fn; }
};

template <typename F>
struct FieldOption<F, ReadFilterFn> {
	static inline void assign(F &f, const ReadFilterFn &fn) { f.readFilterFn = fn; }
};

template <typename F>
struct FieldOption<F, ReplaceFilterFn> {
	static inline void assign(F &f, const ReplaceFilterFn &fn) { f.replaceFilterFn = fn; }
};

template <typename F>
struct FieldOption<F, DefaultFn> {
	static inline void assign(F &f, const DefaultFn &fn) { f.defaultFn = fn; }
};

template <typename F>
struct FieldOption<F, Function<Value()>> {
	static inline void assign(F &f, const Function<Value()> &fn) {
		f.defaultFn = DefaultFn([fn](const Value &) -> Value { return fn(); });
	}
};

template <typename F>
struct FieldOption<F, Transform> {
	static inline void assign(F &f, Transform t) { f.transform = t; }
};

template <typename F>
struct FieldOption<F, Documentation> {
	static inline void assign(F &f, Documentation &&doc) { f.documentation = doc.get(); }
};

template <typename F>
struct FieldOption<F, MinLength> {
	static inline void assign(F &f, MinLength l) { f.minLength = l.get(); }
};

template <typename F>
struct FieldOption<F, MaxLength> {
	static inline void assign(F &f, MaxLength l) { f.maxLength = l.get(); }
};

template <typename F>
struct FieldOption<F, Value> {
	static inline void assign(F &f, Value &&v) { f.def = sp::move(v); }
};

template <typename F>
struct FieldOption<F, PasswordSalt> {
	static inline void assign(F &f, PasswordSalt &&s) {
		f.salt = s.get().pdup(f.name.get_allocator());
	}
};

template <typename F>
struct FieldOption<F, ForeignLink> {
	static inline void assign(F &f, ForeignLink &&s) {
		f.link = s.get().pdup(f.name.get_allocator());
		f.linkage = Linkage::Manual;
	}
};

template <typename F>
struct FieldOption<F, Vector<Field>> {
	static inline void assign(F &f, Vector<Field> &&s) {
		for (auto &it : s) {
			const_cast<Field::Slot *>(it.getSlot())->root = &f;
			f.fields.emplace(it.getName().str<Interface>(), it);
		}
	}
};

template <typename F>
struct FieldOption<F, AutoFieldDef> {
	static inline void assign(F &f, AutoFieldDef &&def) { f.autoField = sp::move(def); }
};

template <typename F>
struct FieldOption<F, std::initializer_list<Field>> {
	static inline void assign(F &f, std::initializer_list<Field> &&s) {
		for (auto &it : s) {
			const_cast<Field::Slot *>(it.getSlot())->root = &f;
			f.fields.emplace(it.getName().str<Interface>(), it);
		}
	}
};

template <typename F>
struct FieldOption<F, MaxFileSize> {
	static inline void assign(F &f, MaxFileSize l) { f.maxSize = l.get(); }
};

template <typename F>
struct FieldOption<F, Vector<String>> {
	static inline void assign(F &f, Vector<String> &&l) { f.allowedTypes = sp::move(l); }
};

template <typename F>
struct FieldOption<F, MaxImageSize> {
	static inline void assign(F &f, MaxImageSize &&s) { f.maxImageSize = sp::move(s); }
};

template <typename F>
struct FieldOption<F, MinImageSize> {
	static inline void assign(F &f, MinImageSize &&s) { f.minImageSize = sp::move(s); }
};

template <typename F>
struct FieldOption<F, Vector<Thumbnail>> {
	static inline void assign(F &f, Vector<Thumbnail> &&s) { f.thumbnails = sp::move(s); }
};

template <typename F>
struct FieldOption<F, RemovePolicy> {
	static inline void assign(F &f, RemovePolicy p) {
		f.onRemove = p;
		if (p == RemovePolicy::Reference || p == RemovePolicy::StrongReference) {
			f.flags |= Flags::Reference;
		}
	}
};

template <typename F>
struct FieldOption<F, Linkage> {
	static inline void assign(F &f, Linkage p) { f.linkage = p; }
};

template <typename F>
struct FieldOption<F, const Scheme *> {
	static inline void assign(F &f, const Scheme *s) { f.scheme = s; }
};
template <typename F>
struct FieldOption<F, Scheme> {
	static inline void assign(F &f, const Scheme &s) { f.scheme = &s; }
};
template <typename F>
struct FieldOption<F, const Scheme> {
	static inline void assign(F &f, const Scheme &s) { f.scheme = &s; }
};
template <typename F>
struct FieldOption<F, Field> {
	static inline void assign(F &f, Field &&s) { f.tfield = s; }
};

template <>
struct FieldOption<FieldArray, Type> {
	static inline void assign(FieldArray &f, Type type) {
		switch (type) {
		case Type::Integer: f.tfield = Field::Integer("value"); break;
		case Type::Float: f.tfield = Field::Float("value"); break;
		case Type::Boolean: f.tfield = Field::Boolean("value"); break;
		case Type::Text: f.tfield = Field::Text("value"); break;
		case Type::Bytes: f.tfield = Field::Bytes("value"); break;
		case Type::Data: f.tfield = Field::Data("value"); break;
		case Type::Extra: f.tfield = Field::Extra("value"); break;
		default: break;
		}
	}
};

// view options

template <>
struct FieldOption<FieldView, Vector<String>> {
	static inline void assign(FieldView &f, Vector<String> &&s) { f.requireFields = sp::move(s); }
};

template <>
struct FieldOption<FieldFullTextView, Vector<String>> {
	static inline void assign(FieldFullTextView &f, Vector<String> &&s) {
		f.requireFields = sp::move(s);
	}
};

template <>
struct FieldOption<FieldFullTextView, search::Configuration *> {
	static inline void assign(FieldFullTextView &f, const search::Configuration *s) {
		f.searchConfiguration = s;
	}
};

template <>
struct FieldOption<FieldFullTextView, search::Configuration> {
	static inline void assign(FieldFullTextView &f, const search::Configuration &s) {
		f.searchConfiguration = &s;
	}
};

template <typename F>
struct FieldOption<F, ViewLinkageFn> {
	static inline void assign(F &f, ViewLinkageFn &&s) { f.linkage = sp::move(s); }
};

template <typename F>
struct FieldOption<F, ViewFn> {
	static inline void assign(F &f, ViewFn &&s) { f.viewFn = sp::move(s); }
};

template <typename F>
struct FieldOption<F, FullTextViewFn> {
	static inline void assign(F &f, FullTextViewFn &&s) { f.viewFn = sp::move(s); }
};

template <typename F>
struct FieldOption<F, FullTextQueryFn> {
	static inline void assign(F &f, FullTextQueryFn &&s) { f.queryFn = sp::move(s); }
};

template <typename F>
struct FieldOption<F, FieldView::DeltaOptions> {
	static inline void assign(F &f, FieldView::DeltaOptions d) {
		if (d == FieldView::Delta) {
			f.delta = true;
		} else {
			f.delta = false;
		}
	}
};

// virtual options

template <>
struct FieldOption<FieldVirtual, Vector<String>> {
	static inline void assign(FieldVirtual &f, Vector<String> &&s) {
		f.requireFields = sp::move(s);
	}
};

template <>
struct FieldOption<FieldVirtual, VirtualReadFn> {
	static inline void assign(FieldVirtual &f, VirtualReadFn &&r) { f.readFn = sp::move(r); }
};

template <>
struct FieldOption<FieldVirtual, VirtualWriteFn> {
	static inline void assign(FieldVirtual &f, VirtualWriteFn &&r) { f.writeFn = sp::move(r); }
};

} // namespace stappler::db

#endif /* STAPPLER_DB_SPDBFIELD_H_ */
