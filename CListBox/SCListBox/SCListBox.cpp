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
#include "../../log/SCLog/SCLog.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#define IDC_EDIT_CELL	WM_USER + 9001

//PreSubclassWindow 에서 자식 윈도우를 *직접* 만들면 MFC CBT hook (wincore.cpp _AfxCbtFilterHook) 안에서
//중첩 CreateEx 가 일어나 AfxHookWindowCreate 의 ASSERT(m_pWndInit == NULL) 가 터진다 — parent 가 동적
//CreateEx 로 만들어지는 CPathCtrl::m_list_folder 같은 경우. 그래서 setup_scrollbar 호출을 PostMessage 로
//지연 — CBT hook 이 m_pWndInit 를 클리어한 뒤 메시지 큐에서 처리되도록 한다.
#define UWM_CSCLISTBOX_SETUP_SCROLLBAR	(WM_USER + 9002)

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
	ON_WM_NCPAINT()
	//ON_WM_CTLCOLOR()
	ON_WM_CTLCOLOR_REFLECT()
	ON_WM_DESTROY()
	ON_WM_NCCALCSIZE()
	ON_WM_VSCROLL()
	ON_REGISTERED_MESSAGE(Message_CSCScrollbar, &CSCListBox::on_message_CSCScrollbar)
	ON_REGISTERED_MESSAGE(Message_CSCEdit, &CSCListBox::on_message_CSCEdit)
	ON_MESSAGE(UWM_CSCLISTBOX_SETUP_SCROLLBAR, &CSCListBox::on_setup_scrollbar_deferred)
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

	//Resource Editor 에서 이 컨트롤을 사용하는 dlg 에 적용된 폰트를 기본으로 사용해야 한다.
	//단, 동적으로 생성된 클래스에서 이 클래스를 사용하거나
	//아직 MainWnd 가 생성되지 않은 상태에서도 이 코드를 만날 수 있으므로 parent 가 NULL 일 수 있다.
	//이전 패턴 (AfxGetMainWnd()->GetFont() + SYSTEM_FONT 폴백) 은 ① MainWnd NULL dereference 위험 ②
	//SYSTEM_FONT 가 비트맵 fixed font 라 stroke 가 두꺼워 bold 처럼 보이는 부작용 → 4단계 폴백으로 교체.
	CWnd*  parent = GetParent();
	CFont* font   = GetFont();
	if (font == NULL && parent != nullptr)
		font = parent->GetFont();

	if (font != NULL)
	{
		font->GetObject(sizeof(m_lf), &m_lf);
	}
	else
	{
		NONCLIENTMETRICS ncm = {};
		ncm.cbSize = sizeof(ncm);
		BOOL ok = ::SystemParametersInfo(SPI_GETNONCLIENTMETRICS, ncm.cbSize, &ncm, 0);
#if (WINVER >= 0x0600)
		if (!ok)
		{
			ncm.cbSize = sizeof(ncm) - sizeof(ncm.iPaddedBorderWidth);
			ok = ::SystemParametersInfo(SPI_GETNONCLIENTMETRICS, ncm.cbSize, &ncm, 0);
		}
#endif
		if (ok)
			m_lf = ncm.lfMessageFont;
		else
			GetObject(GetStockObject(DEFAULT_GUI_FONT), sizeof(m_lf), &m_lf);
	}

	ReconstructFont();

	//setup_scrollbar() 를 직접 호출하면 parent 가 동적 CreateEx 로 만들어진 경우 (예: CPathCtrl::m_list_folder)
	//CBT hook 안의 PreSubclassWindow 호출 시점이라 중첩 CreateEx 가 AfxHookWindowCreate 의
	//ASSERT(m_pWndInit == NULL) 를 터뜨린다. PostMessage 로 큐에 넣어 hook 종료 후 실행.
	PostMessage(UWM_CSCLISTBOX_SETUP_SCROLLBAR);
}

LRESULT CSCListBox::on_setup_scrollbar_deferred(WPARAM, LPARAM)
{
	setup_scrollbar();
	return 0;
}

void CSCListBox::ReconstructFont()
{
	m_font.DeleteObject();
	m_lf.lfCharSet = DEFAULT_CHARSET;
	m_lf.lfQuality = ANTIALIASED_QUALITY;
	BOOL bCreated = m_font.CreateFontIndirect(&m_lf);

	SetFont(&m_font, true);

	m_line_height = -m_lf.lfHeight + 10;
	SetItemHeight(0, -m_lf.lfHeight + 10);

	ASSERT(bCreated);
}

void CSCListBox::set_font(LOGFONT& lf)
{
	memcpy(&m_lf, &lf, sizeof(LOGFONT));
	ReconstructFont();
}

void CSCListBox::set_font_name(CString sFontname, BYTE byCharSet)
{
	m_lf.lfCharSet = byCharSet;
	_tcscpy_s(m_lf.lfFaceName, _countof(m_lf.lfFaceName), sFontname);
	ReconstructFont();
}

