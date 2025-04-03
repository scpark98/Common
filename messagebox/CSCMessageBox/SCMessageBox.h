#pragma once

#include "afxdialogex.h"

#include "../../colors.h"
#include "../../CButton/GdiButton/GdiButton.h"
#include "../../CStatic/SCStatic/SCStatic.h"

// CSCMessageBox 대화 상자
#define DEFAULT_SIZE_CX				400
#define DEFAULT_SIZE_CY				240
#define DEFAULT_BUTTON_CX			84
#define DEFAULT_BUTTON_CY			28
#define DEFAULT_TITLE_HEIGHT		24

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

//표시할 메시지 및 옵션 설정
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

	CGdiButton		m_button_ok;
	CGdiButton		m_button_cancel;
	CGdiButton		m_button_retry;
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
	virtual INT_PTR DoModal(CString msg = _T(""), int type = MB_OK, int timeout = 0);
	afx_msg void OnPaint();
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnTimer(UINT_PTR nIDEvent);
};
