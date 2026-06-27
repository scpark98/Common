#pragma once

#include <afxwin.h>
#include <afxdialogex.h>	// CDialogEx
#include <afxcmn.h>			// CToolTipCtrl (bool/combo/info 셀 label 툴팁)
#include <vector>
#include <memory>
#include <functional>
#include <type_traits>		// std::is_same_v — set_value / get_value template 의 if constexpr 분기
#include "../../colors.h"					// CSCColorTheme (Gdiplus::Color 기반 다크 테마)
#include "../../CStatic/SCStatic/SCStatic.h"	// label+값 셀. value 클릭→CSCEdit 편집 / "_color picker_" 모드→CSCColorPicker
#include "../../CMenu/CSCMenuBar/SCMenu.h"	// combo 드롭다운 = 다크 테마 CSCMenu 팝업

// ── CSCPropertyCtrl ─────────────────────────────────────────────────────────
// 데이터 바인딩형 속성 컨트롤. CMFCPropertyGridCtrl 대체용(다크 테마 + Figma 스타일).
//
//  - CDialogEx 파생 + 코드 생성(#32770 클래스 복제, IDD 불필요) → 프로젝트 .rc 를 건드리지 않음.
//  - 한 줄(prop_row)은 1..N 개의 셀(prop_cell)을 가질 수 있다. 여러 셀이면 라인을 균등 분할.
//    · begin_row()~end_row() 사이의 add_*() 들이 한 라인을 공유한다(없으면 add 마다 1셀 1라인).
//  - 셀 값(text/int/real/color/info)은 CSCStatic 자식으로 호스팅한다.
//    · text/int/real : value 클릭 시 CSCEdit 동적 생성·편집. CSCStatic 이 라벨+값을 그림.
//    · color24/color32: 캡션 "_color picker_" → swatch + "R,G,B"(24, alpha 무시) / "R,G,B,A"(32). 라벨은 패널이 그림.
//    · bool/combo    : 라벨·위젯 모두 패널이 직접 그림.
//  - 모든 셀의 값 left 는 layout() 에서 셀폭·m_label_ratio 로 매번 재계산 → resize 에도 일관 정렬.
//  - 변경은 Message_CSCStatic 으로 통지받아 바인딩 포인터에 되쓰고 on_change(label) 발화.
//  - 선언적 빌드: begin() → section()/[begin_row()]/add_*()/[end_row()] → end().
//  - end() 이후 동적 제어(라벨 변경/값 설정/활성화/옵션 교체/툴팁) 는 find_property / set_* / get_* API.
//
// 사용 예:
//   m_props.create(this, x, y, w, h);
//   m_props.set_color_theme(theme);
//   m_props.on_change = [this](const CString& key){ Invalidate(FALSE); auto_save_project(); };
//   m_props.begin();
//   m_props.section(_T("Canvas"));
//   m_props.begin_row();						// cx, cy 를 한 라인에
//   m_props.add_int  (&m_canvas_cx, _T("cx"));
//   m_props.add_int  (&m_canvas_cy, _T("cy"));
//   m_props.end_row();
//   m_props.add_color24(&m_bg_color, _T("Background"));	// alpha 무의미 → color24
//   m_props.add_combo  (&m_map_algorithm, _T("Morphing"), { _T("edge"), _T("tone_fill"), _T("halftone") });
//   m_props.end();
//
//   // 동적 제어 (end() 이후 어디서나):
//   m_props.set_enabled(_T("cy"), some_condition);                          // cy 항목 비활성
//   m_props.set_enabled_by_binding(&m_bg_color, false);                     // 바인딩으로도 가능
//   m_props.set_label(_T("cx"), _T("width"));                               // 라벨 변경
//   m_props.set_value_int(_T("cx"), 1920);                                  // 값 변경
//   m_props.set_options(_T("Morphing"), { _T("none"), _T("edge") });        // combo 옵션 교체
class CSCPropertyCtrl : public CDialogEx
{
	DECLARE_DYNAMIC(CSCPropertyCtrl)

public:
	CSCPropertyCtrl();
	virtual ~CSCPropertyCtrl();