void CSCListBox::set_font_size(int nSize)
{
	HDC hDC = GetDC()->GetSafeHdc();
	m_lf.lfHeight = -MulDiv(nSize, GetDeviceCaps(hDC, LOGPIXELSY), 72);
	::ReleaseDC(m_hWnd, hDC);
	ReconstructFont();
}

void CSCListBox::set_font_bold(int weight)
{
	m_lf.lfWeight = weight;
	ReconstructFont();
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

	//itemData 에 CSCListBoxItem* 저장됨. alpha=0 = "사용자 미지정 → theme 으로 fallback".
	CSCListBoxItem* item = (CSCListBoxItem*)lpDIS->itemData;
	cr_text = (item && item->cr_text.GetA() != 0) ? item->cr_text : m_theme.cr_text;
	Gdiplus::Color cr_item_back = (item && item->cr_back.GetA() != 0) ? item->cr_back : m_theme.cr_back;

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
			if (lpDIS->itemState & ODS_FOCUS)
				cr_back = m_theme.cr_back_selected;
			else if (m_show_selection_always)
				cr_back = m_theme.cr_back_selected_inactive;
			else
				cr_back = cr_item_back;	//normal-but-selected-not-focused — per-item cr_back 또는 theme.
		}
		else
		{
			cr_back = cr_item_back;	//normal — per-item cr_back override 가능.
		}

	}

	//CBrush brush(cr_back);
	//dc.FillRect(&rect, &brush);
	//draw_rect(pDC, rect, cr_back, cr_back, 1);

	if (lpDIS->itemState & ODS_SELECTED)
	{
		//선택 항목의 색은 자신의 색으로 그냥 그려준다.
		//cr_text = m_cr_text_selected;
		TRACE(_T("ODS_SELECTED\n"));
		if (lpDIS->itemState & ODS_FOCUS)
			draw_rect(pDC, rect, m_theme.cr_selected_border, cr_back, 1);
		else if (m_show_selection_always)
			draw_rect(pDC, rect, m_theme.cr_selected_border_inactive, cr_back, 1);
	}
	else
	{
		draw_rect(pDC, rect, cr_back, cr_back, 1);
	}

	if (!m_as_static && lpDIS->itemState & ODS_DISABLED)
	{
		cr_text = RGB2gpColor(::GetSysColor(COLOR_GRAYTEXT));
	}
	else
	{
		//state 우선순위 — hover > selected (hover 가 더 즉각적 인터랙션). selected text 변경 누락 시
		//selected bg (dark) 위에 본문 dark text 가 그대로 그려져 navy on navy 가독성 ↓.
		if (lpDIS->itemID == m_over_item)
		{
			cr_text = m_theme.cr_text_hover;
		}
		else if (lpDIS->itemState & ODS_SELECTED)
		{
			if (lpDIS->itemState & ODS_FOCUS)
				cr_text = m_theme.cr_text_selected;
			else if (m_show_selection_always)
				cr_text = m_theme.cr_text_selected_inactive;
		}
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

	return insert(-1, new_text);
}

int CSCListBox::add(std::deque<CString> *lists, Gdiplus::Color cr)
{
	if (cr.GetA() == 0)
		cr = m_theme.cr_text;

	for (int i = 0; i < lists->size(); i++)
		insert(-1, lists->at(i), cr);

	return lists->size();
}

//-------------------------------------------------------------------
//
int CSCListBox::insert(int index, CString text, Gdiplus::Color cr_text)
//
// Return Value:	The zero-based index of the position at which the
//						string was inserted. The return value is LB_ERR if
//						an error occurs; the return value is LB_ERRSPACE if
//						insufficient space is available to store the new string.
//
// Parameters	:	index - Specifies the zero-based index of the position
//							to insert the string. -1 = append to end.
//						text - Points to the null-terminated string that
//							is to be inserted.
//						cr_text - Specifies the color to be associated with the item.
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

	int inserted = ((CListBox*)this)->InsertString(index, text);
	if (inserted >= 0)
	{
		//itemData 에 CSCListBoxItem* 저장. text 도 같이 보관해 외부에서 직접 접근 가능 (color 외 메타).
		//cr_back = Transparent → DrawItem 이 m_theme.cr_back 사용 의미.
		CSCListBoxItem* item = new CSCListBoxItem(text, cr_text, Gdiplus::Color::Transparent);
		SetItemData(inserted, (DWORD_PTR)item);
		RedrawWindow();

		if (m_auto_scroll)
			SetTopIndex(inserted);

		sync_scrollbar();
	}

	return inserted;
}	// CSCListBox::insert

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
	CSCListBoxItem* item = (CSCListBoxItem*)GetItemData(nIndex);
	if (item)
		item->cr_text = rgb;
	if (invalidate)
		RedrawWindow();
}

