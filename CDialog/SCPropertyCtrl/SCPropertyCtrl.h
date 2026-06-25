#pragma once

#include <afxwin.h>
#include <afxdialogex.h>	// CDialogEx
#include <afxcmn.h>			// CToolTipCtrl (bool/combo/info 셀 label 툴팁)
#include <vector>
#include <memory>
#include <functional>
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
//
// 사용 예:
//   m_props.create(this, x, y, w, h);
//   m_props.set_color_theme(theme);
//   m_props.on_change = [this](const CString& key){ Invalidate(FALSE); auto_save_project(); };
//   m_props.begin();
//   m_props.section(_T("Canvas"));
//   m_props.begin_row();						// cx, cy 를 한 라인에
//   m_props.add_int  (_T("cx"), &m_canvas_cx);
//   m_props.add_int  (_T("cy"), &m_canvas_cy);
//   m_props.end_row();
//   m_props.add_color24(_T("Background"), &m_bg_color);	// alpha 무의미 → color24
//   m_props.add_combo(_T("Morphing"), (int*)&m_map_algorithm, { _T("edge"), _T("tone_fill"), _T("halftone") });
//   m_props.end();
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

	void add_text (const CString& label, CString* value);
	void add_int  (const CString& label, int* value);
	void add_real (const CString& label, float* value);
	void add_bool (const CString& label, bool* value);
	void add_color24(const CString& label, Gdiplus::Color* value);	// R,G,B (alpha 무시·항상 불투명)
	void add_color32(const CString& label, Gdiplus::Color* value);	// R,G,B,A
	void add_combo(const CString& label, int* index, const std::vector<CString>& options);
	void add_info (const CString& label, const CString& text);	// 읽기 전용
	void set_tooltip(const CString& text);	// 직전 add_*() 셀에 설명 툴팁(hover) 연결
	void end();												// 자식 컨트롤 생성 + 레이아웃 + 다시 그림

	void refresh();	// 외부에서 바인딩 값이 바뀐 경우 자식 컨트롤 표시 동기화 + 다시 그림

	// 사용자가 어떤 셀 값을 바꾸면 호출(인자 = 그 셀 label). 모델 갱신/자동저장 연결용.
	std::function<void(const CString& key)> on_change;

protected:
	enum class field_type { text, integer, real, boolean, color24, color32, combo, info };

	// 값 컬럼에 CSCStatic 자식을 두는 셀(text/int/real/color/info)인지.
	struct prop_cell
	{
		field_type		type = field_type::info;
		CString			label;
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
		int				label_w = 0;			// layout() 에서 계산된 라벨 컬럼 폭(측정 기반)
		CRect			rect;					// 셀 전체 영역(스크롤 적용 전, 패널 좌표)
		std::shared_ptr<CSCStatic> ctrl;		// 값 컬럼 자식(uses_static 셀만)

		// CSCStatic 자식이 값을 호스팅하는 셀. 라벨은 패널이 그리므로 info(값=고정 텍스트)는 패널이 전부 그림 → 제외.
		static bool uses_static(field_type t)
		{
			return t == field_type::text || t == field_type::integer || t == field_type::real
				|| t == field_type::color24 || t == field_type::color32;
		}
	};

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

	prop_cell&	add_cell(field_type type, const CString& label);	// 열린 라인 또는 새 라인에 셀 추가

	void	destroy_controls();				// begin() 시 기존 자식 파기
	void	ensure_controls();				// 값 셀에 CSCStatic 자식 생성(없으면)
	void	sync_value(prop_cell& c);		// 바인딩 포인터 → 자식 표시값
	void	position_cell_ctrl(prop_cell& c);				// 값 셀 자식 위치/표시(스크롤 반영)
	int		measure_label_width(const CString& text) const;	// 라벨 텍스트 픽셀 폭(m_font 기준)
	CRect	cell_value_rect(const CRect& cell_rc, int label_w) const;	// 셀 안 값 컬럼(라벨 컬럼 제외)
	prop_cell* find_cell_by_ctrl(const CWnd* pCtrl);

	void	layout();			// y 커서 누적 → 각 행 rect + 셀 분할/배치 + m_content_h
	void	layout_cells(prop_row& row);	// 라인 셀들을 균등 분할 + 자식 배치
	void	clamp_scroll();
	int		hit_test_row(CPoint pt) const;	// 행 인덱스(-1=없음). pt=클라이언트 좌표
	void	toggle_section(int row_index);
	void	open_combo_menu(prop_cell& c);	// combo 셀 클릭 → 다크 CSCMenu 팝업(옵션 항목 + 현재 선택 radio)
	void	rebuild_widget_tooltips();		// bool/combo/info 셀 label 영역에 설명 툴팁(rect 툴) 재등록

	void	draw_section(CDC& dc, const prop_row& r, const CRect& rc);
	void	draw_cell_label(CDC& dc, const prop_cell& c, const CRect& cell_rc);	// 라벨 컬럼(패널이 직접 그림)
	void	draw_widget(CDC& dc, const prop_cell& c, const CRect& cell_rc);		// bool/combo 위젯(값 컬럼)
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