	bool create(CWnd* parent, int left = 0, int top = 0, int width = 280, int height = 400);
	void set_color_theme(const CSCColorTheme& theme, bool invalidate = true);

	// ── 선언적 빌드 ──
	void begin();											// 모든 행 제거(컨텍스트 교체 시작)
	void section(const CString& title, bool expanded = true);

	void begin_row();										// 이후 add_*() 들이 한 라인을 균등 분할해 공유
	void end_row();											// 라인 공유 종료(이후 add 는 다시 1셀 1라인)

	// 항목 추가 — template 통합. 인자 순서: 값 포인터 → 레이블 → 툴팁(선택).
	// T 추론: CString→text, int→integer, float→real, bool→boolean, Gdiplus::Color→color24 (alpha 무시).
	// 새 타입 지원은 하단 apply_add 의 if constexpr 분기 한 줄 추가.
	template<typename T> void add(T* value, const CString& label, const CString& tooltip = _T(""));
	// combo — options 인자로 add<int> 와 overload dispatch. (인자 수가 달라 add<int> 와 충돌 없음.)
	void add(int* index, const CString& label, const std::vector<CString>& options, const CString& tooltip = _T(""));
	// color32 — alpha 노출(R,G,B,A). default add<Gdiplus::Color> = color24 라 alpha 가 필요할 때만 별도.
	void add_color32(Gdiplus::Color* value, const CString& label, const CString& tooltip = _T(""));
	// info — 바인딩 없는 읽기 전용 셀. value 포인터 대신 표시 텍스트 직접.
	void add_info(const CString& label, const CString& text, const CString& tooltip = _T(""));
	void end();												// 자식 컨트롤 생성 + 레이아웃 + 다시 그림

	void refresh();	// 외부에서 바인딩 값이 바뀐 경우 자식 컨트롤 표시 동기화 + 다시 그림

	// ── 항목 핸들 / 타입 (find_property 리턴 타입) ─────────────────────────
	// prop_cell 의 내부 layout 필드(rect, label_w, ctrl)는 사용자가 수정하면 표시 정합이 깨질 수 있다 — 읽기 전용.
	enum class field_type { text, integer, real, boolean, color24, color32, combo, info };

	struct prop_cell
	{
		field_type		type = field_type::info;
		CString			label;
		bool			enabled = true;			// set_enabled 가 토글. false 시 dim 표시 + 입력 차단.
		// 바인딩(타입별 하나만 유효)
		CString*		p_text  = nullptr;
		int*			p_int   = nullptr;
		float*			p_real  = nullptr;
		bool*			p_bool  = nullptr;
		Gdiplus::Color*	p_color = nullptr;
		int*			p_index = nullptr;
		std::vector<CString> options;
		CString			info_text;
		CString			tooltip;				// 항목 설명(hover). text/int/real/color=자식 CSCStatic 내장 툴팁, bool/combo/info=패널이 label 에 띄움.
		int				label_w = 0;			// [internal] layout() 에서 계산된 라벨 컬럼 폭(측정 기반)
		CRect			rect;					// [internal] 셀 전체 영역(스크롤 적용 전, 패널 좌표)
		std::shared_ptr<CSCStatic> ctrl;		// [internal] 값 컬럼 자식(uses_static 셀만)

		// CSCStatic 자식이 값을 호스팅하는 셀. 라벨은 패널이 그리므로 info(값=고정 텍스트)는 패널이 전부 그림 → 제외.
		static bool uses_static(field_type t)
		{
			return t == field_type::text || t == field_type::integer || t == field_type::real
				|| t == field_type::color24 || t == field_type::color32;
		}
	};

	// ── 항목 동적 변경 (end() 이후) ────────────────────────────────────────
	//  - label / binding 두 키로 동일 동작. label 변형이 primary (on_change 키와 일치).
	//  - 라벨 중복(같은 라벨이 두 셀) 시 첫 일치를 사용.
	//  - 모든 set_* 는 성공 시 true, 키 미일치 / 셀 타입 불일치는 false.
	//  - 프로그래매틱 set_* 는 on_change 콜백을 발화시키지 않는다(사용자 편집과 구분, 재귀 차단).
	prop_cell*			find_property				(const CString& label);
	const prop_cell*	find_property				(const CString& label) const;
	prop_cell*			find_property_by_binding	(const void* binding);
	const prop_cell*	find_property_by_binding	(const void* binding) const;
	bool				has_property				(const CString& label) const;
	bool				has_property_by_binding		(const void* binding) const;

