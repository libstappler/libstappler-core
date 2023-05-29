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

#ifndef STAPPLER_DATA_SPDATA_H_
#define STAPPLER_DATA_SPDATA_H_

#include "SPDataDecode.h"
#include "SPDataEncode.h"

namespace stappler::data {

// command line options parsing
//
// arguments, prefixed with '-' resolved as char switches
// 'switchCallback' used to process this chars
// 'char c' - current char,
// 'const char *str' - string in which this char was found, str[0] = c;
// return value - number of processed chars (usually - 1)
// For string '-name' switchCallback will be called 4 times with each char in string,
// but you can process whole string in first call, then return 4 from callback
//
// arguments, prefixed with '--' resolved as string options
// 'stringCallback' used to process this strings
// 'const String &str' - current parsed string
// 'int argc' - number of strings in argv
// 'const char * argv[]' - remaining strings to parse
// return value - number of parsed strings (usually 1)
//
// Other strings will be added into array 'args' in return value

template <typename Interface>
auto parseCommandLineOptions(int argc, const char * argv[],
		const Callback<int (ValueTemplate<Interface> &ret, char c, const char *str)> &switchCallback,
		const Callback<int (ValueTemplate<Interface> &ret, const StringView &str, int argc, const char * argv[])> &stringCallback)
-> ValueTemplate<Interface>;

template <typename Interface>
auto parseCommandLineOptions(int argc, const char16_t * wargv[],
		const Callback<int (ValueTemplate<Interface> &ret, char c, const char *str)> &switchCallback,
		const Callback<int (ValueTemplate<Interface> &ret, const StringView &str, int argc, const char * argv[])> &stringCallback)
-> ValueTemplate<Interface>;


// decode x-www-urlencoded into data
template <typename Interface>
auto readUrlencoded(StringView, size_t maxLength = maxOf<size_t>(),
		size_t maxVarSize = maxOf<size_t>()) -> data::ValueTemplate<Interface>;


template <typename Interface>
auto parseCommandLineOptions(int argc, const char * argv[],
		const Callback<int (ValueTemplate<Interface> &ret, char c, const char *str)> &switchCallback,
		const Callback<int (ValueTemplate<Interface> &ret, const StringView &str, int argc, const char * argv[])> &stringCallback)
-> ValueTemplate<Interface> {
	if (argc == 0) {
		return ValueTemplate<Interface>();
	}

	ValueTemplate<Interface> ret;
	auto &args = ret.setValue(ValueTemplate<Interface>(ValueTemplate<Interface>::Type::ARRAY), "args");

	int i = argc;
	while (i > 0) {
		const char *value = argv[argc - i];
		char quoted = 0;
		if (value[0] == '\'' || value[0] == '"') {
			quoted = value[0];
			value ++;
		}
		if (value[0] == '-') {
			if (value[1] == '-') {
				if (stringCallback) {
					i -= (stringCallback(ret, &value[2], i - 1, &argv[argc - i + 1]) - 1);
				} else {
					i -= 1;
				}
			} else {
				if (switchCallback) {
					const char *str = &value[1];
					while (str[0] != 0) {
						str += switchCallback(ret, str[0], &str[1]);
					}
				}
			}
		} else {
			if (quoted > 0) {
				size_t len = strlen(value);
				if (len > 0 && value[len - 1] == quoted) {
					-- len;
				}
				args.addString(StringView(value, len));
			} else {
				if (i == argc) {
#ifdef MODULE_STAPPLER_FILESYSTEM
					args.addString(filesystem::native::nativeToPosix<Interface>(value));
#else
					args.addString(value);
#endif
				} else {
					args.addString(value);
				}
			}
		}
		i --;
	}

	return ret;
}

template <typename Interface>
auto parseCommandLineOptions(int argc, const char16_t * wargv[],
		const Callback<int (ValueTemplate<Interface> &ret, char c, const char *str)> &switchCallback,
		const Callback<int (ValueTemplate<Interface> &ret, const StringView &str, int argc, const char * argv[])> &stringCallback)
-> ValueTemplate<Interface> {
	typename Interface::template VectorType<typename Interface::StringType> vec; vec.reserve(argc);
	typename Interface::template VectorType<const char *> argv; argv.reserve(argc);
	for (int i = 0; i < argc; ++ i) {
		vec.push_back(string::toUtf8<Interface>(wargv[i]));
		argv.push_back(vec.back().c_str());
	}

	return parseCommandLineOptions(argc, argv.data(), switchCallback, stringCallback);
}

}

#endif // STAPPLER_DATA_SPDATA_H_
