// SCStaticEdit.cpp
#include "SCStaticEdit.h"
#include <afxdlgs.h>	// CFileDialog (action_file)

// NOTE: DEBUG_NEW는 Gdiplus의 placement-new operator와 충돌하므로 사용하지 않음
// GDI+ 초기화/종료는 Common/SCGdiplusBitmap.h의 CGdiplusDummyForInitialization이
// 자동 처리하므로 본 클래스에서는 GdiplusStartup/GdiplusShutdown 호출하지 않음.

// 헬퍼: Gdiplus::Color → COLORREF (GDI TextOut/FillSolidRect용)
static inline COLORREF to_colorref(const Gdiplus::Color& c)
{
	return RGB(c.GetR(), c.GetG(), c.GetB());
}

// 드래그 선택 중 가장자리 자동 스크롤 타이머 ID
static const UINT_PTR TID_DRAG_AUTOSCROLL = 1;


// ──────────────────────────────────────────────────────────
// Message Map
// ──────────────────────────────────────────────────────────
BEGIN_MESSAGE_MAP(CSCStaticEdit, CStatic)
	ON_WM_PAINT()
	ON_WM_SETFOCUS()
	ON_WM_KILLFOCUS()
	ON_WM_CHAR()
	ON_WM_KEYDOWN()
	ON_WM_LBUTTONDOWN()
	ON_WM_LBUTTONUP()
	ON_WM_MOUSEMOVE()
	ON_WM_LBUTTONDBLCLK()
	ON_WM_CONTEXTMENU()
	ON_WM_ERASEBKGND()
	ON_WM_TIMER()
	ON_WM_ENABLE()
	ON_WM_SIZE()
	ON_WM_GETDLGCODE()
	ON_WM_SETCURSOR()
	ON_WM_MOUSEWHEEL()
	ON_MESSAGE(WM_IME_STARTCOMPOSITION, &CSCStaticEdit::on_ime_start_composition_message)
	ON_MESSAGE(WM_IME_ENDCOMPOSITION, &CSCStaticEdit::on_ime_end_composition_message)
	ON_MESSAGE(WM_IME_COMPOSITION, &CSCStaticEdit::on_ime_composition)
END_MESSAGE_MAP()

// ──────────────────────────────────────────────────────────
// 생성자 / 소멸자
// ──────────────────────────────────────────────────────────
CSCStaticEdit::CSCStaticEdit()
{
	set_color_theme(CSCColorTheme::color_theme_default, false);
}

CSCStaticEdit::~CSCStaticEdit()
{
}

// ──────────────────────────────────────────────────────────
// 컬러 테마 적용
// ──────────────────────────────────────────────────────────
void CSCStaticEdit::set_color_theme(int color_theme, bool invalidate)
{
	m_theme.set_color_theme(color_theme);

	//round 모서리 밖은 부모 dlg 배경으로 채우므로, 본문 배경을 edit 용으로 교정하기 *전에*
	//theme 본문 배경(= 부모 dlg bg 근사)을 parent_back 으로 잡아둔다.
	m_theme.cr_parent_back = m_theme.cr_back;

	//CSCStaticEdit 은 CEdit 처럼 보이지만 CStatic 파생이라 시스템색 테마(default/windows)의 control-kind
	//판정에서 COLOR_BTNFACE(회색) 를 받는다. 실제 edit 컨트롤(CSCEdit=CEdit-kind=COLOR_WINDOW)과 동일하게
	//보이도록 그 두 테마에서만 COLOR_WINDOW 로 교정. 그 외 테마는 CSCEdit 과 동일하게 theme 의
	//cr_back/cr_text 를 그대로 사용 — dark 계열에서 흰 카드로 떠 CSCEdit 과 어긋나던 문제 해소.
	//selection 도 같은 맥락 — 시스템색 테마의 cr_back_selected 는 list/tree 행용 연한 하이라이트
	//(204,235,255) 라 edit 텍스트 선택엔 어색하다. CEdit 처럼 시스템 highlight(파랑)+highlight text(흰)로 교정.
	if (color_theme == CSCColorTheme::color_theme_default || color_theme == CSCColorTheme::color_theme_windows)
	{
		m_theme.cr_back = get_sys_color(COLOR_WINDOW);
		m_theme.cr_back_selected = get_sys_color(COLOR_HIGHLIGHT);
		m_theme.cr_text_selected = get_sys_color(COLOR_HIGHLIGHTTEXT);
	}

	m_has_parent_back_color = true;

	if (invalidate && m_hWnd)
		Invalidate();
}

//호출자가 이미 수정해 둔 CSCColorTheme 을 그대로 적용. operator= 가 아닌 copy_colors_from()
//을 써서 m_parent (= 본 컨트롤) 는 보존. cr_parent_back 은 호출자가 의도적으로 세팅한 값
//(= 부모 dlg 의 cr_back) 그대로 둔다.
void CSCStaticEdit::set_color_theme(const CSCColorTheme& theme, bool invalidate)
{
	m_theme.copy_colors_from(theme);

	//copy_colors_from 이 parent_back(부모 dlg bg) 까지 가져왔으므로 그대로 둔다.
	//시스템색 테마(default/windows)일 때만 CStatic 파생이라 BTNFACE 로 잡힌 본문 배경을 edit 처럼
	//COLOR_WINDOW 로 교정. selection 도 같은 이유로 시스템 highlight 로 교정. (상세 근거는 set_color_theme(int) 주석 참조.)
	if (theme.get_color_theme() == CSCColorTheme::color_theme_default || theme.get_color_theme() == CSCColorTheme::color_theme_windows)
	{
		m_theme.cr_back = get_sys_color(COLOR_WINDOW);
		m_theme.cr_back_selected = get_sys_color(COLOR_HIGHLIGHT);
		m_theme.cr_text_selected = get_sys_color(COLOR_HIGHLIGHTTEXT);
	}

	m_has_parent_back_color = true;

	if (invalidate && m_hWnd)
		Invalidate();
}

// ──────────────────────────────────────────────────────────
// 동적 생성
// ──────────────────────────────────────────────────────────
bool CSCStaticEdit::Create(DWORD dw_style, const RECT& rect, CWnd* p_parent_wnd, UINT id)
{
	dw_style |= WS_CHILD | WS_VISIBLE | SS_NOTIFY | WS_TABSTOP;
	m_dynamic = true;
	return CStatic::Create(_T(""), dw_style, rect, p_parent_wnd, id) != 0;
}

// ──────────────────────────────────────────────────────────
// PreSubclassWindow ? SubclassWindow 직후 초기화
// ──────────────────────────────────────────────────────────
void CSCStaticEdit::PreSubclassWindow()
{
	CStatic::PreSubclassWindow();

	// SS_NOTIFY: 클릭 알림 수신 (마우스 클릭 → WM_LBUTTONDOWN 수신에 필요)
	ModifyStyle(0, SS_NOTIFY);

	// CSCStaticEdit 는 CEdit 처럼 동작하므로 dialog Tab 순서에 항상 포함되도록
	// resource editor 에서 WS_TABSTOP 을 빠뜨려도 자동으로 부여한다.
	ModifyStyle(0, WS_TABSTOP);

	// Resource editor에서 지정한 초기 캡션을 m_text로 반영.
	// 본 클래스의 GetWindowText 오버로드는 m_text를 반환하므로 여기서는
	// HWND의 실제 캡션을 읽기 위해 CStatic(=CWnd)의 것을 명시적으로 호출.
	CString initial_caption;
	CStatic::GetWindowText(initial_caption);
	if (!initial_caption.IsEmpty())
	{
		m_text      = initial_caption;
		m_caret_pos = m_text.GetLength();
		m_sel_start = m_sel_end = m_caret_pos;
	}

	// 부모 폰트 상속
	CFont* p_font = GetParent() ? GetParent()->GetFont() : nullptr;
	if (p_font)
		p_font->GetLogFont(&m_lf);
	else
		GetObject(GetStockObject(DEFAULT_GUI_FONT), sizeof(m_lf), &m_lf);

	rebuild_font();

	// 기존 윈도우 테두리 스타일 제거 (직접 그릴 것이므로)
	if (GetExStyle() & WS_EX_CLIENTEDGE)
		ModifyStyleEx(WS_EX_CLIENTEDGE, 0);
	if (GetStyle() & WS_BORDER)
		ModifyStyle(WS_BORDER, 0);

	// Resource Editor 에서 지정한 Static 정렬 스타일을 m_halign / m_valign 에 반영.
	// 본 클래스는 OnPaint 를 직접 구현하므로 SS_CENTER/SS_RIGHT 비트를 자체적으로
	// 해석해 주지 않으면 정렬이 무시되고 기본 DT_LEFT 로 그려진다.
	DWORD dw_style = GetStyle();
	switch (dw_style & SS_TYPEMASK)
	{
	case SS_CENTER:	m_halign = DT_CENTER;	break;
	case SS_RIGHT:	m_halign = DT_RIGHT;	break;
	default:		m_halign = DT_LEFT;		break;	//SS_LEFT(0) 및 그 외
	}
	if (dw_style & SS_CENTERIMAGE)
		m_valign = DT_VCENTER;

	// action 버튼 영역 툴팁.
	// 동적 Create 경로에서는 MFC 윈도우 생성 훅(m_pWndInit)이 이 컨트롤에 대해
	// 아직 살아있는 상태로 PreSubclassWindow 가 불린다. 여기서 자식 툴팁을 Create 하면
	// AfxHookWindowCreate 에 재진입해 ASSERT(m_pWndInit == NULL) 가 터진다.
	// 따라서 dialog template subclass(!m_dynamic)일 때만 즉시 생성하고,
	// 동적 컨트롤은 첫 사용 시점(ensure_action_tooltip)으로 지연한다.
	if (!m_dynamic)
		ensure_action_tooltip();
}

// 툴팁을 처음 필요한 시점에 생성(지연 생성). 생성 훅 재진입을 피하기 위해
// PreSubclassWindow 가 아닌 draw_action_button / update_action_tooltip /
// PreTranslateMessage 등 생성 완료 이후 호출 지점에서 불린다.
void CSCStaticEdit::ensure_action_tooltip()
{
	if (m_action_tooltip.GetSafeHwnd())
		return;
	if (m_hWnd == NULL)
		return;

	// tool id = 1. rect 는 draw_action_button 에서 SetToolRect 로 갱신.
	if (m_action_tooltip.Create(this, TTS_ALWAYSTIP))
		m_action_tooltip.AddTool(this, _T(""), CRect(0, 0, 0, 0), 1);
}

// ──────────────────────────────────────────────────────────
// 폰트
// ──────────────────────────────────────────────────────────
void CSCStaticEdit::rebuild_font()
{
	m_font.DeleteObject();
	m_lf.lfWidth = 0;
	m_lf.lfCharSet = DEFAULT_CHARSET;
	//패스워드 모드일 때만 lfQuality 를 ANTIALIASED 로 강제.
	//마스크 글리프(U+25CF ● 등) 는 원형 외곽선이라 ClearType 의 grid hinting 효과가 없고
	//grayscale AA 가 없으면 좌우 R/B subpixel fringe 와 톱니가 두드러진다 (사용자 캡처 확인).
	//draw_text 가 GDI TextOut 으로 그리므로 g.SetTextRenderingHint 가 아닌 lfQuality 가 결정.
	//평문 모드의 m_lf.lfQuality 는 외부 set_font_antialias 설정값 그대로 유지하기 위해 lf 로컬 복사본만 수정.
	LOGFONT lf = m_lf;
	if (m_password)
		lf.lfQuality = ANTIALIASED_QUALITY;
	m_font.CreateFontIndirect(&lf);

	// 폰트 높이 캐시 (m_hWnd 무관 ? screen DC 사용)
	HDC h_screen = ::GetDC(NULL);
	{
		CDC tmp; tmp.Attach(h_screen);
		CFont* p_old = tmp.SelectObject(&m_font);
		TEXTMETRIC tm = {};
		tmp.GetTextMetrics(&tm);
		m_font_height = tm.tmHeight;
		tmp.SelectObject(p_old);
		tmp.Detach();
	}
	::ReleaseDC(NULL, h_screen);

	if (m_hWnd)
	{
		//폰트가 바뀌면 글자 폭·높이가 달라지므로 캐럿 크기/위치를 새 메트릭으로 다시 계산한다.
		//(블록 선택 상태에서 폰트 변경 시 캐럿이 옛 글자 위치에 남던 문제 — update_caret_pos 미호출이 원인.)
		m_caret_height = min((int)m_font_height, (int)get_line_area().Height());
		update_caret_pos();
		Invalidate(FALSE);
	}
}

