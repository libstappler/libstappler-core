/**
Copyright (c) 2016-2022 Roman Katuntsev <sbkarr@stappler.org>
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

#include "SPTime.h"
#include "SPPlatform.h"
#include "SPPlatformUnistd.h"

inline time_t _time() {
	return time(NULL);
}

namespace STAPPLER_VERSIONIZED stappler {

static constexpr uint64_t SP_USEC_PER_SEC(1000000);

using sp_time_t = uint32_t;

TimeInterval TimeInterval::Infinite(maxOf<uint64_t>());

TimeInterval TimeInterval::between(const Time &v1, const Time &v2) {
	if (v1 > v2) {
		return TimeInterval(v1._value - v2._value);
	} else {
		return TimeInterval(v2._value - v1._value);
	}
}

uint64_t TimeStorage::toMicroseconds() const {
	return _value;
}
uint64_t TimeStorage::toMilliseconds() const {
	return _value / 1000ULL;
}
uint64_t TimeStorage::toSeconds() const {
	return _value / 1000000ULL;
}
float TimeStorage::toFloatSeconds() const {
	return _value / 1000000.0f;
}
double TimeStorage::toDoubleSeconds() const {
	return _value / 1000000.0;
}

struct tm TimeStorage::asLocal() const {
	auto sec = time_t(toSeconds());
	struct tm tm;
#if WIN32
	localtime_s(&tm, &sec);
#else
	localtime_r(&sec, &tm);
#endif
	return tm;
}

struct tm TimeStorage::asGmt() const {
	auto sec = time_t(toSeconds());
	struct tm tm;
#if WIN32
	gmtime_s(&tm, &sec);
#else
	gmtime_r(&sec, &tm);
#endif
	return tm;
}

void TimeStorage::setMicroseconds(uint64_t v) {
	_value = v;
}
void TimeStorage::setMilliseconds(uint64_t v) {
	_value = v * 1000ULL;
}
void TimeStorage::setSeconds(time_t v) {
	_value = v * 1000000ULL;
}

void TimeStorage::clear() {
	_value = 0;
}

TimeInterval::TimeInterval(nullptr_t) {
	_value = 0;
}
TimeInterval & TimeInterval::operator= (nullptr_t) {
	_value = 0;
	return *this;
}


Time Time::now() {
	return Time(platform::clock(ClockType::Monotonic));
}

Time Time::microseconds(uint64_t mksec) {
	return Time(mksec);
}
Time Time::milliseconds(uint64_t msec) {
	return Time(msec * 1000ULL);
}
Time Time::seconds(time_t sec) {
	return Time(sec * 1000000ULL);
}
Time Time::floatSeconds(float sec) {
	return Time(uint64_t(sec * 1000000.0f));
}

Time::Time(nullptr_t) : TimeStorage(0) { }
Time & Time::operator= (nullptr_t) {
	_value = 0;
	return *this;
}

sp_time_exp_t::sp_time_exp_t() {
	tm_usec = 0;
	tm_sec = 0;
	tm_min = 0;
	tm_hour = 0;
	tm_mday = 0;
	tm_mon = 0;
	tm_year = 0;
	tm_wday = 0;
	tm_yday = 0;
	tm_isdst = 0;
	tm_gmtoff = 0;
}

sp_time_exp_t::sp_time_exp_t(int64_t t, int32_t offset, bool use_localtime) {
	struct tm tm;
	time_t tt = time_t(t / int64_t(SP_USEC_PER_SEC));
	tm_usec = t % int64_t(SP_USEC_PER_SEC);

	if (use_localtime) {
#if WIN32
		localtime_s(&tm, &tt);
#else
		localtime_r(&tt, &tm);
#endif
		tm_gmt_type = gmt_local;
	} else {
#if WIN32
		gmtime_s(&tm, &tt);
#else
		gmtime_r(&tt, &tm);
#endif
		tm_gmt_type = gmt_set;
	}

	tm_sec = tm.tm_sec;
	tm_min = tm.tm_min;
	tm_hour = tm.tm_hour;
	tm_mday = tm.tm_mday;
	tm_mon = tm.tm_mon;
	tm_year = tm.tm_year;
	tm_wday = tm.tm_wday;
	tm_yday = tm.tm_yday;
	tm_isdst = tm.tm_isdst;
#ifndef WIN32
	tm_gmtoff = int32_t(tm.tm_gmtoff);
#else
	tm_gmtoff = offset;
#endif
}

sp_time_exp_t::sp_time_exp_t(int64_t t, int32_t offs) : sp_time_exp_t(t, offs, false) {
	tm_gmtoff = offs;
}

sp_time_exp_t::sp_time_exp_t(int64_t t) : sp_time_exp_t(t, 0, false) {
	tm_gmtoff = 0;
}

sp_time_exp_t::sp_time_exp_t(int64_t t, bool use_localtime) : sp_time_exp_t(t, 0, use_localtime) { }

sp_time_exp_t::sp_time_exp_t(Time t, int32_t offset, bool use_localtime)
: sp_time_exp_t(int64_t(t.toMicroseconds()), offset, use_localtime) { }

// apr_time_exp_tz
sp_time_exp_t::sp_time_exp_t(Time t, int32_t offs)
: sp_time_exp_t(int64_t(t.toMicros()), offs) { }

// apr_time_exp_gmt
sp_time_exp_t::sp_time_exp_t(Time t)
: sp_time_exp_t(int64_t(t.toMicros())) { }

// apr_time_exp_lt
sp_time_exp_t::sp_time_exp_t(Time t, bool use_localtime)
: sp_time_exp_t(int64_t(t.toMicros()), 0, use_localtime) { }

Time sp_time_exp_t::get() const {
	return Time::microseconds(geti());
}

Time sp_time_exp_t::gmt_get() const {
	return Time::microseconds(gmt_geti());
}

Time sp_time_exp_t::ltz_get() const {
	return Time::microseconds(ltz_geti());
}

int64_t sp_time_exp_t::geti() const {
	sp_time_t year = tm_year;
	int64_t days = 0;
	static const int dayoffset[12] = { 306, 337, 0, 31, 61, 92, 122, 153, 184, 214, 245, 275 };

	/* shift new year to 1st March in order to make leap year calc easy */

	if (tm_mon < 2)
		year--;

	/* Find number of days since 1st March 1900 (in the Gregorian calendar). */

	days = year * 365 + year / 4 - year / 100 + (year / 100 + 3) / 4;
	days += dayoffset[tm_mon] + tm_mday - 1;
	days -= 25508; /* 1 jan 1970 is 25508 days since 1 mar 1900 */

	return int64_t( ( ((days * 24 + tm_hour) * 60 + tm_min) * 60 + tm_sec ) * SP_USEC_PER_SEC + tm_usec );
}