	// 활성/비활성: false 면 dim 컬러로 표시 + 클릭/휠/편집 입력 모두 무시.
	bool	set_enabled				(const CString& label, bool enabled);
	bool	set_enabled_by_binding	(const void* binding, bool enabled);
	bool	get_enabled				(const CString& label) const;
	bool	get_enabled_by_binding	(const void* binding) const;

	// 라벨 변경: 표시 라벨 + on_change 키 동시 갱신, 라벨 컬럼 폭 재계산.
	bool	set_label				(const CString& old_label, const CString& new_label);
	bool	set_label_by_binding	(const void* binding, const CString& new_label);
	CString	get_label_by_binding	(const void* binding) const;

	// 값 설정/조회 — template 통합. 지원 T : CString, int, float, bool, Gdiplus::Color.
	//  - set_value(label, v)   : 인자 타입으로 T 자동 추론. set_value(_T("CX"), 1920) → T=int.
	//  - get_value<T>(label)   : T 명시 필요(반환 타입 추론 불가).
	//  - 셀 타입 미일치 시 set 은 false, get 은 기본값(빈 문자열 / 0 / false / Transparent).
	//  - 새 타입 추가는 apply_value_dispatch / get_value_dispatch 의 if constexpr 분기만 늘리면 끝.
	template<typename T> bool	set_value			(const CString& label,   const T& value);
	template<typename T> bool	set_value_by_binding(const void*    binding, const T& value);
	template<typename T> T		get_value			(const CString& label)   const;
	template<typename T> T		get_value_by_binding(const void*    binding) const;

	// combo 옵션 교체: 현재 인덱스를 새 범위로 clamp(빈 목록이면 -1).
	bool	set_options				(const CString& label, const std::vector<CString>& options);
	bool	set_options_by_binding	(const void* binding, const std::vector<CString>& options);
	const std::vector<CString>&	get_options(const CString& label) const;

	// 툴팁: 빈 문자열 → 툴팁 제거. text/int/real/color = 자식 CSCStatic 내장 툴팁, bool/combo/info = 패널이 label 영역에 띄움.
	bool	set_tooltip				(const CString& label, const CString& tooltip = _T(""));
	bool	set_tooltip_by_binding	(const void* binding, const CString& tooltip = _T(""));

	// 사용자가 어떤 셀 값을 바꾸면 호출(인자 = 그 셀 label). 모델 갱신/자동저장 연결용.
	std::function<void(const CString& key)> on_change;

protected:
	struct prop_row
	{
		bool			is_section = false;
		CString			section_title;			// 섹션 헤더 전용
		bool			expanded = true;		// 섹션 헤더 전용
		std::vector<prop_cell> cells;			// 라인 전용(1..N 셀)
		int				section_index = -1;		// 소속 섹션 헤더의 행 인덱스(헤더면 자기 자신)
		bool			visible = true;			// 접힌 섹션의 자식이면 false
		CRect			rect;					// 행 전체(스크롤 적용 전)
	};

	std::vector<prop_row>	m_rows;
	int						m_cur_section = -1;
	int						m_open_line   = -1;	// begin_row()~end_row() 사이면 그 행 인덱스
	CSCColorTheme			m_theme = CSCColorTheme(this);
	CFont					m_font;
	CFont					m_font_bold;	// 섹션(카테고리) 헤더 전용

	// combo 드롭다운 — 다크 테마 CSCMenu 팝업(네이티브 CMenu 대신). 선택은 Message_CSCMenu 로 async 통지.
	CSCMenu					m_combo_menu;
	prop_cell*				m_combo_active = nullptr;	// 현재 열린 combo 메뉴의 대상 셀(통지 처리용)

