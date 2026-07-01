// SCThemeDlg.cpp: 구현 파일

#include "SCThemeDlg.h"

#include "../../Functions.h"
#include "../../MemoryDC.h"
#include "../../win_compat/dwm.h"

//Dwmapi.lib 정적 링크는 XP에서 dwmapi.dll 부재로 로드 실패. win_compat::dwm 가 LoadLibrary 로 안전 호출하므로 정적 링크 제거.
//#pragma comment(lib, "Dwmapi.lib")

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
	//Borderless customized dlg (borderless dialog.md Step 3).
	//WM_NCHITTEST 는 WindowProc 에서 직접 분기하므로 ON_WM_NCHITTEST 등록 안 함.
	ON_WM_NCACTIVATE()
	ON_WM_NCCALCSIZE()
	ON_WM_NCPAINT()
	ON_WM_PAINT()
	ON_WM_SIZE()
	ON_WM_LBUTTONDBLCLK()
	ON_WM_CTLCOLOR()
	ON_WM_SYSCOMMAND()
	ON_WM_TIMER()
	ON_WM_KILLFOCUS()
	ON_WM_WINDOWPOSCHANGED()
	ON_WM_GETMINMAXINFO()
	ON_WM_ACTIVATE()
	ON_WM_SETFOCUS()
END_MESSAGE_MAP()

bool CSCThemeDlg::create(CWnd* parent, int left, int top, int right, int bottom)
{
	//m_parent = parent;

	LONG_PTR dwStyle = WS_POPUP | WS_VISIBLE | WS_TABSTOP;

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
	TRACE(_T("rc = %s\n"), get_rect_info_str(rc));

	if (left == 0 && top == 0)
		CenterWindow(parent);

	//ShowWindow(SW_SHOW);
	return res;
}

// CSCThemeDlg 메시지 처리기
BOOL CSCThemeDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	//[client-기준 자동 레이아웃 Step 0] style 변경 전(= template NC 상태)의 client 크기 캡처.
	//apply_client_titlebar_layout 가 이 값을 기준으로 런타임 client 폭/높이를 복원·예약한다.
	CRect rc_template_client;
	GetClientRect(rc_template_client);

	//Borderless customized dlg 변환 (borderless dialog.md Step 4).
	//resource 의 caption/border 속성 어느 조합이든 *항상 동일 상태* 로 정규화한다.
	//WS_CAPTION/WS_BORDER/WS_DLGFRAME 전부 제거 → 필요 시 WS_THICKFRAME 재부여.
	//SWP_FRAMECHANGED 가 핵심 — NC 영역 재계산을 OS 에 알린다. 없으면 style 변경이 반영 안 됨.
	{
		LONG_PTR style = ::GetWindowLongPtr(m_hWnd, GWL_STYLE);
		style &= ~(WS_CAPTION | WS_THICKFRAME | WS_BORDER | WS_DLGFRAME);
		if (m_use_resizable)
			style |= WS_THICKFRAME;
		style |= (WS_SYSMENU | WS_MINIMIZEBOX | WS_MAXIMIZEBOX);
		::SetWindowLongPtr(m_hWnd, GWL_STYLE, style);
		::SetWindowPos(m_hWnd, nullptr, 0, 0, 0, 0,
			SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE | SWP_FRAMECHANGED);
	}

	//정규화된 style 로부터 border thickness 산출 — resize hit-test fallback path 에서 참조.
	m_has_thickframe = (::GetWindowLongPtr(m_hWnd, GWL_STYLE) & WS_THICKFRAME) != 0;
	if (m_has_thickframe)
	{
		AdjustWindowRectEx(&m_border_thickness, ::GetWindowLongPtr(m_hWnd, GWL_STYLE), FALSE, NULL);
		m_border_thickness.left *= -1;
		m_border_thickness.top *= -1;
	}
	else
	{
		SetRect(&m_border_thickness, 1, 1, 1, 1);
	}

	if (m_titlebar_height > 0)
	{
		m_sys_buttons.create(this);
		//시스템 버튼 높이를 타이틀바 높이에 동기화. (예전엔 derived 의 set_titlebar_height() 가 맞춰줬으나,
		// m_titlebar_height 를 ctor 에서 확정하는 경로에선 create 직후 여기서 동기화해야 버튼이 기본값 32 로 남지 않음.)
		m_sys_buttons.set_button_height(m_titlebar_height);
		m_sys_buttons.set_color_theme(m_theme);
		m_sys_buttons.set_buttons_cmd(SC_CLOSE);
	}

	CFont const* font = GetFont();

	if (font != NULL)
		font->GetObject(sizeof(m_titlebar_lf), &m_titlebar_lf);
	else
		GetObject(GetStockObject(SYSTEM_FONT), sizeof(m_titlebar_lf), &m_titlebar_lf);

	reconstruct_titlebar_font();

	//[client-기준 자동 레이아웃] NC drift 로 커진 client 복원(여백 비대칭 fix) + 타이틀바 자리 자동 예약.
	//m_titlebar_height 가 ctor 에서 확정돼 있어야 정확히 예약된다 (OnInitDialog 뒤에 set_titlebar_height 하면 늦음).
	if (m_auto_client_layout)
		apply_client_titlebar_layout(rc_template_client);

	set_color_theme(CSCColorTheme::color_theme_default);
	//init_shadow();

	return TRUE;  // return TRUE unless you set the focus to a control
				  // 예외: OCX 속성 페이지는 FALSE를 반환해야 합니다.
}

