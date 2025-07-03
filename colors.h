#ifndef _COLOR_MAP_H
#define _COLOR_MAP_H

#include <Afxwin.h>
#include <afxcmn.h>
//#include <Afxdisp.h>
#include <gdiplus.h>
#include <deque>
#include <unordered_map>
#include <string>
#include <algorithm>

/*
* 2024.8.30
- CListCtrl, CTreeCtrl �� MFC �� ����� ��Ʈ�ѵ�� �÷� �׸� ���� �ڵ带 �Ź� �߰��ϴٺ���
  �ߺ��� �������� �ϰ����� �ʾ� CSCColorTheme�̶�� Ŭ������ ���� ó���ϰ��� ��.
  �Ϻ� ���� ������ Ư�� ��Ʈ�ѿ��� �ش�Ǵ� ��쵵 ����.

  .h ���Ͽ� �ν��Ͻ� ����(�ݵ�� this�� �Ķ���ͷ� �� ��)
  #include "../../colors.h"
	CSCColorTheme	m_theme = CSCColorTheme(this);	//m_theme(this); �� ����.
	...
	void		set_color_theme(int theme); �Լ� ����.

  //����� Ŭ������ .h���Ͽ� �ν��Ͻ� ����� this�� �Ѱ��ִ� ������
  //color theme�� ������ �� color_theme_default���
  //��� Ŭ������ CTreeCtrl, CListCtrl���̸� ����� COLOR_WINDOW�� ���õǰ�
  //CButton, CStatic, CDialog���̸� COLOR_BTNFACE�� ���õǾ� �ϹǷ� �̸� �Ǵ��ϱ� ����.


  .cpp�� set_color_theme()���� ������ ���� �׸��� �������ش�.
  m_theme.set_color_theme(CSCColorTheme::color_theme_dark);

- �����Ǵ� color theme ����Ʈ�� ��� ���ؼ��� get_color_theme_list()�� ���ϰ��� �̿��Ѵ�.
*/

// RGB -> YUV(YCbCr)
#define		RGB2Y(R, G, B) CLIP(((66 * (R) + 129 * (G) +  25 * (B) + 128) >> 8) +  16)
#define		RGB2U(R, G, B) CLIP(((-38 * (R) -  74 * (G) + 112 * (B) + 128) >> 8) + 128)
#define		RGB2V(R, G, B) CLIP(((112 * (R) -  94 * (G) -  18 * (B) + 128) >> 8) + 128)
#define		RGB_2Y(color) CLIP(((66 * GetRValue(color) + 129 * GetGValue(color) +  25 * GetBValue(color) + 128) >> 8) +  16)
#define		RGB_2U(color) CLIP(((-38 * GetRValue(color) -  74 * GetGValue(color) + 112 * GetBValue(color) + 128) >> 8) + 128)
#define		RGB_2V(color) CLIP(((112 * GetRValue(color) -  94 * GetGValue(color) -  18 * GetBValue(color) + 128) >> 8) + 128)

// YUV -> RGB
#define		C(Y) ((Y) - 16)
#define		D(U) ((U) - 128)
#define		E(V) ((V) - 128)

#define		YUV2R(Y, U, V) CLIP((298 * C(Y)              + 409 * E(V) + 128) >> 8)
#define		YUV2G(Y, U, V) CLIP((298 * C(Y) - 100 * D(U) - 208 * E(V) + 128) >> 8)
#define		YUV2B(Y, U, V) CLIP((298 * C(Y) + 516 * D(U)              + 128) >> 8)

// RGB -> YCbCr
#define		CRGB2Y(R, G, B) CLIP((19595 * R + 38470 * G + 7471 * B) >> 16)
#define		CRGB2Cb(R, G, B) CLIP((36962 * (B - CLIP((19595 * R + 38470 * G + 7471 * B) >> 16)) >> 16) + 128)
#define		CRGB2Cr(R, G, B) CLIP((46727 * (R - CLIP((19595 * R + 38470 * G + 7471 * B) >> 16)) >> 16) + 128)

// YCbCr -> RGB
#define		CYCbCr2R(Y, Cb, Cr) CLIP(Y + (91881 * Cr >> 16) - 179)
#define		CYCbCr2G(Y, Cb, Cr) CLIP(Y - ((22544 * Cb + 46793 * Cr) >> 16) + 135)
#define		CYCbCr2B(Y, Cb, Cr) CLIP(Y + (116129 * Cb >> 16) - 226)

//rgb<->bgr
#define		rgb_bgr(c) (((c & 0x00ff0000) >> 16) + (c & 0x0000ff00) + ((c & 0x000000ff) << 16))

