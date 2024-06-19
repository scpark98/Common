#include "colors.h"

#include "../Common/Functions.h"


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

//"FF0000"과 같은 컬러 문자열을 COLORREF로 변환
COLORREF	get_color(CString cr)
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

COLORREF gray_color(COLORREF cr)
{
	uint8_t gray = gray_value(GetRValue(cr), GetGValue(cr), GetBValue(cr));
	return RGB(gray, gray, gray);
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