/*
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
*/

void CSCThemeDlg::reconstruct_titlebar_font()
{
	m_titlebar_font.DeleteObject();
	m_titlebar_font.CreateFontIndirect(&m_titlebar_lf);
	Invalidate();
}

//개발자가 리소스에서 *순수 client(top=0) 기준* 으로만 컨트롤을 배치할 수 있게 하는 런타임 보정.
//호출 시점: borderless style 정규화 + m_sys_buttons 생성 직후, 파생 OnInitDialog (CResizeCtrl 등) 가 돌기 전.
//   (1) 목표 client = (template 폭) x (template 높이 + 타이틀바). NC→1px 정규화로 client 이 template 보다 양변
//       합 ~12px 커지는 drift 를 제거 → 좌/우·상/하 여백이 리소스 설계값 그대로 유지된다 (여백 비대칭 fix).
//   (2) 모든 직계 자식(시스템 버튼 제외)을 m_titlebar_height 만큼 하향 → 콘텐츠가 타이틀바 아래에 자동 배치.
//여기서 위치를 확정한 뒤 파생의 CResizeCtrl 가 캡처하므로, 이후 RestoreWindowPosition 등으로 리사이즈해도
//설계 여백이 anchor 로 보존된다.
void CSCThemeDlg::apply_client_titlebar_layout(const CRect& rc_template_client)
{
	int want_cw = rc_template_client.Width();
	int want_ch = rc_template_client.Height() + m_titlebar_height;

	CRect rc_now;
	GetClientRect(rc_now);

	int dw = want_cw - rc_now.Width();
	int dh = want_ch - rc_now.Height();
	if (dw != 0 || dh != 0)
	{
		CRect rc_win;
		GetWindowRect(rc_win);
		SetWindowPos(nullptr, 0, 0, rc_win.Width() + dw, rc_win.Height() + dh,
			SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);
	}

	if (m_titlebar_height <= 0)
		return;

	for (CWnd* child = GetWindow(GW_CHILD); child != nullptr; child = child->GetNextWindow())
	{
		if (child->GetSafeHwnd() == m_sys_buttons.GetSafeHwnd())
			continue;

		CRect rc_child;
		child->GetWindowRect(rc_child);
		ScreenToClient(rc_child);
		child->SetWindowPos(nullptr, rc_child.left, rc_child.top + m_titlebar_height, 0, 0,
			SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);
	}
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
	return FALSE;
}