int64_t sp_time_exp_t::gmt_geti() const {
	return int64_t( geti() - tm_gmtoff * SP_USEC_PER_SEC );
}

int64_t sp_time_exp_t::ltz_geti() const {
	time_t t = time(NULL);
	struct tm lt = {0};
#if WIN32
	TIME_ZONE_INFORMATION tzi;
	GetTimeZoneInformation(&tzi);
	localtime_s(&lt, &t);

	long bias = tzi.Bias;
	if (lt.tm_isdst) {
		if (tzi.DaylightDate.wMonth) {
			bias += tzi.DaylightBias;
		} else if (tzi.StandardDate.wMonth) {
			bias += tzi.StandardBias;
		}
	} else {
		if (tzi.StandardDate.wMonth) {
			bias += tzi.StandardBias;
		}
	}

	return int64_t( geti() + bias * 60 * SP_USEC_PER_SEC );
#else
	localtime_r(&t, &lt);
	return int64_t( geti() - lt.tm_gmtoff * SP_USEC_PER_SEC );
#endif
}

/*
 * Compare a string to a mask
 * Mask characters (arbitrary maximum is 256 characters, just in case):
 *   @ - uppercase letter
 *   $ - lowercase letter
 *   & - hex digit
 *   # - digit
 *   ~ - digit or space
 *   * - swallow remaining characters
 *  <x> - exact match for any other character
 */
static bool sp_date_checkmask(StringView data, StringView mask) {
	while (!mask.empty() && !data.empty()) {
		auto d = data.front();
		switch (mask.front()) {
		case '\0':
			return (d == '\0');

		case '*':
			return true;

		case '@':
			if (!chars::isupper(d))
				return false;
			break;
		case '$':
			if (!chars::islower(d))
				return false;
			break;
		case '#':
			if (!chars::isdigit(d))
				return false;
			break;
		case '&':
			if (!chars::isxdigit(d))
				return false;
			break;
		case '~':
			if ((d != ' ') && !chars::isdigit(d))
				return false;
			break;
		default:
			if (*mask != d)
				return false;
			break;
		}
		++ mask;
		++ data;
	}

	while (data.empty() && mask.is('*')) {
		++ mask;
	}

	return mask.empty() && data.empty();
}

