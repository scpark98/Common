// SCColorPicker.cpp: 구현 파일
//

#include "SCColorPicker.h"

#include <afxdlgs.h>
#include "../../Functions.h"
#include "../../MemoryDC.h"

// ── static 멤버 정의 ──────────────────────────────────────
const float CSCColorPicker::m_hues[PALETTE_COLOR_COLS] = {
	0.f,    // Red
	45.f,   // Orange
	90.f,   // Yellow-Green
	135.f,  // Spring Green
	180.f,  // Cyan
	225.f,  // Azure
	270.f,  // Blue-Violet
	315.f,  // Magenta-Pink
};

const CSCColorPicker::PaletteSV CSCColorPicker::m_sv_rows[PALETTE_ROWS] = {
	//           s      v       기존값              변화량
	{ 0.10f, 1.00f },  // (0.15, 1.00)  Δs= +0.18 시작
	{ 0.28f, 1.00f },  // (0.30, 1.00)  Δs= +0.18
	{ 0.46f, 1.00f },  // (0.50, 1.00)  Δs= +0.18
	{ 0.64f, 1.00f },  // (0.70, 1.00)  Δs= +0.18
	{ 0.82f, 1.00f },  // (1.00, 1.00)  Δs= +0.18
	{ 1.00f, 1.00f },  // (1.00, 0.80)  Δs= +0.18  ← row 4↔5 명확히 구분
	{ 1.00f, 0.65f },  // (1.00, 0.60)  Δv= -0.35
	{ 1.00f, 0.30f },  // (1.00, 0.40)  Δv= -0.35};
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
	ON_WM_LBUTTONDBLCLK()
	ON_WM_ERASEBKGND()
	ON_EN_CHANGE(IDC_EDIT_HEXA, OnEnChangeHexa)
	ON_EN_CHANGE(IDC_EDIT_ARGB_A, OnEnChangeArgb)
	ON_EN_CHANGE(IDC_EDIT_ARGB_R, OnEnChangeArgb)
	ON_EN_CHANGE(IDC_EDIT_ARGB_G, OnEnChangeArgb)
	ON_EN_CHANGE(IDC_EDIT_ARGB_B, OnEnChangeArgb)
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
	LONG_PTR dwStyle = WS_POPUP | WS_VISIBLE | WS_CAPTION | WS_SYSMENU | WS_EX_TOOLWINDOW | WS_CLIPCHILDREN;

	WNDCLASS wc = {};
	::GetClassInfo(AfxGetInstanceHandle(), _T("#32770"), &wc);
	wc.lpszClassName = _T("CSCColorPicker");
	AfxRegisterClass(&wc);

	bool res = CreateEx(NULL, wc.lpszClassName, _T("CSCColorPicker"), dwStyle, CRect(0, 0, 500, 400), parent, 0);

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
	const int width = 256;

	m_margin = width * 0.04f;
	m_cell = (width - 2.f * m_margin) / static_cast<float>(PALETTE_TOTAL_COLS);
	m_radius = m_cell * 0.42f;

	const int palette_h = static_cast<int>(m_margin * 2.f + m_cell * PALETTE_ROWS + 0.5f);
	m_palette_rect = CRect(0, 0, width, palette_h);

	// 최근 색상 + 버튼 행의 하단 y
	const float recent_bottom = m_palette_rect.bottom + m_margin + m_cell;

	// 미리보기 섹션: 두 번째 구분선(recent_bottom + 4) 아래 4px 여백
	const float preview_size = m_cell * 1.5f;
	const float info_top = recent_bottom + 4.0f;

	m_preview_rect = CRect(m_margin + 0.5f, info_top, m_margin + preview_size + 0.5f, info_top + preview_size);

	// ── 알파 슬라이더: 미리보기 오른쪽, 수직 중앙 정렬 ──
	const int slider_h = 26;
	const int slider_cy = m_preview_rect.CenterPoint().y;
	m_slider_rect = CRect(
		m_preview_rect.right + static_cast<int>(m_margin),
		slider_cy - slider_h / 2,
		static_cast<int>(width - m_margin),
		slider_cy + slider_h / 2
	);

	int hexa_width = 80;
	int bottomRef = max(m_preview_rect.bottom, m_slider_rect.bottom);
	int editTop = bottomRef + 24;
	m_edit_area_rect = CRect(m_palette_rect.left + m_margin, editTop, m_palette_rect.right - m_margin, editTop + kEditH + kEditGap + kEditH);

	if (!m_edit_hexa.GetSafeHwnd())
	{
		//Hex (#AARRGGBB)
		m_edit_hexa.Create(
			WS_CHILD | WS_VISIBLE | WS_TABSTOP | WS_BORDER | ES_CENTER | ES_AUTOHSCROLL | ES_MULTILINE,
			CRect(m_edit_area_rect.left, m_edit_area_rect.top, m_edit_area_rect.left + hexa_width, m_edit_area_rect.top + kEditH), this, IDC_EDIT_HEXA);
		m_edit_hexa.LimitText(9);   // '#' + 8 hex digits
		m_edit_hexa.set_font_name(_T("Segoe UI"));
		m_edit_hexa.set_text_color(Gdiplus::Color(52, 68, 71, 70));
		m_edit_hexa.set_back_color(Gdiplus::Color::White);
		m_edit_hexa.set_border_color(Gdiplus::Color::LightGray);

		//ARGB (4등분, 레이블 공간 포함)
		const int cellW = (m_edit_area_rect.Width() - hexa_width - 3 * kCellGap) / 4;
		const int kIds[4] = { IDC_EDIT_ARGB_A, IDC_EDIT_ARGB_R, IDC_EDIT_ARGB_G, IDC_EDIT_ARGB_B };

		for (int i = 0; i < 4; i++)
		{
			const int x0 = m_edit_area_rect.left + hexa_width + i * (cellW + kCellGap);
			m_edit_argb[i].Create(
				WS_CHILD | WS_VISIBLE | WS_TABSTOP | WS_BORDER | ES_CENTER | ES_AUTOHSCROLL | ES_MULTILINE,
				CRect(x0 + kLabelW, m_edit_area_rect.top, x0 + cellW, m_edit_area_rect.top + kEditH),
				this, kIds[i]);
			m_edit_argb[i].LimitText(3);    // 0 ~ 255
			m_edit_argb[i].set_font_name(_T("Segoe UI"));
			m_edit_argb[i].set_text_color(Gdiplus::Color(52, 68, 71, 70));
			m_edit_argb[i].set_back_color(Gdiplus::Color::White);
			m_edit_argb[i].set_border_color(Gdiplus::Color::LightGray);
		}

		sync_edits();
	}
	else    // calc_layout() 재호출 시 위치만 갱신
	{
		m_edit_hexa.MoveWindow(
			CRect(m_edit_area_rect.left, m_edit_area_rect.top,
				m_edit_area_rect.right, m_edit_area_rect.top + kEditH));

		const int cellW = (m_edit_area_rect.Width() - 3 * kCellGap) / 4;
		const int row2Top = m_edit_area_rect.top + kEditH + kEditGap;

		for (int i = 0; i < 4; i++)
		{
			const int x0 = m_edit_area_rect.left + i * (cellW + kCellGap);
			m_edit_argb[i].MoveWindow(
				CRect(x0 + kLabelW, row2Top, x0 + cellW, row2Top + kEditH));
		}
	}

	//const int client_h = info_top + preview_size + m_margin + 0.5f;

	// 창 크기를 콘텐츠에 맞게 조정
	//const DWORD style = (DWORD)::GetWindowLong(m_hWnd, GWL_STYLE);
	//const DWORD styleEx = (DWORD)::GetWindowLong(m_hWnd, GWL_EXSTYLE);
	//CRect wr(0, 0, width, client_h);
	//::AdjustWindowRectEx(&wr, style, FALSE, styleEx);
	//SetWindowPos(nullptr, 0, 0, wr.Width(), wr.Height(),
	//	SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);

	// 다이얼로그 높이를 편집 영역이 들어오도록 확장
	CRect dlgRect;
	GetWindowRect(&dlgRect);
	int neededBottom = m_edit_area_rect.bottom + (int)m_margin;
	if (dlgRect.Height() < neededBottom)
	{
		SetWindowPos(nullptr, 0, 0,
			dlgRect.Width(), neededBottom + (int)m_margin,
			SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);
	}

	// ── 슬라이더 생성 or 위치 갱신 ─────────────────
	if (!m_slider.GetSafeHwnd())
	{
		m_slider.Create(
			WS_CHILD | WS_VISIBLE | TBS_HORZ | TBS_NOTICKS,
			m_slider_rect, this, IDC_SLIDER_ALPHA
		);
		m_slider.set_style(CSCSliderCtrl::style_thumb_round_alpha);
		m_slider.set_range(0, 255);
		//m_slider.set_track_height(10);
		m_slider.set_back_color(Gdiplus::Color::White);
		m_slider.set_text_style(CSCSliderCtrl::text_style_none);
		m_slider.set_pos(255);	// 기본 alpha = 불투명
	}
	else
	{
		m_slider.MoveWindow(m_slider_rect);
	}
}

