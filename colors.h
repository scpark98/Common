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

enum COLOR_THEME_MEMBER_INDEX
{
	cri_text = 0,

};

class CSCColorTheme
{
public :
	CSCColorTheme(CWnd* pWnd, int theme = color_theme_default)
	{
		m_parent = pWnd;
		set_color_theme(color_theme_default);
	};

	enum SC_COLOR_THEMES
	{
		color_theme_default = 0,		//기본 윈도우 테마를 따름
		color_theme_white,				//기존 윈도우의 바탕색을 dlg의 3DFACE가 아닌 white로 함
		color_theme_gray,
		color_theme_dark_gray,
		color_theme_dark,
		color_theme_popup_folder_list,	//CPathCtrl에서 표시하는 폴더 리스트 팝업에 특화된 테마로서 일반적인 테마가 아니므로 get_color_theme_list()의 결과에는 포함되지 않는다.
		color_theme_linkmemine,
		color_theme_linkmemine_se,
		color_theme_anysupport,
		color_theme_helpu,
		color_theme_pcanypro,
		color_theme_custom,
	};

	//현재 설정된 color theme 인덱스를 리턴
	int		get_color_theme() { return m_cur_theme; }
	void	set_color_theme(int color_theme);
	static std::deque<CString> get_color_theme_list();

	Gdiplus::Color	cr_text;
	Gdiplus::Color	cr_text_dim;					//기본 글자색보다 흐릿하게 표현되는 항목
	Gdiplus::Color	cr_text_hover;
	Gdiplus::Color	cr_text_dropHilited;
	Gdiplus::Color	cr_text_selected;
	Gdiplus::Color	cr_text_selected_inactive;

	Gdiplus::Color	cr_back;						//BTNFACE	: for CDialog, CButton, CStatic...
	Gdiplus::Color	cr_back_hover;					//= hover = over = track_select... 다양한 같은 의미가 있으나 hover로 통일하자.
	Gdiplus::Color	cr_back_dropHilited;
	Gdiplus::Color	cr_back_selected;
	Gdiplus::Color	cr_back_selected_inactive;
	Gdiplus::Color	cr_back_alternate;				//list의 경우 짝수라인, 홀수라인 번갈아 색상을 표시하는 목적

	Gdiplus::Color	cr_selected_border;
	Gdiplus::Color	cr_border;

	//caption이 없는 dialog based에서 사용
	Gdiplus::Color	cr_title_text;
	Gdiplus::Color	cr_title_back;
	Gdiplus::Color	cr_sys_buttons_hover_back;
	Gdiplus::Color	cr_sys_buttons_down_back;

	//CListCtrl에서만 사용될것으로 예상.
	Gdiplus::Color	cr_header_text;
	Gdiplus::Color	cr_header_back;
	std::deque<Gdiplus::Color> cr_percentage_bar;	//percentage bar graph color
	Gdiplus::Color	cr_progress;					//progress bar
	//Gdiplus::Color	cr_progress_text;				//progress text

protected:
	CWnd*			m_parent = NULL;
	int				m_cur_theme = color_theme_default;
};

//color name으로 Gdiplus::Color 값을 구하기 위해 정의.
//반대로 Gdiplus::Color 값으로 그 이름을 구하는 경우는 많지 않을 듯 하여 first key를 name으로 정함.
class CSCColorMap
{
public:
	CSCColorMap(std::string _name, Gdiplus::Color _cr)
	{
		name = _name;
		cr = _cr;
		get_color_map()[name] = cr;
	}

	// auto-cast Error to integer error code
	operator Gdiplus::Color() { return cr; }

	static Gdiplus::Color get_color(std::string name)
	{
		//m_cr_map::iterator it = get_color_map().find(name);
		//위와 같이 map.find()를 쓰면 대소문자 구분을 하므로, 대소문자 구분없이 검색하기 위해 std::find_if와 _stricmp()를 사용한다.
		m_cr_map::iterator it = std::find_if(get_color_map().begin(), get_color_map().end(),
			[&](const std::pair<std::string, Gdiplus::Color>& element) ->
			bool
			{
				return (_stricmp(element.first.c_str(), name.c_str()) == 0);
			});

		if (it == get_color_map().end())
			return Gdiplus::Color::Black;

		return it->second;
	}

