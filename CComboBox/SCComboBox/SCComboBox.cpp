// ColorComboBox.cpp : 구현 파일입니다.
//

//#include "stdafx.h"
#include "SCComboBox.h"
#include "../../../Common/Functions.h"
#include "../../CEdit/SCEdit/SCEdit.h"

// CColorComboBox

IMPLEMENT_DYNAMIC(CSCComboBox, CComboBox)

CSCComboBox::CSCComboBox()
{
	m_crText = ::GetSysColor( COLOR_WINDOWTEXT );
	m_crBack = ::GetSysColor( COLOR_WINDOW );
	m_crHighlightText = ::GetSysColor(COLOR_HIGHLIGHTTEXT);
	m_crHighlightTextBack = ::GetSysColor(COLOR_HIGHLIGHT);

	memset(&m_lf, 0, sizeof(LOGFONT));
}

CSCComboBox::~CSCComboBox()
{
	m_font.DeleteObject();

	if (m_pEdit)
	{
		m_pEdit->DestroyWindow();
		delete m_pEdit;
	}
}


BEGIN_MESSAGE_MAP(CSCComboBox, CComboBox)
	//ON_WM_NCPAINT()
	//ON_WM_DRAWITEM()
	//ON_WM_PAINT()
	//ON_CONTROL_REFLECT(CBN_EDITUPDATE, OnEditUpdate)
	ON_WM_SETFOCUS()
	ON_WM_KILLFOCUS()
	ON_CONTROL_REFLECT(CBN_DROPDOWN, &CSCComboBox::OnCbnDropdown)
	ON_WM_PAINT()
	ON_WM_ERASEBKGND()
	ON_CONTROL_REFLECT(CBN_SETFOCUS, &CSCComboBox::OnCbnSetfocus)
	ON_CONTROL_REFLECT(CBN_KILLFOCUS, &CSCComboBox::OnCbnKillfocus)
	ON_CONTROL_REFLECT(CBN_SELCHANGE, &CSCComboBox::OnCbnSelchange)
	ON_CONTROL_REFLECT(CBN_SELENDOK, &CSCComboBox::OnCbnSelendok)
	ON_CONTROL_REFLECT(CBN_SELENDCANCEL, &CSCComboBox::OnCbnSelendcancel)
	ON_WM_CTLCOLOR_REFLECT()
	ON_REGISTERED_MESSAGE(Message_CSCEditMessage, &CSCComboBox::on_message_CSCEdit)
END_MESSAGE_MAP()



// CColorComboBox 메시지 처리기입니다.




/*
HBRUSH CColorComboBox::OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor)
{
	HBRUSH hbr = CComboBox::OnCtlColor(pDC, pWnd, nCtlColor);

	// TODO:  여기서 DC의 특성을 변경합니다.
	//CTLCOLOR_LISTBOX

	pDC->SetBkMode(TRANSPARENT);

	if ( nCtlColor == CTLCOLOR_EDIT )
	{
		pDC->SetTextColor( RGB(255, 255, 255) );
		pDC->SetDCBrushColor( RGB(255, 0, 255) );

	}
	else
	{
		pDC->SetTextColor( m_crText );
		pDC->SetDCBrushColor( m_crBack );
		pDC->SetBkMode(TRANSPARENT);
	}

	return (HBRUSH)GetStockObject(DC_BRUSH);


	// TODO:  기본값이 적당하지 않으면 다른 브러시를 반환합니다.
	return hbr;
}
*/

void CSCComboBox::DrawItem(LPDRAWITEMSTRUCT lpDrawItemStruct)
{
	ASSERT(lpDrawItemStruct->CtlType == ODT_COMBOBOX);

	if( lpDrawItemStruct->itemID == -1 ) return;

	CDC dc;
	CString    strData;

	dc.Attach(lpDrawItemStruct->hDC);
	GetLBText(lpDrawItemStruct->itemID, strData);

	COLORREF crOldTextColor = dc.GetTextColor();
	COLORREF crOldBkColor = dc.GetBkColor();

	if (lpDrawItemStruct->itemState & ODS_SELECTED)
	{
		dc.SetTextColor(m_crHighlightText);
		dc.SetBkColor(m_crHighlightTextBack);
		dc.FillSolidRect( &lpDrawItemStruct->rcItem, m_crHighlightTextBack );
	}
	else
	{
		dc.SetTextColor(m_crText);
		dc.SetBkColor(m_crBack);
		dc.FillSolidRect(&lpDrawItemStruct->rcItem, m_crBack);
	}

	//TRACE(_T("%s\n"), strData);
	dc.DrawText(strData, &lpDrawItemStruct->rcItem, DT_LEFT | DT_SINGLELINE | DT_VCENTER );

	dc.SetTextColor(crOldTextColor);
	dc.SetBkColor(crOldBkColor);

	dc.Detach();
}


