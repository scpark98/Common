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

	if ( r < 0 ) r = 0; else if ( r > 255 ) r = 255;
	if ( g < 0 ) g = 0; else if ( g > 255 ) g = 255;
	if ( b < 0 ) b = 0; else if ( b > 255 ) b = 255;

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

	r = r1 - ((double)(r1 - r2) * ratio);
	g = g1 - ((double)(g1 - g2) * ratio);
	b = b1 - ((double)(b1 - b2) * ratio);

	return RGB(r, g, b);
}

//gray = (2989 * r + 5870 * g + 1140 * b) / 10000; 
//=>0.2989 * 2^14 = 4897.1776; 
//출처: http://kipl.tistory.com/80 [Geometry & Recognition]
//rgb가 175인 값이 넘어오면 >> 14 한 후의 값이 xxx.9~로 나온다. 반올림을 해보자.
uint8_t gray_value(uint8_t r, uint8_t g, uint8_t b)
{
	int cr = r * 4897 + g * 9617 + b * 1868;
	//float crf = cr >> 14;	//shift연산의 결과는 정수이므로 이 식은 부정확하다.
	float crf = (float)cr / pow(2,14);
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
