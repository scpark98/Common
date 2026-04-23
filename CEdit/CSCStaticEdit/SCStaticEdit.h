#pragma once

/*
 * CSCStaticEdit
 * CStatic을 상속받아 CEdit처럼 동작하는 완전 커스터마이즈 가능한 Edit 컨트롤.
 *
 * https://github.com/scpark98/Test_CSCStaticEdit.git
 * 
 * [특징]
 * - OnPaint()를 완전히 직접 그리므로 round rect, 그라디언트, 그림자 등 자유롭게 구현 가능
 * - CEdit 기반의 Default() 덮어쓰기 문제 없음
 * - Resource Editor에서 Static Text 컨트롤로 배치 후 DDX_Control로 연결하여 사용
 * - 캐럿, 선택 영역, 클립보드(Ctrl+C/V/X/A), Backspace/Delete, Home/End 구현
 * - 한글 IME 지원 (WM_IME_COMPOSITION)
 * - Dim text (placeholder) 지원
 * - 포커스 활성/비활성 border 색상 변경
 * - Round rect border 지원
 * - Password 모드 지원
 *
 * [사용법]
 * 1. Resource Editor에서 Static Text 컨트롤 배치, ID 부여 (SS_NOTIFY 스타일 추가 필요 없음 - 코드에서 처리)
 * 2. 다이얼로그 헤더에 CSCStaticEdit m_edit; 선언
 * 3. DoDataExchange에서 DDX_Control(pDX, IDC_STATIC1, m_edit); 연결
 *
 * [주의]
 * - ES_MULTILINE은 현재 미지원 (싱글라인만 지원)
 * - IME 입력 시 조합 중인 문자는 별도 처리됨
 *
 * [네이밍]
 * - 사용자가 추가한 모든 식별자는 GNU 컨벤션 (snake_case)
 * - 기반 클래스 override (Create, SetFont, SetWindowText, PreSubclassWindow, PreTranslateMessage 등) 및
 *   ON_WM_* 매크로로 wiring되는 표준 메시지 핸들러 (OnPaint, OnChar, OnKeyDown 등)는 MFC 컨벤션 유지
 * - ON_MESSAGE 로 직접 wiring하는 커스텀 핸들러는 GNU 컨벤션 (on_ime_composition 등)
 */

#include <afxwin.h>
#include <gdiplus.h>
#pragma comment(lib, "gdiplus.lib")

#include "Common/colors.h"
#include "Common/Functions.h"

// 부모에게 전달하는 커스텀 메시지 (CSCEdit 과 동일한 스타일)
static const UINT Message_CSCStaticEdit = ::RegisterWindowMessage(_T("MessageString_CSCStaticEdit"));

class CSCStaticEditMessage
{
public:
    CSCStaticEditMessage(CWnd* _this, int _message)
    {
        pThis   = _this;
        message = _message;
    }

    CWnd*	pThis = nullptr;
    int		message;
};

class CSCStaticEdit : public CStatic
{
    DECLARE_MESSAGE_MAP()

public:
    CSCStaticEdit();
    virtual ~CSCStaticEdit();

    enum MESSAGES
    {
        message_scstaticedit_killfocus = 0,
        message_scstaticedit_setfocus,
        message_scstaticedit_enter,
        message_scstaticedit_escape,
        message_scstaticedit_text_changed,
    };

    CSCColorTheme	m_theme = CSCColorTheme(this);
    void			set_color_theme(int color_theme, bool invalidate = false); //apply current m_theme colors to the control.


    // ──────────────────────────────────────────────
    // 텍스트
    // ──────────────────────────────────────────────
    void			SetWindowText(LPCTSTR lpsz_string);
    void			GetWindowText(CString& r_string) const;
    CString			get_text() const { return m_text; }
	int             get_int() const { return _ttoi(m_text); }
	float           get_float() const { return (float)_ttof(m_text); }
    void			set_text(const CString& text);
    void			set_text(int n)    { CString s; s.Format(_T("%d"), n); set_text(s); }
    void			set_text(double d) { CString s; s.Format(_T("%g"), d); set_text(s); }
    void			set_textf(LPCTSTR format, ...);
    int				get_text_length() const { return m_text.GetLength(); }

    // ──────────────────────────────────────────────
    // 색상 (모두 m_theme으로 위임. 개별 색상 저장 변수 없음)
    // ──────────────────────────────────────────────
    void			set_text_color(Gdiplus::Color cr)         { m_theme.cr_text = cr; Invalidate(); }
    void			set_back_color(Gdiplus::Color cr)         { m_theme.cr_back = cr; Invalidate(); }
    void			set_selection_color(Gdiplus::Color cr_back, Gdiplus::Color cr_text);
    // round > 0 일 때 모서리 밖을 채울 색. 명시 호출 안 하면 COLOR_3DFACE 로 fallback.
    void			set_parent_back_color(Gdiplus::Color cr_parent_back)
                        { m_theme.cr_parent_back = cr_parent_back; m_has_parent_back_color = true; Invalidate(); }

