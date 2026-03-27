// SCColorPicker.cpp: 구현 파일
//
#include "SCColorPicker.h"

#include <afxdlgs.h>
#include "../../Functions.h"
#include "../../MemoryDC.h"
#include <map>          // ← import 파싱용 (index→Color)

const float CSCColorPicker::m_hues[PALETTE_COLOR_COLS] = {
	0.f,    // Red
	30.f,   // Orange
	60.f,   // Yellow
	120.f,  // Green
	165.f,  // Spring Green
	195.f,  // Cyan
	225.f,  // Azure / Sky Blue
	255.f,  // Blue / Cornflower
	285.f,  // Violet / Purple
	315.f,  // Magenta / Pink
};

const CSCColorPicker::PaletteSV CSCColorPicker::m_sv_rows[PALETTE_COLOR_ROWS] = {
	{ 0.15f, 1.00f },  // 매우 연한 파스텔 (row 1)
	{ 0.35f, 1.00f },  // 연한 (row 2)
	{ 0.60f, 1.00f },  // 중간 (row 3)
	{ 1.00f, 1.00f },  // 선명한 순색 (row 4)
	{ 1.00f, 0.72f },  // 어두운 (row 5)
	{ 1.00f, 0.45f },  // 매우 어두운 (row 6)
};

// CSCColorPicker 대화 상자

IMPLEMENT_DYNAMIC(CSCColorPicker, CDialog)

CSCColorPicker::CSCColorPicker()
{
}

CSCColorPicker::~CSCColorPicker()
{
}

void CSCColorPicker::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
}


BEGIN_MESSAGE_MAP(CSCColorPicker, CDialog)
	ON_BN_CLICKED(IDOK, &CSCColorPicker::OnBnClickedOk)
	ON_BN_CLICKED(IDCANCEL, &CSCColorPicker::OnBnClickedCancel)
	ON_WM_PAINT()
	ON_WM_LBUTTONDOWN()
	ON_WM_LBUTTONUP()
	ON_WM_MOUSEMOVE()
	ON_WM_MOUSEWHEEL()
	ON_WM_RBUTTONDOWN()
	ON_WM_RBUTTONUP()
	ON_REGISTERED_MESSAGE(Message_CSCSliderCtrl, &CSCColorPicker::on_message_CSCSliderCtrl)
	ON_REGISTERED_MESSAGE(Message_CSCEdit, &CSCColorPicker::on_message_CSCEdit)
	ON_WM_LBUTTONDBLCLK()
	ON_WM_ERASEBKGND()
	ON_EN_CHANGE(IDC_EDIT_HEXA, OnEnChangeHexa)
	ON_EN_CHANGE(IDC_EDIT_ARGB_A, OnEnChangeArgb)
	ON_EN_CHANGE(IDC_EDIT_ARGB_R, OnEnChangeArgb)
	ON_EN_CHANGE(IDC_EDIT_ARGB_G, OnEnChangeArgb)
	ON_EN_CHANGE(IDC_EDIT_ARGB_B, OnEnChangeArgb)
	ON_EN_CHANGE(IDC_EDIT_HSL_H, OnEnChangeHsl)
	ON_EN_CHANGE(IDC_EDIT_HSL_S, OnEnChangeHsl)
	ON_EN_CHANGE(IDC_EDIT_HSL_L, OnEnChangeHsl)
	ON_WM_DESTROY()
	ON_WM_CONTEXTMENU()
	ON_MESSAGE(WM_MOUSELEAVE, &CSCColorPicker::OnMouseLeave)
	ON_WM_TIMER()
	ON_WM_WINDOWPOSCHANGED()
END_MESSAGE_MAP()


bool CSCColorPicker::create(CWnd* parent, CString title, bool as_modal)
{
	if (parent == nullptr)
		parent = AfxGetApp()->GetMainWnd(); 
	
	if (parent == nullptr)
		return false;

	m_parent = parent;

	if (title.IsEmpty())
		title = _T("Color Picker");

	m_as_modal = as_modal;

	//WS_CLIPCHILDREN을 줘야만 slider 컨트롤이 깜빡이지 않고 제대로 그려짐. (WS_CLIPCHILDREN 없으면 슬라이더를 움직일 때마다 팔레트 전체가 리페인트됨)
	// WS_EX_TOOLWINDOW은 확장 스타일로 분리 (이전 코드에서 일반 스타일에 잘못 혼입되어 있었음)
	// WS_VISIBLE은 modeless 경로에서만 초기 생성 시 포함.
	// modal 경로는 DoModal()에서 ShowWindow(SW_SHOW)로 직접 표시하므로 초기 가시화 불필요.
	// → UIAutomation이 즉시 추적을 시작하여 DestroyWindow 시점에 충돌하는 현상 방지.

	//dwExStyle을 NULL로 주면 기본 아이콘이 타이틀바에 표시된다. main dialog의 아이콘을 조회해서 가져오는 방법도 있으나 굳이.
	//그렇다고 WS_SYSMENU를 주지 않으면 아이콘은 표시되지 않지만 우측의 종료 버튼도 표시되지 않는다.
	//GPT에 검색해보니 WS_EX_DLGMODALFRAME를 주고 우회하여 아이콘을 보이지 않게 non-client 영역 다시 계산등의 방법을 제안했는데
	//WS_EX_DLGMODALFRAME를 주는것만으로도 x버튼은 살리고 아이콘은 표시되지 않도록 실행된다.
	const DWORD dwExStyle = WS_EX_DLGMODALFRAME;
	DWORD dwStyle = WS_POPUP | WS_CAPTION | WS_SYSMENU | WS_CLIPCHILDREN;
	//if (!as_modal)
	//	dwStyle |= WS_VISIBLE;	// modeless 경로: 생성 즉시 표시

	WNDCLASS wc = {};
	::GetClassInfo(AfxGetInstanceHandle(), _T("#32770"), &wc);
	wc.lpszClassName = _T("CSCColorPicker");
	AfxRegisterClass(&wc);

	bool res = CreateEx(dwExStyle, wc.lpszClassName, _T("CSCColorPicker"), dwStyle, CRect(0, 0, 500, 400), parent, 0);

	if (!res)
		return false;

	SetWindowText(title);
	//CenterWindow(m_parent);
	calc_layout();
	load_recent_colors();	// ← modeless 경로: create()에서 직접 로드

	// ── 최근 색상이 있으면 recent[0] 기본 선택 ──────────────────────
	if (!m_recent_colors.empty())
	{
		m_sel = { HitArea::Recent, -1, -1, 0 };
		m_sel_color = m_recent_colors[0];
		gcolor_to_hsl(m_sel_color, m_hue, m_sat, m_light);
		update_slider_alpha();
		update_slider_hue();
		update_slider_light();
		sync_edits();
	}

	// ── 트래킹 툴팁 생성 ─────────────────────────────────────
	// TTF_TRACK | TTF_ABSOLUTE: 위치·표시를 코드에서 완전 제어
	if (!m_tooltip.GetSafeHwnd())
	{
		m_tooltip.Create(this, TTS_NOPREFIX | TTS_ALWAYSTIP);
		m_tooltip.SetMaxTipWidth(320);          // 줄바꿈 허용 폭

		TOOLINFO ti = {};
		ti.cbSize = sizeof(TOOLINFO);
		ti.uFlags = TTF_TRACK | TTF_ABSOLUTE;
		ti.hwnd = m_hWnd;
		ti.uId = 1;
		ti.lpszText = const_cast<LPTSTR>(_T(""));
		::SendMessage(m_tooltip.m_hWnd, TTM_ADDTOOL, 0, (LPARAM)&ti);
	}

	return res;
}

