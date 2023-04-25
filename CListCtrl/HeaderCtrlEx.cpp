// SkinHeaderCtrl.cpp : implementation file
//

//#include "stdafx.h"
#include "HeaderCtrlEx.h"
#include "ListCtrlEx.h"
#include "../Functions.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CHeaderCtrlEx

CHeaderCtrlEx::CHeaderCtrlEx()
{
	m_crBack = ::GetSysColor( COLOR_3DFACE );
	m_crText = ::GetSysColor( COLOR_BTNTEXT );
	m_bColumnClicked = false;
	m_nColumnClicked = -1;

}

CHeaderCtrlEx::~CHeaderCtrlEx()
{
}


BEGIN_MESSAGE_MAP(CHeaderCtrlEx, CHeaderCtrl)
	//{{AFX_MSG_MAP(CHeaderCtrlEx)
	ON_WM_PAINT()
	ON_WM_ERASEBKGND()
	//}}AFX_MSG_MAP
	ON_WM_LBUTTONDOWN()
	ON_WM_LBUTTONUP()
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CHeaderCtrlEx message handlers
/*
void CHeaderCtrlEx::DrawItem(LPDRAWITEMSTRUCT lpDIS)
{
}
*/
void CHeaderCtrlEx::OnPaint() 
{
	CPaintDC dc(this);
	CRect rc, rItem;

	dc.SelectObject ( GetFont() );

	GetClientRect( rc );
	dc.FillSolidRect( rc, GetParent()->IsWindowEnabled() ? m_crBack : GRAY(164) );

	dc.SetBkMode( TRANSPARENT );
	dc.SetTextColor( GetParent()->IsWindowEnabled() ? m_crText : ::GetSysColor(COLOR_GRAYTEXT) );

	COLORREF crSunkenLight = get_color( m_crBack, 48 );
	COLORREF crSunkenDark  = get_color( m_crBack, -48 );

	for ( int i = 0; i < GetItemCount(); i++ )
	{
		GetItemRect( i, rItem );
		//rItem.top -= 1;
		rItem.bottom -= 1;

		if ( i == m_nColumnClicked )
		{
			DrawSunkenRect( &dc, rItem, true, crSunkenDark, crSunkenLight );
			rItem.OffsetRect( 1, 1 );
		}
		else
		{
			DrawSunkenRect( &dc, rItem, false, crSunkenDark, crSunkenLight );
		}

		DWORD dwAlign = GetColumnTextAlign(i);
		DWORD dwFormat = DT_VCENTER | DT_SINGLELINE;
		if ( dwAlign == HDF_LEFT )
			dwFormat |= DT_LEFT;
		else if ( dwAlign == HDF_CENTER )
			dwFormat |= DT_CENTER;
		else//if ( dwAlign == HDF_LEFT )
			dwFormat |= DT_RIGHT;

		rItem.DeflateRect( 6, 0 );
		dc.DrawText( GetColumnText( i ), rItem, dwFormat );
	}

	/*
	CPaintDC dc(this); // device context for painting
	
	CRect rect, rectItem, clientRect;
	GetClientRect(&rect);
	GetClientRect(&clientRect);
	CMemDC memDC(&dc, rect);
	CDC bitmapDC;
	bitmapDC.CreateCompatibleDC(&dc);
	
	memDC.FillSolidRect(&rect, RGB(76,85,118));

	CBitmap bitmapSpan;
	bitmapSpan.LoadBitmap(IDB_COLUMNHEADER_SPAN);
	CBitmap* pOldBitmapSpan = bitmapDC.SelectObject(&bitmapSpan);

	memDC.StretchBlt(rect.left+2, 0, rect.Width(), 12, &bitmapDC, 0, 0, 1, 12, SRCCOPY);

	bitmapDC.SelectObject(pOldBitmapSpan);
	bitmapSpan.DeleteObject();
	
	int nItems = GetItemCount();

	CBitmap bitmap;
	CBitmap bitmap2;
	CBitmap bitmap3;
	
	bitmap.LoadBitmap(IDB_COLUMNHEADER_START);
	bitmap2.LoadBitmap(IDB_COLUMNHEADER_SPAN);
	bitmap3.LoadBitmap(IDB_COLUMNHEADER_END);

	for(int i = 0; i <nItems; i++)
	{
		
		TCHAR buf1[256];
		HD_ITEM hditem1;
		
		hditem1.mask = HDI_TEXT | HDI_FORMAT | HDI_ORDER;
		hditem1.pszText = buf1;
		hditem1.cchTextMax = 255;
		GetItem( i, &hditem1 );
		
		GetItemRect(i, &rect);
		
		CBitmap* pOldBitmap = NULL;
		
		//make sure we draw the start piece
		//on the first item so it has a left border

		//For the following items we will just use the
		//right border of the previous items as the left
		//border
		if(hditem1.iOrder==0)
		{
			pOldBitmap = bitmapDC.SelectObject(&bitmap);
			memDC.BitBlt(rect.left,rect.top,2,12,&bitmapDC,0,0,SRCCOPY);
		}
		else
		{
			memDC.BitBlt(rect.left-1,rect.top,2,12,&bitmapDC,0,0,SRCCOPY);
			pOldBitmap = bitmapDC.SelectObject(&bitmap2);
			memDC.BitBlt(rect.left+1,rect.top,1,12,&bitmapDC,0,0,SRCCOPY);
		}

		bitmapDC.SelectObject(pOldBitmap);
		
		//span the bitmap for the width of the column header item
		int nWidth = rect.Width() - 4;
		
		CBitmap* pOldBitmap2 = bitmapDC.SelectObject(&bitmap2);
		
		memDC.StretchBlt(rect.left+2, 0, nWidth, 1, &bitmapDC, 0,0, 1, 12, SRCCOPY);

		bitmapDC.SelectObject(pOldBitmap2);
		
		
		//draw the end piece of the column header
		CBitmap* pOldBitmap3 = bitmapDC.SelectObject(&bitmap3);
		memDC.BitBlt((rect.right-2), 0, 2, 12, &bitmapDC,0,0,SRCCOPY);
		bitmapDC.SelectObject(pOldBitmap3);
		
		//Get all the info for the current
		//item so we can draw the text to it
		//in the desired font and style
		DRAWITEMSTRUCT	DrawItemStruct;
		GetItemRect(i, &rectItem);
		
		
		DrawItemStruct.CtlType		= 100;
		DrawItemStruct.hDC			= dc.GetSafeHdc();
		DrawItemStruct.itemAction	= ODA_DRAWENTIRE; 
		DrawItemStruct.hwndItem 	= GetSafeHwnd(); 
		DrawItemStruct.rcItem	= rectItem;
		DrawItemStruct.itemID	= i;
		DrawItem(&DrawItemStruct);
		
		UINT uFormat = DT_SINGLELINE | DT_NOPREFIX | DT_TOP |DT_CENTER | DT_END_ELLIPSIS ;
		
		
		CFont font;
		LOGFONT lf;
		memset(&lf, 0, sizeof(LOGFONT));
		lf.lfHeight = 8;
		strcpy(lf.lfFaceName, "Sevenet 7");
		font.CreateFontIndirect(&lf);
		CFont* def_font = memDC.SelectObject(&font);
		
		memDC.SetBkMode(TRANSPARENT);
		rectItem.DeflateRect(2,2,2,2);
		
		TCHAR buf[256];
		HD_ITEM hditem;
		
		hditem.mask = HDI_TEXT | HDI_FORMAT | HDI_ORDER;
		hditem.pszText = buf;
		hditem.cchTextMax = 255;
		GetItem( DrawItemStruct.itemID, &hditem );

		memDC.DrawText(buf, &rectItem, uFormat);
		memDC.SelectObject(def_font);
		font.DeleteObject();
	}
	*/
}