	// bool/combo/info 셀은 값 컬럼을 호스팅하는 자식 컨트롤이 없어 패널이 직접 툴팁을 띄운다(label 영역에 rect 툴).
	// text/int/real/color 는 자식 CSCStatic 의 내장 툴팁(set_tooltip_text)이 처리하므로 여기 대상이 아니다.
	CToolTipCtrl			m_tooltip;
	UINT					m_tip_tool_count = 0;	// 직전 layout 에서 등록한 rect 툴 개수(스크롤/리레이아웃마다 통째 재구성)

	int		m_scroll_y  = 0;	// 세로 스크롤 오프셋(px). (TODO: CSCScrollbar 연결)
	int		m_content_h = 0;	// 전체 콘텐츠 높이

	// 레이아웃 상수
	int		m_row_h       = 36;	// 한 행 높이(라인 간격) — 값 필드는 이 안에서 수직 여백을 두고 그려짐(박스 ~28px)
	int		m_section_h   = 28;	// 섹션 헤더 높이
	int		m_section_gap = 14;	// 섹션 헤더 위 여백(첫 섹션 제외) — 카테고리를 위 항목과 떨어뜨림
	int		m_pad         = 10;
	int		m_field_round = 6;	// 값 필드 라운드 반경
	int		m_field_vmargin = 4;// 값 필드 상하 여백(행 높이 대비) → 박스 높이 = m_row_h - 2*m_field_vmargin
	int		m_cell_gap    = 8;	// 한 라인에 여러 셀일 때 셀(필드) 사이 가로 간격
	int		m_field_lpad  = 8;	// 박스 안 라벨(prefix) 왼쪽 여백
	int		m_label_gap   = 10;	// 박스 안 라벨(prefix)과 값 사이 간격
	int		m_label_ratio = 45;	// 라벨 컬럼 폭 상한(셀 폭의 %) — 측정값이 이보다 크면 cap
	int		m_shared_label_w = 0;	// 단일 셀 행들의 공통 라벨 폭(값 left 정렬용, layout 에서 계산)

	Gdiplus::Color	field_back_color() const;	// 필드 박스 채움 — 패널 배경보다 약간 밝게(테두리 없음)
	Gdiplus::Color	field_label_color() const;	// 박스 안 라벨(prefix) — 값보다 흐리되 읽히는 회색
	// 패널 그리기 셀(bool/combo/info)의 라벨·값 색 — enabled 면 normal, disabled 면 dim.
	Gdiplus::Color	cell_label_color(const prop_cell& c) const;
	Gdiplus::Color	cell_text_color (const prop_cell& c) const;

	prop_cell&	add_cell(field_type type, const CString& label);	// 열린 라인 또는 새 라인에 셀 추가

	void	destroy_controls();				// begin() 시 기존 자식 파기
	void	ensure_controls();				// 값 셀에 CSCStatic 자식 생성(없으면)
	void	sync_value(prop_cell& c);		// 바인딩 포인터 → 자식 표시값
	void	position_cell_ctrl(prop_cell& c);				// 값 셀 자식 위치/표시(스크롤 반영)
	int		measure_label_width(const CString& text) const;	// 라벨 텍스트 픽셀 폭(m_font 기준)
	CRect	cell_value_rect(const CRect& cell_rc, int label_w) const;	// 셀 안 값 컬럼(라벨 컬럼 제외)
	prop_cell* find_cell_by_ctrl(const CWnd* pCtrl);

	// set_* / 라이브 변경 워커 — label / binding 두 entry 가 동일 워커를 거쳐 동작 일치 보장.
	bool	apply_enabled       (prop_cell* pc, bool enabled);
	void	apply_enabled_visual(prop_cell& c);				// CSCStatic 자식 EnableWindow + dim 색 동기
	bool	apply_label         (prop_cell* pc, const CString& new_label);
	bool	apply_tooltip       (prop_cell* pc, const CString& tooltip);
	bool	apply_options       (prop_cell* pc, const std::vector<CString>& options);
	// add / set_value / get_value template 의 if constexpr dispatch — 본문 자체가 워커. 정의는 헤더 하단 inline.
	//  - apply_add<T>(c, value) : T → field_type 결정 + 바인딩 포인터 셋. add<T> 가 호출.
	//  - apply_set<T>(pc, v)    : T + 셀 타입 검증 + 바인딩에 쓰기 + sync_value + Invalidate. set_value/_by_binding 가 호출.
	//  - apply_get<T>(pc)       : T + 셀 타입 검증 + 바인딩 읽기. get_value/_by_binding 가 호출.
	template<typename T> void	apply_add(prop_cell& c, T* value);
	template<typename T> bool	apply_set(prop_cell* pc, const T& value);
	template<typename T> T		apply_get(const prop_cell* pc) const;