void CSCThemeDlg::OnLButtonDown(UINT nFlags, CPoint point)
{
	// TODO: 여기에 메시지 처리기 코드를 추가 및/또는 기본값을 호출합니다.
	if (m_titlebar_movable && point.y < m_titlebar_height)
		DefWindowProc(WM_NCLBUTTONDOWN, HTCAPTION, MAKEWORD(point.x, point.y));
	//else if (!m_use_resizable)
	//	return;

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
	m_use_resizable = resizable;
	m_has_thickframe = resizable;

	if (resizable)
		ModifyStyle(0, WS_THICKFRAME);
	else
		ModifyStyle(WS_THICKFRAME, 0);

	//SWP_FRAMECHANGED 가 핵심 — NC 재계산 트리거.
	::SetWindowPos(m_hWnd, nullptr, 0, 0, 0, 0,
		SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE | SWP_FRAMECHANGED);

	if (resizable)
	{
		AdjustWindowRectEx(&m_border_thickness,
			::GetWindowLongPtr(m_hWnd, GWL_STYLE), FALSE, NULL);
		m_border_thickness.left *= -1;
		m_border_thickness.top *= -1;
	}
	else
	{
		SetRect(&m_border_thickness, 1, 1, 1, 1);
	}
}


//borderless dialog.md Step 8 — 실제 NC paint 차단은 WindowProc 의 WM_NCACTIVATE → TRUE 에서 한다.
//여기서는 base 위임만. (이전엔 Invalidate()+RedrawWindow() 부수효과가 있었으나
//WM_NCACTIVATE 를 WindowProc 에서 완전 차단하는 새 패턴에서는 불필요.)
BOOL CSCThemeDlg::OnNcActivate(BOOL bActive)
{
	return CDialogEx::OnNcActivate(bActive);
}


//borderless dialog.md Step 6 — NC 영역 축소.
//Win10+ DWM 은 WS_THICKFRAME 윈도우에 default 로 두꺼운 NC margin (Win11=7px) 을 둔다.
//6px 흡수해 NC=1px 만 남기면 시각적으로 두꺼운 띠가 사라지고 OnNcPaint 가 theme 색으로 채운다.
//XP/Vista/7/8 은 DWM 의 두꺼운 frame 이 없으므로 축소하지 않음 (음수 NC margin 은 미정의 동작).
//*반드시 base 호출 후* rgrc 보정 — base 가 자체 계산을 우리 보정 위에 덮어쓰지 않도록.
void CSCThemeDlg::OnNcCalcSize(BOOL bCalcValidRects, NCCALCSIZE_PARAMS* lpncsp)
{
	CDialogEx::OnNcCalcSize(bCalcValidRects, lpncsp);

	if (!m_has_thickframe || !lpncsp || !bCalcValidRects)
		return;

	//Win10+ 에서만 6px 흡수. XP/Vista/7/8 은 OS-default NC 그대로.
	if (get_windows_major_version() >= 10)
	{
		const int kNcShrink = 6;
		lpncsp->rgrc[0].left   -= kNcShrink;
		lpncsp->rgrc[0].top    -= kNcShrink;
		lpncsp->rgrc[0].right  += kNcShrink;
		lpncsp->rgrc[0].bottom += kNcShrink;
	}
}


