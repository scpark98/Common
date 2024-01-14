#pragma once

/*
* titlebar�� ���� dlg�� ���� ����� �Ǵµ� �ý��� ��ư�� ���յ� �پ��� �� �ִ�.
* parent dlg�� h���� ������ ���� �����ϰ�
		CSCSystemButtons	m_sys_buttons;
  cpp���� ������ ���� �������ش�.
		m_sys_buttons.create(this, rc.right, m_titlebar_height, -1, SC_PIN, SC_MINIMIZE, SC_MAXIMIZE, SC_CLOSE);
		//m_sys_buttons.create(this, rc.right, m_titlebar_height, -1, SC_CLOSE);//���� ��ư�� �ʿ��� ���
		m_sys_buttons.ShowWindow(SW_SHOW);
  
* set_color_theme()���� �÷� �׸��� ������ �� �ִ�.
* parent dlg�� OnSize()��� �Ʒ��� ���� ��������� �ùٸ� ��ġ�� ǥ�õȴ�.
		m_sys_buttons.adjust_right(rc.right);
*/

#include <afxwin.h>
#include <deque>
#include "../../GdiplusBitmap.h"

#define DEFAULT_SYSTEM_BUTTON_WIDTH 44
#define SC_PIN	SC_SEPARATOR + 0x900F	//add new sc command for always on top

class CSCSystemButtonProperty
{
public:
	CSCSystemButtonProperty() {};
	CSCSystemButtonProperty(int _cmd, CRect _r)
	{
		cmd = _cmd;
		r = _r;
	}

	int		cmd = -1;
	CRect	r;
	CGdiplusBitmap	img;
};

// CSCSystemButtons
class CSCSystemButtons : public CButton
{
	DECLARE_DYNAMIC(CSCSystemButtons)

public:
	CSCSystemButtons();
	virtual ~CSCSystemButtons();

	//m_sys_buttons.create(this, rc.right, m_titlebar_height, -1, SC_PIN, SC_MINIMIZE, SC_MAXIMIZE, SC_CLOSE);
	//m_sys_buttons.create(this, rc.right, m_titlebar_height, -1, SC_CLOSE);//���� ��ư�� �ʿ��� ���
	template <typename ... Types> void	create(CWnd* parent, int right_end, int height, int width, Types... args)
	{
		int n = sizeof...(args);
		int arg[] = { args... };

		m_button.clear();
		m_button.resize(n);

		if (width < 0)
			width = DEFAULT_SYSTEM_BUTTON_WIDTH;

		//��Ʈ�� ������ �Բ� �� ��ư�� ��ġ�� ��������.
		for (int i = 0; i < m_button.size(); i++)
		{
			m_button[i].cmd = arg[i];
			m_button[i].r = CRect(i * (width + m_gap), 1, i * (width + m_gap) + width, height - 1);
		}

		Create(_T("CSCSystemButtons"), WS_CHILD,
			CRect(right_end - m_button.size() * width - (m_button.size() - 1) * m_gap, 0, right_end, height), parent, 0);

		adjust_right(right_end);

		ShowWindow(SW_SHOW);
	}

	//resource image�� png�� ����Ѵ�.
	template <typename ... Types> void	set_buttons_image(Types... args)
	{
		int n = sizeof...(args);
		int arg[] = { args... };

		//��ư�� resouce image�� �����ϴ� ��쿡�� �ʱ�ȭ��Ű�� �ʴ´�.
		//image�� ������ ��ư�� ������ ��ġ�ؾ� �Ѵ�.
		ASSERT(m_button.size() == n);

		for (int i = 0; i < n; i++)
		{
			m_button[i].img.load(arg[i]);
		}
	}

	void	adjust_right(int right_end);

	enum COLOR_THEME
	{
		color_theme_window = 0,
		color_theme_visualstudio,
		color_theme_gray,
	};
	void	set_color_theme(int theme);

protected:
	std::deque<CSCSystemButtonProperty> m_button;
	int		m_gap = 0;				//��ư ���� ����

	bool	m_mouse_hover = false;	//���콺�� ��Ʈ�ѿ� ���Դ���
	bool	m_mouse_track = false;
	int		m_over_index = -1;		//���콺 ���� �Ǵ� �ٿ� �ε���
	bool	m_down_state = false;

	int		get_button_index(CPoint pt);

	COLORREF	m_cr_back = ::GetSysColor(COLOR_ACTIVECAPTION);
	COLORREF	m_cr_over;	//m_cr_back���� ���
	COLORREF	m_cr_down;	//m_cr_back���� ���
	COLORREF	m_cr_pen;	//�׸��� ����

protected:
	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnPaint();
	afx_msg BOOL OnEraseBkgnd(CDC* pDC);
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
	afx_msg void OnMouseHover(UINT nFlags, CPoint point);
	afx_msg void OnMouseLeave();
};


