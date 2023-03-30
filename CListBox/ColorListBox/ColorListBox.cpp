// ColorListBox.cpp : implementation file

//-------------------------------------------------------------------
//
//	CColorListBox class - 
//		A CListBox-derived class with optional colored items.
//
//		Version: 1.0	01/10/1998 Copyright ?Patrice Godard
//
//		Version: 2.0	09/17/1999 Copyright ?Paul M. Meidinger
//
//-------------------------------------------------------------------

//#include "stdafx.h"
#include "ColorListBox.h"

#include "../../Functions.h"
#include "../../MemoryDC.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CColorListBox

//-------------------------------------------------------------------
//
CColorListBox::CColorListBox()
//
// Return Value:	None.
//
// Parameters	:	None.
//
// Remarks		:	Standard constructor.
//
{
	m_bUseColor = true;
	m_bUseHover = true;
	m_nOverItem = -1;

	m_nGutterCharNumber = 0;

	memset(&m_lf, 0, sizeof(LOGFONT));

	set_color_theme(color_theme_default, false);

	CoInitialize(NULL);
}	// CColorListBox

//-------------------------------------------------------------------
//
CColorListBox::~CColorListBox()
//
// Return Value:	None.
//
// Parameters	:	None.
//
// Remarks		:	Destructor.
//
{
	m_Small.Detach();
}	// ~CColorListBox()


BEGIN_MESSAGE_MAP(CColorListBox, CListBox)
	//{{AFX_MSG_MAP(CColorListBox)
	//}}AFX_MSG_MAP
	ON_WM_MOUSEMOVE()
	ON_WM_ERASEBKGND()
	//ON_MESSAGE(WM_SETFONT, OnSetFont)
	//ON_WM_DRAWITEM_REFLECT()
	ON_WM_KEYDOWN()
	//ON_NOTIFY_REFLECT_EX(LBN_SELCHANGE, &CColorListBox::OnLbnSelchange)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CColorListBox message handlers
/*
LRESULT CColorListBox::OnSetFont(WPARAM wParam, LPARAM)
{
	TRACE("%s\n" __FUNCTION__);
	LRESULT res = Default();

	CRect rc;
	GetWindowRect(&rc);

	WINDOWPOS wp;
	wp.hwnd = m_hWnd;
	wp.cx = rc.Width();
	wp.cy = rc.Height();
	wp.flags = SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOOWNERZORDER | SWP_NOZORDER;

	SendMessage(WM_WINDOWPOSCHANGED, 0, (LPARAM)&wp);

	return res;
}
*/
void CColorListBox::PreSubclassWindow()
{
	// TODO: Add your specialized code here and/or call the base class
	// Get Defalut Font 
	CListBox::PreSubclassWindow();

	CFont* font = GetFont();
	if (font == NULL)
	{
		if (GetParent() == NULL)
			font = GetDesktopWindow()->GetFont();
		else
			font = GetParent()->GetFont();
	}

	if (font != NULL)
		font->GetObject(sizeof(m_lf), &m_lf);
	else
		GetObject(GetStockObject(SYSTEM_FONT), sizeof(m_lf), &m_lf);

	SHFILEINFO shInfo;
	m_Small.Attach((HIMAGELIST)::SHGetFileInfo(_T("C:\\"), 0, &shInfo, sizeof(SHFILEINFO), SHGFI_SYSICONINDEX | SHGFI_SMALLICON));
	CoUninitialize();

	ReconstructFont();
}

void CColorListBox::ReconstructFont()
{
	m_font.DeleteObject();
	BOOL bCreated = m_font.CreateFontIndirect(&m_lf);

	SetFont(&m_font, true);

	SetItemHeight(0, -m_lf.lfHeight + 10);

	ASSERT(bCreated);
}

CColorListBox& CColorListBox::set_font(LOGFONT& lf)
{
	memcpy(&m_lf, &lf, sizeof(LOGFONT));
	ReconstructFont();

	return *this;
}

CColorListBox& CColorListBox::set_font_name(CString sFontname, BYTE byCharSet)
{
	m_lf.lfCharSet = byCharSet;
	_tcscpy_s(m_lf.lfFaceName, _countof(m_lf.lfFaceName), sFontname);
	ReconstructFont();

	return *this;
}

CColorListBox& CColorListBox::set_font_size(int nSize)
{
	HDC hDC = GetDC()->GetSafeHdc();
	m_lf.lfHeight = -MulDiv(nSize, GetDeviceCaps(hDC, LOGPIXELSY), 72);
	::ReleaseDC(m_hWnd, hDC);
	ReconstructFont();

	return *this;
}