// ── 레이아웃 메트릭 계산 (1회 호출) ──────────────────────
void CSCColorPicker::calc_layout()
{
	const int width = 284;

	m_margin = width * 0.04f;
	m_cell = (width - 2.f * m_margin) / static_cast<float>(PALETTE_TOTAL_COLS);
	m_radius = m_cell * 0.42f;

	const int palette_h = static_cast<int>(m_margin * 2.f + m_cell * PALETTE_ROWS + 0.5f);
	m_r_palette = CRect(0, 0, width, palette_h);

	// 최근 색상 + 버튼 행 하단 y — 위아래 여백 모두 m_margin 으로 통일
	const float recent_bottom = m_r_palette.bottom + 2.f * m_margin + m_cell;	// +m_margin → +2*m_margin

	// ── 3개 슬라이더 + 미리보기 섹션 ─────────────────────────────────────
	const int slider_h = 26;
	const int slider_gap = static_cast<int>(m_margin * 0.1f);

	const int total_slider_h = 3 * slider_h + 2 * slider_gap;
	const int preview_diam = total_slider_h;

	const float info_top = recent_bottom;	// + 4.0f 제거 → recent_bottom 바로 사용
	const int slider_section_top = static_cast<int>(info_top) + m_margin;
	const int section_bottom = slider_section_top + total_slider_h;

	// 미리보기 원: 슬라이더 스택 전체와 동일 높이·수직 정렬
	m_r_preview = CRect(m_margin, slider_section_top, m_margin + preview_diam, slider_section_top + preview_diam);

	// 슬라이더 3개: 미리보기 원 오른쪽, slider_gap 간격으로 세로 배치
	const int slider_left = m_margin + preview_diam + m_margin;
	const int slider_right = width - m_margin;

	m_r_slider_alpha = CRect(slider_left, slider_section_top,
		slider_right, slider_section_top + slider_h);
	m_r_slider_hue = CRect(slider_left, slider_section_top + slider_h + slider_gap,
		slider_right, slider_section_top + 2 * slider_h + slider_gap);
	m_r_slider_value = CRect(slider_left, slider_section_top + 2 * slider_h + 2 * slider_gap,
		slider_right, slider_section_top + 3 * slider_h + 2 * slider_gap);

	// 편집 영역: section_bottom + m_margin(gap) + 16px(레이블)
	const int hexa_width = 80;
	const int editTop = section_bottom + m_margin + 16;
	m_r_edit_area = CRect(m_r_palette.left + m_margin, editTop,
		m_r_palette.right - m_margin, editTop + kEditH);

	// ── Hex 레이블 더블클릭 영역 (레이블 상단 ~ 편집 하단, 폭 = kHexaW) ──
	m_r_label_hexa = CRect(m_r_edit_area.left, m_r_edit_area.top - 16,
		m_r_edit_area.left + kHexaW, m_r_edit_area.bottom);

	// ── HSV 행 위치 사전 계산 (cellW를 if/else 밖으로 호이스팅) ──────────
	const int cellW = (m_r_edit_area.Width() - hexa_width - 4 * kCellGap) / 4;
	const int hsvX0 = m_r_edit_area.left + hexa_width + kCellGap + 1 * (cellW + kCellGap); // R 열과 동일
	const int hsvTop = m_r_edit_area.bottom + 8 + 16;	// 8px 시각 간격 + 16px 레이블
	m_r_edit_hsv_area = CRect(hsvX0, hsvTop, m_r_edit_area.right, hsvTop + kEditH);

	// ── Color Name 표시 영역 (HSV 행 왼쪽 빈 공간) ───────────────────
	m_r_color_name = CRect(m_r_edit_area.left, hsvTop, hsvX0 - kCellGap, hsvTop + kEditH);

	if (!m_edit_hexa.GetSafeHwnd())
	{
		//Hex (AARRGGBB)
		m_edit_hexa.Create(
			WS_CHILD | WS_VISIBLE | WS_TABSTOP | WS_BORDER | ES_CENTER | ES_AUTOHSCROLL | ES_MULTILINE,
			CRect(m_r_edit_area.left, m_r_edit_area.top, m_r_edit_area.left + hexa_width, m_r_edit_area.top + kEditH), this, IDC_EDIT_HEXA);
		m_edit_hexa.LimitText(9);
		m_edit_hexa.set_font_name(_T("Consolas"));
		m_edit_hexa.set_font_size(9);
		m_edit_hexa.set_text_color(Gdiplus::Color(52, 68, 71, 70));
		m_edit_hexa.set_back_color(Gdiplus::Color::White);
		m_edit_hexa.set_border_color(Gdiplus::Color(255, 232, 232, 232));
		m_edit_hexa.set_dark_border_on_focus(true);
		m_edit_hexa.set_line_align(DT_VCENTER);


		//ARGB (4등분)
		const int cellW = (m_r_edit_area.Width() - hexa_width - 4 * kCellGap) / 4;
		const int kIds[4] = { IDC_EDIT_ARGB_A, IDC_EDIT_ARGB_R, IDC_EDIT_ARGB_G, IDC_EDIT_ARGB_B };

		for (int i = 0; i < 4; i++)
		{
			const int x0 = m_r_edit_area.left + hexa_width + kCellGap + i * (cellW + kCellGap);
			m_edit_argb[i].Create(
				WS_CHILD | WS_VISIBLE | WS_TABSTOP | WS_BORDER | ES_CENTER | ES_AUTOHSCROLL | ES_MULTILINE,
				CRect(x0, m_r_edit_area.top, x0 + cellW, m_r_edit_area.top + kEditH),
				this, kIds[i]);
			m_edit_argb[i].LimitText(3);
			m_edit_argb[i].set_font_name(_T("Consolas"));
			m_edit_argb[i].set_font_size(9);
			m_edit_argb[i].set_text_color(Gdiplus::Color(52, 68, 71, 70));
			m_edit_argb[i].set_back_color(Gdiplus::Color::White);
			m_edit_argb[i].set_border_color(Gdiplus::Color(255, 232, 232, 232));
			m_edit_argb[i].set_dark_border_on_focus(true);
			m_edit_argb[i].set_line_align(DT_VCENTER);
		}

		// ── HSL (3등분, R·G·B 아래 정렬) ─────────────────────────────────
		const int kHslIds[3] = { IDC_EDIT_HSL_H, IDC_EDIT_HSL_S, IDC_EDIT_HSL_L };
		for (int i = 0; i < 3; i++)
		{
			const int x0 = hsvX0 + i * (cellW + kCellGap);
			m_edit_hsl[i].Create(
				WS_CHILD | WS_VISIBLE | WS_TABSTOP | WS_BORDER | ES_CENTER | ES_AUTOHSCROLL | ES_MULTILINE,
				CRect(x0, hsvTop, x0 + cellW, hsvTop + kEditH),
				this, kHslIds[i]);
			m_edit_hsl[i].LimitText(3);
			m_edit_hsl[i].set_font_name(_T("Consolas"));
			m_edit_hsl[i].set_font_size(9);
			m_edit_hsl[i].set_text_color(Gdiplus::Color(52, 68, 71, 70));
			m_edit_hsl[i].set_back_color(Gdiplus::Color::White);
			m_edit_hsl[i].set_border_color(Gdiplus::Color(255, 232, 232, 232));
			m_edit_hsl[i].set_dark_border_on_focus(true);
			m_edit_hsl[i].set_line_align(DT_VCENTER);
		}

		// ── Color Name Edit (HSV row left empty area) ─────────────────
		m_edit_color_name.Create(
			WS_CHILD | WS_VISIBLE | WS_TABSTOP | WS_BORDER | ES_CENTER | ES_AUTOHSCROLL | ES_MULTILINE,
			m_r_color_name, this, IDC_EDIT_COLOR_NAME);
		m_edit_color_name.LimitText(24);
		m_edit_color_name.set_font_name(_T("Consolas"));
		m_edit_color_name.set_font_size(9);
		m_edit_color_name.set_text_color(Gdiplus::Color(52, 68, 71, 70));
		m_edit_color_name.set_back_color(Gdiplus::Color::White);
		m_edit_color_name.set_border_color(Gdiplus::Color(255, 232, 232, 232));
		m_edit_color_name.set_dark_border_on_focus(true);
		m_edit_color_name.set_line_align(DT_VCENTER);

		sync_edits();
	}
	else    // calc_layout() 재호출 시 위치만 갱신
	{
		m_edit_hexa.MoveWindow(CRect(m_r_edit_area.left, m_r_edit_area.top, m_r_edit_area.left + hexa_width, m_r_edit_area.top + kEditH));

		const int cellW = (m_r_edit_area.Width() - hexa_width - 4 * kCellGap) / 4;

		for (int i = 0; i < 4; i++)
		{
			const int x0 = m_r_edit_area.left + hexa_width + kCellGap + i * (cellW + kCellGap);
			m_edit_argb[i].MoveWindow(CRect(x0, m_r_edit_area.top, x0 + cellW, m_r_edit_area.top + kEditH));
		}

		for (int i = 0; i < 3; i++)
		{
			const int x0 = hsvX0 + i * (cellW + kCellGap);
			m_edit_hsl[i].MoveWindow(CRect(x0, hsvTop, x0 + cellW, hsvTop + kEditH));
		}

		m_edit_color_name.MoveWindow(m_r_color_name);
	}

	// ── 다이얼로그 크기를 콘텐츠에 맞게 자동 조정 ───────────────────────
	{
		const int client_h = m_r_edit_hsv_area.bottom + m_margin;
		const DWORD wStyle = (DWORD)::GetWindowLong(m_hWnd, GWL_STYLE);
		const DWORD wExStyle = (DWORD)::GetWindowLong(m_hWnd, GWL_EXSTYLE);
		CRect wr(0, 0, width, client_h);
		::AdjustWindowRectEx(&wr, wStyle, FALSE, wExStyle);
		SetWindowPos(nullptr, 0, 0, wr.Width(), wr.Height(),
			SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);
	}

	// ── Alpha 슬라이더 생성 or 위치 갱신 ─────────────────────────────────
	if (!m_slider_alpha.GetSafeHwnd())
	{
		m_slider_alpha.Create(
			WS_CHILD | WS_VISIBLE | TBS_HORZ | TBS_NOTICKS,
			m_r_slider_alpha, this, IDC_SLIDER_ALPHA
		);
		m_slider_alpha.set_style(CSCSliderCtrl::style_thumb_round_alpha);
		m_slider_alpha.set_range(0, 255);
		m_slider_alpha.set_back_color(Gdiplus::Color::White);
		m_slider_alpha.set_text_style(CSCSliderCtrl::text_style_none);
		m_slider_alpha.set_pos(255);
	}
	else
	{
		m_slider_alpha.MoveWindow(m_r_slider_alpha);
	}

	// ── Hue 슬라이더 생성 or 위치 갱신 ──────────────────────────────────
	if (!m_slider_hue.GetSafeHwnd())
	{
		m_slider_hue.Create(
			WS_CHILD | WS_VISIBLE | TBS_HORZ | TBS_NOTICKS,
			m_r_slider_hue, this, IDC_SLIDER_HUE
		);
		m_slider_hue.set_style(CSCSliderCtrl::style_thumb_round_hue);
		m_slider_hue.set_range(0, 360);
		m_slider_hue.set_back_color(Gdiplus::Color::White);
		m_slider_hue.set_text_style(CSCSliderCtrl::text_style_none);
		m_slider_hue.set_pos(0);
	}
	else
	{
		m_slider_hue.MoveWindow(m_r_slider_hue);
	}

	// ── Light(밝기) 슬라이더 생성 or 위치 갱신 ───────────────────────────
	if (!m_slider_light.GetSafeHwnd())
	{
		m_slider_light.Create(
			WS_CHILD | WS_VISIBLE | TBS_HORZ | TBS_NOTICKS,
			m_r_slider_value, this, IDC_SLIDER_VALUE
		);
		m_slider_light.set_style(CSCSliderCtrl::style_thumb_round_gradient);
		m_slider_light.set_range(0, 100);
		m_slider_light.set_back_color(Gdiplus::Color::White);
		m_slider_light.set_text_style(CSCSliderCtrl::text_style_none);
		m_slider_light.set_inactive_color(Gdiplus::Color(180, 180, 180));
		m_slider_light.set_active_color(Gdiplus::Color(225, 225, 225));
		m_slider_light.set_pos(50);
	}
	else
	{
		m_slider_light.MoveWindow(m_r_slider_value);
	}

	//시작 시 ARGB edit에 포커스가 표시되어 보기 안좋으므로 m_slider_alpha로 포커스 강제 변경하여 시작.
	m_slider_alpha.SetFocus();

	RestoreWindowPosition(AfxGetApp(), this, _T("color picker"));

	m_initialized = true;
}

// m_sel_color → 편집 컨트롤에 반영 (무한 루프 방지 플래그 포함)
void CSCColorPicker::sync_edits()
{
	if (!m_edit_hexa.GetSafeHwnd())
		return;

	if (m_edit_syncing)
		return;
	m_edit_syncing = true;

	auto cr = m_sel_color;
	CString newText, curText;

	// Hex 편집: 현재 m_hex_format에 따라 바이트 순서 변경
	switch (m_hex_format)
	{
		case HexFormat::ARGB: newText.Format(_T("#%02X%02X%02X%02X"), cr.GetAlpha(), cr.GetRed(), cr.GetGreen(), cr.GetBlue()); break;
		case HexFormat::ABGR: newText.Format(_T("#%02X%02X%02X%02X"), cr.GetAlpha(), cr.GetBlue(), cr.GetGreen(), cr.GetRed()); break;
		case HexFormat::RGBA: newText.Format(_T("#%02X%02X%02X%02X"), cr.GetRed(), cr.GetGreen(), cr.GetBlue(), cr.GetAlpha()); break;
		case HexFormat::BGRA: newText.Format(_T("#%02X%02X%02X%02X"), cr.GetBlue(), cr.GetGreen(), cr.GetRed(), cr.GetAlpha()); break;
	}

	m_edit_hexa.GetWindowText(curText);
	if (newText != curText)
		m_edit_hexa.SetWindowText(newText);

	int vals[4];
	switch (m_hex_format)
	{
		case HexFormat::ARGB: vals[0] = cr.GetAlpha(); vals[1] = cr.GetRed();   vals[2] = cr.GetGreen(); vals[3] = cr.GetBlue();  break;
		case HexFormat::ABGR: vals[0] = cr.GetAlpha(); vals[1] = cr.GetBlue();  vals[2] = cr.GetGreen(); vals[3] = cr.GetRed();   break;
		case HexFormat::RGBA: vals[0] = cr.GetRed();   vals[1] = cr.GetGreen(); vals[2] = cr.GetBlue();  vals[3] = cr.GetAlpha(); break;
		case HexFormat::BGRA: vals[0] = cr.GetBlue();  vals[1] = cr.GetGreen(); vals[2] = cr.GetRed();   vals[3] = cr.GetAlpha(); break;
	}

	for (int i = 0; i < 4; i++)
	{
		newText.Format(_T("%d"), vals[i]);
		m_edit_argb[i].GetWindowText(curText);
		if (newText != curText)
			m_edit_argb[i].SetWindowText(newText);
	}

	// ── H / S / L ─────────────────────────────────────────────────────────
	if (m_edit_hsl[0].GetSafeHwnd())
	{
		float h, s, l;
		gcolor_to_hsl(cr, h, s, l);
		const int hsl_vals[3] = {
			(int)roundf(h),
			(int)roundf(s * 100.f),
			(int)roundf(l * 100.f)
		};
		for (int i = 0; i < 3; i++)
		{
			newText.Format(_T("%d"), hsl_vals[i]);
			m_edit_hsl[i].GetWindowText(curText);
			if (newText != curText)
				m_edit_hsl[i].SetWindowText(newText);
		}
	}

	// ── Color Name ────────────────────────────────────────────────────
	if (m_edit_color_name.GetSafeHwnd())
	{
		// alpha를 무시하고 RGB만으로 이름 검색 (alpha != 255여도 RGB가 일치하면 exact)
		Gdiplus::Color opaque(255, cr.GetR(), cr.GetG(), cr.GetB());
		std::string name = CSCColorList::get_color_name(opaque);

		m_color_name_near = (name.find("near : ") == 0);
		if (m_color_name_near)
			name = name.substr(7);	// strip "near : " prefix

		CString wname(name.c_str());
		m_edit_color_name.GetWindowText(curText);
		if (wname.CompareNoCase(curText) != 0)
			m_edit_color_name.SetWindowText(wname);
		InvalidateRect(&m_r_color_name, FALSE);
	}

	// ── modeless 모드: 색상 변경 시 parent에게 알림 ──────────────────
	if (!m_as_modal && m_initialized)
	{
		if (m_parent && ::IsWindow(m_parent->m_hWnd))
		{
			CSCColorPickerMessage msg(this, m_sel_color);
			m_parent->SendMessage(Message_CSCColorPicker, (WPARAM)&msg,	(LPARAM)this);
		}
	}

	m_edit_syncing = false;
}

// ── apply_edit_to_color() ──────────────────────────────────────
// 편집 컨트롤 → m_sel_color 반영 후 UI 갱신
void CSCColorPicker::apply_edit_to_color()
{
	if (!m_edit_argb[0].GetSafeHwnd())
		return;

	auto clamp = [](int v) -> BYTE { return (BYTE)max(0, min(255, v)); };

	CString s[4];
	for (int i = 0; i < 4; i++)
		m_edit_argb[i].GetWindowText(s[i]);

	const BYTE v0 = clamp(_ttoi(s[0]));
	const BYTE v1 = clamp(_ttoi(s[1]));
	const BYTE v2 = clamp(_ttoi(s[2]));
	const BYTE v3 = clamp(_ttoi(s[3]));

	BYTE a, r, g, b;
	switch (m_hex_format)
	{
		case HexFormat::ARGB: a = v0; r = v1; g = v2; b = v3; break;
		case HexFormat::ABGR: a = v0; b = v1; g = v2; r = v3; break;
		case HexFormat::RGBA: r = v0; g = v1; b = v2; a = v3; break;
		case HexFormat::BGRA: b = v0; g = v1; r = v2; a = v3; break;
		default:              a = v0; r = v1; g = v2; b = v3; break;
	}

	m_sel_color = Gdiplus::Color(a, r, g, b);

	// 편집 → HSL 추출 후 슬라이더 동기화
	gcolor_to_hsl(m_sel_color, m_hue, m_sat, m_light);
	update_slider_alpha();
	update_slider_hue();
	update_slider_light();
	sync_edits();
	Invalidate(FALSE);
}

// ── OnEnChangeHaxa() ───────────────────────────────────────────
// Hex 편집 변경 → m_sel_color 파싱 반영
void CSCColorPicker::OnEnChangeHexa()
{
	if (m_edit_syncing)
		return;

	if (!m_edit_hexa.GetSafeHwnd())
		return;

	CString text;
	m_edit_hexa.GetWindowText(text);
	if (text.GetLength() == 9 && text[0] == '#')
		text = text.Mid(1);

	DWORD raw = 0;
	if (_stscanf_s(text, _T("%8X"), &raw) != 1)
		return;

	// 8자리 Hex를 4바이트로 분해 후 형식에 따라 ARGB 재조합
	const BYTE b0 = (raw >> 24) & 0xFF;
	const BYTE b1 = (raw >> 16) & 0xFF;
	const BYTE b2 = (raw >> 8) & 0xFF;
	const BYTE b3 = (raw) & 0xFF;

	BYTE a, r, g, b;
	switch (m_hex_format)
	{
		case HexFormat::ARGB: a = b0; r = b1; g = b2; b = b3; break;
		case HexFormat::ABGR: a = b0; b = b1; g = b2; r = b3; break;
		case HexFormat::RGBA: r = b0; g = b1; b = b2; a = b3; break;
		case HexFormat::BGRA: b = b0; g = b1; r = b2; a = b3; break;
		default:              a = b0; r = b1; g = b2; b = b3; break;
	}

	m_sel_color = Gdiplus::Color(a, r, g, b);

	gcolor_to_hsl(m_sel_color, m_hue, m_sat, m_light);
	update_slider_alpha();
	update_slider_hue();
	update_slider_light();
	sync_edits();
	Invalidate(FALSE);
}

