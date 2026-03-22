// SCColorPicker.cpp: 구현 파일
//
#include "SCColorPicker.h"

#include <afxdlgs.h>
#include "../../Functions.h"
#include "../../MemoryDC.h"

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

CSCColorPicker::CSCColorPicker(CWnd* parent, CString title, bool as_modal)
{
	if (!parent)
		parent = AfxGetApp()->GetMainWnd();

	//memset(&m_lf, 0, sizeof(LOGFONT));

	create(parent, title, as_modal);
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
	ON_WM_DESTROY()
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
	const DWORD dwExStyle = NULL;// WS_EX_TOOLWINDOW;
	DWORD dwStyle = WS_POPUP | WS_CAPTION | WS_SYSMENU | WS_CLIPCHILDREN;
	if (!as_modal)
		dwStyle |= WS_VISIBLE;	// modeless 경로: 생성 즉시 표시

	WNDCLASS wc = {};
	::GetClassInfo(AfxGetInstanceHandle(), _T("#32770"), &wc);
	wc.lpszClassName = _T("CSCColorPicker");
	AfxRegisterClass(&wc);

	bool res = CreateEx(dwExStyle, wc.lpszClassName, _T("CSCColorPicker"), dwStyle, CRect(0, 0, 500, 400), parent, 0);

	if (!res)
		return false;

	SetWindowText(title);
	calc_layout();
	load_recent_colors();	// ← modeless 경로: create()에서 직접 로드

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

	// 최근 색상 + 버튼 행 하단 y
	const float recent_bottom = m_r_palette.bottom + m_margin + m_cell;

	// ── 3개 슬라이더 + 미리보기 섹션 ─────────────────────────────────────
	const int slider_h = 26;
	//const int mg = static_cast<int>(m_margin);
	const int slider_gap = static_cast<int>(m_margin * 0.1f);	// ← 슬라이더 간 세로 간격 (mg의 80%)

	// alpha + slider_gap + hue + slider_gap + light
	const int total_slider_h = 3 * slider_h + 2 * slider_gap;
	const int preview_diam = total_slider_h;	// 미리보기 원 지름 = 슬라이더 스택 높이

	// 두 번째 구분선 기준점 (recent_bottom + 4)
	const float info_top = recent_bottom + 4.0f;
	// 슬라이더/미리보기 섹션 시작: 구분선 아래 m_margin 여백
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
	m_r_slider_light = CRect(slider_left, slider_section_top + 2 * slider_h + 2 * slider_gap,
		slider_right, slider_section_top + 3 * slider_h + 2 * slider_gap);

	// 편집 영역: section_bottom + m_margin(gap) + 16px(레이블)
	int hexa_width = 80;
	int editTop = section_bottom + m_margin + 16;
	m_r_edit_area = CRect(m_r_palette.left + m_margin, editTop, m_r_palette.right - m_margin, editTop + kEditH);

	if (!m_edit_hexa.GetSafeHwnd())
	{
		//Hex (AARRGGBB)
		m_edit_hexa.Create(
			WS_CHILD | WS_VISIBLE | WS_TABSTOP | WS_BORDER | ES_CENTER | ES_AUTOHSCROLL | ES_MULTILINE,
			CRect(m_r_edit_area.left, m_r_edit_area.top, m_r_edit_area.left + hexa_width, m_r_edit_area.top + kEditH), this, IDC_EDIT_HEXA);
		m_edit_hexa.LimitText(8);
		m_edit_hexa.set_font_name(_T("Consolas"));
		m_edit_hexa.set_font_size(9);
		m_edit_hexa.set_text_color(Gdiplus::Color(52, 68, 71, 70));
		m_edit_hexa.set_back_color(Gdiplus::Color::White);
		m_edit_hexa.set_border_color(Gdiplus::Color::LightGray);
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
			m_edit_argb[i].set_border_color(Gdiplus::Color::LightGray);
			m_edit_argb[i].set_dark_border_on_focus(true);
			m_edit_argb[i].set_line_align(DT_VCENTER);
		}

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
	}

	// ── 다이얼로그 크기를 콘텐츠에 맞게 자동 조정 ───────────────────────
	{
		const int client_h = m_r_edit_area.bottom + m_margin;
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
			m_r_slider_light, this, IDC_SLIDER_LIGHT
		);
		m_slider_light.set_style(CSCSliderCtrl::style_thumb_round_gradient);
		m_slider_light.set_range(0, 100);
		m_slider_light.set_back_color(Gdiplus::Color::White);
		m_slider_light.set_text_style(CSCSliderCtrl::text_style_none);
		m_slider_light.set_inactive_color(Gdiplus::Color(180, 180, 180));
		m_slider_light.set_active_color(Gdiplus::Color(225, 225, 225));
		m_slider_light.set_pos(100);
	}
	else
	{
		m_slider_light.MoveWindow(m_r_slider_light);
	}
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

	// Hex 편집: AARRGGBB — 내용이 달라진 경우에만 SetWindowText 호출
	newText.Format(_T("%02X%02X%02X%02X"),
		cr.GetAlpha(), cr.GetRed(), cr.GetGreen(), cr.GetBlue());
	m_edit_hexa.GetWindowText(curText);
	if (newText != curText)
		m_edit_hexa.SetWindowText(newText);

	// A / R / G / B — 내용이 달라진 경우에만 호출 (같으면 재렌더링 자체를 건너뜀)
	const int vals[4] = {
		(int)cr.GetAlpha(), (int)cr.GetRed(),
		(int)cr.GetGreen(), (int)cr.GetBlue()
	};
	for (int i = 0; i < 4; i++)
	{
		newText.Format(_T("%d"), vals[i]);
		m_edit_argb[i].GetWindowText(curText);
		if (newText != curText)
			m_edit_argb[i].SetWindowText(newText);
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

	BYTE a = clamp(_ttoi(s[0]));
	BYTE r = clamp(_ttoi(s[1]));
	BYTE g = clamp(_ttoi(s[2]));
	BYTE b = clamp(_ttoi(s[3]));

	m_sel_color = Gdiplus::Color(a, r, g, b);

	// 편집 → HSV 추출 후 슬라이더 동기화
	color_to_hsv(m_sel_color, m_hue, m_sat, m_val);
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
	if (text.GetLength() != 8)
		return;

	DWORD argb = 0;
	if (_stscanf_s(text, _T("%8X"), &argb) != 1)
		return;

	BYTE a = (argb >> 24) & 0xFF;
	BYTE r = (argb >> 16) & 0xFF;
	BYTE g = (argb >> 8) & 0xFF;
	BYTE b = (argb) & 0xFF;

	m_sel_color = Gdiplus::Color(a, r, g, b);

	// Hex 편집 → HSV 추출 후 슬라이더 동기화
	color_to_hsv(m_sel_color, m_hue, m_sat, m_val);
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
		return {
			m_r_palette.left + m_margin + t.idx * m_cell + m_cell * 0.5f,
			static_cast<float>(m_r_palette.bottom) + 4.0f + m_cell * 0.5f
		};
	}
	if (t.area == HitArea::Button)
	{
		// 최근 색상 칸(MAX_RECENT_COLORS개) 바로 뒤에 순서대로 배치
		return {
			m_r_palette.left + m_margin + ((int)m_recent_colors.size() + t.idx) * m_cell + m_cell * 0.5f,
			static_cast<float>(m_r_palette.bottom) + 4.f + m_cell * 0.5f
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
	// 버튼은 draw_buttons()에서 자체적으로 hover를 처리하므로 여기서 제외
	if (m_hover.is_valid() && m_hover.area != HitArea::Button && !(m_hover == m_sel))
		draw_hover_circle(g, m_hover);

	// 선택 체크마크
	if (m_sel.is_valid() && m_sel.area != HitArea::Button)
		draw_selected_mark(g, m_sel);
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

Gdiplus::Color CSCColorPicker::hsv_to_color(float h, float s, float v)
{
	float r = v, gr = v, b = v;
	if (s > 0.f)
	{
		h = fmodf(h, 360.f);
		if (h < 0.f) h += 360.f;
		const int   i = static_cast<int>(h / 60.f) % 6;
		const float f = h / 60.f - static_cast<int>(h / 60.f);
		const float p = v * (1.f - s);
		const float q = v * (1.f - f * s);
		const float t = v * (1.f - (1.f - f) * s);
		switch (i)
		{
		case 0: r = v;  gr = t;  b = p;  break;
		case 1: r = q;  gr = v;  b = p;  break;
		case 2: r = p;  gr = v;  b = t;  break;
		case 3: r = p;  gr = q;  b = v;  break;
		case 4: r = t;  gr = p;  b = v;  break;
		case 5: r = v;  gr = p;  b = q;  break;
		}
	}
	return Gdiplus::Color(255,
		static_cast<BYTE>(r * 255.f + 0.5f),
		static_cast<BYTE>(gr * 255.f + 0.5f),
		static_cast<BYTE>(b * 255.f + 0.5f));
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
	return hsv_to_color(m_hues[col], m_sv_rows[row - 1].s, m_sv_rows[row - 1].v);
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

	if (!m_recent_colors.empty() && !m_r_palette.IsRectEmpty())
	{
		// ② 최근 색상
		if (!m_recent_colors.empty())
		{
			for (int i = 0; i < (int)m_recent_colors.size(); ++i)
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
		}

		// ③ 버튼 (BTN_ADD_IDX만 활성; BTN_DROPPER_IDX는 구현 후 추가)
		for (int i = 0; i < 1 /*BTN_ADD_IDX only*/; ++i)
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
	// ── ① "+" 사용자 정의색 추가 버튼 ────────────────────
	{
		const HitTarget       t{ HitArea::Button, -1, -1, BTN_ADD_IDX };
		const Gdiplus::PointF c = get_cell_center(t);
		const bool            is_hov = (m_hover == t);
		const float           r = is_hov ? m_radius + 1.5f : m_radius;

		// 배경 원 (hover 시 더 진한 회색)
		const BYTE bg = is_hov ? 210 : 238;
		const BYTE border = is_hov ? 150 : 180;
		Gdiplus::SolidBrush bgBrush(Gdiplus::Color(bg, bg, bg));
		Gdiplus::Pen        borderPen(Gdiplus::Color(border, border, border), 1.0f);
		g.FillEllipse(&bgBrush, c.X - r, c.Y - r, r * 2.f, r * 2.f);
		g.DrawEllipse(&borderPen, c.X - r, c.Y - r, r * 2.f, r * 2.f);

		// "+" 기호
		const float arm = m_radius * 0.46f;
		Gdiplus::Pen plus(Gdiplus::Color(90, 90, 90), m_radius * 0.18f);
		plus.SetStartCap(Gdiplus::LineCapRound);
		plus.SetEndCap(Gdiplus::LineCapRound);
		g.DrawLine(&plus, c.X - arm, c.Y, c.X + arm, c.Y);		// 가로
		g.DrawLine(&plus, c.X, c.Y - arm, c.X, c.Y + arm);	// 세로
	}

	// ── ② 스포이드 버튼 (미구현 — 비활성 상태로 표시) ────
	{
		const HitTarget       t{ HitArea::Button, -1, -1, BTN_DROPPER_IDX };
		const Gdiplus::PointF c = get_cell_center(t);
		const float           r = m_radius;

		// 배경 원 (비활성: 연한 색)
		Gdiplus::SolidBrush bgBrush(Gdiplus::Color(245, 245, 245));
		Gdiplus::Pen        borderPen(Gdiplus::Color(210, 210, 210), 1.0f);
		g.FillEllipse(&bgBrush, c.X - r, c.Y - r, r * 2.f, r * 2.f);
		g.DrawEllipse(&borderPen, c.X - r, c.Y - r, r * 2.f, r * 2.f);

		// 스포이드 아이콘 (대각선 본체 + 노즐)
		Gdiplus::Pen icon(Gdiplus::Color(190, 190, 190), m_radius * 0.15f);
		icon.SetStartCap(Gdiplus::LineCapRound);
		icon.SetEndCap(Gdiplus::LineCapRound);
		const float a = m_radius * 0.38f;
		g.DrawLine(&icon, c.X + a * 0.8f, c.Y - a * 0.8f,		// 상단 (손잡이)
			c.X - a * 0.3f, c.Y + a * 0.3f);	// 중간
		g.DrawLine(&icon, c.X - a * 0.3f, c.Y + a * 0.3f,	// 중간
			c.X - a * 0.7f, c.Y + a * 0.7f);	// 하단 팁
	}
}

// ── "+" 클릭: 현재 선택 색상을 recent에 즉시 추가 ────────
void CSCColorPicker::on_btn_add_clicked()
{
	if (!m_sel.is_valid())
		return;

	const Gdiplus::Color cr = m_sel_color;

	// ARGB 완전 일치 중복 제거
	for (auto it = m_recent_colors.begin(); it != m_recent_colors.end(); )
	{
		if (it->GetValue() == cr.GetValue())
			it = m_recent_colors.erase(it);
		else
			++it;
	}

	// 맨 앞에 삽입 → 최대 개수 제한
	m_recent_colors.insert(m_recent_colors.begin(), cr);
	if ((int)m_recent_colors.size() > MAX_RECENT_COLORS)
		m_recent_colors.resize(MAX_RECENT_COLORS);

	// 방금 추가한 항목을 선택 상태로 유지
	m_sel = { HitArea::Recent, -1, -1, 0 };
	m_sel_color = cr;

	update_slider_alpha();	// ← 슬라이더 동기화 (alpha 유지 확인)
	Invalidate(FALSE);
}

// CSCColorPicker 메시지 처리기
void CSCColorPicker::OnBnClickedOk()
{
	// 색상이 선택된 상태로 확정했을 때만 레지스트리에 저장
	// (팔레트·최근색 모두 동일 처리)
	if (m_sel.is_valid())
		save_recent_color(m_sel_color);

	m_response = IDOK;

	if (!m_as_modal)
		ShowWindow(SW_HIDE);
}

void CSCColorPicker::OnBnClickedCancel()
{
	m_response = IDCANCEL;

	if (!m_as_modal)
		ShowWindow(SW_HIDE);
}

INT_PTR CSCColorPicker::DoModal(CString title, Gdiplus::Color cr_selected)
{
	//return CDialog::DoModal();

	// TODO: 여기에 특수화된 코드를 추가 및/또는 기본 클래스를 호출합니다.
	if (m_parent == nullptr)
		m_parent = GetParent();

	m_as_modal = true;

	load_recent_colors();

	if (title.IsEmpty())
		title = _T("Color Picker");

	if (cr_selected.GetValue() != Gdiplus::Color::Transparent)
	{
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
				m_sel_color = Gdiplus::Color(cr_selected.GetA(),
					palette_cr.GetR(),
					palette_cr.GetG(),
					palette_cr.GetB());
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
	color_to_hsv(m_sel_color, m_hue, m_sat, m_val);
	update_slider_hue();
	update_slider_light();
	sync_edits();

	SetWindowText(title);
	CenterWindow(m_parent);
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

	Gdiplus::Pen borderPen(Gdiplus::Color(50, 0, 0, 0), 1.0f);

	for (int i = 0; i < (int)m_recent_colors.size(); ++i)
	{
		const Gdiplus::PointF center = get_cell_center({ HitArea::Recent, -1, -1, i });
		const Gdiplus::Color  cr = m_recent_colors[i];

		draw_color_circle(g, center, m_radius, cr);	// 체커보드 + alpha 합성
		g.DrawEllipse(&borderPen, center.X - m_radius, center.Y - m_radius, m_radius * 2.f, m_radius * 2.f);
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

	const BYTE lum = static_cast<BYTE>(
		0.299f * cr.GetR() + 0.587f * cr.GetG() + 0.114f * cr.GetB());
	const Gdiplus::Color check_cr = (lum > 160)
		? Gdiplus::Color(230, 96, 96, 96)
		: Gdiplus::Color(230, 232, 232, 232);

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
	draw_line(&dc, rc.left + 12, (float)m_r_preview.top - m_margin - 0.5f, rc.right - 12, (float)m_r_preview.top - m_margin - 0.5f, GRAY(212));

	draw_color_preview(g);	// ④ 선택 색상 미리보기 (RoundRect)

	draw_overlays(g);		// ⑤ 색상 셀 hover 확대원 + 체크마크

	Gdiplus::Font        labelFont(L"Consolas", 11.0f, Gdiplus::FontStyleBold, Gdiplus::UnitPixel);
	Gdiplus::SolidBrush  labelBrush(Gdiplus::Color(64, 64, 64));
	Gdiplus::StringFormat sf;
	sf.SetAlignment(Gdiplus::StringAlignmentNear);
	sf.SetLineAlignment(Gdiplus::StringAlignmentNear);

	int cell_left = m_r_edit_area.left;
	Gdiplus::RectF label_rect;

	if (m_edit_hexa.GetSafeHwnd())
	{
		label_rect = Gdiplus::RectF(cell_left, m_r_edit_area.top - 16, 80, 24);
		g.DrawString(_T("ARGB"), -1, &labelFont, label_rect, &sf, &labelBrush);
	}

	if (m_edit_argb[0].GetSafeHwnd())
	{
		static const wchar_t* kLabels[4] = { L"A", L"R", L"G", L"B" };

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
}

void CSCColorPicker::OnLButtonDown(UINT nFlags, CPoint point)
{
	// TODO: 여기에 메시지 처리기 코드를 추가 및/또는 기본값을 호출합니다.
	HitTarget t;

	if (hit_test(point, t))
	{
		if (t.area == HitArea::Button)
		{
			if (t.idx == BTN_ADD_IDX)
				on_btn_add_clicked();
			// BTN_DROPPER_IDX: TODO
		}
		else
		{
			m_sel = t;
			m_sel_color = get_color(t);

			// 팔레트/최근색 선택 → HSV 추출 후 3개 슬라이더 모두 동기화
			color_to_hsv(m_sel_color, m_hue, m_sat, m_val);
			update_slider_alpha();
			update_slider_hue();
			update_slider_light();
			Invalidate(FALSE);
		}
	}

	CDialog::OnLButtonDown(nFlags, point);
}

void CSCColorPicker::OnLButtonUp(UINT nFlags, CPoint point)
{
	// TODO: 여기에 메시지 처리기 코드를 추가 및/또는 기본값을 호출합니다.
	sync_edits();

	CDialog::OnLButtonUp(nFlags, point);
}

void CSCColorPicker::OnMouseMove(UINT nFlags, CPoint point)
{
	// TODO: 여기에 메시지 처리기 코드를 추가 및/또는 기본값을 호출합니다.
	HitTarget t;
	hit_test(point, t);

	if (!(t == m_hover))
	{
		m_hover = t;
		Invalidate(FALSE);
	}

	CDialog::OnMouseMove(nFlags, point);
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
		// ── Hue 슬라이더 ──────────────────────────────────────────────
		if (!m_sel.is_valid())
			return 0;

		m_hue = static_cast<float>(max(0, min(360, msg->pos)));

		const Gdiplus::Color cr_new = hsv_to_color(m_hue, m_sat, m_val);
		m_sel_color = Gdiplus::Color(m_sel_color.GetA(),
			cr_new.GetR(), cr_new.GetG(), cr_new.GetB());

		update_slider_light();	// ← Hue 변경 시 Light 그라디언트 갱신
		update_slider_alpha();	// ← Alpha 그라디언트의 활성색 갱신
		sync_edits();
		InvalidateRect(&m_r_preview, FALSE);
	}
	else if (msg->pThis == &m_slider_light)
	{
		// ── Light(밝기) 슬라이더 ─────────────────────────────────────
		if (!m_sel.is_valid())
			return 0;

		m_val = static_cast<float>(max(0, min(100, msg->pos))) / 100.f;

		const Gdiplus::Color cr_new = hsv_to_color(m_hue, m_sat, m_val);
		m_sel_color = Gdiplus::Color(m_sel_color.GetA(),
			cr_new.GetR(), cr_new.GetG(), cr_new.GetB());

		update_slider_alpha();	// ← Alpha 그라디언트의 활성색 갱신
		sync_edits();
		InvalidateRect(&m_r_preview, FALSE);
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
				sync_edits();
				OnBnClickedOk();
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
		m_slider_alpha.set_active_color(
			Gdiplus::Color(255, m_sel_color.GetR(), m_sel_color.GetG(), m_sel_color.GetB()));
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
		m_slider_light.set_inactive_color(Gdiplus::Color(180, 180, 180));
		m_slider_light.set_active_color(Gdiplus::Color(225, 225, 225));
		m_slider_light.set_pos(100);
	}
	else
	{
		// Black(V=0) → 현재 Hue+Sat의 순색(V=1.0) 그라디언트
		const Gdiplus::Color cr_full = hsv_to_color(m_hue, m_sat, 1.0f);
		m_slider_light.set_inactive_color(Gdiplus::Color::Black);
		m_slider_light.set_active_color(cr_full);  // ← White → 현재 순색
		m_slider_light.set_pos(static_cast<int>(m_val * 100.f + 0.5f));
	}
	m_slider_light.Invalidate(FALSE);
}

void CSCColorPicker::OnLButtonDblClk(UINT nFlags, CPoint point)
{
	// TODO: 여기에 메시지 처리기 코드를 추가 및/또는 기본값을 호출합니다.
	sync_edits();
	OnBnClickedOk();

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
}
