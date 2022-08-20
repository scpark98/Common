// NewWizPage.cpp : implementation file
//

#include "stdafx.h"
#include "NewWizPage.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CNewWizPage dialog

CNewWizPage::CNewWizPage(LPCTSTR lpszTemplateName, CWnd* pParent)
:CDialog(lpszTemplateName,pParent)
{
	//{{AFX_DATA_INIT(CNewWizPage)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
	m_bCreated		= FALSE;
	m_bActive		= FALSE;
	m_nDialogID		= 0;
	m_pParent		= NULL; 
	m_bInitialized	= FALSE;
	m_bPaused		= FALSE;
}


CNewWizPage::CNewWizPage(UINT nIDTemplate, CWnd* pParent)
:CDialog(nIDTemplate,pParent)
{
	m_bCreated		= FALSE;
	m_bActive		= FALSE;
	m_nDialogID		= nIDTemplate;
	m_pParent		= NULL; 
	m_bInitialized	= FALSE;
}

CNewWizPage::~CNewWizPage()
{
}



void CNewWizPage::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CNewWizPage)
		// NOTE: the ClassWizard will add DDX and DDV calls here
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CNewWizPage, CDialog)
	//{{AFX_MSG_MAP(CNewWizPage)
	ON_WM_LBUTTONUP()
	ON_WM_RBUTTONUP()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CNewWizPage message handlers


BOOL CNewWizPage::OnInitDialog() 
{
	CDialog::OnInitDialog();
	
	// create the large font
/*	m_LargeFont.CreateFont(-16, 0, 0, 0, 
		FW_BOLD, FALSE, FALSE, 0, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, 
		CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH, _T("MS Sans Serif"));
*/
	
	DWORD style = GetStyle();
	ASSERT((style & WS_CHILD) != 0);
	ASSERT((style & WS_BORDER) == 0);
//	ASSERT((style & WS_DISABLED) != 0);
	
	//m_Brush.CreateSolidBrush(RGB(255, 255, 255));
	m_bInitialized	= TRUE;

	return TRUE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}


BOOL CNewWizPage::OnCreatePage()
{
	m_bCreated = TRUE;
	return TRUE;
}

void CNewWizPage::OnDestroyPage()
{
}

/*
HBRUSH CNewWizPage::OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor) 
{
	//	HBRUSH hbr = CDialog::OnCtlColor(pDC, pWnd, nCtlColor);
	
	switch (nCtlColor)
	{	   
	case CTLCOLOR_STATIC:
		pDC->SetTextColor(RGB(0, 0, 0));
		pDC->SetBkColor(RGB(255,255,255));
	case CTLCOLOR_EDIT:
		pDC->SetTextColor(RGB(0, 0, 0));
		pDC->SetBkColor(RGB(255,255,255));
	case CTLCOLOR_LISTBOX:
		pDC->SetTextColor(RGB(0, 0, 0));
		pDC->SetBkColor(RGB(255,255,255));
	case CTLCOLOR_SCROLLBAR:
		pDC->SetTextColor(RGB(0, 0, 0));
		pDC->SetBkColor(RGB(255,255,255));
	case CTLCOLOR_BTN:
		pDC->SetTextColor(RGB(0, 0, 0));
		pDC->SetBkColor(RGB(255,255,255));
	case CTLCOLOR_DLG:	    
		return m_Brush;
	}	
	
	// TODO: Return a different brush if the default is not desired
	return m_Brush;
}
*/
/*
// refer to CPropertyPage class members for a description of this function
void CNewWizPage::OnCancel()
{
}
*/

// refer to CPropertyPage class members for a description of this function
BOOL CNewWizPage::OnKillActive()
{
	m_bActive = FALSE;
	return TRUE;
}



// called when the page is set active
void CNewWizPage::OnSetActive()
{
	m_bActive = TRUE;
}



// refer to CPropertyPage class members for a description of this function
BOOL CNewWizPage::OnQueryCancel()
{
	return TRUE;
}


// refer to CPropertyPage class members for a description of this function
// return -1 to prevent changing pages
// to move to a page other than the previous one, return the 
// Dialog resource ID of the page to display
LRESULT CNewWizPage::OnWizardBack()
{
	return 0;
}


// refer to CPropertyPage class members for a description of this function
// return -1 to prevent changing pages
// to move to a page other than the next one, return the 
// Dialog resource ID of the page to display
LRESULT CNewWizPage::OnWizardNext()
{
	return 0;
}

void CNewWizPage::OnMasterEvent( int nEvent, WPARAM wParam, LPARAM lParam )
{
}

void CNewWizPage::LoadImageSoundFiles()
{
}

void CNewWizPage::OnPenInputEvent( BOOL bRight, BOOL bDown, int x, int y )
{
}

// refer to CPropertyPage class members for a description of this function
BOOL CNewWizPage::OnWizardFinish()
{
	return TRUE;
}

void CNewWizPage::OnLButtonUp(UINT nFlags, CPoint point) 
{
	// TODO: Add your message handler code here and/or call default
//	m_pParent->SetNextPage();

	CDialog::OnLButtonUp(nFlags, point);
}

void CNewWizPage::OnRButtonUp(UINT nFlags, CPoint point) 
{
	// TODO: Add your message handler code here and/or call default
/*	if ( m_pParent->GetActiveIndex() == 0 )
		return;

	m_pParent->SetPreviousPage();
*/	
	CDialog::OnRButtonUp(nFlags, point);
}

BOOL CNewWizPage::PreTranslateMessage(MSG* pMsg) 
{
	// TODO: Add your specialized code here and/or call the base class
	if ( pMsg->message == WM_KEYDOWN )
	{
		return m_pParent->PreTranslateMessage(pMsg);
/*
//		AfxMessageBox( "WizPage" );
		switch ( pMsg->wParam )
		{
			case VK_LEFT	:
			case VK_RIGHT	:
			case VK_UP		:
			case VK_DOWN	:	
			case VK_RETURN	:	return m_pParent->PreTranslateMessage(pMsg);

			default			:	return m_pParent->PreTranslateMessage(pMsg);
		}

		return TRUE;
*/	}
	
	return CDialog::PreTranslateMessage(pMsg);
}

void CNewWizPage::OnCancel()
{
}

void CNewWizPage::MoveControlPosition( UINT nKey )
{
	CWnd*	pWnd = GetFocus();

	if ( !pWnd )
		return;

	CRect	Rect;
	CPoint	pt;

	if ( nKey == VK_LEFT )
		pt = CPoint( -1,  0 );
	else if ( nKey == VK_RIGHT )
		pt = CPoint( +1,  0 );
	else if ( nKey == VK_UP )
		pt = CPoint(  0, -1 );
	else if ( nKey == VK_DOWN )
		pt = CPoint(  0, +1 );

	pWnd->GetWindowRect( Rect );

	if ( GetKeyState( VK_CONTROL ) & 0x80000000 )
		Rect.OffsetRect( pt.x * 10, pt.y * 10 );
	else
		Rect.OffsetRect( pt );

	pWnd->MoveWindow( Rect );
}

void CNewWizPage::DisplayControlPosition()
{
	CWnd*	pWnd = GetFocus();

	if ( !pWnd )
		return;

	CRect	Rect;

	pWnd->GetWindowRect( Rect );

	CString str;

	str.Format( "%d, %d, %d, %d ( %d x %d )", Rect.left, Rect.top, Rect.right, Rect.bottom, Rect.Width(), Rect.Height() );
	AfxMessageBox( str );
}
