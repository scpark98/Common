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
#include <afxdlgs.h>
#include <commdlg.h>
#include <fstream>
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
	m_nGutterCharNumber = 0;

	m_as_folder_list = false;

	memset(&m_lf, 0, sizeof(LOGFONT));

	set_color_theme(color_theme_explorer, false);
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
}	// ~CColorListBox()


BEGIN_MESSAGE_MAP(CColorListBox, CListBox)
	//{{AFX_MSG_MAP(CColorListBox)
	//}}AFX_MSG_MAP
	ON_WM_MOUSEMOVE()
	ON_WM_ERASEBKGND()
	//ON_MESSAGE(WM_SETFONT, OnSetFont)
	//ON_WM_DRAWITEM_REFLECT()
	ON_WM_KEYDOWN()
	ON_WM_KILLFOCUS()
	ON_CONTROL_REFLECT_EX(LBN_SELCHANGE, &CColorListBox::OnLbnSelchange)
	ON_WM_CONTEXTMENU()
	ON_COMMAND_RANGE(menu_show_log, menu_save_all, OnPopupMenu)
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

	//CWnd* pWnd = GetParent();

	CFont* font = GetFont();

	if (font == NULL)
		//font = GetParent()->GetFont();
		font = AfxGetMainWnd()->GetFont();

	if (font != NULL)
		font->GetObject(sizeof(m_lf), &m_lf);
	else
		GetObject(GetStockObject(SYSTEM_FONT), sizeof(m_lf), &m_lf);

	ReconstructFont();
}

void CColorListBox::ReconstructFont()
{
	m_font.DeleteObject();
	BOOL bCreated = m_font.CreateFontIndirect(&m_lf);

	SetFont(&m_font, true);

	m_line_height = -m_lf.lfHeight + 10;
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

	CString		sText;
	COLORREF	cr_text = (COLORREF)lpDIS->itemData;	// Color information is in item data.
	COLORREF	cr_back = m_cr_back;
	CRect		rect = lpDIS->rcItem;
	CRect		rGutter = rect;

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

	//다른 컨트롤에서는 ReconstructFont()안에서 SetFont(&m_font, true);와 같이 글꼴을 적용시키지만
	//custom draw도 마찬가지.
	//owner draw fixed 속성인 컨트롤의 drawitem에서는 dc.SelectObject(&m_font)를 호출해줘야 사용자 설정 글꼴이 적용된다.

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

	if (lpDIS->itemID == m_over_item)
	{
		cr_back = m_cr_backOver;
	}
	else
	{
		if (lpDIS->itemState & ODS_SELECTED)
		{
			cr_back = m_cr_backSelected;
		}
		else
		{
			cr_back = m_cr_back;
		}

	}

	CBrush brush(cr_back);
	dc.FillRect(&rect, &brush);

	if (lpDIS->itemState & ODS_SELECTED)
	{
		//선택 항목의 색은 자신의 색으로 그냥 그려준다.
		//cr_text = m_cr_textSelected;
	}
	else if (lpDIS->itemState & ODS_DISABLED)
	{
		cr_text = ::GetSysColor(COLOR_GRAYTEXT);
	}
	else
	{
		if (lpDIS->itemID == m_over_item)
			cr_text = m_cr_textOver;
	}

	// Get and display item text.
	GetText(lpDIS->itemID, sText);

	// Setup the text format.
	UINT nFormat = DT_LEFT | DT_SINGLELINE | DT_VCENTER;// | DT_END_ELLIPSIS;
	if (GetStyle() & LBS_USETABSTOPS)
		nFormat |= DT_EXPANDTABS;

	CFont* pOldFont = NULL;
	CFont font_selected;

	if (m_as_folder_list)
	{
		rect.left += 6;		//left margin
		CString real_path = convert_special_folder_to_real_path(m_folder_list[lpDIS->itemID], m_pShellImageList->get_csidl_map());
		m_pShellImageList->m_imagelist_small.Draw(&dc, m_pShellImageList->GetSystemImageListIcon(real_path, true),
			CPoint(rect.left, rect.CenterPoint().y - 8), ILD_TRANSPARENT);
		rect.left += 16;	//small icon width
		rect.left += 14;	//margin between icon and text

		LOGFONT lf;

		if (lpDIS->itemState & ODS_SELECTED)
		{
			memcpy(&lf, &m_lf, sizeof(LOGFONT));
			lf.lfWeight = FW_SEMIBOLD;
			BOOL bCreated = font_selected.CreateFontIndirect(&lf);
			pOldFont = dc.SelectObject(&font_selected);
		}
		else
		{
			pOldFont = dc.SelectObject(&m_font);
		}
	}
	else
	{
		pOldFont = dc.SelectObject(&m_font);
	}

	rect.right -= 10;

	dc.SetBkMode(TRANSPARENT);

	//m_show_time일 경우 시간값은 항상 옅은 회색으로만 표시
	if (m_show_time && m_dim_time_str)
	{
		CString time_str = sText.Left(25);
		CSize sz = dc.GetTextExtent(time_str);

		TEXTMETRIC tm;
		pDC->GetTextMetrics(&tm);

		rect.right = sz.cx + tm.tmAveCharWidth;

		dc.SetTextColor(RGB(192, 192, 192));
		dc.DrawText(time_str, rect, nFormat | DT_NOCLIP);

		sText = sText.Mid(26);
		rect.left = rect.right;
		rect.right = lpDIS->rcItem.right - 10;
	}

	dc.SetTextColor(cr_text);
	dc.DrawText(sText, rect, nFormat);

	dc.SelectObject(pOldFont);

	if (m_as_folder_list)
		font_selected.DeleteObject();
}	// DrawItem


