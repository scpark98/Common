// SCMessageBox.cpp: 구현 파일
//

#include "SCMessageBox.h"
#include "../../Functions.h"
#include "../../MemoryDC.h"

#define MIN_SIZE_CX	320
#define MAX_SIZE_CX	800
#define MIN_SIZE_CY	160

// CSCMessageBox 대화 상자

IMPLEMENT_DYNAMIC(CSCMessageBox, CDialogEx)

CSCMessageBox::CSCMessageBox(CWnd* parent, CString title, UINT icon_id, bool as_modal, int cx, int cy)
{
	if (!parent)
		return;

	m_as_modal = as_modal;
	memset(&m_lf, 0, sizeof(LOGFONT));

	create(parent, title, icon_id, cx, cy);
}

CSCMessageBox::~CSCMessageBox()
{
	for (int i = 0; i < TOTAL_BUTTON_COUNT; i++)
	{
		m_button[i].DestroyWindow();
	}

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
	ON_WM_TIMER()
END_MESSAGE_MAP()


// CSCMessageBox 메시지 처리기
bool CSCMessageBox::create(CWnd* parent, CString title, UINT icon_id, int cx, int cy)
{
	m_as_modal = false;
	m_parent = parent;
	m_title = title;
	m_hIcon = load_icon(NULL, icon_id, 16, 16);

	m_button_caption[IDOK] = _T("확인");
	m_button_caption[IDCANCEL] = _T("취소");
	m_button_caption[IDABORT] = _T("중지");
	m_button_caption[IDRETRY] = _T("재시도");
	m_button_caption[IDIGNORE] = _T("무시");
	m_button_caption[IDYES] = _T("예");
	m_button_caption[IDNO] = _T("아니오");
	m_button_caption[IDCLOSE] = _T("닫기");
	m_button_caption[IDHELP] = _T("도움말");
	m_button_caption[IDTRYAGAIN] = _T("다시 시도");
	m_button_caption[IDCONTINUE] = _T("계속");

	if (cx < 0 || cy < 0)
	{
		m_auto_size = true;
		cx = DEFAULT_SIZE_CX;
		cy = DEFAULT_SIZE_CY;
	}

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

	for (int i = 0; i < TOTAL_BUTTON_COUNT; i++)
	{
		m_button[i].create(m_button_caption[i], WS_CHILD | WS_TABSTOP, CRect(0, 0, m_sz_button.cx, m_sz_button.cy), this, SC_BUTTON_ID + i);
		m_button[i].use_hover();
	}

	//종료 버튼은 IDCLOSE가 아닌 IDCANCEL로 처리해야 한다.
	m_button_quit.create(_T(""), WS_CHILD | WS_VISIBLE | BS_FLAT,
						CRect(rc.right - 2 - m_title_height, rc.top + 2, rc.right - 2, m_title_height - 1), this, SC_BUTTON_ID + IDCANCEL);
	m_button_quit.set_button_cmd(SC_CLOSE);
	m_button_quit.set_text_color(m_theme.cr_title_text);
	m_button_quit.set_back_color(m_theme.cr_title_back);
	m_button_quit.set_hover_back_color(gRGB(232, 17, 35));
	m_button_quit.use_hover();

	m_static_message.create(_T("message"), WS_CHILD | WS_VISIBLE, CRect(0, m_title_height, cx, rc.bottom - 20 - 8 - m_sz_button.cy), this, 0);
	m_static_message.set_back_color(m_theme.cr_back);

	reconstruct_font();

	return res;
}

void CSCMessageBox::reconstruct_font()
{
	m_font.DeleteObject();
	BOOL bCreated = m_font.CreateFontIndirect(&m_lf);

	SetFont(&m_font, true);

	for (int i = 0; i < 12; i++)
	{
		m_button[i].set_font(&m_font);
	}

	ASSERT(bCreated);
}

void CSCMessageBox::set_title(CString title)
{
	m_title = title;
}

void CSCMessageBox::set_message(CString msg, int type, int timeout_sec, DWORD align)
{
	m_message = msg;
	m_type = type;
	m_timeout_sec = timeout_sec;
	m_align = align;

	CRect rc;
	GetClientRect(rc);

	int bottom_gap = 16;	//rc.bottom과 button 사이의 간격
	int button_gap = 8;		//두 버튼 사이의 가로 간격
	int gap = 8;			//타이틀바 <-> 메시지박스 영역 <-> 버튼 여백
	int gap_side = 16;		//메시지박스의 좌우 여백

	rc.top += m_title_height;

	//메시지 너비, 높이에 따라 대화상자 크기를 재조정한다.
	//최소 cx = 240, cy = 140이며 cx의 최대 크기는 800으로 제한한다.
	//이 값을 넘으면 DT_WORDBREAK를 넣어 다시 계산하려 했으나 많이 복잡해진다. 우선 MAX_WIDTH로 제한한다.
	//실제 사용 시 MAX_WIDTH를 넘을 경우는 좌우가 잘리므로 적절하게 '\n'을 넣어준다.
	if (m_auto_size)
	{
		CRect rmsg(rc.left + gap_side, rc.top + gap, rc.Width() - gap_side * 2, rc.Height() - bottom_gap - m_sz_button.cy - gap * 2);
		//rmsg = m_static_message.set_text(msg);
		CClientDC dc(this);
		CFont* pOldFont = dc.SelectObject(&m_font);

		//DWORD dwText = m_static_message.get_text_align();
		dc.DrawText(msg, rmsg, DT_CALCRECT);// | DT_WORDBREAK);

		//TRACE(_T("rmsg = %s\n"), get_rect_info_string(rmsg));

		//right, bottom을 줄 때는 dlg의 최소 크기를 고려한다.
		rc.right = MAX(rmsg.Width() + gap_side * 2, MIN_SIZE_CX);// m_sz_button.cx * 2 + gap + button_gap * 2 + 40);

		//만약 최대 너비를 넘어간다면 DT_WORDBREAK를 주고 다시 계산해서 height를 늘리려했으나 많이 복잡해진다.
		//그냥 메시지박스를 띠울 때 width가 max를
		if (rc.right > MAX_SIZE_CX)
		{
			rc.right = MAX_SIZE_CX;
			//CRect rtext = m_static_message.set_text(msg);
			//rc.bottom = rc.top + rtext.Height();
			//rmsg.right = rc.right;
			////rmsg.bottom = 1000;
			//dc.DrawText(msg, rmsg, DT_CALCRECT | DT_WORDBREAK);
			//rc.bottom = MAX(rc.top + rmsg.Height() + gap, 140);
			//TRACE(_T("re rmsg = %s\n"), get_rect_info_string(rmsg));
			//right, bottom을 줄 때는 dlg의 최소 크기를 고려한다.
			//rc.right = MAX(rmsg.Width() + gap * 2, 240);// m_sz_button.cx * 2 + gap + button_gap * 2 + 40);
		}
		else
		{
			rc.bottom = MAX(rc.top + gap + rmsg.Height() + gap + m_sz_button.cy + bottom_gap, MIN_SIZE_CY);
		}

		dc.SelectObject(pOldFont);

		rc.top -= m_title_height;
		MoveWindow(rc);

		m_button_quit.MoveWindow(CRect(rc.right - 2 - m_title_height, rc.top + 2, rc.right - 2, m_title_height - 1));

		rc.top += m_title_height;
	}

	//버튼의 조합에 따라 버튼 위치를 재조정한다.
	int x;
	int button_count = 1;

	if (type == MB_OKCANCEL)
	{
		button_count = 2;
		x = (rc.Width() - button_count * m_sz_button.cx - (button_count - 1) * button_gap) / 2;
		m_button[IDOK].MoveWindow(x, rc.bottom - bottom_gap - m_sz_button.cy, m_sz_button.cx, m_sz_button.cy);

		x += (button_gap + m_sz_button.cx);
		m_button[IDCANCEL].MoveWindow(x, rc.bottom - bottom_gap - m_sz_button.cy, m_sz_button.cx, m_sz_button.cy);
	}
	else if (type == MB_ABORTRETRYIGNORE)
	{
		button_count = 3;
		x = (rc.Width() - button_count * m_sz_button.cx - (button_count - 1) * button_gap) / 2;
		m_button[IDABORT].MoveWindow(x, rc.bottom - bottom_gap - m_sz_button.cy, m_sz_button.cx, m_sz_button.cy);

		x += (button_gap + m_sz_button.cx);
		m_button[IDRETRY].MoveWindow(x, rc.bottom - bottom_gap - m_sz_button.cy, m_sz_button.cx, m_sz_button.cy);

		x += (button_gap + m_sz_button.cx);
		m_button[IDIGNORE].MoveWindow(x, rc.bottom - bottom_gap - m_sz_button.cy, m_sz_button.cx, m_sz_button.cy);
	}
	else if (type == MB_YESNOCANCEL)
	{
		button_count = 3;
		x = (rc.Width() - button_count * m_sz_button.cx - (button_count - 1) * button_gap) / 2;
		m_button[IDYES].MoveWindow(x, rc.bottom - bottom_gap - m_sz_button.cy, m_sz_button.cx, m_sz_button.cy);

		x += (button_gap + m_sz_button.cx);
		m_button[IDNO].MoveWindow(x, rc.bottom - bottom_gap - m_sz_button.cy, m_sz_button.cx, m_sz_button.cy);

		x += (button_gap + m_sz_button.cx);
		m_button[IDCANCEL].MoveWindow(x, rc.bottom - bottom_gap - m_sz_button.cy, m_sz_button.cx, m_sz_button.cy);
	}
	else if (type == MB_YESNO)
	{
		button_count = 2;
		x = (rc.Width() - button_count * m_sz_button.cx - (button_count - 1) * button_gap) / 2;
		m_button[IDYES].MoveWindow(x, rc.bottom - bottom_gap - m_sz_button.cy, m_sz_button.cx, m_sz_button.cy);

		x += (button_gap + m_sz_button.cx);
		m_button[IDNO].MoveWindow(x, rc.bottom - bottom_gap - m_sz_button.cy, m_sz_button.cx, m_sz_button.cy);
	}
	else if (type == MB_RETRYCANCEL)
	{
		button_count = 2;
		x = (rc.Width() - button_count * m_sz_button.cx - (button_count - 1) * button_gap) / 2;
		m_button[IDRETRY].MoveWindow(x, rc.bottom - bottom_gap - m_sz_button.cy, m_sz_button.cx, m_sz_button.cy);

		x += (button_gap + m_sz_button.cx);
		m_button[IDCANCEL].MoveWindow(x, rc.bottom - bottom_gap - m_sz_button.cy, m_sz_button.cx, m_sz_button.cy);
	}
	else if (type == MB_CANCELTRYCONTINUE)
	{
		button_count = 3;
		x = (rc.Width() - button_count * m_sz_button.cx - (button_count - 1) * button_gap) / 2;
		m_button[IDCANCEL].MoveWindow(x, rc.bottom - bottom_gap - m_sz_button.cy, m_sz_button.cx, m_sz_button.cy);

		x += (button_gap + m_sz_button.cx);
		m_button[IDTRYAGAIN].MoveWindow(x, rc.bottom - bottom_gap - m_sz_button.cy, m_sz_button.cx, m_sz_button.cy);

		x += (button_gap + m_sz_button.cx);
		m_button[IDCONTINUE].MoveWindow(x, rc.bottom - bottom_gap - m_sz_button.cy, m_sz_button.cx, m_sz_button.cy);
	}
	else //if (type == MB_OK)
	{
		button_count = 1;
		x = (rc.Width() - button_count * m_sz_button.cx - (button_count - 1) * button_gap) / 2;
		m_button[IDOK].MoveWindow(x, rc.bottom - bottom_gap - m_sz_button.cy, m_sz_button.cx, m_sz_button.cy);
	}

	//아이콘의 크기만큼 좌측에서 떨어져서 메시지 본문이 표시된다.
	m_static_message.MoveWindow(rc.left + gap_side + 32 + 8, rc.top + gap, rc.Width() - gap_side * 2 - 32 - 8, rc.Height() - bottom_gap - m_sz_button.cy - gap * 2);
	m_static_message.SetWindowText(m_message);
	m_static_message.ModifyStyle(0, m_align);
	m_static_message.ShowWindow(SW_SHOW);
	
	m_button[IDOK].ShowWindow(type <= MB_OKCANCEL ? SW_SHOW : SW_HIDE);
	m_button[IDCANCEL].ShowWindow(type == MB_OKCANCEL || type == MB_YESNOCANCEL ? SW_SHOW : SW_HIDE);
	m_button[IDABORT].ShowWindow(type == MB_ABORTRETRYIGNORE ? SW_SHOW : SW_HIDE);
	m_button[IDRETRY].ShowWindow(type == MB_RETRYCANCEL || type == MB_ABORTRETRYIGNORE ? SW_SHOW : SW_HIDE);
	m_button[IDIGNORE].ShowWindow(type == MB_ABORTRETRYIGNORE ? SW_SHOW : SW_HIDE);
	m_button[IDYES].ShowWindow(type == MB_YESNO || type == MB_YESNOCANCEL ? SW_SHOW : SW_HIDE);
	m_button[IDNO].ShowWindow(type == MB_YESNO || type == MB_YESNOCANCEL ? SW_SHOW : SW_HIDE);
	m_button[IDTRYAGAIN].ShowWindow(type == MB_CANCELTRYCONTINUE ? SW_SHOW : SW_HIDE);
	m_button[IDCONTINUE].ShowWindow(type == MB_CANCELTRYCONTINUE ? SW_SHOW : SW_HIDE);

	Invalidate();
}

void CSCMessageBox::set_align(DWORD align)
{
	m_align = align;
	m_static_message.ModifyStyle(0, align);
}

void CSCMessageBox::set_color_theme(int theme)
{
	m_theme.set_color_theme(theme);

	m_static_message.set_back_color(m_theme.cr_back);
	m_button_quit.set_text_color(m_theme.cr_title_text);
	m_button_quit.set_back_color(m_theme.cr_title_back);
	m_button_quit.set_hover_back_color(gRGB(232, 17, 35));
}

LRESULT CSCMessageBox::on_message_CGdiButton(WPARAM wParam, LPARAM lParam)
{
	auto msg = (CGdiButtonMessage*)wParam;
	if (msg->msg == WM_LBUTTONUP)
	{
		TRACE(_T("on_message_CGdiButton, WM_LBUTTONUP = %s\n"), m_button_caption[msg->ctrl_id - SC_BUTTON_ID]);
		m_response = msg->ctrl_id - SC_BUTTON_ID;

		/*
		if (msg->pWnd == &m_button_ok)
		{
			m_response = IDOK;
			if (!m_as_modal)
				OnBnClickedOk();
		}
		else if (msg->pWnd == &m_button_cancel || msg->pWnd == &m_button_quit)
		{
			m_response = IDCANCEL;
			if (!m_as_modal)
				OnBnClickedCancel();
		}
		else if (msg->pWnd == &m_button_retry)
		{
			m_response = IDRETRY;
			if (!m_as_modal)
				;// OnBnClickedRetry();
		}
		*/
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
	if (!m_as_modal)
		//CDialogEx::OnOK();
		ShowWindow(SW_HIDE);
}

void CSCMessageBox::OnBnClickedCancel()
{
	//CDialogEx::OnCancel();
	m_response = IDCANCEL;
	if (!m_as_modal)
		//CDialogEx::OnCancel();
		ShowWindow(SW_HIDE);
}

//DoModal()을 사용하지 않고도 구현가능하지만
//ret = dlg.DoModal() 과 같이 호출하는 것이 일반적인 방법이므로 이 방식을 지원하기 위해
//DoModal()을 override 함.
//set_message()로 메시지 설정을 한 후 DoModal()을 호출해서 띠워도 되지만
//DoModal(_T("message")) 과 같이 직접 줄 수 있도록 함.
INT_PTR CSCMessageBox::DoModal(CString msg, int type, int timeout_sec)
{
	// TODO: 여기에 특수화된 코드를 추가 및/또는 기본 클래스를 호출합니다.
	//return CDialogEx::DoModal();

	KillTimer(timer_timeout);
	m_response = -1;
	m_as_modal = true;

	if (!msg.IsEmpty())
		set_message(msg, type, timeout_sec, m_align);

	CenterWindow(m_parent);
	ShowWindow(SW_SHOW);

	m_timeout_sec = timeout_sec;
	if (m_timeout_sec > 0)
	{
		SetTimer(timer_timeout, 1000, NULL);
	}

	MSG		stmsg;

	//for Modal Dialog
	m_parent->EnableWindow(FALSE);

	while (m_response < 0)
	{
		while (PeekMessage(&stmsg, NULL, 0, 0, PM_REMOVE))
		{
			//TRACE("GetFocus() = %p, tick = %d\n", GetFocus(), GetTickCount());
			if (stmsg.message == WM_KEYDOWN || stmsg.message == WM_KEYUP)
			{
				GetFocus()->PreTranslateMessage(&stmsg);
			}
			else
			{
				TranslateMessage(&stmsg);
				DispatchMessage(&stmsg);
			}
		}
	}

	m_parent->EnableWindow(TRUE);

	EndDialog(m_response);

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
	CFont* pOldFont = dc.SelectObject(&m_font);
	dc.SetTextColor(m_theme.cr_title_text.ToCOLORREF());

	CString title = m_title;

	if (m_timeout_sec > 0)
		title.Format(_T("%s (%d)"), m_title, m_timeout_sec);

	dc.DrawText(title, rtitle, DT_LEFT | DT_SINGLELINE | DT_VCENTER);
	dc.SelectObject(pOldFont);

	WORD index = 161;
	//HICON hIcon = ExtractAssociatedIcon(AfxGetInstanceHandle(), _T("C:\\Windows\\system32\\shell32.dll"), &index);

	HICON hIcon = ExtractShellIcon(index);
	//SHDefExtractIcon(_T("C:\\Windows\\system32\\shell32.dll"), index, 0, &hIcon, NULL, 0);
	dc.DrawIcon(20, m_title_height + 20, hIcon);

	//draw border
	draw_rectangle(g, rc, Gdiplus::Color::DimGray);
}

void CSCMessageBox::OnLButtonDown(UINT nFlags, CPoint point)
{
	// TODO: Add your message handler code here and/or call default
	if (point.y < m_title_height)
		DefWindowProc(WM_NCLBUTTONDOWN, HTCAPTION, MAKEWORD(point.x, point.y));

	CDialogEx::OnLButtonDown(nFlags, point);
}

void CSCMessageBox::OnTimer(UINT_PTR nIDEvent)
{
	// TODO: 여기에 메시지 처리기 코드를 추가 및/또는 기본값을 호출합니다.
	if (nIDEvent == timer_timeout)
	{
		CRect rc;
		GetClientRect(rc);

		InvalidateRect(CRect(0, 0, rc.right, m_title_height));

		m_timeout_sec--;

		if (m_timeout_sec < 0)
		{
			KillTimer(timer_timeout);

			if (m_as_modal)
			{
				m_response = IDOK;
			}
			else
			{
				OnBnClickedOk();
			}
		}
	}

	CDialogEx::OnTimer(nIDEvent);
}
