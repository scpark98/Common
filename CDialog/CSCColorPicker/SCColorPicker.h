#pragma once
#include "afxwin.h"
#include <vector>
#include <gdiplus.h>	// SCGdiplusBitmap.h 대체 (팔레트 비트맵 제거)

#include "../../colors.h"
#include "../../CEdit/SCEdit/SCEdit.h"
#include "../../CSliderCtrl/SCSliderCtrl/SCSliderCtrl.h"


// CSCColorPicker 대화 상자

class CSCColorPicker : public CDialog
{
	DECLARE_DYNAMIC(CSCColorPicker)

public:
	CSCColorPicker(CWnd* pParent = nullptr, CString title = _T(""), bool as_modal = true);
	virtual ~CSCColorPicker();

	bool			create(CWnd* parent, CString title = _T(""), bool as_modal = true);
	Gdiplus::Color	get_selected_color() const { return m_sel_color; }

protected:
	bool			m_as_modal = true;
	CWnd*			m_parent = NULL;
	int				m_response = -1;
	bool			m_edit_syncing = false;	// sync_edits() 진행 중 EN_CHANGE 재진입 방지. 깜빡임 방지

	CSCSliderCtrl	m_slider_alpha;
	CSCSliderCtrl	m_slider_hue;		// ← NEW: Hue 슬라이더 (0~360)
	CSCSliderCtrl	m_slider_light;		// ← NEW: 밝기(Value) 슬라이더 (0~100)
	CSCEdit			m_edit_hexa;
	CSCEdit			m_edit_argb[4];

	// ── 팔레트 레이아웃 상수 ──────────────────────────────
	static constexpr int PALETTE_COLOR_COLS = 10;
	static constexpr int PALETTE_TOTAL_COLS = PALETTE_COLOR_COLS;		// 무채색 별도 행 → 열 수 동일
	static constexpr int PALETTE_COLOR_ROWS = 6;						// 색상 변화 행 수
	static constexpr int PALETTE_ROWS = PALETTE_COLOR_ROWS + 1;		// 무채색 행(1) + 색상 행(6) = 7

	static constexpr int BTN_ADD_IDX = 0;	// "+" 사용자 정의색 추가
	static constexpr int BTN_DROPPER_IDX = 1;	// 스포이드 (미구현, 공간 예약)
	static constexpr int PALETTE_BTN_COUNT = 2;	// 예약 버튼 수

	// 팔레트 10열 중 버튼 2칸 제외 → 최근 색상 최대 8개
	static constexpr int MAX_RECENT_COLORS = PALETTE_TOTAL_COLS - PALETTE_BTN_COUNT;	// 8

	static constexpr int IDC_SLIDER_ALPHA = 1001;	// ← 알파 슬라이더 ID
	static constexpr int IDC_SLIDER_HUE = 1002;	// ← NEW
	static constexpr int IDC_SLIDER_LIGHT = 1003;	// ← NEW

	// ── 편집 컨트롤 ID ────────────────────────────────────
	static constexpr int IDC_EDIT_HEXA = 1011;
	static constexpr int IDC_EDIT_ARGB_A = 1012;
	static constexpr int IDC_EDIT_ARGB_R = 1013;
	static constexpr int IDC_EDIT_ARGB_G = 1014;
	static constexpr int IDC_EDIT_ARGB_B = 1015;

	// ── 편집 영역 레이아웃 상수 ───────────────────────────
	static constexpr int kEditH = 22;
	static constexpr int kEditGap = 4;
	static constexpr int kLabelW = 14;
	static constexpr int kCellGap = 4;

	struct PaletteSV { float s, v; };
	static const float      m_hues[PALETTE_COLOR_COLS];
	static const PaletteSV  m_sv_rows[PALETTE_COLOR_ROWS];		// [8] → [6]

	// ── 팔레트 레이아웃 메트릭 (calc_layout에서 1회 설정) ─
	float	m_cell			= 0.f;
	float	m_margin		= 0.f;
	float	m_radius		= 0.f;
	CRect	m_r_palette	= { 0, 0, 0, 0 };	// 좌표 계산의 단일 기준
	CRect	m_r_preview	= { 0, 0, 0, 0 };	// 선택 색상 미리보기 영역
	CRect	m_r_slider_alpha	= { 0, 0, 0, 0 };	// ← NEW: 슬라이더 위치 추적
	CRect	m_r_slider_hue	= { 0, 0, 0, 0 };	// ← NEW: Hue 슬라이더 위치 추적
	CRect	m_r_slider_light	= { 0, 0, 0, 0 };	// ← NEW: 밝기(Value) 슬라이더 위치 추적
	CRect	m_r_edit_area = { 0, 0, 0, 0 };	// ← NEW: 편집 컨트롤 전체 영역

