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
- CListCtrl, CTreeCtrl 등 MFC 및 사용자 컨트롤등에서 컬러 테마 관련 코드를 매번 추가하다보니
  중복도 많아지고 일관되지 않아 CSCColorTheme이라는 클래스로 공통 처리하고자 함.
  일부 색상 변수는 특정 컨트롤에만 해당되는 경우도 있음.

  .h 파일에 인스턴스 선언(반드시 this를 파라미터로 줄 것)
  #include "../../colors.h"
	CSCColorTheme	m_theme = CSCColorTheme(this);	//m_theme(this); 는 오류.
	...
	void		set_color_theme(int theme); 함수 선언.

  //사용할 클래스의 .h파일에 인스턴스 선언시 this를 넘겨주는 이유는
  //color theme을 적용할 때 color_theme_default라면
  //대상 클래스가 CTreeCtrl, CListCtrl등이면 배경이 COLOR_WINDOW로 세팅되고
  //CButton, CStatic, CDialog등이면 COLOR_BTNFACE로 세팅되야 하므로 이를 판단하기 위함.


  .cpp의 set_color_theme()에서 다음과 같이 테마를 변경해준다.
  m_theme.set_color_theme(CSCColorTheme::color_theme_dark);

- 지원되는 color theme 리스트를 얻기 위해서는 get_color_theme_list()의 리턴값을 이용한다.
*/

#define		CR_SUCCESS	royalblue
#define		CR_INFO		dimgray
#define		CR_WARNING	crimson
#define		CR_ERROR	red

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

struct RGBA
{
	::byte r;
	::byte g;
	::byte b;
	::byte a;
};
//std::map으로 컬러 표현
//참조 방법 : cr = g_cr["white"]
//컬러 이름과 값을 열거 가능
//컬러값으로 이름 추출(또는 가장 유사한 색 이름 가능)
//맵의 []연산자 단점 : g_cr["asdf"]로 접근하면 해당 컬러가 없음에도 "asdf"를 키로 하는 항목이 추가됨!
//정확히 검색하기 위해서는 find()를 이용해야 한다.
//데이터 추가 시 unordered_map을 이용해도 순서가 보장되지 않는다.
//https://80000coding.oopy.io/8af406a3-b3b1-4f3f-b190-2937b23684ed
extern std::unordered_map<const char*, COLORREF> g_cr;

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
	indianred = 0x005c5ccd,
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
};

COLORREF		get_color(COLORREF crOrigin, int nOffset);
Gdiplus::Color	get_color(Gdiplus::Color crOrigin, int nOffset);
//두 색의 중간 비율 색상값을 구한다. (ratio가 0.0이면 cr1이, 1.0이면 cr2가 리턴된다.)
COLORREF		get_color(COLORREF cr1, COLORREF cr2, double ratio);
Gdiplus::Color	get_color(Gdiplus::Color cr1, Gdiplus::Color cr2, double ratio);
Gdiplus::Color	get_color(COLORREF rgb);

//기준색보다 밝거나 어두운 색을 리턴한다.
//기준색이 밝은색 계열이면 그보다 offset만큼 좀 더 어두운 색을,
//기준색이 어두운 계열이면 그보다 offset만큼 좀 더 밝은 색을 리턴한다.
Gdiplus::Color	get_weak_color(Gdiplus::Color cr, int offset = 32);

//발광색 정규화: 가장 큰 채널을 255 로 끌어올려(max-channel stretch) 어두운 색도 또렷이 보이게 한다.
//min_peak 은 분모 최소값(과증폭 방지) — 모든 채널이 min_peak 이하인 매우 어두운 색의 폭주를 막는다.
Gdiplus::Color	get_normalized_bright_color(Gdiplus::Color cr, int min_peak = 40);

//어떤 색의 ratio 배 색 (검정 기준 스케일). 각 RGB 채널 × ratio, alpha 보존, 0..255 clamp.
//ratio=1.0 → 원색, 0.8 → 80% 밝기(더 어둡게), 1.2 → 120%(더 밝게).
//"어떤 색의 80%" 의 가장 일반적 의미(밝기 80%) 가 이것이다.
Gdiplus::Color	get_ratio_color(Gdiplus::Color cr, float ratio);

