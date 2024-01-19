#pragma once

/*
* titlebar�� ���� dlg�� ���� ����� �Ǵµ� �ý��� ��ư�� ���յ� �پ��� �� �ִ�.
* parent dlg�� h���� ������ ���� �����ϰ�
		CSCSystemButtons	m_sys_buttons;
  cpp���� ������ ���� �������ش�.
		//4�� ��ư�� ����ϴ� ���
		m_sys_buttons.create(this, rc.right, m_titlebar_height, -1, SC_PIN, SC_MINIMIZE, SC_MAXIMIZE, SC_CLOSE);
		//���� ��ư�� �ʿ��� ���� �Ʒ��� ���� ȣ��
		m_sys_buttons.create(this, rc.right, m_titlebar_height, -1, SC_CLOSE);
		//�Ǵ� create(this);�Ͽ� ���� �����ϰ�
		//set_buttons_cmd(SC_HELP, SC_PIN, SC_MINIMIZE, SC_MAXIMIZE, SC_CLOSE); �� ���� ��ư�� ���߿� �߰��ϴ� �͵� �����ϴ�.
  
* set_color_theme()���� �÷� �׸��� ������ �� �ִ�.
* parent dlg�� OnSize()��� �Ʒ��� ���� ��������� �ùٸ� ��ġ�� ǥ�õȴ�.
		m_sys_buttons.adjust_right(rc.right);
*/

#include <afxwin.h>
#include <deque>
#include "../../GdiplusBitmap.h"

#define DEFAULT_SYSTEM_BUTTON_WIDTH 44

enum SCSYSTEM_BUTTON_CUSTOM
{
	SC_PIN = SC_SEPARATOR + 0x1001,	//add new sc command for always on top
	SC_HELP,
};

class CSCSystemButtonProperty
{
public:
	CSCSystemButtonProperty() {};
	CSCSystemButtonProperty(int _cmd, CRect _r = 0)
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

	//�� ��ư�� ũ��� ������10�� ������ ���� �⺻���� ��.
	void create(CWnd* parent, int right_end = -1, int width = 44, int height = 32);

	template <typename ... Types> void	create(CWnd* parent, int right_end, int width, int height, Types... args)
	{
		int n = sizeof...(args);
		int arg[] = { args... };

		//set_buttons_cmd()�� ���� ��ư�� ������ �̹� �����Ǿ� �ִ� ���
		//�� create()�Լ������� �ʱ�ȭ��Ű�� �ٽ� �����ؾ� �Ѵ�.
		if (n > 0)
		{
			m_button.clear();
			m_button.resize(n);
		}

		if (right_end > 0)
			m_right_end = right_end;
		if (width > 0)
			m_button_width = width;
		if (height > 0)
			m_button_height = height;

		//��Ʈ�� ������ �Բ� �� ��ư�� ��ġ�� ��������.
		for (int i = 0; i < m_button.size(); i++)
		{
			m_button[i].cmd = arg[i];
			m_button[i].r = CRect(i * (m_button_width + m_gap), 1, i * (m_button_width + m_gap) + m_button_width, m_button_height - 1);
		}

		Create(_T("CSCSystemButtons"), WS_CHILD,
			CRect(m_right_end - m_button.size() * m_button_width - (m_button.size() - 1) * m_gap, 0, m_right_end, m_button_height), parent, 0);

		ShowWindow(SW_SHOW);
	}

	//���ϴ� �ý��� ��ư���� �߰��Ѵ�. �ּ� 0������ �ִ� 4������ �����Ǿ� �ִ�.
	//�߰��ϰ��� �ϴ� ��ư�� �ִٸ� define�ϰ� �ش� �̹����� �׷��ִ� �ڵ� �߰��ϰ� �ش� �׼ǿ� ���� �ڵ带 �߰��ϸ� �ȴ�.
	//set_buttons_cmd(SC_PIN, SC_MINIMIZE, SC_MAXIMIZE, SC_CLOSE);
	//��ư�� �� ������ ���ϸ� �� ��ư�� ��ġ ���� ��������� �Ѵ�.
	template <typename ... Types> void	set_buttons_cmd(Types... args)
	{
		int n = sizeof...(args);
		int arg[] = { args... };

		m_button.clear();
		m_button.resize(n);

		for (int i = 0; i < n; i++)
		{
			m_button[i].cmd = arg[i];
		}

		resize();
		//MoveWindow(m_right_end - m_button.size() * m_button_width - (m_button.size() - 1) * m_gap, 0,
		//	m_button.size() * m_button_width - (m_button.size() - 1) * m_gap,
		//	m_button_height);
	}

	//Ư�� ��ġ�� ���ο� ��� ��ư�� �߰��Ѵ�.
	void insert_button(int idx, int cmd)
	{
		//idx�� -1�̸� �� ���� ��ư�� �߰��Ѵ�.
		if (idx == -1)
			m_button.push_back(CSCSystemButtonProperty(cmd));
		else
			m_button.insert(m_button.begin() + idx, CSCSystemButtonProperty(cmd));

		resize();
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

	void	resize();
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
	int		m_button_width = 44;
	int		m_button_height = 32;
	int		m_right_end;			//��ư ��Ʈ���� ���� ��ǥ�� ���� n���� ��ư ��ġ�� �����ȴ�.
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