	//get color name by (r, g, b) value. alpha는 무시한다.
	//exactly : true이면 정확히 일치하는 색상만 찾고, false이면 가장 유사한 색상의 이름을 리턴한다.
	static std::string get_color_name(Gdiplus::Color cr, bool exactly = true)
	{
		double distance = 99999999.0;
		std::string nearest_color_name;

		m_cr_map::iterator it = std::find_if(get_color_map().begin(), get_color_map().end(),
			[&](const std::pair<std::string, Gdiplus::Color>& element) ->
			bool
			{
				if (exactly)
				{
					return (element.second.GetR() == cr.GetR() &&
							element.second.GetG() == cr.GetG() &&
							element.second.GetB() == cr.GetB());
				}

				double dist = sqrt( (element.second.GetR() - cr.GetR()) * (element.second.GetR() - cr.GetR()) +
									(element.second.GetG() - cr.GetG()) * (element.second.GetG() - cr.GetG()) +
									(element.second.GetB() - cr.GetB()) * (element.second.GetB() - cr.GetB()));

				if (dist < distance)
				{
					distance = dist;
					nearest_color_name = element.first;
				}
				return false; // continue searching
			});

		if (it == get_color_map().end())
		{
			if (!exactly)
				return nearest_color_name;
			return "Unknown Color";
		}

		return it->first;
	}

private:
	Gdiplus::Color cr;
	std::string name;

	//struct CaseInsensitiveComparator
	//{
	//	bool operator()(const std::string& a, const std::string& b) const noexcept
	//	{
	//		return (_stricmp(a.c_str(), b.c_str()) < 0);
	//	}
	//};

	//bool caseInsensitiveCompare(const std::string& a, const std::string& b) {
	//	std::string aLower = a;
	//	std::string bLower = b;
	//	std::transform(aLower.begin(), aLower.end(), aLower.begin(), [](unsigned char c) { return std::tolower(c); });
	//	std::transform(bLower.begin(), bLower.end(), bLower.begin(), [](unsigned char c) { return std::tolower(c); });
	//	return aLower < bLower;
	//};

	typedef std::unordered_map<std::string, Gdiplus::Color> m_cr_map;
	static m_cr_map& get_color_map()
	{
		static m_cr_map cr_map;
		return cr_map;
	}
};

static CSCColorMap MediumVioletRed	("MediumVioletRed",		Gdiplus::Color::MediumVioletRed);
static CSCColorMap DeepPink			("DeepPink",			Gdiplus::Color::DeepPink);
static CSCColorMap PaleVioletRed	("PaleVioletRed",		Gdiplus::Color::PaleVioletRed);
static CSCColorMap HotPink			("HotPink",				Gdiplus::Color::HotPink);
static CSCColorMap LightPink		("LightPink",			Gdiplus::Color::LightPink);
static CSCColorMap Pink				("Pink",				Gdiplus::Color::Pink);

static CSCColorMap DarkRed			("DarkRed",				Gdiplus::Color::DarkRed);
static CSCColorMap Red				("Red",					Gdiplus::Color::Red);
static CSCColorMap Firebrick		("Firebrick",			Gdiplus::Color::Firebrick);
static CSCColorMap Crimson			("Crimson",				Gdiplus::Color::Crimson);
static CSCColorMap IndianRed		("IndianRed",			Gdiplus::Color::IndianRed);
static CSCColorMap LightCoral		("LightCoral",			Gdiplus::Color::LightCoral);
static CSCColorMap Salmon			("Salmon",				Gdiplus::Color::Salmon);
static CSCColorMap DarkSalmon		("DarkSalmon",			Gdiplus::Color::DarkSalmon);
static CSCColorMap LightSalmon		("LightSalmon",			Gdiplus::Color::LightSalmon);