void CSCStaticEdit::SetFont(CFont* p_font, BOOL b_redraw)
{
	if (!p_font) return;
	p_font->GetLogFont(&m_lf);
	rebuild_font();
}

void CSCStaticEdit::set_font_name(LPCTSTR name)
{
	_tcscpy_s(m_lf.lfFaceName, name);
	rebuild_font();
}

void CSCStaticEdit::set_font_size(int size)
{
	m_lf.lfHeight = -MulDiv(size, GetDeviceCaps(::GetDC(nullptr), LOGPIXELSY), 72);
	rebuild_font();
}

void CSCStaticEdit::set_font_weight(int weight)
{
	m_lf.lfWeight = weight;
	rebuild_font();
}

void CSCStaticEdit::set_font_antialias(bool antialias)
{
	m_lf.lfQuality = antialias ? ANTIALIASED_QUALITY : DEFAULT_QUALITY;
	rebuild_font();
}

// ──────────────────────────────────────────────────────────
// 텍스트 설정 / 취득
// ──────────────────────────────────────────────────────────
void CSCStaticEdit::SetWindowText(LPCTSTR lpsz_string)
{
	set_text(lpsz_string ? CString(lpsz_string) : CString(_T("")));
}

void CSCStaticEdit::GetWindowText(CString& r_string) const
{
	r_string = m_text;
}

void CSCStaticEdit::set_text(const CString& text)
{
	m_text = text;
	m_caret_pos = m_text.GetLength();
	m_sel_start = m_caret_pos;
	m_sel_end = m_caret_pos;
	m_scroll_offset = 0;
	if (m_hWnd)
	{
		ensure_caret_visible();
		Invalidate(FALSE);
	}
}

void CSCStaticEdit::set_textf(LPCTSTR format, ...)
{
	va_list args;
	va_start(args, format);

	CString text;
	text.FormatV(format, args);

	set_text(text);
}

// ──────────────────────────────────────────────────────────
// 선택
// ──────────────────────────────────────────────────────────
void CSCStaticEdit::set_sel(int start, int end)
{
	int len = m_text.GetLength();
	if (end < 0)        // CEdit SetSel(0, -1) 관용구 — end 가 음수면 텍스트 끝까지 (전체 선택)
		end = len;
	m_sel_start = max(0, min(start, len));
	m_sel_end = max(0, min(end, len));
	m_caret_pos = m_sel_end;
	update_caret_pos();
	Invalidate(FALSE);
}

CString CSCStaticEdit::get_sel_text() const
{
	if (m_sel_start == m_sel_end) return _T("");
	int s = min(m_sel_start, m_sel_end);
	int e = max(m_sel_start, m_sel_end);
	return m_text.Mid(s, e - s);
}

void CSCStaticEdit::set_selection_color(Gdiplus::Color cr_back, Gdiplus::Color cr_text)
{
	m_theme.cr_back_selected = cr_back;
	m_theme.cr_text_selected = cr_text;
	Invalidate(FALSE);
}

// ──────────────────────────────────────────────────────────
// 옵션
// ──────────────────────────────────────────────────────────
void CSCStaticEdit::set_readonly(bool readonly)
{
	m_readonly = readonly;
	Invalidate(FALSE);
}

void CSCStaticEdit::set_use_default_readonly_color(bool use_default, Gdiplus::Color cr_back_readonly)
{
	m_use_default_readonly_color = use_default;
	if (!use_default && cr_back_readonly.GetValue() != Gdiplus::Color::Transparent)
		m_theme.cr_back = cr_back_readonly;
	Invalidate();
}

void CSCStaticEdit::set_use_default_disabled_color(bool use_default, Gdiplus::Color cr_back_disabled)
{
	m_use_default_disabled_color = use_default;
	if (cr_back_disabled.GetValue() != Gdiplus::Color::Transparent)
		m_cr_back_disabled = cr_back_disabled;
	Invalidate();
}

void CSCStaticEdit::set_password_mode(bool password, TCHAR mask_char)
{
	m_password = password;
	m_mask_char = mask_char;
	//rebuild_font 가 m_password 보고 lfQuality 를 결정 — 패스워드 모드 진입/이탈 시 폰트 재생성 필수.
	rebuild_font();
	Invalidate(FALSE);
}

void CSCStaticEdit::set_dim_text(const CString& dim_text, Gdiplus::Color cr_dim_text)
{
	m_dim_text = dim_text;
	if (cr_dim_text.GetValue() != Gdiplus::Color::Transparent)
		m_theme.cr_text_dim = cr_dim_text;
	Invalidate(FALSE);
}

void CSCStaticEdit::set_margin(CRect margin)
{
	m_margin = margin;
	if (m_hWnd)
	{
		ensure_caret_visible();
		update_caret_pos();
		Invalidate();
	}
}

void CSCStaticEdit::set_round(int radius)
{
	CRect rc;
	GetClientRect(rc);

	int max_r = rc.Height() / 2;

	// 반지름이 컨트롤 높이의 절반을 넘으면 트랙(알약) 모양 이상의 degenerate 렌더가
	// 되므로 height/2로 제한
	if (radius < 0 || radius > max_r)
		radius = max_r;

	m_border_radius = radius;
	Invalidate();
}

// ──────────────────────────────────────────────────────────
// WM_GETDLGCODE ? 다이얼로그 키 처리 위임.
//   - DLGC_WANTALLKEYS 를 넣으면 IsDialogMessage 가 Tab/Enter/Esc 를 가로채지 않아
//     Tab 포커스 이동이 동작하지 않는다. 싱글라인 CEdit 과 동일하게 CHARS/ARROWS/HASSETSEL
//     만 반환 → Tab 은 다이얼로그로 넘어가 다음 컨트롤로 포커스 이동.
//   - Enter/Escape 는 PreTranslateMessage 에서 dynamic 모드일 때만 우리 쪽에서 흡수하고,
//     일반 다이얼로그 모드에서는 기본 버튼/취소로 동작.
// ──────────────────────────────────────────────────────────
UINT CSCStaticEdit::OnGetDlgCode()
{
	return DLGC_WANTCHARS | DLGC_WANTARROWS | DLGC_HASSETSEL;
}

// ──────────────────────────────────────────────────────────
// 포커스
// ──────────────────────────────────────────────────────────
void CSCStaticEdit::OnSetFocus(CWnd* p_old_wnd)
{
	CStatic::OnSetFocus(p_old_wnd);

	// 캐럿 생성 ? 실제 폰트 높이 기준 (텍스트 영역 높이가 아님)
	{
		CClientDC dc(this);
		CFont* p_old = dc.SelectObject(&m_font);
		TEXTMETRIC tm = {};
		dc.GetTextMetrics(&tm);
		dc.SelectObject(p_old);
		// 캐럿 높이 = 글자 셀(tmHeight). 입력영역(get_line_area = client - border) 으로만 상한 clamp.
		// rc_text(세로 padding 제외) 로 clamp 하면 native EDIT 보다 캐럿이 짧아진다 (선택 블록과 동일 기준).
		m_caret_height = min((int)tm.tmHeight, (int)get_line_area().Height());
	}
	m_caret_width = get_caret_width();
	m_caret_cur_h = m_caret_height;
	CreateSolidCaret(m_caret_width, m_caret_cur_h);
	m_caret_visible = true;
	ShowCaret();

	// CEdit 처럼 포커스 획득 시 전체 선택. 마우스 클릭 경로는 OnLButtonDown 이 SetFocus() 직후
	// 클릭 위치로 selection 을 재설정하므로, tab/프로그램적 포커스에서만 전체 선택이 남는다.
	set_sel(0, -1);

	notify_parent(message_scstaticedit_setfocus);
}

void CSCStaticEdit::OnKillFocus(CWnd* p_new_wnd)
{
	HideCaret();
	DestroyCaret();
	m_caret_visible = false;
	m_selecting = false;

	// 조합 중이던 IME 문자 확정
	if (m_composing)
	{
		if (!m_compose.IsEmpty())
			insert_text(m_compose);
		m_compose = _T("");
		m_composing = false;
	}

	Invalidate(FALSE);
	notify_parent(message_scstaticedit_killfocus);

	CStatic::OnKillFocus(p_new_wnd);
}

// ──────────────────────────────────────────────────────────
// PreTranslateMessage ? Tab/Enter/Escape 처리
// ──────────────────────────────────────────────────────────
BOOL CSCStaticEdit::PreTranslateMessage(MSG* p_msg)
{
	if (m_action_tooltip.GetSafeHwnd())
		m_action_tooltip.RelayEvent(p_msg);

	if (p_msg->message == WM_KEYDOWN)
	{
		switch (p_msg->wParam)
		{
			case VK_RETURN:
				if (!m_dynamic)
					break; // 다이얼로그 기본 처리(OK)에 맡김
				notify_parent(message_scstaticedit_enter);
				return TRUE;

			case VK_ESCAPE:
				if (!m_dynamic)
					break;
				notify_parent(message_scstaticedit_escape);
				return TRUE;

			case VK_TAB:
			{
				// CStatic 기반이라 IsDialogMessage 의 Tab 처리가 동작하지 않아
				// WM_NEXTDLGCTL 로 직접 다음/이전 tab stop 으로 포커스 이동.
				CWnd* p_parent = GetParent();
				if (p_parent)
				{
					const bool shift = (::GetKeyState(VK_SHIFT) & 0x8000) != 0;
					p_parent->SendMessage(WM_NEXTDLGCTL, shift ? TRUE : FALSE, FALSE);
					return TRUE;
				}
				break;
			}
			}
	}
	return CStatic::PreTranslateMessage(p_msg);
}

// ──────────────────────────────────────────────────────────
// OnChar ? 출력 가능한 문자 입력
// ──────────────────────────────────────────────────────────
void CSCStaticEdit::OnChar(UINT n_char, UINT n_rep_cnt, UINT n_flags)
{
	// IME 조합 중에는 OnChar 무시 (OnImeComposition에서 처리)
	if (m_composing) return;

	// 컨트롤 문자 처리
	if (n_char < 0x20)
	{
		switch (n_char)
		{
		case 0x01: set_sel(0, -1);     break;  // Ctrl+A (readonly 여도 허용)
		case 0x03: do_copy();          break;  // Ctrl+C (readonly 여도 허용)
		case 0x16: if (!m_readonly) do_paste(); break;  // Ctrl+V
		case 0x18: if (!m_readonly) do_cut();   break;  // Ctrl+X
		case 0x1A: if (!m_readonly) do_undo();  break;  // Ctrl+Z
		case 0x19: if (!m_readonly) do_redo();  break;  // Ctrl+Y
		case 0x08:                            // Backspace
			if (m_readonly) break;
			push_undo();
			if (has_selection())
				delete_selection();
			else if (m_caret_pos > 0)
			{
				m_caret_pos--;
				m_text.Delete(m_caret_pos, 1);
				m_sel_start = m_sel_end = m_caret_pos;
				notify_parent(message_scstaticedit_text_changed);
			}
			break;
		}
	}
	else
	{
		// 일반 출력 가능 문자
		if (m_readonly) return;
		push_undo();
		insert_text(CString((TCHAR)n_char));
	}

	ensure_caret_visible();
	update_caret_pos();
	Invalidate(FALSE);
}

