#ifndef _COLOR_MAP_H
#define _COLOR_MAP_H

#include <Afxwin.h>
#include <afxcmn.h>
//#include <Afxdisp.h>
#include <gdiplus.h>
#include <deque>
#include <unordered_map>

/*
* 2024.8.30
- CListCtrl, CTreeCtrl 등 MFC 및 사용자 컨트롤등에서 컬러 테마 관련 코드를 매번 추가하다보니
  중복도 많아지고 일관되지 않아 CSCColorTheme이라는 클래스로 공통 처리하고자 함.
  일부 색상 변수는 특정 컨트롤에만 해당되는 경우도 있음.

  //사용할 클래스의 .h파일에 인스턴스 선언시 this를 넘겨주는 이유는
  //color theme을 적용할 때 color_theme_default라면
  //대상 클래스가 CTreeCtrl, CListCtrl등이면 배경이 COLOR_WINDOW로 세팅되고
  //CButton, CStatic, CDialog등이면 COLOR_BTNFACE로 세팅되야 하므로 이를 판단하기 위함.
  CSCColorTheme	m_theme = CSCColorTheme(this);

  //.cpp에서 다음과 같이 테마를 변경해준다.
  m_theme.set_color_theme(CSCColorTheme::color_theme_dark);

- 지원되는 color theme 리스트를 얻기 위해서는 get_color_theme_list()의 리턴값을 이용한다.
*/

class CSCColorTheme
{
public :
	CSCColorTheme(CWnd* pWnd)
	{
		m_parent = pWnd;
		set_color_theme(color_theme_default);
	};

	enum SC_COLOR_THEMES
	{
		color_theme_default = 0,	//윈도우 테마를 따름
		color_theme_gray,
		color_theme_dark_gray,
		color_theme_dark,
	};

	int		get_color_theme() { return cur_theme; }
	void	set_color_theme(int color_theme);
	static std::deque<CString> get_color_theme_list();

	Gdiplus::Color	cr_text;
	Gdiplus::Color	cr_text_hover;
	Gdiplus::Color	cr_text_dropHilited;
	Gdiplus::Color	cr_text_selected;
	Gdiplus::Color	cr_text_selected_inactive;

	Gdiplus::Color	cr_selected_border;

	Gdiplus::Color	cr_back;					//BTNFACE	: for CDialog, CButton, CStatic...
	Gdiplus::Color	cr_back_hover;				//= hover, over, track_select...
	Gdiplus::Color	cr_back_dropHilited;
	Gdiplus::Color	cr_back_selected;
	Gdiplus::Color	cr_back_selected_inactive;
	Gdiplus::Color	cr_back_selected_border;
	Gdiplus::Color	cr_back_alternated;

	//CListCtrl에서만 사용될것으로 예상.
	Gdiplus::Color	cr_header_text;
	Gdiplus::Color	cr_header_back;
	std::deque<Gdiplus::Color> cr_percentage_bar;	//percentage bar graph color
	Gdiplus::Color	cr_progress;					//progress bar
	Gdiplus::Color	cr_progress_text;				//progress text

protected:
	CWnd*			m_parent = NULL;
	int				cur_theme = color_theme_default;
};

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

#define		GRAY_FACE	::GetSysColor(COLOR_3DFACE)
#define		GRAY32		GRAY(32)
#define		GRAY64		GRAY(64)
#define		GRAY128		GRAY(128)
#define		GRAY160		GRAY(160)
#define		GRAY192		GRAY(192)
#define		GRAY208		GRAY(208)
#define		GRAY224		GRAY(224)
#define		GRAY(x)		RGB((x), (x), (x))
#define		gGRAY(x)	Gdiplus::Color(255, (x), (x), (x))

//std::map으로 컬러 표현
//참조 방법 : cr = g_cr["white"]
//컬러 이름과 값을 열거 가능
//컬러값으로 이름 추출(또는 가장 유사한 색 이름 가능)
//맵의 []연산자 단점 : g_cr["asdf"]로 접근하면 해당 컬러가 없음에도 "asdf"를 키로 하는 항목이 추가됨!
//정확히 검색하기 위해서는 find()를 이용해야 한다.
//데이터 추가 시 unordered_map을 이용해도 순서가 보장되지 않는다.
//https://80000coding.oopy.io/8af406a3-b3b1-4f3f-b190-2937b23684ed
extern std::unordered_map<char*, COLORREF> g_cr;

//enum으로 컬러 표현
//참조 방법 : cr = white
//모든 멤버 열거 불가능. 별도의 char[]를 선언하고 동일하게 이름을 넣어놔야 함.
//값으로 컬러이름 출력 불가
enum Colors
{
	//abgr 차례
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
//두 색의 중간 비율 색상값을 구한다. (ratio가 0.0이면 cr1이, 1.0이면 cr2가 리턴된다.)
COLORREF		get_color(COLORREF cr1, COLORREF cr2, double ratio);
Gdiplus::Color	get_color(Gdiplus::Color cr1, Gdiplus::Color cr2, double ratio);

//"FF0000"과 같은 컬러 문자열을 COLORREF로 변환
COLORREF		get_color(CString cr);
//컬러값을 "FF0000"과 같은 문자열로 리턴한다.
CString			get_color_string(COLORREF cr);

//보색
COLORREF		get_complementary_color(COLORREF cr);

//rgb 평균값 리턴
uint8_t			gray_value(uint8_t r, uint8_t g, uint8_t b);
//rgb 평균값 리턴
uint8_t			gray_value(COLORREF cr);
uint8_t			gray_value(Gdiplus::Color cr);
//rgb 평균인 컬러값 리턴
COLORREF		gray_color(COLORREF cr);
Gdiplus::Color	gray_color(Gdiplus::Color cr);

double			color_similarity_distance(COLORREF c1, COLORREF c2);
COLORREF		get_default_color(int index);
COLORREF		get_random_color();

//r,g,b : 0 ~ 255, fH : 0 ~ 360, fS, fV : 0.0f ~ 1.0f
void			rgb2hsv(int r, int g, int b, float& fH, float& fS, float& fV);
//r,g,b : 0 ~ 255, fH : 0 ~ 360, fS, fV : 0.0f ~ 1.0f
void			hsv2rgb(float fH, float fS, float fV, int &r, int &g, int &b);
COLORREF		hsv2rgb(float fH, float fS = 1.0f, float fV = 1.0f);

Gdiplus::Color	RGB2gpColor(COLORREF cr, BYTE alpha = 255);

//cr컬러에서 a,r,g,b 중 한 색을 value로 변경한다.
void			set_color(Gdiplus::Color &cr, int argb, BYTE value);
Gdiplus::Color	get_color(Gdiplus::Color cr, int argb, BYTE value);

//red ~ green 범위에서 37%일때의 색상은? get_color(0, 120, 37); (0:red, 120:green of hue)
//hue : 0(red), 60(yellow), 120(green), 180(cyan), 240(blue), 300(violet), 360(red)
COLORREF		get_color(int hue_start, int hue_end, int percent, float saturation = 1.0f, float value = 1.0f);
int				get_hue(COLORREF cr);

//주어진 컬러와 가장 유사한 표준색의 이름을 리턴.
CString			get_color_name_of_closest(COLORREF cr, COLORREF *cr_closest = NULL);

extern COLORREF g_default_color[16];
#endif
