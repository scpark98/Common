// StaticRange.cpp : implementation file
//

#include "stdafx.h"
#include "StaticRange.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#include "../MemoryDC.h"

/////////////////////////////////////////////////////////////////////////////
// CStaticRange

CStaticRange::CStaticRange()
{
	bInitialized	= FALSE;
	m_nLower		= 0;
	m_nUpper		= 0;
	m_nSelLower		= 0;
	m_nSelUpper		= 0;
	bLClicked		= FALSE;
	bRClicked		= FALSE;
}

CStaticRange::~CStaticRange()
{
}


BEGIN_MESSAGE_MAP(CStaticRange, CStatic)
	//{{AFX_MSG_MAP(CStaticRange)
	ON_WM_PAINT()
	ON_WM_LBUTTONDOWN()
	ON_WM_RBUTTONDOWN()
	ON_WM_LBUTTONUP()
	ON_WM_RBUTTONUP()
	ON_WM_MOUSEMOVE()
	ON_WM_SIZE()
	ON_WM_MBUTTONDOWN()
	ON_WM_MBUTTONUP()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CStaticRange message handlers

void CStaticRange::OnPaint() 
{
	CPaintDC dc1(this); // device context for painting
	
	// TODO: Add your message handler code here
	int		i;
	CRect	Rect;
	GetClientRect( Rect );

	if ( m_ImageBack.m_hImageList == NULL )
		return;

	CMemoryDC	dc( &dc1, &Rect );
	
	// 배경 그림을 칠한다.
	for ( int i = Rect.left; i < Rect.right; i += 2 )
		m_ImageBack.Draw( &dc, 0, CPoint( i, Rect.top ), ILD_TRANSPARENT );

	int	nLeft, nRight;

	if ( m_nLower == 0 && m_nUpper == 0 )
	{
		nLeft	= 0;
		nRight	= Rect.Width();
	}
	else
	{
		nLeft	= (int)( m_nSelLower  * (double)Rect.Width() / (m_nUpper - m_nLower) + m_nLower ); 
		nRight	= (int)( m_nSelUpper * (double)Rect.Width() / (m_nUpper - m_nLower) + m_nLower ); 
	}

	if ( m_nLower == m_nSelLower && m_nUpper == m_nSelUpper )
	{
		ArrowColor = RGB(192,182,172);
		UnderColor = RGB(192,182,172);
	}
	else
	{
		ArrowColor = RGB( 32, 32, 50 );
		UnderColor = RGB( 32, 32, 50 );
	}	

	// 왼쪽 화살표
	for ( i = 0; i < 5; i++ )
		dc.FillSolidRect( nLeft - 0, Rect.top + i, i + 1, 1, ArrowColor );

	// 오른쪽 화살표
	for ( i = 0; i < 5; i++ )
		dc.FillSolidRect( nRight - i - 1, Rect.top + i, i + 1, 1, ArrowColor );

	dc.FillSolidRect( nLeft, Rect.top + 5, nRight - nLeft, 1, UnderColor );
	
	// Do not call CStatic::OnPaint() for painting messages
}

void CStaticRange::SetBackImage( UINT nIDBack )
{
	CBitmap	Bitmap;

	Bitmap.LoadBitmap( nIDBack );
	m_ImageBack.Create( 2, 7, ILC_COLORDDB|ILC_MASK, 1, 0 );
	m_ImageBack.Add( &Bitmap, RGB(0,255,0) );
	m_ImageBack.SetBkColor( CLR_NONE );
	Bitmap.DeleteObject();
}

void CStaticRange::OnLButtonDown(UINT nFlags, CPoint point) 
{
	// TODO: Add your message handler code here and/or call default
	if ( m_nLower == 0 && m_nUpper == 0 )
		return;

	bLClicked = TRUE;
	SetCapture();
	OnMouseMove( nFlags, point );
	
	CStatic::OnLButtonDown(nFlags, point);
}

void CStaticRange::OnLButtonUp(UINT nFlags, CPoint point) 
{
	if ( m_nLower == 0 && m_nUpper == 0 )
		return;

	bLClicked = FALSE;
	ReleaseCapture();
	//::SendMessage( GetParent()->GetSafeHwnd(), MSG_REPEAT_POSITION, -1, 0 );
	
	CStatic::OnLButtonUp(nFlags, point);
}

