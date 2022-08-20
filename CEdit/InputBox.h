
#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include <Afxwin.h>
//scpark. √‚√≥ codeproject.com


class CInputBox : public CFrameWnd
{
	DECLARE_DYNCREATE(CInputBox)
public:
	CInputBox(CWnd *pParent); 
	CInputBox(); 

private:
	CEdit m_edit;
	CStatic m_prompt;
	CButton m_ok;
	CButton m_cancel;
	CStatic m_dummy;

	CString strPrompt;
	CString strTitle;
	CString strDefault;

	int retvalue;


public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CInputBox)
	//}}AFX_VIRTUAL

// Implementation
public:
	void CloseBox();
	CString InputText;
	int ShowInputBox(CString,CString,CString);
	virtual ~CInputBox();

	// Generated message map functions
	//{{AFX_MSG(CInputBox)
	afx_msg void OnClose();
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnOk();
	afx_msg void OnCancel();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
private:		
	CWnd* pParent;
};



//{{AFX_INSERT_LOCATION}}