void CSCComboBox::MeasureItem(LPMEASUREITEMSTRUCT lpMeasureItemStruct )
{
	ASSERT(lpMeasureItemStruct->CtlType == ODT_COMBOBOX);
}


int CSCComboBox::CompareItem(LPCOMPAREITEMSTRUCT lpCompareItemStruct )
{

	ASSERT(lpCompareItemStruct->CtlType == ODT_COMBOBOX);
	LPCTSTR lpszText1 = (LPCTSTR) lpCompareItemStruct->itemData1;
	ASSERT(lpszText1 != NULL);
	LPCTSTR lpszText2 = (LPCTSTR) lpCompareItemStruct->itemData2;
	ASSERT(lpszText2 != NULL);

	return _tcscmp( lpszText1, lpszText2 );

	return 0;
}

/*
void CColorComboBox::OnNcPaint()
{
	// TODO: 여기에 메시지 처리기 코드를 추가합니다.
	// 그리기 메시지에 대해서는 CComboBox::OnNcPaint()을(를) 호출하지 마십시오.
}
*/
/*
void CColorComboBox::OnPaint()
{
	CPaintDC dc(this); // device context for painting
	// TODO: 여기에 메시지 처리기 코드를 추가합니다.
	// 그리기 메시지에 대해서는 CComboBox::OnPaint()을(를) 호출하지 마십시오.

	Default();

	CRect	rc;

	GetClientRect( rc );
	dc.FillSolidRect( rc, m_crBack );

	COLORREF m_clrBtnHilite  = ::GetSysColor(COLOR_BTNHILIGHT);
	COLORREF m_clrBtnShadow  = ::GetSysColor(COLOR_BTNSHADOW);
	COLORREF m_clrBtnFace    = ::GetSysColor(COLOR_BTNFACE);
	COLORREF m_nOffset		= ::GetSystemMetrics(SM_CXHTHUMB);


	// Cover up dark 3D shadow.
	pDC->Draw3dRect(rcItem, m_clrBtnHilite, m_clrBtnShadow);
	rcItem.DeflateRect(1,1);

	if (!IsWindowEnabled()) {
		pDC->Draw3dRect( rcItem, m_clrBtnHilite, m_clrBtnHilite );
	}

	else {
		pDC->Draw3dRect( rcItem, m_clrBtnFace, m_clrBtnFace );
	}

	// Cover up dark 3D shadow on drop arrow.
	rcItem.DeflateRect(1,1);
	rcItem.left = rcItem.right-m_nOffset;
	pDC->Draw3dRect( rcItem, m_clrBtnFace, m_clrBtnFace );

	// Cover up normal 3D shadow on drop arrow.
	rcItem.DeflateRect(1,1);
	pDC->Draw3dRect( rcItem, m_clrBtnFace, m_clrBtnFace );

	if (!IsWindowEnabled())
	{
		return;
	}


	switch (eState)
	{
	case normal:
		rcItem.top -= 1;
		rcItem.bottom += 1;
		pDC->Draw3dRect( rcItem, m_clrBtnHilite, m_clrBtnHilite );
		rcItem.left -= 1;
		pDC->Draw3dRect( rcItem, m_clrBtnHilite, m_clrBtnHilite );
		break;

	case raised:
		rcItem.top -= 1;
		rcItem.bottom += 1;
		pDC->Draw3dRect( rcItem, m_clrBtnHilite, m_clrBtnShadow);
		break;

	case pressed:
		rcItem.top -= 1;
		rcItem.bottom += 1;
		rcItem.OffsetRect(1,1);
		pDC->Draw3dRect( rcItem, m_clrBtnShadow, m_clrBtnHilite );
		break;
	}

	ReleaseDC(pDC);
}

void CColorComboBox::OnEditUpdate() 
{
	// if we are not to auto update the text, get outta here
// 	if (!m_bAutoComp) 
// 		return;

	// Get the text in the edit box
	CString str;
	GetWindowText(str);
	int nLength = str.GetLength();

	// Currently selected range
	DWORD dwCurSel = GetEditSel();
	WORD dStart = LOWORD(dwCurSel);
	WORD dEnd   = HIWORD(dwCurSel);

	// Search for, and select in, and string in the combo box that is prefixed
	// by the text in the edit box
	if (SelectString(-1, str) == CB_ERR)
	{
		SetWindowText(str);		// No text selected, so restore what was there before
		if (dwCurSel != CB_ERR)
			SetEditSel(dStart, dEnd);	//restore cursor postion
	}

	// Set the text selection as the additional text that we have added
	if (dEnd < nLength && dwCurSel != CB_ERR)
		SetEditSel(dStart, dEnd);
	else
		SetEditSel(nLength, -1);
}
*/

