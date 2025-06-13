#pragma once

#include <deque>
#include "../SCStatic/SCStatic.h"
#include "../../data_structure/SCParagraph/SCParagraph.h"

/*
[Usage]
- SCParagraph.h ���� ����.

*/

// CSCParagraphStatic�� CSCStatic�� ��ӹ޾� ��������� OnPaint()�� ������ �ٸ��Ƿ�
//�ʿ��� �κ��� CSCParagraphStatic�� �ٽ� ��������� �Ѵ�.
class CSCParagraphStatic : public CSCStatic
{
	DECLARE_DYNAMIC(CSCParagraphStatic)

public:
	CSCParagraphStatic();
	virtual ~CSCParagraphStatic();

	//tag��	���Ե� ��� �ؽ�Ʈ�� �����Ѵ�. �⺻ ���ڻ�, ���� �� �������� ��� �� �Ŀ�	�� �Լ��� ȣ���ؾ� �Ѵ�.
	CRect			set_text(CString sText);

	//���콺�� hover�� ������ �簢�� ǥ��
	void			draw_word_hover_rect(bool draw = true, Gdiplus::Color cr_rect = Gdiplus::Color::Transparent);

protected:
	std::deque<std::deque<CSCParagraph>> m_para;	//m_para[0][1] : 0�� ������ 1�� �ε��� ����
	CSCLogFont		m_sc_font;						//�⺻ ��Ʈ ����
	void			update_sc_font();				//���� ���氪�� m_sc_font�� ����

	//m_para[line][index]�� ��������� font�� SelectObject()�ϰ� pOldFont�� �����Ѵ�.
	//CFont*			select_paragraph_font(int line, int index, CDC* pDC, CFont* font);
	void			get_paragraph_font(int line, int index, Gdiplus::Graphics& g, Gdiplus::Font** font);


	int				m_max_width;			//�� ���ε� �� �ִ� �ʺ�
	int				m_max_width_line;		//�� ���ε� �� �ִ� �ʺ��� ���� ��ȣ
	CPoint			m_pt_icon;

	bool			m_auto_ctrl_size = true;//��µ� �ؽ�Ʈ�� ũ�Ⱑ ��Ʈ���� ũ�⺸�� Ŭ ��� ��Ʈ���� ũ�⸦ �ڵ����� �÷��ش�. default = true

//���콺�� hover�� �ܾ ǥ��
	bool			m_draw_word_hover_rect = false;				//���콺�� hover�� �ܾ ǥ���� ��. default = false
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

