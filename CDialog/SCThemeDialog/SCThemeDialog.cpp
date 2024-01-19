// SCThemeDialog.cpp: 구현 파일
//

#include "SCThemeDialog.h"

#include "../../Functions.h"
#include "../../MemoryDC.h"

#pragma comment(lib, "Dwmapi.lib")

// CSCThemeDialog 대화 상자

IMPLEMENT_DYNAMIC(CSCThemeDialog, CDialogEx)

CSCThemeDialog::CSCThemeDialog(UINT nResourceID, CWnd* pParent /*=nullptr*/)
	: CDialogEx(nResourceID, pParent)
{

}

CSCThemeDialog::~CSCThemeDialog()
{
}

void CSCThemeDialog::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}


BEGIN_MESSAGE_MAP(CSCThemeDialog, CDialogEx)
	ON_WM_ERASEBKGND()
	ON_WM_LBUTTONDOWN()
	ON_WM_NCHITTEST()
	ON_WM_NCACTIVATE()
	ON_WM_NCCALCSIZE()
	ON_WM_PAINT()
	ON_WM_SIZE()
END_MESSAGE_MAP()


// CSCThemeDialog 메시지 처리기
BOOL CSCThemeDialog::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	//find border thickness
	m_is_resizable = GetWindowLongPtr(m_hWnd, GWL_STYLE) & WS_THICKFRAME;
	if (m_is_resizable)
	{
		AdjustWindowRectEx(&m_border_thickness, GetWindowLongPtr(m_hWnd, GWL_STYLE) & ~WS_CAPTION, FALSE, NULL);
		m_border_thickness.left *= -1;
		m_border_thickness.top *= -1;
	}
	else if (GetWindowLongPtr(m_hWnd, GWL_STYLE) & WS_BORDER)
	{
		SetRect(&m_border_thickness, 1, 1, 1, 1);
	}

	MARGINS margins = { 0 };
	DwmExtendFrameIntoClientArea(m_hWnd, &margins);

	if (m_titlebar_height > 0)
	{
		CRect rc;
		GetClientRect(rc);

		//m_sys_buttons.create(this, rc.right, -1, m_titlebar_height, SC_PIN, SC_MINIMIZE, SC_MAXIMIZE, SC_CLOSE);
		//m_sys_buttons.create(this, rc.right, m_titlebar_height, 44, SC_CLOSE);
		m_system_buttons.create(this);
		m_system_buttons.set_buttons_cmd(SC_CLOSE);
	}

	m_font = GetFont();
	if (m_font == NULL)
		m_font = AfxGetMainWnd()->GetFont();

	return TRUE;  // return TRUE unless you set the focus to a control
				  // 예외: OCX 속성 페이지는 FALSE를 반환해야 합니다.
}

INT_PTR CSCThemeDialog::DoModal()
{
	//m_isModal = TRUE;
	return CDialogEx::DoModal();
}


BOOL CSCThemeDialog::OnEraseBkgnd(CDC* pDC)
{
	// TODO: 여기에 메시지 처리기 코드를 추가 및/또는 기본값을 호출합니다.

	return FALSE;
	//return CDialogEx::OnEraseBkgnd(pDC);
}


void CSCThemeDialog::OnLButtonDown(UINT nFlags, CPoint point)
{
	// TODO: 여기에 메시지 처리기 코드를 추가 및/또는 기본값을 호출합니다.
	DefWindowProc(WM_NCLBUTTONDOWN, HTCAPTION, MAKEWORD(point.x, point.y));

	CDialogEx::OnLButtonDown(nFlags, point);
}

/*
//타이틀바 show/hide
void CSCThemeDialog::show_default_titlebar(bool titlebar)
{
	if (titlebar)
		ModifyStyle(0, WS_CAPTION, SWP_FRAMECHANGED);
	else
		ModifyStyle(WS_CAPTION, 0, SWP_FRAMECHANGED);
}
*/

void CSCThemeDialog::set_titlebar_height(int height)
{

}

