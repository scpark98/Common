#pragma once

#include <algorithm>
#include <afxwin.h>
#include <gdiplus.h>
#include <deque>

/*
[�±׵� �ؽ�Ʈ ����]
	- html�� ���� tagged text�� �����ϱ� ���� ����.
	- CSCShapeDlg, CSCParagraphStatic ��� ���Ǹ� html, smi ���� tagged text�� ǥ���� �� �ִ�.

	�±״� �ҹ��ڷ�, �÷����� Gdiplus::Color�� ���ǵ� �̸�(web color�� naming�� ������)�� ����ؾ� �Ѵ�.
	�÷� �̸��� ��ҹ��ڸ� �������� �ʴ´�. ("red", "Red", "RED" ��� ������)
	�±׸� ����, ������, underscore�� ��� ���õȴ�.
	�����ϴ� �±״� ������ ����.

	<f=����> = <font=����> = <name=����> = <font_name=����>
	<sz=10> = <size=10> = <font_size=10>
	<b>						//bold
	<i>						//italic
	<u>						//underline
	<s>						//strike out
	<cr=red> = <ct=red>		//cr_text
	<crb=Blue> = <cb=Blue>	//cr_back
	<br>					//line break
	ex. "<b><cr=red>This</b></cr > is a <ct=blue><i>sample</i> <b>paragraph</b>."

	- cr�� Gdiplus::Color�� ��õ� �÷� �ܿ� ���� ���ĵ鵵 �����Ѵ�.
	  <cr=#RRGGBBAA>,			//rgba, #���� ���۵Ǵ� 16������.
	  <cr=123,45,67,128>		//rgba, ���ڷθ� 3�ڸ� �Ǵ� 4�ڸ��� �޸��� �����Ѵ�.
	  <cr=h90,30,100>,		//hsi, h�� ���۵ǰ� �޸��� �����Ѵ�.

[�ڵ� �帧]
	- set_text()			: tag�� ����ϴ� �ؽ�Ʈ�� �����Ѵ�.
	  get_tag_str()			: tag�� �ؽ�Ʈ�� �и��Ѵ�.
	  build_paragraph_str()	: tag�� �Ľ��Ͽ� �� ������ �Ӽ��� �����Ѵ�.
	  calc_text_rect()		: �� ������ ��� ��ġ�� ����Ѵ�.
	  OnPaint()				: ������ ���� ��ġ�� �� ���� �ؽ�Ʈ�� ����Ѵ�.

	- calc_text_rect()���� �� �׸��� ��µ� ��ġ���� ��� ���ǰ� OnPaint()������ ������ ��ġ�� �׸��⸸ �Ѵ�.

*/

//���� LOGFONT�� GDI���̰� fontsize�� int�� �����ǰų�, lfFaceName�� TCHAR [] ��
//������ ���� �����Ƿ� Gdi+������ ����� �� �ֵ��� CSCTextProperty Ŭ������ ������.
class CSCTextProperty
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
	CSCTextProperty	text_prop;
	CRect			r;						//�� �ؽ�Ʈ�� �׷��� ��ġ(������ǥ�� �ƴ� 0,0�� �������� �����ǥ)

	//�� paragraph�� CSCTextProperty ������ �´� Gdiplus::Font�� ���Ѵ�.
	void			get_paragraph_font(Gdiplus::Graphics& g, Gdiplus::Font** font);


	//�Ʒ� static �Լ����� �ϳ��� CSCParagraph�� ���� ����Ǵ� �Լ����� �ƴϰ�
	//std::deque<std::deque<CSCParagraph>> ������ paragraph�� ���� �����ؾ� �ϹǷ�
	//CSCParagraph�� ����Լ��� �ƴ� static���� �����Ѵ�.

	//text�� �±׸� �Ľ��Ͽ� �� ������ �Ӽ��� ������ �� para�� �����Ѵ�.
	//ti���� font name, size, style, color ���� ���õǾ� �ְ�
	//Ư���� �±װ� ������ ti�� ������ �⺻���� ����Ѵ�.
	static void		build_paragraph_str(CString& text, std::deque<std::deque<CSCParagraph>>& para, CSCTextProperty* text_prop);

	//paragraph text ������ dc�� ����� �� ��� ũ�⸦ ����ϰ� �� �ؽ�Ʈ�� ��µ� ��ġ���� CSCParagraph ����� �����Ѵ�.
	//��� para�� ��µǴ� �ִ� �簢���� �����Ѵ�.
	static CRect	calc_text_rect(CRect rc, CDC* pDC, std::deque<std::deque<CSCParagraph>>& para, DWORD align);
	static int		get_max_width_line(std::deque<std::deque<CSCParagraph>>& para);

	//static void		draw_text(CDC* pDC, std::deque<std::deque<CSCParagraph>>& para);
	static void		draw_text(Gdiplus::Graphics& g, std::deque<std::deque<CSCParagraph>>& para);

	//calc_text_rect()���� �̹� �� paragraph�� r�� align�� ���� ���������� �̸� �������� �����ϰ��� �� ��� ȣ��.
	static CRect	set_text_align(CRect rc, std::deque<std::deque<CSCParagraph>>& para, DWORD align);

	//��� ��ġ�� �̵���Ų��.
	static CRect	offset(int x, int y);

protected:
};