Gdiplus::Color CSCListBox::get_item_color(int nIndex)
{
	CSCListBoxItem* item = (CSCListBoxItem*)GetItemData(nIndex);
	return item ? item->cr_text : m_theme.cr_text;
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
	insert(index, text, cr);
	SetRedraw(TRUE);
	UpdateWindow();
}

//CSCListBoxItem::alt_text 에 추가 메타 문자열 저장 (full path 등). 표시 텍스트는 그대로 유지.
void CSCListBox::set_alt_text(int index, CString _alt_text)
{
	if (index < 0 || index >= GetCount())
	{
		TRACE(_T("out of range of listbox index = %d.\n"), index);
		return;
	}

	CSCListBoxItem* item = (CSCListBoxItem*)GetItemData(index);
	if (item)
		item->alt_text = _alt_text;
}

CString CSCListBox::get_alt_text(int index)
{
	if (index < 0 || index >= GetCount())
	{
		TRACE(_T("out of range of listbox index = %d.\n"), index);
		return _T("");
	}

	CSCListBoxItem* item = (CSCListBoxItem*)GetItemData(index);
	return item ? item->alt_text : _T("");
}

//start 부터 case-insensitive 로 find_str 검색. Functions.h::find 위임 (case_sensitive=false 고정).
//find_main_text = true: 표시 텍스트 (listbox 의 GetText). false: CSCListBoxItem::alt_text.
//whole_word: Functions.h::find 의 same param — needle 양 끝이 punctuation/string 경계 여야 매치 (word boundary).
int CSCListBox::find(int start, CString find_str, bool find_main_text, bool whole_word)
{
	if (find_str.IsEmpty())
		return -1;
	if (start < 0)
		start = 0;

	int count = GetCount();
	for (int i = start; i < count; i++)
	{
		CString target;
		if (find_main_text)
		{
			target = get_text(i);
		}
		else
		{
			CSCListBoxItem* item = (CSCListBoxItem*)GetItemData(i);
			if (!item)
				continue;
			target = item->alt_text;
		}

		if (::find(target, find_str, 0, false, whole_word) >= 0)
			return i;
	}
	return -1;
}

void CSCListBox::OnMouseMove(UINT nFlags, CPoint point)
{
	// TODO: Add your message handler code here and/or call default
	if (m_use_over)
	{
		BOOL outside = false;
		UINT nHover = ItemFromPoint(point, outside);
		int new_over = outside ? -1 : (int)nHover;

		if (new_over != m_over_item)
		{
			//이전: Invalidate(false) — 전체 client 무효화 → 모든 visible 항목 DrawItem 재호출로 flicker.
			//변경: 이전 hover 항목과 새 hover 항목 두 rect 만 invalidate. erase 단계 생략 (bErase=FALSE)
			//하여 WM_ERASEBKGND 호출 없이 WM_PAINT → DrawItem 의 MemoryDC 더블버퍼링으로 flicker 제거.
			int old_over = m_over_item;
			m_over_item = new_over;

			int count = GetCount();
			CRect rc;
			if (old_over >= 0 && old_over < count)
			{
				GetItemRect(old_over, &rc);
				InvalidateRect(&rc, FALSE);
			}
			if (new_over >= 0 && new_over < count)
			{
				GetItemRect(new_over, &rc);
				InvalidateRect(&rc, FALSE);
			}
		}
	}

	CListBox::OnMouseMove(nFlags, point);
}

//OnPaint override 삭제 (구현 broken — base 가 BeginPaint/EndPaint 로 region validate 후 두 번째 CPaintDC 의 rcPaint
//가 비어있어 border 가 실제로 그려지지 않았음. WS_BORDER 가 default 1px native border 처리).

