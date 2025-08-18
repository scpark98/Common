// SCThemeDlg.cpp: 구현 파일

#include "SCThemeDlg.h"

#include "../../Functions.h"
#include "../../MemoryDC.h"

#pragma comment(lib, "Dwmapi.lib")

// CSCThemeDlg 대화 상자

IMPLEMENT_DYNAMIC(CSCThemeDlg, CDialogEx)

CSCThemeDlg::CSCThemeDlg(CWnd* parent, int left, int top, int right, int bottom)
{
	create(parent, left, top, right, bottom);
}

CSCThemeDlg::CSCThemeDlg(UINT nResourceID, CWnd* pParent /*=nullptr*/)
	: CDialogEx(nResourceID, pParent)
{
}
/*
CSCThemeDlg::CSCThemeDlg(CString message, CString headline)
{

}
*/
CSCThemeDlg::~CSCThemeDlg()
{
}

void CSCThemeDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}


BEGIN_MESSAGE_MAP(CSCThemeDlg, CDialogEx)
	ON_WM_ERASEBKGND()
	ON_WM_LBUTTONDOWN()
	ON_WM_NCHITTEST()
	ON_WM_NCACTIVATE()
	ON_WM_NCCALCSIZE()
	ON_WM_PAINT()
	ON_WM_SIZE()
	ON_WM_LBUTTONDBLCLK()
	ON_WM_CTLCOLOR()
	ON_WM_SYSCOMMAND()
	ON_WM_TIMER()
	ON_WM_KILLFOCUS()
	//ON_WM_ACTIVATE()
	ON_WM_WINDOWPOSCHANGED()
	ON_WM_GETMINMAXINFO()
	ON_WM_SETCURSOR()
END_MESSAGE_MAP()

bool CSCThemeDlg::create(CWnd* parent, int left, int top, int right, int bottom)
{
	m_parent = parent;

	DWORD dwStyle = WS_POPUP;

	WNDCLASS wc = {};
	::GetClassInfo(AfxGetInstanceHandle(), _T("#32770"), &wc);
	wc.lpszClassName = _T("CSCThemeDlg");
	AfxRegisterClass(&wc);

	if (right - left < 40)
		right = left + 320;

	if (bottom - top < 40)
		bottom = top + 200;

	bool res = CreateEx(NULL, wc.lpszClassName, _T("CSCThemeDlg"), dwStyle, CRect(left, top, right, bottom), parent, 0);

	//TRACE(_T("create. rect = (%d,%d) (%d,%d)\n"), left, top, right, bottom);
	CRect rc;
	GetClientRect(rc);
	//TRACE(_T("rc = %s\n"), get_rect_info_string(rc));

	dwStyle = GetWindowLongPtr(m_hWnd, GWL_STYLE);
	dwStyle &= ~(WS_CAPTION);
	SetWindowLongPtr(m_hWnd, GWL_STYLE, dwStyle);

	//dwStyle = GetWindowLong(m_hWnd, GWL_EXSTYLE) | WS_EX_LAYERED;
	//SetWindowLongPtr(m_hWnd, GWL_EXSTYLE, dwStyle);

	GetClientRect(rc);
	TRACE(_T("rc = %s\n"), get_rect_info_string(rc));

	if (left == 0 && top == 0)
		CenterWindow(m_parent);

	ShowWindow(SW_SHOW);
	return res;
}