//borderless dialog.md Step 7 — NC 영역을 theme 색으로 직접 칠한다.
//CDialogEx::OnNcPaint 호출 안 함 — base 호출 시 default frame 이 우리 paint 위에 덮어 그림.
//CWindowDC 로 window 전체 (NC 포함) 잡고, ExcludeClipRect(client) 로 client 영역 침범 차단.
void CSCThemeDlg::OnNcPaint()
{
	CWindowDC dc(this);

	CRect rcWindow;
	GetWindowRect(rcWindow);
	CRect rcWindowLocal(0, 0, rcWindow.Width(), rcWindow.Height());

	CRect rcClient;
	GetClientRect(rcClient);
	CPoint ptClientOrg(0, 0);
	ClientToScreen(&ptClientOrg);
	CRect rcClientInWin = rcClient;
	rcClientInWin.OffsetRect(ptClientOrg.x - rcWindow.left, ptClientOrg.y - rcWindow.top);

	dc.ExcludeClipRect(rcClientInWin);
	//titlebar 가 있는 dlg 의 NC 1px 링 = "창 프레임 테두리" 색.
	//- cr_back(흰색)으로 두면 titlebar 위/좌/우에 흰 띠, cr_title_back_active(타이틀바색)로 두면 창 네 변이 진한 프레임.
	//- 테마의 cr_border_inactive 는 콤보/에디트 등 "흰 본문 위 컨트롤 테두리"용(예: linkmemine #d2d2d2)이라 어두운
	//  chrome 프레임에 쓰면 오히려 너무 밝다. 그래서 창 프레임은 타이틀바색에서 한 톤 파생한 은은한 색으로 별도 계산한다.
	//titlebar 없는 dlg 는 cr_back 그대로(내용과 blend).
	COLORREF nc_fill = (m_titlebar_height > 0)
		? get_frame_border_color().ToCOLORREF()
		: m_theme.cr_back.ToCOLORREF();
	dc.FillSolidRect(rcWindowLocal, nc_fill);
}


