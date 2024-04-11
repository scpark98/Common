// SCProgressDlg.cpp: 구현 파일
//

#include "SCProgressDlg.h"
#include "../../Functions.h"

// CSCProgressDlg 대화 상자

IMPLEMENT_DYNAMIC(CSCProgressDlg, CDialogEx)

CSCProgressDlg::CSCProgressDlg()
{
	memset(&m_lf, 0, sizeof(LOGFONT));
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
END_MESSAGE_MAP()


// CSCProgressDlg 메시지 처리기
bool CSCProgressDlg::create(CWnd* parent, CString text, int left, int top, int right, int bottom)
{
	m_parent = parent;

	//DWORD dwStyle = WS_POPUP;
	DWORD dwStyle = WS_POPUP | WS_BORDER;
	//dwStyle = (dwStyle & ~WS_CAPTION);
	//id를 1234로 주면 왜 생성이 안될까...
	//className과 windowName을 CSCMenu 등 다른걸로 주면 생성이 실패하는 이유는??
	bool res = CreateEx(WS_EX_DLGMODALFRAME | WS_EX_TOPMOST, _T("listbox"), _T("listbox"), dwStyle, CRect(left, top, right, bottom), parent, 0);
	//bool res = CreateEx(WS_EX_STATICEDGE | WS_EX_TOPMOST, _T("listbox"), _T("listbox"), dwStyle, CRect(0, 0, 320, 300), parent, 0);

	if (res)
	{
		CFont* font = GetFont();

		if (font == NULL)
			font = parent->GetFont();

		if (font != NULL)
			font->GetObject(sizeof(m_lf), &m_lf);
		else
			GetObject(GetStockObject(SYSTEM_FONT), sizeof(m_lf), &m_lf);

		int height = 14;
		int bottom_pos = bottom - (double)(bottom - top) * 0.16 - height;
		BOOL bOk;
		
		//m_progress = new CMFCRibbonProgressBar(2734, 200);
		m_progress.Create(WS_CHILD | WS_VISIBLE | PBS_MARQUEE, CRect(left + 20, bottom_pos, right - 20, bottom_pos + height), this, 0);
		m_progress.ShowWindow(SW_SHOW);
		//m_progress.SetMarquee(TRUE, 10);
		//m_progress.SetIndeterminate();
		//m_progress.SetRange(0, 100);
		//m_progress.SetPos(50);
		//m_progress.setinfi
		//m_progress->ShowWindow(SW_SHOW);

		//m_progress->SetInfiniteMode(TRUE);
		//m_progress->SetRange(0, 200);
		//m_progress->SetPos(200, true);
		//m_progress->SetVisible(TRUE);

		bOk = m_static.Create(text, WS_CHILD | WS_VISIBLE | SS_CENTER | SS_CENTERIMAGE, CRect(left + 20, top + 8, right - 20, bottom_pos - 8), this, 0);
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
	dc.FillSolidRect(rc, m_cr_back);
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

void CSCProgressDlg::set_back_color(COLORREF cr)
{
	m_cr_back = cr;
	Invalidate();
}

void CSCProgressDlg::set_range(int32_t lower, int32_t upper)
{
	m_progress.SetIndeterminate(false);
	m_progress.SetRange32(lower, upper);
}

void CSCProgressDlg::set_indeterminate(bool indeterminate)
{
	m_progress.SetIndeterminate(indeterminate);
}

