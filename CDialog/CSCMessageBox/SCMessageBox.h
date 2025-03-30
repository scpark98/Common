#pragma once

#include "afxdialogex.h"

#include "../../colors.h"
#include "../../CButton/GdiButton/GdiButton.h"
#include "../../CStatic/SCStatic/SCStatic.h"

// CSCMessageBox 대화 상자
#define DEFAULT_SIZE_CX		400
#define DEFAULT_SIZE_CY		240

class CSCMessageBox : public CDialogEx
{
	DECLARE_DYNAMIC(CSCMessageBox)

public:
	CSCMessageBox(CWnd* parent = NULL, CString title = _T(""), UINT icon_id = 0, int cx = DEFAULT_SIZE_CX, int cy = DEFAULT_SIZE_CY);   // 표준 생성자입니다.
	virtual ~CSCMessageBox();

	//사용할 클래스의 .h에서 멤버변수로 선언하고
	//.cpp의 OnInitDialog()에서 create() 및 필요한 설정을 하면
	//그 클래스의 어디서든 ret = DoModal(_T("message");처럼 호출해서사용할 수 있다.

	bool			create(CWnd* parent, CString title, UINT icon_id = 0, int cx = DEFAULT_SIZE_CX, int cy = DEFAULT_SIZE_CY);
	void			set_title(CString title);
	void			set_icon(UINT icon_id);

	//표시할 메시지 및 옵션 설정
	void			set_message(CString msg, int type = MB_OK, int timeout = 0, int align = DT_CENTER | DT_VCENTER);

	CSCColorTheme	m_theme = CSCColorTheme(this);
	void			set_color_theme(int theme);

protected:
	CWnd*			m_parent = NULL;
	HICON			m_hIcon = NULL;
	CString			m_message;
	int				m_type = MB_OK;
	int				m_timeout = 0;
	int				m_align = DT_CENTER | DT_VCENTER;

	int				m_response = IDOK;

	CString			m_title;
	int				m_title_height = 32;

	CSize			m_sz_button = CSize(120, 32);

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
	virtual INT_PTR DoModal(CString msg = _T(""), int type = MB_OK, int timeout = 0, int align = SS_CENTER | SS_CENTERIMAGE);
	afx_msg void OnPaint();
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
};
