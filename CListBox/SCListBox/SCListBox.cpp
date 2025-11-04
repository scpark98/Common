// SCListBox.cpp : implementation file

//-------------------------------------------------------------------
//
//	CSCListBox class - 
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
#include "SCListBox.h"

#include "../../Functions.h"
#include "../../MemoryDC.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#define IDC_EDIT_CELL	WM_USER + 9001

/////////////////////////////////////////////////////////////////////////////
// CSCListBox

//-------------------------------------------------------------------
//
CSCListBox::CSCListBox()
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

	set_color_theme(CSCColorTheme::color_theme_default);
}	// CSCListBox

//-------------------------------------------------------------------
//
CSCListBox::~CSCListBox()
{
	if (m_pEdit)
	{
		m_pEdit->DestroyWindow();
		delete m_pEdit;
	}

	for (int i = 0; i < m_imagelist.size(); i++)
	{
		CSCGdiplusBitmap* img = m_imagelist[i];
		delete img;
	}
}

BEGIN_MESSAGE_MAP(CSCListBox, CListBox)
	//{{AFX_MSG_MAP(CSCListBox)
	//}}AFX_MSG_MAP
	ON_WM_MOUSEMOVE()
	ON_WM_ERASEBKGND()
	//ON_MESSAGE(WM_SETFONT, OnSetFont)
	//ON_WM_DRAWITEM_REFLECT()
	ON_WM_KEYDOWN()
	ON_WM_KILLFOCUS()
	ON_CONTROL_REFLECT_EX(LBN_SELCHANGE, &CSCListBox::OnLbnSelchange)
	ON_WM_CONTEXTMENU()
	ON_COMMAND_RANGE(menu_selected_count, menu_delete_selected, OnPopupMenu)
	ON_WM_MOUSEWHEEL()
	ON_WM_MOUSEHWHEEL()
	ON_WM_SIZE()
	ON_WM_LBUTTONDOWN()
	ON_WM_LBUTTONUP()
	//ON_WM_PAINT()
	ON_WM_WINDOWPOSCHANGED()
	//ON_WM_NCPAINT()
	//ON_WM_CTLCOLOR()
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CSCListBox message handlers
/*
LRESULT CSCListBox::OnSetFont(WPARAM wParam, LPARAM)
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
void CSCListBox::PreSubclassWindow()
{
	// TODO: Add your specialized code here and/or call the base class
	// Get Defalut Font 
	CListBox::PreSubclassWindow();

	//이게 왜 적용이 안될까...
	DWORD dwStyle = GetStyle() & LVS_TYPEMASK;
	ModifyStyle(LBS_SORT, dwStyle | LBS_HASSTRINGS | LBS_OWNERDRAWFIXED);

	CFont* font = GetFont();

	if (font == NULL)
		font = AfxGetMainWnd()->GetFont();

	if (font != NULL)
		font->GetObject(sizeof(m_lf), &m_lf);
	else
		GetObject(GetStockObject(SYSTEM_FONT), sizeof(m_lf), &m_lf);

	ReconstructFont();
}

void CSCListBox::ReconstructFont()
{
	m_font.DeleteObject();
	BOOL bCreated = m_font.CreateFontIndirect(&m_lf);

	SetFont(&m_font, true);

	m_line_height = -m_lf.lfHeight + 10;
	SetItemHeight(0, -m_lf.lfHeight + 10);

	ASSERT(bCreated);
}

CSCListBox& CSCListBox::set_font(LOGFONT& lf)
{
	memcpy(&m_lf, &lf, sizeof(LOGFONT));
	ReconstructFont();

	return *this;
}

CSCListBox& CSCListBox::set_font_name(CString sFontname, BYTE byCharSet)
{
	m_lf.lfCharSet = byCharSet;
	_tcscpy_s(m_lf.lfFaceName, _countof(m_lf.lfFaceName), sFontname);
	ReconstructFont();

	return *this;
}

CSCListBox& CSCListBox::set_font_size(int nSize)
{
	HDC hDC = GetDC()->GetSafeHdc();
	m_lf.lfHeight = -MulDiv(nSize, GetDeviceCaps(hDC, LOGPIXELSY), 72);
	::ReleaseDC(m_hWnd, hDC);
	ReconstructFont();

	return *this;
}

CSCListBox& CSCListBox::set_font_bold(int weight)
{
	m_lf.lfWeight = weight;
	ReconstructFont();

	return *this;
}

//-------------------------------------------------------------------
//
void CSCListBox::DrawItem(LPDRAWITEMSTRUCT lpDIS) 
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
	CDC* pDC = CDC::FromHandle(lpDIS->hDC);

	//TRACE(_T("cur index() = %d\n"), (int)lpDIS->itemID);
	if ((int)lpDIS->itemID < GetTopIndex())
		return; 

	//bBg = false로 하면 최신 항목만 표시되고 다른 항목은 표시되지 않는 현상이 발생한다.
	CMemoryDC dc(pDC, NULL, true);	//=> 이대로 사용하면 점차 느려지는 현상 발생하여 사용하지 않았으나 현재는 재현안됨.
	pDC = &dc;
	Gdiplus::Graphics g(pDC->GetSafeHdc());

	CString		sText;
	Gdiplus::Color	cr_text;
	Gdiplus::Color	cr_back = m_theme.cr_back;

	cr_text.SetFromCOLORREF(lpDIS->itemData);	// Color information is in item data.

	CRect		rc;
	CRect		rect = lpDIS->rcItem;

	GetClientRect(rc);

	//현재 아이템의 top이 rc.bottom보다 아래라면 그릴필요없다.
	if (rect.top > rc.bottom)
		return;

	CRect		rGutter = rect;

	if (m_nGutterCharNumber > 0)
	{
		rGutter.right = rGutter.left + pDC->GetTextExtent(_T("M")).cx * m_nGutterCharNumber;
		rect.left = rGutter.right;
		CBrush brush(::GetSysColor(COLOR_3DFACE));
		pDC->FillRect(&rGutter, &brush);
		pDC->SetTextColor(RGB(128, 128, 128));
		sText.Format(_T("%d"), (int)lpDIS->itemID);
		rGutter.right -= 4;
		pDC->DrawText(sText, rGutter, DT_RIGHT | DT_SINGLELINE | DT_VCENTER);
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
		pDC->DrawFocusRect(&rect);

	//// If item does not have focus, redraw (erase) the focus rect.
	if ((lpDIS->itemAction & ODA_FOCUS) &&	!(lpDIS->itemState & ODS_FOCUS))
		pDC->DrawFocusRect(&rect);

	if (lpDIS->itemID == m_over_item)
	{
		cr_back = m_theme.cr_back_hover;
	}
	else
	{
		if (lpDIS->itemState & ODS_SELECTED)
		{
			cr_back = m_theme.cr_back_selected;
		}
		else
		{
			cr_back = m_theme.cr_back;
		}

	}

	//CBrush brush(cr_back);
	//dc.FillRect(&rect, &brush);
	draw_rect(pDC, rect, cr_back, cr_back, 1);

	if (lpDIS->itemState & ODS_SELECTED)
	{
		//선택 항목의 색은 자신의 색으로 그냥 그려준다.
		//cr_text = m_cr_text_selected;
		draw_rect(pDC, rect, GetFocus() ? m_theme.cr_selected_border : cr_back, cr_back, 1);
	}
	else if (!m_as_static && lpDIS->itemState & ODS_DISABLED)
	{
		cr_text = RGB2gpColor(::GetSysColor(COLOR_GRAYTEXT));
	}
	else
	{
		if (lpDIS->itemID == m_over_item)
			cr_text = m_theme.cr_text_hover;
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
		CString real_path = m_pShellImageList->convert_special_folder_to_real_path(!m_is_local, m_folder_list[lpDIS->itemID]);
		int img_index = (is_drive_root(real_path) ? m_pShellImageList->get_drive_icon(!m_is_local, real_path) : m_pShellImageList->GetSystemImageListIcon(!m_is_local, real_path, true));
		m_pShellImageList->m_imagelist_small.Draw(pDC, img_index, CPoint(rect.left, rect.CenterPoint().y - 8), ILD_TRANSPARENT);
		//m_pShellImageList->m_imagelist_small.Draw(pDC, m_folder_list[lpDIS->itemID].,
		//	CPoint(textRect.left, textRect.CenterPoint().y - 8), ILD_TRANSPARENT);
		rect.left += 16;	//small icon width
		rect.left += 14;	//margin between icon and text

		LOGFONT lf;

		if (lpDIS->itemState & ODS_SELECTED)
		{
			memcpy(&lf, &m_lf, sizeof(LOGFONT));
			lf.lfWeight = FW_SEMIBOLD;
			BOOL bCreated = font_selected.CreateFontIndirect(&lf);
			pOldFont = pDC->SelectObject(&font_selected);
		}
		else
		{
			pOldFont = pDC->SelectObject(&m_font);
		}
	}
	else if (m_imagelist.size() > 0)
	{
		pOldFont = pDC->SelectObject(&m_font);

		//scpark 20240313 원래 이미지 인덱스는 각 항목에 저장하는 DWORD를 활용하지만
		//이 클래스는 그 값을 색상저장용으로 사용하는 클래스로 만들어졌다.
		//struct를 새로 선언하고 text color, back color, font property, image index 등을 저장하여 사용할 수 있지만
		//메모리 동적할당 등 많이 복잡해진다. 우선 임시로 텍스트 색상이 blue면 1번 이미지, red면 2번 이미지를 사용한다.
		int image_index = 0;
		if (m_imagelist.size() > 1 && cr_text.GetValue() == Gdiplus::Color::RoyalBlue)
			image_index = 1;
		else if (m_imagelist.size() > 2 && cr_text.GetValue() == Gdiplus::Color::Red)
			image_index = 2;

		m_imagelist[image_index]->draw(g, rect.left, rect.top + (rect.Height() - m_imagelist[image_index]->height)/2);
		rect.left += (m_imagelist[image_index]->width + 4);
	}
	else
	{
		pOldFont = pDC->SelectObject(&m_font);
	}

	//text 출력 왼쪽 여백
	rect.left += 4;

	//rc의 오른쪽 끝 여백 설정
	rect.right -= 10;

	pDC->SetBkMode(TRANSPARENT);

	//시간값은 항상 옅은 회색으로만 표시
	int date_time_length = 0;

	if (m_show_time)
		date_time_length = 12;
	if (m_show_date)
		date_time_length += 10;

	if (date_time_length > 0)
	{
		CSize sz;
		CString time_str = sText.Left(date_time_length);
		GetTextExtentPoint32(pDC->GetSafeHdc(), time_str, time_str.GetLength(), &sz);
		rect.right = sz.cx;

		//CSize sz = dc.GetTextExtent(time_str);
		//TEXTMETRIC tm;
		//pDC->GetTextMetrics(&tm);

		//rect.right = sz.cx + tm.tmAveCharWidth;

		if (m_dim_time_str)
			pDC->SetTextColor(RGB(192, 192, 192));

		pDC->DrawText(time_str, rect, nFormat | DT_NOCLIP);

		sText = sText.Mid(date_time_length + 1);
		rect.left = rect.right + 8;
		rect.right = lpDIS->rcItem.right - 10;
	}

	//가로 스크롤시에 뭔가 rect영역이 부족해서 출력되지 않는 현상이 있어서 DT_NOCLIP을 추가함.
	pDC->SetTextColor(cr_text.ToCOLORREF());
	pDC->DrawText(sText, rect, nFormat | DT_NOCLIP);

	pDC->SelectObject(pOldFont);

	if (m_as_folder_list)
		font_selected.DeleteObject();
}	// DrawItem

/*
//기본 글자색으로 한 줄 추가
int	CSCListBox::add(LPCTSTR lpszFormat, ...)
{
	//가변인자를 그대로 전달하는 방법은 없다.
	//고정인자로 변경한 후 add함수를 호출해줘야 한다.

	CString new_text;
	va_list args;
	va_start(args, lpszFormat);
	new_text.FormatV(lpszFormat, args);

	return add(m_theme.cr_text, new_text);
}

int CSCListBox::add(Gdiplus::Color cr, LPCTSTR lpszFormat, ...)
{
	if (m_hWnd == NULL)
		return -1;

	if (!m_show_log)
		return -1;

	if (cr.GetValue() == Gdiplus::Color::Transparent)
		cr = m_theme.cr_text;

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
	int index = -1;					//AddString에 의해 삽입된 리스트 인덱스
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
		index = AddString(_T(""));

		if (new_text.IsEmpty())
			return index;
	}
	else
	{
		for (i = 0; i < pre_linefeed_count; i++)
		{
			AddString(_T(""));
		}
	}

	CString date_str, time_str;

	if ((m_show_date || m_show_time) && !skip_time_info)
	{
		SYSTEMTIME	t;

		::GetLocalTime(&t);

		date_str.Format(_T("%d-%02d-%02d"), t.wYear, t.wMonth, t.wDay);
		time_str.Format(_T("%02d:%02d:%02d.%03d"), t.wHour, t.wMinute, t.wSecond, t.wMilliseconds);

		if (m_show_time)
			new_text = time_str + _T(" ") + new_text;
		if (m_show_date)
			new_text = date_str + _T(" ") + new_text;
	}

	index = AddString(new_text);

	if (index >= 0)
	{
		if (cr.GetValue() == Gdiplus::Color::Transparent)
			SetItemData(index, m_theme.cr_text.ToCOLORREF());
		else
			SetItemData(index, cr.ToCOLORREF());

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

	recalc_horizontal_extent();

	return index;
}
*/