//complementary color
#define		color_complementary(c) (~(c)&0x00ffffff)

#define		GRAY_FACE		::GetSysColor(COLOR_3DFACE)
#define		GRAY32			GRAY(32)
#define		GRAY64			GRAY(64)
#define		GRAY128			GRAY(128)
#define		GRAY160			GRAY(160)
#define		GRAY192			GRAY(192)
#define		GRAY208			GRAY(208)
#define		GRAY224			GRAY(224)
#define		GRAY(x)			RGB((x), (x), (x))
#define		gGRAY(x)		Gdiplus::Color(255, (x), (x), (x))
#define		gRGB(r,g,b)		Gdiplus::Color(255, (r), (g), (b))

//std::map���� �÷� ǥ��
//���� ��� : cr = g_cr["white"]
//�÷� �̸��� ���� ���� ����
//�÷������� �̸� ����(�Ǵ� ���� ������ �� �̸� ����)
//���� []������ ���� : g_cr["asdf"]�� �����ϸ� �ش� �÷��� �������� "asdf"�� Ű�� �ϴ� �׸��� �߰���!
//��Ȯ�� �˻��ϱ� ���ؼ��� find()�� �̿��ؾ� �Ѵ�.
//������ �߰� �� unordered_map�� �̿��ص� ������ ������� �ʴ´�.
//https://80000coding.oopy.io/8af406a3-b3b1-4f3f-b190-2937b23684ed
extern std::unordered_map<char*, COLORREF> g_cr;