void CSCThemeDialog::enable_resize(bool resizable)
{
	m_is_resizable = resizable;

	if (resizable)
	{
		ModifyStyle(0, WS_THICKFRAME, SWP_FRAMECHANGED);

		//find border thickness
		if (GetWindowLongPtr(m_hWnd, GWL_STYLE) & WS_THICKFRAME)
		{
			AdjustWindowRectEx(&m_border_thickness, GetWindowLongPtr(m_hWnd, GWL_STYLE) & ~WS_CAPTION, FALSE, NULL);
			m_border_thickness.left *= -1;
			m_border_thickness.top *= -1;
		}
		else if (GetWindowLongPtr(m_hWnd, GWL_STYLE) & WS_BORDER)
		{
			SetRect(&m_border_thickness, 1, 1, 1, 1);
		}

		MARGINS margins = { 0 };
		DwmExtendFrameIntoClientArea(m_hWnd, &margins);
	}
	else
	{
		ModifyStyle(WS_THICKFRAME, 0, SWP_FRAMECHANGED);
	}
}


LRESULT CSCThemeDialog::OnNcHitTest(CPoint point)
{
	// TODO: 여기에 메시지 처리기 코드를 추가 및/또는 기본값을 호출합니다.
	if (!m_is_resizable)
	{
		return CDialogEx::OnNcHitTest(point);
	}

	DWORD win_ver = get_windows_major_version();
	LRESULT result;

	if (win_ver >= 10)
	{
		result = CDialogEx::OnNcHitTest(point);

		if (result == HTCLIENT)
		{
			ScreenToClient(&point);
			if (point.y < m_border_thickness.top)
				return HTTOP;
		}
	}
	else
	{
		CRect rc;

		GetClientRect(rc);

		enum { left = 1, top = 2, right = 4, bottom = 8 };

		int hit = 0;

		if (point.x < m_border_thickness.left) hit |= left;
		if (point.x > rc.right - m_border_thickness.right) hit |= right;
		if (point.y < m_border_thickness.top) hit |= top;
		if (point.y > rc.bottom - m_border_thickness.bottom) hit |= bottom;

		if (hit & top && hit & left) return HTTOPLEFT;
		if (hit & top && hit & right) return HTTOPRIGHT;
		if (hit & bottom && hit & left) return HTBOTTOMLEFT;
		if (hit & bottom && hit & right) return HTBOTTOMRIGHT;
		if (hit & left) return HTLEFT;
		if (hit & top) return HTTOP;
		if (hit & right) return HTRIGHT;
		if (hit & bottom) return HTBOTTOM;

		return HTCLIENT;
	}

	return result;
}


BOOL CSCThemeDialog::OnNcActivate(BOOL bActive)
{
	// TODO: 여기에 메시지 처리기 코드를 추가 및/또는 기본값을 호출합니다.
	if (m_is_resizable)
	{
		//return을 FALSE로 하게 되면 메시지박스가 떴을 때 액션이 동작되지 않는다.
		//또한 여기서 Invalidate()을 해주지 않으면 상단 잔상이 생긴다.
		Invalidate();
		//RedrawWindow();
		return TRUE;// FALSE;
	}

	return CDialogEx::OnNcActivate(bActive);
}


void CSCThemeDialog::OnNcCalcSize(BOOL bCalcValidRects, NCCALCSIZE_PARAMS* lpncsp)
{
	// TODO: 여기에 메시지 처리기 코드를 추가 및/또는 기본값을 호출합니다.
	if (m_is_resizable)
	{
		DWORD win_ver = get_windows_major_version();

		if (win_ver < 10)
			return;

		if (lpncsp)
		{
			lpncsp->rgrc[0].left += m_border_thickness.left;
			lpncsp->rgrc[0].right -= m_border_thickness.right;
			lpncsp->rgrc[0].bottom -= m_border_thickness.bottom;
			return;
		}
	}

	CDialogEx::OnNcCalcSize(bCalcValidRects, lpncsp);
}