int CSCListBox::add(LPCTSTR text, ...)
{
	va_list args;
	va_start(args, text);

	CString new_text;

	new_text.FormatV(text, args);

	return insert_string(-1, new_text);
}

int CSCListBox::add(std::deque<CString> *lists, Gdiplus::Color cr)
{
	if (cr.GetValue() == Gdiplus::Color::Transparent)
		cr = m_theme.cr_text;

	for (int i = 0; i < lists->size(); i++)
		insert_string(-1, lists->at(i), cr);

	return lists->size();
}

//-------------------------------------------------------------------
//
int CSCListBox::insert_string(int nIndex, CString text, Gdiplus::Color rgb)
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
	CString date_str, time_str;

	if (m_show_date || m_show_time)
	{
		SYSTEMTIME	t;

		::GetLocalTime(&t);

		date_str.Format(_T("%d-%02d-%02d"), t.wYear, t.wMonth, t.wDay);
		time_str.Format(_T("%02d:%02d:%02d.%03d"), t.wHour, t.wMinute, t.wSecond, t.wMilliseconds);

		if (m_show_time)
			text = time_str + _T(" ") + text;
		if (m_show_date)
			text = date_str + _T(" ") + text;
	}

	int index = ((CListBox*)this)->InsertString(nIndex, text);
	if (index >= 0)
	{
		if (rgb.GetValue() == Gdiplus::Color::Transparent)
			rgb = m_theme.cr_text;
		SetItemData(index, rgb.ToCOLORREF());
		RedrawWindow();

		if (m_auto_scroll)
			SetTopIndex(index);
	}

	return index;
}	// InsertString

