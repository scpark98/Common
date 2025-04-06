#pragma once

/*
* 20250330 scpark
  - AfxMessageBox(), ::MessageBox를 사용하면 타이틀바 커스텀이 불가하므로 새로 구현함.
  - Resource에 IDD_ 와 같은 리소스가 필요없이 동적 생성함.
  - 사용법 및 옵션은 대부분 AfxMessageBox()와 동일함.
  - ColorTheme 역시 CColorTheme을 공통으로 사용하므로
	parent의 ColorTheme을 설정해주면 parent와 동일한 테마로 동작함.
  - 사용법은 아래와 같음.
	* parent의 .h에 CSCMessageBox m_message;를 선언해서 사용해도 되고
	  매번 사용할 때마다 인스턴스 선언 후 dlg.DoModal()을 호출해도 된다.
	1.AfxMessageBox()와 같이 Modal()로 띠울 경우
	  

* 
*/

#include "afxdialogex.h"

#include "../../colors.h"
#include "../../CButton/GdiButton/GdiButton.h"
#include "../../CStatic/SCStatic/SCStatic.h"

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

	bool			create(CWnd* parent, CString title, UINT icon_id = 0, int cx = -1, int cy = -1);

//title 관련
	void			set_title(CString title);
	void			set_title_height(int title_height) { m_title_height = title_height; }
	void			set_icon(UINT icon_id);

//Modeless로 실행할 경우 호출. //실제 사용 시 MAX_WIDTH(800)를 넘을 경우는 좌우가 잘리므로 적절하게 '\n'을 넣어준다.
	void			set_message(CString msg, int type = MB_OK, int timeout = 0, DWORD align = SS_CENTER | SS_CENTERIMAGE);
	void			set_align(DWORD align);

//theme 관련 설정
	CSCColorTheme	m_theme = CSCColorTheme(this);
	void			set_color_theme(int theme);

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
	DWORD			m_align = SS_CENTER | SS_CENTERIMAGE;

	int				m_response = -1;

	CString			m_title;
	int				m_title_height = DEFAULT_TITLE_HEIGHT;

	CSize			m_sz_button = CSize(DEFAULT_BUTTON_CX, DEFAULT_BUTTON_CY);

	CSCStatic		m_static_message;

	CString			m_button_caption[TOTAL_BUTTON_COUNT];
	CGdiButton		m_button[12];	//UNDEF(0), IDOK(1) ~ IDCONTINUE(11)
	CGdiButton		m_button_quit;
	LRESULT			on_message_CGdiButton(WPARAM wParam, LPARAM lParam);

//font
	LOGFONT			m_lf;
	CFont			m_font;
	void			reconstruct_font();

//layout
	bool			m_auto_size = true;
	void			recalc_layout();


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