//borderless dialog.md Step 9 — WindowProc 트랩.
// 1) WM_NCACTIVATE: DefWindowProc(WM_NCACTIVATE, wParam, -1) 은 Win11 에서 frame 자체를 light 색으로
//    그리는 경로가 남는다. 완전 차단을 위해 DefWindowProc 호출 안 하고 TRUE 반환.
// 2) WM_NCHITTEST: NC=1px 라 OS default hit-test 의 resize 폭이 1px → 마우스로 잡기 거의 불가.
//    화면 좌표 (lParam) 기준 가장자리 8px 폭에서 직접 resize 코드 반환.
LRESULT CSCThemeDlg::WindowProc(UINT message, WPARAM wParam, LPARAM lParam)
{
	if (message == WM_NCACTIVATE)
		return TRUE;

	if (message == WM_NCHITTEST && m_has_thickframe)
	{
		POINT pt = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
		CRect rcWin;
		GetWindowRect(rcWin);

		const int kEdge = 8;
		bool left   = pt.x >= rcWin.left   && pt.x <  rcWin.left   + kEdge;
		bool right  = pt.x <  rcWin.right  && pt.x >= rcWin.right  - kEdge;
		bool top    = pt.y >= rcWin.top    && pt.y <  rcWin.top    + kEdge;
		bool bottom = pt.y <  rcWin.bottom && pt.y >= rcWin.bottom - kEdge;

		if (top && left)     return HTTOPLEFT;
		if (top && right)    return HTTOPRIGHT;
		if (bottom && left)  return HTBOTTOMLEFT;
		if (bottom && right) return HTBOTTOMRIGHT;
		if (left)            return HTLEFT;
		if (right)           return HTRIGHT;
		if (top)             return HTTOP;
		if (bottom)          return HTBOTTOM;
		//내부는 default 가 HTCLIENT 반환하도록 base 에 위임.
	}

	return CDialogEx::WindowProc(message, wParam, lParam);
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

	//타이틀바 영역
	if (m_titlebar_height > 0)
	{
		CRect rTitle = rc;

		//윈도우11에서 타이틀바 양쪽에 1픽셀 정도가 모자르게 그려지는데 여기서 InflateRect(1, 0)를 해줘도 그려지지 않는다.
		//rTitle.InflateRect(10, 0);

		rTitle.bottom = m_titlebar_height;

		//active 상태와 inactive 상태에 따라 색상을 구분하려 했으나 CSCSystemButtons에도 영향을 주고
		//여러가지 영향받는 부분이 많다보니 우선은 active 컬러만 사용하도록 한다.
		//if (m_is_activated || (::GetActiveWindow() == m_hWnd))
			dc.FillSolidRect(rTitle, m_theme.cr_title_back_active.ToCOLORREF());
		//else
			//dc.FillSolidRect(rTitle, m_theme.cr_title_back_inactive.ToCOLORREF());

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
				rTitle.left += 6;
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

	//클라이언트 영역 칠하기. 배경이미지가 있다면 이미지를 표시
	CRect rclient = rc;
	rclient.top += m_titlebar_height;
	if (m_img_back.is_valid())
	{
		if (m_img_back_draw_mode == CSCGdiplusBitmap::draw_mode_zoom ||
			m_img_back_draw_mode == CSCGdiplusBitmap::draw_mode_original)
		{
			dc.FillSolidRect(rclient, m_cr_out_of_back_img.ToCOLORREF());
		}

		m_img_back.draw(g, rclient, m_img_back_draw_mode);
	}
	else
	{
		dc.FillSolidRect(rclient, m_theme.cr_back.ToCOLORREF());
	}

	//border
	if (m_draw_border && m_border_width > 0)
	{
		GetClientRect(rc);
		draw_rect(&dc, rc, m_theme.cr_border_inactive, Gdiplus::Color::Transparent, m_border_width);
	}
}


void CSCThemeDlg::set_color_theme(int theme, bool invalidate)
{
	m_theme.set_color_theme(theme);
	//int 인덱스로 부모 m_theme 를 채운 직후 — 자식 sys_buttons 에도 객체 전체를 전달해야
	//cr_title_back_active 등 sys_buttons 가 참조하는 모든 필드가 일관되게 반영된다.
	m_sys_buttons.set_color_theme(m_theme, invalidate);

	//m_theme.set_color_theme(theme);에서는 cr_text, cr_back 등 일반적인 색상값들을 세팅한다.
	//CSCThemeDlg::set_color_theme(int theme) 에서는 m_sys_buttons 등
	//CSCThemeDlg를 상속받은 클래스에서 필요로하는 부가적인 세팅이 필요하다.
	switch (theme)
	{
		case CSCColorTheme::color_theme_linkmemine :
			//SetWindowText(_T("color_theme_linkmemine"));
			m_titlebar_lf.lfWeight = (m_titlebar_bold ? FW_BOLD : FW_NORMAL);
			m_titlebar_lf.lfHeight = get_pixel_size_from_font_size(m_hWnd, 10);
			reconstruct_titlebar_font();
			break;
		case CSCColorTheme::color_theme_linkmemine_origin:
			m_titlebar_lf.lfWeight = (m_titlebar_bold ? FW_BOLD : FW_NORMAL);
			m_titlebar_lf.lfHeight = get_pixel_size_from_font_size(m_hWnd, 10);
			reconstruct_titlebar_font();
			break;
		case CSCColorTheme::color_theme_claude:
			m_titlebar_lf.lfWeight = (m_titlebar_bold ? FW_BOLD : FW_NORMAL);
			m_titlebar_lf.lfHeight = get_pixel_size_from_font_size(m_hWnd, 10);
			reconstruct_titlebar_font();
			break;
		//claude00 ~ 09 시리즈 — claude 와 동일한 titlebar 처리. 10개 동일 블록이라 fall-through 로 묶음.
		case CSCColorTheme::color_theme_claude00:
		case CSCColorTheme::color_theme_claude01:
		case CSCColorTheme::color_theme_claude02:
		case CSCColorTheme::color_theme_claude03:
		case CSCColorTheme::color_theme_claude04:
		case CSCColorTheme::color_theme_claude05:
		case CSCColorTheme::color_theme_claude06:
		case CSCColorTheme::color_theme_claude07:
		case CSCColorTheme::color_theme_claude08:
		case CSCColorTheme::color_theme_claude09:
			m_titlebar_lf.lfWeight = (m_titlebar_bold ? FW_BOLD : FW_NORMAL);
			m_titlebar_lf.lfHeight = get_pixel_size_from_font_size(m_hWnd, 10);
			reconstruct_titlebar_font();
			break;
		case CSCColorTheme::color_theme_linkmemine_se:
			//SetWindowText(_T("color_theme_linkmemine_se"));
			m_titlebar_lf.lfWeight = (m_titlebar_bold ? FW_BOLD : FW_NORMAL);
			m_titlebar_lf.lfHeight = get_pixel_size_from_font_size(m_hWnd, 10);
			reconstruct_titlebar_font();
			break;
		case CSCColorTheme::color_theme_helpu:
			//SetWindowText(_T("color_theme_helpu"));
			m_titlebar_lf.lfWeight = (m_titlebar_bold ? FW_BOLD : FW_NORMAL);
			m_titlebar_lf.lfHeight = get_pixel_size_from_font_size(m_hWnd, 10);
			reconstruct_titlebar_font();
			break;
		case CSCColorTheme::color_theme_anysupport:
			//SetWindowText(_T("color_theme_anysupport"));
			m_titlebar_lf.lfWeight = (m_titlebar_bold ? FW_BOLD : FW_NORMAL);
			m_titlebar_lf.lfHeight = get_pixel_size_from_font_size(m_hWnd, 10);
			reconstruct_titlebar_font();
			break;
		case CSCColorTheme::color_theme_pcanypro:
			//SetWindowText(_T("color_theme_pcanypro"));
			m_titlebar_lf.lfWeight = (m_titlebar_bold ? FW_BOLD : FW_NORMAL);
			m_titlebar_lf.lfHeight = get_pixel_size_from_font_size(m_hWnd, 10);
			reconstruct_titlebar_font();
			break;
		case CSCColorTheme::color_theme_default:
			//SetWindowText(_T("color_theme_default"));
			break;
		case CSCColorTheme::color_theme_white:
			//SetWindowText(_T("color_theme_white"));
			break;
		case CSCColorTheme::color_theme_gray:
			//SetWindowText(_T("color_theme_gray"));
			break;
		case CSCColorTheme::color_theme_dark_gray:
			//SetWindowText(_T("color_theme_dark_gray"));
			break;
		case CSCColorTheme::color_theme_dark:
			//SetWindowText(_T("color_theme_dark"));
			break;
		default:
			;//SetWindowText(_T("not defined color_theme"));
	}

	//borderless dialog.md Step 5 — Win11 DWM 합성 frame 의 dark/round/border-color 동기화.
	//3개 호출 모두 win_compat::dwm 경유라 XP/Vista 등 dwmapi 미존재/속성 미지원 OS 에선 자동 no-op.
	//theme 변경 시점마다 다시 호출되어야 inactive frame 의 색이 새 theme 과 일관.
	if (m_hWnd)
	{
		win_compat::dwm::use_immersive_dark_mode(m_hWnd, true);
		win_compat::dwm::set_window_corner_round(m_hWnd);
		//OnNcPaint 의 nc_fill 과 동일한 색(타이틀바색 한 톤 파생) — 두 경로가 같은 색이라 seam 없음.
		win_compat::dwm::set_border_color(m_hWnd,
			(m_titlebar_height > 0)
				? get_frame_border_color().ToCOLORREF()
				: m_theme.cr_back.ToCOLORREF());
	}

	if (invalidate && m_hWnd)
		Invalidate();
}

//또 다른 CSCThemeDlg / SCxxx 컨트롤이 자식으로 있을 때 부모의 m_theme 객체 자체를 그대로 전파하는 경로.
//int 인덱스만 넘기면 자식이 인덱스 기반 default 만 재계산하여 부모의 titlebar 색 커스터마이즈 등이 누락된다.
//case 별 titlebar font 조정은 set_color_theme(int) 의 책임 (부모가 인덱스로 처음 적용하는 시점) 이고,
//여기서는 색만 복사 + 자식 sys_buttons 동기화 + DWM 호출 동기화 만 수행한다.
void CSCThemeDlg::set_color_theme(const CSCColorTheme& theme, bool invalidate)
{
	m_theme.copy_colors_from(theme);
	m_sys_buttons.set_color_theme(m_theme, invalidate);

	if (m_hWnd)
	{
		win_compat::dwm::use_immersive_dark_mode(m_hWnd, true);
		win_compat::dwm::set_window_corner_round(m_hWnd);
		//OnNcPaint 의 nc_fill 과 동일한 색(타이틀바색 한 톤 파생) — 두 경로가 같은 색이라 seam 없음.
		win_compat::dwm::set_border_color(m_hWnd,
			(m_titlebar_height > 0)
				? get_frame_border_color().ToCOLORREF()
				: m_theme.cr_back.ToCOLORREF());
	}

	if (invalidate && m_hWnd)
		Invalidate();
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
	m_theme.cr_title_back_active = cr;
	m_sys_buttons.set_back_color(cr);
}

//parent창이 resize 될 때 호출해줘야만 m_sys_buttons가 위치를 바로잡는다.
void CSCThemeDlg::adjust_sys_buttons()
{
	if (m_sys_buttons.m_hWnd == NULL)
		return;

	CRect r;
	GetClientRect(r);
	m_sys_buttons.adjust(r.top, r.right);
}

BOOL CSCThemeDlg::PreTranslateMessage(MSG* pMsg)
{
	// TODO: 여기에 특수화된 코드를 추가 및/또는 기본 클래스를 호출합니다.
	if (pMsg->message == WM_KEYDOWN)
	{
		TRACE(_T("keydown on CSCThemeDlg\n"));
	}

	return CDialogEx::PreTranslateMessage(pMsg);
}

//만약 CSCThemeDlg를 상속받은 CMainDlg에 타이틀바가 표시되는 구조라면 여기서 처리해도 되지만
//ASeeDlg 프로젝트와 같이 MainDlg가 있고 타이틀바가 별도로 구현된 경우, CTitleDlg에서 dblclk을 할 경우
//minimize, maximize 처리도 해야하지만 CTitleDlg를 show/hide 해야하는 등의 처리도 필요하므로
//이 클래스에서 이 핸들러를 처리하면 복잡해진다. 실제 클래스에서 처리하자.
//GetParent() == AfxGetApp()->GetMainWnd()라면 CTitleDlg와 같은 별도 구현된 타이틀바가 있는 경우이므로
//이는 CTitleDlg에서 처리?
//GetParent() != AfxGetApp()->GetMainWnd()인 경우라면 바로 메시지를 처래해도 될 듯하다.
void CSCThemeDlg::OnLButtonDblClk(UINT nFlags, CPoint point)
{
	// TODO: 여기에 메시지 처리기 코드를 추가 및/또는 기본값을 호출합니다.
	if (m_sys_buttons.has_button(SC_MAXIMIZE) && (point.y < m_titlebar_height))
	{
		CWnd* target_wnd = this;
		CWnd* parent = GetParent();
		CWnd* mainWnd = AfxGetApp()->GetMainWnd();
		if (parent == nullptr || parent == AfxGetApp()->GetMainWnd())
			target_wnd = AfxGetApp()->GetMainWnd();

		//프로그램 아이콘을 더블클릭한 경우
		if (point.x < 32)
		{
			target_wnd->PostMessage(WM_SYSCOMMAND, SC_CLOSE);
			return;
		}

		if (target_wnd->IsZoomed())
			target_wnd->PostMessage(WM_SYSCOMMAND, SC_RESTORE);
		else
			target_wnd->PostMessage(WM_SYSCOMMAND, SC_MAXIMIZE);

		m_sys_buttons.parent_maximized(target_wnd->IsZoomed() ? false : true);
		//m_sys_buttons.Invalidate();
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

	//SC_CLOSE(61536)으로 넘어올 때 CDialogEx::OnSysCommand(nID, lParam);를 호출하면
	//종료될 것으로 예상했으나 if ((nID & 0xFFF0) == SC_CLOSE)과 같은 처리를 해줘야만 동작한다.
	//if ((nID & 0xFFF0) == SC_CLOSE)
	//{
	//	EndDialog(IDCANCEL);
	//}
	//else if ((nID & 0xFFF0) == SC_MINIMIZE)
	//{
	//	ShowWindow(SW_MINIMIZE);
	//}
	//else
	{
		CDialogEx::OnSysCommand(nID, lParam);
	}
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

//OnSetCursor 제거 — 이전 구현 `if (!m_use_resizable) return TRUE;` 는 cursor 미설정 채 TRUE 반환이라
//resize 가능 영역 밖에서도 직전 cursor (resize 화살표 등) 가 잔류하는 부수효과가 있었다.
//WindowProc 의 WM_NCHITTEST 트랩이 m_has_thickframe=false 일 때 resize 코드를 반환하지 않으므로
//OS default 가 알아서 client cursor 를 세팅한다. 별도 OnSetCursor 트랩 불필요.

void CSCThemeDlg::set_draw_border(bool draw, int width, Gdiplus::Color cr)
{
	m_draw_border = draw;
	m_border_width = width;
	m_theme.cr_border_inactive = cr;
	Invalidate();
	RedrawWindow();
}

//창 외곽 프레임(NC 링 + DWM border) 색 override. Transparent 를 주면 기본값(타이틀바색 파생)으로 되돌아간다.
void CSCThemeDlg::set_frame_border_color(Gdiplus::Color cr)
{
	m_frame_border_color = cr;

	//즉시 반영 — DWM border 색 갱신 + SWP_FRAMECHANGED 로 NC 재계산/재그리기(OnNcPaint 의 nc_fill 재적용).
	if (m_hWnd && m_titlebar_height > 0)
	{
		win_compat::dwm::set_border_color(m_hWnd, get_frame_border_color().ToCOLORREF());
		SetWindowPos(NULL, 0, 0, 0, 0,
			SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE | SWP_FRAMECHANGED);
	}
}

/*
void CSCThemeDlg::refresh_activate_status(bool is_activated)
{
	//m_is_activated = is_activated;
	//m_sys_buttons.refresh_activate_status(m_is_activated);
	//Invalidate();
}
*/

void CSCThemeDlg::OnActivate(UINT nState, CWnd* pWndOther, BOOL bMinimized)
{
	CDialogEx::OnActivate(nState, pWndOther, bMinimized);

	// TODO: 여기에 메시지 처리기 코드를 추가합니다.
	//TRACE(_T("CSCThemeDlg::OnActivate. nState = %d\n"), nState);
	//m_is_activated = (nState == 2);
	//m_sys_buttons.refresh_activate_status(m_is_activated);
	//Invalidate();
}

void CSCThemeDlg::OnSetFocus(CWnd* pOldWnd)
{
	CDialogEx::OnSetFocus(pOldWnd);

	// TODO: 여기에 메시지 처리기 코드를 추가합니다.
	//TRACE(_T("CSCThemeDlg::OnSetFocus\n"));
	//Invalidate();
}

void CSCThemeDlg::OnKillFocus(CWnd* pNewWnd)
{
	CDialogEx::OnKillFocus(pNewWnd);

	// TODO: 여기에 메시지 처리기 코드를 추가합니다.
	//TRACE(_T("CSCThemeDlg::OnKillFocus\n"));
	//Invalidate();
}