// ──────────────────────────────────────────────────────────
// OnKeyDown ? 방향키, Delete, Home, End
// ──────────────────────────────────────────────────────────
void CSCStaticEdit::OnKeyDown(UINT n_char, UINT n_rep_cnt, UINT n_flags)
{
	const bool shift = (::GetKeyState(VK_SHIFT) & 0x8000) != 0;
	const bool ctrl = (::GetKeyState(VK_CONTROL) & 0x8000) != 0;
	const int  text_len = m_text.GetLength();
	(void)ctrl;

	switch (n_char)
	{
	case VK_LEFT:
		if (shift)
		{
			if (m_caret_pos > 0) m_caret_pos--;
			m_sel_end = m_caret_pos;
		}
		else
		{
			if (has_selection())
				m_caret_pos = min(m_sel_start, m_sel_end);
			else if (m_caret_pos > 0)
				m_caret_pos--;
			clear_selection();
		}
		break;

	case VK_RIGHT:
		if (shift)
		{
			if (m_caret_pos < text_len) m_caret_pos++;
			m_sel_end = m_caret_pos;
		}
		else
		{
			if (has_selection())
				m_caret_pos = max(m_sel_start, m_sel_end);
			else if (m_caret_pos < text_len)
				m_caret_pos++;
			clear_selection();
		}
		break;

	case VK_HOME:
		if (shift)
		{
			m_caret_pos = 0;
			m_sel_end = 0;
		}
		else
		{
			m_caret_pos = 0;
			clear_selection();
		}
		break;

	case VK_END:
		if (shift)
		{
			m_caret_pos = text_len;
			m_sel_end = text_len;
		}
		else
		{
			m_caret_pos = text_len;
			clear_selection();
		}
		break;

	case VK_DELETE:
		if (!m_readonly)
		{
			push_undo();
			if (has_selection())
				delete_selection();
			else if (m_caret_pos < text_len)
			{
				m_text.Delete(m_caret_pos, 1);
				notify_parent(message_scstaticedit_text_changed);
			}
		}
		break;

	case VK_UP:
	case VK_DOWN:
		{
			// Shift 없이 누른 Up/Down 은 증감 대상 아님 ? 탭/폼 내비게이션 중 실수로
			// 값이 바뀌는 footgun 방지. MouseWheel 과 동일하게 Shift 필요.
			if (!shift || m_readonly || !m_use_updown_key || m_composing)
			{
				CStatic::OnKeyDown(n_char, n_rep_cnt, n_flags);
				return;
			}
			// _tcstod 로 전체가 실수인지 검사. 공백/부호/소수점 허용, 그 외 문자가 하나라도
			// 섞이면 스킵한다.
			CString trimmed = m_text;
			trimmed.Trim();
			if (trimmed.IsEmpty())
				return;
			LPCTSTR begin = trimmed;
			LPTSTR  p_end = nullptr;
			double val = _tcstod(begin, &p_end);
			if (p_end == begin || p_end == nullptr || *p_end != _T('\0'))
				return;
			push_undo();

			// step 결정 — m_updown_interval 값에 따라 두 모드:
			//   > 0 : 고정 step. 표시 자리수 = max(interval 자릿수, 텍스트 자릿수).
			//   == 0 : auto. 입력 텍스트의 소숫점 자릿수로 step = 10^(-text_prec).
			//     예) "0.5"→0.1, "0.000345"→1e-6, "5"→1.
			// %g 는 trailing 0 을 제거해 "0.09 + 0.01 = 0.10" 이 "0.1" 로 보이므로 %.Nf 로 고정 자리수 표시.
			int text_prec = 0;
			int dot = trimmed.ReverseFind(_T('.'));
			if (dot >= 0)
				text_prec = trimmed.GetLength() - dot - 1;

			double step;
			int    precision;
			if (m_updown_interval > 0.0f)
			{
				step = m_updown_interval;
				// interval 자리수 추출: float 의 이진 부정확성 때문에 반복 곱하기 방식은 오버카운트됨
				// (0.0001f 는 실제 9.99999974e-5 라 ×10 을 반복해도 fractional part 가 안 사라짐).
				// → (double) 승격 후 %g 문자열 표현에서 '.' / 'e' 위치로 직접 계산.
				CString s_iv;
				s_iv.Format(_T("%g"), (double)m_updown_interval);
				int iexp = s_iv.FindOneOf(_T("eE"));
				int idot = s_iv.Find(_T('.'));
				int mantissa_end  = (iexp >= 0) ? iexp : s_iv.GetLength();
				int mantissa_prec = (idot >= 0 && idot < mantissa_end) ? (mantissa_end - idot - 1) : 0;
				int exp_val       = (iexp >= 0) ? _ttoi(s_iv.Mid(iexp + 1)) : 0;
				int interval_prec = mantissa_prec - exp_val;
				if (interval_prec < 0)
					interval_prec = 0;
				precision = max(interval_prec, text_prec);
			}
			else
			{
				step = pow(10.0, -text_prec);
				precision = text_prec;
			}

			val += (n_char == VK_UP) ? step : -step;

			CString new_text;
			new_text.Format(_T("%.*f"), precision, val);
			m_text = new_text;
			m_caret_pos = m_text.GetLength();
			clear_selection();
			m_scroll_offset = 0;
			notify_parent(message_scstaticedit_text_changed);
		}
		break;

	default:
		CStatic::OnKeyDown(n_char, n_rep_cnt, n_flags);
		return;
	}

	ensure_caret_visible();
	update_caret_pos();
	Invalidate(FALSE);
}

// ──────────────────────────────────────────────────────────
// 마우스
// ──────────────────────────────────────────────────────────
void CSCStaticEdit::OnLButtonDown(UINT n_flags, CPoint point)
{
	// Action 버튼 위 클릭은 텍스트 선택/포커스 이동 없이 버튼 동작만
	if (has_action_button() && m_r_action_button.PtInRect(point))
	{
		m_action_button_capturing = true;
		m_action_button_pressed   = true;
		SetCapture();
		Invalidate(FALSE);
		return;
	}

	SetFocus();

	int pos = hit_test_char(point);
	if (::GetKeyState(VK_SHIFT) & 0x8000)
	{
		// Shift+클릭: 선택 확장
		m_caret_pos = pos;
		m_sel_end = pos;
	}
	else
	{
		m_caret_pos = pos;
		m_sel_start = pos;
		m_sel_end = pos;
	}

	m_selecting = true;
	SetCapture();

	update_caret_pos();
	Invalidate(FALSE);
	CStatic::OnLButtonDown(n_flags, point);
}

void CSCStaticEdit::OnLButtonUp(UINT n_flags, CPoint point)
{
	if (m_action_button_capturing)
	{
		// pressed 상태로 mouseup 되면 = 영역 안에서 손 뗀 것 → 클릭 성공
		bool fire = m_action_button_pressed;
		m_action_button_capturing = false;
		m_action_button_pressed   = false;
		if (GetCapture() == this)
			ReleaseCapture();
		Invalidate(FALSE);
		if (fire)
			perform_action_button_click();
		return;
	}

	m_selecting = false;
	if (m_drag_scrolling)
	{
		KillTimer(TID_DRAG_AUTOSCROLL);
		m_drag_scrolling = false;
	}
	if (GetCapture() == this)
		ReleaseCapture();
	CStatic::OnLButtonUp(n_flags, point);
}

void CSCStaticEdit::OnMouseMove(UINT n_flags, CPoint point)
{
	// Action 버튼 캡처 중: 영역 내/외 따라 pressed 시각만 토글 (드래그 아웃하면 release)
	if (m_action_button_capturing)
	{
		bool inside = m_r_action_button.PtInRect(point);
		if (inside != m_action_button_pressed)
		{
			m_action_button_pressed = inside;
			Invalidate(FALSE);
		}
		return;
	}

	if (m_selecting && (n_flags & MK_LBUTTON))
	{
		int pos = hit_test_char(point);
		m_caret_pos = pos;
		m_sel_end   = pos;

		// 드래그 중에도 스크롤이 캐럿을 따라가야 캐럿이 컨트롤 밖에 그려지지 않음
		ensure_caret_visible();
		update_caret_pos();
		Invalidate(FALSE);

		// 마우스가 좌/우 가장자리 밖으로 나간 상태로 멈추면 OnMouseMove 가 더 이상
		// 오지 않으므로 타이머로 연속 스크롤. 안으로 돌아오면 타이머 정지.
		m_last_drag_point = point;
		CRect rc_text = get_text_area();
		bool at_edge = (point.x < rc_text.left) || (point.x > rc_text.right);
		if (at_edge && !m_drag_scrolling)
		{
			SetTimer(TID_DRAG_AUTOSCROLL, 50, nullptr);
			m_drag_scrolling = true;
		}
		else if (!at_edge && m_drag_scrolling)
		{
			KillTimer(TID_DRAG_AUTOSCROLL);
			m_drag_scrolling = false;
		}
	}
	CStatic::OnMouseMove(n_flags, point);
}

BOOL CSCStaticEdit::OnSetCursor(CWnd* p_wnd, UINT n_hit_test, UINT message)
{
	if (IsWindowEnabled() && n_hit_test == HTCLIENT)
	{
		// Action 버튼 위에서는 화살표 커서 (텍스트 영역만 IBEAM)
		if (has_action_button())
		{
			CPoint pt;
			::GetCursorPos(&pt);
			ScreenToClient(&pt);
			if (m_r_action_button.PtInRect(pt))
			{
				::SetCursor(::LoadCursor(nullptr, IDC_ARROW));
				return TRUE;
			}
		}
		::SetCursor(::LoadCursor(nullptr, IDC_IBEAM));
		return TRUE;
	}
	return CStatic::OnSetCursor(p_wnd, n_hit_test, message);
}

// Shift+MouseWheel = VK_UP / VK_DOWN 과 동일한 수치 증감.
// 게이트(readonly / use_updown_key / composing) 를 여기서 한 번 확인한 뒤
// OnKeyDown 의 VK_UP/VK_DOWN 분기로 위임 ? 파싱/자리수 로직 재사용.
BOOL CSCStaticEdit::OnMouseWheel(UINT n_flags, short z_delta, CPoint pt)
{
	if ((n_flags & MK_SHIFT) && !m_readonly && m_use_updown_key && !m_composing)
	{
		UINT key = (z_delta > 0) ? VK_UP : VK_DOWN;
		int ticks = abs((int)z_delta) / WHEEL_DELTA;
		if (ticks <= 0)
			ticks = 1;
		for (int i = 0; i < ticks; i++)
			OnKeyDown(key, 1, 0);
		return TRUE;
	}
	return CStatic::OnMouseWheel(n_flags, z_delta, pt);
}

void CSCStaticEdit::OnLButtonDblClk(UINT n_flags, CPoint point)
{
	// 더블클릭: 단어 선택
	int pos = hit_test_char(point);
	int len = m_text.GetLength();

	// 단어 시작 찾기
	int start = pos;
	while (start > 0 && !_istspace(m_text[start - 1]))
		start--;

	// 단어 끝 찾기
	int end = pos;
	while (end < len && !_istspace(m_text[end]))
		end++;

	m_sel_start = start;
	m_sel_end = end;
	m_caret_pos = end;
	update_caret_pos();
	Invalidate(FALSE);
}

