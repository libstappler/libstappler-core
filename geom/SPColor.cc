/**
Copyright (c) 2008-2010 Ricardo Quesada
Copyright (c) 2010-2012 cocos2d-x.org
Copyright (c) 2011      Zynga Inc.
Copyright (c) 2013-2014 Chukong Technologies Inc.
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

#include "SPColor.h"
#include "SPString.h"

namespace STAPPLER_VERSIONIZED stappler::geom {

#define MD_COLOR_SPEC_COMPONENT(Name, Id, Value, Index, Group) \
	Color Color::Name ## _ ## Id(0x ## Value, Group * 16 + Index); \

#define MD_COLOR_SPEC_BASE_DEFINE(Name, Group, b50, b100, b200, b300, b400, b500, b600, b700, b800, b900 ) \
	MD_COLOR_SPEC_COMPONENT(Name, 50, b50, 0, Group) \
	MD_COLOR_SPEC_COMPONENT(Name, 100, b100, 1, Group) \
	MD_COLOR_SPEC_COMPONENT(Name, 200, b200, 2, Group) \
	MD_COLOR_SPEC_COMPONENT(Name, 300, b300, 3, Group) \
	MD_COLOR_SPEC_COMPONENT(Name, 400, b400, 4, Group) \
	MD_COLOR_SPEC_COMPONENT(Name, 500, b500, 5, Group) \
	MD_COLOR_SPEC_COMPONENT(Name, 600, b600, 6, Group) \
	MD_COLOR_SPEC_COMPONENT(Name, 700, b700, 7, Group) \
	MD_COLOR_SPEC_COMPONENT(Name, 800, b800, 8, Group) \
	MD_COLOR_SPEC_COMPONENT(Name, 900, b900, 9, Group)

#define MD_COLOR_SPEC_ACCENT_DEFINE(Name, Group, a100, a200, a400, a700 ) \
	MD_COLOR_SPEC_COMPONENT(Name, A100, a100, 10, Group) \
	MD_COLOR_SPEC_COMPONENT(Name, A200, a200, 11, Group) \
	MD_COLOR_SPEC_COMPONENT(Name, A400, a400, 12, Group) \
	MD_COLOR_SPEC_COMPONENT(Name, A700, a700, 13, Group)

#define MD_COLOR_SPEC_DEFINE(Name, Group, b50, b100, b200, b300, b400, b500, b600, b700, b800, b900, a100, a200, a400, a700) \
	MD_COLOR_SPEC_BASE_DEFINE(Name, Group, b50, b100, b200, b300, b400, b500, b600, b700, b800, b900) \
	MD_COLOR_SPEC_ACCENT_DEFINE(Name, Group, a100, a200, a400, a700)

namespace table {

static struct ColorDataTable {
	uint32_t value;
	uint16_t index;
	uint32_t hash;
	char str[16];
} data[256] = {
	{ 0xffebee, 0x000, 0x923cbb49, "Red50" },
	{ 0xffcdd2, 0x001, 0x76a5549f, "Red100" },
	{ 0xef9a9a, 0x002, 0x049de544, "Red200" },
	{ 0xe57373, 0x003, 0x0aa02d4d, "Red300" },
	{ 0xef5350, 0x004, 0x78988bd2, "Red400" },
	{ 0xf44336, 0x005, 0x7e9ad3db, "Red500" },
	{ 0xe53935, 0x006, 0x6c93fbe0, "Red600" },
	{ 0xd32f2f, 0x007, 0x729643e9, "Red700" },
	{ 0xc62828, 0x008, 0x80b6adbe, "Red800" },
	{ 0xb71c1c, 0x009, 0x86b8f5a7, "Red900" },
	{ 0xff8a80, 0x00a, 0x90873a24, "RedA100" },
	{ 0xff5252, 0x00b, 0x028ea97f, "RedA200" },
	{ 0xff1744, 0x00c, 0xfe7f9849, "RedA400" },
	{ 0xd50000, 0x00d, 0x8482aa32, "RedA700" },
	{ 0xfce4ec, 0x010, 0x4b45fd26, "Pink50" },
	{ 0xf8bbd0, 0x011, 0x1d373a26, "Pink100" },
	{ 0xf48fb1, 0x012, 0x3735245d, "Pink200" },
	{ 0xf06292, 0x013, 0xb13212d4, "Pink300" },
	{ 0xec407a, 0x014, 0xab2fcaeb, "Pink400" },
	{ 0xe91e63, 0x015, 0xa52d82e2, "Pink500" },
	{ 0xd81b60, 0x016, 0x9f2b3af9, "Pink600" },
	{ 0xc2185b, 0x017, 0x19282970, "Pink700" },
	{ 0xad1457, 0x018, 0xb34decb7, "Pink800" },
	{ 0x880e4f, 0x019, 0x2d4adace, "Pink900" },
	{ 0xff80ab, 0x01a, 0x3f00b37b, "PinkA100" },
	{ 0xff4081, 0x01b, 0xacf91180, "PinkA200" },
	{ 0xf50057, 0x01c, 0xb10822b6, "PinkA400" },
	{ 0xc51162, 0x01d, 0x4b05436d, "PinkA700" },
	{ 0xf3e5f5, 0x020, 0x054596d0, "Purple50" },
	{ 0xe1bee7, 0x021, 0xb4965324, "Purple100" },
	{ 0xce93d8, 0x022, 0x269dc27f, "Purple200" },
	{ 0xba68c8, 0x023, 0x209b7a76, "Purple300" },
	{ 0xab47bc, 0x024, 0x228eb149, "Purple400" },
	{ 0x9c27b0, 0x025, 0x1c8c6940, "Purple500" },
	{ 0x8e24aa, 0x026, 0xae940b3b, "Purple600" },
	{ 0x7b1fa2, 0x027, 0xa891c332, "Purple700" },
	{ 0x6a1b9a, 0x028, 0x2aacd315, "Purple800" },
	{ 0x4a148c, 0x029, 0x24aa8b2c, "Purple900" },
	{ 0xea80fc, 0x02a, 0x5d994875, "PurpleA100" },
	{ 0xe040fb, 0x02b, 0xe39c59fe, "PurpleA200" },
	{ 0xd500f9, 0x02c, 0xdf8d48c8, "PurpleA400" },
	{ 0xaa00ff, 0x02d, 0x5194b803, "PurpleA700" },
	{ 0xede7f6, 0x030, 0x7f6f7c22, "DeepPurple50" },
	{ 0xd1c4e9, 0x031, 0x4676b272, "DeepPurple100" },
	{ 0xb39ddb, 0x032, 0xc073a089, "DeepPurple200" },
	{ 0x9575cd, 0x033, 0xba715880, "DeepPurple300" },
	{ 0x7e57c2, 0x034, 0xc482b1bf, "DeepPurple400" },
	{ 0x673ab7, 0x035, 0xbe8069b6, "DeepPurple500" },
	{ 0x5e35b1, 0x036, 0x587d8a6d, "DeepPurple600" },
	{ 0x512da8, 0x037, 0x527b4264, "DeepPurple700" },
	{ 0x4527a0, 0x038, 0x3c8c68e3, "DeepPurple800" },
	{ 0x311b92, 0x039, 0x568a531a, "DeepPurple900" },
	{ 0xb388ff, 0x03a, 0xdc75aabf, "DeepPurpleA100" },
	{ 0x7c4dff, 0x03b, 0x6a6e3b64, "DeepPurpleA200" },
	{ 0x651fff, 0x03c, 0x5e69ab72, "DeepPurpleA400" },
	{ 0x6200ea, 0x03d, 0xd8669989, "DeepPurpleA700" },
	{ 0xe8eaf6, 0x040, 0xdcf7027c, "Indigo50" },
	{ 0xc5cae9, 0x041, 0xbcceffa0, "Indigo100" },
	{ 0x9fa8da, 0x042, 0xced5d79b, "Indigo200" },
	{ 0x7986cb, 0x043, 0xc8d38f92, "Indigo300" },
	{ 0x5c6bc0, 0x044, 0x5adb310d, "Indigo400" },
	{ 0x3f51b5, 0x045, 0x54d8e904, "Indigo500" },
	{ 0x3949ab, 0x046, 0xc6e0585f, "Indigo600" },
	{ 0x303f9f, 0x047, 0x40dd46d6, "Indigo700" },
	{ 0x283593, 0x048, 0xd2e4e8b1, "Indigo800" },
	{ 0x1a237e, 0x049, 0xcce2a048, "Indigo900" },
	{ 0x8c9eff, 0x04a, 0x847da209, "IndigoA100" },
	{ 0x536dfe, 0x04b, 0x0a80b3f2, "IndigoA200" },
	{ 0x3d5afe, 0x04c, 0x168543e4, "IndigoA400" },
	{ 0x304ffe, 0x04d, 0x888cb33f, "IndigoA700" },
	{ 0xe3f2fd, 0x050, 0xfece83ca, "Blue50" },
	{ 0xbbdefb, 0x051, 0x710fbf8a, "Blue100" },
	{ 0x90caf9, 0x052, 0x6b0d7781, "Blue200" },
	{ 0x64b5f6, 0x053, 0x650b2f98, "Blue300" },
	{ 0x42a5f5, 0x054, 0x6f1c88b7, "Blue400" },
	{ 0x2196f3, 0x055, 0xe91976ce, "Blue500" },
	{ 0x1e88e5, 0x056, 0xe3172ec5, "Blue600" },
	{ 0x1976d2, 0x057, 0xdd14e6dc, "Blue700" },
	{ 0x1565c0, 0x058, 0x66fe66eb, "Blue800" },
	{ 0x0d47a1, 0x059, 0x60fc1ee2, "Blue900" },
	{ 0x82b1ff, 0x05a, 0xa0daaf87, "BlueA100" },
	{ 0x448aff, 0x05b, 0x8ed3d7ac, "BlueA200" },
	{ 0x2979ff, 0x05c, 0x22ceb05a, "BlueA400" },
	{ 0x2962ff, 0x05d, 0x1ccc6851, "BlueA700" },
	{ 0xe1f5fe, 0x060, 0xfd10b1d4, "LightBlue50" },
	{ 0xb3e5fc, 0x061, 0xbd3e3948, "LightBlue100" },
	{ 0x81d4fa, 0x062, 0x2f45a883, "LightBlue200" },
	{ 0x4fc3f7, 0x063, 0xc942c9ba, "LightBlue300" },
	{ 0x29b6f6, 0x064, 0x3b4a38f5, "LightBlue400" },
	{ 0x03a9f4, 0x065, 0x3547f08c, "LightBlue500" },
	{ 0x039be5, 0x066, 0xc74f9267, "LightBlue600" },
	{ 0x0288d1, 0x067, 0xc14d4a7e, "LightBlue700" },
	{ 0x0277bd, 0x068, 0xb32ce0a9, "LightBlue800" },
	{ 0x01579b, 0x069, 0xad2a98a0, "LightBlue900" },
	{ 0x80d8ff, 0x06a, 0x5b962f01, "LightBlueA100" },
	{ 0x40c4ff, 0x06b, 0x6198770a, "LightBlueA200" },
	{ 0x00b0ff, 0x06c, 0xcd9d9e5c, "LightBlueA400" },
	{ 0x0091ea, 0x06d, 0x5fa54037, "LightBlueA700" },
	{ 0xe0f7fa, 0x070, 0xecdfdf13, "Cyan50" },
	{ 0xb2ebf2, 0x071, 0x8f7614bd, "Cyan100" },
	{ 0x80deea, 0x072, 0xf578f386, "Cyan200" },
	{ 0x4dd0e1, 0x073, 0xfb7b3b8f, "Cyan300" },
	{ 0x26c6da, 0x074, 0xf169e2d0, "Cyan400" },
	{ 0x00bcd4, 0x075, 0xf76c2ad9, "Cyan500" },
	{ 0x00acc1, 0x076, 0xfd6e72c2, "Cyan600" },
	{ 0x0097a7, 0x077, 0x8371844b, "Cyan700" },
	{ 0x00838f, 0x078, 0x79873b3c, "Cyan800" },
	{ 0x006064, 0x079, 0xff8a4ca5, "Cyan900" },
	{ 0x84ffff, 0x07a, 0xdc9a481e, "CyanA100" },
	{ 0x18ffff, 0x07b, 0xd6980015, "CyanA200" },
	{ 0x00e5ff, 0x07c, 0x4a92a6a3, "CyanA400" },
	{ 0x00b8d4, 0x07d, 0xd88b3768, "CyanA700" },
	{ 0xe0f2f1, 0x080, 0x257885fc, "Teal50" },
	{ 0xb2dfdb, 0x081, 0x60b10220, "Teal100" },
	{ 0x80cbc4, 0x082, 0x72b7da1b, "Teal200" },
	{ 0x4db6ac, 0x083, 0x6cb59212, "Teal300" },
	{ 0x26a69a, 0x084, 0xfebd338d, "Teal400" },
	{ 0x009688, 0x085, 0xf8baeb84, "Teal500" },
	{ 0x00897b, 0x086, 0x6ac25adf, "Teal600" },
	{ 0x00796b, 0x087, 0xe4bf4956, "Teal700" },
	{ 0x00695c, 0x088, 0x76c6eb31, "Teal800" },
	{ 0x004d40, 0x089, 0x70c4a2c8, "Teal900" },
	{ 0xa7ffeb, 0x08a, 0x01479189, "TealA100" },
	{ 0x64ffda, 0x08b, 0x874aa372, "TealA200" },
	{ 0x1de9b6, 0x08c, 0x934f3364, "TealA400" },
	{ 0x00bfa5, 0x08d, 0x0556a2bf, "TealA700" },
	{ 0xe8f5e9, 0x090, 0x8209d369, "Green50" },
	{ 0xc8e6c9, 0x091, 0x968185bf, "Green100" },
	{ 0xa5d6a7, 0x092, 0x247a1664, "Green200" },
	{ 0x81c784, 0x093, 0x2a7c5e6d, "Green300" },
	{ 0x66bb6a, 0x094, 0x18758672, "Green400" },
	{ 0x4caf50, 0x095, 0x1e77ce7b, "Green500" },
	{ 0x43a047, 0x096, 0x8c702c80, "Green600" },
	{ 0x388e3c, 0x097, 0x92727489, "Green700" },
	{ 0x2e7d32, 0x098, 0xa092de5e, "Green800" },
	{ 0x1b5e20, 0x099, 0xa6952647, "Green900" },
	{ 0xb9f6ca, 0x09a, 0x1e7bbc84, "GreenA100" },
	{ 0x69f0ae, 0x09b, 0x90832bdf, "GreenA200" },
	{ 0x00e676, 0x09c, 0x8c741b29, "GreenA400" },
	{ 0x00c853, 0x09d, 0x92766312, "GreenA700" },
	{ 0xf1f8e9, 0x0a0, 0x1b4f6967, "LightGreen50" },
	{ 0xdcedc8, 0x0a1, 0xecf935d1, "LightGreen100" },
	{ 0xc5e1a5, 0x0a2, 0xf2fb7dda, "LightGreen200" },
	{ 0xaed581, 0x0a3, 0xd8fd93a3, "LightGreen300" },
	{ 0x9ccc65, 0x0a4, 0x5f00a52c, "LightGreen400" },
	{ 0x8bc34a, 0x0a5, 0x6502ed15, "LightGreen500" },
	{ 0x7cb342, 0x0a6, 0x6b05351e, "LightGreen600" },
	{ 0x689f38, 0x0a7, 0x71077d07, "LightGreen700" },
	{ 0x558b2f, 0x0a8, 0x56e28340, "LightGreen800" },
	{ 0x33691e, 0x0a9, 0x5ce4cb49, "LightGreen900" },
	{ 0xccff90, 0x0aa, 0xe23fc0da, "LightGreenA100" },
	{ 0xb2ff59, 0x0ab, 0xdc3d78d1, "LightGreenA200" },
	{ 0x76ff03, 0x0ac, 0x604bc007, "LightGreenA400" },
	{ 0x64dd17, 0x0ad, 0x4e44e82c, "LightGreenA700" },
	{ 0xf9fbe7, 0x0b0, 0xb0f07589, "Lime50" },
	{ 0xf0f4c3, 0x0b1, 0x0b93875f, "Lime100" },
	{ 0xe6ee9c, 0x0b2, 0x998c1804, "Lime200" },
	{ 0xdce775, 0x0b3, 0x9f8e600d, "Lime300" },
	{ 0xd4e157, 0x0b4, 0x0d86be92, "Lime400" },
	{ 0xcddc39, 0x0b5, 0x1389069b, "Lime500" },
	{ 0xc0ca33, 0x0b6, 0x01822ea0, "Lime600" },
	{ 0xafb42b, 0x0b7, 0x078476a9, "Lime700" },
	{ 0x9e9d24, 0x0b8, 0x15a4e07e, "Lime800" },
	{ 0x827717, 0x0b9, 0x1ba72867, "Lime900" },
	{ 0xf4ff81, 0x0ba, 0x36dac3e4, "LimeA100" },
	{ 0xeeff41, 0x0bb, 0xa8e2333f, "LimeA200" },
	{ 0xc6ff00, 0x0bc, 0xa4d32209, "LimeA400" },
	{ 0xaeea00, 0x0bd, 0x2ad633f2, "LimeA700" },
	{ 0xfffde7, 0x0c0, 0xea0669fa, "Yellow50" },
	{ 0xfff9c4, 0x0c1, 0x6a0e53fa, "Yellow100" },
	{ 0xfff59d, 0x0c2, 0x640c0bf1, "Yellow200" },
	{ 0xfff176, 0x0c3, 0x5e09c388, "Yellow300" },
	{ 0xffee58, 0x0c4, 0x681b1ca7, "Yellow400" },
	{ 0xffeb3b, 0x0c5, 0x6218d4be, "Yellow500" },
	{ 0xfdd835, 0x0c6, 0xdc15c335, "Yellow600" },
	{ 0xfbc02d, 0x0c7, 0xd6137acc, "Yellow700" },
	{ 0xf9a825, 0x0c8, 0x5ffcfadb, "Yellow800" },
	{ 0xf57f17, 0x0c9, 0x59fab2d2, "Yellow900" },
	{ 0xffff8d, 0x0ca, 0x2712fad7, "YellowA100" },
	{ 0xffff00, 0x0cb, 0x950b597c, "YellowA200" },
	{ 0xffea00, 0x0cc, 0xa906fbaa, "YellowA400" },
	{ 0xffd600, 0x0cd, 0x2303ea21, "YellowA700" },
	{ 0xfff8e1, 0x0d0, 0x7aa9e319, "Amber50" },
	{ 0xffecb3, 0x0d1, 0xaa7a37af, "Amber100" },
	{ 0xffe082, 0x0d2, 0x3872c854, "Amber200" },
	{ 0xffd54f, 0x0d3, 0xbe75d9dd, "Amber300" },
	{ 0xffca28, 0x0d4, 0x2c6e3862, "Amber400" },
	{ 0xffc107, 0x0d5, 0x3270806b, "Amber500" },
	{ 0xffb300, 0x0d6, 0xa068def0, "Amber600" },
	{ 0xffa000, 0x0d7, 0x266bf079, "Amber700" },
	{ 0xff8f00, 0x0d8, 0xb48b904e, "Amber800" },
	{ 0xff6f00, 0x0d9, 0x3a8ea237, "Amber900" },
	{ 0xffe57f, 0x0da, 0x9c707854, "AmberA100" },
	{ 0xffd740, 0x0db, 0x0e77e7af, "AmberA200" },
	{ 0xffc400, 0x0dc, 0x8a69a079, "AmberA400" },
	{ 0xffab00, 0x0dd, 0x906be862, "AmberA700" },
	{ 0xfff3e0, 0x0e0, 0xe9d03320, "Orange50" },
	{ 0xffe0b2, 0x0e1, 0xcaca6534, "Orange100" },
	{ 0xffcc80, 0x0e2, 0x3cd1d40f, "Orange200" },
	{ 0xffb74d, 0x0e3, 0x36cf8c06, "Orange300" },
	{ 0xffa726, 0x0e4, 0x38c2c359, "Orange400" },
	{ 0xff9800, 0x0e5, 0x32c07b50, "Orange500" },
	{ 0xfb8c00, 0x0e6, 0xc4c81ccb, "Orange600" },
	{ 0xf57c00, 0x0e7, 0x3ec50b42, "Orange700" },
	{ 0xef6c00, 0x0e8, 0x40e0e525, "Orange800" },
	{ 0xe65100, 0x0e9, 0xbaddd3bc, "Orange900" },
	{ 0xffd180, 0x0ea, 0x455a4d25, "OrangeA100" },
	{ 0xffab40, 0x0eb, 0x4b5c952e, "OrangeA200" },
	{ 0xff9100, 0x0ec, 0x474d8478, "OrangeA400" },
	{ 0xff6d00, 0x0ed, 0xb954f3b3, "OrangeA700" },
	{ 0xfbe9e7, 0x0f0, 0x60eb4716, "DeepOrange50" },
	{ 0xffccbc, 0x0f1, 0x206a9ed6, "DeepOrange100" },
	{ 0xffab91, 0x0f2, 0x3a68890d, "DeepOrange200" },
	{ 0xff8a65, 0x0f3, 0x34664104, "DeepOrange300" },
	{ 0xff7043, 0x0f4, 0xae632f9b, "DeepOrange400" },
	{ 0xff5722, 0x0f5, 0xa860e792, "DeepOrange500" },
	{ 0xf4511e, 0x0f6, 0xa25e9fa9, "DeepOrange600" },
	{ 0xe64a19, 0x0f7, 0x9c5c57a0, "DeepOrange700" },
	{ 0xd84315, 0x0f8, 0xb6815167, "DeepOrange800" },
	{ 0xbf360c, 0x0f9, 0xb07f097e, "DeepOrange900" },
	{ 0xff9e80, 0x0fa, 0x09b7420b, "DeepOrangeA100" },
	{ 0xff6e40, 0x0fb, 0x77afa090, "DeepOrangeA200" },
	{ 0xff3d00, 0x0fc, 0x7bbeb146, "DeepOrangeA400" },
	{ 0xdd2c00, 0x0fd, 0x15bbd27d, "DeepOrangeA700" },
	{ 0xefebe9, 0x100, 0x79194eec, "Brown50" },
	{ 0xd7ccc8, 0x101, 0xf6cd53d0, "Brown100" },
	{ 0xbcaaa4, 0x102, 0x88d4f54b, "Brown200" },
	{ 0xa1887f, 0x103, 0x02d1e3c2, "Brown300" },
	{ 0x8d6e63, 0x104, 0x94d985bd, "Brown400" },
	{ 0x795548, 0x105, 0x8ed73db4, "Brown500" },
	{ 0x6d4c41, 0x106, 0x00deac8f, "Brown600" },
	{ 0x5d4037, 0x107, 0xfadc6486, "Brown700" },
	{ 0x4e342e, 0x108, 0x0ce33ce1, "Brown800" },
	{ 0x3e2723, 0x109, 0x06e0f4f8, "Brown900" },
	{ 0xfafafa, 0x110, 0x4635a5e7, "Grey50" },
	{ 0xf5f5f5, 0x111, 0xf56a7351, "Grey100" },
	{ 0xeeeeee, 0x112, 0xfb6cbb5a, "Grey200" },
	{ 0xe0e0e0, 0x113, 0xe16ed123, "Grey300" },
	{ 0xbdbdbd, 0x114, 0x6771e2ac, "Grey400" },
	{ 0x9e9e9e, 0x115, 0x6d742a95, "Grey500" },
	{ 0x757575, 0x116, 0x7376729e, "Grey600" },
	{ 0x616161, 0x117, 0x7978ba87, "Grey700" },
	{ 0x424242, 0x118, 0x5f53c0c0, "Grey800" },
	{ 0x212121, 0x119, 0x655608c9, "Grey900" },
	{ 0xeceff1, 0x120, 0x669ef003, "BlueGrey50" },
	{ 0xcfd8dc, 0x121, 0x273dbe6d, "BlueGrey100" },
	{ 0xb0bec5, 0x122, 0x8d409db6, "BlueGrey200" },
	{ 0x90a4ae, 0x123, 0x9342e5bf, "BlueGrey300" },
	{ 0x78909c, 0x124, 0x89318c80, "BlueGrey400" },
	{ 0x607d8b, 0x125, 0x8f33d489, "BlueGrey500" },
	{ 0x546e7a, 0x126, 0x1536e672, "BlueGrey600" },
	{ 0x455a64, 0x127, 0x1b392e7b, "BlueGrey700" },
	{ 0x37474f, 0x128, 0x914fae6c, "BlueGrey800" },
	{ 0x263238, 0x129, 0x9751f655, "BlueGrey900" },
	{ 0xffffff, 0x130, 0x401e7178, "White" },
	{ 0x000000, 0x131, 0x5e8e7be2, "Black" },
};

static struct ColorIndexValue {
	uint32_t value;
	uint8_t idx;
} valueIndex[256] = {
	{ 0x000000, 0xff }, { 0x004d40, 0x79 }, { 0x006064, 0x6b }, { 0x00695c, 0x78 }, { 0x00796b, 0x77 }, { 0x00838f, 0x6a }, { 0x00897b, 0x76 }, { 0x0091ea, 0x61 },
	{ 0x009688, 0x75 }, { 0x0097a7, 0x69 }, { 0x00acc1, 0x68 }, { 0x00b0ff, 0x60 }, { 0x00b8d4, 0x6f }, { 0x00bcd4, 0x67 }, { 0x00bfa5, 0x7d }, { 0x00c853, 0x8b },
	{ 0x00e5ff, 0x6e }, { 0x00e676, 0x8a }, { 0x01579b, 0x5d }, { 0x0277bd, 0x5c }, { 0x0288d1, 0x5b }, { 0x039be5, 0x5a }, { 0x03a9f4, 0x59 }, { 0x0d47a1, 0x4f },
	{ 0x1565c0, 0x4e }, { 0x18ffff, 0x6d }, { 0x1976d2, 0x4d }, { 0x1a237e, 0x41 }, { 0x1b5e20, 0x87 }, { 0x1de9b6, 0x7c }, { 0x1e88e5, 0x4c }, { 0x212121, 0xf3 },
	{ 0x2196f3, 0x4b }, { 0x263238, 0xfd }, { 0x26a69a, 0x74 }, { 0x26c6da, 0x66 }, { 0x283593, 0x40 }, { 0x2962ff, 0x53 }, { 0x2979ff, 0x52 }, { 0x29b6f6, 0x58 },
	{ 0x2e7d32, 0x86 }, { 0x303f9f, 0x3f }, { 0x304ffe, 0x45 }, { 0x311b92, 0x33 }, { 0x33691e, 0x95 }, { 0x37474f, 0xfc }, { 0x388e3c, 0x85 }, { 0x3949ab, 0x3e },
	{ 0x3d5afe, 0x44 }, { 0x3e2723, 0xe9 }, { 0x3f51b5, 0x3d }, { 0x40c4ff, 0x5f }, { 0x424242, 0xf2 }, { 0x42a5f5, 0x4a }, { 0x43a047, 0x84 }, { 0x448aff, 0x51 },
	{ 0x4527a0, 0x32 }, { 0x455a64, 0xfb }, { 0x4a148c, 0x25 }, { 0x4caf50, 0x83 }, { 0x4db6ac, 0x73 }, { 0x4dd0e1, 0x65 }, { 0x4e342e, 0xe8 }, { 0x4fc3f7, 0x57 },
	{ 0x512da8, 0x31 }, { 0x536dfe, 0x43 }, { 0x546e7a, 0xfa }, { 0x558b2f, 0x94 }, { 0x5c6bc0, 0x3c }, { 0x5d4037, 0xe7 }, { 0x5e35b1, 0x30 }, { 0x607d8b, 0xf9 },
	{ 0x616161, 0xf1 }, { 0x6200ea, 0x37 }, { 0x64b5f6, 0x49 }, { 0x64dd17, 0x99 }, { 0x64ffda, 0x7b }, { 0x651fff, 0x36 }, { 0x66bb6a, 0x82 }, { 0x673ab7, 0x2f },
	{ 0x689f38, 0x93 }, { 0x69f0ae, 0x89 }, { 0x6a1b9a, 0x24 }, { 0x6d4c41, 0xe6 }, { 0x757575, 0xf0 }, { 0x76ff03, 0x98 }, { 0x78909c, 0xf8 }, { 0x795548, 0xe5 },
	{ 0x7986cb, 0x3b }, { 0x7b1fa2, 0x23 }, { 0x7c4dff, 0x35 }, { 0x7cb342, 0x92 }, { 0x7e57c2, 0x2e }, { 0x80cbc4, 0x72 }, { 0x80d8ff, 0x5e }, { 0x80deea, 0x64 },
	{ 0x81c784, 0x81 }, { 0x81d4fa, 0x56 }, { 0x827717, 0xa3 }, { 0x82b1ff, 0x50 }, { 0x84ffff, 0x6c }, { 0x880e4f, 0x17 }, { 0x8bc34a, 0x91 }, { 0x8c9eff, 0x42 },
	{ 0x8d6e63, 0xe4 }, { 0x8e24aa, 0x22 }, { 0x90a4ae, 0xf7 }, { 0x90caf9, 0x48 }, { 0x9575cd, 0x2d }, { 0x9c27b0, 0x21 }, { 0x9ccc65, 0x90 }, { 0x9e9d24, 0xa2 },
	{ 0x9e9e9e, 0xef }, { 0x9fa8da, 0x3a }, { 0xa1887f, 0xe3 }, { 0xa5d6a7, 0x80 }, { 0xa7ffeb, 0x7a }, { 0xaa00ff, 0x29 }, { 0xab47bc, 0x20 }, { 0xad1457, 0x16 },
	{ 0xaed581, 0x8f }, { 0xaeea00, 0xa7 }, { 0xafb42b, 0xa1 }, { 0xb0bec5, 0xf6 }, { 0xb2dfdb, 0x71 }, { 0xb2ebf2, 0x63 }, { 0xb2ff59, 0x97 }, { 0xb388ff, 0x34 },
	{ 0xb39ddb, 0x2c }, { 0xb3e5fc, 0x55 }, { 0xb71c1c, 0x09 }, { 0xb9f6ca, 0x88 }, { 0xba68c8, 0x1f }, { 0xbbdefb, 0x47 }, { 0xbcaaa4, 0xe2 }, { 0xbdbdbd, 0xee },
	{ 0xbf360c, 0xdb }, { 0xc0ca33, 0xa0 }, { 0xc2185b, 0x15 }, { 0xc51162, 0x1b }, { 0xc5cae9, 0x39 }, { 0xc5e1a5, 0x8e }, { 0xc62828, 0x08 }, { 0xc6ff00, 0xa6 },
	{ 0xc8e6c9, 0x7f }, { 0xccff90, 0x96 }, { 0xcddc39, 0x9f }, { 0xce93d8, 0x1e }, { 0xcfd8dc, 0xf5 }, { 0xd1c4e9, 0x2b }, { 0xd32f2f, 0x07 }, { 0xd4e157, 0x9e },
	{ 0xd50000, 0x0d }, { 0xd500f9, 0x28 }, { 0xd7ccc8, 0xe1 }, { 0xd81b60, 0x14 }, { 0xd84315, 0xda }, { 0xdce775, 0x9d }, { 0xdcedc8, 0x8d }, { 0xdd2c00, 0xdf },
	{ 0xe040fb, 0x27 }, { 0xe0e0e0, 0xed }, { 0xe0f2f1, 0x70 }, { 0xe0f7fa, 0x62 }, { 0xe1bee7, 0x1d }, { 0xe1f5fe, 0x54 }, { 0xe3f2fd, 0x46 }, { 0xe53935, 0x06 },
	{ 0xe57373, 0x03 }, { 0xe64a19, 0xd9 }, { 0xe65100, 0xcd }, { 0xe6ee9c, 0x9c }, { 0xe8eaf6, 0x38 }, { 0xe8f5e9, 0x7e }, { 0xe91e63, 0x13 }, { 0xea80fc, 0x26 },
	{ 0xec407a, 0x12 }, { 0xeceff1, 0xf4 }, { 0xede7f6, 0x2a }, { 0xeeeeee, 0xec }, { 0xeeff41, 0xa5 }, { 0xef5350, 0x04 }, { 0xef6c00, 0xcc }, { 0xef9a9a, 0x02 },
	{ 0xefebe9, 0xe0 }, { 0xf06292, 0x11 }, { 0xf0f4c3, 0x9b }, { 0xf1f8e9, 0x8c }, { 0xf3e5f5, 0x1c }, { 0xf44336, 0x05 }, { 0xf4511e, 0xd8 }, { 0xf48fb1, 0x10 },
	{ 0xf4ff81, 0xa4 }, { 0xf50057, 0x1a }, { 0xf57c00, 0xcb }, { 0xf57f17, 0xb1 }, { 0xf5f5f5, 0xeb }, { 0xf8bbd0, 0x0f }, { 0xf9a825, 0xb0 }, { 0xf9fbe7, 0x9a },
	{ 0xfafafa, 0xea }, { 0xfb8c00, 0xca }, { 0xfbc02d, 0xaf }, { 0xfbe9e7, 0xd2 }, { 0xfce4ec, 0x0e }, { 0xfdd835, 0xae }, { 0xff1744, 0x0c }, { 0xff3d00, 0xde },
	{ 0xff4081, 0x19 }, { 0xff5252, 0x0b }, { 0xff5722, 0xd7 }, { 0xff6d00, 0xd1 }, { 0xff6e40, 0xdd }, { 0xff6f00, 0xbf }, { 0xff7043, 0xd6 }, { 0xff80ab, 0x18 },
	{ 0xff8a65, 0xd5 }, { 0xff8a80, 0x0a }, { 0xff8f00, 0xbe }, { 0xff9100, 0xd0 }, { 0xff9800, 0xc9 }, { 0xff9e80, 0xdc }, { 0xffa000, 0xbd }, { 0xffa726, 0xc8 },
	{ 0xffab00, 0xc3 }, { 0xffab40, 0xcf }, { 0xffab91, 0xd4 }, { 0xffb300, 0xbc }, { 0xffb74d, 0xc7 }, { 0xffc107, 0xbb }, { 0xffc400, 0xc2 }, { 0xffca28, 0xba },
	{ 0xffcc80, 0xc6 }, { 0xffccbc, 0xd3 }, { 0xffcdd2, 0x01 }, { 0xffd180, 0xce }, { 0xffd54f, 0xb9 }, { 0xffd600, 0xb5 }, { 0xffd740, 0xc1 }, { 0xffe082, 0xb8 },
	{ 0xffe0b2, 0xc5 }, { 0xffe57f, 0xc0 }, { 0xffea00, 0xb4 }, { 0xffeb3b, 0xad }, { 0xffebee, 0x00 }, { 0xffecb3, 0xb7 }, { 0xffee58, 0xac }, { 0xfff176, 0xab },
	{ 0xfff3e0, 0xc4 }, { 0xfff59d, 0xaa }, { 0xfff8e1, 0xb6 }, { 0xfff9c4, 0xa9 }, { 0xfffde7, 0xa8 }, { 0xffff00, 0xb3 }, { 0xffff8d, 0xb2 }, { 0xffffff, 0xfe },
};

static struct ColorIndexIndex {
	uint16_t index;
	uint8_t idx;
} indexIndex[256] = {
	{ 0x000, 0x00 }, { 0x001, 0x01 }, { 0x002, 0x02 }, { 0x003, 0x03 }, { 0x004, 0x04 }, { 0x005, 0x05 }, { 0x006, 0x06 }, { 0x007, 0x07 },
	{ 0x008, 0x08 }, { 0x009, 0x09 }, { 0x00a, 0x0a }, { 0x00b, 0x0b }, { 0x00c, 0x0c }, { 0x00d, 0x0d }, { 0x010, 0x0e }, { 0x011, 0x0f },
	{ 0x012, 0x10 }, { 0x013, 0x11 }, { 0x014, 0x12 }, { 0x015, 0x13 }, { 0x016, 0x14 }, { 0x017, 0x15 }, { 0x018, 0x16 }, { 0x019, 0x17 },
	{ 0x01a, 0x18 }, { 0x01b, 0x19 }, { 0x01c, 0x1a }, { 0x01d, 0x1b }, { 0x020, 0x1c }, { 0x021, 0x1d }, { 0x022, 0x1e }, { 0x023, 0x1f },
	{ 0x024, 0x20 }, { 0x025, 0x21 }, { 0x026, 0x22 }, { 0x027, 0x23 }, { 0x028, 0x24 }, { 0x029, 0x25 }, { 0x02a, 0x26 }, { 0x02b, 0x27 },
	{ 0x02c, 0x28 }, { 0x02d, 0x29 }, { 0x030, 0x2a }, { 0x031, 0x2b }, { 0x032, 0x2c }, { 0x033, 0x2d }, { 0x034, 0x2e }, { 0x035, 0x2f },
	{ 0x036, 0x30 }, { 0x037, 0x31 }, { 0x038, 0x32 }, { 0x039, 0x33 }, { 0x03a, 0x34 }, { 0x03b, 0x35 }, { 0x03c, 0x36 }, { 0x03d, 0x37 },
	{ 0x040, 0x38 }, { 0x041, 0x39 }, { 0x042, 0x3a }, { 0x043, 0x3b }, { 0x044, 0x3c }, { 0x045, 0x3d }, { 0x046, 0x3e }, { 0x047, 0x3f },
	{ 0x048, 0x40 }, { 0x049, 0x41 }, { 0x04a, 0x42 }, { 0x04b, 0x43 }, { 0x04c, 0x44 }, { 0x04d, 0x45 }, { 0x050, 0x46 }, { 0x051, 0x47 },
	{ 0x052, 0x48 }, { 0x053, 0x49 }, { 0x054, 0x4a }, { 0x055, 0x4b }, { 0x056, 0x4c }, { 0x057, 0x4d }, { 0x058, 0x4e }, { 0x059, 0x4f },
	{ 0x05a, 0x50 }, { 0x05b, 0x51 }, { 0x05c, 0x52 }, { 0x05d, 0x53 }, { 0x060, 0x54 }, { 0x061, 0x55 }, { 0x062, 0x56 }, { 0x063, 0x57 },
	{ 0x064, 0x58 }, { 0x065, 0x59 }, { 0x066, 0x5a }, { 0x067, 0x5b }, { 0x068, 0x5c }, { 0x069, 0x5d }, { 0x06a, 0x5e }, { 0x06b, 0x5f },
	{ 0x06c, 0x60 }, { 0x06d, 0x61 }, { 0x070, 0x62 }, { 0x071, 0x63 }, { 0x072, 0x64 }, { 0x073, 0x65 }, { 0x074, 0x66 }, { 0x075, 0x67 },
	{ 0x076, 0x68 }, { 0x077, 0x69 }, { 0x078, 0x6a }, { 0x079, 0x6b }, { 0x07a, 0x6c }, { 0x07b, 0x6d }, { 0x07c, 0x6e }, { 0x07d, 0x6f },
	{ 0x080, 0x70 }, { 0x081, 0x71 }, { 0x082, 0x72 }, { 0x083, 0x73 }, { 0x084, 0x74 }, { 0x085, 0x75 }, { 0x086, 0x76 }, { 0x087, 0x77 },
	{ 0x088, 0x78 }, { 0x089, 0x79 }, { 0x08a, 0x7a }, { 0x08b, 0x7b }, { 0x08c, 0x7c }, { 0x08d, 0x7d }, { 0x090, 0x7e }, { 0x091, 0x7f },
	{ 0x092, 0x80 }, { 0x093, 0x81 }, { 0x094, 0x82 }, { 0x095, 0x83 }, { 0x096, 0x84 }, { 0x097, 0x85 }, { 0x098, 0x86 }, { 0x099, 0x87 },
	{ 0x09a, 0x88 }, { 0x09b, 0x89 }, { 0x09c, 0x8a }, { 0x09d, 0x8b }, { 0x0a0, 0x8c }, { 0x0a1, 0x8d }, { 0x0a2, 0x8e }, { 0x0a3, 0x8f },
	{ 0x0a4, 0x90 }, { 0x0a5, 0x91 }, { 0x0a6, 0x92 }, { 0x0a7, 0x93 }, { 0x0a8, 0x94 }, { 0x0a9, 0x95 }, { 0x0aa, 0x96 }, { 0x0ab, 0x97 },
	{ 0x0ac, 0x98 }, { 0x0ad, 0x99 }, { 0x0b0, 0x9a }, { 0x0b1, 0x9b }, { 0x0b2, 0x9c }, { 0x0b3, 0x9d }, { 0x0b4, 0x9e }, { 0x0b5, 0x9f },
	{ 0x0b6, 0xa0 }, { 0x0b7, 0xa1 }, { 0x0b8, 0xa2 }, { 0x0b9, 0xa3 }, { 0x0ba, 0xa4 }, { 0x0bb, 0xa5 }, { 0x0bc, 0xa6 }, { 0x0bd, 0xa7 },
	{ 0x0c0, 0xa8 }, { 0x0c1, 0xa9 }, { 0x0c2, 0xaa }, { 0x0c3, 0xab }, { 0x0c4, 0xac }, { 0x0c5, 0xad }, { 0x0c6, 0xae }, { 0x0c7, 0xaf },
	{ 0x0c8, 0xb0 }, { 0x0c9, 0xb1 }, { 0x0ca, 0xb2 }, { 0x0cb, 0xb3 }, { 0x0cc, 0xb4 }, { 0x0cd, 0xb5 }, { 0x0d0, 0xb6 }, { 0x0d1, 0xb7 },
	{ 0x0d2, 0xb8 }, { 0x0d3, 0xb9 }, { 0x0d4, 0xba }, { 0x0d5, 0xbb }, { 0x0d6, 0xbc }, { 0x0d7, 0xbd }, { 0x0d8, 0xbe }, { 0x0d9, 0xbf },
	{ 0x0da, 0xc0 }, { 0x0db, 0xc1 }, { 0x0dc, 0xc2 }, { 0x0dd, 0xc3 }, { 0x0e0, 0xc4 }, { 0x0e1, 0xc5 }, { 0x0e2, 0xc6 }, { 0x0e3, 0xc7 },
	{ 0x0e4, 0xc8 }, { 0x0e5, 0xc9 }, { 0x0e6, 0xca }, { 0x0e7, 0xcb }, { 0x0e8, 0xcc }, { 0x0e9, 0xcd }, { 0x0ea, 0xce }, { 0x0eb, 0xcf },
	{ 0x0ec, 0xd0 }, { 0x0ed, 0xd1 }, { 0x0f0, 0xd2 }, { 0x0f1, 0xd3 }, { 0x0f2, 0xd4 }, { 0x0f3, 0xd5 }, { 0x0f4, 0xd6 }, { 0x0f5, 0xd7 },
	{ 0x0f6, 0xd8 }, { 0x0f7, 0xd9 }, { 0x0f8, 0xda }, { 0x0f9, 0xdb }, { 0x0fa, 0xdc }, { 0x0fb, 0xdd }, { 0x0fc, 0xde }, { 0x0fd, 0xdf },
	{ 0x100, 0xe0 }, { 0x101, 0xe1 }, { 0x102, 0xe2 }, { 0x103, 0xe3 }, { 0x104, 0xe4 }, { 0x105, 0xe5 }, { 0x106, 0xe6 }, { 0x107, 0xe7 },
	{ 0x108, 0xe8 }, { 0x109, 0xe9 }, { 0x110, 0xea }, { 0x111, 0xeb }, { 0x112, 0xec }, { 0x113, 0xed }, { 0x114, 0xee }, { 0x115, 0xef },
	{ 0x116, 0xf0 }, { 0x117, 0xf1 }, { 0x118, 0xf2 }, { 0x119, 0xf3 }, { 0x120, 0xf4 }, { 0x121, 0xf5 }, { 0x122, 0xf6 }, { 0x123, 0xf7 },
	{ 0x124, 0xf8 }, { 0x125, 0xf9 }, { 0x126, 0xfa }, { 0x127, 0xfb }, { 0x128, 0xfc }, { 0x129, 0xfd }, { 0x130, 0xfe }, { 0x131, 0xff },
};

static struct ColorIndexName {
	uint32_t hash;
	uint8_t idx;
} nameIndex[256] = {
	{ 0x00deac8f, 0xe6 }, { 0x01479189, 0x7a }, { 0x01822ea0, 0xa0 }, { 0x028ea97f, 0x0b }, { 0x02d1e3c2, 0xe3 }, { 0x049de544, 0x02 }, { 0x054596d0, 0x1c }, { 0x0556a2bf, 0x7d },
	{ 0x06e0f4f8, 0xe9 }, { 0x078476a9, 0xa1 }, { 0x09b7420b, 0xdc }, { 0x0a80b3f2, 0x43 }, { 0x0aa02d4d, 0x03 }, { 0x0b93875f, 0x9b }, { 0x0ce33ce1, 0xe8 }, { 0x0d86be92, 0x9e },
	{ 0x0e77e7af, 0xc1 }, { 0x1389069b, 0x9f }, { 0x1536e672, 0xfa }, { 0x15a4e07e, 0xa2 }, { 0x15bbd27d, 0xdf }, { 0x168543e4, 0x44 }, { 0x18758672, 0x82 }, { 0x19282970, 0x15 },
	{ 0x1b392e7b, 0xfb }, { 0x1b4f6967, 0x8c }, { 0x1ba72867, 0xa3 }, { 0x1c8c6940, 0x21 }, { 0x1ccc6851, 0x53 }, { 0x1d373a26, 0x0f }, { 0x1e77ce7b, 0x83 }, { 0x1e7bbc84, 0x88 },
	{ 0x206a9ed6, 0xd3 }, { 0x209b7a76, 0x1f }, { 0x228eb149, 0x20 }, { 0x22ceb05a, 0x52 }, { 0x2303ea21, 0xb5 }, { 0x247a1664, 0x80 }, { 0x24aa8b2c, 0x25 }, { 0x257885fc, 0x70 },
	{ 0x266bf079, 0xbd }, { 0x269dc27f, 0x1e }, { 0x2712fad7, 0xb2 }, { 0x273dbe6d, 0xf5 }, { 0x2a7c5e6d, 0x81 }, { 0x2aacd315, 0x24 }, { 0x2ad633f2, 0xa7 }, { 0x2c6e3862, 0xba },
	{ 0x2d4adace, 0x17 }, { 0x2f45a883, 0x56 }, { 0x3270806b, 0xbb }, { 0x32c07b50, 0xc9 }, { 0x34664104, 0xd5 }, { 0x3547f08c, 0x59 }, { 0x36cf8c06, 0xc7 }, { 0x36dac3e4, 0xa4 },
	{ 0x3735245d, 0x10 }, { 0x3872c854, 0xb8 }, { 0x38c2c359, 0xc8 }, { 0x3a68890d, 0xd4 }, { 0x3a8ea237, 0xbf }, { 0x3b4a38f5, 0x58 }, { 0x3c8c68e3, 0x32 }, { 0x3cd1d40f, 0xc6 },
	{ 0x3ec50b42, 0xcb }, { 0x3f00b37b, 0x18 }, { 0x401e7178, 0xfe }, { 0x40dd46d6, 0x3f }, { 0x40e0e525, 0xcc }, { 0x455a4d25, 0xce }, { 0x4635a5e7, 0xea }, { 0x4676b272, 0x2b },
	{ 0x474d8478, 0xd0 }, { 0x4a92a6a3, 0x6e }, { 0x4b05436d, 0x1b }, { 0x4b45fd26, 0x0e }, { 0x4b5c952e, 0xcf }, { 0x4e44e82c, 0x99 }, { 0x5194b803, 0x29 }, { 0x527b4264, 0x31 },
	{ 0x54d8e904, 0x3d }, { 0x568a531a, 0x33 }, { 0x56e28340, 0x94 }, { 0x587d8a6d, 0x30 }, { 0x59fab2d2, 0xb1 }, { 0x5adb310d, 0x3c }, { 0x5b962f01, 0x5e }, { 0x5ce4cb49, 0x95 },
	{ 0x5d994875, 0x26 }, { 0x5e09c388, 0xab }, { 0x5e69ab72, 0x36 }, { 0x5e8e7be2, 0xff }, { 0x5f00a52c, 0x90 }, { 0x5f53c0c0, 0xf2 }, { 0x5fa54037, 0x61 }, { 0x5ffcfadb, 0xb0 },
	{ 0x604bc007, 0x98 }, { 0x60b10220, 0x71 }, { 0x60eb4716, 0xd2 }, { 0x60fc1ee2, 0x4f }, { 0x6198770a, 0x5f }, { 0x6218d4be, 0xad }, { 0x640c0bf1, 0xaa }, { 0x6502ed15, 0x91 },
	{ 0x650b2f98, 0x49 }, { 0x655608c9, 0xf3 }, { 0x669ef003, 0xf4 }, { 0x66fe66eb, 0x4e }, { 0x6771e2ac, 0xee }, { 0x681b1ca7, 0xac }, { 0x6a0e53fa, 0xa9 }, { 0x6a6e3b64, 0x35 },
	{ 0x6ac25adf, 0x76 }, { 0x6b05351e, 0x92 }, { 0x6b0d7781, 0x48 }, { 0x6c93fbe0, 0x06 }, { 0x6cb59212, 0x73 }, { 0x6d742a95, 0xef }, { 0x6f1c88b7, 0x4a }, { 0x70c4a2c8, 0x79 },
	{ 0x71077d07, 0x93 }, { 0x710fbf8a, 0x47 }, { 0x729643e9, 0x07 }, { 0x72b7da1b, 0x72 }, { 0x7376729e, 0xf0 }, { 0x76a5549f, 0x01 }, { 0x76c6eb31, 0x78 }, { 0x77afa090, 0xdd },
	{ 0x78988bd2, 0x04 }, { 0x79194eec, 0xe0 }, { 0x7978ba87, 0xf1 }, { 0x79873b3c, 0x6a }, { 0x7aa9e319, 0xb6 }, { 0x7bbeb146, 0xde }, { 0x7e9ad3db, 0x05 }, { 0x7f6f7c22, 0x2a },
	{ 0x80b6adbe, 0x08 }, { 0x8209d369, 0x7e }, { 0x8371844b, 0x69 }, { 0x847da209, 0x42 }, { 0x8482aa32, 0x0d }, { 0x86b8f5a7, 0x09 }, { 0x874aa372, 0x7b }, { 0x888cb33f, 0x45 },
	{ 0x88d4f54b, 0xe2 }, { 0x89318c80, 0xf8 }, { 0x8a69a079, 0xc2 }, { 0x8c702c80, 0x84 }, { 0x8c741b29, 0x8a }, { 0x8d409db6, 0xf6 }, { 0x8ed3d7ac, 0x51 }, { 0x8ed73db4, 0xe5 },
	{ 0x8f33d489, 0xf9 }, { 0x8f7614bd, 0x63 }, { 0x906be862, 0xc3 }, { 0x90832bdf, 0x89 }, { 0x90873a24, 0x0a }, { 0x914fae6c, 0xfc }, { 0x923cbb49, 0x00 }, { 0x92727489, 0x85 },
	{ 0x92766312, 0x8b }, { 0x9342e5bf, 0xf7 }, { 0x934f3364, 0x7c }, { 0x94d985bd, 0xe4 }, { 0x950b597c, 0xb3 }, { 0x968185bf, 0x7f }, { 0x9751f655, 0xfd }, { 0x998c1804, 0x9c },
	{ 0x9c5c57a0, 0xd9 }, { 0x9c707854, 0xc0 }, { 0x9f2b3af9, 0x14 }, { 0x9f8e600d, 0x9d }, { 0xa068def0, 0xbc }, { 0xa092de5e, 0x86 }, { 0xa0daaf87, 0x50 }, { 0xa25e9fa9, 0xd8 },
	{ 0xa4d32209, 0xa6 }, { 0xa52d82e2, 0x13 }, { 0xa6952647, 0x87 }, { 0xa860e792, 0xd7 }, { 0xa891c332, 0x23 }, { 0xa8e2333f, 0xa5 }, { 0xa906fbaa, 0xb4 }, { 0xaa7a37af, 0xb7 },
	{ 0xab2fcaeb, 0x12 }, { 0xacf91180, 0x19 }, { 0xad2a98a0, 0x5d }, { 0xae632f9b, 0xd6 }, { 0xae940b3b, 0x22 }, { 0xb07f097e, 0xdb }, { 0xb0f07589, 0x9a }, { 0xb10822b6, 0x1a },
	{ 0xb13212d4, 0x11 }, { 0xb32ce0a9, 0x5c }, { 0xb34decb7, 0x16 }, { 0xb48b904e, 0xbe }, { 0xb4965324, 0x1d }, { 0xb6815167, 0xda }, { 0xb954f3b3, 0xd1 }, { 0xba715880, 0x2d },
	{ 0xbaddd3bc, 0xcd }, { 0xbcceffa0, 0x39 }, { 0xbd3e3948, 0x55 }, { 0xbe75d9dd, 0xb9 }, { 0xbe8069b6, 0x2f }, { 0xc073a089, 0x2c }, { 0xc14d4a7e, 0x5b }, { 0xc482b1bf, 0x2e },
	{ 0xc4c81ccb, 0xca }, { 0xc6e0585f, 0x3e }, { 0xc74f9267, 0x5a }, { 0xc8d38f92, 0x3b }, { 0xc942c9ba, 0x57 }, { 0xcaca6534, 0xc5 }, { 0xcce2a048, 0x41 }, { 0xcd9d9e5c, 0x60 },
	{ 0xced5d79b, 0x3a }, { 0xd2e4e8b1, 0x40 }, { 0xd6137acc, 0xaf }, { 0xd6980015, 0x6d }, { 0xd8669989, 0x37 }, { 0xd88b3768, 0x6f }, { 0xd8fd93a3, 0x8f }, { 0xdc15c335, 0xae },
	{ 0xdc3d78d1, 0x97 }, { 0xdc75aabf, 0x34 }, { 0xdc9a481e, 0x6c }, { 0xdcf7027c, 0x38 }, { 0xdd14e6dc, 0x4d }, { 0xdf8d48c8, 0x28 }, { 0xe16ed123, 0xed }, { 0xe23fc0da, 0x96 },
	{ 0xe3172ec5, 0x4c }, { 0xe39c59fe, 0x27 }, { 0xe4bf4956, 0x77 }, { 0xe91976ce, 0x4b }, { 0xe9d03320, 0xc4 }, { 0xea0669fa, 0xa8 }, { 0xecdfdf13, 0x62 }, { 0xecf935d1, 0x8d },
	{ 0xf169e2d0, 0x66 }, { 0xf2fb7dda, 0x8e }, { 0xf56a7351, 0xeb }, { 0xf578f386, 0x64 }, { 0xf6cd53d0, 0xe1 }, { 0xf76c2ad9, 0x67 }, { 0xf8baeb84, 0x75 }, { 0xfadc6486, 0xe7 },
	{ 0xfb6cbb5a, 0xec }, { 0xfb7b3b8f, 0x65 }, { 0xfd10b1d4, 0x54 }, { 0xfd6e72c2, 0x68 }, { 0xfe7f9849, 0x0c }, { 0xfebd338d, 0x74 }, { 0xfece83ca, 0x46 }, { 0xff8a4ca5, 0x6b },
};

namespace fnv1 {
// see https://en.wikipedia.org/wiki/Fowler%E2%80%93Noll%E2%80%93Vo_hash_function#FNV-1_hash
// parameters from http://www.boost.org/doc/libs/1_38_0/libs/unordered/examples/fnv1.hpp

constexpr uint32_t _fnv_offset_basis() { return uint32_t(2166136261llu); }
constexpr uint32_t _fnv_prime() { return uint32_t(16777619llu); }

constexpr uint32_t _fnv1(const uint8_t* ptr, size_t len) {
	uint32_t hash = _fnv_offset_basis();
	for (size_t i = 0; i < len; i++) {
	     hash *= _fnv_prime();
	     hash ^= ptr[i];
	}
	return hash;
}

constexpr uint32_t _fnv1Signed(const char* ptr, size_t len) {
	uint32_t hash = _fnv_offset_basis();
	for (size_t i = 0; i < len; i++) {
	     hash *= _fnv_prime();
	     if constexpr (std::numeric_limits<char>::is_signed) {
		     if (ptr[i] >= 0) {
			     hash ^= ptr[i];
		     } else {
		    	 hash ^= -ptr[i] + 127;
		     }
	     } else {
		     hash ^= ptr[i];
	     }
	}
	return hash;
}

constexpr uint32_t hash32(const char* str, size_t len) {
    return _fnv1Signed(str, len);
}

}

bool getColor(const StringView &str, uint32_t &color) {
	auto h = fnv1::hash32(str.data(), str.size());

	auto it = std::lower_bound(&nameIndex[0], (&nameIndex[0]) + 256, h, [] (const ColorIndexName &l, const uint32_t &r) -> bool {
		return l.hash < r;
	});

	if (it != (&nameIndex[0]) + 256) {
		uint8_t idx = it->idx;
		auto &d = data[idx];
		if (d.hash == h) {
			color = d.value;
			return true;
		}
	}

	return false;
}

bool getColor(const StringView &str, Color3B &color) {
	uint32_t value = 0;
	if (getColor(str, value)) {
		color.r = (value >> 16) & 0xFF;
		color.g = (value >> 8) & 0xFF;
		color.b = value  & 0xFF;
		return true;
	}
	return false;
}

StringView getName(uint32_t value) {
	auto it = std::lower_bound(&valueIndex[0], (&valueIndex[0]) + 256, value, [] (const ColorIndexValue &l, const uint32_t &r) -> bool {
		return l.value < r;
	});

	if (it != (&valueIndex[0]) + 256) {
		uint8_t idx = it->idx;
		auto &d = data[idx];
		if (d.value == value) {
			return StringView(d.str);
		}
	}

	return StringView();
}

StringView getName(const Color3B &color) {
	return getName((uint32_t)( ((color.r << 16) & 0xFF0000) | ((color.g << 8) & 0xFF00) | (color.b & 0xFF) ));
}

Color getByIndex(uint16_t idx) {
	auto it = std::lower_bound(&indexIndex[0], (&indexIndex[0]) + 256, idx, [] (const ColorIndexIndex &l, const uint16_t &r) -> bool {
		return l.index < r;
	});

	if (it != (&indexIndex[0]) + 256) {
		auto &d = data[it->idx];
		if (d.index == idx) {
			return Color(d.value, d.index);
		}
	}

	return Color();
}

uint16_t getColorIndex(uint32_t value) {
	auto it = std::lower_bound(&valueIndex[0], (&valueIndex[0]) + 256, value, [] (const ColorIndexValue &l, const uint32_t &r) -> bool {
		return l.value < r;
	});

	if (it != (&valueIndex[0]) + 256) {
		uint8_t idx = it->idx;
		auto &d = data[idx];
		if (d.value == value) {
			return d.index;
		}
	}

	return maxOf<uint16_t>();
}

}


static void setHslColor(Color3B &color, float h, float sl, float l) {
	float v;

	h = h / 360.0f;
	sl = sl / 100.0f;
	l = l / 100.0f;

	float r,g,b;
	r = l;   // default to gray
	g = l;
	b = l;

	v = (l <= 0.5) ? (l * (1.0 + sl)) : (l + sl - l * sl);

	if (v > 0) {
		float m;
		float sv;
		int sextant;
		float fract, vsf, mid1, mid2;

		m = l + l - v;
		sv = (v - m ) / v;
		h *= 6.0;
		sextant = (int)h;
		fract = h - sextant;
		vsf = v * sv * fract;
		mid1 = m + vsf;
		mid2 = v - vsf;

		switch (sextant) {
		case 0: r = v; g = mid1; b = m; break;
		case 1: r = mid2; g = v; b = m; break;
		case 2: r = m; g = v; b = mid1; break;
		case 3: r = m; g = mid2; b = v; break;
		case 4: r = mid1; g = m; b = v; break;
		case 5: r = v; g = m; b = mid2; break;
		}
	}

	color.r = (uint8_t)(r * 255.0f);
	color.g = (uint8_t)(g * 255.0f);
	color.b = (uint8_t)(b * 255.0f);
}

static bool readColorDigits(const StringView &origStr, float *b, int num, bool isRgb) {
	StringView str(origStr);
	str.skipChars<StringView::CharGroup<CharGroupId::WhiteSpace>>();
	if (!str.is('(')) {
		return false;
	}
	++ str;

	for (int i = 0; i < num; i++) {
		if (!str.readFloat().grab(b[i])) {
			return false;
		}

		if (b[i] < 0.0f) { b[i] = 0.0f; } // error - less then 0
		if (isRgb) {
			if (b[i] > 255.0f) {
				b[i] = 255.0f; // for RGB absolute values 255 is max
			}
		} else if (!isRgb && (i != 3)) {
			if (i == 0 && b[i] > 359.0f) {
				b[i] = 359.0f; // for HSL: H value max is 359
			} else if (i != 0 && b[i] > 100.0f) {
				b[i] = 100.0f; // for HSL: S and L max is 100%
			}
		} else if (i == 3 && b[i] > 1.0f) {
			b[i] = 1.0f; // for alpha - 1.0 is max
		}

		if (i == 3) {
			b[i] = b[i] * 255.0f; // translate alpha to (0-255)
		}

		str.skipChars<StringView::CharGroup<CharGroupId::WhiteSpace>>();

		if (str.empty()) {
			return false;
		}

		if (str.is('%')) {
			++ str;
			if (b[i] > 100.0f) {
				b[i] = 100.0f; // for percent values - max is 100
			}
			if (isRgb) {
				b[i] = 255.0f * b[i] / 100.0f; // translate RGB values to (0-255)
			} else if ((!isRgb && i == 0) || i == 3) {
				return false; // for H in HSL and any alpha percent values is denied
			}
			str.skipChars<StringView::CharGroup<CharGroupId::WhiteSpace>>();
		} else if (!isRgb && (i == 1 || i == 2) && !str.is('%')) {
			return false; // for S and L in HSL only percent values allowed
		}

		if (str.empty()) {
			return false;
		}

		str.skipChars<StringView::CharGroup<CharGroupId::WhiteSpace>, StringView::Chars<','>>();

		if (str.is(')') && i == num - 1) {
			return true;
		}
	}

	return true;
}

static bool readRgbaColor(const StringView &origStr, Color3B &color, uint8_t &opacity) {
	float b[4] = { 0.0f };
	if (readColorDigits(origStr, b, 4, true)) {
		color.r = (uint8_t)b[0];
		color.g = (uint8_t)b[1];
		color.b = (uint8_t)b[2];
		opacity = (uint8_t)b[3];
		return true;
	}
	return false;
}

static bool readRgbColor(const StringView &origStr, Color3B &color) {
	float b[3] = { 0.0f };
	if (readColorDigits(origStr, b, 3, true)) {
		color.r = (uint8_t)b[0];
		color.g = (uint8_t)b[1];
		color.b = (uint8_t)b[2];
		return true;
	}
	return false;
}

static bool readHslaColor(const StringView &origStr, Color3B &color, uint8_t &opacity) {
	float b[4] = { 0.0f };
	if (readColorDigits(origStr, b, 4, false)) {
		setHslColor(color, b[0], b[1], b[2]);
		opacity = (uint8_t)b[3];
		return true;
	}
	return false;
}

static bool readHslColor(const StringView &origStr, Color3B &color) {
	float b[3] = { 0.0f };
	if (readColorDigits(origStr, b, 3, false)) {
		setHslColor(color, b[0], b[1], b[2]);
		return true;
	}
	return false;
}

static bool readHashColor(const StringView &origStr, Color3B &color) {
	StringView str(origStr);
	++ str;
	if (str.size() == 6) {
		color.r = base16::hexToChar(str[0], str[1]);
		color.g = base16::hexToChar(str[2], str[3]);
		color.b = base16::hexToChar(str[4], str[5]);
	} else if (str.size() == 3) {
		color.r = base16::hexToChar(str[0], str[0]);
		color.g = base16::hexToChar(str[1], str[1]);
		color.b = base16::hexToChar(str[2], str[2]);
	} else {
		return false;
	}

	return true;
}

static bool readHashColor(const StringView &origStr, Color4B &color) {
	StringView str(origStr);
	++ str;
	if (str.size() == 8) {
		color.r = base16::hexToChar(str[0], str[1]);
		color.g = base16::hexToChar(str[2], str[3]);
		color.b = base16::hexToChar(str[4], str[5]);
		color.a = base16::hexToChar(str[4], str[5]);
	} else if (str.size() == 4) {
		color.r = base16::hexToChar(str[0], str[0]);
		color.g = base16::hexToChar(str[1], str[1]);
		color.b = base16::hexToChar(str[2], str[2]);
		color.a = base16::hexToChar(str[2], str[2]);
	} else {
		return false;
	}

	return true;
}

static bool readNamedColor(const StringView &origStr, Color3B &color) {
	if (origStr.equals("white")) {
		color = Color3B::WHITE;
	} else if (origStr.equals("silver")) {
		color = Color3B(192,192,192);
	} else if (origStr.equals("gray") || origStr.equals("grey")) {
		color = Color3B(128,128,128);
	} else if (origStr.equals("black")) {
		color = Color3B::BLACK;
	} else if (origStr.equals("maroon")) {
		color = Color3B(128,0,0);
	} else if (origStr.equals("red")) {
		color = Color3B(255,0,0);
	} else if (origStr.equals("orange")) {
		color = Color3B(255,165,0);
	} else if (origStr.equals("yellow")) {
		color = Color3B(255,255,0);
	} else if (origStr.equals("olive")) {
		color = Color3B(128,128,0);
	} else if (origStr.equals("lime")) {
		color = Color3B(0,255,0);
	} else if (origStr.equals("green")) {
		color = Color3B(0,128,0);
	} else if (origStr.equals("aqua")) {
		color = Color3B(0,255,255);
	} else if (origStr.equals("blue")) {
		color = Color3B(0,0,255);
	} else if (origStr.equals("navy")) {
		color = Color3B(0,0,128);
	} else if (origStr.equals("teal")) {
		color = Color3B(0,128,128);
	} else if (origStr.equals("fuchsia")) {
		color = Color3B(255,0,255);
	} else if (origStr.equals("purple")) {
		color = Color3B(128,0,128);
	} else if (!table::getColor(origStr, color)) {
		return false;
	}
	return true;
}

bool readColor(const StringView &str, Color4B &color4) {
	Color3B color;
	uint8_t opacity = 0;
	if (str.starts_with("rgba")) {
		if (readRgbaColor(StringView(str.data() + 4, str.size() - 4), color, opacity)) {
			color4 = Color4B(color, opacity);
			return true;
		}
	} else if (str.starts_with("hsla")) {
		if (readHslaColor(StringView(str.data() + 4, str.size() - 4), color, opacity)) {
			color4 = Color4B(color, opacity);
			return true;
		}
	} else if (str.starts_with("rgb")) {
		if (readRgbColor(StringView(str.data() + 3, str.size() - 3), color)) {
			color4 = Color4B(color);
			return true;
		}
	} else if (str.starts_with("hsl")) {
		if (readHslColor(StringView(str.data() + 3, str.size() - 3), color)) {
			color4 = Color4B(color);
			return true;
		}
	} else if (str.is('#') && (str.size() == 4 || str.size() == 7)) {
		if (readHashColor(str, color)) {
			color4 = Color4B(color);
			return true;
		}
	} else if (str.is('#') && (str.size() == 5 || str.size() == 9)) {
		if (readHashColor(str, color4)) {
			return true;
		}
	} else {
		if (readNamedColor(str, color)) {
			color4 = Color4B(color);
			return true;
		}
	}
	return false;
}

bool readColor(const StringView &str, Color3B &color) {
	if (str.starts_with("rgb")) {
		if (readRgbColor(StringView(str.data() + 3, str.size() - 3), color)) {
			return true;
		}
	} else if (str.starts_with("hsl")) {
		if (readHslColor(StringView(str.data() + 3, str.size() - 3), color)) {
			return true;
		}
	} else if (str.is('#')) {
		if (readHashColor(str, color)) {
			return true;
		}
	} else {
		if (readNamedColor(str, color)) {
			return true;
		}
	}
	return false;
}


MD_COLOR_SPEC_DEFINE(Red, 0, ffebee, ffcdd2, ef9a9a, e57373, ef5350, f44336, e53935, d32f2f, c62828, b71c1c, ff8a80, ff5252, ff1744, d50000 );
MD_COLOR_SPEC_DEFINE(Pink, 1, fce4ec, f8bbd0, f48fb1, f06292, ec407a, e91e63, d81b60, c2185b, ad1457, 880e4f, ff80ab, ff4081, f50057, c51162 );
MD_COLOR_SPEC_DEFINE(Purple, 2, f3e5f5, e1bee7, ce93d8, ba68c8, ab47bc, 9c27b0, 8e24aa, 7b1fa2, 6a1b9a, 4a148c, ea80fc, e040fb, d500f9, aa00ff );
MD_COLOR_SPEC_DEFINE(DeepPurple, 3, ede7f6, d1c4e9, b39ddb, 9575cd, 7e57c2, 673ab7, 5e35b1, 512da8, 4527a0, 311b92, b388ff, 7c4dff, 651fff, 6200ea );
MD_COLOR_SPEC_DEFINE(Indigo, 4, e8eaf6, c5cae9, 9fa8da, 7986cb, 5c6bc0, 3f51b5, 3949ab, 303f9f, 283593, 1a237e, 8c9eff, 536dfe, 3d5afe, 304ffe );
MD_COLOR_SPEC_DEFINE(Blue, 5, e3f2fd, bbdefb, 90caf9, 64b5f6, 42a5f5, 2196f3, 1e88e5, 1976d2, 1565c0, 0d47a1, 82b1ff, 448aff, 2979ff, 2962ff );
MD_COLOR_SPEC_DEFINE(LightBlue, 6, e1f5fe, b3e5fc, 81d4fa, 4fc3f7, 29b6f6, 03a9f4, 039be5, 0288d1, 0277bd, 01579b, 80d8ff, 40c4ff, 00b0ff, 0091ea );
MD_COLOR_SPEC_DEFINE(Cyan, 7, e0f7fa, b2ebf2, 80deea, 4dd0e1, 26c6da, 00bcd4, 00acc1, 0097a7, 00838f, 006064, 84ffff, 18ffff, 00e5ff, 00b8d4 );
MD_COLOR_SPEC_DEFINE(Teal, 8, e0f2f1, b2dfdb, 80cbc4, 4db6ac, 26a69a, 009688, 00897b, 00796b, 00695c, 004d40, a7ffeb, 64ffda, 1de9b6, 00bfa5 );
MD_COLOR_SPEC_DEFINE(Green, 9, e8f5e9, c8e6c9, a5d6a7, 81c784, 66bb6a, 4caf50, 43a047, 388e3c, 2e7d32, 1b5e20, b9f6ca, 69f0ae, 00e676, 00c853 );
MD_COLOR_SPEC_DEFINE(LightGreen, 10, f1f8e9, dcedc8, c5e1a5, aed581, 9ccc65, 8bc34a, 7cb342, 689f38, 558b2f, 33691e, ccff90, b2ff59, 76ff03, 64dd17 );
MD_COLOR_SPEC_DEFINE(Lime, 11, f9fbe7, f0f4c3, e6ee9c, dce775, d4e157, cddc39, c0ca33, afb42b, 9e9d24, 827717, f4ff81, eeff41, c6ff00, aeea00 );
MD_COLOR_SPEC_DEFINE(Yellow, 12, fffde7, fff9c4, fff59d, fff176, ffee58, ffeb3b, fdd835, fbc02d, f9a825, f57f17, ffff8d, ffff00, ffea00, ffd600 );
MD_COLOR_SPEC_DEFINE(Amber, 13, fff8e1, ffecb3, ffe082, ffd54f, ffca28, ffc107, ffb300, ffa000, ff8f00, ff6f00, ffe57f, ffd740, ffc400, ffab00 );
MD_COLOR_SPEC_DEFINE(Orange, 14, fff3e0, ffe0b2, ffcc80, ffb74d, ffa726, ff9800, fb8c00, f57c00, ef6c00, e65100, ffd180, ffab40, ff9100, ff6d00 );
MD_COLOR_SPEC_DEFINE(DeepOrange, 15, fbe9e7, ffccbc, ffab91, ff8a65, ff7043, ff5722, f4511e, e64a19, d84315, bf360c, ff9e80, ff6e40, ff3d00, dd2c00 );
MD_COLOR_SPEC_BASE_DEFINE(Brown, 16, efebe9, d7ccc8, bcaaa4, a1887f, 8d6e63, 795548, 6d4c41, 5d4037, 4e342e, 3e2723 );
MD_COLOR_SPEC_BASE_DEFINE(Grey, 17, fafafa, f5f5f5, eeeeee, e0e0e0, bdbdbd, 9e9e9e, 757575, 616161, 424242, 212121 );
MD_COLOR_SPEC_BASE_DEFINE(BlueGrey, 18, eceff1, cfd8dc, b0bec5, 90a4ae, 78909c, 607d8b, 546e7a, 455a64, 37474f, 263238 );

Color Color::White(0xFFFFFF, uint16_t(19 * 16 + 0));
Color Color::Black(0x000000, uint16_t(19 * 16 + 1));

struct _ColorHsl {
	float h;
	float s;
	float l;
};

static _ColorHsl rgb_to_hsl(uint32_t color) {
	_ColorHsl ret;
    float
        r = float((color >> (2 * 8)) & maxOf<uint8_t>()) / maxOf<uint8_t>(),
        g = float((color >> (1 * 8)) & maxOf<uint8_t>()) / maxOf<uint8_t>(),
        b = float((color >> (0 * 8)) & maxOf<uint8_t>()) / maxOf<uint8_t>();

    float
        maxv = std::max(std::max(r, g), b),
        minv = std::min(std::min(r, g), b),
		d = maxv - minv;

    ret.l = (maxv + minv) / 2;

    if (maxv != minv) {
    	ret.s = ret.l > 0.5f ? d / (2 - maxv - minv) : d / (maxv + minv);
        if (maxv == r) {
        	ret.h = (g - b) / d + (g < b ? 6 : 0);
        } else if (maxv == g) {
        	ret.h = (b - r) / d + 2;
        } else if (maxv == b) {
        	ret.h = (r - g) / d + 4;
        }
        ret.h /= 6;
    }
    return ret;
}

static float hue_to_rgb(float v1, float v2, float vH) {
	if (vH < 0)
		vH += 1;

	if (vH > 1)
		vH -= 1;

	if ((6 * vH) < 1)
		return (v1 + (v2 - v1) * 6 * vH);

	if ((2 * vH) < 1)
		return v2;

	if ((3 * vH) < 2)
		return (v1 + (v2 - v1) * ((2.0f / 3) - vH) * 6);

	return v1;
}

static uint32_t hsl_to_rgb(const _ColorHsl &color, uint32_t source) {
	uint8_t r = 0;
	uint8_t g = 0;
	uint8_t b = 0;

	if (color.s == 0.0f) {
		r = g = b = (uint8_t)(color.l * 255);
	} else {
		float v1, v2;
		const float hue = color.h;

		v2 = (color.l < 0.5) ? (color.l * (1 + color.s)) : ((color.l + color.s) - (color.l * color.s));
		v1 = 2 * color.l - v2;

		r = (uint8_t)(255 * hue_to_rgb(v1, v2, hue + (1.0f / 3)));
		g = (uint8_t)(255 * hue_to_rgb(v1, v2, hue));
		b = (uint8_t)(255 * hue_to_rgb(v1, v2, hue - (1.0f / 3)));
	}

    return (uint32_t(r) << (2 * 8)) |
           (uint32_t(g) << (1 * 8)) |
           (uint32_t(b) << (0 * 8)) |
           (uint32_t((source >> (3 * 8)) & maxOf<uint8_t>()) << (3 * 8));
}

static float color_index_to_l(uint8_t id) {
	return 1.0f - (id + 1.5f) / 12.0f;
}

static uint8_t color_l_to_index(float l) {
	const float tmp = (1.0f - l) * 12.0f;
	if (tmp < 1.0f || tmp >= 11.0f) {
		return maxOf<uint8_t>();
	}
	return uint8_t(roundf(tmp - 1.5f));
}

static uint32_t make_lighter(uint32_t color, uint8_t index) {
    _ColorHsl hsl = rgb_to_hsl(color);

    uint8_t id = color_l_to_index(hsl.l);
    if (id == maxOf<uint8_t>()) {
    	return color;
    }
	if (id < index) {
		id = 0;
	} else {
		id = (id + 10 - index) % 10 + 1;
	}
	hsl.l = color_index_to_l(id);

    return hsl_to_rgb(hsl, color);
}
static uint32_t make_darker(uint32_t color, uint8_t index) {
    _ColorHsl hsl = rgb_to_hsl(color);

    uint8_t id = color_l_to_index(hsl.l);
    if (id == maxOf<uint8_t>()) {
    	return color;
    }
	if (id + index > 9) {
		id = 9;
	} else {
		id = (id + 10 + index) % 10;
	}
	hsl.l = color_index_to_l(id);

    return hsl_to_rgb(hsl, color);
}

static uint32_t make_specific(uint32_t color, uint8_t index) {
    _ColorHsl hsl = rgb_to_hsl(color);

    if (index == 10) {
    	index = 1;
    } else if (index == 11) {
    	index = 2;
    } else if (index == 12) {
    	index = 4;
    } else if (index == 13) {
    	index = 7;
    } else {
    	index = 5;
    }

	hsl.l = color_index_to_l(index);

    return hsl_to_rgb(hsl, color);
}

Color Color::getById(uint16_t index) {
	return table::getByIndex(index);
}

uint16_t Color::getColorIndex(uint32_t value) {
	return table::getColorIndex(value);
}

Color::Color(uint32_t value, int16_t index) {
	_value = value;
	_index = index;
}

Color Color::text() const {
	float r = ((_value >> 16) & 0xFF) / 255.0f;
	float g = ((_value >> 8) & 0xFF) / 255.0f;
	float b = (_value & 0xFF) / 255.0f;

	float l = 0.2989f * r + 0.5870f * g + 0.1140f * b;

	if (l <= 0.55f) {
		return Color::White;
	} else {
		return Color::Black;
	}
}

Color::Color(uint32_t value) : Color(value, getColorIndex(value)) { }

Color::Color(const Color3B &color)
: Color((uint32_t)(((color.r << 16) & 0xFF0000) | ((color.g << 8) & 0xFF00) | (color.b & 0xFF))) {
	 _index = getColorIndex(_value);
}

Color::Color(const Color4B &color)
: Color((uint32_t)(((color.r << 16) & 0xFF0000) | ((color.g << 8) & 0xFF00) | (color.b & 0xFF))) {
	 _index = getColorIndex(_value);
}

Color::Color(Tone tone, Level level) {
	*this = getById((int16_t)tone * 16 | (int16_t)level);
}

Color Color::previous() const {
	return lighter(1);
}
Color Color::next() const {
	return darker(1);
}

Color Color::lighter(uint8_t index) const {
	if (_index == maxOf<uint16_t>()) {
		return Color(make_lighter(_value, index));
	}

	auto targetIndex = _index;
	if (index > 0 && targetIndex == Color::Black._index) {
		targetIndex = Color::Grey_900._index;
		index --;
	}

	uint16_t color = targetIndex & 0xFFF0;
	uint16_t id = targetIndex & 0x0F;
	if (id >= 0 && id <= 9) {
        if (id < index) {
            return getById(color | 0);
        }
		id = (id + 10 - index) % 10;
		return getById(color | id);
    } else if (id >= 10 && id <= 13) {
        if (id - 10 < index) {
            return getById(color | 10);
        }
		id = (id - index);
		return getById(color | id);
	} else {
		return Color(0);
	}
}
Color Color::darker(uint8_t index) const {
	if (_index == maxOf<uint16_t>()) {
		return Color(make_darker(_value, index));
	}

	auto targetIndex = _index;
	if (index > 0 && _index == Color::White._index) {
		targetIndex = Color::Grey_50._index;
		index --;
	}

	uint16_t color = targetIndex &0xFFF0;
	uint16_t id = targetIndex & 0x0F;
	if (id >= 0 && id <= 9) {
        if (id + index >= 9) {
            return getById(color | 9);
        }
		id = (id + index) % 10;
		return getById(color | id);
    } else if (id >= 10 && id <= 13) {
        if (id + index >= 13) {
            return getById(color | 13);
        }
		id = (id + index);
		return getById(color | id);
	} else {
		return Color(0);
	}
}

Color Color::medium() const {
	if (_index == maxOf<uint16_t>()) {
		return make_specific(_value, 5);
	}
	uint16_t color = _index &0xFFF0;
	return getById(color | 5);
}
Color Color::specific(uint8_t index) const {
	if (_index == maxOf<uint16_t>()) {
		return make_specific(_value, index);
	}
	uint16_t color = _index &0xFFF0;
	return getById(color | index);
}

Color Color::specific(Level tone) const {
	return specific((uint8_t)tone);
}

template <>
auto Color::name<memory::PoolInterface>() const -> memory::PoolInterface::StringType {
	auto ret = table::getName(_value);
	if (ret.empty()) {
		ret = string::toString<memory::StandartInterface>(
				"rgb(", uint32_t(_value >> 16 & 0xFF), ", ", uint32_t(_value >> 8 & 0xFF), ", ", uint32_t(_value & 0xFF), ")");
	}
	return ret.str<memory::PoolInterface>();
}

template <>
auto Color3B::name<memory::StandartInterface>() const -> memory::StandartInterface::StringType {
	auto ret = table::getName(*this);
	if (ret.empty()) {
		ret = string::toString<memory::StandartInterface>(
				"rgb(", uint32_t(r >> 16 & 0xFF), ", ", uint32_t(g >> 8 & 0xFF), ", ", uint32_t(b & 0xFF), ")");
	}
	return ret.str<memory::StandartInterface>();
}

template <>
auto Color3B::name<memory::PoolInterface>() const -> memory::PoolInterface::StringType {
	auto ret = table::getName(*this);
	if (ret.empty()) {
		ret = string::toString<memory::StandartInterface>(
				"rgb(", uint32_t(r >> 16 & 0xFF), ", ", uint32_t(g >> 8 & 0xFF), ", ", uint32_t(b & 0xFF), ")");
	}
	return ret.str<memory::PoolInterface>();
}

template <>
auto Color::name<memory::StandartInterface>() const -> memory::StandartInterface::StringType {
	auto ret = table::getName(_value);
	if (ret.empty()) {
		ret = string::toString<memory::StandartInterface>(
				"rgb(", uint32_t(_value >> 16 & 0xFF), ", ", uint32_t(_value >> 8 & 0xFF), ", ", uint32_t(_value & 0xFF), ")");
	}
	return ret.str<memory::StandartInterface>();
}

Color Color::getColorByName(const StringView &str, const Color &def) {
	Color3B color;
	if (readColor(str, color)) {
		return Color(color);
	}
	return def;
}

Color Color::progress(const Color &a, const Color &b, float fp) {
	uint8_t p = (uint8_t) (fp * 255.0f);
	uint32_t val = (
			( ( (a._value >> 16 & 0xFF) * (255 - p) + (b._value >> 16 & 0xFF) * p ) / 255 << 16 & 0xFF0000 ) |
			( ( (a._value >> 8 & 0xFF) * (255 - p) + (b._value >> 8 & 0xFF) * p ) / 255 << 8 & 0xFF00) |
			( ( (a._value & 0xFF) * (255 - p) + (b._value & 0xFF) * p ) / 255 & 0xFF)
	);
	return Color(val);
}

Color3B Color3B::getColorByName(StringView str, const Color3B &def) {
	Color3B color;
	if (readColor(str, color)) {
		return color;
	}
	return def;
}

Color4B Color4B::getColorByName(StringView str, const Color4B &def) {
	Color4B color;
	if (readColor(str, color)) {
		return color;
	}
	return def;
}

Color3B::Color3B(const Color4B& color) : r(color.r), g(color.g), b(color.b) {}
Color3B::Color3B(const Color4F& color) : r(color.r * 255.0f), g(color.g * 255.0f), b(color.b * 255.0f) {}

bool Color3B::operator==(const Color3B& right) const {
    return (r == right.r && g == right.g && b == right.b);
}

bool Color3B::operator==(const Color4B& right) const {
    return (r == right.r && g == right.g && b == right.b && 255 == right.a);
}

bool Color3B::operator==(const Color4F& right) const {
    return (right.a == 1.0f && Color4F(*this) == right);
}

bool Color3B::operator!=(const Color3B& right) const {
    return !(*this == right);
}

bool Color3B::operator!=(const Color4B& right) const {
    return !(*this == right);
}

bool Color3B::operator!=(const Color4F& right) const {
    return !(*this == right);
}

Color3B Color3B::progress(const Color3B &a, const Color3B &b, float fp) {
	const uint8_t p = (uint8_t) (fp * 255.0f);
	return Color3B(
		(a.r * (255 - p) + b.r * p) / 255,
		(a.g * (255 - p) + b.g * p) / 255,
		(a.b * (255 - p) + b.b * p) / 255
	);
}

Color4B::Color4B(const Color3B& color) : r(color.r), g(color.g), b(color.b), a(255) { }
Color4B::Color4B(const Color4F& color) : r(color.r * 255), g(color.g * 255), b(color.b * 255), a(color.a * 255) { }

bool Color4B::operator==(const Color4B& right) const {
	return (r == right.r && g == right.g && b == right.b && a == right.a);
}

bool Color4B::operator==(const Color3B& right) const {
	return (r == right.r && g == right.g && b == right.b && a == 255);
}

bool Color4B::operator==(const Color4F& right) const {
	return (*this == Color4B(right));
}

bool Color4B::operator!=(const Color4B& right) const {
	return !(*this == right);
}

bool Color4B::operator!=(const Color3B& right) const {
	return !(*this == right);
}

bool Color4B::operator!=(const Color4F& right) const {
	return !(*this == right);
}

Color4B Color4B::white(uint8_t opacity) {
	return Color4B(255, 255, 255, opacity);
}

Color4B Color4B::black(uint8_t opacity) {
	return Color4B(0, 0, 0, opacity);
}

Color4B Color4B::progress(const Color4B &a, const Color4B &b, float fp) {
	const uint8_t p = (uint8_t) (fp * 255.0f);
	return Color4B(
		(a.r * (255 - p) + b.r * p) / 255,
		(a.g * (255 - p) + b.g * p) / 255,
		(a.b * (255 - p) + b.b * p) / 255,
		(a.a * (255 - p) + b.a * p) / 255
	);
}


bool Color4F::operator==(const Color4F& right) const {
	return (r == right.r && g == right.g && b == right.b && a == right.a);
}

bool Color4F::operator==(const Color3B& right) const {
	return (a == 1.0f && Color3B(*this) == right);
}

bool Color4F::operator==(const Color4B& right) const {
	return (*this == Color4F(right));
}

bool Color4F::operator!=(const Color4F& right) const {
	return !(*this == right);
}

bool Color4F::operator!=(const Color3B& right) const {
	return !(*this == right);
}

bool Color4F::operator!=(const Color4B& right) const {
	return !(*this == right);
}

Color3B Color4F::getColor() const {
	return Color3B(uint8_t(r * 255.0f), uint8_t(g * 255.0f), uint8_t(b * 255.0f));
}

uint8_t Color4F::getOpacity() const {
	return uint8_t(a * 255.0f);
}

void Color4F::setMasked(const Color4F &color, ColorMask mask) {
	if ((mask & ColorMask::R) != ColorMask::None) { r = color.r; }
	if ((mask & ColorMask::G) != ColorMask::None) { g = color.g; }
	if ((mask & ColorMask::B) != ColorMask::None) { b = color.b; }
	if ((mask & ColorMask::A) != ColorMask::None) { a = color.a; }
}

void Color4F::setUnmasked(const Color4F &color, ColorMask mask) {
	if ((mask & ColorMask::R) == ColorMask::None) { r = color.r; }
	if ((mask & ColorMask::G) == ColorMask::None) { g = color.g; }
	if ((mask & ColorMask::B) == ColorMask::None) { b = color.b; }
	if ((mask & ColorMask::A) == ColorMask::None) { a = color.a; }
}

std::ostream & operator<<(std::ostream & stream, const Color & obj) {
	stream << "Color:" << obj.name<memory::StandartInterface>() << ";";
	return stream;
}

std::ostream & operator<<(std::ostream & stream, const Color3B & obj) {
	stream << "Color3B(r:" << obj.r << " g:" << obj.g << " b:" << obj.b << ");";
	return stream;
}

std::ostream & operator<<(std::ostream & stream, const Color4B & obj) {
	stream << "Color4B(r:" << obj.r << " g:" << obj.g << " b:" << obj.b << " a:" << obj.a << ");";
	return stream;
}

std::ostream & operator<<(std::ostream & stream, const Color4F & obj) {
	stream << "Color4F(r:" << obj.r << " g:" << obj.g << " b:" << obj.b << " a:" << obj.a << ");";
	return stream;
}

#ifdef __LCC__

constexpr const Color3B Color3B::WHITE(255, 255, 255);
constexpr const Color3B Color3B::BLACK(0, 0, 0);

constexpr const Color4B Color4B::WHITE(255, 255, 255, 255);
constexpr const Color4B Color4B::BLACK(0, 0, 0, 255);

constexpr const Color4F Color4F::WHITE(1, 1, 1, 1);
constexpr const Color4F Color4F::BLACK(0, 0, 0, 1);
constexpr const Color4F Color4F::ZERO(0, 0, 0, 0);
constexpr const Color4F Color4F::ONE(1, 1, 1, 1);

#endif

}