//기본 글자색으로 한 줄 추가
int	CColorListBox::add(LPCTSTR lpszFormat, ...)
{
	//가변인자를 그대로 전달하는 방법은 없다.
	//고정인자로 변경한 후 add함수를 호출해줘야 한다.

	CString new_text;
	va_list args;
	va_start(args, lpszFormat);
	new_text.FormatV(lpszFormat, args);

	return add(m_cr_text, new_text);
}

int CColorListBox::add(COLORREF cr, LPCTSTR lpszFormat, ...)
{
	if (m_hWnd == NULL)
		return -1;

	if (!m_show_log)
		return -1;

	if (cr == -1)
		cr = m_cr_text;

	//CString으로 변환
	CString new_text;
	va_list args;
	va_start(args, lpszFormat);
	new_text.FormatV(lpszFormat, args);


	//만약 텍스트의 맨 앞에 \n이 붙어 있으면 이전 로그 라인과 라인을 구분하기 위함인데
	//그냥 기록하면 시간정보를 출력한 후 라인이 변경된다.
	//text의 맨 앞에 \n이 있다면 먼저 처리해준다.
	int i;
	int pre_linefeed_count = 0;		//앞에 붙은 \n 처리
	int post_linefeed_count = 0;	//뒤에 붙은 \n 처리
	bool skip_time_info = false;

	for (i = 0; i < new_text.GetLength(); i++)
	{
		if (new_text[i] == '\n')
			pre_linefeed_count++;
		else
			break;
	}

	//new_text의 앞부분에 있는 '\n'을 제외한 나머지 문자열
	new_text = new_text.Mid(pre_linefeed_count);

	//CEdit컨트롤과는 달리 뒤에 붙은 \n도 별도로 처리해줘야 한다.
	i = new_text.GetLength() - 1;
	while (i >= 0 && new_text[i--] == '\n')
	{
		post_linefeed_count++;
	}


	//앞에 붙은 '\n'의 개수만큼 라인 추가
	if (pre_linefeed_count == 0 && new_text.IsEmpty())
	{
		AddString(_T(""));
	}
	else
	{
		for (i = 0; i < pre_linefeed_count; i++)
		{
			AddString(_T(""));
		}
	}


	CString time_str;
	int index = -1;

	if (m_show_time && !skip_time_info)
	{
		SYSTEMTIME	t;

		::GetLocalTime(&t);
		time_str.Format(_T("%d-%02d-%02d %02d:%02d:%02d(%03d) "), t.wYear, t.wMonth, t.wDay, t.wHour, t.wMinute, t.wSecond, t.wMilliseconds);
		new_text = time_str + _T(" ") + new_text;
	}

	index = AddString(new_text);

	if (index >= 0)
	{
		if (cr < 0)
			SetItemData(index, m_cr_text);
		else
			SetItemData(index, cr);

		//뒤에 붙었던 '\n'개수만큼 빈 줄을 추가해준다.
		for (i = 0; i < post_linefeed_count; i++)
		{
			AddString(_T(""));
		}

		//if (invalidate)
			//RedrawWindow();
			//Invalidate();	//Invalidate()을 호출하면 맨 마지막 항목이 갱신되지 않는 height가 있다.

		if (m_auto_scroll)
			SetTopIndex(GetCount() - 1);
	}

	//항목의 출력 너비에 따라 가로 스크롤바를 재조정해준다.
	CDC* pDC = GetDC();
	CFont* pOldFont = (CFont*)pDC->SelectObject(&m_font);
	TEXTMETRIC tm;
	CSize sz;

	pDC->GetTextMetrics(&tm);
	sz = pDC->GetTextExtent(new_text);
	sz.cx += tm.tmAveCharWidth;

	if (sz.cx > m_max_horizontal_extent)
	{
		m_max_horizontal_extent = sz.cx;
	}

	pDC->SelectObject(pOldFont);
	ReleaseDC(pDC);

	SetHorizontalExtent(m_max_horizontal_extent + GetSystemMetrics(SM_CXVSCROLL));

	return index;
}