static CSCColorMap OrangeRed		("OrangeRed",			Gdiplus::Color::OrangeRed);
static CSCColorMap Tomato			("Tomato",				Gdiplus::Color::Tomato);
static CSCColorMap DarkOrange		("DarkOrange",			Gdiplus::Color::DarkOrange);
static CSCColorMap Coral			("Coral",				Gdiplus::Color::Coral);
static CSCColorMap Orange			("Orange",				Gdiplus::Color::Orange);

static CSCColorMap DarkKhaki		("DarkKhaki",			Gdiplus::Color::DarkKhaki);
static CSCColorMap Gold				("Gold",				Gdiplus::Color::Gold);
static CSCColorMap Khaki			("Khaki",				Gdiplus::Color::Khaki);
static CSCColorMap PeachPuff		("PeachPuff",			Gdiplus::Color::PeachPuff);
static CSCColorMap Yellow			("Yellow",				Gdiplus::Color::Yellow);
static CSCColorMap PaleGoldenrod	("PaleGoldenrod",		Gdiplus::Color::PaleGoldenrod);
static CSCColorMap Moccasin			("Moccasin",			Gdiplus::Color::Moccasin);
static CSCColorMap PapayaWhip		("PapayaWhip",			Gdiplus::Color::PapayaWhip);
static CSCColorMap LightGoldenrodYellow("LightGoldenrodYellow", Gdiplus::Color::LightGoldenrodYellow);
static CSCColorMap LemonChiffon		("LemonChiffon",		Gdiplus::Color::LemonChiffon);
static CSCColorMap LightYellow		("LightYellow",			Gdiplus::Color::LightYellow);

static CSCColorMap Maroon			("Maroon",				Gdiplus::Color::Maroon);
static CSCColorMap Brown			("Brown",				Gdiplus::Color::Brown);
static CSCColorMap SaddleBrown		("SaddleBrown",			Gdiplus::Color::SaddleBrown);
static CSCColorMap Sienna			("Sienna",				Gdiplus::Color::Sienna);
static CSCColorMap Chocolate		("Chocolate",			Gdiplus::Color::Chocolate);
static CSCColorMap DarkGoldenrod	("DarkGoldenrod",		Gdiplus::Color::DarkGoldenrod);
static CSCColorMap Peru				("Peru",				Gdiplus::Color::Peru);
static CSCColorMap RosyBrown		("RosyBrown",			Gdiplus::Color::RosyBrown);
static CSCColorMap Goldenrod		("Goldenrod",			Gdiplus::Color::Goldenrod);
static CSCColorMap SandyBrown		("SandyBrown",			Gdiplus::Color::SandyBrown);
static CSCColorMap Tan				("Tan",					Gdiplus::Color::Tan);
static CSCColorMap BurlyWood		("BurlyWood",			Gdiplus::Color::BurlyWood);
static CSCColorMap Wheat			("Wheat",				Gdiplus::Color::Wheat);
static CSCColorMap NavajoWhite		("NavajoWhite",			Gdiplus::Color::NavajoWhite);
static CSCColorMap Bisque			("Bisque",				Gdiplus::Color::Bisque);
static CSCColorMap BlanchedAlmond	("BlanchedAlmond",		Gdiplus::Color::BlanchedAlmond);
static CSCColorMap Cornsilk			("Cornsilk",			Gdiplus::Color::Cornsilk);