CColorListBox& CColorListBox::set_font_bold(bool bBold)
{
	m_lf.lfWeight = (bBold ? FW_BOLD : FW_NORMAL);
	ReconstructFont();

	return *this;
}

//-------------------------------------------------------------------
//
void CColorListBox::DrawItem(LPDRAWITEMSTRUCT lpDIS) 
//
// Return Value:	None.
//
// Parameters	:	lpDIS - A long pointer to a DRAWITEMSTRUCT structure 
//							that contains information about the type of drawing required.
//
// Remarks		:	Called by the framework when a visual aspect of 
//						an owner-draw list box changes. 
//
{
	if ((int)lpDIS->itemID < 0)
		return; 

	CDC* pDC = CDC::FromHandle(lpDIS->hDC);
	CMemoryDC dc(pDC, NULL, true);


	COLORREF	crText;
	CString		sText;
	COLORREF	crNorm = (COLORREF)lpDIS->itemData;		// Color information is in item data.
	COLORREF	crHilite = RGB(255-GetRValue(crNorm), 255-GetGValue(crNorm), 255-GetBValue(crNorm));
	CRect		rect = lpDIS->rcItem;
	CRect		rGutter = rect;

	// Set the background mode to TRANSPARENT to draw the text.
	int nBkMode = dc.SetBkMode(TRANSPARENT);

	if (m_nGutterCharNumber > 0)
	{
		rGutter.right = rGutter.left + dc.GetTextExtent(_T("M")).cx * m_nGutterCharNumber;
		rect.left = rGutter.right;
		CBrush brush(::GetSysColor(COLOR_3DFACE));
		dc.FillRect(&rGutter, &brush);
		dc.SetTextColor(RGB(128, 128, 128));
		sText.Format(_T("%d"), (int)lpDIS->itemID);
		rGutter.right -= 4;
		dc.DrawText(sText, rGutter, DT_RIGHT | DT_SINGLELINE | DT_VCENTER);
	}

	//다른 컨트롤에서는 ReconstructFont()안에서 SetFont( &m_font, true );와 같이 글꼴을 적용시키지만
	//custom draw도 마찬가지.
	//owner draw fixed 속성인 컨트롤의 drawitem에서는 dc.SelectObject(&m_font)를 호출해줘야 사용자 설정 글꼴이 적용된다.
	dc.SelectObject(&m_font);

	// If item has been selected, draw the highlight rectangle using the item's color.
	//if ((lpDIS->itemState & ODS_SELECTED) &&
	//	 (lpDIS->itemAction & (ODA_SELECT | ODA_DRAWENTIRE)))
	//{
	//	CBrush brush(RGB(0,0,255));
	//	dc.FillRect(&lpDIS->rcItem, &brush);
	//}

	// If item has been deselected, draw the rectangle using the window color.
	//if (!(lpDIS->itemState & ODS_SELECTED) &&	(lpDIS->itemAction & ODA_SELECT))
	//{
	//	CBrush brush(::GetSysColor(COLOR_WINDOW));
	//	pDC->FillRect(&lpDIS->rcItem, &brush);
	//}	 	

	// If item has focus, draw the focus rect.
	if ((lpDIS->itemAction & ODA_FOCUS) && (lpDIS->itemState & ODS_FOCUS))
		dc.DrawFocusRect(&rect); 

	//// If item does not have focus, redraw (erase) the focus rect.
	if ((lpDIS->itemAction & ODA_FOCUS) &&	!(lpDIS->itemState & ODS_FOCUS))
		dc.DrawFocusRect(&rect);

	if (lpDIS->itemID == m_nOverItem)
	{
		CBrush brush(m_crBackOver);
		dc.FillRect(&rect, &brush);
	}
	else
	{
		if (lpDIS->itemState & ODS_SELECTED)
		{
			CBrush brush(m_crBackSelected);//RGB(0, 0, 255));
			dc.FillRect(&rect, &brush);
		}
		else
		{
			CBrush brush(m_crBack);//::GetSysColor(COLOR_WINDOW));
			dc.FillRect(&rect, &brush);
		}

	}

	// If the item's color information is set, use the highlight color
	// gray text color, or normal color for the text.
	if (m_bUseColor)		
	{
		if (lpDIS->itemState & ODS_SELECTED)
			crText = dc.SetTextColor(m_crTextSelected);
		else if (lpDIS->itemState & ODS_DISABLED)
			crText = dc.SetTextColor(::GetSysColor(COLOR_GRAYTEXT));
		else
		{
			if (lpDIS->itemID == m_nOverItem)
				crNorm = m_crTextOver;
			else
				crNorm = m_crText;

			crText = dc.SetTextColor(crNorm);
		}
	}
	// Else the item's color information is not set, so use the
	// system colors for the text.
	else
	{
		if (lpDIS->itemState & ODS_SELECTED)
			crText = dc.SetTextColor(m_crTextSelected);
		else if (lpDIS->itemState & ODS_DISABLED)
			crText = dc.SetTextColor(::GetSysColor(COLOR_GRAYTEXT));
		else
			crText = dc.SetTextColor(m_crText);
	}

	// Get and display item text.
	GetText(lpDIS->itemID, sText);

	// Setup the text format.
	UINT nFormat = DT_LEFT | DT_SINGLELINE | DT_VCENTER;
	if (GetStyle() & LBS_USETABSTOPS)
		nFormat |= DT_EXPANDTABS;

	if (m_Small.GetImageCount() > 0)
	{
		//m_Small.Draw(&dc, GetSystemImageListIcon(sText, false), CPoint(rect.left, rect.CenterPoint().y - 8), ILD_TRANSPARENT);
		if (lpDIS->itemID == 0)
			m_Small.Draw(&dc, GetSystemImageListIcon(_T("D:\\1.Projects_C++\\0.backup\\MiniMini"), false), CPoint(rect.left, rect.CenterPoint().y - 8), ILD_TRANSPARENT);
		else
			m_Small.Draw(&dc, GetSystemImageListIcon(sText, false), CPoint(rect.left, rect.CenterPoint().y - 8), ILD_TRANSPARENT);
		rect.left += 20;
	}
	dc.DrawText(sText, rect, nFormat);

	dc.SetTextColor(crText);
	dc.SetBkMode(nBkMode);
}	// DrawItem

