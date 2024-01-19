/*===========================================================================
====                                                                     ====
====    File name           :  EditTrans.cpp                             ====
====    Creation date       :  7/10/2001                                 ====
====    Author(s)           :  Dany Cantin                               ====
====                                                                     ====
===========================================================================*/

//#include "stdafx.h"
#include "EditTrans.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CEditTrans

CEditTrans::CEditTrans()
{
    m_TextColor = RGB(0, 0, 0);
    m_BackColor = TRANS_BACK;
	m_bClicked	= FALSE;
	m_ptClicked = CPoint(0,0);
}

CEditTrans::~CEditTrans()
{
}


BEGIN_MESSAGE_MAP(CEditTrans, CEdit)
	//{{AFX_MSG_MAP(CEditTrans)
	ON_WM_CTLCOLOR_REFLECT()
	ON_CONTROL_REFLECT(EN_UPDATE, OnUpdate)
	ON_WM_LBUTTONDOWN()
	ON_CONTROL_REFLECT(EN_KILLFOCUS, OnKillfocus)
	ON_WM_WINDOWPOSCHANGED()
	ON_WM_LBUTTONUP()
	ON_WM_MOUSEMOVE()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CEditTrans message handlers

HBRUSH CEditTrans::CtlColor(CDC* pDC, UINT nCtlColor) 
{
    m_Brush.DeleteObject();

    if (m_BackColor == TRANS_BACK) {
        m_Brush.CreateStockObject(HOLLOW_BRUSH);
        pDC->SetBkMode(TRANSPARENT);
    }
    else {
        m_Brush.CreateSolidBrush(m_BackColor);
        pDC->SetBkColor(m_BackColor);
    }

    pDC->SetTextColor(m_TextColor);

    return (HBRUSH)m_Brush;
}


void CEditTrans::OnKillfocus() 
{
    UpdateCtrl();
}


void CEditTrans::OnUpdate() 
{
    UpdateCtrl();
}

void CEditTrans::UpdateCtrl()
{
    CWnd* pParent = GetParent();
    CRect rect;
    
    GetWindowRect(rect);
    pParent->ScreenToClient(rect);
	rect.top -= 5;
    //rect.InflateRect( 5, 5 );		//이거 안해주면 위치이동시 잔상생김
    
    pParent->InvalidateRect(rect, FALSE);    
}

void CEditTrans::OnWindowPosChanged(WINDOWPOS FAR* lpwndpos) 
{
	CEdit::OnWindowPosChanged(lpwndpos);
	
	// TODO: Add your message handler code here
	UpdateCtrl();
}

void CEditTrans::OnLButtonDown(UINT nFlags, CPoint point) 
{
    UpdateCtrl();
	SetCapture();
	::SendMessage( m_hParent, MSG_MOUSE_EVENT, WM_LBUTTONDOWN, 0 );
	/*m_bClicked = TRUE;
	m_ptClicked = point;
    TRACE( "Clicked = %d, %d\n", point.x, point.y );
	*/	
	CEdit::OnLButtonDown(nFlags, point);
}

void CEditTrans::OnLButtonUp(UINT nFlags, CPoint point) 
{
	//m_bClicked = FALSE;
	ReleaseCapture();
	::SendMessage( m_hParent, MSG_MOUSE_EVENT, WM_LBUTTONUP, 0 );

	CEdit::OnLButtonUp(nFlags, point);
}

void CEditTrans::OnMouseMove(UINT nFlags, CPoint point) 
{
	// TODO: Add your message handler code here and/or call default
/*	TRACE( "Moved = %d, %d\n", point.x, point.y );
	if ( !m_bClicked )
		return;
	
	CRect	Rect;

	GetWindowRect( Rect );
	TRACE( "Old = %d, %d, %d, %d\n", Rect.left, Rect.top, Rect.right, Rect.bottom );

	Rect.left = Rect.left + ( point.x - m_ptClicked.x );
	Rect.top = Rect.top + ( point.y - m_ptClicked.y );
	TRACE( "New = %d, %d, %d, %d\n", Rect.left, Rect.top, Rect.right, Rect.bottom );
	MoveWindow( Rect );
	m_ptClicked = point;
*/
	::SendMessage( m_hParent, MSG_MOUSE_EVENT, WM_MOUSEMOVE, 0 );

	//CEdit::OnMouseMove(nFlags, point);
}