static CSCColorMap Indigo			("Indigo",				Gdiplus::Color::Indigo);
static CSCColorMap Purple			("Purple",				Gdiplus::Color::Purple);
static CSCColorMap DarkMagenta		("DarkMagenta",			Gdiplus::Color::DarkMagenta);
static CSCColorMap DarkViolet		("DarkViolet",			Gdiplus::Color::DarkViolet);
static CSCColorMap DarkSlateBlue	("DarkSlateBlue",		Gdiplus::Color::DarkSlateBlue);
static CSCColorMap BlueViolet		("BlueViolet",			Gdiplus::Color::BlueViolet);
static CSCColorMap DarkOrchid		("DarkOrchid",			Gdiplus::Color::DarkOrchid);
static CSCColorMap Fuchsia			("Fuchsia",				Gdiplus::Color::Fuchsia);
static CSCColorMap Magenta			("Magenta",				Gdiplus::Color::Magenta);
static CSCColorMap SlateBlue		("SlateBlue",			Gdiplus::Color::SlateBlue);
static CSCColorMap MediumSlateBlue	("MediumSlateBlue",		Gdiplus::Color::MediumSlateBlue);
static CSCColorMap MediumOrchid		("MediumOrchid",		Gdiplus::Color::MediumOrchid);
static CSCColorMap MediumPurple		("MediumPurple",		Gdiplus::Color::MediumPurple);
static CSCColorMap Orchid			("Orchid",				Gdiplus::Color::Orchid);
static CSCColorMap Violet			("Violet",				Gdiplus::Color::Violet);
static CSCColorMap Plum				("Plum",				Gdiplus::Color::Plum);
static CSCColorMap Thistle			("Thistle",				Gdiplus::Color::Thistle);
static CSCColorMap Lavender			("Lavender",			Gdiplus::Color::Lavender);

static CSCColorMap MidnightBlue		("MidnightBlue",		Gdiplus::Color::MidnightBlue);
static CSCColorMap Navy				("Navy",				Gdiplus::Color::Navy);
static CSCColorMap DarkBlue			("DarkBlue",			Gdiplus::Color::DarkBlue);
static CSCColorMap MediumBlue		("MediumBlue",			Gdiplus::Color::MediumBlue);
static CSCColorMap Blue				("Blue",				Gdiplus::Color::Blue);
static CSCColorMap RoyalBlue		("RoyalBlue",			Gdiplus::Color::RoyalBlue);
static CSCColorMap SteelBlue		("SteelBlue",			Gdiplus::Color::SteelBlue);
static CSCColorMap DodgerBlue		("DodgerBlue",			Gdiplus::Color::DodgerBlue);
static CSCColorMap DeepSkyBlue		("DeepSkyBlue",			Gdiplus::Color::DeepSkyBlue);
static CSCColorMap CornflowerBlue	("CornflowerBlue",		Gdiplus::Color::CornflowerBlue);
static CSCColorMap SkyBlue			("SkyBlue",				Gdiplus::Color::SkyBlue);
static CSCColorMap LightSkyBlue		("LightSkyBlue",		Gdiplus::Color::LightSkyBlue);
static CSCColorMap LightSteelBlue	("LightSteelBlue",		Gdiplus::Color::LightSteelBlue);
static CSCColorMap LightBlue		("LightBlue",			Gdiplus::Color::LightBlue);
static CSCColorMap PowderBlue		("PowderBlue",			Gdiplus::Color::PowderBlue);

static CSCColorMap Teal				("Teal",				Gdiplus::Color::Teal);
static CSCColorMap DarkCyan			("DarkCyan",			Gdiplus::Color::DarkCyan);
static CSCColorMap LightSeaGreen	("LightSeaGreen",		Gdiplus::Color::LightSeaGreen);
static CSCColorMap CadetBlue		("CadetBlue",			Gdiplus::Color::CadetBlue);
static CSCColorMap DarkTurquoise	("DarkTurquoise",		Gdiplus::Color::DarkTurquoise);
static CSCColorMap MediumTurquoise	("MediumTurquoise",		Gdiplus::Color::MediumTurquoise);
static CSCColorMap Turquoise		("Turquoise",			Gdiplus::Color::Turquoise);
static CSCColorMap Aqua				("Aqua",				Gdiplus::Color::Aqua);
static CSCColorMap Cyan				("Cyan",				Gdiplus::Color::Cyan);
static CSCColorMap Aquamarine		("Aquamarine",			Gdiplus::Color::Aquamarine);
static CSCColorMap PaleTurquoise	("PaleTurquoise",		Gdiplus::Color::PaleTurquoise);
static CSCColorMap LightCyan		("LightCyan",			Gdiplus::Color::LightCyan);