void CSCComboBox::OnSetFocus(CWnd* pOldWnd)
{
	CComboBox::OnSetFocus(pOldWnd);

	// TODO: 여기에 메시지 처리기 코드를 추가합니다.
	//UpdateWindow();
}


void CSCComboBox::OnKillFocus(CWnd* pNewWnd)
{
	CComboBox::OnKillFocus(pNewWnd);

	// TODO: 여기에 메시지 처리기 코드를 추가합니다.
	//UpdateWindow();
}


void CSCComboBox::OnCbnDropdown()
{
	// Reset the dropped width
	int nNumEntries = GetCount();
	int nWidth = 0;
	CString str;

	CClientDC dc(this);
	int nSave = dc.SaveDC();
	dc.SelectObject(GetFont());

	int nScrollWidth = ::GetSystemMetrics(SM_CXVSCROLL);
	for (int i = 0; i < nNumEntries; i++)
	{
		GetLBText(i, str);
		int nLength = dc.GetTextExtent(str).cx + nScrollWidth;
		nWidth = max(nWidth, nLength);
	}

	// Add margin space to the calculations
	nWidth += dc.GetTextExtent(_T("m")).cx;

	dc.RestoreDC(nSave);
	SetDroppedWidth(nWidth);
}

void CSCComboBox::set_font_name(LPCTSTR sFontname, BYTE byCharSet)
{
	if (sFontname == _T(""))
		return;
	m_lf.lfCharSet = byCharSet;
	_tcscpy_s(m_lf.lfFaceName, _countof(m_lf.lfFaceName), sFontname);
	reconstruct_font();
}

//-1 : reduce, +1 : enlarge
void CSCComboBox::set_font_size(int font_size)
{
	if (font_size == 0)
		return;

	if (font_size == -1)
		m_font_size--;
	else if (font_size == 1)
		m_font_size++;
	else
		m_font_size = font_size;

	//For the MM_TEXT mapping mode,
	//you can use the following formula to specify 
	//a height for a font with a specified point size:
	m_lf.lfHeight = -MulDiv(m_font_size, GetDeviceCaps(::GetDC(GetParent()->GetSafeHwnd()), LOGPIXELSY), 72);
	reconstruct_font();
}

//이 함수에서는 m_lf 정보를 이용해서 폰트를 재생성한다.
//즉, m_lf.lfHeight 값을 이용해서 폰트가 만들어지므로
//m_font_size 멤버 변수의 값이 변경되었다면
//공식을 이용해 이를 m_lf.lfHeight 값으로 변경한 후 이 함수가 호출되어야 한다.
//m_lf.lfHeight값의 절대값이 MM_TEXT모드의 현재 DC에서의 실제 픽셀크기가 된다.
//따라서 스크롤 크기 등을 계산할때는 m_font_size를 이용하는게 아니라
//m_lf.lfHeight값을 이용해야 정확한 스크롤 크기가 계산된다.
//m_font_size는 단지 사용자에게 일반적인 폰트 크기 설정 수치로 쓰이는 직관적인 수치이다.
void CSCComboBox::reconstruct_font()
{
	m_font.DeleteObject();
	BOOL bCreated = m_font.CreateFontIndirect(&m_lf);
	SetFont(&m_font, true);

	//-1을 주면 입력박스의 높이가 변경된다.
	SetItemHeight(-1, -m_lf.lfHeight + 4);
	//0을 주면 리스트박스의 모든 아이템의 높이가 변경된다.
	SetItemHeight(0, -m_lf.lfHeight + 4);

	ASSERT(bCreated);
}