//cr 을 흰색 쪽으로 t(0~1) 비율만큼 밝게 한다. HSL 의 L 만 흰색 방향으로 이동(new_L = L + (1-L)*t)하므로
//hue·채도 보존 + alpha 보존 — 가산(get_color +n)·곱셈(get_ratio_color) 과 달리 채널 포화로 인한 색조 드리프트가 없다.
//t=0 → 원색, t=0.1 → 흰색 쪽 10%, t=0.5 → 흰색 쪽 절반, t=1 → 흰색. 채도 높은 강조색을 같은 톤으로 밝게 할 때 사용.
//(어둡게는 곱셈이 검정 쪽으로 포화가 없어 hue 안전 → get_ratio_color(cr, <1) 를 쓴다.)
Gdiplus::Color	get_lightened_color(Gdiplus::Color cr, double t);

//테마 강도(intensity) 보간. 중성색(흰색) 기준으로 cr 의 편차를 level 배로 조절한다.
//level=1.0 → cr 그대로, 0.0 → 흰색(편차 0), 0.8 → 편차 80%(20% 옅게), >1.0 → 편차 확대(더 진하게).
//알파 보존. get_ratio_color 와 달리 *흰색* 기준이라 dark 테마를 옅게 하면 "덜 어두워진다"(밝아짐).
//CSCColorTheme::set_theme_level 이 모든 색에 일괄 적용해, 유사 톤 테마(dark_gray_medium 등)를
//따로 추가하지 않고 한 테마의 강도만 조절하는 데 쓴다.
Gdiplus::Color	get_leveled_color(Gdiplus::Color cr, float level);

//컬러 이름으로 Gdiplus::Color를 리턴한다. 대소문자를 구분하지 않으며 이름이 없으면 검정색을 리턴한다.
//get_color("red") 또는 get_color(_T("Red")) -> Gdiplus::Color(255, 255, 0, 0);
Gdiplus::Color	get_color(CString cr_str);
Gdiplus::Color	get_color(std::string cr_name);

#ifndef _USING_V110_SDK71_
D2D1::ColorF	get_d2color(Gdiplus::Color cr);
inline bool operator==(const D2D1_COLOR_F& a, const D2D1_COLOR_F& b)
{
	return a.r == b.r && a.g == b.g && a.b == b.b && a.a == b.a;
}

inline bool operator!=(const D2D1_COLOR_F& a, const D2D1_COLOR_F& b)
{
	return !(a == b);
}
#endif


//#RRGGBB와 같은 16진수 컬러 문자열을 COLORREF로 변환
COLORREF		get_color_from_hexa_str(CString hexa_str);
//#RRGGBB 또는 #AARRGGBB와 같은 16진수 컬러 문자열을 Gdiplus::Color로 변환
Gdiplus::Color	get_gcolor_from_hexa_str(CString hexa_str);

//rgb 순서로 "123, 12, 255" 형태의 문자열을 리턴.
CString			get_color_str(COLORREF cr, CString sep = _T(", "));
//argb 순서로 "255, 123, 12, 255" 형태의 문자열을 리턴.
CString			get_color_str(Gdiplus::Color cr, bool include_alpha = true, CString sep = _T(", "));

//str = _T("255, 128, 0") 일 경우 token = ", "으로 하여 argb 컬러값을 얻을 수 있다.
Gdiplus::Color	get_color_from_token_str(CString str, CString separator);

//컬러값을 "FF0000"과 같은 문자열로 리턴한다.
//#FF0000과 같이 리턴받고자 한다면 prefix = _T("#")으로 호출한다.
CString			get_color_hexa_str(COLORREF cr, bool upper_case = true, CString prefix = _T(""));

//include_alpha = true이면 16진수 RRGGGBBAA, false이면 RRGGGBB
CString			get_color_hexa_str(Gdiplus::Color cr, bool upper_case = true, bool include_alpha = true);

