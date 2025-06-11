#pragma once

#include <afxwin.h>
#include <gdiplus.h>
#include <deque>

//기존 LOGFONT는 GDI용이고 fontsize가 int만 지원되거나, lfFaceName이 TCHAR [] 등
//불편한 점이 많으므로 Gdi+에서도 사용할 수 있도록 추가함.
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

/*
* text coloring을 지원하기 위해 생성.
* CSCParagraphStatic에서 사용되며 html, smi 등의 tagged text를 지원.
*/

class CSCParagraph
{
public:
	CSCParagraph();
	~CSCParagraph();

	CString			text;
	CSCLogFont		lf;
	CRect			r;						//이 텍스트가 그려질 위치(절대좌표가 아닌 0,0을 기준으로 상대좌표)

	//text의 태그를 파싱하여 각 구문의 속성을 설정한 후 para에 저장한다.
	//cr_text, cr_back은 글자, 배경 기본값
	static void		build_paragraph_str(CString text, std::deque<std::deque<CSCParagraph>>& para, CSCLogFont* lf);

protected:
};