// ──────────────────────────────────────────────────────────
// 컨텍스트 메뉴 (우클릭 / Shift+F10 / 메뉴 키)
//   - CEdit 표준 메뉴의 주요 편집 항목(실행 취소/잘라내기/복사/붙여넣기/삭제/모두 선택)만 구현.
//   - 상태(readonly, 선택 유무, 클립보드 내용, undo 스택)에 따라 enable/disable.
// ──────────────────────────────────────────────────────────
void CSCStaticEdit::OnContextMenu(CWnd* /*p_wnd*/, CPoint point)
{
	// 메뉴키(Shift+F10 포함)는 point = (-1,-1)로 옴 → 캐럿 근처로 배치
	if (point.x == -1 && point.y == -1)
	{
		CPoint pt_caret = calc_caret_pixel_pos(m_caret_pos);
		ClientToScreen(&pt_caret);
		point = pt_caret;
	}

	// 포커스 없는 상태에서도 메뉴는 뜨므로 명령 실행 시 포커스 확보
	SetFocus();

	bool has_sel   = has_selection();
	bool b_can_undo = can_undo();
	bool can_paste = !m_readonly && ::IsClipboardFormatAvailable(
#ifdef UNICODE
		CF_UNICODETEXT
#else
		CF_TEXT
#endif
	) != FALSE;
	bool has_text  = !m_text.IsEmpty();

	enum {
		ID_UNDO = 1, ID_CUT, ID_COPY, ID_PASTE, ID_DELETE, ID_SELALL
	};

	UINT f_undo  = b_can_undo               ? MF_ENABLED : MF_GRAYED;
	UINT f_cut   = (has_sel && !m_readonly) ? MF_ENABLED : MF_GRAYED;
	UINT f_copy  = has_sel                  ? MF_ENABLED : MF_GRAYED;
	UINT f_paste = can_paste                ? MF_ENABLED : MF_GRAYED;
	UINT f_del   = (has_sel && !m_readonly) ? MF_ENABLED : MF_GRAYED;
	UINT f_all   = has_text                 ? MF_ENABLED : MF_GRAYED;

	CMenu menu;
	menu.CreatePopupMenu();
	menu.AppendMenu(MF_STRING | f_undo,  ID_UNDO,   _T("실행 취소(&U)"));
	menu.AppendMenu(MF_SEPARATOR);
	menu.AppendMenu(MF_STRING | f_cut,   ID_CUT,    _T("잘라내기(&T)"));
	menu.AppendMenu(MF_STRING | f_copy,  ID_COPY,   _T("복사(&C)"));
	menu.AppendMenu(MF_STRING | f_paste, ID_PASTE,  _T("붙여넣기(&P)"));
	menu.AppendMenu(MF_STRING | f_del,   ID_DELETE, _T("삭제(&D)"));
	menu.AppendMenu(MF_SEPARATOR);
	menu.AppendMenu(MF_STRING | f_all,   ID_SELALL, _T("모두 선택(&A)"));

	UINT cmd = menu.TrackPopupMenu(
		TPM_LEFTALIGN | TPM_RIGHTBUTTON | TPM_RETURNCMD,
		point.x, point.y, this);

	switch (cmd)
	{
	case ID_UNDO:  do_undo();    break;
	case ID_CUT:   do_cut();     break;
	case ID_COPY:  do_copy();    break;
	case ID_PASTE: do_paste();   break;
	case ID_DELETE:
		if (!m_readonly && has_sel)
		{
			push_undo();
			delete_selection();
			ensure_caret_visible();
			update_caret_pos();
			Invalidate(FALSE);
		}
		break;
	case ID_SELALL: set_sel(0, -1); break;
	}
}

// ──────────────────────────────────────────────────────────
// IME 처리 (한글 입력)
// ──────────────────────────────────────────────────────────
LRESULT CSCStaticEdit::on_ime_start_composition_message(WPARAM w_param, LPARAM l_param)
{
	//선택 영역이 있는 상태에서 한글 조합을 시작하면 *첫 자모* 순간 선택 영역이 교체돼야 표준 동작과 일치한다.
	//(영문은 WM_CHAR→insert_text 라 즉시 교체되지만, 한글은 첫 음절 확정(GCS_RESULTSTR)이 두 번째 글자에서야
	// 일어나 그때까지 선택이 남아 있던 버그.) 조합 시작 시 선택을 먼저 지워 조합 문자열이 빈 캐럿 위치에서 그려지게 한다.
	if (!m_readonly && has_selection())
	{
		push_undo();
		delete_selection();
		m_composition_undo_pushed = true;	//첫 RESULTSTR 의 중복 push_undo 방지.
	}

	// 캐럿 위치를 IME에 알려줌
	CPoint pt = calc_caret_pixel_pos(m_caret_pos);
	HIMC himc = ImmGetContext(m_hWnd);
	if (himc)
	{
		COMPOSITIONFORM cf = {};
		cf.dwStyle = CFS_POINT;
		cf.ptCurrentPos = pt;
		ImmSetCompositionWindow(himc, &cf);

		// 현재 폰트를 IME에 적용
		LOGFONT lf = m_lf;
		ImmSetCompositionFont(himc, &lf);

		ImmReleaseContext(m_hWnd, himc);
	}
	m_composing = true;
	return 0;
}

LRESULT CSCStaticEdit::on_ime_end_composition_message(WPARAM w_param, LPARAM l_param)
{
	m_compose = _T("");
	m_composing = false;
	//조합이 ESC 등으로 결과 없이 끝난 경우(첫 RESULTSTR 미발생) 플래그 잔존 방지 — 다음 조합에 오염되지 않도록 리셋.
	m_composition_undo_pushed = false;
	Invalidate(FALSE);
	return 0;
}

LRESULT CSCStaticEdit::on_ime_composition(WPARAM w_param, LPARAM l_param)
{
	if (m_readonly) return 0;

	HIMC himc = ImmGetContext(m_hWnd);
	if (!himc) return 0;

	// ImmGetCompositionString 주의:
	//  - 반환/요청 길이는 BYTE 단위 (WCHAR 단위 아님)
	//  - 버퍼를 null-terminate 하지 않음 → 반드시 ReleaseBuffer(n_chars)로 명시 길이 전달
	if (l_param & GCS_RESULTSTR)
	{
		int n_bytes = ImmGetCompositionString(himc, GCS_RESULTSTR, nullptr, 0);
		if (n_bytes > 0)
		{
			int n_chars = n_bytes / sizeof(TCHAR);
			CString result;
			TCHAR* buf = result.GetBufferSetLength(n_chars);
			ImmGetCompositionString(himc, GCS_RESULTSTR, buf, n_bytes);
			result.ReleaseBuffer(n_chars);

			m_compose = _T("");
			//조합 시작에서 이미 선택 삭제 + undo push 했으면 첫 음절은 그 undo 에 합쳐 한 번에 되돌리게 중복 push 생략.
			if (!m_composition_undo_pushed)
				push_undo();
			m_composition_undo_pushed = false;
			insert_text(result);
		}
	}
	else if (l_param & GCS_COMPSTR)
	{
		int n_bytes = ImmGetCompositionString(himc, GCS_COMPSTR, nullptr, 0);
		m_compose.Empty();
		if (n_bytes > 0)
		{
			int n_chars = n_bytes / sizeof(TCHAR);
			TCHAR* buf = m_compose.GetBufferSetLength(n_chars);
			ImmGetCompositionString(himc, GCS_COMPSTR, buf, n_bytes);
			m_compose.ReleaseBuffer(n_chars);
		}
	}

	ImmReleaseContext(m_hWnd, himc);
	ensure_caret_visible();
	update_caret_pos();
	Invalidate(FALSE);
	return 0;
}

// ──────────────────────────────────────────────────────────
// 기타 메시지
// ──────────────────────────────────────────────────────────
BOOL CSCStaticEdit::OnEraseBkgnd(CDC* p_dc)
{
	return TRUE; // OnPaint에서 전부 그리므로 배경 지우기 불필요
}

void CSCStaticEdit::OnTimer(UINT_PTR id_event)
{
	if (id_event == TID_DRAG_AUTOSCROLL)
	{
		// 드래그가 끝났거나 캡처가 풀렸으면 타이머 정리
		if (!m_selecting || GetCapture() != this)
		{
			KillTimer(TID_DRAG_AUTOSCROLL);
			m_drag_scrolling = false;
			return;
		}

		CRect rc_text = get_text_area();
		bool at_left  = m_last_drag_point.x < rc_text.left;
		bool at_right = m_last_drag_point.x > rc_text.right;
		if (!at_left && !at_right)
		{
			KillTimer(TID_DRAG_AUTOSCROLL);
			m_drag_scrolling = false;
			return;
		}

		// 방향에 따라 캐럿을 한 문자씩 이동. ensure_caret_visible 이 스크롤 갱신.
		int len = m_text.GetLength();
		int old_caret = m_caret_pos;
		if (at_left  && m_caret_pos > 0)    m_caret_pos--;
		if (at_right && m_caret_pos < len)  m_caret_pos++;

		if (m_caret_pos == old_caret)
			return;  // 경계(0 또는 len)에 도달 → 타이머는 유지하되 아무 일도 안 함

		m_sel_end = m_caret_pos;
		ensure_caret_visible();
		update_caret_pos();
		Invalidate(FALSE);
		return;
	}
	CStatic::OnTimer(id_event);
}

void CSCStaticEdit::OnEnable(BOOL b_enable)
{
	CStatic::OnEnable(b_enable);
	Invalidate(FALSE);
}

void CSCStaticEdit::OnSize(UINT n_type, int cx, int cy)
{
	CStatic::OnSize(n_type, cx, cy);
	// 컨트롤 크기가 변하면 border radius 상한도 변하므로 재적용
	int max_r = cy / 2;
	if (m_border_radius > max_r)
		m_border_radius = max_r;

	Invalidate(FALSE);
}

// ──────────────────────────────────────────────────────────
// OnPaint ? 핵심: 모든 그리기를 Gdiplus로 직접
// ──────────────────────────────────────────────────────────
void CSCStaticEdit::OnPaint()
{
	CPaintDC dc(this);
	CRect rc_client;
	GetClientRect(rc_client);

	// 플리커 방지: 메모리 DC에 먼저 그리고 한 번에 비트블록
	CDC     mem_dc;
	CBitmap mem_bmp;
	mem_dc.CreateCompatibleDC(&dc);
	mem_bmp.CreateCompatibleBitmap(&dc, rc_client.Width(), rc_client.Height());
	CBitmap* p_old_bmp = mem_dc.SelectObject(&mem_bmp);

	// round rect 바깥쪽 코너 채움:
	//   ① set_parent_back_color() 로 명시된 색 (m_has_parent_back_color == true) ? 호출자 지정.
	//   ② 미지정 → COLOR_3DFACE fallback.
	// 주의: Gdiplus::Color() default 생성자는 Black opaque 로 초기화되므로 alpha 값으로
	// "미지정" 을 판단할 수 없고 별도 플래그가 필요하다.
	if (m_border_radius > 0)
	{
		if (m_has_parent_back_color)
			mem_dc.FillSolidRect(&rc_client, m_theme.cr_parent_back.ToCOLORREF());
		else
			mem_dc.FillSolidRect(&rc_client, GetSysColor(COLOR_3DFACE));
	}

	{
		Gdiplus::Graphics g(mem_dc.GetSafeHdc());
		g.SetSmoothingMode(Gdiplus::SmoothingModeAntiAlias);
		// 기본 PixelOffsetMode(=None)에서는 AA 처리 시 우/하단 1px이 부분 coverage로
		// 렌더링되어 경계가 흐리게 잘린다. Half로 설정하면 geometric 좌표가
		// 픽셀 경계에 정렬되어 rc_client의 네 변이 온전히 그려진다.
		g.SetPixelOffsetMode(Gdiplus::PixelOffsetModeHalf);
		//텍스트는 draw_text 안에서 GDI dc.TextOut 으로 그려진다 — 여기서 설정하는
		//SetTextRenderingHint 는 GDI+ DrawString 경로용이라 텍스트에 영향 없음.
		//(GDI TextOut 의 AA 는 HFONT::lfQuality 가 결정 — rebuild_font 가 m_password 보고 자동 설정.)
		//draw_dim_text 도 GDI 경로라 동일.
		g.SetTextRenderingHint(Gdiplus::TextRenderingHintClearTypeGridFit);

		CRect rc_text = get_text_area();

		draw_background(g, rc_client);
		draw_border(g, rc_client);

		// 클리핑: 가로는 rc_text 로 제한(텍스트 좌/우 넘침 방지), 세로는 rc_client 로 열어둠.
		// 세로까지 rc_text 로 제한하면 선택 블록(GDI+로 그림)이 컨트롤 높이가 작을 때
		// 세로로 잘려 빔 캐럿(시스템 캐럿이라 클립 무시) 높이보다 작아 보인다.
		Gdiplus::Region clip_region(Gdiplus::RectF(
			(float)rc_text.left, (float)rc_client.top,
			(float)rc_text.Width(), (float)rc_client.Height()));
		g.SetClip(&clip_region);

		bool empty = m_text.IsEmpty() && m_compose.IsEmpty();
		bool focused = (GetFocus() == this);

		if (empty && !focused && !m_dim_text.IsEmpty())
			draw_dim_text(g, rc_text);
		else
		{
			//focus 없을 때 selection 시각 hide — native CEdit (ES_NOHIDESEL 미설정) 동작.
			//selection 상태(m_sel_start/_end) 자체는 보존되어 focus 다시 얻으면 재표시.
			if (focused)
				draw_selection(g, rc_text);
			draw_text(g, rc_text);
		}

		// 텍스트 영역 클립을 해제한 뒤 prefix 이미지 / action 버튼을 그린다 (둘 다 클립 밖)
		g.ResetClip();
		draw_prefix_image(g);
		draw_action_button(g);
	}

	dc.BitBlt(0, 0, rc_client.Width(), rc_client.Height(), &mem_dc, 0, 0, SRCCOPY);
	mem_dc.SelectObject(p_old_bmp);
}