	// ── 선택 색상 ─────────────────────────────────────────
	Gdiplus::Color	m_sel_color;

	// ── HSV 상태 (슬라이더 연동용) ────────────────────────
	float	m_hue = 0.f;	// ← NEW: 색조 0~360
	float	m_sat = 1.f;	// ← NEW: 채도 0~1
	float	m_val = 1.f;	// ← NEW: 밝기 0~1

	// ── 최근 색상 ─────────────────────────────────────────
	std::vector<Gdiplus::Color>	m_recent_colors;

	// ── 히트 타겟 ─────────────────────────────────────────
	enum class HitArea { None, Palette, Recent, Button };

	struct HitTarget {
		HitArea area = HitArea::None;
		int     col = -1;   // Palette
		int     row = -1;   // Palette
		int     idx = -1;   // Recent / Button

		bool operator==(const HitTarget& o) const {
			return area == o.area && col == o.col && row == o.row && idx == o.idx;
		}
		bool is_valid() const { return area != HitArea::None; }
	};

	HitTarget				m_hover;
	HitTarget				m_sel;

	// ── 레이아웃 ──────────────────────────────────────────
	void					calc_layout();									// create()에서 1회 호출
	Gdiplus::PointF			get_cell_center(const HitTarget& t) const;		// 유일한 좌표 계산 함수

	// ── 데이터 헬퍼 ───────────────────────────────────────
	static Gdiplus::Color	hsv_to_color(float h, float s, float v);
	Gdiplus::Color			get_color_at(int col, int row) const;
	Gdiplus::Color			get_color(const HitTarget& t) const;
	bool					hit_test(CPoint pt, HitTarget& out) const;

	void					load_recent_colors();
	void					save_recent_color(Gdiplus::Color cr);

	// ── 렌더링 ────────────────────────────────────────────
	void					draw_palette(Gdiplus::Graphics& g) const;
	void					draw_color_circle(Gdiplus::Graphics& g, Gdiplus::PointF center, float r, Gdiplus::Color cr) const;
	void					draw_recent_colors(Gdiplus::Graphics& g) const;
	void					draw_buttons(Gdiplus::Graphics& g) const;	// "+" 및 스포이드 버튼
	void					open_custom_color_dlg();					// "+" 클릭 시 호출
	void					draw_hover_circle(Gdiplus::Graphics& g, const HitTarget& t) const;
	void					draw_selected_mark(Gdiplus::Graphics& g, const HitTarget& t) const;
	void					draw_overlays(Gdiplus::Graphics& g) const;		// hover + 체크마크 통합
	void					draw_color_preview(Gdiplus::Graphics& g) const;	// ← NEW: RoundRect 미리보기

	// ── 슬라이더 ──────────────────────────────────────────
	void					update_slider_alpha();	// 선택 색상에 맞춰 슬라이더 위치·색상 동기화
	void					update_slider_hue();	// ← NEW
	void					update_slider_light();	// ← NEW

	// ── 편집 컨트롤 동기화 ────────────────────────────────
	void					sync_edits();				// ← NEW: m_sel_color → 편집 컨트롤 반영
	void					apply_edit_to_color();		// ← NEW: 편집 컨트롤 → m_sel_color 반영

	void					on_btn_add_clicked();							// ← RENAMED: "+" 클릭 → recent 추가
	HitTarget				find_color(Gdiplus::Color target) const;

	LRESULT					on_message_CSCSliderCtrl(WPARAM wParam, LPARAM lParam);	// ← 슬라이더 이벤트 핸들러
	LRESULT					on_message_CSCEdit(WPARAM wParam, LPARAM lParam);

protected:
	virtual void DoDataExchange(CDataExchange* pDX);

	DECLARE_MESSAGE_MAP()
public:
	virtual INT_PTR DoModal(CString title = _T(""), Gdiplus::Color cr_selected = Gdiplus::Color::Transparent);
	afx_msg void OnBnClickedOk();
	afx_msg void OnBnClickedCancel();
	virtual BOOL OnInitDialog();
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	afx_msg void OnPaint();
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
	afx_msg void OnRButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnRButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnLButtonDblClk(UINT nFlags, CPoint point);
	afx_msg BOOL OnEraseBkgnd(CDC* pDC);
	afx_msg void OnEnChangeHexa();		// ← NEW
	afx_msg void OnEnChangeArgb();		// ← NEW
	virtual void PostNcDestroy();
	afx_msg void OnDestroy();
};
