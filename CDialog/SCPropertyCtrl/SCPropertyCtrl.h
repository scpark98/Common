#pragma once

#include <afxwin.h>
#include <afxdialogex.h>	// CDialogEx
#include <vector>
#include <memory>
#include <functional>
#include "../../colors.h"					// CSCColorTheme (Gdiplus::Color 기반 다크 테마)
#include "../../CStatic/SCStatic/SCStatic.h"	// label+값 행. value 클릭→CSCEdit 편집 / "_color picker_" 모드→CSCColorPicker

// ── CSCPropertyCtrl ─────────────────────────────────────────────────────────
// 데이터 바인딩형 속성 컨트롤. CMFCPropertyGridCtrl 대체용(다크 테마 + Figma 스타일).
//
//  - CDialogEx 파생 + 코드 생성(#32770 클래스 복제, IDD 불필요) → 프로젝트 .rc 를 건드리지 않음.
//  - 레이아웃은 세로 1방향 누적(y 커서). 섹션 접기/펼치기 지원.
//  - 값 행(text/int/real/color/info)은 UXStudio 처럼 CSCStatic 자식으로 호스팅한다.
//    · text/int/real : set_use_edit → value 영역 클릭 시 CSCEdit 동적 생성·편집.
//    · color         : 캡션 "_color picker_" → swatch+RGB 표시, swatch 클릭 시 CSCColorPicker.
//    · 라벨은 패널이 직접 그리고(컬럼 정렬 일관), CSCStatic 은 값 컬럼만 차지한다.
//  - 변경은 Message_CSCStatic 으로 통지받아 바인딩 포인터에 되쓰고 on_change(label) 발화.
//  - 선언적 빌드: begin() → section()/add_*() → end(). 컨텍스트(전역/드론/타임라인) 전환 시 다시 build.
//
// 사용 예:
//   m_props.create(this, x, y, w, h);
//   m_props.set_color_theme(theme);
//   m_props.on_change = [this](const CString& key){ Invalidate(FALSE); auto_save_project(); };
//   m_props.begin();
//   m_props.section(_T("Canvas"));
//   m_props.add_int  (_T("cx"), &m_canvas_cx);
//   m_props.add_color(_T("Background"), &m_bg_color);
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
	void add_text (const CString& label, CString* value);
	void add_int  (const CString& label, int* value);
	void add_real (const CString& label, float* value);
	void add_bool (const CString& label, bool* value);
	void add_color(const CString& label, Gdiplus::Color* value);
	void add_combo(const CString& label, int* index, const std::vector<CString>& options);
	void add_info (const CString& label, const CString& text);	// 읽기 전용
	void end();												// 자식 컨트롤 생성 + 레이아웃 + 다시 그림

	void refresh();	// 외부에서 바인딩 값이 바뀐 경우 자식 컨트롤 표시 동기화 + 다시 그림

	// 사용자가 어떤 행 값을 바꾸면 호출(인자 = 그 행 label). 모델 갱신/자동저장 연결용.
	std::function<void(const CString& key)> on_change;

protected:
	enum class row_type { section, text, integer, real, boolean, color, combo, info };

	// 값 컬럼에 CSCStatic 자식을 두는 행(text/int/real/color/info)인지.
	static bool uses_static(row_type t)
	{
		return t == row_type::text || t == row_type::integer || t == row_type::real
			|| t == row_type::color || t == row_type::info;
	}

	struct prop_row
	{
		row_type		type = row_type::info;
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
		bool			expanded = true;		// 섹션 헤더 전용
		int				section_index = -1;		// 소속 섹션 헤더의 행 인덱스(헤더면 자기 자신)
		bool			visible = true;			// 접힌 섹션의 자식이면 false
		CRect			rect;					// 콘텐츠 좌표(스크롤 적용 전)
		std::shared_ptr<CSCStatic> ctrl;		// 값 컬럼 자식(uses_static 행만)
	};

	std::vector<prop_row>	m_rows;
	int						m_cur_section = -1;
	CSCColorTheme			m_theme = CSCColorTheme(this);
	CFont					m_font;

	int		m_scroll_y  = 0;	// 세로 스크롤 오프셋(px). (TODO: CSCScrollbar 연결)
	int		m_content_h = 0;	// 전체 콘텐츠 높이

	// 레이아웃 상수
	int		m_row_h      = 26;
	int		m_section_h  = 28;
	int		m_pad        = 10;
	int		m_label_ratio = 45;	// 라벨 칼럼 비율(%)

	void	destroy_controls();				// begin() 시 기존 자식 파기
	void	ensure_controls();				// 값 행에 CSCStatic 자식 생성(없으면)
	void	sync_value(prop_row& r);		// 바인딩 포인터 → 자식 표시값
	CRect	value_rect(const CRect& row_rect) const;	// 값 컬럼(라벨 컬럼 제외)
	int		find_row_by_ctrl(const CWnd* pCtrl) const;

	void	layout();			// y 커서 누적 → 각 행 rect + 자식 위치/표시 + m_content_h
	void	clamp_scroll();
	int		hit_test(CPoint pt) const;	// 행 인덱스(-1=없음). pt=클라이언트 좌표
	void	toggle_section(int row_index);

	void	draw_section(CDC& dc, const prop_row& r, const CRect& rc);
	void	draw_row_label(CDC& dc, const prop_row& r, const CRect& rc);	// 라벨 컬럼(패널이 직접 그림)
	void	draw_widget(CDC& dc, const prop_row& r, const CRect& rc);	// bool/combo 위젯(값 컬럼)
	static void draw_caret(CDC& dc, const CRect& box, bool expanded, COLORREF cr);
	static void draw_tri_down(CDC& dc, const CRect& box, COLORREF cr);

	// 자식 컨트롤로 쓰이므로 Enter/Esc 로 다이얼로그가 닫히면 안 된다(기본 OnOK/OnCancel = 닫기).
	// 편집 중 Esc 는 CSCStatic 내부 edit 이 먼저 처리하므로 여기 빈 override 로 닫힘만 막는다.
	virtual void OnOK();
	virtual void OnCancel();

	DECLARE_MESSAGE_MAP()
	afx_msg void OnPaint();
	afx_msg BOOL OnEraseBkgnd(CDC* pDC);
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg BOOL OnMouseWheel(UINT nFlags, short zDelta, CPoint pt);
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg LRESULT on_message_CSCStatic(WPARAM wParam, LPARAM lParam);	// CSCStatic 값 변경 통지
};
