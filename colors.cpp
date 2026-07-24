//#ifndef WINVER
//#define WINVER 0x0501
//#endif
//#ifndef _WIN32_WINNT
//#define _WIN32_WINNT 0x0501
//#endif

#include "colors.h"
#include "Functions.h"

static CSCColorList init_color_list_dummy;

//regathered from https://www.colordic.org/
/*
<<!caution!>>
html color	: #ff0000 = red
gdi color	: 0x000000ff = red
즉, html color는 RGB순으로 표기되고
gdi의 COLORREF는 0xaabbggrr의 순으로 저장됨에 주의해야 한다.
*/
COLORREF g_default_color[16] = {
		black,
		dimgray,
		gray,
		darkgray,
		silver,
		lightgray,
		gainsboro,
		whitesmoke,
		white,
		snow,
		ghostwhite,
		floralwhite,
		linen,
		antiquewhite,
		papayawhip,
		blanchedalmond,
	};

std::unordered_map<const char*, COLORREF> g_cr =
{
	{"aliceblue",		(COLORREF)0x00FFF8F0},
	{"antiquewhite",	(COLORREF)0x00D7EBFA},
	{"aqua",			(COLORREF)0x00FFFF00},
	{"aquamarine",		(COLORREF)0x00D4FF7F},
	{"azure",			(COLORREF)0x00FFFFF0},
	{"beige",			(COLORREF)0x00DCF5F5},
	{"bisque",			(COLORREF)0x00C4E4FF},
	{"black",			(COLORREF)0x00000000},
	{"blanchedalmond",	(COLORREF)0x00CDEBFF},
	{"blue",			(COLORREF)0x00FF0000},
	{"blueviolet",		(COLORREF)0x00E22B8A},
	{"brown",			(COLORREF)0x002A2AA5},
	{"burlywood",		(COLORREF)0x0087B8DE},
	{"cadetblue",		(COLORREF)0x00A09E5F},
	{"chartreuse",		(COLORREF)0x0000FF7F},
	{"chocolate",		(COLORREF)0x001E69D2},
	{"coral",			(COLORREF)0x00507FFF},
	{"cornflowerblue",	(COLORREF)0x00ED9564},
	{"cornsilk",		(COLORREF)0x00DCF8FF},
	{"crimson",			(COLORREF)0x003C14DC},
	{"cyan",			(COLORREF)0x00FFFF00},
	{"darkblue",		(COLORREF)0x008B0000},
	{"darkcyan",		(COLORREF)0x008B8B00},
	{"darkgoldenrod",	(COLORREF)0x000B86B8},
	{"darkgray",		(COLORREF)0x00A9A9A9},
	{"darkgreen",		(COLORREF)0x00006400},
	{"darkkhaki",		(COLORREF)0x006BB7BD},
	{"darkmagenta",		(COLORREF)0x008B008B},
	{"darkolivegreen",	(COLORREF)0x002F6B55},
	{"darkorange",		(COLORREF)0x00008CFF},
	{"darkorchid",		(COLORREF)0x00CC3299},
	{"darkred",			(COLORREF)0x0000008B},
	{"darksalmon",		(COLORREF)0x007A96E9},
	{"darkseagreen",	(COLORREF)0x008BBC8F},
	{"darkslateblue",	(COLORREF)0x008B3D48},
	{"darkslategray",	(COLORREF)0x004F4F2F},
	{"darkturquoise",	(COLORREF)0x00D1CE00},
	{"darkviolet",		(COLORREF)0x00D30094},
	{"deeppink",		(COLORREF)0x009314FF},
	{"deepskyblue",		(COLORREF)0x00FFBF00},
	{"dimgray",			(COLORREF)0x00696969},
	{"dodgerblue",		(COLORREF)0x00FF901E},
	{"firebrick",		(COLORREF)0x002222B2},
	{"floralwhite",		(COLORREF)0x00F0FAFF},
	{"forestgreen",		(COLORREF)0x00228B22},
	{"fuchsia",			(COLORREF)0x00FF00FF},
	{"gainsboro",		(COLORREF)0x00DCDCDC},
	{"ghostwhite",		(COLORREF)0x00FFF8F8},
	{"gold",			(COLORREF)0x0000D7FF},
	{"goldenrod",		(COLORREF)0x0020A5DA},
	{"gray",			(COLORREF)0x00808080},
	{"green",			(COLORREF)0x00008000},
	{"greenyellow",		(COLORREF)0x002FFFAD},
	{"honeydew",		(COLORREF)0x00F0FFF0},
	{"hotpink",			(COLORREF)0x00B469FF},
	{"indianred",		(COLORREF)0x005C5CCD},
	{"indigo",			(COLORREF)0x0082004B},
	{"ivory",			(COLORREF)0x00F0FFFF},
	{"khaki",			(COLORREF)0x008CE6F0},
	{"lavender",		(COLORREF)0x00FAE6E6},
	{"lavenderblush",	(COLORREF)0x00F5F0FF},
	{"lawngreen",		(COLORREF)0x0000FC7C},
	{"lemonchiffon",	(COLORREF)0x00CDFAFF},
	{"lightblue",		(COLORREF)0x00E6D8AD},
	{"lightcoral",		(COLORREF)0x008080F0},
	{"lightcyan",		(COLORREF)0x00FFFFE0},
	{"lightgoldenrodyellow",(COLORREF)0x00D2FAFA},
	{"lightgray",		(COLORREF)0x00D3D3D3},
	{"lightgreen",		(COLORREF)0x0090EE90},
	{"lightpink",		(COLORREF)0x00C1B6FF},
	{"lightsalmon",		(COLORREF)0x007AA0FF},
	{"lightseagreen",	(COLORREF)0x00AAB220},
	{"lightskyblue",	(COLORREF)0x00FACE87},
	{"lightslategray",	(COLORREF)0x00998877},
	{"lightsteelblue",	(COLORREF)0x00DEC4B0},
	{"lightyellow",		(COLORREF)0x00E0FFFF},
	{"lime",			(COLORREF)0x0000FF00},
	{"limegreen",		(COLORREF)0x0032CD32},
	{"linen",			(COLORREF)0x00E6F0FA},
	{"magenta",			(COLORREF)0x00FF00FF},
	{"maroon",			(COLORREF)0x00000080},
	{"mediumaquamarine",(COLORREF)0x00AACD66},
	{"mediumblue",		(COLORREF)0x00CD0000},
	{"mediumorchid",	(COLORREF)0x00D355BA},
	{"mediumpurple",	(COLORREF)0x00DB7093},
	{"mediumseagreen",	(COLORREF)0x0071B33C},
	{"mediumslateblue",	(COLORREF)0x00EE687B},
	{"mediumspringgreen",(COLORREF)0x009AFA00},
	{"mediumturquoise",	(COLORREF)0x00CCD148},
	{"mediumvioletred",	(COLORREF)0x008515C7},
	{"midnightblue",	(COLORREF)0x00701919},
	{"mintcream",		(COLORREF)0x00FAFFF5},
	{"mistyrose",		(COLORREF)0x00E1E4FF},
	{"moccasin",		(COLORREF)0x00B5E4FF},
	{"navajowhite",		(COLORREF)0x00ADDEFF},
	{"navy",			(COLORREF)0x00800000},
	{"oldlace",			(COLORREF)0x00E6F5FD},
	{"olive",			(COLORREF)0x00008080},
	{"olivedrab",		(COLORREF)0x00238E6B},
	{"orange",			(COLORREF)0x0000A5FF},
	{"orangered",		(COLORREF)0x000045FF},
	{"orchid",			(COLORREF)0x00D670DA},
	{"palegoldenrod",	(COLORREF)0x00AAE8EE},
	{"palegreen",		(COLORREF)0x0098FB98},
	{"paleturquoise",	(COLORREF)0x00EEEEAF},
	{"palevioletred",	(COLORREF)0x009370DB},
	{"papayawhip",		(COLORREF)0x00D5EFFF},
	{"peachpuff",		(COLORREF)0x00B9DAFF},
	{"peru",			(COLORREF)0x003F85CD},
	{"pink",			(COLORREF)0x00CBC0FF},
	{"plum",			(COLORREF)0x00DDA0DD},
	{"powderblue",		(COLORREF)0x00E6E0B0},
	{"purple",			(COLORREF)0x00800080},
	{"red",				(COLORREF)0x000000FF},
	{"rosybrown",		(COLORREF)0x008F8FBC},
	{"royalblue",		(COLORREF)0x00E16941},
	{"saddlebrown",		(COLORREF)0x0013458B},
	{"salmon",			(COLORREF)0x007280FA},
	{"sandybrown",		(COLORREF)0x0060A4F4},
	{"seagreen",		(COLORREF)0x00578B2E},
	{"seashell",		(COLORREF)0x00EEF5FF},
	{"sienna",			(COLORREF)0x002D52A0},
	{"silver",			(COLORREF)0x00C0C0C0},
	{"skyblue",			(COLORREF)0x00EBCE87},
	{"slateblue",		(COLORREF)0x00CD5A6A},
	{"slategray",		(COLORREF)0x00908070},
	{"snow",			(COLORREF)0x00FAFAFF},
	{"springgreen",		(COLORREF)0x007FFF00},
	{"steelblue",		(COLORREF)0x00B48246},
	{"tan",				(COLORREF)0x008CB4D2},
	{"teal",			(COLORREF)0x00808000},
	{"thistle",			(COLORREF)0x00D8BFD8},
	{"tomato",			(COLORREF)0x004763FF},
	{"turquoise",		(COLORREF)0x00D0E040},
	{"violet",			(COLORREF)0x00EE82EE},
	{"wheat",			(COLORREF)0x00B3DEF5},
	{"white",			(COLORREF)0x00FFFFFF},
	{"whitesmoke",		(COLORREF)0x00F5F5F5},
	{"yellow",			(COLORREF)0x0000FFFF},
	{"yellowgreen",		(COLORREF)0x0032CD9A}
};

COLORREF get_default_color(int index)
{
	index %= 16;
	return g_default_color[index];
}

COLORREF get_random_color()
{
	int r = random19937(0, 255);
	int g = random19937(0, 255);
	int b = random19937(0, 255);
	return RGB(r, g, b);
}

Gdiplus::Color get_random_gcolor(bool use_alpha)
{
	int a = 255;
	int r = random19937(0, 255);
	int g = random19937(0, 255);
	int b = random19937(0, 255);

	if (use_alpha)
		a = random19937(0, 255);

	return Gdiplus::Color(a, r, g, b);
}

COLORREF get_color(COLORREF crOrigin, int nOffset)
{
	int r	= GetRValue(crOrigin) + nOffset;
	int g	= GetGValue(crOrigin) + nOffset;
	int b	= GetBValue(crOrigin) + nOffset;

	if (r < 0) r = 0; else if (r > 255) r = 255;
	if (g < 0) g = 0; else if (g > 255) g = 255;
	if (b < 0) b = 0; else if (b > 255) b = 255;

	return RGB(r, g, b);
}

Gdiplus::Color	get_color(Gdiplus::Color crOrigin, int nOffset)
{
	int a = crOrigin.GetA();
	int r = crOrigin.GetR() + nOffset;
	int g = crOrigin.GetG() + nOffset;
	int b = crOrigin.GetB() + nOffset;

	if (r < 0) r = 0; else if (r > 255) r = 255;
	if (g < 0) g = 0; else if (g > 255) g = 255;
	if (b < 0) b = 0; else if (b > 255) b = 255;

	return Gdiplus::Color(a, r, g, b);
}

//두 색의 중간 비율 색상값을 구한다. (ratio는 0.0~1.0사이이고 이 값이 0.0이면 cr1이, 1.0이면 cr2가 리턴된다.)
COLORREF get_color(COLORREF cr1, COLORREF cr2, double ratio)
{
	Clamp(ratio, 0.0, 1.0);

	int r1 = GetRValue(cr1);
	int g1 = GetGValue(cr1);
	int b1 = GetBValue(cr1);

	int r2 = GetRValue(cr2);
	int g2 = GetGValue(cr2);
	int b2 = GetBValue(cr2);
	int r, g, b;

	r = r1 - (int)((double)(r1 - r2) * ratio);
	g = g1 - (int)((double)(g1 - g2) * ratio);
	b = b1 - (int)((double)(b1 - b2) * ratio);

	return RGB(r, g, b);
}

Gdiplus::Color get_color(Gdiplus::Color cr1, Gdiplus::Color cr2, double ratio)
{
	Clamp(ratio, 0.0, 1.0);

	int a1 = cr1.GetA();
	int r1 = cr1.GetR();
	int g1 = cr1.GetG();
	int b1 = cr1.GetB();

	int a2 = cr2.GetA();
	int r2 = cr2.GetR();
	int g2 = cr2.GetG();
	int b2 = cr2.GetB();
	int a, r, g, b;

	a = a1 - (int)((double)(a1 - a2) * ratio);
	r = r1 - (int)((double)(r1 - r2) * ratio);
	g = g1 - (int)((double)(g1 - g2) * ratio);
	b = b1 - (int)((double)(b1 - b2) * ratio);

	return Gdiplus::Color(a, r, g, b);
}

Gdiplus::Color get_color(COLORREF rgb)
{
	Gdiplus::Color cr;
	cr.SetFromCOLORREF(rgb);
	return cr;
}

//기준색보다 밝거나 어두운 색을 리턴한다.
//기준색이 밝은색 계열이면 그보다 offset만큼 좀 더 어두운 색을,
//기준색이 어두운 계열이면 그보다 offset만큼 좀 더 밝은 색을 리턴한다.
//만약 밝은색은 좀 더 밝게, 어두운 색은 좀더 어둡게를 의도할 경우는 get_weak_color에 음수를 전달하면 되지만
//기준색이 흰색에 가까울 정도로 밝거나, 검정에 가까울 정도로 어둡다면 거의 변화가 없을 수 있다.
//[중요/주의] offset 은 *절대 거리 이동* 이라 기준색과 배경색의 간격(대비)을 전혀 고려하지 않는다.
//  따라서 "본문색(fg)을 배경(bg) 위에서 약하게 = 보조 텍스트(크기/날짜 등)" 표현하려고 get_weak_color(fg, offset)
//  를 쓰면 테마마다 결과가 크게 달라진다 — 고대비 테마에선 적당히 약하지만, 저대비 테마(fg 가 bg 와 가까운)에선
//  같은 offset 이동만으로 fg 가 bg 를 지나치거나 근접해 거의 안 보이게 되는 부작용이 있다.
//  → *전경색을 배경 대비로 약화* 하려는 용도라면 get_weak_color 가 아니라 get_color(fg, bg, ratio)(대비 비례 blend,
//     ratio 0=fg / 1=bg, 절대 bg 까진 안 감) 를 써야 모든 테마에서 일관된다.
//  get_weak_color 는 "배경/테두리/타이틀바처럼 *배경 계열에서 한 톤 파생*"(작은 offset)하는 용도에만 안전하다.
//  (실제 사례: CVtListCtrlEx 의 크기/날짜 보조색이 get_weak_color(cr_text, N) 이라 일부 테마에서 안 보여 ratio blend 로 전환.)
Gdiplus::Color	get_weak_color(Gdiplus::Color cr, int offset)
{
	byte lum = get_luminance(cr);

	return get_color(cr, (lum > 128) ? -offset : offset);
}

//어떤 색의 ratio 배 (검정 기준 스케일). 각 RGB 채널 × ratio, alpha 보존, 0..255 clamp.
//ratio↑ = 밝아짐 (밝기 배수). "어떤 색의 80% 밝기" = get_ratio_color(cr, 0.8f).
Gdiplus::Color get_ratio_color(Gdiplus::Color cr, float ratio)
{
	auto ch = [ratio](int v) -> BYTE
	{
		float r = v * ratio;
		return (BYTE)(r < 0.0f ? 0.0f : (r > 255.0f ? 255.0f : r));
	};

	return Gdiplus::Color(cr.GetA(), ch(cr.GetR()), ch(cr.GetG()), ch(cr.GetB()));
}

//RGB→HSL 후 L 만 흰색 쪽으로 이동 → HSL→RGB. H·S 를 고정하므로 채널 포화로 인한 hue 드리프트가 없다(가산/곱셈 대비 이점).
//HSL 식은 image_processing/hsl/hsl.h 의 HSL 클래스와 동일 — 새 의존(include) 없이 colors 모듈 안에서 자급.
Gdiplus::Color get_lightened_color(Gdiplus::Color cr, double t)
{
	if (t <= 0.0)
		return cr;
	if (t > 1.0)
		t = 1.0;

	double r = cr.GetR() / 255.0;
	double g = cr.GetG() / 255.0;
	double b = cr.GetB() / 255.0;

	double mn = r < g ? (r < b ? r : b) : (g < b ? g : b);
	double mx = r > g ? (r > b ? r : b) : (g > b ? g : b);
	double l = (mn + mx) / 2.0;

	double h = 0.0;
	double s = 0.0;
	double chroma = mx - mn;
	if (chroma != 0.0)
	{
		s = (l <= 0.5) ? chroma / (mx + mn) : chroma / (2.0 - mx - mn);
		if (mx == r)
		{
			h = 60.0 * (g - b) / chroma;
			if (h < 0.0)
				h += 360.0;
		}
		else if (mx == g)
			h = 120.0 + 60.0 * (b - r) / chroma;
		else
			h = 240.0 + 60.0 * (r - g) / chroma;
	}

	l = l + (1.0 - l) * t;	//흰색 쪽으로 t 만큼 이동(H·S 고정).

	double q = (l < 0.5) ? l * (1.0 + s) : l + s - l * s;
	double p = 2.0 * l - q;
	double hk = h / 360.0;
	double seg[3] = { hk + 1.0 / 3.0, hk, hk - 1.0 / 3.0 };

	BYTE out[3];
	for (int i = 0; i < 3; ++i)
	{
		double c = seg[i];
		if (c < 0.0)
			c += 1.0;
		else if (c > 1.0)
			c -= 1.0;

		double v;
		if (c < 1.0 / 6.0)
			v = p + (q - p) * 6.0 * c;
		else if (c < 0.5)
			v = q;
		else if (c < 2.0 / 3.0)
			v = p + (q - p) * (4.0 - 6.0 * c);
		else
			v = p;

		v = v * 255.0 + 0.5;
		out[i] = (BYTE)(v < 0.0 ? 0.0 : (v > 255.0 ? 255.0 : v));
	}

	return Gdiplus::Color(cr.GetA(), out[0], out[1], out[2]);
}