//-------------------------------------------------------------------
//
void CSCListBox::set_item_color(int nIndex, Gdiplus::Color rgb, bool invalidate)
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
	SetItemData(nIndex, rgb.ToCOLORREF());
	if (invalidate)
		RedrawWindow();
}

Gdiplus::Color CSCListBox::get_item_color(int nIndex)
{
	return (Gdiplus::Color)GetItemData(nIndex);
}

CString CSCListBox::get_text(int index)
{
	CString text;

	if (index < 0 || index >= GetCount())
	{
		TRACE(_T("out of range of listbox index = %d.\n"), index);
		return text;
	}

	GetText(index, text);

	return text;
}

void CSCListBox::set_text(int index, CString text, Gdiplus::Color cr)
{
	if (index < 0 || index >= GetCount())
	{
		TRACE(_T("out of range of listbox index = %d.\n"), index);
		return;
	}

	SetRedraw(FALSE);
	DeleteString(index);
	insert_string(index, text, cr);
	SetRedraw(TRUE);
	UpdateWindow();
}

void CSCListBox::OnMouseMove(UINT nFlags, CPoint point)
{
	// TODO: Add your message handler code here and/or call default
	if (m_use_over)
	{
		BOOL outside = false;
		UINT nHover = ItemFromPoint(point, outside);
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

void CSCListBox::OnPaint()
{
	CListBox::OnPaint();

	CPaintDC dc1(this);
	CRect rc;

	GetClientRect(rc);
	
	CMemoryDC dc(&dc1, &rc);

	//DrawItem()에서 각 아이템을 모두 그린 후 OnPaint()에서 border를 그리려 했으나
	//DrawItem()에서 그려진 내용이 모두 가려진다. 우선 WM_PAINT는 주석처리한다.
	draw_rect(&dc, rc, m_theme.cr_border);
}

BOOL CSCListBox::OnEraseBkgnd(CDC* pDC)
{
	// TODO: Add your message handler code here and/or call default

	//여기서 배경색으로 칠해주지 않으면 항목이 없는 영역은 다른 색으로 채워져있다.
	//CRect rc;
	//GetClientRect(rc);
	//pDC->FillSolidRect(rc, m_theme.cr_back.ToCOLORREF());
	//draw_line(pDC, 0, 0, 100, 100);

	return FALSE;
	return CListBox::OnEraseBkgnd(pDC);
}

CSize CSCListBox::resizeToFit(bool bHori, bool bVert)
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

void CSCListBox::set_line_height(int _line_height)
{
	if (_line_height < 8)
		_line_height = 8;

	m_line_height = _line_height;
	SetItemHeight(0, m_line_height);
	Invalidate();
}

BOOL CSCListBox::PreTranslateMessage(MSG* pMsg)
{
	// TODO: Add your specialized code here and/or call the base class
	if (pMsg->message == WM_KEYDOWN)
	{
		TRACE(_T("CSCListBox message = %d\n"), pMsg->wParam);

		switch (pMsg->wParam)
		{
		case VK_F2:
			if (!m_use_edit)
				return CListBox::PreTranslateMessage(pMsg);
			edit(-1);
			return true;
		case VK_RETURN :
			if (m_use_edit && m_in_editing)
			{
				edit_end();
				return true;
			}
			break;
		case VK_ESCAPE:
			if (m_use_edit && m_in_editing)
			{
				edit_end(false);
				return true;
			}
			break;
		case VK_DELETE:
			delete_items();
			return true;
		}
	}
	else if (pMsg->message == WM_KILLFOCUS)
	{
		TRACE(_T("%d, %d\n"), pMsg->wParam, pMsg->lParam);
	}
	else if (pMsg->message == WM_LBUTTONDBLCLK)
	{
		if (m_use_edit && !m_in_editing)
		{
			edit();
			return true;
		}
	}

	return CListBox::PreTranslateMessage(pMsg);
}


void CSCListBox::OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags)
{
	// TODO: Add your message handler code here and/or call default
	TRACE(_T("CSCListBox::OnKeyDown = %d\n"), nChar);
	//HWND hWnd = GetParent()->GetSafeHwnd();
	//::PostMessage(hWnd, WM_KEYDOWN, (WPARAM)nChar, 0);
	CListBox::OnKeyDown(nChar, nRepCnt, nFlags);
}
/*
void CSCListBox::set_color_theme(int theme, bool apply_now)
{
	switch (theme)
	{
	case color_theme_default:
		m_cr_text.SetFromCOLORREF(::GetSysColor(COLOR_BTNTEXT));
		m_cr_text_selected.SetFromCOLORREF(::GetSysColor(COLOR_HIGHLIGHTTEXT));
		m_cr_text_selected_inactive.SetFromCOLORREF(::GetSysColor(COLOR_INACTIVECAPTIONTEXT));
		m_cr_text_over.SetFromCOLORREF(::GetSysColor(COLOR_HIGHLIGHTTEXT));

		m_cr_back.SetFromCOLORREF(::GetSysColor(COLOR_WINDOW));
		m_cr_back_selected = Gdiplus::Color(255, 204, 232, 255);	//m_cr_back_selected = ::GetSysColor(COLOR_HIGHLIGHT);
		m_cr_back_selected_rect = Gdiplus::Color(255, 153, 209, 255);
		m_cr_back_selected_inactive.SetFromCOLORREF(::GetSysColor(COLOR_HIGHLIGHT));
		m_cr_back_over = Gdiplus::Color(255, 195, 222, 245); //m_cr_back_over = ::GetSysColor(COLOR_HIGHLIGHT);
		break;
	case color_theme_explorer:
		m_cr_text.SetFromCOLORREF(::GetSysColor(COLOR_BTNTEXT));
		m_cr_text_selected = m_cr_text;// ::GetSysColor(COLOR_HIGHLIGHTTEXT);
		m_cr_text_selected_inactive.SetFromCOLORREF(::GetSysColor(COLOR_INACTIVECAPTIONTEXT));
		m_cr_text_over = m_cr_text;

		m_cr_back.SetFromCOLORREF(::GetSysColor(COLOR_WINDOW)); //RGB(242, 242, 242);// ::GetSysColor(COLOR_WINDOW);
		m_cr_back_selected = Gdiplus::Color(255, 204, 232, 255);// ::GetSysColor(COLOR_HIGHLIGHT);
		m_cr_back_selected_rect = Gdiplus::Color(255, 153, 209, 255);
		m_cr_back_selected_inactive.SetFromCOLORREF(::GetSysColor(COLOR_HIGHLIGHT));
		m_cr_back_over = Gdiplus::Color(255, 195, 222, 245);
		break;
	case color_theme_popup_folder_list:
		m_cr_text.SetFromCOLORREF(::GetSysColor(COLOR_BTNTEXT));
		m_cr_text_selected = m_cr_text;// ::GetSysColor(COLOR_HIGHLIGHTTEXT);
		m_cr_text_selected_inactive.SetFromCOLORREF(::GetSysColor(COLOR_INACTIVECAPTIONTEXT));
		m_cr_text_over = m_cr_text;

		m_cr_back.SetFromCOLORREF(::GetSysColor(COLOR_3DFACE)); //RGB(242, 242, 242);// ::GetSysColor(COLOR_WINDOW);
		m_cr_back_selected = Gdiplus::Color(255, 204, 232, 255);// ::GetSysColor(COLOR_HIGHLIGHT);
		m_cr_back_selected_rect = Gdiplus::Color(255, 153, 209, 255);
		m_cr_back_selected_inactive.SetFromCOLORREF(::GetSysColor(COLOR_HIGHLIGHT));
		m_cr_back_over = Gdiplus::Color(255, 195, 222, 245);
		break;
	}

	if (apply_now)
		Invalidate();
}
*/

void CSCListBox::OnKillFocus(CWnd* pNewWnd)
{
	CListBox::OnKillFocus(pNewWnd);

	// TODO: 여기에 메시지 처리기 코드를 추가합니다.
	if (m_as_popup)
		ShowWindow(SW_HIDE);

	if (m_use_edit && m_in_editing)
		edit_end();
}

int CSCListBox::set_path(CString root, CString selected_text)
{
	root = m_pShellImageList->convert_special_folder_to_real_path(!m_is_local, root);

	if (root.IsEmpty())
	{
		m_folder_list.clear();
		m_folder_list.push_back(get_system_label(CSIDL_DRIVES));
		m_folder_list.push_back(get_system_label(CSIDL_MYDOCUMENTS));
		m_folder_list.push_back(get_system_label(CSIDL_DESKTOP));
	}
	else
	{
		get_sub_folders(root, &m_folder_list, true);
	}

	return set_folder_list(NULL, selected_text);
}

void CSCListBox::set_as_folder_list()
{
	m_use_over = true;
	m_use_popup_menu = false;
	m_as_folder_list = true;
	m_as_popup = true;
	m_show_time = false;
	m_theme.cr_back = ::GetSysColor(COLOR_3DFACE);
}

int CSCListBox::set_folder_list(std::deque<CString>* lists, CString selected_text)
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
		insert_string(-1, get_part(m_folder_list[i], fn_name));
	}

	SelectString(-1, selected_text);

	Invalidate();

	return m_folder_list.size();
}

