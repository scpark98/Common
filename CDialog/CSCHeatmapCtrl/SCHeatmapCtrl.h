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

	D2D1_COLOR_F	color = { 0.8f, 0.8f, 0.8f, 1.0f };
	CString			text;
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
	void					add_cell(int add_num, int cell_col = 0);

	void					set_cell_size(int cell_size_x, int cell_size_y) { m_cell_size.cx = cell_size_x; m_cell_size.cy = cell_size_y; Invalidate(); }
	void					set_cell_color(int x, int y, Gdiplus::Color cr);
	void					set_cell_color(int index, Gdiplus::Color cr);
	void					set_cell_text(int x, int y, const CString& text);

	DWORD_PTR				get_cell_data(int index);
	void					set_cell_data(int index, DWORD_PTR data);

	//fit_to_client=true이면 전체 셀이 client에 맞도록 자동 축소/확대. false이면 m_zoom 수치 사용.
	void					set_fit_to_client(bool fit);
	bool					get_fit_to_client() const { return m_fit_to_client; }

	int						size() const { return m_total; }
	int						row_count() const { return (m_total > 0 && m_cell_col > 0) ? (m_total + m_cell_col - 1) / m_cell_col : 0; }
protected:
	CWnd* m_parent = nullptr;
	CSCD2Context* m_pSharedD2DC = nullptr;
	CSCD2Context			m_d2dc;
	ID2D1SolidColorBrush* m_brush;
	IDWriteFactory* m_WriteFactory;
	IDWriteTextFormat* m_WriteFormat;

	int						m_cell_col = 0;		// 가로 열 수
	int						m_total = 0;		// 전체 셀 수

	//각 셀의 크기. <=0 이면 자동 계산
	CSize					m_cell_size = { 0, 0 };
	//셀 간격
	CSize					m_cell_gap = { 4, 4 };
	D2D1_COLOR_F			m_cr_cell = { 0.8f, 0.8f, 0.8f, 1.0f };
	D2D1_COLOR_F			m_cr_back = { 1.0f, 1.0f, 1.0f, 1.0f };

	// 개별 셀 정보 저장 (key: {x, y})
	std::map<std::pair<int, int>, CSCHeatmapCell>	m_cell;

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