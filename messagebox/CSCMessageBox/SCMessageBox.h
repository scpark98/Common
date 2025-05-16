#pragma once

/*
* 20250330 scpark
  - AfxMessageBox(), ::MessageBox를 사용하면 타이틀바 커스텀이 불가하므로 새로 구현함.
  - Resource에 IDD_ 와 같은 리소스가 필요없이 동적 생성함.
  - 사용법 및 옵션은 대부분 AfxMessageBox()와 동일함.
  - ColorTheme 역시 CColorTheme을 공통으로 사용하므로
	parent의 ColorTheme을 설정해주면 parent와 동일한 테마로 동작함.

  - 사용법은 아래와 같음.
	* parent의 .h에 CSCMessageBox m_messagebox;를 선언해서 사용해도 되고
	  매번 사용할 때마다 인스턴스 선언 후 dlg.DoModal()을 호출해도 되지만 전자를 권장함.
	  왜냐하면 m_messagebox.set_color_theme(CSCColorTheme::color_theme_linkmemine);과 같이 테마를 설정하면
	  해당 프로젝트에서는 지정한 테마가 적용되므로 매번 테마를 설정할 필요가 없음.

	1.AfxMessageBox()와 같이 Modal()로 띠울 경우
		//OnInitDialog() 등에서 아래와 같이 생성해준 후
		m_messagebox.create(this, _T("Title Text"), IDR_MAINFRAME);
		m_messagebox.set_color_theme(CSCColorTheme::color_theme_linkmemine);
		...
		//필요한 곳에서 DoModal()로 호출해서 사용.
		int res = m_messagebox.DoModal(_T("Test MessageBox"));	//선택된 버튼이 res로 넘어옴. if (res == MB_OK) ...

	2.Modeless로 띠울 경우
		//OnInitDialog() 등에서 아래와 같이 생성해준 후
		- m_messagebox.create(this, _T("Title Text"), IDR_MAINFRAME, false);
		- m_messagebox.set_color_theme(CSCColorTheme::color_theme_linkmemine);
		...
		//필요한 곳에서 set_message()로 메시지박스 표시
		- m_messagebox.set_message(_T("Test MessageBox"), MB_OKCANCEL);	//버튼 선택값은 메시지를 통해 parent로 전달됨.
	  
	* text의 width에 따라 어느 정도까지는 자동으로 넓혀지지만 MAX에 다다르면 잘리게 되는데
	  WORDWRAP으로 처리해보고자 했으나 로직이 많이 복잡해지므로 굳이 처리하지 않음.
	  text가 길어서 잘릴듯하다면 '\n'을 넣어서 멀티라인으로 출력할 것!

* 
* 
*/

#include "afxdialogex.h"

#include "../../colors.h"
#include "../../CButton/GdiButton/GdiButton.h"
#include "../../CStatic/SCStatic/SCStatic.h"

//modeless로 생성한 경우 클릭된 버튼의 응답결과를 parent로 전달하기 위한 메시지
static const UINT Message_CSCMessageBox = ::RegisterWindowMessage(_T("MessageString_CSCMessageBox"));

// CSCMessageBox 대화 상자
#define DEFAULT_SIZE_CX				440
#define DEFAULT_SIZE_CY				240
#define DEFAULT_BUTTON_CX			84
#define DEFAULT_BUTTON_CY			28
#define DEFAULT_TITLE_HEIGHT		24

#define TOTAL_BUTTON_COUNT			12
#define SC_BUTTON_ID				(WM_USER + 123)	//#define의 계산식은 반드시 괄호로 묶어주는 것을 잊지 않아야 한다.

/*
* 각 버튼의 ID는 winuser.h에 정의된 것과 동일하게 사용한다.
#define IDOK                1
#define IDCANCEL            2
#define IDABORT             3
#define IDRETRY             4
#define IDIGNORE            5
#define IDYES               6
#define IDNO                7
#if(WINVER >= 0x0400)
#define IDCLOSE         8
#define IDHELP          9
#endif
#if(WINVER >= 0x0500)
#define IDTRYAGAIN      10
#define IDCONTINUE      11
*/