//보색. color_complementary() 매크로와 동일
COLORREF		get_complementary_color(COLORREF cr);
//보색. color_complementary() 매크로와 동일
Gdiplus::Color	get_complementary_gcolor(Gdiplus::Color cr);
COLORREF		get_complementary_color(COLORREF cr_color, COLORREF cr_back);

//보색과는 달리 밝기에 따라 black or white를 리턴한다.
//어떤 배경색과 확연히 구분되는 컬러를 보색으로 하면 128, 128, 128과 같은 색상의 보색 역시 동일한 색이 되므로 구분되지 않는다.
COLORREF		get_distinct_color(COLORREF cr);
COLORREF		get_distinct_bw_color(COLORREF cr);

//get_distinct_bw_color()와는 달리 hue를 180도 이동시킨 색상을 리턴한다.
Gdiplus::Color	get_distinct_color(Gdiplus::Color cr);
//보색과는 달리 alpha까지 고려한 밝기에 따라 black or white를 리턴한다.
//어떤 배경색과 확연히 구분되는 컬러를 보색으로 하면 128, 128, 128과 같은 색상의 보색 역시 동일한 색이 되므로 구분되지 않는다.
Gdiplus::Color	get_distinct_bw_color(Gdiplus::Color cr);

//rgb 평균값 리턴
uint8_t			get_gray_value(uint8_t r, uint8_t g, uint8_t b);
//rgb 평균값 리턴
uint8_t			get_gray_value(COLORREF cr);
uint8_t			get_gray_value(Gdiplus::Color cr);
//rgb 평균인 컬러값 리턴
COLORREF		get_gray_color(COLORREF cr);
Gdiplus::Color	get_gray_color(Gdiplus::Color cr);

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
Gdiplus::Color	hsv_to_gcolor(float h, float s, float v);
void			gcolor_to_hsv(Gdiplus::Color cr, float& h, float& s, float& v);

// HSL 변환 (h: 0~360, s: 0~1, l: 0~1)
void			gcolor_to_hsl(Gdiplus::Color cr, float& h, float& s, float& l);
Gdiplus::Color	hsl_to_gcolor(float h, float s, float l);

Gdiplus::Color	RGB2gpColor(COLORREF cr, BYTE alpha = 255);

//cr컬러에서 a(0), r(1), g(2), b(3) 중 한 색을 value 값으로 변경하고 그 값도 리턴한다.
Gdiplus::Color	set_color(Gdiplus::Color &cr, int argb_index, BYTE value);

//red ~ green 범위에서 37%일때의 색상은? get_color(0, 120, 37); (0:red, 120:green of hue)
//hue : 0(red), 60(yellow), 120(green), 180(cyan), 240(blue), 300(violet), 360(red)
COLORREF		get_color(int hue_start, int hue_end, int percent, float saturation = 1.0f, float value = 1.0f);
int				get_hue(COLORREF cr);
Gdiplus::Color	get_color(float hue);
Gdiplus::Color	get_color(float hue, float saturation, float value);
Gdiplus::Color	lerp_color(Gdiplus::Color a, Gdiplus::Color b, float t);

//주어진 컬러와 가장 유사한 표준색의 이름을 리턴.
CString			get_nearest_color_name(COLORREF cr, COLORREF * cr_nearest = nullptr);

//alpha까지 고려하여 컬러의 밝기값을 리턴한다.
::byte			get_luminance(Gdiplus::Color cr);

Gdiplus::Color	get_sys_color(int index);
#ifndef _USING_V110_SDK71_
D2D1_COLOR_F	get_sys_d2color(int index, int alpha = 255);
//보색과는 달리 alpha까지 고려한 밝기에 따라 black or white를 리턴한다.
//어떤 배경색과 확연히 구분되는 컬러를 보색으로 하면 128, 128, 128과 같은 색상의 보색 역시 동일한 색이 되므로 구분되지 않는다.
D2D1_COLOR_F	get_distinct_bw_color(D2D1_COLOR_F cr);
//get_distinct_bw_color()와는 달리 hue를 180도 이동시킨 색상을 리턴한다.
D2D1_COLOR_F	get_distinct_color(D2D1_COLOR_F cr);
#endif