//기존 get_color(cr1, cr2, ratio) 는 ratio 를 [0,1] 로 clamp 하고 alpha 까지 보간하므로
//level>1(더 진하게) 과 alpha 보존이 안 된다. 여기서는 흰색(255) 기준으로 각 채널 편차를
//level 배 한 뒤 0..255 clamp, alpha 는 원본 유지 → 테마 강도 다이얼 용도.
//get_ratio_color 와 기준점이 반대(흰색 vs 검정)라, dark 색을 옅게 하면 "덜 어두워진다"(밝아짐).
Gdiplus::Color get_leveled_color(Gdiplus::Color cr, float level)
{
	auto ch = [level](int v) -> BYTE
	{
		float r = 255.0f + (v - 255) * level;
		return (BYTE)(r < 0.0f ? 0.0f : (r > 255.0f ? 255.0f : r));
	};

	return Gdiplus::Color(cr.GetA(), ch(cr.GetR()), ch(cr.GetG()), ch(cr.GetB()));
}

//alpha까지 고려하여 컬러의 밝기값을 리턴한다.
::byte get_luminance(Gdiplus::Color cr)
{
	const float a = cr.GetA() / 255.0f;
	const BYTE R_eff = static_cast<BYTE>(cr.GetR() * a + 255.0f * (1.0f - a));
	const BYTE G_eff = static_cast<BYTE>(cr.GetG() * a + 255.0f * (1.0f - a));
	const BYTE B_eff = static_cast<BYTE>(cr.GetB() * a + 255.0f * (1.0f - a));
	return get_gray_value(R_eff, G_eff, B_eff);
}

Gdiplus::Color get_color(std::string cr_name)
{
	return CSCColorList::get_color(cr_name);
}

#ifndef _USING_V110_SDK71_
D2D1::ColorF get_d2color(Gdiplus::Color cr)
{
	return D2D1::ColorF((float)cr.GetR() / 255.0f, (float)cr.GetG() / 255.0f, (float)cr.GetB() / 255.0f, (float)cr.GetA() / 255.0f);
}
#endif

Gdiplus::Color get_color(CString cr_str)
{
	Gdiplus::Color cr = Gdiplus::Color::Black;

	if (cr_str.IsEmpty())
		return cr;

	if (cr_str[0] == '#')
	{
		cr = get_gcolor_from_hexa_str(cr_str);
	}
	else if (cr_str[0] == 'h')
	{
		cr_str = cr_str.Mid(1);

	}
	else if (get_char_count(cr_str, ',') >= 2)
	{
		std::deque<CString> token;
		get_token_str(cr_str, token, _T(","), false);
		if (token.size() == 3)
		{
			cr = Gdiplus::Color(_ttoi(token[0]), _ttoi(token[1]), _ttoi(token[2]));
		}
		else if (token.size() == 4)
		{
			cr = Gdiplus::Color(_ttoi(token[0]), _ttoi(token[1]), _ttoi(token[2]), _ttoi(token[3]));
		}
	}
	else
	{
		//prefix 없는 6/8자리 hex 도 hex 로 처리 (예: "FFA500", "FF0000FF").
		//SMI/SAMI 자막에서 흔히 prefix 생략됨. 이 분기를 named color 보다 먼저 검사.
		int len = cr_str.GetLength();
		if (len == 6 || len == 8)
		{
			bool all_hex = true;
			for (int k = 0; k < len; k++)
			{
				TCHAR c = cr_str[k];
				if (!((c >= _T('0') && c <= _T('9')) ||
					  (c >= _T('A') && c <= _T('F')) ||
					  (c >= _T('a') && c <= _T('f'))))
				{
					all_hex = false;
					break;
				}
			}
			if (all_hex)
				return get_gcolor_from_hexa_str(cr_str);
		}

		//cr_str == _T("Red")라면 Gdiplus::Color::Red 라는 컬러값을 리턴한다.
		std::string scr_name = CString2string(cr_str);
		cr = CSCColorList::get_color(scr_name);
	}

	return cr;
}

//"FF0000"과 같은 컬러 문자열을 COLORREF로 변환
COLORREF	get_color_from_hexa_str(CString hexa_str)
{
	if (hexa_str.GetLength() == 7 && hexa_str[0] == '#')
		hexa_str = hexa_str.Mid(1);

	COLORREF color;
	if ((color = _tcstol(hexa_str, NULL, 16)) == 0)
		color = 0xFFFFFFFF;  // default is white
	else
		color = rgb_bgr(color);

	return color;
}

Gdiplus::Color get_gcolor_from_hexa_str(CString hexa_str)
{
	if (hexa_str.GetLength() < 6)
		return Gdiplus::Color();

	if (hexa_str[0] == '#')
		hexa_str = hexa_str.Mid(1);

	int a = 0, r = 0, g = 0, b = 0;

	if (hexa_str.GetLength() == 6)
	{
		a = 255;
		CString str = hexa_str.Mid(0, 2);
		r = _tcstol(hexa_str.Mid(0, 2), NULL, 16);
		g = _tcstol(hexa_str.Mid(2, 2), NULL, 16);
		b = _tcstol(hexa_str.Mid(4, 2), NULL, 16);
	}
	else if (hexa_str.GetLength() == 8)
	{
		a = _tcstol(hexa_str.Mid(0, 2), NULL, 16);
		r = _tcstol(hexa_str.Mid(2, 2), NULL, 16);
		g = _tcstol(hexa_str.Mid(4, 2), NULL, 16);
		b = _tcstol(hexa_str.Mid(6, 2), NULL, 16);
	}

	return Gdiplus::Color(a, r, g, b);
}

//rgb 순서로 "123, 12, 255" 형태의 문자열을 리턴.
CString get_color_str(COLORREF cr, CString sep)
{
	CString str;

	str.Format(_T("%d%s%d%s%d"), GetRValue(cr), sep, GetGValue(cr), sep, GetBValue(cr));
	return str;
}

//argb 순서로 "255, 123, 12, 255" 형태의 문자열을 리턴.
CString	get_color_str(Gdiplus::Color cr, bool include_alpha, CString sep)
{
	CString str;

	str.Format(_T("%d%s%d%s%d"), cr.GetR(), sep, cr.GetG(), sep, cr.GetB());

	if (include_alpha)
	{
		CString tmp;
		tmp.Format(_T("%d%s%s"), cr.GetA(), sep.GetString(), str.GetString());
		str = tmp;
	}

	return str;
}

//rgb str = _T("255, 128, 0") 또는
//argb str = _T("255, 255, 128, 0") 일 경우 token = ", "으로 하여 argb 컬러값을 얻을 수 있다.
Gdiplus::Color get_color_from_token_str(CString str, CString separator)
{
	std::deque<CString> token;
	get_token_str(str, token, separator, false);

	if (token.size() == 3)
		return Gdiplus::Color(255, _ttoi(token[0]), _ttoi(token[1]), _ttoi(token[2]));
	else if (token.size() == 4)
		return Gdiplus::Color(_ttoi(token[0]), _ttoi(token[1]), _ttoi(token[2]), _ttoi(token[3]));

	return Gdiplus::Color();
}

//컬러값을 "FF0000"과 같은 문자열로 리턴한다.
CString	get_color_hexa_str(COLORREF cr, bool upper_case, CString prefix)
{
	CString str;

	if (upper_case)
		str.Format(_T("%02X%02X%02X"), GetRValue(cr), GetGValue(cr), GetBValue(cr));
	else
		str.Format(_T("%02x%02x%02x"), GetRValue(cr), GetGValue(cr), GetBValue(cr));

	if (!prefix.IsEmpty())
		str = prefix + str;

	return str;
}

CString	get_color_hexa_str(Gdiplus::Color cr, bool upper_case, bool include_alpha)
{
	CString str;

	if (upper_case)
		str.Format(_T("%02X%02X%02X"), cr.GetR(), cr.GetG(), cr.GetB());
	else
		str.Format(_T("%02x%02x%02x"), cr.GetR(), cr.GetG(), cr.GetB());

	return str;
}

//보색
COLORREF	get_complementary_color(COLORREF cr)
{
	//int r = 255 - GetRValue(cr);
	//int g = 255 - GetGValue(cr);
	//int b = 255 - GetBValue(cr);
	return RGB(255 - GetRValue(cr), 255 - GetGValue(cr), 255 - GetBValue(cr));
}

Gdiplus::Color	get_complementary_gcolor(Gdiplus::Color cr)
{
	return Gdiplus::Color(cr.GetA(), 255 - cr.GetR(), 255 - cr.GetG(), 255 - cr.GetB());
}

COLORREF get_complementary_color(COLORREF crColor, COLORREF crBack)
{
	int	r = abs(GetRValue(crBack) - GetRValue(crColor));
	int	g = abs(GetGValue(crBack) - GetGValue(crColor));
	int	b = abs(GetBValue(crBack) - GetBValue(crColor));

	Clamp(r, 0, 255);
	Clamp(g, 0, 255);
	Clamp(b, 0, 255);

	return RGB(r, g, b);
}

//보색과는 달리 밝기에 따라 black or white를 리턴한다.
//어떤 배경색과 확연히 구분되는 컬러를 보색으로 하면 128, 128, 128과 같은 색상의 보색 역시 동일한 색이 되므로 구분되지 않는다.
COLORREF get_distinct_color(COLORREF cr)
{
	float h, s, v;
	rgb2hsv(GetRValue(cr), GetGValue(cr), GetBValue(cr), h, s, v);

	h = fmodf(h + 180.f, 360.f);

	if (s < 0.05f)
		v = 1.f - v;

	return hsv2rgb(h, s, v);
}

COLORREF get_distinct_bw_color(COLORREF cr)
{
	uint8_t lum = get_gray_value(cr);

	COLORREF distinct_bw_cr = (lum > 160) ? RGB(0, 0, 0) : RGB(255, 255, 255);

	return distinct_bw_cr;
}

//보색과는 달리 alpha까지 고려한 밝기에 따라 black or white를 리턴한다.
//어떤 배경색과 확연히 구분되는 컬러를 보색으로 하면 128, 128, 128과 같은 색상의 보색 역시 동일한 색이 되므로 구분되지 않는다.
Gdiplus::Color get_distinct_bw_color(Gdiplus::Color cr)
{
	const float a = cr.GetA() / 255.0f;
	const BYTE R_eff = static_cast<BYTE>(cr.GetR() * a + 255.0f * (1.0f - a));
	const BYTE G_eff = static_cast<BYTE>(cr.GetG() * a + 255.0f * (1.0f - a));
	const BYTE B_eff = static_cast<BYTE>(cr.GetB() * a + 255.0f * (1.0f - a));
	const BYTE lum = get_gray_value(R_eff, G_eff, B_eff);

	const Gdiplus::Color distinct_bw_cr = (lum > 160) ? Gdiplus::Color::Black : Gdiplus::Color::White;

	return distinct_bw_cr;
}

double get_wcag_contrast(Gdiplus::Color a, Gdiplus::Color b)
{
	auto rel_lum = [](const Gdiplus::Color& c) -> double {
		auto ch = [](int v) -> double {
			double f = v / 255.0;
			return (f <= 0.03928) ? f / 12.92 : pow((f + 0.055) / 1.055, 2.4);
		};
		return 0.2126 * ch(c.GetR()) + 0.7152 * ch(c.GetG()) + 0.0722 * ch(c.GetB());
	};

	double la = rel_lum(a);
	double lb = rel_lum(b);

	if (la < lb)
		std::swap(la, lb);

	return (la + 0.05) / (lb + 0.05);
}

Gdiplus::Color get_readable_text_color(Gdiplus::Color cr_back, Gdiplus::Color cr_preferred, double min_contrast)
{
	if (cr_preferred.GetValue() != Gdiplus::Color::Transparent &&
		get_wcag_contrast(cr_preferred, cr_back) >= min_contrast)
		return cr_preferred;

	//흑/백 중 실제 대비가 높은 쪽. get_distinct_bw_color 의 고정 문턱(gray>160)과 달리 중간 톤에서도 최적을 고른다
	//(#5BA2D9 는 gray=147 이라 문턱 방식이 White 를 골라 대비 2.76 이지만, 실제 비교로는 Black 이 7.61).
	return (get_wcag_contrast(Gdiplus::Color::Black, cr_back) >= get_wcag_contrast(Gdiplus::Color::White, cr_back))
			? Gdiplus::Color::Black : Gdiplus::Color::White;
}

Gdiplus::Color get_distinct_color(Gdiplus::Color cr)
{
	float h, s, v;
	gcolor_to_hsv(cr, h, s, v);

	// hue를 180도 이동
	h = fmodf(h + 180.f, 360.f);

	// 무채색(saturation이 매우 낮은 경우)에는 hue shift가 무의미하므로 명도를 반전
	if (s < 0.05f)
		v = 1.f - v;

	int r, g, b;
	hsv2rgb(h, s, v, r, g, b);

	return Gdiplus::Color(cr.GetA(),
		static_cast<BYTE>(r),
		static_cast<BYTE>(g),
		static_cast<BYTE>(b));
}

//gray = (2989 * r + 5870 * g + 1140 * b) / 10000; 
//=>0.2989 * 2^14 = 4897.1776; 
//출처: http://kipl.tistory.com/80 [Geometry & Recognition]
//rgb가 175인 값이 넘어오면 >> 14 한 후의 값이 xxx.9~로 나온다. 반올림을 해보자.
uint8_t get_gray_value(uint8_t r, uint8_t g, uint8_t b)
{
	int cr = r * 4897 + g * 9617 + b * 1868;
	//float crf = cr >> 14;	//shift연산의 결과는 정수이므로 이 식은 부정확하다.
	float crf = (float)(double)cr / pow(2,14);
	uint8_t gray = (uint8_t)ROUND(crf, 0);
	return gray;
}

uint8_t	get_gray_value(COLORREF cr)
{
	return get_gray_value(GetRValue(cr), GetGValue(cr), GetBValue(cr));
}

uint8_t	get_gray_value(Gdiplus::Color cr)
{
	//TRACE(_T("%d, %d, %d, %d gray value = %d\n"), cr.GetA(), cr.GetR(), cr.GetG(), cr.GetB(), gray_value(cr.GetR(), cr.GetG(), cr.GetB()));
	return get_gray_value(cr.GetR(), cr.GetG(), cr.GetB());
}

COLORREF get_gray_color(COLORREF cr)
{
	uint8_t gray = get_gray_value(GetRValue(cr), GetGValue(cr), GetBValue(cr));
	return RGB(gray, gray, gray);
}

Gdiplus::Color get_gray_color(Gdiplus::Color cr)
{
	uint8_t gray = get_gray_value(cr.GetR(), cr.GetG(), cr.GetB());
	return Gdiplus::Color(cr.GetA(), gray, gray, gray);
}

double get_distance(COLORREF c1, COLORREF c2)
{
	return sqrt(
		(GetRValue(c1)-GetRValue(c2))*(GetRValue(c1)-GetRValue(c2)) +
		(GetGValue(c1)-GetGValue(c2))*(GetGValue(c1)-GetGValue(c2)) +
		(GetGValue(c1)-GetGValue(c2))*(GetGValue(c1)-GetGValue(c2))
	);
}

double get_distance(Gdiplus::Color c1, Gdiplus::Color c2)
{
	long rmean = (c1.GetR() + c2.GetR()) / 2;
	long r = c1.GetR() - c2.GetR();
	long g = c1.GetG() - c2.GetG();
	long b = c1.GetB() - c2.GetB();
	return sqrt((((512 + rmean) * r * r) >> 8) + 4 * g * g + (((767 - rmean) * b * b) >> 8));
	/*
	return sqrt(
		(c1.GetR() - c2.GetR()) * (c1.GetR() - c2.GetR()) +
		(c1.GetG() - c2.GetG()) * (c1.GetG() - c2.GetG()) +
		(c1.GetB() - c2.GetB()) * (c1.GetB() - c2.GetB())
	);
	*/

	double twoPiOver240 = 2.0F * 3.1415926 / 240.0F;

	int dH, dL, dS;		// deltas in HLS coords
	WORD tHH, tLL, tSS;	// test color in HLS coords
	WORD iHH, iLL, iSS;	// input color in HLS coords
	COLORREF crTest;

	::ColorRGBToHLS(c1.ToCOLORREF(), &iHH, &iLL, &iSS);

	crTest = c2.ToCOLORREF();
	::ColorRGBToHLS(crTest, &tHH, &tLL, &tSS);

	// compute color difference in HLS space (which are cylindrical coords)
	//dc = s1^2 + s2^2 - 2*s1*s2*cos(h1-h2) + (l1-l2)^2

	dH = iHH - tHH;
	dL = iLL - tLL;
	dS = iSS * iSS + tSS * tSS;

	return (double)dS - 2.0 * iSS * tSS * cos(twoPiOver240 * dH) + (double)dL * dL;
}

