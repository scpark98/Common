// SCHeatmapCtrl.cpp: 구현 파일
//

#include "SCHeatmapCtrl.h"
#include "../../Functions.h"

// CSCHeatmapCtrl 대화 상자

IMPLEMENT_DYNAMIC(CSCHeatmapCtrl, CDialogEx)

CSCHeatmapCtrl::CSCHeatmapCtrl(CWnd* pParent /*=nullptr*/)
{

}

CSCHeatmapCtrl::~CSCHeatmapCtrl()
{
}

void CSCHeatmapCtrl::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}


BEGIN_MESSAGE_MAP(CSCHeatmapCtrl, CDialogEx)
	ON_WM_PAINT()
	ON_WM_SIZE()
	ON_WM_ERASEBKGND()
	ON_WM_MOUSEMOVE()
	ON_WM_MOUSELEAVE()
	ON_WM_MOUSEWHEEL()
	ON_MESSAGE(WM_MOUSEHWHEEL, OnMouseHWheel)
	ON_WM_LBUTTONDOWN()
	ON_WM_LBUTTONUP()
	ON_WM_SETCURSOR()
END_MESSAGE_MAP()

bool CSCHeatmapCtrl::create(CWnd* parent, int cell_col)
{
	if (parent == nullptr)
		parent = AfxGetApp()->GetMainWnd();

	if (parent == nullptr)
		return false;

	m_parent = parent;

	if (cell_col > 0)
		m_cell_col = cell_col;

	//WS_CLIPCHILDREN을 줘야만 slider 컨트롤이 깜빡이지 않고 제대로 그려짐. (WS_CLIPCHILDREN 없으면 슬라이더를 움직일 때마다 팔레트 전체가 리페인트됨)
	// WS_EX_TOOLWINDOW은 확장 스타일로 분리 (이전 코드에서 일반 스타일에 잘못 혼입되어 있었음)
	// WS_VISIBLE은 modeless 경로에서만 초기 생성 시 포함.
	// modal 경로는 DoModal()에서 ShowWindow(SW_SHOW)로 직접 표시하므로 초기 가시화 불필요.
	// → UIAutomation이 즉시 추적을 시작하여 DestroyWindow 시점에 충돌하는 현상 방지.
	DWORD dwStyle = WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN;

	WNDCLASS wc = {};
	::GetClassInfo(AfxGetInstanceHandle(), _T("#32770"), &wc);
	wc.lpszClassName = _T("CSCHeatmapCtrl");
	AfxRegisterClass(&wc);

	bool res = CreateEx(NULL, wc.lpszClassName, _T("CSCHeatmapCtrl"), dwStyle, CRect(0, 0, 500, 400), parent, 0);

	if (!res)
		return false;

	dwStyle = GetWindowLongPtr(m_hWnd, GWL_STYLE);
	dwStyle &= ~(WS_CAPTION);
	SetWindowLongPtr(m_hWnd, GWL_STYLE, dwStyle);

	if (m_pSharedD2DC)
		m_d2dc.init(m_hWnd, m_pSharedD2DC);
	else
		m_d2dc.init(m_hWnd);

	HRESULT hr = DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED, __uuidof(IDWriteFactory), reinterpret_cast<IUnknown**>(&m_WriteFactory));
	if (SUCCEEDED(hr))
	{
		m_WriteFactory->CreateTextFormat(_T("Arial"), nullptr, DWRITE_FONT_WEIGHT_NORMAL, DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL, 10.0f, _T("ko-kr"), &m_WriteFormat);
		m_WriteFormat->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_CENTER);
		m_WriteFormat->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
	}

	m_d2dc.get_d2dc()->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::Black), &m_brush);


	// 툴팁 초기화
	m_tooltip.Create(this, TTS_ALWAYSTIP | TTS_NOPREFIX);
	m_tooltip.SetDelayTime(TTDT_AUTOPOP, 10000);
	m_tooltip.SetDelayTime(TTDT_INITIAL, 1);
	m_tooltip.SetDelayTime(TTDT_RESHOW, 500);
	m_tooltip.SetMaxTipWidth(300);

	CRect rc;
	GetClientRect(rc);
	m_tooltip.AddTool(this, _T(""), &rc, 1);
	m_tooltip.Activate(TRUE);

	return res;
}

