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
		CGdiplusBitmap* img = m_imagelist[i];
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

	//�̰� �� ������ �ȵɱ�...
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

CSCListBox& CSCListBox::set_font_bold(bool bBold)
{
	m_lf.lfWeight = (bBold ? FW_BOLD : FW_NORMAL);
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
	TRACE(_T("GetTopIndex() = %d\n"), GetTopIndex());
	if ((int)lpDIS->itemID < GetTopIndex())
		return; 

	CDC* pDC = CDC::FromHandle(lpDIS->hDC);
	CMemoryDC dc(pDC, NULL, true);

	Gdiplus::Graphics g(dc.GetSafeHdc());

	CString		sText;
	Gdiplus::Color	cr_text;
	Gdiplus::Color	cr_back = m_theme.cr_back;

	cr_text.SetFromCOLORREF(lpDIS->itemData);	// Color information is in item data.

	CRect		rc;
	CRect		rect = lpDIS->rcItem;

	GetClientRect(rc);
	if (rect.top > rc.bottom)
		return;
	//rect.DeflateRect(1, 1);
	//rect.right -= 4;
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

	//�ٸ� ��Ʈ�ѿ����� ReconstructFont()�ȿ��� SetFont(&m_font, true);�� ���� �۲��� �����Ű����
	//custom draw�� ��������.
	//owner draw fixed �Ӽ��� ��Ʈ���� drawitem������ dc.SelectObject(&m_font)�� ȣ������� ����� ���� �۲��� ����ȴ�.

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
	draw_rectangle(&dc, rect, cr_back, cr_back, 1);

	if (lpDIS->itemState & ODS_SELECTED)
	{
		//���� �׸��� ���� �ڽ��� ������ �׳� �׷��ش�.
		//cr_text = m_cr_text_selected;
		draw_rectangle(&dc, rect, GetFocus() ? m_theme.cr_back_selected_border : cr_back, cr_back, 1);
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
	else if (m_imagelist.size() > 0)
	{
		pOldFont = dc.SelectObject(&m_font);

		//scpark 20240313 ���� �̹��� �ε����� �� �׸� �����ϴ� DWORD�� Ȱ��������
		//�� Ŭ������ �� ���� ������������� ����ϴ� Ŭ������ ���������.
		//struct�� ���� �����ϰ� text color, back color, font property, image index ���� �����Ͽ� ����� �� ������
		//�޸� �����Ҵ� �� ���� ����������. �켱 �ӽ÷� �ؽ�Ʈ ������ blue�� 1�� �̹���, red�� 2�� �̹����� ����Ѵ�.
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
		pOldFont = dc.SelectObject(&m_font);
	}

	//text ��� ���� ����
	rect.left += 4;

	//rc�� ������ �� ���� ����
	rect.right -= 10;

	dc.SetBkMode(TRANSPARENT);

	//�ð����� �׻� ���� ȸ�����θ� ǥ��
	int date_time_length = 0;
	if (m_show_date)
		date_time_length += 10;
	if (m_show_time)
		date_time_length += 12;
	if (m_show_date && m_show_time)
		date_time_length++;

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
			dc.SetTextColor(RGB(192, 192, 192));

		dc.DrawText(time_str, rect, nFormat | DT_NOCLIP);

		sText = sText.Mid(date_time_length + 1);
		rect.left = rect.right + 8;
		rect.right = lpDIS->rcItem.right - 10;
	}

	//���� ��ũ�ѽÿ� ���� rect������ �����ؼ� ��µ��� �ʴ� ������ �־ DT_NOCLIP�� �߰���.
	dc.SetTextColor(cr_text.ToCOLORREF());
	dc.DrawText(sText, rect, nFormat | DT_NOCLIP);

	dc.SelectObject(pOldFont);

	if (m_as_folder_list)
		font_selected.DeleteObject();
}	// DrawItem


//�⺻ ���ڻ����� �� �� �߰�
int	CSCListBox::add(LPCTSTR lpszFormat, ...)
{
	//�������ڸ� �״�� �����ϴ� ����� ����.
	//�������ڷ� ������ �� add�Լ��� ȣ������� �Ѵ�.

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

	//CString���� ��ȯ
	CString new_text;
	va_list args;
	va_start(args, lpszFormat);
	new_text.FormatV(lpszFormat, args);


	//���� �ؽ�Ʈ�� �� �տ� \n�� �پ� ������ ���� �α� ���ΰ� ������ �����ϱ� �����ε�
	//�׳� ����ϸ� �ð������� ����� �� ������ ����ȴ�.
	//text�� �� �տ� \n�� �ִٸ� ���� ó�����ش�.
	int i;
	int pre_linefeed_count = 0;		//�տ� ���� \n ó��
	int post_linefeed_count = 0;	//�ڿ� ���� \n ó��
	int index = -1;					//AddString�� ���� ���Ե� ����Ʈ �ε���
	bool skip_time_info = false;

	for (i = 0; i < new_text.GetLength(); i++)
	{
		if (new_text[i] == '\n')
			pre_linefeed_count++;
		else
			break;
	}

	//new_text�� �պκп� �ִ� '\n'�� ������ ������ ���ڿ�
	new_text = new_text.Mid(pre_linefeed_count);

	//CEdit��Ʈ�Ѱ��� �޸� �ڿ� ���� \n�� ������ ó������� �Ѵ�.
	i = new_text.GetLength() - 1;
	while (i >= 0 && new_text[i--] == '\n')
	{
		post_linefeed_count++;
	}


	//�տ� ���� '\n'�� ������ŭ ���� �߰�
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

		//�ڿ� �پ��� '\n'������ŭ �� ���� �߰����ش�.
		for (i = 0; i < post_linefeed_count; i++)
		{
			AddString(_T(""));
		}

		//if (invalidate)
			//RedrawWindow();
			//Invalidate();	//Invalidate()�� ȣ���ϸ� �� ������ �׸��� ���ŵ��� �ʴ� height�� �ִ�.

		if (m_auto_scroll)
			SetTopIndex(GetCount() - 1);
	}

	recalc_horizontal_extent();

	return index;
}

