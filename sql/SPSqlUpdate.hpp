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

#ifndef STAPPLER_SQL_SPSQLUPDATE_HPP_
#define STAPPLER_SQL_SPSQLUPDATE_HPP_

#include "SPSql.h"

namespace STAPPLER_VERSIONIZED stappler::sql {

template <typename Binder, typename Interface>
template <typename ... Args>
auto Query<Binder, Interface>::Update::where(Args && ... args) -> UpdateWhere {
	this->query->stream << " WHERE";
	UpdateWhere q(this->query);
	q.where(sql::Operator::And, forward<Args>(args)...);
	return q;
};

template <typename Binder, typename Interface>
auto Query<Binder, Interface>::Update::where() -> UpdateWhere {
	this->query->stream << " WHERE";
	return UpdateWhere(this->query);
}

template <typename Binder, typename Interface>
auto Query<Binder, Interface>::Update::returning() -> Returning {
	this->query->stream << " RETURNING";
	return Returning(this->query);
}

template <typename Binder, typename Interface>
auto Query<Binder, Interface>::UpdateWhere::returning() -> Returning {
	this->query->stream << " RETURNING";
	return Returning(this->query);
}

template <typename Binder, typename Interface>
auto Query<Binder, Interface>::update(const StringView & field) -> Update {
	stream << "UPDATE " << field << " SET";
	target = field;
	return Update(this);
}

template <typename Binder, typename Interface>
auto Query<Binder, Interface>::update(const StringView &field, const StringView &alias) -> Update {
	stream << "UPDATE " << field << " AS " << alias << " SET";
	target = field;
	return Update(this);
}


template <typename Binder, typename Interface>
template <typename ... Args>
auto Query<Binder, Interface>::Delete::where(Args && ... args) -> DeleteWhere {
	this->query->stream << " WHERE";
	DeleteWhere q(this->query);
	q.where(sql::Operator::And, forward<Args>(args)...);
	return q;
};

template <typename Binder, typename Interface>
auto Query<Binder, Interface>::Delete::where() -> DeleteWhere {
	this->query->stream << " WHERE";
	return DeleteWhere(this->query);
}

template <typename Binder, typename Interface>
auto Query<Binder, Interface>::Delete::returning() -> Returning {
	this->query->stream << " RETURNING";
	return Returning(this->query);
}

template <typename Binder, typename Interface>
auto Query<Binder, Interface>::DeleteWhere::returning() -> Returning {
	this->query->stream << " RETURNING";
	return Returning(this->query);
}

template <typename Binder, typename Interface>
auto Query<Binder, Interface>::remove(const StringView & field) -> Delete {
	stream << "DELETE FROM " << field;
	target = field;
	return Delete(this);
}

template <typename Binder, typename Interface>
auto Query<Binder, Interface>::remove(const StringView &field, const StringView &alias) -> Delete {
	stream << "DELETE FROM " << field << " AS " << alias;
	target = field;
	return Delete(this);
}

}

#endif /* STAPPLER_SQL_SPSQLUPDATE_HPP_ */