// ──────────────────────────────────────────────────────────
// 그리기 헬퍼들
// ──────────────────────────────────────────────────────────
void CSCStaticEdit::draw_background(Gdiplus::Graphics& g, const CRect& rc)
{
	// disabled : 기본 LightGray. set_use_default_disabled_color(false, cr) 로 테마 색 지정 가능.
	// readonly + use_default_readonly_color : 표준 다이얼로그 배경(COLOR_3DFACE) 그대로.
	Gdiplus::Color cr_back =
		!IsWindowEnabled()                                       ? (m_use_default_disabled_color ? Gdiplus::Color::LightGray : m_cr_back_disabled) :
		(m_readonly && m_use_default_readonly_color)             ? get_sys_color(COLOR_3DFACE)         :
		                                                           m_theme.cr_back;

	Gdiplus::SolidBrush brush(cr_back);

	if (m_border_radius > 0)
	{
		// fill 용도이므로 stroke_thick = 0 으로 full rect 에 꽉 채움
		Gdiplus::GraphicsPath path;
		get_round_rect_path(&path,
			Gdiplus::Rect(rc.left, rc.top, rc.Width(), rc.Height()),
			(float)m_border_radius, 0);
		g.FillPath(&brush, &path);
	}
	else
	{
		g.FillRectangle(&brush, rc.left, rc.top, rc.Width(), rc.Height());
	}
}

void CSCStaticEdit::draw_border(Gdiplus::Graphics& g, const CRect& rc)
{
	if (!m_draw_border) return;

	//disable 시에는 border 를 그리지 않는다(CSCEdit 의 disable 모습과 통일 — 배경만 회색으로).
	if (!IsWindowEnabled()) return;

	bool focused = (GetFocus() == this);
	Gdiplus::Color cr_border = focused ? m_theme.cr_border_active : m_theme.cr_border_inactive;

	Gdiplus::Pen pen(cr_border, (float)m_border_width);
	// PenAlignmentInset 은 GDI+ 에서 곡선 path 의 AA 파이프라인을 완전히 타지 않아
	// 코너가 거칠게 그려진다. 기본 Center alignment 를 쓰고 path 를 half 픽셀
	// translate 하여 stroke 가 rc 안에 정확히 수용되도록 보정.

	if (m_border_radius > 0)
	{
		float half = (float)m_border_width / 2.0f;

		// get_round_rect_path 는 stroke_thick 만큼 right/bottom 을 줄여주지만
		// top/left 는 유지하므로 Matrix 로 (half, half) 이동시켜 대칭 보정.
		// radius 도 outer stroke 기준 m_border_radius 와 일치시키려면 half 만큼 작게 전달.
		Gdiplus::GraphicsPath path;
		get_round_rect_path(&path,
			Gdiplus::Rect(rc.left, rc.top, rc.Width(), rc.Height()),
			(float)m_border_radius - half, m_border_width);

		Gdiplus::Matrix mt;
		mt.Translate(half, half);
		path.Transform(&mt);

		g.DrawPath(&pen, &path);
	}
	else
	{
		float half = (float)m_border_width / 2.0f;
		Gdiplus::RectF rc_f(
			(float)rc.left + half,
			(float)rc.top + half,
			(float)rc.Width() - (float)m_border_width,
			(float)rc.Height() - (float)m_border_width);
		g.DrawRectangle(&pen, rc_f);
	}
}

int CSCStaticEdit::get_text_top(const CRect& rc_text, int text_h) const
{
	if (m_valign & DT_VCENTER)
		return rc_text.top + (rc_text.Height() - text_h) / 2;
	else if (m_valign & DT_BOTTOM)
		return rc_text.bottom - text_h;
	return rc_text.top;
}

void CSCStaticEdit::draw_selection(Gdiplus::Graphics& g, const CRect& rc_text)
{
	if (!has_selection()) return;

	int sel_s = min(m_sel_start, m_sel_end);
	int sel_e = max(m_sel_start, m_sel_end);

	CPoint pt_s = calc_caret_pixel_pos(sel_s);
	CPoint pt_e = calc_caret_pixel_pos(sel_e);

	// 선택 블록 = native EDIT 의 selection 과 동일한 "문자 셀 박스".
	// EDIT 은 선택 글자를 SetBkMode(OPAQUE)+highlight bk 로 그리므로 블록이 ExtTextOut 의 opaque box
	// (높이 = tmHeight, top = 글자가 그려지는 text_y) 와 정확히 같다. 셀 top 은 tmInternalLeading 만큼
	// 가시 글자(대문자 윗선) 위에 있어 "글자보다 살짝 위로 올라간" CEdit 외관이 metric 으로 자동 재현되고,
	// 글자 그리기와 동일한 get_text_top/tmHeight 를 쓰므로 폰트가 바뀌어도 그대로 일치한다. (padding 가산 없음)
	CClientDC dc(this);
	CFont* p_old = dc.SelectObject(&m_font);
	TEXTMETRIC tm = {};
	dc.GetTextMetrics(&tm);
	dc.SelectObject(p_old);

	// clamp 기준은 rc_text 가 아니라 입력영역(get_line_area = client - border). rc_text 는 세로 padding
	// 까지 빼므로 단일행에서 rc_text.Height() < tmHeight 가 되어 블록이 셀보다 잘려 얇아진다.
	CRect line = get_line_area();

	int text_h = tm.tmHeight;
	int h = min(text_h, (int)line.Height());
	int top = get_text_top(rc_text, text_h);   // 글자 셀과 동일 세로 기준 (valign 반영)
	top = max(top, (int)line.top);             // 폰트가 입력영역보다 큰 경우에만 안으로 clamp
	top = min(top, (int)line.bottom - h);

	Gdiplus::SolidBrush brush(m_theme.cr_back_selected);
	g.FillRectangle(&brush, pt_s.x, top, pt_e.x - pt_s.x, h);
}

void CSCStaticEdit::draw_text(Gdiplus::Graphics& g, const CRect& rc_text)
{
	// 표시 텍스트 (패스워드 마스킹 + IME 조합 중 문자 포함)
	CString display = get_display_text();
	if (m_composing)
		display.Insert(m_caret_pos, m_compose);

	if (display.IsEmpty()) return;

	Gdiplus::Color cr_text = IsWindowEnabled() ? m_theme.cr_text : m_theme.cr_disabled_text;

	// 텍스트는 GDI(TextOut)로 렌더링 ? 측정(GetTextExtent)과 픽셀 단위로 완벽 일치.
	// (GDI+ MeasureString/DrawString 조합은 측정-렌더링 간 서브픽셀 오차가 있어
	//  IME 블록 캐럿이 글자와 정확히 정렬되지 않는 문제가 있음)
	HDC hdc = g.GetHDC();
	{
		CDC dc;
		dc.Attach(hdc);

		// GDI clip 명시 ? Gdiplus::SetClip 이 GDI TextOut 에 안정적으로 전달되지 않는 케이스 (가로 스크롤 시
		// 텍스트가 prefix 이미지/액션버튼 영역까지 그려지는 현상) 방어.
		int saved_clip = dc.SaveDC();
		dc.IntersectClipRect(&rc_text);

		CFont* p_old_font = dc.SelectObject(&m_font);
		int   old_bk_mode = dc.SetBkMode(TRANSPARENT);
		COLORREF old_color = dc.SetTextColor(to_colorref(cr_text));

		TEXTMETRIC tm = {};
		dc.GetTextMetrics(&tm);

		int text_h = tm.tmHeight;
		int text_y = get_text_top(rc_text, text_h);

		// 가로 정렬 + 스크롤 반영한 시작 x
		CSize sz_total = dc.GetTextExtent(display);
		int text_x = get_text_start_x(sz_total.cx);

		// 일반 텍스트
		dc.TextOut(text_x, text_y, display);

		// 선택 영역 텍스트는 다른 색 (cr_text_selected = white) 으로 덮어 그리기.
		// 단 focus 가 없으면 draw_selection 이 navy bg 를 안 그리므로 (focus 잃은 후 selection 시각 hide)
		// selection 영역 텍스트도 일반 색으로 두어야 함. 안 그러면 white text 가 white bg 위에 그려져
		// 글자가 사라진 듯한 시각 깨짐 발생.
		if (has_selection() && GetFocus() == this)
		{
			int sel_s = min(m_sel_start, m_sel_end);
			int sel_e = max(m_sel_start, m_sel_end);
			CSize sz_before = dc.GetTextExtent(display.Left(sel_s));
			CSize sz_sel = dc.GetTextExtent(display.Mid(sel_s, sel_e - sel_s));

			CRect rc_clip(
				text_x + sz_before.cx,
				rc_text.top,
				text_x + sz_before.cx + sz_sel.cx,
				rc_text.bottom);

			dc.SaveDC();
			dc.IntersectClipRect(&rc_clip);
			dc.SetTextColor(to_colorref(m_theme.cr_text_selected));
			dc.TextOut(text_x, text_y, display);
			dc.RestoreDC(-1);
		}

		dc.SetTextColor(old_color);
		dc.SetBkMode(old_bk_mode);
		dc.SelectObject(p_old_font);
		dc.RestoreDC(saved_clip);
		dc.Detach();
	}
	g.ReleaseHDC(hdc);
}

void CSCStaticEdit::draw_dim_text(Gdiplus::Graphics& g, const CRect& rc_text)
{
	if (m_dim_text.IsEmpty()) return;

	//과거: Gdiplus::FontFamily(m_lf.lfFaceName) + g.DrawString 으로 그렸으나
	//GDI+ 의 FontFamily(name) 은 정확한 이름이 없으면 FontFamilyNotFound 상태가 되어
	//DrawString 이 조용히 no-op. XP 처럼 Segoe UI 미포함 환경에서 dim text 만 사라짐.
	//draw_text 와 동일하게 GDI(TextOut + HFONT) 경로로 통일 — 시스템 폰트 substitution
	//이 자동 적용되어 OS 무관하게 그려지고, 픽셀 정렬도 draw_text 와 일치.
	HDC hdc = g.GetHDC();
	{
		CDC dc;
		dc.Attach(hdc);

		int saved_clip = dc.SaveDC();
		dc.IntersectClipRect(&rc_text);

		CFont* p_old_font = dc.SelectObject(&m_font);
		int   old_bk_mode = dc.SetBkMode(TRANSPARENT);
		COLORREF old_color = dc.SetTextColor(to_colorref(m_theme.cr_text_dim));

		TEXTMETRIC tm = {};
		dc.GetTextMetrics(&tm);

		int text_y = rc_text.top + (rc_text.Height() - tm.tmHeight) / 2;

		//dim 텍스트는 실제 입력이 없으므로 m_scroll_offset 은 무시하고 가로 정렬만 적용.
		CSize sz = dc.GetTextExtent(m_dim_text);
		int text_x = get_text_start_x(sz.cx);

		dc.TextOut(text_x, text_y, m_dim_text);

		dc.SetTextColor(old_color);
		dc.SetBkMode(old_bk_mode);
		dc.SelectObject(p_old_font);
		dc.RestoreDC(saved_clip);
		dc.Detach();
	}
	g.ReleaseHDC(hdc);
}