extern COLORREF g_default_color[16];


class CSCColorTheme
{
public:
	CSCColorTheme() = default;
	CSCColorTheme(const CSCColorTheme&) = default;
	CSCColorTheme& operator=(const CSCColorTheme&) = default;
	CSCColorTheme(CSCColorTheme&&) noexcept = default;
	CSCColorTheme& operator=(CSCColorTheme&&) noexcept = default;
	CSCColorTheme(CWnd* pWnd, int theme = color_theme_default)
	{
		m_parent = pWnd;
		set_color_theme(theme);
	};

	~CSCColorTheme() = default;

	enum SC_COLOR_THEMES
	{
		color_theme_default = 0,		//기본 윈도우 테마를 따름
		color_theme_white,				//기존 윈도우의 바탕색을 dlg의 3DFACE가 아닌 white로 함
		color_theme_gray,
		color_theme_dark_gray,
		color_theme_dark,
		color_theme_linkmemine,
		color_theme_linkmemine_origin,	//LMMLoginManager renewal 이전의 origin Agent UI (dark slate bg + light blue 버튼 + cyan accent)
		color_theme_linkmemine_se,
		color_theme_anysupport,
		color_theme_helpu,
		color_theme_pcanypro,

		//Notepad++ GlobalStyles 추출 import 테마 (2026-05-24). get_color_theme_list 의 push 순서 = 이 enum 값 순서여야 함
		//(콤보 인덱스를 그대로 set_color_theme 에 넘기는 구조라 둘이 어긋나면 다른 테마가 적용됨).
		color_theme_zenburn,
		color_theme_bespin,
		color_theme_black_board,
		color_theme_choco,
		color_theme_danslerush_dark,
		color_theme_dark_mode_default,
		color_theme_deep_black,
		color_theme_hello_kitty,			//light
		color_theme_hot_fudge_sundae,
		color_theme_khaki,					//light
		color_theme_mono_industrial,
		color_theme_monokai,
		color_theme_mossy_lawn,
		color_theme_navajo,					//light
		color_theme_obsidian,
		color_theme_plastic_code_wrap,
		color_theme_ruby_blue,
		color_theme_solarized_light,		//light
		color_theme_solarized,
		color_theme_twilight,
		color_theme_vibrant_ink,
		color_theme_vim_dark_blue,

		//브랜드 테마지만 enum 끝에 추가 — 중간 삽입 시 뒤따르는 인덱스가 밀려 저장된 테마 번호가 깨진다.
		color_theme_claude,				//Claude.ai (Anthropic) chrome — warm cream bg + rust orange 버튼

		color_theme_sepia,				//세피아 — 빛바랜 종이/사진 톤 (따뜻한 크림 배경 + 갈색 글자)
		color_theme_windows,			//현재 윈도우 테마의 시스템 색(GetSysColor)을 그대로 반영. default 는 curated 기본 룩.

		//Claude 가 생성한 chrome 시안 10종 (screenshot/0.login before (claudeNN).png 픽셀에서 히스토그램 추출).
		//5 종 light + 5 종 dark, 각자 시그니처 accent 색만 다른 동일 구조 — 사용자 비교 평가용.
		//custom/popup_folder_list 인덱스가 +10 밀리지만, 둘 다 콤보 비노출/내부용이라 영속화 영향 거의 없음.
		color_theme_claude00,			//light : #FAFAFA bg + #2563EB blue accent
		color_theme_claude01,			//dark  : #1E1E1E bg + #EDEDED mono accent
		color_theme_claude02,			//dark  : #2E3440 Nord polar + #88C0D0 frost cyan accent
		color_theme_claude03,			//light : #F5F7FB bg + #4F46E5 indigo accent
		color_theme_claude04,			//dark  : #161616 bg + #0F62FE vivid blue accent
		color_theme_claude05,			//light : #FAF7F2 warm cream + #B45309 rust orange (= color_theme_claude 와 동일 팔레트)
		color_theme_claude06,			//dark  : #0F172A slate-950 + #3B82F6 blue-500 accent
		color_theme_claude07,			//light : #FFFFFF bg + #10B981 emerald accent
		color_theme_claude08,			//dark  : #1A1A1D bg + #A78BFA purple-400 accent
		color_theme_claude09,			//light : #FFFFFF bg + #1D4ED8 deep blue accent

