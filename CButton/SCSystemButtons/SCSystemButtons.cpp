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
	m_button.resize(3);
	m_button[0].cmd = SC_MINIMIZE;
	m_button[1].cmd = SC_MAXIMIZE;
	m_button[2].cmd = SC_CLOSE;

	set_color_theme(color_theme_window);
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
/*
void CSCSystemButtons::create(CWnd* parent, int right_end, int height, int width)
{
	//컨트롤 생성과 함께 각 버튼의 위치가 정해진다.
	for (int i = 0; i < m_button.size(); i++)
	{
		m_button[i].r = CRect(i * (width + m_gap), 1, i * (width + m_gap) + width, height - 1);
	}

	Create(_T("CSCSystemButtons"), WS_CHILD | BS_PUSHBUTTON,
		CRect(right_end - m_button.size() * width - (m_button.size() - 1) * m_gap, 0, right_end, height), parent, 0);

	//Invalidate();
	//RedrawWindow();
	//UpdateWindow();
}
*/

void CSCSystemButtons::adjust_right(int right_end)
{
	CRect rc;
	GetClientRect(rc);

	MoveWindow(right_end - rc.Width(), rc.top, rc.Width(), rc.Height());
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

	dc.FillSolidRect(rc, m_cr_back);

	for (size_t i = 0; i < m_button.size(); i++)
	{
		if (i == m_over_index)
		{
			if (m_down_state)
				dc.FillSolidRect(m_button[i].r, m_button[i].cmd == SC_CLOSE ? RGB(232, 17, 35) : m_cr_down);
			else
				dc.FillSolidRect(m_button[i].r, m_button[i].cmd == SC_CLOSE ? RGB(232, 17, 35) : m_cr_over);
		}

		//각 버튼을 그려준다.
		Gdiplus::Graphics g(dc.GetSafeHdc());
		g.SetSmoothingMode(Gdiplus::SmoothingModeAntiAlias);

		if (m_button[i].img.valid())
		{
			m_button[i].img.draw(&g, m_button[i].r);
		}
		else
		{
			Gdiplus::Color cr, cr_back;
			cr.SetFromCOLORREF(m_cr_pen);
			cr_back.SetFromCOLORREF(m_cr_back);

			Gdiplus::Pen gp(cr, 1.5F);
			Gdiplus::Pen gpTrack(Gdiplus::Color(155, 188, 54), 14.0F);
			Gdiplus::SolidBrush br_back(cr_back);
			Gdiplus::SolidBrush br_pin(Gdiplus::Color(0, 0, 255));

			gpTrack.SetStartCap(Gdiplus::LineCapRound);
			gpTrack.SetEndCap(Gdiplus::LineCapRound);

			CPoint cp = m_button[i].r.CenterPoint();

			if (m_button[i].cmd == SC_MINIMIZE)
			{
				g.DrawLine(&gp, cp.x - 5, cp.y + 1, cp.x + 5, cp.y + 1);
			}
			else if (m_button[i].cmd == SC_MAXIMIZE)
			{
				if (GetParent()->IsZoomed())
				{
					g.DrawRectangle(&gp, cp.x - 3, cp.y - 4, 8, 8);
					g.FillRectangle(&br_back, cp.x - 6, cp.y - 1, 8, 8);
					g.DrawRectangle(&gp, cp.x - 6, cp.y - 1, 8, 8);
				}
				else
				{
					g.DrawRectangle(&gp, cp.x - 5, cp.y - 5, 10, 10);
				}
			}
			else if (m_button[i].cmd == SC_CLOSE)
			{
				g.DrawLine(&gp, cp.x - 5, cp.y - 4, cp.x + 5, cp.y + 6);
				g.DrawLine(&gp, cp.x - 5, cp.y + 6, cp.x + 5, cp.y - 4);
			}
			else if (m_button[i].cmd == SC_PIN)
			{
				//DWORD dwExStyle = ::
				g.DrawLine(&gpTrack, cp.x - 7, cp.y + 1, cp.x + 7, cp.y + 1);
				g.FillEllipse(&br_pin, cp.x + (is_top_most(GetParent()->GetSafeHwnd()) ? 1 : -11), cp.y + 1 - 5, 10, 10);
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