void CStaticRange::OnRButtonDown(UINT nFlags, CPoint point) 
{
	if ( m_nLower == 0 && m_nUpper == 0 )
		return;

	bRClicked = TRUE;
	SetCapture();
	OnMouseMove( nFlags, point );

	CStatic::OnRButtonDown(nFlags, point);
}

void CStaticRange::OnRButtonUp(UINT nFlags, CPoint point) 
{
	if ( m_nLower == 0 && m_nUpper == 0 )
		return;

	bRClicked = FALSE;
	ReleaseCapture();
	//::SendMessage( GetParent()->GetSafeHwnd(), MSG_REPEAT_POSITION, -1, 0 );
	
	CStatic::OnRButtonUp(nFlags, point);
}

void CStaticRange::OnMouseMove(UINT nFlags, CPoint point) 
{
	// TODO: Add your message handler code here and/or call default
	if ( !bLClicked && !bRClicked )
		return;

	if ( m_nLower == 0 && m_nUpper == 0 )
		return;
	
	int		nPoint;
	CRect	Rect;
	GetClientRect( Rect );

	if ( bLClicked )
	{
		if ( point.x < 0 )
			nPoint = Rect.left;
		else if ( point.x < Rect.right )
			nPoint = point.x;
		else
			nPoint = Rect.right;

		m_nSelLower	= (int)((double)(m_nUpper - m_nLower) * (double)nPoint  / (double)(Rect.Width()));

		if ( m_nSelLower >= m_nSelUpper )
			m_nSelLower = m_nSelUpper - 1;
	}
	else if ( bRClicked )
	{
		if ( point.x >= Rect.right )
			nPoint = Rect.right;
		else if ( point.x >= Rect.left )
			nPoint = point.x;
		else
			nPoint = Rect.left;

		m_nSelUpper	= (int)((double)(m_nUpper - m_nLower) * (double)nPoint / (double)(Rect.Width()));

		if ( m_nSelUpper <= m_nSelLower )
			m_nSelUpper = m_nSelLower + 1;
	}

	CPoint	pt = point;
	ClientToScreen( &pt );

	//::SendMessage( GetParent()->GetSafeHwnd(), MSG_REPEAT_POSITION, (WPARAM)m_nSelLower, (LPARAM)m_nSelUpper );
	
	Invalidate();
	
	CStatic::OnMouseMove(nFlags, point);
}

void CStaticRange::SetSelRange( int nSelLower, int nSelUpper )
{
	if ( nSelLower < m_nLower )
		nSelLower = m_nLower;

	if ( nSelUpper > m_nUpper )
		nSelUpper = m_nUpper;

	if ( nSelLower > nSelUpper + 1 )
	{
		int	Temp	= nSelUpper;
		nSelUpper	= nSelLower;
		nSelLower	= Temp;
	}

	m_nSelLower	= nSelLower;
	m_nSelUpper	= nSelUpper;

	Invalidate();
}

void CStaticRange::OnSize(UINT nType, int cx, int cy) 
{
	CStatic::OnSize(nType, cx, cy);
	
	// TODO: Add your message handler code here
	Invalidate();
}

void CStaticRange::OnMButtonDown(UINT nFlags, CPoint point) 
{
	if ( m_nLower == 0 && m_nUpper == 0 )
		return;

	// 선택 구간을 초기화 한다. -2를 전송한다.
	m_nSelLower	= m_nLower;
	m_nSelUpper	= m_nUpper;

	CPoint	pt = point;
	ClientToScreen( &pt );

	//::SendMessage( GetParent()->GetSafeHwnd(), MSG_REPEAT_POSITION, -2, 0 );

	Invalidate();
	
	CStatic::OnMButtonDown(nFlags, point);
}

void CStaticRange::OnMButtonUp(UINT nFlags, CPoint point) 
{
	if ( m_nLower == 0 && m_nUpper == 0 )
		return;

	ReleaseCapture();
	//::SendMessage( GetParent()->GetSafeHwnd(), MSG_REPEAT_POSITION, -1, 0 );
	
	CStatic::OnMButtonUp(nFlags, point);
}