		color_theme_custom,
		color_theme_popup_folder_list,	//CPathCtrl에서 표시하는 폴더 리스트 팝업에 특화된 테마로서 일반적인 테마가 아니므로 get_color_theme_list()의 결과에는 포함되지 않는다.
	};

	//현재 설정된 color theme 인덱스를 리턴
	int		get_color_theme() const { return m_cur_theme; }
	void	set_color_theme(int color_theme);
	static void get_color_theme_list(std::deque<CString> &theme_list);

	//테마 영속화는 int 인덱스 대신 *이름* 으로 저장/복원하는 것을 권장한다.
	//enum 중간에 테마가 추가/재정렬돼도 저장값이 다른 테마로 매핑되지 않는다.
	//저장: get_theme_name(get_color_theme()) / 복원: set_color_theme(get_theme_index(saved_name)).
	//매핑 불가 시 get_theme_name 은 "default", get_theme_index 는 fallback 을 리턴한다.
	static CString	get_theme_name(int theme);
	static int		get_theme_index(LPCTSTR name, int fallback = color_theme_default);

	//테마 강도(intensity) 조절. 같은 테마를 유지한 채 모든 키 컬러를 흰색(중성) 기준으로 스케일한다.
	//dark_gray_medium 처럼 유사 톤 테마를 따로 추가하지 않고 한 테마의 강도만 조절하기 위함.
	//예: set_color_theme(color_theme_dark_gray) 후 set_theme_level(0.8f) → 기본보다 20% 옅게(덜 어둡게).
	//1.0 = 테마 정의값, <1 = 옅게, >1 = 진하게. 이후 set_color_theme 를 호출해도 값은 유지된다.
	float	get_theme_level() const { return m_theme_level; }
	void	set_theme_level(float level = 1.0f);

	//Notepad++ / VSCode 등 외부 에디터 테마를 import 할 때 사용. 에디터 테마가 노출하는 소수의 색
	//(본문 bg/fg, 선택 bg, gutter fg/bg)만 받아 title/button/border/hover/alternate/separator 등
	//나머지 슬롯을 get_weak_color/get_color 로 파생한다. import 테마는 에디터 색을 그대로 써서
	//흰 카드 baseline 을 적용하지 않는다 — cr_edit_back/text = bg/fg.
	void	set_theme_from_editor_palette(Gdiplus::Color bg, Gdiplus::Color fg, Gdiplus::Color sel_bg,
										Gdiplus::Color header_fg, Gdiplus::Color header_bg);