static CSCColorMap DarkGreen		("DarkGreen",			Gdiplus::Color::DarkGreen);
static CSCColorMap Green			("Green",				Gdiplus::Color::Green);
static CSCColorMap DarkOliveGreen	("DarkOliveGreen",		Gdiplus::Color::DarkOliveGreen);
static CSCColorMap ForestGreen		("ForestGreen",			Gdiplus::Color::ForestGreen);
static CSCColorMap SeaGreen			("SeaGreen",			Gdiplus::Color::SeaGreen);
static CSCColorMap Olive			("Olive",				Gdiplus::Color::Olive);
static CSCColorMap OliveDrab		("OliveDrab",			Gdiplus::Color::OliveDrab);
static CSCColorMap MediumSeaGreen	("MediumSeaGreen",		Gdiplus::Color::MediumSeaGreen);
static CSCColorMap LimeGreen		("LimeGreen",			Gdiplus::Color::LimeGreen);
static CSCColorMap Lime				("Lime",				Gdiplus::Color::Lime);
static CSCColorMap SpringGreen		("SpringGreen",			Gdiplus::Color::SpringGreen);
static CSCColorMap MediumSpringGreen("MediumSpringGreen",	Gdiplus::Color::MediumSpringGreen);
static CSCColorMap DarkSeaGreen		("DarkSeaGreen",		Gdiplus::Color::DarkSeaGreen);
static CSCColorMap MediumAquamarine	("MediumAquamarine",	Gdiplus::Color::MediumAquamarine);
static CSCColorMap YellowGreen		("YellowGreen",			Gdiplus::Color::YellowGreen);
static CSCColorMap LawnGreen		("LawnGreen",			Gdiplus::Color::LawnGreen);
static CSCColorMap Chartreuse		("Chartreuse",			Gdiplus::Color::Chartreuse);
static CSCColorMap LightGreen		("LightGreen",			Gdiplus::Color::LightGreen);
static CSCColorMap GreenYellow		("GreenYellow",			Gdiplus::Color::GreenYellow);
static CSCColorMap PaleGreen		("PaleGreen",			Gdiplus::Color::PaleGreen);

static CSCColorMap MistyRose		("MistyRose",			Gdiplus::Color::MistyRose);
static CSCColorMap AntiqueWhite		("AntiqueWhite",		Gdiplus::Color::AntiqueWhite);
static CSCColorMap Linen			("Linen",				Gdiplus::Color::Linen);
static CSCColorMap Beige			("Beige",				Gdiplus::Color::Beige);
static CSCColorMap WhiteSmoke		("WhiteSmoke",			Gdiplus::Color::WhiteSmoke);
static CSCColorMap LavenderBlush	("LavenderBlush",		Gdiplus::Color::LavenderBlush);
static CSCColorMap OldLace			("OldLace",				Gdiplus::Color::OldLace);
static CSCColorMap AliceBlue		("AliceBlue",			Gdiplus::Color::AliceBlue);
static CSCColorMap SeaShell			("SeaShell",			Gdiplus::Color::SeaShell);
static CSCColorMap GhostWhite		("GhostWhite",			Gdiplus::Color::GhostWhite);
static CSCColorMap Honeydew			("Honeydew",			Gdiplus::Color::Honeydew);
static CSCColorMap FloralWhite		("FloralWhite",			Gdiplus::Color::FloralWhite);
static CSCColorMap Azure			("Azure",				Gdiplus::Color::Azure);
static CSCColorMap MintCream		("MintCream",			Gdiplus::Color::MintCream);
static CSCColorMap Snow				("Snow",				Gdiplus::Color::Snow);
static CSCColorMap Ivory			("Ivory",				Gdiplus::Color::Ivory);
static CSCColorMap White			("White",				Gdiplus::Color::White);