BOOL CSCListBox::OnLbnSelchange()
{
	int index = GetCurSel();

	if (index < 0 || index >= GetCount())
		return FALSE;

	if (m_use_edit && m_in_editing)
	{
		edit_end();
	}

	//Ctrl+End 또는 PageDown으로 맨 마지막 항목이 선택되면 자동 스크롤,
	//그 외 항목이 선택되면 자동 스크롤을 멈춘다.
	m_auto_scroll = (index == GetCount() - 1);

	if (m_as_popup)
	{
		CString text;
		
		if (m_as_folder_list)
			text = m_folder_list[index];
		else
			GetText(index, text);

		if (m_hParentWnd)
		{
			if (m_as_popup)
				ShowWindow(SW_HIDE);

			::SendMessage(m_hParentWnd, Message_CSCListBox, (WPARAM)&CSCListBoxMessage(this, message_sclistbox_selchanged), (LPARAM)&text);
		}
	}

	return FALSE;
}

//선택된 항목 리스트 또는 선택된 개수를 리턴
int CSCListBox::get_selected_items(std::deque<int>* selected)
{
	int selected_count = GetSelCount();

	if (selected)
		selected->clear();

	if (selected_count > 0)
	{
		CArray<int, int> aryListBoxSel;
		aryListBoxSel.SetSize(selected_count);
		GetSelItems(selected_count, aryListBoxSel.GetData());

		for (int i = 0; i < aryListBoxSel.GetCount(); i++)
			selected->push_back(aryListBoxSel[i]);
	}
		
	return selected_count;
}


