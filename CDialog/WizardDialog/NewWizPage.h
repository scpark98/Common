#if !defined(AFX_NEWWIZPAGE_H__915D4C8B_2E27_11D4_9FA9_0030DB0011C6__INCLUDED_)
#define AFX_NEWWIZPAGE_H__915D4C8B_2E27_11D4_9FA9_0030DB0011C6__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// NewWizPage.h : header file
//

#include "NewWizDialog.h"
//class CNewWizDialog;

/////////////////////////////////////////////////////////////////////////////
// CNewWizPage dialog

class CNewWizPage : public CDialog
{
	friend class CNewWizDialog;
	
	// Construction
public:
	CNewWizPage(CWnd* pParent = NULL);   // standard constructor
	CNewWizPage(LPCTSTR lpszTemplateName, CWnd* pParent = NULL);
	CNewWizPage(UINT nIDTemplate, CWnd* pParent = NULL);
	
	virtual ~CNewWizPage();
	virtual BOOL	OnCreatePage();
	virtual void	OnDestroyPage();
	
	// these functions are the same as CPropertyPage
	virtual	BOOL	IsActive() { return m_bActive; }
	virtual BOOL	IsInitialized() { return m_bInitialized; }
	virtual void	OnCancel();
	virtual BOOL	OnKillActive();
	virtual void	OnSetActive();
	virtual BOOL	OnQueryCancel( );
	virtual LRESULT OnWizardBack();
	virtual LRESULT OnWizardNext();
	virtual BOOL	OnWizardFinish();
	virtual void	OnMasterEvent( int nEvent, WPARAM wParam = 0, LPARAM lParam = 0 );
	virtual void	LoadImageSoundFiles();
	virtual void	OnPenInputEvent( BOOL bRight, BOOL bDown, int x, int y );

	void			MoveControlPosition( UINT nKey );
	void			DisplayControlPosition();
	
	// Attributes
public:
	//CFont			m_LargeFont; // a large font for any titles we may want
	BOOL			m_bCreated; // flag to tell us if the dialog has been created

	//scpark 2010-3-10 11:13:33
	//각 페이지마다 카운트다운이 진행중인지, 일시정지상태인지.
	BOOL			m_bPaused;
	CString			m_sDesignPath;
	CString			m_sSoundPath;


protected:
	//CBrush		m_Brush; // brush for white background
	CNewWizDialog	*m_pParent; // Parent dialog
	
private:
	UINT			m_nDialogID;	// resource ID for thie page
	BOOL			m_bActive;		// flag to tell is if the dialog is the active page
	BOOL			m_bInitialized;	//적어도 한번은 페이지가 로딩됨.

	// Operations
public:
	
	// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CNewWizPage)
	public:
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL
	
	// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(CNewWizPage)
	virtual BOOL OnInitDialog();
	afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnRButtonUp(UINT nFlags, CPoint point);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_NEWWIZPAGE_H__915D4C8B_2E27_11D4_9FA9_0030DB0011C6__INCLUDED_)