/*
 * Parses an HTTP date in one of three standard forms:
 *
 *     Sun, 06 Nov 1994 08:49:37 GMT  ; RFC 822, updated by RFC 1123
 *     Sunday, 06-Nov-94 08:49:37 GMT ; RFC 850, obsoleted by RFC 1036
 *     Sun Nov  6 08:49:37 1994       ; ANSI C's asctime() format
 *     2011-04-28T06:34:00+09:00      ; Atom time format
 *
 * and returns the apr_time_t number of microseconds since 1 Jan 1970 GMT,
 * or APR_DATE_BAD if this would be out of range or if the date is invalid.
 *
 * The restricted HTTP syntax is
 *
 *     HTTP-date    = rfc1123-date | rfc850-date | asctime-date
 *
 *     rfc1123-date = wkday "," SP date1 SP time SP "GMT"
 *     rfc850-date  = weekday "," SP date2 SP time SP "GMT"
 *     asctime-date = wkday SP date3 SP time SP 4DIGIT
 *
 *     date1        = 2DIGIT SP month SP 4DIGIT
 *                    ; day month year (e.g., 02 Jun 1982)
 *     date2        = 2DIGIT "-" month "-" 2DIGIT
 *                    ; day-month-year (e.g., 02-Jun-82)
 *     date3        = month SP ( 2DIGIT | ( SP 1DIGIT ))
 *                    ; month day (e.g., Jun  2)
 *
 *     time         = 2DIGIT ":" 2DIGIT ":" 2DIGIT
 *                    ; 00:00:00 - 23:59:59
 *
 *     wkday        = "Mon" | "Tue" | "Wed"
 *                  | "Thu" | "Fri" | "Sat" | "Sun"
 *
 *     weekday      = "Monday" | "Tuesday" | "Wednesday"
 *                  | "Thursday" | "Friday" | "Saturday" | "Sunday"
 *
 *     month        = "Jan" | "Feb" | "Mar" | "Apr"
 *                  | "May" | "Jun" | "Jul" | "Aug"
 *                  | "Sep" | "Oct" | "Nov" | "Dec"
 *
 * However, for the sake of robustness (and Netscapeness), we ignore the
 * weekday and anything after the time field (including the timezone).
 *
 * This routine is intended to be very fast; 10x faster than using sscanf.
 *
 * Originally from Andrew Daviel <andrew@vancouver-webpages.com>, 29 Jul 96
 * but many changes since then.
 *
 */

static const int s_months[12] = {
		('J' << 16) | ('a' << 8) | 'n', ('F' << 16) | ('e' << 8) | 'b',
		('M' << 16) | ('a' << 8) | 'r', ('A' << 16) | ('p' << 8) | 'r',
		('M' << 16) | ('a' << 8) | 'y', ('J' << 16) | ('u' << 8) | 'n',
		('J' << 16) | ('u' << 8) | 'l', ('A' << 16) | ('u' << 8) | 'g',
		('S' << 16) | ('e' << 8) | 'p', ('O' << 16) | ('c' << 8) | 't',
		('N' << 16) | ('o' << 8) | 'v', ('D' << 16) | ('e' << 8) | 'c'
};

static inline bool sp_time_exp_read_time(sp_time_exp_t &ds, StringView timstr) {
	ds.tm_hour = ((timstr[0] - '0') * 10) + (timstr[1] - '0');
	ds.tm_min = ((timstr[3] - '0') * 10) + (timstr[4] - '0');
	ds.tm_sec = ((timstr[6] - '0') * 10) + (timstr[7] - '0');

	if ((ds.tm_hour > 23) || (ds.tm_min > 59) || (ds.tm_sec > 61)) { return false; }

	return true;
}