int CColorListBox::add(std::deque<CString> *lists, COLORREF cr)
{
	if (cr == -1)
		cr = m_cr_text;

	for (int i = 0; i < lists->size(); i++)
		add(cr, lists->at(i));

	return lists->size();
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
	if (m_use_over)
	{
		UINT nHover = ItemFromPoint(point, m_bOutside);
		if (nHover != m_over_item)
		{
			m_over_item = nHover;
			//over일때 해당 아이템을 selected로 하면 편하지만
			//over와 selected를 별도로 처리하고자 함.
			Invalidate(false);
		}
	}

	CListBox::OnMouseMove(nFlags, point);
}


BOOL CColorListBox::OnEraseBkgnd(CDC* pDC)
{
	// TODO: Add your message handler code here and/or call default
	//CRect rc;
	//GetClientRect(rc);
	//pDC->FillSolidRect(rc, m_crBack);
	return FALSE;
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

void CColorListBox::line_height(int _line_height)
{
	if (_line_height < 8)
		_line_height = 8;

	m_line_height = _line_height;
	SetItemHeight(0, m_line_height);
	Invalidate();
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

void CColorListBox::set_color_theme(int theme, bool apply_now)
{
	switch (theme)
	{
	case color_theme_default:
		m_cr_text = ::GetSysColor(COLOR_BTNTEXT);
		m_cr_textSelected = ::GetSysColor(COLOR_HIGHLIGHTTEXT);
		m_cr_textSelectedInactive = ::GetSysColor(COLOR_INACTIVECAPTIONTEXT);
		m_cr_textOver = ::GetSysColor(COLOR_HIGHLIGHTTEXT);

		m_cr_back = ::GetSysColor(COLOR_WINDOW);
		m_cr_backSelected = RGB(204, 232, 255);	//m_crBackSelected = ::GetSysColor(COLOR_HIGHLIGHT);
		m_cr_backSelectedInactive = ::GetSysColor(COLOR_HIGHLIGHT);
		m_cr_backOver = RGB(195, 222, 245); //m_crBackOver = ::GetSysColor(COLOR_HIGHLIGHT);
		break;
	case color_theme_explorer:
		m_cr_text = ::GetSysColor(COLOR_BTNTEXT);
		m_cr_textSelected = m_cr_text;// ::GetSysColor(COLOR_HIGHLIGHTTEXT);
		m_cr_textSelectedInactive = ::GetSysColor(COLOR_INACTIVECAPTIONTEXT);
		m_cr_textOver = m_cr_text;

		m_cr_back = ::GetSysColor(COLOR_WINDOW); //RGB(242, 242, 242);// ::GetSysColor(COLOR_WINDOW);
		m_cr_backSelected = RGB(204, 232, 255);// ::GetSysColor(COLOR_HIGHLIGHT);
		m_cr_backSelectedInactive = ::GetSysColor(COLOR_HIGHLIGHT);
		m_cr_backOver = RGB(195, 222, 245);
		break;
	}

	if (apply_now)
		Invalidate();
}


void CColorListBox::OnKillFocus(CWnd* pNewWnd)
{
	CListBox::OnKillFocus(pNewWnd);

	// TODO: 여기에 메시지 처리기 코드를 추가합니다.
	if (m_as_popup)
		ShowWindow(SW_HIDE);
}

int CColorListBox::set_path(CString root, CString selected_text)
{
	if (root.IsEmpty())
	{
		m_folder_list.clear();
		m_folder_list.push_back(get_system_label(CSIDL_DRIVES));
		m_folder_list.push_back(get_system_label(CSIDL_PERSONAL));
		m_folder_list.push_back(get_system_label(CSIDL_DESKTOP));
	}
	else
	{
		get_sub_folders(root, &m_folder_list, true);
	}

	return set_folder_list(NULL, selected_text);
}

void CColorListBox::set_as_folder_list()
{
	m_use_over = true;
	m_use_popup_menu = false;
	m_as_folder_list = true;
	m_as_popup = true;
	m_show_time = false;
}

int CColorListBox::set_folder_list(std::deque<CString>* lists, CString selected_text)
{
	ResetContent();

	if (lists != NULL)
	{
		m_folder_list.clear();
		m_folder_list.assign(lists->begin(), lists->end());
	}

	m_over_item = -1;

	for (int i = 0; i < m_folder_list.size(); i++)
	{
		add(get_part(m_folder_list[i], fn_name));
	}

	SelectString(-1, selected_text);

	Invalidate();

	return m_folder_list.size();
}

BOOL CColorListBox::OnLbnSelchange()
{
	int index = GetCurSel();

	if (index < 0 || index >= GetCount())
		return FALSE;

	//Ctrl+End 또는 PageDown으로 맨 마지막 항목이 선택되면 자동 스크롤,
	//그 외 항목이 선택되면 자동 스크롤을 멈춘다.
	m_auto_scroll = (index == GetCount() - 1);

	if (m_as_folder_list)
	{
		CString text = m_folder_list[index];

		if (m_hParentWnd)
		{
			::SendMessage(m_hParentWnd, Message_CColorListBox, (WPARAM)&CColorListBoxMessage(this, message_colorlistbox_selchange), (LPARAM)&text);

			if (m_as_popup)
				ShowWindow(SW_HIDE);
		}
	}

	return FALSE;
}

//선택된 항목 리스트 또는 선택된 개수를 리턴
int CColorListBox::get_selected_items(std::vector<int>* selected)
{
	int selected_count = GetSelCount();

	if (selected != NULL)
	{
		CArray<int, int> aryListBoxSel;
		aryListBoxSel.SetSize(selected_count);
		GetSelItems(selected_count, aryListBoxSel.GetData());

		for (int i = 0; i < aryListBoxSel.GetCount(); i++)
			selected->push_back(aryListBoxSel[i]);
	}
		
	return selected_count;
}


void CColorListBox::OnContextMenu(CWnd* pWnd, CPoint point)
{
	if (!m_use_popup_menu)
		return;

	std::vector<int> selected;
	get_selected_items(&selected);

	CMenu menu;

	menu.CreatePopupMenu();

	if (selected.size() > 0)
	{
		menu.AppendMenu(MF_STRING, menu_selected_count, i2S(selected.size()) + _T(" item(s) selected"));
		menu.EnableMenuItem(menu_selected_count, MF_DISABLED);
		menu.AppendMenu(MF_SEPARATOR);
	}

	menu.AppendMenu(MF_STRING, menu_show_log, _T("로그 표시"));
	menu.CheckMenuItem(menu_show_log, m_show_log ? MF_CHECKED : MF_UNCHECKED);

	menu.AppendMenu(MF_STRING, menu_show_timeinfo, _T("시간 표시"));
	menu.CheckMenuItem(menu_show_timeinfo, m_show_time ? MF_CHECKED : MF_UNCHECKED);
	menu.AppendMenu(MF_SEPARATOR);

	menu.AppendMenu(MF_STRING, menu_auto_scroll, _T("자동 스크롤\tCtrl+End"));
	menu.CheckMenuItem(menu_auto_scroll, m_auto_scroll ? MF_CHECKED : MF_UNCHECKED);
	menu.AppendMenu(MF_SEPARATOR);

	menu.AppendMenu(MF_STRING, menu_clear_all, _T("모두 지우기(&L)"));
	menu.AppendMenu(MF_SEPARATOR);

	menu.AppendMenu(MF_STRING, menu_copy_selected_to_clipboard, _T("Copy selected items"));
	menu.AppendMenu(MF_STRING, menu_copy_all_to_clipboard, _T("Copy all items(&C)"));
	menu.AppendMenu(MF_SEPARATOR);

	menu.AppendMenu(MF_STRING, menu_save_selected, _T("Save selected items to a file..."));
	menu.AppendMenu(MF_STRING, menu_save_all, _T("Save all items to a file...(&S)"));

	menu.TrackPopupMenu(TPM_LEFTALIGN, point.x, point.y, this);
	menu.DestroyMenu();
}

void CColorListBox::OnPopupMenu(UINT nMenuID)
{
	switch (nMenuID)
	{
	case menu_show_log:
		m_show_log = !m_show_log;
		break;
	case menu_show_timeinfo:
		m_show_time = !m_show_time;
		break;
	case menu_auto_scroll:
		m_auto_scroll = !m_auto_scroll;
		break;
	case menu_clear_all:
		ResetContent();
		break;
	case menu_copy_selected_to_clipboard:
		copy_selected_to_clipboard();
		break;
	case menu_copy_all_to_clipboard:
		copy_all_to_clipboard();
		break;
	case menu_save_selected:
		save_selected_to_file();
		break;
	case menu_save_all:
		save_all_to_file();
		break;
	}
}

CString CColorListBox::get_all_text(bool selected_only)
{
	int i;
	CString text;
	CString result;

	if (selected_only)
	{
		std::vector<int> selected;
		get_selected_items(&selected);

		for (i = 0; i < selected.size(); i++)
		{
			GetText(selected[i], text);
			result += (text + _T("\n"));
		}
	}
	else
	{
		for (i = 0; i < GetCount(); i++)
		{
			GetText(i, text);
			result += (text + _T("\n"));
		}
	}

	return result;
}

void CColorListBox::copy_selected_to_clipboard()
{
	CString text = get_all_text(true);
	copy_to_clipboard(m_hWnd, text);
}

void CColorListBox::copy_all_to_clipboard()
{
	CString text = get_all_text();
	copy_to_clipboard(m_hWnd, text);
}

void CColorListBox::save_selected_to_file()
{
	CString recent = AfxGetApp()->GetProfileString(_T("setting\\ColorListBox"), _T("recent saved file"), _T(""));

	if (recent.IsEmpty())
		recent.Format(_T("%s\\logs.txt"), GetExeDirectory());
	
	CFileDialog dlg(false, _T("*.txt"), recent, OFN_EXPLORER | OFN_OVERWRITEPROMPT | OFN_LONGNAMES, _T("*Text file|*.txt|모든 파일|*.*||"));
	if (dlg.DoModal() == IDCANCEL)
		return;

	CString text = get_all_text(true);

	save(dlg.GetPathName(), text, CP_UTF8);
	//CT2A(text)			: ANSI로 저장
	//CT2CA(text, CP_UTF8)	: UTF8로 저장됨
	//std::ofstream of;
	//of.open(dlg.GetPathName(), std::ofstream::out);
	//of << CT2CA(text, CP_UTF8);
	//of.close();
}

void CColorListBox::save_all_to_file()
{
	CString recent = AfxGetApp()->GetProfileString(_T("setting\\ColorListBox"), _T("recent saved file"), _T(""));

	if (recent.IsEmpty())
		recent.Format(_T("%s\\logs.txt"), GetExeDirectory());

	CFileDialog dlg(false, _T("*.txt"), recent, OFN_EXPLORER | OFN_OVERWRITEPROMPT | OFN_LONGNAMES, _T("Text file|*.txt|모든 파일|*.*||"));
	if (dlg.DoModal() == IDCANCEL)
		return;

	CString text = get_all_text();
	/*
	FILE* fp = _tfopen(recent, _T("wt")CHARSET);
	if (!fp)
	{
		AfxMessageBox(recent + _T("\nfail to create file."));
		return;
	}
	fwrite(text, sizeof(TCHAR), text.GetLength(), fp);
	fclose(fp);
	*/


	//CT2A(text)			: ANSI로 저장
	//CT2CA(text, CP_UTF8)	: UTF8로 저장됨
	//std::ofstream of;
	//of.open(dlg.GetPathName(), std::ofstream::out);
	//of << CT2CA(text, CP_UTF8);
	//of.close();
	save(dlg.GetPathName(), text, CP_UTF8);
	//save(dlg.GetPathName(), text, CP_ACP);
}
