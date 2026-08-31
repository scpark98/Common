// ColorComboBox.cpp : 구현 파일입니다.
//

//#include "stdafx.h"
#include "SCComboBox.h"
#include "../../../Common/Functions.h"
#include "../../MemoryDC.h"
#include "../../CEdit/SCEdit/SCEdit.h"
#include <imm.h>	// ImmGetCompositionString — IME 조합 중 문자열 취득 (필터링용)
#include <commctrl.h>	// SetWindowSubclass / RemoveWindowSubclass
#pragma comment(lib, "comctl32.lib")

// Combobox 내부 edit 용 subclass proc.
// CBN_EDITCHANGE 는 IME composition 중 edit buffer 가 변하지 않아 발생하지 않는다.
// WM_IME_COMPOSITION 을 직접 받아 필터링을 트리거해야 "한" 조합 단계에서도 반응한다.
static LRESULT CALLBACK sccombo_edit_subclass(
	HWND hwnd, UINT msg, WPARAM wp, LPARAM lp,
	UINT_PTR /*id_subclass*/, DWORD_PTR ref_data)
{
	//20260823 by claude. 편집 가능한 콤보(CBS_DROPDOWN)는 휠로 항목이 바뀌지 않는다.
	//WM_MOUSEWHEEL 이 내부 edit 으로 가는데, EDIT 은 스크롤 가능한 컨트롤이라 이 메시지를 *자기가 소비*하고
	//(한 줄 edit 이라 스크롤할 것이 없어 아무 일도 안 한다) 콤보로 넘기지 않기 때문이다.
	//CBS_DROPDOWNLIST 는 edit 자체가 없어 콤보가 직접 받으므로 원래 동작한다 — 편집 콤보도 같아지도록 넘긴다.
	//드롭다운이 펼쳐진 동안은 listbox 가 직접 받아 스크롤하므로 건드리지 않는다.
	if (msg == WM_MOUSEWHEEL)
	{
		CSCComboBox* self = reinterpret_cast<CSCComboBox*>(ref_data);
		HWND combo = self ? self->GetSafeHwnd() : nullptr;
		//combo != hwnd 확인 — GetComboBoxInfo 가 edit 대신 콤보 자신을 돌려주는 경우 무한 재귀가 된다.
		if (combo && combo != hwnd && !self->GetDroppedState())
			return ::SendMessage(combo, msg, wp, lp);
	}

	//20260831 by claude. 한 줄 Edit 은 글자를 자기 클라이언트 *위쪽* 에 붙여 그린다. EM_SETRECT 는
	//multiline 전용이라 세로 정렬을 지정할 방법이 없다. 그래서 클라이언트를 글자 높이만큼으로 줄여
	//선택영역 가운데에 둔다. 창 크기는 그대로라 배경·클릭 범위·캐럿 높이가 어긋나지 않는다.
	//얼마나 줄일지는 콤보가 계산해 둔다(CSCComboBox::apply_edit_text_padding).
	if (msg == WM_NCCALCSIZE)
	{
		LRESULT r = ::DefSubclassProc(hwnd, msg, wp, lp);

		CSCComboBox* self = reinterpret_cast<CSCComboBox*>(ref_data);
		if (self && (self->get_edit_pad_top() > 0 || self->get_edit_pad_bottom() > 0))
		{
			//wp=TRUE 면 rgrc[0], FALSE 면 lp 자체가 "창 rect → 클라이언트 rect" 자리다.
			RECT* prc = wp ? &reinterpret_cast<NCCALCSIZE_PARAMS*>(lp)->rgrc[0] : reinterpret_cast<RECT*>(lp);

			//클라이언트가 없어질 만큼 줄이지는 않는다 — 캐럿·선택이 동작하지 않는다.
			if (prc->bottom - prc->top > self->get_edit_pad_top() + self->get_edit_pad_bottom())
			{
				prc->top += self->get_edit_pad_top();
				prc->bottom -= self->get_edit_pad_bottom();
			}
		}
		return r;
	}

	//떼어낸 위아래 띠는 Edit 도 콤보도 칠하지 않아 이전 픽셀이 남는다. 콤보 배경과 같은 색으로 채운다.
	if (msg == WM_NCPAINT)
	{
		LRESULT r = ::DefSubclassProc(hwnd, msg, wp, lp);

		CSCComboBox* self = reinterpret_cast<CSCComboBox*>(ref_data);
		if (self && (self->get_edit_pad_top() > 0 || self->get_edit_pad_bottom() > 0))
		{
			RECT rw;
			::GetWindowRect(hwnd, &rw);

			RECT rc;
			::GetClientRect(hwnd, &rc);

			POINT origin = { 0, 0 };
			::ClientToScreen(hwnd, &origin);

			int off_y = origin.y - rw.top;
			int w = rw.right - rw.left;
			int h = rw.bottom - rw.top;

			HDC hdc = ::GetWindowDC(hwnd);
			if (hdc)
			{
				HBRUSH br = ::CreateSolidBrush(self->get_field_back_color());

				RECT band = { 0, 0, w, off_y };
				::FillRect(hdc, &band, br);

				band.top = off_y + (rc.bottom - rc.top);
				band.bottom = h;
				::FillRect(hdc, &band, br);

				::DeleteObject(br);
				::ReleaseDC(hwnd, hdc);
			}
		}
		return r;
	}

	LRESULT r = ::DefSubclassProc(hwnd, msg, wp, lp);
	if (msg == WM_IME_COMPOSITION)
	{
		CSCComboBox* self = reinterpret_cast<CSCComboBox*>(ref_data);
		if (self && self->GetSafeHwnd())
			self->OnCbnEditchange();
	}
	return r;
}

// CColorComboBox

IMPLEMENT_DYNAMIC(CSCComboBox, CComboBox)

CSCComboBox::CSCComboBox()
{
	//m_theme.set_color_theme(CSCColorTheme::color_theme_default);
	memset(&m_lf, 0, sizeof(LOGFONT));
}

CSCComboBox::~CSCComboBox()
{
}


