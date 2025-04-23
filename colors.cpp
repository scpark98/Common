#include "colors.h"

#include "Functions.h"


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

std::unordered_map<char*, COLORREF> g_cr =
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
	Gdiplus::Color gcr;
	gcr.SetFromCOLORREF(rgb);
	return gcr;
}

Gdiplus::Color get_color(std::string cr_name)
{
	return CSCColorMap::get_color(cr_name);
}

//"FF0000"과 같은 컬러 문자열을 COLORREF로 변환
COLORREF	get_color_from_hexadecimal(CString cr)
{
	COLORREF color;
	if ((color = _tcstol(cr, NULL, 16)) == 0)
		color = 0x00ffffff;  // default is white
	else
		color = rgb_bgr(color);

	return color;
}

//컬러값을 "FF0000"과 같은 문자열로 리턴한다.
CString		get_color_string(COLORREF cr)
{
	CString str;
	str.Format(_T("%02x%02x%02x"), GetRValue(cr), GetGValue(cr), GetBValue(cr));
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

//gray = (2989 * r + 5870 * g + 1140 * b) / 10000; 
//=>0.2989 * 2^14 = 4897.1776; 
//출처: http://kipl.tistory.com/80 [Geometry & Recognition]
//rgb가 175인 값이 넘어오면 >> 14 한 후의 값이 xxx.9~로 나온다. 반올림을 해보자.
uint8_t gray_value(uint8_t r, uint8_t g, uint8_t b)
{
	int cr = r * 4897 + g * 9617 + b * 1868;
	//float crf = cr >> 14;	//shift연산의 결과는 정수이므로 이 식은 부정확하다.
	float crf = (float)(double)cr / pow(2,14);
	uint8_t gray = ROUND(crf, 0);
	return gray;
}

uint8_t	gray_value(COLORREF cr)
{
	return gray_value(GetRValue(cr), GetGValue(cr), GetBValue(cr));
}

uint8_t	gray_value(Gdiplus::Color cr)
{
	return gray_value(cr.GetR(), cr.GetG(), cr.GetB());
}

COLORREF gray_color(COLORREF cr)
{
	uint8_t gray = gray_value(GetRValue(cr), GetGValue(cr), GetBValue(cr));
	return RGB(gray, gray, gray);
}

Gdiplus::Color	gray_color(Gdiplus::Color cr)
{
	uint8_t gray = gray_value(cr.GetR(), cr.GetG(), cr.GetB());
	return Gdiplus::Color(cr.GetA(), gray, gray, gray);
}

double color_similarity_distance(COLORREF c1, COLORREF c2)
{
	return sqrt(
		(GetRValue(c1)-GetRValue(c2))*(GetRValue(c1)-GetRValue(c2)) +
		(GetGValue(c1)-GetGValue(c2))*(GetGValue(c1)-GetGValue(c2)) +
		(GetGValue(c1)-GetGValue(c2))*(GetGValue(c1)-GetGValue(c2))
	);
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

COLORREF gpColor2RGB(Gdiplus::Color cr)
{
	return RGB(cr.GetRed(), cr.GetGreen(), cr.GetBlue());
}

Gdiplus::Color RGB2gpColor(COLORREF cr, BYTE alpha)
{
	return Gdiplus::Color(alpha, GetRValue(cr), GetGValue(cr), GetBValue(cr));
}

//cr컬러에서 a,r,g,b 중 한 색을 value로 변경한다.
void set_color(Gdiplus::Color &cr, int argb, BYTE value)
{
	BYTE a = (argb == 0 ? value : cr.GetA());
	BYTE r = (argb == 1 ? value : cr.GetR());
	BYTE g = (argb == 2 ? value : cr.GetG());
	BYTE b = (argb == 3 ? value : cr.GetB());

	cr = Gdiplus::Color(a, r, g, b);
}

Gdiplus::Color get_color(Gdiplus::Color cr, int argb, BYTE value)
{
	Gdiplus::Color cr_new = cr;
	set_color(cr_new, argb, value);
	return cr_new;
}

//주어진 컬러와 가장 유사한 표준색의 이름을 리턴.
//https://en.wikipedia.org/wiki/Web_colors
//RGB(255, 190, 200)과 가장 유사한 색은 Pink(255, 192, 203)이라고 보이지만
//LightPink(255, 182, 193)를 리턴한다. 계산식이 잘못인지 확인 필요.
CString	get_color_name_of_closest(COLORREF cr_src, COLORREF* cr_closest)
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
			if (cr_closest)
				*cr_closest = cr.second;
		}
	}

	return result;
}

