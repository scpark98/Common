#pragma once

#include <afxwin.h>
#include <gdiplus.h>
#include <deque>

/*
* text coloring�� �����ϱ� ���� ����.
* CSCParagraphStatic���� ���Ǹ� html, smi ���� tagged text�� ����.
*/

class CSCParagraph
{
public:
	CSCParagraph();
	~CSCParagraph();

	Gdiplus::Color	cr_text;									//<cr>
	Gdiplus::Color	cr_back = Gdiplus::Color::Transparent;		//<cb> = <crb>
	Gdiplus::Color	cr_stroke = Gdiplus::Color::Transparent;	//<sc> stroke color
	Gdiplus::Color	cr_shadow = Gdiplus::Color::Transparent;	//<sd> shadow color

	CString			text;
	CString 		name;			//<fontname="Arial">
	int				size;			//<fontsize=10>
	bool			bold;			//<b>
	bool			italic;			//<i>
	bool			underline;		//<u>
	bool			strike;			//<s>
	Gdiplus::REAL	thickness;		//<st> stroke thickness
	CRect			r;				//�� �ؽ�Ʈ�� �׷��� ��ġ(������ǥ�� �ƴ� 0,0�� �������� �����ǥ)

	//text�� �±׸� �Ľ��Ͽ� �� ������ �Ӽ��� ������ �� para�� �����Ѵ�.
	//cr_text, cr_back�� ����, ��� �⺻��
	static void		build_paragraph_str(CString text, std::deque<std::deque<CSCParagraph>>& para, LOGFONT* lf,
										Gdiplus::Color cr_text,
										Gdiplus::Color cr_back,
										Gdiplus::Color cr_stroke = Gdiplus::Color::Transparent,
										Gdiplus::Color cr_shadow = Gdiplus::Color::Transparent);

protected:
};
