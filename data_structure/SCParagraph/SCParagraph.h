#pragma once

#include <afxwin.h>
#include <gdiplus.h>
#include <deque>

/*
[태그된 텍스트 지원]
	- rich text를 지원하기 위해 제작.
	- CSCShapeDlg, CSCParagraphStatic 등에서 사용되며 html, smi 등의 tagged text를 표현할 수 있다.

	태그는 소문자로, 컬러값은 Gdiplus::Color에 정의된 이름(web color와 naming이 동일함)을 사용해야 한다.
	컬러 이름은 대소문자를 구분하지 않는다. ("red", "Red", "RED" 모두 동일함)
	태그명에 공백, 하이픈, underscore는 모두 무시된다.
	지원하는 태그는 다음과 같다.

	<f=굴림> = <font=굴림> = <font name=굴림> = <font_name=굴림>
	<sz=10> = <size=10> = <font size=10> = <font_size=10>
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
//불편한 점이 많으므로 Gdi+에서도 사용할 수 있도록 CSCLogFont를 정의함.
class CSCLogFont
{
public:
	CString		name = _T("Arial");
	float		size = 10.0f;
	int			style = Gdiplus::FontStyleRegular;	//bold, italic, underline, strikeout
	float		shadow_depth = 0.0f;
	float		thickness = 0.0f;
	Gdiplus::Color cr_text = Gdiplus::Color::Black;
	Gdiplus::Color cr_back = Gdiplus::Color::Transparent;
	Gdiplus::Color cr_stroke = Gdiplus::Color::Transparent;
	Gdiplus::Color cr_shadow = Gdiplus::Color::Transparent;
};

class CSCParagraph
{
public:
	CSCParagraph();
	~CSCParagraph();

	CString			text;
	CSCLogFont		lf;
	CRect			r;						//이 텍스트가 그려질 위치(절대좌표가 아닌 0,0을 기준으로 상대좌표)

	//text의 태그를 파싱하여 각 음절의 속성을 설정한 후 para에 저장한다.
	//lf에는 font name, size, style, color 등이 세팅되어 있고
	//특별한 태그가 없으면 lf에 설정된 기본값을 사용한다.
	static void		build_paragraph_str(CString& text, std::deque<std::deque<CSCParagraph>>& para, CSCLogFont* lf);

	//paragraph text 정보를 dc에 출력할 때 출력 크기를 계산하고 각 텍스트가 출력될 위치까지 CSCParagraph 멤버에 저장한다.
	static CRect	calc_text_rect(CRect rc, CDC* pDC, std::deque<std::deque<CSCParagraph>>& para, DWORD align);

	void			get_paragraph_font(Gdiplus::Graphics& g, Gdiplus::Font** font);

protected:
};
