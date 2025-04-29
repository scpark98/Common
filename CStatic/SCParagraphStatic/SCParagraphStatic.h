#pragma once

#include <deque>
#include "../SCStatic/SCStatic.h"
#include "../../data_structure/SCParagraph/SCParagraph.h"

/*
* <name=�ü�><b><cr=Red><u>This</b></cr> <size=18>is</size></u> a<br><crb=Red><i><size=14>��</i>��</size></name><cr=Green><b><size=30>pa</fontname><cr=Yellow>ra<s>gr</cr>a</s>ph</b>.
* 
* ���� ���� �±׸� ����Ͽ� �ؽ�Ʈ�� ǥ���� �� �ִ�.
* 
* �±״� �ҹ��ڷ�, �÷����� Gdiplus::Color�� ���ǵ� �̸��� ����ؾ� �Ѵ�.
* �±׸� ����, ������, underscore�� ��� ���õȴ�.
* �����ϴ� �±״� ������ ����.
* <n=����> = <name=����> = <font name=����> = <font_name=����>
* <sz=10> = <size=10> = <font size=10> = <font_size=10>
* <b>						//bold
* <i>						//italic
* <u>						//underline
* <s>						//strike out
* <cr=Red> = <ct=Red>		//text color. Red�� Gdiplus::Color�� ���ǵ� ���� �̸��� ����Ѵ�.
* <crb=Blue> = <cb=Blue>	//text back color
* <br>						//line break
* 
* �帧 : set_text() -> get_tag_str() -> recalc_text_size() -> OnPaint()
* recalc_text_size()���� �� �׸��� ��µ� ��ġ���� ��� ���ǰ� OnPaint()������ ������ ��ġ�� �׸��⸸ �Ѵ�.

* cr�� Gdiplus::Color�� ��õ� �÷� �ܿ� ���� ���ĵ鵵 �����Ѵ�.
	<cr=#RRGGBBAA>,		//rgba, #���� ���۵Ǵ� 16������.
	<cr=123,45,67,128>	//rgba, ���ڷθ� 3�ڸ� �Ǵ� 4�ڸ��� �޸��� ǥ���Ѵ�.
	<cr=h90,30,100>,	//hsi	h�� ���۵ǰ� �޸��� ǥ���Ѵ�.
*
* [������ ����]
* - recalc_text_rect �� OnPaint()���� CDC�� �ƴ� Gdiplus�� ��� ��ȯ�ϰ��� ������ GetTextExtent()�� MeasureString()�� ���� ������ �켱 ������.
* - outline <o>
* - shadow <sd>
* - <sdc=Red>		//shadow color
* - <sdd=3>			//shadow depth
*/

// CSCParagraphStatic�� CSCStatic�� ��ӹ޾� ��������� OnPaint()�� ������ �ٸ��Ƿ�
//�ʿ��� �κ��� CSCParagraphStatic�� �ٽ� ��������� �Ѵ�.
class CSCParagraphStatic : public CSCStatic
{
	DECLARE_DYNAMIC(CSCParagraphStatic)

public:
	CSCParagraphStatic();
	virtual ~CSCParagraphStatic();

	//�±״� �ҹ��ڷ�, �÷����� Gdiplus::Color�� ���ǵ� �̸��� ����ؾ� �Ѵ�.
	//�±׸� ����, ������, underscore�� ��� ���õȴ�.
	//�����ϴ� �±״� ������ ����.
	//<f=����> = <font=����> = <font name=����> = <font_name=����>
	//<sz=10> = <size=10> = <font size=10> = <font_size=10>
	//<b>						//bold
	//<i>						//italic
	//<u>						//underline
	//<s>						//strike out
	//<cr=Red> = <ct=Red>		//text color. Red�� Gdiplus::Color�� ���ǵ� ���� �̸��� ����Ѵ�.
	//<crb=Blue> = <cb=Blue>	//text back color.
	//"<b><cr=Red>This</b></cr > is a <ct=Blue><i>sample</i> <b>paragraph</b>."
	CRect			set_text(CString sText, Gdiplus::Color cr_text = Gdiplus::Color::Transparent);

	//���콺�� hover�� ������ �簢�� ǥ��
	void			draw_word_hover_rect(bool draw = true, Gdiplus::Color cr_rect = Gdiplus::Color::Transparent);

protected:
	std::deque<std::deque<CSCParagraph>> m_para;	//m_para[0][1] : 0�� ������ 1�� �ε��� ����

	CRect			recalc_text_size();

	//m_para[line][index]�� ��������� font�� SelectObject()�ϰ� pOldFont�� �����Ѵ�.
	CFont*			select_paragraph_font(int line, int index, CDC* pDC, CFont* font);
	void			get_paragraph_font(int line, int index, Gdiplus::Graphics& g, Gdiplus::Font** font);

	int				m_max_width;			//�� ���ε� �� �ִ� �ʺ�
	int				m_max_width_line;		//�� ���ε� �� �ִ� �ʺ��� ���� ��ȣ
	CPoint			m_pt_icon;

//���콺�� hover�� �ܾ ǥ��
	bool			m_draw_word_hover_rect = false;				//���콺�� hover�� �ܾ ǥ���� ��
	Gdiplus::Color	m_cr_word_hover_rect = Gdiplus::Color::Red;	//hover�� �ܾ ǥ���� ����
	CPoint			m_pos_word_hover = CPoint(-1, -1);			//���콺�� ��ġ�� �ܾ��� i, j �ε��� (��ǥ���� �ƴ� �ε���)

protected:
	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnPaint();
	virtual void PreSubclassWindow();
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
};

