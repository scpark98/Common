// SCMessageBox.cpp: 구현 파일
//

#include "SCMessageBox.h"
#include "../../Functions.h"
#include "../../MemoryDC.h"
#include "../../win_compat/dwm.h"

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

	//WS_EX_TOOLWINDOW 는 *owner 가 있을 때만* 적용.
	//Tool window 는 Windows 설계상 owner 윈도우의 보조 부유(floating) 윈도우다.
	//owner 가 없는 tool window 는 ShowWindow(SW_HIDE) → SW_SHOW 의 재활성 흐름이 깨져
	//두 번째 호출부터 visible 이지만 foreground 가 아닌 상태로 입력을 못 받는 버그가 있다.
	//owner 가 없으면 일반 popup 으로 생성 — taskbar 에 잠깐 뜨더라도 동작은 정상.
	//(DoModal 에서 lazy binding 으로 m_parent 가 잡히면 SetWindowLongPtr GWLP_HWNDPARENT 로
	// owner 가 설정되며, owned 윈도우는 WS_EX_TOOLWINDOW 없이도 taskbar 에서 자동 제외된다.)
	DWORD ex_style = (parent != nullptr) ? WS_EX_TOOLWINDOW : 0;
	bool res = CreateEx(ex_style, wc.lpszClassName, _T("CSCMessageDlg"), (DWORD)dwStyle, CRect(0, 0, cx, cy), parent, 0);
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
	//XP/Win10 이하는 헬퍼 안에서 자동 no-op.
	win_compat::dwm::set_window_corner_round(m_hWnd);


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
		//apply_theme 와 동일한 경로 (CGdiButton::set_color_theme) 로 초기 색 산출 — main dlg 의 일반
		//GdiButton 과 외관 일관성 확보. set_color_theme 안에서 set_round(4, ...) 도 함께 적용됨.
		m_button[i].set_color_theme(m_theme);
	}

	reconstruct_font();

	//종료 버튼은 IDCLOSE가 아닌 IDCANCEL로 처리해야 한다.
	m_button_quit.create(_T(""), WS_CHILD | WS_VISIBLE | BS_FLAT,
						CRect(rc.right - 2 - m_title_height, rc.top + 2, rc.right - 2, m_title_height - 1), this, SC_BUTTON_ID + IDCANCEL);
	m_button_quit.set_button_cmd(SC_CLOSE);
	m_button_quit.set_text_color(m_theme.cr_title_text);
	//title bar 가 cr_title_back_inactive 로 그려지므로 X 버튼도 같은 색으로 blend.
	m_button_quit.set_back_color(m_theme.cr_title_back_inactive);
	m_button_quit.set_hover_back_color(gRGB(232, 17, 35));
	m_button_quit.use_hover();

	m_static_message.create(_T("message"), WS_CHILD | WS_VISIBLE, CRect(0, m_title_height, cx, rc.bottom - 20 - 8 - m_sz_button.cy), this, 0);
	m_static_message.set_back_color(m_theme.cr_back);
	m_static_message.set_text_color(m_theme.cr_text);
	m_static_message.set_font(&m_font);
	m_static_message.set_halign(DT_CENTER);
	m_static_message.set_valign(DT_VCENTER);
	//SCStatic 단락 모드 기본 line_spacing 은 1.2x. 메시지박스에서는 1.0 이 너무 좁다는 피드백으로 1.3 으로 절충.
	//아이콘 정렬 수식은 set_message 에서 spacing-aware 로 (line_count + (line_count-1)*(spacing-1)) 계산.
	m_static_message.set_line_spacing(1.3f);
	//paragraph 의 안티앨리어싱 기본값은 false (작은 시스템 폰트 흐림 방지). 메시지박스 텍스트는
	//태그 컬러 강조 등 큰 글자도 종종 들어오므로 ON 으로 켜서 부드럽게 렌더링.
	m_static_message.set_font_antialiasing(true);

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

	//halign 결정 — set_text 호출 전에 두면 rebuild_layout 이 즉시 올바른 정렬로 빌드.
	bool single_line = (m_message.Find(_T('\n')) < 0);
	bool has_icon = (m_icon_index >= 0);
	m_static_message.set_halign((!has_icon && single_line) ? DT_CENTER : DT_LEFT);

	//메시지 너비, 높이에 따라 대화상자 크기를 재조정한다.
	//최소 cx = 240, cy = 140이며 cx의 최대 크기는 800으로 제한한다.
	//이 값을 넘으면 좌우가 잘리므로 호출 측에서 적절하게 '\n'을 넣어 줘야 함.
	if (m_auto_size)
	{
		//paragraph 의 자체 layout 으로 측정. 태그 strip, 폰트 크기 변화(<sz=N>), line_spacing 모두 반영.
		//GDI DrawText(DT_CALCRECT) 는 raw 텍스트(태그 포함)를 단일 폰트로 측정하므로 부정확.
		//측정 정확성을 위해 임시로 MAX_SIZE_CX 너비 + 충분한 높이로 키워 자연 폭으로 layout 하게 둠.
		CRect rmsg_temp(0, 0, MAX_SIZE_CX, 4000);
		m_static_message.MoveWindow(rmsg_temp);
		CRect rect_text = m_static_message.set_tagged_text(m_message);

		//right, bottom을 줄 때는 dlg의 최소 크기를 고려한다.
		//width는 MIN_SIZE_CX 가 기본이지만 메시지가 길어지면 아이콘과의 기본 간격 유지하며 MAX_SIZE_CX 까지 늘어남.
		int needed_width = icon_size + icon_msg_gap + rect_text.Width() + gap_side * 2;
		rc.right = MAX(needed_width + 48, MIN_SIZE_CX);
		if (rc.right > MAX_SIZE_CX)
			rc.right = MAX_SIZE_CX;

		rc.bottom = MAX(rc.top + gap + rect_text.Height() + gap + m_sz_button.cy + bottom_gap, MIN_SIZE_CY);

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
		//focus_rect 도 apply_theme 와 동일하게 cr_border_inactive 로 통일 — active/deactive 외관 일치 위함.
		for (int i = 0; i < TOTAL_BUTTON_COUNT; i++)
		{
			m_button[i].draw_focus_rect(true, m_theme.cr_border_inactive);
			//m_button[i].set_text_color(m_theme.cr_title_text);
			//m_button[i].set_back_color(m_theme.cr_title_back);
		}

		m_button_quit.set_text_color(m_theme.cr_title_text);
		//OnPaint 가 title bar 를 cr_title_back_inactive 로 그리므로 X 버튼도 같은 색이어야 blend.
		//이전엔 active 로 칠해 X 가 title bar 와 다른 색으로 도드라지는 문제 (특히 흰/회색 테마).
		//hover 는 Windows close button 의 빨간색 그대로.
		m_button_quit.set_back_color(m_theme.cr_title_back_inactive);
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
	m_static_message.ModifyStyle(0, m_align);

	//halign / set_text 는 auto_size 경로에서 이미 처리됨.
	//auto_size==false 일 때만 여기서 처리. MoveWindow 후이므로 OnSize 가 rebuild_layout 을 트리거함.
	if (!m_auto_size)
	{
		m_static_message.set_tagged_text(m_message);
	}
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
		//spacing-aware 총 높이 공식: 첫 줄(line_height) + (line_count-1) * line_height * spacing.
		//CSCStatic::reapply_line_spacings (단락 모드) 의 누적 shift 와 동일한 결과.
		float line_spacing = m_static_message.get_line_spacing();
		int text_block_height = line_height + (int)((line_count - 1) * line_height * line_spacing);
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
void CSCMessageBox::set_color_theme(int theme, bool invalidate)
{
	m_theme.set_color_theme(theme);
	apply_theme(invalidate);
}

//호출자가 이미 cr_back 등을 수정해 둔 CSCColorTheme 객체를 그대로 적용.
//operator= 가 아닌 copy_colors_from() 을 사용해 m_parent (= 본 msgbox) 는 보존.
void CSCMessageBox::set_color_theme(const CSCColorTheme& theme, bool invalidate)
{
	m_theme.copy_colors_from(theme);
	apply_theme(invalidate);
}

void CSCMessageBox::apply_theme(bool invalidate)
{
	if (m_static_message.m_hWnd)
	{
		m_static_message.set_text_color(m_theme.cr_text);
		m_static_message.set_back_color(m_theme.cr_back);

		//set_text_color 는 m_static_message.m_theme.cr_text 만 갱신 — 이미 set_tagged_text 로 파싱돼 m_para 안에
		//baked 된 untagged 세그먼트의 cr_text 는 *옛 테마의 색* 그대로 남는다 (예: dark 에서 set_message
		//후 default 로 전환 시 untagged 텍스트가 dark 의 light gray 로 잔존). 메시지가 비어있지 않으면
		//re-parse 해 새 cr_text 로 다시 stamping.
		if (!m_message.IsEmpty())
			m_static_message.set_tagged_text(m_message);
	}

	if (m_button_quit)
	{
		m_button_quit.set_text_color(m_theme.cr_title_text);
		//OnPaint 가 title bar 를 cr_title_back_inactive 로 그리므로 X 버튼도 같은 색이어야 blend.
		//이전엔 active 로 칠해 X 가 title bar 와 다른 색으로 도드라지는 문제 (특히 흰/회색 테마).
		//hover 는 Windows close button 의 빨간색 그대로.
		m_button_quit.set_back_color(m_theme.cr_title_back_inactive);
		m_button_quit.set_hover_back_color(gRGB(232, 17, 35));
	}

	for (int i = 0; i < TOTAL_BUTTON_COUNT; i++)
	{
		if (m_button[i].m_hWnd)
		{
			//hand-rolled 색 지정 대신 CGdiButton 자체의 set_color_theme 경로 사용 — main dlg 의
			//일반 GdiButton 과 *동일한* Win11-look 산출 공식 (face = cr_back +16, border = cr_back +40
			//for dark; light 는 -8 / -40) 을 그대로 타게 해 외관 일관성 확보.
			m_button[i].set_color_theme(m_theme);
			//focus_rect 끔 — 탭으로 focus 가 옮겨가도 두 버튼 외관이 동일하게 유지되도록.
			//(이전엔 cr_border_inactive 로 dotted overlay 를 그려 focused 만 다르게 보였음 — 사용자 지적 2026-05-21.
			// "탭은 별도 효과 OR 생략" 중 생략 선택.)
			m_button[i].draw_focus_rect(false);
		}
	}

	//dialog 자체의 OnPaint 가 cr_title_back_inactive / cr_back 등을 직접 사용하므로
	//invalidate=true 면 명시적으로 redraw 트리거. 자식 컨트롤들은 set_*_color 안에서 자체 invalidate 함.
	if (invalidate && m_hWnd)
		Invalidate();
}

LRESULT CSCMessageBox::on_message_CGdiButton(WPARAM wParam, LPARAM lParam)
{
	auto msg = (CGdiButtonMessage*)wParam;

	//[버그 수정] 과거에 여기서 "m_parent 가 NULL 이면 버튼의 GetParent() 로 채운다" 는 코드가 있었으나,
	//버튼의 parent = CSCMessageBox **자기 자신**. 결과적으로 m_parent = this 로 박혀버려
	//다음 DoModal 호출 시 m_parent->EnableWindow(FALSE) 가 *자기 자신을 disable* — 두 번째 메시지박스
	//가 visible 이지만 키보드/마우스 무응답이 되는 진짜 원인이었다.
	//m_parent 는 DoModal 의 lazy binding 에서만 설정한다.

	if (msg->message == WM_LBUTTONUP)
	{
		//TRACE(_T("on_message_CGdiButton, WM_LBUTTONUP = %s\n"), m_button_caption[msg->ctrl_id - SC_BUTTON_ID]);
		m_response = msg->ctrl_id - SC_BUTTON_ID;

		if (!m_as_modal)
		{
			if (m_parent && ::IsWindow(m_parent->m_hWnd))
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
				return TRUE;

			case VK_RETURN :
				//모든 버튼이 CGdiButton 으로 동적 생성되어 표준 BS_DEFPUSHBUTTON 이 없다.
				//엔터키는 여기서 직접 m_type 에 따라 기본 응답을 결정해 종료한다.
				switch (m_type & MB_TYPEMASK)
				{
				case MB_YESNO :
				case MB_YESNOCANCEL :
					m_response = IDYES; break;
				case MB_RETRYCANCEL :
				case MB_ABORTRETRYIGNORE :
					m_response = IDRETRY; break;
				default :
					m_response = IDOK;  break;
				}

				//우리 popup 은 CreateEx 로 만들었으므로 ::EndDialog 호출은 undefined behavior.
				//modal: m_response 만 설정 → 펌프 루프 자체 종료 → DoModal 끝에서 ShowWindow(SW_HIDE).
				//modeless: parent 에 통지 + 즉시 hide.
				if (!m_as_modal)
				{
					if (m_parent && ::IsWindow(m_parent->m_hWnd))
						::SendMessage(m_parent->m_hWnd, Message_CSCMessageBox, (WPARAM)this, m_response);
					ShowWindow(SW_HIDE);
				}
				return TRUE;
		}
	}

	return CDialogEx::PreTranslateMessage(pMsg);
}

void CSCMessageBox::OnBnClickedOk()
{
	m_response = IDOK;
	//modal: m_response 만 설정 → 펌프 종료 → DoModal 끝에서 ShowWindow(SW_HIDE).
	//modeless: 즉시 hide.
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

	//임시 인스턴스 사용 패턴 지원 — create() 가 호출되지 않았으면 기본 인자로 자동 생성.
	//생성자에서 create 를 못 부르는 이유(글로벌 ctor 가 MFC 초기화 전에 실행)는 여기 해당 없음 —
	//DoModal 은 항상 InitInstance 이후 호출되므로 안전.
	//theApp.m_msgbox 처럼 이미 create() 된 케이스는 create() 내부의 m_hWnd 가드로 no-op.
	if (m_hWnd == NULL)
	{
		if (!create(nullptr, _T(""), 0, true))
			return -1;
	}

	//m_parent lazy binding — *반드시 우리 프로세스의 윈도우만* 채택한다.
	//AfxGetMainWnd() / GetForegroundWindow() / GetActiveWindow() 는 InitInstance 처럼 우리 프로세스
	//main 윈도우가 없는 시점에는 **다른 프로세스의 foreground 윈도우**를 반환한다 (예: VS, 탐색기).
	//그것을 owner 로 잡으면 SetWindowLongPtr(GWLP_HWNDPARENT) 가 외부 프로세스 윈도우를 owner 로
	//지정하고, EnableWindow(FALSE) 가 *다른 프로세스의 윈도우* 를 disable 한다. 두 번째 DoModal
	//호출에서 외부 윈도우 상태가 변해 activation 흐름이 깨지는 게 그동안의 "두번째 메시지박스 무응답"
	//진짜 원인이었다.
	DWORD this_pid = ::GetCurrentProcessId();
	auto is_our_process = [&](CWnd* w) -> bool
	{
		if (w == nullptr || w->m_hWnd == NULL) return false;
		DWORD pid = 0;
		::GetWindowThreadProcessId(w->m_hWnd, &pid);
		return pid == this_pid;
	};

	if (m_parent != nullptr && !is_our_process(m_parent))
		m_parent = nullptr;     //이전 호출 캐시가 외부 윈도우면 버린다.

	if (m_parent == nullptr)
		m_parent = GetParent();
	if (!is_our_process(m_parent))
		m_parent = nullptr;

	if (m_parent == nullptr)
	{
		CWnd* cand = AfxGetMainWnd();
		if (is_our_process(cand))
			m_parent = cand;
	}
	if (m_parent == nullptr)
	{
		CWnd* cand = CWnd::GetActiveWindow();
		if (is_our_process(cand))
			m_parent = cand;
	}
	//여전히 NULL 이면 NULL 로 둔다. 아래 호출들은 m_parent NULL 가드되어 있다 —
	//우리 프로세스에 유효한 owner 가 없으면 owner-less top-level popup 으로 동작.

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

	//owner 가 WS_EX_TOPMOST 면 messagebox 도 TOPMOST 로 띄운다.
	//그렇지 않으면 owner 가 z-order 상 항상 위라 messagebox 가 visible/active 더라도 owner 뒤에 가려져
	//mouse hit-test 가 messagebox 에 도달하지 못한다 (키보드는 active 라 동작, 마우스만 안 되는 증상).
	//(SetWindowPos 는 SWP_NOACTIVATE 없이 호출 — 활성화도 같이 처리.)
	bool parent_topmost = (m_parent && m_parent->m_hWnd
		&& (::GetWindowLong(m_parent->m_hWnd, GWL_EXSTYLE) & WS_EX_TOPMOST));
	::SetWindowPos(m_hWnd, parent_topmost ? HWND_TOPMOST : HWND_TOP,
		0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);

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
		if (!PeekMessage(&stmsg, NULL, 0, 0, PM_REMOVE))
		{
			//WaitMessage 없이 PeekMessage 만 돌리면 CPU 100% 스핀. 메시지 큐가 빌 때 대기.
			WaitMessage();
			continue;
		}

		if (stmsg.message == WM_QUIT)
		{
			::PostQuitMessage((int)stmsg.wParam);
			break;
		}

		//MFC 표준 PreTranslate 트리 워킹 — 포커스 받은 윈도우부터 m_hWnd 까지 위로 올라가며
		//각 윈도우의 PreTranslateMessage 를 호출한다. 이래야 CGdiButton 의 VK_SPACE 처리
		//(포커스된 버튼을 클릭처럼 처리) 와 본 dialog 의 VK_RETURN/VK_ESCAPE 처리가 모두 동작한다.
		//단순히 this->PreTranslateMessage 만 호출하면 자식 컨트롤의 PreTranslate 는 누락됨.
		if (CWnd::WalkPreTranslateTree(m_hWnd, &stmsg))
			continue;

		TranslateMessage(&stmsg);
		DispatchMessage(&stmsg);
	}

	if (m_parent && m_parent->m_hWnd)
		m_parent->EnableWindow(TRUE);

	//::EndDialog 는 DialogBox/DialogBoxIndirect 로 만든 윈도우 전용 (MSDN: "must not be used
	//for any other purpose"). 우리 popup 은 CreateEx 로 만들었으므로 ShowWindow(SW_HIDE).
	//두 번째 DoModal 호출 시 무응답이던 버그의 직접 원인.
	ShowWindow(SW_HIDE);

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
