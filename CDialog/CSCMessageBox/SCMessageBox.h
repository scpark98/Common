#pragma once

#include "afxdialogex.h"
#include "../../CButton/GdiButton/GdiButton.h"
#include "../../CStatic/SCStatic/SCStatic.h"

// CSCMessageBox 대화 상자

class CSCMessageBox : public CDialogEx
{
	DECLARE_DYNAMIC(CSCMessageBox)

public:
	CSCMessageBox(CWnd* parent = NULL, int cx = 320, int cy = 200);   // 표준 생성자입니다.
	virtual ~CSCMessageBox();

	bool			create(CWnd* parent, int cx = 320, int cy = 200);
	void			set_title(CString title);

	//표시할 메시지 및 옵션 설정
	void			set_message(CString msg, int type = MB_OK, int timeout = 0, int align = DT_CENTER | DT_VCENTER);

	LRESULT			on_message_CGdiButton(WPARAM wParam, LPARAM lParam);

protected:
	CWnd*			m_parent = NULL;
	CString			m_title;
	CString			m_message;
	int				m_type = MB_OK;
	int				m_timeout = 0;
	int				m_align = DT_CENTER | DT_VCENTER;

	int				m_response = IDOK;

	int				m_title_height = 32;

	CSize			m_sz_button = CSize(100, 24);

	CSCStatic		m_static_message;
	CGdiButton		m_button_ok;
	CGdiButton		m_button_cancel;
	CGdiButton		m_button_retry;
	CGdiButton		m_button_exit;

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
	virtual INT_PTR DoModal();
	afx_msg void OnPaint();
};