    Gdiplus::Color	get_text_color() const { return m_theme.cr_text; }
    Gdiplus::Color	get_back_color() const { return m_theme.cr_back; }

    // ──────────────────────────────────────────────
    // 테두리
    // ──────────────────────────────────────────────
    void			set_border_width(int width)               { m_border_width = width; Invalidate(); }
    // border 색은 active(focus) / inactive(unfocus) 두 상태로 저장한다.
    //   - set_border_color(cr)                     : 두 상태 동일하게 세팅 (가장 흔한 케이스)
    //   - set_border_color(cr_inactive, cr_active) : 두 상태를 다른 색으로 명시 세팅
    //   - set_border_color_inactive / _active      : 한쪽만 변경
    void			set_border_color(Gdiplus::Color cr)
                        { m_theme.cr_border_inactive = cr; m_theme.cr_border_active = cr; Invalidate(); }
    void			set_border_color(Gdiplus::Color cr_inactive, Gdiplus::Color cr_active)
                        { m_theme.cr_border_inactive = cr_inactive; m_theme.cr_border_active = cr_active; Invalidate(); }
    void			set_border_color_inactive(Gdiplus::Color cr){ m_theme.cr_border_inactive = cr; Invalidate(); }
    void			set_border_color_active  (Gdiplus::Color cr){ m_theme.cr_border_active   = cr; Invalidate(); }

    Gdiplus::Color	get_border_color_inactive() const { return m_theme.cr_border_inactive; }
    Gdiplus::Color	get_border_color_active  () const { return m_theme.cr_border_active;   }
    void			set_round(int radius = -1);
    void			set_draw_border(bool draw)                { m_draw_border = draw; Invalidate(); }

    // ──────────────────────────────────────────────
    // 폰트
    // ──────────────────────────────────────────────
    void			SetFont(CFont* p_font, BOOL b_redraw = TRUE);
    void			set_font_name(LPCTSTR name);
    void			set_font_size(int size);
    void			set_font_bold(bool bold = true);
    void			set_font_antialias(bool antialias = true);
    // ──────────────────────────────────────────────
    // 기능 옵션
    // ──────────────────────────────────────────────
    void		    set_readonly(bool readonly = true);
    bool		    is_readonly() const { return m_readonly; }
    //readonly일 때 원래 기본색인 gray로 표시할 것인지, 특정색을 사용할 지, transparent라면 m_cr_back을 사용하게 된다.
    void			set_use_default_readonly_color(bool use_default_readonly_color = true, Gdiplus::Color cr_back_readonly = Gdiplus::Color::Transparent);


    void		    set_password_mode(bool password = true, TCHAR mask_char = _T('*'));
    void		    set_max_length(int max)              { m_max_length = max; }

    // shift + up/down 방향키로 수치 증감. shift + mousewheel로도 동일.
    // 텍스트가 실수로 파싱되지 않으면 스킵.
    // interval 은 실수 허용 (예: 0.001). 표시 자리수 = max(interval 소수자리수, 현재 텍스트 소수자리수).
    // 예: interval=0.01, "0.09" → up → "0.10" (trailing 0 보존).
    void		    set_use_updown_key(bool use_updown_key = true, float interval = 1.0f)
                        { m_use_updown_key = use_updown_key; m_updown_interval = interval; }
    void		    set_dim_text(const CString& dim_text);
    void		    set_padding(int padding)             { m_padding = padding; Invalidate(); }

    // 우측에 카피 버튼 표시. true 면 텍스트 영역이 우측 32px 줄어들고 그 자리에 클립보드
    // 아이콘(라운드 사각형 2개 교차) 을 그린다. 클릭 시 현재 전체 텍스트를 클립보드로 복사.
    // 별도 CButton 을 만들지 않고 OnPaint/마우스 핸들러에서 직접 처리.
    void		    set_use_copy_button(bool use = true);
    bool		    is_use_copy_button() const { return m_use_copy_button; }

