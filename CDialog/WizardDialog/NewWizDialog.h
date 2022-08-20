#if !defined(AFX_NEWWIZDIALOG_H__915D4C89_2E27_11D4_9FA9_0030DB0011C6__INCLUDED_)
#define AFX_NEWWIZDIALOG_H__915D4C89_2E27_11D4_9FA9_0030DB0011C6__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// NewWizDialog.h : header file
//

//#include "newwizpage.h"
class CNewWizPage;

/////////////////////////////////////////////////////////////////////////////
// CNewWizDialog dialog

class CNewWizDialog : public CDialog
{
	// Construction
public:
	CNewWizDialog (LPCTSTR lpszTemplate, CWnd* pParent = NULL);
	CNewWizDialog (UINT nIDTemplate, CWnd* pParent = NULL);
	virtual ~CNewWizDialog ();
	
	
	// Attributes
public:

protected:
	CObList		m_PageList; // this list of CNewWizPage objects
	UINT		m_nPlaceholderID; // control ID for the palceholder
	
	// Operations
public:
	void AddPage(CNewWizPage* pPage, UINT nID);
	
	BOOL SetActivePageByResource(UINT nResourceID);
	BOOL SetFirstPage();
	void SetNextPage();
	void SetPreviousPage();
	
	void EnableFinish(BOOL bEnable);
	void EnableBack(BOOL bEnable);
	void EnableNext(BOOL bEnable);
	int GetActiveIndex() const;
	int GetPageIndex(CNewWizPage* pPage) const;
	int GetPageCount();
	CNewWizPage* GetPage(int nPage) const;
	CNewWizPage* GetPageByResourceID(UINT nResourceID);
	CNewWizPage* GetActivePage() const;
	BOOL SetActivePage(int nPage);
	BOOL SetActivePage(CNewWizPage* pPage);
	void SetTitle(LPCTSTR lpszText);
	void SetTitle(UINT nIDText);
	void SetFinishText(LPCTSTR lpszText);
	void SetFinishText(UINT nIDText);
	void FinishWizard();							// masterdlg�� �����Ų��.

protected:
	BOOL ActivatePage(CNewWizPage* pPage);
	BOOL DeactivatePage();
	void SetPlaceholderID(int nPlaceholderID);
	virtual void OnWizardFinish();
	virtual void OnWizardBack(); 
	virtual void OnWizardNext(); 
	virtual void OnCancel(); 
	
private:
	void Init();
	BOOL DestroyPage(CNewWizPage* pPage);
	CNewWizPage* GetFirstPage();
	CNewWizPage* GetLastPage();
	CNewWizPage* GetNextPage();
	
	
	// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CNewWizDialog)
	public:
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL
	
	// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(CNewWizDialog)
	afx_msg void OnDestroy();
	virtual BOOL OnInitDialog();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_NEWWIZDIALOG_H__915D4C89_2E27_11D4_9FA9_0030DB0011C6__INCLUDED_)