void rgb2hsv(int r, int g, int b, float& fH, float& fS, float& fV)
{
	float	fR = (float)((double)r / 255.0);
	float	fG = (float)((double)g / 255.0);
	float	fB = (float)((double)b / 255.0);
	float fCMax = max(max(fR, fG), fB);
	float fCMin = min(min(fR, fG), fB);
	float fDelta = fCMax - fCMin;

	if (fDelta > 0) {
		if (fCMax == fR) {
			fH = 60 * (fmod(((fG - fB) / fDelta), 6));
		}
		else if (fCMax == fG) {
			fH = 60 * (((fB - fR) / fDelta) + 2);
		}
		else if (fCMax == fB) {
			fH = 60 * (((fR - fG) / fDelta) + 4);
		}

		if (fCMax > 0) {
			fS = fDelta / fCMax;
		}
		else {
			fS = 0;
		}

		fV = fCMax;
	}
	else {
		fH = 0;
		fS = 0;
		fV = fCMax;
	}

	if (fH < 0) {
		fH = 360 + fH;
	}
}

void hsv2rgb(float fH, float fS, float fV, int &r, int &g, int &b)
{
	float fR, fG, fB;

	float fC = fV * fS; // Chroma
	float fHPrime = fmod(fH / 60.0, 6);
	float fX = fC * (1 - fabs(fmod(fHPrime, 2) - 1));
	float fM = fV - fC;

	if (0 <= fHPrime && fHPrime < 1) {
		fR = fC;
		fG = fX;
		fB = 0;
	}
	else if (1 <= fHPrime && fHPrime < 2) {
		fR = fX;
		fG = fC;
		fB = 0;
	}
	else if (2 <= fHPrime && fHPrime < 3) {
		fR = 0;
		fG = fC;
		fB = fX;
	}
	else if (3 <= fHPrime && fHPrime < 4) {
		fR = 0;
		fG = fX;
		fB = fC;
	}
	else if (4 <= fHPrime && fHPrime < 5) {
		fR = fX;
		fG = 0;
		fB = fC;
	}
	else if (5 <= fHPrime && fHPrime < 6) {
		fR = fC;
		fG = 0;
		fB = fX;
	}
	else {
		fR = 0;
		fG = 0;
		fB = 0;
	}

	fR += fM;
	fG += fM;
	fB += fM;

	r = ROUND(fR * 255.0, 0);
	g = ROUND(fG * 255.0, 0);
	b = ROUND(fB * 255.0, 0);
}

COLORREF hsv2rgb(float fH, float fS, float fV)
{
	int r, g, b;
	hsv2rgb(fH, fS, fV, r, g, b);
	return RGB(r, g, b);
}

Gdiplus::Color hsv_to_gcolor(float h, float s, float v)
{
	float r = v, gr = v, b = v;
	if (s > 0.f)
	{
		h = fmodf(h, 360.f);
		if (h < 0.f) h += 360.f;
		const int   i = static_cast<int>(h / 60.f) % 6;
		const float f = h / 60.f - static_cast<int>(h / 60.f);
		const float p = v * (1.f - s);
		const float q = v * (1.f - f * s);
		const float t = v * (1.f - (1.f - f) * s);
		switch (i)
		{
		case 0: r = v;  gr = t;  b = p;  break;
		case 1: r = q;  gr = v;  b = p;  break;
		case 2: r = p;  gr = v;  b = t;  break;
		case 3: r = p;  gr = q;  b = v;  break;
		case 4: r = t;  gr = p;  b = v;  break;
		case 5: r = v;  gr = p;  b = q;  break;
		}
	}
	return Gdiplus::Color(255,
		static_cast<BYTE>(r * 255.f + 0.5f),
		static_cast<BYTE>(gr * 255.f + 0.5f),
		static_cast<BYTE>(b * 255.f + 0.5f));
}

void gcolor_to_hsv(Gdiplus::Color cr, float& h, float& s, float& v)
{
	const float r = cr.GetR() / 255.f;
	const float g = cr.GetG() / 255.f;
	const float b = cr.GetB() / 255.f;

	const float maxC = max(r, max(g, b));
	const float minC = min(r, min(g, b));
	const float delta = maxC - minC;

	v = maxC;
	s = (maxC > 1e-6f) ? delta / maxC : 0.f;

	if (delta < 1e-6f)
		h = 0.f;
	else if (maxC == r)
		h = 60.f * fmodf((g - b) / delta, 6.f);
	else if (maxC == g)
		h = 60.f * ((b - r) / delta + 2.f);
	else
		h = 60.f * ((r - g) / delta + 4.f);

	if (h < 0.f) h += 360.f;
}

// ── RGB → HSL ─────────────────────────────────────────────
// h: 0~360, s: 0~1, l: 0~1
void gcolor_to_hsl(Gdiplus::Color cr, float& h, float& s, float& l)
{
	const float r = cr.GetR() / 255.f;
	const float g = cr.GetG() / 255.f;
	const float b = cr.GetB() / 255.f;
	const float maxC = max(r, max(g, b));
	const float minC = min(r, min(g, b));
	const float delta = maxC - minC;

	// Lightness
	l = (maxC + minC) * 0.5f;

	// Saturation
	if (delta < 1e-6f)
		s = 0.f;
	else
		s = delta / (1.f - fabsf(2.f * l - 1.f));

	// clamp (부동소수점 오차로 1.0 초과 방지)
	if (s > 1.f) s = 1.f;

	// Hue (HSV와 동일한 계산)
	if (delta < 1e-6f)
		h = 0.f;
	else if (maxC == r)
		h = 60.f * fmodf((g - b) / delta, 6.f);
	else if (maxC == g)
		h = 60.f * ((b - r) / delta + 2.f);
	else
		h = 60.f * ((r - g) / delta + 4.f);

	if (h < 0.f) h += 360.f;
}

// ── HSL → RGB ─────────────────────────────────────────────
// h: 0~360, s: 0~1, l: 0~1
Gdiplus::Color hsl_to_gcolor(float h, float s, float l)
{
	// achromatic
	if (s < 1e-6f)
	{
		const BYTE gray = static_cast<BYTE>(l * 255.f + 0.5f);
		return Gdiplus::Color(255, gray, gray, gray);
	}

	h = fmodf(h, 360.f);
	if (h < 0.f) h += 360.f;

	const float c = (1.f - fabsf(2.f * l - 1.f)) * s;   // chroma
	const float hp = h / 60.f;
	const float x = c * (1.f - fabsf(fmodf(hp, 2.f) - 1.f));
	const float m = l - c * 0.5f;

	float r1 = 0.f, g1 = 0.f, b1 = 0.f;
	const int sector = static_cast<int>(hp) % 6;
	switch (sector)
	{
		case 0: r1 = c;  g1 = x;  b1 = 0;  break;
		case 1: r1 = x;  g1 = c;  b1 = 0;  break;
		case 2: r1 = 0;  g1 = c;  b1 = x;  break;
		case 3: r1 = 0;  g1 = x;  b1 = c;  break;
		case 4: r1 = x;  g1 = 0;  b1 = c;  break;
		case 5: r1 = c;  g1 = 0;  b1 = x;  break;
	}

	auto to_byte = [](float v) -> BYTE {
		return static_cast<BYTE>(max(0.f, min(255.f, v * 255.f + 0.5f)));
		};

	return Gdiplus::Color(255, to_byte(r1 + m), to_byte(g1 + m), to_byte(b1 + m));
}

//red ~ green 범위에서 37%일때의 색상은? get_color(0, 120, 37); (0:red, 120:green of hue)
//hue : 0(red), 60(yellow), 120(green), 180(cyan), 240(blue), 300(violet), 360(red)
COLORREF get_color(int hue_start, int hue_end, int percent, float saturation, float value)
{
	int pos = hue_start + (double)(hue_end - hue_start) * (double)percent/100.0;
	int r, g, b;

	Clamp(saturation, 0.0f, 1.0f);
	Clamp(value, 0.0f, 1.0f);

	hsv2rgb(pos, saturation, value, r, g, b);

	return RGB(r, g, b);
}

int	get_hue(COLORREF cr)
{
	float h, s, v;
	rgb2hsv(GetRValue(cr), GetGValue(cr), GetBValue(cr), h, s, v);
	return (int)h;
}

Gdiplus::Color	get_color(float hue)
{
	hue = fmodf(hue, 360.f);
	if (hue < 0.f) hue += 360.f;
	const int   i = static_cast<int>(hue / 60.f) % 6;
	const float f = hue / 60.f - floorf(hue / 60.f);
	const float q = 1.f - f;
	float r, g, b;

	switch (i)
	{
		case 0: r = 1.f; g = f;   b = 0.f; break;
		case 1: r = q;   g = 1.f; b = 0.f; break;
		case 2: r = 0.f; g = 1.f; b = f;   break;
		case 3: r = 0.f; g = q;   b = 1.f; break;
		case 4: r = f;   g = 0.f; b = 1.f; break;
		case 5: r = 1.f; g = 0.f; b = q;   break;
		default: r = g = b = 0.f;
	}
	return Gdiplus::Color(255,
		static_cast<BYTE>(r * 255.f + 0.5f),
		static_cast<BYTE>(g * 255.f + 0.5f),
		static_cast<BYTE>(b * 255.f + 0.5f));
}

Gdiplus::Color	get_color(float hue, float saturation, float value)
{
	hue = fmodf(hue, 360.f);
	if (hue < 0.f) hue += 360.f;

	Clamp(saturation, 0.f, 1.f);
	Clamp(value, 0.f, 1.f);

	int r, g, b;
	hsv2rgb(hue, saturation, value, r, g, b);

	return Gdiplus::Color(255,
		static_cast<BYTE>(r),
		static_cast<BYTE>(g),
		static_cast<BYTE>(b));
}

Gdiplus::Color lerp_color(Gdiplus::Color a, Gdiplus::Color b, float t)
{
	return Gdiplus::Color(255,
		static_cast<BYTE>(static_cast<int>(a.GetR()) + static_cast<int>((static_cast<int>(b.GetR()) - static_cast<int>(a.GetR())) * t + 0.5f)),
		static_cast<BYTE>(static_cast<int>(a.GetG()) + static_cast<int>((static_cast<int>(b.GetG()) - static_cast<int>(a.GetG())) * t + 0.5f)),
		static_cast<BYTE>(static_cast<int>(a.GetB()) + static_cast<int>((static_cast<int>(b.GetB()) - static_cast<int>(a.GetB())) * t + 0.5f)));
}

Gdiplus::Color get_normalized_bright_color(Gdiplus::Color cr, int min_peak)
{
	const int R = cr.GetR(), G = cr.GetG(), B = cr.GetB();
	const int peak = (std::max)(min_peak, (std::max)(R, (std::max)(G, B)));
	const double k = 255.0 / peak;
	return Gdiplus::Color(255,
		(BYTE)(std::min)(255, (int)(R * k)),
		(BYTE)(std::min)(255, (int)(G * k)),
		(BYTE)(std::min)(255, (int)(B * k)));
}

COLORREF gpColor2RGB(Gdiplus::Color cr)
{
	return RGB(cr.GetRed(), cr.GetGreen(), cr.GetBlue());
}

Gdiplus::Color RGB2gpColor(COLORREF cr, BYTE alpha)
{
	return Gdiplus::Color(alpha, GetRValue(cr), GetGValue(cr), GetBValue(cr));
}

//cr컬러에서 a(0), r(1), g(2), b(3) 중 한 색을 value 값으로 변경하고 그 값도 리턴한다.
Gdiplus::Color set_color(Gdiplus::Color &cr, int argb_index, BYTE value)
{
	BYTE a = (argb_index == 0 ? value : cr.GetA());
	BYTE r = (argb_index == 1 ? value : cr.GetR());
	BYTE g = (argb_index == 2 ? value : cr.GetG());
	BYTE b = (argb_index == 3 ? value : cr.GetB());

	cr = Gdiplus::Color(a, r, g, b);
	return cr;
}
/*
//cr컬러에서 a(0), r(1), g(2), b(3) 중 한 색을 value 값으로 변경한 값을 리턴해준다.
Gdiplus::Color get_color(Gdiplus::Color cr, int argb_index, BYTE value)
{
	Gdiplus::Color cr_new = cr;
	set_color(cr_new, argb_index, value);
	return cr_new;
}
*/
//주어진 컬러와 가장 유사한 표준색의 이름을 리턴.
//https://en.wikipedia.org/wiki/Web_colors
//RGB(255, 190, 200)과 가장 유사한 색은 Pink(255, 192, 203)이라고 보이지만
//LightPink(255, 182, 193)를 리턴한다. 계산식이 잘못인지 확인 필요.
CString	get_nearest_color_name(COLORREF cr_src, COLORREF* cr_nearest)
{
	CString result;

	double twoPiOver240 = 2.0F * 3.1415926 / 240.0F;

	int dclr, dclrmax;	// delta errors
	int dH, dL, dS;		// deltas in HLS coords
	WORD tHH, tLL, tSS;	// test color in HLS coords
	WORD iHH, iLL, iSS;	// input color in HLS coords
	dclrmax = SHRT_MAX;	// big number
	COLORREF crTest;

	::ColorRGBToHLS(cr_src, &iHH, &iLL, &iSS);

	for (auto cr : g_cr)
	{
		crTest = cr.second;
		::ColorRGBToHLS(crTest, &tHH, &tLL, &tSS);

		// compute color difference in HLS space (which are cylindrical coords)
		//dc = s1^2 + s2^2 - 2*s1*s2*cos(h1-h2) + (l1-l2)^2

		dH = iHH - tHH;
		dL = iLL - tLL;
		dS = iSS * iSS + tSS * tSS;

		dclr = (int)((double)dS - 2.0 * iSS * tSS * cos(twoPiOver240 * dH) + (double)dL * dL);

		if (dclr < dclrmax)
		{
			dclrmax = dclr;
			result = cr.first;
			if (cr_nearest)
				*cr_nearest = cr.second;
		}
	}

	return result;
}

//테마 enum ↔ 콤보 표시 이름의 단일 정의 테이블.
//get_color_theme_list / get_theme_name / get_theme_index 가 모두 이 테이블만 참조한다.
//(과거엔 enum 과 별도의 push_back 목록을 손으로 동기화해야 했고, 한쪽만 어긋나면
// 콤보 인덱스→set_color_theme 매핑이 조용히 틀어졌다. 이제 이름은 이 테이블이 단일 진실원.)
//custom / popup_folder_list 는 콤보 비노출 테마라 enum 끝(claude 뒤)에 두고 테이블엔 넣지 않는다.
namespace
{
	struct sc_theme_entry
	{
		int				id;
		const TCHAR*	name;
	};

	const sc_theme_entry g_sc_theme_table[] =
	{
		{ CSCColorTheme::color_theme_default,			_T("default") },
		{ CSCColorTheme::color_theme_white,				_T("white") },
		{ CSCColorTheme::color_theme_gray,				_T("gray") },
		{ CSCColorTheme::color_theme_dark_gray,			_T("dark_gray") },
		{ CSCColorTheme::color_theme_dark,				_T("dark") },
		{ CSCColorTheme::color_theme_linkmemine,		_T("linkmemine") },
		{ CSCColorTheme::color_theme_linkmemine_origin,	_T("linkmemine_origin") },
		{ CSCColorTheme::color_theme_linkmemine_se,		_T("linkmemine_se") },
		{ CSCColorTheme::color_theme_anysupport,		_T("anysupport") },
		{ CSCColorTheme::color_theme_helpu,				_T("helpu") },
		{ CSCColorTheme::color_theme_pcanypro,			_T("pcanypro") },
		{ CSCColorTheme::color_theme_zenburn,			_T("zenburn") },
		{ CSCColorTheme::color_theme_bespin,			_T("bespin") },
		{ CSCColorTheme::color_theme_black_board,		_T("black_board") },
		{ CSCColorTheme::color_theme_choco,				_T("choco") },
		{ CSCColorTheme::color_theme_danslerush_dark,	_T("danslerush_dark") },
		{ CSCColorTheme::color_theme_dark_mode_default,	_T("dark_mode_default") },
		{ CSCColorTheme::color_theme_deep_black,		_T("deep_black") },
		{ CSCColorTheme::color_theme_hello_kitty,		_T("hello_kitty") },
		{ CSCColorTheme::color_theme_hot_fudge_sundae,	_T("hot_fudge_sundae") },
		{ CSCColorTheme::color_theme_khaki,				_T("khaki") },
		{ CSCColorTheme::color_theme_mono_industrial,	_T("mono_industrial") },
		{ CSCColorTheme::color_theme_monokai,			_T("monokai") },
		{ CSCColorTheme::color_theme_mossy_lawn,		_T("mossy_lawn") },
		{ CSCColorTheme::color_theme_navajo,			_T("navajo") },
		{ CSCColorTheme::color_theme_obsidian,			_T("obsidian") },
		{ CSCColorTheme::color_theme_plastic_code_wrap,	_T("plastic_code_wrap") },
		{ CSCColorTheme::color_theme_ruby_blue,			_T("ruby_blue") },
		{ CSCColorTheme::color_theme_solarized_light,	_T("solarized_light") },
		{ CSCColorTheme::color_theme_solarized,			_T("solarized") },
		{ CSCColorTheme::color_theme_twilight,			_T("twilight") },
		{ CSCColorTheme::color_theme_vibrant_ink,		_T("vibrant_ink") },
		{ CSCColorTheme::color_theme_vim_dark_blue,		_T("vim_dark_blue") },
		{ CSCColorTheme::color_theme_claude,			_T("claude") },
		{ CSCColorTheme::color_theme_sepia,				_T("sepia") },
		{ CSCColorTheme::color_theme_windows,			_T("windows") },
		{ CSCColorTheme::color_theme_claude00,			_T("claude00") },
		{ CSCColorTheme::color_theme_claude01,			_T("claude01") },
		{ CSCColorTheme::color_theme_claude02,			_T("claude02") },
		{ CSCColorTheme::color_theme_claude03,			_T("claude03") },
		{ CSCColorTheme::color_theme_claude04,			_T("claude04") },
		{ CSCColorTheme::color_theme_claude05,			_T("claude05") },
		{ CSCColorTheme::color_theme_claude06,			_T("claude06") },
		{ CSCColorTheme::color_theme_claude07,			_T("claude07") },
		{ CSCColorTheme::color_theme_claude08,			_T("claude08") },
		{ CSCColorTheme::color_theme_claude09,			_T("claude09") },
	};