//enum���� �÷� ǥ��
//���� ��� : cr = white
//��� ��� ���� �Ұ���. ������ char[]�� �����ϰ� �����ϰ� �̸��� �־���� ��.
//������ �÷��̸� ��� �Ұ�
enum Colors
{
	//abgr ����
	black = 0x00000000,
	dimgray = 0x00696969,
	gray = 0x00808080,
	darkgray = 0x00a9a9a9,
	silver = 0x00c0c0c0,
	lightgray = 0x00d3d3d3,
	gainsboro = 0x00dcdcdc,
	whitesmoke = 0x00f5f5f5,
	white = 0x00ffffff,
	snow = 0x00fafaff,
	ghostwhite = 0x00fff8f8,
	floralwhite = 0x00f0faff,
	linen = 0x00e6f0fa,
	antiquewhite = 0x00d7ebfa,
	papayawhip = 0x00d5efff,
	blanchedalmond = 0x00cdebff,
	bisque = 0x00c4e4ff,
	moccasin = 0x00b5e4ff,
	navajowhite = 0x00addeff,
	peachpuff = 0x00b9daff,
	mistyrose = 0x00e1e4ff,
	lavenderblush = 0x00f5f0ff,
	seashell = 0x00eef5ff,
	oldlace = 0x00e6f5fd,
	ivory = 0x00f0ffff,
	honeydew = 0x00f0fff0,
	mintcream = 0x00fafff5,
	azure = 0x00fffff0,
	aliceblue = 0x00fff8f0,
	lavender = 0x00fae6e6,
	lightsteelblue = 0x00dec4b0,
	lightslategray = 0x00998877,
	slategray = 0x00908070,
	steelblue = 0x00b48246,
	royalblue = 0x00e16941,
	midnightblue = 0x00701919,
	navy = 0x00800000,
	darkblue = 0x008b0000,
	mediumblue = 0x00cd0000,
	blue = 0x00ff0000,
	dodgerblue = 0x00ff901e,
	cornflowerblue = 0x00ed9564,
	deepskyblue = 0x00ffbf00,
	lightskyblue = 0x00face87,
	skyblue = 0x00ebce87,
	lightblue = 0x00e6d8ad,
	powderblue = 0x00e6e0b0,
	paleturquoise = 0x00eeeeaf,
	lightcyan = 0x00ffffe0,
	cyan = 0x00ffff00,
	aqua = 0x00ffff00,
	turquoise = 0x00d0e040,
	mediumturquoise = 0x00ccd148,
	darkturquoise = 0x00d1ce00,
	lightseagreen = 0x00aab220,
	cadetblue = 0x00a09e5f,
	darkcyan = 0x008b8b00,
	teal = 0x00808000,
	darkslategray = 0x004f4f2f,
	darkgreen = 0x00006400,
	green = 0x00007f00,
	forestgreen = 0x00228b22,
	seagreen = 0x00578b2e,
	mediumseagreen = 0x0071b33c,
	mediumaquamarine = 0x00aacd66,
	darkseagreen = 0x008fbc8f,
	aquamarine = 0x00d4ff7f,
	palegreen = 0x0098fb98,
	lightgreen = 0x0090ee90,
	springgreen = 0x007fff00,
	mediumspringgreen = 0x009afa00,
	lawngreen = 0x0000fc7c,
	chartreuse = 0x0000ff7f,
	greenyellow = 0x002fffad,
	lime = 0x0000ff00,
	limegreen = 0x0032cd32,
	yellowgreen = 0x0032cd9a,
	darkolivegreen = 0x002f6b55,
	olivedrab = 0x00238e6b,
	olive = 0x00008080,
	darkkhaki = 0x006bb7bd,
	palegoldenrod = 0x00aae8ee,
	cornsilk = 0x00dcf8ff,
	beige = 0x00dcf5f5,
	lightyellow = 0x00e0ffff,
	lightgoldenrodyellow = 0x00d2fafa,
	lemonchiffon = 0x00cdfaff,
	wheat = 0x00b3def5,
	burlywood = 0x0087b8de,
	tan_color = 0x008cb4d2,
	khaki = 0x008ce6f0,
	yellow = 0x0000ffff,
	gold = 0x0000d7ff,
	orange = 0x0000a5ff,
	sandybrown = 0x0060a4f4,
	darkorange = 0x00008cff,
	goldenrod = 0x0020a5da,
	peru = 0x003f85cd,
	darkgoldenrod = 0x000b86b8,
	chocolate = 0x001e69d2,
	sienna = 0x002d52a0,
	saddlebrown = 0x0013458b,
	maroon = 0x00000080,
	darkred = 0x0000008b,
	brown = 0x002a2aa5,
	firebrick = 0x002222b2,
	indianre = 0x005c5ccd,
	rosybrown = 0x008f8fbc,
	darksalmon = 0x007a96e9,
	lightcoral = 0x008080f0,
	salmon = 0x007280fa,
	lightsalmon = 0x007aa0ff,
	coral = 0x00507fff,
	tomato = 0x004763ff,
	orangered = 0x000045ff,
	red = 0x000000ff,
	crimson = 0x003c14dc,
	mediumvioletred = 0x008515c7,
	deeppink = 0x009314ff,
	hotpink = 0x00b469ff,
	palevioletred = 0x009370db,
	pink = 0x00cbc0ff,
	lightpink = 0x00c1b6ff,
	thistle = 0x00d8bfd8,
	magenta = 0x00ff00ff,
	fuchsia = 0x00ff00ff,
	violet = 0x00ee82ee,
	plum = 0x00dda0dd,
	orchid = 0x00d670da,
	mediumorchid = 0x00d355ba,
	darkorchid = 0x00cc3299,
	darkviolet = 0x00d30094,
	darkmagenta = 0x008b008b,
	purple = 0x00800080,
	indigo = 0x0082004b,
	darkslateblue = 0x008b3d48,
	blueviolet = 0x00e22b8a,
	mediumpurple = 0x00db7093,
	slateblue = 0x00cd5a6a,
	mediumslateblue = 0x00ee687b,

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

COLORREF		get_color(COLORREF crOrigin, int nOffset);
Gdiplus::Color	get_color(Gdiplus::Color crOrigin, int nOffset);
//�� ���� �߰� ���� ������ ���Ѵ�. (ratio�� 0.0�̸� cr1��, 1.0�̸� cr2�� ���ϵȴ�.)
COLORREF		get_color(COLORREF cr1, COLORREF cr2, double ratio);
Gdiplus::Color	get_color(Gdiplus::Color cr1, Gdiplus::Color cr2, double ratio);
Gdiplus::Color	get_color(COLORREF rgb);

//�÷� �̸����� Gdiplus::Color�� �����Ѵ�. ��ҹ��ڸ� �������� ������ �̸��� ������ �������� �����Ѵ�.
//get_color("red") �Ǵ� get_color(_T("Red")) -> Gdiplus::Color(255, 255, 0, 0);
Gdiplus::Color	get_color(CString cr_str);
Gdiplus::Color	get_color(std::string cr_name);

//#RRGGBB�� ���� 16���� �÷� ���ڿ��� COLORREF�� ��ȯ
COLORREF		get_color_from_hexadecimal(CString cr);
//#RRGGBB �Ǵ� #AARRGGBB�� ���� 16���� �÷� ���ڿ��� Gdiplus::Color�� ��ȯ
Gdiplus::Color	get_gcolor_from_hexadecimal(CString cr);

//�÷����� "FF0000"�� ���� ���ڿ��� �����Ѵ�.
//#FF0000�� ���� ���Ϲް��� �Ѵٸ� prefix = _T("#")���� ȣ���Ѵ�.
CString			get_color_string(COLORREF cr, bool upper_case = true, CString prefix = _T(""));

//����
COLORREF		get_complementary_color(COLORREF cr);
Gdiplus::Color	get_complementary_gcolor(Gdiplus::Color cr);

//�������� �޸� ��⿡ ���� black or white�� �����Ѵ�.
//� ������ Ȯ���� ���еǴ� �÷��� �������� �ϸ� 128, 128, 128�� ���� ������ ���� ���� ������ ���� �ǹǷ� ���е��� �ʴ´�.
COLORREF		get_distinct_color(COLORREF cr);
//�������� �޸� ��⿡ ���� black or white�� �����Ѵ�.
//� ������ Ȯ���� ���еǴ� �÷��� �������� �ϸ� 128, 128, 128�� ���� ������ ���� ���� ������ ���� �ǹǷ� ���е��� �ʴ´�.
Gdiplus::Color	get_distinct_color(Gdiplus::Color cr);

//rgb ��հ� ����
uint8_t			gray_value(uint8_t r, uint8_t g, uint8_t b);
//rgb ��հ� ����
uint8_t			gray_value(COLORREF cr);
uint8_t			gray_value(Gdiplus::Color cr);
//rgb ����� �÷��� ����
COLORREF		gray_color(COLORREF cr);
Gdiplus::Color	gray_color(Gdiplus::Color cr);

double			get_distance(COLORREF c1, COLORREF c2);
double			get_distance(Gdiplus::Color c1, Gdiplus::Color c2);

COLORREF		get_default_color(int index);
COLORREF		get_random_color();
Gdiplus::Color	get_random_gcolor(bool use_alpha = false);

//r,g,b : 0 ~ 255, fH : 0 ~ 360, fS, fV : 0.0f ~ 1.0f
void			rgb2hsv(int r, int g, int b, float& fH, float& fS, float& fV);
//r,g,b : 0 ~ 255, fH : 0 ~ 360, fS, fV : 0.0f ~ 1.0f
void			hsv2rgb(float fH, float fS, float fV, int &r, int &g, int &b);
COLORREF		hsv2rgb(float fH, float fS = 1.0f, float fV = 1.0f);

Gdiplus::Color	RGB2gpColor(COLORREF cr, BYTE alpha = 255);

//cr�÷����� a,r,g,b �� �� ���� value�� �����Ѵ�.
void			set_color(Gdiplus::Color &cr, int argb, BYTE value);
Gdiplus::Color	get_color(Gdiplus::Color cr, int argb, BYTE value);

//red ~ green �������� 37%�϶��� ������? get_color(0, 120, 37); (0:red, 120:green of hue)
//hue : 0(red), 60(yellow), 120(green), 180(cyan), 240(blue), 300(violet), 360(red)
COLORREF		get_color(int hue_start, int hue_end, int percent, float saturation = 1.0f, float value = 1.0f);
int				get_hue(COLORREF cr);

//�־��� �÷��� ���� ������ ǥ�ػ��� �̸��� ����.
CString			get_nearest_color_name(COLORREF cr, COLORREF * cr_nearest = NULL);

Gdiplus::Color	get_sys_color(int index);

extern COLORREF g_default_color[16];


class CSCColorTheme
{
public:
	CSCColorTheme(CWnd* pWnd, int theme = color_theme_default)
	{
		m_parent = pWnd;
		set_color_theme(color_theme_default);
	};