// CSCHeatmapCtrl 메시지 처리기

BOOL CSCHeatmapCtrl::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// TODO:  여기에 추가 초기화 작업을 추가합니다.

	return TRUE;  // return TRUE unless you set the focus to a control
	// 예외: OCX 속성 페이지는 FALSE를 반환해야 합니다.
}

void CSCHeatmapCtrl::PreSubclassWindow()
{
	// TODO: 여기에 특수화된 코드를 추가 및/또는 기본 클래스를 호출합니다.

	CDialogEx::PreSubclassWindow();
}

BOOL CSCHeatmapCtrl::PreTranslateMessage(MSG* pMsg)
{
	//키입력은 무조건 parent에서 처리한다.
	if (pMsg->message == WM_KEYDOWN)
	{
		return FALSE;
	}

	// 툴팁에 마우스 메시지 전달
	if (m_tooltip.GetSafeHwnd())
		m_tooltip.RelayEvent(pMsg);

	return CDialogEx::PreTranslateMessage(pMsg);
}

void CSCHeatmapCtrl::OnPaint()
{
	CPaintDC dc(this); // device context for painting
	// TODO: 여기에 메시지 처리기 코드를 추가합니다.
	// 그리기 메시지에 대해서는 CDialogEx::OnPaint()을(를) 호출하지 마십시오.
	CRect rc;
	GetClientRect(rc);

	ID2D1DeviceContext* d2dc = m_d2dc.get_d2dc();
	if (d2dc == nullptr)
		return;

	D2D1_SIZE_F sz_dc = m_d2dc.get_size();

	d2dc->BeginDraw();
	d2dc->SetTransform(D2D1::Matrix3x2F::Identity());

	//black으로 칠한 후
	d2dc->Clear(m_cr_back);

	draw_heatmap();

	HRESULT hr = d2dc->EndDraw();

	if (SUCCEEDED(hr))
		hr = m_d2dc.get_swapchain()->Present(0, 0);
}

void CSCHeatmapCtrl::OnSize(UINT nType, int cx, int cy)
{
	CDialogEx::OnSize(nType, cx, cy);

	// TODO: 여기에 메시지 처리기 코드를 추가합니다.
	if (m_d2dc.get_d2dc() == nullptr)
		return;

	m_d2dc.on_size_changed(cx, cy);

	// 툴팁 영역도 갱신
	if (m_tooltip.GetSafeHwnd())
	{
		CRect rc;
		GetClientRect(rc);
		m_tooltip.SetToolRect(this, 1, &rc);
	}

	clamp_scroll();
	Invalidate();
}

void CSCHeatmapCtrl::set_back_color(Gdiplus::Color cr_back)
{
	m_cr_back = get_d2color(cr_back);
	Invalidate();
}

//num 갯수만큼 더 추가한다. cell_col, 즉 가로 셀의 갯수를 재지정 할 수 있다.
//0이면 현재 설정값 사용. 만약 현재 설정값이 0이라면 기본값으로 자동 계산한다.
//고정모드(m_cell_size > 0)에서는 m_cell_col > 0일 때 열 수 고정, 아니면 너비에서 동적 계산.
int CSCHeatmapCtrl::add_cell(int num, int cell_col)
{
	if (num <= 0)
		return -1;

	bool auto_mode = (m_cell_size.cx <= 0 && m_cell_size.cy <= 0);

	if (cell_col > 0)
	{
		m_cell_col = cell_col;
	}
	else if (auto_mode && m_cell_col <= 0)
	{
		// 자동모드에서만 m_cell_col 초기화 필요 (고정모드는 calc_col_count()에서 동적 계산)
		CRect rc;
		GetClientRect(rc);
		m_cell_col = (rc.Width() / (8 + m_cell_gap.cx));
	}

	m_total += num;

	clamp_scroll();
	Invalidate();

	return (m_total - 1);
}