// ──────────────────────────────────────────────────────────
// 헬퍼: 텍스트 그리기 영역
// ──────────────────────────────────────────────────────────
CRect CSCStaticEdit::get_text_area() const
{
	CRect rc;
	GetClientRect(rc);
	int inset = m_padding + (m_draw_border ? m_border_width : 0);
	rc.DeflateRect(inset, inset);

	// round rect 의 좌/우 코너 curve 가 텍스트 영역을 침범하므로 radius 에 비례해
	// 좌/우를 추가로 줄인다. 특히 트랙 모양(radius = height/2)일 때 글자가 curve 에
	// 붙어 보이는 현상을 방지. 위/아래는 curve 가 수직 중앙 기준으로 멀어지므로 영향 적음.
	if (m_border_radius > 0)
	{
		int extra = m_border_radius / 3;
		rc.left  += extra;
		rc.right -= extra;
	}

	// 사용자 정의 4면 margin 적용 (left/top/right/bottom 각 면 추가 inset)
	rc.DeflateRect(m_margin);

	// Prefix 이미지 사용 시 좌측 영역 확보 (이미지 width = size 비례 계산)
	if (has_prefix_image())
	{
		CRect img = get_prefix_image_area();
		if (!img.IsRectEmpty())
			rc.left = img.right + 5;
	}

	// Action 버튼 사용 시 우측 영역 확보 (버튼 좌측에서 5px 갭).
	// 버튼 = client.right - m_margin.right - 6 (right margin) - m_font_height (정사각). 텍스트 우측 = 버튼 좌측 - 5.
	if (has_action_button())
	{
		CRect client;
		GetClientRect(client);
		rc.right = min((LONG)rc.right, (LONG)(client.right - m_margin.right - 6 - m_font_height - 5));
	}

	return rc;
}

CRect CSCStaticEdit::get_line_area() const
{
	CRect rc;
	GetClientRect(rc);
	int border = m_draw_border ? m_border_width : 0;
	rc.DeflateRect(border, border);
	return rc;
}

int CSCStaticEdit::get_caret_width() const
{
	DWORD w = 1;
	::SystemParametersInfo(SPI_GETCARETWIDTH, 0, &w, 0);
	return max(1, (int)w);
}

// ──────────────────────────────────────────────────────────
// 헬퍼: 실제 그려진 텍스트의 bounding box (= 글자 픽셀 크기 + 정렬·스크롤 반영 위치)
//   - get_text_area() 와 다름: area 는 그려질 수 있는 공간, rect 는 실제 글자가 차지하는 영역.
//   - draw_text 의 text_x/text_y 계산과 동기 ? 거기 로직 바뀌면 같이 수정 필요.
//   - CClientDC / CFont* 는 비-const 시그니처라 const_cast 필요 (calc_caret_pixel_pos 와 동일 패턴).
// ──────────────────────────────────────────────────────────
CRect CSCStaticEdit::get_text_rect() const
{
	CRect rc_area = get_text_area();

	CClientDC dc(const_cast<CSCStaticEdit*>(this));
	CFont* p_old = dc.SelectObject(const_cast<CFont*>(&m_font));

	CString display = get_display_text();
	if (m_composing)
		display.Insert(m_caret_pos, m_compose);

	CSize      sz = dc.GetTextExtent(display);
	TEXTMETRIC tm = {};
	dc.GetTextMetrics(&tm);

	dc.SelectObject(p_old);

	int text_w = sz.cx;
	int text_h = tm.tmHeight;

	int text_y;
	if (m_valign & DT_VCENTER)
		text_y = rc_area.top + (rc_area.Height() - text_h) / 2;
	else if (m_valign & DT_BOTTOM)
		text_y = rc_area.bottom - text_h;
	else
		text_y = rc_area.top;

	int text_x = get_text_start_x(text_w);

	return CRect(text_x, text_y, text_x + text_w, text_y + text_h);
}

// ──────────────────────────────────────────────────────────
// 헬퍼: 가로 정렬 + 가로 스크롤을 반영한 텍스트 시작 x 좌표
//  - LEFT  : rc_text.left - m_scroll_offset (길어지면 스크롤)
//  - CENTER: 길이 ≤ rc 폭이면 가운데, 넘치면 LEFT 처럼 스크롤 폴백
//  - RIGHT : 항상 rc_text.right - text_width (넘쳐도 끝을 rc.right 에 맞추고 앞이 잘림 ?
//            일반 입력기에서 캐럿이 끝에 있을 때 보이는 자연스러운 right 동작)
// ──────────────────────────────────────────────────────────
int CSCStaticEdit::get_text_start_x(int text_width) const
{
	CRect rc_text = get_text_area();
	if (m_halign == DT_RIGHT)
		return rc_text.right - text_width;
	if (m_halign == DT_CENTER && text_width <= rc_text.Width())
		return rc_text.left + (rc_text.Width() - text_width) / 2;
	return rc_text.left - m_scroll_offset;
}

// ──────────────────────────────────────────────────────────
// 헬퍼: 문자 인덱스 → 픽셀 좌표
// ──────────────────────────────────────────────────────────
CPoint CSCStaticEdit::calc_caret_pixel_pos(int char_pos) const
{
	CClientDC dc(const_cast<CSCStaticEdit*>(this));
	CFont* p_old = dc.SelectObject(const_cast<CFont*>(&m_font));

	CString display = get_display_text();
	if (m_composing)
		display.Insert(m_caret_pos, m_compose);

	CSize sz_total = dc.GetTextExtent(display);
	CString left = display.Left(char_pos);
	CSize sz = dc.GetTextExtent(left);
	dc.SelectObject(p_old);

	// 가로 정렬 + 스크롤을 반영한 텍스트 시작 x 에 문자까지의 폭을 더한다.
	int x = get_text_start_x(sz_total.cx) + sz.cx;
	int y = get_text_area().top;
	return CPoint(x, y);
}

// ──────────────────────────────────────────────────────────
// 헬퍼: 픽셀 좌표 → 가장 가까운 문자 인덱스
// ──────────────────────────────────────────────────────────
int CSCStaticEdit::hit_test_char(CPoint pt) const
{
	CClientDC dc(const_cast<CSCStaticEdit*>(this));
	CFont* p_old = dc.SelectObject(const_cast<CFont*>(&m_font));

	CString display = get_display_text();
	// 텍스트 실제 시작 x (정렬 + 스크롤 반영). rel_x 는 "텍스트 시작점으로부터의 오프셋".
	CSize sz_total = dc.GetTextExtent(display);
	int text_start_x = get_text_start_x(sz_total.cx);
	int rel_x = pt.x - text_start_x;

	int len  = display.GetLength();
	int best = 0;

	if (len > 0)
	{
		// GetTextExtentExPoint 한 번의 호출로 각 문자까지의 누적 폭을 얻음(O(n)).
		// 기존 display.Left(i) 루프는 문자열 복사 + 측정을 매 반복 수행 → O(n²).
		CArray<INT> widths;
		widths.SetSize(len);
		INT n_fit = 0;
		CSize sz_total;
		::GetTextExtentExPoint(dc.GetSafeHdc(), display, len, 0x7FFFFFFF,
		                       &n_fit, widths.GetData(), &sz_total);

		int best_dist = abs(0 - rel_x);   // 위치 0 (맨 앞)
		for (int i = 0; i < len; i++)
		{
			int dist = abs(widths[i] - rel_x);
			if (dist < best_dist)
			{
				best_dist = dist;
				best = i + 1;
			}
			else if (widths[i] > rel_x)
			{
				break;  // 누적 폭은 단조증가 → 이후 거리는 더 커짐
			}
		}
	}

	dc.SelectObject(p_old);
	return best;
}

// ──────────────────────────────────────────────────────────
// 헬퍼: 조합 중 글자가 그려지는 픽셀 박스를 계산 (GDI 기준)
// draw_text()와 동일한 GetTextExtent/TextOut 기반으로 계산 → 픽셀 단위로 정확히 일치
// ──────────────────────────────────────────────────────────
CRect CSCStaticEdit::get_compose_draw_box() const
{
	if (!m_composing || m_compose.IsEmpty())
		return CRect();

	CRect rc_text = get_text_area();
	CClientDC dc(const_cast<CSCStaticEdit*>(this));
	CFont* p_old = dc.SelectObject(const_cast<CFont*>(&m_font));

	CString display = get_display_text();
	display.Insert(m_caret_pos, m_compose);

	CSize sz_total = dc.GetTextExtent(display);
	CSize sz_before = dc.GetTextExtent(display.Left(m_caret_pos));
	CSize sz_compose = dc.GetTextExtent(m_compose);

	TEXTMETRIC tm = {};
	dc.GetTextMetrics(&tm);

	dc.SelectObject(p_old);

	int text_h = tm.tmHeight;
	int text_y = get_text_top(rc_text, text_h);

	// IME 블록도 빔 커서/선택 블록과 동일하게 rc_text 안으로 clamp.
	// 가로 정렬 + 스크롤 반영한 텍스트 시작 x 에서 조합 이전 부분의 폭만큼 오른쪽으로.
	int x = get_text_start_x(sz_total.cx) + sz_before.cx;
	int h = min(text_h, rc_text.Height());
	int y = text_y + (text_h - h) / 2;   // clamp 시 중앙 정렬 유지
	int w = sz_compose.cx;
	if (w < 2) w = 2;
	return CRect(x, y, x + w, y + h);
}

// ──────────────────────────────────────────────────────────
// 헬퍼: 캐럿 위치 업데이트
//  - 일반 입력: 빔 캐럿 (폭 2px, 폰트 높이), m_caret_pos 위치
//  - IME 조합 중: 블록 캐럿, 조합 글자가 그려지는 박스에 정확히 겹침
// ──────────────────────────────────────────────────────────
void CSCStaticEdit::update_caret_pos()
{
	if (!m_caret_visible) return;

	int want_w, want_h, caret_x, caret_y;

	if (m_composing && !m_compose.IsEmpty())
	{
		CRect r_box = get_compose_draw_box();
		want_w = r_box.Width();
		want_h = r_box.Height();
		caret_x = r_box.left;
		caret_y = r_box.top;
	}
	else
	{
		CPoint pt = calc_caret_pixel_pos(m_caret_pos);
		CRect  rcT  = get_text_area();
		CRect  line = get_line_area();
		want_w = get_caret_width();
		// 폰트가 입력영역보다 큰 경우만 line(client - border) 안으로 clamp. rc_text 가 아닌 line 기준이라
		// 세로 padding 으로 캐럿이 짧아지지 않는다. m_caret_height 는 폰트 변경 시 stale 할 수 있어 재clamp.
		int h_base = m_caret_height > 0 ? m_caret_height : line.Height();
		want_h = min(h_base, (int)line.Height());
		// 빔 커서가 다음 글자의 첫 픽셀과 겹치는 현상 보정 (1px 왼쪽 이동)
		caret_x = pt.x - 1;
		// 선택 블록과 동일하게 글자 셀 세로 기준(get_text_top) + 입력영역 clamp → 캐럿·블록·글자 정렬 일치.
		caret_y = get_text_top(rcT, want_h);
		caret_y = max(caret_y, (int)line.top);
		caret_y = min(caret_y, (int)line.bottom - want_h);
	}

	// 크기가 바뀌었으면 캐럿 재생성
	if (want_w != m_caret_width || want_h != m_caret_cur_h)
	{
		DestroyCaret();
		CreateSolidCaret(want_w, want_h);
		ShowCaret();
		m_caret_width = want_w;
		m_caret_cur_h = want_h;
	}

	SetCaretPos(CPoint(caret_x, caret_y));
}

// ──────────────────────────────────────────────────────────
// 헬퍼: 캐럿이 보이도록 스크롤 조정
// ──────────────────────────────────────────────────────────
void CSCStaticEdit::ensure_caret_visible()
{
	CRect rc_text = get_text_area();
	int pos = m_caret_pos + (m_composing ? m_compose.GetLength() : 0);
	CPoint pt = calc_caret_pixel_pos(pos);

	// 스크롤 전 좌표로 계산 (calc_caret_pixel_pos가 이미 m_scroll_offset 반영)
	// 실제 픽셀 위치가 텍스트 영역 밖이면 스크롤
	if (pt.x < rc_text.left)
		m_scroll_offset -= (rc_text.left - pt.x) + 10;
	else if (pt.x > rc_text.right - 2)
		m_scroll_offset += (pt.x - (rc_text.right - 2)) + 10;

	if (m_scroll_offset < 0) m_scroll_offset = 0;
}

// ──────────────────────────────────────────────────────────
// 헬퍼: 표시용 텍스트 (패스워드 마스킹)
// ──────────────────────────────────────────────────────────
CString CSCStaticEdit::get_display_text() const
{
	if (!m_password) return m_text;
	CString str;
	for (int i = 0; i < m_text.GetLength(); i++)
		str += m_mask_char;
	return str;
}

