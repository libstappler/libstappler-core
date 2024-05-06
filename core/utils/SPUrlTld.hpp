/**
Copyright (c) 2022 Roman Katuntsev <sbkarr@stappler.org>
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

#ifndef STAPPLER_CORE_UTILS_SPURLTLD_HPP_
#define STAPPLER_CORE_UTILS_SPURLTLD_HPP_

namespace STAPPLER_VERSIONIZED stappler {

static const char *s_IdnTld[] = {
// Generic IDN TLD
	"موقع",
	"كوم",
	"كوم",
	"موبايلي",
	"كاثوليك",
	"شبكة",
	"بيتك",
	"بازار",
	"在线",
	"中文网",
	"网址",
	"网站",
	"网络",
	"公司",
	"商城",
	"机构",
	"我爱你",
	"商标",
	"世界",
	"集团",
	"慈善",
	"公益",
	"八卦",
	"дети",
	"католик",
	"ком",
	"онлайн",
	"орг",
	"сайт",
	"संगठन",
	"कॉम",
	"नेट",
	"닷컴",
	"닷넷",
	"קום",
	"みんな",
	"セール",
	"ファッション",
	"ストア",
	"ポイント",
	"クラウド",
	"コム",
	"คอม",
// Country-code IDN TLD
	"الجزائر",
	"հայ",
	"البحرين",
	"বাংলা",
	"бел",
	"бг",
	"中国",
	"中國",
	"مصر",
	"ею",
	"ευ",
	"გე",
	"ελ",
	"香港",
	"भारत",
	"بھارت",
	"భారత్",
	"ભારત",
	"ਭਾਰਤ",
	"இந்தியா",
	"ভারত",
	"ಭಾರತ",
	"ഭാരതം",
	"ভাৰত",
	"ଭାରତ ",
	"بارت",
	"भारतम्",
	"भारोत",
	"ڀارت",
	"ایران",
	"عراق",
	"الاردن",
	"қаз",
	"ລາວ",
	"澳门",
	"澳門",
	"مليسيا",
	"موريتانيا",
	"мон",
	"المغرب",
	"мкд",
	"عمان",
	"پاکستان",
	"فلسطين",
	"قطر",
	"рф",
	"السعودية",
	"срб",
	"新加坡",
	"சிங்கப்பூர்",
	"한국",
	"ලංකා",
	"இலங்கை",
	"سودان",
	"سورية",
	"台湾",
	"台灣",
	"ไทย",
	"تونس",
	"укр",
	"امارات",
	"اليمن",
// Internationalized geographic top-level domains
	"佛山",
	"广东",
	"москва",
	"рус",
	"ابوظبي",
	"عرب",
// Internationalized brand top-level domains
	"ارامكو",
	"اتصالات",
	"联通",
	"移动",
	"中信",
	"香格里拉",
	"淡马锡",
	"大众汽车",
	"vermögensberater",
	"vermögensberatung",
	"グーグル",
	"谷歌",
	"gǔgē",
	"工行",
	"嘉里",
	"嘉里大酒店",
	"飞利浦",
	"诺基亚",
	"電訊盈科",
	"삼성",
	nullptr
};

}

#endif /* STAPPLER_CORE_UTILS_SPURLTLD_HPP_ */