BOOL CHeaderCtrlEx::OnEraseBkgnd(CDC* pDC) 
{
	return false;	
	//return CHeaderCtrl::OnEraseBkgnd(pDC);
}

CString	CHeaderCtrlEx::GetColumnText( int nColumn )
{
	TCHAR				szText[256];
	HDITEM				hdItem;

	hdItem.mask			= HDI_TEXT;
	hdItem.pszText		= szText;
	hdItem.cchTextMax	= 255;

	GetItem( nColumn, &hdItem );

	return szText;
}

void CHeaderCtrlEx::SetColumnText( int nColumn, CString sText )
{
	TCHAR				szText[256];
	HDITEM				hdItem;

	hdItem.mask			= HDI_TEXT;
	hdItem.pszText		= szText;
	hdItem.cchTextMax	= 255;

	GetItem( nColumn, &hdItem );

	//_stprintf_s(hdItem.pszText, _countof(hdItem.pszText), _T("%s"), sText);
	SetItem(nColumn, &hdItem);
}

int	CHeaderCtrlEx::GetColumnTextAlign( int nColumn )
{
	if ( GetItemCount() <= 0 )
		return HDF_LEFT;

	HDITEM	hdItem;

	//scpark 2010-3-25 10:01:15
	//아래 memset을 안해주면 heap error난다.
	memset(&hdItem, 0, sizeof(hdItem));

	hdItem.mask	= HDI_FORMAT;
	GetItem( nColumn, &hdItem );

	if ( hdItem.fmt & HDF_CENTER )
		return HDF_CENTER;
	if ( hdItem.fmt & HDF_RIGHT )
		return HDF_RIGHT;
	return HDF_LEFT;
}

void CHeaderCtrlEx::SetColumnTextAlign( int nColumn, int format )
{
	if ( GetItemCount() <= 0 )
		return;

	HDITEM	hdItem;

	//scpark 2010-3-25 10:01:15
	//아래 memset을 안해주면 heap error난다.
	memset(&hdItem, 0, sizeof(hdItem));
	
	GetItem( nColumn, &hdItem );
	hdItem.mask	= HDI_FORMAT;
	hdItem.fmt	= format;
	SetItem( nColumn, &hdItem );
}

int CHeaderCtrlEx::GetClickedColumn( CPoint point )
{
	CRect	rItem;

	for ( int i = 0; i < GetItemCount(); i++ )
	{
		GetItemRect( i, rItem );
		//separator와 구분하기 위해 줄여서 체크.

		rItem.DeflateRect( 6, 0 );
		if ( rItem.PtInRect( point ) )
		{
			m_nColumnClicked = i;
			return m_nColumnClicked;
		}
	}

	m_nColumnClicked = -1;
	return m_nColumnClicked;
}

void CHeaderCtrlEx::OnLButtonDown(UINT nFlags, CPoint point)
{
	// TODO: Add your message handler code here and/or call default
	CListCtrlEx*	pListCtrl = (CListCtrlEx*)GetParent();

	if ( pListCtrl->m_bAllowSort == false )
		return;

	m_bColumnClicked = true;
	m_nColumnClicked = GetClickedColumn( point );
	Invalidate();
	CHeaderCtrl::OnLButtonDown(nFlags, point);
}


void CHeaderCtrlEx::OnLButtonUp(UINT nFlags, CPoint point)
{
	CListCtrlEx*	pListCtrl = (CListCtrlEx*)GetParent();

	if ( pListCtrl->m_bAllowSort == false )
		return;

	// TODO: Add your message handler code here and/or call default
	m_bColumnClicked = false;
	m_nColumnClicked = -1;
	Invalidate();
	CHeaderCtrl::OnLButtonUp(nFlags, point);
}