int CSCListBox::add(std::deque<CString> *lists, Gdiplus::Color cr)
{
	if (cr.GetValue() == Gdiplus::Color::Transparent)
		cr = m_theme.cr_text;

	for (int i = 0; i < lists->size(); i++)
		add(cr, lists->at(i));

	return lists->size();
}

//-------------------------------------------------------------------
//
int CSCListBox::insert_string(int nIndex, CString lpszItem)
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
int CSCListBox::insert_string(int nIndex, CString lpszItem, Gdiplus::Color rgb)
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
		if (rgb.GetValue() == Gdiplus::Color::Transparent)
			rgb = m_theme.cr_text;
		SetItemData(index, rgb.ToCOLORREF());
		RedrawWindow();
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
			//over�϶� �ش� �������� selected�� �ϸ� ��������
			//over�� selected�� ������ ó���ϰ��� ��.
			Invalidate(false);
		}
	}

	CListBox::OnMouseMove(nFlags, point);
}

void CSCListBox::OnPaint()
{
	CListBox::OnPaint();

	//CPaintDC dc(this);
	//CRect rc;

	//GetClientRect(rc);
	//dc.FillSolidRect(rc, RGB(255, 0, 0));
}

BOOL CSCListBox::OnEraseBkgnd(CDC* pDC)
{
	// TODO: Add your message handler code here and/or call default

	//���⼭ �������� ĥ������ ������ �׸��� ���� ������ �ٸ� ������ ä�����ִ�.
	CRect rc;
	GetClientRect(rc);
	pDC->FillSolidRect(rc, m_theme.cr_back.ToCOLORREF());

	return TRUE;
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
		//TRACE(_T("CSCListBox message = %d\n"), pMsg->wParam);

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
	//TRACE(_T("CSCListBox::OnKeyDown = %d\n"), nChar);
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

	// TODO: ���⿡ �޽��� ó���� �ڵ带 �߰��մϴ�.
	if (m_as_popup)
		ShowWindow(SW_HIDE);

	if (m_use_edit && m_in_editing)
		edit_end();
}

int CSCListBox::set_path(CString root, CString selected_text)
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
		add(get_part(m_folder_list[i], fn_name));
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

	//Ctrl+End �Ǵ� PageDown���� �� ������ �׸��� ���õǸ� �ڵ� ��ũ��,
	//�� �� �׸��� ���õǸ� �ڵ� ��ũ���� �����.
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
			::SendMessage(m_hParentWnd, Message_CSCListBox, (WPARAM)&CSCListBoxMessage(this, message_sclistbox_selchanged), (LPARAM)&text);

			if (m_as_popup)
				ShowWindow(SW_HIDE);
		}
	}

	return FALSE;
}

//���õ� �׸� ����Ʈ �Ǵ� ���õ� ������ ����
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

	//���� ���õ��� ���� �׸� ��Ŭ���ߴٸ�
	//�ٸ� �����׸���� ��� ���� ������Ű�� �ش� �׸��� �������� ���� �Ŀ� �˾��޴��� ������ �Ѵ�.
	//����ڴ� ��Ŭ���� �׸� ���� � ����� �����Ϸ� �߱� �����̴�.
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

	menu.AppendMenu(MF_STRING, menu_show_date, _T("Show date"));
	menu.CheckMenuItem(menu_show_date, m_show_date ? MF_CHECKED : MF_UNCHECKED);

	menu.AppendMenu(MF_SEPARATOR);
	menu.AppendMenu(MF_STRING, menu_show_time, _T("Show time"));
	menu.CheckMenuItem(menu_show_time, m_show_time ? MF_CHECKED : MF_UNCHECKED);

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
	
	CFileDialog dlg(false, _T("*.txt"), recent, OFN_EXPLORER | OFN_OVERWRITEPROMPT | OFN_LONGNAMES, _T("*Text file|*.txt|��� ����|*.*||"));
	if (dlg.DoModal() == IDCANCEL)
		return;

	CString text = get_all_text(true);

	save(dlg.GetPathName(), text, CP_UTF8);
	//CT2A(text)			: ANSI�� ����
	//CT2CA(text, CP_UTF8)	: UTF8�� �����
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

	CFileDialog dlg(false, _T("*.txt"), recent, OFN_EXPLORER | OFN_OVERWRITEPROMPT | OFN_LONGNAMES, _T("Text file|*.txt|��� ����|*.*||"));
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


	//CT2A(text)			: ANSI�� ����
	//CT2CA(text, CP_UTF8)	: UTF8�� �����
	//std::ofstream of;
	//of.open(dlg.GetPathName(), std::ofstream::out);
	//of << CT2CA(text, CP_UTF8);
	//of.close();
	save(dlg.GetPathName(), text, CP_UTF8);
	//save(dlg.GetPathName(), text, CP_ACP);
}