BEGIN_MESSAGE_MAP(CSCComboBox, CComboBox)
	//ON_WM_NCPAINT()
	//ON_WM_PAINT()
	//ON_CONTROL_REFLECT(CBN_EDITUPDATE, OnEditUpdate)
	ON_WM_SETFOCUS()
	ON_WM_KILLFOCUS()
	ON_CONTROL_REFLECT(CBN_DROPDOWN, &CSCComboBox::OnCbnDropdown)
	ON_WM_PAINT()
	ON_WM_ERASEBKGND()
	ON_CONTROL_REFLECT(CBN_SETFOCUS, &CSCComboBox::OnCbnSetfocus)
	ON_CONTROL_REFLECT(CBN_KILLFOCUS, &CSCComboBox::OnCbnKillfocus)
	ON_CONTROL_REFLECT_EX(CBN_SELCHANGE, &CSCComboBox::OnCbnSelchange)
	ON_CONTROL_REFLECT(CBN_SELENDOK, &CSCComboBox::OnCbnSelendok)
	ON_CONTROL_REFLECT(CBN_SELENDCANCEL, &CSCComboBox::OnCbnSelendcancel)
	//ON_REGISTERED_MESSAGE(Message_CSCEdit, &CSCComboBox::on_message_CSCEdit)
	ON_WM_NCPAINT()
	//CBS_DROPDOWN 의 내부 child Edit 이 부모(콤보) 에게 보내는 WM_CTLCOLOREDIT 를 콤보가 직접 처리 — theme bg/text 적용.
	//ON_WM_CTLCOLOR_REFLECT() 만으론 dialog 가 child(edit) 로 반사해버려 stock CEdit 가 처리 못 함 → 시스템색 잔존.
	ON_WM_CTLCOLOR()
	ON_WM_DESTROY()
	ON_CONTROL_REFLECT(CBN_EDITCHANGE, &CSCComboBox::OnCbnEditchange)
	ON_WM_DRAWITEM()
	ON_WM_TIMER()
	ON_WM_CTLCOLOR_REFLECT()
	ON_WM_MOUSEWHEEL()		//20260831 by claude. 휠로 항목이 바뀔 때 edit 텍스트가 통째로 선택되는 것을 지운다.
END_MESSAGE_MAP()



// CColorComboBox 메시지 처리기입니다.
HBRUSH CSCComboBox::OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor)
{
	return CtlColor(pDC, nCtlColor);
}

HBRUSH CSCComboBox::CtlColor(CDC* pDC, UINT nCtlColor)
{
	pDC->SetTextColor(m_theme.cr_text.ToCOLORREF());
	pDC->SetBkColor(m_theme.cr_back.ToCOLORREF());

	m_br_back.DeleteObject();
	m_br_back.CreateSolidBrush(m_theme.cr_back.ToCOLORREF());
	return (HBRUSH)m_br_back;
}