	void	layout();			// y 커서 누적 → 각 행 rect + 셀 분할/배치 + m_content_h
	void	layout_cells(prop_row& row);	// 라인 셀들을 균등 분할 + 자식 배치
	void	clamp_scroll();
	int		hit_test_row(CPoint pt) const;	// 행 인덱스(-1=없음). pt=클라이언트 좌표
	void	toggle_section(int row_index);
	void	open_combo_menu(prop_cell& c);	// combo 셀 클릭 → 다크 CSCMenu 팝업(옵션 항목 + 현재 선택 radio)
	void	rebuild_widget_tooltips();		// bool/combo/info 셀 label 영역에 설명 툴팁(rect 툴) 재등록

	void	draw_section(CDC& dc, const prop_row& r, const CRect& rc);
	void	draw_cell_label(CDC& dc, const prop_cell& c, const CRect& cell_rc);	// 라벨 컬럼(패널이 직접 그림)
	void	draw_widget(CDC& dc, const prop_cell& c, const CRect& cell_rc);		// combo 위젯(값 컬럼)
	void	draw_bool  (CDC& dc, const prop_cell& c, const CRect& cell_rc);		// bool 셀(라벨 넓게 + 체크박스 우측)
	static void draw_caret(CDC& dc, const CRect& box, bool expanded, COLORREF cr);
	static void draw_tri_down(CDC& dc, const CRect& box, COLORREF cr);

	// 자식 컨트롤로 쓰이므로 Enter/Esc 로 다이얼로그가 닫히면 안 된다(기본 OnOK/OnCancel = 닫기).
	// 편집 중 Esc 는 CSCStatic 내부 edit 이 먼저 처리하므로 여기 빈 override 로 닫힘만 막는다.
	virtual void OnOK();
	virtual void OnCancel();
	virtual BOOL PreTranslateMessage(MSG* pMsg);	// 패널 툴팁(bool/combo/info) hover relay

	DECLARE_MESSAGE_MAP()
	afx_msg void OnPaint();
	afx_msg BOOL OnEraseBkgnd(CDC* pDC);
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg BOOL OnMouseWheel(UINT nFlags, short zDelta, CPoint pt);
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg LRESULT on_message_CSCStatic(WPARAM wParam, LPARAM lParam);	// CSCStatic 값 변경 통지
	afx_msg LRESULT on_message_CSCMenu(WPARAM wParam, LPARAM lParam);	// combo CSCMenu 선택 통지
};

// ── add / set_value / get_value template 정의 (inline) ─────────────────────
// 본문이 곧 dispatch — apply_add / apply_set / apply_get 의 if constexpr 분기 한 곳에 T 처리 로직 집중.
// 새 타입 추가는 세 워커의 분기 한 줄씩 추가하면 끝 (add_T / set_value_T / get_value_T 별도 함수 작성 불필요).

template<typename T>
inline void CSCPropertyCtrl::apply_add(prop_cell& c, T* value)
{
	if constexpr (std::is_same_v<T, CString>)             { c.type = field_type::text;    c.p_text  = value; }
	else if constexpr (std::is_same_v<T, int>)            { c.type = field_type::integer; c.p_int   = value; }
	else if constexpr (std::is_same_v<T, float>)          { c.type = field_type::real;    c.p_real  = value; }
	else if constexpr (std::is_same_v<T, bool>)           { c.type = field_type::boolean; c.p_bool  = value; }
	else if constexpr (std::is_same_v<T, Gdiplus::Color>) { c.type = field_type::color24; c.p_color = value; }	// alpha 무시(=color32 는 별도 add_color32)
	else { static_assert(sizeof(T) == 0, "add: 지원 T 는 CString / int / float / bool / Gdiplus::Color"); }
}