	enum SC_COLOR_THEMES
	{
		color_theme_default = 0,		//�⺻ ������ �׸��� ����
		color_theme_white,				//���� �������� �������� dlg�� 3DFACE�� �ƴ� white�� ��
		color_theme_gray,
		color_theme_dark_gray,
		color_theme_dark,
		color_theme_popup_folder_list,	//CPathCtrl���� ǥ���ϴ� ���� ����Ʈ �˾��� Ưȭ�� �׸��μ� �Ϲ����� �׸��� �ƴϹǷ� get_color_theme_list()�� ������� ���Ե��� �ʴ´�.
		color_theme_linkmemine,
		color_theme_linkmemine_se,
		color_theme_anysupport,
		color_theme_helpu,
		color_theme_pcanypro,
		color_theme_custom,
	};

	//���� ������ color theme �ε����� ����
	int		get_color_theme() { return m_cur_theme; }
	void	set_color_theme(int color_theme);
	static std::deque<CString> get_color_theme_list();

	Gdiplus::Color	cr_text;
	Gdiplus::Color	cr_text_dim;					//�⺻ ���ڻ����� �帴�ϰ� ǥ���Ǵ� �׸�
	Gdiplus::Color	cr_text_hover;
	Gdiplus::Color	cr_text_dropHilited;
	Gdiplus::Color	cr_text_selected;
	Gdiplus::Color	cr_text_selected_inactive;

