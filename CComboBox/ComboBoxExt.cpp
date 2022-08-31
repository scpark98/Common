// ColorComboBox.cpp : 구현 파일입니다.
//

//#include "stdafx.h"
#include "ComboBoxExt.h"


// CColorComboBox

IMPLEMENT_DYNAMIC(CComboBoxExt, CComboBox)

CComboBoxExt::CComboBoxExt()
{
	m_crText = ::GetSysColor( COLOR_WINDOWTEXT );
	m_crBack = ::GetSysColor( COLOR_WINDOW );
	m_crHighlightText = ::GetSysColor(COLOR_HIGHLIGHT);
	m_crHighlightTextBack = ::GetSysColor(COLOR_HIGHLIGHTTEXT);

	memset(&m_lf, 0, sizeof(LOGFONT));
}

CComboBoxExt::~CComboBoxExt()
{
	m_font.DeleteObject();
}


BEGIN_MESSAGE_MAP(CComboBoxExt, CComboBox)
	//ON_WM_NCPAINT()
	//ON_WM_DRAWITEM()
	//ON_WM_PAINT()
	//ON_CONTROL_REFLECT(CBN_EDITUPDATE, OnEditUpdate)
	ON_WM_SETFOCUS()
	ON_WM_KILLFOCUS()
	ON_CONTROL_REFLECT(CBN_DROPDOWN, &CComboBoxExt::OnCbnDropdown)
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

void CComboBoxExt::DrawItem(LPDRAWITEMSTRUCT lpDrawItemStruct)
{
	ASSERT(lpDrawItemStruct->CtlType == ODT_COMBOBOX);

	if( lpDrawItemStruct->itemID == -1 ) return;

	CDC dc;
	CString    strData;

	dc.Attach(lpDrawItemStruct->hDC);
	GetLBText(lpDrawItemStruct->itemID, strData);

	COLORREF crOldTextColor = dc.GetTextColor();
	COLORREF crOldBkColor = dc.GetBkColor();

	if( ( lpDrawItemStruct->itemState & ODS_SELECTED ) )
	{
		dc.SetTextColor( m_crHighlightText );
		dc.SetBkColor( m_crHighlightTextBack );
		dc.FillSolidRect( &lpDrawItemStruct->rcItem, m_crHighlightTextBack );
	}
	else
	{
		dc.SetTextColor( m_crText );
		dc.SetBkColor( m_crBack );
		dc.FillSolidRect( &lpDrawItemStruct->rcItem, m_crBack );
	}

	dc.DrawText(strData, &lpDrawItemStruct->rcItem, DT_LEFT | DT_SINGLELINE | DT_VCENTER );

	dc.SetTextColor(crOldTextColor);
	dc.SetBkColor(crOldBkColor);

	dc.Detach();

}


void CComboBoxExt::MeasureItem(LPMEASUREITEMSTRUCT lpMeasureItemStruct )
{
	ASSERT(lpMeasureItemStruct->CtlType == ODT_COMBOBOX);
}


int CComboBoxExt::CompareItem(LPCOMPAREITEMSTRUCT lpCompareItemStruct )
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

void CComboBoxExt::OnSetFocus(CWnd* pOldWnd)
{
	CComboBox::OnSetFocus(pOldWnd);

	// TODO: 여기에 메시지 처리기 코드를 추가합니다.
	UpdateWindow();
}


void CComboBoxExt::OnKillFocus(CWnd* pNewWnd)
{
	CComboBox::OnKillFocus(pNewWnd);

	// TODO: 여기에 메시지 처리기 코드를 추가합니다.
	UpdateWindow();
}


void CComboBoxExt::OnCbnDropdown()
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

void CComboBoxExt::set_font_name(LPCTSTR sFontname, BYTE byCharSet)
{
	if (sFontname == _T(""))
		return;
	m_lf.lfCharSet = byCharSet;
	_tcscpy(m_lf.lfFaceName, sFontname);
	reconstruct_font();
}

//-1 : reduce, +1 : enlarge
void CComboBoxExt::set_font_size(int font_size)
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
void CComboBoxExt::reconstruct_font()
{
	m_font.DeleteObject();
	BOOL bCreated = m_font.CreateFontIndirect(&m_lf);
	SetFont(&m_font, true);

	//m_font_size = get_font_size();

	//set_line_height(4 - m_lf.lfHeight);
	//if (m_auto_line_height)
	//recalculate_line_height();

	ASSERT(bCreated);
}


void CComboBoxExt::PreSubclassWindow()
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

	CComboBox::PreSubclassWindow();
}