static inline bool sp_time_exp_check_mon(sp_time_exp_t &ds) {
	if (ds.tm_mday <= 0 || ds.tm_mday > 31) { return false; }
	if (ds.tm_mon >= 12) { return false; }
	if ((ds.tm_mday == 31) && (ds.tm_mon == 3 || ds.tm_mon == 5 || ds.tm_mon == 8 || ds.tm_mon == 10)) { return false; }

	if ((ds.tm_mon == 1)
			&& ((ds.tm_mday > 29)
					|| ((ds.tm_mday == 29)
							&& ((ds.tm_year & 3) || (((ds.tm_year % 100) == 0) && (((ds.tm_year % 400) != 100)))))
			))
		return false;
	return true;
}

static inline bool sp_time_exp_read_mon(sp_time_exp_t &ds, StringView monstr) {
	int mon = 0;
	if (ds.tm_mday <= 0 || ds.tm_mday > 31) { return false; }

	if (monstr.size() >= 3) {
		auto mint = (monstr[0] << 16) | (monstr[1] << 8) | monstr[2];
		for (mon = 0; mon < 12; mon++)
			if (mint == s_months[mon])
				break;
	} else {
		mon = ds.tm_mon - 1;
	}

	if (mon >= 12) { return false; }
	if ((ds.tm_mday == 31) && (mon == 3 || mon == 5 || mon == 8 || mon == 10)) { return false; }

	if ((mon == 1)
			&& ((ds.tm_mday > 29)
					|| ((ds.tm_mday == 29)
							&& ((ds.tm_year & 3) || (((ds.tm_year % 100) == 0) && (((ds.tm_year % 400) != 100)))))
			))
		return false;

	ds.tm_mon = mon;
	return true;
}

static inline bool sp_time_exp_read_gmt(sp_time_exp_t &ds, StringView gmtstr) {
	ds.tm_gmtoff = 0;
	/* Do we have a timezone ? */
	if (!gmtstr.empty()) {
		if (gmtstr == "GMT") {
			ds.tm_gmt_type = sp_time_exp_t::gmt_set;
			return true;
		}
		int sign = 0;
		switch (*gmtstr) {
		case '-': sign = -1; break;
		case '+': sign = 1; break;
		case 'Z': ds.tm_gmt_type = sp_time_exp_t::gmt_set; break;
		default: break;
		}

		++ gmtstr;
		auto off1 = gmtstr.readChars<StringView::CharGroup<CharGroupId::Numbers>>();
		if (off1.size() == 2 && gmtstr.is(':')) {
			++gmtstr;
			auto off2 = gmtstr.readChars<StringView::CharGroup<CharGroupId::Numbers>>();
			if (off2.size() == 2) {
				ds.tm_gmtoff += sign * off1.readInteger().get() * 60 * 60;
				ds.tm_gmtoff += sign * off2.readInteger().get() * 60;
				ds.tm_gmt_type = sp_time_exp_t::gmt_set;
			}
		} else if (off1.size() == 4) {
			auto offset = off1.readInteger().get();
			ds.tm_gmtoff += sign * (offset / 100) * 60 * 60;
			ds.tm_gmtoff += sign * (offset % 100) * 60;
			ds.tm_gmt_type = sp_time_exp_t::gmt_set;
		}
	} else {
		ds.tm_gmt_type = sp_time_exp_t::gmt_local;
	}
	return true;
}

/*
 * Parses an HTTP date in one of three standard forms:
 *
 *     Sun, 06 Nov 1994 08:49:37 GMT  ; RFC 822, updated by RFC 1123
 *     Sunday, 06-Nov-94 08:49:37 GMT ; RFC 850, obsoleted by RFC 1036
 *     Sun Nov  6 08:49:37 1994       ; ANSI C's asctime() format
 *     2011-04-28T06:34:00+09:00      ; Atom time format
 */