int CColorListBox::AddString(CString text, COLORREF crText, COLORREF crBack, bool invalidate)
{
	return add_string(text, crText, crBack, invalidate);
}

//-------------------------------------------------------------------
//
int CColorListBox::add_string(CString lpszItem, COLORREF crText, COLORREF crBack, bool invalidate)
//
// Return Value:	The zero-based index to the string in the list box. 
//						The return value is LB_ERR if an error occurs; the 
//						return value is LB_ERRSPACE if insufficient space 
//						is available to store the new string.
//
// Parameters	:	lpszItem - Points to the null-terminated 
//							string that is to be added.
//
// Remarks		:	Call this member function to add a string to a list 
//						box. Provided because CListBox::AddString is NOT
//						a virtual function.
//
{
	int index = ((CListBox*)this)->AddString(lpszItem);
	set_item_color(index, crText, invalidate);
	//SetTopIndex(GetCount() - 1);

	return index;
}	// AddString

//-------------------------------------------------------------------
//
int CColorListBox::add_string(CString lpszItem, COLORREF rgb, bool invalidate)
//
// Return Value:	The zero-based index to the string in the list box. 
//						The return value is LB_ERR if an error occurs; the 
//						return value is LB_ERRSPACE if insufficient space 
//						is available to store the new string.
//
// Parameters	:	lpszItem - Points to the null-terminated 
//							string that is to be added.
//						rgb - Specifies the color to be associated with the item.
//
// Remarks		:	Call this member function to add a string to a list 
//						box with a custom color.
//
{
	int nItem = ((CListBox*)this)->AddString(lpszItem);
	if (nItem >= 0)
	{
		SetItemData(nItem, rgb);
		if (invalidate)
			RedrawWindow();
	}

	return nItem;
}	// AddString

int CColorListBox::add_string(std::deque<CString> *lists, bool invalidate)
{
	for (int i = 0; i < lists->size(); i++)
		add_string(lists->at(i), GetSysColor(COLOR_WINDOWTEXT), ::GetSysColor(COLOR_WINDOW), invalidate);

	return 0;
}

//-------------------------------------------------------------------
//
int CColorListBox::insert_string(int nIndex, CString lpszItem)
//
// Return Value:	The zero-based index of the position at which the 
//						string was inserted. The return value is LB_ERR if 
//						an error occurs; the return value is LB_ERRSPACE if 
//						insufficient space is available to store the new string.
//
// Parameters	:	nIndex - Specifies the zero-based index of the position
//							to insert the string. If this parameter is ?, the string
//							is added to the end of the list.
//						lpszItem - Points to the null-terminated string that 
//							is to be inserted.
//
// Remarks		:	Inserts a string into the list box.	Provided because 
//						CListBox::InsertString is NOT a virtual function.
//
{
	int nItem = ((CListBox*)this)->InsertString(nIndex, lpszItem);
	SetTopIndex(GetCount() - 1);
	
	return nItem;

}	// InsertString