// ──────────────────────────────────────────────────────────
// 텍스트 삽입 / 선택 삭제
// ──────────────────────────────────────────────────────────
void CSCStaticEdit::insert_text(const CString& str)
{
	if (has_selection())
		delete_selection();

	if (m_limit_text > 0 &&
		m_text.GetLength() + str.GetLength() > m_limit_text)
	{
		// 최대 길이 초과 방지
		int avail = m_limit_text - m_text.GetLength();
		if (avail <= 0) return;
		m_text.Insert(m_caret_pos, str.Left(avail));
		m_caret_pos += avail;
	}
	else
	{
		m_text.Insert(m_caret_pos, str);
		m_caret_pos += str.GetLength();
	}

	m_sel_start = m_sel_end = m_caret_pos;
	notify_parent(message_scstaticedit_text_changed);
}

void CSCStaticEdit::delete_selection()
{
	if (!has_selection()) return;
	int start = min(m_sel_start, m_sel_end);
	int end = max(m_sel_start, m_sel_end);
	m_text.Delete(start, end - start);
	m_caret_pos = start;
	m_sel_start = m_sel_end = m_caret_pos;
	notify_parent(message_scstaticedit_text_changed);
}

// ──────────────────────────────────────────────────────────
// 클립보드
// ──────────────────────────────────────────────────────────
void CSCStaticEdit::do_copy()
{
	copy_to_clipboard(m_hWnd, get_sel_text());
}

void CSCStaticEdit::do_cut()
{
	if (m_readonly) return;
	do_copy();
	push_undo();
	delete_selection();
	ensure_caret_visible();
	update_caret_pos();
	Invalidate(FALSE);
}

void CSCStaticEdit::do_paste()
{
	if (m_readonly) return;
	if (!::OpenClipboard(m_hWnd)) return;

	CString paste;
#ifdef UNICODE
	HGLOBAL h_mem = ::GetClipboardData(CF_UNICODETEXT);
	if (h_mem)
	{
		LPCTSTR p = (LPCTSTR)::GlobalLock(h_mem);
		if (p) paste = p;
		::GlobalUnlock(h_mem);
	}
#else
	HGLOBAL h_mem = ::GetClipboardData(CF_TEXT);
	if (h_mem)
	{
		LPCSTR p = (LPCSTR)::GlobalLock(h_mem);
		if (p) paste = p;
		::GlobalUnlock(h_mem);
	}
#endif
	::CloseClipboard();

	if (!paste.IsEmpty())
	{
		push_undo();
		// 줄바꿈 제거 (싱글라인)
		paste.Remove(_T('\r'));
		paste.Remove(_T('\n'));
		insert_text(paste);
		ensure_caret_visible();
		update_caret_pos();
		Invalidate(FALSE);
	}
}

// ──────────────────────────────────────────────────────────
// Action 버튼 (우측 아이콘)
//   - 별도 CButton 없이 OnPaint 에서 직접 그리고, OnLButtonDown/Up/MouseMove 에서
//     마우스 캡처를 받아 버튼처럼 동작.
//   - get_text_area() 가 버튼 영역만큼 우측을 줄여 텍스트와 캐럿이 자연스럽게 잘림.
//   - action 종류별로 아이콘과 클릭 동작이 분기 (draw_action_button / perform_action_button_click).
// ──────────────────────────────────────────────────────────
void CSCStaticEdit::set_action_button(action_button_type action)
{
	if (m_action_type == action)
		return;
	m_action_type = action;

	// password_toggle 액션을 켜면 password 모드도 함께 ON (편의).
	// 끌 때는 사용자 의도가 모호하므로 password 상태 그대로 둠.
	if (action == action_password_toggle)
		m_password = true;

	if (m_hWnd)
	{
		update_action_tooltip();
		ensure_caret_visible();
		update_caret_pos();
		Invalidate();
	}
}

void CSCStaticEdit::set_action_button_tooltip(LPCTSTR text)
{
	m_action_tooltip_text = text;
	if (m_hWnd)
		update_action_tooltip();
}

CString CSCStaticEdit::action_tooltip_default_text() const
{
	switch (m_action_type)
	{
		case action_copy:				return _T("복사");
		case action_clear:				return _T("지우기");
		case action_find:				return _T("찾기");
		case action_password_toggle:	return _T("표시/감추기");
		case action_file:				return _T("파일 선택");
		case action_folder:				return _T("폴더 선택");
		default:						return _T("");
	}
}

void CSCStaticEdit::update_action_tooltip()
{
	ensure_action_tooltip();
	if (!m_action_tooltip.GetSafeHwnd())
		return;

	if (!has_action_button())
	{
		m_action_tooltip.Activate(FALSE);
		return;
	}

	CString text = m_action_tooltip_text.IsEmpty() ? action_tooltip_default_text() : m_action_tooltip_text;
	m_action_tooltip.UpdateTipText(text, this, 1);
	m_action_tooltip.Activate(TRUE);
}

void CSCStaticEdit::draw_action_button(Gdiplus::Graphics& g)
{
	if (!has_action_button())
		return;

	CRect rc;
	GetClientRect(rc);

	// 버튼 사이즈 = 텍스트 영역 높이 기준 정사각, 우측 (6 + m_margin.right) 안쪽에 배치.
	m_r_action_button = get_text_rect();
	m_r_action_button.right = rc.right - 6 - m_margin.right;
	m_r_action_button.left  = m_r_action_button.right - m_r_action_button.Height();
	m_r_action_button.InflateRect(0, 0, 1, 1);

	ensure_action_tooltip();
	if (m_action_tooltip.GetSafeHwnd())
		m_action_tooltip.SetToolRect(this, 1, m_r_action_button);

	if (m_r_action_button.IsRectEmpty())
		return;

	// 눌림 시 1px 우하 offset 을 Graphics translate 한 번으로 적용 (모든 헬퍼 공통).
	if (m_action_button_pressed)
		g.TranslateTransform(1.0f, 1.0f);

	switch (m_action_type)
	{
		case action_copy:
			draw_action_icon_copy(g);
			break;
		case action_clear:
			draw_action_icon_clear(g);
			break;
		case action_find:
			draw_action_icon_find(g);
			break;
		case action_password_toggle:
			draw_action_icon_password_toggle(g);
			break;
		case action_file:
			draw_action_icon_file(g);
			break;
		case action_folder:
			draw_action_icon_folder(g);
			break;
		default:
			break;
	}

	if (m_action_button_pressed)
		g.TranslateTransform(-1.0f, -1.0f);
}

void CSCStaticEdit::draw_action_icon_copy(Gdiplus::Graphics& g)
{
	CRect rc_btn_each = m_r_action_button;
	rc_btn_each.DeflateRect(3, 2);

	// 두 라운드 사각형이 좌하 / 우상으로 어긋나게 그려져 클립보드 아이콘 형태가 됨.
	rc_btn_each.OffsetRect(-3, +2);
	draw_round_rect(&g, CRect_to_gpRect(rc_btn_each),
		m_action_button_pressed ? Gdiplus::Color::RoyalBlue : Gdiplus::Color::DimGray,
		m_action_button_pressed ? Gdiplus::Color::Moccasin  : m_theme.cr_back, 2, 1);

	rc_btn_each.OffsetRect(+4, -3);
	draw_round_rect(&g, CRect_to_gpRect(rc_btn_each),
		m_action_button_pressed ? Gdiplus::Color::DimGray  : Gdiplus::Color::RoyalBlue,
		m_action_button_pressed ? m_theme.cr_back : Gdiplus::Color::Moccasin, 2, 1);
}

void CSCStaticEdit::draw_action_icon_clear(Gdiplus::Graphics& g)
{
	// 원형 배경 + 흰 X ? close/clear 의 보편 패턴.
	Gdiplus::Color cr_stroke = m_theme.cr_text;

	g.SetSmoothingMode(Gdiplus::SmoothingModeAntiAlias);

	int inset = max(3, m_r_action_button.Width() / 4);
	int x1 = m_r_action_button.left   + inset;
	int y1 = m_r_action_button.top    + inset;
	int x2 = m_r_action_button.right  - inset - 1;
	int y2 = m_r_action_button.bottom - inset - 1;

	Gdiplus::Pen pen(cr_stroke, 1.5f);
	pen.SetStartCap(Gdiplus::LineCapRound);
	pen.SetEndCap  (Gdiplus::LineCapRound);
	g.DrawLine(&pen, x1, y1, x2, y2);
	g.DrawLine(&pen, x2, y1, x1, y2);
}

void CSCStaticEdit::draw_action_icon_find(Gdiplus::Graphics& g)
{
	// 돋보기: 좌상단 원 + 우하단 손잡이.
	Gdiplus::Color cr_stroke = m_action_button_pressed ? Gdiplus::Color::RoyalBlue : Gdiplus::Color::DimGray;

	g.SetSmoothingMode(Gdiplus::SmoothingModeAntiAlias);

	int side    = min(m_r_action_button.Width(), m_r_action_button.Height());
	int circle  = side * 2 / 3;
	int cx      = m_r_action_button.left + circle / 2 + 1;
	int cy      = m_r_action_button.top  + circle / 2 + 1;
	int radius  = circle / 2;

	Gdiplus::Pen pen(cr_stroke, 1.5f);
	pen.SetStartCap(Gdiplus::LineCapRound);
	pen.SetEndCap  (Gdiplus::LineCapRound);

	g.DrawEllipse(&pen, cx - radius, cy - radius, radius * 2, radius * 2);

	// 손잡이: 원의 우하단 45° 접점 → 버튼 우하단 직전.
	double k     = 0.7071; // sin(45) = cos(45)
	int    hx1   = cx + (int)(radius * k);
	int    hy1   = cy + (int)(radius * k);
	int    hx2   = m_r_action_button.right  - 2;
	int    hy2   = m_r_action_button.bottom - 2;
	g.DrawLine(&pen, hx1, hy1, hx2, hy2);
}

void CSCStaticEdit::draw_action_icon_password_toggle(Gdiplus::Graphics& g)
{
	// 아몬드(렌즈) 형 눈 외곽: 상/하 cubic bezier 두 개로 양 끝이 뾰족한 아이라인을 그린다.
	// 가운데 원형 동공. password 모드 ON 이면 우상→좌하 슬래시 덧그림.
	Gdiplus::Color cr_stroke = get_weak_color(m_theme.cr_back, 48);// m_theme.cr_text_dim;// m_action_button_pressed ? Gdiplus::Color::RoyalBlue : Gdiplus::Color::DimGray;

	g.SetSmoothingMode(Gdiplus::SmoothingModeAntiAlias);

	int eye_w = m_r_action_button.Width()  - 2;
	int eye_h = max(8, (m_r_action_button.Height() * 4) / 5);
	int eye_x = m_r_action_button.left + 1;
	int eye_y = m_r_action_button.top  + (m_r_action_button.Height() - eye_h) / 2;

	float left   = (float)eye_x;
	float right  = (float)(eye_x + eye_w - 1);
	float top    = (float)eye_y;
	float bottom = (float)(eye_y + eye_h - 1);
	float mid_y  = (top + bottom) * 0.5f;
	float bow    = eye_w * 0.25f;

	Gdiplus::Pen pen(cr_stroke, 1.8f);
	pen.SetStartCap(Gdiplus::LineCapRound);
	pen.SetEndCap  (Gdiplus::LineCapRound);

	// 위쪽 곡선
	g.DrawBezier(&pen,
		Gdiplus::PointF(left,        mid_y),
		Gdiplus::PointF(left + bow,  top),
		Gdiplus::PointF(right - bow, top),
		Gdiplus::PointF(right,       mid_y));
	// 아래쪽 곡선
	g.DrawBezier(&pen,
		Gdiplus::PointF(left,        mid_y),
		Gdiplus::PointF(left + bow,  bottom),
		Gdiplus::PointF(right - bow, bottom),
		Gdiplus::PointF(right,       mid_y));

	// 동공: 눈 높이의 절반 크기 원, 중앙 정렬
	// 동공 작게 + 외곽선만(ring) 으로 그려 실제 눈처럼 보이는 무서움 완화
	float pupil = (float)max(3, eye_h / 3);
	Gdiplus::Pen pen_pupil(cr_stroke, 1.8f);
	g.DrawEllipse(&pen_pupil,
		(left + right) * 0.5f - pupil * 0.5f,
		mid_y - pupil * 0.5f,
		pupil, pupil);

	if (m_password)
	{
		Gdiplus::Pen pen_slash(cr_stroke, 1.8f);
		pen_slash.SetStartCap(Gdiplus::LineCapRound);
		pen_slash.SetEndCap  (Gdiplus::LineCapRound);
		g.DrawLine(&pen_slash,
			m_r_action_button.right - 2, m_r_action_button.top    + 2,
			m_r_action_button.left  + 2, m_r_action_button.bottom - 2);
	}
}