bool sp_time_exp_t::read(StringView r) {
	StringView monstr, timstr, gmtstr;

	r.skipChars<StringView::CharGroup<CharGroupId::WhiteSpace>>();

	if (r.empty()) {
		return false;
	}

	auto tmp = r;
	tmp.skipUntil<StringView::Chars<' '>>();

	tm_gmt_type = gmt_unset;
	if (!tmp.is(' ')) {
		if (sp_date_checkmask(r, "####-##-##T##:##:##*")) {
			// 2011-04-28T06:34:00+09:00 ; Atom time format
			tm_year = ((r[0] - '0') * 10 + (r[1] - '0') - 19) * 100;
			if (tm_year < 0) { return false; }

			tm_year += ((r[2] - '0') * 10) + (r[3] - '0');
			tm_mon = ((r[5] - '0') * 10) + (r[6] - '0') - 1;
			tm_mday = ((r[8] - '0') * 10) + (r[9] - '0');

			r += 11;
			if (!sp_time_exp_read_time(*this, r.sub(0, 8))) { return false; }
			if (!sp_time_exp_check_mon(*this)) { return false; }
			r += 8;

			if (r.is('.')) {
				double v = r.readDouble().get();
				tm_usec = 1000000 * v;
			}
			return sp_time_exp_read_gmt(*this, r.empty()?"Z":r);
		} else if (sp_date_checkmask(r, "####-##-##*")) {
			// 2011-04-28 ; Atom date format
			tm_year = ((r[0] - '0') * 10 + (r[1] - '0') - 19) * 100;
			if (tm_year < 0)
				return false;

			tm_year += ((r[2] - '0') * 10) + (r[3] - '0');
			tm_mon = ((r[5] - '0') * 10) + (r[6] - '0') - 1;
			tm_mday = ((r[8] - '0') * 10) + (r[9] - '0');
			if (!sp_time_exp_check_mon(*this)) { return false; }
			r += "####-##-##"_len;
			return sp_time_exp_read_gmt(*this, r.empty()?"Z":r);
		} else if (sp_date_checkmask(r, "##.##.####")) {
			// 12.03.2010
			tm_year = ((r[6] - '0') * 10 + (r[7] - '0') - 19) * 100;
			if (tm_year < 0)
				return false;

			tm_year += ((r[8] - '0') * 10) + (r[9] - '0');
			tm_mday = ((r[0] - '0') * 10) + (r[1] - '0');
			tm_mon = ((r[3] - '0') * 10) + (r[4] - '0') - 1;
			if (!sp_time_exp_check_mon(*this)) { return false; }
			return sp_time_exp_read_gmt(*this, StringView("Z"));
		}
		return false;
	}

	 if (sp_date_checkmask(r, "@$$ @$$ ~# ##:##:## *")) {
		// Sun Nov  6 08:49:37 1994       ; ANSI C's asctime() format
		auto ydate = r.sub(20); // StringView(r.data() + 20, r.size() - 20);
		ydate.skipUntil<StringView::CharGroup<CharGroupId::Numbers>>();
		if (ydate.size() < 4) {
			return false;
		}

		tm_year = ((ydate[0] - '0') * 10 + (ydate[1] - '0') - 19) * 100;
		if (tm_year < 0) { return false; }
		tm_year += ((ydate[2] - '0') * 10) + (ydate[3] - '0');
		tm_mday = (r[8] == ' ') ? (r[9] - '0') : (((r[8] - '0') * 10) + (r[9] - '0'));

		monstr = r.sub(4, 3);
		timstr = r.sub(11, 8);

		if (!sp_time_exp_read_time(*this, timstr)) { return false; }
		if (!sp_time_exp_read_mon(*this, monstr)) { return false; }

		tm_usec = 0;
		tm_gmtoff = 0;
		tm_gmt_type = gmt_local;
		return true;
	}

	r.skipUntil<StringView::CharGroup<CharGroupId::Numbers>>();

	if (sp_date_checkmask(r, "## @$$ #### ##:##:## *")) {
		// Sun, 06 Nov 1994 08:49:37 GMT  ; RFC 822, updated by RFC 1123
		tm_year = ((r[7] - '0') * 10 + (r[8] - '0') - 19) * 100;
		if (tm_year < 0) { return false; }

		tm_year += ((r[9] - '0') * 10) + (r[10] - '0');
		tm_mday = ((r[0] - '0') * 10) + (r[1] - '0');

		monstr = r.sub(3, 3);
		timstr = r.sub(12, 8);
		gmtstr = r.sub(21);
	} else if (sp_date_checkmask(r, "# @$$ #### ##:##:## *")) {
		/* RFC 1123 format with one day */
		tm_year = ((r[6] - '0') * 10 + (r[7] - '0') - 19) * 100;
		if (tm_year < 0)
			return false;

		tm_year += ((r[8] - '0') * 10) + (r[9] - '0');
		tm_mday = (r[0] - '0');

		monstr = r.sub(2, 3);
		timstr = r.sub(11, 8);
		gmtstr = r.sub(20);
	} else if (sp_date_checkmask(r, "##-@$$-## ##:##:## *")) {
		// Sunday, 06-Nov-94 08:49:37 GMT ; RFC 850, obsoleted by RFC 1036
		tm_year = ((r[7] - '0') * 10) + (r[8] - '0');
		if (tm_year < 70)
			tm_year += 100;

		tm_mday = ((r[0] - '0') * 10) + (r[1] - '0');

		monstr = r.sub(3, 3);
		timstr = r.sub(10, 8);
		gmtstr = r.sub(19);
	} else {
		return false;
	}

	if (!sp_time_exp_read_time(*this, timstr)) { return false; }
	if (!sp_time_exp_read_mon(*this, monstr)) { return false; }

	tm_usec = 0;

	if (!gmtstr.empty()) {
		if (!sp_time_exp_read_gmt(*this, gmtstr)) {
			return false;
		}
	} else {
		tm_gmtoff = 0;
	}

	return true;
}