	//호출자가 set_color_theme(int) 후 cr_back 등 일부 필드를 수정한 테마를 그대로 다른 객체에 전달할 때 사용.
	//operator= 와 달리 m_parent / m_cur_theme 은 *수신측 본인 것을 유지* 하여
	//이후 default 테마 재적용 시 부모 종류 판정이 어긋나는 문제를 방지한다.
	void	copy_colors_from(const CSCColorTheme& src)
	{
		//src 가 default 테마면 수신측의 m_parent 기준으로 default 를 재적용한다.
		//배경: dlg 가 자기 m_theme(default) 을 자식 컨트롤에 일괄 전파할 때 dlg 의 cr_back (=COLOR_BTNFACE)
		//이 List/Tree/Edit 류 컨트롤의 cr_back 으로 그대로 들어가, 본래 COLOR_WINDOW (흰색) 이어야 할
		//배경이 회색이 되는 문제. default 의 set_color_theme(int) 안에서 m_parent->IsKindOf 로
		//컨트롤 종류별 적절한 cr_back 을 골라주는 분기가 이미 있으므로, 그 경로를 다시 태운다.
		//단, m_parent 가 NULL 이면 (theme 객체가 dlg/control 외부에서 단독으로 쓰이는 경우) 일반 copy 로 폴백.
		//강도(level)는 default 재진입 분기보다 먼저 복사해야 그 안의 set_color_theme(default) 가
		//src 의 강도로 재계산한다 (아래 일반 copy 경로는 이미 leveled 된 색을 그대로 복사).
		m_theme_level = src.m_theme_level;

		//default 와 windows 테마는 control-kind 별 cr_back 분기(List/Tree/Edit 류=COLOR_WINDOW, 그 외=COLOR_BTNFACE)를
		//set_color_theme(int) 안에서 m_parent->IsKindOf 로 수행한다. 이 두 테마는 그 경로를 다시 태워야 자식 컨트롤이
		//부모 dlg 의 cr_back(BTNFACE)을 그대로 받지 않고 자기 종류에 맞는 배경을 갖는다. (windows 누락 시 Edit/List/Tree 가 회색이던 문제)
		if ((src.m_cur_theme == color_theme_default || src.m_cur_theme == color_theme_windows) && m_parent != nullptr)
		{
			set_color_theme(src.m_cur_theme);
			return;
		}

		cr_text						= src.cr_text;
		cr_text_dim					= src.cr_text_dim;
		cr_disabled_text			= src.cr_disabled_text;
		cr_text_hover				= src.cr_text_hover;
		cr_text_dropHilited			= src.cr_text_dropHilited;
		cr_text_selected			= src.cr_text_selected;
		cr_text_selected_inactive	= src.cr_text_selected_inactive;

		cr_back						= src.cr_back;
		cr_back_hover				= src.cr_back_hover;
		cr_back_dropHilited			= src.cr_back_dropHilited;
		cr_back_selected			= src.cr_back_selected;
		cr_back_selected_inactive	= src.cr_back_selected_inactive;
		cr_back_selected_hover		= src.cr_back_selected_hover;
		cr_back_alternate			= src.cr_back_alternate;

		cr_parent_back				= src.cr_parent_back;

		cr_edit_text				= src.cr_edit_text;
		cr_edit_back				= src.cr_edit_back;
		cr_button_text				= src.cr_button_text;
		cr_button_back				= src.cr_button_back;
		cr_button_border			= src.cr_button_border;

		cr_selected_border			= src.cr_selected_border;
		cr_selected_border_inactive	= src.cr_selected_border_inactive;
		cr_border_active			= src.cr_border_active;
		cr_border_inactive			= src.cr_border_inactive;
		cr_separator				= src.cr_separator;
		cr_gridlines				= src.cr_gridlines;

		cr_title_text				= src.cr_title_text;
		cr_title_back_active		= src.cr_title_back_active;
		cr_title_back_inactive		= src.cr_title_back_inactive;
		cr_sys_buttons_hover_back	= src.cr_sys_buttons_hover_back;
		cr_sys_buttons_down_back	= src.cr_sys_buttons_down_back;

		cr_header_text				= src.cr_header_text;
		cr_header_back				= src.cr_header_back;
		cr_percentage_bar			= src.cr_percentage_bar;
		cr_progress_active					= src.cr_progress_active;

		cr_success					= src.cr_success;
		cr_info						= src.cr_info;
		cr_warning					= src.cr_warning;
		cr_error					= src.cr_error;

		//원본의 theme 인덱스도 가져옴 — 호출자가 의도한 "수정된 프리셋" 의 베이스를 추적할 수 있도록.
		//(m_theme_level 은 함수 맨 앞에서 이미 복사했다.)
		m_cur_theme					= src.m_cur_theme;
	}