void CSCComboBox::PreSubclassWindow()
{
	// TODO: 여기에 특수화된 코드를 추가 및/또는 기본 클래스를 호출합니다.
	//자기 자신에게 부여된 폰트가 없다면 null이 리턴된다.
	//dlg의 parent의 font를 얻어와야 한다.
	CFont* font = GetParent()->GetFont();

	if (font != NULL)
		font->GetObject(sizeof(m_lf), &m_lf);
	else
		GetObject(GetStockObject(SYSTEM_FONT), sizeof(m_lf), &m_lf);

	reconstruct_font();

	//repos_edit();

	CComboBox::PreSubclassWindow();
}

void CSCComboBox::load_history(CWinApp* app, CString section)
{
	ResetContent();

	int count = app->GetProfileInt(section, _T("history count"), 0);
	int index = app->GetProfileInt(section, _T("current index"), 0);

	CString key;
	CString text;

	for (int i = 0; i < count; i++)
	{
		key.Format(_T("%03d"), i);
		text = app->GetProfileString(section, key, _T(""));

		if (!text.IsEmpty())
			AddString(text);
	}

	if (index < 0)
		index = 0;

	if (count > 0)
		SetCurSel(index);
}

void CSCComboBox::save_history(CWinApp* app, CString section)
{
	app->WriteProfileInt(section, _T("history count"), GetCount());
	app->WriteProfileInt(section, _T("current index"), GetCurSel());

	CString key;
	CString text;

	for (int i = 0; i < GetCount(); i++)
	{
		GetLBText(i, text);
		key.Format(_T("%03d"), i);
		app->WriteProfileString(section, key, text);
	}
}

//src내에 존재하는 콤보박스 아이템의 인덱스를 리턴.
int CSCComboBox::find_string(CString src)
{
	CString text;

	for (int i = 0; i < GetCount(); i++)
	{
		GetLBText(i, text);
		if (src.Find(text) >= 0)
			return i;
	}

	return -1;
}


void CSCComboBox::OnPaint()
{
	CComboBox::OnPaint();
	return;
	CPaintDC dc(this); // device context for painting
					   // TODO: 여기에 메시지 처리기 코드를 추가합니다.
					   // 그리기 메시지에 대해서는 CComboBox::OnPaint()을(를) 호출하지 마십시오.
	CRect rc;
	GetClientRect(rc);

	//dc.FillSolidRect(rc, RGB(255, 0, 0));
	draw_rectangle(&dc, rc, gGRAY(192), m_crBack);

	//owner draw fixed, has string 때문인지 dropdown 버튼이 표시되지 않는다.
	//우선 수동으로 그려준다.
	CRect r = rc;
	r.left = r.right - r.Height() + 2;

	//dc.FillSolidRect(r, RGB(255, 0, 0));
	CPoint cp = r.CenterPoint();
	cp.Offset(-1, 2);
	int sz = 4;
	draw_line(&dc, cp.x - sz, cp.y - sz, cp.x, cp.y, gGRAY(128), 2);
	draw_line(&dc, cp.x + sz, cp.y - sz, cp.x, cp.y, gGRAY(128), 2);
}


BOOL CSCComboBox::OnEraseBkgnd(CDC* pDC)
{
	// TODO: 여기에 메시지 처리기 코드를 추가 및/또는 기본값을 호출합니다.
	return FALSE;
	return CComboBox::OnEraseBkgnd(pDC);
}


void CSCComboBox::OnCbnSetfocus()
{
	if (false)//m_use_edit)
	{
		repos_edit();

		GetWindowText(m_old_text);

		m_pEdit->ShowWindow(SW_SHOW);
		m_pEdit->SetWindowText(m_old_text);

		m_pEdit->SetSel(0, -1);
		m_pEdit->SetFocus();
	}
}

void CSCComboBox::edit_end(bool valid)
{
	if (!m_pEdit || !m_pEdit->IsWindowVisible() || (GetFocus() != m_pEdit))
		return;

	TRACE(_T("edit_end(%d)\n"), valid);

	m_pEdit->ShowWindow(SW_HIDE);

	CString text;

	if (valid)
		m_pEdit->GetWindowText(text);
	else
		text = m_old_text;

	SetWindowText(text);
	AddString(text);
}