template<typename T>
inline bool CSCPropertyCtrl::apply_set(prop_cell* pc, const T& value)
{
	if (!pc)
		return false;

	if constexpr (std::is_same_v<T, CString>)
	{
		switch (pc->type)
		{
		case field_type::text: if (pc->p_text) *pc->p_text = value; break;
		case field_type::info: pc->info_text = value; break;					// info 는 바인딩 없음 → 셀 내부 텍스트 직접
		default: return false;
		}
	}
	else if constexpr (std::is_same_v<T, int>)
	{
		switch (pc->type)
		{
		case field_type::integer: if (pc->p_int) *pc->p_int = value; break;
		case field_type::combo:
			if (pc->p_index)
			{
				const int n = static_cast<int>(pc->options.size());
				*pc->p_index = (n == 0) ? -1 : (std::max)(0, (std::min)(n - 1, value));
			}
			break;
		default: return false;
		}
	}
	else if constexpr (std::is_same_v<T, float>)
	{
		if (pc->type != field_type::real) return false;
		if (pc->p_real) *pc->p_real = value;
	}
	else if constexpr (std::is_same_v<T, bool>)
	{
		if (pc->type != field_type::boolean) return false;
		if (pc->p_bool) *pc->p_bool = value;
	}
	else if constexpr (std::is_same_v<T, Gdiplus::Color>)
	{
		if (pc->type != field_type::color24 && pc->type != field_type::color32) return false;
		if (pc->p_color) *pc->p_color = value;
	}
	else { static_assert(sizeof(T) == 0, "set_value: 지원 T 는 CString / int / float / bool / Gdiplus::Color"); }

	sync_value(*pc);					// 자식 컨트롤 있는 셀만 표시 갱신, bool/info/combo 등 자식 없는 셀은 안전 no-op
	if (GetSafeHwnd())
		Invalidate(FALSE);
	return true;
}

template<typename T>
inline T CSCPropertyCtrl::apply_get(const prop_cell* pc) const
{
	if constexpr (std::is_same_v<T, CString>)
	{
		if (!pc)
			return CString();
		if (pc->type == field_type::text && pc->p_text)
			return *pc->p_text;
		if (pc->type == field_type::info)
			return pc->info_text;
		return CString();
	}
	else if constexpr (std::is_same_v<T, int>)
	{
		if (!pc)
			return 0;
		if (pc->type == field_type::integer && pc->p_int)
			return *pc->p_int;
		if (pc->type == field_type::combo   && pc->p_index)
			return *pc->p_index;
		return 0;
	}
	else if constexpr (std::is_same_v<T, float>)
		return (pc && pc->type == field_type::real    && pc->p_real) ? *pc->p_real : 0.f;
	else if constexpr (std::is_same_v<T, bool>)
		return (pc && pc->type == field_type::boolean && pc->p_bool) ? *pc->p_bool : false;
	else if constexpr (std::is_same_v<T, Gdiplus::Color>)
	{
		if (pc && (pc->type == field_type::color24 || pc->type == field_type::color32) && pc->p_color)
			return *pc->p_color;
		return Gdiplus::Color::Transparent;
	}
	else { static_assert(sizeof(T) == 0, "get_value: 지원 T 는 CString / int / float / bool / Gdiplus::Color"); }
}

template<typename T>
inline void CSCPropertyCtrl::add(T* value, const CString& label, const CString& tooltip)
{
	prop_cell& c = add_cell(field_type::info, label);	// 임시 type, apply_add 가 T 에 맞게 덮어씀
	apply_add(c, value);
	c.tooltip = tooltip;
}

template<typename T> inline bool	CSCPropertyCtrl::set_value				(const CString& label,   const T& value)	{ return apply_set(find_property(label),             value); }
template<typename T> inline bool	CSCPropertyCtrl::set_value_by_binding	(const void*    binding, const T& value)	{ return apply_set(find_property_by_binding(binding), value); }
template<typename T> inline T		CSCPropertyCtrl::get_value				(const CString& label)   const				{ return apply_get<T>(find_property(label)); }
template<typename T> inline T		CSCPropertyCtrl::get_value_by_binding	(const void*    binding) const				{ return apply_get<T>(find_property_by_binding(binding)); }