// ── OnEnChangeArgb() ───────────────────────────────────────────
// ARGB 개별 편집 변경 → m_sel_color 반영
void CSCColorPicker::OnEnChangeArgb()
{
	if (m_edit_syncing)	// sync_edits()가 진행 중이면 무시 (재진입 차단)
		return;

	apply_edit_to_color();
}

void CSCColorPicker::OnEnChangeHsl()
{
	if (m_edit_syncing)
		return;

	if (!m_edit_hsl[0].GetSafeHwnd())
		return;

	CString s[3];
	for (int i = 0; i < 3; i++)
		m_edit_hsl[i].GetWindowText(s[i]);

	const float h = max(0.f, min(360.f, (float)_ttof(s[0])));
	const float sat = max(0.f, min(100.f, (float)_ttof(s[1]))) / 100.f;
	const float lit = max(0.f, min(100.f, (float)_ttof(s[2]))) / 100.f;

	m_hue = h;
	m_sat = sat;
	m_light = lit;

	const BYTE           a = m_sel_color.GetA();
	const Gdiplus::Color rgb = hsl_to_gcolor(m_hue, m_sat, m_light);
	m_sel_color = Gdiplus::Color(a, rgb.GetR(), rgb.GetG(), rgb.GetB());

	update_slider_alpha();
	update_slider_hue();
	update_slider_light();
	sync_edits();
	Invalidate(FALSE);
}

// ── 유일한 좌표 계산 함수 ────────────────────────────────
// hit_test·draw_* 모두 이 함수 하나에서 좌표를 얻음
Gdiplus::PointF CSCColorPicker::get_cell_center(const HitTarget& t) const
{
	if (t.area == HitArea::Palette)
	{
		return {
			m_r_palette.left + m_margin + t.col * m_cell + m_cell * 0.5f,
			m_r_palette.top + m_margin + t.row * m_cell + m_cell * 0.5f
		};
	}
	if (t.area == HitArea::Recent)
	{
		// ← 스크롤 오프셋 적용: 화면상 열 = idx - m_recent_scroll
		const int col = t.idx - m_recent_scroll;
		return {
			m_r_palette.left + m_margin + col * m_cell + m_cell * 0.5f,
			static_cast<float>(m_r_palette.bottom) + m_margin + m_cell * 0.5f
		};
	}
	if (t.area == HitArea::Button)
	{
		// recent 개수와 무관하게 항상 RECENT_DISPLAY_COLS(8) 이후 고정
		return {
			m_r_palette.left + m_margin + (RECENT_DISPLAY_COLS + t.idx) * m_cell + m_cell * 0.5f,
			static_cast<float>(m_r_palette.bottom) + m_margin + m_cell * 0.5f
		};
	}

	return { 0.f, 0.f };
}

void CSCColorPicker::draw_palette(Gdiplus::Graphics& g) const
{
	Gdiplus::Pen borderPen(Gdiplus::Color(50, 0, 0, 0), 1.0f);

	for (int row = 0; row < PALETTE_ROWS; ++row)
	{
		for (int col = 0; col < PALETTE_TOTAL_COLS; ++col)
		{
			const Gdiplus::Color   cr = get_color_at(col, row);
			const Gdiplus::PointF  center = get_cell_center({ HitArea::Palette, col, row, -1 });

			Gdiplus::SolidBrush brush(cr);
			g.FillEllipse(&brush, center.X - m_radius, center.Y - m_radius, m_radius * 2.f, m_radius * 2.f);
			g.DrawEllipse(&borderPen, center.X - m_radius, center.Y - m_radius, m_radius * 2.f, m_radius * 2.f);
		}
	}
}

// ── 체커보드 배경 + alpha 합성 원 그리기 ──────────────────
// alpha < 255인 색상은 체커보드 위에 반투명으로 합성
void CSCColorPicker::draw_color_circle(Gdiplus::Graphics& g, Gdiplus::PointF center, float r, Gdiplus::Color cr) const
{
	if (cr.GetA() < 255)
	{
		Gdiplus::TextureBrush tb(CSCGdiplusBitmap::checker_bmp(5), Gdiplus::WrapModeTile);
		g.FillEllipse(&tb, center.X - r, center.Y - r, r * 2.f, r * 2.f);
	}
	Gdiplus::SolidBrush brush(cr);
	g.FillEllipse(&brush, center.X - r, center.Y - r, r * 2.f, r * 2.f);
}

void CSCColorPicker::draw_overlays(Gdiplus::Graphics& g) const
{
	auto is_recent_visible = [&](const HitTarget& t) -> bool {
		if (t.area != HitArea::Recent)
			return true;
		const int col = t.idx - m_recent_scroll;
		return col >= 0 && col < RECENT_DISPLAY_COLS;
		};

	if (m_hover.is_valid() && m_hover.area != HitArea::Button && !(m_hover == m_sel))
	{
		if (is_recent_visible(m_hover))
			draw_hover_circle(g, m_hover);
	}

	if (m_sel.is_valid() && m_sel.area != HitArea::Button)
	{
		if (is_recent_visible(m_sel))
			draw_selected_mark(g, m_sel);
	}
}

// ── 선택 색상 미리보기 (RoundRect) ───────────────────────
void CSCColorPicker::draw_color_preview(Gdiplus::Graphics& g) const
{
	if (m_r_preview.IsRectEmpty())
		return;

	const Gdiplus::RectF rf(
		static_cast<float>(m_r_preview.left),
		static_cast<float>(m_r_preview.top),
		static_cast<float>(m_r_preview.Width()),
		static_cast<float>(m_r_preview.Height())
	);

	const bool          has_sel = m_sel.is_valid();
	const Gdiplus::Color cr = has_sel ? m_sel_color
		: Gdiplus::Color(255, 210, 210, 210);

	// alpha < 255 이면 체커보드 배경 먼저 (원형으로 클리핑)
	if (has_sel && cr.GetA() < 255)
	{
		Gdiplus::GraphicsPath clip_path;
		clip_path.AddEllipse(rf);

		g.SetClip(&clip_path);
		Gdiplus::TextureBrush tb(CSCGdiplusBitmap::checker_bmp(5), Gdiplus::WrapModeTile);
		g.FillRectangle(&tb, rf.X - 1.f, rf.Y - 1.f, rf.Width + 2.f, rf.Height + 2.f);
		g.ResetClip();
	}

	// 실제 색상 채우기
	Gdiplus::SolidBrush brush(cr);
	g.FillEllipse(&brush, rf);

	// 테두리
	Gdiplus::Pen borderPen(Gdiplus::Color(80, 0, 0, 0), 1.f);
	g.DrawEllipse(&borderPen, rf);
}

// ── (col, row) → 색상 ────────────────────────────────────
Gdiplus::Color CSCColorPicker::get_color_at(int col, int row) const
{
	if (row == 0)   // 무채색 행 (col 0=검정 → col 9=흰색)
	{
		const float t = (PALETTE_TOTAL_COLS > 1)
			? static_cast<float>(col) / (PALETTE_TOTAL_COLS - 1) : 0.f;
		const BYTE gray = static_cast<BYTE>(t * 255.f + 0.5f);
		return Gdiplus::Color(gray, gray, gray);
	}
	// row 1~6 → m_sv_rows[row-1]
	return hsv_to_gcolor(m_hues[col], m_sv_rows[row - 1].s, m_sv_rows[row - 1].v);
}


// ── HitTarget → 색상 ─────────────────────────────────────
Gdiplus::Color CSCColorPicker::get_color(const HitTarget& t) const
{
	if (t.area == HitArea::Palette)
		return get_color_at(t.col, t.row);

	if (t.area == HitArea::Recent &&
		t.idx >= 0 && t.idx < (int)m_recent_colors.size())
		return m_recent_colors[t.idx];

	return Gdiplus::Color::Transparent;
}

// ── 팔레트 + 최근 색상 통합 히트 테스트 ─────────────────
bool CSCColorPicker::hit_test(CPoint pt, HitTarget& out) const
{
	// ① 팔레트
	for (int row = 0; row < PALETTE_ROWS; ++row)
	{
		for (int col = 0; col < PALETTE_TOTAL_COLS; ++col)
		{
			HitTarget t{ HitArea::Palette, col, row, -1 };
			const Gdiplus::PointF c = get_cell_center(t);
			const float dx = static_cast<float>(pt.x) - c.X;
			const float dy = static_cast<float>(pt.y) - c.Y;
			if (dx * dx + dy * dy <= m_radius * m_radius)
			{
				out = t;
				return true;
			}
		}
	}

	if (!m_r_palette.IsRectEmpty())
	{
		// ② 최근 색상 — 가시 범위만
		const int vis_start = m_recent_scroll;
		const int vis_end = min((int)m_recent_colors.size(), m_recent_scroll + RECENT_DISPLAY_COLS);
		for (int i = vis_start; i < vis_end; ++i)
		{
			HitTarget t{ HitArea::Recent, -1, -1, i };
			const Gdiplus::PointF c = get_cell_center(t);
			const float dx = static_cast<float>(pt.x) - c.X;
			const float dy = static_cast<float>(pt.y) - c.Y;
			if (dx * dx + dy * dy <= m_radius * m_radius)
			{
				out = t;
				return true;
			}
		}

		// ③ 버튼
		for (int i = 0; i < PALETTE_BTN_COUNT; ++i)
		{
			HitTarget t{ HitArea::Button, -1, -1, i };
			const Gdiplus::PointF c = get_cell_center(t);
			const float dx = static_cast<float>(pt.x) - c.X;
			const float dy = static_cast<float>(pt.y) - c.Y;
			if (dx * dx + dy * dy <= m_radius * m_radius)
			{
				out = t;
				return true;
			}
		}
	}

	out = {};
	return false;
}

void CSCColorPicker::draw_buttons(Gdiplus::Graphics& g) const
{
	//사용자 정의색 추가 버튼
	{
		const HitTarget       t{ HitArea::Button, -1, -1, BTN_ADD_IDX };
		const Gdiplus::PointF c = get_cell_center(t);
		const bool            is_hover = (m_hover == t);
		const float           r = is_hover ? m_radius + 1.5f : m_radius;

		// 배경 원 (hover 시 더 진한 회색)
		const BYTE bg = is_hover ? 210 : 238;
		const BYTE border = is_hover ? 150 : 180;
		Gdiplus::SolidBrush bgBrush(Gdiplus::Color(bg, bg, bg));
		Gdiplus::Pen        borderPen(Gdiplus::Color(border, border, border), 1.0f);
		g.FillEllipse(&bgBrush, c.X - r, c.Y - r, r * 2.f, r * 2.f);
		g.DrawEllipse(&borderPen, c.X - r, c.Y - r, r * 2.f, r * 2.f);

		// "+" 기호
		const float arm = m_radius * 0.46f;
		Gdiplus::Pen plus(Gdiplus::Color(32, 128, 212), m_radius * 0.18f);
		plus.SetStartCap(Gdiplus::LineCapRound);
		plus.SetEndCap(Gdiplus::LineCapRound);
		g.DrawLine(&plus, c.X - arm, c.Y, c.X + arm, c.Y);		// 가로
		g.DrawLine(&plus, c.X, c.Y - arm, c.X, c.Y + arm);	// 세로
	}

	//스포이드 버튼
	{
		const HitTarget       t{ HitArea::Button, -1, -1, BTN_DROPPER_IDX };
		const Gdiplus::PointF c = get_cell_center(t);
		const bool            is_hover = (m_hover == t);
		const float           r = is_hover ? m_radius + 1.5f : m_radius;

		// 배경 원 (hover 시 더 진한 회색)
		const BYTE bg = is_hover ? 210 : 238;
		const BYTE border = is_hover ? 150 : 180;
		Gdiplus::SolidBrush bgBrush(Gdiplus::Color(bg, bg, bg));
		Gdiplus::Pen        borderPen(Gdiplus::Color(border, border, border), 1.0f);
		g.FillEllipse(&bgBrush, c.X - r, c.Y - r, r * 2.f, r * 2.f);
		g.DrawEllipse(&borderPen, c.X - r, c.Y - r, r * 2.f, r * 2.f);

		// ── 스포이드 아이콘 (45° 대각선, 3파트: 팁·몸통·벌브) ──
		const float u = m_radius * 0.22f;

		// 색상 분리: 몸통 = "+" 기호와 동일, 팁·벌브 = 더 진한색
		const Gdiplus::Color bodyColor(128, 192, 212);
		const Gdiplus::Color accentColor(64, 128, 255);

		// 방향 벡터: 좌하→우상 45°
		const float dx = 0.7071f;
		const float dy = -0.7071f;
		const float nx = -dy;
		const float ny = dx;

		const float ox = c.X - dx * 0.3f * u;
		const float oy = c.Y - dy * 0.3f * u;

		// ① 뾰족한 팁 (삼각형) — 진한색
		const Gdiplus::PointF tip = { ox - dx * 3.2f * u, oy - dy * 3.2f * u };
		const Gdiplus::PointF tipL = { ox - dx * 1.6f * u + nx * 0.5f * u,
										oy - dy * 1.6f * u + ny * 0.5f * u };
		const Gdiplus::PointF tipR = { ox - dx * 1.6f * u - nx * 0.5f * u,
										oy - dy * 1.6f * u - ny * 0.5f * u };
		Gdiplus::SolidBrush accentBrush(accentColor);
		Gdiplus::PointF tipPts[3] = { tip, tipL, tipR };
		g.FillPolygon(&accentBrush, tipPts, 3);

		// ② 몸통 (두꺼운 둥근 선) — "+"와 동일한 색
		const Gdiplus::PointF bodyBot = { ox - dx * 1.4f * u, oy - dy * 1.4f * u };
		const Gdiplus::PointF bodyTop = { ox + dx * 1.8f * u, oy + dy * 1.8f * u };
		Gdiplus::Pen bodyPen(bodyColor, u * 1.3f);
		bodyPen.SetStartCap(Gdiplus::LineCapRound);
		bodyPen.SetEndCap(Gdiplus::LineCapRound);
		g.DrawLine(&bodyPen, bodyBot, bodyTop);

		// ③ 벌브 (상단 채워진 원) — 진한색
		const float bulbR = u * 1.3f;
		const float bx = ox + dx * 2.6f * u;
		const float by = oy + dy * 2.6f * u;
		g.FillEllipse(&accentBrush, bx - bulbR, by - bulbR, bulbR * 2.f, bulbR * 2.f);
	}
}