BOOL CSCListBox::OnEraseBkgnd(CDC* pDC)
{
	//전체 client 를 fill 하면 DrawItem 이 그 위에 다시 그려 erase ↔ paint 사이 bg flash → 스크롤 시 깜빡임.
	//항목 영역은 DrawItem 이 cr_back 포함해 그리므로 여기서 erase 안 해도 됨. 마지막 항목 *아래* 의 빈 영역만 fill.
	CRect rc;
	GetClientRect(rc);

	int count = GetCount();
	int top = GetTopIndex();
	int items_bottom = 0;
	if (count > 0 && m_line_height > 0)
	{
		items_bottom = (count - top) * m_line_height;
		if (items_bottom > rc.Height())
			items_bottom = rc.Height();
	}

	if (items_bottom < rc.Height())
	{
		CRect r_empty(rc.left, items_bottom, rc.right, rc.Height());
		pDC->FillSolidRect(r_empty, m_theme.cr_back.ToCOLORREF());
	}

	return TRUE;
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
			if (m_use_edit && m_in_editing)
			{
				return false;
			}
			delete_items();
			return true;
		}
	}
	else if (pMsg->message == WM_CONTEXTMENU)
	{
		//return FALSE를 해줘야만 parent에서 이 메시지를 처리할 수 있다.
		if (!m_use_popup_menu)
			return FALSE;
	}
	else if (pMsg->message == WM_KILLFOCUS)
	{
		TRACE(_T("%d, %d\n"), pMsg->wParam, pMsg->lParam);
	}
	//else if (pMsg->message == WM_LBUTTONDBLCLK)
	//{
	//	if (m_use_edit && !m_in_editing)
	//	{
	//		edit();
	//		return true;
	//	}
	//}

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
	TRACE(_T("CSCListBox::OnKillFocus"));
	if (m_as_popup)
		ShowWindow(SW_HIDE);

	if (m_use_edit && m_in_editing)
		edit_end();

	Invalidate();
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

	//popup chrome — Win11 탐색기 주소표시줄 폴더 팝업과 동일하게 둥근 모서리 + DWM shadow.
	//WS_BORDER 는 시스템이 NC 안쪽 1px 을 default 색 (light gray) 으로 그려 dark theme 에서 흰 frame 잔상의 원인 →
	//제거하고 DWM border (DWMWA_BORDER_COLOR) 로 대체. SWP_FRAMECHANGED 로 즉시 frame 재계산.
	//WS_POPUP|WS_BORDER 만으로는 Win11 자동 라운딩 미적용이라 DwmSetWindowAttribute 로 명시 opt-in.
	//shadow 는 borderless-popup 이라 DwmExtendFrameIntoClientArea margins{0,0,0,1} 트릭.
	//DWM border 색은 default light gray 라 popup 둘레 흰 halo 회피용으로 cr_back 으로 지정.
	//XP/Win10 에서는 dwm 헬퍼가 no-op 이라 안전.
	//WS_BORDER / WS_EX_WINDOWEDGE 는 CreateEx 시점부터 제외 (PathCtrl 의 CreateEx 에서 통일 처리).
	//DwmExtendFrameIntoClientArea 는 borderless 윈도우에 DWM shadow 를 끌어들이는 용도지만 1px 영역이
	//DWM glass 로 합성되어 dark theme 에서 흰 띠로 보일 수 있어 일단 제외. shadow 가 약해지면 그때 재고려.
	if (::IsWindow(m_hWnd))
	{
		win_compat::dwm::set_window_corner_round(m_hWnd);
		win_compat::dwm::set_border_color(m_hWnd, m_theme.cr_back.ToCOLORREF());
	}
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
		insert(-1, get_part(m_folder_list[i], fn_name));
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

	//화살표 키 등 keyboard navigation 으로 listbox internal auto-scroll 발생 시 m_scrollbar.m_pos 가 stale.
	//여기서 sync 하면 새로운 top 이 scrollbar 에 반영. 추가로 RedrawWindow 로 ScrollWindow 후 잔류 픽셀 artifact 제거 — RDW_NOERASE 로 erase 단계 skip해 깜빡임 회피, RDW_UPDATENOW 로 동기 paint.
	sync_scrollbar();
	if (::IsWindow(m_scrollbar.m_hWnd))
		m_scrollbar.RedrawWindow(NULL, NULL, RDW_INVALIDATE | RDW_NOERASE | RDW_UPDATENOW);

	if (m_as_popup)
	{
		CString text;
		
		if (m_as_folder_list)
			text = m_folder_list[index];
		else
			GetText(index, text);

		if (m_hParentWnd == NULL)
			m_hParentWnd = GetParent()->GetSafeHwnd();

		if (m_as_popup)
			ShowWindow(SW_HIDE);

		CSCListBoxMessage msg(this, message_selchanged);
		::SendMessage(m_hParentWnd, Message_CSCListBox, (WPARAM)&msg, (LPARAM)&text);
	}

	return FALSE;
}

//선택된 항목 리스트 또는 선택된 개수를 리턴
int CSCListBox::get_selected_items(std::deque<int>* selected)
{
	if (selected)
		selected->clear();

	//single selection인 경우와 multiple selection인 경우를 구분해서 처리해야 한다.
	if ((GetStyle() & (LBS_MULTIPLESEL | LBS_EXTENDEDSEL)) != 0)
	{
		int selected_count = GetSelCount();

		if (selected_count > 0)
		{
			CArray<int, int> aryListBoxSel;
			aryListBoxSel.SetSize(selected_count);
			GetSelItems(selected_count, aryListBoxSel.GetData());

			if (selected)
			{
				for (int i = 0; i < aryListBoxSel.GetCount(); i++)
					selected->push_back(aryListBoxSel[i]);
			}
		}
	
		return selected_count;
	}
	else
	{
		int index = GetCurSel();
		if (index < 0 || index >= GetCount())
			return 0;
		
		if (selected)
			selected->push_back(index);
		return 1;
	}
	
	return 0;
}

int CSCListBox::get_item_from_pos(int x, int y)
{
	BOOL bOutside;
	CPoint pt(x, y);
	ScreenToClient(&pt);

	int rclicked_index = ItemFromPoint(pt, bOutside);
	TRACE(_T("rclicked_index = %d, bOutside = %d\n"), rclicked_index, bOutside);

	if (bOutside)
		return -1;

	return rclicked_index;
}

