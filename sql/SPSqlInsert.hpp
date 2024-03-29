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

// Excluded from documentation/codegen tool
///@ SP_EXCLUDE

#ifndef STAPPLER_SQL_SPSQLINSERT_HPP_
#define STAPPLER_SQL_SPSQLINSERT_HPP_

#include "SPSql.h"

namespace STAPPLER_VERSIONIZED stappler::sql {

template <typename Binder, typename Interface>
template <typename ...Args>
auto Query<Binder, Interface>::Insert::values(Args && ... args) -> InsertValues {
	switch (this->state) {
	case State::None: this->query->stream << " VALUES"; break;
	case State::Some: this->query->stream << ")VALUES"; break;
	case State::Init: break;
	}

	InsertValues v(this->query, State::Init);
	Expand<InsertValues>::values(v, forward<Args>(args)...);
	return v;
}

template <typename Binder, typename Interface>
template <typename ...Args>
auto Query<Binder, Interface>::InsertValues::values(Args && ... args) -> InsertValues & {
	if (this->state == State::Some) {
		this->query->stream << ")";
		this->state = State::None;
		this->query->finalization = FinalizationState::None;
	}

	Expand<InsertValues>::values(*this, forward<Args>(args)...);

	return *this;
}

template <typename Binder, typename Interface>
template <typename Value>
auto Query<Binder, Interface>::InsertValues::value(Value &&val) -> InsertValues & {
	switch (this->state) {
	case State::None:
		this->query->stream << ",(";
		this->state = State::Some;
		this->query->finalization = FinalizationState::Parentesis;
		break;
	case State::Init:
		this->query->stream << "(";
		this->state = State::Some;
		this->query->finalization = FinalizationState::Parentesis;
		break;
	case State::Some:
		this->query->stream << ",";
		break;
	}

	this->query->writeBind(forward<Value>(val));
	return *this;
}

template <typename Binder, typename Interface>
auto Query<Binder, Interface>::InsertValues::def() -> InsertValues & {
	switch (this->state) {
	case State::None:
		this->query->stream << ",(";
		this->state = State::Some;
		this->query->finalization = FinalizationState::Parentesis;
		break;
	case State::Init:
		this->query->stream << "(";
		this->state = State::Some;
		this->query->finalization = FinalizationState::Parentesis;
		break;
	case State::Some:
		this->query->stream << ",";
		break;
	}

	switch (this->query->profile) {
	case Profile::Postgres:
		this->query->stream << "DEFAULT";
		break;
	case Profile::Sqlite:
		this->query->stream << "NULL";
		break;
	}
	return *this;
}

template <typename Binder, typename Interface>
auto Query<Binder, Interface>::InsertValues::onConflict(const StringView &field) -> InsertConflict {
	switch (this->state) {
	case State::None: break;
	case State::Init: break;
	case State::Some:
		this->query->stream << ")";
		this->query->finalization = FinalizationState::None;
		break;
	}

	this->state = State::None;
	this->query->stream << "ON CONFLICT(\"" << field << "\")";
	return InsertConflict(this->query);
}

template <typename Binder, typename Interface>
auto Query<Binder, Interface>::InsertValues::onConflictDoNothing() -> InsertPostConflict {
	switch (this->state) {
	case State::None: break;
	case State::Init: break;
	case State::Some:
		this->query->stream << ")";
		this->query->finalization = FinalizationState::None;
		break;
	}
	this->query->stream << "ON CONFLICT DO NOTHING";
	return InsertPostConflict(this->query);
}

template <typename Binder, typename Interface>
auto Query<Binder, Interface>::InsertValues::returning() -> Returning {
	switch (this->state) {
	case State::None: break;
	case State::Init: break;
	case State::Some:
		this->query->stream << ")";
		this->query->finalization = FinalizationState::None;
		break;
	}
	this->query->stream << " RETURNING";
	return Returning(this->query);
}

template <typename Binder, typename Interface>
auto Query<Binder, Interface>::InsertValues::next() -> InsertValues {
	switch (this->state) {
	case State::None:
	case State::Init:
		this->query->stream << "\n";
		break;
	case State::Some:
		this->query->stream << ")\n";
		break;
	}

	this->query->finalization = FinalizationState::None;
	InsertValues v(this->query, State::None);
	return v;
}

template <typename Binder, typename Interface>
auto Query<Binder, Interface>::InsertConflict::doNothing() -> InsertPostConflict {
	this->query->stream << " DO NOTHING ";
	return InsertPostConflict(this->query);
}

template <typename Binder, typename Interface>
auto Query<Binder, Interface>::InsertConflict::doUpdate() -> InsertUpdateValues {
	this->query->stream << " DO UPDATE SET";
	return InsertUpdateValues(this->query);
}

template <typename Binder, typename Interface>
auto Query<Binder, Interface>::InsertUpdateValues::excluded(StringView f) -> InsertUpdateValues & {
	if (this->state == State::None) { this->state = State::Some; } else { this->query->stream << ","; }
	this->query->stream << " \"" << f << "\"=EXCLUDED.\"" << f << "\"";
	return *this;
}

template <typename Binder, typename Interface>
auto Query<Binder, Interface>::InsertUpdateValues::excluded(StringView f, StringView v) -> InsertUpdateValues & {
	if (this->state == State::None) { this->state = State::Some; } else { this->query->stream << ","; }
	this->query->stream << " \"" << f << "\"=EXCLUDED.\"" << v << "\"";
	return *this;
}

template <typename Binder, typename Interface>
template <typename ... Args>
auto Query<Binder, Interface>::InsertUpdateValues::where(Args && ... args) -> InsertWhereValues {
	this->query->stream << " WHERE";
	InsertWhereValues q(this->query);
	q.where(sql::Operator::And, forward<Args>(args)...);
	return q;
};

template <typename Binder, typename Interface>
auto Query<Binder, Interface>::InsertUpdateValues::where() -> InsertWhereValues {
	this->query->stream << " WHERE";
	return InsertWhereValues(this->query);
};

template <typename Binder, typename Interface>
auto Query<Binder, Interface>::InsertUpdateValues::returning() -> Returning {
	this->query->stream << " RETURNING";
	return Returning(this->query);
}

template <typename Binder, typename Interface>
auto Query<Binder, Interface>::InsertWhereValues::returning() -> Returning {
	this->query->stream << " RETURNING";
	return Returning(this->query);
}

template <typename Binder, typename Interface>
auto Query<Binder, Interface>::InsertPostConflict::returning() -> Returning {
	this->query->stream << " RETURNING";
	return Returning(this->query);
}

template <typename Binder, typename Interface>
auto Query<Binder, Interface>::Returning::all() -> Returning & {
	if (this->state == State::None) { this->state = State::Some; } else { this->query->stream << ","; }
	this->query->stream << " *";
	return *this;
}

template <typename Binder, typename Interface>
auto Query<Binder, Interface>::Returning::count() -> Returning & {
	if (this->state == State::None) { this->state = State::Some; } else { this->query->stream << ","; }
	this->query->stream << " COUNT(*)";
	return *this;
}

template <typename Binder, typename Interface>
auto Query<Binder, Interface>::Returning::count(const StringView &alias) -> Returning & {
	if (this->state == State::None) { this->state = State::Some; } else { this->query->stream << ","; }
	this->query->stream << " COUNT(*) AS \"" << alias << "\"";
	return *this;
}

template <typename Binder, typename Interface>
auto Query<Binder, Interface>::insert(const StringView & field) -> Insert {
	stream << "INSERT INTO " << field;
	target = field;
	return Insert(this, State::Init);
}

template <typename Binder, typename Interface>
auto Query<Binder, Interface>::insert(const StringView &field, const StringView &alias) -> Insert {
	stream << "INSERT INTO " << field << " AS " << alias;
	target = field;
	return Insert(this, State::Init);
}

}

#endif /* STAPPLER_SQL_SPSQLINSERT_HPP_ */
