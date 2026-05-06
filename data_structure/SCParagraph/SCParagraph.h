#pragma once

#include <algorithm>
#include <afxwin.h>
#include <gdiplus.h>
#include <deque>

/*
[태그된 텍스트 지원]
	- html과 같이 tagged text를 지원하기 위해 제작.
	- CSCShapeDlg, CSCParagraphStatic 등에서 사용되며 html, smi 등의 tagged text를 표현할 수 있다.

	태그는 소문자로, 컬러값은 Gdiplus::Color에 정의된 이름(web color와 naming이 동일함)을 사용해야 한다.
	컬러 이름은 대소문자를 구분하지 않는다. ("red", "Red", "RED" 모두 동일함)
	태그명에 공백, 하이픈, underscore는 모두 무시된다.
	지원하는 태그는 다음과 같다.

	<f=굴림> = <font=굴림> = <name=굴림> = <font_name=굴림>
	<sz=10> = <size=10> = <font_size=10>
	<b>						//bold
	<i>						//italic
	<u>						//underline
	<s>						//strike out
	<cr=red> = <ct=red>		//cr_text
	<crb=Blue> = <cb=Blue>	//cr_back
	<br>					//line break
	ex. "<b><cr=red>This</b></cr > is a <ct=blue><i>sample</i> <b>paragraph</b>."

	- cr은 Gdiplus::Color에 명시된 컬러 외에 다음 형식들도 지원한다.
	  <cr=#RRGGBBAA>,			//rgba, #으로 시작되는 16진수값.
	  <cr=123,45,67,128>		//rgba, 숫자로만 3자리 또는 4자리를 콤마로 구분한다.
	  <cr=h90,30,100>,		//hsi, h로 시작되고 콤마로 구분한다.

[코드 흐름]
	- set_text()			: tag를 허용하는 텍스트를 설정한다.
	  get_tag_str()			: tag와 텍스트를 분리한다.
	  build_paragraph_str()	: tag를 파싱하여 각 음절의 속성을 설정한다.
	  calc_text_rect()		: 각 음절의 출력 위치를 계산한다.
	  OnPaint()				: 위에서 계산된 위치에 각 음절 텍스트를 출력한다.

	- calc_text_rect()에서 각 항목이 출력될 위치까지 모두 계산되고 OnPaint()에서는 정해진 위치에 그리기만 한다.

*/

//기존 LOGFONT는 GDI용이고 fontsize가 int만 지원되거나, lfFaceName이 TCHAR [] 등
//불편한 점이 많으므로 Gdi+에서도 사용할 수 있도록 CSCTextProperty 클래스를 정의함.
class CSCTextProperty
{
public:
	TCHAR		name[64] = _T("Arial");
	float		size = 10.0f;
	int			style = Gdiplus::FontStyleRegular;	//bold, italic, underline, strikeout
	float		shadow_depth = 0.0f;

	//> 0 이면 음절 그림자들을 별도 layer에 모아 fast_gaussian_blur로 한 번에 흐리게 처리한다.
	//0 이면 기존 하드 엣지 offset 그림자(호환). para[0][0] 값이 대표값으로 사용된다.
	float		shadow_blur_sigma = 3.0f;
	//< 1.0 이면 blur layer를 gray()로 톤 다운. 1.0 이면 컬러 톤 유지.
	float		shadow_gray_weight = 1.0f;

	float		thickness = 0.0f;
	float		round = 0.0f;
	float		round_thickness = 0.0f;
	Gdiplus::Color cr_text = Gdiplus::Color::Black;
	Gdiplus::Color cr_back = Gdiplus::Color::Transparent;
	Gdiplus::Color cr_stroke = Gdiplus::Color::Transparent;
	Gdiplus::Color cr_shadow = Gdiplus::Color::Transparent;
	Gdiplus::Color cr_round = Gdiplus::Color::Transparent;
	Gdiplus::Color cr_round_stroke = Gdiplus::Color::Transparent;
};

class CSCParagraph
{
public:
	CSCParagraph();
	~CSCParagraph();

	CString			text;
	CSCTextProperty	text_prop;
	CRect			r;						//이 텍스트가 그려질 위치(절대좌표가 아닌 0,0을 기준으로 상대좌표)

	//이 paragraph의 CSCTextProperty 설정에 맞는 Gdiplus::Font를 구한다.
	void			get_paragraph_font(Gdiplus::Graphics& g, Gdiplus::Font** font);


	//아래 static 함수들은 하나의 CSCParagraph에 대해 수행되는 함수들이 아니고
	//std::deque<std::deque<CSCParagraph>> 구조의 paragraph에 대해 수행해야 하므로
	//CSCParagraph의 멤버함수가 아닌 static으로 선언한다.

	//text의 태그를 파싱하여 각 음절의 속성을 설정한 후 para에 저장한다.
	//ti에는 font name, size, style, color 등이 세팅되어 있고
	//특별한 태그가 없으면 ti에 설정된 기본값을 사용한다.
	static void		build_paragraph_str(CString& text, std::deque<std::deque<CSCParagraph>>& para, CSCTextProperty* text_prop);

	//paragraph text 정보를 dc에 출력할 때 출력 크기를 계산하고 각 텍스트가 출력될 위치까지 CSCParagraph 멤버에 저장한다.
	//모든 para가 출력되는 최대 사각형을 리턴한다.
	static CRect	calc_text_rect(CRect rc, CDC* pDC, std::deque<std::deque<CSCParagraph>>& para, DWORD align);
	static int		get_max_width_line(std::deque<std::deque<CSCParagraph>>& para);

	//각 paragraph의 r이 계산된 후에 줄 간격을 spacing 배수로 조정한다. spacing이 1.0f이면 기본 줄 간격, 2.0f이면 줄 간격이 2배가 된다.
	static CRect	set_line_spacing(std::deque<std::deque<CSCParagraph>>& para, float spacing = 1.0f);
	//line번째 라인의 윗 간격(line-1 번째 라인과의 간격)만 spacing 배수로 조정한다. line >= 1 이어야 한다.
	static CRect	set_line_spacing(std::deque<std::deque<CSCParagraph>>& para, int line, float spacing = 1.0f);

	//static void		draw_text(CDC* pDC, std::deque<std::deque<CSCParagraph>>& para);
	static void		draw_text(Gdiplus::Graphics& g, std::deque<std::deque<CSCParagraph>>& para);

	//calc_text_rect()에서 이미 각 paragraph의 r이 align에 따라 정해지지만 이를 동적으로 변경하고자 할 경우 호출.
	static CRect	set_text_align(CRect rc, std::deque<std::deque<CSCParagraph>>& para, DWORD align);

	//텍스트 상하좌우 여백
	//static void		set_margin(std::deque<std::deque<CSCParagraph>>& para, float margin);

	//출력 위치를 이동시킨다.
	static CRect	offset(int x, int y);

	//para의 정보를 문자열로 리턴한다.
	//"(row, col) : text = %s, r = %s, name = %s, size = %.1f, style = %d\n" 스타일로 리턴한다.
	//실제 
	static CString	get_paragraph_info_string(std::deque<std::deque<CSCParagraph>>& para);
protected:
};
