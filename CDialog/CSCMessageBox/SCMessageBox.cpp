// SCMessageBox.cpp: 구현 파일
//

#include "SCMessageBox.h"
#include "../../MemoryDC.h"

// CSCMessageBox 대화 상자

IMPLEMENT_DYNAMIC(CSCMessageBox, CDialogEx)

CSCMessageBox::CSCMessageBox(CWnd* parent, int cx, int cy)
{
	if (!parent)
		return;

	memset(&m_lf, 0, sizeof(LOGFONT));
	create(parent, cx, cy);
}

CSCMessageBox::~CSCMessageBox()
{
	m_button_ok.DestroyWindow();
	m_button_cancel.DestroyWindow();
	m_button_retry.DestroyWindow();
	m_static_message.DestroyWindow();

	DestroyWindow();
}

void CSCMessageBox::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}


BEGIN_MESSAGE_MAP(CSCMessageBox, CDialogEx)
	ON_BN_CLICKED(IDOK, &CSCMessageBox::OnBnClickedOk)
	ON_BN_CLICKED(IDCANCEL, &CSCMessageBox::OnBnClickedCancel)
	ON_REGISTERED_MESSAGE(Message_CGdiButton, &CSCMessageBox::on_message_CGdiButton)
	ON_WM_PAINT()
END_MESSAGE_MAP()


// CSCMessageBox 메시지 처리기
bool CSCMessageBox::create(CWnd* parent, int cx, int cy)
{
	m_parent = parent;

	memset(&m_lf, 0, sizeof(LOGFONT));


	DWORD dwStyle = WS_POPUP;

	WNDCLASS wc = {};
	::GetClassInfo(AfxGetInstanceHandle(), _T("#32770"), &wc);
	wc.lpszClassName = _T("CSCMessageDlg");
	AfxRegisterClass(&wc);

	bool res = CreateEx(NULL, wc.lpszClassName, _T("CSCMessageDlg"), dwStyle, CRect(0, 0, cx, cy), parent, 0);
	if (!res)
		return false;

	dwStyle = GetWindowLongPtr(m_hWnd, GWL_STYLE);
	dwStyle &= ~(WS_CAPTION);
	SetWindowLongPtr(m_hWnd, GWL_STYLE, dwStyle);

	CRect rc;
	GetClientRect(rc);

	CFont* font = GetFont();

	if (font == NULL)
		font = parent->GetFont();

	if (font != NULL)
		font->GetObject(sizeof(m_lf), &m_lf);
	else
		GetObject(GetStockObject(SYSTEM_FONT), sizeof(m_lf), &m_lf);

	m_button_ok.create(_T("Ok"), WS_CHILD, CRect(0, 0, m_sz_button.cx, m_sz_button.cy), this, 0);
	m_button_cancel.create(_T("Cancel"), WS_CHILD, CRect(0, 0, m_sz_button.cx, m_sz_button.cy), this, 0);
	m_button_retry.create(_T("Retry"), WS_CHILD, CRect(0, 0, m_sz_button.cx, m_sz_button.cy), this, 0);
	m_static_message.create(_T("message"), WS_CHILD, CRect(0, m_title_height, cx, rc.bottom - 20 - 8 - m_sz_button.cy), this, 0);

	reconstruct_font();

	return res;
}

void CSCMessageBox::reconstruct_font()
{
	m_font.DeleteObject();
	BOOL bCreated = m_font.CreateFontIndirect(&m_lf);

	SetFont(&m_font, true);

	m_button_ok.set_font(&m_font);
	m_button_cancel.set_font(&m_font);
	m_button_retry.set_font(&m_font);
	m_static_message.set_font(&m_font);

	ASSERT(bCreated);
}

void CSCMessageBox::set_title(CString title)
{
	m_title = title;
}

void CSCMessageBox::set_message(CString msg, int type, int timeout, int align)
{
	m_parent->EnableWindow(FALSE);

	m_message = msg;
	m_type = type;
	m_timeout = timeout;
	m_align = align;

	//메시지, 버튼 설정에 따라 재조정한다.
	//CClientDC dc(this);
	//dc.SelectObject(&m_font);
	//dc.DrawText(msg, )

	CRect rc;
	GetClientRect(rc);

	int bottom_gap = 8;	//rc.bottom과 button 사이의 간격
	int button_gap = 8;	//두 버튼 사이의 가로 간격

	rc.top += m_title_height;

	m_button_cancel.ShowWindow(type == MB_OKCANCEL ? SW_SHOW : SW_HIDE);

	if (type = MB_OKCANCEL)
	{
		int x = rc.CenterPoint().x + button_gap / 2;
		m_button_cancel.MoveWindow(x, rc.bottom - bottom_gap - m_sz_button.cy, m_sz_button.cx, m_sz_button.cy);

		x = rc.CenterPoint().x - button_gap / 2 - m_sz_button.cx;
		m_button_ok.MoveWindow(x, rc.bottom - bottom_gap - m_sz_button.cy, m_sz_button.cx, m_sz_button.cy);
	}
	else
	{
		int x = rc.CenterPoint().x - button_gap / 2;
		m_button_cancel.MoveWindow(x, rc.bottom - bottom_gap - m_sz_button.cy, m_sz_button.cx, m_sz_button.cy);
	}

	m_static_message.MoveWindow(rc.left + 8, rc.top, rc.Width() - 8 * 2, rc.Height() - bottom_gap - m_sz_button.cy - 8);
	

	m_static_message.SetWindowText(m_message);
	m_static_message.ShowWindow(SW_SHOW);
	
	m_button_ok.ShowWindow(SW_SHOW);
	m_button_cancel.ShowWindow(SW_SHOW);
}

LRESULT CSCMessageBox::on_message_CGdiButton(WPARAM wParam, LPARAM lParam)
{
	auto msg = (CGdiButtonMessage*)wParam;
	if (msg->message == WM_LBUTTONUP)
	{
		if (msg->pWnd == &m_button_ok)
			//m_response = IDOK;
			CDialogEx::OnOK();
		else if (msg->pWnd == &m_button_cancel)
			//m_response = IDCANCEL;
			CDialogEx::OnCancel();
		else if (msg->pWnd == &m_button_retry)
			m_response = IDRETRY;
	}

	return 0;
}

BOOL CSCMessageBox::PreTranslateMessage(MSG* pMsg)
{
	// TODO: 여기에 특수화된 코드를 추가 및/또는 기본 클래스를 호출합니다.

	return CDialogEx::PreTranslateMessage(pMsg);
}

void CSCMessageBox::OnBnClickedOk()
{

}

void CSCMessageBox::OnBnClickedCancel()
{
	CDialogEx::OnCancel();
}

INT_PTR CSCMessageBox::DoModal()
{
	// TODO: 여기에 특수화된 코드를 추가 및/또는 기본 클래스를 호출합니다.
	//return CDialogEx::DoModal();

	ShowWindow(SW_SHOW);

	return m_response;
}

void CSCMessageBox::OnPaint()
{
	CPaintDC dc1(this); // device context for painting
	// TODO: 여기에 메시지 처리기 코드를 추가합니다.
	// 그리기 메시지에 대해서는 CDialogEx::OnPaint()을(를) 호출하지 마십시오.
}
