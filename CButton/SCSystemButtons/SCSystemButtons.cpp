// SCSystemButtons.cpp : implementation file
//

#include "SCSystemButtons.h"
#include "../../Functions.h"
#include "../../MemoryDC.h"

// CSCSystemButtons

IMPLEMENT_DYNAMIC(CSCSystemButtons, CButton)

CSCSystemButtons::CSCSystemButtons()
{
	//특별한 세팅이 없다면 기본 3개의 시스템 버튼으로 시작한다.
	m_button.resize(1);
	m_button[0].cmd = SC_CLOSE;

	set_color_theme(CSCColorTheme::color_theme_default);
}

CSCSystemButtons::~CSCSystemButtons()
{
}


BEGIN_MESSAGE_MAP(CSCSystemButtons, CButton)
	ON_WM_PAINT()
	ON_WM_ERASEBKGND()
	ON_WM_LBUTTONDOWN()
	ON_WM_LBUTTONUP()
	ON_WM_MOUSEMOVE()
	ON_WM_MOUSEHOVER()
	ON_WM_MOUSELEAVE()
	ON_WM_MOUSEACTIVATE()
	ON_WM_SETFOCUS()
END_MESSAGE_MAP()

//타이틀바 시스템 버튼은 클릭해도 keyboard focus 를 가져가서는 안 됨 (Alt 키가 단축키로 쓰이고 Alt 누를 때
//focus 컨트롤에 focus rect 가 그려지는 OS 동작 회피). MA_NOACTIVATE = 클릭은 정상 처리하지만 focus 이동 안 됨.
int CSCSystemButtons::OnMouseActivate(CWnd* pDesktopWnd, UINT nHitTest, UINT message)
{
	return MA_NOACTIVATE;
}

//MA_NOACTIVATE 는 마우스 클릭 경로만 차단. dialog init / 외부 SetFocus() 호출 등으로 focus 가 도달하면
//그대로 받아들임 → Alt 키 누를 때 OS 의 keyboard cues 가 focus rect 그림. OnSetFocus 에서 즉시 parent 로 회수.
void CSCSystemButtons::OnSetFocus(CWnd* pOldWnd)
{
	CWnd* parent = GetParent();
	if (parent && parent->GetSafeHwnd() && parent->m_hWnd != m_hWnd)
		parent->SetFocus();
}



// CSCSystemButtons message handlers

void CSCSystemButtons::create(CWnd* parent, int top, int right_end, int width, int height)
{
	//m_parent = parent;

	m_button.clear();

	m_top = top + 1;

	if (right_end > 0)
	{
		m_right = right_end - 1;
	}
	else
	{
		CRect parentrc;
		parent->GetClientRect(parentrc);
		m_right = parentrc.right;
	}

	if (width > 0)
		m_button_width = width;
	if (height > 0)
		m_button_height = height;

	//컨트롤 생성과 함께 각 버튼의 위치가 정해진다.
	for (int i = 0; i < m_button.size(); i++)
	{
		//박스 bottom 을 타이틀바 bottom 보다 ~2px 위로. strip 이 top 에서 m_top 만큼 내려와 상단 여백이
		//생기는데 bottom 이 m_button_height 꽉 차면 비대칭(상단 ~2px / 하단 0). (m_button_height - m_top) 가
		//strip-client 좌표에서의 타이틀바 bottom 이고, 거기서 2px 더 빼 상/하 여백을 맞춘다.
		m_button[i].r = CRect(i * (m_button_width + m_gap), m_top, i * (m_button_width + m_gap) + m_button_width, m_button_height - m_top - 2);
	}

	Create(_T("CSCSystemButtons"), WS_CHILD | BS_PUSHBUTTON,
		CRect(m_right - m_button.size() * m_button_width - (m_button.size() - 1) * m_gap, m_top, m_right, m_button_height), parent, 0);

	ShowWindow(SW_SHOW);
	//Invalidate();
	//RedrawWindow();
	//UpdateWindow();
}