static const char sp_month_snames[12][4] = {
    "Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
};
static const char sp_day_snames[7][4] = {
    "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"
};

size_t sp_time_exp_t::encodeRfc822(char *date_str) const {
	auto start = date_str;
	const char *s;
	int real_year;

	/* example: "Sat, 08 Jan 2000 18:31:41 GMT" */
	/*           12345678901234567890123456789  */

	s = &sp_day_snames[tm_wday][0];
	*date_str++ = *s++;
	*date_str++ = *s++;
	*date_str++ = *s++;
	*date_str++ = ',';
	*date_str++ = ' ';
	*date_str++ = tm_mday / 10 + '0';
	*date_str++ = tm_mday % 10 + '0';
	*date_str++ = ' ';
	s = &sp_month_snames[tm_mon][0];
	*date_str++ = *s++;
	*date_str++ = *s++;
	*date_str++ = *s++;
	*date_str++ = ' ';
	real_year = 1900 + tm_year;
	/* This routine isn't y10k ready. */
	*date_str++ = real_year / 1000 + '0';
	*date_str++ = real_year % 1000 / 100 + '0';
	*date_str++ = real_year % 100 / 10 + '0';
	*date_str++ = real_year % 10 + '0';
	*date_str++ = ' ';
	*date_str++ = tm_hour / 10 + '0';
	*date_str++ = tm_hour % 10 + '0';
	*date_str++ = ':';
	*date_str++ = tm_min / 10 + '0';
	*date_str++ = tm_min % 10 + '0';
	*date_str++ = ':';
	*date_str++ = tm_sec / 10 + '0';
	*date_str++ = tm_sec % 10 + '0';
	*date_str++ = ' ';
	*date_str++ = 'G';
	*date_str++ = 'M';
	*date_str++ = 'T';
	*date_str++ = 0;
	return date_str - start - 1;
}

size_t sp_time_exp_t::encodeCTime(char *date_str) const {
	auto start = date_str;
	const char *s;
	int real_year;

	/* example: "Wed Jun 30 21:49:08 1993" */
	/*           123456789012345678901234  */

	s = &sp_day_snames[tm_wday][0];
	*date_str++ = *s++;
	*date_str++ = *s++;
	*date_str++ = *s++;
	*date_str++ = ' ';
	s = &sp_month_snames[tm_mon][0];
	*date_str++ = *s++;
	*date_str++ = *s++;
	*date_str++ = *s++;
	*date_str++ = ' ';
	*date_str++ = tm_mday / 10 + '0';
	*date_str++ = tm_mday % 10 + '0';
	*date_str++ = ' ';
	*date_str++ = tm_hour / 10 + '0';
	*date_str++ = tm_hour % 10 + '0';
	*date_str++ = ':';
	*date_str++ = tm_min / 10 + '0';
	*date_str++ = tm_min % 10 + '0';
	*date_str++ = ':';
	*date_str++ = tm_sec / 10 + '0';
	*date_str++ = tm_sec % 10 + '0';
	*date_str++ = ' ';
	real_year = 1900 + tm_year;
	*date_str++ = real_year / 1000 + '0';
	*date_str++ = real_year % 1000 / 100 + '0';
	*date_str++ = real_year % 100 / 10 + '0';
	*date_str++ = real_year % 10 + '0';
	*date_str++ = 0;
	return date_str - start - 1;
}