	Gdiplus::Color	cr_back;						//BTNFACE	: for CDialog, CButton, CStatic...
	Gdiplus::Color	cr_back_hover;					//= hover = over = track_select... �پ��� ���� �ǹ̰� ������ hover�� ��������.
	Gdiplus::Color	cr_back_dropHilited;
	Gdiplus::Color	cr_back_selected;
	Gdiplus::Color	cr_back_selected_inactive;
	Gdiplus::Color	cr_back_alternate;				//list�� ��� ¦������, Ȧ������ ������ ������ ǥ���ϴ� ����

	Gdiplus::Color	cr_selected_border;
	Gdiplus::Color	cr_border;

	//caption�� ���� dialog based���� ���
	Gdiplus::Color	cr_title_text;
	Gdiplus::Color	cr_title_back;
	Gdiplus::Color	cr_sys_buttons_hover_back;
	Gdiplus::Color	cr_sys_buttons_down_back;

	//CListCtrl������ ���ɰ����� ����.
	Gdiplus::Color	cr_header_text;
	Gdiplus::Color	cr_header_back;
	std::deque<Gdiplus::Color> cr_percentage_bar;	//percentage bar graph color
	Gdiplus::Color	cr_progress;					//progress bar
	//Gdiplus::Color	cr_progress_text;				//progress text

protected:
	CWnd* m_parent = NULL;
	int				m_cur_theme = color_theme_default;
};

//color name���� Gdiplus::Color ���� ���ϱ� ���� ����.
//get_color(cr_str)�� ���� ȣ���ϰ��� ������.
//map�̳� unordered_map�� insert order�� �������� �����Ƿ� �߰��� ������ �ȷ�Ʈ�� ǥ���� �� ���� ������ �ִ�.
//std::vector<CString, Gdiplus::Color> �� ���� �����ϸ� ������ �����ϸ�
//map�� ���� ū ������ first Ű������ second�� ���ϴ� ���ε� �̴� std::find()�� ���� �����ϴ�.
class CSCColorList
{
public:
	CSCColorList()
	{
		get_color_list().emplace_back("MediumVioletRed", Gdiplus::Color::MediumVioletRed);
		get_color_list().emplace_back("DeepPink", Gdiplus::Color::DeepPink);
		get_color_list().emplace_back("PaleVioletRed", Gdiplus::Color::PaleVioletRed);
		get_color_list().emplace_back("HotPink", Gdiplus::Color::HotPink);
		get_color_list().emplace_back("LightPink", Gdiplus::Color::LightPink);
		get_color_list().emplace_back("Pink", Gdiplus::Color::Pink);

		get_color_list().emplace_back("DarkRed", Gdiplus::Color::DarkRed);
		get_color_list().emplace_back("Red", Gdiplus::Color::Red);
		get_color_list().emplace_back("Firebrick", Gdiplus::Color::Firebrick);
		get_color_list().emplace_back("Crimson", Gdiplus::Color::Crimson);
		get_color_list().emplace_back("IndianRed", Gdiplus::Color::IndianRed);
		get_color_list().emplace_back("LightCoral", Gdiplus::Color::LightCoral);
		get_color_list().emplace_back("Salmon", Gdiplus::Color::Salmon);
		get_color_list().emplace_back("DarkSalmon", Gdiplus::Color::DarkSalmon);
		get_color_list().emplace_back("LightSalmon", Gdiplus::Color::LightSalmon);

		get_color_list().emplace_back("OrangeRed", Gdiplus::Color::OrangeRed);
		get_color_list().emplace_back("Tomato", Gdiplus::Color::Tomato);
		get_color_list().emplace_back("DarkOrange", Gdiplus::Color::DarkOrange);
		get_color_list().emplace_back("Coral", Gdiplus::Color::Coral);
		get_color_list().emplace_back("Orange", Gdiplus::Color::Orange);

		get_color_list().emplace_back("DarkKhaki", Gdiplus::Color::DarkKhaki);
		get_color_list().emplace_back("Gold", Gdiplus::Color::Gold);
		get_color_list().emplace_back("Khaki", Gdiplus::Color::Khaki);
		get_color_list().emplace_back("PeachPuff", Gdiplus::Color::PeachPuff);
		get_color_list().emplace_back("Yellow", Gdiplus::Color::Yellow);
		get_color_list().emplace_back("PaleGoldenrod", Gdiplus::Color::PaleGoldenrod);
		get_color_list().emplace_back("Moccasin", Gdiplus::Color::Moccasin);
		get_color_list().emplace_back("PapayaWhip", Gdiplus::Color::PapayaWhip);
		get_color_list().emplace_back("LightGoldenrodYellow", Gdiplus::Color::LightGoldenrodYellow);
		get_color_list().emplace_back("LemonChiffon", Gdiplus::Color::LemonChiffon);
		get_color_list().emplace_back("LightYellow", Gdiplus::Color::LightYellow);

		get_color_list().emplace_back("Maroon", Gdiplus::Color::Maroon);
		get_color_list().emplace_back("Brown", Gdiplus::Color::Brown);
		get_color_list().emplace_back("SaddleBrown", Gdiplus::Color::SaddleBrown);
		get_color_list().emplace_back("Sienna", Gdiplus::Color::Sienna);
		get_color_list().emplace_back("Chocolate", Gdiplus::Color::Chocolate);
		get_color_list().emplace_back("DarkGoldenrod", Gdiplus::Color::DarkGoldenrod);
		get_color_list().emplace_back("Peru", Gdiplus::Color::Peru);
		get_color_list().emplace_back("RosyBrown", Gdiplus::Color::RosyBrown);
		get_color_list().emplace_back("Goldenrod", Gdiplus::Color::Goldenrod);
		get_color_list().emplace_back("SandyBrown", Gdiplus::Color::SandyBrown);
		get_color_list().emplace_back("Tan", Gdiplus::Color::Tan);
		get_color_list().emplace_back("BurlyWood", Gdiplus::Color::BurlyWood);
		get_color_list().emplace_back("Wheat", Gdiplus::Color::Wheat);
		get_color_list().emplace_back("NavajoWhite", Gdiplus::Color::NavajoWhite);
		get_color_list().emplace_back("Bisque", Gdiplus::Color::Bisque);
		get_color_list().emplace_back("BlanchedAlmond", Gdiplus::Color::BlanchedAlmond);
		get_color_list().emplace_back("Cornsilk", Gdiplus::Color::Cornsilk);

		get_color_list().emplace_back("Indigo", Gdiplus::Color::Indigo);
		get_color_list().emplace_back("Purple", Gdiplus::Color::Purple);
		get_color_list().emplace_back("DarkMagenta", Gdiplus::Color::DarkMagenta);
		get_color_list().emplace_back("DarkViolet", Gdiplus::Color::DarkViolet);
		get_color_list().emplace_back("DarkSlateBlue", Gdiplus::Color::DarkSlateBlue);
		get_color_list().emplace_back("BlueViolet", Gdiplus::Color::BlueViolet);
		get_color_list().emplace_back("DarkOrchid", Gdiplus::Color::DarkOrchid);
		get_color_list().emplace_back("Fuchsia", Gdiplus::Color::Fuchsia);
		get_color_list().emplace_back("Magenta", Gdiplus::Color::Magenta);
		get_color_list().emplace_back("SlateBlue", Gdiplus::Color::SlateBlue);
		get_color_list().emplace_back("MediumSlateBlue", Gdiplus::Color::MediumSlateBlue);
		get_color_list().emplace_back("MediumOrchid", Gdiplus::Color::MediumOrchid);
		get_color_list().emplace_back("MediumPurple", Gdiplus::Color::MediumPurple);
		get_color_list().emplace_back("Orchid", Gdiplus::Color::Orchid);
		get_color_list().emplace_back("Violet", Gdiplus::Color::Violet);
		get_color_list().emplace_back("Plum", Gdiplus::Color::Plum);
		get_color_list().emplace_back("Thistle", Gdiplus::Color::Thistle);
		get_color_list().emplace_back("Lavender", Gdiplus::Color::Lavender);

		get_color_list().emplace_back("MidnightBlue", Gdiplus::Color::MidnightBlue);
		get_color_list().emplace_back("Navy", Gdiplus::Color::Navy);
		get_color_list().emplace_back("DarkBlue", Gdiplus::Color::DarkBlue);
		get_color_list().emplace_back("MediumBlue", Gdiplus::Color::MediumBlue);
		get_color_list().emplace_back("Blue", Gdiplus::Color::Blue);
		get_color_list().emplace_back("RoyalBlue", Gdiplus::Color::RoyalBlue);
		get_color_list().emplace_back("SteelBlue", Gdiplus::Color::SteelBlue);
		get_color_list().emplace_back("DodgerBlue", Gdiplus::Color::DodgerBlue);
		get_color_list().emplace_back("DeepSkyBlue", Gdiplus::Color::DeepSkyBlue);
		get_color_list().emplace_back("CornflowerBlue", Gdiplus::Color::CornflowerBlue);
		get_color_list().emplace_back("SkyBlue", Gdiplus::Color::SkyBlue);
		get_color_list().emplace_back("LightSkyBlue", Gdiplus::Color::LightSkyBlue);
		get_color_list().emplace_back("LightSteelBlue", Gdiplus::Color::LightSteelBlue);
		get_color_list().emplace_back("LightBlue", Gdiplus::Color::LightBlue);
		get_color_list().emplace_back("PowderBlue", Gdiplus::Color::PowderBlue);

		get_color_list().emplace_back("Teal", Gdiplus::Color::Teal);
		get_color_list().emplace_back("DarkCyan", Gdiplus::Color::DarkCyan);
		get_color_list().emplace_back("LightSeaGreen", Gdiplus::Color::LightSeaGreen);
		get_color_list().emplace_back("CadetBlue", Gdiplus::Color::CadetBlue);
		get_color_list().emplace_back("DarkTurquoise", Gdiplus::Color::DarkTurquoise);
		get_color_list().emplace_back("MediumTurquoise", Gdiplus::Color::MediumTurquoise);
		get_color_list().emplace_back("Turquoise", Gdiplus::Color::Turquoise);
		get_color_list().emplace_back("Aqua", Gdiplus::Color::Aqua);
		get_color_list().emplace_back("Cyan", Gdiplus::Color::Cyan);
		get_color_list().emplace_back("Aquamarine", Gdiplus::Color::Aquamarine);
		get_color_list().emplace_back("PaleTurquoise", Gdiplus::Color::PaleTurquoise);
		get_color_list().emplace_back("LightCyan", Gdiplus::Color::LightCyan);

		get_color_list().emplace_back("DarkGreen", Gdiplus::Color::DarkGreen);
		get_color_list().emplace_back("Green", Gdiplus::Color::Green);
		get_color_list().emplace_back("DarkOliveGreen", Gdiplus::Color::DarkOliveGreen);
		get_color_list().emplace_back("ForestGreen", Gdiplus::Color::ForestGreen);
		get_color_list().emplace_back("SeaGreen", Gdiplus::Color::SeaGreen);
		get_color_list().emplace_back("Olive", Gdiplus::Color::Olive);
		get_color_list().emplace_back("OliveDrab", Gdiplus::Color::OliveDrab);
		get_color_list().emplace_back("MediumSeaGreen", Gdiplus::Color::MediumSeaGreen);
		get_color_list().emplace_back("LimeGreen", Gdiplus::Color::LimeGreen);
		get_color_list().emplace_back("Lime", Gdiplus::Color::Lime);
		get_color_list().emplace_back("SpringGreen", Gdiplus::Color::SpringGreen);
		get_color_list().emplace_back("MediumSpringGreen", Gdiplus::Color::MediumSpringGreen);
		get_color_list().emplace_back("DarkSeaGreen", Gdiplus::Color::DarkSeaGreen);
		get_color_list().emplace_back("MediumAquamarine", Gdiplus::Color::MediumAquamarine);
		get_color_list().emplace_back("YellowGreen", Gdiplus::Color::YellowGreen);
		get_color_list().emplace_back("LawnGreen", Gdiplus::Color::LawnGreen);
		get_color_list().emplace_back("Chartreuse", Gdiplus::Color::Chartreuse);
		get_color_list().emplace_back("LightGreen", Gdiplus::Color::LightGreen);
		get_color_list().emplace_back("GreenYellow", Gdiplus::Color::GreenYellow);
		get_color_list().emplace_back("PaleGreen", Gdiplus::Color::PaleGreen);

		get_color_list().emplace_back("MistyRose", Gdiplus::Color::MistyRose);
		get_color_list().emplace_back("AntiqueWhite", Gdiplus::Color::AntiqueWhite);
		get_color_list().emplace_back("Linen", Gdiplus::Color::Linen);
		get_color_list().emplace_back("Beige", Gdiplus::Color::Beige);
		get_color_list().emplace_back("WhiteSmoke", Gdiplus::Color::WhiteSmoke);
		get_color_list().emplace_back("LavenderBlush", Gdiplus::Color::LavenderBlush);
		get_color_list().emplace_back("OldLace", Gdiplus::Color::OldLace);
		get_color_list().emplace_back("AliceBlue", Gdiplus::Color::AliceBlue);
		get_color_list().emplace_back("SeaShell", Gdiplus::Color::SeaShell);
		get_color_list().emplace_back("GhostWhite", Gdiplus::Color::GhostWhite);
		get_color_list().emplace_back("Honeydew", Gdiplus::Color::Honeydew);
		get_color_list().emplace_back("FloralWhite", Gdiplus::Color::FloralWhite);
		get_color_list().emplace_back("Azure", Gdiplus::Color::Azure);
		get_color_list().emplace_back("MintCream", Gdiplus::Color::MintCream);
		get_color_list().emplace_back("Snow", Gdiplus::Color::Snow);
		get_color_list().emplace_back("Ivory", Gdiplus::Color::Ivory);
		get_color_list().emplace_back("White", Gdiplus::Color::White);

		get_color_list().emplace_back("Black", Gdiplus::Color::Black);
		get_color_list().emplace_back("DarkSlateGray", Gdiplus::Color::DarkSlateGray);
		get_color_list().emplace_back("DimGray", Gdiplus::Color::DimGray);
		get_color_list().emplace_back("SlateGray", Gdiplus::Color::SlateGray);
		get_color_list().emplace_back("Gray", Gdiplus::Color::Gray);
		get_color_list().emplace_back("LightSlateGray", Gdiplus::Color::LightSlateGray);
		get_color_list().emplace_back("DarkGray", Gdiplus::Color::DarkGray);
		get_color_list().emplace_back("Silver", Gdiplus::Color::Silver);
		get_color_list().emplace_back("LightGray", Gdiplus::Color::LightGray);
		get_color_list().emplace_back("Gainsboro", Gdiplus::Color::Gainsboro);
	}

