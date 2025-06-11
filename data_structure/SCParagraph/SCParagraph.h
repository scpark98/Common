#pragma once

#include <afxwin.h>
#include <gdiplus.h>
#include <deque>

//���� LOGFONT�� GDI���̰� fontsize�� int�� �����ǰų�, lfFaceName�� TCHAR [] ��
//������ ���� �����Ƿ� Gdi+������ ����� �� �ֵ��� �߰���.
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
* text coloring�� �����ϱ� ���� ����.
* CSCParagraphStatic���� ���Ǹ� html, smi ���� tagged text�� ����.
*/

class CSCParagraph
{
public:
	CSCParagraph();
	~CSCParagraph();

	CString			text;
	CSCLogFont		lf;
	CRect			r;						//�� �ؽ�Ʈ�� �׷��� ��ġ(������ǥ�� �ƴ� 0,0�� �������� �����ǥ)

	//text�� �±׸� �Ľ��Ͽ� �� ������ �Ӽ��� ������ �� para�� �����Ѵ�.
	//cr_text, cr_back�� ����, ��� �⺻��
	static void		build_paragraph_str(CString text, std::deque<std::deque<CSCParagraph>>& para, CSCLogFont* lf);

protected:
};
