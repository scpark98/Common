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
bool CSCMessageBox::create(CWnd* parent, CString title, UINT icon_id, bool as_modal, int cx, int cy)
{
	m_as_modal = as_modal;
	m_parent = parent;
	m_title = title;
	m_hIcon = load_icon(NULL, icon_id, 16, 16);

	LANGID lang = GetUserDefaultUILanguage();
	m_button_caption[IDOK]			= (lang == 1042 ? _T("확인") : _T("Ok"));
	m_button_caption[IDCANCEL]		= (lang == 1042 ? _T("취소") : _T("Cancel"));
	m_button_caption[IDABORT]		= (lang == 1042 ? _T("중지") : _T("Stop"));
	m_button_caption[IDRETRY]		= (lang == 1042 ? _T("재시도") : _T("Retry"));
	m_button_caption[IDIGNORE]		= (lang == 1042 ? _T("무시") : _T("Ignore"));
	m_button_caption[IDYES]			= (lang == 1042 ? _T("예") : _T("Yes"));
	m_button_caption[IDNO]			= (lang == 1042 ? _T("아니오") : _T("No"));
	m_button_caption[IDCLOSE]		= (lang == 1042 ? _T("닫기") : _T("Close"));
	m_button_caption[IDHELP]		= (lang == 1042 ? _T("도움말") : _T("Help"));
	m_button_caption[IDTRYAGAIN]	= (lang == 1042 ? _T("다시 시도") : _T("Retry"));
	m_button_caption[IDCONTINUE]	= (lang == 1042 ? _T("계속") : _T("Continue"));

	//extract 4 icons from imageres.dll
	CString dll_path = _T("C:\\Windows\\System32\\imageres.dll");
	ExtractIconEx(dll_path, 93, &m_icons[0], NULL, 1);
	ExtractIconEx(dll_path, 94, &m_icons[1], NULL, 1);
	ExtractIconEx(dll_path, 79, &m_icons[2], NULL, 1);
	ExtractIconEx(dll_path, 76, &m_icons[3], NULL, 1);

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

	//필요한 버튼들만 생성하고자 했으나 복잡도 문제로 모두 생성한다.
	for (int i = 0; i < TOTAL_BUTTON_COUNT; i++)
	{
		m_button[i].create(m_button_caption[i], WS_CHILD | WS_TABSTOP, CRect(0, 0, m_sz_button.cx, m_sz_button.cy), this, SC_BUTTON_ID + i);
		m_button[i].use_hover();
		m_button[i].set_round(8, Gdiplus::Color::LightGray);
		//m_button[i].draw_focus_rect(true, Gdiplus::Color::Red);
	}

	reconstruct_font();

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
	m_static_message.set_font(&m_font);

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

//set_message(_T("changed message"));와 같이 동적으로 메시지만 변경할 경우는 type, timeout_sec, align은 기본 음수를 가지며
//음수일 경우는 기본값 또는 이미 설정된 값을 사용한다.
void CSCMessageBox::set_message(CString msg, int type, int timeout_sec, int align)
{
	m_message = msg;

	if (type >= MB_OK)
		m_type = type;

	if (timeout_sec > 0)
		m_timeout_sec = timeout_sec;

	if (align > -1)
		m_align = align;

	CRect rc;
	GetClientRect(rc);

	int bottom_gap = 16;	//rc.bottom과 button 사이의 간격
	int button_gap = 8;		//두 버튼 사이의 가로 간격
	int gap = 8;			//타이틀바 <-> 메시지박스 영역 <-> 버튼 여백
	int gap_side = 16;		//메시지박스의 좌우 여백
	int icon_msg_gap = 16;	//아이콘과 메시지 사이의 간격	-> 설정 함수로 변경할 것!
	int icon_size = 32;

	if (m_icon_index < 0)
	{
		icon_size = 0;
		icon_msg_gap = 0;
	}

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
		//width는 MIN_SIZE_CX 크기가 기본이지만 메시지가 길어지면 아이콘과의 기본 간격을 유지하면서 MAX_SIZE_CX까지 늘어난다.
		int needed_width = icon_size + icon_msg_gap + rmsg.Width() + gap_side * 2;
		rc.right = MAX(needed_width + 48, MIN_SIZE_CX);	//m_sz_button.cx * 2 + gap + button_gap * 2 + 40);

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

	//m_type에 따라 필요한 버튼들만 SW_SHOW하므로 이전에 실행됐을 때 SW_SHOW였던 버튼들은 모두 SW_HIDE로 만들어놓고 시작해야 한다.
	for (int i = 0; i < TOTAL_BUTTON_COUNT; i++)
		m_button[i].ShowWindow(SW_HIDE);

	//m_type에 따라 필요한 버튼들만 SW_SHOW 시킨다.
	if ((m_type & MB_OKCANCEL) == MB_OKCANCEL)
	{
		button_count = 2;
		x = (rc.Width() - button_count * m_sz_button.cx - (button_count - 1) * button_gap) / 2;
		m_button[IDOK].MoveWindow(x, rc.bottom - bottom_gap - m_sz_button.cy, m_sz_button.cx, m_sz_button.cy);
		m_button[IDOK].ShowWindow(SW_SHOW);

		x += (button_gap + m_sz_button.cx);
		m_button[IDCANCEL].MoveWindow(x, rc.bottom - bottom_gap - m_sz_button.cy, m_sz_button.cx, m_sz_button.cy);
		m_button[IDCANCEL].ShowWindow(SW_SHOW);
	}
	else if ((m_type & MB_ABORTRETRYIGNORE) == MB_ABORTRETRYIGNORE)
	{
		button_count = 3;
		x = (rc.Width() - button_count * m_sz_button.cx - (button_count - 1) * button_gap) / 2;
		m_button[IDABORT].MoveWindow(x, rc.bottom - bottom_gap - m_sz_button.cy, m_sz_button.cx, m_sz_button.cy);
		m_button[IDABORT].ShowWindow(SW_SHOW);

		x += (button_gap + m_sz_button.cx);
		m_button[IDRETRY].MoveWindow(x, rc.bottom - bottom_gap - m_sz_button.cy, m_sz_button.cx, m_sz_button.cy);
		m_button[IDRETRY].ShowWindow(SW_SHOW);

		x += (button_gap + m_sz_button.cx);
		m_button[IDIGNORE].MoveWindow(x, rc.bottom - bottom_gap - m_sz_button.cy, m_sz_button.cx, m_sz_button.cy);
		m_button[IDIGNORE].ShowWindow(SW_SHOW);
	}
	else if ((m_type & MB_YESNOCANCEL) == MB_YESNOCANCEL)
	{
		button_count = 3;
		x = (rc.Width() - button_count * m_sz_button.cx - (button_count - 1) * button_gap) / 2;
		m_button[IDYES].MoveWindow(x, rc.bottom - bottom_gap - m_sz_button.cy, m_sz_button.cx, m_sz_button.cy);
		m_button[IDYES].ShowWindow(SW_SHOW);

		x += (button_gap + m_sz_button.cx);
		m_button[IDNO].MoveWindow(x, rc.bottom - bottom_gap - m_sz_button.cy, m_sz_button.cx, m_sz_button.cy);
		m_button[IDNO].ShowWindow(SW_SHOW);

		x += (button_gap + m_sz_button.cx);
		m_button[IDCANCEL].MoveWindow(x, rc.bottom - bottom_gap - m_sz_button.cy, m_sz_button.cx, m_sz_button.cy);
		m_button[IDCANCEL].ShowWindow(SW_SHOW);
	}
	else if ((m_type & MB_YESNO) == MB_YESNO)
	{
		button_count = 2;
		x = (rc.Width() - button_count * m_sz_button.cx - (button_count - 1) * button_gap) / 2;
		m_button[IDYES].MoveWindow(x, rc.bottom - bottom_gap - m_sz_button.cy, m_sz_button.cx, m_sz_button.cy);
		m_button[IDYES].ShowWindow(SW_SHOW);

		x += (button_gap + m_sz_button.cx);
		m_button[IDNO].MoveWindow(x, rc.bottom - bottom_gap - m_sz_button.cy, m_sz_button.cx, m_sz_button.cy);
		m_button[IDNO].ShowWindow(SW_SHOW);
	}
	else if ((m_type & MB_RETRYCANCEL) == MB_RETRYCANCEL)
	{
		button_count = 2;
		x = (rc.Width() - button_count * m_sz_button.cx - (button_count - 1) * button_gap) / 2;
		m_button[IDRETRY].MoveWindow(x, rc.bottom - bottom_gap - m_sz_button.cy, m_sz_button.cx, m_sz_button.cy);
		m_button[IDRETRY].ShowWindow(SW_SHOW);

		x += (button_gap + m_sz_button.cx);
		m_button[IDCANCEL].MoveWindow(x, rc.bottom - bottom_gap - m_sz_button.cy, m_sz_button.cx, m_sz_button.cy);
		m_button[IDCANCEL].ShowWindow(SW_SHOW);
	}
	else if ((m_type & MB_CANCELTRYCONTINUE) == MB_CANCELTRYCONTINUE)
	{
		button_count = 3;
		x = (rc.Width() - button_count * m_sz_button.cx - (button_count - 1) * button_gap) / 2;
		m_button[IDCANCEL].MoveWindow(x, rc.bottom - bottom_gap - m_sz_button.cy, m_sz_button.cx, m_sz_button.cy);
		m_button[IDCANCEL].ShowWindow(SW_SHOW);

		x += (button_gap + m_sz_button.cx);
		m_button[IDTRYAGAIN].MoveWindow(x, rc.bottom - bottom_gap - m_sz_button.cy, m_sz_button.cx, m_sz_button.cy);
		m_button[IDTRYAGAIN].ShowWindow(SW_SHOW);

		x += (button_gap + m_sz_button.cx);
		m_button[IDCONTINUE].MoveWindow(x, rc.bottom - bottom_gap - m_sz_button.cy, m_sz_button.cx, m_sz_button.cy);
		m_button[IDCONTINUE].ShowWindow(SW_SHOW);
	}
	else //if (m_type == MB_OK)
	{
		button_count = 1;
		x = (rc.Width() - button_count * m_sz_button.cx - (button_count - 1) * button_gap) / 2;
		m_button[IDOK].MoveWindow(x, rc.bottom - bottom_gap - m_sz_button.cy, m_sz_button.cx, m_sz_button.cy);
		m_button[IDOK].ShowWindow(SW_SHOW);
	}


	//옵션에 따라 아이콘 인덱스 결정
	//m_use_typed_title_back_color이 true이면 question은 연두색, error는 핑크색, info는 하늘색, warning은 주황색으로 표시된다.
	if ((m_type & MB_ICONMASK) == MB_ICONSTOP)
	{
		m_icon_index = 0;
		if (m_use_typed_title_back_color)
			m_theme.cr_title_back = gRGB(242, 222, 222);
	}
	else if ((m_type & MB_ICONMASK) == MB_ICONQUESTION)
	{
		m_icon_index = 1;
		if (m_use_typed_title_back_color)
			m_theme.cr_title_back = gRGB(223, 240, 216);
	}
	else if ((m_type & MB_ICONMASK) == MB_ICONEXCLAMATION)
	{
		m_icon_index = 2;
		if (m_use_typed_title_back_color)
			m_theme.cr_title_back = gRGB(254, 219, 156);
	}
	else if ((m_type & MB_ICONMASK) == MB_ICONINFORMATION)
	{
		m_icon_index = 3;
		if (m_use_typed_title_back_color)
			m_theme.cr_title_back = gRGB(217, 237, 247);
	}
	else
	{
		m_icon_index = -1;
	}

	//메시지 타입에 따라 타이틀바 색상을 다르게 한다.
	if (m_use_typed_title_back_color)
	{
		m_theme.cr_title_text = gRGB(0, 0, 0);

		//버튼 색상도 타이틀바 색상과 동일하게 하려 했으나 우선 포커스 색상만 동일하게 한다.
		for (int i = 0; i < TOTAL_BUTTON_COUNT; i++)
		{
			m_button[i].draw_focus_rect(true, m_theme.cr_title_back);
			//m_button[i].set_text_color(m_theme.cr_title_text);
			//m_button[i].set_back_color(m_theme.cr_title_back);
		}

		m_button_quit.set_text_color(m_theme.cr_title_text);
		m_button_quit.set_back_color(m_theme.cr_title_back);
		m_button_quit.set_hover_back_color(gRGB(232, 17, 35));
	}


	//아이콘을 이 dlg에서 표시한다? CSCStatic에서 표시한다?
	//CSCStatic에서도 아이콘을 표시하는 기능이 있지만 주로 한줄의 캡션과 함께 사용하는 것이 많고
	//메시지박스의 아이콘의 레이아웃을 보면 메시지가 센터정렬이라고 해도 아이콘은 왼쪽 정렬되어 표시되는 등 다른점이 있으므로
	//CSCStatic에 표시하지 않고 이 클래스의 OnPaint()에서 별도로 표시한다.
	//아이콘의 크기만큼 좌측에서 떨어져서 메시지 본문이 표시된다.
	m_static_message.MoveWindow(rc.left + gap_side, rc.top + gap, rc.Width() - gap_side * 2, rc.Height() - bottom_gap - m_sz_button.cy - gap * 2);
	//m_static_message.MoveWindow(rc.left + gap_side, rc.top + gap, rc.Width() - gap_side * 2, rc.Height() - bottom_gap - m_sz_button.cy - gap * 2);
	m_static_message.SetWindowText(m_message);
	m_static_message.ModifyStyle(0, m_align);
	if (m_icon_index >= 0)
		m_static_message.set_icon(m_icons[m_icon_index], icon_size, true);
	m_static_message.ShowWindow(SW_SHOW);

	Invalidate();

	if (!m_as_modal)
	{
		CenterWindow(m_parent);
		ShowWindow(SW_SHOW);

		if (m_timeout_sec > 0)
			SetTimer(timer_timeout, 1000, NULL);
	}
}

void CSCMessageBox::set_align(int align)
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

		if (!m_as_modal)
		{
			::SendMessage(m_parent->m_hWnd, Message_CSCMessageBox, (WPARAM)this, m_response);
			ShowWindow(SW_HIDE);
		}
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
		ShowWindow(SW_HIDE);
}

