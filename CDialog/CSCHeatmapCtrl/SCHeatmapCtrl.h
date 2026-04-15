#pragma once
#include "afxdialogex.h"

#include <map>
#include "Common/directx/CSCD2Context/SCD2Context.h"

#include <dwrite.h>
#pragma comment(lib, "dwrite")

// CSCHeatmapCtrl 대화 상자

class CSCHeatmapCell
{
public:
	CSCHeatmapCell() {}
	~CSCHeatmapCell() {}

	D2D1_COLOR_F	cr_fill;
	D2D1_COLOR_F	cr_border = { 0.0f, 0.0f, 0.0f, 0.0f };	//alpha=0이면 기본값 사용(m_cr_border)
	D2D1_COLOR_F	cr_text = { 0.0f, 0.0f, 0.0f, 1.0f };		//셀 텍스트 색상. 기본 검정.
	CString			text;		//셀 중앙에 그려지는 텍스트
	CString			tooltip;
	DWORD_PTR		data = 0;	//사용자 임의 데이터 저장용. 필요 없으면 사용하지 않아도 됨.
};

class CSCHeatmapCtrl : public CDialogEx
{
	DECLARE_DYNAMIC(CSCHeatmapCtrl)

public:
	CSCHeatmapCtrl(CWnd* pParent = nullptr);   // 표준 생성자입니다.
	virtual ~CSCHeatmapCtrl();

	bool					create(CWnd* parent, int cell_col = -1);
	void					set_back_color(Gdiplus::Color cr_back);

	void					set_cell_col(int col) { if (col > 0) m_cell_col = col; Invalidate(); }
	//num 갯수만큼 더 추가한다. cell_col, 즉 가로 셀의 갯수를 재지정 할 수 있다.
	//0이면 현재 설정값 사용. 만약 현재 설정값이 0이라면 기본값으로 자동 계산한다.
	int						add_cell(int add_num, int cell_col = 0);

	void					set_cell_size(int cell_size_x, int cell_size_y) { m_cell_size.cx = cell_size_x; m_cell_size.cy = cell_size_y; Invalidate(); }

	//Shift+Wheel로 확대/축소할 때의 셀 크기 제한 범위 설정
	void					set_cell_size_range(CSize size_min, CSize size_max) { m_cell_size_min = size_min; m_cell_size_max = size_max; }

	void					set_cell_color(int x, int y, Gdiplus::Color cr, bool redraw = true);
	void					set_cell_color(int index, Gdiplus::Color cr, bool redraw = true);
	void					set_cell_text(int x, int y, CString& text, bool redraw = true);
	void					set_cell_text(int index, CString& text, bool redraw = true);
	void					set_cell_tooltip(int x, int y, CString& tooltip, bool redraw = true);
	void					set_cell_tooltip(int index, CString& tooltip, bool redraw = true);

	void					set_cell_border_color(int index, Gdiplus::Color cr_border, bool redraw = true);

	//전체 셀의 기본 테두리 색상 설정. Transparent이면 테두리 없음.
	void					set_border_color(Gdiplus::Color cr_border = Gdiplus::Color::Transparent, bool redraw = true);

	DWORD_PTR				get_item_data(int index);
	void					set_item_data(int index, DWORD_PTR data);

	//fit_to_client=true이면 전체 셀이 client에 맞도록 자동 축소/확대. false이면 m_zoom 수치 사용.
	void					set_fit_to_client(bool fit);
	bool					get_fit_to_client() const { return m_fit_to_client; }

	int						size() const { return m_total; }

	// 현재 유효 열 수 반환 (고정모드: 윈도우 너비 기반 동적 계산, 자동모드: m_cell_col)
	int						calc_col_count();
	// 현재 유효 행 수 반환
	int						row_count() { int c = calc_col_count(); return (m_total > 0 && c > 0) ? (m_total + c - 1) / c : 0; }