BOOL CSCListBox::OnMouseWheel(UINT nFlags, short zDelta, CPoint pt)
{
	// TODO: ���⿡ �޽��� ó���� �ڵ带 �߰� ��/�Ǵ� �⺻���� ȣ���մϴ�.
	
	//mouse wheel �̺�Ʈ�� �߻��ϸ� �ڵ� ��ũ���� �����.
	m_auto_scroll = false;

	return CListBox::OnMouseWheel(nFlags, zDelta, pt);
}


void CSCListBox::OnMouseHWheel(UINT nFlags, short zDelta, CPoint pt)
{
	// �� ����� ����Ϸ��� Windows Vista �̻��� �־�� �մϴ�.
	// _WIN32_WINNT ��ȣ�� 0x0600���� ũ�ų� ���ƾ� �մϴ�.
	// TODO: ���⿡ �޽��� ó���� �ڵ带 �߰� ��/�Ǵ� �⺻���� ȣ���մϴ�.
	// 
	//�α׸� �Ͻ� ������Ų �� ���� ��ũ�� �� 
	//Invalidate();
	RedrawWindow();
	UpdateWindow();
	CListBox::OnMouseHWheel(nFlags, zDelta, pt);
}


void CSCListBox::OnSize(UINT nType, int cx, int cy)
{
	CListBox::OnSize(nType, cx, cy);

	// TODO: ���⿡ �޽��� ó���� �ڵ带 �߰��մϴ�.
	//�ڵ� ��ũ���� off���¿��� ���η� resize�ϸ� ����� �� ���ŵ�����
	//���η� resize �Ǵ� ���η� ��ũ���ϸ� �ؽ�Ʈ���� ǥ�ð� �ȵȴ�.
	//time_str�� ���õ� �׸��� �� ��Ÿ����.
	//���⼭ Invalidate()�ؼ� �ذ��� ������ �ƴϴ�. DrawItem()���� ������ ã��.
	//DrawItem()���� DrawText()�Ҷ� NO_CLIP�� �ְ� ���⼭ Invalidate()�ϸ�
	//���� ������ ���� �� ������ ������ �� �̷� ������ �߻��ߴ����� ���� ��������.
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
		m_pEdit->SetMargins(0, 20);	//�̰� ������� �ʴ´�. ���η� �߾������ϰ� �ʹٸ� text height�� ���ؼ� �����ؾ� ��.
	}

	m_pEdit->SetWindowText(text);
	m_pEdit->MoveWindow(rItem);
	m_pEdit->ShowWindow(SW_SHOW);

	m_pEdit->SetSel(0, -1);
	m_pEdit->SetFocus();

	m_in_editing = true;
}

//modify�� true�̸� ������ �ؽ�Ʈ�� ����, �׷��� ������ ���� �ؽ�Ʈ ����.
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
	// TODO: ���⿡ �޽��� ó���� �ڵ带 �߰� ��/�Ǵ� �⺻���� ȣ���մϴ�.

	CListBox::OnLButtonDown(nFlags, point);
}


void CSCListBox::OnLButtonUp(UINT nFlags, CPoint point)
{
	// TODO: ���⿡ �޽��� ó���� �ڵ带 �߰� ��/�Ǵ� �⺻���� ȣ���մϴ�.

	CListBox::OnLButtonUp(nFlags, point);
}

void CSCListBox::add_to_imagelist(UINT id)
{
	CGdiplusBitmap* img = new CGdiplusBitmap(_T("PNG"), (UINT)id);
	m_imagelist.push_back(img);
}


void CSCListBox::OnWindowPosChanged(WINDOWPOS* lpwndpos)
{
	CListBox::OnWindowPosChanged(lpwndpos);

	// TODO: ���⿡ �޽��� ó���� �ڵ带 �߰��մϴ�.
	//â�� �Ϻθ� ����� ������ �����ٰ� ���� ��� ȭ���� ���ŵ��� �ʴ� �������� �߰�.
	//���⿡ �־ ���� ������ �ִ�. �켱�� parent���� ó����.
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
	//�׸��� ��� �ʺ� ���� ���� ��ũ�ѹٸ� ���������ش�.
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
