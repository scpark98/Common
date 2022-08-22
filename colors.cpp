#include "colors.h"

#include "../Common/Functions.h"


//regathered from https://www.colordic.org/
/*
<<!caution!>>
html color	: #ff0000 = red
gdi color	: 0x000000ff = red
��, html color�� RGB������ ǥ��ǰ�
gdi�� COLORREF�� 0xaabbggrr�� ������ ����ʿ� �����ؾ� �Ѵ�.
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


//�� ���� �߰� ���� ������ ���Ѵ�. (ratio�� 0.0~1.0�����̰� �� ���� 0.0�̸� cr1��, 1.0�̸� cr2�� ���ϵȴ�.)
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
//��ó: http://kipl.tistory.com/80 [Geometry & Recognition]
//rgb�� 175�� ���� �Ѿ���� >> 14 �� ���� ���� xxx.9~�� ���´�. �ݿø��� �غ���.
uint8_t gray_value(uint8_t r, uint8_t g, uint8_t b)
{
	int cr = r * 4897 + g * 9617 + b * 1868;
	//float crf = cr >> 14;	//shift������ ����� �����̹Ƿ� �� ���� ����Ȯ�ϴ�.
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
