#pragma once

#include <afxwin.h>
#include <gdiplus.h>
#include <deque>

/*
* text coloring을 지원하기 위해 생성.
* CSCParagraphStatic에서 사용되며 html, smi 등의 tagged text를 지원.
*/

class CSCParagraph
{
public:
	CSCParagraph();
	~CSCParagraph();

	Gdiplus::Color	cr_text;		//<cr>
	Gdiplus::Color	cr_back;		//<crb>

	CString			text;
	CString 		name;			//<fontname="Arial">
	int				size;			//<fontsize=10>
	bool			bold;			//<b>
	bool			italic;			//<i>
	bool			underline;		//<u>
	bool			strike;			//<s>
	CRect			r;				//이 텍스트가 그려질 위치(절대좌표가 아닌 0,0을 기준으로 상대좌표)

	//text의 태그를 파싱하여 각 구문의 속성을 설정한 후 para에 저장한다.
	//cr_text, cr_back은 글자, 배경 기본값
	static void		build_paragraph_str(CString text, std::deque<std::deque<CSCParagraph>>& para, LOGFONT* lf, Gdiplus::Color cr_text, Gdiplus::Color cr_back);

protected:
};