void CSCStaticEdit::draw_action_icon_file(Gdiplus::Graphics& g)
{
	// 문서: 우상단 모서리가 접힌(dog-ear) 종이 + 내용 라인 2 개.
	Gdiplus::Color cr_stroke = m_action_button_pressed ? Gdiplus::Color::RoyalBlue : Gdiplus::Color::DimGray;

	g.SetSmoothingMode(Gdiplus::SmoothingModeAntiAlias);

	int side   = min(m_r_action_button.Width(), m_r_action_button.Height());
	int mx     = side / 5;
	int my     = max(2, side / 6);
	int left   = m_r_action_button.left   + mx;
	int right  = m_r_action_button.right  - mx - 1;
	int top    = m_r_action_button.top    + my;
	int bottom = m_r_action_button.bottom - my - 1;
	int fold   = max(3, (right - left) * 2 / 5);

	Gdiplus::Pen pen(cr_stroke, 1.3f);
	pen.SetLineJoin(Gdiplus::LineJoinRound);

	// 외곽(우상단이 fold 만큼 잘린 5각형). AddPolygon 이 마지막→첫 점(좌변)을 자동으로 닫는다.
	Gdiplus::Point pts[] =
	{
		Gdiplus::Point(left,         top),
		Gdiplus::Point(right - fold, top),
		Gdiplus::Point(right,        top + fold),
		Gdiplus::Point(right,        bottom),
		Gdiplus::Point(left,         bottom),
	};
	Gdiplus::GraphicsPath path;
	path.AddPolygon(pts, 5);
	g.DrawPath(&pen, &path);

	// 접힌 모서리 삼각형(대각변은 외곽이 이미 그림).
	g.DrawLine(&pen, right - fold, top,        right - fold, top + fold);
	g.DrawLine(&pen, right - fold, top + fold, right,        top + fold);

	// 내용 라인.
	int lx1 = left  + max(2, (right - left) / 6);
	int lx2 = right - max(2, (right - left) / 6);
	int ly1 = top + (bottom - top) * 1 / 2;
	int ly2 = top + (bottom - top) * 7 / 10;
	Gdiplus::Pen pen_line(cr_stroke, 1.0f);
	g.DrawLine(&pen_line, lx1, ly1, lx2, ly1);
	g.DrawLine(&pen_line, lx1, ly2, lx2, ly2);
}

void CSCStaticEdit::draw_action_icon_folder(Gdiplus::Graphics& g)
{
	// 폴더: 좌측 상단에 탭(tab) 이 솟은 전형적 폴더 실루엣.
	Gdiplus::Color cr_stroke = m_action_button_pressed ? Gdiplus::Color::RoyalBlue : Gdiplus::Color::DimGray;

	g.SetSmoothingMode(Gdiplus::SmoothingModeAntiAlias);

	int side     = min(m_r_action_button.Width(), m_r_action_button.Height());
	int mx       = side / 6;
	int left     = m_r_action_button.left   + mx;
	int right    = m_r_action_button.right  - mx - 1;
	int bottom   = m_r_action_button.bottom - max(2, side / 5) - 1;
	int body_top = m_r_action_button.top + side / 3;
	int tab_top  = m_r_action_button.top + side / 5;
	int tab_w    = (right - left) * 9 / 20;
	int slope    = max(2, side / 8);

	Gdiplus::Pen pen(cr_stroke, 1.3f);
	pen.SetLineJoin(Gdiplus::LineJoinRound);

	// 탭 상단 → 사선 → 몸통 상단 → 우변 → 하단 → (좌변 자동 닫힘).
	Gdiplus::Point pts[] =
	{
		Gdiplus::Point(left,                 tab_top),
		Gdiplus::Point(left + tab_w,         tab_top),
		Gdiplus::Point(left + tab_w + slope, body_top),
		Gdiplus::Point(right,                body_top),
		Gdiplus::Point(right,                bottom),
		Gdiplus::Point(left,                 bottom),
	};
	Gdiplus::GraphicsPath path;
	path.AddPolygon(pts, 6);
	g.DrawPath(&pen, &path);
}

void CSCStaticEdit::perform_action_button_click()
{
	switch (m_action_type)
	{
	case action_copy:
		copy_to_clipboard(m_hWnd, m_text);
		break;

	case action_clear:
		if (!m_text.IsEmpty())
		{
			push_undo();
			m_text.Empty();
			m_caret_pos = m_sel_start = m_sel_end = 0;
			m_scroll_offset = 0;
			ensure_caret_visible();
			update_caret_pos();
			Invalidate(FALSE);
			notify_parent(message_scstaticedit_text_changed);
		}
		break;

	case action_password_toggle:
		m_password = !m_password;
		//rebuild_font 가 m_password 보고 lfQuality 를 결정 — 토글 즉시 반영.
		rebuild_font();
		ensure_caret_visible();
		update_caret_pos();
		Invalidate(FALSE);
		break;

	case action_file:
	{
		CString start = action_button_start_folder();
		CFileDialog dlg(TRUE, NULL, m_text,
			OFN_HIDEREADONLY | OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST,
			_T("All Files (*.*)|*.*||"), this);
		if (!start.IsEmpty())
			dlg.m_ofn.lpstrInitialDir = start;
		if (dlg.DoModal() == IDOK)
		{
			CString path = dlg.GetPathName();
			push_undo();
			set_text(path);
			AfxGetApp()->WriteProfileString(_T("settings"), _T("recent_folder"), get_part(path, fn_folder));
			notify_parent(message_scstaticedit_text_changed);
		}
		break;
	}

	case action_folder:
	{
		CString start = action_button_start_folder();
		CString folder;
		if (BrowseForFolder(m_hWnd, (TCHAR*)_T("폴더 선택"), folder, start.IsEmpty() ? NULL : (LPCTSTR)start))
		{
			push_undo();
			set_text(folder);
			AfxGetApp()->WriteProfileString(_T("settings"), _T("recent_folder"), folder);
			notify_parent(message_scstaticedit_text_changed);
		}
		break;
	}

	case action_find:
	default:
		// parent 가 처리해야 하는 action ? 알림만 발신.
		notify_parent(message_scstaticedit_action_button);
		break;
	}
}

CString CSCStaticEdit::action_button_start_folder() const
{
	// 1) 현재 텍스트가 가리키는 폴더가 실제로 존재하면 거기서 시작.
	if (!m_text.IsEmpty())
	{
		CString folder = get_part(m_text, fn_folder);
		if (!folder.IsEmpty())
		{
			DWORD attr = GetFileAttributes(folder);
			if (attr != INVALID_FILE_ATTRIBUTES && (attr & FILE_ATTRIBUTE_DIRECTORY))
				return folder;
		}
	}

	// 2) 직전 사용 폴더(§7 recent_folder).
	CString recent = AfxGetApp()->GetProfileString(_T("settings"), _T("recent_folder"), _T(""));
	if (!recent.IsEmpty())
		return recent;

	// 3) exe 디렉토리.
	return get_exe_directory();
}

// ──────────────────────────────────────────────────────────
// Prefix 이미지 (좌측 아이콘)
//   - PNG 리소스 ID 를 받아 텍스트 좌측에 그린다.
//   - 영역: 좌측 6px 마진 + 텍스트 영역 높이 정사각, 그 후 5px 갭으로 텍스트 시작.
//   - 그리기: 종횡비 유지하며 영역에 맞춰 축소·중앙 정렬 (영역보다 큰 이미지 → 축소, 작으면 그대로).
// ──────────────────────────────────────────────────────────
bool CSCStaticEdit::set_prefix_image(UINT id, int size)
{
	bool ok = m_prefix_image.load(id);
	m_prefix_image_size = size;
	if (m_hWnd)
	{
		ensure_caret_visible();
		update_caret_pos();
		Invalidate();
	}
	return ok;
}

void CSCStaticEdit::clear_prefix_image()
{
	m_prefix_image.release();
	if (m_hWnd)
	{
		ensure_caret_visible();
		update_caret_pos();
		Invalidate();
	}
}

CRect CSCStaticEdit::get_prefix_image_area() const
{
	if (!has_prefix_image())
		return CRect();

	Gdiplus::Bitmap* p_bmp = m_prefix_image.m_pBitmap;
	int img_w = (int)p_bmp->GetWidth();
	int img_h = (int)p_bmp->GetHeight();
	if (img_w <= 0 || img_h <= 0)
		return CRect();

	CRect rc;
	GetClientRect(rc);
	int inset = m_padding + (m_draw_border ? m_border_width : 0);
	rc.DeflateRect(inset, inset);

	if (m_border_radius > 0)
	{
		int extra = m_border_radius / 3;
		rc.left += extra;
	}

	// 사용자 정의 margin 적용 (좌측·상하 영향).
	rc.DeflateRect(m_margin);

	int draw_h = m_prefix_image_size;
	int draw_w = MulDiv(draw_h, img_w, img_h);

	CRect img_area;
	img_area.left   = rc.left + 2;
	img_area.right  = img_area.left + draw_w;
	img_area.top    = rc.top + (rc.Height() - draw_h) / 2;
	img_area.bottom = img_area.top + draw_h;
	return img_area;
}

void CSCStaticEdit::draw_prefix_image(Gdiplus::Graphics& g)
{
	if (!has_prefix_image())
		return;

	CRect area = get_prefix_image_area();
	if (area.IsRectEmpty())
		return;

	m_prefix_image.draw(g, area, CSCGdiplusBitmap::draw_mode_zoom);
}

// ──────────────────────────────────────────────────────────
// Undo / Redo
// ──────────────────────────────────────────────────────────
void CSCStaticEdit::push_undo()
{
	// 편집 직전 상태를 저장. 새 편집은 redo 체인을 무효화.
	undo_state state;
	state.text      = m_text;
	state.caret_pos = m_caret_pos;
	m_undo_stack.Add(state);
	m_redo_stack.RemoveAll();
}

void CSCStaticEdit::do_undo()
{
	if (m_undo_stack.GetSize() <= 0) return;

	// 현재 상태는 redo 스택으로 이동
	undo_state cur;
	cur.text      = m_text;
	cur.caret_pos = m_caret_pos;
	m_redo_stack.Add(cur);

	// undo 스택 top 상태를 복원
	int top = (int)m_undo_stack.GetSize() - 1;
	undo_state state = m_undo_stack[top];
	m_undo_stack.RemoveAt(top);

	m_text      = state.text;
	m_caret_pos = state.caret_pos;
	clear_selection();
	ensure_caret_visible();
	update_caret_pos();
	Invalidate(FALSE);
	notify_parent(message_scstaticedit_text_changed);
}

void CSCStaticEdit::do_redo()
{
	if (m_redo_stack.GetSize() <= 0) return;

	// 현재 상태는 undo 스택으로 이동
	undo_state cur;
	cur.text      = m_text;
	cur.caret_pos = m_caret_pos;
	m_undo_stack.Add(cur);

	// redo 스택 top 상태를 복원
	int top = (int)m_redo_stack.GetSize() - 1;
	undo_state state = m_redo_stack[top];
	m_redo_stack.RemoveAt(top);

	m_text      = state.text;
	m_caret_pos = state.caret_pos;
	clear_selection();
	ensure_caret_visible();
	update_caret_pos();
	Invalidate(FALSE);
	notify_parent(message_scstaticedit_text_changed);
}

// ──────────────────────────────────────────────────────────
// 부모에게 알림
// ──────────────────────────────────────────────────────────
void CSCStaticEdit::notify_parent(int message)
{
	CWnd* p_parent = GetParent();
	if (!p_parent || !p_parent->m_hWnd) return;

	CSCStaticEditMessage msg(this, message);
	::SendMessage(p_parent->m_hWnd, Message_CSCStaticEdit, (WPARAM)&msg, 0);
}