void CSCThemeDialog::OnPaint()
{
	//아래의 기본 그리기 코드들을 OnEraseBkgnd()에 넣었었으나
	//deactive상태가 될 때 컨트롤들이 제대로 표시되지 않는 현상이 있었고
	//OnPaint()에 넣으니 정상 동작함.
	CPaintDC dc1(this);
	CRect rc;
	GetClientRect(rc);

	CMemoryDC dc(&dc1, &rc, false);

	Gdiplus::Graphics g(dc.GetSafeHdc());
	g.SetSmoothingMode(Gdiplus::SmoothingModeAntiAlias);
	g.SetTextRenderingHint(Gdiplus::TextRenderingHintAntiAlias);

	//타이틀바 칠하기
	if (m_titlebar_height > 0)
	{
		CRect rTitle = rc;
		rTitle.bottom = m_titlebar_height;
		dc.FillSolidRect(rTitle, m_cr_titlebar_back);

		//프로그램 아이콘 표시
		if (m_img_logo.is_valid())//icon exist?
		{
			m_img_logo.draw(&g, 0, 0);
		}
		else
		{
			//윈도우 기본 타이틀바에 표시되는 아이콘은 16x16이고 32x32 영역에 그려진다.
			HICON hIcon = load_icon(AfxGetInstanceHandle(), 128, 16, 16);
			draw_icon(&dc, hIcon, CRect(0, 0, m_titlebar_height, m_titlebar_height));
			rTitle.left += m_titlebar_height;
		}

		//rTitle.left += 4;

		dc.SetBkMode(TRANSPARENT);
		dc.SetTextColor(m_cr_titlebar_text);

		CFont* pOldFont = (CFont*)dc.SelectObject(m_font);
		dc.DrawText(m_title, rTitle, DT_LEFT | DT_SINGLELINE | DT_VCENTER);
		dc.SelectObject(pOldFont);

		//타이틀바 아래의 client영역의 시작.
		rc.top = rTitle.bottom;

		//m_sys_buttons.adjust_right(rc.right);
		//m_sys_buttons.draw(pDC);
	}

	//클라이언트 영역 칠하기
	if (m_img_back.is_valid())
	{
		m_img_back.draw(&g, rc, true);
	}
	else
	{
		dc.FillSolidRect(rc, m_cr_back);
	}
}

void CSCThemeDialog::set_color_theme(int theme)
{
	m_system_buttons.set_color_theme(theme);

	switch (theme)
	{
	case color_theme_window :
		m_cr_titlebar_text = ::GetSysColor(COLOR_CAPTIONTEXT);
		m_cr_titlebar_back = ::GetSysColor(COLOR_ACTIVECAPTION);
		m_cr_back = ::GetSysColor(COLOR_3DFACE);
		break;
	case color_theme_visualstudio:
		m_cr_titlebar_text = RGB(192, 192, 192);
		m_cr_titlebar_back = RGB(31, 31, 31);
		m_cr_back = RGB(54, 54, 54);
		break;
	case color_theme_gray:
		m_cr_titlebar_text = RGB(192, 192, 192);
		m_cr_titlebar_back = RGB(31, 31, 31);
		m_cr_back = ::GetSysColor(COLOR_3DFACE);
		break;
	}
}


void CSCThemeDialog::OnSize(UINT nType, int cx, int cy)
{
	CDialogEx::OnSize(nType, cx, cy);

	// TODO: Add your message handler code here
	if (!m_system_buttons.m_hWnd)
		return;

	m_system_buttons.adjust_right(cx);
	Invalidate();
}

void CSCThemeDialog::set_back_image(CString imgType, UINT nResourceID, bool stretch)
{
	m_img_back.load(imgType, nResourceID);
	Invalidate();
}
/*
void CSCThemeDialog::set_back_image(CString img_path, bool stretch)
{
	m_img_back.load(img_path);
	Invalidate();
}
*/