/**
Copyright (c) 2019-2022 Roman Katuntsev <sbkarr@stappler.org>
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

#ifndef STAPPLER_DB_STSTORAGECONFIG_H_
#define STAPPLER_DB_STSTORAGECONFIG_H_

#include "SPTime.h"

// Server frameworks usually provides their own config for db module

#ifdef MODULE_STAPPLER_DB_CONFIG

#include MODULE_STAPPLER_DB_CONFIG

#else

// use common config

#include "SPFilesystem.h"

namespace stappler::db::config {

constexpr auto getMaxAuthTime() { return 720_sec; }
constexpr auto getMaxLoginFailure() { return 4; }

constexpr size_t getMaxVarSize() { return 255; }
constexpr size_t getMaxRequestSize() { return 0; }
constexpr size_t getMaxFileSize() { return 0; }

constexpr auto getInputUpdateTime() { return TimeInterval::seconds(1); }
constexpr auto getInputUpdateFrequency() { return 0.1f; }

constexpr size_t getMaxInputPostSize() { return 250_MiB; }
constexpr size_t getMaxInputFileSize() { return 250_MiB; }
constexpr size_t getMaxInputVarSize() { return 8_KiB; }

constexpr size_t getMaxExtraFieldSize() { return 8_KiB; }

constexpr auto getDefaultTextMin() { return 3; }
constexpr auto getDefaultTextMax() { return 256; }

inline auto getDefaultPasswordSalt() { return "StapplerUserPasswordKey"_weak; }

inline stappler::TimeInterval getKeyValueStorageTime() { return TimeInterval::seconds(60 * 60 * 24 * 365); } // one year
inline stappler::TimeInterval getInternalsStorageTime() { return TimeInterval::seconds(60 * 60 * 24 * 30); }

constexpr auto getStorageInterfaceKey() { return "ST.StorageInterface"; }
constexpr auto getTransactionPrefixKey() { return "ST.Tr."; } // limit for 6 chars to use with SOO opts (6 + 16 < 23)
constexpr auto getTransactionStackKey() { return "ST.Transaction.Stack"; }

constexpr auto getUploadTmpFilePrefix() { return "sa.upload"; }
constexpr auto getUploadTmpImagePrefix() { return "sa.image"; }

constexpr auto getStorageBroadcastChannelName() { return "serenity_broadcast"; }

constexpr uint16_t getResourceResolverMaxDepth() { return 4; }

}

#endif

#endif /* STAPPLER_DB_STSTORAGECONFIG_H_ */