    // 세로 정렬 (line align): DT_TOP / DT_VCENTER (기본) / DT_BOTTOM
    void		    set_line_align(DWORD align = DT_VCENTER) { m_valign = align; Invalidate(); }
    DWORD		    get_line_align() const { return m_valign; }
    // 가로 정렬 (text align): DT_LEFT (기본) / DT_CENTER / DT_RIGHT
    //  - LEFT  : 길어지면 m_scroll_offset 으로 가로 스크롤 (시작점이 rc.left 보다 왼쪽으로).
    //  - CENTER: 길이가 rc 폭 이내면 가운데, 초과 시 LEFT 처럼 스크롤 폴백.
    //  - RIGHT : 항상 끝을 rc.right 에 정렬. 길어지면 시작이 rc.left 보다 왼쪽으로 가서 앞부분이
    //            잘림 (캐럿이 끝에 있을 때의 자연스러운 right 동작).
    //  - 정렬 변경 시 누적 스크롤은 0 으로 리셋.
    void		    set_text_align(DWORD align = DT_LEFT)
                        { m_halign = align; m_scroll_offset = 0; if (m_hWnd) { update_caret_pos(); Invalidate(); } }
    DWORD		    get_text_align() const { return m_halign; }

    // 선택
    void		    set_sel(int start, int end);
    void		    get_sel(int& start, int& end) const  { start = m_sel_start; end = m_sel_end; }
    void		    select_all();
    CString		    get_sel_text() const;

    // 동적 생성용
    bool		    Create(DWORD dw_style, const RECT& rect, CWnd* parent, UINT id);

protected:
    virtual void	PreSubclassWindow() override;
    virtual BOOL	PreTranslateMessage(MSG* p_msg) override;

    // 메시지 핸들러
    afx_msg void	OnPaint();
    afx_msg void	OnSetFocus(CWnd* p_old_wnd);
    afx_msg void	OnKillFocus(CWnd* p_new_wnd);
    afx_msg UINT	OnGetDlgCode();
    afx_msg void	OnChar(UINT n_char, UINT n_rep_cnt, UINT n_flags);
    afx_msg void	OnKeyDown(UINT n_char, UINT n_rep_cnt, UINT n_flags);
    afx_msg void	OnLButtonDown(UINT n_flags, CPoint point);
    afx_msg void	OnLButtonUp(UINT n_flags, CPoint point);
    afx_msg void	OnMouseMove(UINT n_flags, CPoint point);
    afx_msg void	OnLButtonDblClk(UINT n_flags, CPoint point);
    afx_msg void	OnContextMenu(CWnd* p_wnd, CPoint point);
    afx_msg BOOL	OnEraseBkgnd(CDC* p_dc);
    afx_msg void	OnTimer(UINT_PTR id_event);
    afx_msg void	OnEnable(BOOL b_enable);
    afx_msg void	OnSize(UINT n_type, int cx, int cy);
    afx_msg BOOL	OnSetCursor(CWnd* p_wnd, UINT n_hit_test, UINT message);
    afx_msg BOOL	OnMouseWheel(UINT n_flags, short z_delta, CPoint pt);

    // IME (MFC에 ON_WM_IME_STARTCOMPOSITION 매크로가 없어 ON_MESSAGE로 직접 처리)
    afx_msg LRESULT	on_ime_start_composition_message(WPARAM w_param, LPARAM l_param);
    afx_msg LRESULT	on_ime_end_composition_message(WPARAM w_param, LPARAM l_param);
    afx_msg LRESULT	on_ime_composition(WPARAM w_param, LPARAM l_param);

private:
    // ── 텍스트 버퍼 ──
    CString		m_text;              // 실제 텍스트
    int			m_caret_pos  = 0;    // 캐럿 문자 인덱스
    int			m_sel_start  = 0;    // 선택 시작
    int			m_sel_end    = 0;    // 선택 끝 (= 캐럿 위치)
    bool		m_selecting  = false;// 마우스 드래그 선택 중

    // 드래그 선택 중 가장자리 자동 스크롤
    bool		m_drag_scrolling = false;
    CPoint		m_last_drag_point;

    // IME 조합 중 문자
    CString		m_compose;           // 현재 조합 중인 문자열
    bool		m_composing = false;

    // ── 테두리 ──
    bool		m_draw_border   = true;
    int			m_border_width  = 1;
    int			m_border_radius = 0;   // 0 = 직사각형, >0 = 라운드

    // ── 폰트 ──
    LOGFONT		m_lf = {};
    CFont		m_font;
    void		rebuild_font();

    // ── 옵션 ──
    bool		m_readonly   = false;
    bool        m_use_default_readonly_color = true;
    bool		m_password   = false;
    TCHAR		m_mask_char  = _T('*');
    int			m_max_length = 0;      // 0 = 제한 없음
    int			m_padding    = 4;      // 텍스트 여백
    DWORD		m_valign     = DT_VCENTER;
    DWORD		m_halign     = DT_LEFT;    // DT_LEFT / DT_CENTER / DT_RIGHT
    bool		m_dynamic    = false;  // 동적 생성 여부
    bool		m_use_updown_key  = false;
    float		m_updown_interval = 1.0f;