// ── "+" 클릭: 현재 선택 색상을 recent에 즉시 추가 ────────
void CSCColorPicker::on_btn_add_clicked()
{
	if (!m_sel.is_valid())
		return;

	const Gdiplus::Color cr = m_sel_color;

	for (auto it = m_recent_colors.begin(); it != m_recent_colors.end(); )
	{
		if (it->GetValue() == cr.GetValue())
			it = m_recent_colors.erase(it);
		else
			++it;
	}

	m_recent_colors.insert(m_recent_colors.begin(), cr);
	if ((int)m_recent_colors.size() > MAX_RECENT_COLORS)
		m_recent_colors.resize(MAX_RECENT_COLORS);

	m_recent_scroll = 0;				// ← 새로 추가된 항목이 보이도록 맨 앞으로
	m_sel = { HitArea::Recent, -1, -1, 0 };
	m_sel_color = cr;

	update_slider_alpha();
	Invalidate(FALSE);
}

#include "SCDropperDlg.h"

void CSCColorPicker::on_btn_dropper_clicked()
{
	//shift를 누르고 실행하면 자기 자신, 즉 color picker창을 숨기지 않고 실행시킨다.
	if (!IsShiftPressed())
		ShowWindow(SW_HIDE);

	//아래 주석처리된 코드블록처럼 이 함수내에서 끝까지 실행하는 방식으로 구현했었으나
	//메시지 펌프, SetWindowDisplayAffinity까지 써봐도 여전히 ColorPicker창이 완전히 hide됨이 보장되지 않았다.
	//간단히 타이머로 500ms ~ 1초후에 동작되도록 수정함.
	//물론 타이머로 동작한다고 해서 hide를 반드시 보장하는 것은 아니다.
	//다만 500ms면 hide하고 창이 화면에서 사라지는 시간으로는 충분하다는 가정으로 결정한 것임.
	SetTimer(timer_run_dropper_wnd, 500, NULL);
	return;
	/*
	MSG msg = {};

	// ① 컬러피커 숨김 (확대창 화면에 찍히지 않도록)
	// ShowWindow 전에 호출 - DWM에게 이 창을 캡처에서 제외하라고 지시
	// Windows 10 2004(build 19041) 이상에서 지원
	::SetWindowDisplayAffinity(m_hWnd, WDA_EXCLUDEFROMCAPTURE);  // 0x11

	ShowWindow(SW_HIDE);

	// (2) 메시지 펌프: 윈도우 매니저가 SW_HIDE를 완전히 처리하고
	//     DWM에 "합성 표면 제거" 통보를 전달할 수 있도록 대기
	while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
	{
		if (msg.message == WM_QUIT)
		{
			PostQuitMessage(static_cast<int>(msg.wParam));
			return;
		}
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	// (3) DwmFlush 2회:
	DwmFlush();	//1회차 - DWM이 제거 통보를 수신하고 새 프레임 합성 시작
	DwmFlush();	//2회차 - 컬러피커가 제외된 프레임이 실제 표시 완료됨을 보장

	// ② CSCDropperDlg 생성 (CWnd-derived: DoModal() 대신 create() + 메시지 루프)
	CSCDropperDlg dlg;
	dlg.create(this);

	//위에서 SW_HIDE 후 DwmFlush()를 2회 해줘도 여전히 color picker창이 숨겨지지 않은 채 화면이 캡처되어 SetWindowDisplayAffinity()를 사용함.
	if (!dlg.GetSafeHwnd())    // create() 내부 CreateEx 실패 시
	{
		::SetWindowDisplayAffinity(m_hWnd, WDA_NONE);
		ShowWindow(SW_SHOW);
		return;
	}

	// ESC 키 수신을 위해 포커스 명시적 지정
	dlg.SetForegroundWindow();
	dlg.SetFocus();

	// ③ 메시지 루프 — dlg가 DestroyWindow()로 닫힐 때까지 대기
	//    (LButtonDown 또는 VK_ESCAPE → KillTimer + DestroyWindow → GetSafeHwnd() == NULL)
	bool quit_posted = false;
	msg = {};
	while (dlg.GetSafeHwnd() != NULL)
	{
		while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
		{
			if (msg.message == WM_QUIT)
			{
				quit_posted = true;
				break;
			}
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		if (quit_posted) break;
		if (dlg.GetSafeHwnd() != NULL)
			WaitMessage();
	}
	if (quit_posted)
		PostQuitMessage(static_cast<int>(msg.wParam));

	// ④ 컬러피커 복원
	// 드로퍼 종료 후 복원
	::SetWindowDisplayAffinity(m_hWnd, WDA_NONE);
	ShowWindow(SW_SHOW);
	SetForegroundWindow();

	// ⑤ 색상 반영 (is_picked() == true: 클릭으로 확정, false: ESC 취소)
	if (dlg.is_picked())
	{
		m_sel_color = dlg.get_picked_color();

		// recent에 추가 후 m_sel을 recent[0]으로 지정 (미리보기·슬라이더 활성화)
		for (auto it = m_recent_colors.begin(); it != m_recent_colors.end(); )
		{
			if (it->GetValue() == m_sel_color.GetValue())
				it = m_recent_colors.erase(it);
			else
				++it;
		}
		m_recent_colors.insert(m_recent_colors.begin(), m_sel_color);
		if ((int)m_recent_colors.size() > MAX_RECENT_COLORS)
			m_recent_colors.resize(MAX_RECENT_COLORS);

		m_recent_scroll = 0;
		m_sel = { HitArea::Recent, -1, -1, 0 };

		gcolor_to_hsl(m_sel_color, m_hue, m_sat, m_light);
		update_slider_alpha();
		update_slider_hue();
		update_slider_light();
		sync_edits();
		Invalidate(FALSE);
	}
	*/
}

// CSCColorPicker 메시지 처리기
void CSCColorPicker::OnBnClickedOk()
{
	m_response = IDOK;

	if (!m_as_modal)
		ShowWindow(SW_HIDE);

	//선택 색상을 레지스트리에 저장
	if (m_sel.is_valid())
		save_recent_color(m_sel_color);
}

void CSCColorPicker::OnBnClickedCancel()
{
	m_response = IDCANCEL;

	if (!m_as_modal)
		ShowWindow(SW_HIDE);
}

INT_PTR CSCColorPicker::DoModal(CWnd* parent, Gdiplus::Color cr_selected, CString title)
{
	//return CDialog::DoModal();

	// TODO: 여기에 특수화된 코드를 추가 및/또는 기본 클래스를 호출합니다.
	if (parent == nullptr)
		return -1;

	m_parent = parent;
	m_as_modal = true;

	if (title.IsEmpty())
		title = _T("Color Picker");

	if (m_hWnd == nullptr)
		create(m_parent, title, m_as_modal);

	load_recent_colors();


	if (cr_selected.GetValue() != Gdiplus::Color::Transparent)
	{
		m_sel_color = cr_selected;

		const HitTarget found = find_color(cr_selected);
		if (found.is_valid())
		{
			m_sel = found;
			if (found.area == HitArea::Recent)
			{
				// ARGB 완전 일치 → 저장된 alpha 그대로 사용
				m_sel_color = m_recent_colors[found.idx];
			}
			else	// HitArea::Palette
			{
				// RGB 일치 → 팔레트 RGB + 전달된 alpha 유지
				const Gdiplus::Color palette_cr = get_color_at(found.col, found.row);
			}
		}
	}
	else if (!m_recent_colors.empty())
	{
		// cr_selected == Transparent → recent[0]을 기본 선택
		m_sel = { HitArea::Recent, -1, -1, 0 };
		m_sel_color = m_recent_colors[0];
	}

	update_slider_alpha();	// ← m_sel 확정 후 슬라이더 동기화
	gcolor_to_hsl(m_sel_color, m_hue, m_sat, m_light);
	update_slider_hue();
	update_slider_light();
	sync_edits();

	SetWindowText(title);
	//CenterWindow(m_parent);
	ShowWindow(SW_SHOW);
	Invalidate(FALSE);

	m_slider_alpha.SetFocus();

	MSG		stmsg;

	//for Modal Dialog
	m_parent->EnableWindow(FALSE);

	while (m_response < 0)
	{
		while (PeekMessage(&stmsg, NULL, 0, 0, PM_REMOVE))
		{
			if (!CWnd::WalkPreTranslateTree(GetSafeHwnd(), &stmsg))
			{
				TranslateMessage(&stmsg);
				DispatchMessage(&stmsg);
			}

			/*
			//TRACE("GetFocus() = %p, tick = %d\n", GetFocus(), GetTickCount());
			if (stmsg.message == WM_KEYDOWN || stmsg.message == WM_KEYUP)
			{
				GetFocus()->PreTranslateMessage(&stmsg);
			}
			else
			{
				TranslateMessage(&stmsg);
				DispatchMessage(&stmsg);
			}
			*/
		}

		// 메시지 큐가 비었을 때 CPU를 양보 → UIAutomation 스레드가 작업을 완료할 수 있음
		if (m_response < 0)
			WaitMessage();
	}

	m_parent->EnableWindow(TRUE);
	m_parent->SetForegroundWindow();

	// EndDialog 대신 DestroyWindow 호출:
	// EndDialog는 SW_HIDE만 수행하고 m_hWnd를 NULL로 만들지 않음.
	// DestroyWindow → WM_NCDESTROY → CWnd::OnNcDestroy → m_hWnd = NULL
	// → ~CDialog()에서 이중 파괴 방지 → UIAutomation 예외 해소
	ShowWindow(SW_HIDE);
	DestroyWindow();

	//ok, cancel 구분 없이 최근 색상 목록 저장
	//save_recent_color(m_sel_color);

	return m_response;
}


BOOL CSCColorPicker::OnInitDialog()
{
	CDialog::OnInitDialog();

	// TODO:  여기에 추가 초기화 작업을 추가합니다.

	//create() → CreateEx()  ← WM_INITDIALOG 발생하지 않음.
	//load_recent_colors();

	return TRUE;  // return TRUE unless you set the focus to a control
	// 예외: OCX 속성 페이지는 FALSE를 반환해야 합니다.
}

BOOL CSCColorPicker::PreTranslateMessage(MSG* pMsg)
{
	// TODO: 여기에 특수화된 코드를 추가 및/또는 기본 클래스를 호출합니다.
	//TRACE(_T("CSCColorPicker::PreTranslateMessage\n"));
	return CDialog::PreTranslateMessage(pMsg);
}

void CSCColorPicker::load_recent_colors()
{
	// ── Hex 표시 형식 복원 ────────────────────────────────────────────────
	const int fmt = AfxGetApp()->GetProfileInt(_T("setting\\color picker"), _T("hex_format"), (int)HexFormat::ARGB);
	m_hex_format = (fmt >= (int)HexFormat::ARGB && fmt <= (int)HexFormat::BGRA)
		? static_cast<HexFormat>(fmt)
		: HexFormat::ARGB;	// 범위 벗어난 값은 기본값(ARGB)으로 복원

	// ── 툴팁 표시 설정 복원 ──────────────────────────────────────────────
	m_show_tooltip = AfxGetApp()->GetProfileInt(
		_T("setting\\color picker"), _T("show_tooltip"), false) != false;  // 기본값: false

	m_recent_colors.clear();
	const int count = min(
		AfxGetApp()->GetProfileInt(_T("setting\\color picker"), _T("count"), 0),
		MAX_RECENT_COLORS);

	for (int i = 0; i < count; ++i)
	{
		CString key;
		key.Format(_T("color_%d"), i);
		CString val = AfxGetApp()->GetProfileString(_T("setting\\color picker"), key, _T(""));
		if (val.GetLength() == 8)
		{
			DWORD argb = _tcstoul(val, nullptr, 16);
			m_recent_colors.push_back(Gdiplus::Color(argb));
		}
	}

	m_recent_scroll = 0;				// ← 항상 맨 앞부터 표시
}