void CSCComboBox::OnCbnKillfocus()
{
	// TODO: 여기에 컨트롤 알림 처리기 코드를 추가합니다.
}


void CSCComboBox::OnCbnSelchange()
{
	// TODO: 여기에 컨트롤 알림 처리기 코드를 추가합니다.
}


void CSCComboBox::OnCbnSelendok()
{
	// TODO: 여기에 컨트롤 알림 처리기 코드를 추가합니다.
}


void CSCComboBox::OnCbnSelendcancel()
{
	// TODO: 여기에 컨트롤 알림 처리기 코드를 추가합니다.
}


HBRUSH CSCComboBox::CtlColor(CDC* /*pDC*/, UINT /*nCtlColor*/)
{
	// TODO:  여기서 DC의 특성을 변경합니다.

	// TODO:  부모 처리기가 호출되지 않을 경우 Null이 아닌 브러시를 반환합니다.
	return NULL;
}

//resize를 하면 여백이 리셋되므로 다시 여백 설정
void CSCComboBox::repos_edit()
{
	CRect rc;

	GetClientRect(rc);
	rc.DeflateRect(1, 1);
	rc.right = rc.right - rc.Height() + 2;

	//버튼 너비만큼 제외?
	if (m_pEdit == NULL)
	{
		DWORD dwStyle = WS_CHILD | ES_AUTOHSCROLL | ES_MULTILINE;
		m_pEdit = new CSCEdit();// (this, item, subItem, GetItemText(item, subItem));
		m_pEdit->Create(dwStyle, rc, this, 0);
		m_pEdit->SetFont(&m_font, true);
	}

	m_pEdit->MoveWindow(rc);

	CRect margin = rc;
	CSize szText;
	CClientDC dc(m_pEdit);

	szText = dc.GetTextExtent(_T("A"));	//height만 필요함
	//rc.DeflateRect(2, (rc.Height() - szText.cy) / 2);
	margin.left = 0;
	margin.top += (margin.Height() - szText.cy) / 2 - 1;
	margin.bottom -= (margin.Height() - szText.cy) / 2;

	m_pEdit->SetRect(&margin);
	TRACE(_T("rc = %s\n"), get_rect_info_string(margin, 2));
}

LRESULT CSCComboBox::on_message_CSCEdit(WPARAM wParam, LPARAM lParam)
{
	CSCEdit* pEdit = (CSCEdit*)wParam;
	int	msg = (int)lParam;

	if (!pEdit->IsWindowVisible())
		return 0;

	TRACE(_T("message(%d) from CSCEdit(%p)\n"), (int)lParam, pEdit);
	//if (msg == WM_KILLFOCUS)
	//	pEdit->ShowWindow(SW_HIDE);

	Invalidate();

	return 0;
}

BOOL CSCComboBox::PreTranslateMessage(MSG* pMsg)
{
	// TODO: 여기에 특수화된 코드를 추가 및/또는 기본 클래스를 호출합니다.
	if (pMsg->message == WM_KEYDOWN)
	{
		bool	is_exist = false;
		CString text;

		switch (pMsg->wParam)
		{
			case VK_RETURN:
				edit_end(true);
				return true;

			case VK_ESCAPE:
				if (!m_pEdit || !m_pEdit->IsWindowVisible() || (GetFocus() != m_pEdit))
					break;

				TRACE(_T("escape\n"));
				m_pEdit->ShowWindow(SW_HIDE);
				SetWindowText(m_old_text);
				return true;
		}
	}

	return CComboBox::PreTranslateMessage(pMsg);
}

//현재 입력된 텍스트를 읽어오고 항목에 존재하지 않으면 추가시킨다. 레지스트리에도 저장한다.
int CSCComboBox::add(CString text)
{
	if (text.IsEmpty())
		GetWindowText(text);

	if (text.IsEmpty())
		return -1;

	if (FindString(-1, text) < 0)
	{
		AddString(text);

		if (!m_reg_section.IsEmpty())
		{
			AfxGetApp()->WriteProfileInt(m_reg_section, _T("history count"), GetCount());
			AfxGetApp()->WriteProfileInt(m_reg_section, _T("current index"), GetCurSel());

			CString key;
			key.Format(_T("%03d"), GetCount() - 1);
			AfxGetApp()->WriteProfileString(m_reg_section, key, text);
		}
	}
}