void CSCHeatmapCtrl::set_fit_to_client(bool fit)
{
	m_fit_to_client = fit;
	if (fit)
	{
		m_zoom = 1.0f;
		m_scroll = { 0.0f, 0.0f };
	}
	Invalidate();
}

void CSCHeatmapCtrl::OnOK()
{
}

void CSCHeatmapCtrl::OnCancel()
{
}

void CSCHeatmapCtrl::set_cell_color(int x, int y, Gdiplus::Color cr, bool redraw)
{
	int cols = calc_col_count();
	if (cols <= 0 || x < 0 || x >= cols || y < 0)
		return;

	int index = y * cols + x;
	if (index >= m_total)
		return;

	D2D1_COLOR_F cr_fill = get_d2color(cr);
	if (m_cell[index].cr_fill != cr_fill)
	{
		m_cell[index].cr_fill = cr_fill;
		if (m_redraw && redraw) Invalidate();
	}
}

void CSCHeatmapCtrl::set_cell_color(int index, Gdiplus::Color cr, bool redraw)
{
	if (index < 0 || index >= m_total)
		return;

	D2D1_COLOR_F cr_fill = get_d2color(cr);
	if (m_cell[index].cr_fill != cr_fill)
	{
		m_cell[index].cr_fill = cr_fill;
		if (m_redraw && redraw) Invalidate();
	}
}

void CSCHeatmapCtrl::set_cell_text(int x, int y, CString& text, bool redraw)
{
	int cols = calc_col_count();
	if (cols <= 0 || x < 0 || x >= cols || y < 0)
		return;
	int index = y * cols + x;
	if (index >= m_total)
		return;
	if (m_cell[index].text != text)
	{
		m_cell[index].text = text;
		if (m_redraw && redraw) Invalidate();
	}
}

void CSCHeatmapCtrl::set_cell_text(int index, CString& text, bool redraw)
{
	if (index < 0 || index >= m_total)
		return;
	if (m_cell[index].text != text)
	{
		m_cell[index].text = text;
		if (m_redraw && redraw) Invalidate();
	}
}

void CSCHeatmapCtrl::set_cell_tooltip(int x, int y, CString& tooltip, bool redraw)
{
	int cols = calc_col_count();
	if (cols <= 0 || x < 0 || x >= cols || y < 0)
		return;

	int index = y * cols + x;
	if (index >= m_total)
		return;

	if (m_cell[index].tooltip != tooltip)
	{
		m_cell[index].tooltip = tooltip;
		if (m_redraw && redraw) Invalidate();
	}
}

void CSCHeatmapCtrl::set_cell_tooltip(int index, CString& tooltip, bool redraw)
{
	if (index < 0 || index >= m_total)
		return;

	if (m_cell[index].tooltip != tooltip)
	{
		m_cell[index].tooltip = tooltip;
		if (m_redraw && redraw) Invalidate();
	}
}

void CSCHeatmapCtrl::set_cell_border_color(int index, Gdiplus::Color cr_border, bool redraw)
{
	if (index < 0 || index >= m_total)
		return;

	D2D1_COLOR_F cr_b = get_d2color(cr_border);
	if (m_cell[index].cr_border != cr_b)
	{
		m_cell[index].cr_border = cr_b;
		if (m_redraw && redraw) Invalidate();
	}
}

void CSCHeatmapCtrl::set_border_color(Gdiplus::Color cr_border, bool redraw)
{
	m_cr_border = get_d2color(cr_border);
	if (m_redraw && redraw) Invalidate();
}

DWORD_PTR CSCHeatmapCtrl::get_item_data(int index)
{
	if (index < 0 || index >= m_total)
		return 0;

	auto it = m_cell.find(index);
	if (it != m_cell.end())
		return it->second.data;

	return 0;
}

void CSCHeatmapCtrl::set_item_data(int index, DWORD_PTR data)
{
	if (index < 0 || index >= m_total)
		return;

	m_cell[index].data = data;

	//새로 생성한 셀의 채움색은 전체에 적용되는 기본 색상으로 초기화한다.
	m_cell[index].cr_fill = m_cr_cell;
}

