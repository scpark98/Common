#pragma once

/*
* titlebar가 없는 dlg를 자주 만들게 되는데 시스템 버튼의 조합도 다양할 수 있다.
* parent dlg의 h에서 다음과 같이 선언하고
		CSCSystemButtons	m_sys_buttons;
  cpp에서 다음과 같이 생성해준다.
		//4개 버튼을 사용하는 경우
		m_sys_buttons.create(this, rc.right, m_titlebar_height, -1, SC_PIN, SC_MINIMIZE, SC_MAXIMIZE, SC_CLOSE);
		//종료 버튼만 필요한 경우는 아래와 같이 호출
		m_sys_buttons.create(this, rc.right, m_titlebar_height, -1, SC_CLOSE);
		//또는 create(this);하여 먼저 생성하고
		//set_buttons_cmd(SC_HELP, SC_PIN, SC_MINIMIZE, SC_MAXIMIZE, SC_CLOSE); 와 같이 버튼을 나중에 추가하는 것도 가능하다.
  
* set_color_theme()으로 컬러 테마를 선택할 수 있다.
* parent dlg의 OnSize()등에서 아래와 같이 보정해줘야 올바른 위치에 표시된다.
		m_sys_buttons.adjust_right(rc.right);
*/

#include <afxwin.h>
#include <deque>
#include "../../GdiplusBitmap.h"
#include "../../colors.h"

#define DEFAULT_SYSTEM_BUTTON_WIDTH 44

enum SCSYSTEM_BUTTON_CUSTOM
{
	//사용자 정의 cmd를 SC_SEPARATOR + 0x1001부터 했으나
	//SC_HELP가 다른 시스템 명령과 겹치는 듯하여 WM_USER + 0x1001로 수정함.
	//SC_SEPARATOR가 WinUser.h파일에서 보면 SC_로 시작되는 #define들 중에 가장 마지막 값이 아니므로
	//뭔가 다른 값이 겹칠 가능성이 높기 때문에 WM_USER로 변경함.
	SC_PIN = WM_USER + 0x1001,	//add new sc command for always on top
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
	//m_sys_buttons.create(this, rc.right, m_titlebar_height, -1, SC_CLOSE);//종료 버튼만 필요한 경우

	//각 버튼의 크기는 윈도우10과 동일한 값을 기본값을 함.
	void create(CWnd* parent, int top = 0, int right = -1, int width = 44, int height = 32);

	template <typename ... Types> void	create(CWnd* parent, int top, int right, int width, int height, Types... args)
	{
		int n = sizeof...(args);
		int arg[] = { args... };

		//set_buttons_cmd()에 의해 버튼의 종류가 이미 설정되어 있는 경우
		//이 create()함수에서는 초기화시키고 다시 설정해야 한다.
		if (n > 0)
		{
			m_button.clear();
			m_button.resize(n);
		}

		m_top = top + 1;

		if (right <= 0)
		{
			CRect r;
			parent->GetClientRect(r);
			right = r.right;
		}

		m_right = right;

		if (width > 0)
			m_button_width = width;
		if (height > 0)
			m_button_height = height;

		//컨트롤 생성과 함께 각 버튼의 위치가 정해진다.
		for (int i = 0; i < m_button.size(); i++)
		{
			m_button[i].cmd = arg[i];
			m_button[i].r = CRect(i * (m_button_width + m_gap), m_top, i * (m_button_width + m_gap) + m_button_width, m_button_height);
		}

		Create(_T("CSCSystemButtons"), WS_CHILD,
			CRect(m_right - m_button.size() * m_button_width - (m_button.size() - 1) * m_gap, m_top, m_right, m_button_height), parent, 0);

		ShowWindow(SW_SHOW);
	}

	//원하는 시스템 버튼들을 추가한다. 최소 0개에서 최대 4개까지 구현되어 있다.
	//추가하고자 하는 버튼이 있다면 define하고 해당 이미지를 그려주는 코드 추가하고 해당 액션에 대한 코드를 추가하면 된다.
	//set_buttons_cmd(SC_PIN, SC_MINIMIZE, SC_MAXIMIZE, SC_CLOSE);
	//버튼의 총 개수가 변하면 각 버튼의 위치 또한 변경해줘야 한다.
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

	//특정 위치에 새로운 명령 버튼을 추가한다.
	void insert_button(int idx, int cmd)
	{
		//idx가 -1이면 맨 끝에 버튼을 추가한다.
		if (idx == -1)
			m_button.push_back(CSCSystemButtonProperty(cmd));
		else
			m_button.insert(m_button.begin() + idx, CSCSystemButtonProperty(cmd));

		resize();
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

	void	resize();
	void	adjust(int top, int right);

	CSCColorTheme	m_theme = CSCColorTheme(this);
	void	set_color_theme(int theme);
	void	set_text_color(Gdiplus::Color cr_text) { m_theme.cr_text = cr_text; }
	void	set_back_color(Gdiplus::Color cr_back) { m_theme.cr_back = cr_back; }
	void	set_back_hover_color(Gdiplus::Color cr_back_hover) { m_theme.cr_back_hover = cr_back_hover; }
	int		get_button_width() { return m_button_width; }
	void	set_button_width(int width);
	int		get_button_height() { return m_button_height; }
	void	set_button_height(int height);

protected:

	std::deque<CSCSystemButtonProperty> m_button;
	int		m_button_width = 44;
	int		m_button_height = 32;
	int		m_top = 1;
	int		m_right;				//버튼 컨트롤의 우측 좌표에 맞춰 n개의 버튼 위치가 결정된다.
	int		m_gap = 0;				//버튼 사이 간격

	bool	m_mouse_hover = false;	//마우스가 컨트롤에 들어왔는지
	bool	m_mouse_track = false;
	int		m_over_index = -1;		//마우스 오버 또는 다운 인덱스
	bool	m_down_state = false;

	int		get_button_index(CPoint pt);

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
	virtual BOOL PreTranslateMessage(MSG* pMsg);
};