// ── 최근 색상 저장 (중복 제거 → 맨 앞 삽입 → 레지스트리 기록) ──
void CSCColorPicker::save_recent_color(Gdiplus::Color cr)
{
	// alpha 포함 전체 ARGB 그대로 저장

	// ARGB 완전 일치 중복 제거 (alpha 포함 비교)
	for (auto it = m_recent_colors.begin(); it != m_recent_colors.end(); )
	{
		if (it->GetA() == cr.GetA() &&
			it->GetR() == cr.GetR() &&
			it->GetG() == cr.GetG() &&
			it->GetB() == cr.GetB())
			it = m_recent_colors.erase(it);
		else
			++it;
	}

	// 가장 최근 색을 맨 앞에
	m_recent_colors.insert(m_recent_colors.begin(), cr);

	// 최대 개수 제한
	if ((int)m_recent_colors.size() > MAX_RECENT_COLORS)
		m_recent_colors.resize(MAX_RECENT_COLORS);

	// 레지스트리에 기록
	AfxGetApp()->WriteProfileInt(_T("setting\\color picker"), _T("count"), (int)m_recent_colors.size());
	for (int i = 0; i < (int)m_recent_colors.size(); ++i)
	{
		CString key, val;
		key.Format(_T("color_%d"), i);
		val.Format(_T("%08X"), m_recent_colors[i].GetValue());	// GetValue() = ARGB 전체
		AfxGetApp()->WriteProfileString(_T("setting\\color picker"), key, val);
	}
}

void CSCColorPicker::draw_recent_colors(Gdiplus::Graphics& g) const
{
	if (m_recent_colors.empty())
		return;

	// ① 표시 영역 클리핑 (스크롤로 잘린 원이 넘치지 않도록)
	const float clip_x = m_r_palette.left + m_margin;
	const float clip_y = static_cast<float>(m_r_palette.bottom) + m_margin - m_radius - 1.f;
	const float clip_w = RECENT_DISPLAY_COLS * m_cell;
	const float clip_h = m_cell + 2.f * m_radius + 2.f;
	g.SetClip(Gdiplus::RectF(clip_x, clip_y, clip_w, clip_h));

	Gdiplus::Pen borderPen(Gdiplus::Color(50, 0, 0, 0), 1.0f);

	const int vis_start = m_recent_scroll;
	const int vis_end = min((int)m_recent_colors.size(), m_recent_scroll + RECENT_DISPLAY_COLS);

	for (int i = vis_start; i < vis_end; ++i)
	{
		const Gdiplus::PointF center = get_cell_center({ HitArea::Recent, -1, -1, i });
		const Gdiplus::Color  cr = m_recent_colors[i];

		draw_color_circle(g, center, m_radius, cr);
		g.DrawEllipse(&borderPen, center.X - m_radius, center.Y - m_radius, m_radius * 2.f, m_radius * 2.f);
	}

	g.ResetClip();

	// ② 스크롤 가능 인디케이터 ◄ ► (클립 해제 후 그림)
	const float row_cy = static_cast<float>(m_r_palette.bottom) + m_margin + m_cell * 0.5f;
	const float tri = m_radius * 0.28f;

	if (m_recent_scroll > 0)
	{
		// ◄ 왼쪽 — 첫 번째 보이는 원 색상 기준으로 대비색 결정
		Gdiplus::Color indColor = get_distinct_bw_color(m_recent_colors[m_recent_scroll]);
		Gdiplus::SolidBrush indBrush(Gdiplus::Color(180, indColor.GetR(), indColor.GetG(), indColor.GetB()));

		const float tx = clip_x + tri + 1.f;
		Gdiplus::PointF pts[3] = {
			{ tx + tri, row_cy - tri },
			{ tx + tri, row_cy + tri },
			{ tx,       row_cy       }
		};
		g.FillPolygon(&indBrush, pts, 3);
	}
	if (m_recent_scroll + RECENT_DISPLAY_COLS < (int)m_recent_colors.size())
	{
		// ► 오른쪽 — 마지막 보이는 원 색상 기준으로 대비색 결정
		int last_vis = m_recent_scroll + RECENT_DISPLAY_COLS - 1;
		Gdiplus::Color indColor = get_distinct_bw_color(m_recent_colors[last_vis]);
		Gdiplus::SolidBrush indBrush(Gdiplus::Color(180, indColor.GetR(), indColor.GetG(), indColor.GetB()));

		const float tx = clip_x + clip_w - tri - 1.f;
		Gdiplus::PointF pts[3] = {
			{ tx - tri, row_cy - tri },
			{ tx - tri, row_cy + tri },
			{ tx,       row_cy       }
		};
		g.FillPolygon(&indBrush, pts, 3);
	}
}

void CSCColorPicker::draw_hover_circle(Gdiplus::Graphics& g, const HitTarget& t) const
{
	const Gdiplus::PointF center = get_cell_center(t);
	const float           r = m_radius + 1.5f;
	const Gdiplus::Color  cr = get_color(t);

	draw_color_circle(g, center, r, cr);	// alpha < 255이면 체커보드도 함께

	Gdiplus::Pen borderPen(Gdiplus::Color(50, 0, 0, 0), 1.0f);
	g.DrawEllipse(&borderPen, center.X - r, center.Y - r, r * 2.f, r * 2.f);
}

void CSCColorPicker::draw_selected_mark(Gdiplus::Graphics& g, const HitTarget& t) const
{
	const Gdiplus::PointF center = get_cell_center(t);
	const float           cx = center.X;
	const float           cy = center.Y;
	const float           r = m_radius;
	const Gdiplus::Color  cr = get_color(t);

	// 알파를 고려해 흰색 배경과 합성된 실효 색상으로 밝기를 계산한다.
	// 알파가 낮을수록 실효 색상은 배경(흰색)에 가까워지므로
	// 체크 색상도 그에 맞게 어두운 색을 선택해야 구분된다.
	const float a = cr.GetA() / 255.0f;
	const BYTE R_eff = static_cast<BYTE>(cr.GetR() * a + 255.0f * (1.0f - a));
	const BYTE G_eff = static_cast<BYTE>(cr.GetG() * a + 255.0f * (1.0f - a));
	const BYTE B_eff = static_cast<BYTE>(cr.GetB() * a + 255.0f * (1.0f - a));
	const BYTE lum = static_cast<BYTE>(0.299f * R_eff + 0.587f * G_eff + 0.114f * B_eff);

	const Gdiplus::Color check_cr = (lum > 160)
		? Gdiplus::Color(230, 64, 64, 64)     // 밝은 배경 → 어두운 체크
		: Gdiplus::Color(230, 232, 232, 232);  // 어두운 배경 → 밝은 체크

	const Gdiplus::PointF pts[3] = {
		{ cx - r * 0.42f, cy + r * 0.02f },
		{ cx - r * 0.08f, cy + r * 0.38f },
		{ cx + r * 0.46f, cy - r * 0.32f },
	};

	Gdiplus::Pen pen(check_cr, r * 0.20f);
	pen.SetStartCap(Gdiplus::LineCapRound);
	pen.SetEndCap(Gdiplus::LineCapRound);
	pen.SetLineJoin(Gdiplus::LineJoinRound);
	g.DrawLines(&pen, pts, 3);
}

// ── 색상 검색: recent(ARGB 완전 일치) → palette(RGB 일치) ──
// 찾으면 해당 HitTarget, 없으면 is_valid()==false인 빈 HitTarget 반환
CSCColorPicker::HitTarget CSCColorPicker::find_color(Gdiplus::Color target) const
{
	// ① recent colors — ARGB 완전 일치 우선 (alpha까지 같은 색)
	for (int i = 0; i < (int)m_recent_colors.size(); ++i)
	{
		if (m_recent_colors[i].GetValue() == target.GetValue())
			return { HitArea::Recent, -1, -1, i };
	}

	// ② palette — RGB 일치 (alpha 무시; 팔레트 색은 항상 alpha=255)
	for (int row = 0; row < PALETTE_ROWS; ++row)
	{
		for (int col = 0; col < PALETTE_TOTAL_COLS; ++col)
		{
			const Gdiplus::Color cr = get_color_at(col, row);
			if (cr.GetR() == target.GetR() &&
				cr.GetG() == target.GetG() &&
				cr.GetB() == target.GetB())
				return { HitArea::Palette, col, row, -1 };
		}
	}

	return {};	// 어디에도 없음 → is_valid() == false
}

void CSCColorPicker::OnPaint()
{
	CPaintDC dc1(this); // device context for painting
	CRect rc;

	GetClientRect(rc);
	CMemoryDC dc(&dc1, &rc);
	dc.FillSolidRect(rc, white);

	if (m_cell == 0.f)		// calc_layout() 미호출 시 안전 가드
		return;

	Gdiplus::Graphics g(dc.GetSafeHdc());
	g.SetSmoothingMode(Gdiplus::SmoothingModeAntiAlias);
	g.SetPixelOffsetMode(Gdiplus::PixelOffsetModeHighQuality);

	draw_palette(g);		// ① 팔레트 원

	//구분선. palette와 recent colors 영역 구분
	draw_line(&dc, rc.left + 12, m_r_palette.bottom, rc.right - 12, m_r_palette.bottom, GRAY(212));

	draw_recent_colors(g);	// ③ 최근 색상 원
	draw_buttons(g);		// ④ "+" 및 스포이드 버튼 (hover 자체 처리)

	//구분선. recent colors와 preview + sliders 영역 구분
	draw_line(&dc, rc.left + 12, (float)m_r_preview.top - m_margin, rc.right - 12, (float)m_r_preview.top - m_margin, GRAY(212));

	draw_color_preview(g);	// ④ 선택 색상 미리보기 (RoundRect)

	draw_overlays(g);		// ⑤ 색상 셀 hover 확대원 + 체크마크

	Gdiplus::Font        labelFont(L"Consolas", 11.0f, Gdiplus::FontStyleBold, Gdiplus::UnitPixel);
	Gdiplus::SolidBrush  labelBrush(Gdiplus::Color(64, 64, 64));
	Gdiplus::StringFormat sf;
	sf.SetAlignment(Gdiplus::StringAlignmentCenter);
	sf.SetLineAlignment(Gdiplus::StringAlignmentNear);

	int cell_left = m_r_edit_area.left;
	Gdiplus::RectF label_rect;

	if (m_edit_hexa.GetSafeHwnd())
	{
		label_rect = Gdiplus::RectF((float)m_r_edit_area.left,
			(float)(m_r_edit_area.top - 16), (float)kHexaW, 24.f);
		g.DrawString(get_hex_format_label(), -1, &labelFont, label_rect, &sf, &labelBrush);
	}

	if (m_edit_argb[0].GetSafeHwnd())
	{
		const wchar_t* kLabels[4] = {};
		switch (m_hex_format)
		{
			case HexFormat::ARGB: kLabels[0] = L"A"; kLabels[1] = L"R"; kLabels[2] = L"G"; kLabels[3] = L"B"; break;
			case HexFormat::ABGR: kLabels[0] = L"A"; kLabels[1] = L"B"; kLabels[2] = L"G"; kLabels[3] = L"R"; break;
			case HexFormat::RGBA: kLabels[0] = L"R"; kLabels[1] = L"G"; kLabels[2] = L"B"; kLabels[3] = L"A"; break;
			case HexFormat::BGRA: kLabels[0] = L"B"; kLabels[1] = L"G"; kLabels[2] = L"R"; kLabels[3] = L"A"; break;
		}

		for (int i = 0; i < 4; i++)
		{
			CRect r;
			m_edit_argb[i].GetWindowRect(r);
			ScreenToClient(r);
			r.OffsetRect(0, -16);
			label_rect = Gdiplus::RectF(r.left, r.top, r.Width(), r.Height());
			g.DrawString(kLabels[i], -1, &labelFont, label_rect, &sf, &labelBrush);
		}
	}

	// ── H / S / L 레이블 ──────────────────────────────────────────────────
	if (m_edit_hsl[0].GetSafeHwnd())
	{
		static const wchar_t* kHslLabels[3] = { L"H", L"S", L"L" };

		for (int i = 0; i < 3; i++)
		{
			CRect r;
			m_edit_hsl[i].GetWindowRect(r);
			ScreenToClient(r);
			r.OffsetRect(0, -16);
			label_rect = Gdiplus::RectF(r.left, r.top, r.Width(), r.Height());
			g.DrawString(kHslLabels[i], -1, &labelFont, label_rect, &sf, &labelBrush);
		}
	}

	// ── Color Name Label (near 여부에 따라 레이블 변경) ───────────────
	if (!m_r_color_name.IsRectEmpty())
	{
		trace(m_color_name_near);
		const wchar_t* name_label = m_color_name_near ? L"Name(near)" : L"Name";
		label_rect = Gdiplus::RectF((float)m_r_color_name.left,
			(float)(m_r_color_name.top - 16),
			(float)m_r_color_name.Width(), 16.f);
		g.DrawString(name_label, -1, &labelFont, label_rect, &sf, &labelBrush);
	}
}

void CSCColorPicker::OnLButtonDown(UINT nFlags, CPoint point)
{
	// ① 버튼만 즉시 처리 (드래그 충돌 없음)
	HitTarget t;
	if (hit_test(point, t))
	{
		if (t.area == HitArea::Button)
		{
			if (t.idx == BTN_ADD_IDX)
				on_btn_add_clicked();
			else if (t.idx == BTN_DROPPER_IDX)    // ← 추가
				on_btn_dropper_clicked();
		}
	}

	// ② Recent 행 드래그 스크롤 준비
	const float row_top = static_cast<float>(m_r_palette.bottom);
	const float row_bot = row_top + 2.f * m_margin + m_cell;
	if ((float)point.y >= row_top && (float)point.y <= row_bot)
	{
		m_recent_drag_on = true;
		m_recent_drag_start = point;
		m_recent_drag_scroll_start = m_recent_scroll;
		m_recent_drag_moved = false;
		SetCapture();
	}

	CDialog::OnLButtonDown(nFlags, point);
}

