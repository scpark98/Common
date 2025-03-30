// SCMessageBox.cpp: 구현 파일
//

#include "SCMessageBox.h"
#include "../../Functions.h"
#include "../../MemoryDC.h"

// CSCMessageBox 대화 상자

IMPLEMENT_DYNAMIC(CSCMessageBox, CDialogEx)

CSCMessageBox::CSCMessageBox(CWnd* parent, CString title, UINT icon_id, int cx, int cy)
{
	if (!parent)
		return;

	memset(&m_lf, 0, sizeof(LOGFONT));
	create(parent, title, icon_id, cx, cy);
}

CSCMessageBox::~CSCMessageBox()
{
	m_button_ok.DestroyWindow();
	m_button_cancel.DestroyWindow();
	m_button_retry.DestroyWindow();
	m_button_quit.DestroyWindow();
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
	ON_WM_LBUTTONDOWN()
END_MESSAGE_MAP()


// CSCMessageBox 메시지 처리기
bool CSCMessageBox::create(CWnd* parent, CString title, UINT icon_id, int cx, int cy)
{
	m_parent = parent;
	m_title = title;
	m_hIcon = load_icon(NULL, icon_id, 20, 20);

	memset(&m_lf, 0, sizeof(LOGFONT));


	LONG_PTR dwStyle = WS_POPUP;

	WNDCLASS wc = {};
	::GetClassInfo(AfxGetInstanceHandle(), _T("#32770"), &wc);
	wc.lpszClassName = _T("CSCMessageDlg");
	AfxRegisterClass(&wc);

	bool res = CreateEx(NULL, wc.lpszClassName, _T("CSCMessageDlg"), (DWORD)dwStyle, CRect(0, 0, cx, cy), parent, 0);
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
	m_button_ok.use_hover();

	m_button_cancel.create(_T("Cancel"), WS_CHILD, CRect(0, 0, m_sz_button.cx, m_sz_button.cy), this, 0);
	m_button_cancel.use_hover();

	m_button_retry.create(_T("Retry"), WS_CHILD, CRect(0, 0, m_sz_button.cx, m_sz_button.cy), this, 0);
	m_button_quit.create(_T("X"), WS_CHILD | WS_VISIBLE | BS_FLAT, CRect(rc.right - 1 - m_title_height, rc.top + 1, rc.right - 1, m_title_height - 1), this, 0);
	m_static_message.create(_T("message"), WS_CHILD | WS_VISIBLE, CRect(0, m_title_height, cx, rc.bottom - 20 - 8 - m_sz_button.cy), this, 0);

	m_button_quit.set_text_color(m_theme.cr_title_text);
	m_button_quit.set_back_color(m_theme.cr_title_back);
	m_button_quit.use_hover();

	m_static_message.set_back_color(m_theme.cr_back);

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

	int bottom_gap = 16;	//rc.bottom과 button 사이의 간격
	int button_gap = 8;		//두 버튼 사이의 가로 간격
	int gap = 8;	//타이틀바 <-> 메시지박스 영역 <-> 버튼 여백, 메시지박스의 좌우 여백

	rc.top += m_title_height;

	if (type == MB_OKCANCEL)
	{
		int x = rc.CenterPoint().x + button_gap / 2;
		m_button_cancel.MoveWindow(x, rc.bottom - bottom_gap - m_sz_button.cy, m_sz_button.cx, m_sz_button.cy);

		x = rc.CenterPoint().x - button_gap / 2 - m_sz_button.cx;
		m_button_ok.MoveWindow(x, rc.bottom - bottom_gap - m_sz_button.cy, m_sz_button.cx, m_sz_button.cy);
	}
	else //if (type == MB_OK)
	{
		int x = rc.CenterPoint().x - m_sz_button.cx / 2;
		m_button_ok.MoveWindow(x, rc.bottom - bottom_gap - m_sz_button.cy, m_sz_button.cx, m_sz_button.cy);
	}

	m_static_message.MoveWindow(rc.left + gap, rc.top + gap, rc.Width() - gap * 2, rc.Height() - bottom_gap - m_sz_button.cy - gap * 2);
	m_static_message.SetWindowText(m_message);
	m_static_message.ModifyStyle(0, m_align);
	m_static_message.ShowWindow(SW_SHOW);
	
	m_button_ok.ShowWindow(SW_SHOW);
	m_button_cancel.ShowWindow(type == MB_OKCANCEL ? SW_SHOW : SW_HIDE);
}

void CSCMessageBox::set_color_theme(int theme)
{
	m_theme.set_color_theme(theme);
	m_theme.cr_back = Gdiplus::Color::White;

	m_static_message.set_back_color(m_theme.cr_back);
}

LRESULT CSCMessageBox::on_message_CGdiButton(WPARAM wParam, LPARAM lParam)
{
	auto msg = (CGdiButtonMessage*)wParam;
	if (msg->msg == WM_LBUTTONUP)
	{
		if (msg->pWnd == &m_button_ok)
			m_response = IDOK;
			//CDialogEx::OnOK();
		else if (msg->pWnd == &m_button_cancel || msg->pWnd == &m_button_quit)
			m_response = IDCANCEL;
			//CDialogEx::OnCancel();
		else if (msg->pWnd == &m_button_retry)
			m_response = IDRETRY;
	}

	return 0;
}

BOOL CSCMessageBox::PreTranslateMessage(MSG* pMsg)
{
	// TODO: 여기에 특수화된 코드를 추가 및/또는 기본 클래스를 호출합니다.
	if (pMsg->message == WM_KEYDOWN)
	{
		switch (pMsg->wParam)
		{
			case VK_ESCAPE :
				OnBnClickedCancel();
				break;
		}
	}

	return CDialogEx::PreTranslateMessage(pMsg);
}

void CSCMessageBox::OnBnClickedOk()
{
	m_response = IDOK;
}

void CSCMessageBox::OnBnClickedCancel()
{
	//CDialogEx::OnCancel();
	m_response = IDCANCEL;
}

//DoModal()을 사용하지 않고도 구현가능하지만
//ret = dlg.DoModal() 과 같이 호출하는 것이 일반적인 방법이므로 이 방식을 지원하기 위해
//DoModal()을 override 함.
//set_message()로 메시지 설정을 한 후 DoModal()을 호출해서 띠워도 되지만
//DoModal(_T("message")) 과 같이 직접 줄 수 있도록 함.
INT_PTR CSCMessageBox::DoModal(CString msg, int type, int timeout, int align)
{
	// TODO: 여기에 특수화된 코드를 추가 및/또는 기본 클래스를 호출합니다.
	//return CDialogEx::DoModal();

	//
	if (!msg.IsEmpty())
		set_message(msg, type, timeout, align);

	CenterWindow(m_parent);
	ShowWindow(SW_SHOW);

	MSG		stmsg;

	m_response = -1;

	//for Modal Dialog
	m_parent->EnableWindow(FALSE);

	while (m_response < 0)
	{
		while (PeekMessage(&stmsg, NULL, 0, 0, PM_REMOVE))
		{
			TRACE("tick = %d\n", GetTickCount());
			if (stmsg.message == WM_KEYDOWN)
			{
				PreTranslateMessage(&stmsg);
			}
			else
			{
				TranslateMessage(&stmsg);
				DispatchMessage(&stmsg);
			}
		}
	}

	m_parent->EnableWindow(TRUE);

	if (m_response == IDOK)
		CDialog::OnOK();
	else if (m_response == IDCANCEL)
		CDialog::OnCancel();

	return m_response;
}

void CSCMessageBox::OnPaint()
{
	CPaintDC dc1(this); // device context for painting
	// TODO: 여기에 메시지 처리기 코드를 추가합니다.
	// 그리기 메시지에 대해서는 CDialogEx::OnPaint()을(를) 호출하지 마십시오.
	CRect rc, rtitle;
	GetClientRect(rc);

	CMemoryDC dc(&dc1, &rc);
	Gdiplus::Graphics g(dc.m_hDC);
	
	//draw titlebar
	rtitle = rc;
	rtitle.bottom = rtitle.top + m_title_height;
	dc.FillSolidRect(rtitle, m_theme.cr_title_back.ToCOLORREF());

	rtitle.left += 8;

	//draw icon
	if (m_hIcon)
	{
		CRect ricon = rtitle;
		ricon.right = ricon.left + 16;
		rtitle.left = ricon.right + 8;
		draw_icon(&dc, m_hIcon, ricon);
	}
	
	//draw title text
	dc.SetTextColor(m_theme.cr_title_text.ToCOLORREF());
	dc.DrawText(m_title, rtitle, DT_LEFT | DT_SINGLELINE | DT_VCENTER);

	draw_rectangle(g, rc, Gdiplus::Color::DimGray);
}

void CSCMessageBox::OnLButtonDown(UINT nFlags, CPoint point)
{
	// TODO: Add your message handler code here and/or call default
	if (point.y < m_title_height)
		DefWindowProc(WM_NCLBUTTONDOWN, HTCAPTION, MAKEWORD(point.x, point.y));

	CDialogEx::OnLButtonDown(nFlags, point);
}
