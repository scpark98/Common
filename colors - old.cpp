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
enum Colors
{
	{_T("black"), 				0x00000000, 	0},
	{_T("dimgray"), 			0x00696969, 	1},
	{_T("gray"), 				0x00808080, 	2},
	{_T("darkgray"), 			0x00a9a9a9, 	3},
	{_T("silver"), 				0x00c0c0c0, 	4},
	{_T("lightgray"), 			0x00d3d3d3, 	5},
	{_T("gainsboro"), 			0x00dcdcdc, 	6},
	{_T("whitesmoke"), 			0x00f5f5f5, 	7},
	{_T("white"), 				0x00ffffff, 	8},
	{_T("snow"), 				0x00fafaff, 	9},
	{_T("ghostwhite"), 			0x00fff8f8, 	10},
	{_T("floralwhite"), 		0x00f0faff, 	11},
	{_T("linen"), 				0x00e6f0fa, 	12},
	{_T("antiquewhite"), 		0x00d7ebfa, 	13},
	{_T("papayawhip"), 			0x00d5efff, 	14},
	{_T("blanchedalmond"), 		0x00cdebff, 	15},
	{_T("bisque"), 				0x00c4e4ff, 	16},
	{_T("moccasin"), 			0x00b5e4ff, 	17},
	{_T("navajowhite"), 		0x00addeff, 	18},
	{_T("peachpuff"), 			0x00b9daff, 	19},
	{_T("mistyrose"), 			0x00e1e4ff, 	20},
	{_T("lavenderblush"), 		0x00f5f0ff, 	21},
	{_T("seashell"), 			0x00eef5ff, 	22},
	{_T("oldlace"), 			0x00e6f5fd, 	23},
	{_T("ivory"), 				0x00f0ffff, 	24},
	{_T("honeydew"), 			0x00f0fff0, 	25},
	{_T("mintcream"), 			0x00fafff5, 	26},
	{_T("azure"), 				0x00fffff0, 	27},
	{_T("aliceblue"), 			0x00fff8f0, 	28},
	{_T("lavender"), 			0x00fae6e6, 	29},
	{_T("lightsteelblue"), 		0x00dec4b0, 	30},
	{_T("lightslategray"), 		0x00998877, 	31},
	{_T("slategray"), 			0x00908070, 	32},
	{_T("steelblue"), 			0x00b48246, 	33},
	{_T("royalblue"), 			0x00e16941, 	34},
	{_T("midnightblue"), 		0x00701919, 	35},
	{_T("navy"), 				0x00800000, 	36},
	{_T("darkblue"), 			0x008b0000, 	37},
	{_T("mediumblue"), 			0x00cd0000, 	38},
	{_T("blue"), 				0x00ff0000, 	39},
	{_T("dodgerblue"), 			0x00ff901e, 	40},
	{_T("cornflowerblue"), 		0x00ed9564, 	41},
	{_T("deepskyblue"), 		0x00ffbf00, 	42},
	{_T("lightskyblue"), 		0x00face87, 	43},
	{_T("skyblue"), 			0x00ebce87, 	44},
	{_T("lightblue"), 			0x00e6d8ad, 	45},
	{_T("powderblue"), 			0x00e6e0b0, 	46},
	{_T("paleturquoise"), 		0x00eeeeaf, 	47},
	{_T("lightcyan"), 			0x00ffffe0, 	48},
	{_T("cyan"), 				0x00ffff00, 	49},
	{_T("aqua"), 				0x00ffff00, 	50},
	{_T("turquoise"), 			0x00d0e040, 	51},
	{_T("mediumturquoise"),		0x00ccd148, 	52},
	{_T("darkturquoise"), 		0x00d1ce00, 	53},
	{_T("lightseagreen"), 		0x00aab220, 	54},
	{_T("cadetblue"), 			0x00a09e5f, 	55},
	{_T("darkcyan"), 			0x008b8b00, 	56},
	{_T("teal"), 				0x00808000, 	57},
	{_T("darkslategray"), 		0x004f4f2f, 	58},
	{_T("darkgreen"), 			0x00006400, 	59},
	{_T("green"), 				0x00008000, 	60},
	{_T("forestgreen"), 		0x00228b22, 	61},
	{_T("seagreen"), 			0x00578b2e, 	62},
	{_T("mediumseagreen"), 		0x0071b33c, 	63},
	{_T("mediumaquamarine"),	0x00aacd66, 	64},
	{_T("darkseagreen"), 		0x008fbc8f, 	65},
	{_T("aquamarine"), 			0x00d4ff7f, 	66},
	{_T("palegreen"), 			0x0098fb98, 	67},
	{_T("lightgreen"), 			0x0090ee90, 	68},
	{_T("springgreen"), 		0x007fff00, 	69},
	{_T("mediumspringgreen"), 	0x009afa00, 	70},
	{_T("lawngreen"), 			0x0000fc7c, 	71},
	{_T("chartreuse"), 			0x0000ff7f, 	72},
	{_T("greenyellow"), 		0x002fffad, 	73},
	{_T("lime"), 				0x0000ff00, 	74},
	{_T("limegreen"), 			0x0032cd32, 	75},
	{_T("yellowgreen"), 		0x0032cd9a, 	76},
	{_T("darkolivegreen"), 		0x002f6b55, 	77},
	{_T("olivedrab"), 			0x00238e6b, 	78},
	{_T("olive"), 				0x00008080, 	79},
	{_T("darkkhaki"), 			0x006bb7bd, 	80},
	{_T("palegoldenrod"), 		0x00aae8ee, 	81},
	{_T("cornsilk"), 			0x00dcf8ff, 	82},
	{_T("beige"), 				0x00dcf5f5, 	83},
	{_T("lightyellow"), 		0x00e0ffff, 	84},
	{_T("lightgoldenrodyellow"),0x00d2fafa, 	85},
	{_T("lemonchiffon"), 		0x00cdfaff, 	86},
	{_T("wheat"), 				0x00b3def5, 	87},
	{_T("burlywood"), 			0x0087b8de, 	88},
	{_T("tan"), 				0x008cb4d2, 	89},
	{_T("khaki"), 				0x008ce6f0, 	90},
	{_T("yellow"), 				0x0000ffff, 	91},
	{_T("gold"), 				0x0000d7ff, 	92},
	{_T("orange"), 				0x0000a5ff, 	93},
	{_T("sandybrown"),			0x0060a4f4, 	94},
	{_T("darkorange"),			0x00008cff, 	95},
	{_T("goldenrod"), 			0x0020a5da, 	96},
	{_T("peru"), 				0x003f85cd, 	97},
	{_T("darkgoldenrod"), 		0x000b86b8, 	98},
	{_T("chocolate"), 			0x001e69d2, 	99},
	{_T("sienna"), 				0x002d52a0, 	100},
	{_T("saddlebrown"), 		0x0013458b, 	101},
	{_T("maroon"), 				0x00000080, 	102},
	{_T("darkred"), 			0x0000008b, 	103},
	{_T("brown"), 				0x002a2aa5, 	104},
	{_T("firebrick"), 			0x002222b2, 	105},
	{_T("indianred"), 			0x005c5ccd, 	106},
	{_T("rosybrown"), 			0x008f8fbc, 	107},
	{_T("darksalmon"), 			0x007a96e9, 	108},
	{_T("lightcoral"), 			0x008080f0, 	109},
	{_T("salmon"), 				0x007280fa, 	110},
	{_T("lightsalmon"), 		0x007aa0ff, 	111},
	{_T("coral"), 				0x00507fff, 	112},
	{_T("tomato"), 				0x004763ff, 	113},
	{_T("orangered"), 			0x000045ff, 	114},
	{_T("red"), 				0x000000ff, 	115},
	{_T("crimson"), 			0x003c14dc, 	116},
	{_T("mediumvioletred"), 	0x008515c7, 	117},
	{_T("deeppink"), 			0x009314ff, 	118},
	{_T("hotpink"), 			0x00b469ff, 	119},
	{_T("palevioletred"), 		0x009370db, 	120},
	{_T("pink"), 				0x00cbc0ff, 	121},
	{_T("lightpink"), 			0x00c1b6ff, 	122},
	{_T("thistle"), 			0x00d8bfd8, 	123},
	{_T("magenta"), 			0x00ff00ff, 	124},
	{_T("fuchsia"), 			0x00ff00ff, 	125},
	{_T("violet"), 				0x00ee82ee, 	126},
	{_T("plum"), 				0x00dda0dd, 	127},
	{_T("orchid"), 				0x00d670da, 	128},
	{_T("mediumorchid"), 		0x00d355ba, 	129},
	{_T("darkorchid"), 			0x00cc3299, 	130},
	{_T("darkviolet"), 			0x00d30094, 	131},
	{_T("darkmagenta"), 		0x008b008b, 	132},
	{_T("purple"), 				0x00800080, 	133},
	{_T("indigo"), 				0x0082004b, 	134},
	{_T("darkslateblue"), 		0x008b3d48, 	135},
	{_T("blueviolet"), 			0x00e22b8a, 	136},
	{_T("mediumpurple"), 		0x00db7093, 	137},
	{_T("slateblue"), 			0x00cd5a6a, 	138},
	{_T("mediumslateblue"), 	0x00ee687b, 	139},

/*
	// gathered from http://www.netwave.or.jp/~shikai/shikai/shcolor.htm
	{_T("white"), 0xffffff, 0},
	{_T("whitesmoke"), 0xf5f5f5, 0},
	{_T("ghostwhite"), 0xf8f8ff, 0},
	{_T("snow"), 0xfffafa, 0},
	{_T("gainsboro"), 0xdcdcdc, 0},
	{_T("lightgrey"), 0xd3d3d3, 0},
	{_T("silver"), 0xc0c0c0, 0},
	{_T("darkgray"), 0xa9a9a9, 0},
	{_T("gray"), 0x808080, 0},
	{_T("dimgray"), 0x696969, 0},
	{_T("lightslategray"), 0x778899, 0},
	{_T("slategray"), 0x708090, 0},
	{_T("darkslategray"), 0x2f4f4f, 0},
	{_T("black"), 0x000000, 0},

	{_T("azure"), 0xf0ffff, 1},
	{_T("aliceblue"), 0xf0f8ff, 1},
	{_T("mintcream"), 0xf5fffa, 1},
	{_T("honeydew"), 0xf0fff0, 1},
	{_T("lightcyan"), 0xe0ffff, 1},
	{_T("paleturqoise"), 0xafeeee, 1},
	{_T("powderblue"), 0xb0e0e6, 1},
	{_T("lightblue"), 0xadd8ed, 1},
	{_T("lightsteelblue"), 0xb0c4de, 1},
	{_T("skyblue"), 0x87ceeb, 1},
	{_T("lightskyblue"), 0x87cefa, 1},
	{_T("cyan"), 0x00ffff, 1},

	{_T("aqua"), 0x00ff80, 2},
	{_T("deepskyblue"), 0x00bfff, 2},
	{_T("aquamarine"), 0x7fffd4, 2},
	{_T("turquoise"), 0x40e0d0, 2},
	{_T("darkturquoise"), 0x00ced1, 2},
	{_T("lightseagreen"), 0x20b2aa, 2},
	{_T("mediumturquoise"), 0x40e0dd, 2},
	{_T("mediumaquamarine"), 0x66cdaa, 2},
	{_T("cadetblue"), 0x5f9ea0, 2},
	{_T("teal"), 0x008080, 2},
	{_T("darkcyan"), 0x008b8b, 2},
	{_T("comflowerblue"), 0x6495ed, 2},
	{_T("dodgerblue"), 0x1e90ff, 2},
	{_T("steelblue"), 0x4682b4, 2},
	{_T("royalblue"), 0x4169e1, 2},
	{_T("blue"), 0x0000ff, 3},
	{_T("mediumblue"), 0x0000cd, 3},
	{_T("mediumslateblue"), 0x7b68ee, 3},
	{_T("slateblue"), 0x6a5acd, 3},
	{_T("darkslateblue"), 0x483d8b, 3},
	{_T("darkblue"), 0x00008b, 3},
	{_T("midnightblue"), 0x191970, 3},
	{_T("navy"), 0x000080, 3},

	{_T("palegreen"), 0x98fb98, 4},
	{_T("lightgreen"), 0x90ee90, 4},
	{_T("mediumspringgreen"), 0x00fa9a, 4},
	{_T("springgreen"), 0x00ff7f, 4},
	{_T("chartreuse"), 0x7fff00, 4},
	{_T("lawngreen"), 0x7cfc00, 4},
	{_T("lime"), 0x00ff00, 4},
	{_T("limegreen"), 0x32cd32, 4},
	{_T("greenyellow"), 0xadff2f, 4},
	{_T("yellowgreen"), 0x9acd32, 4},
	{_T("darkseagreen"), 0x8fbc8f, 4},
	{_T("mediumseagreen"), 0x3cb371, 4},
	{_T("seagreen"), 0x2e8b57, 4},
	{_T("olivedrab"), 0x6b8e23, 4},
	{_T("forestgreen"), 0x228b22, 4},
	{_T("green"), 0x008000, 4},
	{_T("darkkhaki"), 0xbdb76b, 4},
	{_T("olive"), 0x808000, 4},
	{_T("darkolivegreen"), 0x556b2f, 4},
	{_T("darkgreen"), 0x006400, 4},

	{_T("floralwhite"), 0xfffaf0, 5},
	{_T("seashell"), 0xfff5ee, 5},
	{_T("ivory"), 0xfffff0, 5},
	{_T("beige"), 0xf5f5dc, 5},
	{_T("cornsilk"), 0xfff8dc, 5},
	{_T("lemonchiffon"), 0xfffacd, 5},
	{_T("lightyellow"), 0xffffe0, 5},
	{_T("lightgoldenrodyellow"), 0xfafad2, 5},
	{_T("papayawhip"), 0xffefd5, 5},
	{_T("blanchedalmond"), 0xffedcd, 5},
	{_T("palegoldenrod"), 0xeee8aa, 5},
	{_T("khaki"), 0xf0eb8c, 5},
	{_T("bisque"), 0xffe4c4, 5},
	{_T("moccasin"), 0xffe4b5, 5},
	{_T("navajowhite"), 0xffdead, 5},
	{_T("peachpuff"), 0xffdab9, 5},
	{_T("yellow"), 0xffff00, 5},
	{_T("gold"), 0xffd700, 5},
	{_T("wheat"), 0xf5deb3, 5},
	{_T("orange"), 0xffa500, 5},
	{_T("darkorange"), 0xff8c00, 5},

	{_T("oldlace"), 0xfdf5e6, 6},
	{_T("linen"), 0xfaf0e6, 6},
	{_T("antiquewhite"), 0xfaebd7, 6},
	{_T("lightsalmon"), 0xffa07a, 6},
	{_T("darksalmon"), 0xe9967a, 6},
	{_T("salmon"), 0xfa8072, 6},
	{_T("lightcoral"), 0xf08080, 6},
	{_T("indianred"), 0xcd5c5c, 6},
	{_T("coral"), 0xff7f50, 6},
	{_T("tomato"), 0xff6347, 6},
	{_T("orangered"), 0xff4500, 6},
	{_T("red"), 0xff0000, 6},
	{_T("crimson"), 0xdc143c, 6},
	{_T("firebrick"), 0xb22222, 6},
	{_T("maroon"), 0x800000, 6},
	{_T("darkred"), 0x8b0000, 6},

	{_T("lavender"), 0xe6e6fe, 7},
	{_T("lavenderblush"), 0xfff0f5, 7},
	{_T("mistyrose"), 0xffe4e1,			7},
	{_T("thistle"), 0xd8bfd8,			7},
	{_T("pink"), 0xffc0cb,				7},
	{_T("lightpink"), 0xffb6c1,			7},
	{_T("palevioletred"), 0xdb7093,		7},
	{_T("hotpink"), 0xff69b4,			7},
	{_T("fuchsia"), 0xff00ee,			7},
	{_T("magenta"), 0xff00ff,			7},
	{_T("mediumvioletred"), 0xc71585,	7},
	{_T("deeppink"), 0xff1493,			7},
	{_T("plum"), 0xdda0dd,				7},
	{_T("violet"), 0xee82ee,			7},
	{_T("orchid"), 0xda70d6,			7},
	{_T("mediumorchid"), 0xba55d3,		7},
	{_T("mediumpurple"), 0x9370db,		7},
	{_T("purple"), 0x9370db,			7},
	{_T("blueviolet"), 0x8a2be2,		7},
	{_T("darkviolet"), 0x9400d3,		7},
	{_T("darkorchid"), 0x9932cc,		7},

	{_T("tan"), 0xd2b48c,				8},
	{_T("burlywood"), 0xdeb887,			8},
	{_T("sandybrown"), 0xf4a460,		8},
	{_T("peru"), 0xcd853f,				8},
	{_T("goldenrod"), 0xdaa520,			8},
	{_T("darkgoldenrod"), 0xb8860b,		8},
	{_T("chocolate"), 0xd2691e,			8},
	{_T("rosybrown"), 0xbc8f8f,			8},
	{_T("sienna"), 0xa0522d,			8},
	{_T("saddlebrown"), 0x8b4513,		8},
	{_T("brown"), 0xa52a2a,				8},
*/
};

