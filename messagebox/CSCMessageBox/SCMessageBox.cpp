// SCMessageBox.cpp: 구현 파일
//

#include "SCMessageBox.h"
#include "../../Functions.h"
#include "../../MemoryDC.h"

//SHGetStockIconInfo (Vista+). XP toolset (v110_xp) 에서는 #ifndef _USING_V110_SDK71_ 가드로 보호됨.
#ifndef _USING_V110_SDK71_
#include <shellapi.h>
#pragma comment(lib, "shell32.lib")
#endif

#define MIN_SIZE_CX	240
#define MAX_SIZE_CX	800
#define MIN_SIZE_CY	160

// CSCMessageBox 대화 상자

IMPLEMENT_DYNAMIC(CSCMessageBox, CDialogEx)

CSCMessageBox::CSCMessageBox(CWnd* parent, CString title, UINT icon_id, bool as_modal, int cx, int cy)
{
	memset(&m_lf, 0, sizeof(LOGFONT));

	//과거에는 여기서 create() 를 자동 호출했으나, theApp.m_msgbox 와 같이 CWinApp 멤버로
	//선언되면 WinMain 이전 global object ctor 단계에서 호출되어 AfxRegisterClass / CreateEx 가
	//MFC 초기화 전에 실행되는 문제가 있었다. 사용자가 InitInstance 등에서 명시적으로 create() 를
	//호출해야 한다.
	//인자도 무시 (parent, title, icon_id, as_modal, cx, cy 모두 사용자가 create() 호출 시 전달).
	(void)parent; (void)title; (void)icon_id; (void)as_modal; (void)cx; (void)cy;
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
	//이미 생성되어 있으면 재생성하지 않는다. (중복 호출 가드)
	//중복 CreateEx 는 MFC 의 CWnd 가 이미 HWND 와 연결된 상태에서 ASSERT.
	if (m_hWnd != NULL)
		return true;

	//AfxGetApp() 가 NULL 일 수 있는 매우 이른 시점 호출도 안전하게.
	CWinApp* p_app = AfxGetApp();
	if (parent == nullptr && p_app != nullptr)
		parent = p_app->GetMainWnd();

	//parent 가 여전히 nullptr 이어도 진행한다.
	//theApp 에서 m_msgbox 를 멤버로 두는 사용 패턴에서는 InitInstance 시점에
	//m_pMainWnd 가 아직 NULL 이라 fallback 도 실패한다. 이 경우 m_parent 는
	//DoModal()/set_message() 시점에 GetActiveWindow() 로 lazy binding 한다.
	//WS_POPUP 윈도우는 owner 없이도 CreateEx 로 생성 가능.
	m_parent = parent;
	m_title = title;

	m_as_modal = as_modal;

	//icon_id 가 0(미지정) 이면 타이틀 아이콘 없이 진행. load_icon 자체도 스킵.
	//아이콘 로드 실패는 치명적이지 않으므로 NULL 이어도 그대로 둔다 (그리기 측에서 가드).
	m_hIcon = (icon_id != 0) ? load_icon(NULL, icon_id, 16, 16) : NULL;

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

	//메시지박스 아이콘 로딩.
	//Vista+ : SHGetStockIconInfo 로 현재 테마/DPI 에 맞는 시스템 stock 아이콘.
	//XP     : LoadIcon(NULL, IDI_xxx) 로 폴백 — XP SDK 에서 SHGetStockIconInfo 미정의.
	//(_USING_V110_SDK71_ 매크로는 v110_xp toolset 에서 정의되며 XP 호환 빌드를 의미.)
#ifndef _USING_V110_SDK71_
	{
		auto load_stock = [](SHSTOCKICONID id) -> HICON
		{
			SHSTOCKICONINFO sii = {};
			sii.cbSize = sizeof(sii);
			if (SUCCEEDED(::SHGetStockIconInfo(id, SHGSI_ICON, &sii)))
				return sii.hIcon;
			return NULL;
		};
		m_icons[0] = load_stock(SIID_ERROR);     //MB_ICONSTOP/ERROR/HAND
		m_icons[1] = load_stock(SIID_HELP);      //MB_ICONQUESTION
		m_icons[2] = load_stock(SIID_WARNING);   //MB_ICONEXCLAMATION/WARNING
		m_icons[3] = load_stock(SIID_INFO);      //MB_ICONINFORMATION/ASTERISK
	}
#else
	m_icons[0] = ::LoadIcon(NULL, IDI_ERROR);        //MB_ICONSTOP/ERROR/HAND
	m_icons[1] = ::LoadIcon(NULL, IDI_QUESTION);     //MB_ICONQUESTION
	m_icons[2] = ::LoadIcon(NULL, IDI_WARNING);      //MB_ICONEXCLAMATION/WARNING
	m_icons[3] = ::LoadIcon(NULL, IDI_INFORMATION);  //MB_ICONINFORMATION/ASTERISK
#endif

	if (cx < 0 || cy < 0)
	{
		m_auto_size = true;
		cx = DEFAULT_SIZE_CX;
		cy = DEFAULT_SIZE_CY;
	}

	memset(&m_lf, 0, sizeof(LOGFONT));


	LONG_PTR dwStyle = WS_POPUP;

	//"#32770" 은 user32 가 등록한 시스템 dialog class. GetClassInfo 의 hInstance 는 시스템
	//클래스를 조회할 때 NULL 이어야 한다. 앱 hInstance 로 조회하면 못 찾아 wc 가 {} 로 남아
	//lpfnWndProc=NULL → RegisterClass 실패 → CreateEx 실패의 연쇄가 발생.
	WNDCLASS wc = {};
	if (!::GetClassInfo(NULL, _T("#32770"), &wc))
	{
		//시스템 클래스 조회 실패 시 안전한 기본값으로 폴백. 본 메시지박스는 dialog manager 를
		//거치지 않고 CreateEx 로 직접 popup 을 띄우므로 DefWindowProc 만으로 충분.
		wc.style		= CS_DBLCLKS | CS_SAVEBITS;
		wc.lpfnWndProc	= ::DefWindowProc;
		wc.cbClsExtra	= 0;
		wc.cbWndExtra	= DLGWINDOWEXTRA;
		wc.hIcon		= NULL;
		wc.hCursor		= ::LoadCursor(NULL, IDC_ARROW);
		wc.hbrBackground = (HBRUSH)(COLOR_3DFACE + 1);
		wc.lpszMenuName	= NULL;
	}
	//hInstance 는 등록 모듈로 강제. (GetClassInfo 결과가 user32 hInstance 일 수 있음.)
	wc.hInstance		= AfxGetInstanceHandle();
	wc.lpszClassName	= _T("CSCMessageDlg");
	//중복 등록은 RegisterClass 가 ERROR_CLASS_ALREADY_EXISTS 로 실패하지만 본 호출은 idempotent.
	AfxRegisterClass(&wc);

	//WS_EX_TOOLWINDOW: parent 가 NULL 이어도 taskbar / Alt+Tab 에서 제외.
	//(parent 없는 owner-less 팝업은 기본적으로 taskbar 에 별도 항목으로 뜨기 때문.)
	//본 클래스는 caption 을 직접 그리므로 WS_EX_TOOLWINDOW 가 강제하는 짧은 캡션바 영향은 없음.
	bool res = CreateEx(WS_EX_TOOLWINDOW, wc.lpszClassName, _T("CSCMessageDlg"), (DWORD)dwStyle, CRect(0, 0, cx, cy), parent, 0);
	if (!res)
		return false;

	dwStyle = ::GetWindowLongPtr(m_hWnd, GWL_STYLE);

	//캡션 + 모든 테두리 제거
	dwStyle &= ~(WS_CAPTION | WS_THICKFRAME | WS_BORDER | WS_DLGFRAME);
	//dwStyle |= WS_THICKFRAME; //resize를 지원할 필요가 없을 경우는 주석처리한다.
	::SetWindowLongPtr(m_hWnd, GWL_STYLE, dwStyle);

	//반드시 필요 (프레임 다시 계산)
	::SetWindowPos(m_hWnd, nullptr, 0, 0, 0, 0,
		SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER |
		SWP_NOACTIVATE | SWP_FRAMECHANGED);

	//캡션바를 제거해도 직사각이 아닌 윈11처럼 라운드 모양으로.
	//XP에서는 지원되지 않을것이다.
#ifndef _USING_V110_SDK71_
	DWORD corner = DWMWCP_ROUND;
	DwmSetWindowAttribute(m_hWnd, DWMWA_WINDOW_CORNER_PREFERENCE, &corner, sizeof(corner));
#endif


	CRect rc;
	GetClientRect(rc);

	//폰트 결정 우선순위:
	//  1) 자기 자신의 GetFont()  - popup 으로 막 생성된 상태라 보통 NULL.
	//  2) parent->GetFont()      - parent 가 있을 때 외관 일관성을 위해.
	//  3) SPI_GETNONCLIENTMETRICS.lfMessageFont
	//                            - Windows 가 "메시지 상자" 용으로 지정한 시스템 폰트
	//                              (디스플레이 설정에 따라 보통 Segoe UI). parent 가 없어도
	//                              메시지박스에 가장 적절한 모던 폰트.
	//  4) DEFAULT_GUI_FONT       - 3) 도 실패할 때의 최후 폴백. SYSTEM_FONT 는 비트맵 계열의
	//                              구식 폰트라 사용하지 않는다 (안티앨리어싱 없음).
	CFont* font = GetFont();
	if (font == NULL && parent != nullptr)
		font = parent->GetFont();

	if (font != NULL)
	{
		font->GetObject(sizeof(m_lf), &m_lf);
	}
	else
	{
		NONCLIENTMETRICS ncm = {};
		ncm.cbSize = sizeof(ncm);
		if (::SystemParametersInfo(SPI_GETNONCLIENTMETRICS, sizeof(ncm), &ncm, 0))
			m_lf = ncm.lfMessageFont;
		else
			GetObject(GetStockObject(DEFAULT_GUI_FONT), sizeof(m_lf), &m_lf);
	}

	//필요한 버튼들만 생성하고자 했으나 복잡도 문제로 모두 생성한다.
	for (int i = 0; i < TOTAL_BUTTON_COUNT; i++)
	{
		m_button[i].create(m_button_caption[i], WS_CHILD | WS_TABSTOP, CRect(0, 0, m_sz_button.cx, m_sz_button.cy), this, SC_BUTTON_ID + i);
		m_button[i].use_hover();
		m_button[i].set_round(4, Gdiplus::Color::LightGray);

		m_button[i].set_text_color(m_theme.cr_text);
		m_button[i].set_back_color(m_theme.cr_title_back_inactive);
		m_button[i].set_parent_back_color(m_theme.cr_back);
	}

	reconstruct_font();

	//종료 버튼은 IDCLOSE가 아닌 IDCANCEL로 처리해야 한다.
	m_button_quit.create(_T(""), WS_CHILD | WS_VISIBLE | BS_FLAT,
						CRect(rc.right - 2 - m_title_height, rc.top + 2, rc.right - 2, m_title_height - 1), this, SC_BUTTON_ID + IDCANCEL);
	m_button_quit.set_button_cmd(SC_CLOSE);
	m_button_quit.set_text_color(m_theme.cr_title_text);
	m_button_quit.set_back_color(m_theme.cr_title_back_active);
	m_button_quit.set_hover_back_color(gRGB(232, 17, 35));
	m_button_quit.use_hover();

	m_static_message.create(_T("message"), WS_CHILD | WS_VISIBLE, CRect(0, m_title_height, cx, rc.bottom - 20 - 8 - m_sz_button.cy), this, 0);
	m_static_message.set_back_color(m_theme.cr_back);
	m_static_message.set_text_color(m_theme.cr_text);
	m_static_message.set_font(&m_font);
	m_static_message.set_halign(DT_CENTER);
	m_static_message.set_valign(DT_VCENTER);

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

	//if (type >= MB_OK)
	m_type = type;

	if (timeout_sec > 0)
		m_timeout_sec = timeout_sec;

	if (align > -1)
		m_align = align;

	//m_icon_index 를 가장 먼저 결정한다. 이후의 icon_size / m_static_message 위치 / m_icon_rect
	//계산이 모두 m_icon_index 에 의존하므로 순서가 중요. (이전 버전은 layout 코드 *뒤*에 결정해
	//icon_size 가 0 으로 떨어지는 버그가 있었음.)
	if ((m_type & MB_ICONMASK) == MB_ICONSTOP)             m_icon_index = 0;
	else if ((m_type & MB_ICONMASK) == MB_ICONQUESTION)    m_icon_index = 1;
	else if ((m_type & MB_ICONMASK) == MB_ICONEXCLAMATION) m_icon_index = 2;
	else if ((m_type & MB_ICONMASK) == MB_ICONINFORMATION) m_icon_index = 3;
	else                                                   m_icon_index = -1;

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

	//MB_HELP는 다른 버튼들의 조합 프리셋에는 기본 포함되지 않으므로
	//has_help인지 판별하고 그에 따라 버튼 갯수가 달라지며
	//항상 맨 오른쪽에 붙기 때문에 아래 if문 후에 오른쪽 끝에 붙인다.
	bool has_help = ((m_type >= 0) && (m_type & MB_HELP) == MB_HELP);

	//m_type에 따라 필요한 버튼들만 SW_SHOW 시킨다.
	if ((m_type >= 0) && (m_type & MB_OKCANCEL) == MB_OKCANCEL)
	{
		button_count = 2 + (has_help ? 1 : 0);

		x = (rc.Width() - button_count * m_sz_button.cx - (button_count - 1) * button_gap) / 2;
		m_button[IDOK].MoveWindow(x, rc.bottom - bottom_gap - m_sz_button.cy, m_sz_button.cx, m_sz_button.cy);
		m_button[IDOK].ShowWindow(SW_SHOW);

		x += (button_gap + m_sz_button.cx);
		m_button[IDCANCEL].MoveWindow(x, rc.bottom - bottom_gap - m_sz_button.cy, m_sz_button.cx, m_sz_button.cy);
		m_button[IDCANCEL].ShowWindow(SW_SHOW);
	}
	else if ((m_type >= 0) && (m_type & MB_ABORTRETRYIGNORE) == MB_ABORTRETRYIGNORE)
	{
		button_count = 3 + (has_help ? 1 : 0);
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
	else if ((m_type >= 0) && (m_type & MB_YESNOCANCEL) == MB_YESNOCANCEL)
	{
		button_count = 3 + (has_help ? 1 : 0);
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
	else if ((m_type >= 0) && (m_type & MB_YESNO) == MB_YESNO)
	{
		button_count = 2 + (has_help ? 1 : 0);
		x = (rc.Width() - button_count * m_sz_button.cx - (button_count - 1) * button_gap) / 2;
		m_button[IDYES].MoveWindow(x, rc.bottom - bottom_gap - m_sz_button.cy, m_sz_button.cx, m_sz_button.cy);
		m_button[IDYES].ShowWindow(SW_SHOW);

		x += (button_gap + m_sz_button.cx);
		m_button[IDNO].MoveWindow(x, rc.bottom - bottom_gap - m_sz_button.cy, m_sz_button.cx, m_sz_button.cy);
		m_button[IDNO].ShowWindow(SW_SHOW);

		//for common test for tootip text. 툴팁 정상 표시 확인됨.
		//m_button[IDYES].set_tooltip_text(_T("This is IDYES button"));
	}
	else if ((m_type >= 0) && (m_type & MB_RETRYCANCEL) == MB_RETRYCANCEL)
	{
		button_count = 2 + (has_help ? 1 : 0);
		x = (rc.Width() - button_count * m_sz_button.cx - (button_count - 1) * button_gap) / 2;
		m_button[IDRETRY].MoveWindow(x, rc.bottom - bottom_gap - m_sz_button.cy, m_sz_button.cx, m_sz_button.cy);
		m_button[IDRETRY].ShowWindow(SW_SHOW);

		x += (button_gap + m_sz_button.cx);
		m_button[IDCANCEL].MoveWindow(x, rc.bottom - bottom_gap - m_sz_button.cy, m_sz_button.cx, m_sz_button.cy);
		m_button[IDCANCEL].ShowWindow(SW_SHOW);
	}
	else if ((m_type >= 0) && (m_type & MB_CANCELTRYCONTINUE) == MB_CANCELTRYCONTINUE)
	{
		button_count = 3 + (has_help ? 1 : 0);
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
	else if ((m_type >= 0) && (m_type & MB_TYPEMASK) == MB_OK)
	{
		//MB_OK == 0 이라서 단순 == 비교는 icon flag (예: MB_ICONEXCLAMATION = 0x30) 등이
		//OR 로 같이 들어오면 매치되지 않는다. MB_TYPEMASK(0x0F) 로 버튼 타입 비트만 추출해 비교.
		button_count = 1 + (has_help ? 1 : 0);
		x = (rc.Width() - button_count * m_sz_button.cx - (button_count - 1) * button_gap) / 2;
		m_button[IDOK].MoveWindow(x, rc.bottom - bottom_gap - m_sz_button.cy, m_sz_button.cx, m_sz_button.cy);
		m_button[IDOK].ShowWindow(SW_SHOW);
	}

	if (has_help)
	{
		x += (button_gap + m_sz_button.cx);
		m_button[IDHELP].MoveWindow(x, rc.bottom - bottom_gap - m_sz_button.cy, m_sz_button.cx, m_sz_button.cy);
		m_button[IDHELP].ShowWindow(SW_SHOW);
	}



	//m_icon_index 는 이미 함수 진입부에서 결정됨.
	//여기서는 m_use_typed_title_back_color 옵션이 켜진 경우 타입별 타이틀바 색만 분기.
	//error → 핑크, question → 연두, warning → 주황, info → 하늘색.
	if (m_use_typed_title_back_color)
	{
		switch (m_icon_index)
		{
		case 0: m_theme.cr_title_back_active = gRGB(242, 222, 222); break;
		case 1: m_theme.cr_title_back_active = gRGB(223, 240, 216); break;
		case 2: m_theme.cr_title_back_active = gRGB(254, 219, 156); break;
		case 3: m_theme.cr_title_back_active = gRGB(217, 237, 247); break;
		default: break;
		}
	}

	//메시지 타입에 따라 타이틀바 색상을 다르게 한다.
	if (m_use_typed_title_back_color)
	{
		m_theme.cr_title_text = gRGB(0, 0, 0);

		//버튼 색상도 타이틀바 색상과 동일하게 하려 했으나 우선 포커스 색상만 동일하게 한다.
		for (int i = 0; i < TOTAL_BUTTON_COUNT; i++)
		{
			m_button[i].draw_focus_rect(true, m_theme.cr_title_back_active);
			//m_button[i].set_text_color(m_theme.cr_title_text);
			//m_button[i].set_back_color(m_theme.cr_title_back);
		}

		m_button_quit.set_text_color(m_theme.cr_title_text);
		m_button_quit.set_back_color(m_theme.cr_title_back_active);
		m_button_quit.set_hover_back_color(gRGB(232, 17, 35));
	}


	//메시지 아이콘은 CSCStatic 의 set_icon 이 아니라 본 클래스 OnPaint 에서 직접 그린다.
	//이유: CSCStatic 의 set_icon 은 전체 텍스트 블록 중심 정렬 위주라 멀티라인에서 첫 줄과 정렬되지 않음.
	//본 클래스는 m_icon_rect 를 "첫 줄 텍스트의 세로 중심" 기준으로 계산해 둠.
	//아이콘 공간만큼 m_static_message 를 우측으로 들여쓴다.
	int message_left_offset = (m_icon_index >= 0) ? (icon_size + icon_msg_gap) : 0;
	m_static_message.MoveWindow(
		rc.left + gap_side + message_left_offset,
		rc.top + gap,
		rc.Width() - gap_side * 2 - message_left_offset,
		rc.Height() - bottom_gap - m_sz_button.cy - gap * 2);
	m_static_message.SetWindowText(m_message);
	m_static_message.ModifyStyle(0, m_align);
	m_static_message.ShowWindow(SW_SHOW);

	//m_icon_rect 계산 — 첫 줄 텍스트의 세로 중심에 아이콘의 세로 중심을 맞춤.
	//단일라인이면 자연스럽게 정중앙 정렬처럼 보이고, 멀티라인이면 첫 줄을 라벨링하는 모양.
	if (m_icon_index >= 0)
	{
		//line_height 는 LOGFONT 의 절대 픽셀 높이 + leading. tmHeight 까지 안 쓰고 근사 사용.
		int line_height = abs(m_lf.lfHeight);
		if (line_height <= 0)
			line_height = 16;

		int line_count = 1;
		for (int i = 0; i < m_message.GetLength(); i++)
		{
			if (m_message[i] == _T('\n'))
				line_count++;
		}

		CRect static_rect;
		m_static_message.GetWindowRect(static_rect);
		ScreenToClient(static_rect);

		//m_static_message 가 DT_VCENTER 라서 텍스트 블록이 세로 중앙에 배치됨.
		//텍스트 블록의 첫 줄 top = static_rect.top + (static_height - text_block_height) / 2.
		int text_block_height = line_count * line_height;
		int first_line_top = static_rect.top + (static_rect.Height() - text_block_height) / 2;
		int first_line_center = first_line_top + line_height / 2;

		m_icon_rect.left = rc.left + gap_side;
		m_icon_rect.top = first_line_center - icon_size / 2;
		m_icon_rect.right = m_icon_rect.left + icon_size;
		m_icon_rect.bottom = m_icon_rect.top + icon_size;

		//아이콘이 타이틀바를 침범하지 않도록 가드.
		int min_top = rc.top + m_title_height + gap;
		if (m_icon_rect.top < min_top)
			m_icon_rect.OffsetRect(0, min_top - m_icon_rect.top);
	}
	else
	{
		m_icon_rect.SetRectEmpty();
	}

	//for common test for tootip text. 툴팁 정상 표시 확인됨.
	//m_static_message.set_tooltip_text(_T("This is CSCStatic tootip."));

	Invalidate();

	//if (m_as_modal)
	{
		//m_parent==NULL 이면 화면 기준 가운데. GetDesktopWindow() 보다 다중모니터 안전.
		CenterWindow(m_show_on_parent_center ? m_parent : nullptr);
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

//set_color_theme()은 create() 전에도 호출될 수 있으므로 그에 대한 처리도 필요하다.
void CSCMessageBox::set_color_theme(int theme)
{
	m_theme.set_color_theme(theme);
	apply_theme();
}

//호출자가 이미 cr_back 등을 수정해 둔 CSCColorTheme 객체를 그대로 적용.
//operator= 가 아닌 copy_colors_from() 을 사용해 m_parent (= 본 msgbox) 는 보존.
void CSCMessageBox::set_color_theme(const CSCColorTheme& theme)
{
	m_theme.copy_colors_from(theme);
	apply_theme();
}

void CSCMessageBox::apply_theme()
{
	if (m_static_message.m_hWnd)
	{
		m_static_message.set_text_color(m_theme.cr_text);
		m_static_message.set_back_color(m_theme.cr_back);
	}

	if (m_button_quit)
	{
		m_button_quit.set_text_color(m_theme.cr_title_text);
		m_button_quit.set_back_color(m_theme.cr_title_back_active);
		m_button_quit.set_hover_back_color(gRGB(232, 17, 35));
	}

	for (int i = 0; i < TOTAL_BUTTON_COUNT; i++)
	{
		if (m_button[i].m_hWnd)
		{
			m_button[i].set_text_color(m_theme.cr_title_text);
			m_button[i].set_back_color(m_theme.cr_title_back_inactive);
			m_button[i].set_parent_back_color(m_theme.cr_back);
			m_button[i].set_hover_back_color(m_theme.cr_title_back_active);
			m_button[i].draw_focus_rect(true, m_theme.cr_title_back_active);
		}
	}
}

LRESULT CSCMessageBox::on_message_CGdiButton(WPARAM wParam, LPARAM lParam)
{
	auto msg = (CGdiButtonMessage*)wParam;

	if (m_parent == nullptr)
		m_parent = msg->pThis->GetParent();

	if (msg->message == WM_LBUTTONUP)
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

	if (m_as_modal)
		EndDialog(m_response);
	else
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

	//m_parent lazy binding 순서: 명시 지정 → GetParent() → AfxGetMainWnd() → GetActiveWindow().
	//그래도 NULL 이면 NULL 로 두고 아래 호출들을 가드한다.
	//(theApp 에서 m_msgbox.create(nullptr,...) 으로 생성된 케이스 대응.)
	if (m_parent == nullptr)
		m_parent = GetParent();
	if (m_parent == nullptr)
		m_parent = AfxGetMainWnd();
	if (m_parent == nullptr)
		m_parent = CWnd::GetActiveWindow();

	//owner 관계 설정 — owned window 는 owner 보다 항상 위에 z-order 가 강제되므로
	//메인앱 클릭으로 메시지박스가 뒤로 가는 것을 막는다.
	//create() 시점엔 m_parent==NULL 이라 owner 없이 생성되었을 수 있으므로 여기서 보정.
	if (m_parent && m_parent->m_hWnd && m_hWnd)
		::SetWindowLongPtr(m_hWnd, GWLP_HWNDPARENT, (LONG_PTR)m_parent->m_hWnd);

	KillTimer(timer_timeout);
	m_response = -1;
	m_as_modal = true;

	if (!msg.IsEmpty())
		set_message(msg, type, timeout_sec, m_align);

	//CenterWindow(NULL) 은 MFC 가 화면 기준으로 가운데 배치하므로 GetDesktopWindow() 보다 안전.
	//GetDesktopWindow() 는 virtual desktop 전체(다중 모니터 합산) 기준이라 모니터 사이에 뜰 수 있음.
	CenterWindow(m_show_on_parent_center ? m_parent : nullptr);
	ShowWindow(SW_SHOW);

	m_timeout_sec = timeout_sec;
	if (m_timeout_sec > 0)
		SetTimer(timer_timeout, 1000, NULL);

	MSG		stmsg;

	//for Modal Dialog. m_parent 가 NULL 이면 차단할 부모가 없는 진짜 top-level 모달.
	if (m_parent && m_parent->m_hWnd)
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

	if (m_parent && m_parent->m_hWnd)
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

	draw_rect(g, rc, m_theme.cr_title_back_inactive, m_theme.cr_back);

	//draw titlebar
	rtitle = rc;
	rtitle.bottom = rtitle.top + m_title_height;
	dc.FillSolidRect(rtitle, m_theme.cr_title_back_inactive.ToCOLORREF());

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

	//draw message icon — set_message 에서 계산된 m_icon_rect 에 직접 그림.
	//첫 줄 텍스트의 세로 중심과 정렬되도록 m_icon_rect 가 미리 셋팅됨.
	if (m_icon_index >= 0 && m_icons[m_icon_index] && !m_icon_rect.IsRectEmpty())
	{
		::DrawIconEx(dc.GetSafeHdc(),
			m_icon_rect.left, m_icon_rect.top,
			m_icons[m_icon_index],
			m_icon_rect.Width(), m_icon_rect.Height(),
			0, NULL, DI_NORMAL);
	}
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