	Gdiplus::Color	cr_text;
	Gdiplus::Color	cr_text_dim;					//기본 글자색보다 흐릿하게 표현되는 항목 (placeholder 등)
	Gdiplus::Color	cr_disabled_text = Gdiplus::Color::DarkGray;	//disabled 컨트롤의 텍스트/아이콘 stroke 공용 색
	Gdiplus::Color	cr_text_hover;
	Gdiplus::Color	cr_text_dropHilited;
	Gdiplus::Color	cr_text_selected;
	Gdiplus::Color	cr_text_selected_inactive;

	Gdiplus::Color	cr_back;						//BTNFACE	: for CDialog, CButton, CStatic...
	Gdiplus::Color	cr_back_hover;					//= hover = over = track_select... 다양한 같은 의미가 있으나 hover로 통일하자.
	Gdiplus::Color	cr_back_dropHilited;
	Gdiplus::Color	cr_back_selected;
	Gdiplus::Color	cr_back_selected_inactive;
	Gdiplus::Color	cr_back_selected_hover;			//selected 항목 위 hover combo 색. selected identity 유지하면서 미세하게 강조.
	Gdiplus::Color	cr_back_alternate;				//list의 경우 짝수라인, 홀수라인 번갈아 색상을 표시하는 목적

	Gdiplus::Color	cr_parent_back;					//round가 적용된 컨트롤의 경우 parent back 색상으로 칠한 후 round가 그려져야 한다.

	//CSCStatic, CSCTreeCtrl, CVtListCtrlEx, CSCStaticEdit 등 편집/입력 컨트롤의 본문 색.
	//기본값은 흰색 + near-black — 어떤 테마에서도 input field 는 "흰 카드 + 어두운 글자"
	//를 design baseline 으로 한다. 별도 지정이 필요한 테마(예: 모노톤 dark IDE) 만
	//set_color_theme 에서 덮는다.
	//(VSCode / Visual Studio / Notepad++ 등 외부 테마 import 시에도 input.background /
	// input.foreground 슬롯과 직접 매핑된다.)
	Gdiplus::Color	cr_edit_text = gRGB(32, 32, 32);
	Gdiplus::Color	cr_edit_back = Gdiplus::Color::White;

	//CGdiButton 의 primary action button 색. default = Transparent (alpha=0) 이면
	//set_color_theme 안에서 cr_back luma 기반 Win11 push-button 자동 산출 경로 사용.
	//alpha != 0 이면 그 값을 face/text 로 직접 사용 — 테마가 명시적으로 primary 버튼 색을
	//정의하는 경우 (예: LinkMeMine Agent 의 light blue 로그인 버튼).
	//(VSCode 의 button.background / button.foreground 와 매핑.)
	Gdiplus::Color	cr_button_back = Gdiplus::Color::Transparent;
	Gdiplus::Color	cr_button_text = Gdiplus::Color::Transparent;
	//버튼 테두리. 미지정(alpha=0)이면 face 와 동일색 → 테두리 없음. 보통 각 테마에서
	//get_weak_color(cr_back, N) 로 자동산출 — cr_button_back≈cr_back 인 테마(default)에서도
	//cr_back 대비로 버튼 윤곽이 유지된다. (VSCode 의 button.border 와 매핑.)
	Gdiplus::Color	cr_button_border = Gdiplus::Color::Transparent;

	Gdiplus::Color	cr_selected_border;
	Gdiplus::Color	cr_selected_border_inactive;
	Gdiplus::Color	cr_border_active;
	Gdiplus::Color	cr_border_inactive;

	//CSCMenu separator / 컨트롤 내부 분리선 — cr_border_inactive 보다 약간 얕음. theme 별 정의.
	Gdiplus::Color	cr_separator;

	//20260723 by claude. 리스트 격자선. 테마별로 정의하지 않고 set_color_theme 말미에서
	//get_weak_color(cr_back, 16) 으로 일괄 파생한다 — 격자는 내용을 구분만 하면 되므로
	//cr_separator(30) 보다 더 옅어야 하고, 어떤 테마에서도 배경에서 한 톤만 떨어지면 충분하다.
	Gdiplus::Color	cr_gridlines;