	//테마 추가 시 enum 과 테이블 양쪽을 갱신하지 않으면 컴파일 타임에 잡힌다.
	static_assert(_countof(g_sc_theme_table) == CSCColorTheme::color_theme_claude09 + 1,
				  "g_sc_theme_table 개수가 SC_COLOR_THEMES(default..claude09) 와 어긋남");
}

void CSCColorTheme::get_color_theme_list(std::deque<CString>& theme_list)
{
	theme_list.clear();

#ifdef _DEBUG
	//콤보 인덱스를 그대로 set_color_theme 에 넘기는 구조라 테이블 순서 = enum 값이어야 한다.
	//(static_assert 는 개수만 본다 — 순서/값 어긋남은 여기서 검출.)
	for (int i = 0; i < (int)_countof(g_sc_theme_table); i++)
		ASSERT(g_sc_theme_table[i].id == i);
#endif

	for (const auto& e : g_sc_theme_table)
		theme_list.push_back(e.name);
}

//static
CString CSCColorTheme::get_theme_name(int theme)
{
	for (const auto& e : g_sc_theme_table)
	{
		if (e.id == theme)
			return e.name;
	}
	return _T("default");
}

//static
int CSCColorTheme::get_theme_index(LPCTSTR name, int fallback)
{
	for (const auto& e : g_sc_theme_table)
	{
		if (_tcsicmp(e.name, name) == 0)
			return e.id;
	}
	return fallback;
}

// cr_text_dim 산출 — cr_text 와 cr_back 의 RGB 보간 + WCAG contrast 자동 보정.
// 단순 ratio 보간(0.25 고정) 은 *저대비 테마* 에서 dim 이 cr_back 과 시각적으로 거의 같아져 안 보임.
// 알고리즘:
//   1. ratio=0.25 candidate (가독 + 값/라벨 hierarchy 적정)
//   2. dim 과 cr_back 의 WCAG contrast 가 2.5 미만이면 ratio 점진 감소(0.20→0.05) 로 dim 을 cr_text 쪽으로 끌어당김
//   3. 모든 시도 실패 시 r=0.05 (거의 cr_text) — cr_text 자체가 cr_back 과 차이 작은 테마(우리 한계)
//
// WCAG relative luminance + gamma 보정 contrast = 시각 대비를 8bit RGB 차이보다 정확히 반영.
// 임계값 2.5 — WCAG AA-large(3.0) 보다 약간 낮음. dim 은 보조 라벨이라 본문 4.5 가 아니라 2.5 로 충분.
static Gdiplus::Color compute_text_dim(const Gdiplus::Color& cr_text, const Gdiplus::Color& cr_back)
{
	auto rel_lum = [](const Gdiplus::Color& c) -> double {
		auto ch = [](int v) -> double {
			double f = v / 255.0;
			return (f <= 0.03928) ? f / 12.92 : pow((f + 0.055) / 1.055, 2.4);
		};
		return 0.2126 * ch(c.GetR()) + 0.7152 * ch(c.GetG()) + 0.0722 * ch(c.GetB());
	};
	auto contrast = [&](const Gdiplus::Color& a, const Gdiplus::Color& b) -> double {
		double la = rel_lum(a), lb = rel_lum(b);
		if (la < lb) std::swap(la, lb);
		return (la + 0.05) / (lb + 0.05);
	};

	const double ratios[] = { 0.25, 0.20, 0.15, 0.10, 0.05 };
	Gdiplus::Color dim;
	for (double r : ratios)
	{
		dim = get_color(cr_text, cr_back, r);
		if (contrast(dim, cr_back) >= 2.5)
			return dim;
	}
	return dim;	// 마지막 시도 — cr_text 자체가 cr_back 과 거의 같은 저대비 테마
}

void CSCColorTheme::set_theme_from_editor_palette(Gdiplus::Color bg, Gdiplus::Color fg, Gdiplus::Color sel_bg,
								Gdiplus::Color header_fg, Gdiplus::Color header_bg)
{
	//본문
	cr_back			= bg;
	cr_parent_back	= bg;
	cr_text			= fg;
	cr_text_hover	= fg;
	//cr_text 와 cr_back 의 RGB 보간(ratio=0.35) → cr_text 쪽에 가까운 dim 색.
	//ratio 가 작을수록 cr_back 에 가깝고(흐림→ 안 보임), 클수록 cr_text 와 구분 안 됨. 0.35 = 가독 유지 + 값/라벨 hierarchy 확보.
	//get_weak_color(fg, 100) 같이 *fg 절대 거리 이동* 방식은 fg/bg 간격을 무시해 일부 테마에서 너무 dim / 너무 진함.
	cr_text_dim		= compute_text_dim(cr_text, cr_back);
	cr_disabled_text = cr_text_dim;

	//edit 본문 = 에디터 본문. import 테마는 dark IDE 류라 흰 카드 baseline 대신 에디터색 그대로 쓴다.
	cr_edit_back	= bg;
	cr_edit_text	= fg;

	//선택 — 선택 배경색의 luma 로 본문색을 흑/백 자동 (NPP 의 fgColor=000000 잠금값은 신뢰 불가).
	cr_back_selected			= sel_bg;
	cr_text_selected			= (get_luminance(sel_bg) < 128) ? Gdiplus::Color::White : Gdiplus::Color::Black;
	cr_text_selected_inactive	= cr_text_selected;
	cr_back_selected_inactive	= get_gray_color(sel_bg);
	cr_back_dropHilited			= sel_bg;
	cr_text_dropHilited			= cr_text_selected;

	//hover = 본문↔선택 사이, 교차행 = 본문 한 톤
	cr_back_hover		= get_color(bg, sel_bg, 0.4);
	cr_back_alternate	= get_weak_color(bg, 10);

	//타이틀바 = 본문 한 톤 시프트(get_weak_color auto: dark→lighter / light→darker). 버튼이 이 위에 얹힘.
	cr_title_back_active	= get_weak_color(bg, 24);
	cr_title_back_inactive	= cr_title_back_active;
	cr_title_text			= fg;
	cr_sys_buttons_hover_back = get_color(cr_title_back_active, 24);
	cr_sys_buttons_down_back  = get_color(cr_title_back_active, -24);

	//버튼 — 전 테마 동일 규칙 (face = title_inactive, border = 본문 대비색이라 face≈bg 여도 윤곽 유지)
	cr_button_back		= cr_title_back_inactive;
	cr_button_text		= cr_title_text;
	cr_button_border	= get_weak_color(bg, 40);

	//accent(선택색) = focus / selected border / progress
	cr_border_active			= sel_bg;
	cr_selected_border			= sel_bg;
	cr_selected_border_inactive	= cr_back_selected_inactive;
	cr_border_inactive			= get_weak_color(bg, 56);
	cr_separator				= get_weak_color(bg, 30);
	cr_gridlines				= get_weak_color(bg, 16);

	//list 헤더 = 에디터의 line-number margin(gutter) 색
	cr_header_back	= header_bg;
	cr_header_text	= header_fg;

	cr_percentage_bar.clear();
	cr_percentage_bar.push_back(get_weak_color(bg, 32));
	cr_progress_active		= sel_bg;
}

void CSCColorTheme::set_theme_level(float level)
{
	m_theme_level = level;
	//현재 테마를 base(정의값) 부터 다시 계산. set_color_theme 끝에서 apply_theme_level(m_theme_level)
	//가 새 강도를 일괄 적용한다. (각 case 가 색을 절대값으로 재설정하므로 호출 누적 없이 매번 base 부터.)
	set_color_theme(m_cur_theme);
}

//m_cur_theme 의 base 색(정의값) 에 강도 level 을 일괄 적용. level 이 클수록 흰 baseline 에서 멀어져
//테마색이 강해진다 (dark 는 더 어둡게, sepia 는 더 진하게). 1.0 이면 정의값 그대로라 건너뛴다.
//
//글자색(cr_*_text)·상태색은 *스케일하지 않는다*. 강도 다이얼의 목적은 "배경의 강함" 조절이고,
//글자를 배경과 같이 흰색 쪽으로 옅게 하면 level 이 낮을수록 글자·배경이 함께 흐려져 대비가
//무너진다 (level=0.4 에서 글자 가독성 저하 보고). 글자는 가독성 담당이라 정의값을 유지하면
//배경만 옅어져 light 테마는 오히려 대비가 좋아지고, dark 테마도 정상 사용 범위에서 안전하다.
void CSCColorTheme::apply_theme_level(float level)
{
	if (level == 1.0f)
		return;

	cr_back						= get_leveled_color(cr_back, level);
	cr_back_hover				= get_leveled_color(cr_back_hover, level);
	cr_back_dropHilited			= get_leveled_color(cr_back_dropHilited, level);
	cr_back_selected			= get_leveled_color(cr_back_selected, level);
	cr_back_selected_inactive	= get_leveled_color(cr_back_selected_inactive, level);
	cr_back_selected_hover		= get_leveled_color(cr_back_selected_hover, level);
	cr_back_alternate			= get_leveled_color(cr_back_alternate, level);

	cr_parent_back				= get_leveled_color(cr_parent_back, level);
	cr_edit_back				= get_leveled_color(cr_edit_back, level);

	cr_button_back				= get_leveled_color(cr_button_back, level);
	cr_button_border			= get_leveled_color(cr_button_border, level);

	cr_selected_border			= get_leveled_color(cr_selected_border, level);
	cr_selected_border_inactive	= get_leveled_color(cr_selected_border_inactive, level);
	cr_border_active			= get_leveled_color(cr_border_active, level);
	cr_border_inactive			= get_leveled_color(cr_border_inactive, level);
	cr_separator				= get_leveled_color(cr_separator, level);

	cr_title_back_active		= get_leveled_color(cr_title_back_active, level);
	cr_title_back_inactive		= get_leveled_color(cr_title_back_inactive, level);
	cr_sys_buttons_hover_back	= get_leveled_color(cr_sys_buttons_hover_back, level);
	cr_sys_buttons_down_back	= get_leveled_color(cr_sys_buttons_down_back, level);

	cr_header_back				= get_leveled_color(cr_header_back, level);

	cr_progress_active					= get_leveled_color(cr_progress_active, level);
	for (auto& c : cr_percentage_bar)
		c = get_leveled_color(c, level);
}