static CSCColorMap Black			("Black",				Gdiplus::Color::Black);
static CSCColorMap DarkSlateGray	("DarkSlateGray",		Gdiplus::Color::DarkSlateGray);
static CSCColorMap DimGray			("DimGray",				Gdiplus::Color::DimGray);
static CSCColorMap SlateGray		("SlateGray",			Gdiplus::Color::SlateGray);
static CSCColorMap Gray				("Gray",				Gdiplus::Color::Gray);
static CSCColorMap LightSlateGray	("LightSlateGray",		Gdiplus::Color::LightSlateGray);
static CSCColorMap DarkGray			("DarkGray",			Gdiplus::Color::DarkGray);
static CSCColorMap Silver			("Silver",				Gdiplus::Color::Silver);
static CSCColorMap LightGray		("LightGray",			Gdiplus::Color::LightGray);
static CSCColorMap Gainsboro		("Gainsboro",			Gdiplus::Color::Gainsboro);

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
Gdiplus::Color	get_color(COLORREF rgb);

//컬러 이름으로 Gdiplus::Color를 리턴한다. 대소문자를 구분하지 않으며 이름이 없으면 검정색을 리턴한다.
//get_color("red") 또는 get_color(_T("Red")) -> Gdiplus::Color(255, 255, 0, 0);
Gdiplus::Color	get_color(CString cr_str);
Gdiplus::Color	get_color(std::string cr_name);

//#RRGGBB와 같은 16진수 컬러 문자열을 COLORREF로 변환
COLORREF		get_color_from_hexadecimal(CString cr);
//#RRGGBB 또는 #AARRGGBB와 같은 16진수 컬러 문자열을 Gdiplus::Color로 변환
Gdiplus::Color	get_gcolor_from_hexadecimal(CString cr);

//컬러값을 "FF0000"과 같은 문자열로 리턴한다.
CString			get_color_string(COLORREF cr);

//보색
COLORREF		get_complementary_color(COLORREF cr);
Gdiplus::Color	get_complementary_gcolor(Gdiplus::Color cr);

//보색과는 달리 밝기에 따라 black or white를 리턴한다.
//어떤 배경색과 확연히 구분되는 컬러를 보색으로 하면 128, 128, 128과 같은 색상의 보색 역시 동일한 색이 되므로 구분되지 않는다.
COLORREF		get_distinct_color(COLORREF cr);
//보색과는 달리 밝기에 따라 black or white를 리턴한다.
//어떤 배경색과 확연히 구분되는 컬러를 보색으로 하면 128, 128, 128과 같은 색상의 보색 역시 동일한 색이 되므로 구분되지 않는다.
Gdiplus::Color	get_distinct_gcolor(Gdiplus::Color cr);

//rgb 평균값 리턴
uint8_t			gray_value(uint8_t r, uint8_t g, uint8_t b);
//rgb 평균값 리턴
uint8_t			gray_value(COLORREF cr);
uint8_t			gray_value(Gdiplus::Color cr);
//rgb 평균인 컬러값 리턴
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

//cr컬러에서 a,r,g,b 중 한 색을 value로 변경한다.
void			set_color(Gdiplus::Color &cr, int argb, BYTE value);
Gdiplus::Color	get_color(Gdiplus::Color cr, int argb, BYTE value);

//red ~ green 범위에서 37%일때의 색상은? get_color(0, 120, 37); (0:red, 120:green of hue)
//hue : 0(red), 60(yellow), 120(green), 180(cyan), 240(blue), 300(violet), 360(red)
COLORREF		get_color(int hue_start, int hue_end, int percent, float saturation = 1.0f, float value = 1.0f);
int				get_hue(COLORREF cr);

//주어진 컬러와 가장 유사한 표준색의 이름을 리턴.
CString			get_nearest_color_name(COLORREF cr, COLORREF * cr_nearest = NULL);

Gdiplus::Color	get_sys_color(int index);

extern COLORREF g_default_color[16];
#endif
