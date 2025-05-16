// SCSystemButtons.cpp : implementation file
//

#include "SCSystemButtons.h"
#include "../../Functions.h"
#include "../../MemoryDC.h"

// CSCSystemButtons

IMPLEMENT_DYNAMIC(CSCSystemButtons, CButton)

CSCSystemButtons::CSCSystemButtons()
{
	//Ư���� ������ ���ٸ� �⺻ 3���� �ý��� ��ư���� �����Ѵ�.
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
END_MESSAGE_MAP()



// CSCSystemButtons message handlers

void CSCSystemButtons::create(CWnd* parent, int top, int right_end, int width, int height)
{
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

	//��Ʈ�� ������ �Բ� �� ��ư�� ��ġ�� ��������.
	for (int i = 0; i < m_button.size(); i++)
	{
		m_button[i].r = CRect(i * (m_button_width + m_gap), m_top, i * (m_button_width + m_gap) + m_button_width, m_button_height);
	}

	Create(_T("CSCSystemButtons"), WS_CHILD | BS_PUSHBUTTON,
		CRect(m_right - m_button.size() * m_button_width - (m_button.size() - 1) * m_gap, m_top, m_right, m_button_height), parent, 0);

	ShowWindow(SW_SHOW);
	//Invalidate();
	//RedrawWindow();
	//UpdateWindow();
}

//SC_MAXIMIZE�� ���� Ư�� ����� ���ԵǾ� �ִ��� �Ǻ�
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
	for (int i = 0; i < m_button.size(); i++)
	{
		m_button[i].r = CRect(i * (m_button_width + m_gap), m_top, i * (m_button_width + m_gap) + m_button_width, m_button_height);
	}

	MoveWindow(m_right - 1 - m_button.size() * m_button_width - (m_button.size() - 1) * m_gap,
		m_top,
		m_button.size() * m_button_width - (m_button.size() - 1) * m_gap,
		m_button_height);

	//MoveWindow()�� ȣ���ϴ� Invalidate()�� ���ʿ��ϰ� �������� MoveWindow()�� ������ ũ���� repaint�� ���� �ʴ� �� �ϴ�.
	//parent dlg�� resize�� �� CSCSystemButtons�� ��ġ�� ����Ǵµ� �� �� ũ��� ������� �����Ƿ� MoveWindow()���� repaint�� ���� �ʴ� �� �ϴ�.
	//���� ������ Invalidate()�� ȣ���Ѵ�.
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

	dc.FillSolidRect(rc, m_theme.cr_back.ToCOLORREF());

	for (size_t i = 0; i < m_button.size(); i++)
	{
		if (i == m_over_index)
		{
			if (m_down_state)
				dc.FillSolidRect(m_button[i].r, m_button[i].cmd == SC_CLOSE ? RGB(232, 17, 35) : m_theme.cr_sys_buttons_down_back.ToCOLORREF());
			else
				dc.FillSolidRect(m_button[i].r, m_button[i].cmd == SC_CLOSE ? RGB(232, 17, 35) : m_theme.cr_sys_buttons_hover_back.ToCOLORREF());
		}

		//�� ��ư�� �׷��ش�.
		Gdiplus::Graphics g(dc.GetSafeHdc());
		g.SetSmoothingMode(Gdiplus::SmoothingModeAntiAlias);

		if (m_button[i].img.is_valid())
		{
			m_button[i].img.draw(g, m_button[i].r);
		}
		else
		{
			Gdiplus::Pen pen(m_theme.cr_text, 1.5F);
			Gdiplus::Pen pen_pin(Gdiplus::Color(45, 122, 190), 17.0F);
			Gdiplus::SolidBrush br_back(m_theme.cr_back);
			Gdiplus::SolidBrush br_pin(Gdiplus::Color(255, 255, 255, 255));

			pen_pin.SetStartCap(Gdiplus::LineCapRound);
			pen_pin.SetEndCap(Gdiplus::LineCapRound);

			CPoint cp = m_button[i].r.CenterPoint();

			if (m_button[i].cmd == SC_MINIMIZE)
			{
				g.DrawLine(&pen, cp.x - 5, cp.y + 3, cp.x + 5, cp.y + 3);
			}
			else if (m_button[i].cmd == SC_MAXIMIZE)
			{
				if (GetParent()->IsZoomed())
				{
					g.DrawRectangle(&pen, cp.x - 3, cp.y - 5, 8, 8);
					g.FillRectangle(&br_back, cp.x - 6, cp.y - 2, 8, 8);
					g.DrawRectangle(&pen, cp.x - 6, cp.y - 2, 8, 8);
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
				g.DrawLine(&pen_pin, cp.x - 7, cp.y, cp.x + 7, cp.y);
				g.FillEllipse(&br_pin, cp.x + (is_top_most(GetParent()->GetSafeHwnd()) ? 1 : -11), cp.y - 5, 10, 10);
			}
			else if (m_button[i].cmd == SC_HELP)
			{
				draw_text(g, m_button[i].r, _T("?"), 12, 0, 1, 1.0f, _T("���� ���"), m_theme.cr_text);// , m_theme.cr_text, DT_CENTER | DT_VCENTER | DT_SINGELINE);
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
	TRACE(_T("m_over_index = %d\n"), m_over_index);
	m_down_state = false;
	//RedrawWindow();
	Invalidate();

	if (m_over_index >= 0)
	{
		switch (m_button[m_over_index].cmd)
		{
		case SC_HELP:
			//AfxMessageBox(_T("���� �Ǵ� CAboutDlg ǥ�� ����"));
			::PostMessage(GetParent()->GetSafeHwnd(), WM_SYSCOMMAND, SC_HELP, 0);
			break;
		case SC_PIN:
			if (is_top_most(GetParent()->GetSafeHwnd()))
				GetParent()->SetWindowPos(&wndNoTopMost, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE);
			else
				GetParent()->SetWindowPos(&wndTopMost, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE);
			break;
		case SC_MINIMIZE:
		case SC_CLOSE:
			::PostMessage(GetParent()->GetSafeHwnd(), WM_SYSCOMMAND, m_button[m_over_index].cmd, 0);
			break;
		case SC_MAXIMIZE:
			if (GetParent()->IsZoomed())
				::PostMessage(GetParent()->GetSafeHwnd(), WM_SYSCOMMAND, SC_RESTORE, 0);
			else
				::PostMessage(GetParent()->GetSafeHwnd(), WM_SYSCOMMAND, SC_MAXIMIZE, 0);
		}
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

void CSCSystemButtons::set_color_theme(int theme)
{
	m_theme.set_color_theme(theme);
	/*
	switch (theme)
	{
	case color_theme_window:
		m_cr_back = ::GetSysColor(COLOR_3DFACE);
		break;
	case color_theme_visualstudio:
		m_cr_back = RGB(31, 31, 31);
		m_cr_over = RGB(61, 61, 61);
		m_cr_down = RGB(56, 56, 56);
		m_cr_pen = RGB(192, 192, 192);
		break;
	case color_theme_gray:
		m_cr_back = ::GetSysColor(COLOR_3DFACE);
		break;
	}
	*/
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
	// TODO: ���⿡ Ư��ȭ�� �ڵ带 �߰� ��/�Ǵ� �⺻ Ŭ������ ȣ���մϴ�.
	if (pMsg->message == WM_KEYDOWN ||
		pMsg->message == WM_KEYUP ||
		pMsg->message == WM_NCHITTEST)
	{
		TRACE(_T("msg(%d) on CSCSystemButtons\n"), pMsg->message);
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
	resize();
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