void CSCColorTheme::set_color_theme(int color_theme)
{
	//일부 색상을 변경했을 경우 다시 호출하면 기본값으로 롤백된다.
	m_cur_theme = color_theme;

	//edit/입력 본문 baseline = 흰 카드 + near-black. 손으로 작성한 light/neutral 테마 case 는
	//이 두 필드를 명시하지 않으므로, 직전 테마(특히 set_theme_from_editor_palette 로 dark 본문을
	//넣는 import 테마)의 값이 그대로 누수돼 CSCStaticEdit 등 cr_edit_back 을 본문색으로 쓰는
	//컨트롤이 light 테마로 바꿔도 계속 어둡게 남는 문제가 있었다. switch 진입 전 baseline 으로
	//고정하면 dark→light 전환이 결정적이 되고, dark IDE 류 import 테마는 switch 안의
	//set_theme_from_editor_palette 가 이 baseline 을 덮는다.
	cr_edit_back = Gdiplus::Color::White;
	cr_edit_text = gRGB(32, 32, 32);

	switch (color_theme)
	{
		case color_theme_linkmemine :
			//LinkMeMine 1.0 — admin.linkmemine.com 의 실제 chrome 색 + BI 4색 종합.
			//BI 가 1.0 / 3.0 SE 를 같은 행에 묶었으나 실제 두 제품 chrome 은 다르다.
			//1.0 은 navy primary + orange accent, SE 는 orange primary (color_theme_linkmemine_se 참조).
			//
			//  primary chrome   #222E3D : dark navy — admin chrome dominant 색 (#232f3e 와 매핑).
			//                              BI 에선 'logo 보조' 분류였지만 실제 1.0 UI 의 primary.
			//  accent / progress #FD7E14 : 시그니처 주황 — BI logo color, accent 로 활용
			//  body text         #16191f : near-black navy (admin 본문)
			//  secondary text    #869fb1 : mid gray-blue
			//  light bg          #f7f7f7 : 보조 배경
			//  border-inactive   #d2d2d2 : admin 일반 보더

			cr_title_text = Gdiplus::Color::White;            //navy 위 white
			cr_title_back_active = gRGB(34, 46, 61);          //#222E3D — 1.0 의 실제 primary navy
			cr_title_back_inactive = cr_title_back_active;
			cr_sys_buttons_hover_back = get_color(cr_title_back_active, 32);
			cr_sys_buttons_down_back = get_color(cr_title_back_active, 24);

			cr_text = gRGB(22, 25, 31);                       //#16191f — admin 본문 dark
			cr_text_dim = compute_text_dim(cr_text, cr_back);                //#869fb1 — mid gray-blue
			cr_text_hover = Gdiplus::Color::White;            //hover bg 가 logo color +32 (dark) 라 dark text 가독성 ↓ — white 강제
			cr_text_selected = Gdiplus::Color::White;         //selected 배경 = navy → white text
			cr_text_selected_inactive = get_color(cr_text, 32);
			cr_text_dropHilited = Gdiplus::Color::White;

			cr_back = gRGB(250, 250, 250);                    //순수 흰색 대신 살짝 톤 다운한 off-white
			cr_parent_back = cr_back;
			//primary navy (#222E3D, luma ~44) 가 매우 어두워 selection/hover 에 직접 쓰면 흰 본문 위에서 대비가 너무 강함.
			//한 톤 lighter 변환 — primary 정체성 유지하면서 톤 완화. (다른 brand 는 중간 명도라 primary 직접 사용 OK.)
			cr_back_selected = get_color(cr_title_back_active, 64);   //navy +64 — medium navy-gray
			cr_back_selected_inactive = get_color(cr_back_selected, 96);
			cr_back_dropHilited = RGB2gpColor(::GetSysColor(COLOR_HIGHLIGHT));
			cr_back_hover = get_color(cr_title_back_active, 96);      //hover 는 selected 보다 한 톤 더 lighter
			cr_back_alternate = gRGB(247, 247, 247);          //#f7f7f7 — admin light gray bg

			cr_selected_border = cr_title_back_active;        //navy
			cr_selected_border_inactive = cr_back_selected_inactive;

			cr_header_text = cr_text;
			cr_header_back = gRGB(247, 247, 247);             //#f7f7f7
			cr_percentage_bar.clear();
			cr_percentage_bar.push_back(get_color(cr_back, 32));
			cr_progress_active = gRGB(253, 126, 20);                 //#FD7E14 — BI logo orange (accent)

			//focus border 는 다른 brand theme 과 동일하게 primary chrome 색 사용. orange accent 를 border 에 쓰면
			//navy chrome + orange border 가 시각적으로 충돌. orange 는 cr_progress_active 등 강조 액션에만 한정.
			cr_border_active = cr_title_back_active;          //#222E3D — primary navy
			cr_border_inactive = gRGB(210, 210, 210);         //#d2d2d2 — admin 보더
			cr_button_back = cr_title_back_inactive;
			cr_button_text = cr_title_text;
			cr_button_border = get_weak_color(cr_back, 40);
			cr_separator = get_weak_color(cr_back, 30);
			break;

		case color_theme_linkmemine_origin :
			//LMMLoginManager renewal 이전 origin Agent UI 의 chrome 복원.
			//color_theme_linkmemine 은 admin web chrome (white bg + navy primary) 이고,
			//이 origin 테마는 데스크탑 Agent 실행파일 chrome — dark slate bg + 흰 텍스트 +
			//light blue 시그니처 버튼 + cyan accent (group box 보더, link).
			//
			//샘플 화면(LinkMeMine Agent, LinkMeMine 3.0 SE Agent) 의 pixel 추출:
			//  body bg          #52575C : medium slate (사용자 지정 픽셀값)
			//  title bg         #282D33 : body 보다 한 단계 어두운 슬레이트 (사용자 지시 — 본문과 명확히 구분)
			//                             샘플 Agent 화면(IDS_TITLE) 은 OS title bar 자체를 안 그려 같은 색이라도 무방하지만,
			//                             같은 테마를 쓰는 다른 dlg (VersionDlg 등) 는 title bar 가 있어야 하므로 schema 에서 분리.
			//  section header   #B0B5BC : 'login', '업데이트' 등 light gray
			//  edit bg / text   #FFFFFF / near-black
			//  primary button   #5BA2D9 : signature light blue (로그인 / 업데이트)
			//  group accent     #6CCFD3 : cyan — group box 보더 + '회원가입' 링크 + checkbox 체크
			//  body text        white

			cr_title_text = Gdiplus::Color::White;
			cr_title_back_active = gRGB(40, 45, 51);          //#282D33 — body(#52575C) 보다 어두운 슬레이트
			cr_title_back_inactive = cr_title_back_active;
			cr_sys_buttons_hover_back = get_color(cr_title_back_active, 32);
			cr_sys_buttons_down_back = get_color(cr_title_back_active, 24);

			cr_text = Gdiplus::Color::White;
			cr_text_dim = compute_text_dim(cr_text, cr_back);                //#B0B5BC — section header / '현재버전' 등 보조 텍스트
			cr_text_hover = Gdiplus::Color::White;            //hover bg 가 blue (#5BA2D9 +) — white 강제
			cr_text_selected = Gdiplus::Color::White;         //selected bg = blue 위 white
			cr_text_selected_inactive = get_color(cr_text, -32);
			cr_text_dropHilited = Gdiplus::Color::White;

			cr_back = gRGB(82, 87, 92);                       //#52575C — medium slate body bg (사용자 지정)
			cr_parent_back = cr_back;
			cr_back_selected = gRGB(91, 162, 217);            //#5BA2D9 — signature light blue (로그인/업데이트 버튼)
			cr_back_selected_inactive = get_gray_color(cr_back_selected);
			cr_back_dropHilited = RGB2gpColor(::GetSysColor(COLOR_HIGHLIGHT));
			cr_back_hover = get_color(cr_back_selected, 32);
			cr_back_alternate = get_color(cr_back, 8);

			//edit box 본문은 class default (white bg + near-black text) 그대로 — 별도 지정 불요.

			//focus / selected border / progress = cr_back_selected (시그니처 블루 #5BA2D9) 로 통일 —
			//테마 내 강조 색을 하나로 유지하면 edit focus / checkbox 체크 / progress 가 같은 톤으로 묶여 보임.
			//(이전엔 #6CCFD3 cyan 으로 잡았으나 본문 톤과 어긋나 어색했음 — 사용자 지적 2026-05-21.)
			//버튼색은 다른 테마와 동일 규칙 (cr_title 계열) 을 따른다 — case 끝에서 할당.
			cr_selected_border = cr_back_selected;
			cr_selected_border_inactive = cr_back_selected_inactive;

			cr_header_text = Gdiplus::Color::White;
			cr_header_back = get_color(cr_back, 16);

			cr_percentage_bar.clear();
			cr_percentage_bar.push_back(get_color(cr_back, 32));
			cr_progress_active = cr_back_selected;                   //#5BA2D9 — 시그니처 블루

			cr_border_active = cr_back_selected;              //focus border 도 동일 블루
			cr_border_inactive = get_weak_color(cr_back, 64);
			cr_button_back = cr_title_back_inactive;
			cr_button_text = cr_title_text;
			cr_button_border = get_weak_color(cr_back, 40);
			cr_separator = get_weak_color(cr_back, 30);
			break;

		case color_theme_claude:
			//Claude.ai (Anthropic) chrome 재현 — 따뜻한 크림 배경 + 시그니처 rust orange.
			//claude05 스크린샷 픽셀 추출 (histogram mode):
			//  body/title bg   #FAF7F2 (250,247,242) : warm cream — dominant
			//  primary button  #B45309 (180, 83,  9) : rust/amber orange — 로그인/재시작/확인
			//  dark text       #3F3A33 ( 63, 58, 51) : warm charcoal — headline/body
			//  deep cream      #E8E2D6 (232,226,214) : 보조버튼(white) 테두리 / separator / 헤더
			//  edit bg         #FFFFFF              : white (class default — 별도 지정 불요)
			//color_theme_linkmemine 과 같은 light 테마 구조 (밝은 bg + dark text + 단일 accent).

			cr_title_text = gRGB(63, 58, 51);                 //#3F3A33 — 밝은 title bar 위 dark text
			cr_title_back_active = gRGB(232, 226, 214);       //#E8E2D6 — body(#FAF7F2) 보다 한 단계 깊은 크림 (본문과 구분)
			cr_title_back_inactive = cr_title_back_active;
			cr_sys_buttons_hover_back = get_color(cr_title_back_active, -16);
			cr_sys_buttons_down_back = get_color(cr_title_back_active, -28);

			cr_text = gRGB(63, 58, 51);                       //#3F3A33 — 본문 dark
			cr_text_dim = compute_text_dim(cr_text, cr_back);             //~#8F8A83 — 버전/보조 텍스트 (추출 #8B8377 근사)
			cr_text_hover = Gdiplus::Color::White;            //hover bg = orange → white text 강제
			cr_text_selected = Gdiplus::Color::White;         //selected bg = orange → white text
			cr_text_selected_inactive = cr_text;              //inactive selection bg 가 옅은 크림이므로 본문 dark 색 그대로 — 대비 확보
			cr_text_dropHilited = Gdiplus::Color::White;

			cr_back = gRGB(250, 247, 242);                    //#FAF7F2 — warm cream body bg
			cr_parent_back = cr_back;
			cr_back_selected = gRGB(180, 83, 9);              //#B45309 — rust orange (selection/primary)
			cr_back_selected_inactive = get_color(cr_back, -24);  //cr_back(크림)보다 살짝만 진한 톤 — 비활성 선택을 은은하게

			cr_back_dropHilited = RGB2gpColor(::GetSysColor(COLOR_HIGHLIGHT));
			cr_back_hover = get_color(cr_back_selected, 32);  //orange 보다 한 톤 밝게
			cr_back_alternate = gRGB(232, 226, 214);          //#E8E2D6 — 교대 행은 deep cream

			//edit box 본문은 class default (white bg + near-black text) 그대로.

			cr_selected_border = cr_back_selected;            //focus/selected border = rust orange
			cr_selected_border_inactive = cr_back_selected_inactive;

			cr_header_text = cr_text;
			cr_header_back = gRGB(232, 226, 214);             //#E8E2D6

			cr_percentage_bar.clear();
			cr_percentage_bar.push_back(get_color(cr_back, -16));
			cr_progress_active = cr_back_selected;                   //#B45309 — rust orange

			cr_border_active = cr_back_selected;              //focus border = rust orange
			cr_border_inactive = gRGB(232, 226, 214);         //#E8E2D6 — 일반 보더 (deep cream)
			cr_button_back = cr_back_selected;                //#B45309 — primary button rust orange
			cr_button_text = Gdiplus::Color::White;
			cr_button_border = gRGB(232, 226, 214);           //#E8E2D6 — 보조버튼(white) 테두리
			cr_separator = gRGB(232, 226, 214);               //#E8E2D6
			break;

		//--------------------------------------------------------------------------------------------------
		// claude00 ~ claude09 : Claude 가 생성한 chrome 시안 10종.
		//   각 변형은 0.documents/screenshot/0.login before (claudeNN).png 의 픽셀 히스토그램에서 추출.
		//   light 5종 (00,03,05,07,09) 은 claude (color_theme_claude) 와 같은 골격 — bg/accent 색만 교체.
		//   dark  5종 (01,02,04,06,08) 은 linkmemine_origin 골격 — body=dark, edit box 는 class default (white) 유지.
		//   selected/button text 는 accent 의 luma 에 따라 white 또는 bg 색 채택해 대비 ≥ 48 확보.
		//--------------------------------------------------------------------------------------------------

		case color_theme_claude00:
			//light : bg #FAFAFA, alt #E5E7EB, accent #2563EB blue-600, text #111827
			cr_title_text = gRGB(17, 24, 39);
			cr_title_back_active = gRGB(229, 231, 235);
			cr_title_back_inactive = cr_title_back_active;
			cr_sys_buttons_hover_back = get_color(cr_title_back_active, -16);
			cr_sys_buttons_down_back = get_color(cr_title_back_active, -28);

			cr_text = gRGB(17, 24, 39);
			cr_text_dim = compute_text_dim(cr_text, cr_back);
			cr_text_hover = Gdiplus::Color::White;
			cr_text_selected = Gdiplus::Color::White;
			cr_text_selected_inactive = cr_text;
			cr_text_dropHilited = Gdiplus::Color::White;

			cr_back = gRGB(250, 250, 250);
			cr_parent_back = cr_back;
			cr_back_selected = gRGB(37, 99, 235);
			cr_back_selected_inactive = get_color(cr_back, -24);
			cr_back_dropHilited = RGB2gpColor(::GetSysColor(COLOR_HIGHLIGHT));
			cr_back_hover = get_color(cr_back_selected, 32);
			cr_back_alternate = gRGB(229, 231, 235);

			cr_selected_border = cr_back_selected;
			cr_selected_border_inactive = cr_back_selected_inactive;

			cr_header_text = cr_text;
			cr_header_back = gRGB(229, 231, 235);

			cr_percentage_bar.clear();
			cr_percentage_bar.push_back(get_color(cr_back, -16));
			cr_progress_active = cr_back_selected;

			cr_border_active = cr_back_selected;
			cr_border_inactive = gRGB(229, 231, 235);
			cr_button_back = cr_back_selected;
			cr_button_text = Gdiplus::Color::White;
			cr_button_border = gRGB(229, 231, 235);
			cr_separator = gRGB(229, 231, 235);
			break;

		case color_theme_claude01:
			//dark mono : bg #1E1E1E, title #2A2A2A, accent #EDEDED (light gray), text white.
			//accent 가 매우 밝은 회색이라 selected/button text 는 bg 색(#1E1E1E)으로 두어 가독성 확보.
			cr_title_text = Gdiplus::Color::White;
			cr_title_back_active = gRGB(42, 42, 42);
			cr_title_back_inactive = cr_title_back_active;
			cr_sys_buttons_hover_back = get_color(cr_title_back_active, 32);
			cr_sys_buttons_down_back = get_color(cr_title_back_active, 24);

			cr_text = Gdiplus::Color::White;
			cr_text_dim = compute_text_dim(cr_text, cr_back);
			cr_text_hover = gRGB(30, 30, 30);                 //hover bg = light gray → dark text
			cr_text_selected = gRGB(30, 30, 30);
			cr_text_selected_inactive = cr_text;
			cr_text_dropHilited = Gdiplus::Color::White;

			cr_back = gRGB(30, 30, 30);
			cr_parent_back = cr_back;
			cr_back_selected = gRGB(237, 237, 237);
			cr_back_selected_inactive = get_gray_color(cr_back_selected);
			cr_back_dropHilited = RGB2gpColor(::GetSysColor(COLOR_HIGHLIGHT));
			cr_back_hover = get_color(cr_back_selected, -32);
			cr_back_alternate = get_color(cr_back, 16);

			cr_selected_border = cr_back_selected;
			cr_selected_border_inactive = cr_back_selected_inactive;

			cr_header_text = cr_text;
			cr_header_back = get_color(cr_back, 16);

			cr_percentage_bar.clear();
			cr_percentage_bar.push_back(get_color(cr_back, 32));
			cr_progress_active = cr_back_selected;

			cr_border_active = cr_back_selected;
			cr_border_inactive = get_weak_color(cr_back, 64);
			cr_button_back = cr_back_selected;
			cr_button_text = gRGB(30, 30, 30);
			cr_button_border = get_weak_color(cr_back, 40);
			cr_separator = get_weak_color(cr_back, 30);
			break;

		case color_theme_claude02:
			//dark Nord : bg #2E3440 polar-night-0, title #3B4252, accent #88C0D0 frost-2 cyan, text #ECEFF4 snow-storm.
			//frost cyan 위 정통 Nord 텍스트 = polar-night → selected/button text = bg 색.
			cr_title_text = gRGB(236, 239, 244);
			cr_title_back_active = gRGB(59, 66, 82);
			cr_title_back_inactive = cr_title_back_active;
			cr_sys_buttons_hover_back = get_color(cr_title_back_active, 32);
			cr_sys_buttons_down_back = get_color(cr_title_back_active, 24);

			cr_text = gRGB(236, 239, 244);
			cr_text_dim = compute_text_dim(cr_text, cr_back);
			cr_text_hover = gRGB(46, 52, 64);
			cr_text_selected = gRGB(46, 52, 64);
			cr_text_selected_inactive = cr_text;
			cr_text_dropHilited = Gdiplus::Color::White;

			cr_back = gRGB(46, 52, 64);
			cr_parent_back = cr_back;
			cr_back_selected = gRGB(136, 192, 208);
			cr_back_selected_inactive = get_gray_color(cr_back_selected);
			cr_back_dropHilited = RGB2gpColor(::GetSysColor(COLOR_HIGHLIGHT));
			cr_back_hover = get_color(cr_back_selected, -32);
			cr_back_alternate = get_color(cr_back, 16);

			cr_selected_border = cr_back_selected;
			cr_selected_border_inactive = cr_back_selected_inactive;

			cr_header_text = cr_text;
			cr_header_back = get_color(cr_back, 16);

			cr_percentage_bar.clear();
			cr_percentage_bar.push_back(get_color(cr_back, 32));
			cr_progress_active = cr_back_selected;

			cr_border_active = cr_back_selected;
			cr_border_inactive = get_weak_color(cr_back, 64);
			cr_button_back = cr_back_selected;
			cr_button_text = gRGB(46, 52, 64);
			cr_button_border = get_weak_color(cr_back, 40);
			cr_separator = get_weak_color(cr_back, 30);
			break;

		case color_theme_claude03:
			//light : bg #F5F7FB, alt #E5E7EB, accent #4F46E5 indigo-600, text #1F2937
			cr_title_text = gRGB(31, 41, 55);
			cr_title_back_active = gRGB(229, 231, 235);
			cr_title_back_inactive = cr_title_back_active;
			cr_sys_buttons_hover_back = get_color(cr_title_back_active, -16);
			cr_sys_buttons_down_back = get_color(cr_title_back_active, -28);

			cr_text = gRGB(31, 41, 55);
			cr_text_dim = compute_text_dim(cr_text, cr_back);
			cr_text_hover = Gdiplus::Color::White;
			cr_text_selected = Gdiplus::Color::White;
			cr_text_selected_inactive = cr_text;
			cr_text_dropHilited = Gdiplus::Color::White;

			cr_back = gRGB(245, 247, 251);
			cr_parent_back = cr_back;
			cr_back_selected = gRGB(79, 70, 229);
			cr_back_selected_inactive = get_color(cr_back, -24);
			cr_back_dropHilited = RGB2gpColor(::GetSysColor(COLOR_HIGHLIGHT));
			cr_back_hover = get_color(cr_back_selected, 32);
			cr_back_alternate = gRGB(229, 231, 235);

			cr_selected_border = cr_back_selected;
			cr_selected_border_inactive = cr_back_selected_inactive;

			cr_header_text = cr_text;
			cr_header_back = gRGB(229, 231, 235);

			cr_percentage_bar.clear();
			cr_percentage_bar.push_back(get_color(cr_back, -16));
			cr_progress_active = cr_back_selected;

			cr_border_active = cr_back_selected;
			cr_border_inactive = gRGB(229, 231, 235);
			cr_button_back = cr_back_selected;
			cr_button_text = Gdiplus::Color::White;
			cr_button_border = gRGB(229, 231, 235);
			cr_separator = gRGB(229, 231, 235);
			break;

		case color_theme_claude04:
			//dark : bg #161616, title #262626, accent #0F62FE (IBM Carbon blue), text #F4F4F4.
			cr_title_text = gRGB(244, 244, 244);
			cr_title_back_active = gRGB(38, 38, 38);
			cr_title_back_inactive = cr_title_back_active;
			cr_sys_buttons_hover_back = get_color(cr_title_back_active, 32);
			cr_sys_buttons_down_back = get_color(cr_title_back_active, 24);

			cr_text = gRGB(244, 244, 244);
			cr_text_dim = compute_text_dim(cr_text, cr_back);
			cr_text_hover = Gdiplus::Color::White;
			cr_text_selected = Gdiplus::Color::White;
			cr_text_selected_inactive = cr_text;
			cr_text_dropHilited = Gdiplus::Color::White;

			cr_back = gRGB(22, 22, 22);
			cr_parent_back = cr_back;
			cr_back_selected = gRGB(15, 98, 254);
			cr_back_selected_inactive = get_gray_color(cr_back_selected);
			cr_back_dropHilited = RGB2gpColor(::GetSysColor(COLOR_HIGHLIGHT));
			cr_back_hover = get_color(cr_back_selected, 32);
			cr_back_alternate = get_color(cr_back, 16);

			cr_selected_border = cr_back_selected;
			cr_selected_border_inactive = cr_back_selected_inactive;

			cr_header_text = cr_text;
			cr_header_back = get_color(cr_back, 16);

			cr_percentage_bar.clear();
			cr_percentage_bar.push_back(get_color(cr_back, 32));
			cr_progress_active = cr_back_selected;

			cr_border_active = cr_back_selected;
			cr_border_inactive = get_weak_color(cr_back, 64);
			cr_button_back = cr_back_selected;
			cr_button_text = Gdiplus::Color::White;
			cr_button_border = get_weak_color(cr_back, 40);
			cr_separator = get_weak_color(cr_back, 30);
			break;

		case color_theme_claude05:
			//light warm : bg #FAF7F2 cream, alt #E8E2D6, accent #B45309 rust orange, text #3F3A33.
			//= color_theme_claude 와 같은 팔레트 — 10종 시리즈 일관성 위해 별도 슬롯 유지.
			cr_title_text = gRGB(63, 58, 51);
			cr_title_back_active = gRGB(232, 226, 214);
			cr_title_back_inactive = cr_title_back_active;
			cr_sys_buttons_hover_back = get_color(cr_title_back_active, -16);
			cr_sys_buttons_down_back = get_color(cr_title_back_active, -28);

			cr_text = gRGB(63, 58, 51);
			cr_text_dim = compute_text_dim(cr_text, cr_back);
			cr_text_hover = Gdiplus::Color::White;
			cr_text_selected = Gdiplus::Color::White;
			cr_text_selected_inactive = cr_text;
			cr_text_dropHilited = Gdiplus::Color::White;

			cr_back = gRGB(250, 247, 242);
			cr_parent_back = cr_back;
			cr_back_selected = gRGB(180, 83, 9);
			cr_back_selected_inactive = get_color(cr_back, -24);
			cr_back_dropHilited = RGB2gpColor(::GetSysColor(COLOR_HIGHLIGHT));
			cr_back_hover = get_color(cr_back_selected, 32);
			cr_back_alternate = gRGB(232, 226, 214);

			cr_selected_border = cr_back_selected;
			cr_selected_border_inactive = cr_back_selected_inactive;

			cr_header_text = cr_text;
			cr_header_back = gRGB(232, 226, 214);

			cr_percentage_bar.clear();
			cr_percentage_bar.push_back(get_color(cr_back, -16));
			cr_progress_active = cr_back_selected;

			cr_border_active = cr_back_selected;
			cr_border_inactive = gRGB(232, 226, 214);
			cr_button_back = cr_back_selected;
			cr_button_text = Gdiplus::Color::White;
			cr_button_border = gRGB(232, 226, 214);
			cr_separator = gRGB(232, 226, 214);
			break;

		case color_theme_claude06:
			//dark slate : bg #0F172A slate-950, title #1E293B slate-800, accent #3B82F6 blue-500, text #E2E8F0 slate-200.
			cr_title_text = gRGB(226, 232, 240);
			cr_title_back_active = gRGB(30, 41, 59);
			cr_title_back_inactive = cr_title_back_active;
			cr_sys_buttons_hover_back = get_color(cr_title_back_active, 32);
			cr_sys_buttons_down_back = get_color(cr_title_back_active, 24);

			cr_text = gRGB(226, 232, 240);
			cr_text_dim = compute_text_dim(cr_text, cr_back);
			cr_text_hover = Gdiplus::Color::White;
			cr_text_selected = Gdiplus::Color::White;
			cr_text_selected_inactive = cr_text;
			cr_text_dropHilited = Gdiplus::Color::White;

			cr_back = gRGB(15, 23, 42);
			cr_parent_back = cr_back;
			cr_back_selected = gRGB(59, 130, 246);
			cr_back_selected_inactive = get_gray_color(cr_back_selected);
			cr_back_dropHilited = RGB2gpColor(::GetSysColor(COLOR_HIGHLIGHT));
			cr_back_hover = get_color(cr_back_selected, 32);
			cr_back_alternate = get_color(cr_back, 16);

			cr_selected_border = cr_back_selected;
			cr_selected_border_inactive = cr_back_selected_inactive;

			cr_header_text = cr_text;
			cr_header_back = get_color(cr_back, 16);

			cr_percentage_bar.clear();
			cr_percentage_bar.push_back(get_color(cr_back, 32));
			cr_progress_active = cr_back_selected;

			cr_border_active = cr_back_selected;
			cr_border_inactive = get_weak_color(cr_back, 64);
			cr_button_back = cr_back_selected;
			cr_button_text = Gdiplus::Color::White;
			cr_button_border = get_weak_color(cr_back, 40);
			cr_separator = get_weak_color(cr_back, 30);
			break;

		case color_theme_claude07:
			//light : bg #FFFFFF, alt #E5E7EB, accent #10B981 emerald-500, text #0F172A.
			cr_title_text = gRGB(15, 23, 42);
			cr_title_back_active = gRGB(229, 231, 235);
			cr_title_back_inactive = cr_title_back_active;
			cr_sys_buttons_hover_back = get_color(cr_title_back_active, -16);
			cr_sys_buttons_down_back = get_color(cr_title_back_active, -28);

			cr_text = gRGB(15, 23, 42);
			cr_text_dim = compute_text_dim(cr_text, cr_back);
			cr_text_hover = Gdiplus::Color::White;
			cr_text_selected = Gdiplus::Color::White;
			cr_text_selected_inactive = cr_text;
			cr_text_dropHilited = Gdiplus::Color::White;

			cr_back = Gdiplus::Color::White;
			cr_parent_back = cr_back;
			cr_back_selected = gRGB(16, 185, 129);
			cr_back_selected_inactive = get_color(cr_back, -24);
			cr_back_dropHilited = RGB2gpColor(::GetSysColor(COLOR_HIGHLIGHT));
			cr_back_hover = get_color(cr_back_selected, 32);
			cr_back_alternate = gRGB(229, 231, 235);

			cr_selected_border = cr_back_selected;
			cr_selected_border_inactive = cr_back_selected_inactive;

			cr_header_text = cr_text;
			cr_header_back = gRGB(229, 231, 235);

			cr_percentage_bar.clear();
			cr_percentage_bar.push_back(get_color(cr_back, -16));
			cr_progress_active = cr_back_selected;

			cr_border_active = cr_back_selected;
			cr_border_inactive = gRGB(229, 231, 235);
			cr_button_back = cr_back_selected;
			cr_button_text = Gdiplus::Color::White;
			cr_button_border = gRGB(229, 231, 235);
			cr_separator = gRGB(229, 231, 235);
			break;

		case color_theme_claude08:
			//dark purple : bg #1A1A1D, title #3A3A40, accent #A78BFA purple-400, text #F5F5F5.
			cr_title_text = gRGB(245, 245, 245);
			cr_title_back_active = gRGB(58, 58, 64);
			cr_title_back_inactive = cr_title_back_active;
			cr_sys_buttons_hover_back = get_color(cr_title_back_active, 32);
			cr_sys_buttons_down_back = get_color(cr_title_back_active, 24);

			cr_text = gRGB(245, 245, 245);
			cr_text_dim = compute_text_dim(cr_text, cr_back);
			cr_text_hover = Gdiplus::Color::White;
			cr_text_selected = Gdiplus::Color::White;
			cr_text_selected_inactive = cr_text;
			cr_text_dropHilited = Gdiplus::Color::White;

			cr_back = gRGB(26, 26, 29);
			cr_parent_back = cr_back;
			cr_back_selected = gRGB(167, 139, 250);
			cr_back_selected_inactive = get_gray_color(cr_back_selected);
			cr_back_dropHilited = RGB2gpColor(::GetSysColor(COLOR_HIGHLIGHT));
			cr_back_hover = get_color(cr_back_selected, 32);
			cr_back_alternate = get_color(cr_back, 16);

			cr_selected_border = cr_back_selected;
			cr_selected_border_inactive = cr_back_selected_inactive;

			cr_header_text = cr_text;
			cr_header_back = get_color(cr_back, 16);

			cr_percentage_bar.clear();
			cr_percentage_bar.push_back(get_color(cr_back, 32));
			cr_progress_active = cr_back_selected;

			cr_border_active = cr_back_selected;
			cr_border_inactive = get_weak_color(cr_back, 64);
			cr_button_back = cr_back_selected;
			cr_button_text = Gdiplus::Color::White;
			cr_button_border = get_weak_color(cr_back, 40);
			cr_separator = get_weak_color(cr_back, 30);
			break;

		case color_theme_claude09:
			//light : bg #FFFFFF, alt #E5E7EB, accent #1D4ED8 blue-700 (deep blue), text #111827.
			cr_title_text = gRGB(17, 24, 39);
			cr_title_back_active = gRGB(229, 231, 235);
			cr_title_back_inactive = cr_title_back_active;
			cr_sys_buttons_hover_back = get_color(cr_title_back_active, -16);
			cr_sys_buttons_down_back = get_color(cr_title_back_active, -28);

			cr_text = gRGB(17, 24, 39);
			cr_text_dim = compute_text_dim(cr_text, cr_back);
			cr_text_hover = Gdiplus::Color::White;
			cr_text_selected = Gdiplus::Color::White;
			cr_text_selected_inactive = cr_text;
			cr_text_dropHilited = Gdiplus::Color::White;

			cr_back = Gdiplus::Color::White;
			cr_parent_back = cr_back;
			cr_back_selected = gRGB(29, 78, 216);
			cr_back_selected_inactive = get_color(cr_back, -24);
			cr_back_dropHilited = RGB2gpColor(::GetSysColor(COLOR_HIGHLIGHT));
			cr_back_hover = get_color(cr_back_selected, 32);
			cr_back_alternate = gRGB(229, 231, 235);

			cr_selected_border = cr_back_selected;
			cr_selected_border_inactive = cr_back_selected_inactive;

			cr_header_text = cr_text;
			cr_header_back = gRGB(229, 231, 235);

			cr_percentage_bar.clear();
			cr_percentage_bar.push_back(get_color(cr_back, -16));
			cr_progress_active = cr_back_selected;

			cr_border_active = cr_back_selected;
			cr_border_inactive = gRGB(229, 231, 235);
			cr_button_back = cr_back_selected;
			cr_button_text = Gdiplus::Color::White;
			cr_button_border = gRGB(229, 231, 235);
			cr_separator = gRGB(229, 231, 235);
			break;

		case color_theme_linkmemine_se:
			//코이노 BI Color 문서 — LinkMeMine 3.0 SE.
			//BI 에서 1.0 과 같은 행에 묶여 4색 (#FD7E14, #222E3D, #0045FF, #ff7b22) 공유.
			//1.0 과 시각적 구분을 위해 SE 는 BI 의 보조 주황 #ff7b22 를 primary 로 채택.
			//이전 #BA5912 는 BI 4색 어느 것과도 일치 안 함 → 폐기.

			cr_title_text = Gdiplus::Color::White;
			cr_title_back_active = gRGB(255, 123, 34);        //#ff7b22 — BI 보조 주황 (SE 시그니처)
			cr_title_back_inactive = cr_title_back_active;
			cr_sys_buttons_hover_back = get_color(cr_title_back_active, 16);
			cr_sys_buttons_down_back = get_color(cr_title_back_active, -16);

			cr_text = gRGB(34, 46, 61);                       //#222E3D — BI dark navy-gray (1.0 과 공유)
			cr_text_dim = compute_text_dim(cr_text, cr_back);
			cr_text_hover = Gdiplus::Color::White;            //hover bg 가 logo color +32 (dark) — white 강제
			cr_text_selected = Gdiplus::Color::White;
			cr_text_selected_inactive = get_color(cr_text, 32);
			cr_text_dropHilited = Gdiplus::Color::White;

			cr_back = Gdiplus::Color::White;
			cr_parent_back = cr_back;
			cr_back_selected = cr_title_back_active;
			cr_back_selected_inactive = get_color(cr_back_selected, 160);
			cr_back_dropHilited = RGB2gpColor(::GetSysColor(COLOR_HIGHLIGHT));
			cr_back_hover = get_color(cr_title_back_active, 32);
			cr_back_alternate = get_color(cr_back, 8);

			cr_selected_border = cr_text;                     //#222E3D
			cr_selected_border_inactive = cr_back_selected_inactive;

			cr_header_text = cr_text;
			cr_header_back = get_color(cr_back, -16);
			cr_percentage_bar.clear();
			cr_percentage_bar.push_back(get_color(cr_back, 32));
			cr_progress_active = gRGB(0, 69, 255);                   //#0045FF — BI 홈페이지 strong blue (accent)

			cr_border_active = cr_title_back_active;          //logo orange (CornflowerBlue 폐기)
			cr_border_inactive = Gdiplus::Color::LightGray;
			cr_button_back = cr_title_back_inactive;
			cr_button_text = cr_title_text;
			cr_button_border = get_weak_color(cr_back, 40);
			cr_separator = get_weak_color(cr_back, 30);
			break;

		case color_theme_anysupport :
			//코이노 BI Color 문서 — ANYSUPPORT 행 매핑.
			//  logo color    #309AC0 : medium teal-blue 시그니처
			//  logo 보조      #0D2947 : dark navy (text/border accent)
			//  홈페이지       #145AFF : strong blue (accent only)
			//  홈페이지 보조   #0F1F3D : dark navy variant
			//이전 #3B455B (짙은 청회색) 은 BI 와 다른 톤 → 폐기.

			cr_title_text = Gdiplus::Color::White;
			cr_title_back_active = gRGB(48, 154, 192);        //#309AC0 — BI logo color
			cr_title_back_inactive = cr_title_back_active;
			cr_sys_buttons_hover_back = get_color(cr_title_back_active, 16);
			cr_sys_buttons_down_back = get_color(cr_title_back_active, -16);

			cr_text = gRGB(13, 41, 71);                       //#0D2947 — BI logo 보조 dark navy
			cr_text_dim = compute_text_dim(cr_text, cr_back);
			cr_text_hover = Gdiplus::Color::White;            //hover bg 가 logo color +32 (dark) — white 강제
			cr_text_selected = Gdiplus::Color::White;
			cr_text_selected_inactive = get_color(cr_text, 32);
			cr_text_dropHilited = Gdiplus::Color::White;

			cr_back = Gdiplus::Color::White;
			cr_parent_back = cr_back;
			cr_back_selected = cr_title_back_active;
			cr_back_selected_inactive = get_color(cr_back_selected, 160);
			cr_back_dropHilited = RGB2gpColor(::GetSysColor(COLOR_HIGHLIGHT));
			cr_back_hover = get_color(cr_title_back_active, 32);
			cr_back_alternate = get_color(cr_back, 8);

			cr_selected_border = cr_text;                     //#0D2947
			cr_selected_border_inactive = cr_back_selected_inactive;

			cr_header_text = cr_text;
			cr_header_back = get_color(cr_back, -16);

			cr_percentage_bar.clear();
			cr_percentage_bar.push_back(get_color(cr_back, 32));
			cr_progress_active = gRGB(20, 90, 255);                  //#145AFF — BI 홈페이지 strong blue

			cr_border_active = cr_title_back_active;          //logo blue (CornflowerBlue 폐기)
			cr_border_inactive = Gdiplus::Color::LightGray;
			cr_button_back = cr_title_back_inactive;
			cr_button_text = cr_title_text;
			cr_button_border = get_weak_color(cr_back, 40);
			cr_separator = get_weak_color(cr_back, 30);
			break;

		case color_theme_helpu:
			//코이노 원격서비스 BI Color 문서 (Google Slides, 디자이너 정리) — helpU 행 매핑.
			//  logo color   #20B2AE  : helpU 시그니처 teal (= 홈페이지 color 와 동일)
			//  logo 보조     #12183A  : very dark navy — text/border accent
			//  보조 회색     #9F9F9F  : mid gray — secondary text / border-inactive
			//  light bg     #F4F5F8  : very light gray — header/divider 배경
			//웹페이지(helpu.co.kr)의 #2a969d 는 BI 의 "홈페이지 color" 가 아닌, helpu.css 의 후속 변형이라
			//BI 가 source-of-truth. (BI 표에선 logo/홈페이지 둘 다 #20B2AE 로 통일.)

			cr_title_text = gRGB(18, 24, 58);                //#12183A — BI dark navy
			cr_title_back_active = gRGB(32, 178, 174);       //#20B2AE — BI logo color
			cr_title_back_inactive = cr_title_back_active;
			cr_sys_buttons_hover_back = get_color(cr_title_back_active, 16);
			cr_sys_buttons_down_back = get_color(cr_title_back_active, -16);

			cr_text = cr_title_text;                         //본문도 동일 dark navy
			cr_text_dim = compute_text_dim(cr_text, cr_back);               //#9F9F9F — BI gray
			cr_text_hover = Gdiplus::Color::White;           //hover bg 가 logo teal +32 — white 강제
			cr_text_selected = Gdiplus::Color::White;        //selected = logo teal 위 white
			cr_text_selected_inactive = get_color(cr_text, 32);
			cr_text_dropHilited = Gdiplus::Color::White;

			cr_back = Gdiplus::Color::White;
			cr_parent_back = cr_back;
			cr_back_selected = cr_title_back_active;         //selected = logo teal
			cr_back_selected_inactive = get_color(cr_back_selected, 160);
			cr_back_dropHilited = RGB2gpColor(::GetSysColor(COLOR_HIGHLIGHT));
			cr_back_hover = get_color(cr_title_back_active, 32);
			cr_back_alternate = gRGB(244, 245, 248);         //#F4F5F8 — BI light gray bg

			cr_selected_border = cr_title_text;              //#12183A — BI dark navy
			cr_selected_border_inactive = cr_back_selected_inactive;

			cr_header_text = cr_title_text;
			cr_header_back = gRGB(244, 245, 248);            //#F4F5F8 — BI light gray bg

			cr_percentage_bar.clear();
			cr_percentage_bar.push_back(get_color(cr_back, 32));
			cr_progress_active = cr_title_back_active;              //logo teal (RoyalBlue 폐기)

			cr_border_active = cr_title_back_active;         //logo teal (CornflowerBlue 폐기)
			cr_border_inactive = gRGB(159, 159, 159);        //#9F9F9F — BI gray
			cr_button_back = cr_title_back_inactive;
			cr_button_text = cr_title_text;
			cr_button_border = get_weak_color(cr_back, 40);
			cr_separator = get_weak_color(cr_back, 30);
			break;

		case color_theme_pcanypro:
			//코이노 BI Color 문서 — PCANYPRO 행 매핑.
			//  logo color    #F5A419 : golden orange 시그니처 (BI 명시 색 1개)
			//BI 에 보조 색 명시 없음 — text/border 는 다른 제품과 일관된 dark navy/gray 톤 사용.
			//이전 #D68108 은 BI 와 한 톤 다름 → 폐기.

			cr_title_text = Gdiplus::Color::White;
			cr_title_back_active = gRGB(245, 164, 25);        //#F5A419 — BI logo color
			cr_title_back_inactive = cr_title_back_active;
			cr_sys_buttons_hover_back = get_color(cr_title_back_active, 16);
			cr_sys_buttons_down_back = get_color(cr_title_back_active, -16);

			cr_text = gRGB(34, 46, 61);                       //BI 보조 명시 없음 — 일반 dark navy
			cr_text_dim = compute_text_dim(cr_text, cr_back);
			cr_text_hover = Gdiplus::Color::White;            //hover bg 가 logo orange +32 — white 강제
			cr_text_selected = Gdiplus::Color::White;
			cr_text_selected_inactive = get_color(cr_text, 32);
			cr_text_dropHilited = Gdiplus::Color::White;

			cr_back = Gdiplus::Color::White;
			cr_parent_back = cr_back;
			cr_back_selected = cr_title_back_active;
			cr_back_selected_inactive = get_color(cr_back_selected, 160);
			cr_back_dropHilited = RGB2gpColor(::GetSysColor(COLOR_HIGHLIGHT));
			cr_back_hover = get_color(cr_title_back_active, 32);
			cr_back_alternate = get_color(cr_back, 8);

			cr_selected_border = cr_text;
			cr_selected_border_inactive = cr_back_selected_inactive;

			cr_header_text = cr_text;
			cr_header_back = get_color(cr_back, -16);

			cr_percentage_bar.clear();
			cr_percentage_bar.push_back(get_color(cr_back, 32));
			cr_progress_active = cr_title_back_active;               //logo orange (RoyalBlue 폐기)

			cr_border_active = cr_title_back_active;          //logo orange (CornflowerBlue 폐기)
			cr_border_inactive = Gdiplus::Color::LightGray;
			cr_button_back = cr_title_back_inactive;
			cr_button_text = cr_title_text;
			cr_button_border = get_weak_color(cr_back, 40);
			cr_separator = get_weak_color(cr_back, 30);
			break;

		case color_theme_dark_gray :
			cr_title_text			= Gdiplus::Color::LightGray;
			cr_title_back_active	= gGRAY(24);
			cr_title_back_inactive	= gGRAY(24);
			cr_sys_buttons_hover_back = get_color(cr_title_back_active, 32);
			cr_sys_buttons_down_back = get_color(cr_title_back_active, 24);

			cr_text					= Gdiplus::Color(255, 192, 192, 192);
			cr_text_dim				= compute_text_dim(cr_text, cr_back);
			cr_text_hover			= cr_text;
			cr_text_selected		= Gdiplus::Color(255, 241, 241, 241);
			cr_text_selected_inactive = cr_text_selected;
			cr_text_dropHilited		= white;

			cr_back					= Gdiplus::Color(255, 64, 64, 64);
			cr_parent_back			= cr_back;
			cr_back_selected		= get_color(cr_back, -16);
			cr_back_selected_inactive = get_gray_color(cr_back_selected);
			cr_back_dropHilited		= RGB2gpColor(::GetSysColor(COLOR_HIGHLIGHT));
			cr_back_hover			= get_weak_color(cr_back, 16);
			cr_back_alternate		= get_color(cr_back, 8);

			cr_selected_border		= Gdiplus::Color(255, 128, 128, 128);
			cr_selected_border_inactive = cr_back_selected_inactive;

			cr_header_text			= get_color(cr_text, -16);
			cr_header_back			= get_color(cr_back, -16);

			cr_percentage_bar.clear();
			cr_percentage_bar.push_back(get_color(cr_back, 32));
			cr_progress_active = get_color(cr_back, 96);

			cr_border_active		= get_weak_color(cr_back, 128);
			cr_border_inactive		= get_weak_color(cr_back, 64);
			cr_button_back = cr_title_back_inactive;
			cr_button_text = cr_title_text;
			cr_button_border = get_weak_color(cr_back, 40);
			cr_separator = get_weak_color(cr_back, 30);
			break;

		case color_theme_dark :
			cr_text					= Gdiplus::Color(255, 212, 212, 212);
			cr_text_dim				= compute_text_dim(cr_text, cr_back);
			cr_text_hover			= cr_text;
			cr_text_selected		= Gdiplus::Color(255, 241, 241, 241);
			cr_text_selected_inactive = cr_text_selected;
			cr_text_dropHilited		= white;

			cr_back					= Gdiplus::Color(255, 37, 37, 38);
			cr_parent_back			= cr_back;
			cr_back_selected		= Gdiplus::Color(255, 0, 120, 215);
			cr_back_selected_inactive = get_gray_color(cr_back_selected);
			cr_back_dropHilited		= RGB2gpColor(::GetSysColor(COLOR_HIGHLIGHT));
			cr_back_hover			= get_color(cr_back_selected, 48);
			cr_back_alternate		= get_color(cr_back, 8);

			cr_title_text			= Gdiplus::Color::LightGray;
			cr_title_back_active	= gGRAY(16);
			cr_sys_buttons_hover_back = get_color(cr_title_back_active, 32);
			cr_sys_buttons_down_back = get_color(cr_title_back_active, 24);

			cr_selected_border		= cr_back_selected;
			cr_selected_border_inactive = cr_back_selected_inactive;

			cr_header_back			= get_color(cr_back, 16);
			cr_header_text			= get_color(cr_text, -32);

			cr_percentage_bar.clear();
			cr_percentage_bar.push_back(get_color(cr_back, 32));
			cr_progress_active = get_color(cr_back, 96);

			//dark 배경 위에 LightGray 는 거의 흰 띠로 보여 버튼 테두리가 부자연스러웠음.
		//dark_gray 와 동일하게 cr_back 기준으로 가산해 어두운 톤 안에서 적절히 보이게 한다.
		//active/inactive 차이도 작게 두어 dialog 활성 전환 시 외관 jolt 최소화.
		cr_border_active   = get_weak_color(cr_back, 128);
			cr_border_inactive = get_weak_color(cr_back, 64);
			//dark 테마는 위에서 cr_title_back_inactive 를 세팅하지 않아 stale 위험 — 여기서 active 와 동일 톤으로 확정.
			cr_title_back_inactive = cr_title_back_active;
			cr_button_back = cr_title_back_inactive;
			cr_button_text = cr_title_text;
			cr_button_border = get_weak_color(cr_back, 40);
			cr_separator = get_weak_color(cr_back, 30);
			break;

		case color_theme_popup_folder_list:
			cr_text.SetFromCOLORREF(::GetSysColor(COLOR_BTNTEXT));
			cr_text_selected		= cr_text;// ::GetSysColor(COLOR_HIGHLIGHTTEXT);
			cr_text_selected_inactive.SetFromCOLORREF(::GetSysColor(COLOR_INACTIVECAPTIONTEXT));
			cr_text_hover			= cr_text;

			//Windows Explorer 의 주소표시줄 폴더 팝업과 동일하게 일반 윈도우 배경(흰색 default) 사용.
			//이전에 COLOR_3DFACE (회색) 였으나 사용자 피드백으로 COLOR_WINDOW 로 정정.
			cr_back.SetFromCOLORREF(::GetSysColor(COLOR_WINDOW));
			cr_back_selected		= Gdiplus::Color(255, 204, 232, 255);// ::GetSysColor(COLOR_HIGHLIGHT);
			cr_back_selected_inactive.SetFromCOLORREF(::GetSysColor(COLOR_HIGHLIGHT));
			cr_back_hover			= Gdiplus::Color(255, 195, 222, 245);

			cr_selected_border		= Gdiplus::Color(255, 153, 209, 255);
			cr_selected_border_inactive = cr_back_selected_inactive;

			cr_border_active = Gdiplus::Color::LightGray;
			cr_border_inactive = Gdiplus::Color::LightGray;
			cr_separator = get_weak_color(cr_back, 30);
			break;

		case color_theme_white :
			cr_text = RGB2gpColor(::GetSysColor(COLOR_BTNTEXT));
			cr_text_dim = compute_text_dim(cr_text, cr_back);// get_color(cr_text, 32);
			cr_text_hover = cr_text;
			cr_text_selected = cr_text;// RGB2gpColor(::GetSysColor(COLOR_HIGHLIGHTTEXT));;
			cr_text_selected_inactive = cr_text_selected;
			cr_text_dropHilited = RGB2gpColor(::GetSysColor(COLOR_HIGHLIGHTTEXT));

			//컨트롤 종류에 따라 기본 배경색이 다르다.
			cr_back = Gdiplus::Color::White;
			cr_parent_back = cr_back;
			cr_back_selected = gRGB(204, 235, 255);//RGB2gpColor(::GetSysColor(COLOR_HIGHLIGHT));
			cr_back_selected_inactive = get_gray_color(cr_back_selected);
			cr_back_dropHilited = get_sys_color(COLOR_HIGHLIGHT);
			cr_back_hover = Gdiplus::Color(255, 229, 243, 255);
			cr_back_alternate = get_color(cr_back, -12);


			cr_title_text = Gdiplus::Color::Black;
			cr_title_back_active = Gdiplus::Color::LightGray;
			//cr_title_back_inactive 누락 시 이전 테마 값이 stale 로 유지돼 title bar / X 버튼 / OK 버튼 색이 어긋남.
			//white 테마는 title 도 같은 LightGray 톤으로 통일 (active==inactive — 색차 거의 없는 minimal 테마).
			cr_title_back_inactive = cr_title_back_active;
			cr_sys_buttons_hover_back = get_color(cr_title_back_active, 16);
			cr_sys_buttons_down_back = get_color(cr_title_back_active, -16);

			//CVtListCtrlEx에서 사용되는 컬러
			cr_header_text = get_sys_color(COLOR_BTNTEXT);
			cr_header_back = get_sys_color(COLOR_3DFACE);

			cr_percentage_bar.clear();
			cr_percentage_bar.push_back(Gdiplus::Color::RoyalBlue);// gGRAY(192));
			cr_progress_active = Gdiplus::Color::RoyalBlue;// Gdiplus::Color(255, 49, 108, 244);

			cr_selected_border = gRGB(153, 209, 255);
			cr_selected_border_inactive = cr_back_selected_inactive;

			cr_border_active = Gdiplus::Color::CornflowerBlue;
			cr_border_inactive = Gdiplus::Color::LightGray;
			cr_button_back = cr_title_back_inactive;
			cr_button_text = cr_title_text;
			cr_button_border = get_weak_color(cr_back, 40);
			cr_separator = get_weak_color(cr_back, 30);
			break;

		case color_theme_gray:
			cr_text = gRGB(64, 64, 64);
			cr_text_dim = compute_text_dim(cr_text, cr_back);
			cr_text_hover = cr_text;
			cr_text_selected = cr_text;
			cr_text_selected_inactive = cr_text_selected;
			cr_text_dropHilited = RGB2gpColor(::GetSysColor(COLOR_HIGHLIGHTTEXT));

			cr_back = Gdiplus::Color::White;
			cr_parent_back = cr_back;
			cr_back_selected = get_color(cr_back, -24);
			cr_back_selected_inactive = get_gray_color(cr_back_selected);
			cr_back_dropHilited = get_sys_color(COLOR_HIGHLIGHT);
			cr_back_hover = get_color(cr_back, -16);
			cr_back_alternate = get_color(cr_back, -12);

			cr_title_text = Gdiplus::Color::Black;
			cr_title_back_active = Gdiplus::Color::LightGray;
			//cr_title_back_inactive 누락 시 이전 테마 값이 stale 로 유지돼 title bar / X 버튼 / OK 버튼 색이 어긋남.
			cr_title_back_inactive = cr_title_back_active;
			cr_sys_buttons_hover_back = get_color(cr_title_back_active, 16);
			cr_sys_buttons_down_back = get_color(cr_title_back_active, -16);

			//CVtListCtrlEx에서 사용되는 컬러
			cr_header_text = get_sys_color(COLOR_BTNTEXT);
			cr_header_back = get_sys_color(COLOR_3DFACE);

			cr_percentage_bar.clear();
			cr_percentage_bar.push_back(gGRAY(192));
			cr_progress_active = gGRAY(128);

			cr_selected_border = gRGB(153, 209, 255);
			cr_selected_border_inactive = cr_back_selected_inactive;

			cr_border_active = Gdiplus::Color::CornflowerBlue;
			cr_border_inactive = Gdiplus::Color::LightGray;
			cr_button_back = cr_title_back_inactive;
			cr_button_text = cr_title_text;
			cr_button_border = get_weak_color(cr_back, 40);
			cr_separator = get_weak_color(cr_back, 30);
			break;

		//세피아 — 빛바랜 종이/사진 톤. 따뜻한 크림 배경 + 갈색 글자 (light 테마 계열).
		case color_theme_sepia:
			cr_text = gRGB(80, 60, 42);					//dark sepia brown
			cr_text_dim = compute_text_dim(cr_text, cr_back);
			cr_text_hover = cr_text;
			cr_text_selected = cr_text;
			cr_text_selected_inactive = cr_text_selected;
			cr_text_dropHilited = cr_text;

			cr_back = gRGB(244, 236, 219);				//aged paper
			cr_parent_back = cr_back;
			cr_back_selected = gRGB(222, 196, 150);		//warm tan highlight
			cr_back_selected_inactive = get_gray_color(cr_back_selected);
			cr_back_dropHilited = cr_back_selected;
			cr_back_hover = get_color(cr_back, -12);
			cr_back_alternate = get_color(cr_back, -8);

			cr_title_text = cr_text;
			cr_title_back_active = gRGB(224, 208, 178);	//한 톤 진한 세피아
			cr_title_back_inactive = cr_title_back_active;
			cr_sys_buttons_hover_back = get_color(cr_title_back_active, -16);
			cr_sys_buttons_down_back = get_color(cr_title_back_active, -28);

			cr_header_text = cr_text;
			cr_header_back = cr_title_back_active;

			cr_percentage_bar.clear();
			cr_percentage_bar.push_back(gRGB(150, 110, 70));
			cr_progress_active = gRGB(150, 110, 70);

			cr_selected_border = gRGB(190, 160, 110);
			cr_selected_border_inactive = cr_back_selected_inactive;

			cr_border_active = gRGB(160, 125, 80);		//sepia brown accent
			cr_border_inactive = gRGB(206, 190, 160);	//light sepia
			cr_button_back = cr_title_back_inactive;
			cr_button_text = cr_title_text;
			cr_button_border = get_weak_color(cr_back, 40);
			cr_separator = get_weak_color(cr_back, 30);
			break;

		//--- Notepad++ GlobalStyles 추출 import 테마 (2026-05-24). 인자: 본문 bg, fg, 선택 bg, gutter fg, gutter bg ---
		case color_theme_zenburn:
			set_theme_from_editor_palette(gRGB(0x3F,0x3F,0x3F), gRGB(0xDC,0xDC,0xCC), gRGB(0x58,0x58,0x58), gRGB(0x8A,0x8A,0x8A), gRGB(0x0C,0x0C,0x0C));
			break;
		case color_theme_bespin:
			set_theme_from_editor_palette(gRGB(0x2A,0x21,0x1C), gRGB(0xBD,0xAE,0x9D), gRGB(0x83,0x67,0x5A), gRGB(0xE5,0xC1,0x38), gRGB(0x4C,0x4A,0x41));
			break;
		case color_theme_black_board:
			set_theme_from_editor_palette(gRGB(0x0C,0x10,0x21), gRGB(0xF8,0xF8,0xF8), gRGB(0x25,0x3B,0x76), gRGB(0xEE,0xEE,0xEC), gRGB(0x2E,0x34,0x36));
			break;
		case color_theme_choco:
			set_theme_from_editor_palette(gRGB(0x1A,0x0F,0x0B), gRGB(0xC3,0xBE,0x98), gRGB(0x37,0x20,0x17), gRGB(0xEE,0xEE,0xEC), gRGB(0x2E,0x34,0x36));
			break;
		case color_theme_danslerush_dark:
			set_theme_from_editor_palette(gRGB(0x2E,0x2E,0x2E), gRGB(0xC7,0xC7,0xC7), gRGB(0x4D,0x4D,0x4D), gRGB(0x8F,0x8F,0x8F), gRGB(0x36,0x36,0x36));
			break;
		case color_theme_dark_mode_default:
			set_theme_from_editor_palette(gRGB(0x3F,0x3F,0x3F), gRGB(0xDC,0xDC,0xCC), gRGB(0x58,0x58,0x58), gRGB(0x8A,0x8A,0x8A), gRGB(0x0C,0x0C,0x0C));
			break;
		case color_theme_deep_black:
			set_theme_from_editor_palette(gRGB(0x00,0x00,0x00), gRGB(0xFF,0xFF,0xFF), gRGB(0x66,0x99,0xCC), gRGB(0xC0,0xC0,0xC0), gRGB(0x33,0x33,0x33));
			break;
		case color_theme_hello_kitty:
			set_theme_from_editor_palette(gRGB(0xFF,0xB0,0xFF), gRGB(0x00,0x00,0x00), gRGB(0xFF,0xD5,0xFF), gRGB(0xFF,0xFF,0xFF), gRGB(0xFF,0x80,0xFF));
			break;
		case color_theme_hot_fudge_sundae:
			set_theme_from_editor_palette(gRGB(0x2B,0x0F,0x01), gRGB(0xB7,0x97,0x5D), gRGB(0x58,0x58,0x58), gRGB(0x8B,0x64,0x2B), gRGB(0x43,0x25,0x0B));
			break;
		case color_theme_khaki:
			set_theme_from_editor_palette(gRGB(0xD7,0xD7,0xAF), gRGB(0x5F,0x5F,0x00), gRGB(0xD7,0xFF,0x87), gRGB(0x5F,0x5F,0x00), gRGB(0xAF,0xAF,0x87));
			break;
		case color_theme_mono_industrial:
			set_theme_from_editor_palette(gRGB(0x22,0x2C,0x28), gRGB(0xFF,0xFF,0xFF), gRGB(0x91,0x99,0x94), gRGB(0xEE,0xEE,0xEC), gRGB(0x2E,0x34,0x36));
			break;
		case color_theme_monokai:
			set_theme_from_editor_palette(gRGB(0x27,0x28,0x22), gRGB(0xF8,0xF8,0xF2), gRGB(0x49,0x48,0x3E), gRGB(0xEE,0xEE,0xEC), gRGB(0x2E,0x34,0x36));
			break;
		case color_theme_mossy_lawn:
			set_theme_from_editor_palette(gRGB(0x58,0x69,0x3D), gRGB(0xF2,0xC4,0x76), gRGB(0x8B,0x67,0x33), gRGB(0x60,0x3D,0x13), gRGB(0x7E,0x8A,0x28));
			break;
		case color_theme_navajo:
			set_theme_from_editor_palette(gRGB(0xBA,0x9C,0x80), gRGB(0x00,0x00,0x00), gRGB(0xBC,0xBC,0xBC), gRGB(0x00,0x00,0x00), gRGB(0x80,0x80,0x80));
			break;
		case color_theme_obsidian:
			set_theme_from_editor_palette(gRGB(0x29,0x31,0x34), gRGB(0xE0,0xE2,0xE4), gRGB(0x40,0x4E,0x51), gRGB(0x81,0x96,0x9A), gRGB(0x3F,0x4B,0x4E));
			break;
		case color_theme_plastic_code_wrap:
			set_theme_from_editor_palette(gRGB(0x0B,0x16,0x1D), gRGB(0xF8,0xF8,0xF8), gRGB(0x16,0x2E,0x3D), gRGB(0xEE,0xEE,0xEC), gRGB(0x2E,0x34,0x36));
			break;
		case color_theme_ruby_blue:
			set_theme_from_editor_palette(gRGB(0x11,0x24,0x35), gRGB(0xFF,0xFF,0xFF), gRGB(0x00,0x00,0xFF), gRGB(0xFF,0xFF,0xFF), gRGB(0x1F,0x46,0x61));
			break;
		case color_theme_solarized_light:
			set_theme_from_editor_palette(gRGB(0xFD,0xF6,0xE3), gRGB(0x65,0x7B,0x83), gRGB(0x07,0x36,0x42), gRGB(0x93,0xA1,0xA1), gRGB(0xEE,0xE8,0xD5));
			break;
		case color_theme_solarized:
			set_theme_from_editor_palette(gRGB(0x00,0x2B,0x36), gRGB(0x83,0x94,0x96), gRGB(0xEE,0xE8,0xD5), gRGB(0x58,0x6E,0x75), gRGB(0x07,0x36,0x42));
			break;
		case color_theme_twilight:
			set_theme_from_editor_palette(gRGB(0x14,0x14,0x14), gRGB(0xF8,0xF8,0xF8), gRGB(0x3E,0x3E,0x3E), gRGB(0xEE,0xEE,0xEC), gRGB(0x2E,0x34,0x36));
			break;
		case color_theme_vibrant_ink:
			set_theme_from_editor_palette(gRGB(0x00,0x00,0x00), gRGB(0xFF,0xFF,0xFF), gRGB(0x66,0x99,0xCC), gRGB(0xE4,0xE4,0xE4), gRGB(0x33,0x33,0x33));
			break;
		case color_theme_vim_dark_blue:
			set_theme_from_editor_palette(gRGB(0x00,0x00,0x40), gRGB(0xFF,0xFF,0xBF), gRGB(0x20,0x50,0xD0), gRGB(0xFF,0xFF,0xFF), gRGB(0x00,0x00,0x40));
			break;

		case color_theme_windows:
			//현재 윈도우 테마의 시스템 색(GetSysColor)을 그대로 반영한다. default 는 curated Win11 룩이라
			//테두리/선택색 등을 하드코딩하지만, 이 테마는 라이브 OS 색(accent 포함 — Win11 의 COLOR_HIGHLIGHT
			//= 사용자 accent)을 그대로 쓴다. control-kind 별 배경 분기는 default 와 동일.
			cr_text					= get_sys_color(COLOR_WINDOWTEXT);
			cr_text_dim				= compute_text_dim(cr_text, cr_back);
			cr_text_hover			= cr_text;
			cr_text_selected		= get_sys_color(COLOR_HIGHLIGHTTEXT);
			cr_text_selected_inactive = cr_text;
			cr_text_dropHilited		= get_sys_color(COLOR_HIGHLIGHTTEXT);

			if (m_parent && (m_parent->IsKindOf(RUNTIME_CLASS(CListCtrl)) ||
				m_parent->IsKindOf(RUNTIME_CLASS(CTreeCtrl)) ||
				m_parent->IsKindOf(RUNTIME_CLASS(CListBox)) ||
				m_parent->IsKindOf(RUNTIME_CLASS(CComboBox)) ||
				m_parent->IsKindOf(RUNTIME_CLASS(CEdit)) ||
				m_parent->IsKindOf(RUNTIME_CLASS(CRichEditCtrl))))
				cr_back = get_sys_color(COLOR_WINDOW);
			else
				cr_back = get_sys_color(COLOR_BTNFACE);
			cr_parent_back			= cr_back;
			cr_back_selected		= get_sys_color(COLOR_HIGHLIGHT);
			cr_back_selected_inactive = get_sys_color(COLOR_BTNFACE);
			cr_back_dropHilited		= get_sys_color(COLOR_HIGHLIGHT);
			cr_back_hover			= get_color(cr_back, get_sys_color(COLOR_HIGHLIGHT), 0.18);
			cr_back_alternate		= get_color(cr_back, -12);

			cr_edit_text			= get_sys_color(COLOR_WINDOWTEXT);
			cr_edit_back			= get_sys_color(COLOR_WINDOW);

			cr_title_text			= get_sys_color(COLOR_CAPTIONTEXT);
			//active/inactive 두 필드는 향후 창 포커스 상태별 타이틀바 렌더링용으로 스키마에 남겨두되, 현재 앱은
			//포커스 구분 없이 inactive 만 쓴다(TitleDlg 가 타이틀바를 항상 cr_title_back_inactive 로 칠함). 따라서
			//시스템 버튼(cr_title_back_active 사용)과 어긋나지 않도록 두 필드 모두 inactive 값으로 통일한다.
			//(Win10/11 은 DWM 이 타이틀바를 그려 COLOR_ACTIVECAPTION 이 실제 타이틀바와 무관한 legacy 값이라 부적합.)
			cr_title_back_inactive	= get_sys_color(COLOR_INACTIVECAPTION);
			cr_title_back_active	= cr_title_back_inactive;
			cr_sys_buttons_hover_back = get_color(cr_title_back_active, -16);
			cr_sys_buttons_down_back = get_color(cr_title_back_active, -28);

			cr_header_text			= get_sys_color(COLOR_BTNTEXT);
			cr_header_back			= get_sys_color(COLOR_3DFACE);

			cr_percentage_bar.clear();
			cr_percentage_bar.push_back(get_sys_color(COLOR_HIGHLIGHT));
			cr_progress_active				= get_sys_color(COLOR_HIGHLIGHT);

			cr_selected_border		= get_sys_color(COLOR_HIGHLIGHT);
			cr_selected_border_inactive = get_sys_color(COLOR_BTNSHADOW);

			cr_border_active		= get_sys_color(COLOR_HIGHLIGHT);
			cr_border_inactive		= get_sys_color(COLOR_BTNSHADOW);
			cr_button_back			= get_sys_color(COLOR_BTNFACE);
			cr_button_text			= get_sys_color(COLOR_BTNTEXT);
			cr_button_border		= get_sys_color(COLOR_BTNSHADOW);
			cr_separator			= get_sys_color(COLOR_BTNSHADOW);
			break;

		default: //case color_theme_default :
			cr_text					= get_sys_color(COLOR_BTNTEXT);
			cr_text_dim				= compute_text_dim(cr_text, cr_back);
			cr_text_hover			= cr_text;
			cr_text_selected		= cr_text;// RGB2gpColor(::GetSysColor(COLOR_HIGHLIGHTTEXT));;
			cr_text_selected_inactive = cr_text_selected;
			cr_text_dropHilited		= get_sys_color(COLOR_HIGHLIGHTTEXT);

			//컨트롤 종류에 따라 기본 배경색이 다르다 (List/Tree/Edit 류 = COLOR_WINDOW, 그 외 = COLOR_BTNFACE).
			//m_parent 는 강제 — NULL 이면 잘못된 선언 (`CSCColorTheme m_theme;`) 이므로 즉시 알림.
			//올바른 선언: `CSCColorTheme m_theme = CSCColorTheme(this);` (this 로 컨트롤 종류 판별).
			if (m_parent == NULL)
			{
				TRACE(_T("CSCColorTheme::set_color_theme(default): m_parent is NULL. Declare as `CSCColorTheme m_theme = CSCColorTheme(this);` to enable control-kind detection.\n"));
				ASSERT(FALSE);
			}

			if (m_parent->IsKindOf(RUNTIME_CLASS(CListCtrl)) ||
				m_parent->IsKindOf(RUNTIME_CLASS(CTreeCtrl)) ||
				m_parent->IsKindOf(RUNTIME_CLASS(CListBox)) ||
				m_parent->IsKindOf(RUNTIME_CLASS(CComboBox)) ||
				m_parent->IsKindOf(RUNTIME_CLASS(CEdit)) ||
				m_parent->IsKindOf(RUNTIME_CLASS(CRichEditCtrl)))
			{
				cr_back = RGB2gpColor(::GetSysColor(COLOR_WINDOW));
			}
			else
			{
				cr_back = RGB2gpColor(::GetSysColor(COLOR_BTNFACE));
			}
			cr_parent_back			= cr_back;
			cr_back_selected		= gRGB(204, 235, 255);//RGB2gpColor(::GetSysColor(COLOR_HIGHLIGHT));
			cr_back_selected_inactive = get_gray_color(cr_back_selected);
			cr_back_dropHilited		= get_sys_color(COLOR_HIGHLIGHT);
			cr_back_hover			= Gdiplus::Color(255, 229, 243, 255);
			cr_back_alternate		= get_color(cr_back, -12);

			cr_edit_text			= cr_text;
			cr_edit_back			= Gdiplus::Color::White;

			cr_title_text			= Gdiplus::Color(32, 32, 32);// get_sys_color(COLOR_CAPTIONTEXT);
			cr_title_back_active	= Gdiplus::Color(239, 244, 249);// get_sys_color(COLOR_ACTIVECAPTION);
			cr_title_back_inactive	= Gdiplus::Color(239, 244, 249);// get_sys_color(COLOR_INACTIVECAPTION);
			cr_sys_buttons_hover_back = get_color(cr_title_back_active, 16);
			cr_sys_buttons_down_back = get_color(cr_title_back_active, -16);


			//CVtListCtrlEx에서 사용되는 컬러
			cr_header_text			= get_sys_color(COLOR_BTNTEXT);
			cr_header_back			= get_sys_color(COLOR_3DFACE);

			cr_percentage_bar.clear();
			cr_percentage_bar.push_back(Gdiplus::Color::RoyalBlue);// gGRAY(192));
			cr_progress_active				= Gdiplus::Color::RoyalBlue;// Gdiplus::Color(255, 49, 108, 244);

			cr_selected_border		= gRGB(153, 209, 255);
			cr_selected_border_inactive	= cr_back_selected_inactive;

			cr_border_active = Gdiplus::Color::CornflowerBlue;
			cr_border_inactive = Gdiplus::Color::LightGray;

			cr_button_back = cr_title_back_inactive;
			cr_button_text = cr_title_text;
			cr_button_border = get_weak_color(cr_back, 40);
			cr_separator = get_weak_color(cr_back, 30);
	}

	//cr_text_dim 권위 산출 — switch 안의 각 case 가 cr_text 를 먼저 쓰고 cr_back 을 그 *뒤* 줄에서
	//지정하므로, case 안의 compute_text_dim(cr_text, cr_back) 는 *직전 테마* 의 cr_back 으로 계산되는
	//순서 버그가 있었다(특히 light→dark_gray 첫 전환 시 dim 이 거의 cr_text 와 같아져 disabled 가
	//enabled 와 구분 안 됨). cr_text·cr_back 이 모두 확정된 이 시점에서 한 번 더 산출해 모든 테마를
	//일괄 교정하고, 향후 테마 추가 시 같은 순서 함정도 방지한다. (cr_text·cr_text_dim 은 apply_theme_level
	//대상이 아니므로 leveling 전 base 색 기준으로 계산하는 기존 설계와도 일치.)
	cr_text_dim			= compute_text_dim(cr_text, cr_back);
	cr_disabled_text	= cr_text_dim;

	//selected + hover combo — selected 의 luma 방향으로 12% 만큼 black/white 와 블렌딩.
	//(이전 공식 = cr_back→cr_back_selected delta 의 절반 가산 — light bg + dark selected 처럼 delta 가 크면
	// 채널 cap 0/255 에 박혀 selected_hover 가 거의 검정/흰색 되어 contrast 깨졌음.)
	//selected 가 dark 면 black 쪽으로 12% 블렌딩해 한 톤 더 어둡게, light 면 white 쪽으로 같은 비율.
	{
		const bool selected_dark = (get_luminance(cr_back_selected) < 128);
		Gdiplus::Color cr_target = selected_dark ? Gdiplus::Color(255, 0, 0, 0) : Gdiplus::Color(255, 255, 255, 255);
		cr_back_selected_hover = get_color(cr_back_selected, cr_target, 0.12);
	}

#ifdef _DEBUG
	//테마 정의 실수(글자색≈배경색) 조기 검출 — 핵심 가독성 쌍의 명도차만 본다.
	//Gdiplus 기본 생성자가 불투명 검정이라 "필드 누락" 자체는 투명도로 못 잡으므로 대비로 대신 검출.
	//dim/secondary 쌍은 의도적 저대비라 제외. Transparent(alpha=0=자동산출) 슬롯은 검사 대상 아님.
	{
		auto check = [this](LPCTSTR what, Gdiplus::Color fg, Gdiplus::Color bg)
		{
			if (fg.GetA() == 0 || bg.GetA() == 0)
				return;
			int diff = (int)get_luminance(fg) - (int)get_luminance(bg);
			if (diff < 0)
				diff = -diff;
			if (diff < 48)
				TRACE(_T("CSCColorTheme: low contrast [%s] luma diff=%d (theme %d)\n"), what, diff, m_cur_theme);
			ASSERT(diff >= 16);		//거의 비가시 조합 — 테마 정의 오류일 가능성이 높다.
		};
		check(_T("body"),	cr_text,			cr_back);
		check(_T("title"),	cr_title_text,		cr_title_back_active);
		check(_T("edit"),	cr_edit_text,		cr_edit_back);
		check(_T("select"),	cr_text_selected,	cr_back_selected);
		check(_T("button"),	cr_button_text,		cr_button_back);
	}
#endif

	//마지막으로 테마 강도 적용. 위 debug 검사는 base(정의값) 대비를 검증하고, 사용자가 조절한
	//강도 변형은 의도적이므로 그 뒤에 일괄 스케일한다. level=1.0 이면 no-op.
	apply_theme_level(m_theme_level);

	//20260723 by claude. 격자선은 테마별 정의 없이 cr_back 에서 파생한다. apply_theme_level *뒤* 에
	//산출해야 leveled cr_back 기준이 되어, apply_theme_level 목록에 따로 넣지 않아도 항상 일치한다.
	cr_gridlines = get_weak_color(cr_back, 16);
}