size_t sp_time_exp_t::encodeIso8601(char *date_str, size_t precision) const {
	auto start = date_str;
	int real_year;

	real_year = 1900 + tm_year;
	*date_str++ = real_year / 1000 + '0';		// 1
	*date_str++ = real_year % 1000 / 100 + '0';	// 2
	*date_str++ = real_year % 100 / 10 + '0';	// 3
	*date_str++ = real_year % 10 + '0';			// 4
	*date_str++ = '-';							// 5
	*date_str++ = (tm_mon + 1) / 10 + '0';	// 6
	*date_str++ = (tm_mon + 1) % 10 + '0';	// 7
	*date_str++ = '-';							// 8
	*date_str++ = tm_mday / 10 + '0';		// 9
	*date_str++ = tm_mday % 10 + '0';		// 10
	*date_str++ = 'T';							// 11
	*date_str++ = tm_hour / 10 + '0';		// 12
	*date_str++ = tm_hour % 10 + '0';		// 13
	*date_str++ = ':';							// 14
	*date_str++ = tm_min / 10 + '0';			// 15
	*date_str++ = tm_min % 10 + '0';			// 16
	*date_str++ = ':';							// 17
	*date_str++ = tm_sec / 10 + '0';			// 18
	*date_str++ = tm_sec % 10 + '0';			// 19

	if (precision > 0 && precision <= 6) {
		auto intpow = [] (int val, int p) {
			int ret = 1;
			while (p > 0) {
				ret *= val;
				-- p;
			}
			return ret;
		};

		*date_str++ = '.';
		const int desc = SP_USEC_PER_SEC / intpow(10, int(precision));
		auto val = int32_t(std::round(tm_usec / double(desc)));
		while (precision > 0) {
			auto d = val / intpow(10, int(precision - 1));
			*date_str++ = '0' + d;
			val = val % intpow(10, int(precision - 1));
			-- precision;
		}
	}

	*date_str++ = 'Z';							// 20
	*date_str++ = 0;
	return date_str - start - 1;
}

Time Time::fromCompileTime(const char *date, const char *time) {
	sp_time_exp_t ds;
	ds.tm_year = ((date[7] - '0') * 10 + (date[8] - '0') - 19) * 100;
	if (ds.tm_year < 0) {
		return Time();
	}

	ds.tm_year += ((date[9] - '0') * 10) + (date[10] - '0');
	ds.tm_mday = ((date[4] != ' ') ? ((date[4] - '0') * 10) : 0) + (date[5] - '0');

	int mint = (date[0] << 16) | (date[1] << 8) | date[2];
	int mon = 0;
	for (; mon < 12; mon++) {
		if (mint == s_months[mon])
			break;
	}

	if (mon == 12)
		return Time();

	if (ds.tm_mday <= 0 || ds.tm_mday > 31)
		return Time();

	if ((ds.tm_mday == 31) && (mon == 3 || mon == 5 || mon == 8 || mon == 10))
		return Time();

	if ((mon == 1)
			&& ((ds.tm_mday > 29)
					|| ((ds.tm_mday == 29)
							&& ((ds.tm_year & 3) || (((ds.tm_year % 100) == 0) && (((ds.tm_year % 400) != 100))))
					)
			))
		return Time();

	ds.tm_mon = mon;
	ds.tm_hour = ((time[0] - '0') * 10) + (time[1] - '0');
	ds.tm_min = ((time[3] - '0') * 10) + (time[4] - '0');
	ds.tm_sec = ((time[6] - '0') * 10) + (time[7] - '0');

	if ((ds.tm_hour > 23) || (ds.tm_min > 59) || (ds.tm_sec > 61))
		return Time();

	ds.tm_usec = 0;
	ds.tm_gmtoff = 0;
	return ds.ltz_get();
}

Time Time::fromHttp(StringView r) {
	sp_time_exp_t ds;
	if (!ds.read(r)) {
		return Time();
	}

	switch (ds.tm_gmt_type) {
	case sp_time_exp_t::gmt_set: return ds.gmt_get();
	case sp_time_exp_t::gmt_local: return ds.ltz_get();
	case sp_time_exp_t::gmt_unset: return ds.get();
	}

	return Time();
}

size_t Time::encodeToFormat(char *buf, size_t bufSize, const char *fmt) const {
	struct tm tm;
	time_t tt = toSeconds();
#if WIN32
	gmtime_s(&tm, &tt);
#else
	gmtime_r(&tt, &tm);
#endif

	return strftime(buf, bufSize, fmt, &tm);
}

}