//SC_MAXIMIZE와 같이 특정 명령이 포함되어 있는지 판별
bool CSCSystemButtons::has_button(int cmd)
{
	for (int i = 0; i < m_button.size(); i++)
	{
		if (m_button[i].cmd == cmd)
			return true;
	}

	return false;
}

void CSCSystemButtons::resize()
{
	//폭은 height 에서 파생 — toolbar 면 거의 정사각, 아니면 캡션 비율(44:32). 레이아웃하는 이 한 곳에서 매번
	//계산하므로 create/set_button_height/set_as_toolbar 등 어떤 경로·순서로 와도 폭이 항상 일관된다.
	//(폭을 setter 마다 따로 저장하면 호출 순서에 따라 stale 값으로 덮이는 문제가 있었음.)
	m_button_width = m_toolbar_mode
		? m_button_height
		: (int)(m_button_height * (DEFAULT_SYSTEM_BUTTON_WIDTH / 32.0f) + 0.5f);

	for (int i = 0; i < m_button.size(); i++)
	{
		//박스 bottom 을 타이틀바 bottom 보다 ~2px 위로. strip 이 top 에서 m_top 만큼 내려와 상단 여백이
		//생기는데 bottom 이 m_button_height 꽉 차면 비대칭(상단 ~2px / 하단 0). (m_button_height - m_top) 가
		//strip-client 좌표에서의 타이틀바 bottom 이고, 거기서 2px 더 빼 상/하 여백을 맞춘다.
		m_button[i].r = CRect(i * (m_button_width + m_gap), m_top, i * (m_button_width + m_gap) + m_button_width, m_button_height - m_top - 2);

		//toolbar 모드면 버튼 박스를 각 변 toolbar_inset px 작게 — 호버 fill·히트·글리프 기준이 모두 이 rect 라 한 번에 작아진다.
		//★ 버튼 크기 미세조정은 이 toolbar_inset 한 값만 바꾸면 됨.
		if (m_toolbar_mode)
		{
			const int toolbar_inset = 2;
			m_button[i].r.DeflateRect(toolbar_inset, toolbar_inset);
		}
	}

	MoveWindow(m_right - 1 - m_button.size() * m_button_width - (m_button.size() - 1) * m_gap,
		m_top,
		m_button.size() * m_button_width - (m_button.size() - 1) * m_gap,
		m_button_height);

	//MoveWindow()를 호출하니 Invalidate()이 불필요하게 보이지만 MoveWindow()는 동일한 크기라면 repaint를 하지 않는 듯 하다.
	//parent dlg가 resize될 때 CSCSystemButtons의 위치가 변경되는데 이 때 크기는 변경되지 않으므로 MoveWindow()에서 repaint를 하지 않는 듯 하다.
	//따라서 강제로 Invalidate()을 호출한다.
	Invalidate();
}

void CSCSystemButtons::adjust(int top, int right)
{
	CRect rc;
	GetClientRect(rc);

	m_top = top + 1;
	m_right = right - 1;
	resize();
	//MoveWindow(m_right - rc.Width(), m_top, rc.Width(), rc.Height());
}

int CSCSystemButtons::get_button_index(CPoint pt)
{
	for (int i = 0; i < m_button.size(); i++)
	{
		if (m_button[i].r.PtInRect(pt))
		{
			return i;
		}
	}

	return -1;
}