void CSCColorPicker::OnLButtonUp(UINT nFlags, CPoint point)
{
	const bool was_drag_moved = m_recent_drag_moved;

	if (m_recent_drag_on)
	{
		m_recent_drag_on = false;
		m_recent_drag_moved = false;
		ReleaseCapture();
	}

	// 드래그 스크롤이 아닌 경우에만 색상 선택
	if (!was_drag_moved)
	{
		HitTarget t;
		if (hit_test(point, t) && t.area != HitArea::Button)
		{
			m_sel = t;
			m_sel_color = get_color(t);

			gcolor_to_hsl(m_sel_color, m_hue, m_sat, m_light);
			update_slider_alpha();
			update_slider_hue();
			update_slider_light();
			Invalidate(FALSE);
		}
	}

	sync_edits();

	CDialog::OnLButtonUp(nFlags, point);
}

void CSCColorPicker::OnMouseMove(UINT nFlags, CPoint point)
{
	// ── WM_MOUSELEAVE 수신을 위한 추적 요청 (매번 갱신해야 함) ──
	TRACKMOUSEEVENT tme = { sizeof(TRACKMOUSEEVENT), TME_LEAVE, m_hWnd, 0 };
	::TrackMouseEvent(&tme);

	// 버튼이 떨어졌는데 드래그 플래그가 남아 있으면 강제 해제
	if (m_recent_drag_on && !(nFlags & MK_LBUTTON))
	{
		m_recent_drag_on = false;
		m_recent_drag_moved = false;
		ReleaseCapture();
	}

	if (m_recent_drag_on && (nFlags & MK_LBUTTON))
	{
		const int dx = m_recent_drag_start.x - point.x;	// 왼쪽 드래그 = 양수 = scroll 증가

		if (abs(dx) > 4)
			m_recent_drag_moved = true;

		if (m_recent_drag_moved)
		{
			const int new_scroll = m_recent_drag_scroll_start
				+ static_cast<int>((float)dx / m_cell);
			const int max_scroll = max(0, (int)m_recent_colors.size() - RECENT_DISPLAY_COLS);
			m_recent_scroll = max(0, min(new_scroll, max_scroll));

			m_hover = {};		// 드래그 중에는 hover 제거
			Invalidate(FALSE);

			CDialog::OnMouseMove(nFlags, point);
			return;
		}
	}

	HitTarget t;
	hit_test(point, t);

	if (!(t == m_hover))
	{
		m_hover = t;
		Invalidate(FALSE);
	}

	update_tooltip(point);

	CDialog::OnMouseMove(nFlags, point);
}

BOOL CSCColorPicker::OnMouseWheel(UINT nFlags, short zDelta, CPoint pt)
{
	ScreenToClient(&pt);

	const float row_top = static_cast<float>(m_r_palette.bottom);
	const float row_bot = row_top + 2.f * m_margin + m_cell;

	if ((float)pt.y >= row_top && (float)pt.y <= row_bot)
	{
		// 휠 위(zDelta > 0): 더 최근(인덱스 감소) / 휠 아래: 더 오래된 색상(인덱스 증가)
		m_recent_scroll += (zDelta > 0) ? -1 : 1;
		clamp_recent_scroll();
		Invalidate(FALSE);
		return TRUE;
	}

	return CDialog::OnMouseWheel(nFlags, zDelta, pt);
}

void CSCColorPicker::OnRButtonDown(UINT nFlags, CPoint point)
{
	// TODO: 여기에 메시지 처리기 코드를 추가 및/또는 기본값을 호출합니다.

	CDialog::OnRButtonDown(nFlags, point);
}

void CSCColorPicker::OnRButtonUp(UINT nFlags, CPoint point)
{
	// TODO: 여기에 메시지 처리기 코드를 추가 및/또는 기본값을 호출합니다.

	CDialog::OnRButtonUp(nFlags, point);
}

LRESULT CSCColorPicker::on_message_CSCSliderCtrl(WPARAM wParam, LPARAM lParam)
{
	const CSCSliderCtrlMsg* msg = reinterpret_cast<CSCSliderCtrlMsg*>(wParam);
	if (!msg)
		return 0;

	if (msg->pThis == &m_slider_alpha)
	{
		// ── Alpha 슬라이더 ─────────────────────────────────────────────
		if (!m_sel.is_valid())
			return 0;

		const BYTE alpha = static_cast<BYTE>(max(0, min(255, msg->pos)));
		m_sel_color = Gdiplus::Color(alpha,
			m_sel_color.GetR(), m_sel_color.GetG(), m_sel_color.GetB());

		sync_edits();
		InvalidateRect(&m_r_preview, FALSE);
	}
	else if (msg->pThis == &m_slider_hue)
	{
		if (!m_sel.is_valid())
			return 0;

		m_hue = static_cast<float>(max(0, min(360, msg->pos)));

		const Gdiplus::Color cr_new = hsl_to_gcolor(m_hue, m_sat, m_light);
		m_sel_color = Gdiplus::Color(m_sel_color.GetA(),
			cr_new.GetR(), cr_new.GetG(), cr_new.GetB());

		update_slider_alpha();
		update_slider_light();
		sync_edits();
		InvalidateRect(&m_r_preview, FALSE);
		InvalidateRect(&m_r_color_name, FALSE);
	}
	else if (msg->pThis == &m_slider_light)
	{
		if (!m_sel.is_valid())
			return 0;

		m_light = static_cast<float>(max(0, min(100, msg->pos))) / 100.f;

		const Gdiplus::Color cr_new = hsl_to_gcolor(m_hue, m_sat, m_light);
		m_sel_color = Gdiplus::Color(m_sel_color.GetA(),
			cr_new.GetR(), cr_new.GetG(), cr_new.GetB());

		update_slider_alpha();
		sync_edits();
		InvalidateRect(&m_r_preview, FALSE);
		InvalidateRect(&m_r_color_name, FALSE);
	}

	return 0;
}

LRESULT CSCColorPicker::on_message_CSCEdit(WPARAM wParam, LPARAM lParam)
{
	const CSCEditMessage* msg = reinterpret_cast<CSCEditMessage*>(wParam);
	if (!msg || msg->pThis == nullptr)
		return 0;

	if (msg->message == WM_KEYDOWN)
	{
		int key = static_cast<int>(lParam);
		switch (key)
		{
			case VK_RETURN:
				if (msg->pThis == &m_edit_color_name)
				{
					// ── Color Name → apply color ──────────────────────
					CString name_text;
					m_edit_color_name.GetWindowText(name_text);
					name_text.Trim();
					if (name_text.IsEmpty())
						break;

					std::string ssname = CString2string(name_text);
					Gdiplus::Color cr = CSCColorList::get_color(ssname);
					m_edit_color_name.SetWindowText(ssname.c_str());


					// alpha 유지 (선택 없으면 255)
					const BYTE alpha = m_sel.is_valid() ? m_sel_color.GetA() : 255;
					m_sel_color = Gdiplus::Color(alpha, cr.GetR(), cr.GetG(), cr.GetB());

					// 팔레트/recent에서 해당 색상 검색하여 선택 상태 갱신
					const HitTarget found = find_color(m_sel_color);
					if (found.is_valid())
						m_sel = found;

					gcolor_to_hsl(m_sel_color, m_hue, m_sat, m_light);
					update_slider_alpha();
					update_slider_hue();
					update_slider_light();
					sync_edits();
					Invalidate(FALSE);
				}
				else
				{
					sync_edits();
					OnBnClickedOk();
				}
				break;
			case VK_ESCAPE:
				OnBnClickedCancel();
				break;
		}
	}

	return 0;
}

void CSCColorPicker::update_slider_alpha()
{
	if (!m_slider_alpha.GetSafeHwnd())
		return;

	if (!m_sel.is_valid())
	{
		m_slider_alpha.set_pos(255);
		m_slider_alpha.set_active_color(Gdiplus::Color(180, 180, 180));
		m_slider_alpha.set_inactive_color(Gdiplus::Color(225, 225, 225));
	}
	else
	{
		m_slider_alpha.set_pos(static_cast<int>(m_sel_color.GetA()));

		// active 구간 = 현재 RGB의 불투명 버전 → 어떤 색인지 한눈에 파악 가능
		m_slider_alpha.set_active_color(Gdiplus::Color(255, m_sel_color.GetR(), m_sel_color.GetG(), m_sel_color.GetB()));
		m_slider_alpha.set_inactive_color(Gdiplus::Color(225, 225, 225));
	}

	m_slider_alpha.Invalidate(FALSE);
}

void CSCColorPicker::update_slider_hue()
{
	if (!m_slider_hue.GetSafeHwnd())
		return;

	// 무지개 트랙은 style이 자체 렌더링하므로 위치만 갱신
	m_slider_hue.set_pos(static_cast<int>(m_hue + 0.5f));
	m_slider_hue.Invalidate(FALSE);
}

// ── Light 슬라이더 동기화 (m_val + 현재 색조 → 그라디언트 + 위치) ─────────
void CSCColorPicker::update_slider_light()
{
	if (!m_slider_light.GetSafeHwnd())
		return;

	if (!m_sel.is_valid())
	{
		m_slider_light.set_gradient_colors({Gdiplus::Color(180, 180, 180), Gdiplus::Color(225, 225, 225)});
		m_slider_light.set_pos(50);
	}
	else
	{
		// HSL L=0(Black) → L=0.5(순색) → L=1(White)
		const Gdiplus::Color pure = hsl_to_gcolor(m_hue, m_sat, 0.5f);
		m_slider_light.set_gradient_colors({Gdiplus::Color::Black, pure, Gdiplus::Color::White});
		m_slider_light.set_pos(static_cast<int>(m_light * 100.f + 0.5f));
	}
	m_slider_light.Invalidate(FALSE);
}

void CSCColorPicker::OnLButtonDblClk(UINT nFlags, CPoint point)
{
	// ── Hex 레이블 영역 더블클릭 → 형식 토글 (OK 처리보다 우선) ──────────
	if (!m_r_label_hexa.IsRectEmpty() && m_r_label_hexa.PtInRect(point))
	{
		toggle_hex_format();
		CDialog::OnLButtonDblClk(nFlags, point);
		return;
	}

	// ── 팔레트 / 최근 색상 셀 히트 여부 ──────────────────────────────────
	HitTarget t;
	const bool on_cell = hit_test(point, t)
		&& (t.area == HitArea::Palette || t.area == HitArea::Recent);

	// ── 미리보기 원 내부 히트 여부 (원형 판정) ────────────────────────────
	const float pc_x = static_cast<float>(m_r_preview.CenterPoint().x);
	const float pc_y = static_cast<float>(m_r_preview.CenterPoint().y);
	const float pr = static_cast<float>(m_r_preview.Width()) * 0.5f;
	const float dx = static_cast<float>(point.x) - pc_x;
	const float dy = static_cast<float>(point.y) - pc_y;
	const bool on_preview = (dx * dx + dy * dy <= pr * pr);

	if (on_cell || on_preview)
	{
		sync_edits();
		OnBnClickedOk();
	}

	CDialog::OnLButtonDblClk(nFlags, point);
}

BOOL CSCColorPicker::OnEraseBkgnd(CDC* pDC)
{
	// TODO: 여기에 메시지 처리기 코드를 추가 및/또는 기본값을 호출합니다.
	return FALSE;
	return CDialog::OnEraseBkgnd(pDC);
}

void CSCColorPicker::PostNcDestroy()
{
	// TODO: 여기에 특수화된 코드를 추가 및/또는 기본 클래스를 호출합니다.

	CDialog::PostNcDestroy();
}

void CSCColorPicker::OnDestroy()
{
	CDialog::OnDestroy();

	// TODO: 여기에 메시지 처리기 코드를 추가합니다.
	if (m_tooltip.GetSafeHwnd())
		m_tooltip.DestroyWindow();
}

void CSCColorPicker::clamp_recent_scroll()
{
	const int max_scroll = max(0, (int)m_recent_colors.size() - RECENT_DISPLAY_COLS);
	m_recent_scroll = max(0, min(m_recent_scroll, max_scroll));
}

const wchar_t* CSCColorPicker::get_hex_format_label() const
{
	switch (m_hex_format)
	{
		case HexFormat::ARGB: return L"ARGB";
		case HexFormat::ABGR: return L"ABGR";
		case HexFormat::RGBA: return L"RGBA";
		case HexFormat::BGRA: return L"BGRA";
		default:              return L"ARGB";
	}
}

// ── toggle_hex_format() ────────────────────────────────────────
// ARGB → ABGR → RGBA → BGRA → ARGB 순환 토글
void CSCColorPicker::toggle_hex_format()
{
	switch (m_hex_format)
	{
		case HexFormat::ARGB: m_hex_format = HexFormat::ABGR; break;
		case HexFormat::ABGR: m_hex_format = HexFormat::RGBA; break;
		case HexFormat::RGBA: m_hex_format = HexFormat::BGRA; break;
		case HexFormat::BGRA: m_hex_format = HexFormat::ARGB; break;
	}

	AfxGetApp()->WriteProfileInt(_T("setting\\color picker"), _T("hex_format"), (int)m_hex_format);

	sync_edits();      // Hex 편집 텍스트 갱신
	Invalidate(FALSE); // 레이블 텍스트 갱신
}

