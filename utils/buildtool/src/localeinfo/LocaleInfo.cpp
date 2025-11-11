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

#include "SPCommon.h" // << Prefix header
#include "LocaleInfo.h"
#include "SPMemory.h"
#include "SPNetworkHandle.h"
#include "SPLocaleInfo.h"

namespace STAPPLER_VERSIONIZED stappler::buildtool {

static constexpr auto sep = '|';

static void encodeCountry(const CallbackStream &cb, const CountryInfo &country) {
	cb << string::tolower<memory::StandartInterface>(country.code) << ":" << country.name << sep
	   << country.nameLocal << sep << country.code << sep << country.continent << sep
	   << country.region << sep << country.capital << sep << country.currency << sep
	   << country.currencyLocal << sep << country.currencyCode << sep << country.currencySymbol
	   << sep << country.currencySubunit << sep << country.languages << sep << country.flagSymbol
	   << sep << country.timezones << sep << country.borders << sep << country.postalCodeFormat
	   << sep << country.iso3166_1alpha2 << sep << country.iso3166_1alpha3 << sep << country.tld
	   << sep << country.vehicleCode << sep << country.un_locode << sep << country.iso3166_1numeric;
}

static void encodeLanguage(const CallbackStream &cb, const LanguageInfo &language) {
	cb << string::tolower<memory::StandartInterface>(language.code) << ":" << language.name << sep
	   << language.nameLocal << sep << language.code << sep << language.iso639_1 << sep
	   << language.iso639_2 << sep << language.iso639_3 << sep << language.countries;
}

static CountryInfo parseCountry(const data::ValueTemplate<memory::StandartInterface> &data) {
	CountryInfo ret;
	for (auto &it : data.asDict()) {
		if (it.first == "name") {
			ret.name = StringView(it.second.asString()).pdup();
		} else if (it.first == "name_local") {
			ret.nameLocal = StringView(it.second.asString()).pdup();
		} else if (it.first == "code") {
			ret.code = StringView(it.second.asString()).pdup();
		} else if (it.first == "continent") {
			ret.continent = StringView(it.second.asString()).pdup();
		} else if (it.first == "region") {
			ret.region = StringView(it.second.asString()).pdup();
		} else if (it.first == "capital_name") {
			ret.capital = StringView(it.second.asString()).pdup();
		} else if (it.first == "currency") {
			ret.currency = StringView(it.second.asString()).pdup();
		} else if (it.first == "currency_local") {
			ret.currencyLocal = StringView(it.second.asString()).pdup();
		} else if (it.first == "currency_code") {
			ret.currencyCode = StringView(it.second.asString()).pdup();
		} else if (it.first == "currency_symbol") {
			ret.currencySymbol = StringView(it.second.asString()).pdup();
		} else if (it.first == "currency_subunit_name") {
			ret.currencySubunit = StringView(it.second.asString()).pdup();
		} else if (it.first == "languages") {
			memory::PoolInterface::StringStreamType out;
			for (auto &iit : it.second.asArray()) {
				if (!out.empty()) {
					out << ";";
				}
				if (iit.getString("iso_639_1").empty()) {
					out << iit.getString("iso_639_2");
				} else {
					out << iit.getString("iso_639_1");
				}
			}
			ret.languages = StringView(out.weak()).pdup();
		} else if (it.first == "flag") {
			ret.flagSymbol = StringView(it.second.asString()).pdup();
		} else if (it.first == "timezones") {
			memory::PoolInterface::StringStreamType out;
			for (auto &iit : it.second.asArray()) {
				if (!out.empty()) {
					out << ";";
				}
				out << iit.getString();
			}
			ret.timezones = StringView(out.weak()).pdup();
		} else if (it.first == "borders") {
			memory::PoolInterface::StringStreamType out;
			for (auto &iit : it.second.asArray()) {
				if (!out.empty()) {
					out << ";";
				}
				out << iit.getString();
			}
			ret.borders = StringView(out.weak()).pdup();
		} else if (it.first == "postal_code_format") {
			ret.postalCodeFormat = StringView(it.second.asString()).pdup();
		} else if (it.first == "iso_3166_1_numeric") {
			ret.iso3166_1numeric = it.second.getInteger();
		} else if (it.first == "iso_3166_1_alpha2") {
			ret.iso3166_1alpha2 = StringView(it.second.asString()).pdup();
		} else if (it.first == "iso_3166_1_alpha3") {
			ret.iso3166_1alpha3 = StringView(it.second.asString()).pdup();
		} else if (it.first == "tld") {
			ret.tld = StringView(it.second.asString()).pdup();
		} else if (it.first == "vehicle_code") {
			ret.vehicleCode = StringView(it.second.asString()).pdup();
		} else if (it.first == "un_locode") {
			ret.un_locode = StringView(it.second.asString()).pdup();
		}
	}
	return ret;
}

static LanguageInfo parseLanguage(const data::ValueTemplate<memory::StandartInterface> &data) {
	LanguageInfo ret;
	for (auto &it : data.asDict()) {
		if (it.first == "name") {
			ret.name = StringView(it.second.asString()).pdup();
		} else if (it.first == "name_local") {
			ret.nameLocal = StringView(it.second.asString()).pdup();
		} else if (it.first == "iso_639_1") {
			ret.iso639_1 = StringView(it.second.asString()).pdup();
		} else if (it.first == "iso_639_2") {
			ret.iso639_2 = StringView(it.second.asString()).pdup();
		} else if (it.first == "iso_639_3") {
			ret.iso639_3 = StringView(it.second.asString()).pdup();
		} else if (it.first == "countries") {
			memory::PoolInterface::StringStreamType out;
			for (auto &iit : it.second.asArray()) {
				if (!out.empty()) {
					out << ";";
				}
				out << iit.getString("code");
			}
			ret.countries = StringView(out.weak()).pdup();
		}
	}
	return ret;
}

static bool isSymbolic(StringView str) {
	str.skipChars<StringView::Latin>();
	return str.empty();
}

static uint32_t getSymbolicIndex(StringView istr, uint32_t cap) {
	return istr.hash32() & (cap - 1);
}

static bool parseLocaleData(const data::ValueTemplate<memory::StandartInterface> &data) {
	static constexpr auto LanguagesCapacity = 256;
	static constexpr auto CountriesCapacity = 512;

	uint32_t nlanguages = 0;
	uint32_t ncountries = 0;

	std::array<StringView, LanguagesCapacity> languagesArray;
	std::array<StringView, CountriesCapacity> countriesArray;

	auto getLanguageByCode = [&](StringView code) {
		//uint32_t offset = 0;
		auto idx = getSymbolicIndex(code, LanguagesCapacity);
		while (!languagesArray[idx].starts_with(code)) {
			++idx;
			idx &= (LanguagesCapacity - 1);
			//++offset;
		}
		//if (offset > 0) {
		//	std::cout << code << ": " << offset << "\n";
		//}
		return languagesArray[idx];
	};

	auto getCountryByCode = [&](StringView code) {
		//uint32_t offset = 0;
		auto idx = getSymbolicIndex(code, CountriesCapacity);
		while (!countriesArray[idx].starts_with(code)) {
			++idx;
			idx &= (CountriesCapacity - 1);
			//++offset;
		}
		//if (offset > 0) {
		//	std::cout << code << ": " << offset << "\n";
		//}
		return countriesArray[idx];
	};

	memory::StandartInterface::MapType<StringView, LanguageInfo> languages;
	memory::StandartInterface::MapType<StringView, CountryInfo> countries;
	memory::StandartInterface::MapType<StringView, LocaleInfo> locales;

	for (auto &it : data.asArray()) {
		auto name = StringView(string::tolower<memory::StandartInterface>(it.getString("locale")))
							.pdup();
		auto country = parseCountry(it.getValue("country"));
		auto cIt = countries.find(country.code);
		if (cIt == countries.end()) {
			if (country.code.size() > 2 || !isSymbolic(country.code)) {
				std::cout << name << " code.size() > 2" << "\n";
			}

			auto code = string::tolower<memory::StandartInterface>(country.code);
			auto idx = getSymbolicIndex(code, CountriesCapacity);
			while (!countriesArray[idx].empty()) {
				++idx;
				idx &= (CountriesCapacity - 1);
			}
			memory::PoolInterface::StringStreamType out;
			encodeCountry(memory::makeCallback(out), country);
			countriesArray[idx] = StringView(out.weak()).pdup();
			cIt = countries.emplace(country.code, country).first;
			++ncountries;
		}

		auto language = parseLanguage(it.getValue("language"));
		language.code = StringView(name).readUntil<StringView::Chars<'-'>>();
		auto lIt = languages.find(language.code);
		if (lIt == languages.end()) {
			auto idx = getSymbolicIndex(language.code, LanguagesCapacity);
			while (!languagesArray[idx].empty()) {
				++idx;
				idx &= (LanguagesCapacity - 1);
			}
			memory::PoolInterface::StringStreamType out;
			encodeLanguage(memory::makeCallback(out), language);
			languagesArray[idx] = StringView(out.weak()).pdup();
			lIt = languages.emplace(language.code, language).first;
			++nlanguages;
		}

		auto iit = locales.find(name);
		if (iit != locales.end()) {
			locales.emplace(name, LocaleInfo{name, lIt->second, cIt->second});
		}
	}

	for (auto &it : languages) {
		auto v = getLanguageByCode(it.second.code);
		if (v.starts_with(it.second.code)) {
			//std::cout << v << "\n";
		} else {
			std::cout << "Fail to find: " << it.second.code << "\n";
		}
	}
	for (auto &it : countries) {
		auto code = string::tolower<memory::StandartInterface>(it.second.code);
		auto v = getCountryByCode(code);
		if (v.starts_with(StringView(code))) {
			//std::cout << v << "\n";
		} else {
			std::cout << "Fail to find: " << it.second.code << "\n";
		}
	}

	std::cout << nlanguages << " " << ncountries << "\n";

	std::cout << "static constexpr std::array<StringView, " << LanguagesCapacity
			  << "> s_languagesArray({\n";
	for (auto &it : languagesArray) {
		if (!it.empty()) {
			std::cout << "\tStringView(\"" << it << "\"),\n";
		} else {
			std::cout << "\tStringView(),\n";
		}
	}
	std::cout << "});\n";

	std::cout << "static constexpr std::array<StringView, " << CountriesCapacity
			  << "> s_countriesArray({\n";
	for (auto &it : countriesArray) {
		if (!it.empty()) {
			std::cout << "\tStringView(\"" << it << "\"),\n";
		} else {
			std::cout << "\tStringView(),\n";
		}
	}
	std::cout << "});\n";

	return true;
}

bool buildLocaleInfo(FileInfo inputFile) {
	auto data = data::readFile<memory::StandartInterface>(inputFile);
	if (!data) {
		return false;
	}

	return parseLocaleData(data);
}

bool buildLocaleInfoFromNetwork() {
	//auto l1 = LocaleInfo::get("ru-ru");
	//auto l2 = LocaleInfo::get("ru_RU.UTF_8");

	network::Handle<memory::StandartInterface> handle;
	handle.init(network::Method::Get, "https://cdn.simplelocalize.io/public/v1/locales");

	memory::StandartInterface::StringStreamType dataStream;
	handle.setReceiveCallback([&](char *buf, size_t size) -> size_t {
		dataStream << StringView(buf, size);
		return size;
	});

	auto result = handle.perform();
	if (!result) {
		return false;
	}
	auto data = data::read<memory::StandartInterface>(dataStream.str());
	if (!data) {
		return false;
	}

	return parseLocaleData(data);
}

} // namespace stappler::buildtool