    // set_parent_back_color() 명시 호출 여부.
    // Gdiplus::Color() 의 default 생성자가 Black opaque(0xFF000000) 로 초기화되기 때문에
    // m_theme.cr_parent_back.GetA() 만으로는 "미지정" 과 "사용자가 검정 설정" 을 구분 못 한다.
    bool		m_has_parent_back_color = false;

    // ── Dim text ──
    CString		m_dim_text;       // placeholder 텍스트. 색상은 m_theme.cr_text_dim 사용

    // ── 캐럿 ──
    int			m_caret_height  = 0;    // 빔 캐럿 기본 높이 (폰트 높이 기준)
    int			m_caret_width   = 2;    // 현재 생성된 캐럿 폭
    int			m_caret_cur_h   = 0;    // 현재 생성된 캐럿 높이
    bool		m_caret_visible = false;

    // ── 스크롤 (가로) ──
    int			m_scroll_offset = 0;    // 픽셀 단위 스크롤 오프셋

    // ── 카피 버튼 (우측 클립보드 아이콘) ──
    bool		m_use_copy_button       = false;
    bool		m_copy_button_capturing = false; // 마우스 캡처 중
    bool		m_copy_button_pressed   = false; // 시각 상태 (캡처 중 + 영역 내)
    CRect		get_copy_button_area() const;
    void		draw_copy_button(Gdiplus::Graphics& g);

    // ── 내부 헬퍼 ──
    CString		get_display_text() const;      // 패스워드 마스킹 적용된 표시용 텍스트
    CRect		get_text_area() const;         // 텍스트가 그려질 수 있는 영역 (패딩/보더/round/카피버튼 제외)
    CRect		get_text_rect() const;         // 실제 그려진 텍스트의 bounding box (글자 크기 + 정렬·스크롤 반영)
    // 가로 정렬 + 가로 스크롤을 반영한 "텍스트의 첫 글자 픽셀 x". draw_text / calc_caret_pixel_pos /
    // hit_test_char / get_compose_draw_box / draw_dim_text 가 공통으로 사용하여 정렬이 동기화됨.
    // text_width 는 현재 표시될 문자열(패스워드 마스킹 + IME 조합 포함) 의 GDI TextExtent.cx.
    int			get_text_start_x(int text_width) const;
    CPoint		calc_caret_pixel_pos(int char_pos) const; // 문자 인덱스 → 픽셀 좌표
    int			hit_test_char(CPoint pt) const;           // 픽셀 좌표 → 문자 인덱스
    CRect		get_compose_draw_box() const;             // 조합 중 글자가 그려지는 박스
    void		update_caret_pos();
    void		ensure_caret_visible();        // 캐럿이 보이도록 스크롤 조정

    void		insert_text(const CString& str);
    void		delete_selection();
    bool		has_selection() const { return m_sel_start != m_sel_end; }
    void		clear_selection()     { m_sel_start = m_sel_end = m_caret_pos; }

    void		do_copy();
    void		do_cut();
    void		do_paste();

    void		draw_background(Gdiplus::Graphics& g, const CRect& rc);
    void		draw_border(Gdiplus::Graphics& g, const CRect& rc);
    void		draw_selection(Gdiplus::Graphics& g, const CRect& rc_text);
    void		draw_text(Gdiplus::Graphics& g, const CRect& rc_text);
    void		draw_dim_text(Gdiplus::Graphics& g, const CRect& rc_text);


    void		notify_parent(int message);

    // Undo / Redo (이중 스택).
    //   - push_undo(): 편집 직전의 상태를 m_undo_stack에 저장하고 m_redo_stack은 clear.
    //   - do_undo()  : m_undo_stack pop → 현재 상태는 m_redo_stack으로 이동 → pop한 상태 적용.
    //   - do_redo()  : m_redo_stack pop → 현재 상태는 m_undo_stack으로 이동 → pop한 상태 적용.
    struct undo_state {
        CString	text;
        int		caret_pos;
    };
    CArray<undo_state, undo_state&> m_undo_stack;
    CArray<undo_state, undo_state&> m_redo_stack;
    void		push_undo();
    void		do_undo();
    void		do_redo();
    bool		can_undo() const { return m_undo_stack.GetSize() > 0; }
    bool		can_redo() const { return m_redo_stack.GetSize() > 0; }
};