BOOL CSCHeatmapCtrl::OnEraseBkgnd(CDC* pDC)
{
	// TODO: 여기에 메시지 처리기 코드를 추가 및/또는 기본값을 호출합니다.
	return FALSE;
	return CDialogEx::OnEraseBkgnd(pDC);
}

//------------------------------------------------------------
// 현재 유효 열 수 반환
// - 자동모드(m_cell_size <= 0): m_cell_col 반환
// - 고정모드(m_cell_size > 0):
//     m_cell_col > 0이면 그 값으로 고정
//     m_cell_col <= 0이면 윈도우 너비에서 동적 계산
//------------------------------------------------------------
int CSCHeatmapCtrl::calc_col_count()
{
	bool auto_mode = (m_cell_size.cx <= 0 && m_cell_size.cy <= 0);
	if (auto_mode)
		return m_cell_col;

	// 고정 셀 크기 모드: m_cell_col > 0이면 해당 값 사용
	if (m_cell_col > 0)
		return m_cell_col;

	// m_cell_col <= 0이면 클라이언트 너비에서 열 수 동적 계산
	float cell_w = (float)max(1, m_cell_size.cx);
	float gap_x = (float)m_cell_gap.cx;
	float margin_x = gap_x;

	float width;
	if (m_d2dc.get_d2dc())
		width = m_d2dc.get_size().width;
	else
	{
		CRect rc;
		GetClientRect(rc);
		width = (float)rc.Width();
	}

	if (width <= margin_x * 2.0f)
		return 1;

	int cols = (int)((width - margin_x * 2.0f + gap_x) / (cell_w + gap_x));
	return max(1, cols);
}

//------------------------------------------------------------
// 고정모드에서 실제 렌더링에 사용되는 셀 크기를 반환
// - m_cell_col > 0: 셀 너비를 클라이언트 너비에 맞게 자동 계산 (가로 꽉 채움)
// - m_cell_col <= 0: m_cell_size를 그대로 사용
//------------------------------------------------------------
void CSCHeatmapCtrl::get_effective_cell_size(float& cell_w, float& cell_h)
{
	bool auto_mode = (m_cell_size.cx <= 0 && m_cell_size.cy <= 0);

	if (auto_mode)
	{
		float fit = calc_fit_cell_size();
		float sz = m_fit_to_client ? fit : max(1.0f, fit * m_zoom);
		cell_w = cell_h = sz;
		return;
	}

	// 고정모드 기본값
	cell_w = (m_cell_size.cx > 0) ? (float)m_cell_size.cx : 1.0f;
	cell_h = (m_cell_size.cy > 0) ? (float)m_cell_size.cy : cell_w;

	// m_cell_col > 0이면 셀 너비를 클라이언트 너비에 맞게 자동 계산
	if (m_cell_col > 0)
	{
		float gap_x = (float)m_cell_gap.cx;
		float margin_x = gap_x;

		float width;
		if (m_d2dc.get_d2dc())
			width = m_d2dc.get_size().width;
		else
		{
			CRect rc;
			GetClientRect(rc);
			width = (float)rc.Width();
		}

		float area_w = width - margin_x * 2.0f;
		cell_w = (area_w - gap_x * max(0, m_cell_col - 1)) / (float)m_cell_col;
		if (cell_w < 1.0f) cell_w = 1.0f;
	}
}

