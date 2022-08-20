// AutoRepeatButton.cpp : implementation file
//

#include "stdafx.h"
#include "AutoRepeatButton.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


#define TIMER_REPEAT	0


/////////////////////////////////////////////////////////////////////////////
// CAutoRepeatButton

CAutoRepeatButton::CAutoRepeatButton()
{
	m_nInitialDelay		= 1;
	m_nRepeatDelay		= 100;
	m_bUseAutoRepeat	= true;
}

CAutoRepeatButton::~CAutoRepeatButton()
{
}


BEGIN_MESSAGE_MAP(CAutoRepeatButton, CButton)
	//{{AFX_MSG_MAP(CAutoRepeatButton)
	ON_WM_LBUTTONDOWN()
	ON_WM_LBUTTONUP()
	ON_WM_TIMER()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CAutoRepeatButton message handlers

void CAutoRepeatButton::OnLButtonDown(UINT nFlags, CPoint point) 
{
	GetParent()->SendMessage(WM_COMMAND, MAKELONG(GetDlgCtrlID(), BN_CLICKED), (LPARAM)m_hWnd);

	if ( m_bUseAutoRepeat )
		SetTimer( TIMER_REPEAT, m_nInitialDelay, NULL);

	sent = 0;

	CButton::OnLButtonDown(nFlags, point);
}

void CAutoRepeatButton::OnLButtonUp(UINT nFlags, CPoint point) 
{
	KillTimer( TIMER_REPEAT );

	if(GetCapture() != NULL)
	{ /* release capture */
		ReleaseCapture();
		if(sent == 0 && (GetState() & BST_PUSHED) != 0)
			GetParent()->SendMessage(WM_COMMAND, MAKELONG(GetDlgCtrlID(), BN_CLICKED), (LPARAM)m_hWnd);
	} /* release capture */


	//CButton::OnLButtonUp(nFlags, point);
}

void CAutoRepeatButton::OnTimer(UINT nIDEvent) 
{
	if( (GetState() & BST_PUSHED) == 0)
		return;
	SetTimer( TIMER_REPEAT, m_nRepeatDelay, NULL);
	GetParent()->SendMessage(WM_COMMAND, MAKELONG(GetDlgCtrlID(), BN_CLICKED), (LPARAM)m_hWnd);
	sent++;

	CButton::OnTimer(nIDEvent);
}