void CSCSystemButtons::OnPaint()
{
	CPaintDC dc1(this); // device context for painting
	// TODO: Add your message handler code here
	// Do not call CButton::OnPaint() for painting messages
	CRect rc;
	GetClientRect(rc);

	CMemoryDC dc(&dc1, &rc);
	Gdiplus::Color cr_back = m_theme.cr_title_back_active;
	//if (m_is_activated)
		dc.FillSolidRect(rc, cr_back.ToCOLORREF());
	//else
	//	dc.FillSolidRect(rc, m_theme.cr_title_back_inactive.ToCOLORREF());

	for (size_t i = 0; i < m_button.size(); i++)
	{
		//SC_PIN 은 toggle 상태 (always-on-top on/off) 가 self-evident 한 색 변화로 표시되므로
		//hover 배경 강조 불필요. 다른 명령들 (MIN/MAX/CLOSE) 만 hover 효과 적용.
		if (i == m_over_index && m_button[i].cmd != SC_PIN)
		{
			//if (m_down_state)
			//	cr_back = (m_button[i].cmd == SC_CLOSE ? gRGB(232, 17, 35) : m_theme.cr_sys_buttons_down_back);
			//else
				cr_back = (m_button[i].cmd == SC_CLOSE ? gRGB(232, 17, 35) : m_theme.cr_sys_buttons_hover_back);

			dc.FillSolidRect(m_button[i].r, cr_back.ToCOLORREF());
		}

		//각 버튼을 그려준다.
		Gdiplus::Graphics g(dc.GetSafeHdc());
		g.SetSmoothingMode(Gdiplus::SmoothingModeAntiAlias);

		if (m_button[i].img.is_valid())
		{
			m_button[i].img.draw(g, m_button[i].r);
		}
		else
		{
			Gdiplus::Pen pen(m_theme.cr_title_text, 1.0f);
			Gdiplus::Pen pen_pin(Gdiplus::Color(45, 122, 190), 17.0f);
			Gdiplus::Pen pen_pin_gray(get_weak_color(m_theme.cr_title_back_inactive, 48), 17.0f);
			Gdiplus::SolidBrush br_back(m_theme.cr_back);
			Gdiplus::SolidBrush br_pin(Gdiplus::Color(255, 255, 255, 255));

			pen_pin.SetStartCap(Gdiplus::LineCapRound);
			pen_pin.SetEndCap(Gdiplus::LineCapRound);
			pen_pin_gray.SetStartCap(Gdiplus::LineCapRound);
			pen_pin_gray.SetEndCap(Gdiplus::LineCapRound);

			CPoint cp = m_button[i].r.CenterPoint();

			if (m_button[i].cmd == SC_MINIMIZE)
			{
				g.DrawLine(&pen, cp.x - 5, cp.y + 3, cp.x + 5, cp.y + 3);
			}
			//대상 윈도우가 zoomed인지 아닌지에 따라 그려지는 모양이 다른데
			//그렇다고 이 클래스에서 대상 윈도우 포인터를 직접 가지게 하면 번거로워진다.
			//대상 윈도우는 parent일수도 있고 parent의 parent일수도 있기 때문이다.
			//(CASeeDlg에서 CTitleDlg를 가지고 있고 그 CTitleDlg가 CSCThemeDlg를 상속받은 클래스 일 경우는 두번의 전달이 필요해진다)
			//1.대상 윈도우가 zoomed인지 아닌지도 물어 물어 확인해서 그리거나
			//2.확인하는 콜백함수를 여기에 정의하고 그릴 때 콜백함수를 호출해서 확인하거나
			//3.메인에서 maximized / restore 할 때 버튼 모양을 다시 그릴 수 있게 이 클래스의 설정함수를 호출하는 방식이 있다.
			//우선 3번 방식으로 구현한다.
			else if (m_button[i].cmd == SC_RESTORE)
			{
				CRect r = make_rect(cp.x - 3, cp.y - 6, 9, 9);
				draw_rect(g, r, m_theme.cr_title_text, cr_back);
				r.OffsetRect(-3, 3);
				draw_rect(g, r, m_theme.cr_title_text, cr_back);
			}
			else if (m_button[i].cmd == SC_MAXIMIZE)
			{
				if (m_parent_maximized)
				{
					CRect r = make_rect(cp.x - 3, cp.y - 6, 9, 9);
					draw_rect(g, r, m_theme.cr_title_text, cr_back);
					r.OffsetRect(-3, 3);
					draw_rect(g, r, m_theme.cr_title_text, cr_back);
				}
				else
				{
					g.DrawRectangle(&pen, cp.x - 5, cp.y - 6, 10, 10);
				}
			}
			else if (m_button[i].cmd == SC_CLOSE)
			{
				g.DrawLine(&pen, cp.x - 5, cp.y - 6, cp.x + 5, cp.y + 4);
				g.DrawLine(&pen, cp.x - 5, cp.y + 4, cp.x + 5, cp.y - 6);
			}
			else if (m_button[i].cmd == SC_PIN)
			{
				//SC_PIN 의 always-on-top 상태는 *최상위 top-level window* 의 WS_EX_TOPMOST 로 판정.
				//SystemButtons 가 TitleDlg 같은 중간 child dialog 안에 있으면 GetParent() 는 그
				//child 의 hwnd 라 EX_TOPMOST 가 자동 sync 안 됨 (메인만 TOPMOST 되어도 child 는 그대로).
				//GetAncestor(GA_ROOT) 로 메인 dlg 의 hwnd 직접 얻어 일관 판정.
				HWND hwnd_root = ::GetAncestor(GetSafeHwnd(), GA_ROOT);
				bool on_top = is_top_most(hwnd_root);

				//세로 중앙 보정 — 일반창 모드에서만 1px 위로 보정. 전체화면 / maximized 에선
				//보정 없이 원래 정상 위치. m_parent_maximized 는 호출자 (TitleDlg::set_floating_mode
				//등) 가 명시적으로 갱신 — IsZoomed 자체 체크는 self-styled fullscreen 미감지라 신뢰 X.
				int y_off = m_parent_maximized ? 0 : -1;
				g.DrawLine(on_top ? &pen_pin : &pen_pin_gray,
					cp.x - 7, cp.y + y_off, cp.x + 7, cp.y + y_off);
				g.FillEllipse(&br_pin,
					cp.x + (on_top ? 1 : -11), cp.y + y_off - 5, 10, 10);
			}
			else if (m_button[i].cmd == SC_HELP)
			{
				draw_text(g, m_button[i].r, _T("?"), 12, 0, 1, 1.0f, _T("맑은 고딕"), m_theme.cr_text);// , m_theme.cr_text, DT_CENTER | DT_VCENTER | DT_SINGELINE);
			}
		}
	}
}