//------------------------------------------------------------
// 자동모드에서 client에 전체 셀이 들어가는 정사각 셀 크기 반환
//------------------------------------------------------------
float CSCHeatmapCtrl::calc_fit_cell_size()
{
	if (!m_d2dc.get_d2dc())
		return 1.0f;

	int cols = calc_col_count();
	int rows = (m_total > 0 && cols > 0) ? (m_total + cols - 1) / cols : 0;
	if (cols <= 0 || rows <= 0)
		return 1.0f;

	D2D1_SIZE_F sz_dc = m_d2dc.get_size();
	float margin_x = (float)m_cell_gap.cx;
	float margin_y = (float)m_cell_gap.cy;
	float area_w = sz_dc.width - margin_x * 2.0f;
	float area_h = sz_dc.height - margin_y * 2.0f;
	if (area_w < 1.0f || area_h < 1.0f)
		return 1.0f;

	float gap_x = (float)m_cell_gap.cx;
	float gap_y = (float)m_cell_gap.cy;

	float fit_w = (area_w - gap_x * (cols - 1)) / (float)cols;
	float fit_h = (area_h - gap_y * (rows - 1)) / (float)rows;

	return max(1.0f, min(fit_w, fit_h));
}

//------------------------------------------------------------
// 스크롤 오프셋을 유효 범위 내로 클램프
//------------------------------------------------------------
void CSCHeatmapCtrl::clamp_scroll()
{
	int cols = calc_col_count();
	int rows = (m_total > 0 && cols > 0) ? (m_total + cols - 1) / cols : 0;

	bool auto_mode = (m_cell_size.cx <= 0 && m_cell_size.cy <= 0);

	// 고정모드: 가로 스크롤 없음
	if (!auto_mode)
		m_scroll.x = 0.0f;

	// fit_to_client 자동모드에서는 스크롤 불필요
	if (auto_mode && m_fit_to_client)
	{
		m_scroll = { 0.0f, 0.0f };
		return;
	}

	if (!m_d2dc.get_d2dc())
		return;

	if (cols <= 0 || rows <= 0)
	{
		m_scroll = { 0.0f, 0.0f };
		return;
	}

	D2D1_SIZE_F sz_dc = m_d2dc.get_size();
	float gap_x = (float)m_cell_gap.cx;
	float gap_y = (float)m_cell_gap.cy;
	float margin_x = gap_x;
	float margin_y = gap_y;

	float cell_w, cell_h;
	get_effective_cell_size(cell_w, cell_h);

	float total_w = margin_x * 2.0f + cols * cell_w + (cols - 1) * gap_x;
	float total_h = margin_y * 2.0f + rows * cell_h + (rows - 1) * gap_y;

	float max_x = auto_mode ? max(0.0f, total_w - sz_dc.width) : 0.0f;
	float max_y = max(0.0f, total_h - sz_dc.height);

	m_scroll.x = max(0.0f, min(m_scroll.x, max_x));
	m_scroll.y = max(0.0f, min(m_scroll.y, max_y));
}

//------------------------------------------------------------
// 히트 테스트 : 마우스 좌표 → 셀 인덱스 (스크롤 반영)
//------------------------------------------------------------
CPoint CSCHeatmapCtrl::hit_test_cell(CPoint pt)
{
	if (!m_d2dc.get_d2dc())
		return { -1, -1 };

	int cols = calc_col_count();
	int rows = (m_total > 0 && cols > 0) ? (m_total + cols - 1) / cols : 0;
	if (cols <= 0 || rows <= 0)
		return { -1, -1 };

	float margin_x = (float)m_cell_gap.cx;
	float margin_y = (float)m_cell_gap.cy;
	float gap_x = (float)m_cell_gap.cx;
	float gap_y = (float)m_cell_gap.cy;

	float cell_w, cell_h;
	get_effective_cell_size(cell_w, cell_h);

	// 화면 좌표 → 컨텐츠 좌표 (스크롤 반영)
	float fx = (float)pt.x + m_scroll.x - margin_x;
	float fy = (float)pt.y + m_scroll.y - margin_y;
	if (fx < 0 || fy < 0)
		return { -1, -1 };

	float stride_x = cell_w + gap_x;
	float stride_y = cell_h + gap_y;

	int ix = (int)(fx / stride_x);
	int iy = (int)(fy / stride_y);

	if (ix < 0 || ix >= cols || iy < 0 || iy >= rows)
		return { -1, -1 };

	// 마지막 행 부분 채움 처리: 선형 인덱스가 m_total 이상이면 무효
	if (iy * cols + ix >= m_total)
		return { -1, -1 };

	// gap 영역 위인지 확인
	float local_x = fx - ix * stride_x;
	float local_y = fy - iy * stride_y;
	if (local_x > cell_w || local_y > cell_h)
		return { -1, -1 };

	return { ix, iy };
}