// m_sel_color → 편집 컨트롤에 반영 (무한 루프 방지 플래그 포함)
void CSCColorPicker::sync_edits()
{
	if (!m_edit_hexa.GetSafeHwnd())
		return;

	static bool s_syncing = false;
	if (s_syncing)
		return;
	s_syncing = true;

	auto cr = m_sel_color;

	// Hex 편집: #AARRGGBB
	CString hex;
	hex.Format(_T("#%02X%02X%02X%02X"),
		cr.GetAlpha(), cr.GetRed(), cr.GetGreen(), cr.GetBlue());
	m_edit_hexa.SetWindowText(hex);

	// A / R / G / B 개별 편집
	CString val;
	val.Format(_T("%d"), (int)cr.GetAlpha()); m_edit_argb[0].SetWindowText(val);
	val.Format(_T("%d"), (int)cr.GetRed());   m_edit_argb[1].SetWindowText(val);
	val.Format(_T("%d"), (int)cr.GetGreen()); m_edit_argb[2].SetWindowText(val);
	val.Format(_T("%d"), (int)cr.GetBlue());  m_edit_argb[3].SetWindowText(val);

	s_syncing = false;
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
	update_slider_alpha();
	sync_edits();
	Invalidate(FALSE);
}

// ── OnEnChangeHaxa() ───────────────────────────────────────────
// Hex 편집 변경 → m_sel_color 파싱 반영
void CSCColorPicker::OnEnChangeHexa()
{
	if (!m_edit_hexa.GetSafeHwnd())
		return;

	CString text;
	m_edit_hexa.GetWindowText(text);
	text.TrimLeft(_T('#'));
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
	update_slider_alpha();
	sync_edits();   // ARGB 개별 박스 동기화 (hex는 s_syncing으로 스킵됨)
	Invalidate(FALSE);
}