//-------------------------------------------------------------------
//
int CColorListBox::insert_string(int nIndex, CString lpszItem, COLORREF rgb)
//
// Return Value:	The zero-based index of the position at which the 
//						string was inserted. The return value is LB_ERR if 
//						an error occurs; the return value is LB_ERRSPACE if 
//						insufficient space is available to store the new string.
//
// Parameters	:	nIndex - Specifies the zero-based index of the position
//							to insert the string. If this parameter is ?, the string
//							is added to the end of the list.
//						lpszItem - Points to the null-terminated string that 
//							is to be inserted.
//						rgb - Specifies the color to be associated with the item.
//
// Remarks		:	Inserts a colored string into the list box.
//
{
	int index = ((CListBox*)this)->InsertString(nIndex,lpszItem);
	if (index >= 0)
	{
		SetItemData(index, rgb);
		RedrawWindow();
	}

	return index;
}	// InsertString

//-------------------------------------------------------------------
//
void CColorListBox::set_item_color(int nIndex, COLORREF rgb, bool invalidate)
//
// Return Value:	None.
//
// Parameters	:	nIndex - Specifies the zero-based index of the item.
//						rgb - Specifies the color to be associated with the item.
//
// Remarks		:	Sets the 32-bit value associated with the specified
//						item in the list box.
//
{
	SetItemData(nIndex, rgb);
	if (invalidate)
		RedrawWindow();
}

COLORREF CColorListBox::get_item_color( int nIndex )
{
	return (COLORREF)GetItemData( nIndex );
}


void CColorListBox::OnMouseMove(UINT nFlags, CPoint point)
{
	// TODO: Add your message handler code here and/or call default
	if (m_bUseHover)
	{
		UINT nHover = ItemFromPoint(point, m_bOutside);
		if (nHover != m_nOverItem)
		{
			m_nOverItem = nHover;
			Invalidate(false);
		}
	}

	CListBox::OnMouseMove(nFlags, point);
}


BOOL CColorListBox::OnEraseBkgnd(CDC* pDC)
{
	// TODO: Add your message handler code here and/or call default
	CRect rc;
	GetClientRect(rc);
	pDC->FillSolidRect(rc, m_crBack);
	return false;
	return CListBox::OnEraseBkgnd(pDC);
}

CSize CColorListBox::resizeToFit(bool bHori, bool bVert)
{
	CRect	r;
	GetWindowRect(r);

	if (bHori)
	{
		CDC *pDC = GetDC();
		CString str;
		int nMaxTextWidth = 0;
		int nGutterWidth = 0;

		if (m_nGutterCharNumber > 0)
			nGutterWidth = pDC->GetTextExtent(_T("M")).cx * m_nGutterCharNumber;

		pDC->SelectObject(&m_font);

		UINT nFormat = DT_LEFT | DT_SINGLELINE | DT_VCENTER;
		if (GetStyle() & LBS_USETABSTOPS)
			nFormat |= DT_EXPANDTABS;

		for (int i = 0; i < GetCount(); i++)
		{
			GetText(i, str);
			pDC->DrawText(str, -1, &r, nFormat | DT_CALCRECT);
			if (r.Width() > nMaxTextWidth)
				nMaxTextWidth = r.Width();
		}

		r.right = r.left + nMaxTextWidth + nGutterWidth + 4;	//4 = 2 + 2 = border size, 10 = space margin

		ReleaseDC(pDC);
	}

	if (bVert)
	{
		int h = GetItemHeight(0);

		if ((m_nMinimumLines > 0) && (GetCount() < m_nMinimumLines))
			r.bottom = r.top + GetItemHeight(0) * m_nMinimumLines + 4;
		else
			r.bottom = r.top + GetItemHeight(0) * GetCount() + 4;
	}

	SetWindowPos(NULL, 0, 0, r.Width(), r.Height(), SWP_NOMOVE | SWP_NOZORDER);

	return CSize(r.Width(), r.Height());
}


BOOL CColorListBox::PreTranslateMessage(MSG* pMsg)
{
	// TODO: Add your specialized code here and/or call the base class
	if (pMsg->message == WM_KEYDOWN)
	{
		TRACE(_T("CColorListBox message = %d\n"), pMsg->wParam);
		return false;
	}

	return CListBox::PreTranslateMessage(pMsg);
}


void CColorListBox::OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags)
{
	// TODO: Add your message handler code here and/or call default
	//TRACE(_T("CColorListBox::OnKeyDown = %d\n"), nChar);
	//HWND hWnd = GetParent()->GetSafeHwnd();
	//::PostMessage(hWnd, WM_KEYDOWN, (WPARAM)nChar, 0);
	CListBox::OnKeyDown(nChar, nRepCnt, nFlags);
}