// CSCThemeDlg 메시지 처리기
BOOL CSCThemeDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	//기본 타이틀바 제거
	//DWORD dwStyle = GetWindowLongPtr(m_hWnd, GWL_STYLE);
	//dwStyle &= ~(WS_CAPTION);
	//SetWindowLongPtr(m_hWnd, GWL_STYLE, dwStyle);

	//find border thickness
	m_has_thickframe = GetWindowLongPtr(m_hWnd, GWL_STYLE) & WS_THICKFRAME;
	if (m_has_thickframe)
	{
		AdjustWindowRectEx(&m_border_thickness, GetWindowLongPtr(m_hWnd, GWL_STYLE), FALSE, NULL);
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
		m_sys_buttons.create(this);
		m_sys_buttons.set_color_theme(m_theme.get_color_theme());
		m_sys_buttons.set_buttons_cmd(SC_CLOSE);
		//m_sys_buttons.set_text_color(m_cr_titlebar_text);
		//m_sys_buttons.set_back_color(m_cr_titlebar_back);
		//m_sys_buttons.set_button_height(m_titlebar_height);
	}

	CFont* font = GetFont();

	if (font != NULL)
		font->GetObject(sizeof(m_titlebar_lf), &m_titlebar_lf);
	else
		GetObject(GetStockObject(SYSTEM_FONT), sizeof(m_titlebar_lf), &m_titlebar_lf);

	reconstruct_titlebar_font();

	set_color_theme(CSCColorTheme::color_theme_default);
	//init_shadow();

	return TRUE;  // return TRUE unless you set the focus to a control
				  // 예외: OCX 속성 페이지는 FALSE를 반환해야 합니다.
}

void CSCThemeDlg::init_shadow()
{
	CWndShadow::Initialize(AfxGetInstanceHandle());
	m_shadow.Create(GetSafeHwnd());
	m_shadow.SetSize(8);	// -19 ~ 19
	m_shadow.SetSharpness(19);	// 0 ~ 19
	m_shadow.SetDarkness(200);	// 0 ~ 254
	m_shadow.SetPosition(0, 0);	// -19 ~ 19
	m_shadow.SetColor(RGB(0, 0, 0));
}

void CSCThemeDlg::set_use_shadow(bool use_shadow)
{
	m_shadow.SetSize(use_shadow ? 8 : 0);
}

void CSCThemeDlg::reconstruct_titlebar_font()
{
	m_titlebar_font.DeleteObject();
	m_titlebar_font.CreateFontIndirect(&m_titlebar_lf);
	Invalidate();
}

void CSCThemeDlg::set_titlebar_font_name(LPCTSTR sFontname, BYTE byCharSet)
{
	m_titlebar_lf.lfCharSet = byCharSet;
	_tcscpy_s(m_titlebar_lf.lfFaceName, _countof(m_titlebar_lf.lfFaceName), sFontname);
	reconstruct_titlebar_font();
}

void CSCThemeDlg::set_titlebar_icon(UINT icon_id, int cx, int cy)
{
	m_hIcon = load_icon(NULL, icon_id, cx, cy);
}

void CSCThemeDlg::set_titlebar_icon(HICON hIcon)
{
	m_hIcon = hIcon;
}

void CSCThemeDlg::set_titlebar_font_size(int size)
{
	m_titlebar_lf.lfHeight = -MulDiv(size, GetDeviceCaps(::GetDC(GetParent()->GetSafeHwnd()), LOGPIXELSY), 72);
	reconstruct_titlebar_font();
}

void CSCThemeDlg::set_titlebar_bold(bool bold)
{
	m_titlebar_lf.lfWeight = (bold ? FW_BOLD : FW_NORMAL);
	reconstruct_titlebar_font();
}

INT_PTR CSCThemeDlg::DoModal()
{
	return CDialogEx::DoModal();
}


BOOL CSCThemeDlg::OnEraseBkgnd(CDC* pDC)
{
	// TODO: 여기에 메시지 처리기 코드를 추가 및/또는 기본값을 호출합니다.
	return TRUE;
}


void CSCThemeDlg::OnLButtonDown(UINT nFlags, CPoint point)
{
	// TODO: 여기에 메시지 처리기 코드를 추가 및/또는 기본값을 호출합니다.
	if (m_titlebar_movable && point.y < m_titlebar_height)
		DefWindowProc(WM_NCLBUTTONDOWN, HTCAPTION, MAKEWORD(point.x, point.y));
	else if (!m_use_resizable)
		return;

	CDialogEx::OnLButtonDown(nFlags, point);
}

/*
//타이틀바 show/hide
void CSCThemeDlg::show_default_titlebar(bool titlebar)
{
	if (titlebar)
		ModifyStyle(0, WS_CAPTION, SWP_FRAMECHANGED);
	else
		ModifyStyle(WS_CAPTION, 0, SWP_FRAMECHANGED);
}
*/

void CSCThemeDlg::set_titlebar_height(int height)
{
	m_titlebar_height = height;
	m_sys_buttons.set_button_height(height);
	Invalidate();
}