void CSCListBox::OnContextMenu(CWnd* pWnd, CPoint point)
{
	if (!m_use_popup_menu)
		return;

	//만약 선택되지 않은 항목에 우클릭했다면
	//다른 선택항목들은 모두 선택 해제시키고 해당 항목을 선택으로 만든 후에 팝업메뉴를 띠워줘야 한다.
	//사용자는 우클릭한 항목에 대한 어떤 명령을 수행하려 했기 때문이다.
	std::deque<int> selected;
	get_selected_items(&selected);

	BOOL bOutside;
	CPoint pt = point;
	ScreenToClient(&pt);

	int rclicked_index = ItemFromPoint(pt, bOutside);

	if (rclicked_index >= 0 && rclicked_index < GetCount())
	{
		if (find_index(selected, rclicked_index) < 0)
		{
			for (int i = 0; i < selected.size(); i++)
				SetSel(selected[i], FALSE);

			selected.clear();
			selected.push_back(rclicked_index);
			SetSel(rclicked_index);
			TRACE(_T("rclicked_index = %d\n"), rclicked_index);
		}
	}

	CMenu menu;

	menu.CreatePopupMenu();

	if (selected.size() > 0)
	{
		menu.AppendMenu(MF_STRING, menu_selected_count, i2S(selected.size()) + _T(" item(s) selected"));
		menu.EnableMenuItem(menu_selected_count, MF_DISABLED);
	}

	menu.AppendMenu(MF_SEPARATOR);
	menu.AppendMenu(MF_STRING, menu_clear_all, _T("Clear all(&L)"));

	menu.AppendMenu(MF_SEPARATOR);
	menu.AppendMenu(MF_STRING, menu_show_log, _T("Show log"));
	menu.CheckMenuItem(menu_show_log, m_show_log ? MF_CHECKED : MF_UNCHECKED);

	menu.AppendMenu(MF_SEPARATOR);
	menu.AppendMenu(MF_STRING, menu_show_time, _T("Show time"));
	menu.CheckMenuItem(menu_show_time, m_show_time ? MF_CHECKED : MF_UNCHECKED);
	menu.AppendMenu(MF_STRING, menu_show_date, _T("Show date"));
	menu.CheckMenuItem(menu_show_date, m_show_date ? MF_CHECKED : MF_UNCHECKED);

	menu.AppendMenu(MF_SEPARATOR);
	menu.AppendMenu(MF_STRING, menu_auto_scroll, _T("Auto scroll\tCtrl+End"));
	menu.CheckMenuItem(menu_auto_scroll, m_auto_scroll ? MF_CHECKED : MF_UNCHECKED);

	menu.AppendMenu(MF_SEPARATOR);
	menu.AppendMenu(MF_STRING, menu_copy_selected_to_clipboard, _T("Copy selected items"));
	menu.AppendMenu(MF_STRING, menu_copy_all_to_clipboard, _T("Copy all items(&C)"));

	menu.AppendMenu(MF_SEPARATOR);
	menu.AppendMenu(MF_STRING, menu_save_selected, _T("Save selected items to a file..."));
	menu.AppendMenu(MF_STRING, menu_save_all, _T("Save all items to a file...(&S)"));

	menu.AppendMenu(MF_SEPARATOR);
	menu.AppendMenu(MF_STRING, menu_delete_selected, _T("Delete selected items(&D)"));

	menu.TrackPopupMenu(TPM_LEFTALIGN, point.x, point.y, this);
	menu.DestroyMenu();
}

