// SCProgressDlg.cpp: 구현 파일
//

#include "SCProgressDlg.h"
#include "../../Functions.h"

// CSCProgressDlg 대화 상자

IMPLEMENT_DYNAMIC(CSCProgressDlg, CDialogEx)

CSCProgressDlg::CSCProgressDlg()
{
	memset(&m_lf, 0, sizeof(LOGFONT));

	m_cr_back.SetFromCOLORREF(::GetSysColor(COLOR_3DFACE));
}

CSCProgressDlg::~CSCProgressDlg()
{
}

void CSCProgressDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}


BEGIN_MESSAGE_MAP(CSCProgressDlg, CDialogEx)
	ON_WM_PAINT()
	ON_WM_ERASEBKGND()
	ON_WM_TIMER()
	ON_WM_LBUTTONDOWN()
END_MESSAGE_MAP()


// CSCProgressDlg 메시지 처리기
bool CSCProgressDlg::create(CWnd* parent, CString text, int left, int top, int right, int bottom)
{
	m_parent = parent;

	DWORD dwStyle = WS_POPUP | WS_BORDER;

	WNDCLASS wc = {};
	::GetClassInfo(AfxGetInstanceHandle(), _T("#32770"), &wc);
	wc.lpszClassName = _T("CSCProgressDlg");
	AfxRegisterClass(&wc);

	bool res = CreateEx(WS_EX_WINDOWEDGE | WS_EX_TOPMOST, wc.lpszClassName, _T("CSCProgressDlg"), dwStyle, CRect(left, top, right, bottom), parent, 0);
	//bool res = CreateEx(WS_EX_STATICEDGE | WS_EX_TOPMOST, _T("listbox"), _T("listbox"), dwStyle, CRect(0, 0, 320, 300), parent, 0);

	MoveWindow(left, top, right - left, bottom - top);
	CRect rc;
	GetClientRect(rc);

	if (res)
	{
		CFont* font = GetFont();

		if (font == NULL)
			font = parent->GetFont();

		if (font != NULL)
			font->GetObject(sizeof(m_lf), &m_lf);
		else
			GetObject(GetStockObject(SYSTEM_FONT), sizeof(m_lf), &m_lf);

		int progress_height = 14;
		int bottom_pos = rc.bottom - (double)(rc.bottom - rc.top) * 0.16 - progress_height;
		BOOL bOk;
		
		m_progress.Create(WS_CHILD | WS_VISIBLE, CRect(rc.left + 20, bottom_pos, rc.right - 20, bottom_pos + progress_height), this, 0);
		m_progress.ShowWindow(SW_SHOW);

		bOk = m_static.create(text, WS_CHILD | WS_VISIBLE | SS_CENTER | SS_CENTERIMAGE, CRect(rc.left + 20, rc.top + 8, rc.right - 20, bottom_pos - 8), this, 0);

		m_static.set_transparent();

		reconstruct_font();
	}

	return res;
}

void CSCProgressDlg::reconstruct_font()
{
	m_font.DeleteObject();
	BOOL bCreated = m_font.CreateFontIndirect(&m_lf);

	SetFont(&m_font, true);
	//m_progress->SetFont(&m_font, true);
	m_static.set_font(&m_font);

	ASSERT(bCreated);
}


void CSCProgressDlg::OnPaint()
{
	CPaintDC dc(this); // device context for painting
					   // TODO: 여기에 메시지 처리기 코드를 추가합니다.
					   // 그리기 메시지에 대해서는 CDialogEx::OnPaint()을(를) 호출하지 마십시오.
	CRect rc;

	GetClientRect(rc);
	dc.FillSolidRect(rc, m_cr_back.ToCOLORREF());
}


BOOL CSCProgressDlg::OnEraseBkgnd(CDC* pDC)
{
	// TODO: 여기에 메시지 처리기 코드를 추가 및/또는 기본값을 호출합니다.
	return FALSE;
	return CDialogEx::OnEraseBkgnd(pDC);
}

void CSCProgressDlg::set_text(CString text)
{
	m_static.set_text(text);
}

void CSCProgressDlg::set_text_color(Gdiplus::Color cr)
{
	m_static.set_text_color(cr);
	//Invalidate();
}

void CSCProgressDlg::set_back_color(Gdiplus::Color cr)
{
	m_cr_back = cr;
	m_progress.set_back_color(m_cr_back.ToCOLORREF());
	m_static.set_back_color(m_cr_back);
	Invalidate();
}

void CSCProgressDlg::set_font_size(int size)
{
	m_static.set_font_size(size);
}

void CSCProgressDlg::set_range(int32_t lower, int32_t upper)
{
	m_progress.SetIndeterminate(false);
	m_progress.SetRange32(lower, upper);
}

void CSCProgressDlg::get_range(int32_t& lower, int32_t upper)
{
	m_progress.GetRange(lower, upper);
}

int32_t CSCProgressDlg::get_lower()
{
	if (m_progress.GetIndeterminate())
		return 0;

	int32_t lower, upper;
	m_progress.GetRange(lower, upper);
	return lower;
}

int32_t CSCProgressDlg::get_upper()
{
	if (m_progress.GetIndeterminate())
		return 0;

	int32_t lower, upper;
	m_progress.GetRange(lower, upper);
	return upper;
}


void CSCProgressDlg::set_indeterminate(bool indeterminate)
{
	m_progress.SetIndeterminate(indeterminate);
}



//step 단위 증감
int32_t CSCProgressDlg::step(int step)
{
	int pos = m_progress.GetPos();
	m_progress.SetPos(pos + step);
	return m_progress.GetPos();
}

void CSCProgressDlg::timeout(int ms)
{
	ShowWindow(SW_SHOW);
	SetTimer(timer_auto_hide, ms, NULL);
}

//특정 실행파일이 실행되면 자동 종료
void CSCProgressDlg::auto_hide_when_app_found(CString exe_name, int max_timeout_ms, int intentional_delay_before_hide_ms)
{
	ShowWindow(SW_SHOW);

	int total_wait_time = 0;

	while (true)
	{
		if (get_process_running_count(exe_name) > 0)
		{
			Wait(intentional_delay_before_hide_ms);
			break;
		}

		Wait(100);
		total_wait_time += 100;

		if (total_wait_time > max_timeout_ms)
		{
			break;
		}
	}

	ShowWindow(SW_HIDE);
}

void CSCProgressDlg::OnTimer(UINT_PTR nIDEvent)
{
	// TODO: 여기에 메시지 처리기 코드를 추가 및/또는 기본값을 호출합니다.
	if (nIDEvent == timer_auto_hide)
	{
		KillTimer(timer_auto_hide);
		ShowWindow(SW_HIDE);
	}

	CDialogEx::OnTimer(nIDEvent);
}

void CSCProgressDlg::OnLButtonDown(UINT nFlags, CPoint point)
{
	// TODO: 여기에 메시지 처리기 코드를 추가 및/또는 기본값을 호출합니다.
	if (m_enable_move)
		DefWindowProc(WM_NCLBUTTONDOWN, HTCAPTION, MAKEWORD(point.x, point.y));

	CDialogEx::OnLButtonDown(nFlags, point);
}