Gdiplus::Color get_sys_color(int index)
{
	Gdiplus::Color cr;
	cr.SetFromCOLORREF(::GetSysColor(index));
	return cr;
}

#ifndef _USING_V110_SDK71_
D2D1_COLOR_F get_sys_d2color(int index, int alpha)
{
	COLORREF cr = GetSysColor(index);
	D2D1::ColorF d2cr{ GetRValue(cr) / 255.0f, GetGValue(cr) / 255.0f, GetBValue(cr) / 255.f, (float)alpha / 255.0f };
	return d2cr;
}
//보색과는 달리 alpha까지 고려한 밝기에 따라 black or white를 리턴한다.
//어떤 배경색과 확연히 구분되는 컬러를 보색으로 하면 128, 128, 128과 같은 색상의 보색 역시 동일한 색이 되므로 구분되지 않는다.
D2D1_COLOR_F	get_distinct_bw_color(D2D1_COLOR_F cr)
{
	// alpha를 고려한 유효 RGB (흰색 배경 가정)
	const float a = cr.a;
	const float R_eff = cr.r * a + 1.0f * (1.0f - a);
	const float G_eff = cr.g * a + 1.0f * (1.0f - a);
	const float B_eff = cr.b * a + 1.0f * (1.0f - a);

	// 0~255 스케일로 변환 후 휘도 계산
	const uint8_t lum = get_gray_value(
		static_cast<uint8_t>(R_eff * 255.0f + 0.5f),
		static_cast<uint8_t>(G_eff * 255.0f + 0.5f),
		static_cast<uint8_t>(B_eff * 255.0f + 0.5f));

	return (lum > 160)
		? D2D1::ColorF(0.0f, 0.0f, 0.0f, 1.0f)   // Black
		: D2D1::ColorF(1.0f, 1.0f, 1.0f, 1.0f);   // White
}

//get_distinct_bw_color()와는 달리 hue를 180도 이동시킨 색상을 리턴한다.
D2D1_COLOR_F	get_distinct_color(D2D1_COLOR_F cr)
{
	float h, s, v;
	rgb2hsv(static_cast<int>(cr.r * 255.0f + 0.5f),
		static_cast<int>(cr.g * 255.0f + 0.5f),
		static_cast<int>(cr.b * 255.0f + 0.5f),
		h, s, v);

	// hue를 180도 이동
	h = fmodf(h + 180.0f, 360.0f);

	// 무채색(saturation이 매우 낮은 경우)에는 hue shift가 무의미하므로 명도를 반전
	if (s < 0.05f)
		v = 1.0f - v;

	int r, g, b;
	hsv2rgb(h, s, v, r, g, b);

	return D2D1::ColorF(r / 255.0f, g / 255.0f, b / 255.0f, cr.a);
}
#endif