void CSCComboBox::DrawItem(LPDRAWITEMSTRUCT lpDrawItemStruct)
{
	ASSERT(lpDrawItemStruct->CtlType == ODT_COMBOBOX);

	if (lpDrawItemStruct->itemID == -1)
		return;

	CDC screen_dc;
	screen_dc.Attach(lpDrawItemStruct->hDC);

	//CMemoryDC 의 destructor 는 m_pDC(=&screen_dc) 로 BitBlt 한다.
	//screen_dc.Detach() 가 dc 의 destructor 보다 먼저 실행되면 NULL HDC 에 BitBlt → 크래시.
	//따라서 dc 의 lifetime 을 inner scope 로 한정해 destructor 가 Detach 보다 먼저 돌게 한다.
	{
		CRect rc_item = lpDrawItemStruct->rcItem;
		CMemoryDC dc(&screen_dc, &rc_item, true);

		CString strData;
		GetLBText(lpDrawItemStruct->itemID, strData);

		COLORREF cr_text = m_theme.cr_text.ToCOLORREF();
		COLORREF cr_back = m_theme.cr_back.ToCOLORREF();

		CRect rItem = lpDrawItemStruct->rcItem;

		if (!m_is_font_combo)
		{
			CSCComboBoxColor* cr = (CSCComboBoxColor*)GetItemData(lpDrawItemStruct->itemID);
			if (cr && (cr->cr_text.GetValue() != m_theme.cr_text.GetValue()) && (cr->cr_text.GetValue() != Gdiplus::Color::Transparent))
				cr_text = cr->cr_text.ToCOLORREF();
		}

		//selected/hover 시 cr_back 만 바꾸고 cr_text 변경을 누락하면 dark bg + dark text 가독성 ↓.
		//cr_text_selected / cr_text_hover 로 함께 변경해야 contrast 확보 (theme 측에서 white 등 설정 전제).
		if (lpDrawItemStruct->itemState & ODS_SELECTED)
		{
			cr_back = m_theme.cr_back_selected.ToCOLORREF();
			cr_text = m_theme.cr_text_selected.ToCOLORREF();
		}
		else if (lpDrawItemStruct->itemState & ODS_HOTLIGHT)
		{
			cr_back = m_theme.cr_back_hover.ToCOLORREF();
			cr_text = m_theme.cr_text_hover.ToCOLORREF();
		}

		//20260728 by claude. disabled 상태 텍스트 색 — 테마의 disabled 색(배경 쪽으로 흐려진 색). 이전 get_gray_color 는
		//명도를 보존해 검은 글자가 그대로 검게 남아(무채색화만) 회색으로 보이지 않았다. selected/hover 보다 최종 우선.
		if (!IsWindowEnabled())
			cr_text = m_theme.cr_text_disabled.ToCOLORREF();

		dc.SetBkMode(TRANSPARENT);
		dc.FillSolidRect(rItem, cr_back);
		dc.SetTextColor(cr_text);

		CRect rtext = rItem;
		rtext.DeflateRect(4, 0);

		if (m_is_font_combo)
		{
			CFont cf;
			//m_font_size 는 사용자 의도(=메모장 표시) point 단위. CreateFont 의 nHeight 는 논리 단위(픽셀) 라
			//point→pixel 변환 후 음수(=character height) 로 넘겨야 동일 크기로 표시.
			int lf_height = -MulDiv(m_font_size, dc.GetDeviceCaps(LOGPIXELSY), 72);
			if (cf.CreateFont(lf_height, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, ANTIALIASED_QUALITY, DEFAULT_PITCH, strData))
			{
				HFONT hf = (HFONT)dc.SelectObject(cf);
				dc.DrawText(strData, &rtext, DT_LEFT | DT_SINGLELINE | DT_VCENTER | DT_NOCLIP);
				dc.SelectObject(hf);
			}
			else
			{
				dc.DrawText(strData, &rtext, DT_LEFT | DT_SINGLELINE | DT_VCENTER | DT_NOCLIP);
			}
		}
		else
		{
			//Owner-draw 콤보의 LPDRAWITEMSTRUCT::hDC 는 OS 가 콤보의 폰트(SetFont 로 설정한 것)를
			//자동으로 select 해주지 않는다. 명시적으로 m_font 를 select 해야 SetFont 효과가 dropdown 항목에 반영된다.
			//(SetFont 효과는 콤보 자신의 edit 영역 에만 자동 적용. listbox 의 각 항목은 owner-draw 라
			//우리가 직접 폰트를 select 해야 함.)
			CFont* pOldFont = dc.SelectObject(&m_font);
			dc.DrawText(strData, &rtext, DT_LEFT | DT_SINGLELINE | DT_VCENTER | DT_NOCLIP);
			dc.SelectObject(pOldFont);
		}
	}

	screen_dc.Detach();
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

void CSCComboBox::set_line_height(int height_logical_unit)
{
	m_line_height = -get_pixel_size_from_font_size(m_hWnd, height_logical_unit);

	//0보다 크다면 지정된 높이로 변경하고
	if (m_line_height > 0)
	{
		//-1을 주면 입력박스의 높이가 변경된다.
		SetItemHeight(-1, m_line_height);
		//0을 주면 리스트박스의 모든 아이템의 높이가 변경된다.
		SetItemHeight(0, m_line_height);
	}
	//0이하라면 폰트 크기에 따라 자동 조정된다.
	else
	{
		reconstruct_font();
	}
}

CString CSCComboBox::get_font_name()
{
	return CString(m_lf.lfFaceName);
}

void CSCComboBox::set_font_name(LPCTSTR sFontname, BYTE byCharSet)
{
	if (m_is_font_combo)
		return;

	if (sFontname == _T(""))
		return;

	m_lf.lfCharSet = byCharSet;
	_tcscpy_s(m_lf.lfFaceName, _countof(m_lf.lfFaceName), sFontname);
	reconstruct_font();
}

//-1 : reduce, +1 : enlarge
void CSCComboBox::set_font_size(int font_size)
{
	if (m_is_font_combo)
	{
		m_font_size = font_size;
		Invalidate();
		return;
	}

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

void CSCComboBox::set_font_bold(int weight)
{
	if (m_is_font_combo)
		return;

	m_lf.lfWeight = weight;
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

	int list_height;	//dropdown listbox 각 항목 높이.
	int edit_height;	//선택영역(닫힌 콤보의 보이는 부분) 높이.

	if (m_line_height > 0)
	{
		//명시 지정 — 두 높이 동일.
		list_height = m_line_height;
		edit_height = m_line_height;
	}
	else
	{
		//auto — native CComboBox 와 동일하게 맞춘다(같은 폰트로 native 측정값 list=tmHeight, edit=tmHeight+2).
		//-m_lf.lfHeight(em 높이)만 쓰면 ascent/descent/leading 이 빠져 native 보다 좁고,
		//선택영역까지 tmHeight 로 두면 닫힌 콤보가 native 보다 2px 낮다 — edit 내부 상하 여백(각 1px) 만큼 더해야 함.
		list_height = -m_lf.lfHeight;
		edit_height = list_height;
		if (m_hWnd)
		{
			CClientDC dc(this);
			CFont* old_font = dc.SelectObject(&m_font);
			TEXTMETRIC tm = {};
			dc.GetTextMetrics(&tm);
			dc.SelectObject(old_font);
			list_height = tm.tmHeight + tm.tmExternalLeading;
			edit_height = list_height + 2;
		}
	}

	//-1 = 선택영역(닫힌 콤보) 높이, 0 = listbox 항목 높이.
	SetItemHeight(-1, edit_height);
	SetItemHeight(0, list_height);

	//20260831 by claude. 선택영역 높이가 정해졌으니 자식 Edit 의 글자 세로 위치도 다시 맞춘다.
	apply_edit_text_padding();

	ASSERT(bCreated);
}

//20260831 by claude. CBS_DROPDOWN 콤보의 글자를 선택영역 세로 중앙에 오게 한다.
//
//닫힌 칸이 진짜 자식 Edit 이라 WM_DRAWITEM 이 오지 않고, 한 줄 Edit 은 글자를 자기 클라이언트
//위쪽에 붙여 그린다. EM_SETRECT 는 multiline 전용이라 세로 정렬을 지정할 방법도 없다.
//실측(2026-08-31): 선택영역 21px 안에 16px 글자가 위에 붙어 아래로 5px 가 남았다.
//
//Edit 창 자체를 줄여 옮기지는 않는다 — 창이 선택영역보다 작아지면 클릭으로 캐럿이 잡히는 범위와
//포커스 표시가 어긋난다. 창은 그대로 두고 *클라이언트* 만 줄인다(subclass 의 WM_NCCALCSIZE).
//줄어든 위아래는 NC 영역이 되므로 콤보 배경과 같은 색으로 칠한다(WM_NCPAINT).
//창 높이를 바꾸지 않으니 몇 번을 불러도 같은 값에 머문다(누적 드리프트 없음).
void CSCComboBox::apply_edit_text_padding()
{
	COMBOBOXINFO info = { 0 };
	info.cbSize = sizeof(info);

	if (!GetComboBoxInfo(&info))
		return;

	//DROPDOWNLIST 는 hwndItem 이 콤보 자신이다 — 자식 Edit 이 없어 할 일이 없다.
	if (info.hwndItem == NULL || info.hwndItem == m_hWnd)
		return;

	CRect rc_edit;
	::GetWindowRect(info.hwndItem, &rc_edit);

	CClientDC dc(this);
	CFont* old_font = dc.SelectObject(&m_font);
	TEXTMETRIC tm = {};
	dc.GetTextMetrics(&tm);
	dc.SelectObject(old_font);

	int pad_top = 0;
	int pad_bottom = 0;

	if (tm.tmHeight > 0 && rc_edit.Height() > tm.tmHeight)
	{
		int extra = rc_edit.Height() - tm.tmHeight;
		pad_top = extra / 2;
		pad_bottom = extra - pad_top;
	}

	if (pad_top == m_edit_pad_top && pad_bottom == m_edit_pad_bottom)
		return;

	m_edit_pad_top = pad_top;
	m_edit_pad_bottom = pad_bottom;

	//WM_NCCALCSIZE 를 다시 태워야 새 값이 반영된다.
	::SetWindowPos(info.hwndItem, NULL, 0, 0, 0, 0,
		SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE | SWP_FRAMECHANGED);
}


void CSCComboBox::PreSubclassWindow()
{
	//Resource Editor 에서 이 컨트롤을 사용하는 dlg 에 적용된 폰트를 기본으로 사용해야 한다.
	//단, 동적으로 생성된 클래스에서 이 클래스를 사용하거나
	//아직 MainWnd 가 생성되지 않은 상태에서도 이 코드를 만날 수 있으므로 parent 가 NULL 일 수 있다.
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
		//Vista+ : lfMessageFont = Segoe UI 9pt. XP : Tahoma 8pt.
		//Vista+ SDK 로 빌드한 exe 를 XP 에서 실행하면 NONCLIENTMETRICS 끝의 iPaddedBorderWidth (4byte) 가
		//XP 커널이 인식하는 구조체보다 크다 → SystemParametersInfo 가 ERROR_INVALID_PARAMETER 로 실패.
		//실패 시 4byte 줄여 재시도하면 XP 에서도 lfMessageFont 를 정상 획득.
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

	reconstruct_font();

	// 원본 리소스에 지정된 콤보 window height 를 cache — 동적 dropdown height 의 상한.
	CRect rc;
	GetWindowRect(&rc);
	m_initial_height = rc.Height();

	CComboBox::PreSubclassWindow();

	// 내부 edit subcontrol 을 subclass 해서 WM_IME_COMPOSITION 직접 수신.
	COMBOBOXINFO cbi = { sizeof(COMBOBOXINFO) };
	//20260831 by claude. OnPaint 가 client 전체를 cr_back 으로 채우는데, 이 비트가 없으면 그 채우기가
	//자식 Edit(CBS_DROPDOWN 의 닫힌 칸) 위까지 덮는다. 콤보가 먼저 그리고 Edit 이 나중에 그리므로
	//그 사이 한 프레임 동안 글자가 사라진 것처럼 보인다 — 휠로 항목을 넘길 때 눈에 띄던 깜빡임이 이것이다.
	//켜두면 콤보의 그리기가 자식 영역을 아예 건드리지 않는다.
	ModifyStyle(0, WS_CLIPCHILDREN);

	if (::GetComboBoxInfo(m_hWnd, &cbi) && cbi.hwndItem)
		::SetWindowSubclass(cbi.hwndItem, sccombo_edit_subclass, 1, (DWORD_PTR)this);

	//20260831 by claude. subclass 가 붙은 뒤에 다시 계산한다 — 위 reconstruct_font 에서 이미 한 번 돌았지만
	//그때는 WM_NCCALCSIZE 를 받아줄 subclass 가 없어 반영되지 않는다.
	//값이 같으면 early return 하므로 강제로 다시 적용되게 초기화한 뒤 부른다.
	m_edit_pad_top = m_edit_pad_bottom = 0;
	apply_edit_text_padding();
}

// 필터링 결과 항목 수에 맞춰 dropdown listbox 영역 높이를 조정.
// CComboBox 는 window height 를 "edit 필드 + dropdown listbox" 로 해석하고
// edit 필드 높이는 폰트에서 자동 결정되는 고유값이라, SetWindowPos 로 height 를 바꾸면
// 그 차이는 listbox 영역에만 반영된다. 초기 height 를 상한으로 clamp 하여 원본 리소스에
// 지정된 최대 dropdown 크기를 넘지 않게 한다.
void CSCComboBox::adjust_dropdown_height()
{
	const int count = GetCount();
	if (count <= 0)
		return;

	const int item_h = GetItemHeight(0);	// listbox 한 항목 높이
	const int edit_h = GetItemHeight(-1);	// edit(selection field) 높이
	const int list_padding = 4;				// listbox 윤곽/여백

	int desired_h = edit_h + count * item_h + list_padding;
	if (m_initial_height > 0 && desired_h > m_initial_height)
		desired_h = m_initial_height;

	CRect rc;
	GetWindowRect(&rc);
	SetWindowPos(nullptr, 0, 0, rc.Width(), desired_h,
		SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);
}

void CSCComboBox::load_history(CWinApp* app, CString section)
{
	if (m_is_font_combo)
		return;

	ResetContent();

	m_reg_section = section;

	int count = app->GetProfileInt(section, _T("history count"), 0);
	int index = app->GetProfileInt(section, _T("current index"), 0);

	CString key;
	CString text;

	for (int i = 0; i < count; i++)
	{
		key.Format(_T("%03d"), i);
		text = app->GetProfileString(section, key, _T(""));

		if (!text.IsEmpty())
			CComboBox::AddString(text);
	}

	if (index < 0)
		index = 0;

	if (count > 0)
		SetCurSel(index);
}

void CSCComboBox::save_history(CWinApp* app, CString section)
{
	if (m_is_font_combo)
		return;

	if (section.IsEmpty() == false)
		m_reg_section = section;

	if (m_reg_section.IsEmpty())
	{
		TRACE(_T("[error] m_reg_section is empty.\n"));
		return;
	}

	app->WriteProfileInt(m_reg_section, _T("history count"), GetCount());
	app->WriteProfileInt(m_reg_section, _T("current index"), GetCurSel());

	CString key;
	CString text;

	for (int i = 0; i < GetCount(); i++)
	{
		GetLBText(i, text);
		key.Format(_T("%03d"), i);
		app->WriteProfileString(m_reg_section, key, text);
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


//20260831 by claude. Default() 를 부르지 않는다. 콤보의 겉모습은 전부 여기서 그린다.
//
//예전에는 Default() 를 먼저 불러 comctl32 가 visual styles 로 그리게 하고 그 위에 덧그렸다.
//그 결과가 화면 픽셀로 확인된 것(dark_gray 테마, 2026-08-31 스크린샷 실측):
//  - 프레임 안쪽에 흰색(255,255,255) 띠 — CBS_DROPDOWN 은 2px, CBS_DROPDOWNLIST 는 1px.
//    같은 CSCComboBox 인데 리소스 스타일만 다르면 테두리 두께가 달라 보이던 원인이다.
//  - 드롭다운 버튼 영역(우측 약 19px)이 통째로 흰색. 어두운 테마인데 그 부분만 밝게 남던 원인이다.
//  - 게다가 Default() 가 BeginPaint/EndPaint 로 무효 영역을 이미 검증해버려, 그 뒤에 CPaintDC 를
//    새로 열어 덧그리는 것은 클립이 비어 버려질 수 있다. 앞선 시도들이 화면에 반영되지 않은 이유다.
//
//같은 저장소의 CBitComboBox 도 Default() 없이 CPaintDC 로 전부 자기가 그린다. 그 방식에 맞춘다.
//CBS_DROPDOWN 의 자식 Edit 은 별도 창이라 자기 WM_PAINT 로 계속 그려지므로 글자는 그대로 나온다.
//CBS_DROPDOWNLIST 는 자식이 없어 comctl32 가 WM_DRAWITEM 으로 보내주던 닫힌 칸 글자를 여기서 직접 그린다.
void CSCComboBox::OnPaint()
{
	CPaintDC dc(this);

	CRect rc;
	GetClientRect(rc);

	dc.FillSolidRect(rc, m_theme.cr_back.ToCOLORREF());

	//선택영역 위치는 상수로 박지 않고 OS 가 잡아준 값을 쓴다(테마·DPI 에 따라 달라진다).
	COMBOBOXINFO info = { 0 };
	info.cbSize = sizeof(info);
	bool has_info = (GetComboBoxInfo(&info) != FALSE);

	//닫힌 칸 글자 — 자식 Edit 이 없는 CBS_DROPDOWNLIST 만 해당.
	//(hwndItem 이 콤보 자신이면 자식 Edit 이 없다는 뜻이다.)
	if (has_info && (info.hwndItem == NULL || info.hwndItem == m_hWnd))
	{
		int sel = GetCurSel();
		if (sel >= 0)
		{
			CString text;
			GetLBText(sel, text);

			COLORREF cr_text = m_theme.cr_text.ToCOLORREF();

			//항목별 지정색 — DrawItem 의 판단 기준과 동일하게 맞춘다.
			CSCComboBoxColor* cr = (CSCComboBoxColor*)GetItemData(sel);
			if (!m_is_font_combo && cr &&
				(cr->cr_text.GetValue() != m_theme.cr_text.GetValue()) &&
				(cr->cr_text.GetValue() != Gdiplus::Color::Transparent))
				cr_text = cr->cr_text.ToCOLORREF();

			if (!IsWindowEnabled())
				cr_text = m_theme.cr_text_disabled.ToCOLORREF();

			CRect r_text = info.rcItem;
			r_text.DeflateRect(4, 0);		//DrawItem 의 여백과 동일.

			CFont* old_font = dc.SelectObject(&m_font);
			dc.SetBkMode(TRANSPARENT);
			dc.SetTextColor(cr_text);
			dc.DrawText(text, &r_text, DT_LEFT | DT_SINGLELINE | DT_VCENTER | DT_WORD_ELLIPSIS | DT_NOPREFIX);
			dc.SelectObject(old_font);
		}
	}

	//owner draw fixed, has string 때문인지 dropdown 버튼이 표시되지 않는다.
	//우선 수동으로 그려준다.
	CRect r = rc;
	r.left = r.right - r.Height() + 2;

	//dc.FillSolidRect(r, RGB(255, 0, 0));
	CPoint cp = r.CenterPoint();
	cp.Offset(-1, 2);
	int sz = 4;
	draw_line(&dc, cp.x - sz, cp.y - sz, cp.x, cp.y, GRAY(128), 2);
	draw_line(&dc, cp.x + sz, cp.y - sz, cp.x, cp.y, GRAY(128), 2);

	//테두리는 맨 마지막에 — 위의 배경 채우기가 덮지 않도록.
	//색 기준은 OnNcPaint 와 동일(자식 Edit 이 포커스면 콤보 포커스로 본다 — CBS_DROPDOWN).
	bool focused = (GetFocus() == this) || (GetFocus() != nullptr && IsChild(GetFocus()));
	CBrush br_border(focused ? m_theme.cr_border_active.ToCOLORREF() : m_theme.cr_border_inactive.ToCOLORREF());
	dc.FrameRect(rc, &br_border);
}


BOOL CSCComboBox::OnEraseBkgnd(CDC* pDC)
{
	// TODO: 여기에 메시지 처리기 코드를 추가 및/또는 기본값을 호출합니다.
	return FALSE;
	return CComboBox::OnEraseBkgnd(pDC);
}

//20260831 by claude. 휠로 항목을 넘길 때 edit 의 글자가 통째로 파랗게 선택되던 것을 없앤다.
//
//포커스 문제가 아니다. 콤보는 현재 선택이 바뀔 때마다 그 항목의 글자를 edit 에 새로 넣으면서
//전체를 선택 상태로 만든다(CB_SETCURSEL 의 표준 동작). 목록에서 마우스로 고를 때는 그게 자연스럽지만,
//휠로 연속해 넘길 때는 매 칸마다 선택 블록이 번쩍여 거슬린다.
//CBS_DROPDOWNLIST 에 이 현상이 없는 것은 edit 자체가 없기 때문이다.
//
//눈에 보이는 순서가 [휠 → 현재 글자 전체선택 → 다음 항목 글자] 다. 즉 중간 상태가 실제로 화면에
//한 번 나온다는 뜻이라, 다 끝난 뒤에 선택만 지우는 것으로는 번쩍임이 남는다.
//그래서 기본 처리를 하는 동안 edit 의 그리기를 잠시 멈춰 중간 프레임이 화면에 나가지 않게 하고,
//끝난 뒤 선택을 지운 상태로 한 번만 그린다. 항목 이동 자체는 그대로 comctl32 가 한다.
//자식 edit 의 휠은 subclass 가 이 콤보로 넘기므로(sccombo_edit_subclass) 두 경로가 여기로 모인다.
BOOL CSCComboBox::OnMouseWheel(UINT nFlags, short zDelta, CPoint pt)
{
	//드롭다운이 펼쳐진 동안은 listbox 가 스크롤할 뿐 edit 은 건드리지 않는다.
	if (GetDroppedState())
	{
		Default();
		return TRUE;
	}

	COMBOBOXINFO info = { 0 };
	info.cbSize = sizeof(info);

	HWND h_edit = NULL;
	if (GetComboBoxInfo(&info) && info.hwndItem && info.hwndItem != m_hWnd)
		h_edit = info.hwndItem;

	//콤보와 자식 Edit 을 함께 멈춘다. Edit 하나만 멈춰서는 부족했다 —
	//폰트 콤보는 선택이 바뀔 때마다 OnCbnSelchange 가 reconstruct_font() 를 부르고,
	//그 안에서 SetFont / SetItemHeight / Edit 재배치(SWP_FRAMECHANGED)가 연쇄로 일어난다.
	//그 각각이 중간 상태를 화면에 흘린다. 어느 것이 흘리는지 하나씩 좇는 대신 전부 묶는다.
	SetRedraw(FALSE);
	if (h_edit)
		::SendMessage(h_edit, WM_SETREDRAW, FALSE, 0);

	Default();

	if (h_edit)
	{
		//캐럿을 글자 끝에 두고 선택은 없앤다. EM_SETSEL 에 -1 을 쓰는 방식은 문서마다 해석이 갈려
		//길이를 직접 구해 (len, len) 으로 명시한다.
		int len = (int)::SendMessage(h_edit, WM_GETTEXTLENGTH, 0, 0);
		::SendMessage(h_edit, EM_SETSEL, (WPARAM)len, (LPARAM)len);

		::SendMessage(h_edit, WM_SETREDRAW, TRUE, 0);
	}
	SetRedraw(TRUE);

	//SetRedraw(FALSE) 동안 쌓인 무효화는 버려지므로 직접 무효화하고 그 자리에서 그린다.
	//RDW_ERASE 는 주지 않는다 — 배경 지우기가 한 프레임 먼저 나가면 글자가 사라졌다 나타난다.
	//콤보도 Edit 도 자기 WM_PAINT 안에서 배경까지 칠하므로 별도 erase 가 필요 없다.
	RedrawWindow(NULL, NULL, RDW_INVALIDATE | RDW_ALLCHILDREN | RDW_UPDATENOW);

	return TRUE;
}


void CSCComboBox::OnCbnSetfocus()
{
	/*
	if (false)//m_use_edit)
	{
		//repos_edit();

		GetWindowText(m_old_text);

		m_pEdit->ShowWindow(SW_SHOW);
		m_pEdit->SetWindowText(m_old_text);

		m_pEdit->SetSel(0, -1);
		m_pEdit->SetFocus();
	}
	*/
}
/*
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
	CComboBox::AddString(text);
}
*/

void CSCComboBox::OnCbnKillfocus()
{
	// TODO: 여기에 컨트롤 알림 처리기 코드를 추가합니다.
}

//자주 사용되는 event hanlder이므로 parent에서도 이벤트를 처리할 수 있도록 함
//(ON_CONTROL_REFLECT_EX: FALSE 반환 시 parent 에게도 CBN_SELCHANGE 가 전달됨)
BOOL CSCComboBox::OnCbnSelchange()
{
	//font combo라면 현재 선택된 폰트로 m_lf가 자동 변경되어야 한다.
	//set_font_name()은 m_is_font_combo==true 인 경우 early return 이므로
	//여기서는 m_lf를 직접 갱신하고 reconstruct_font() 를 호출.
	if (m_is_font_combo)
	{
		CString font_name = get_cur_sel_text();
		if (!font_name.IsEmpty())
		{
			_tcscpy_s(m_lf.lfFaceName, _countof(m_lf.lfFaceName), (LPCTSTR)font_name);
			m_lf.lfCharSet = DEFAULT_CHARSET;
			reconstruct_font();
		}
	}

	return FALSE;
}


void CSCComboBox::OnCbnSelendok()
{
	// TODO: 여기에 컨트롤 알림 처리기 코드를 추가합니다.
}


void CSCComboBox::OnCbnSelendcancel()
{
	// TODO: 여기에 컨트롤 알림 처리기 코드를 추가합니다.
}

//resize를 하면 여백이 리셋되므로 다시 여백 설정
/*
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
*/
BOOL CSCComboBox::PreTranslateMessage(MSG* pMsg)
{
	// TODO: 여기에 특수화된 코드를 추가 및/또는 기본 클래스를 호출합니다.
	//이 코드를 넣어줘야 disabled에서도 툴팁이 동작하는데
	//이 코드를 컨트롤 클래스에 넣어줘도 소용없다.
	//이 코드는 main에 있어야만 disable 상태일때도 잘 표시된다.
	if (m_use_tooltip && m_tooltip && m_tooltip->m_hWnd)
	{
		//msg를 따로 선언해서 사용하지 않고 *pMsg를 그대로 이용하면 이상한 현상이 발생한다.
		MSG msg = *pMsg;
		msg.hwnd = (HWND)m_tooltip->SendMessage(TTM_WINDOWFROMPOINT, 0, (LPARAM) & (msg.pt));

		CPoint pt = msg.pt;

		if (msg.message >= WM_MOUSEFIRST && msg.message <= WM_MOUSELAST)
			::ScreenToClient(msg.hwnd, &pt);

		msg.lParam = MAKELONG(pt.x, pt.y);

		// relay mouse event before deleting old tool 
		m_tooltip->SendMessage(TTM_RELAYEVENT, 0, (LPARAM)&msg);
	}

	if (pMsg->message == WM_KEYDOWN)
	{
		bool	is_exist = false;
		CString text;

		/*
		switch (pMsg->wParam)
		{
			case VK_RETURN:
				if (m_use_edit)
				{
					edit_end(true);
					return true;
				}
				break;
			case VK_ESCAPE:
				if (!m_pEdit || !m_pEdit->IsWindowVisible() || (GetFocus() != m_pEdit))
					break;

				TRACE(_T("escape\n"));
				m_pEdit->ShowWindow(SW_HIDE);
				SetWindowText(m_old_text);
				return true;
		}
		*/
	}

	return CComboBox::PreTranslateMessage(pMsg);
}

CString CSCComboBox::get_cur_sel_text()
{
	int index = GetCurSel();
	if (index < 0 || index >= GetCount())
		return CString();

	CString text;

	GetLBText(index, text);
	//TRACE(_T("GetCurSel() = %d, text = %s\n"), index, text);

	return text;
}

bool CSCComboBox::set_cur_sel(int index)
{
	if (index < 0 || index >= GetCount())
		return false;

	CComboBox::SetCurSel(index);

	// font combo 의 불변식: m_lf 는 항상 현재 선택된 폰트와 일치해야 한다.
	// SetCurSel 은 CBN_SELCHANGE 를 발생시키지 않으므로 여기서 직접 동기화.
	if (m_is_font_combo)
	{
		CString font_name;
		GetLBText(index, font_name);
		if (!font_name.IsEmpty())
		{
			_tcscpy_s(m_lf.lfFaceName, _countof(m_lf.lfFaceName), (LPCTSTR)font_name);
			m_lf.lfCharSet = DEFAULT_CHARSET;
			reconstruct_font();
		}
	}

	return true;
}

//현재 입력된 텍스트를 읽어오고 항목에 존재하지 않으면 추가시킨다. 레지스트리에도 저장한다.
int CSCComboBox::add(CString text, Gdiplus::Color cr_text)
{
	if (m_is_font_combo)
		return -1;

	int index = -1;

	if (text.IsEmpty())
		GetWindowText(text);

	if (text.IsEmpty())
		return -1;

	//중복 방지는 *정확히 같은* 항목만 — FindString 은 prefix 매칭이라 "dark"가 기존 "dark_gray"에, "solarized"가
	//"solarized_light"에 걸려 추가가 조용히 스킵되고 콤보 인덱스가 밀린다(2026-06-03 테마 콤보 발견). FindStringExact 로.
	if (FindStringExact(-1, text) < 0)
	{
		index = CComboBox::AddString(text);
		if (cr_text.GetValue() != Gdiplus::Color::Transparent)
		{
			CSCComboBoxColor* cr = new CSCComboBoxColor(cr_text);
			SetItemData(index, (DWORD_PTR)cr);
		}

		if (!m_reg_section.IsEmpty())
		{
			AfxGetApp()->WriteProfileInt(m_reg_section, _T("history count"), GetCount());
			AfxGetApp()->WriteProfileInt(m_reg_section, _T("current index"), GetCurSel());

			CString key;
			key.Format(_T("%03d"), GetCount() - 1);
			AfxGetApp()->WriteProfileString(m_reg_section, key, text);
		}
	}

	return index;
}

void CSCComboBox::set_color_theme(int theme)
{
	m_theme.set_color_theme(theme);
}

void CSCComboBox::set_color_theme(const CSCColorTheme& theme, bool invalidate)
{
	m_theme.copy_colors_from(theme);
	if (invalidate && m_hWnd)
		Invalidate();
}

void CSCComboBox::set_text_color(Gdiplus::Color cr_text)
{
	m_theme.cr_text = cr_text;
	Invalidate();
}

void CSCComboBox::set_back_color(Gdiplus::Color cr_back)
{
	m_theme.cr_back = cr_back;
	Invalidate();
}

void CSCComboBox::OnNcPaint()
{
	//combobox 의 NC border 를 theme 색으로 그린다. 이전엔 debug 용 빨강/마젠타 Draw3dRect 가 남아있었음.
	CWindowDC dc(this);

	CRect rc;
	GetWindowRect(&rc);
	rc.OffsetRect(-rc.TopLeft());

	//focus 여부에 따라 active/inactive border 색 선택 — CSCEdit 등 다른 SC* 컨트롤과 일관.
	bool focused = (GetFocus() == this) || (GetFocus() != nullptr && IsChild(GetFocus()));
	COLORREF cr_border = focused
		? m_theme.cr_border_active.ToCOLORREF()
		: m_theme.cr_border_inactive.ToCOLORREF();

	CBrush br(cr_border);
	dc.FrameRect(rc, &br);
}

void CSCComboBox::prepare_tooltip()
{
	if (m_tooltip)
	{
		m_tooltip->DestroyWindow();
		delete m_tooltip;
	}

	m_tooltip = new CToolTipCtrl();

	try
	{
		BOOL b = m_tooltip->Create(this, TTS_ALWAYSTIP | TTS_NOPREFIX | TTS_NOANIMATE);
	}
	catch (CException*)
	{
		CString str = get_error_str(GetLastError());
	}

	//m_tooltip->SetDelayTime(TTDT_AUTOPOP, -1);	//optional. 툴팁 표시 지속시간 설정.
	m_tooltip->SetDelayTime(TTDT_INITIAL, 500);		//optional. 툴팁을 표시하기 위해 마우스가 머물러야 할 최소 시간.
	//m_tooltip->SetDelayTime(TTDT_RESHOW, 0);		//optional. 포인터가 한 도구에서 다른 도구로 이동할 때 후속 도구 설명 창이 표시되는 데 걸리는 시간.
	m_tooltip->SetMaxTipWidth(400);					//optional. 툴팁창의 최대 너비로서 여러줄의 툴팁을 표시할 경우 필수. ‘\n’ 문자로 멀티라인 표현 가능.
	m_tooltip->Activate(TRUE);
	//EnableToolTips(TRUE);
	//EnableTrackingToolTips(TRUE);
}

void CSCComboBox::set_tooltip_text(CString text)
{
	m_tooltip_text = text;

	if (!text.IsEmpty())
		m_use_tooltip = true;

	if (!m_tooltip)
		prepare_tooltip();

	m_tooltip->UpdateTipText(m_tooltip_text, this);
	m_tooltip->AddTool(this, m_tooltip_text);
}

static BOOL CALLBACK EnumFontProc(LPLOGFONT lplf, LPTEXTMETRIC lptm, DWORD dwType, LPARAM lpData)
{
	CSCComboBox* pThis = reinterpret_cast<CSCComboBox*>(lpData);

	int index;

	if ((lplf->lfFaceName[0] == '@') || (dwType != TRUETYPE_FONTTYPE))
	{
		return TRUE;
	}
	else
	{
		pThis->add_font_list(lplf->lfFaceName);
		index = pThis->CComboBox::AddString(lplf->lfFaceName);
		//TRACE(_T("%s\n"), lplf->lfFaceName);
	}
	ASSERT(index != -1);

	return TRUE;
}

//font combo로 동작
void CSCComboBox::set_as_font_combo()
{
	m_is_font_combo = true;
	m_font_list.clear();
	CClientDC dc(this);

	EnumFonts(dc, 0, (FONTENUMPROC)EnumFontProc, (LPARAM)this); //Enumerate font
}

void CSCComboBox::OnDestroy()
{
	// edit subcontrol subclass 해제 (base OnDestroy 전에 처리).
	COMBOBOXINFO cbi = { sizeof(COMBOBOXINFO) };
	if (m_hWnd && ::GetComboBoxInfo(m_hWnd, &cbi) && cbi.hwndItem)
		::RemoveWindowSubclass(cbi.hwndItem, sccombo_edit_subclass, 1);

	CComboBox::OnDestroy();

	// TODO: 여기에 메시지 처리기 코드를 추가합니다.
	if (m_tooltip)
	{
		m_tooltip->DestroyWindow();
		delete m_tooltip;
	}

	m_font.DeleteObject();

	for (int i = 0; i < GetCount(); i++)
	{
		CSCComboBoxColor* cr = (CSCComboBoxColor*)GetItemData(i);
		if (cr)
			delete cr;
	}
}

void CSCComboBox::OnCbnEditchange()
{
	if (!m_use_input_filtering)
		return;

	// 입력마다 타이머 reset — 사용자가 타이핑을 "멈춘 시점" 에서 필터링 실행.
	// composing 중 즉시 필터링하면 ResetContent/ShowDropDown 의 연쇄 호출이 IMM 조합 오버레이를
	// 깨뜨리므로, 지연을 두고 만료 시점에 IMM 을 강제 commit 하여 안전한 상태에서 수행한다.
	static constexpr UINT filter_delay_ms_ime    = 700;	// 한글 조합 중 '멈춤' 감지 delay
	static constexpr UINT filter_delay_ms_normal = 1;	// 비-IME 입력은 거의 즉시
	KillTimer(TIMER_INPUT_FILTER);
	const UINT delay = is_ime_composing(m_hWnd) ? filter_delay_ms_ime : filter_delay_ms_normal;
	SetTimer(TIMER_INPUT_FILTER, delay, nullptr);
}

void CSCComboBox::OnTimer(UINT_PTR nIDEvent)
{
	if (nIDEvent == TIMER_INPUT_FILTER)
	{
		KillTimer(TIMER_INPUT_FILTER);

		// composing 중이면 IMM 에 "현재 조합을 완성된 문자열로 commit" 요청.
		// commit 이 edit buffer 를 업데이트해 CBN_EDITCHANGE 를 발생시키고, 그 경로에서 이 함수가
		// 다시 호출되어 non-composing 상태로 타이머가 걸린 뒤 apply_filter_now() 가 수행된다.
		// 여기서는 commit 만 하고 반환.
		if (is_ime_composing(m_hWnd))
		{
			COMBOBOXINFO cbi = { sizeof(COMBOBOXINFO) };
			HWND h_edit = (::GetComboBoxInfo(m_hWnd, &cbi) && cbi.hwndItem) ? cbi.hwndItem : m_hWnd;
			HIMC himc = ::ImmGetContext(h_edit);
			if (himc)
			{
				::ImmNotifyIME(himc, NI_COMPOSITIONSTR, CPS_COMPLETE, 0);
				::ImmReleaseContext(h_edit, himc);
			}
			return;
		}

		apply_filter_now();
		return;
	}

	CComboBox::OnTimer(nIDEvent);
}

void CSCComboBox::apply_filter_now()
{
	//20260823 by claude. 이 필터는 m_font_list 를 원본으로 목록을 *다시 만든다*. 그 목록은 set_as_font_combo /
	//add_font_list 로만 채워지는 폰트 콤보 전용이라, 일반 입력 콤보에서는 비어 있다. 그대로 진행하면
	//ResetContent 로 항목이 전부 지워지고 아무것도 복구되지 않아 *타이핑하는 순간 목록이 통째로 사라진다*.
	//(2026-08-23 발견: CBS_DROPDOWN 히스토리 콤보에서 입력할 때마다 히스토리가 1개로 리셋됐다.
	// 기존 사용처는 모두 CBS_DROPDOWNLIST 라 타이핑 경로가 없어 드러나지 않았다.)
	if (m_font_list.empty())
		return;

	CString filter;
	GetWindowText(filter);

	const CString raw_text = filter;
	filter.MakeLower();

	// 동일 filter 연속 호출 시 재구성 skip
	if (filter == m_last_filter)
		return;
	m_last_filter = filter;

	// 커서/선택 위치 보관
	const DWORD pos = GetEditSel();
	const int sel_start = LOWORD(pos);
	const int sel_end   = HIWORD(pos);

	SetRedraw(FALSE);
	ResetContent();
	for (const auto& item : m_font_list)
	{
		CString text = item;
		text.MakeLower();
		if (filter.IsEmpty() || text.Find(filter) != -1)
			CComboBox::AddString(item);
	}
	SetRedraw(TRUE);

	// 드롭다운 열기 — 필터된 항목 수에 맞춰 높이 동적 조정 후 표시.
	// 동적 height 로 리사이즈된 리스트박스가 slide-down 애니메이션으로 천천히 펼쳐지는 게
	// 어색하므로, 시스템 전역 SPI_SETCOMBOBOXANIMATION 을 잠깐 OFF → ShowDropDown → 원복.
	if (GetCount() > 0)
	{
		adjust_dropdown_height();

		BOOL combo_anim = TRUE;
		::SystemParametersInfo(SPI_GETCOMBOBOXANIMATION, 0, &combo_anim, 0);
		if (combo_anim)
			::SystemParametersInfo(SPI_SETCOMBOBOXANIMATION, 0, (PVOID)(DWORD_PTR)FALSE, 0);

		ShowDropDown(TRUE);

		if (combo_anim)
			::SystemParametersInfo(SPI_SETCOMBOBOXANIMATION, 0, (PVOID)(DWORD_PTR)TRUE, 0);
	}

	// Edit 텍스트 / 커서 복원 — 대소문자 보존용.
	SetWindowText(raw_text);
	SetEditSel(sel_start, sel_end);
}

void CSCComboBox::OnDrawItem(int nIDCtl, LPDRAWITEMSTRUCT lpDrawItemStruct)
{
	// 내부 ListBox가 보내는 WM_DRAWITEM의 CtlType은 ODT_LISTBOX이므로
	// DrawItem의 ASSERT를 통과하도록 ODT_COMBOBOX로 변경
	lpDrawItemStruct->CtlType = ODT_COMBOBOX;
	DrawItem(lpDrawItemStruct);
}
