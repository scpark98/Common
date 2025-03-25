#pragma once

#include <afxwin.h>
#include <deque>
#include "../../GdiplusBitmap.h"

// CSCStepCtrl
/*
* CSliderCtrl or CProgressCtrl���� �ణ �ٸ��� ���� ������ �ؽ�Ʈ�� �Բ� ǥ���� �ʿ䰡 �־�
  CStatic�� ��ӹ޾� ������.
  CSCSliderCtrl�� style_step�� �߰��Ͽ� �������̾����� CSliderCtrl or CProgressCtrl����
  �ٸ��� ó���Ǿ�� �ϴ� �κе��� �����Ƿ� ���� ������.

  �� �� ��Ʈ���� SetRange�� ������ ����� ������ StepCtrl�� �������� �Ϸ��ı����� ������ �ʿ��ϴ�.
  m_step.set_range(0, 10);�̶�� -1 ~ 11���� ǥ�� �����ϴ�. -1=������, 11=�Ϸ�

  ���� �� ���ܸ��� �ؽ�Ʈ�� ǥ���� �� �ִ�.

* ������ ��Ģ
  step	: ������ �ܰ� �׸��� ��Ī�ϸ� thumb�� text�� �����ϴ� �ǹ̷� ���
  thumb	: �� ���ܿ� �׷����� ���� (���� �Ǵ� �簢��)
  text	: �� ������ ���� �Ǵ� �ϴܿ� ǥ�õǴ� ���̺�

* ����
  //.h�� #include �� ���� ����
  CSCStepCtrl		m_step;

  //.cpp���� ����
  m_step.set_direction(true);
  m_step.set_step_style(step_style_circle);
  m_step.set_step_count(4);
*/

//�� ������ ���
enum THUMB_STYLE
{
	thumb_style_none = -1,
	thumb_style_circle,
	thumb_style_circle_with_num,
	thumb_style_rect,
	thumb_style_rect_round,
	thumb_style_diamond,
	thumb_style_arrow,
};

class CSCStepElement
{
public:
	CRect			r;		//step�� �׷����� rect�̸� �ؽ�Ʈ�� r�� �������� �׷����Ƿ� �ؽ�Ʈ ������ �������� �ʴ´�.
	CString			text;

	//�� ���� ����� m_step_style�� �⺻ ��Ÿ�Ϸ� ������ ��� ���� �޸� ǥ���� �ʿ䵵 �ִ�.
	int				thumb_style = thumb_style_none;

	//�� �⺻���� ����� �״���̸� ��ǥ �÷��� m_cr_thumb�� ����ϰ� �����Ǿ� ������ �ش� ������ ǥ��
	Gdiplus::Color	cr_thumb = Gdiplus::Color::Transparent;
	Gdiplus::Color	cr_text = Gdiplus::Color::Transparent;
};

class CSCStepCtrl : public CStatic
{
	DECLARE_DYNAMIC(CSCStepCtrl)

public:
	CSCStepCtrl();
	virtual ~CSCStepCtrl();

	void			set_style(bool is_horz, int thumb_style = thumb_style_circle) { m_horz = is_horz; m_thumb_style = thumb_style; }

	//�� �ܰ� ��
	void			set_step_count(int count) { m_pos = -1; m_step.resize(count); }
	int				get_step_count() { return m_step.size(); }

	//index < 0�̸� ��� thumb�� ����.
	void			set_thumb_style(int index, int style);

	//l, t, r, b�� margin. �������� ��� �ؽ�Ʈ�� ����� ǥ�õǵ��� �¿� ������ ��� �Ѵ�.
	void			set_margin(int left, int top, int right, int bottom) { m_margin = CRect(left, top, right, bottom); }
	//step�� �ؽ�Ʈ ����
	void			set_gap_to_text(int gap) { m_gap_to_text = gap; }

	int				get_pos() { return m_pos; }
	void			set_pos(int pos) { m_pos = pos; Invalidate(); }
	void			step() { m_pos++; Invalidate(); }

	//�� ���ܿ� �ؽ�Ʈ ����. Transparent���̸� �ش� �׸� ���õ�.
	void			set_text(int index, CString text, Gdiplus::Color cr = Gdiplus::Color::Transparent);
	template <typename ... Types> void set_texts(Types... args)
	{
		int n = min(m_step.size(), sizeof...(args));
		CString texts[] = { args... };

		for (int i = 0; i < n; i++)
			m_step[i].text = texts[i];
	}

	//thumb�� text�� ������ ��� �����Ѵ�.
	void			set_step_color(int index, Gdiplus::Color cr);
	void			set_thumb_color(int index, Gdiplus::Color cr);
	void			set_text_color(int index, Gdiplus::Color cr);

	//Ư�� step�� thumb�� text�� ������ ���½�Ű�� �⺻���� ����ϰ� �Ѵ�.
	//index == -1�̸� ��� ���� ����
	void			reset_step_color(int index = -1);

	//�� ������ thumb ũ��
	void			set_thumb_size(int size);

	void			set_draw_line(bool draw) { m_draw_line = draw; }

	void			set_back_color(Gdiplus::Color cr) { m_cr_back = cr; }
	void			set_color_line_active(Gdiplus::Color cr) { m_cr_line_active = cr; }
	void			set_color_line_inactive(Gdiplus::Color cr) { m_cr_line_inactive = cr; }

protected:
	bool			m_horz = true;
	bool			m_draw_line = true;
	int				m_pos = -1;
	int				m_thumb_style = thumb_style_circle;
	int				m_thumb_size = 18;
	CRect			m_margin = CRect(8, 8, 8, 8);
	int				m_gap_to_text = 8;	//step�� �ؽ�Ʈ ���� ����

	std::deque<CSCStepElement> m_step;

//color
	Gdiplus::Color	m_cr_back;	//���� dlg�� ����� ����� GetSysColor(COLOR_3DFACE)�ε� �� ���� ���⼭�� �ʱ�ȭ �� �� ���� �����ڿ��� �ؾ� ��.

	Gdiplus::Color	m_cr_text_active = Gdiplus::Color::RoyalBlue;
	Gdiplus::Color	m_cr_text_current = Gdiplus::Color::Blue;
	Gdiplus::Color	m_cr_text_inactive = Gdiplus::Color::Gray;

	Gdiplus::Color	m_cr_thumb_active = Gdiplus::Color::RoyalBlue;
	Gdiplus::Color	m_cr_thumb_current = Gdiplus::Color::Blue;
	Gdiplus::Color	m_cr_thumb_inactive = Gdiplus::Color::Gray;

	Gdiplus::Color	m_cr_thumb_outline = Gdiplus::Color::RoyalBlue;
	Gdiplus::Color	m_cr_line_active = Gdiplus::Color::RoyalBlue;
	Gdiplus::Color	m_cr_line_inactive = Gdiplus::Color::LightGray;

//font
	LOGFONT			m_lf;
	CFont			m_font;
	void			reconstruct_font();

protected:
	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnPaint();
	afx_msg void OnSize(UINT nType, int cx, int cy);
	virtual void PreSubclassWindow();
	virtual BOOL PreTranslateMessage(MSG* pMsg);
};