//------------------------------------------------------------
// Shift+Wheel: 셀 크기 확대/축소,  Wheel: 세로 스크롤
//------------------------------------------------------------
BOOL CSCHeatmapCtrl::OnMouseWheel(UINT nFlags, short zDelta, CPoint pt)
{
	if (!m_d2dc.get_d2dc())
		return CDialogEx::OnMouseWheel(nFlags, zDelta, pt);

	ScreenToClient(&pt);

	bool auto_mode = (m_cell_size.cx <= 0 && m_cell_size.cy <= 0);

	if (nFlags & MK_SHIFT)
	{
		//--- 고정모드: Shift+Wheel로 m_cell_size 확대/축소 ---
		if (!auto_mode)
		{
			// 현재 크기의 ~20%씩 변경 (최소 1px)
			int step = max(1, m_cell_size.cy / 5);
			if (zDelta < 0) step = -step;

			int new_cx = max((int)m_cell_size_min.cx, min((int)m_cell_size_max.cx, (int)m_cell_size.cx + step));
			int new_cy = max((int)m_cell_size_min.cy, min((int)m_cell_size_max.cy, (int)m_cell_size.cy + step));

			if (new_cx != m_cell_size.cx || new_cy != m_cell_size.cy)
			{
				m_cell_size.cx = new_cx;
				m_cell_size.cy = new_cy;
				clamp_scroll();
				Invalidate();
			}
			return TRUE;
		}

		//--- 자동모드: 기존 줌 동작 유지 ---
		float fit = calc_fit_cell_size();
		float old_cell = m_fit_to_client ? fit : max(1.0f, fit * m_zoom);

		// fit 상태에서 축소 방향이면 무시
		if (m_fit_to_client && zDelta <= 0)
			return TRUE;

		// fit → 수동 전환
		if (m_fit_to_client)
		{
			m_fit_to_client = false;
			m_zoom = 1.0f;
			m_scroll = { 0.0f, 0.0f };
		}

		// 줌 적용
		float factor = (zDelta > 0) ? 1.15f : (1.0f / 1.15f);
		m_zoom *= factor;

		// 줌 아웃하여 1.0 이하가 되면 fit 모드로 복귀
		if (m_zoom <= 1.0f)
		{
			m_zoom = 1.0f;
			m_fit_to_client = true;
			m_scroll = { 0.0f, 0.0f };
			Invalidate();
			return TRUE;
		}

		// 마우스 위치 기준으로 줌 (마우스 아래 셀이 고정되도록 스크롤 보정)
		float new_cell = max(1.0f, fit * m_zoom);
		float gap_x = (float)m_cell_gap.cx;
		float gap_y = (float)m_cell_gap.cy;
		float margin_x = gap_x;
		float margin_y = gap_y;

		float stride_old_x = old_cell + gap_x;
		float stride_old_y = old_cell + gap_y;
		float stride_new_x = new_cell + gap_x;
		float stride_new_y = new_cell + gap_y;

		if (stride_old_x > 0 && stride_old_y > 0)
		{
			float frac_x = (m_scroll.x + (float)pt.x - margin_x) / stride_old_x;
			float frac_y = (m_scroll.y + (float)pt.y - margin_y) / stride_old_y;
			m_scroll.x = frac_x * stride_new_x + margin_x - (float)pt.x;
			m_scroll.y = frac_y * stride_new_y + margin_y - (float)pt.y;
		}

		clamp_scroll();
		Invalidate();
		return TRUE;
	}
	else
	{
		//--- 세로 스크롤 ---
		float step = 40.0f;
		m_scroll.y += (float)(-zDelta) / WHEEL_DELTA * step;
		clamp_scroll();
		Invalidate();
		return TRUE;
	}
}

