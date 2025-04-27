/**
 Copyright (c) 2025 Stappler LLC <admin@stappler.dev>

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

#include "SPXCodeProject.h"
#include "SPPlatform.h"
#include "SPString.h"

namespace STAPPLER_VERSIONIZED stappler::buildtool::xcode {

struct Globalidentifier {
	int8_t user; // encoded username
	int8_t pid; // encoded current pid #
	int16_t _random; // encoded random value
	int32_t _time; // encoded time value
	int8_t zero; // zero
	int8_t host_shift; // encoded, shifted hostid
	int8_t host_h; // high byte of lower bytes of hostid
	int8_t host_l; // low byte of lower bytes of hostid
} __attribute__((packed));

static auto s_template =
		R"(// !$*UTF8*$!
{
	archiveVersion = 1;
	classes = {
	};
	objectVersion = 77;
	objects = {

/* Begin PBXAggregateTarget section */
/* End PBXAggregateTarget section */

/* Begin PBXContainerItemProxy section */
/* End PBXContainerItemProxy section */

/* Begin PBXFileReference section */
/* End PBXFileReference section */

/* Begin PBXFileSystemSynchronizedBuildFileExceptionSet section */
/* End PBXFileSystemSynchronizedBuildFileExceptionSet section */

/* Begin PBXFileSystemSynchronizedRootGroup section */
/* End PBXFileSystemSynchronizedRootGroup section */

/* Begin PBXFrameworksBuildPhase section */
/* End PBXFrameworksBuildPhase section */

/* Begin PBXGroup section */
/* End PBXGroup section */

/* Begin PBXHeadersBuildPhase section */
/* End PBXHeadersBuildPhase section */

/* Begin PBXNativeTarget section */
/* End PBXNativeTarget section */

/* Begin PBXProject section */
/* End PBXProject section */

/* Begin PBXResourcesBuildPhase section */
/* End PBXResourcesBuildPhase section */

/* Begin PBXSourcesBuildPhase section */
/* End PBXSourcesBuildPhase section */

/* Begin PBXTargetDependency section */
/* End PBXTargetDependency section */

/* Begin XCBuildConfiguration section */
/* End XCBuildConfiguration section */

/* Begin XCConfigurationList section */
/* End XCConfigurationList section */
	};
	rootObject = 2670DFEC2CC5766A00B3A76A /* Project object */;
})";

// globals

static uint64_t packedValueForChar = 0x1f1f'1f1f'1f1f'1f1f;
static int8_t hasInitialized = false;
static int32_t lasttime = 0;
static int16_t firstseq = 0;
static struct Globalidentifier gid = {};

static const char *getCurrentUsername() { return getlogin(); };

// convenience method for bswap
#define bswap32(x) \
({ \
    uint32_t __x = (x); \
    ((uint32_t)( \
        (((uint32_t)(__x) & (uint32_t)0x0000'00ffUL) << 24) | \
        (((uint32_t)(__x) & (uint32_t)0x0000'ff00UL) <<  8) | \
        (((uint32_t)(__x) & (uint32_t)0x00ff'0000UL) >>  8) | \
        (((uint32_t)(__x) & (uint32_t)0xff00'0000UL) >> 24) )); \
})

// convenience method for ROL
static int16_t rotl16(int16_t value, unsigned int count) {
	const unsigned int mask = (CHAR_BIT * sizeof(value) - 1);
	count &= mask;
	return (value << count) | (value >> ((-count) & mask));
}

// the original function (same name) can be found in DevToolsSupport.framework
static int8_t *TSGenerateUniqueGlobalID(int8_t *buff) {

	/* Note: the "gid" variable is a reference to the global identifier struct */

	int8_t *buffer = buff;
	if (hasInitialized == 0) {
		hasInitialized = 1;

		pid_t current_pid = getpid();
		gid.pid = (current_pid & 0xff);

		const char *username = getCurrentUsername();
		uint64_t index = 0;
		char letter = username[index];
		int32_t counter = 0;
		int8_t output = 0;

		// perfom the encoding on the username
		do {
			int8_t value = 0x1f;
			letter = username[index];
			if (letter >= 0) {
				value = (int8_t)((letter & 0xff) + packedValueForChar);
			}
			if (counter != 0) {
				value = (value & 0xff) << counter >> 0x8 | (value & 0xff) << counter;
			}
			counter = (counter + 0x5) & 0x7;
			output = output ^ value;
			index++;
		} while (letter != '\0');
		// store the encoded byte
		gid.user = output;

		int32_t host_id = 0;
		int32_t temp = gethostid(); // this is used even though it is deprecated on OS X
		if (temp != -1) {
			host_id = temp;
		}
		// generate the random seed
		int64_t time_seed = platform::nanoclock();
		srandom(((current_pid & 0xff) << 0x10 | host_id) ^ time_seed);
		if (host_id == 0) {
			host_id = random();
		}

		gid.zero = 0;
		gid.host_shift = (host_id >> 0x10) & 0xff; // low byte after shift
		gid.host_h = (host_id & 0xff00) >> 0x8; // high byte
		gid.host_l = (host_id & 0xff); // low byte
		gid._random = random();
	}

	// increment the random value
	int16_t random_value = gid._random;
	random_value += 1;
	gid._random = random_value;

	// encode the time value and check to make sure we don't have conflicts with a previous add
	// (eg when two adds happen in close enough timeframe)
	int64_t time_val = platform::nanoclock();
	int32_t encoded_time = 0;
	if (time_val > lasttime) {
		firstseq = random_value;
		lasttime = time_val;
	} else {
		if (firstseq == random_value) {
			lasttime += 1;
		}
	}
	encoded_time = lasttime;
	// now swap byte ordering
	gid._time = bswap32(encoded_time);

	// rotate the random value for output
	int16_t random_rol = rotl16(random_value, 0x8);
	gid._random = random_rol;

	// copy to passed buffer, this will always be 12 bytes
	memcpy(buffer, &gid, sizeof(Globalidentifier));

	// roll the rolled value again for storing
	gid._random = rotl16(random_rol, 0x8);

	// return passed buffer
	return buffer;
}

Id PBXObject::generateId() {
	Id ret;
	TSGenerateUniqueGlobalID((int8_t *)ret.data());
	return ret;
}

StringId PBXObject::getStringId(Id id) {
	StringId ret;
	base16::encode(ret.data(), ret.size(), CoderSource(id), true);
	return ret;
}

} // namespace stappler::buildtool::xcode