	//caption이 없는 dialog based에서 사용
	Gdiplus::Color	cr_title_text;
	//active 상태와 inactive 상태에 따라 색상을 구분하려 했으나 CSCSystemButtons에도 영향을 주고
	//여러가지 영향받는 부분이 많다보니 우선은 active 컬러만 사용하도록 한다.
	Gdiplus::Color	cr_title_back_active;
	Gdiplus::Color	cr_title_back_inactive;			//사용하지 않음.	
	Gdiplus::Color	cr_sys_buttons_hover_back;
	Gdiplus::Color	cr_sys_buttons_down_back;

	//CListCtrl에서만 사용될것으로 예상.
	Gdiplus::Color	cr_header_text;
	Gdiplus::Color	cr_header_back;
	std::deque<Gdiplus::Color> cr_percentage_bar;	//percentage bar graph color
	Gdiplus::Color	cr_progress_active;					//progress bar
	//Gdiplus::Color	cr_progress_text;				//progress text

	//로그, 상태등의 텍스트	표시용 컬러. 컬러 테마마다 약간씩 다를 수 있으므로
	//필요한 경우 해당 테마에서 아래 값들을 수정해 줄 필요도 있다.
	Gdiplus::Color	cr_success = Gdiplus::Color::RoyalBlue;
	Gdiplus::Color	cr_info = Gdiplus::Color::DimGray;
	Gdiplus::Color	cr_warning = Gdiplus::Color::Crimson;
	Gdiplus::Color	cr_error = Gdiplus::Color::Red;


protected:
	CWnd*			m_parent = nullptr;
	int				m_cur_theme = color_theme_default;
	float			m_theme_level = 1.0f;	//테마 강도. set_theme_level 로 조절. 1.0 = 정의값.

	//m_theme_level 을 모든 색 필드에 일괄 적용 (set_color_theme 끝에서 호출).
	void			apply_theme_level(float level);
};

//color name으로 Gdiplus::Color 값을 구하기 위해 정의.
//get_color(cr_str)과 같이 호출하고자 정의함.
//map이나 unordered_map은 insert order를 보장하지 않으므로 추가된 순서로 팔레트를 표현할 수 없는 단점이 있다.
//std::vector<CString, Gdiplus::Color> 와 같이 구현하면 순서를 보장하며
//map의 가장 큰 장점이 first 키값으로 second를 구하는 것인데 이는 std::find()로 구현 가능하다.
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

	//"royalblue"로 검색하면 Gdiplus::Color::RoyalBlue로 리턴하고 name 또한 "RoyalBlue"로 변경된다.
	static Gdiplus::Color get_color(std::string& name)
	{
		//m_cr_map::iterator it = get_color_map().find(name);
		//위와 같이 map.find()를 쓰면 대소문자 구분을 하므로, 대소문자 구분없이 검색하기 위해 std::find_if와 _stricmp()를 사용한다.
		std::vector<std::pair<std::string, Gdiplus::Color>>::iterator it = std::find_if(get_color_list().begin(), get_color_list().end(),
			[&](const std::pair<std::string, Gdiplus::Color>& element) ->
			bool
			{
				return (_stricmp(element.first.c_str(), name.c_str()) == 0);
			});

		if (it == get_color_list().end())
		{
			name = "Black";
			return Gdiplus::Color::Black;
		}

		name = it->first;
		return it->second;
	}

	//get color name by (r, g, b) value. alpha는 무시한다.
	//exactly : true이면 정확히 일치하는 색상만 찾고, false이면 가장 유사한 색상의 이름을 "near : "을 붙여서 리턴한다.
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
			//20260724 by claude. 이름 판정은 RGB만 본다 — ARGB(0,255,255,255)처럼 알파만 다른 값도 White다.
			//알파는 이름이 아니라 호출측의 ARGB/RGBA 표기가 전달한다. (기존에는 알파가 255가 아니면 near로 빠졌다.)
			if (distance == 0.0)
				return color_name;

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