//------------------------------------------------------------
// WM_MOUSEHWHEEL: 가로 스크롤 (틸트 휠, 자동모드 전용)
//------------------------------------------------------------
LRESULT CSCHeatmapCtrl::OnMouseHWheel(WPARAM wParam, LPARAM lParam)
{
	// 고정 셀 크기 모드에서는 가로 스크롤 무시
	bool auto_mode = (m_cell_size.cx <= 0 && m_cell_size.cy <= 0);
	if (!auto_mode)
		return 0;

	short zDelta = GET_WHEEL_DELTA_WPARAM(wParam);
	float step = 40.0f;
	m_scroll.x += (float)(zDelta) / WHEEL_DELTA * step;
	clamp_scroll();
	Invalidate();
	return 0;
}

//------------------------------------------------------------
// 드래그 스크롤 : 클릭 → 드래그 → 릴리즈
//------------------------------------------------------------
void CSCHeatmapCtrl::OnLButtonDown(UINT nFlags, CPoint point)
{
	m_bDragging = true;
	m_drag_start_pt = point;
	m_drag_start_scroll = m_scroll;
	SetCapture();

	// 드래그 시작 시 툴팁 숨김
	m_tooltip.Activate(FALSE);

	CDialogEx::OnLButtonDown(nFlags, point);
}

void CSCHeatmapCtrl::OnLButtonUp(UINT nFlags, CPoint point)
{
	if (m_bDragging)
	{
		m_bDragging = false;
		ReleaseCapture();
	}

	CDialogEx::OnLButtonUp(nFlags, point);
}

BOOL CSCHeatmapCtrl::OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message)
{
	if (m_bDragging)
	{
		::SetCursor(::LoadCursor(NULL, IDC_SIZEALL));
		return TRUE;
	}
	return CDialogEx::OnSetCursor(pWnd, nHitTest, message);
}

void CSCHeatmapCtrl::OnMouseMove(UINT nFlags, CPoint point)
{
	// WM_MOUSEWHEEL 수신을 위해 포커스 설정
	if (GetFocus() != this)
		SetFocus();

	// 마우스 추적 등록 (WM_MOUSELEAVE 수신용)
	if (!m_bMouseTracking)
	{
		TRACKMOUSEEVENT tme = {};
		tme.cbSize = sizeof(tme);
		tme.dwFlags = TME_LEAVE;
		tme.hwndTrack = m_hWnd;
		::TrackMouseEvent(&tme);
		m_bMouseTracking = true;
	}

	//--- 드래그 중이면 스크롤 업데이트 ---
	if (m_bDragging)
	{
		bool auto_mode = (m_cell_size.cx <= 0 && m_cell_size.cy <= 0);
		// 자동모드에서만 가로 드래그 허용
		if (auto_mode)
			m_scroll.x = m_drag_start_scroll.x - (float)(point.x - m_drag_start_pt.x);

		m_scroll.y = m_drag_start_scroll.y - (float)(point.y - m_drag_start_pt.y);
		clamp_scroll();
		Invalidate();
		return;
	}

	//--- 툴팁 처리 ---
	CPoint cell = hit_test_cell(point);

	if (cell != m_hover_cell)
	{
		m_hover_cell = cell;

		if (cell.x >= 0 && cell.y >= 0)
		{
			int cols = calc_col_count();
			int index = cell.y * cols + cell.x;
			CString tip;
			auto it = m_cell.find(index);
			if (it != m_cell.end() && !it->second.tooltip.IsEmpty())
				tip.Format(_T("%d (%d, %d) %s"), index, cell.x, cell.y, (LPCTSTR)it->second.tooltip);
			else
				tip.Format(_T("%d (%d, %d)"), index, cell.x, cell.y);

			m_tooltip.UpdateTipText(tip, this, 1);
			m_tooltip.Activate(TRUE);
		}
		else
		{
			m_tooltip.Activate(FALSE);
		}
	}

	CDialogEx::OnMouseMove(nFlags, point);
}