// ── OnEnChangeArgb() ───────────────────────────────────────────
// ARGB 개별 편집 변경 → m_sel_color 반영
void CSCColorPicker::OnEnChangeArgb()
{
	apply_edit_to_color();
}

// ── 유일한 좌표 계산 함수 ────────────────────────────────
// hit_test·draw_* 모두 이 함수 하나에서 좌표를 얻음
Gdiplus::PointF CSCColorPicker::get_cell_center(const HitTarget& t) const
{
	if (t.area == HitArea::Palette)
	{
		return {
			m_palette_rect.left + m_margin + t.col * m_cell + m_cell * 0.5f,
			m_palette_rect.top + m_margin + t.row * m_cell + m_cell * 0.5f
		};
	}
	if (t.area == HitArea::Recent)
	{
		return {
			m_palette_rect.left + m_margin + t.idx * m_cell + m_cell * 0.5f,
			static_cast<float>(m_palette_rect.bottom) + 4.0f + m_cell * 0.5f
		};
	}
	if (t.area == HitArea::Button)
	{
		// 최근 색상 칸(MAX_RECENT_COLORS개) 바로 뒤에 순서대로 배치
		return {
			m_palette_rect.left + m_margin + (MAX_RECENT_COLORS + t.idx) * m_cell + m_cell * 0.5f,
			static_cast<float>(m_palette_rect.bottom) + 4.f + m_cell * 0.5f
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
	if (m_preview_rect.IsRectEmpty())
		return;

	const Gdiplus::RectF rf(
		static_cast<float>(m_preview_rect.left),
		static_cast<float>(m_preview_rect.top),
		static_cast<float>(m_preview_rect.Width()),
		static_cast<float>(m_preview_rect.Height())
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
	if (col == PALETTE_COLOR_COLS)	// 무채색 열
	{
		const float t = (PALETTE_ROWS > 1) ? 1.f - static_cast<float>(row) / (PALETTE_ROWS - 1) : 1.f;
		const BYTE  gray = static_cast<BYTE>(t * 255.f + 0.5f);
		return Gdiplus::Color(gray, gray, gray);
	}
	return hsv_to_color(m_hues[col], m_sv_rows[row].s, m_sv_rows[row].v);
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

	if (!m_recent_colors.empty() && !m_palette_rect.IsRectEmpty())
	{
		// ② 최근 색상
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

	if (m_as_modal)
		EndDialog(m_response);
	else
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

	SetWindowText(title);
	CenterWindow(m_parent);
	ShowWindow(SW_SHOW);
	Invalidate(FALSE);

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
	}

	m_parent->EnableWindow(TRUE);
	EndDialog(m_response);

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

	//구분선
	draw_line(&dc, rc.left + 12, m_palette_rect.bottom, rc.right - 12, m_palette_rect.bottom, GRAY(212));

	draw_recent_colors(g);	// ③ 최근 색상 원
	draw_buttons(g);		// ④ "+" 및 스포이드 버튼 (hover 자체 처리)

	//구분선
	draw_line(&dc, rc.left + 12, m_preview_rect.top - 4.5f, rc.right - 12, m_preview_rect.top - 4.5f, GRAY(212));

	draw_color_preview(g);	// ④ 선택 색상 미리보기 (RoundRect)

	draw_overlays(g);		// ⑤ 색상 셀 hover 확대원 + 체크마크

	Gdiplus::Font        labelFont(L"Segoe UI", 10.0f, Gdiplus::FontStyleBold, Gdiplus::UnitPixel);
	Gdiplus::SolidBrush  labelBrush(Gdiplus::Color(64, 64, 64));
	Gdiplus::StringFormat sf;
	sf.SetAlignment(Gdiplus::StringAlignmentNear);
	sf.SetLineAlignment(Gdiplus::StringAlignmentNear);

	int cell_left = m_edit_area_rect.left;
	Gdiplus::RectF label_rect;

	if (m_edit_hexa.GetSafeHwnd())
	{
		label_rect = Gdiplus::RectF(cell_left, m_edit_area_rect.top - 16, 80, 24);
		g.DrawString(_T("Hexa"), -1, &labelFont, label_rect, &sf, &labelBrush);
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
			update_slider_alpha();	// ← 슬라이더 위치·색상 동기화
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
	if (!msg || msg->pThis != &m_slider)
		return 0;

	// 선택된 색상이 없으면 슬라이더 이벤트를 무시
	if (!m_sel.is_valid())
		return 0;

	const BYTE alpha = static_cast<BYTE>(max(0, min(255, msg->pos)));
	m_sel_color = Gdiplus::Color(alpha,
		m_sel_color.GetR(),
		m_sel_color.GetG(),
		m_sel_color.GetB());

	sync_edits();

	// 전체 Invalidate(FALSE) 대신 미리보기 영역만 갱신
	InvalidateRect(&m_preview_rect, FALSE);
	return 0;
}

void CSCColorPicker::update_slider_alpha()
{
	if (!m_slider.GetSafeHwnd())
		return;

	if (!m_sel.is_valid())
	{
		m_slider.set_pos(255);
		m_slider.set_active_color(Gdiplus::Color(180, 180, 180));
		m_slider.set_inactive_color(Gdiplus::Color(225, 225, 225));
	}
	else
	{
		m_slider.set_pos(static_cast<int>(m_sel_color.GetA()));

		// active 구간 = 현재 RGB의 불투명 버전 → 어떤 색인지 한눈에 파악 가능
		m_slider.set_active_color(
			Gdiplus::Color(255, m_sel_color.GetR(), m_sel_color.GetG(), m_sel_color.GetB()));
		m_slider.set_inactive_color(Gdiplus::Color(225, 225, 225));
	}

	m_slider.Invalidate(FALSE);
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