	void					set_redraw(bool redraw) { m_redraw = redraw; }
protected:
	CWnd*					m_parent = nullptr;
	CSCD2Context*			m_pSharedD2DC = nullptr;
	CSCD2Context			m_d2dc;
	ID2D1SolidColorBrush*	m_brush = nullptr;
	IDWriteFactory*			m_WriteFactory = nullptr;
	IDWriteTextFormat*		m_WriteFormat = nullptr;

	int						m_cell_col = 0;		// 가로 열 수. >0이면 고정, <=0이면 셀 크기와 너비로 자동 계산
	int						m_total = 0;		// 전체 셀 수

	//각 셀의 크기. <=0 이면 자동 계산(줌/가로세로 스크롤 지원), >0 이면 고정 크기(세로 스크롤만 지원)
	CSize					m_cell_size = { 12, 12 };
	//셀 간격
	CSize					m_cell_gap = { 4, 4 };

	//Shift+Wheel로 확대/축소할 때의 셀 크기 제한 범위
	CSize					m_cell_size_min = { 8, 8 };
	CSize					m_cell_size_max = { 80, 80 };

	D2D1_COLOR_F			m_cr_cell = { 0.94f, 0.94f, 0.94f, 1.0f };
	D2D1_COLOR_F			m_cr_back = { 1.0f, 1.0f, 1.0f, 1.0f };
	D2D1_COLOR_F			m_cr_border = { 0.0f, 0.0f, 0.0f, 0.0f };	//전체 기본 테두리 색상. alpha=0이면 테두리 없음.

	// 개별 셀 정보 저장 (key: 선형 인덱스)
	std::map<int, CSCHeatmapCell>	m_cell;

	// 줌/스크롤 (m_cell_size <= 0 자동모드에서 사용)
	bool					m_fit_to_client = true;		// true: 전체 셀이 client에 맞도록 자동 크기 조절
	float					m_zoom = 1.0f;				// fit 기준 배율 (fit_to_client=false일 때 적용)
	D2D1_POINT_2F			m_scroll = { 0.0f, 0.0f };	// 스크롤 오프셋 (px)

	// 드래그 스크롤
	bool					m_bDragging = false;
	CPoint					m_drag_start_pt;			// 드래그 시작 마우스 위치
	D2D1_POINT_2F			m_drag_start_scroll;		// 드래그 시작 시 스크롤 값

	//설정값대로 heatmap을 다시 그린다.
	void					draw_heatmap();

	//고정모드에서 실제 렌더링에 사용되는 셀 크기를 반환
	//m_cell_col > 0이면 셀 너비를 클라이언트 너비에 맞게 자동 계산 (가로 꽉 채움)
	//m_cell_col <= 0이면 m_cell_size를 그대로 사용
	void					get_effective_cell_size(float& cell_w, float& cell_h);

	// 마우스 좌표로부터 셀 인덱스를 반환. 셀 위가 아니면 {-1,-1}
	CPoint					hit_test_cell(CPoint pt);

	// 자동모드에서 client에 전체 셀이 들어가는 기준 셀 크기 반환
	float					calc_fit_cell_size();
	// 스크롤 범위를 유효 범위로 클램프
	void					clamp_scroll();

	// 툴팁
	CToolTipCtrl			m_tooltip;
	CPoint					m_hover_cell = { -1, -1 };
	bool					m_bMouseTracking = false;

	//다수를 추가할 때 매번 invalidate()하지 않기 위해.
	bool					m_redraw = true;

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 지원입니다.

	DECLARE_MESSAGE_MAP()
public:
	virtual BOOL OnInitDialog();
	virtual void PreSubclassWindow();
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	afx_msg void OnPaint();
	afx_msg void OnSize(UINT nType, int cx, int cy);
	virtual void OnOK();
	virtual void OnCancel();
	afx_msg BOOL OnEraseBkgnd(CDC* pDC);
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
	afx_msg void OnMouseLeave();
	afx_msg BOOL OnMouseWheel(UINT nFlags, short zDelta, CPoint pt);
	afx_msg LRESULT OnMouseHWheel(WPARAM wParam, LPARAM lParam);
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
	afx_msg BOOL OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message);
};