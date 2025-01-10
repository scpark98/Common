// ColorComboBox.cpp : ���� �����Դϴ�.
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



// CColorComboBox �޽��� ó�����Դϴ�.




/*
HBRUSH CColorComboBox::OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor)
{
	HBRUSH hbr = CComboBox::OnCtlColor(pDC, pWnd, nCtlColor);

	// TODO:  ���⼭ DC�� Ư���� �����մϴ�.
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


	// TODO:  �⺻���� �������� ������ �ٸ� �귯�ø� ��ȯ�մϴ�.
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
	// TODO: ���⿡ �޽��� ó���� �ڵ带 �߰��մϴ�.
	// �׸��� �޽����� ���ؼ��� CComboBox::OnNcPaint()��(��) ȣ������ ���ʽÿ�.
}
*/
/*
void CColorComboBox::OnPaint()
{
	CPaintDC dc(this); // device context for painting
	// TODO: ���⿡ �޽��� ó���� �ڵ带 �߰��մϴ�.
	// �׸��� �޽����� ���ؼ��� CComboBox::OnPaint()��(��) ȣ������ ���ʽÿ�.

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

	// TODO: ���⿡ �޽��� ó���� �ڵ带 �߰��մϴ�.
	//UpdateWindow();
}


void CSCComboBox::OnKillFocus(CWnd* pNewWnd)
{
	CComboBox::OnKillFocus(pNewWnd);

	// TODO: ���⿡ �޽��� ó���� �ڵ带 �߰��մϴ�.
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

//�� �Լ������� m_lf ������ �̿��ؼ� ��Ʈ�� ������Ѵ�.
//��, m_lf.lfHeight ���� �̿��ؼ� ��Ʈ�� ��������Ƿ�
//m_font_size ��� ������ ���� ����Ǿ��ٸ�
//������ �̿��� �̸� m_lf.lfHeight ������ ������ �� �� �Լ��� ȣ��Ǿ�� �Ѵ�.
//m_lf.lfHeight���� ���밪�� MM_TEXT����� ���� DC������ ���� �ȼ�ũ�Ⱑ �ȴ�.
//���� ��ũ�� ũ�� ���� ����Ҷ��� m_font_size�� �̿��ϴ°� �ƴ϶�
//m_lf.lfHeight���� �̿��ؾ� ��Ȯ�� ��ũ�� ũ�Ⱑ ���ȴ�.
//m_font_size�� ���� ����ڿ��� �Ϲ����� ��Ʈ ũ�� ���� ��ġ�� ���̴� �������� ��ġ�̴�.
void CSCComboBox::reconstruct_font()
{
	m_font.DeleteObject();
	BOOL bCreated = m_font.CreateFontIndirect(&m_lf);
	SetFont(&m_font, true);

	//-1�� �ָ� �Է¹ڽ��� ���̰� ����ȴ�.
	SetItemHeight(-1, -m_lf.lfHeight + 4);
	//0�� �ָ� ����Ʈ�ڽ��� ��� �������� ���̰� ����ȴ�.
	SetItemHeight(0, -m_lf.lfHeight + 4);

	ASSERT(bCreated);
}


void CSCComboBox::PreSubclassWindow()
{
	// TODO: ���⿡ Ư��ȭ�� �ڵ带 �߰� ��/�Ǵ� �⺻ Ŭ������ ȣ���մϴ�.
	//�ڱ� �ڽſ��� �ο��� ��Ʈ�� ���ٸ� null�� ���ϵȴ�.
	//dlg�� parent�� font�� ���;� �Ѵ�.
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

//src���� �����ϴ� �޺��ڽ� �������� �ε����� ����.
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
					   // TODO: ���⿡ �޽��� ó���� �ڵ带 �߰��մϴ�.
					   // �׸��� �޽����� ���ؼ��� CComboBox::OnPaint()��(��) ȣ������ ���ʽÿ�.
	CRect rc;
	GetClientRect(rc);

	//dc.FillSolidRect(rc, RGB(255, 0, 0));
	draw_rectangle(&dc, rc, gGRAY(192), m_crBack);

	//owner draw fixed, has string �������� dropdown ��ư�� ǥ�õ��� �ʴ´�.
	//�켱 �������� �׷��ش�.
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
	// TODO: ���⿡ �޽��� ó���� �ڵ带 �߰� ��/�Ǵ� �⺻���� ȣ���մϴ�.
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
	// TODO: ���⿡ ��Ʈ�� �˸� ó���� �ڵ带 �߰��մϴ�.
}


void CSCComboBox::OnCbnSelchange()
{
	// TODO: ���⿡ ��Ʈ�� �˸� ó���� �ڵ带 �߰��մϴ�.
}


void CSCComboBox::OnCbnSelendok()
{
	// TODO: ���⿡ ��Ʈ�� �˸� ó���� �ڵ带 �߰��մϴ�.
}


void CSCComboBox::OnCbnSelendcancel()
{
	// TODO: ���⿡ ��Ʈ�� �˸� ó���� �ڵ带 �߰��մϴ�.
}


HBRUSH CSCComboBox::CtlColor(CDC* /*pDC*/, UINT /*nCtlColor*/)
{
	// TODO:  ���⼭ DC�� Ư���� �����մϴ�.

	// TODO:  �θ� ó���Ⱑ ȣ����� ���� ��� Null�� �ƴ� �귯�ø� ��ȯ�մϴ�.
	return NULL;
}

//resize�� �ϸ� ������ ���µǹǷ� �ٽ� ���� ����
void CSCComboBox::repos_edit()
{
	CRect rc;

	GetClientRect(rc);
	rc.DeflateRect(1, 1);
	rc.right = rc.right - rc.Height() + 2;

	//��ư �ʺ�ŭ ����?
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

	szText = dc.GetTextExtent(_T("A"));	//height�� �ʿ���
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
	// TODO: ���⿡ Ư��ȭ�� �ڵ带 �߰� ��/�Ǵ� �⺻ Ŭ������ ȣ���մϴ�.
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

//���� �Էµ� �ؽ�Ʈ�� �о���� �׸� �������� ������ �߰���Ų��. ������Ʈ������ �����Ѵ�.
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