void CSCThemeDlg::enable_resize(bool resizable)
{
	m_has_thickframe = resizable;

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


LRESULT CSCThemeDlg::OnNcHitTest(CPoint point)
{
	// TODO: 여기에 메시지 처리기 코드를 추가 및/또는 기본값을 호출합니다.
	CPoint pt;
	GetCursorPos(&pt);
	ScreenToClient(&pt);
	//TRACE(_T("pt.y = %d\n"), pt.y);

	//resize하지 않는 dlg라도 타이틀바는 반응해야 한다.
	if (!m_use_resizable && (pt.y < 7 || pt.y > m_titlebar_height))
		return -1;

	if (!m_has_thickframe)
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


BOOL CSCThemeDlg::OnNcActivate(BOOL bActive)
{
	// TODO: 여기에 메시지 처리기 코드를 추가 및/또는 기본값을 호출합니다.
	//if (m_is_resizable)
	{
		//return을 FALSE로 하게 되면 메시지박스가 떴을 때 액션이 동작되지 않는다.
		//또한 여기서 Invalidate()을 해주지 않으면 상단 잔상이 생긴다.
		Invalidate();
		RedrawWindow();
		return TRUE;// FALSE;
	}

	return CDialogEx::OnNcActivate(bActive);
}


void CSCThemeDlg::OnNcCalcSize(BOOL bCalcValidRects, NCCALCSIZE_PARAMS* lpncsp)
{
	// TODO: 여기에 메시지 처리기 코드를 추가 및/또는 기본값을 호출합니다.
	if (m_has_thickframe)
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


void CSCThemeDlg::OnPaint()
{
	//아래의 기본 그리기 코드들을 OnEraseBkgnd()에 넣었었으나
	//deactive상태가 될 때 컨트롤들이 제대로 표시되지 않는 현상이 있었고
	//OnPaint()에 넣으니 정상 동작함.
	CPaintDC dc1(this);
	CRect rc;
	GetClientRect(rc);

	CMemoryDC dc(&dc1, &rc);

	Gdiplus::Graphics g(dc.GetSafeHdc());
	g.SetSmoothingMode(Gdiplus::SmoothingModeAntiAlias);
	g.SetTextRenderingHint(Gdiplus::TextRenderingHintAntiAlias);

	//dc.FillSolidRect(rc, m_cr_back.ToCOLORREF());

	//타이틀바 영역
	if (m_titlebar_height > 0)
	{
		CRect rTitle = rc;
		rTitle.bottom = m_titlebar_height;
		//dc.FillSolidRect(rTitle, m_cr_titlebar_back.ToCOLORREF());
		dc.FillSolidRect(rTitle, m_theme.cr_title_back.ToCOLORREF());

		//프로그램 아이콘 표시. 
		if (m_show_logo)
		{
			//png logo 이미지가 설정되어 있으면 우선하여 그리고
			if (m_img_logo.is_valid())//
			{
				m_img_logo.draw(g, 0, 0);
			}
			//없다면 main에서 set_title_icon()으로 설정한 아이콘을 그려준다.
			else if (m_hIcon)//icon exist?
			{
				//윈도우 기본 타이틀바에 표시되는 아이콘은 16x16이고 32x32 영역에 그려진다.
				draw_icon(&dc, m_hIcon, CRect(0, 0, m_titlebar_height, m_titlebar_height));
				rTitle.left += m_titlebar_height;
			}
			else
			{
				
			}
		}
		else
		{
			rTitle.left += 6;
		}


		//타이틀 출력
		dc.SetBkMode(TRANSPARENT);
		dc.SetTextColor(m_theme.cr_title_text.ToCOLORREF());

		CString title;
		GetWindowText(title);

		CFont* pOldFont = (CFont*)dc.SelectObject(&m_titlebar_font);
		dc.DrawText(title, rTitle, DT_LEFT | DT_SINGLELINE | DT_VCENTER);
		dc.SelectObject(pOldFont);
	}

	/*
	//클라이언트 영역 칠하기
	CRect rclient = rc;
	rclient.top += m_titlebar_height;
	if (m_img_back.is_valid())
	{
		if (m_img_back_draw_mode == CSCGdiplusBitmap::draw_mode_zoom ||
			m_img_back_draw_mode == CSCGdiplusBitmap::draw_mode_origin)
		{
			dc.FillSolidRect(rclient, m_cr_out_of_back_img.ToCOLORREF());
		}

		m_img_back.draw(g, rclient, m_img_back_draw_mode);
	}
	else
	{
		dc.FillSolidRect(rclient, m_cr_back.ToCOLORREF());
	}
	*/

	//border
	if (m_draw_border && m_border_width > 0)
	{
		GetClientRect(rc);
		draw_rectangle(&dc, rc, m_theme.cr_border, Gdiplus::Color::Transparent, m_border_width);
	}
}


void CSCThemeDlg::set_color_theme(int theme)
{
	m_theme.set_color_theme(theme);
	m_sys_buttons.set_color_theme(theme);
	//m_sys_buttons.set_button_height(m_titlebar_height);

	//m_theme.set_color_theme(theme);에서는 cr_text, cr_back 등 일반적인 색상값들을 세팅한다.
	//CSCThemeDlg::set_color_theme(int theme) 에서는 m_sys_buttons 등
	//CSCThemeDlg를 상속받은 클래스에서 필요로하는 부가적인 세팅이 필요하다.
	switch (theme)
	{
	case CSCColorTheme::color_theme_linkmemine :
		SetWindowText(_T("LinkMeMine"));

		m_sys_buttons.set_text_color(m_theme.cr_title_text);

		//m_cr_sys_buttons_back_hover = get_color(m_cr_titlebar_back, 30);
		m_sys_buttons.set_back_color(m_theme.cr_title_back);
		m_sys_buttons.set_back_hover_color(m_theme.cr_sys_buttons_hover_back);
		m_sys_buttons.set_button_height(m_titlebar_height - 2);

		m_titlebar_lf.lfWeight = (m_titlebar_bold ? FW_BOLD : FW_NORMAL);
		m_titlebar_lf.lfHeight = get_pixel_size_from_font_size(m_hWnd, 10);
		reconstruct_titlebar_font();
		break;
	case CSCColorTheme::color_theme_linkmemine_se:
		SetWindowText(_T("LinkMeMine 3.0 SE"));

		m_sys_buttons.set_text_color(m_theme.cr_title_text);

		m_sys_buttons.set_back_color(m_theme.cr_title_back);
		m_sys_buttons.set_back_hover_color(m_theme.cr_sys_buttons_hover_back);
		m_sys_buttons.set_button_height(m_titlebar_height - 2);

		m_titlebar_lf.lfWeight = (m_titlebar_bold ? FW_BOLD : FW_NORMAL);
		m_titlebar_lf.lfHeight = get_pixel_size_from_font_size(m_hWnd, 10);
		reconstruct_titlebar_font();
		break;
	case CSCColorTheme::color_theme_helpu:
		SetWindowText(_T("HelpU"));

		m_sys_buttons.set_text_color(m_theme.cr_title_text);

		m_sys_buttons.set_back_color(m_theme.cr_title_back);
		m_sys_buttons.set_back_hover_color(m_theme.cr_sys_buttons_hover_back);
		m_sys_buttons.set_button_height(m_titlebar_height - 2);

		m_titlebar_lf.lfWeight = (m_titlebar_bold ? FW_BOLD : FW_NORMAL);
		m_titlebar_lf.lfHeight = get_pixel_size_from_font_size(m_hWnd, 10);
		reconstruct_titlebar_font();
		break;
	case CSCColorTheme::color_theme_anysupport:
		SetWindowText(_T("AnySupport"));

		m_sys_buttons.set_text_color(m_theme.cr_title_text);

		m_sys_buttons.set_back_color(m_theme.cr_title_back);
		m_sys_buttons.set_back_hover_color(m_theme.cr_sys_buttons_hover_back);
		m_sys_buttons.set_button_height(m_titlebar_height - 2);

		m_titlebar_lf.lfWeight = (m_titlebar_bold ? FW_BOLD : FW_NORMAL);
		m_titlebar_lf.lfHeight = get_pixel_size_from_font_size(m_hWnd, 10);
		reconstruct_titlebar_font();
		break;
	case CSCColorTheme::color_theme_pcanypro:
		SetWindowText(_T("PCAnyPro"));

		m_sys_buttons.set_text_color(m_theme.cr_title_text);

		m_sys_buttons.set_back_color(m_theme.cr_title_back);
		m_sys_buttons.set_back_hover_color(m_theme.cr_sys_buttons_hover_back);
		m_sys_buttons.set_button_height(m_titlebar_height - 2);

		m_titlebar_lf.lfWeight = (m_titlebar_bold ? FW_BOLD : FW_NORMAL);
		m_titlebar_lf.lfHeight = get_pixel_size_from_font_size(m_hWnd, 10);
		reconstruct_titlebar_font();
		break;
	//case CSCColorTheme::color_theme_default:
	//	m_sys_buttons.set_text_color(m_theme.cr_title_text);

	//	//m_sys_buttons.set_back_color(m_theme.cr_title_back);
	//	//m_sys_buttons.set_back_hover_color(m_theme.cr_sys_buttons_hover_back);
	//	//m_sys_buttons.set_button_height(m_titlebar_height - 2);

	//	m_titlebar_lf.lfWeight = (m_titlebar_bold ? FW_BOLD : FW_NORMAL);
	//	m_titlebar_lf.lfHeight = get_pixel_size_from_font_size(m_hWnd, 10);
	//	reconstruct_titlebar_font();
	//	break;
	}
}

//CSCThemeDlg를 상속받은 클래스가 크기 변경 가능한 dlg라면
//그 클래스의 OnSize()에서 기본 핸들러 호출을
//CDialogEx::OnSize(nType, cx, cy); 가 아닌
//CSCThemeDlg::OnSize(nType, cx, cy); 로 호출해줘야 한다.
//만약 dlg가 resize를 지원하지 않거나 OnSize()에서 별도의 처리가 불필요한 경우는
//아예 WM_SIZE에 대한 이벤트 핸들러를 추가할 필요가 없다.
void CSCThemeDlg::OnSize(UINT nType, int cx, int cy)
{
	CDialogEx::OnSize(nType, cx, cy);

	// TODO: Add your message handler code here
	if (!m_sys_buttons.m_hWnd)
		return;

	m_sys_buttons.adjust(0, cx);
	Invalidate();
}

void CSCThemeDlg::set_titlebar_text_color(Gdiplus::Color cr)
{
	m_theme.cr_title_text = cr;
	m_sys_buttons.set_text_color(cr);
}

void CSCThemeDlg::set_titlebar_back_color(Gdiplus::Color cr)
{
	m_theme.cr_title_back = cr;
	m_sys_buttons.set_back_color(cr);
}

//parent창이 resize 될 때 호출해줘야만 m_sys_buttons가 위치를 바로잡는다.
void CSCThemeDlg::adjust_sys_buttons()
{
	if (m_sys_buttons.m_hWnd == NULL)
		return;

	CRect r;
	GetClientRect(r);
	//TRACE(_T("CSCThemeDlg::adjust_sys_buttons(). r.w = %d, r.h = %d\n"), r.Width(), r.Height());
	m_sys_buttons.adjust(r.top, r.right);
}


BOOL CSCThemeDlg::PreTranslateMessage(MSG* pMsg)
{
	// TODO: 여기에 특수화된 코드를 추가 및/또는 기본 클래스를 호출합니다.
	if (pMsg->message == WM_KEYDOWN)
	{
		TRACE(_T("keydown on CSCThemeDlg\n"));
		return FALSE;
	}

	return CDialogEx::PreTranslateMessage(pMsg);
}


void CSCThemeDlg::OnLButtonDblClk(UINT nFlags, CPoint point)
{
	// TODO: 여기에 메시지 처리기 코드를 추가 및/또는 기본값을 호출합니다.
	if (m_sys_buttons.has_button(SC_MAXIMIZE) && (point.y < m_titlebar_height))
	{
		//프로그램 아이콘을 더블클릭한 경우
		if (point.x < 32)
		{
			PostMessage(WM_SYSCOMMAND, SC_CLOSE);
			return;
		}

		if (IsZoomed())
		{
			ShowWindow(SW_RESTORE);
		}
		else
		{
			ShowWindow(SW_MAXIMIZE);
		}

		m_sys_buttons.Invalidate();
	}

	CDialogEx::OnLButtonDblClk(nFlags, point);
}

void CSCThemeDlg::set_back_image(CString imgType, UINT nResourceID, int draw_mode)
{
	m_img_back.load(imgType, nResourceID);
	m_img_back_draw_mode = draw_mode;
	m_cr_out_of_back_img = m_img_back.get_color(0, 0);
}

void CSCThemeDlg::set_back_image(CString img_path, int draw_mode)
{
	m_img_back.load(img_path);
	m_img_back_draw_mode = draw_mode;
	m_cr_out_of_back_img = m_img_back.get_color(0, 0);
}

void CSCThemeDlg::set_back_image(UINT resource_id, int draw_mode)
{
	m_img_back.load(resource_id);
	m_img_back_draw_mode = draw_mode;
	m_cr_out_of_back_img = m_img_back.get_color(0, 0);
}

HBRUSH CSCThemeDlg::OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor)
{
	HBRUSH hbr = CDialogEx::OnCtlColor(pDC, pWnd, nCtlColor);

	// TODO:  여기서 DC의 특성을 변경합니다.
	//if (nCtlColor == CTLCOLOR_STATIC)
	//{
	//	pDC->SetBkMode(TRANSPARENT); /// 배경을 투명하게
	//	return (HBRUSH)::GetStockObject(NULL_BRUSH);
	//}

	//hbr = (HBRUSH)::GetStockObject(NULL_BRUSH);
	//return (HBRUSH)GetStockObject(WHITE_BRUSH);


	// TODO:  기본값이 적당하지 않으면 다른 브러시를 반환합니다.
	return hbr;
}


void CSCThemeDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
	// TODO: 여기에 메시지 처리기 코드를 추가 및/또는 기본값을 호출합니다.
	TRACE(_T("CSCThemeDlg::OnSysCommand. nID = %d, lParam = %ld\n"), nID, lParam);
	CDialogEx::OnSysCommand(nID, lParam);

	//UINT id = (nID & 0xFFF0);
	//if ((nID & 0xFFF0) == SC_MOVE)
	//	m_sys_buttons.Invalidate();
	//	SetTimer(0, 1000, NULL);
}

/*
void CSCThemeDlg::OnActivate(UINT nState, CWnd* pWndOther, BOOL bMinimized)
{
	CDialogEx::OnActivate(nState, pWndOther, bMinimized);

	// TODO: 여기에 메시지 처리기 코드를 추가합니다.

	//nFTDServer 프로젝트에서 deactivate되면 타이틀바 영역의
	if (nState == 0)
		//::SetTimer(m_hWnd, timer_refresh_title_area, 10, NULL);
		SetTimer(timer_refresh_title_area, 10, NULL);
	//Wait(1000);

	//Invalidate();
	//RedrawWindow();
}
*/

//base class인 이 클래스에서 SetTimer()를 호출해도 OnTimer는 derived class의 OnTimer가 먼저 호출된다.
//deactive될 때 타이틀바 영역이 깨지는 현상때문에 refresh하도록 main에 코드를 추가함.
void CSCThemeDlg::OnTimer(UINT_PTR nIDEvent)
{
	// TODO: 여기에 메시지 처리기 코드를 추가 및/또는 기본값을 호출합니다.
	//if (nIDEvent == timer_refresh_title_area)
	//{
	//	TRACE(_T("timer_refresh_title_area\n"));
	//}
	CDialogEx::OnTimer(nIDEvent);
}

void CSCThemeDlg::OnKillFocus(CWnd* pNewWnd)
{
	CDialogEx::OnKillFocus(pNewWnd);

	// TODO: 여기에 메시지 처리기 코드를 추가합니다.
	//Invalidate();
	//RedrawWindow();
}


void CSCThemeDlg::OnWindowPosChanged(WINDOWPOS* lpwndpos)
{
	CDialogEx::OnWindowPosChanged(lpwndpos);

	// TODO: 여기에 메시지 처리기 코드를 추가합니다.
	if (!m_sys_buttons.m_hWnd)
		return;

	//parent가 RestoreWindowPosition(), SetWindowPos() 등에 의해 크기 변경이 되면
	//sys_buttons가 parent의 right align을 하게 되는데
	//parent가 크기 변경 이벤트가 없는 앱이라면 right align되지 않은 상태로 시작된다.
	//이를 해결하기 위해 여기서 adjust_sys_buttons()를 호출해준다.
	adjust_sys_buttons();

	//parent 창이 움직이거나 가려질 때 간혹 상단에 흰색 영역이 표시되어
	//타이틀바 그래픽이 깨질때가 있어서 여기서 Invalidate()을 호출했었으나 
	//Invalidate();
}

//For top - level windows, this value is based on(...) the primary monitor.
void CSCThemeDlg::OnGetMinMaxInfo(MINMAXINFO* lpMMI)
{
	// TODO: 여기에 메시지 처리기 코드를 추가 및/또는 기본값을 호출합니다.
	CSize sz;
	bool is_shown_taskbar = get_taskbar_size(&sz);

	CPoint pt;
	GetCursorPos(&pt);
	int index = get_monitor_index(pt.x, pt.y);
	//TRACE(_T("index = %d, r = %s\n"), index, get_rect_info_string(g_monitors[index].rMonitor));
	lpMMI->ptMaxPosition.y = 0;

	//TRACE(_T("before ptMaxSize.x = %ld, ptMaxSize.y = %ld, ptMaxTrackSize.x = %ld, ptMaxTrackSize.y = %ld\n"), lpMMI->ptMaxSize.x, lpMMI->ptMaxSize.y, lpMMI->ptMaxTrackSize.x, lpMMI->ptMaxTrackSize.y);

	//GetMonitorInfo()
	//lpMMI값은 항상 기본 모니터를 기준으로 넘어오므로
	//다른 모니터에서 SW_MAXIMIZE할 때 크기가 기본 모니터를 기준으로 동작하여 넘치거나 잘리는 현상이 있었다.
	//따라서 현재 모니터 인덱스를 구하고 그 영역 정보로 lpMMI를 다시 채워줘야 한다.
	//단, ptMaxSize와 실제 모니터의 크기가 차이가 발생했는데 scpark 모니터에서 테스트 한 경우 그 차이값이 14가 발생했고
	//g_monitors의 0번이 기본 모니터이므로 그 차이값 만큼 보정해줘야 한다.
	CSize sz_diff = CSize(lpMMI->ptMaxSize.x - g_monitors[0].rMonitor.Width(), lpMMI->ptMaxSize.y - g_monitors[0].rMonitor.Height());

	lpMMI->ptMaxSize.x = g_monitors[index].rMonitor.Width() +sz_diff.cx;
	lpMMI->ptMaxSize.y = g_monitors[index].rMonitor.Height() +sz_diff.cy;
	//lpMMI->ptMaxSize.y -= (sz_diff.cy + 1 + (is_shown_taskbar ? sz.cy : 0));
	lpMMI->ptMaxSize.y -= (2 + (is_shown_taskbar ? sz.cy : 0));

	//TRACE(_T("after  ptMaxSize.x = %ld, ptMaxSize.y = %ld, ptMaxTrackSize.x = %ld, ptMaxTrackSize.y = %ld\n"), lpMMI->ptMaxSize.x, lpMMI->ptMaxSize.y, lpMMI->ptMaxTrackSize.x, lpMMI->ptMaxTrackSize.y);

	CDialogEx::OnGetMinMaxInfo(lpMMI);
}

BOOL CSCThemeDlg::OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message)
{
	// TODO: 여기에 메시지 처리기 코드를 추가 및/또는 기본값을 호출합니다.
	if (!m_use_resizable)
	{
		return TRUE;
	}
	return CDialogEx::OnSetCursor(pWnd, nHitTest, message);
}

void CSCThemeDlg::set_draw_border(bool draw, int width, Gdiplus::Color cr)
{
	m_draw_border = draw;
	m_border_width = width;
	m_theme.cr_border = cr;
	Invalidate();
	RedrawWindow();
}