BOOL CSCSystemButtons::OnEraseBkgnd(CDC* pDC)
{
	// TODO: Add your message handler code here and/or call default
	return FALSE;
	return CButton::OnEraseBkgnd(pDC);
}


void CSCSystemButtons::OnLButtonDown(UINT nFlags, CPoint point)
{
	// TODO: Add your message handler code here and/or call default
	m_down_state = (get_button_index(point) >= 0);
	//RedrawWindow();
	Invalidate();

	CButton::OnLButtonDown(nFlags, point);
}


void CSCSystemButtons::OnLButtonUp(UINT nFlags, CPoint point)
{
	// TODO: Add your message handler code here and/or call default
	m_over_index = get_button_index(point);
	//TRACE(_T("m_over_index = %d\n"), m_over_index);
	m_down_state = false;
	//RedrawWindow();
	Invalidate();

	CPoint pt;
	GetCursorPos(&pt);

	if (m_over_index >= 0)
	{
		//해당 명령은 무조건 parent로 보낸다. parent든, parent의 parent든 실제 해당 명령을 처리해야 할 윈도우까지 전달되어야 한다.
		//CASeeDlg에서는 최종 명령을 CASeeDlg에서 처리해야 하므로 CSCSystemButtons -> CTitleDlg -> CASeeDlg로 전달되어야 한다.
		//즉, 중간에 있는 CTitleDlg는 이 메시지를 직접 처리해야 하는 주체가 아니므로 이를 parent인 CASeeDlg로 전달해야 한다.
		//WM_SYSCOMMAND로 보냈었으나 이 시스템 메시지는 부가적인 처리까지 동반되어 원래 의도했던 명령이 제대로 전달되지 않는 문제가 있었다.
		//따라서 사용자 정의 메시지로 보내고 각 메시지에 맞는 처리를 하도록 수정함.
		//parent에서는 메시지 핸들러를 추가하여 Message_CSCSystemButtons 메시지를 처리하는 코드가 추가되어야 한다.
		CSCSystemButtonsMessage msg(this, m_button[m_over_index].cmd);
		CWnd* parent = GetParent();
		if (parent && parent->GetSafeHwnd())
			parent->SendMessage(Message_CSCSystemButtons, (WPARAM)&msg, 0);
	}

	CButton::OnLButtonUp(nFlags, point);
}