	// auto-cast Error to integer error code
	//operator Gdiplus::Color() { return cr; }

	//static std::unordered_map<std::string, Gdiplus::Color> get_color_map()
	//{
	//	return get_color_map();
	//}

	static Gdiplus::Color get_color(std::string name)
	{
		//m_cr_map::iterator it = get_color_map().find(name);
		//���� ���� map.find()�� ���� ��ҹ��� ������ �ϹǷ�, ��ҹ��� ���о��� �˻��ϱ� ���� std::find_if�� _stricmp()�� ����Ѵ�.
		std::vector<std::pair<std::string, Gdiplus::Color>>::iterator it = std::find_if(get_color_list().begin(), get_color_list().end(),
			[&](const std::pair<std::string, Gdiplus::Color>& element) ->
			bool
			{
				return (_stricmp(element.first.c_str(), name.c_str()) == 0);
			});

		if (it == get_color_list().end())
			return Gdiplus::Color::Black;

		return it->second;
	}

	//get color name by (r, g, b) value. alpha�� �����Ѵ�.
	//exactly : true�̸� ��Ȯ�� ��ġ�ϴ� ���� ã��, false�̸� ���� ������ ������ �̸��� "near : "�� �ٿ��� �����Ѵ�.
	static std::string get_color_name(Gdiplus::Color cr, bool exactly = false)
	{
		double distance = 99999999.0;
		std::string color_name;

		std::vector<std::pair<std::string, Gdiplus::Color>>::iterator it = std::find_if(get_color_list().begin(), get_color_list().end(),
			[&](const std::pair<std::string, Gdiplus::Color>& element) ->
			bool
			{
				if (exactly)
				{
					return (element.second.GetR() == cr.GetR() &&
						element.second.GetG() == cr.GetG() &&
						element.second.GetB() == cr.GetB());
				}

				double dist = get_distance(element.second, cr);
				//sqrt( (element.second.GetR() - cr.GetR()) * (element.second.GetR() - cr.GetR()) +
				//				(element.second.GetG() - cr.GetG()) * (element.second.GetG() - cr.GetG()) +
				//				(element.second.GetB() - cr.GetB()) * (element.second.GetB() - cr.GetB()));

				if (dist < distance)
				{
					distance = dist;
					color_name = element.first;
				}
				return false; // continue searching
			});

		if (it != get_color_list().end())
			return it->first;

		if (!exactly)
		{
			if (distance == 0.0f)
				return color_name;
			else
				return "near : " + color_name;
		}

		return "Unknown Color";
	}

	static std::vector<std::pair<std::string, Gdiplus::Color>>& get_color_list()
	{
		static std::vector<std::pair<std::string, Gdiplus::Color>> m_cr_list;
		return m_cr_list;
	}

protected:
	//static std::vector<std::pair<std::string, Gdiplus::Color>> m_cr_list;

	//Gdiplus::Color cr;
	//std::string name;
};

#endif
