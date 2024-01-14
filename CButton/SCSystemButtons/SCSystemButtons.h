#pragma once

/*
* titlebar가 없는 dlg를 자주 만들게 되는데 시스템 버튼의 조합도 다양할 수 있다.
* parent dlg의 h에서 다음과 같이 선언하고
		CSCSystemButtons	m_sys_buttons;
  cpp에서 다음과 같이 생성해준다.
		m_sys_buttons.create(this, rc.right, m_titlebar_height, -1, SC_PIN, SC_MINIMIZE, SC_MAXIMIZE, SC_CLOSE);
		//m_sys_buttons.create(this, rc.right, m_titlebar_height, -1, SC_CLOSE);//종료 버튼만 필요한 경우
		m_sys_buttons.ShowWindow(SW_SHOW);
  
* set_color_theme()으로 컬러 테마를 선택할 수 있다.
* parent dlg의 OnSize()등에서 아래와 같이 보정해줘야 올바른 위치에 표시된다.
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
	//m_sys_buttons.create(this, rc.right, m_titlebar_height, -1, SC_CLOSE);//종료 버튼만 필요한 경우
	template <typename ... Types> void	create(CWnd* parent, int right_end, int height, int width, Types... args)
	{
		int n = sizeof...(args);
		int arg[] = { args... };

		m_button.clear();
		m_button.resize(n);

		if (width < 0)
			width = DEFAULT_SYSTEM_BUTTON_WIDTH;

		//컨트롤 생성과 함께 각 버튼의 위치가 정해진다.
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

	//resource image는 png만 사용한다.
	template <typename ... Types> void	set_buttons_image(Types... args)
	{
		int n = sizeof...(args);
		int arg[] = { args... };

		//버튼에 resouce image를 설정하는 경우에는 초기화시키진 않는다.
		//image의 개수는 버튼의 개수와 일치해야 한다.
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
	int		m_gap = 0;				//버튼 사이 간격

	bool	m_mouse_hover = false;	//마우스가 컨트롤에 들어왔는지
	bool	m_mouse_track = false;
	int		m_over_index = -1;		//마우스 오버 또는 다운 인덱스
	bool	m_down_state = false;

	int		get_button_index(CPoint pt);

	COLORREF	m_cr_back = ::GetSysColor(COLOR_ACTIVECAPTION);
	COLORREF	m_cr_over;	//m_cr_back으로 계산
	COLORREF	m_cr_down;	//m_cr_back으로 계산
	COLORREF	m_cr_pen;	//그리기 색상

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