std::deque<CHtmlColor> g_color;
COLORREF g_default_color[16];
CZombie g_zombie;

//기본 정의된 컬러를 이용할 경우는
//app에서 이 함수를 한번은 호출해줘야 한다.
//단, ColorDialogEx와 같이 클래스를 사용하는 경우는 호출 코드가 포함되어 있으니
//별도로 불러주지 않아도 된다.
void g_color_init()
{
	g_color.clear();

	for(ptrdiff_t i = 0; i < countof(hmtlcolors); i++)
	{
		g_color.push_back(CHtmlColor(hmtlcolors[i].name, hmtlcolors[i].color, hmtlcolors[i].group));
	}

	int default_color[16] = {11,  13,  16,  18,
							 25,  30,  43,  40,
							 67,  76,  89,  99,
							109, 119, 129, 139};

	for (int i = 0; i < 16; i++)
		g_default_color[i] = hmtlcolors[default_color[i]].color;
}

COLORREF get_color(CString text)
{
	for (int i = 0; i < g_color.size(); i++)
	{
		if (g_color[i].name == text)
			return g_color[i].color;
	}

	COLORREF color;
	if ((color = _tcstol(text, NULL, 16)) == 0)
		color = 0x00ffffff;  // default is white
	else
		color = rgb_bgr(color);

	return color;
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

//해당 색상의 이름값이 있으면 찾아서 리턴한다.
//없거나 hex가 true이면 16진수 표현값으로 리턴한다.
CString	color_str(COLORREF cr, bool hex)
{
	CString str;

	str.Format(_T("%02x%02x%02x"), GetRValue(cr), GetGValue(cr), GetBValue(cr));
	if (!hex)
	{
		for (int i = 0; i < g_color.size(); i++)
		{
			if (cr == hmtlcolors[i].color)
			{
				return hmtlcolors[i].name;
			}
		}
	}

	return str;
}