void CSCSystemButtons::OnMouseMove(UINT nFlags, CPoint point)
{
	// TODO: Add your message handler code here and/or call default
	if (!m_mouse_track)
	{
		TRACKMOUSEEVENT tme;
		tme.cbSize = sizeof(tme);
		tme.hwndTrack = m_hWnd;
		tme.dwFlags = TME_LEAVE;
		tme.dwHoverTime = 0;
		m_mouse_hover = true;
		m_mouse_track = _TrackMouseEvent(&tme);
		_TrackMouseEvent(&tme);
	}

	m_over_index = get_button_index(point);
	
	Invalidate();
	//RedrawWindow();
	//UpdateWindow();

	CButton::OnMouseMove(nFlags, point);
}

void CSCSystemButtons::set_color_theme(int theme, bool invalidate)
{
	m_theme.set_color_theme(theme);
	if (invalidate && m_hWnd)
		Invalidate();
}

void CSCSystemButtons::set_color_theme(const CSCColorTheme& theme, bool invalidate)
{
	m_theme.copy_colors_from(theme);
	if (invalidate && m_hWnd)
		Invalidate();
}


void CSCSystemButtons::OnMouseHover(UINT nFlags, CPoint point)
{
	// TODO: Add your message handler code here and/or call default
	m_mouse_hover = true;
	Trace(_T("\n"));
	CButton::OnMouseHover(nFlags, point);
}


void CSCSystemButtons::OnMouseLeave()
{
	// TODO: Add your message handler code here and/or call default
	/*
	TRACKMOUSEEVENT MouseEvent;
	::ZeroMemory(&MouseEvent, sizeof(MouseEvent));
	MouseEvent.cbSize = sizeof(MouseEvent);
	MouseEvent.dwFlags = TME_CANCEL;
	MouseEvent.hwndTrack = m_hWnd;

	::_TrackMouseEvent(&MouseEvent);
	*/
	m_mouse_track = false;
	m_over_index = -1;
	RedrawWindow();
	Trace(_T("\n"));
	CButton::OnMouseLeave();
}


BOOL CSCSystemButtons::PreTranslateMessage(MSG* pMsg)
{
	// TODO: 여기에 특수화된 코드를 추가 및/또는 기본 클래스를 호출합니다.
	if (pMsg->message == WM_KEYDOWN ||
		pMsg->message == WM_KEYUP ||
		pMsg->message == WM_NCHITTEST)
	{
		//TRACE(_T("msg(%d) on CSCSystemButtons\n"), pMsg->message);
		return FALSE;
	}

	return CButton::PreTranslateMessage(pMsg);
}

void CSCSystemButtons::set_button_width(int width)
{
	m_button_width = width;
	resize();
}

void CSCSystemButtons::set_button_height(int height)
{
	m_button_height = height - 1;
	resize();		//폭은 resize() 가 height(+toolbar 모드)에서 파생.
	/*
	return;
	for (int i = 0; i < m_button.size(); i++)
	{
		m_button[i].r.bottom = m_button[i].r.top + m_button_height;
	}

	CRect rc;
	GetClientRect(rc);

	SetWindowPos(NULL, 0, 0, rc.Width(), m_button_height, SWP_NOMOVE | SWP_NOZORDER);

	//CRect rc;

	//GetRect(rc);
	//ScreenToClient(rc);
	//rc.bottom = rc.top + height;
	//MoveWindow(rc);
	*/
}

void CSCSystemButtons::set_as_toolbar(bool toolbar)
{
	m_toolbar_mode = toolbar;
	if (m_hWnd)
		resize();		//폭은 resize() 가 mode+height 에서 파생.
}