void CSCMessageBox::OnBnClickedCancel()
{
	m_response = IDCANCEL;
	if (!m_as_modal)
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
		SetTimer(timer_timeout, 1000, NULL);

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

	//draw title icon
	if (m_hIcon)
	{
		CRect ricon = rtitle;
		ricon.right = ricon.left + 16;
		rtitle.left = ricon.right + 8;
		draw_icon(&dc, m_hIcon, ricon);
	}
	//end of draw title icon

	
	//draw title text
	CFont* pOldFont = dc.SelectObject(&m_font);
	dc.SetTextColor(m_theme.cr_title_text.ToCOLORREF());

	CString title = m_title;

	if (m_timeout_sec > 0)
		title.Format(_T("%s (%d)"), m_title, m_timeout_sec);

	dc.DrawText(title, rtitle, DT_LEFT | DT_SINGLELINE | DT_VCENTER);
	dc.SelectObject(pOldFont);
	//end of draw title text


	//draw messagebox icon
	//아이콘의 세로 위치는 메시지박스의 m_align에	따라 다르다.
	//CRect rmsg;
	//m_static_message.GetWindowRect(rmsg);
	//ScreenToClient(rmsg);
	//if (m_align & SS_CENTERIMAGE)
	//	dc.DrawIcon(16, rmsg.CenterPoint().y - 16, m_icons[m_icon_index]);
	//else
	//	dc.DrawIcon(16, rmsg.top, m_icons[m_icon_index]);


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

		if (m_timeout_sec <= 0)
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