class CSCMessageBox : public CDialogEx
{
	DECLARE_DYNAMIC(CSCMessageBox)

public:
	CSCMessageBox(CWnd* parent = NULL, CString title = _T(""), UINT icon_id = 0, bool as_modal = true, int cx = -1, int cy = -1);   // 표준 생성자입니다.
	virtual ~CSCMessageBox();

	//사용할 클래스의 .h에서 멤버변수로 선언하고
	//.cpp의 OnInitDialog()에서 create() 및 필요한 설정을 하면
	//그 클래스의 어디서든 ret = DoModal(_T("message");처럼 호출해서사용할 수 있다.

	bool			create(CWnd* parent, CString title, UINT icon_id = 0, bool as_modal = true, int cx = -1, int cy = -1);

//title 관련
	void			set_title(CString title);
	void			set_title_height(int title_height) { m_title_height = title_height; }
	void			set_title_icon(UINT icon_id);

//Modeless로 실행할 경우 호출. //실제 사용 시 MAX_WIDTH(800)를 넘을 경우는 좌우가 잘리므로 적절하게 '\n'을 넣어준다.
	//set_message(_T("changed message"));와 같이 동적으로 메시지만 변경할 경우는 type, timeout_sec, align은 기본 음수를 가지며
	//음수일 경우는 기본값 또는 이미 설정된 값을 사용한다.
	void			set_message(CString msg, int type = -1, int timeout = -1, int align = -1);
	void			set_align(int align);

//theme 관련 설정
	CSCColorTheme	m_theme = CSCColorTheme(this);
	void			set_color_theme(int theme);

	//normal message일 경우는 theme에 지정된 title_back_color를 사용하지만 이 함수를 사용하면
	//question은 연두색, warning은 주황색, error는 핑크색, info는 하늘색으로 표시된다.
	//타이틀바의 글자색은 검정으로 고정된다.
	void			use_typed_title_back_color(bool use = true) { m_use_typed_title_back_color = use; }

protected:
	enum TIMER_ID
	{
		timer_timeout = 0,
	};

	bool			m_as_modal = true;		//기본값은 modal로 동작하지만 이 값이 false이면 modeless로 동작함
	CWnd*			m_parent = NULL;
	HICON			m_hIcon = NULL;
	CString			m_message;
	int				m_type = MB_OK;
	int				m_timeout_sec = 0;
	int				m_align = SS_CENTER | SS_CENTERIMAGE;

	int				m_response = -1;

	CString			m_title;
	int				m_title_height = DEFAULT_TITLE_HEIGHT;

	CSize			m_sz_button = CSize(DEFAULT_BUTTON_CX, DEFAULT_BUTTON_CY);

	CSCStatic		m_static_message;

	CString			m_button_caption[TOTAL_BUTTON_COUNT];
	CGdiButton		m_button[TOTAL_BUTTON_COUNT];	//UNDEF(0), IDOK(1) ~ IDCONTINUE(11)
	CGdiButton		m_button_quit;
	LRESULT			on_message_CGdiButton(WPARAM wParam, LPARAM lParam);

//messagebox icon
//https://blog.naver.com/pks1217/220407691110
	//MB_ICONSTOP, MB_ICONQUESTION, MB_ICONEXCLAMATION, MB_ICONINFORMATION
	int				m_icon_index = 3;
	HICON			m_icons[4];

//font
	LOGFONT			m_lf;
	CFont			m_font;
	void			reconstruct_font();

//layout
	bool			m_auto_size = true;
	void			recalc_layout();

//theme
	//true이면 question은 연두색, error는 핑크색, info는 하늘색, warning은 주황색으로 표시된다.
	bool			m_use_typed_title_back_color = false;

// 대화 상자 데이터입니다.
//#ifdef AFX_DESIGN_TIME
//	enum { IDD =  };
//#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 지원입니다.

	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnBnClickedOk();
	afx_msg void OnBnClickedCancel();
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	//실제 사용 시 MAX_WIDTH(800)를 넘을 경우는 좌우가 잘리므로 적절하게 '\n'을 넣어준다.
	virtual INT_PTR DoModal(CString msg = _T(""), int type = MB_OK, int timeout = 0);
	afx_msg void OnPaint();
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnTimer(UINT_PTR nIDEvent);
};