BOOL CColorListBox::OnLbnSelchange()
{
	return FALSE;
}


int CColorListBox::GetSystemImageListIcon(CString szFile, BOOL bDrive)
{
	SHFILEINFO shFileInfo;

	if (false)//szFile == GetStringById(NFTD_IDS_COMPUTER))
	{
		LPITEMIDLIST pidl_Computer = NULL;
		SHGetFolderLocation(NULL, CSIDL_DRIVES, NULL, 0, &pidl_Computer); // 컴퓨터
		SHGetFileInfo((wchar_t*)pidl_Computer, 0, &shFileInfo, sizeof(shFileInfo), SHGFI_DISPLAYNAME | SHGFI_SYSICONINDEX | SHGFI_PIDL);
	}
	else if (false)//szFile == GetStringById(NFTD_IDS_DOCUMENT))
	{
		LPITEMIDLIST pidl_Document = NULL;
		SHGetFolderLocation(NULL, CSIDL_PERSONAL, NULL, 0, &pidl_Document); // 내문서
		SHGetFileInfo((wchar_t*)pidl_Document, 0, &shFileInfo, sizeof(shFileInfo), SHGFI_DISPLAYNAME | SHGFI_SYSICONINDEX | SHGFI_PIDL);
	}
	else if (false)//szFile == GetStringById(NFTD_IDS_DESKTOP))
	{
		LPITEMIDLIST pidl_Desktop = NULL;
		SHGetFolderLocation(NULL, CSIDL_DESKTOP, NULL, 0, &pidl_Desktop); // 바탕화면
		SHGetFileInfo((wchar_t*)pidl_Desktop, 0, &shFileInfo, sizeof(shFileInfo), SHGFI_DISPLAYNAME | SHGFI_SYSICONINDEX | SHGFI_PIDL);
	}
	else
	{
		SHGetFileInfo(szFile, 0, &shFileInfo, sizeof(shFileInfo), SHGFI_SYSICONINDEX | SHGFI_SMALLICON);
		if (shFileInfo.iIcon < 0)
			SHGetFileInfo(szFile, 0, &shFileInfo, sizeof(shFileInfo), SHGFI_SYSICONINDEX | SHGFI_SMALLICON | SHGFI_USEFILEATTRIBUTES);
		//else
		//{
		//	if (PathIsDirectory(szFile))
		//		SHGetFileInfo(szFile, FILE_ATTRIBUTE_DIRECTORY, &shFileInfo, sizeof(shFileInfo), SHGFI_SYSICONINDEX | SHGFI_SMALLICON | SHGFI_USEFILEATTRIBUTES);
		//	else
		//		SHGetFileInfo(szFile, 0, &shFileInfo, sizeof(shFileInfo), SHGFI_SYSICONINDEX | SHGFI_SMALLICON | SHGFI_USEFILEATTRIBUTES);
		//}
	}

	return shFileInfo.iIcon;
}

void CColorListBox::set_color_theme(int theme, bool apply_now)
{
	switch (theme)
	{
	case color_theme_default:
		m_crText = ::GetSysColor(COLOR_BTNTEXT);
		m_crTextSelected = ::GetSysColor(COLOR_HIGHLIGHTTEXT);
		m_crTextSelectedInactive = ::GetSysColor(COLOR_INACTIVECAPTIONTEXT);
		m_crTextOver = ::GetSysColor(COLOR_HIGHLIGHTTEXT);

		m_crBack = ::GetSysColor(COLOR_WINDOW);
		m_crBackSelected = ::GetSysColor(COLOR_HIGHLIGHT);
		m_crBackSelectedInactive = ::GetSysColor(COLOR_HIGHLIGHT);
		m_crBackOver = ::GetSysColor(COLOR_HIGHLIGHT);
		break;
	case color_theme_explorer:
		m_crText = ::GetSysColor(COLOR_BTNTEXT);
		m_crTextSelected = ::GetSysColor(COLOR_HIGHLIGHTTEXT);
		m_crTextSelectedInactive = ::GetSysColor(COLOR_INACTIVECAPTIONTEXT);
		m_crTextOver = m_crText;

		m_crBack = ::GetSysColor(COLOR_WINDOW);
		m_crBackSelected = ::GetSysColor(COLOR_HIGHLIGHT);
		m_crBackSelectedInactive = ::GetSysColor(COLOR_HIGHLIGHT);
		m_crBackOver = RGB(195, 222, 245);
		break;
	}

	if (apply_now)
		Invalidate();
}