std::deque<CString>  CSCColorTheme::get_color_theme_list()
{
	std::deque<CString> dq;
	dq.push_back(_T("default"));
	dq.push_back(_T("gray"));
	dq.push_back(_T("dark_gray"));
	dq.push_back(_T("dark"));

	return dq;
}

void CSCColorTheme::set_color_theme(int color_theme)
{
	//일부 색상을 변경했을 경우 다시 호출하면 기본값으로 롤백된다.
	m_cur_theme = color_theme;

	switch (color_theme)
	{
		case color_theme_linkmemine :
			cr_text = Gdiplus::Color::Black;
			cr_text_dim = Gdiplus::Color::DimGray;
			cr_text_hover = cr_text;
			cr_text_selected = Gdiplus::Color(255, 241, 241, 241);
			cr_text_selected_inactive = cr_text_selected;
			cr_text_dropHilited = white;

			cr_back = Gdiplus::Color::White;
			cr_back_selected = get_color(cr_back, 16);
			cr_back_selected_inactive = gray_color(cr_back_selected);
			cr_back_dropHilited = RGB2gpColor(::GetSysColor(COLOR_HIGHLIGHT));
			cr_back_hover = cr_back_selected;
			cr_back_alternate = get_color(cr_back, 8);

			cr_title_text = Gdiplus::Color::White;
			cr_title_back = gRGB(97, 132, 180);
			cr_sys_buttons_hover_back = get_color(cr_title_back, 30);

			cr_selected_border = Gdiplus::Color(255, 128, 128, 128);

			cr_header_text = get_color(cr_text, -16);
			cr_header_back = get_color(cr_back, -16);
			cr_percentage_bar.clear();
			cr_percentage_bar.push_back(get_color(cr_back, 32));
			cr_progress = Gdiplus::Color(255, 32, 32, 255);
			//cr_progress_text		= Gdiplus::Color(255, 192, 192, 192);

			cr_border = Gdiplus::Color::DarkGray;
			break;
		case color_theme_linkmemine_se:
			cr_text = Gdiplus::Color::Black;
			cr_text_dim = Gdiplus::Color::DimGray;
			cr_text_hover = cr_text;
			cr_text_selected = Gdiplus::Color(255, 241, 241, 241);
			cr_text_selected_inactive = cr_text_selected;
			cr_text_dropHilited = white;

			cr_back = Gdiplus::Color::White;
			cr_back_selected = get_color(cr_back, 16);
			cr_back_selected_inactive = gray_color(cr_back_selected);
			cr_back_dropHilited = RGB2gpColor(::GetSysColor(COLOR_HIGHLIGHT));
			cr_back_hover = cr_back_selected;
			cr_back_alternate = get_color(cr_back, 8);

			cr_title_text = Gdiplus::Color::White; //gRGB(253, 126, 20);
			cr_title_back = gRGB(186, 89, 18);
			cr_sys_buttons_hover_back = get_color(cr_title_back, 30);

			cr_selected_border = Gdiplus::Color(255, 128, 128, 128);

			cr_header_text = get_color(cr_text, -16);
			cr_header_back = get_color(cr_back, -16);
			cr_percentage_bar.clear();
			cr_percentage_bar.push_back(get_color(cr_back, 32));
			cr_progress = Gdiplus::Color(255, 32, 32, 255);
			//cr_progress_text		= Gdiplus::Color(255, 192, 192, 192);

			cr_border = Gdiplus::Color::DarkGray;
			break;
		case color_theme_anysupport :
			cr_text = Gdiplus::Color::Black;
			cr_text_dim = Gdiplus::Color::DimGray;
			cr_text_hover = cr_text;
			cr_text_selected = Gdiplus::Color(255, 241, 241, 241);
			cr_text_selected_inactive = cr_text_selected;
			cr_text_dropHilited = white;

			cr_back = Gdiplus::Color::White;
			cr_back_selected = get_color(cr_back, 16);
			cr_back_selected_inactive = gray_color(cr_back_selected);
			cr_back_dropHilited = RGB2gpColor(::GetSysColor(COLOR_HIGHLIGHT));
			cr_back_hover = cr_back_selected;
			cr_back_alternate = get_color(cr_back, 8);

			cr_title_text = gRGB(13, 41, 71);
			cr_title_back = gRGB(48, 154, 192); 
			cr_sys_buttons_hover_back = get_color(cr_title_back, 30);

			cr_selected_border = Gdiplus::Color(255, 128, 128, 128);

			cr_header_text = get_color(cr_text, -16);
			cr_header_back = get_color(cr_back, -16);
			cr_percentage_bar.clear();
			cr_percentage_bar.push_back(get_color(cr_back, 32));
			cr_progress = Gdiplus::Color(255, 32, 32, 255);
			//cr_progress_text		= Gdiplus::Color(255, 192, 192, 192);

			cr_border = Gdiplus::Color::DarkGray;
			break;
		case color_theme_helpu:
			cr_text = Gdiplus::Color::Black;
			cr_text_dim = Gdiplus::Color::DimGray;
			cr_text_hover = cr_text;
			cr_text_selected = Gdiplus::Color(255, 241, 241, 241);
			cr_text_selected_inactive = cr_text_selected;
			cr_text_dropHilited = white;

			cr_back = Gdiplus::Color::White;
			cr_back_selected = get_color(cr_back, 16);
			cr_back_selected_inactive = gray_color(cr_back_selected);
			cr_back_dropHilited = RGB2gpColor(::GetSysColor(COLOR_HIGHLIGHT));
			cr_back_hover = cr_back_selected;
			cr_back_alternate = get_color(cr_back, 8);

			cr_title_text = gRGB(18, 24, 58);
			cr_title_back = gRGB(32, 178, 174);
			cr_sys_buttons_hover_back = get_color(cr_title_back, 30);

			cr_selected_border = Gdiplus::Color(255, 128, 128, 128);

			cr_header_text = get_color(cr_text, -16);
			cr_header_back = get_color(cr_back, -16);
			cr_percentage_bar.clear();
			cr_percentage_bar.push_back(get_color(cr_back, 32));
			cr_progress = Gdiplus::Color(255, 32, 32, 255);
			//cr_progress_text		= Gdiplus::Color(255, 192, 192, 192);

			cr_border = Gdiplus::Color::DarkGray;
			break;
		case color_theme_pcanypro:
			cr_text = Gdiplus::Color::Black;
			cr_text_dim = Gdiplus::Color::DimGray;
			cr_text_hover = cr_text;
			cr_text_selected = Gdiplus::Color(255, 241, 241, 241);
			cr_text_selected_inactive = cr_text_selected;
			cr_text_dropHilited = white;

			cr_back = Gdiplus::Color::White;
			cr_back_selected = get_color(cr_back, 16);
			cr_back_selected_inactive = gray_color(cr_back_selected);
			cr_back_dropHilited = RGB2gpColor(::GetSysColor(COLOR_HIGHLIGHT));
			cr_back_hover = cr_back_selected;
			cr_back_alternate = get_color(cr_back, 8);

			cr_title_text = Gdiplus::Color::White;// gRGB(18, 24, 58);
			cr_title_back = gRGB(214, 129, 8);
			cr_sys_buttons_hover_back = get_color(cr_title_back, 30);

			cr_selected_border = Gdiplus::Color(255, 128, 128, 128);

			cr_header_text = get_color(cr_text, -16);
			cr_header_back = get_color(cr_back, -16);
			cr_percentage_bar.clear();
			cr_percentage_bar.push_back(get_color(cr_back, 32));
			cr_progress = Gdiplus::Color(255, 32, 32, 255);
			//cr_progress_text		= Gdiplus::Color(255, 192, 192, 192);

			cr_border = Gdiplus::Color::DarkGray;
			break;
		case color_theme_dark_gray :
			cr_text					= Gdiplus::Color(255, 164, 164, 164);
			cr_text_dim				= Gdiplus::Color(255, 96, 96, 96);
			cr_text_hover			= cr_text;
			cr_text_selected		= Gdiplus::Color(255, 241, 241, 241);
			cr_text_selected_inactive = cr_text_selected;
			cr_text_dropHilited		= white;

			cr_back					= Gdiplus::Color(255, 64, 64, 64);
			cr_back_selected		= get_color(cr_back, -16);
			cr_back_selected_inactive = gray_color(cr_back_selected);
			cr_back_dropHilited		= RGB2gpColor(::GetSysColor(COLOR_HIGHLIGHT));
			cr_back_hover			= cr_back_selected;
			cr_back_alternate		= get_color(cr_back, 8);

			cr_selected_border		= Gdiplus::Color(255, 128, 128, 128);

			cr_header_text			= get_color(cr_text, -16);
			cr_header_back			= get_color(cr_back, -16);
			cr_percentage_bar.clear();
			cr_percentage_bar.push_back(get_color(cr_back, 32));
			cr_progress				= Gdiplus::Color(255, 32, 32, 255);
			//cr_progress_text		= Gdiplus::Color(255, 192, 192, 192);

			cr_border				= Gdiplus::Color::DarkGray;
			break;
		case color_theme_dark :
			cr_text					= Gdiplus::Color(255, 212, 212, 212);
			cr_text_dim				= Gdiplus::Color(255, 96, 96, 96);
			cr_text_hover			= cr_text;
			cr_text_selected		= Gdiplus::Color(255, 241, 241, 241);
			cr_text_selected_inactive = cr_text_selected;
			cr_text_dropHilited		= white;

			cr_back					= Gdiplus::Color(255, 37, 37, 38);
			cr_back_selected		= Gdiplus::Color(255, 0, 120, 215);
			cr_back_selected_inactive = gray_color(cr_back_selected);
			cr_back_dropHilited		= RGB2gpColor(::GetSysColor(COLOR_HIGHLIGHT));
			cr_back_hover			= get_color(cr_back_selected, 48);
			cr_back_alternate		= get_color(cr_back, 8);

			cr_selected_border		= cr_back_selected;

			cr_header_back			= get_color(cr_back, 16);
			cr_header_text			= get_color(cr_text, -32);
			cr_percentage_bar.clear();
			cr_percentage_bar.push_back(get_color(cr_back, 32));
			cr_progress				= Gdiplus::Color(255, 32, 32, 255);
			//cr_progress_text		= Gdiplus::Color(255, 192, 192, 192);

			cr_border				= Gdiplus::Color::Black;
			break;

		case color_theme_popup_folder_list:
			cr_text.SetFromCOLORREF(::GetSysColor(COLOR_BTNTEXT));
			cr_text_selected		= cr_text;// ::GetSysColor(COLOR_HIGHLIGHTTEXT);
			cr_text_selected_inactive.SetFromCOLORREF(::GetSysColor(COLOR_INACTIVECAPTIONTEXT));
			cr_text_hover			= cr_text;

			cr_back.SetFromCOLORREF(::GetSysColor(COLOR_3DFACE)); //RGB(242, 242, 242);// ::GetSysColor(COLOR_WINDOW);
			cr_back_selected		= Gdiplus::Color(255, 204, 232, 255);// ::GetSysColor(COLOR_HIGHLIGHT);
			cr_back_selected_inactive.SetFromCOLORREF(::GetSysColor(COLOR_HIGHLIGHT));
			cr_back_hover			= Gdiplus::Color(255, 195, 222, 245);

			cr_selected_border		= Gdiplus::Color(255, 153, 209, 255);

			cr_border				= Gdiplus::Color::DimGray;
			break;

		default : //case color_theme_default :
			cr_text					= RGB2gpColor(::GetSysColor(COLOR_BTNTEXT));
			cr_text_dim				= get_color(cr_text, 32);
			cr_text_hover			= cr_text;
			cr_text_selected		= cr_text;// RGB2gpColor(::GetSysColor(COLOR_HIGHLIGHTTEXT));;
			cr_text_selected_inactive = cr_text_selected;
			cr_text_dropHilited		= RGB2gpColor(::GetSysColor(COLOR_HIGHLIGHTTEXT));

			//컨트롤 종류에 따라 기본 배경색이 다르다.
			if (m_parent->IsKindOf(RUNTIME_CLASS(CListCtrl)) ||
				m_parent->IsKindOf(RUNTIME_CLASS(CTreeCtrl)) || 
				m_parent->IsKindOf(RUNTIME_CLASS(CListBox)) ||
				m_parent->IsKindOf(RUNTIME_CLASS(CEdit)) ||
				m_parent->IsKindOf(RUNTIME_CLASS(CRichEditCtrl)))
			{
				cr_back = RGB2gpColor(::GetSysColor(COLOR_WINDOW));
			}
			else
			{
				cr_back = RGB2gpColor(::GetSysColor(COLOR_BTNFACE));
			}

			cr_back_selected		= gRGB(204, 235, 255);//RGB2gpColor(::GetSysColor(COLOR_HIGHLIGHT));
			cr_back_selected_inactive = gray_color(cr_back_selected);
			cr_back_dropHilited		= RGB2gpColor(::GetSysColor(COLOR_HIGHLIGHT));
			cr_back_hover			= Gdiplus::Color(255, 229, 243, 255);
			cr_back_alternate		= get_color(cr_back, -16);

			cr_title_text			= Gdiplus::Color::Black;
			cr_title_back			= Gdiplus::Color::LightGray;

			//CVtListCtrlEx에서 사용되는 컬러
			cr_header_text			= RGB2gpColor(::GetSysColor(COLOR_BTNTEXT));
			cr_header_back			= RGB2gpColor(::GetSysColor(COLOR_3DFACE));
			cr_percentage_bar.clear();
			cr_percentage_bar.push_back(gGRAY(192));
			cr_progress				= Gdiplus::Color(255, 49, 108, 244);

			cr_selected_border		= gRGB(153, 209, 255);//get_color(cr_back_selected, -32);

			cr_border				= Gdiplus::Color::DimGray;
	}
}

Gdiplus::Color get_sys_color(int index)
{
	Gdiplus::Color gcr;
	gcr.SetFromCOLORREF(::GetSysColor(index));
	return gcr;
}
