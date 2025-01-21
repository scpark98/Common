// SkinHeaderCtrl.cpp : implementation file
//

//#include "stdafx.h"
#include "HeaderCtrlEx.h"
#include "VtListCtrlEx.h"
#include "../../Functions.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CHeaderCtrlEx

CHeaderCtrlEx::CHeaderCtrlEx()
{
	m_cr_back.SetFromCOLORREF(::GetSysColor(COLOR_3DFACE));
	m_cr_text.SetFromCOLORREF(::GetSysColor(COLOR_BTNTEXT));

	if (gray_value(m_cr_back) < 128)
		m_cr_separator = get_color(m_cr_back, 32);
	else
		m_cr_separator = get_color(m_cr_back, -32);

	m_header_is_clicked = false;
	m_header_clicked_index = -1;
	m_header_height = 16;
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
	ON_MESSAGE(HDM_LAYOUT, &CHeaderCtrlEx::OnLayout)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CHeaderCtrlEx message handlers
/*
void CHeaderCtrlEx::DrawItem(LPDRAWITEMSTRUCT lpDIS)
{
}
*/

LRESULT CHeaderCtrlEx::OnLayout(WPARAM, LPARAM lParam)
{
	LPHDLAYOUT pHL = reinterpret_cast<LPHDLAYOUT>(lParam);
	if (pHL->prc == NULL || pHL->prc == INVALID_HANDLE_VALUE ||
		pHL->pwpos == NULL || pHL->pwpos == INVALID_HANDLE_VALUE)
		return 0;

	memcpy(&m_HDLayout, pHL, sizeof(HDLAYOUT));

	//*** The table list rectangle
	RECT* pRect = pHL->prc;

	//*** The table header rectangle
	WINDOWPOS* pWPos = pHL->pwpos;

	//*** Sends HDM_LAYOUT message to the base class
	int nRet = CHeaderCtrl::DefWindowProc(HDM_LAYOUT, 0, lParam);

	//아래 x, cy값은 큰 의미는 없는듯하다. 오히려 cy값을 주면 스크롤시 잔상이 생긴다.
	//*** Moves the table header to the right
	pWPos->x += 0;
	//pWPos->x += m_nHdrWeightDefect;

	//*** New table header height
	pWPos->cy = m_header_height + 0;
	//pWPos->cy = m_nHdrHeight + m_nHdrHeightDefect;

	//*** Decreases the table list height on the table header height
	pRect->top = m_header_height;// m_nHdrHeight;

	return nRet;
}

void CHeaderCtrlEx::OnPaint() 
{
	CPaintDC dc(this);
	CRect rc, rItem;

	dc.SelectObject (GetFont());

	GetClientRect(rc);

	//if (m_flat_style)
	//	dc.FillSolidRect(rc, GetParent()->IsWindowEnabled() ? ::GetSysColor(COLOR_WINDOW) : GRAY(164));
	//else
		dc.FillSolidRect(rc, GetParent()->IsWindowEnabled() ? m_cr_back.ToCOLORREF() : GRAY(164));

	dc.SetBkMode(TRANSPARENT);
	dc.SetTextColor(GetParent()->IsWindowEnabled() ? m_cr_text.ToCOLORREF() : ::GetSysColor(COLOR_GRAYTEXT));

	Gdiplus::Color crSunkenLight = get_color(m_cr_back, 48);
	Gdiplus::Color crSunkenDark  = get_color(m_cr_back, -48);

	int header_count = GetItemCount();
	for (int i = 0; i < GetItemCount(); i++)
	{
		GetItemRect(i, rItem);
		//rItem.top -= 1;
		rItem.bottom -= 1;

		if (i == m_header_clicked_index)
		{
			draw_sunken_rect(&dc, rItem, true, crSunkenDark, crSunkenLight);
			rItem.OffsetRect(1, 1);
		}
		else
		{
			if (m_flat_style)
				draw_line(&dc, rItem.right, rItem.top + 3, rItem.right, rItem.bottom - 3, crSunkenDark);
			else
				draw_sunken_rect(&dc, rItem, false, crSunkenDark, crSunkenLight);
		}

		DWORD dwAlign = m_header_text_align[i];
		DWORD dwFormat = DT_VCENTER | DT_SINGLELINE;
		if (dwAlign == HDF_LEFT)
			dwFormat |= DT_LEFT;
		else if (dwAlign == HDF_CENTER)
			dwFormat |= DT_CENTER;
		else//if (dwAlign == HDF_LEFT)
			dwFormat |= DT_RIGHT;

		rItem.DeflateRect(6, 0);
		dc.DrawText(get_header_text(i), rItem, dwFormat);
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
		GetItem(i, &hditem1);
		
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
		GetItem(DrawItemStruct.itemID, &hditem);

		memDC.DrawText(buf, &rectItem, uFormat);
		memDC.SelectObject(def_font);
		font.DeleteObject();
	}
	*/
}

BOOL CHeaderCtrlEx::OnEraseBkgnd(CDC* pDC) 
{
	return FALSE;	
	//return CHeaderCtrl::OnEraseBkgnd(pDC);
}

CString	CHeaderCtrlEx::get_header_text(int column)
{
	TCHAR				szText[256];
	HDITEM				hdItem;

	hdItem.mask			= HDI_TEXT;
	hdItem.pszText		= szText;
	hdItem.cchTextMax	= 255;

	GetItem(column, &hdItem);

	return szText;
}

void CHeaderCtrlEx::set_header_text(int column, CString sText)
{
	TCHAR				szText[256];
	HDITEM				hdItem;

	hdItem.mask			= HDI_TEXT;
	hdItem.pszText		= szText;
	hdItem.cchTextMax	= 255;

	GetItem(column, &hdItem);

	_stprintf_s(hdItem.pszText, _countof(szText), _T("%s"), sText);
	SetItem(column, &hdItem);
}

int	CHeaderCtrlEx::get_header_text_align(int column)
{
	return m_header_text_align[column];
	/*
	if (GetItemCount() <= 0)
		return HDF_LEFT;

	HDITEM	hdItem;

	//scpark 2010-3-25 10:01:15
	//아래 memset을 안해주면 heap error난다.
	memset(&hdItem, 0, sizeof(hdItem));

	hdItem.mask	= HDI_FORMAT;
	GetItem(column, &hdItem);

	if (hdItem.fmt & HDF_CENTER)
		return HDF_CENTER;
	if (hdItem.fmt & HDF_RIGHT)
		return HDF_RIGHT;
	return HDF_LEFT;
	*/
}

void CHeaderCtrlEx::set_header_text_align(int column, int format)
{
	if (GetItemCount() <= 0)
		return;

	HDITEM	hdItem;

	//scpark 2010-3-25 10:01:15
	//아래 memset을 안해주면 heap error난다.
	memset(&hdItem, 0, sizeof(hdItem));
	
	GetItem(column, &hdItem);
	hdItem.mask	= HDI_FORMAT;
	hdItem.fmt	= format;
	SetItem(column, &hdItem);

	m_header_text_align[column] = format;
}

int CHeaderCtrlEx::get_clicked_header(CPoint point)
{
	CRect	rItem;

	for (int i = 0; i < GetItemCount(); i++)
	{
		GetItemRect(i, rItem);
		//separator와 구분하기 위해 줄여서 체크.

		rItem.DeflateRect(6, 0);
		if (rItem.PtInRect(point))
		{
			m_header_clicked_index = i;
			return m_header_clicked_index;
		}
	}

	m_header_clicked_index = -1;
	return m_header_clicked_index;
}

void CHeaderCtrlEx::OnLButtonDown(UINT nFlags, CPoint point)
{
	// TODO: Add your message handler code here and/or call default
	CVtListCtrlEx*	pListCtrl = (CVtListCtrlEx*)GetParent();

	//if (pListCtrl->m_bAllowSort == false)
	//	return;

	m_header_is_clicked = true;
	m_header_clicked_index = get_clicked_header(point);
	Invalidate();
	CHeaderCtrl::OnLButtonDown(nFlags, point);
}


void CHeaderCtrlEx::OnLButtonUp(UINT nFlags, CPoint point)
{
	CVtListCtrlEx*	pListCtrl = (CVtListCtrlEx*)GetParent();

	//if (pListCtrl->m_bAllowSort == false)
	//	return;

	// TODO: Add your message handler code here and/or call default
	m_header_is_clicked = false;
	m_header_clicked_index = -1;
	Invalidate();
	CHeaderCtrl::OnLButtonUp(nFlags, point);
}

void CHeaderCtrlEx::set_color(Gdiplus::Color cr_text, Gdiplus::Color cr_back, Gdiplus::Color cr_separator)
{
	m_cr_text = cr_text;
	m_cr_back = cr_back;

	if (m_cr_separator.GetValue() == Gdiplus::Color::Transparent)
	{
		if (gray_value(m_cr_back) < 128)
			m_cr_separator = get_color(m_cr_back, 32);
		else
			m_cr_separator = get_color(m_cr_back, -32);
	}

	if (!m_hWnd)
		return;

	Invalidate();
}

void CHeaderCtrlEx::set_text_color(Gdiplus::Color cr_text)
{
	m_cr_text = cr_text;

	if (!m_hWnd)
		return;
	Invalidate();
}

void CHeaderCtrlEx::set_back_color(Gdiplus::Color crBack)
{
	m_cr_back = crBack;

	if (!m_hWnd)
		return;
	Invalidate();
}

int CHeaderCtrlEx::get_header_height()
{
	CRect r;

	GetItemRect(0, r);
	
	return r.Height();
}

void CHeaderCtrlEx::set_header_height(int height)
{
	m_header_height = height;
	Invalidate();
	//OnLayout이 호출되야 실제 높이가 적용되는데
	//아래 화면갱신 함수로는 적용되지 않는다.
	//SetFont / MoveWindow / SetWindowPos 등이 필요.
	//Invalidate();
	//RedrawWindow();
	//UpdateWindow();
	//SendMessage(HDM_LAYOUT, 0, (LPARAM)&m_HDLayout);
	/*
	//=> CFont를 생성하여 SetFont()하면 된다?
	CDC* pDC = GetDC();
	int real_height = -MulDiv(height, pDC->GetDeviceCaps(LOGPIXELSY), 72);
	ReleaseDC(pDC);

	CFont NewFont;
	NewFont.CreateFont(
		real_height,               // nHeight 
		0,                         // nWidth 
		0,                         // nEscapement 
		0,                         // nOrientation 
		FW_NORMAL,                 // nWeight 
		FALSE,                     // bItalic 
		FALSE,                     // bUnderline 
		0,                         // cStrikeOut 
		ANSI_CHARSET,              // nCharSet 
		OUT_DEFAULT_PRECIS,        // nOutPrecision 
		CLIP_DEFAULT_PRECIS,       // nClipPrecision 
		DEFAULT_QUALITY,           // nQuality 
		DEFAULT_PITCH | FF_SWISS,  // nPitchAndFamily 
		_T("Arial"));             // lpszFacename   

	SetFont(&NewFont);
	*/
}

void CHeaderCtrlEx::set_header_flat_style(bool flat)
{
	m_flat_style = flat;
	Invalidate();
}