void CSCListBox::OnContextMenu(CWnd* pWnd, CPoint point)
{
	edit_end();

	if (!m_use_popup_menu)
	{
		::PostMessage(GetParent()->GetSafeHwnd(), WM_CONTEXTMENU, (WPARAM)m_hWnd, MAKELPARAM(point.x, point.y));
		return;
	}

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
	//mouse wheel 이벤트가 발생하면 자동 스크롤을 멈춘다.
	m_auto_scroll = false;

	//WS_VSCROLL 제거된 상태라 base CListBox 의 default wheel 처리는 동작 안 함. 수동으로 SetTopIndex.
	int notches = zDelta / WHEEL_DELTA;	//양수 = 위로 휠.
	UINT scroll_lines = 3;
	::SystemParametersInfo(SPI_GETWHEELSCROLLLINES, 0, &scroll_lines, 0);
	if ((int)scroll_lines == WHEEL_PAGESCROLL)	scroll_lines = 1;	//page scroll 모드는 1라인 fallback.

	int new_top = GetTopIndex() - notches * (int)scroll_lines;
	int max_top = GetCount() - 1;
	if (max_top < 0) max_top = 0;
	if (new_top < 0) new_top = 0;
	if (new_top > max_top) new_top = max_top;

	SetTopIndex(new_top);	//override 가 sync_scrollbar 도 호출.
	return TRUE;
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

	//과거 가로 resize 시 텍스트 미표시 → DrawItem 의 DT_NOCLIP 으로 해결. 여기서 Invalidate + RedrawWindow + UpdateWindow
	//3중 호출은 그 응급처치 잔재. 정상 paint cycle 로 충분. resize 시 native 가 invalidate 처리.
	sync_scrollbar();
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

	CFont* font = GetFont();
	CClientDC dc(this);
	dc.SelectObject(font);
	CSize sz = dc.GetTextExtent(text);
	//rItem.top += 1;// ((rItem.Height() - sz.cy) / 2 + 1);
	//rItem.DeflateRect(3, 1);

	if (!m_pEdit)
	{
		DWORD dwStyle = WS_BORDER | WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL | ES_MULTILINE | (m_edit_readonly ? ES_READONLY : 0);
		m_pEdit = new CSCEdit();
		m_pEdit->create(dwStyle, rItem, this, IDC_EDIT_CELL);
		m_pEdit->SetFont(&m_font);
		DWORD margin = m_pEdit->GetMargins();
		m_pEdit->SetMargins(4, 4);
		m_pEdit->set_line_align(DT_VCENTER);
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
		CString text;
		m_pEdit->GetWindowText(text);
		TRACE(_T("index = %d\n"), m_edit_index);

		CString old_text = get_text(m_edit_index);
		if (!text.IsEmpty() && (text == old_text))
		{
			m_in_editing = false;
			return;
		}

		if (text.IsEmpty() == false || m_accept_empty_edit_str)
		{
			SetRedraw(FALSE);
			//기존 itemData 의 색을 보존하기 위해 delete 전 미리 추출. DeleteString 이 CSCListBoxItem* 를 free 한다.
			CSCListBoxItem* old_item = (CSCListBoxItem*)GetItemData(m_edit_index);
			Gdiplus::Color cr_save = old_item ? old_item->cr_text : m_theme.cr_text;
			DeleteString(m_edit_index);
			insert(m_edit_index, text, cr_save);
			SetRedraw(TRUE);
			SetCurSel(m_edit_index);
		}
		else if (text.IsEmpty() && !m_accept_empty_edit_str)
		{
			DeleteString(m_edit_index);
			m_edit_index = -1;
		}

		if (m_hParentWnd == NULL)
			m_hParentWnd = GetParent()->GetSafeHwnd();

		CSCListBoxMessage msg(this, message_edit_end);
		::SendMessage(m_hParentWnd, Message_CSCListBox, (WPARAM)&msg, (LPARAM)m_edit_index);
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

void CSCListBox::delete_items(bool for_selected)
{
	if (!for_selected)
	{
		ResetContent();
	}
	else
	{
		std::deque<int> sel_list;
		get_selected_items(&sel_list);

		for (int i = sel_list.size() - 1; i >= 0; i--)
			DeleteString(sel_list[i]);
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

//항목 이동. single selection이고 auto sort가 아닐 경우에만 정상 동작함.
void CSCListBox::move_item(int from_index, int to_index)
{
	if (from_index < 0 || from_index >= GetCount())
		return;
	if (to_index < 0 || to_index >= GetCount())
		return;
	if (from_index == to_index)
		return;

	CString text;
	Gdiplus::Color cr;
	GetText(from_index, text);
	cr = get_item_color(from_index);
	SetRedraw(FALSE);
	DeleteString(from_index);
	insert(to_index, text, cr);
	SetRedraw(TRUE);
	SetCurSel(to_index);
	UpdateWindow();
}

void CSCListBox::move_item_up(int index)
{
	if (index <= 0)
		return;
	move_item(index, index - 1);
}

void CSCListBox::move_item_down(int index)
{
	if (index < 0 || index >= GetCount() - 1)
		return;
	move_item(index, index + 1);
}

bool CSCListBox::is_available_move_item_up(int index)
{
	return (index > 0);
}

bool CSCListBox::is_available_move_item_down(int index)
{
	return (index >= 0 && index < GetCount() - 1);
}

void CSCListBox::set_color_theme(int theme, bool invalidate)
{
	m_theme.set_color_theme(theme);

	//cr_back brush 재생성 — CtlColor (reflected) 가 owner-draw listbox 의 빈 영역 색으로 사용.
	if (m_br_back.GetSafeHandle())
		m_br_back.DeleteObject();
	m_br_back.CreateSolidBrush(m_theme.cr_back.ToCOLORREF());

	if (::IsWindow(m_scrollbar.m_hWnd))
		m_scrollbar.set_color_theme(m_theme, false);

	//popup 모드는 DWM border 색도 theme 에 맞춤. NC 영역까지 갱신되도록 RDW_FRAME 동반.
	if (m_as_popup && ::IsWindow(m_hWnd))
		win_compat::dwm::set_border_color(m_hWnd, m_theme.cr_back.ToCOLORREF());

	if (invalidate && m_hWnd)
	{
		Invalidate();
		if (m_as_popup)
			RedrawWindow(NULL, NULL, RDW_INVALIDATE | RDW_FRAME | RDW_NOERASE);
	}
}

void CSCListBox::set_color_theme(const CSCColorTheme& theme, bool invalidate)
{
	m_theme.copy_colors_from(theme);

	if (m_as_popup && ::IsWindow(m_hWnd))
		win_compat::dwm::set_border_color(m_hWnd, m_theme.cr_back.ToCOLORREF());

	if (m_br_back.GetSafeHandle())
		m_br_back.DeleteObject();
	m_br_back.CreateSolidBrush(m_theme.cr_back.ToCOLORREF());

	//scrollbar 도 invalidate 전파 — 이전엔 false 하드코딩이라 scrollbar 색만 바뀌고 redraw 안 됐다.
	if (::IsWindow(m_scrollbar.m_hWnd))
		m_scrollbar.set_color_theme(m_theme, invalidate);

	//Invalidate(TRUE) — erase background 강제. owner-draw listbox 의 빈 영역 (항목 없는 부분) 이
	//이전 theme 의 brush 로 남아있는 잔상 차단. CtlColor 가 m_br_back 반환하지 않는 경로 보완.
	if (invalidate && m_hWnd)
	{
		Invalidate(TRUE);
		if (m_as_popup)
			RedrawWindow(NULL, NULL, RDW_INVALIDATE | RDW_FRAME | RDW_NOERASE);
	}
}


void CSCListBox::OnNcPaint()
{
	//popup mode 는 NC padding band 를 cr_back 으로 fill — OnNcCalcSize 가 reserve 한 영역에
	//stale pixels 가 보이지 않도록.
	if (m_as_popup && m_popup_padding > 0)
	{
		CWindowDC dc(this);
		CRect rWindow, rClient;
		GetWindowRect(&rWindow);
		GetClientRect(&rClient);
		ClientToScreen(&rClient);

		//CWindowDC origin = window 좌상단. screen → window 좌표 변환.
		int width  = rWindow.Width();
		int height = rWindow.Height();
		int cLeft   = rClient.left   - rWindow.left;
		int cTop    = rClient.top    - rWindow.top;
		int cRight  = rClient.right  - rWindow.left;
		int cBottom = rClient.bottom - rWindow.top;

		COLORREF cr_bg = m_theme.cr_back.ToCOLORREF();

		if (cTop > 0)
			dc.FillSolidRect(0, 0, width, cTop, cr_bg);
		if (cBottom < height)
			dc.FillSolidRect(0, cBottom, width, height - cBottom, cr_bg);
		if (cLeft > 0)
			dc.FillSolidRect(0, cTop, cLeft, cBottom - cTop, cr_bg);
		if (cRight < width)
			dc.FillSolidRect(cRight, cTop, width - cRight, cBottom - cTop, cr_bg);
		return;
	}

	//일반 listbox — resource editor 의 Border 속성을 받았다면 m_draw_border=true (default).
	//기존 코드는 여기서 그냥 return 해 themed border 가 전혀 그려지지 않았음 — OnNcCalcSize 가
	//m_scrollbar_setup 시 NC 영역을 0 으로 만들어 native WS_BORDER 도 함께 무효화되었기 때문에
	//사용자 입장에선 "Border 속성을 줬는데 안 그려진다" 가 되었음.
	if (!m_draw_border)
		return;

	CWindowDC dc(this);
	CRect rc;
	GetWindowRect(&rc);
	rc.OffsetRect(-rc.TopLeft());
	draw_rect(&dc, rc, m_theme.cr_border_inactive, Gdiplus::Color::Transparent);
}


HBRUSH CSCListBox::OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor)
{
	//owner-draw listbox 의 빈 영역 (항목 없는 부분) 은 OS 가 CtlColor 의 brush 로 fill 한다.
	//base 반환을 그대로 두면 system COLOR_WINDOW brush 사용 → theme 변경 시 listbox 배경이 흰색 그대로 잔존.
	//theme 의 cr_back 으로 갱신된 m_br_back 을 반환해 빈 영역도 theme 색을 따르게 한다.
	pDC->SetTextColor(m_theme.cr_text.ToCOLORREF());
	pDC->SetBkColor(m_theme.cr_back.ToCOLORREF());
	if (m_br_back.GetSafeHandle())
		return (HBRUSH)m_br_back.GetSafeHandle();
	return CListBox::OnCtlColor(pDC, pWnd, nCtlColor);
}

//reflected: 부모가 WM_CTLCOLORLISTBOX 처리 안 하면 MFC 가 child 의 CtlColor 호출.
//owner-draw listbox 의 빈 영역도 이 brush 로 칠해져 cr_back 일관 적용.
HBRUSH CSCListBox::CtlColor(CDC* pDC, UINT nCtlColor)
{
	pDC->SetTextColor(m_theme.cr_text.ToCOLORREF());
	pDC->SetBkColor(m_theme.cr_back.ToCOLORREF());
	if (m_br_back.GetSafeHandle())
		return (HBRUSH)m_br_back.GetSafeHandle();
	return NULL;
}

//base 의 ResetContent / DeleteString 은 itemData (= CSCListBoxItem*) 를 그냥 버리므로 leak. override 해서 먼저 해제.
void CSCListBox::ResetContent()
{
	int count = CListBox::GetCount();
	for (int i = 0; i < count; i++)
	{
		CSCListBoxItem* item = (CSCListBoxItem*)CListBox::GetItemData(i);
		delete item;
	}
	CListBox::ResetContent();
	sync_scrollbar();
}

int CSCListBox::DeleteString(UINT nIndex)
{
	if ((int)nIndex < CListBox::GetCount())
	{
		CSCListBoxItem* item = (CSCListBoxItem*)CListBox::GetItemData(nIndex);
		delete item;
	}
	int r = CListBox::DeleteString(nIndex);
	sync_scrollbar();
	return r;
}

int CSCListBox::SetCurSel(int nSelect)
{
	int r = CListBox::SetCurSel(nSelect);
	//SetCurSel 이 internal 로 SetTopIndex 호출해 auto-scroll 시킬 수 있음 — top 갱신 가능성 있어 sync.
	sync_scrollbar();
	return r;
}

int CSCListBox::SetTopIndex(int nIndex)
{
	int r = CListBox::SetTopIndex(nIndex);
	sync_scrollbar();
	return r;
}

void CSCListBox::OnDestroy()
{
	//창 destroy 시점에 남아있는 항목들의 itemData 정리. ResetContent 로 처리하면 base 가 invalidate 등 호출 시도하므로 manual loop.
	int count = CListBox::GetCount();
	for (int i = 0; i < count; i++)
	{
		CSCListBoxItem* item = (CSCListBoxItem*)CListBox::GetItemData(i);
		delete item;
	}
	CListBox::OnDestroy();
}

void CSCListBox::setup_scrollbar()
{
	if (m_scrollbar_setup || !::IsWindow(m_hWnd))
		return;

	//flag 를 먼저 true 로 — 다음 SetWindowPos(SWP_FRAMECHANGED) 가 트리거하는 WM_NCCALCSIZE 가
	//OnNcCalcSize 의 m_scrollbar_setup 분기를 타도록 (false 면 base 가 native scrollbar NC 영역 reserve 함).
	m_scrollbar_setup = true;

	//WS_HSCROLL 제거 + WS_VSCROLL 유지 + WS_CLIPCHILDREN 추가. WS_VSCROLL 유지 이유 — base CListBox 내부 scroll 시
	//WM_VSCROLL 을 자체 발화 → OnVScroll catch → sync_scrollbar 가 base scroll 직후 동기 실행 (SCTreeCtrl 패턴과 동일).
	//OnNcCalcSize 가 NC 공간 0 으로 만들어 native scrollbar 시각적 비표시. ShowScrollBar 로 추가 차단.
	LONG_PTR style = ::GetWindowLongPtr(m_hWnd, GWL_STYLE);
	style &= ~(LONG_PTR)WS_HSCROLL;
	style |= WS_VSCROLL | WS_CLIPCHILDREN;
	::SetWindowLongPtr(m_hWnd, GWL_STYLE, style);

	::SetWindowPos(m_hWnd, NULL, 0, 0, 0, 0,
		SWP_FRAMECHANGED | SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);

	ShowScrollBar(SB_VERT, FALSE);
	ShowScrollBar(SB_HORZ, FALSE);

	CRect rc;
	GetClientRect(&rc);
	m_scrollbar.create(this, CSCScrollbar::vertical,
		rc.right - m_scrollbar_width, 0, m_scrollbar_width, rc.Height());
	m_scrollbar.set_color_theme(m_theme, false);
	m_scrollbar.set_line(3);		//화살표 클릭 = 3 라인.
	m_scrollbar.ShowWindow(SW_HIDE);
}

void CSCListBox::sync_scrollbar()
{
	if (!m_scrollbar_setup || !::IsWindow(m_scrollbar.m_hWnd))
		return;

	//listbox 가 항목 추가 등으로 native scrollbar 재표시 시도 — 매 sync 마다 강제 hide.
	ShowScrollBar(SB_VERT, FALSE);
	ShowScrollBar(SB_HORZ, FALSE);

	int total = GetCount();

	//visible item 수 — owner-draw fixed 이므로 line_height 로 계산.
	CRect rc;
	GetClientRect(&rc);
	int visible = (m_line_height > 0) ? (rc.Height() / m_line_height) : 0;

	//위치 sync — width 은 scrollbar 자체 hover 확장 관리, 우측 edge / height 만 갱신.
	CRect rCur;
	m_scrollbar.GetWindowRect(rCur);
	ScreenToClient(rCur);
	int cur_width = rCur.Width();
	CRect rTarget(rc.right - cur_width, 0, rc.right, rc.Height());
	if (rCur != rTarget)
	{
		CRect rOld = rCur;
		m_scrollbar.MoveWindow(rTarget);
		InvalidateRect(rOld, TRUE);
	}

	bool need = (total > visible) && (visible > 0);
	int top = GetTopIndex();
	if (top < 0) top = 0;

	if (!need)
	{
		if (m_scrollbar.IsWindowVisible())
			m_scrollbar.ShowWindow(SW_HIDE);
		return;
	}

	m_scrollbar.set_range(0, total - 1);
	m_scrollbar.set_page(visible);
	m_scrollbar.set_pos(top);

	if (!m_scrollbar.IsWindowVisible())
		m_scrollbar.ShowWindow(SW_SHOW);
}

LRESULT CSCListBox::on_message_CSCScrollbar(WPARAM wParam, LPARAM lParam)
{
	CSCScrollbarMsg* msg = (CSCScrollbarMsg*)wParam;
	if (msg == nullptr || msg->pThis != &m_scrollbar)
		return 0;

	if (msg->msg == CSCScrollbarMsg::msg_scrollbar_pos_changed)
	{
		SetTopIndex(msg->pos);
	}
	return 0;
}

void CSCListBox::OnNcCalcSize(BOOL bCalcValidRects, NCCALCSIZE_PARAMS* lpncsp)
{
	//CSCScrollbar overlay 설치된 후엔 native scrollbar 의 NC 공간 미할당 — 시각적으로 안 그려짐.
	if (m_scrollbar_setup && lpncsp)
	{
		//popup folder list 모드일 때 4면에 padding 을 NC 영역으로 reserve — items 가 border 에 붙지 않도록.
		//WS_BORDER 의 1px 와 별개. OnNcPaint 가 이 padding band 를 cr_back 으로 fill.
		if (m_as_popup && m_popup_padding > 0 && lpncsp->rgrc)
		{
			lpncsp->rgrc[0].left   += m_popup_padding;
			lpncsp->rgrc[0].top    += m_popup_padding;
			lpncsp->rgrc[0].right  -= m_popup_padding;
			lpncsp->rgrc[0].bottom -= m_popup_padding;
		}
		//일반 listbox 에서 themed border 그릴 1px 공간 확보 (4변 각 1px).
		//이게 없으면 OnNcPaint 가 그린 border 위에 DrawItem 의 항목 fill 이 1px 만큼 덮어쓰게 됨.
		else if (m_draw_border && bCalcValidRects && lpncsp->rgrc)
		{
			lpncsp->rgrc[0].left   += 1;
			lpncsp->rgrc[0].top    += 1;
			lpncsp->rgrc[0].right  -= 1;
			lpncsp->rgrc[0].bottom -= 1;
		}
		return;
	}

	CListBox::OnNcCalcSize(bCalcValidRects, lpncsp);
}

void CSCListBox::OnVScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar)
{
	CListBox::OnVScroll(nSBCode, nPos, pScrollBar);
	sync_scrollbar();
}

LRESULT CSCListBox::on_message_CSCEdit(WPARAM wParam, LPARAM lParam)
{
	auto msg = (CSCEditMessage*)wParam;
	if (msg->pThis != m_pEdit)
		return 0;

	if (msg->message == WM_KILLFOCUS)
	{
		edit_end();
	}
	else if (msg->message == WM_KEYDOWN)
	{
		switch ((int)lParam)
		{
		case VK_RETURN:
			edit_end();
			break;
		case VK_ESCAPE:
			edit_end(false);
			break;
		}
	}

	return 0;
}