void CSCListBox::OnPopupMenu(UINT nMenuID)
{
	switch (nMenuID)
	{
	case menu_show_log:
		m_show_log = !m_show_log;
		AfxGetApp()->WriteProfileInt(_T("setting\\SCListBox"), _T("show log"), m_show_log);
		break;
	case menu_show_date:
		m_show_date = !m_show_date;
		AfxGetApp()->WriteProfileInt(_T("setting\\SCListBox"), _T("show date"), m_show_date);
		break;
	case menu_show_time:
		m_show_time = !m_show_time;
		AfxGetApp()->WriteProfileInt(_T("setting\\SCListBox"), _T("show time"), m_show_time);
		break;
	case menu_auto_scroll:
		m_auto_scroll = !m_auto_scroll;
		AfxGetApp()->WriteProfileInt(_T("setting\\SCListBox"), _T("auto scroll"), m_auto_scroll);
		break;
	case menu_clear_all:
		delete_items(false);
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
	case menu_delete_selected:
		delete_items(true);
	}
}

//1:show, 0:hide, -1:no change
void CSCListBox::show_date_time(int date, int time)
{
	if (date >= 0)
		m_show_date = date;

	if (time >= 0)
		m_show_time = time;
}

CString CSCListBox::get_all_text(bool selected_only)
{
	int i;
	CString text;
	CString result;

	if (selected_only)
	{
		std::deque<int> selected;
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

	result.Trim();

	return result;
}

void CSCListBox::copy_selected_to_clipboard()
{
	CString text = get_all_text(true);
	copy_to_clipboard(m_hWnd, text);
}

void CSCListBox::copy_all_to_clipboard()
{
	CString text = get_all_text();
	copy_to_clipboard(m_hWnd, text);
}

void CSCListBox::save_selected_to_file()
{
	CString recent = AfxGetApp()->GetProfileString(_T("setting\\SCListBox"), _T("recent saved file"), _T(""));

	if (recent.IsEmpty())
		recent.Format(_T("%s\\logs.txt"), get_exe_directory());
	
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

void CSCListBox::save_all_to_file()
{
	CString recent = AfxGetApp()->GetProfileString(_T("setting\\SCListBox"), _T("recent saved file"), _T(""));

	if (recent.IsEmpty())
		recent.Format(_T("%s\\logs.txt"), get_exe_directory());

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


BOOL CSCListBox::OnMouseWheel(UINT nFlags, short zDelta, CPoint pt)
{
	// TODO: 여기에 메시지 처리기 코드를 추가 및/또는 기본값을 호출합니다.
	
	//mouse wheel 이벤트가 발생하면 자동 스크롤을 멈춘다.
	m_auto_scroll = false;

	return CListBox::OnMouseWheel(nFlags, zDelta, pt);
}


void CSCListBox::OnMouseHWheel(UINT nFlags, short zDelta, CPoint pt)
{
	// 이 기능을 사용하려면 Windows Vista 이상이 있어야 합니다.
	// _WIN32_WINNT 기호는 0x0600보다 크거나 같아야 합니다.
	// TODO: 여기에 메시지 처리기 코드를 추가 및/또는 기본값을 호출합니다.
	// 
	//로그를 일시 중지시킨 후 가로 스크롤 할 
	//Invalidate();
	RedrawWindow();
	UpdateWindow();
	CListBox::OnMouseHWheel(nFlags, zDelta, pt);
}


void CSCListBox::OnSize(UINT nType, int cx, int cy)
{
	CListBox::OnSize(nType, cx, cy);

	// TODO: 여기에 메시지 처리기 코드를 추가합니다.
	//자동 스크롤을 off상태에서 세로로 resize하면 목록이 잘 갱신되지만
	//가로로 resize 또는 가로로 스크롤하면 텍스트들이 표시가 안된다.
	//time_str과 선택된 항목은 잘 나타난다.
	//여기서 Invalidate()해서 해결할 문제가 아니다. DrawItem()에서 원인을 찾자.
	//DrawItem()에서 DrawText()할때 NO_CLIP을 주고 여기서 Invalidate()하면
	//전혀 문제는 없이 잘 갱신은 되지만 왜 이런 현상이 발생했는지는 아직 미지수임.
	Invalidate();
	RedrawWindow();
	UpdateWindow();
}


void CSCListBox::edit(int index)
{
	if (index < 0 || index >= GetCount())
	{
		index = GetCurSel();
		if (index < 0)
			return;
	}

	m_edit_index = index;

	CRect rItem;
	GetItemRect(index, rItem);

	CString text;
	GetText(index, text);

	CClientDC dc(this);
	CSize sz = dc.GetTextExtent(text);
	//rItem.top = (rItem.Height() - sz.cy) / 2;

	if (!m_pEdit)
	{
		DWORD dwStyle = WS_BORDER | WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL | (m_edit_readonly ? ES_READONLY : 0);
		m_pEdit = new CEdit();
		m_pEdit->Create(dwStyle, rItem, this, IDC_EDIT_CELL);
		m_pEdit->SetFont(&m_font);
		m_pEdit->SetMargins(0, 20);	//이건 적용되지 않는다. 세로로 중앙정렬하고 싶다면 text height를 구해서 보정해야 함.
	}

	m_pEdit->SetWindowText(text);
	m_pEdit->MoveWindow(rItem);
	m_pEdit->ShowWindow(SW_SHOW);

	m_pEdit->SetSel(0, -1);
	m_pEdit->SetFocus();

	m_in_editing = true;
}

//modify가 true이면 편집된 텍스트로 변경, 그렇지 않으면 기존 텍스트 유지.
void CSCListBox::edit_end(bool modify)
{
	if (!m_use_edit || !m_in_editing || m_edit_index < 0 || m_edit_index >= GetCount())
		return;

	m_pEdit->ShowWindow(SW_HIDE);

	if (modify)
	{
		DWORD cr;
		CString text;
		m_pEdit->GetWindowText(text);
		TRACE(_T("index = %d\n"), m_edit_index);

		cr = GetItemData(m_edit_index);
		DeleteString(m_edit_index);
		insert_string(m_edit_index, text, cr);
	}

	m_in_editing = false;
}


void CSCListBox::OnLButtonDown(UINT nFlags, CPoint point)
{
	// TODO: 여기에 메시지 처리기 코드를 추가 및/또는 기본값을 호출합니다.

	CListBox::OnLButtonDown(nFlags, point);
}


void CSCListBox::OnLButtonUp(UINT nFlags, CPoint point)
{
	// TODO: 여기에 메시지 처리기 코드를 추가 및/또는 기본값을 호출합니다.

	CListBox::OnLButtonUp(nFlags, point);
}

void CSCListBox::add_to_imagelist(UINT id)
{
	CSCGdiplusBitmap* img = new CSCGdiplusBitmap(_T("PNG"), (UINT)id);
	m_imagelist.push_back(img);
}


void CSCListBox::OnWindowPosChanged(WINDOWPOS* lpwndpos)
{
	CListBox::OnWindowPosChanged(lpwndpos);

	// TODO: 여기에 메시지 처리기 코드를 추가합니다.
	//창의 일부를 모니터 밖으로 나갔다가 들어올 경우 화면이 갱신되지 않는 현상으로 추가.
	//여기에 넣어도 동일 오류가 있다. 우선은 parent에서 처리함.
	//Invalidate();
	//RedrawWindow();
	//UpdateWindow();
}

void CSCListBox::delete_items(bool only_selected)
{
	if (!only_selected)
	{
		ResetContent();
	}
	else
	{
		int count = GetSelCount();

		if (count == 0)
			return;
		
		int* sel_list = new int[count];
		GetSelItems(count, sel_list);

		for (int i = count - 1; i >= 0; i--)
		{
			DeleteString(sel_list[i]);
		}
	}

	Invalidate();
	RedrawWindow();
	UpdateWindow();

	recalc_horizontal_extent();
}

void CSCListBox::recalc_horizontal_extent(CString added_text)
{
	//항목의 출력 너비에 따라 가로 스크롤바를 재조정해준다.
	CDC* pDC = GetDC();
	CFont* pOldFont = (CFont*)pDC->SelectObject(&m_font);
	CSize sz;

	if (added_text.IsEmpty())
	{
		CString text;
		int max_cx = 0;

		for (int i = 0; i < GetCount(); i++)
		{
			GetText(i, text);
			GetTextExtentPoint32(pDC->GetSafeHdc(), added_text, added_text.GetLength(), &sz);
			if (sz.cx > max_cx)
				sz.cx = max_cx;
		}
	}
	else
	{
		GetTextExtentPoint32(pDC->GetSafeHdc(), added_text, added_text.GetLength(), &sz);
	}

	if (sz.cx > m_max_horizontal_extent)
	{
		m_max_horizontal_extent = sz.cx;
	}

	pDC->SelectObject(pOldFont);
	ReleaseDC(pDC);

	SetHorizontalExtent(m_max_horizontal_extent + GetSystemMetrics(SM_CXVSCROLL));
}

void CSCListBox::set_color_theme(int theme)
{
	m_theme.set_color_theme(theme);

	if (!m_hWnd)
		return;

	Invalidate();
}


void CSCListBox::OnNcPaint()
{
	// TODO: 여기에 메시지 처리기 코드를 추가합니다.
	// 그리기 메시지에 대해서는 CListBox::OnNcPaint()을(를) 호출하지 마십시오.
}


HBRUSH CSCListBox::OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor)
{
	HBRUSH hbr = CListBox::OnCtlColor(pDC, pWnd, nCtlColor);

	// TODO:  여기서 DC의 특성을 변경합니다.
	// TODO:  기본값이 적당하지 않으면 다른 브러시를 반환합니다.
	return hbr;
}