void CSCColorPicker::OnContextMenu(CWnd* pWnd, CPoint point)
{
	// 키보드(Shift+F10, App키)로 트리거 시 point = (-1, -1) → 중앙에 표시
	if (point.x == -1 && point.y == -1)
	{
		CRect rc;
		GetClientRect(rc);
		point = rc.CenterPoint();
		ClientToScreen(&point);
	}

	// ── 우클릭 위치가 최근 색상 셀인지 판별 ──────────────
	CPoint clientPt = point;
	ScreenToClient(&clientPt);
	HitTarget ctx_hit;
	const bool on_recent = hit_test(clientPt, ctx_hit) && ctx_hit.area == HitArea::Recent;

	CMenu menu;
	menu.CreatePopupMenu();

	// ── 클립보드 복사 (선택 색상이 있을 때만 활성) ────────
	{
		const UINT flags = m_sel.is_valid() ? MF_STRING : (MF_STRING | MF_GRAYED);
		const wchar_t* fmt_label = get_hex_format_label();

		CString caption_hex;
		caption_hex.Format(_T("Copy %s Hexa"), fmt_label);
		menu.AppendMenu(flags, IDM_COPY_HEX, caption_hex);

		CString caption_comp;
		caption_comp.Format(_T("Copy %s Values(&C)"), fmt_label);
		menu.AppendMenu(flags, IDM_COPY_COMPONENTS, caption_comp);

		menu.AppendMenu(flags, IDM_COPY_WEB_COLOR, _T("Copy Web Color Value(&W)"));
	}

	menu.AppendMenu(MF_SEPARATOR);

	menu.AppendMenu(m_sel.is_valid() ? MF_STRING : (MF_STRING | MF_GRAYED), IDM_VIEW_COMPLEMENTARY, _T("보색 보기(&V)"));

	menu.AppendMenu(MF_SEPARATOR);

	// ── 최근 색상 삭제 ───────────────────────────────────
	menu.AppendMenu(MF_STRING, IDM_DELETE_RECENT, _T("이 색상 항목 삭제(&D)"));
	menu.EnableMenuItem(IDM_DELETE_RECENT, (on_recent ? MF_ENABLED : MF_DISABLED));

	menu.AppendMenu(m_recent_colors.empty() ? (MF_STRING | MF_GRAYED) : MF_STRING,
		IDM_DELETE_ALL_RECENT, _T("모든 최근 색상 삭제(&A)..."));

	menu.AppendMenu(MF_SEPARATOR);

	// ── 툴팁 표시 (체크 토글) ─────────────────────────────
	menu.AppendMenu(MF_STRING | (m_show_tooltip ? MF_CHECKED : MF_UNCHECKED),
		IDM_TOOLTIP, _T("툴팁 표시(&T)"));

	menu.AppendMenu(MF_SEPARATOR);

	// ── Hex 표시 형식 선택 (라디오 불릿) ─────────────────
	// IDM_FMT_ARGB(2002) = IDM_FMT_ARGB + HexFormat::ARGB(0)
	// IDM_FMT_BGRA(2005) = IDM_FMT_ARGB + HexFormat::BGRA(3)
	menu.AppendMenu(MF_STRING, IDM_FMT_ARGB, _T("ARGB"));
	menu.AppendMenu(MF_STRING, IDM_FMT_ABGR, _T("ABGR"));
	menu.AppendMenu(MF_STRING, IDM_FMT_RGBA, _T("RGBA"));
	menu.AppendMenu(MF_STRING, IDM_FMT_BGRA, _T("BGRA"));
	menu.CheckMenuRadioItem(
		IDM_FMT_ARGB, IDM_FMT_BGRA,
		IDM_FMT_ARGB + static_cast<int>(m_hex_format),
		MF_BYCOMMAND);

	menu.AppendMenu(MF_SEPARATOR);

	// ── 최근 색상 내보내기 / 가져오기 ────────────────────
	menu.AppendMenu(MF_STRING, IDM_EXPORT_RECENT, _T("최근 색상 내보내기"));
	menu.AppendMenu(MF_STRING, IDM_IMPORT_RECENT, _T("최근 색상 가져오기"));

	//ClientToScreen(&point);
	const int cmd = static_cast<int>(
		menu.TrackPopupMenu(
			TPM_RETURNCMD | TPM_RIGHTBUTTON | TPM_LEFTALIGN,
			point.x, point.y, this));

	switch (cmd)
	{
		case IDM_COPY_HEX:
		{
			// HexFormat에 따른 바이트 순서로 8자리 Hex 문자열 복사
			const auto cr = m_sel_color;
			BYTE b[4];

			switch (m_hex_format)
			{
				case HexFormat::ARGB: b[0] = cr.GetA(); b[1] = cr.GetR(); b[2] = cr.GetG(); b[3] = cr.GetB(); break;
				case HexFormat::ABGR: b[0] = cr.GetA(); b[1] = cr.GetB(); b[2] = cr.GetG(); b[3] = cr.GetR(); break;
				case HexFormat::RGBA: b[0] = cr.GetR(); b[1] = cr.GetG(); b[2] = cr.GetB(); b[3] = cr.GetA(); break;
				case HexFormat::BGRA: b[0] = cr.GetB(); b[1] = cr.GetG(); b[2] = cr.GetR(); b[3] = cr.GetA(); break;
			}

			CString text;
			text.Format(_T("%02X%02X%02X%02X"), b[0], b[1], b[2], b[3]);
			copy_to_clipboard(m_hWnd, text);
			break;
		}

		case IDM_COPY_COMPONENTS:
		{
			// HexFormat에 따른 순서로 "A, R, G, B" 형식 복사
			const auto cr = m_sel_color;
			int v[4];

			switch (m_hex_format)
			{
				case HexFormat::ARGB: v[0] = cr.GetA(); v[1] = cr.GetR(); v[2] = cr.GetG(); v[3] = cr.GetB(); break;
				case HexFormat::ABGR: v[0] = cr.GetA(); v[1] = cr.GetB(); v[2] = cr.GetG(); v[3] = cr.GetR(); break;
				case HexFormat::RGBA: v[0] = cr.GetR(); v[1] = cr.GetG(); v[2] = cr.GetB(); v[3] = cr.GetA(); break;
				case HexFormat::BGRA: v[0] = cr.GetB(); v[1] = cr.GetG(); v[2] = cr.GetR(); v[3] = cr.GetA(); break;
			}

			CString text;
			text.Format(_T("%d, %d, %d, %d"), v[0], v[1], v[2], v[3]);
			copy_to_clipboard(m_hWnd, text);
			break;
		}

		case IDM_COPY_WEB_COLOR:
		{
			// alpha == 255: #RRGGBB / alpha < 255: rgba(R, G, B, 0.XX)
			const auto cr = m_sel_color;
			CString text;

			if (cr.GetA() == 255)
				text.Format(_T("#%02X%02X%02X"), cr.GetR(), cr.GetG(), cr.GetB());
			else
				text.Format(_T("rgba(%d, %d, %d, %.2f)"),
					(int)cr.GetR(), (int)cr.GetG(), (int)cr.GetB(),
					cr.GetA() / 255.0);

			copy_to_clipboard(m_hWnd, text);
			break;
		}

		case IDM_VIEW_COMPLEMENTARY:
		{
			if (!m_sel.is_valid())
				break;

			// 보색 적용 (alpha 유지)
			const Gdiplus::Color comp = get_complementary_gcolor(m_sel_color);
			m_sel_color = Gdiplus::Color(m_sel_color.GetA(),
				comp.GetR(), comp.GetG(), comp.GetB());

			gcolor_to_hsl(m_sel_color, m_hue, m_sat, m_light);
			update_slider_alpha();
			update_slider_hue();
			update_slider_light();
			sync_edits();
			Invalidate(FALSE);
			break;
		}

		case IDM_DELETE_RECENT:
		{
			if (on_recent && ctx_hit.idx >= 0 && ctx_hit.idx < (int)m_recent_colors.size())
			{
				m_recent_colors.erase(m_recent_colors.begin() + ctx_hit.idx);

				// 선택 상태 보정: 삭제된 항목이면 해제, 이후 항목이면 인덱스 감소
				if (m_sel.area == HitArea::Recent)
				{
					if (m_sel.idx == ctx_hit.idx)
						m_sel = {};
					else if (m_sel.idx > ctx_hit.idx)
						m_sel.idx--;
				}

				m_hover = {};
				clamp_recent_scroll();

				// 레지스트리에 반영
				AfxGetApp()->WriteProfileInt(_T("setting\\color picker"), _T("count"), (int)m_recent_colors.size());
				for (int i = 0; i < (int)m_recent_colors.size(); ++i)
				{
					CString key, val;
					key.Format(_T("color_%d"), i);
					val.Format(_T("%08X"), m_recent_colors[i].GetValue());
					AfxGetApp()->WriteProfileString(_T("setting\\color picker"), key, val);
				}

				Invalidate(FALSE);
			}
			break;
		}

		case IDM_DELETE_ALL_RECENT:
		{
			if (!m_recent_colors.empty())
			{
				if (AfxMessageBox(_T("모든 최근 색상을 삭제하시겠습니까?"),
					MB_YESNO | MB_ICONQUESTION) != IDYES)
					break;

				m_recent_colors.clear();
				m_recent_scroll = 0;

				if (m_sel.area == HitArea::Recent)
					m_sel = {};

				m_hover = {};

				// 레지스트리에 반영
				AfxGetApp()->WriteProfileInt(_T("setting\\color picker"), _T("count"), 0);

				Invalidate(FALSE);
			}
			break;
		}

		case IDM_TOOLTIP:
			m_show_tooltip = !m_show_tooltip;
			AfxGetApp()->WriteProfileInt(
				_T("setting\\color picker"), _T("show_tooltip"), m_show_tooltip ? 1 : 0);
			// 비활성화 시 현재 표시 중인 툴팁 즉시 숨김
			if (!m_show_tooltip && m_tooltip.GetSafeHwnd())
			{
				TOOLINFO ti = {};
				ti.cbSize = sizeof(TOOLINFO);
				ti.hwnd = m_hWnd;
				ti.uId = 1;
				::SendMessage(m_tooltip.m_hWnd, TTM_TRACKACTIVATE, FALSE, (LPARAM)&ti);
			}
			break;
		case IDM_FMT_ARGB: m_hex_format = HexFormat::ARGB; break;
		case IDM_FMT_ABGR: m_hex_format = HexFormat::ABGR; break;
		case IDM_FMT_RGBA: m_hex_format = HexFormat::RGBA; break;
		case IDM_FMT_BGRA: m_hex_format = HexFormat::BGRA; break;

		case IDM_EXPORT_RECENT:
			export_recent_colors();
			break;

		case IDM_IMPORT_RECENT:
			import_recent_colors();
			break;

		default:
			return;	// 선택 없이 닫힌 경우
	}

	// Hex 형식 변경 시 레지스트리 저장 + 편집 컨트롤·레이블 갱신
	if (cmd >= IDM_FMT_ARGB && cmd <= IDM_FMT_BGRA)
	{
		AfxGetApp()->WriteProfileInt(
			_T("setting\\color picker"), _T("hex_format"), static_cast<int>(m_hex_format));
		sync_edits();
		Invalidate(FALSE);
	}
}

LRESULT CSCColorPicker::OnMouseLeave(WPARAM, LPARAM)
{
	m_hover = {};

	if (m_tooltip.GetSafeHwnd())
	{
		TOOLINFO ti = {};
		ti.cbSize = sizeof(TOOLINFO);
		ti.hwnd = m_hWnd;
		ti.uId = 1;
		::SendMessage(m_tooltip.m_hWnd, TTM_TRACKACTIVATE, FALSE, (LPARAM)&ti);
	}

	Invalidate(FALSE);
	return 0;
}

CString CSCColorPicker::make_tooltip_text(Gdiplus::Color cr) const
{
	BYTE b[4];
	LPCTSTR label;

	switch (m_hex_format)
	{
	case HexFormat::ARGB: b[0] = cr.GetA(); b[1] = cr.GetR(); b[2] = cr.GetG(); b[3] = cr.GetB(); label = _T("ARGB"); break;
	case HexFormat::ABGR: b[0] = cr.GetA(); b[1] = cr.GetB(); b[2] = cr.GetG(); b[3] = cr.GetR(); label = _T("ABGR"); break;
	case HexFormat::RGBA: b[0] = cr.GetR(); b[1] = cr.GetG(); b[2] = cr.GetB(); b[3] = cr.GetA(); label = _T("RGBA"); break;
	case HexFormat::BGRA: b[0] = cr.GetB(); b[1] = cr.GetG(); b[2] = cr.GetR(); b[3] = cr.GetA(); label = _T("BGRA"); break;
	default:              b[0] = cr.GetA(); b[1] = cr.GetR(); b[2] = cr.GetG(); b[3] = cr.GetB(); label = _T("ARGB"); break;
	}

	CString text;
	text.Format(_T("%s : %02X%02X%02X%02X (%d, %d, %d, %d)"),
		label,
		b[0], b[1], b[2], b[3],
		(int)b[0], (int)b[1], (int)b[2], (int)b[3]);

	return text;
}

