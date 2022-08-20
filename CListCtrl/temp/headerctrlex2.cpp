// HeaderCtrlEx.cpp : implementation file
//

#include "stdafx.h"
#include "HeaderCtrlEx.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CHeaderCtrlEx

CHeaderCtrlEx::CHeaderCtrlEx()
	: marker_rect(0,0,0,0)
{
	m_pWidth = NULL;
	m_bDragging = FALSE;
	m_bCheckForDrag = FALSE;
	m_fpDragCol = NULL;
	m_pOwnerWnd = NULL;
}

CHeaderCtrlEx::CHeaderCtrlEx(CWnd *pWnd, void (CWnd::*fpDragCol)(int, int)) 
	: marker_rect(0,0,0,0)
{
	m_pWidth = NULL;
	m_bDragging = FALSE;
	m_bCheckForDrag = FALSE;
	m_fpDragCol = fpDragCol;
	m_pOwnerWnd = pWnd;
}

CHeaderCtrlEx::~CHeaderCtrlEx()
{
}


BEGIN_MESSAGE_MAP(CHeaderCtrlEx, CHeaderCtrl)
	//{{AFX_MSG_MAP(CHeaderCtrlEx)
	ON_WM_MOUSEMOVE()
	ON_WM_LBUTTONUP()
	ON_WM_LBUTTONDOWN()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CHeaderCtrlEx message handlers

void CHeaderCtrlEx::OnMouseMove(UINT nFlags, CPoint point) 
{
	if( (MK_LBUTTON & nFlags) == 0)
	{
		// The left mouse button is not pressed - so reset flags
		m_bCheckForDrag = FALSE;
		m_bDragging = FALSE;
	}
	else if( m_bDragging )
	{
		// Get column number that falls under the mouse
		int i=0, cx = 0;
		if( point.x > 0 )
			for( i = 0; i < GetItemCount(); i++ )
			{
				if( point.x >= cx && point.x < cx + m_pWidth[i] )
					break;
				cx += m_pWidth[i];
			}

		if( i != m_nDropPos )
		{
			m_nDropPos = i;

			CRect rect;
			GetWindowRect( &rect );

			// Invalidate area occupied by previous marker
			InvalidateRect( &marker_rect );

			// Draw a new marker
			CClientDC dc(this);
			POINT pts[3];
			pts[0].x = cx; pts[1].x = cx -3; pts[2].x = cx +3;
			pts[0].y = rect.Height(); pts[1].y = pts[2].y = rect.Height() -7; 
			dc.Polygon( pts, 3 );

			// save marker information
			marker_rect.left = cx - 4;
			marker_rect.top = rect.Height() -8;
			marker_rect.right = cx + 4;
			marker_rect.bottom = rect.Height();
		}
		return;
	}
	else if( m_bCheckForDrag )
	{
		// The mouse button was pressed over a column header
		// and now the mouse has moved - so start drag
		m_bCheckForDrag = FALSE;

		m_bDragging = TRUE;
		m_nDropPos = m_nDragCol;

		SetCapture();

		// Store information for later use
		int iCount = GetItemCount();
		HD_ITEM hd_item;
		m_pWidth = new int[iCount];
		for( int i = 0; i < iCount; i++ )
		{
			hd_item.mask = HDI_WIDTH;
			GetItem( i, &hd_item );
			m_pWidth[i] = hd_item.cxy;

		}
		return;
	}

	CHeaderCtrl::OnMouseMove(nFlags, point);
}

void CHeaderCtrlEx::OnLButtonUp(UINT nFlags, CPoint point) 
{
	ASSERT( m_pOwnerWnd != NULL && m_fpDragCol != NULL );

	if( m_bDragging )
	{
		m_bDragging = FALSE;
		delete[] m_pWidth;
		ReleaseCapture();
		Invalidate();

		// Call the callback function.
		if( m_nDragCol != m_nDropPos && m_nDragCol != m_nDropPos -1 )
			(m_pOwnerWnd->*m_fpDragCol)( m_nDragCol, m_nDropPos );
	}
	
	CHeaderCtrl::OnLButtonUp(nFlags, point);
}

void CHeaderCtrlEx::OnLButtonDown(UINT nFlags, CPoint point) 
{
	// Determine if mouse was pressed over a column header
	HD_HITTESTINFO hd_hittestinfo;
	hd_hittestinfo.pt = point;
	SendMessage(HDM_HITTEST, 0, (LPARAM)(&hd_hittestinfo));
	if( hd_hittestinfo.flags == HHT_ONHEADER )
	{
		m_nDragCol = hd_hittestinfo.iItem;
		m_bCheckForDrag = TRUE;
	}

	CHeaderCtrl::OnLButtonDown(nFlags, point);
}

void CHeaderCtrlEx::SetCallback( CWnd* pWnd, void (CWnd::*fpDragCol)(int, int) )
{
	m_fpDragCol = fpDragCol;
	m_pOwnerWnd = pWnd;
}