void CSCHeatmapCtrl::OnMouseLeave()
{
	m_bMouseTracking = false;
	m_hover_cell = { -1, -1 };
	m_tooltip.Activate(FALSE);

	if (m_bDragging)
	{
		m_bDragging = false;
		ReleaseCapture();
	}

	CDialogEx::OnMouseLeave();
}

//------------------------------------------------------------
// 히트맵 그리기 (줌/스크롤 반영)
//------------------------------------------------------------
void CSCHeatmapCtrl::draw_heatmap()
{
	ID2D1DeviceContext* d2dc = m_d2dc.get_d2dc();
	if (d2dc == nullptr)
		return;

	int cols = calc_col_count();
	int rows = (m_total > 0 && cols > 0) ? (m_total + cols - 1) / cols : 0;
	if (cols <= 0 || rows <= 0)
		return;

	D2D1_SIZE_F sz_dc = m_d2dc.get_size();

	float margin_x = (float)m_cell_gap.cx;
	float margin_y = (float)m_cell_gap.cy;
	float area_w = sz_dc.width - margin_x * 2.0f;
	float area_h = sz_dc.height - margin_y * 2.0f;
	if (area_w < 1.0f || area_h < 1.0f)
		return;

	float gap_x = (float)m_cell_gap.cx;
	float gap_y = (float)m_cell_gap.cy;

	//--- 셀 크기 결정 ---
	float cell_w, cell_h;
	get_effective_cell_size(cell_w, cell_h);

	float radius = min(cell_w, cell_h) * 0.2f;
	if (radius < 1.0f) radius = 1.0f;

	ComPtr<ID2D1SolidColorBrush> brush;
	HRESULT hr = d2dc->CreateSolidColorBrush(m_cr_cell, &brush);
	if (FAILED(hr))
		return;

	float stride_x = cell_w + gap_x;
	float stride_y = cell_h + gap_y;

	//--- 화면에 보이는 셀 범위만 계산하여 그린다 ---
	int ix_start = max(0L, (long)floor((m_scroll.x - margin_x) / stride_x));
	int ix_end = min((long)cols, (long)ceil((m_scroll.x + sz_dc.width - margin_x) / stride_x) + 1);
	int iy_start = max(0L, (long)floor((m_scroll.y - margin_y) / stride_y));
	int iy_end = min((long)rows, (long)ceil((m_scroll.y + sz_dc.height - margin_y) / stride_y) + 1);

	for (int iy = iy_start; iy < iy_end; iy++)
	{
		for (int ix = ix_start; ix < ix_end; ix++)
		{
			// 마지막 행 부분 채움 처리: 선형 인덱스가 m_total 이상이면 건너뜀
			int index = iy * cols + ix;
			if (index >= m_total)
				continue;

			float left = margin_x + ix * stride_x - m_scroll.x;
			float top = margin_y + iy * stride_y - m_scroll.y;
			float right = left + cell_w;
			float bottom = top + cell_h;

			D2D1_ROUNDED_RECT rr = D2D1::RoundedRect(
				D2D1::RectF(left, top, right, bottom),
				radius, radius);

			// 채우기 색상
			auto it = m_cell.find(index);
			if (it != m_cell.end())
				brush->SetColor(it->second.cr_fill);
			else
				brush->SetColor(m_cr_cell);

			d2dc->FillRoundedRectangle(rr, brush.Get());

			// 테두리 색상: 개별 셀에 지정되어 있으면(alpha>0) 그것을 사용, 아니면 전체 기본값 사용
			D2D1_COLOR_F border_cr = m_cr_border;
			if (it != m_cell.end() && it->second.cr_border.a > 0.0f)
				border_cr = it->second.cr_border;

			if (border_cr.a > 0.0f)
			{
				brush->SetColor(border_cr);
				d2dc->DrawRoundedRectangle(rr, brush.Get(), 1.0f);
			}

			m_d2dc.draw_text(m_WriteFactory, m_WriteFormat, it->second.text, D2D1::RectF(left, top, right, bottom), m_brush);
			//d2dc->DrawText(it->second.text, it->second.text.GetLength(), m_WriteFormat, D2D1::RectF(left, top, right, bottom), m_brush);
		}
	}
}