// ── 트래킹 툴팁 갱신 (OnMouseMove에서 매번 호출) ─────────
void CSCColorPicker::update_tooltip(CPoint pt)
{
	if (!m_tooltip.GetSafeHwnd())
		return;

	TOOLINFO ti = {};
	ti.cbSize = sizeof(TOOLINFO);
	ti.hwnd = m_hWnd;
	ti.uId = 1;

	// m_show_tooltip 비활성 시 항상 숨김
	if (!m_show_tooltip)
	{
		::SendMessage(m_tooltip.m_hWnd, TTM_TRACKACTIVATE, FALSE, (LPARAM)&ti);
		return;
	}

	// ── 표시 대상 색상 결정 ─────────────────────────────────
	Gdiplus::Color cr;
	bool show = false;

	// ① 팔레트 / 최근 색상 hover (버튼 제외)
	if (m_hover.is_valid() && m_hover.area != HitArea::Button)
	{
		cr = get_color(m_hover);
		show = true;
	}
	// ② 미리보기 원 위 (원형 판정)
	else if (m_sel.is_valid() && !m_r_preview.IsRectEmpty())
	{
		const float cx = static_cast<float>(m_r_preview.CenterPoint().x);
		const float cy = static_cast<float>(m_r_preview.CenterPoint().y);
		const float r = static_cast<float>(m_r_preview.Width()) * 0.5f;
		const float dx = static_cast<float>(pt.x) - cx;
		const float dy = static_cast<float>(pt.y) - cy;
		if (dx * dx + dy * dy <= r * r)
		{
			cr = m_sel_color;
			show = true;
		}
	}

	if (show)
	{
		const CString text = make_tooltip_text(cr);
		ti.lpszText = const_cast<LPTSTR>((LPCTSTR)text);
		::SendMessage(m_tooltip.m_hWnd, TTM_UPDATETIPTEXT, 0, (LPARAM)&ti);

		// 커서 오른쪽 아래에 표시 (겹침 방지)
		CPoint screen_pt = pt;
		ClientToScreen(&screen_pt);
		::SendMessage(m_tooltip.m_hWnd, TTM_TRACKPOSITION, 0,
			MAKELPARAM(screen_pt.x + 14, screen_pt.y + 20));
		::SendMessage(m_tooltip.m_hWnd, TTM_TRACKACTIVATE, TRUE, (LPARAM)&ti);
	}
	else
	{
		::SendMessage(m_tooltip.m_hWnd, TTM_TRACKACTIVATE, FALSE, (LPARAM)&ti);
	}
}

void CSCColorPicker::export_recent_colors()
{
	if (m_recent_colors.empty())
	{
		AfxMessageBox(_T("내보낼 색상이 없습니다."), MB_OK | MB_ICONINFORMATION);
		return;
	}

	// 초기 폴더: 마지막으로 사용한 폴더 → 없으면 exe 폴더
	CString init_dir = AfxGetApp()->GetProfileString(
		_T("setting\\color picker"), _T("recent colors exported folder"), _T(""));
	if (init_dir.IsEmpty())
		init_dir = get_exe_directory();

	CFileDialog dlg(FALSE, _T("reg"), _T("recent_colors"),
		OFN_OVERWRITEPROMPT | OFN_PATHMUSTEXIST,
		_T("Registry Files (*.reg)|*.reg||"), this);
	dlg.m_ofn.lpstrInitialDir = init_dir;

	if (dlg.DoModal() != IDOK)
		return;

	const CString filepath = dlg.GetPathName();

	// 마지막으로 사용한 폴더 저장
	AfxGetApp()->WriteProfileString(
		_T("setting\\color picker"), _T("recent colors exported folder"),
		get_part(filepath, fn_folder));

	// UTF-8 BOM + UTF-8 텍스트 쓰기 (CHARSET 매크로: ccs=UTF-8 → BOM 자동 기록)
	FILE* fp = nullptr;
	if (_tfopen_s(&fp, filepath, _T("wt") CHARSET) != 0 || !fp)
	{
		AfxMessageBox(_T("파일을 저장할 수 없습니다."), MB_OK | MB_ICONERROR);
		return;
	}

	// ── .reg 형식으로 색상 목록 기록 ──────────────────────────────────
	LPCTSTR app_key = AfxGetApp()->m_pszRegistryKey;
	if (!app_key || !app_key[0])
		app_key = _T("MyApp");

	_ftprintf(fp, _T("Windows Registry Editor Version 5.00\n\n"));
	_ftprintf(fp, _T("[HKEY_CURRENT_USER\\Software\\%s\\setting\\color picker\\recent_colors]\n"),
		app_key);

	_ftprintf(fp, _T("\"count\"=\"%d\"\n"), (int)m_recent_colors.size());
	for (int i = 0; i < (int)m_recent_colors.size(); ++i)
	{
		CString val;
		val.Format(_T("%08X"), m_recent_colors[i].GetValue());
		_ftprintf(fp, _T("\"color_%d\"=\"%s\"\n"), i, (LPCTSTR)val);
	}

	fclose(fp);
}

// ── 최근 색상 가져오기 (.reg 형식, UTF-8 BOM / UTF-16 LE 자동 감지) ────
void CSCColorPicker::import_recent_colors()
{
	// 초기 폴더: 마지막으로 사용한 폴더 → 없으면 exe 폴더
	CString init_dir = AfxGetApp()->GetProfileString(_T("setting\\color picker"), _T("recent colors exported folder"), _T(""));
	if (init_dir.IsEmpty())
		init_dir = get_exe_directory();

	CFileDialog dlg(TRUE, _T("reg"), nullptr, OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST, _T("Registry Files (*.reg)|*.reg|All Files (*.*)|*.*||"), this);
	dlg.m_ofn.lpstrInitialDir = init_dir;

	if (dlg.DoModal() != IDOK)
		return;

	const CString filepath = dlg.GetPathName();

	// 마지막으로 사용한 폴더 저장
	AfxGetApp()->WriteProfileString(_T("setting\\color picker"), _T("recent colors exported folder"), get_part(filepath, fn_folder));

	// ── 파일 읽기 (바이너리 → BOM 감지 → CString 변환) ────────────────
	CFile file;
	if (!file.Open(filepath, CFile::modeRead | CFile::typeBinary))
	{
		AfxMessageBox(_T("파일을 열 수 없습니다."), MB_OK | MB_ICONERROR);
		return;
	}

	const ULONGLONG file_size = file.GetLength();
	if (file_size < 2)
	{
		file.Close();
		return;
	}

	std::vector<BYTE> buf(static_cast<size_t>(file_size) + 2, 0);
	file.Read(buf.data(), static_cast<UINT>(file_size));
	file.Close();

	CString content;

	if (file_size >= 3 && buf[0] == 0xEF && buf[1] == 0xBB && buf[2] == 0xBF)
	{
		// UTF-8 BOM → wchar_t 변환
		const char* src = reinterpret_cast<const char*>(buf.data() + 3);
		const int    src_len = static_cast<int>(file_size - 3);
		const int    wlen = MultiByteToWideChar(CP_UTF8, 0, src, src_len, nullptr, 0);
		std::vector<wchar_t> wbuf(wlen + 1, 0);
		MultiByteToWideChar(CP_UTF8, 0, src, src_len, wbuf.data(), wlen);
		content = CString(wbuf.data(), wlen);
	}
	else if (file_size >= 2 && buf[0] == 0xFF && buf[1] == 0xFE)
	{
		// UTF-16 LE BOM (이전 버전 파일 호환)
		const wchar_t* wbuf = reinterpret_cast<const wchar_t*>(buf.data() + 2);
		const int      wlen = static_cast<int>((file_size - 2) / sizeof(wchar_t));
		content = CString(wbuf, wlen);
	}
	else
	{
		// BOM 없음 → UTF-8 no BOM 시도
		const char* src = reinterpret_cast<const char*>(buf.data());
		const int   src_len = static_cast<int>(file_size);
		const int   wlen = MultiByteToWideChar(CP_UTF8, 0, src, src_len, nullptr, 0);
		std::vector<wchar_t> wbuf(wlen + 1, 0);
		MultiByteToWideChar(CP_UTF8, 0, src, src_len, wbuf.data(), wlen);
		content = CString(wbuf.data(), wlen);
	}

	// ── .reg 파일 파싱: "color_N"="XXXXXXXX" ──────────────────────────
	std::map<int, Gdiplus::Color> color_map;

	int pos = 0;
	while (pos < content.GetLength())
	{
		const int nl = content.Find(_T('\n'), pos);
		CString line = (nl < 0) ? content.Mid(pos) : content.Mid(pos, nl - pos);
		pos = (nl < 0) ? content.GetLength() : nl + 1;

		line.TrimRight(_T("\n "));
		line.TrimLeft();

		// "color_N"="XXXXXXXX" 최소 길이: len("color_0"="00000000") = 18
		if (line.GetLength() < 18 || line[0] != _T('"'))
			continue;

		const int q2 = line.Find(_T('"'), 1);
		if (q2 < 0) continue;

		const CString key = line.Mid(1, q2 - 1);       // color_N
		if (key.Left(6) != _T("color_")) continue;

		const int eq = line.Find(_T('='), q2 + 1);
		if (eq < 0) continue;

		const int q3 = line.Find(_T('"'), eq + 1);
		const int q4 = (q3 >= 0) ? line.Find(_T('"'), q3 + 1) : -1;
		if (q3 < 0 || q4 < 0 || q4 - q3 - 1 != 8) continue;

		const CString val = line.Mid(q3 + 1, 8);       // XXXXXXXX
		const int  idx = _ttoi((LPCTSTR)key.Mid(6));
		const DWORD argb = _tcstoul(val, nullptr, 16);
		color_map[idx] = Gdiplus::Color(argb);
	}

	if (color_map.empty())
	{
		AfxMessageBox(_T("가져올 색상 데이터가 없습니다."), MB_OK | MB_ICONWARNING);
		return;
	}

	// ── m_recent_colors 재구성 (index 순서 유지) ──────────────────────
	m_recent_colors.clear();
	for (auto& kv : color_map)
		m_recent_colors.push_back(kv.second);

	if ((int)m_recent_colors.size() > MAX_RECENT_COLORS)
		m_recent_colors.resize(MAX_RECENT_COLORS);

	m_recent_scroll = 0;

	// ── 레지스트리에 반영 ─────────────────────────────────────────────
	AfxGetApp()->WriteProfileInt(
		_T("setting\\color picker"), _T("count"), (int)m_recent_colors.size());
	for (int i = 0; i < (int)m_recent_colors.size(); ++i)
	{
		CString key, val;
		key.Format(_T("color_%d"), i);
		val.Format(_T("%08X"), m_recent_colors[i].GetValue());
		AfxGetApp()->WriteProfileString(_T("setting\\color picker"), key, val);
	}

	Invalidate(FALSE);
}

void CSCColorPicker::OnTimer(UINT_PTR nIDEvent)
{
	// TODO: 여기에 메시지 처리기 코드를 추가 및/또는 기본값을 호출합니다.
	if (nIDEvent == timer_run_dropper_wnd)
	{
		KillTimer(timer_run_dropper_wnd);

		MSG msg = {};

		CSCDropperDlg dlg;
		dlg.create(this);

		// ESC 키 수신을 위해 포커스 명시적 지정
		//dlg.SetForegroundWindow();
		//dlg.SetFocus();

		// ③ 메시지 루프 — dlg가 DestroyWindow()로 닫힐 때까지 대기
//    (LButtonDown 또는 VK_ESCAPE → KillTimer + DestroyWindow → GetSafeHwnd() == NULL)
		bool quit_posted = false;
		msg = {};
		while (dlg.GetSafeHwnd() != NULL)
		{
			while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
			{
				if (msg.message == WM_QUIT)
				{
					quit_posted = true;
					break;
				}
				TranslateMessage(&msg);
				DispatchMessage(&msg);
			}
			if (quit_posted) break;
			if (dlg.GetSafeHwnd() != NULL)
				WaitMessage();
		}
		if (quit_posted)
			PostQuitMessage(static_cast<int>(msg.wParam));

		// ④ 컬러피커 복원
		// 드로퍼 종료 후 복원
		//::SetWindowDisplayAffinity(m_hWnd, WDA_NONE);
		ShowWindow(SW_SHOW);
		SetForegroundWindow();

		// ⑤ 색상 반영 (is_picked() == true: 클릭으로 확정, false: ESC 취소)
		if (dlg.is_picked())
		{
			m_sel_color = dlg.get_picked_color();

			// recent에 추가 후 m_sel을 recent[0]으로 지정 (미리보기·슬라이더 활성화)
			for (auto it = m_recent_colors.begin(); it != m_recent_colors.end(); )
			{
				if (it->GetValue() == m_sel_color.GetValue())
					it = m_recent_colors.erase(it);
				else
					++it;
			}
			m_recent_colors.insert(m_recent_colors.begin(), m_sel_color);
			if ((int)m_recent_colors.size() > MAX_RECENT_COLORS)
				m_recent_colors.resize(MAX_RECENT_COLORS);

			m_recent_scroll = 0;
			m_sel = { HitArea::Recent, -1, -1, 0 };

			gcolor_to_hsl(m_sel_color, m_hue, m_sat, m_light);
			update_slider_alpha();
			update_slider_hue();
			update_slider_light();
			sync_edits();
			Invalidate(FALSE);
		}
	}

	CDialog::OnTimer(nIDEvent);
}

void CSCColorPicker::OnWindowPosChanged(WINDOWPOS* lpwndpos)
{
	CDialog::OnWindowPosChanged(lpwndpos);

	// TODO: 여기에 메시지 처리기 코드를 추가합니다.
	SaveWindowPosition(AfxGetApp(), this, _T("color picker"));
}
