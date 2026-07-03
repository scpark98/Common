// VtListCtrlEx.cpp : implementation file
//

//#include "stdafx.h"
#include "VtListCtrlEx.h"

#include <algorithm>
#include <set>

#include <locale>

#include "../../colors.h"
#include "../../MemoryDC.h"
#include <afxvslistbox.h>

#define IDC_EDIT_CELL	1001
#define WM_LIST_END_EDIT	(WM_USER + 9023)

// CVtListCtrlEx

IMPLEMENT_DYNAMIC(CVtListCtrlEx, CListCtrl)

CVtListCtrlEx* CVtListCtrlEx::s_editing_list = NULL;

//셀별 font style 안전 접근 — column 수 변화 등으로 미초기화면 default 0/false.
//OnDrawItem 의 cell 그리기에서 직접 호출 + setter 들의 _apply_style_field 와 짝.
static int  _safe_weight   (const std::deque<int>&  v, int i) { return ((int)v.size() > i && i >= 0) ? v[i] : 0; }
static BYTE _safe_bytestyle(const std::deque<BYTE>& v, int i) { return ((int)v.size() > i && i >= 0) ? v[i] : (BYTE)0; }

CVtListCtrlEx::CVtListCtrlEx()
{
	memset(&m_lf, 0, sizeof(LOGFONT));

	NCOverride = FALSE; //False as default...
	Who = SB_BOTH; //Default remove both...
}

CVtListCtrlEx::~CVtListCtrlEx()
{
	if (m_button_scroll_to_end)
	{
		m_button_scroll_to_end->DestroyWindow();
		delete m_button_scroll_to_end;
	}

	m_font.DeleteObject();
	clear_font_style_cache();
	safe_release_gradient_rect_handle();

	if (m_pEdit)
	{
		m_pEdit->DestroyWindow();
		delete m_pEdit;
	}

	CoUninitialize();
}


BEGIN_MESSAGE_MAP(CVtListCtrlEx, CListCtrl)
	ON_NOTIFY_REFLECT(LVN_GETDISPINFO, &CVtListCtrlEx::OnLvnGetdispinfo)
	ON_NOTIFY_REFLECT(LVN_ODFINDITEM, &CVtListCtrlEx::OnLvnOdfinditem)
	ON_NOTIFY_REFLECT(LVN_ODCACHEHINT, &CVtListCtrlEx::OnLvnOdcachehint)
	ON_NOTIFY_REFLECT(LVN_COLUMNCLICK, &CVtListCtrlEx::OnLvnColumnclick)
	ON_NOTIFY_REFLECT(LVN_ODSTATECHANGED, &CVtListCtrlEx::OnLvnOdstatechanged)
	ON_WM_ERASEBKGND()
	ON_WM_PAINT()
	ON_NOTIFY_REFLECT_EX(LVN_BEGINLABELEDIT, &CVtListCtrlEx::OnLvnBeginLabelEdit)
	ON_NOTIFY_REFLECT_EX(LVN_ENDLABELEDIT, &CVtListCtrlEx::OnLvnEndLabelEdit)
	ON_NOTIFY_REFLECT_EX(NM_DBLCLK, &CVtListCtrlEx::OnNMDblclk)
	ON_NOTIFY_REFLECT_EX(NM_CLICK, &CVtListCtrlEx::OnNMClick)
	ON_WM_DROPFILES()
	ON_WM_MEASUREITEM_REFLECT()
	ON_NOTIFY_REFLECT(LVN_BEGINDRAG, &CVtListCtrlEx::OnLvnBeginDrag)
	ON_WM_MOUSEMOVE()
	ON_WM_LBUTTONUP()
	ON_NOTIFY_REFLECT_EX(LVN_ITEMCHANGED, &CVtListCtrlEx::OnLvnItemchanged)
	ON_WM_CONTEXTMENU()
	ON_NOTIFY_REFLECT_EX(NM_RCLICK, &CVtListCtrlEx::OnNMRClick)
	ON_COMMAND_RANGE(menu_select_all, menu_unselect_all, &CVtListCtrlEx::OnPopupMenu)
	ON_WM_MOUSEHOVER()
	ON_WM_MOUSELEAVE()
	ON_WM_SETFOCUS()
	ON_WM_KILLFOCUS()
	ON_REGISTERED_MESSAGE(Message_CSCStaticEdit, &CVtListCtrlEx::on_message_CSCStaticEdit)
	ON_MESSAGE(WM_LIST_END_EDIT, &CVtListCtrlEx::on_end_edit_posted)
	ON_NOTIFY_REFLECT_EX(LVN_ITEMCHANGING, &CVtListCtrlEx::OnLvnItemchanging)
	ON_WM_LBUTTONDOWN()
	ON_WM_NCHITTEST()
	ON_WM_HSCROLL()
	ON_WM_VSCROLL()
	ON_NOTIFY_REFLECT(LVN_BEGINSCROLL, &CVtListCtrlEx::OnLvnBeginScroll)
	ON_NOTIFY_REFLECT(LVN_ENDSCROLL, &CVtListCtrlEx::OnLvnEndScroll)
	ON_WM_SIZE()
	ON_WM_WINDOWPOSCHANGED()
	ON_REGISTERED_MESSAGE(Message_CGdiButton, &CVtListCtrlEx::on_message_CGdiButton)
	ON_REGISTERED_MESSAGE(Message_CHeaderCtrlEx, &CVtListCtrlEx::on_message_CHeaderCtrlEx)
	ON_WM_NCPAINT()
	ON_WM_NCCALCSIZE()
	ON_WM_STYLECHANGING()
	ON_WM_MOUSEWHEEL()
	ON_WM_MOUSEHWHEEL()
	ON_REGISTERED_MESSAGE(Message_CSCScrollbar, &CVtListCtrlEx::on_message_CSCScrollbar)
	ON_WM_TIMER()
END_MESSAGE_MAP()

//드래그 중 대상 컨트롤(트리/리스트) 가장자리 호버 시 연속 자동 스크롤용 타이머 ID.
#define TIMER_ID_DRAG_AUTO_SCROLL	0x7A31



// CVtListCtrlEx message handlers
void CVtListCtrlEx::HideScrollBars(int Type, int Which)
{
	if (Type == LCSB_CLIENTDATA) //This is the clientrect function
	{
		RECT ierect;
		int cxvs, cyvs;
		GetClientRect(&ierect); //Get client width and height

		cxvs = GetSystemMetrics(SM_CXVSCROLL); //Get the system metrics - VERT
		cyvs = GetSystemMetrics(SM_CYVSCROLL); //Get the system metrics - HORZ

		if (Which == SB_HORZ) cxvs = 0; //Set VERT to zero when choosen HORZ
		if (Which == SB_VERT) cyvs = 0; //Set HORZ to zero when choosen VERT

		//Here we set the position of the window to the clientrect + the size of the scrollbars
		SetWindowPos(NULL, ierect.left, ierect.top, ierect.right + cxvs, ierect.bottom + cyvs, SWP_NOMOVE | SWP_NOZORDER);

		//Her we modify the rect so the right part is subbed from the rect.
		if (Which == SB_BOTH || Which == SB_HORZ) ierect.bottom -= ierect.top;
		if (Which == SB_BOTH || Which == SB_VERT) ierect.right -= ierect.left;

		//Just to be safe that the left/top corner is 0...
		ierect.top = 0;
		ierect.left = 0;

		HRGN iehrgn = NULL; //This range is created base on which scrollbar that is going to be removed!

		//The -2 is probably a border of some kind that we also need to remove. I could not find any good
		//metrics that gave me an 2 as an answer. So insted we makes it static with -2.
		if (Which == SB_BOTH) iehrgn = CreateRectRgn(ierect.left, ierect.top, ierect.right - 2, ierect.bottom - 2);
		if (Which == SB_HORZ) iehrgn = CreateRectRgn(ierect.left, ierect.top, ierect.right, ierect.bottom - 2);
		if (Which == SB_VERT) iehrgn = CreateRectRgn(ierect.left, ierect.top, ierect.right - 2, ierect.bottom);

		//After the range has been made we add it...
		SetWindowRgn(iehrgn, TRUE);

		//Reset of NCOverride
		NCOverride = FALSE;
	}

	if (Type == LCSB_NCOVERRIDE) //This is the NcCalcSize override
	{
		NCOverride = TRUE; //Set to true, so we run the code on each OnNcCalcSize.
		Who = Which; //Selects which scrollbars to get hidden.
	}
}




void CVtListCtrlEx::OnLvnGetdispinfo(NMHDR *pNMHDR, LRESULT *pResult)
{
	*pResult = 0;

	//if (!m_use_virtual_list)
	//	return;

	//NMLVDISPINFO *pDispInfo = reinterpret_cast<NMLVDISPINFO*>(pNMHDR);
	// TODO: Add your control notification handler code here
	LV_DISPINFO* pDispInfo = (LV_DISPINFO*)pNMHDR;

	//Create a pointer to the item
	LV_ITEM* pItem = &(pDispInfo)->item;

	//Which item number?

	//Do the list need text information?
	if (pItem->mask & LVIF_TEXT)
	{
		CString text;

		//Which column?
		if (pItem->iItem < 0 || pItem->iItem >= m_list_db.size() || pItem->iSubItem < 0 || pItem->iSubItem >= m_list_db[pItem->iItem].text.size())
			return;

		text = m_list_db[pItem->iItem].text[pItem->iSubItem];

		//Copy the text to the LV_ITEM structure
		//Maximum number of characters is in pItem->cchTextMax
		lstrcpyn(pItem->pszText, text, pItem->cchTextMax);
	}

	//Do the list need image information?
	if(pItem->mask & LVIF_IMAGE) 
	{
		//Set which image to use
		//pItem->iImage = m_list_db[pItem->iItem].img_idx;

		//Show check box?
		if(pItem->mask & LVIF_STATE)
		{
			//To enable check box, we have to enable state mask...
			pItem->mask |= LVIF_STATE;
			pItem->stateMask = LVIS_STATEIMAGEMASK;

			//m_list_db[pItem->iItem].checked = !m_list_db[pItem->iItem].checked;
			//TRACE(_T("onGetDispInfo() iItem = %d, checked = %d\n"), pItem->iItem, m_list_db[pItem->iItem].checked);
			//if(GetCheck(pItem->iItem))
			if (m_list_db[pItem->iItem].checked)
			{
				SetCheck(pItem->iItem, m_list_db[pItem->iItem].checked);
				//Turn check box on..
				pItem->state = INDEXTOSTATEIMAGEMASK(2);
				//SetItemState(pItem->iItem, INDEXTOSTATEIMAGEMASK(2), LVIS_STATEIMAGEMASK);
			}
			else
			{
				SetCheck(pItem->iItem, m_list_db[pItem->iItem].checked);
				//Turn check box off
				pItem->state = INDEXTOSTATEIMAGEMASK(1);
				//SetItemState(pItem->iItem, INDEXTOSTATEIMAGEMASK(1), LVIS_STATEIMAGEMASK);
			}
		}
	}

	*pResult = 0;
}

//virtual listctrl인 경우는 FindItem() 사용 시 무조건 0을 리턴하게 된다.
//따라서 이 이벤트 핸들러 함수를 다음과 같이 수정함.
void CVtListCtrlEx::OnLvnOdfinditem(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMLVFINDITEM pFindInfo = reinterpret_cast<LPNMLVFINDITEM>(pNMHDR);
	// TODO: Add your control notification handler code here
	*pResult = -1;

	if ((pFindInfo->lvfi.flags & LVFI_STRING) == 0)
		return;

	CString searchText = pFindInfo->lvfi.psz;
	int find_index = -1;

	std::deque<CListCtrlData>::iterator it = std::find_if(m_list_db.begin(), m_list_db.end(),
		[&](const CListCtrlData& item) ->
		bool
		{
			return (searchText.CompareNoCase(item.text[0]) == 0);
		});

	if (it == m_list_db.end())
		*pResult = -1;
	else
		*pResult = std::distance(m_list_db.begin(), it);
}


void CVtListCtrlEx::OnLvnOdcachehint(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMLVCACHEHINT pCacheHint = reinterpret_cast<LPNMLVCACHEHINT>(pNMHDR);
	// TODO: Add your control notification handler code here
	*pResult = 0;
}


void CVtListCtrlEx::DrawItem(LPDRAWITEMSTRUCT lpDIS/*lpDrawItemStruct*/)
{
	if (!m_use_virtual_list)
		return;

	int			iItem		= (int)lpDIS->itemID;
	int			iSubItem;
	CDC			*pDC		= CDC::FromHandle(lpDIS->hDC);
	CRect		rowRect;
	CRect		itemRect;
	CRect		textRect;
	bool		is_show_selection_always = (GetStyle() & LVS_SHOWSELALWAYS);
	//TRACE(_T("is_show_selection_always = %d\n"), is_show_selection_always);
	bool		is_selected = (GetItemState(iItem, LVIS_SELECTED) & LVIS_SELECTED);
	bool		is_drophilited = GetItemState(iItem, LVIS_DROPHILITED);
	bool		is_full_row_selection = (GetExtendedStyle() & LVS_EX_FULLROWSELECT);

	Gdiplus::Color	crText = m_theme.cr_text;
	Gdiplus::Color	crBack = m_theme.cr_back;
	//Gdiplus::Graphics	g(pDC->m_hDC);


	//TRACE(_T("iItem = %d, is_selected = % d\n"), iItem, is_selected);

	GetItemRect(iItem, &rowRect, LVIR_BOUNDS);

	for (iSubItem = 0; iSubItem < get_column_count(); iSubItem++)
	{
		//TRACE(_T("%d, %d\n"), iItem, iSubItem);
		if (iItem == 1)
			iItem = 1;

		crText = m_list_db[iItem].crText[iSubItem];
		crBack = m_list_db[iItem].crBack[iSubItem];

		GetSubItemRect(iItem, iSubItem, LVIR_BOUNDS, itemRect);

		if (iSubItem == 0)
		{
			//LVIR_BOUNDS로 구할 경우 0번 컬럼은 한 라인의 사각형 영역을 리턴한다.
			//따라서 right값을 보정해줘야 한다.
			//1번 컬럼부터는 해당 셀의 사각형 영역만을 리턴한다.
			//itemRect = get_item_rect(iItem, iSubItem);
			itemRect.right = itemRect.left + GetColumnWidth(0);
		}
		//0번 컬럼만 선택상태로 표시한다.
		//else if (!is_full_row_selection)
		//{
		//	is_selected = false;
		//}

		//if(lpDIS->itemState & ODS_SELECTED) //ok
		//포커스를 가졌거나 Always Show Selection이라면 선택 항목의 색상을 표시해주고
		//주의. fullrowselection & is_selected라도 해당 셀의 색상이 기본색이 아닌 별도 색으로 지정되어 있다면
		//그 색상 그대로 표시되어야 한다.
		if ((m_has_focus || is_show_selection_always) && is_selected) //ok
		{
			//셀 색 지정이 있으면 selected 든 inactive selected 든 그 색 우선 — 사용자가 set_text_color/set_back_color 로
			//명시한 의도가 더 강함. 미지정 (listctrlex_unused_color) 셀만 theme 의 selected 색으로 fallback.
			//(이전엔 selected 시 셀 색을 무조건 덮어써 자막창의 red 자막이 selected 시 white 로 바뀌는 회귀 발생.)
			if (m_has_focus)
			{
				//unused(미지정) 또는 weak(보조 텍스트) 셀은 선택 시 cr_text_selected 로 — 보조색은 선택 배경 위에서 가독성 위해 양보.
				if (crText.GetValue() == listctrlex_unused_color.GetValue() || crText.GetValue() == listctrlex_weak_color.GetValue())
					crText = m_theme.cr_text_selected;
				if ((is_full_row_selection || iSubItem == 0) && crBack.GetValue() == listctrlex_unused_color.GetValue())
					crBack = m_theme.cr_back_selected;
			}
			else
			{
				if (crText.GetValue() == listctrlex_unused_color.GetValue() || crText.GetValue() == listctrlex_weak_color.GetValue())
					crText = m_theme.cr_text_selected_inactive;
				if ((is_full_row_selection || iSubItem == 0) && crBack.GetValue() == listctrlex_unused_color.GetValue())
					crBack = m_theme.cr_back_selected_inactive;
			}
		}
		//drophilited라면 active에 관계없이 drop hilited 색상으로 표시한다.
		//단 대상 항목이 파일인 경우는 drop hilited 표시를 하지 않는다.
		else if (is_drophilited) //ok
		{
			if (crText.GetValue() == listctrlex_unused_color.GetValue() || crText.GetValue() == listctrlex_weak_color.GetValue())
				crText = m_theme.cr_text_selected;
			if (crBack.GetValue() == listctrlex_unused_color.GetValue())
				crBack = m_theme.cr_back_selected;
		}
		else
		{
			//비선택: unused → 기본 글자색, weak sentinel → 본문색을 cr_back 쪽으로 *비율 blend* 해 보조색(크기/날짜)으로, 그 외 명시색 유지.
			//get_weak_color(cr, offset) 의 절대 offset 방식은 테마의 fg/bg 간격을 무시해, 어떤 테마는 적절히 약하지만
			//저대비 테마(본문색이 배경과 가까운)에선 offset 이동만으로 배경과 비슷해져 거의 안 보였다.
			//get_color(cr_text, cr_back, ratio) 는 *대비에 비례* 약화 → 모든 테마에서 일관된 보조색(절대 배경색까진 안 감).
			//ratio: 0=본문색, 1=배경색. 0.4 = 배경 쪽으로 40% (가독성 유지하며 명확히 흐림). 값만 조정하면 강도 튜닝 가능.
			if (crText.GetValue() == listctrlex_unused_color.GetValue())
				crText = m_theme.cr_text;
			else if (crText.GetValue() == listctrlex_weak_color.GetValue())
				crText = get_color(m_theme.cr_text, m_theme.cr_back, 0.4);
			else
				crText = m_list_db[iItem].crText[iSubItem];

			crBack = m_list_db[iItem].crBack[iSubItem];
			if (crBack.GetValue() == listctrlex_unused_color.GetValue())
			{
				if (m_use_alternate_back_color && (iItem % 2))
					crBack = m_theme.cr_back_alternate;
				else
					crBack = m_theme.cr_back;
			}
		}

		//선택 항목의 텍스트 색상은 무조건 컬러 스킴을 따르는게 아니라
		//지정된 색이 있다면 그 색을 우선으로 해야 한다.
		//if (m_list_db[iItem].crText[iSubItem].GetValue() != listctrlex_unused_color.GetValue())
		//	crText = m_list_db[iItem].crText[iSubItem];
		//if (m_list_db[iItem].crBack[iSubItem].GetValue() != listctrlex_unused_color.GetValue())
		//	crBack = m_list_db[iItem].crBack[iSubItem];
	
		if (crBack.GetValue() != listctrlex_unused_color.GetValue())
		{
			if (crBack.GetValue() == Gdiplus::Color::Transparent)
				crBack = m_list_db[iItem].crBack[iSubItem];

			pDC->FillSolidRect(itemRect, crBack.ToCOLORREF());
		}
		else if (m_use_alternate_back_color && (iItem % 2))
		{
			pDC->FillSolidRect(itemRect, m_theme.cr_back_alternate.ToCOLORREF());
		}

		if ((iSubItem == 0) && (GetExtendedStyle() & LVS_EX_CHECKBOXES))
		{
			//checkbox는 좌우 4 여백을 두고 크기는 14x14이다.
			CRect r = itemRect;
			r.left += 4;
			r.right = r.left + 14;
			r.top = r.CenterPoint().y - 7;
			r.bottom = r.top + 14;
			draw_rect(pDC, r, m_theme.cr_text);

			int check_state = m_list_db[iItem].checked;// GetCheck(iItem);

			if (check_state == BST_CHECKED)
			{
				//1은 얇고 2는 두껍다. Gdiplus::Pen으로 그려야하는데 DrawItem()에서 Gdiplus::Graphics를 사용하면 문제가 있다.
				//Gdiplus::Pen 설정 시 width를 1.5로 해도 적용되지 않는다. 
				draw_line(pDC, r.left + 2, r.CenterPoint().y - 1, r.left + 5, r.CenterPoint().y + 2, m_theme.cr_text.ToCOLORREF(), 2);
				draw_line(pDC, r.left + 5, r.CenterPoint().y + 2, r.left + 5 + 6, r.CenterPoint().y + 2 - 6, m_theme.cr_text.ToCOLORREF(), 2);
			}
			else if (check_state == BST_INDETERMINATE)
			{
				CRect inner = r;
				inner.DeflateRect(3, 3);
				draw_rect(pDC, inner, Gdiplus::Color::Transparent, m_theme.cr_text);
			}

			itemRect.left = r.right + 2;
		}

		//percentage 타입이면 바그래프 형태로 그려주고
		if (get_column_data_type(iSubItem) == column_data_type_percentage_bar)
		{
			CRect r = itemRect;

			//바그래프는 셀의 높이, 즉 라인 간격과는 관계없다. 폰트의 높이값에 비례하는 높이로 그려주자.
			int cy = r.CenterPoint().y;
			r.DeflateRect(4, 0);
			r.top = cy + m_lf.lfHeight/3.4;		//이 값을 키우면 바그래프의 높이가 낮아진다.
			r.bottom = cy - m_lf.lfHeight/3.4;

			double d = _ttof(m_list_db[iItem].text[iSubItem]);
			d /= 100.0;
			Clamp(d, 0.0, 1.0);
			
			r.right = r.left + (double)(r.Width()) * d;

			if (m_theme.cr_percentage_bar.size() == 1)
			{
				pDC->FillSolidRect(r, m_theme.cr_percentage_bar[0].ToCOLORREF());
			}
			else if (m_theme.cr_percentage_bar.size() > 1)
			{
				//현재 레벨에 맞는 단색으로 채울 경우
				if (false)
				{
					pDC->FillSolidRect(r, get_color(m_theme.cr_percentage_bar[0], m_theme.cr_percentage_bar[1], d).ToCOLORREF());
				}
				//현재 레벨까지 그라디언트로 채울 경우
				else
				{
					std::deque<Gdiplus::Color> dqColor;
					dqColor.push_back(m_theme.cr_percentage_bar[0]);
					dqColor.push_back(get_color(m_theme.cr_percentage_bar[0], m_theme.cr_percentage_bar[1], d));
					gradient_rect(pDC, r, dqColor, false);
				}
			}
		}
		else if (get_column_data_type(iSubItem) == column_data_type_percentage_grid)
		{
			CRect r = itemRect;

			//바그래프는 셀의 높이, 즉 라인 간격과는 관계없다. 폰트의 높이값에 비례하는 높이로 그려주자.
			int cy = r.CenterPoint().y;
			r.DeflateRect(4, 0);
			r.top = cy + m_lf.lfHeight / 3.4;		//이 값을 키우면 바그래프의 높이가 낮아진다.
			r.bottom = cy - m_lf.lfHeight / 3.4;

			double d = _ttof(m_list_db[iItem].text[iSubItem]);
			d /= 100.0;
			Clamp(d, 0.0, 1.0);

			r.right = r.left + (double)(r.Width()) * d;

			if (m_theme.cr_percentage_bar.size() == 1)
			{
				pDC->FillSolidRect(r, m_theme.cr_percentage_bar[0].ToCOLORREF());
			}
			else if (m_theme.cr_percentage_bar.size() > 1)
			{
				std::deque<Gdiplus::Color> dqColor;
				dqColor.push_back(m_theme.cr_percentage_bar[0]);
				int hue0 = get_hue(m_theme.cr_percentage_bar[0].ToCOLORREF());
				int hue1 = get_hue(m_theme.cr_percentage_bar[1].ToCOLORREF());
				dqColor.push_back(get_color(hue0, hue1, (int)(d * 100.0), 1.0f, 1.0f));
				gradient_rect(pDC, r, dqColor, false);
			}

			int i;
			int half = (int)((double)(r.Height()) / 2.2);
			for (i = r.left + half; i < itemRect.right - half; i += (half * 1.5))
			{
				pDC->FillSolidRect(i, r.top, half / 2, r.Height(), crBack.ToCOLORREF());
			}
		}
		else if (get_column_data_type(iSubItem) == column_data_type_progress)
		{
			CRect r = itemRect;

			//바그래프는 셀의 높이, 즉 라인 간격에 상대적이지 않고 폰트의 높이값에 비례하는 높이로 그려줘야 한다.
			int cy = r.CenterPoint().y;
			r.top = cy + (double)m_lf.lfHeight / 1.8;		//나누는 값이 커지면 바그래프의 높이가 낮아진다.
			r.bottom = cy - (double)m_lf.lfHeight / 1.8;	//m_lf.lfHeight가 음수이므로 -,+가 아니라 +,-인 점에 주의

			double d = _ttof(m_list_db[iItem].text[iSubItem]);
			d /= 100.0;
			Clamp(d, 0.0, 1.0);

			r.right = r.left + (double)(r.Width()) * d;
			pDC->FillSolidRect(r, m_theme.cr_progress.ToCOLORREF());

			//20231102 CSCSliderCtrl에서와 동일하게 progress 경과 위치에 따라 왼쪽과 오른쪽을 각각 다른 색으로 표현하고자
			//아래 코드를 사용했으나 텍스트가 전혀 출력되지 않는다.
			//pDC가 아닌 CMemoryDC를 얻어서 출력하니 원하는대로 출력되지만 다른 row의 값이 그려지지 않는다.
			//Rgn과 관련하여 OnPaint()와 DrawItem은 뭔가 차이가 있다.
			if (m_show_progress_text)
			{
				CString text = m_list_db[iItem].text[iSubItem];
				CString sPercent;
				
				if (!text.IsEmpty())
				{
					//progress type이라도 무조건 %를 붙여서는 안된다. text가 숫자이며 끝에 %가 없을 경우에만 자동으로 붙여준다.
					if (IsNumericString(text) && (text.GetLength() > 0) && (text.Right(1) != '%'))
						text += _T("%");

					//text color를 별도로 준 경우는 그 컬러를 쓰고 그렇지 않으면 m_theme에 정의된 컬러를 쓴다.
					if (m_list_db[iItem].crText[iSubItem].GetValue() != listctrlex_unused_color.GetValue())
						pDC->SetTextColor(m_list_db[iItem].crText[iSubItem].ToCOLORREF());
					else
						//pDC->SetTextColor(m_theme.cr_progress_text.ToCOLORREF());
						pDC->SetTextColor(crText.ToCOLORREF());
#if 0
					pDC->DrawText(text, itemRect, DT_VCENTER | DT_CENTER | DT_SINGLELINE);
#else
					//progress 경과 위치에 따라 왼쪽과 오른쪽을 각각 다른 색으로 표현하기 위해 클리핑 영역을 나눠서 텍스트를 두 번 그려준다.
					CRect rcLeft, rcRight;
					rcLeft = rcRight = itemRect;
					rcRight.left = rcLeft.right = r.right;

					//OnPaint()에서 header ctrl 유무에 따라 MemoryDC의 높이를 보정해주므로
					//SelectClipRgn()을 사용하려면 LPtoDP()로 좌표 보정이 필요하다.
					pDC->LPtoDP(&rcLeft);
					pDC->LPtoDP(&rcRight);

					CRgn rgnLeft, rgnRight;
					rgnLeft.CreateRectRgnIndirect(&rcLeft);
					rgnRight.CreateRectRgnIndirect(&rcRight);

					pDC->SetTextColor(m_theme.cr_back.ToCOLORREF());
					pDC->SelectClipRgn(&rgnLeft);
					pDC->DrawText(text, itemRect, DT_VCENTER | DT_CENTER | DT_SINGLELINE | DT_NOCLIP | DT_NOPREFIX);

					pDC->SetTextColor(m_theme.cr_progress.ToCOLORREF());
					pDC->SelectClipRgn(&rgnRight);
					pDC->DrawText(text, itemRect, DT_VCENTER | DT_CENTER | DT_SINGLELINE | DT_NOCLIP | DT_NOPREFIX);
					rgnLeft.DeleteObject();
					rgnRight.DeleteObject();
					pDC->SelectClipRgn(NULL);
#endif
				}
			}			
		}
		//텍스트 형태이면 문자열을 출력해준다.
		else
		{
			pDC->SetTextColor(crText.ToCOLORREF());

			//텍스트가 그려질 때 itemRect에 그리면 좌우 여백이 없어서 양쪽이 꽉차보인다.
			//약간 줄여서 출력해야 보기 쉽다.
			textRect = itemRect;
			CString text = get_text(iItem, iSubItem);

			//0번 컬럼의 text 앞에 공백이 있을 경우 이를 들여쓰기로 처리하는 경우
			if (iSubItem == 0 && m_use_indent_from_prefix_space)
			{
				int space_count = get_char_count(text, ' ', true);
				textRect.left += (space_count * 16);
				text = text.Mid(space_count);
			}

			//아이콘 표시
			if (iSubItem == 0 &&
				(m_is_shell_listctrl ||
				(m_use_own_imagelist && m_pShellImageList && m_pShellImageList->get_imagelist() && m_pShellImageList->get_imagelist()->GetImageCount())))
			{
				//16x16 아이콘을 22x21 영역에 표시한다. (21은 기본 height이며 m_line_height에 따라 달라진다.)
				textRect.left += 3;
				int icon_index;

				if (m_list_db[iItem].img_idx >= 0 && (m_list_db[iItem].img_idx < m_pShellImageList->m_imagelist_small.GetImageCount()))
				{
					icon_index = m_list_db[iItem].img_idx;
					m_pShellImageList->m_imagelist_small.Draw(pDC, icon_index,
							CPoint(textRect.left, textRect.CenterPoint().y - 8), ILD_TRANSPARENT);
				}

				textRect.left += 16;	//small icon width
				textRect.left += 3;		//margin between icon and text
			}
			else
			{
				textRect.DeflateRect(4, 0);
			}

			//레이블 출력
			UINT format = 0;

			if (get_column_text_align(iSubItem) == LVCFMT_LEFT)
				format = DT_LEFT;
			else if (get_column_text_align(iSubItem) == LVCFMT_CENTER)
				format = DT_CENTER;
			else if (get_column_text_align(iSubItem) == LVCFMT_RIGHT)
				format = DT_RIGHT;

			format = format | DT_SINGLELINE | DT_VCENTER | DT_WORD_ELLIPSIS | DT_NOPREFIX;

			//셀별 font style — 하나라도 set 되어 있으면 캐시된 styled font 로 SelectObject 후 그리고 복원.
			//모두 default (weight=0, italic/underline/strikeout=0) 이면 base font 그대로 (캐시 우회).
			int  cell_weight    = _safe_weight   (m_list_db[iItem].weight,    iSubItem);
			bool cell_italic    = _safe_bytestyle(m_list_db[iItem].italic,    iSubItem) != 0;
			bool cell_underline = _safe_bytestyle(m_list_db[iItem].underline, iSubItem) != 0;
			bool cell_strikeout = _safe_bytestyle(m_list_db[iItem].strikeout, iSubItem) != 0;
			CFont* pOldFont = nullptr;
			if (cell_weight != 0 || cell_italic || cell_underline || cell_strikeout)
			{
				CFont* pStyled = get_styled_font(cell_weight, cell_italic, cell_underline, cell_strikeout);
				if (pStyled)
					pOldFont = pDC->SelectObject(pStyled);
			}
			pDC->DrawText(text, textRect, format);
			if (pOldFont)
				pDC->SelectObject(pOldFont);
		}
	}

	//선택된 항목은 선택 색상보다 진한 색으로 테두리가 그려진다.
	//Tree 와 일관 — focus 있을 때만 border. inactive 는 배경 fill (위 crBack) 만.
	if (m_draw_selected_border && !m_in_editing && m_has_focus && is_selected)
	{
		GetSubItemRect(iItem, 0, LVIR_BOUNDS, rowRect);
		if (!is_full_row_selection)
			rowRect.right = rowRect.left + GetColumnWidth(0);

		//선택된 항목을 표시하는 사각형을 그릴때는 반드시 PenAlignmentInset으로 그려줘야 한다.
		//특히 width가 2이상이면 unselect되는 항목의 선택 사각형 표시가 갱신되지 않게 되므로
		//선택 사각형은 반드시 inset으로 그려져야 한다.
		if (m_use_distinct_border_color)
			draw_rect(pDC, rowRect, get_distinct_color(crBack), Gdiplus::Color::Transparent, m_selected_border_width, Gdiplus::PenAlignmentInset, m_selected_border_style);
		else
			draw_rect(pDC, rowRect, m_theme.cr_selected_border, Gdiplus::Color::Transparent, m_selected_border_width, Gdiplus::PenAlignmentInset, m_selected_border_style);
	}

	GetSubItemRect(iItem, 0, LVIR_BOUNDS, rowRect);

	if (m_draw_top_line)
	{
		draw_line(pDC, rowRect.left, rowRect.top, rowRect.right, rowRect.top, m_cr_top_line.ToCOLORREF());
	}

	if (m_draw_bottom_line)
	{
		//rowRect.bottom으로 써주면 아이템 영역밖이므로 그려지지 않는다. 반드시 -1을 해야 함.
		draw_line(pDC, rowRect.left, rowRect.bottom - 1, rowRect.right, rowRect.bottom - 1, m_cr_bottom_line.ToCOLORREF());
	}
}

// ex. "No,20;Item1,50;Item2,50"
//scpark 20230801
//어느 프로젝트에서 이 함수가 2번 호출되어 실제 컬럼은 3개인데
//6개로 세팅되어 오류가 발생함.
//매번 호출시 리셋하고 처리해도 되고 다른 처리 방법도 있으나
//정확히 한번만 호출하여 사용하는 것이 정석이므로 우선 별도의 처리는 하지 않음.
bool CVtListCtrlEx::set_headings(const CString& strHeadings)
{
	//TRACE(_T("list = %p\n"), this);

	int iStart = 0;
	int	column = 0;

	for(;;)
	{
		const int iComma = strHeadings.Find(_T(','), iStart);

		if(iComma == -1)
			break;

		const CString strHeading = strHeadings.Mid(iStart, iComma - iStart);

		iStart = iComma + 1;

		int iSemiColon = strHeadings.Find(_T(';'), iStart);

		if(iSemiColon == -1)
			iSemiColon = strHeadings.GetLength();

		const int iWidth = _tstoi((TCHAR*)(LPCTSTR)strHeadings.Mid(iStart, iSemiColon - iStart));

		iStart = iSemiColon + 1;

		if (InsertColumn(column++, strHeading, LVCFMT_LEFT, iWidth) == -1)
			break;
	}

	//컬럼의 수가 결정되면 컬럼과 관련된 정보 저장 공간의 크기를 세팅해준다.
	m_column_data_type.resize(column);
	m_column_data_type.assign(column, column_data_type_text);

	m_column_sort_type.resize(column);
	m_column_sort_type.assign(column, sort_none);

	m_column_text_align.resize(column);
	m_column_text_align.assign(column, LVCFMT_LEFT);

	//modify_style();

	if (m_HeaderCtrlEx && m_HeaderCtrlEx.m_hWnd)
	{
		m_HeaderCtrlEx.m_header_text_align.resize(column);
		m_HeaderCtrlEx.m_header_text_align.assign(column, HDF_LEFT);
	}

	m_allow_edit_column.resize(column);

	return TRUE;
}

//column = -1이면 전체 헤더의 텍스트를 '|'로 구분자로 하여 리턴한다.
CString	CVtListCtrlEx::get_header_text(int column)
{
	if (column < 0)
	{
		CString sText;
		for (int i = 0; i < get_column_count(); i++)
		{
			if (i > 0)
				sText += _T("|");
			sText += m_HeaderCtrlEx.get_header_text(i);
		}
		return sText;
	}

	return m_HeaderCtrlEx.get_header_text(column);
}

void CVtListCtrlEx::set_header_text(int column, CString sText)
{
	m_HeaderCtrlEx.set_header_text(column, sText);
}

int CVtListCtrlEx::get_column_count()
{
	return m_HeaderCtrlEx.GetItemCount();
}

int	CVtListCtrlEx::get_column_text_align(int column)
{
	return m_column_text_align[column];
}

//해당 컬럼에서 문자열 길이 최대값을 리턴.
int CVtListCtrlEx::get_column_max_text_length(int column, bool real_bytes)
{
	int len;
	int max_len = 0;

	for (int i = 0; i < size(); i++)
	{
		if (real_bytes)
			len = WideCharToMultiByte(CP_ACP, 0, (CStringW)get_text(i, column), -1, NULL, 0, NULL, NULL);
		else
			len = get_text(i, column).GetLength();
		if (len > max_len)
			max_len = len;
	}

	return max_len;
}

void CVtListCtrlEx::get_column_max_text_length(std::vector<int>& col_len, bool real_bytes)
{
	col_len.clear();
	col_len.resize(get_column_count());

	for (int i = 0; i < get_column_count(); i++)
		col_len[i] = get_column_max_text_length(i, real_bytes);
}

//해당 컬럼에서 출력시에 width 최대값을 리턴.(미구현)
int CVtListCtrlEx::get_column_max_text_width(int column)
{
	//헤더 분리자 더블클릭 시 자동맞춤 너비 산출. 시스템 LVSCW_AUTOSIZE 는
	// (a) 첫 호출과 두 번째 호출이 헤더/항목 기준을 toggle 하는 부자연스러운 동작,
	// (b) owner-draw + small image 가 그리는 아이콘 폭을 measure 에 포함 안 함,
	// (c) virtual list 일부 케이스에서 measure 실패로 무반응,
	//세 문제 모두 가져 — 직접 max(헤더, 각 row) + 아이콘 + padding 계산이 정석.
	if (column < 0 || column >= get_column_count())
		return 0;

	CClientDC dc(this);
	CFont* pOldFont = dc.SelectObject(GetFont());

	//헤더 텍스트 폭
	CString header_text = get_header_text(column);
	CSize sz;
	GetTextExtentPoint32(dc.GetSafeHdc(), header_text, header_text.GetLength(), &sz);
	int header_w = sz.cx;

	//각 row 의 sub item 텍스트 폭 max
	int max_row_w = 0;
	int item_count = GetItemCount();
	for (int row = 0; row < item_count; row++)
	{
		CString text = GetItemText(row, column);
		GetTextExtentPoint32(dc.GetSafeHdc(), text, text.GetLength(), &sz);
		if (sz.cx > max_row_w)
			max_row_w = sz.cx;
	}

	dc.SelectObject(pOldFont);

	//아이콘 폭 — col 0 이고 small image list 있으면 (LVS_EX_SUBITEMIMAGES 확장은 다른 col 도 가능하나 흔치 않음).
	int icon_w = 0;
	if (column == 0)
	{
		CImageList* pImg = GetImageList(LVSIL_SMALL);
		if (pImg)
		{
			IMAGEINFO ii = {0};
			if (pImg->GetImageInfo(0, &ii))
				icon_w = ii.rcImage.right - ii.rcImage.left;
			else
				icon_w = 16;
		}
	}

	//padding — 좌측 + (아이콘 + gap) + 텍스트 + 우측 + safety.
	//safety: GDI GetTextExtentPoint32 측정 vs 실제 그리는 폭 차이 흡수 (한글 trailing).
	const int left_padding  = 6;
	const int icon_gap      = 4;
	const int right_padding = 10;
	const int safety        = 8;

	int row_full    = left_padding + (icon_w > 0 ? icon_w + icon_gap : 0) + max_row_w + right_padding + safety;
	int header_full = left_padding + header_w + right_padding + safety;

	return max(header_full, row_full);
}


//default = LVCFMT_LEFT
void CVtListCtrlEx::set_column_text_align(int column, int format, bool set_to_header)
{
	if (column < 0)
	{
		for (int i = 0; i < get_column_count(); i++)
		{
			m_column_text_align[i] = format;
			if (set_to_header)
				m_HeaderCtrlEx.set_header_text_align(i, format);
		}
	}
	else if (column >= get_column_count())
	{
		return;
	}
	else
	{
		m_column_text_align[column] = format;
		m_HeaderCtrlEx.set_header_text_align(column, format);
	}
}

int	CVtListCtrlEx::get_header_text_align(int column)
{
	return m_HeaderCtrlEx.get_header_text_align(column);
}

void CVtListCtrlEx::set_header_text_align(int column, int format)
{
	if (column < 0)
	{
		for (int i = 0; i < get_column_count(); i++)
			m_HeaderCtrlEx.set_header_text_align(i, format);
	}
	else if (column >= get_column_count())
	{
		return;
	}
	else
	{
		m_HeaderCtrlEx.set_header_text_align(column, format);
	}
}

int	CVtListCtrlEx::get_column_data_type(int column)
{
	if (column >= get_column_count())
	{
		return -1;
	}

	return m_column_data_type[column];
}

void CVtListCtrlEx::set_column_data_type(int column, int nType, bool invalidate)
{
	if (column >= m_column_data_type.size())
		m_column_data_type.resize(column + 1);

	m_column_data_type[column] = nType;

	//data type이 progress type이라면 text align은 무조건 center 정렬방식으로 설정한다.
	if (m_column_data_type[column] == column_data_type_progress)
		m_column_text_align[column] = HDF_CENTER;

	if (invalidate)
		Invalidate();
}

int CVtListCtrlEx::get_header_height()
{
	if (m_HeaderCtrlEx.m_hWnd != NULL)
		return m_HeaderCtrlEx.get_header_height();
	return 0;
}

void CVtListCtrlEx::set_header_height(int height, bool invalidate)
{
	if (m_HeaderCtrlEx.m_hWnd != NULL)
		m_HeaderCtrlEx.set_header_height(height);

	//커스텀 헤더 높이는 listview 가 헤더에 HDM_LAYOUT 을 보낼 때(CHeaderCtrlEx::OnLayout, 보통 WM_SIZE)만
	//실제 적용된다. resize 가 없는 컨텍스트(예: 고정 크기로 임베드된 환경설정 리스트)에서도 즉시 반영되도록
	//listview 크기를 1px 늘렸다 되돌려 *실제* WM_SIZE 두 번을 발생시켜 HDM_LAYOUT 을 강제 재전송한다.
	//(동일 크기 WM_SIZE 는 listview 가 무시할 수 있어 헤더 relayout 이 안 일어난다 — 컬럼 너비 조정 같은
	// 실제 레이아웃 변화에서만 적용되던 증상. set_line_height 의 imagelist 트릭과 동일 취지의 즉시 적용.)
	if (m_hWnd != NULL)
	{
		CRect rc;
		GetWindowRect(rc);
		int w = rc.Width();
		int h = rc.Height();
		SetWindowPos(NULL, 0, 0, w, h + 1, SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);
		SetWindowPos(NULL, 0, 0, w, h,     SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);
	}

	if (invalidate)
		Invalidate();
}

void CVtListCtrlEx::set_line_height(int height, bool invalidate)
{
	m_line_height = height;

	//SetImageList(LVSIL_SMALL) 호출이 listview 의 item height 재측정을 강제 (WM_MEASUREITEM 재발화).
	//own imagelist 를 쓰는 케이스도 동일하게 즉시 적용되도록 prev imagelist 백업 후 더미 set → 직후 복원.
	//Why: m_line_height 만 변경하면 listview 가 cached itemHeight 그대로 사용 → 다음 WM_SIZE 까지 미반영.
	HIMAGELIST hPrev = m_use_own_imagelist
		? ListView_GetImageList(m_hWnd, LVSIL_SMALL)
		: NULL;
	CImageList gapImage;
	gapImage.Create(1, height, ILC_COLORDDB, 1, 0);
	SetImageList(&gapImage, LVSIL_SMALL);
	if (m_use_own_imagelist && hPrev)
		ListView_SetImageList(m_hWnd, hPrev, LVSIL_SMALL);

	if (invalidate)
		Invalidate();
}

void CVtListCtrlEx::set_column_width(int nCol, int cx, bool invalidate)
{
	if (nCol < 0 || nCol >= get_column_count())
		return;

	SetColumnWidth(nCol, cx);

	//컬럼 폭은 가로 scrollbar 필요 여부를 결정하는 single source.
	//CSCScrollbar overlay 가 단독 결정자이므로 폭 변경마다 동기화 필수.
	//(SetColumnWidth 직접 호출은 CVtListCtrlEx 내부에서 이 함수만 사용 — 외부도 동일 정책.)
	sync_scrollbar();

	if (invalidate)
		Invalidate();
}

void CVtListCtrlEx::restore_column_width(CWinApp* pApp, CString sSection, bool invalidate)
{
	for (int i = 0; i < m_HeaderCtrlEx.GetItemCount(); i++)
	{
		int width = pApp->GetProfileInt(sSection + _T("\\column width"), i2S(i), 0);
		if (width > 0)
			set_column_width(i, width, false);   //단일 진입점 경유 — 마지막에 묶어 한 번만 sync.
	}

	sync_scrollbar();

	if (invalidate)
		Invalidate();
}

void CVtListCtrlEx::save_column_width(CWinApp* pApp, CString sSection)
{
	int		i;
	CString str;

	if (!m_HeaderCtrlEx || !m_HeaderCtrlEx.m_hWnd)
		return;

	for (i = 0; i < m_HeaderCtrlEx.GetItemCount(); i++)
	{
		pApp->WriteProfileInt(sSection + _T("\\column width"), i2S(i), GetColumnWidth(i));
	}
}

CRect CVtListCtrlEx::get_item_rect(int item, int subItem)
{
	if (!m_hWnd)
		return CRect();

	// Get the column offset
	int Offset = 0;
	for (int iColumn = 0; iColumn < subItem; iColumn++)
		Offset += GetColumnWidth(iColumn);

	CRect rc;
	CRect Rect;

	GetItemRect(item, &Rect, LVIR_BOUNDS);

	// Now scroll if we need to expose the column
	GetClientRect(&rc);

	if (Offset + Rect.left < 0 || Offset + Rect.left > rc.right)
	{
		CSize Size;
		if (Offset + Rect.left > 0)
			Size.cx = -(Offset - Rect.left);
		else
			Size.cx = Offset - Rect.left;
		Size.cy = 0;
		Scroll(Size);
		Rect.left -= Size.cx;
	}

	Rect.left += Offset;
	Rect.right = Rect.left + GetColumnWidth(subItem);
	//Rect.top += 2;

	if (subItem == 0)
	{
		DWORD dwExStyle = ListView_GetExtendedListViewStyle(GetSafeHwnd());
		DWORD dwExStyle1 = GetExtendedStyle();
		//if (dwExStyle & LVS_EX_CHECKBOXES)
		if (GetExtendedStyle() & LVS_EX_CHECKBOXES)
			Rect.left += 18;

		if (m_is_shell_listctrl)
		{
			Rect.left += 19;	//editbox자체의 left-margin이 있으므로 22가 아닌 19만 더해준다.
			Rect.OffsetRect(0, -1);
		}
	}

	return Rect;
}

//클릭위치에 따라 item은 올바르게 판별되나 subItem은 그렇지 않아서(마우스 이벤트 핸들러 함수에서) 새로 추가함.
int CVtListCtrlEx::hit_test(CPoint pt, int& item, int& subItem, bool include_icon)
{
	CRect rc;
	CRect itemRect;
	
	item = subItem = -1;

	// Now scroll if we need to expose the column
	GetClientRect(&rc);

	if (!rc.PtInRect(pt))
		return LVHT_NOWHERE;

	int first = GetTopIndex();
	int last = MIN(size(), first + GetCountPerPage());

	for (int i = first; i < last; i++)
	{
		//LVIR_BOUNDS로 itemRect를 구하면 맨 끝 컬럼의 우측 클릭 시 범위안에 들어오지 않으므로
		//item도 -1을 리턴한다. itemRect.right는 rc.right로 검사해줘야 subItem은 -1이더라도 item은 알 수 있다.
		GetItemRect(i, &itemRect, LVIR_BOUNDS);
		itemRect.right = rc.right;

		if (itemRect.PtInRect(pt))
		{
			item = i;

			//어느 컬럼인지도 찾는다.
			for (int j = get_column_count() - 1; j >= 0; j--)
			{
				GetSubItemRect(i, j, LVIR_BOUNDS, itemRect);

				//0번 컬럼은 한 라인 전체의 영역을 리턴하므로
				if (j == 0)
				{
					//0번 컬럼의 width로 보정해주고
					itemRect.right = itemRect.left + GetColumnWidth(0);

					if (GetExtendedStyle() & LVS_EX_CHECKBOXES)
					{
						//DrawItem()에서 체크박스를 그려주는데 그 크기는 좌우 4 여백을 두고 크기는 14x14이다.
						if (pt.x > itemRect.left + 4 && pt.x < itemRect.left + 4 + 14)
						{
							subItem = 0;
							return LVHT_ONITEMSTATEICON;
						}

						itemRect.left += 18; //checkbox가 있는 경우는 left를 18만큼 보정해준다.
					}

					//imagelist를 사용한다면 이미지 영역만큼 다시 보정해줘야 한다.
					if (m_use_own_imagelist)
					{
						if (include_icon)
						{
							if (itemRect.PtInRect(pt))
							{
								subItem = 0;
								return LVHT_ONITEMLABEL;
							}
							itemRect.left += 22;
						}
						else
						{
							subItem = 0;
							return LVHT_ONITEMICON;
						}
					}
				}

				if (itemRect.PtInRect(pt))
				{
					subItem = j;
					return LVHT_ONITEMICON;
				}
			}

			return LVHT_ONITEMICON;
		}
	}

	return LVHT_NOWHERE;
}

void CVtListCtrlEx::set_draw_selected_border(bool draw, Gdiplus::Color cr_border, int selected_border_width, int pen_style)
{
	m_draw_selected_border = draw;
	
	if (cr_border.GetValue() != Gdiplus::Color::Transparent)
		m_theme.cr_selected_border = cr_border;

	if (selected_border_width > 0)
		m_selected_border_width = selected_border_width;

	if (pen_style >= 0)
		m_selected_border_style = pen_style;

	Invalidate();
}

void CVtListCtrlEx::allow_sort(bool allow)
{
	m_allow_sort = allow;
	m_HeaderCtrlEx.allow_sort(m_allow_sort);
}

//debug모드에서는 매우 느리다. lambda때문인지 모르겠다.
//0번 컬럼이 아닌 다른 컬럼으로 정렬할 때 두 값이 같으면 0번 컬럼으로 한번 더 검사한다.
void CVtListCtrlEx::sort(int subItem, int ascending)
{
	if (!m_allow_sort)
		return;

	m_cur_sort_column = subItem;

	if (ascending == -1)
	{
		if (m_column_sort_type[subItem] == -1)
			m_column_sort_type[subItem] = sort_ascending;
		else
			m_column_sort_type[subItem] = !m_column_sort_type[subItem];
	}
	else
	{
		m_column_sort_type[subItem] = ascending;
	}


	int iSub = subItem;
	int sort_asc = m_column_sort_type[subItem];
	int data_type = get_column_data_type(subItem);
	bool include_null = false;

	TRACE(_T("iSub = %d, sort_asc = %d, data_type = %d\n"), iSub, sort_asc, data_type);

	setlocale(LC_ALL, NULL);

	//shelllist인 경우는 폴더와 파일을 나눠서 정렬한다.
	if (m_is_shell_listctrl)
	{
		std::sort(m_cur_folders.begin(), m_cur_folders.end(),
			[sort_asc, iSub, data_type, include_null](CVtFileInfo a, CVtFileInfo b)
			{
				if (iSub == col_filename)
				{
					bool res = (StrCmpLogicalW((CStringW)a.data.cFileName, (CStringW)b.data.cFileName) < 0);
					if (!sort_asc)
						res = (StrCmpLogicalW((CStringW)a.data.cFileName, (CStringW)b.data.cFileName) > 0);

					//TRACE(_T("%s vs %s = %d\n"), a.data.cFileName, b.data.cFileName, res);
					return res;
				}
				else if (iSub == col_filesize)
				{
					ULONGLONG size_a = get_file_size(a.data);
					ULONGLONG size_b = get_file_size(b.data);
					//TRACE(_T("%s vs %s = %llu vs %llu\n"), a.data.cFileName, b.data.cFileName, size_a, size_b);

					//파일크기가 같다면 이름순으로 정렬한다.
					if (sort_asc)
					{
						if (size_a == size_b)
							return (StrCmpLogicalW((CStringW)a.data.cFileName, (CStringW)b.data.cFileName) < 0);
						return (size_a < size_b);
					}

					if (size_a == size_b)
						return (StrCmpLogicalW((CStringW)a.data.cFileName, (CStringW)b.data.cFileName) > 0);

					return (size_a > size_b);
				}
				else if (iSub == col_filedate)
				{
					if (sort_asc)
						return (CompareFileTime(&a.data.ftLastWriteTime, &b.data.ftLastWriteTime) < 0);

					return (CompareFileTime(&a.data.ftLastWriteTime, &b.data.ftLastWriteTime) > 0);
				}
				return false;
			});

		std::sort(m_cur_files.begin(), m_cur_files.end(),
			[sort_asc, iSub, data_type](CVtFileInfo a, CVtFileInfo b)
			{
				if (iSub == col_filename)
				{
					bool res = (StrCmpLogicalW((CStringW)a.data.cFileName, (CStringW)b.data.cFileName) < 0);

					if (!sort_asc)
						res = (StrCmpLogicalW((CStringW)a.data.cFileName, (CStringW)b.data.cFileName) > 0);
					
					//TRACE(_T("%s vs %s = %d\n"), a.data.cFileName, b.data.cFileName, res);
					return res;
				}
				else if (iSub == col_filesize)
				{
					ULONGLONG size_a = get_file_size(a.data);
					ULONGLONG size_b = get_file_size(b.data);
					//TRACE(_T("%s vs %s = %llu vs %llu\n"), a.data.cFileName, b.data.cFileName, size_a, size_b);

					//파일크기가 같다면 이름순으로 정렬한다.
					if (sort_asc)
					{
						if (size_a == size_b)
							return (StrCmpLogicalW((CStringW)a.data.cFileName, (CStringW)b.data.cFileName) < 0);
						return (size_a < size_b);
					}

					if (size_a == size_b)
						return (StrCmpLogicalW((CStringW)a.data.cFileName, (CStringW)b.data.cFileName) > 0);

					return (size_a > size_b);
				}
				else if (iSub == col_filedate)
				{
					if (sort_asc)
						return (CompareFileTime(&a.data.ftLastWriteTime, &b.data.ftLastWriteTime) < 0);

					return (CompareFileTime(&a.data.ftLastWriteTime, &b.data.ftLastWriteTime) > 0);
				}
				return false;
			});

		refresh_list(false);
	}
	else
	{
		//20250619 정상 동작하던 정렬 코드였으나 Debug모드에서 갑자기 에러 발생함.
		//0번 컬럼은 정상적으로 정렬되나 1번 컬럼부터는 오류가 발생하는데
		//이 때 m_list_db를 보면 특정 항목의 text가 empty로 변하여 std::sort에서 오류가 발생함.
		//1번 또는 11번 항목이 empty.
		std::sort(m_list_db.begin(), m_list_db.end(),
			[sort_asc, iSub, data_type, include_null](CListCtrlData a, CListCtrlData b)
			{
				if (sort_asc)
				{
					if (data_type == column_data_type_text)
					{
						return (a.text[iSub].CompareNoCase(b.text[iSub]) > 0);
					}
					else if (data_type == column_data_type_text_ip)
					{
						if (iSub != 0)
						{
							if (!include_null)
							{
								return (compare_str(a.text[iSub], b.text[iSub]) > 0);
							}
							else
							{
								return (compare_str(a.text[iSub], b.text[iSub]) > 0);
							}
						}
						else
						{
							return (compare_str(a.text[iSub], b.text[iSub]) > 0);
						}

						return (compare_str(a.text[iSub], b.text[iSub]) > 0);
					}
					else
					{
						a.text[iSub].Remove(',');
						b.text[iSub].Remove(',');
						return (_ttof(a.text[iSub]) < _ttof(b.text[iSub]));
					}
				}
				else
				{
					if (data_type == column_data_type_text)
					{
						return (a.text[iSub].CompareNoCase(b.text[iSub]) < 0);
					}
					else if (data_type == column_data_type_text_ip)
					{
						if (iSub != 0)
						{
							if (!include_null)
							{
								return (compare_str(a.text[iSub], b.text[iSub]) == -1);
							}
							else
							{
								return (compare_str(a.text[iSub], b.text[iSub]) == -1);
							}
						}
						else
						{
							return (compare_str(a.text[iSub], b.text[iSub]) == -1);
						}

						return (compare_str(a.text[iSub], b.text[iSub]) == -1);
					}
					else
					{
						a.text[iSub].Remove(',');
						b.text[iSub].Remove(',');
						return (_ttof(a.text[iSub]) > _ttof(b.text[iSub]));
					}
				}
			}
		);
	}

	m_HeaderCtrlEx.set_sort_arrow(subItem, sort_asc);
	Invalidate();
}

void CVtListCtrlEx::sort_by_text_color(int subItem, int ascending, bool text_sort_on_same_color)
{
	if (!m_allow_sort)
		return;

	if (ascending == -1)
	{
		if (m_column_sort_type[subItem] == -1)
			m_column_sort_type[subItem] = sort_ascending;
		else
			m_column_sort_type[subItem] = !m_column_sort_type[subItem];
	}
	else
	{
		m_column_sort_type[subItem] = ascending;
	}

	int iSub = subItem;
	bool sort_asc = m_column_sort_type[subItem];
	bool text_sort_on_same_color_item = text_sort_on_same_color;

	std::sort(m_list_db.begin(), m_list_db.end(),
		[sort_asc, iSub, text_sort_on_same_color_item](CListCtrlData a, CListCtrlData b)
	{
		if (sort_asc)
		{
			//색상이 같으면 파일명으로 정렬하려 했으나
			//뭔가 원하는대로 표시되지 않고 시간은 꽤 낭비된다.
			//일단 색상만으로 정렬한다.
			//색상이 같을 경우 파일명으로 정렬할 때
			//일반 색상 항목은 제외시키니 원하는대로 동작됨.
			//return (a.crText[iSub] > b.crText[iSub]);
			
			if (a.crText[iSub].GetValue() > b.crText[iSub].GetValue())
				return true;
			else if (a.crText[iSub].GetValue() < b.crText[iSub].GetValue())
				return false;
			//텍스트 색상이 같으면 텍스트로 비교하되 일반 항목은 제외시킨다.
			else if (text_sort_on_same_color_item && (a.crText[iSub].GetValue() != listctrlex_unused_color.GetValue()))
			{
				if (a.text[iSub] < b.text[iSub])
					return true;
				else// if (a.text[iSub] < b.text[iSub])
					return false;
			}
		}
		else
		{
			return (a.crText[iSub].GetValue() < b.crText[iSub].GetValue());
			/*
			if (a.crText[iSub] < b.crText[iSub])
				return true;
			else if (a.crText[iSub] > b.crText[iSub])
				return false;
			else //텍스트 색상이 같으면 텍스트로 비교
			{
				if (a.text[iSub] < b.text[iSub])
					return true;
				else// if (a.text[iSub] > b.text[iSub])
					return false;
			}
			*/
		}
		return false;
		}
	);

	Invalidate();
}

void CVtListCtrlEx::OnLvnColumnclick(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMLISTVIEW pNMLV = reinterpret_cast<LPNMLISTVIEW>(pNMHDR);
	// TODO: Add your control notification handler code here
	//컨트롤 키가 눌려지면 색상값으로 정렬한다.
	if (IsCtrlPressed())
		sort_by_text_color(pNMLV->iSubItem, -1);
	else
		sort(pNMLV->iSubItem, -1);

	*pResult = 0;
}


BOOL CVtListCtrlEx::PreCreateWindow(CREATESTRUCT& cs)
{
	// TODO: 여기에 특수화된 코드를 추가 및/또는 기본 클래스를 호출합니다.
	//AfxMessageBox(_T("sldkf"));
	return CListCtrl::PreCreateWindow(cs);
}

void CVtListCtrlEx::PreSubclassWindow()
{
	// TODO: Add your specialized code here and/or call the base class
	//동적으로 생성할 경우 PreSubclassWindow()함수내에서
	//GetHeaderCtrl()은 항상 NULL을 리턴한다.
	//이를 보완하기 위해 타이머, SendMessage등의 방법을 사용해봤으나 쉽지 않다.
	CListCtrl::PreSubclassWindow();

	modify_style();
	setup_scrollbar();
}

void CVtListCtrlEx::modify_style()
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

	//리소스에서 WS_BORDER / WS_EX_CLIENTEDGE 가 켜져 있으면 native border 제거하고 자체 그리기로 전환.
	//m_scrollbar_setup 시 OnNcCalcSize 가 NC=0 으로 만들기 때문에 native WM_NCPAINT 가 그릴 영역이 없어
	//OS native border 도 우리 self-paint border 도 보이지 않는다 → 자체 1px NC 확보가 필요.
	//(CSCTreeCtrl::PreSubclassWindow 와 동일 패턴.)
	if ((GetStyle() & WS_BORDER) || (GetExStyle() & WS_EX_CLIENTEDGE))
	{
		m_draw_border = true;
		ModifyStyle(WS_BORDER, 0);
		ModifyStyleEx(WS_EX_CLIENTEDGE | WS_EX_STATICEDGE, 0);
		SetWindowPos(NULL, 0, 0, 0, 0,
			SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE | SWP_FRAMECHANGED);
	}

	//헤더컨트롤을 제어할 일이 있는지 확인 필요.
	// if view style is other than LVS_REPORT
	// returned pointer will be NULL
	CHeaderCtrl* pHeader = GetHeaderCtrl();

	if (pHeader)
	{
		// voila!
		m_HeaderCtrlEx.SubclassWindow(pHeader->m_hWnd);
	}

#if 1
	// get original view style
	//LVS_OWNERDATA는 동적으로 설정되지 않는듯하다. resource editor에서 주고 시작하자.
	//DWORD dwStyle = GetStyle() & LVS_TYPEMASK;
	//ModifyStyle(LVS_TYPEMASK, dwStyle | LVS_REPORT | LVS_OWNERDRAWFIXED | LVS_OWNERDATA | LVS_NOSORTHEADER);
	//BOOL b = ModifyStyle(0, dwStyle | LVS_REPORT | LVS_OWNERDRAWFIXED | LVS_OWNERDATA | LVS_NOSORTHEADER);


	//위에서 스타일을 변경하므로 여기서는 제거.
	/*
	LONG lStyleOld = GetWindowLongPtr(GetSafeHwnd(), GWL_STYLE);
	lStyleOld &= ~(LVS_TYPEMASK);
	lStyleOld |= (LVS_REPORT | LVS_OWNERDRAWFIXED | LVS_OWNERDATA);

	SetWindowLongPtr(GetSafeHwnd(), GWL_STYLE, lStyleOld | LVS_NOCOLUMNHEADER | LVS_NOSORTHEADER);
	*/

	//ASSERT(pHeader->m_hWnd != NULL);
#endif

	//위에서 headerCtrl까지 구한 후 font 세팅을 해야 헤더에도 동일하게 적용된다.
	reconstruct_font();
}

void CVtListCtrlEx::move_parent_folder()
{
	if (m_in_editing)
	{
		edit_end(false);
		return;
	}

	if (m_is_shell_listctrl)
	{
		CString path = m_pShellImageList->convert_special_folder_to_real_path(!m_is_local, get_path());
		CString thisPC = m_pShellImageList->m_volume[!m_is_local].get_label(CSIDL_DRIVES);

		if (path == thisPC)
			return;

		//드라이브라면 내 컴퓨터로 back (path = "C:", go to ThisPC)
		if (is_drive_root(path))
		{
			path = thisPC;
		}
		//일반 폴더라면 상위폴더로 이동
		else
		{
			path = get_parent_dir(path);
		}

		if (m_is_local)
			set_path(path);

		//VtListCtrlEx 내부에서 어떤 이벤트에 의해 경로가 변경되는 경우라면 parent에게 이를 알려야한다.
		//set_path에서 메시지 전송을 포함시키면 recursive call이 발생하므로 별도로 호출한다.
		::SendMessage(GetParent()->GetSafeHwnd(), Message_CVtListCtrlEx, (WPARAM) & (CVtListCtrlExMessage(this, message_path_changed, NULL)), (LPARAM)&path);
	}
}

BOOL CVtListCtrlEx::PreTranslateMessage(MSG* pMsg)
{
	// TODO: Add your specialized code here and/or call the base class
	// TODO: 여기에 특수화된 코드를 추가 및/또는 기본 클래스를 호출합니다.

	//편집 중 (m_in_editing) 이고 메시지 target 이 내부 편집기 (m_pEdit) 이면, walk 가 부모 dialog 로
	//계속 가서 메인 단축키 (Q~I 화질조정 / Space 재생 등) 가 발화되는 문제 차단.
	//자체 dispatch 후 TRUE 반환 — edit 는 정상 입력 받고 부모 walk 만 멈춤.
	if (m_in_editing && m_pEdit && m_pEdit->GetSafeHwnd() == pMsg->hwnd &&
		(pMsg->message == WM_KEYDOWN || pMsg->message == WM_KEYUP ||
		 pMsg->message == WM_CHAR || pMsg->message == WM_SYSKEYDOWN ||
		 pMsg->message == WM_SYSKEYUP || pMsg->message == WM_SYSCHAR))
	{
		::TranslateMessage(pMsg);
		::DispatchMessage(pMsg);
		return TRUE;
	}

	//마우스 back button up
	if (pMsg->message == WM_XBUTTONUP)
	{
		move_parent_folder();
	}
	else if (pMsg->message == WM_MOUSEWHEEL && m_in_editing)
	{
		return true;

		/*
		//폴翡중에 스크롤이 되툈E폴翡중인 셀의 사각?E정보를 구해서
		//edit을 그 좌표로 이동시켜줘야 한다. client영역을 벗엉廐툈E화면에서 보여지툈E안된다.
		CRect	r;

		GetSubItemRect(m_nEditItem, m_nEditSubItem, LVIR_BOUNDS, r);
		m_pEdit->MoveWindow(r);
		Invalidate();
		*/
	}
	else if (pMsg->message == WM_KEYDOWN && !m_in_editing)
	{
		if (!IsWindowVisible())
		{
			TRACE(_T("not visible listctrl\n"), pMsg->wParam);
			return FALSE;
		}

		TRACE(_T("VtListCtrl key = %d\n"), pMsg->wParam);
		switch (pMsg->wParam)
		{
			case VK_RETURN:
			{
				if (m_in_editing)
				{
					edit_end(true);
					return TRUE;
				}
				break;
			}
			case VK_ESCAPE:
			{
				if (m_in_editing)
				{
					edit_end(false);
					return TRUE;
				}
				break;
			}
			case VK_SPACE:
			{
				if (!m_use_virtual_list || m_in_editing)
					break;

				if (GetExtendedStyle() & LVS_EX_CHECKBOXES)
				{
					int selected = get_selected_index();
					if (selected < 0 || selected >= size())
						return FALSE;

					m_list_db[selected].checked = !m_list_db[selected].checked;
					TRACE(_T("checkbox toggle by spacebar. m_list_db[%d].checked = %d\n"), selected, m_list_db[selected].checked);
					Invalidate();
					::SendMessage(GetParent()->GetSafeHwnd(), Message_CVtListCtrlEx, (WPARAM) & (CVtListCtrlExMessage(this, message_checked_item, NULL)), selected);
					return TRUE;
				}
				break;
			}
			case VK_BACK:
			{
				if (m_in_editing)
					break;

				//로컬일 경우 Back키에 대해 다음 동작을 수행시키는 것은 간편한 사용이 될 수도 있지만
				//main에서 어떻게 사용하느냐에 따라 방해가 될 수도 있다.
				if (m_is_shell_listctrl)
				{
					if (m_path == m_pShellImageList->m_volume[!m_is_local].get_label(CSIDL_DRIVES))
						return true;

					CString new_path;

					//드라이브면 내 PC로 가고
					if (is_drive_root(m_path))
						new_path = m_pShellImageList->m_volume[!m_is_local].get_label(CSIDL_DRIVES);
					//그렇지 않으면 상위 디렉토리로 이동
					else
						new_path = get_parent_dir(m_path);

					set_path(new_path);
				
					//VtListCtrlEx 내부에서 어떤 이벤트에 의해 경로가 변경되는 경우라면 parent에게 이를 알려야한다.
					//set_path에서 메시지 전송을 포함시키면 recursive call이 발생하므로 별도로 호출한다.
					::SendMessage(GetParent()->GetSafeHwnd(), Message_CVtListCtrlEx, (WPARAM) & (CVtListCtrlExMessage(this, message_path_changed, NULL)), (LPARAM)&new_path);
				}
				
				break;
			}

			//case 220		:	return true;	//'\'키를 누르면 리스트 맨 처음으로 이동되는 현상 방지.
			case 'A'		:	if (!m_in_editing && GetKeyState(VK_CONTROL) & 0x8000)
								{
									select_item(-1);
								}
								break;
			case 'C'		:	if (!m_in_editing && GetKeyState(VK_CONTROL) & 0x8000)
								{
									//CopyToClipboard(_T("|"));
									MessageBeep(MB_ICONINFORMATION);
									return true;
								}
								break;
			case 'V'		:	if (!m_in_editing && GetKeyState(VK_CONTROL) & 0x8000)
								{
									//PasteInsertFromClipboard();
									//MessageBeep(MB_ICONINFORMATION);
									return false;
								}
								break;
			case VK_F2		:	if (m_in_editing)
								{
									return true;
								}
								else
								{
									//현재 경로가 "내 PC"인 경우는 우선 편집을 보류한다.
									if (!m_is_shell_listctrl || get_path() != m_pShellImageList->get_system_label(!m_is_local, CSIDL_DRIVES))
										edit_item(get_selected_index(), m_edit_subItem);
									return true;
								}
								break;
			case VK_F5		:	
								//editing일 경우는 F5키가 CEdit에서 발생하므로 여기 오지 않는다.
								//CEdit을 파생해서 F5 이벤트가 발생하면 편집을 종료시키고 새로고침 해줘야 한다.
								if (m_in_editing)
									edit_end();

								refresh_list();
								return true;
			case VK_HOME:		if (!m_in_editing && (GetKeyState(VK_CONTROL) & 0x8000))
								{
									//set_auto_scroll(true);
									//return true;	//여기서 true를 리턴하지 않으면 CListCtrl의 기본 end 키 처리가 수행되고 현재 화면에서 맨 아래 항목이 선택되어 m_auto_scroll이 다시 false로 변한다.
								}
								return false;
			case VK_END :		if (!m_in_editing && (GetKeyState(VK_CONTROL) & 0x8000))
								{
									set_auto_scroll(true);
									return true;	//여기서 true를 리턴하지 않으면 CListCtrl의 기본 end 키 처리가 수행되고 현재 화면에서 맨 아래 항목이 선택되어 m_auto_scroll이 다시 false로 변한다.
								}
								return false;
			case VK_PRIOR:
								if (m_in_editing)
									return FALSE;

				/*
			//키보드에 의한 항목 삭제 처리는 메인에서 해야 안전하다.
			case VK_DELETE	:	if (m_bInEditing)
									return false;
								else
									DeleteSelectedItems();
				break;
			*/
		}
	}
	return CListCtrl::PreTranslateMessage(pMsg);
}


void CVtListCtrlEx::OnLvnOdstatechanged(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMLVODSTATECHANGE pStateChanged = reinterpret_cast<LPNMLVODSTATECHANGE>(pNMHDR);
	// TODO: Add your control notification handler code here
	*pResult = 0;
}


BOOL CVtListCtrlEx::OnEraseBkgnd(CDC* pDC)
{
	// TODO: Add your message handler code here and/or call default
	return false;
	return CListCtrl::OnEraseBkgnd(pDC);
}


void CVtListCtrlEx::OnPaint()
{
	CPaintDC dc1(this); // device context for painting
					   // TODO: Add your message handler code here
					   // Do not call CListCtrl::OnPaint() for painting messages
	CRect rc;
	CRect rcHeader;
	CHeaderCtrl* pHeaderCtrl = GetHeaderCtrl();
	GetClientRect(rc);

	//header가 있다면 header의 높이만큼 리스트의 시작점을 아래로 보정해준다.
	//그래야만 header도 사용자 지정색으로 그려진다.
	if (pHeaderCtrl)
	{
		pHeaderCtrl->GetClientRect(&rcHeader);
		rc.top += rcHeader.Height();
	}

	if (m_scrollbar_setup)
	{
		CRect rcFull;
		GetClientRect(rcFull);

		CRect rcCornerTop;
		if (pHeaderCtrl && ::IsWindow(pHeaderCtrl->GetSafeHwnd()))
		{
			CRect rh;
			pHeaderCtrl->GetWindowRect(rh);
			ScreenToClient(rh);
			if (rh.right < rcFull.right)
				rcCornerTop = CRect(rh.right, rh.top, rcFull.right, rh.bottom);
		}

		//paintDC 에 직접 [FillSolidRect(erase) → DefWindowProc(WM_PAINT)(draw)] 흐름은
		//컬럼 폭 드래그 같은 partial invalidate 케이스에서 erase 프레임이 그대로 화면에 노출돼
		//변경된 컬럼만 깜빡인다. memory DC 에 모두 그린 뒤 destructor 가 BitBlt 으로 한 번에
		//paintDC 로 옮기면 erase 단계가 보이지 않는다 (비-overlay 분기와 동일한 패턴).
		CMemoryDC mdc(&dc1, &rcFull, true);
		CDC* pDC = &mdc;

		if (!rcCornerTop.IsRectEmpty())
			pDC->FillSolidRect(rcCornerTop, m_theme.cr_header_back.ToCOLORREF());

		if (!rcCornerTop.IsRectEmpty())
			pDC->ExcludeClipRect(rcCornerTop);

		pDC->FillSolidRect(rc, m_theme.cr_back.ToCOLORREF());

		DefWindowProc(WM_PAINT, (WPARAM)pDC->m_hDC, (LPARAM)0);
		return;
	}

	//MemorDC 또한 header만큼 낮춘 크기로 잡히게 되는데 이로 인해 DrawItem()에서
	//progress를 그려줄 때 LPtoDP()를 사용해서 SelectClipRgn()의 좌표를 보정해주는 코드가 추가되었다.
	//(DrawText()는 논리좌표를 사용하고 SelectClipRgn()은 물리좌표를 사용하기 때문에 둘의 좌표가 달라지기 때문)
	CMemoryDC dc(&dc1, &rc, true);
	Gdiplus::Graphics g(dc.m_hDC);

	//CTreeCtrl, CListCtrl의 배경색은
	//dc.FillSolidRect(rc, m_theme.cr_back.ToCOLORREF());
	dc.FillSolidRect(rc, m_theme.cr_back.ToCOLORREF());

	if (m_list_db.size() == 0 && !m_text_on_empty.IsEmpty())
	{
		draw_text(g, rc, m_text_on_empty, m_text_on_empty_size, false, 0, 0, _T("맑은 고딕"), m_text_on_empty_color);
	}

	//CListCtrl::OnPaint();
	DefWindowProc(WM_PAINT, (WPARAM)dc.m_hDC, (LPARAM)0);
}


BOOL CVtListCtrlEx::OnLvnBeginLabelEdit(NMHDR *pNMHDR, LRESULT *pResult)
{
	NMLVDISPINFO *pDispInfo = reinterpret_cast<NMLVDISPINFO*>(pNMHDR);
	// TODO: Add your control notification handler code here
	//*pResult = 0;
	return FALSE;
}


BOOL CVtListCtrlEx::OnLvnEndLabelEdit(NMHDR *pNMHDR, LRESULT *pResult)
{
	//이미 edit_end()에서 필요한 모든 처리를 마친 상태이므로 여기서는 별도 처리없이
	//return FALSE;하여 main dlg에서 LVN_ENDLABELEDIT에 대한 메시지를 처리하도록 하면 된다.
	/*
	LV_DISPINFO *plvDispInfo = (LV_DISPINFO *)pNMHDR;
	LV_ITEM	*plvItem = &plvDispInfo->item;

	if (plvItem->pszText != NULL && plvItem->pszText != m_edit_old_text)
	{
		m_modified = true;

		//멀티 선택된 상태였다면 선택된 항목 모두 해당 컬럼들의 값을 변경해준다.
		//만약 메인에서도 OnLvnEndlabeledit()을 호출해서
		//ini등 다른 파일에도 기록해야 한다면 거기서도 이 처리를 꼭 해줘야한다.
		//단, 마우스로 다른 곳을 클릭하면서 편집을 종료하는 경우
		//여기서 get_selected_items()를 이용하여 구하면 인덱스가 달라지는 문제가 된다.
		//편집이 시작될 때 미리 선택된 항목들의 번호를 기억해 놔야 한다.
		//편집이 완료되면 선택된 항목들을 모두 변경해줄지
		//편집된 셀만 변경해줄지는 장단점이 있다.
		//따라서 Ctrl키를 누르고 편집을 종료하는 경우에는 선택된 항목들을 모두 변경하도록 수정한다.
		if (IsShiftPressed())
		{
			for (int i = 0; i < m_dqSelected_list_for_edit.size(); i++)
				set_text(m_dqSelected_list_for_edit[i], plvItem->iSubItem, plvItem->pszText);
		}
		else
		{
			bool res = true;
			set_text(m_edit_item, m_edit_subItem, plvItem->pszText);

			//local일 경우는 MoveFile()이 실패하면 undo시킨다.
			if (m_is_shell_listctrl && m_is_local)
			{
				//실제 파일명 변경이 성공해야 아이템의 텍스트도 변경한다.
				CString old_name, new_name;
				old_name.Format(_T("%s\\%s"), get_path(), m_edit_old_text);
				new_name.Format(_T("%s\\%s"), get_path(), plvItem->pszText);
				res = MoveFile(old_name, new_name);
				if (!res)
					undo_edit_label();
			}
		}
	}

	m_in_editing = false;

	*pResult = FALSE;
	*/
	return FALSE;
}

void CVtListCtrlEx::allow_edit(bool allow_edit, bool one_click_edit)
{
	m_allow_edit = allow_edit;
	m_allow_edit_column.assign(get_column_count(), m_allow_edit);

	m_allow_one_click_edit = one_click_edit;
}

void CVtListCtrlEx::allow_edit_column(int column, bool allow_edit)
{
	if (m_allow_edit_column.size() <= column)
		m_allow_edit_column.resize(column + 1);

	m_allow_edit_column[column] = allow_edit;

	//한 컬럼이라도 allow_edit이라면 m_allow_edit은 true이어야 한다.
	if (allow_edit)
	{
		m_allow_edit = true;
		m_allow_one_click_edit = true;
	}
}


CSCStaticEdit* CVtListCtrlEx::edit_item(int item, int subItem)
{
	// Make sure that nCol is valid
	CHeaderCtrl* pHeader = (CHeaderCtrl*)GetDlgItem(0);
	int nColumnCount = pHeader->GetItemCount();

	if (item < 0)
		item = get_selected_index();

	//맨 처음 subItem이 설정된 적이 없다면 -1이 넘어오는데
	//그럴 경우는 0번 컬럼의 편집이라고 가정한다.
	if (subItem < 0)
		subItem = 0;

	if (subItem >= nColumnCount || GetColumnWidth(subItem) < 5)
		return NULL;

	if (!m_allow_edit_column[subItem])
		return NULL;
	// The returned pointer should not be saved

	// Make sure that the item is visible
	if (!EnsureVisible(item, false)) 
	{
		//InsertItem()
		if (!EnsureVisible(item, TRUE)) 
			return NULL;
	}

	m_last_clicked_time = 0;


	CRect r = get_item_rect(item, subItem);
	CRect rc;

	//r.OffsetRect(0, 1);

	GetClientRect(rc);

	//컬럼의 가로 정렬을 편집기 가로 정렬로 매핑.
	DWORD halign = DT_LEFT;
	int align = get_column_text_align(subItem);
	if (align == HDF_CENTER)
		halign = DT_CENTER;
	else if (align == HDF_RIGHT)
		halign = DT_RIGHT;

	//세로 오버레이가 보이면 edit 의 우측이 그 영역을 침범하지 않도록 m_scrollbar.get_width() 만큼 줄임.
	int right_limit = rc.right;
	if (::IsWindow(m_scrollbar.GetSafeHwnd()) && m_scrollbar.IsWindowVisible())
		right_limit -= m_scrollbar.get_width();
	if (r.right > right_limit)
		r.right = right_limit;

	m_edit_old_text = GetItemText(item, subItem);

	//20260214 scpark.
	//edit_end()에서 DestroyWindow() 및 delete을 했었으나 타이밍이 맞지 않으면 double delete이 발생하므로
	//edit_end()에서는 우선 hide 시키고 edit_item()에서 DestroyWindow() 및 delete을 하도록 수정한다.
	if (m_pEdit)
	{
		if (m_pEdit->m_hWnd)
			m_pEdit->DestroyWindow();
		delete m_pEdit;
		m_pEdit = NULL;
	}

	m_pEdit = new CSCStaticEdit;
	m_pEdit->Create(0, r, this, IDC_EDIT_CELL);

	m_pEdit->SetFont(&m_font, true);
	m_pEdit->set_text_align(halign);
	m_pEdit->set_line_align(DT_VCENTER);
	//셀 편집기 — 행 높이 = edit 높이 이므로 자체 padding 을 2px 로 줄여 텍스트가 위/아래로 자연스럽게 채워지게.
	//기본 m_padding(4) + border(1) = 5px 씩 inset 이라 작은 행 높이에서 텍스트가 가운데 띄워져 보임.
	//0 으로 두면 선택 하이라이트가 보더에 붙어 일반 edit 컨트롤 외관과 어긋남.
	m_pEdit->set_padding(2);
	m_pEdit->SetWindowText(m_edit_old_text);
	m_pEdit->set_readonly(m_edit_readonly);

	m_pEdit->ShowWindow(SW_SHOW);
	m_pEdit->SetFocus();

	//파일명 편집이면 확장자 앞까지만 선택, 그 외엔 전체 선택.
	CString ext = get_part(m_edit_old_text, fn_ext);
	if ((ext.GetLength() == 3 || ext.GetLength() == 4) && IsAlphaNumeric(ext))
		m_pEdit->set_sel(0, m_edit_old_text.GetLength() - ext.GetLength() - 1);
	else
		m_pEdit->set_sel(0, -1);

	m_in_editing = true;
	m_edit_item = item;
	m_edit_subItem = subItem;
	get_selected_items(&m_dqSelected_list_for_edit);

	//편집 모드 외부 클릭 감지용 마우스 훅 설치 (다른 컨트롤·다이얼로그 빈 영역·NC 어디든) — CSCListBox/CPathCtrl 패턴.
	install_edit_mouse_hook();

	// Send Notification to parent of ListView ctrl
	//기본 지원되는 폴翡 기능을 이퓖E舊갋않컖EEditSubItem이라는 함수를 제작해서 사퓖E薩갋때문에
	//parent에게는 LVN_BEGINLABELEDIT 메시지가 픸E瀕프갋않는다.
	//아래 메시지를 수동으로 픸E徘臼?parent에서 해큱E리스트 컨트롤의 폴翡이 시작되었음을 알린다.
	//parent의 OnLvnBeginlabeleditListCtrl() 메시햨E핸들러에서 필요한 처리가 가능함.
	LV_DISPINFO dispinfo;
	dispinfo.hdr.hwndFrom = m_hWnd;
	dispinfo.hdr.idFrom = GetDlgCtrlID();
	dispinfo.hdr.code = LVN_BEGINLABELEDIT;

	dispinfo.item.mask = LVIF_TEXT;
	dispinfo.item.iItem = item;
	dispinfo.item.iSubItem = subItem;
	//dispinfo.item.pszText = bEscape ? NULL : LPTSTR((LPCTSTR)Text);
	//dispinfo.item.cchTextMax = Text.GetLength();

	GetParent()->SendMessage(WM_NOTIFY, GetDlgCtrlID(), (LPARAM)&dispinfo);

	return m_pEdit;
}

//편집 전의 텍스트로 되돌린다.(편집 레이블이 파일명이고 파일명 변경이 실패한 경우 쓸 수 있다.)
void CVtListCtrlEx::undo_edit_label()
{
	set_text(m_edit_item, m_edit_subItem, m_edit_old_text);
}

LRESULT CVtListCtrlEx::on_message_CSCStaticEdit(WPARAM wParam, LPARAM lParam)
{
	CSCStaticEditMessage* msg = (CSCStaticEditMessage*)wParam;

	if (msg && msg->pThis && !msg->pThis->IsWindowVisible())
		return 0;

	//CSCStaticEdit 은 타이핑마다 message_scstaticedit_text_changed 를 보낸다.
	//여기서 무조건 Invalidate() 하면 키 입력마다 리스트 전체가 repaint 되어 깜빡인다.
	//셀 내용이 실제로 바뀌는 커밋/취소 시에만 리스트를 갱신한다.
	//
	//killfocus 는 *재진입* 안전 차원에서 즉시 edit_end() 하지 않고 PostMessage 로 분리:
	//OnKillFocus → SendMessage(notify_parent) → 여기서 edit_end → ShowWindow(SW_HIDE) 가
	//OnKillFocus 본문이 끝나기 전에 실행되면 mfc 내부 default OnKillFocus 가 stale state 위에서
	//crash (CStatic::OnKillFocus 라인 423 access violation) 한다.
	switch (msg->message)
	{
		case CSCStaticEdit::message_scstaticedit_killfocus:
			PostMessage(WM_LIST_END_EDIT, 1, 0);
			break;
		case CSCStaticEdit::message_scstaticedit_enter:
			edit_end();
			Invalidate();
			break;
		case CSCStaticEdit::message_scstaticedit_escape:
			edit_end(false);
			Invalidate();
			break;
	}

	return 0;
}

//---- 편집 모드 외부 클릭 감지 (CSCListBox/CPathCtrl 동일 패턴) -----------------------------------------

void CVtListCtrlEx::install_edit_mouse_hook()
{
	if (m_mouse_hook != NULL)
		return;

	//WH_MOUSE 는 자기 스레드 메시지 큐만 감시 → 다른 앱 클릭은 보지 않는다(그쪽은 killfocus 가 처리).
	//이 훅의 목적은 "같은 스레드(같은 dlg) 의 edit 바깥 클릭" 감지뿐이다.
	s_editing_list = this;
	m_mouse_hook = ::SetWindowsHookEx(WH_MOUSE, edit_mouse_hook_proc, NULL, ::GetCurrentThreadId());
}

void CVtListCtrlEx::remove_edit_mouse_hook()
{
	if (m_mouse_hook != NULL)
	{
		::UnhookWindowsHookEx(m_mouse_hook);
		m_mouse_hook = NULL;
	}
	if (s_editing_list == this)
		s_editing_list = NULL;
}

LRESULT CALLBACK CVtListCtrlEx::edit_mouse_hook_proc(int code, WPARAM wParam, LPARAM lParam)
{
	if (code == HC_ACTION && s_editing_list != NULL && s_editing_list->m_pEdit != NULL)
	{
		if (wParam == WM_LBUTTONDOWN || wParam == WM_RBUTTONDOWN || wParam == WM_MBUTTONDOWN ||
			wParam == WM_NCLBUTTONDOWN || wParam == WM_NCRBUTTONDOWN || wParam == WM_NCMBUTTONDOWN)
		{
			MOUSEHOOKSTRUCT* mh = (MOUSEHOOKSTRUCT*)lParam;
			CRect rc_edit;
			s_editing_list->m_pEdit->GetWindowRect(&rc_edit);

			//edit 내부 클릭은 캐럿 이동이므로 종료 금지. 바깥이면 종료를 post (클릭 자체는 그대로 통과시킴).
			if (!rc_edit.PtInRect(mh->pt))
				s_editing_list->PostMessage(WM_LIST_END_EDIT, 0, 0);
		}
	}

	return ::CallNextHookEx(NULL, code, wParam, lParam);
}

LRESULT CVtListCtrlEx::on_end_edit_posted(WPARAM wParam, LPARAM lParam)
{
	edit_end(true);
	return 0;
}

void CVtListCtrlEx::set_color_theme(int theme, bool invalidate)
{
	m_theme.set_color_theme(theme);
	m_HeaderCtrlEx.set_color(m_theme.cr_header_text, m_theme.cr_header_back);

	//invalidate 를 그대로 전파 — 호출자가 true 로 호출하면 scrollbar 도 즉시 redraw 되어야 한다.
	//(이전엔 false 하드코딩이라 색만 바뀌고 화면 갱신은 안 됐다.)
	if (::IsWindow(m_scrollbar.m_hWnd))
		m_scrollbar.set_color_theme(m_theme, invalidate);

	if (::IsWindow(m_scrollbar_h.m_hWnd))
		m_scrollbar_h.set_color_theme(m_theme, invalidate);

	if (m_scrollbar_setup && ::IsWindow(m_hWnd))
	{
		SetBkColor(m_theme.cr_back.ToCOLORREF());
		SetTextBkColor(m_theme.cr_back.ToCOLORREF());
	}

	//테두리(non-client)까지 갱신 — Invalidate() 만으로는 OnNcPaint 가 안 불려 테두리에 이전 테마 색 잔상.
	if (m_hWnd && invalidate)
		RedrawWindow(NULL, NULL, RDW_INVALIDATE | RDW_FRAME);
}

void CVtListCtrlEx::set_color_theme(const CSCColorTheme& theme, bool invalidate)
{
	m_theme.copy_colors_from(theme);
	m_HeaderCtrlEx.set_color(m_theme.cr_header_text, m_theme.cr_header_back);

	//invalidate 를 그대로 전파 — 호출자가 true 로 호출하면 scrollbar 도 즉시 redraw 되어야 한다.
	//(이전엔 false 하드코딩이라 색만 바뀌고 화면 갱신은 안 됐다.)
	if (::IsWindow(m_scrollbar.m_hWnd))
		m_scrollbar.set_color_theme(m_theme, invalidate);

	if (::IsWindow(m_scrollbar_h.m_hWnd))
		m_scrollbar_h.set_color_theme(m_theme, invalidate);

	if (m_scrollbar_setup && ::IsWindow(m_hWnd))
	{
		SetBkColor(m_theme.cr_back.ToCOLORREF());
		SetTextBkColor(m_theme.cr_back.ToCOLORREF());
	}

	//테두리(non-client)까지 갱신 — Invalidate() 만으로는 OnNcPaint 가 안 불려 테두리에 이전 테마 색 잔상.
	if (m_hWnd && invalidate)
		RedrawWindow(NULL, NULL, RDW_INVALIDATE | RDW_FRAME);
}

Gdiplus::Color CVtListCtrlEx::get_text_color(int item, int subItem)
{
	//item이 -1이면 해당 셀의 색상이 아니라 기본 텍스트 색상값을 원하는 것이다.
	if (item < 0)
		return m_theme.cr_text;

	Gdiplus::Color cr = m_list_db[item].crText[subItem];
	if (cr.GetValue() == listctrlex_unused_color.GetValue())
		return m_theme.cr_text;
	return cr;
}

Gdiplus::Color CVtListCtrlEx::get_back_color(int item, int subItem)
{
	Gdiplus::Color cr = m_list_db[item].crBack[subItem];
	if (cr.GetValue() == listctrlex_unused_color.GetValue())
		return m_theme.cr_back;
	return cr;
}

void CVtListCtrlEx::set_back_alternate_color(bool use, Gdiplus::Color cr)
{
	m_use_alternate_back_color = use;

	if (cr.GetValue() != Gdiplus::Color::Transparent)
		m_theme.cr_back_alternate = cr;
}

void CVtListCtrlEx::set_text_color(int item, int subItem, Gdiplus::Color crText, bool erase, bool invalidate)
{
	if (!m_use_virtual_list)
		return;

	int i, j;

	if (item < 0)
	{
		if (subItem < 0)
		{
			for (i = 0; i < m_list_db.size(); i++)
			{
				for (j = 0; j < get_column_count(); j++)
				{
					if (erase)
						m_list_db[i].crText[j] = listctrlex_unused_color;
					else
						m_list_db[i].crText[j] = crText;
				}
			}
		}
		else
		{
			for (i = 0; i < m_list_db.size(); i++)
			{
				if (erase)
					m_list_db[i].crText[subItem] = listctrlex_unused_color;
				else
					m_list_db[i].crText[subItem] = crText;
			}
		}
	}
	else
	{
		if (subItem < 0)
		{
			for (j = 0; j < get_column_count(); j++)
			{
				if (erase)
					m_list_db[item].crText[j] = listctrlex_unused_color;
				else
					m_list_db[item].crText[j] = crText;
			}
		}
		else
		{
			if (erase)
				m_list_db[item].crText[subItem] = listctrlex_unused_color;
			else
				m_list_db[item].crText[subItem] = crText;
		}
	}

	if (invalidate && !m_in_bulk_insert)
		Invalidate();
}

void CVtListCtrlEx::set_back_color(int item, int subItem, Gdiplus::Color crBack, bool erase, bool invalidate)
{
	int i, j;

	if (item < 0)
	{
		if (subItem < 0)
		{
			for (i = 0; i < m_list_db.size(); i++)
			{
				for (j = 0; j < get_column_count(); j++)
				{
					if (erase)
						m_list_db[i].crBack[j] = listctrlex_unused_color;
					else
						m_list_db[i].crBack[j] = crBack;
				}
			}
		}
		else
		{
			for (i = 0; i < m_list_db.size(); i++)
			{
				if (erase)
					m_list_db[i].crBack[subItem] = listctrlex_unused_color;
				else
					m_list_db[i].crBack[subItem] = crBack;
			}
		}
	}
	else
	{
		if (subItem < 0)
		{
			for (j = 0; j < get_column_count(); j++)
			{
				if (erase)
					m_list_db[item].crBack[j] = listctrlex_unused_color;
				else
					m_list_db[item].crBack[j] = crBack;
			}
		}
		else
		{
			if (erase)
				m_list_db[item].crBack[subItem] = listctrlex_unused_color;
			else
				m_list_db[item].crBack[subItem] = crBack;
		}
	}

	if (invalidate)
		Invalidate();
}

void CVtListCtrlEx::set_item_color(int item, int subItem, Gdiplus::Color crText, Gdiplus::Color crBack)
{
	int i, j;

	if (item < 0)
	{
		if (subItem < 0)
		{
			for (i = 0; i < m_list_db.size(); i++)
			{
				for (j = 0; j < get_column_count(); j++)
				{
					m_list_db[i].crText[j] = crText;
					m_list_db[i].crBack[j] = crBack;
				}
			}
		}
		else
		{
			for (i = 0; i < m_list_db.size(); i++)
			{
				m_list_db[i].crText[subItem] = crText;
				m_list_db[i].crBack[subItem] = crBack;
			}
		}
	}
	else
	{
		if (subItem < 0)
		{
			for (j = 0; j < get_column_count(); j++)
			{
				m_list_db[item].crText[j] = crText;
				m_list_db[item].crBack[j] = crBack;
			}
		}
		else
		{
			m_list_db[item].crText[subItem] = crText;
			m_list_db[item].crBack[subItem] = crBack;
		}
	}

	Invalidate();
}

template <typename ValueT, typename DequeT>
static void _apply_style_field(std::deque<CListCtrlData>& db, int item, int subItem, int col_count,
	DequeT CListCtrlData::* member, ValueT value)
{
	auto set = [&](int i, int j)
	{
		auto& v = db[i].*member;
		if ((int)v.size() <= j)
			v.resize(j + 1);
		v[j] = (typename DequeT::value_type)value;
	};
	if (item < 0)
	{
		for (int i = 0; i < (int)db.size(); i++)
		{
			if (subItem < 0)
				for (int j = 0; j < col_count; j++) set(i, j);
			else
				set(i, subItem);
		}
	}
	else
	{
		if (item >= (int)db.size())
			return;
		if (subItem < 0)
			for (int j = 0; j < col_count; j++) set(item, j);
		else
			set(item, subItem);
	}
}

void CVtListCtrlEx::set_text_weight(int item, int subItem, int weight, bool invalidate)
{
	_apply_style_field<int>(m_list_db, item, subItem, get_column_count(), &CListCtrlData::weight, weight);
	if (invalidate)
		Invalidate();
}

void CVtListCtrlEx::set_text_italic(int item, int subItem, bool italic, bool invalidate)
{
	_apply_style_field<BYTE>(m_list_db, item, subItem, get_column_count(), &CListCtrlData::italic, italic ? 1 : 0);
	if (invalidate)
		Invalidate();
}

void CVtListCtrlEx::set_text_underline(int item, int subItem, bool underline, bool invalidate)
{
	_apply_style_field<BYTE>(m_list_db, item, subItem, get_column_count(), &CListCtrlData::underline, underline ? 1 : 0);
	if (invalidate)
		Invalidate();
}

void CVtListCtrlEx::set_text_strikeout(int item, int subItem, bool strikeout, bool invalidate)
{
	_apply_style_field<BYTE>(m_list_db, item, subItem, get_column_count(), &CListCtrlData::strikeout, strikeout ? 1 : 0);
	if (invalidate)
		Invalidate();
}

void CVtListCtrlEx::set_text_style(int item, int subItem, int weight, bool italic, bool underline, bool strikeout, bool invalidate)
{
	int col = get_column_count();
	_apply_style_field<int> (m_list_db, item, subItem, col, &CListCtrlData::weight,    weight);
	_apply_style_field<BYTE>(m_list_db, item, subItem, col, &CListCtrlData::italic,    italic    ? 1 : 0);
	_apply_style_field<BYTE>(m_list_db, item, subItem, col, &CListCtrlData::underline, underline ? 1 : 0);
	_apply_style_field<BYTE>(m_list_db, item, subItem, col, &CListCtrlData::strikeout, strikeout ? 1 : 0);
	if (invalidate && !m_in_bulk_insert)
		Invalidate();
}

int CVtListCtrlEx::get_text_weight(int item, int subItem) const
{
	if (item < 0 || item >= (int)m_list_db.size())
		return 0;
	return _safe_weight(m_list_db[item].weight, subItem);
}

bool CVtListCtrlEx::get_text_italic(int item, int subItem) const
{
	if (item < 0 || item >= (int)m_list_db.size())
		return false;
	return _safe_bytestyle(m_list_db[item].italic, subItem) != 0;
}

bool CVtListCtrlEx::get_text_underline(int item, int subItem) const
{
	if (item < 0 || item >= (int)m_list_db.size())
		return false;
	return _safe_bytestyle(m_list_db[item].underline, subItem) != 0;
}

bool CVtListCtrlEx::get_text_strikeout(int item, int subItem) const
{
	if (item < 0 || item >= (int)m_list_db.size())
		return false;
	return _safe_bytestyle(m_list_db[item].strikeout, subItem) != 0;
}

void CVtListCtrlEx::clear_font_style_cache()
{
	for (auto& kv : m_font_style_cache)
		delete kv.second;
	m_font_style_cache.clear();
}

CFont* CVtListCtrlEx::get_styled_font(int weight, bool italic, bool underline, bool strikeout)
{
	//key — weight 16 bit + italic/underline/strikeout 각 1 bit.
	DWORD key = ((DWORD)(weight & 0xFFFF) << 16) | (italic ? 1 : 0) | (underline ? 2 : 0) | (strikeout ? 4 : 0);

	auto it = m_font_style_cache.find(key);
	if (it != m_font_style_cache.end())
		return it->second;

	//base font (m_font) 의 LOGFONT 복사 + style 필드 변경 + CreateFontIndirect.
	LOGFONT lf = m_lf;
	if (weight > 0)
		lf.lfWeight = weight;
	lf.lfItalic    = italic    ? 1 : 0;
	lf.lfUnderline = underline ? 1 : 0;
	lf.lfStrikeOut = strikeout ? 1 : 0;

	CFont* pFont = new CFont;
	if (!pFont->CreateFontIndirect(&lf))
	{
		delete pFont;
		return nullptr;
	}
	m_font_style_cache[key] = pFont;
	return pFont;
}

void CVtListCtrlEx::set_progress_color(Gdiplus::Color crProgress)
{
	m_theme.cr_progress = crProgress;
	Invalidate();
}

int CVtListCtrlEx::add_item(CString text, int image_index, bool ensureVisible, bool invalidate)
{
	return insert_item(-1, text, image_index, ensureVisible, invalidate);
}

int CVtListCtrlEx::add_item(std::deque<CString> dqText, int image_index, bool ensureVisible, bool invalidate)
{
	int last_index = 0;

	for (int i = 0; i < dqText.size(); i++)
	{
		if (dqText[i].IsEmpty())
			continue;

		last_index = insert_item(-1, dqText[i], image_index, ensureVisible, invalidate);
	}

	return last_index;
}

int CVtListCtrlEx::insert_line(int index, CString line_string, CString separator, int image_index, bool ensureVisible, bool invalidate)
{
	std::deque<CString> dq;
	get_token_str(line_string, dq, separator);
	return insert_item(index, dq, image_index, ensureVisible, invalidate);
}

//index 위치에 0번 컬럼이 text인 라인을 추가한다.(-1이면 맨 마지막에 추가)
int CVtListCtrlEx::insert_item(int index, CString text, int image_index, bool ensureVisible, bool invalidate)
{
	if (get_column_count() <= 0)
	{
		TRACE(_T("column count is 0. use set_headings(...) first.\n"));
		return -1;
	}

	if (index < 0)
		index = size();

	/*
	//이미 해당 index의 row가 존재하는 경우는 값을 변경한다. => 절대 있어서는 안되는 코드임. 왜 이 기능을 구현했는지 확인 필요!
	if (index < m_list_db.size())
	{
		set_text(index, 0, text, false);
	}
	else
	*/

	if (image_index < 0 && m_pShellImageList)
	{
		image_index = m_pShellImageList->GetSystemImageListIcon(!m_is_local, text, false);
	}

	if (m_use_virtual_list)
	{
		m_list_db.insert(m_list_db.begin() + index, CListCtrlData(text, image_index, m_HeaderCtrlEx.GetItemCount()));

		//LVSICF_NOSCROLL 옵션을 주지 않으면 특정 항목 선택 후 해당 항목이 보이지 않도록 스크롤하려 해도
		//데이터가 계속 추가되는 상황에서는 선택된 항목이 보이지 않는 영역으로의 스크롤이 되지 않는 현상이 있다.
		//bulk 모드(대량 populate)면 매 항목 SetItemCountEx 를 생략하고 end_bulk_insert(또는 display_filelist 루프
		//종료)가 한 번에 갱신 — 수천 항목에서 N 회 호출 비용 제거.
		if (!m_in_bulk_insert)
			SetItemCountEx(m_list_db.size(), LVSICF_NOSCROLL);
	}
	else
	{
		LV_ITEM item;
		item.mask = LVIF_TEXT | LVIF_IMAGE | LVIF_STATE;
		item.iItem = index;
		item.iSubItem = 0;
		item.pszText = (LPTSTR)(LPCTSTR)text;
		item.iImage = image_index;
		item.state = 0;
		item.stateMask = 0;
		item.mask |= LVIF_STATE;
		index = InsertItem(&item);
	}

	//bulk_insert 모드면 ensure/Invalidate/sync 모두 skip — end_bulk_insert 가 한 번에 처리.
	if (!m_in_bulk_insert)
	{
		//ensureVisible이면 Invalidate()을 생략해도 된다.
		//TRACE(_T("m_auto_scroll = %d, ensureVisible = %d\n"), m_auto_scroll, ensureVisible);
		if (m_auto_scroll && ensureVisible)
		{
			ensure_visible(index, visible_last);
		}
		else if (invalidate)
		{
			Invalidate();
		}

		sync_scrollbar();
	}

	return index;
}


int CVtListCtrlEx::insert_item(int index, std::deque<CString> dqText, int image_index, bool ensureVisible, bool invalidate)
{
	if (dqText.size() == 0)
		return -1;

	int count = MIN(get_column_count(), dqText.size());
	
	index = insert_item(index, dqText[0], image_index, ensureVisible, invalidate);

	for (int i = 1; i < count; i++)
		set_text(index, i, dqText[i]);

	return index;
}

int CVtListCtrlEx::insert_item(int index, WIN32_FIND_DATA data, bool ensureVisible, bool invalidate)
{
	int img_idx;
	CString filename = get_part(data.cFileName, fn_name);

	if (data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
		img_idx = m_pShellImageList->GetSystemImageListIcon(!m_is_local, _T("C:\\Windows"), true);
	else
		img_idx = m_pShellImageList->GetSystemImageListIcon(!m_is_local, data.cFileName, false);

	//기존에 존재하는 파일이라면 크기, 수정한 날짜를 갱신해주고
	//없다면 리스트에 추가한다.
	int old_index = find(filename, 0, true);
	if (old_index < 0)
	{
		index = insert_item(index, filename, img_idx, ensureVisible, invalidate);
		if (data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
		{
			m_list_db[index].set_text(col_filesize, _T(""));
		}
		else
		{
			m_list_db[index].set_text(col_filesize, get_file_size_str(data));
		}

		m_list_db[index].set_text(col_filedate, get_file_time_str(data.ftLastWriteTime));
		Invalidate();
	}
	else
	{
		index = update_item(old_index, data);
	}

	return index;
}

int CVtListCtrlEx::update_item(int index, WIN32_FIND_DATA data, bool ensureVisible, bool invalidate)
{
	if (index < 0 || index >= size())
		return -1;

	CString filename = get_part(data.cFileName, fn_name);
	m_list_db[index].set_text(col_filename, filename);

	if (data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
	{
		m_list_db[index].set_text(col_filesize, _T(""));
	}
	else
	{
		m_list_db[index].set_text(col_filesize, get_file_size_str(data));
	}

	m_list_db[index].set_text(col_filedate, get_file_time_str(data.ftLastWriteTime));

	return index;
}

int CVtListCtrlEx::insert_folder(int index, CString new_folder_name, bool is_remote)
{
	int img_idx = m_pShellImageList->GetSystemImageListIcon(!m_is_local, _T("c:\\windows"), true);
	index = insert_item(index, new_folder_name, img_idx, false, false);

	set_text(index, col_filesize, _T(""));
	set_text(index, col_filedate, get_cur_datetime_str(2, true, _T(" "), false, false));
	set_text_color(index, col_filedate, listctrlex_weak_color);

	CVtFileInfo fi;
	WIN32_FIND_DATA data;

	memset(&data, 0, sizeof(data));
	_tcscpy_s(fi.data.cFileName, _countof(fi.data.cFileName), concat_path(m_path, new_folder_name));
	fi.data.dwFileAttributes = FILE_ATTRIBUTE_DIRECTORY;
	fi.is_remote = is_remote;
	m_cur_folders.push_back(fi);

	return index;
}

//현재 폴더에 새 폴더를 생성하고 편집모드로 표시한다.
CString CVtListCtrlEx::new_folder(CString &new_folder_title)
{
	CString folder;

	if (!m_is_shell_listctrl)
		return _T("");

	int index;
	
	folder = m_pShellImageList->convert_special_folder_to_real_path(!m_is_local, m_path);

	if (m_is_local)
	{
		index = get_file_index(folder, new_folder_title);
	}
	else
	{
		::SendMessage(GetParent()->GetSafeHwnd(),
			Message_CVtListCtrlEx,
			(WPARAM) & (CVtListCtrlExMessage(this, message_request_new_folder_index, NULL, folder, new_folder_title)),
			(LPARAM)&index);
	}

	folder = m_path;
	if (is_drive_root(folder))
		truncate(folder, 1);

	if (index == 1)
		folder.AppendFormat(_T("\\%s"), new_folder_title.GetString());
	else
		folder.AppendFormat(_T("\\%s (%d)"), new_folder_title.GetString(), index);

	//실제 폴더를 생성한 후 리스트에 목록을 추가한다.
	BOOL res;
	
	if (m_is_local)
	{
		res = CreateDirectory(folder, NULL);
	}
	else
	{
		::SendMessage(GetParent()->GetSafeHwnd(),
			Message_CVtListCtrlEx,
			(WPARAM) & (CVtListCtrlExMessage(this, message_request_new_folder, NULL, folder)),
			(LPARAM)&res);
	}

	if (!res)
	{
		TRACE(_T("fail to create folder : %s. error = %d"), folder, GetLastError());
		return _T("");
	}

	folder = get_part(folder, fn_name);
	index = insert_folder(-1, folder, false);

	if (index < 0)
		return _T("");

	select_item(index, true, true, true);
	edit_item(index, 0);

	return folder;
}

//현재 폴더에서 "새 폴더" 생성 시 인덱스를 구한다. ex. "새 폴더 (2)"
int CVtListCtrlEx::get_file_index(CString path, CString new_folder_title)
{
	int i;
	int max_index = -1;
	std::set<int> idx_set;

	CString folder;

	for (i = 0; i < size(); i++)
	{
		if (!get_text(i, col_filesize).IsEmpty())
			continue;

		folder = get_text(i, col_filename);
		folder.Replace(path + _T("\\"), _T(""));

		if (folder == new_folder_title)
			idx_set.insert(1);

		//끝 ')'를 찾고
		int start_paren = -1;
		int end_paren = folder.ReverseFind(')');
		int found_index = -1;

		if (end_paren > 0)
		{
			//시작 '('를 찾아서 그 사이의 숫자를 추출
			folder = folder.Left(end_paren);
			start_paren = folder.ReverseFind('(');

			if (start_paren > 0)
			{
				folder = folder.Mid(start_paren + 1);
				found_index = _ttoi(folder);
			}
		}

		if (found_index > 0)
			idx_set.insert(found_index);
	}

	//set 항목 중 비어있는 인덱스를 리턴해준다.
	int index = 0;
	bool found = false;

	for (int elem : idx_set)
	{
		index++;
		if (elem != index)
		{
			found = true;
			break;
		}
	}

	//만약 1 ~ n까지 모든 순번이 순차적으로 들어있다면 1 증가된 값을 리턴해주면 된다.
	if (!found)
		index++;

	return index;
}

/*
int CVtListCtrlEx::insert_item(int index, LPCTSTR pszText, ...)
{
	if (index < 0)
		index = size();

	index = insert_item(index, pszText);

	if (get_column_count() == 1)
		return index;

	int subItem = 1;

	if (_tcslen(pszText) > 0)
	{
		va_list list;
		va_start(list, pszText);

		pszText = va_arg(list, LPCTSTR);

		while (pszText)
		{
			//CListCtrl::SetItem(index, nSubItem++, LVIF_TEXT, pszText, 0, 0, 0, 0);
			set_text(index, subItem++, pszText);

			if (subItem >= get_column_count())
				break;

			pszText = va_arg(list, LPCTSTR);
		}

		va_end(list);
	}

	return index;
}
*/

int CVtListCtrlEx::size()
{
	return (m_use_virtual_list ? m_list_db.size() : GetItemCount());
}

//지우기 전 확인창은 호출루틴에서 처리해야 함
//shell일 경우는 실제 폴더 또는 파일을 삭제한 후에 목록에서도 지워야한다.
//local 파일이라면 여기서 처리해도 관계없으나 remote의 파일일수도 있으니 메인에서 처리하자.
//실제 파일을 삭제하는 코드는 메인에서 처리한 후 삭제가 성공하면 리스트에서도 지운다.
void CVtListCtrlEx::delete_selected_items()
{
	int i;
	int index;
	std::deque<int> dqSelected;
	get_selected_items(&dqSelected);

	//뒤에서부터 지움에 주목.
	for (i = dqSelected.size() - 1; i >= 0; i--)
	{
		index = dqSelected[i];

		bool deleted = true;

		if (m_is_shell_listctrl && m_is_local)
		{
			CString file;
			file.Format(_T("%s\\%s"), m_path, get_text(index, col_filename));
			if (!delete_file(file, true))
			{
				get_error_str(true);
				break;
			}
		}

		CListCtrl::DeleteItem(index);
		m_list_db.erase(m_list_db.begin() + index);
	}

	SetItemCount(m_list_db.size());
	sync_scrollbar();
}

//shell_listctrl이라면 파일 또한 삭제할 지 파라미터를 줄 수 있고 실제 파일을 지운 후에 리스트에서도 삭제해야 한다.
bool CVtListCtrlEx::delete_item(int index, bool delete_physical_file)
{
	bool res = true;

	if (index < 0 || index >= m_list_db.size())
		return false;

	if (m_is_shell_listctrl && m_is_local && delete_physical_file)
	{
		CString file = get_path(index);
		if (PathFileExists(file) == false)
			return false;

		res = delete_file(file, true);
	}

	if (res)
	{
		CListCtrl::DeleteItem(index);
		m_list_db.erase(m_list_db.begin() + index);
		SetItemCount(m_list_db.size());
		sync_scrollbar();
	}

	return res;
}

bool CVtListCtrlEx::delete_item(CString label)
{
	//shell_listctrl일 경우 fullpath라면 파일명 또는 폴더명만 추출한다.
	if (m_is_shell_listctrl)
	{
		CString filename = get_part(label, fn_name);

		std::deque<CListCtrlData>::iterator it_data;
		it_data = std::find_if(m_list_db.begin(), m_list_db.end(),
			[filename](CListCtrlData data)
			{
				return (data.text[0].Compare(filename) == 0);
			});

		int index = std::distance(m_list_db.begin(), it_data);

		//리스트에서 지우고
		delete_item(index);

		//파일, 폴더 목록에서도 지워준다.
		std::deque<CVtFileInfo>::iterator it;
		it = std::find_if(m_cur_folders.begin(), m_cur_folders.end(),
			[label](CVtFileInfo fi)
			{
				return (label.CompareNoCase(fi.data.cFileName) == 0);
			});

		if (it != m_cur_folders.end())
		{
			m_cur_folders.erase(it);
			return true;
		}

		it = std::find_if(m_cur_files.begin(), m_cur_files.end(),
			[label](CVtFileInfo fi)
			{
				return (label.CompareNoCase(fi.data.cFileName) == 0);
			});

		if (it != m_cur_files.end())
		{
			m_cur_files.erase(it);
			return true;
		}

	}

	return true;
}

void CVtListCtrlEx::delete_empty_lines()
{
	CString line_text;

	for (int i = size() - 1; i >= 0; i--)
	{
		line_text = get_line_text(i, 0, -1, _T(""));
		if (line_text.IsEmpty())
			delete_item(i);
	}
}

void CVtListCtrlEx::delete_all_items(bool delete_file_list)
{
	SetRedraw(FALSE);

	CListCtrl::DeleteAllItems();
	m_list_db.clear();
	SetItemCount(0);

	if (delete_file_list)
	{
		m_cur_files.clear();
		m_cur_folders.clear();
	}

	SetRedraw(TRUE);
	sync_scrollbar();
}

CString CVtListCtrlEx::get_text(int item, int subItem)
{
	if (item < 0 || item >= size() || subItem < 0 || subItem >= get_column_count())
		return _T("");

	if (m_use_virtual_list)
		return m_list_db[item].text[subItem];

	return GetItemText(item, subItem);
}

void CVtListCtrlEx::set_text(int item, int subItem, CString text, bool invalidate)
{
	if (item < 0 || item >= size())
		return;

	if (!m_hWnd || m_hWnd == INVALID_HANDLE_VALUE)
		return;

	if (m_is_shell_listctrl && !m_pShellImageList)
		return;

	if (m_use_virtual_list)
	{
		//양방향 파일전송2에서 전송 중 취소를 누르면 간혹 m_list_db의 size가 매우 큰 수로 나오는 경우가 있다.
		//원인을 파악중이며 우선 임시로 처리한다.
		//20250822. dispatch info 등 민감한 핸들러에서 뭔가 잘못 처리되어
		//m_list_db의 메모리 영역을 침범한 듯 하다.
		//현재로서는 문제없으므로 아래 제한을 두었던 코드는 주석처리함.
		//if (m_list_db.size() > 10000)
		//	return;

		m_list_db[item].text[subItem] = text;
	}
	else
	{
		SetItemText(item, subItem, text);
	}

	if (invalidate && !m_in_bulk_insert)
		InvalidateRect(get_item_rect(item, subItem), false);
}

//한 라인의 각 컬럼값은 separator로 구분되어 있는 여러 라인 데이터로 index위치부터 리스트를 채운다.
//reset = true이면 모든 데이터를 삭제한 후 새로 채운다. 이 때 index는 0으로 리셋된다.
void CVtListCtrlEx::set_text(int index, CString text, CString separator, bool reset)
{
	if (reset)
	{
		delete_all_items();
		index = 0;
	}

	std::deque<CString> dqlines;
	get_token_str(text, dqlines, _T("\n"));

	for (int i = 0; i < dqlines.size(); i++)
	{
		insert_line(index + i, dqlines[i], separator, -1, false, false);
	}
}

CString CVtListCtrlEx::get_line_text(int index, int from, int to, CString sep)
{
	int		i;
	CString sText = _T("");

	if (index < 0 || index >= size())
		return _T("");

	if (to == -1 || to >= get_column_count())
		to = get_column_count() - 1;

	std::deque<int> dqColumn;
	for (i = from; i <= to; i++)
		dqColumn.push_back(i);

	return get_line_text(index, &dqColumn, sep);
}

CString CVtListCtrlEx::get_line_text(int index, std::deque<int>* dqColumn, CString sep)
{
	int i;
	CString linetext = _T("");

	for (i = 0; i < dqColumn->size(); i++)
		linetext = linetext + get_text(index, dqColumn->at(i)) + (i == dqColumn->size() - 1 ? _T("") : sep);

	return linetext;
}

std::deque<CString> CVtListCtrlEx::get_line_text_list(int index, int from, int to)
{
	if (index < 0 || index >= size())
		return std::deque<CString>();

	if (to == -1 || to >= get_column_count())
		to = get_column_count() - 1;

	std::deque<int> dqColumn;
	for (int i = from; i <= to; i++)
		dqColumn.push_back(i);

	return get_line_text_list(index, &dqColumn);
}

std::deque<CString> CVtListCtrlEx::get_line_text_list(int index, std::deque<int>* dqColumn)
{
	std::deque<CString> dqResult;

	int		i;
	CString sText = _T("");

	if (index < 0 || index >= size())
		return dqResult;

	for (i = 0; i < dqColumn->size(); i++)
		dqResult.push_back(get_text(index, dqColumn->at(i)));

	return dqResult;
}

//txt 출력 시 컬럼 세로 정렬을 맞게 출력하도록 공백을 보정하여 리턴
//CString은 최대 64K까지만 담을 수 있으므로 vector를 이용함.
void CVtListCtrlEx::get_line_text_list(std::vector<CString>* vt)
{

}

//리스트에 표시할 항목이 없을 경우 표시할 텍스트 설정
void CVtListCtrlEx::set_text_on_empty(CString text, int font_size, Gdiplus::Color cr)
{
	m_text_on_empty = text;
	m_text_on_empty_size = font_size;
	m_text_on_empty_color = cr;

	Invalidate();
};

//shell_listctrl일 때 윈도우 탐색기에서 파일/폴더의 레이블을 변경하는 이벤트가 발생하면
//main에서 이 함수를 호출하여 레이블을 변경한다.
//shell_listctrl이 아니어도 사용 가능하나 레이블로 비교하므로 동일한 항목이 있을 경우는 정상 처리되지 않으므로
//사용하지 말 것!
void CVtListCtrlEx::rename(CString old_text, CString new_text)
{
	//shell_listctrl일 경우 fullpath라면 파일명 또는 폴더명만 추출한다.
	if (m_is_shell_listctrl)
	{
		CString old_label = get_part(old_text, fn_name);
		CString new_label = get_part(new_text, fn_name);

		std::deque<CListCtrlData>::iterator it_data;
		it_data = std::find_if(m_list_db.begin(), m_list_db.end(),
			[old_label](CListCtrlData data)
			{
				return (data.text[0].Compare(old_label) == 0);
			});

		if (it_data == m_list_db.end())
		{
			TRACE(_T("%s item not found."), old_text);
			return;
		}

		int index = std::distance(m_list_db.begin(), it_data);

		//리스트에서 label을 변경시키고
		set_text(index, col_filename, new_label);

		//파일, 폴더 목록에서도 변경시켜준다.

		//우선 폴더 목록에서 찾아보고
		std::deque<CVtFileInfo>::iterator it;
		it = std::find_if(m_cur_folders.begin(), m_cur_folders.end(),
			[old_text](CVtFileInfo fi)
			{
				return (old_text.CompareNoCase(fi.data.cFileName) == 0);
			});

		if (it != m_cur_folders.end())
		{
			_tcscpy_s(it->data.cFileName, _countof(it->data.cFileName), new_text);
			return;
		}

		//없으면 파일 목록에서 찾아서 변경해준다.
		it = std::find_if(m_cur_files.begin(), m_cur_files.end(),
			[old_text](CVtFileInfo fi)
			{
				return (old_text.CompareNoCase(fi.data.cFileName) == 0);
			});

		if (it != m_cur_files.end())
		{
			_tcscpy_s(it->data.cFileName, _countof(it->data.cFileName), new_text);
			return;
		}
	}
}

int CVtListCtrlEx::get_selected_index(int start)
{
	int index = -1;

	if (size() == 0)
		return -1;
	
	POSITION pos = GetFirstSelectedItemPosition();
	if (pos == NULL)
		return index;

	while (true)
	{
		index = GetNextSelectedItem(pos);
		if (index >= start)
			return index;
		else if (pos == NULL)
			return index;
	}

	return index;
}

int CVtListCtrlEx::get_last_selected_item()
{
	for (int i = size() - 1; i >= 0; i--)
	{
		if (GetItemState(i, LVIS_SELECTED))
			return i;
	}

	return -1;
}

//선택된 항목들을 dqSelected에 담는다. dqSelected가 null이면 그냥 선택 갯수를 리턴받아 사용한다.
int CVtListCtrlEx::get_selected_items(std::deque<int> *dq)
{
	if (dq != NULL)
		dq->clear();

	int index;
	int selected_count = 0;
	POSITION pos = GetFirstSelectedItemPosition();

	while (pos)
	{
		index = GetNextSelectedItem(pos);
		selected_count++;
		if (dq != NULL)
			dq->push_back(index);
	}

	return selected_count;
}

//선택된 항목들의 목록을 dq에 담는다. shelllist일 경우 fullpath = true이면 각 항목의 전체경로를 담는다.
int CVtListCtrlEx::get_selected_items(std::deque<CString>* dq, bool is_fullpath)
{
	if (dq)
		dq->clear();

	std::deque<int> dq_index;
	get_selected_items(&dq_index);

	if (dq_index.size() == 0)
		return -1;

	if (!m_is_shell_listctrl)
	{
		for (auto item : dq_index)
		{
			dq->push_back(get_line_text(item, 0, -1));
		}

		return dq_index[0];
	}

	CString folder = m_pShellImageList->convert_special_folder_to_real_path(!m_is_local, get_path());//convert_special_folder_to_real_path(get_path(), m_pShellImageList, !m_is_local);
	CString fullpath;

	if (is_drive_root(folder))
		truncate(folder, 1);
	
	if (dq)
	{
		dq->clear();
		for (auto item : dq_index)
		{
			if (m_is_shell_listctrl && is_fullpath)
			{
				fullpath = concat_path(folder, get_text(item, col_filename));
				dq->push_back(fullpath);
			}
			else
			{
				dq->push_back(get_text(item, col_filename));
			}
		}
	}

	return dq_index.size();
}

//선택된 항목들의 목록을 dq에 담는다. shelllist에서만 사용되며 cFileName은 이미 전체경로를 가지고 있다.
int CVtListCtrlEx::get_selected_items(std::deque<WIN32_FIND_DATA>* dq)
{
	std::deque<int> selected_index;
	get_selected_items(&selected_index);

	if (dq)
	{
		dq->clear();
		for (int i = 0; i < selected_index.size(); i++)
		{
			WIN32_FIND_DATA data;
			memset(&data, 0, sizeof(data));
			data = get_win32_find_data(selected_index[i]);
			TRACE(_T("get_selected_items. selected_index[%d] = %d. label = %s\n"), i, selected_index[i], data.cFileName);
			if (_tcslen(data.cFileName) > 0)
				dq->push_back(data);
		}
	}

	return selected_index.size();
}

void CVtListCtrlEx::select_item(int nIndex, bool bSelect /*= true*/, bool after_unselect, bool insure_visible)
{
	if (after_unselect)
		unselect_selected_item();

	SetItemState(nIndex, bSelect ? LVIS_SELECTED : 0, LVIS_SELECTED);
	SetItemState(nIndex, bSelect ? LVIS_FOCUSED : 0, LVIS_FOCUSED);

	if (insure_visible)
		EnsureVisible(nIndex, FALSE);
		//ensure_visible(nIndex, visible_last);
}

int CVtListCtrlEx::get_check(int index)
{
	if (index < 0 || index >= size())
		return 0;

	if (m_use_virtual_list)
		return m_list_db[index].checked;

	return GetCheck(index);
}

//index = -1 : 전체선택
void CVtListCtrlEx::set_check(int index, bool check)
{
	if (index < 0)
	{
		for (int i = 0; i < size(); i++)
		{
			CListCtrl::SetCheck(i, check);
			if (m_use_virtual_list)
				m_list_db[i].checked = check;
		}

		Invalidate();
	}
	else
	{
		if (index >= size())
			return;

		CListCtrl::SetCheck(index, check);
		if (m_use_virtual_list)
			m_list_db[index].checked = check;
	}
}

void CVtListCtrlEx::set_check(std::deque<int> indices, bool check)
{
	for (const auto& index : indices)
	{
		if (index < 0 || index >= size())
			continue;

		CListCtrl::SetCheck(index, check);
		if (m_use_virtual_list)
			m_list_db[index].checked = check;
	}
}

//check된 항목 리스트를 dq에 담고 체크된 항목의 개수를 리턴한다.
int CVtListCtrlEx::get_checked_items(std::deque<int>* dq)
{
	std::deque<int> checked;

	for (int i = 0; i < size(); i++)
	{
		if (m_use_virtual_list)
		{
			if (m_list_db[i].checked)
				checked.push_back(i);
		}
		else
		{
			if (GetCheck(i))
				checked.push_back(i);
		}
	}

	if (dq != NULL)
	{
		dq->clear();
		dq->assign(checked.begin(), checked.end());
	}

	return checked.size();
}

void CVtListCtrlEx::unselect_selected_item()
{
	SetItemState(-1, 0, LVIS_SELECTED);

	//std::deque<int> dqSelected;
	//get_selected_items(&dqSelected);
	//for (int i = 0; i < dqSelected.size(); i++)
	//	select_item(dqSelected[i], false);
}

//아이템의 상태값이 특정 상태값이 항목 또는 그 개수 구하기
//LVIS_DROPHILITED or LVIS_SELECTED 항목을 구할 수 있다.
//drag 도중에 마우스가 다른 앱 영역으로 나가서 WM_LBUTTONUP 될 경우 drophilited 상태로 아이템이 남는 문제를 제거하기 위해.
int CVtListCtrlEx::get_items_by_state(UINT state, std::deque<int>* dq)
{
	std::deque<int> items;

	for (int i = 0; i < size(); i++)
	{
		if (GetItemState(i, state) & state)
			items.push_back(i);
	}

	if (dq != NULL)
		dq->assign(items.begin(), items.end());

	return items.size();
}

//dq 목록의 아이템들의 state 세팅. dq가 null이면 모든 항목에 대해 실행
//선택 : set_items_with_state(LVIS_SELECTED|LVIS_FOCUSED, LVIS_SELECTED|LVIS_FOCUSED, dq);
//해제 : set_items_with_state(0, LVIS_SELECTED|LVIS_FOCUSED, dq);
int CVtListCtrlEx::set_items_with_state(UINT state, UINT mask, std::deque<int>* dq)
{
	//모든 항목에 대해 수행
	if (dq == NULL)
	{
		for (int i = 0; i < size(); i++)
		{
			SetItemState(i, state, mask);
		}
	}
	//해당 항목들에 대해 수행
	else
	{
		for (int i = 0; i < dq->size(); i++)
		{
			SetItemState(dq->at(i), state, mask);
		}
	}
	
	return 0;
}

//기본 검색함수인 FindItem()을 이용해서 0번 컬럼에서만 데이터를 찾는다. virtual list이므로 OnLvnOdfinditem() 함수 수정 필수.
int CVtListCtrlEx::find(CString str, int start, bool whole_word_olny, bool case_sensitive)
{
	LVFINDINFO info;

	info.flags = (whole_word_olny ? LVFI_STRING : LVFI_PARTIAL|LVFI_STRING);
	info.psz = str;

	// Delete all of the items that begin with the string.
	return FindItem(&info, start);

	/*
	//virtual list가 아니라면 위 코드가 동작하겠지만
	//virtual list라면 OnLvnOdfinditem()함수가 다시 호출되고
	//거기서 결국 찾는 코드가 들어가야 한다.
	//기본 FindItem함수를 이용한다면 위 함수 안에 검색 코드를 추가해야겠지만
	//여기서 간단히 구현해서 사용하자.
	int i;
	CString sText;

	if (start < 0)
		start = 0;

	for (i = start; i < GetItemCount(); i++)
	{
		sText = get_text(i, 0);

		if (!bCaseSensitive)
		{
			sText.MakeLower();
			str.MakeLower();
		}

		if ((bWholeWord && (sText == str)) ||
			(!bWholeWord && (sText.Find(str) >= 0)))
		{
			return i;
		}
	}

	return -1;
	*/
}

//CListCtrl의 FindItem으로도 모든 라인의 모든 컬럼을 검사할 수 있지만
//일단 직접 구현해서 쓴다.
//단 결과 result에는 컬럼번호는 저장되지 않고 라인번호만 저장되므로
//한 라인에서 이미 찾았다면 그 라인의 다른 컬럼에서는 찾지 않고 그냥 break해야 한다.
//그렇지 않으면 동일한 라인 번호가 result에 모두 저장된다.
//result_reset이 true이면 검색 결과를 리셋한 후 검색하고 false이면 추가로 넣는다.
//'|'로 구분하면 각 단어들이 하나라도 포함된 목록을 리턴하고(OR)
//'&'로 구분하면 각 단어가 모두 들어간 목록을(AND)
//구분 기호가 없으면 wholeword로 검색한다.
//작품명의 경우 영문대문자-숫자인 패턴이 많으므로 '-'가 없다면 숫자 앞에 자동 넣어준다.
int CVtListCtrlEx::find(CString find_target, std::deque<int>* result,
								int start_idx, int end_idx,
								std::deque<int>* dqColumn, bool stop_first_found,
								bool whole_word_olny, bool case_sensitive)
{
	int		i;
	TCHAR	op;
	CString sText;
	std::deque<CString> dqTarget;	//separator가 ""이 아닐 경우는 토큰으로 분리하여 모두 찾는다.
	std::deque<int> dq_columns;
	std::deque<int> find_result;

	if (start_idx < 0)
		start_idx = 0;

	if (end_idx < 0 || end_idx >= size())
		end_idx = size() - 1;
	if (end_idx < 0)
		return -1;
	if (start_idx > end_idx)
		return -1;

	//찾은 항목을 선택으로 표시하는 경우 먼저 기존 선택된 항목들을 초기화시켜준다.
	//if (select)
	//	select_item(-1, false);

	if (find_target.Find('&') > 0)
		op = '&';
	else
		op = '|';

	get_token_str(find_target, dqTarget, op, false);
	trim(&dqTarget);

	if (dqColumn == NULL || dqColumn->size() == 0)
	{
		dqColumn = &dq_columns;
		for (i = 0; i < get_column_count(); i++)
			dqColumn->push_back(i);
	}

	int cur_idx = start_idx;
	while (true)
	{
		::SendMessage(GetParent()->GetSafeHwnd(), Message_CVtListCtrlEx, (WPARAM)&CVtListCtrlExMessage(this, message_progress_pos), (LPARAM)cur_idx);

		std::deque<CString> dqLine = get_line_text_list(cur_idx, dqColumn);
		//sline 문자열에서 dqTarget 문자열들이 존재하는지 op방식에 따라 검색.
		if (find_dqstring(dqLine, dqTarget, op, whole_word_olny, case_sensitive) >= 0)
		{
			find_result.push_back(cur_idx);

			if (stop_first_found)
			{
				::SendMessage(GetParent()->GetSafeHwnd(), Message_CVtListCtrlEx, (WPARAM)&CVtListCtrlExMessage(this, message_progress_pos), (LPARAM)-1);
				if (result)
				{
					result->clear();
					result->assign(find_result.begin(), find_result.end());
					return result->at(0);
				}
				return cur_idx;
			}
		}

		cur_idx++;
		if (cur_idx > end_idx)
			break;

		/*
		//한 라인의 데이터를 하나의 스트링 리스트로 얻어와서 비교
		std::deque<CString> dqLine = get_line_text_list(i, column_start, column_end);

		//functions에 find_string으로 선언하였으나 인식이 안되서 find_dqstring으로 이름 변경
		if (find_dqstring(dqLine, dqSrc, bWholeWord, bCaseSensitive) >= 0)
		{
			result->push_back(i);
			if (select)
				select_item(i);
		}
		*/
	}

	if (find_result.size() == 0)
		return -1;

	if (result)
	{
		result->clear();
		result->assign(find_result.begin(), find_result.end());
		return result->at(0);
	}

	return find_result[0];
}

BOOL CVtListCtrlEx::OnNMDblclk(NMHDR *pNMHDR, LRESULT *pResult)
{
	*pResult = 0;

	//dbclick을 편집으로 사용하거나 어떤 액션으로 사용하는 것은
	//이 클래스를 사용하는 메인에서 구현하는 것이 맞다.
	//여기서 구현하면 편리한 경우도 있으나 범용성이 없어진다.
	//=>shell_listctrl이라면 여기서 처리하는 것이 더 범용적인듯하다.
	//다만 더블클릭으로 경로가 변경되는 이벤트가 발생한 것이므로
	//이를 parent에도 알려줘야 CPathCtrl, CSCTreeCtrl도 경로가 변경될 것이다.
	//return FALSE;

	LPNMITEMACTIVATE pNMItemActivate = reinterpret_cast<LPNMITEMACTIVATE>(pNMHDR);
	// TODO: Add your control notification handler code here
	int item;// = pNMItemActivate->iItem;
	int subItem;// = pNMItemActivate->iSubItem;

	//TRACE(_T("%d, %d\n"), item, subItem);

	if (m_is_shell_listctrl)// && m_is_local)
	{
		CString new_path;

		if (hit_test(pNMItemActivate->ptAction, item, subItem, true) == LVHT_NOWHERE ||
			item < 0 || subItem < 0)
		{
			move_parent_folder();
			return TRUE;
		}

		if (item < 0 || item >= size() || subItem < 0 || subItem >= get_column_count())
			return TRUE;

		if (m_path == m_pShellImageList->m_volume[!m_is_local].get_label(CSIDL_DRIVES))
		{
			new_path = m_pShellImageList->convert_special_folder_to_real_path(!m_is_local, get_text(item, col_filename));//convert_special_folder_to_real_path(get_text(item, col_filename), m_pShellImageList, !m_is_local);
		}
		else
		{
			//파일일 경우는 현재로는 아무처리하지 않는다.
			//if (!(get_win32_find_data(item).dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
			if (!get_text(item, col_filesize).IsEmpty())
				return TRUE;

			new_path = concat_path(m_path, get_text(item, col_filename));
		}

		set_path(new_path);
		::SendMessage(GetParent()->GetSafeHwnd(), Message_CVtListCtrlEx, (WPARAM) & (CVtListCtrlExMessage(this, message_path_changed, NULL)), (LPARAM)&new_path);

		return TRUE;
	}


	/*
	if (m_allow_edit == false)
		return FALSE;


	if (m_allow_edit_column[iSubItem] == false)
		return FALSE;

	if (iItem >= 0 && iItem < GetItemCount() &&
		iSubItem >= 0 && iSubItem < get_column_count())
	{
		edit_item(iItem, iSubItem);
		return TRUE;
	}
	*/

	/*
	else
	{
		CString str;

		str.Format(_T("%d"), GetItemCount());
		iItem = AddItem(str, NULL);
		SelectItem(iItem);
		EnsureVisible(iItem, false);
		EditSubItem(iItem, 0);
		return TRUE;
	}
	*/
	return FALSE;
}


bool CVtListCtrlEx::list_copy_to_clipboard(bool onlySelected /*= true*/, TCHAR separator, bool bHead /*= false*/)
{
	ASSERT(::IsWindow(GetSafeHwnd()));

	int		i;
	CString sResult = _T("");

	if (onlySelected && get_selected_index() < 0)
		return false;

	int		nCount = 0;
	int		nColumn = 1;
	CWaitCursor wait;

	if ((GetStyle() & LVS_TYPEMASK) == LVS_REPORT &&
		(GetExtendedStyle() & LVS_EX_FULLROWSELECT))
	{
		CHeaderCtrl* pHeader = GetHeaderCtrl();
		nColumn = pHeader ? pHeader->GetItemCount() : 1;

		//shift키깩?E조합되툈E?E超蝴갋복사한다.
		if ((bHead || (GetKeyState(VK_SHIFT) < 0)) && pHeader)
		{
			for (i = 0; i < nColumn; ++i)
			{
				sResult += get_header_text(i);
				if (i != nColumn - 1)
					sResult += separator;
			}

			sResult += _T("\n");
		}
	}

	/*
	while (pos)
	{
		nItem = GetNextSelectedItem(pos);
		if (0 != nCount)
			sResult += _T("\n");

		for (i = 0; i < nColumn; ++i)
		{
			sResult += GetItemText(nItem, i);
			if (i != nColumn - 1)
				sResult += lpszSeparator;
		}
		++nCount;
	}
	*/
	for (i = 0; i < size(); i++)
	{
		if (!onlySelected || (GetItemState(i, LVIS_SELECTED) == LVIS_SELECTED))
		{
			sResult = sResult + get_line_text(i, 0, -1, separator) + _T("\n");
		}
	}

	copy_to_clipboard(m_hWnd, sResult);

	return TRUE;
}

#if 0
void CVtListCtrlEx::paste_from_clipboard()
{
	TCHAR	*buffer = NULL;
	HGLOBAL	hGlobal;

	int		i;
	int		pos;
	int		iItem = get_selected_index();
	bool	bDifferWarning = true;
	CString sText, str, sToken;

	if (OpenClipboard() == false)
		return;

	if (hGlobal = GetClipboardData(CF_TEXT))
	{
		buffer = (TCHAR*)GlobalLock(hGlobal);
		GlobalUnlock(hGlobal);

		//1 | KIA | K9 | 150
		//2 | 99 | 9 | 999
		sText = buffer;

		sText.Replace(_T("\r\n"), _T("\n"));
		sText.Replace(_T("\t"), _T(","));

		while (sText != "")
		{
			pos = sText.Find(_T("\n"));
			if (pos == -1)
				str = sText;
			else
				str = sText.Left(pos);

			int idx;
			int nSepCount = get_char_count(str, '|');
			CStringArray ar;

			GetToken(str, _T("|"), ar);

			if (bDifferWarning && (get_column_count() != (nSepCount+1)))
			{
				int result = AfxMessageBox(_T("붙여넣을 데이터의 컬럼 갯수가 큱E갋리스트의 컬럼 갯수와 다릅니다.\n그큱E?붙여넣콅E할깩?E"), MB_YESNO);
				if (result == IDNO)
				{
					CloseClipboard();
					return;
				}
				bDifferWarning = false;
			}

			for (i = 0; i < MIN(GetColumnCount(), ar.GetSize()); i++)
			{
				if (i == 0)
				{
					if (iItem < GetItemCount())
					{
						idx = iItem;
						set_text(idx, 0, ar.GetAt(i));
					}
					else
					{
						idx = add_item(ar.GetAt(i), NULL);
					}
				}	
				else
				{
					set_text(idx, i, ar.GetAt(i));
				}
			}

			if (pos == -1)
				break;

			iItem++;
			sText = sText.Mid(pos + 1);
		}
	}

	CloseClipboard();

	::SendMessage(GetParent()->GetSafeHwnd(), MESSAGE_LISTCTRLEX_ITEM_CHANGED, GetDlgCtrlID(), 0);
}
#endif

//파일에서 불러와서 리스트를 채웝芩. 컬럼의 수가 동일해야 한다.
//컬럼 구성이 다른 데이터 파일들을 알아서 불러오햨E않는다.
//이미 컬럼의 구성이 픽스되푳E있컖E그 구성으로 저장된 파일만 불러오도록 되푳E있다.
bool CVtListCtrlEx::load(CString sfile, TCHAR separator, bool match_column_count /*= true*/, bool reset_before_load /*= true*/, bool add_index /*= false*/)
{
	if (get_column_count() == 0)
		return false;

	if (reset_before_load)
		delete_all_items();

	//int		i, j;
	TCHAR	sLine[512];
	CString str;

	FILE* fp;
	file_open(&fp, _T("rt"), sfile);

	if (fp == NULL)
		return false;


	while (_fgetts(sLine, 512, fp))
	{
		if (match_column_count && (get_char_count(sLine, separator)) >= get_column_count())
		{
			fclose(fp);
			return false;
		}
		insert_line(-1, CString(sLine), separator);
	}

	fclose(fp);
	return true;
}

//리스트의 내퓖E?파일로 저장한다.
bool CVtListCtrlEx::save(CString sfile, TCHAR separator, bool selected_only /*= false*/)
{
	int		i, j;
	CString str = _T("");

	if (size() == 0)
		return false;

	FILE* fp;
	
	_tfopen_s(&fp, sfile, _T("wt")CHARSET);

	if (fp == NULL)
		return false;

	if (true)//includeHeader)
	{
		for (i = 0; i < get_column_count(); i++)
		{
			if (i < get_column_count() - 1)
				str = str + get_header_text(i) + separator;
			else
				str += get_header_text(i);
		}

		_ftprintf(fp, _T("%s\n"), str);
	}

	for (i = 0; i < size(); i++)
	{
		str = _T("");

		if (selected_only && GetItemState(i, LVIS_SELECTED) != LVIS_SELECTED)
			continue;

		for (j = 0; j < get_column_count(); j++)
		{
			if (j < get_column_count() - 1)
				str = str + get_text(i, j) + separator;
			else
				str += get_text(i, j);
		}

		_ftprintf(fp, _T("%s\n"), str);
	}

	fclose(fp);
	return true;
}

//이 함수에서는 m_lf 정보를 이용해서 폰트를 재생성한다.
//즉, m_lf.lfHeight 값을 이용해서 폰트가 만들어지므로
//m_font_size 멤버 변수의 값이 변경되었다면
//공식을 이용해 이를 m_lf.lfHeight 값으로 변경한 후 이 함수가 호출되어야 한다.
//m_lf.lfHeight값의 절대값이 MM_TEXT모드의 현재 DC에서의 실제 픽셀크기가 된다.
//따라서 스크롤 크기 등을 계산할때는 m_font_size를 이용하는게 아니라
//m_lf.lfHeight값을 이용해야 정확한 스크롤 크기가 계산된다.
//m_font_size는 단지 사용자에게 일반적인 폰트 크기 설정 수치로 쓰이는 직관적인 수치이다.
void CVtListCtrlEx::reconstruct_font(bool invalidate)
{
	m_font.DeleteObject();
	BOOL bCreated = m_font.CreateFontIndirect(&m_lf);
	SetFont(&m_font, false);

	if (m_HeaderCtrlEx)
		m_HeaderCtrlEx.set_font(&m_lf);

	m_font_size = get_font_size();

	//base font 변경 시 셀별 style font 캐시 무효화 — 다음 draw 에서 새 base 기준으로 lazy 재생성.
	clear_font_style_cache();

	if (invalidate)
		Invalidate();
}

int CVtListCtrlEx::get_font_size()
{
	m_font_size = get_font_size_from_pixel_size(GetParent()->GetSafeHwnd(), m_lf.lfHeight);
	return m_font_size;
}

//-1 : reduce, +1 : enlarge
void CVtListCtrlEx::set_font_size(int font_size, bool invalidate)
{
	if (font_size == 0)
		return;

	if (font_size == -1)
	{
		enlarge_font_size(false);
		return;
	}
	else if (font_size == 1)
	{
		enlarge_font_size(true);
		return;
	}

	m_font_size = font_size;

	//For the MM_TEXT mapping mode,
	//you can use the following formula to specify 
	//a height for a font with a specified point size:
	m_lf.lfHeight = get_pixel_size_from_font_size(GetParent()->GetSafeHwnd(), m_font_size);

	reconstruct_font(invalidate);
}

void CVtListCtrlEx::enlarge_font_size(bool enlarge, bool invalidate)
{
	m_font_size = get_font_size_from_pixel_size(m_hWnd, m_lf.lfHeight);
	enlarge ? m_font_size++ : m_font_size--;

	if (m_font_size < 4)
		m_font_size = 4;
	if (m_font_size > 40)
		m_font_size = 40;

	m_lf.lfHeight = get_pixel_size_from_font_size(GetParent()->GetSafeHwnd(), m_font_size);

	reconstruct_font(invalidate);
}

void CVtListCtrlEx::set_font_name(LPCTSTR sFontname, BYTE byCharSet, bool invalidate)
{
	if (sFontname == _T(""))
		return;
	m_lf.lfCharSet = byCharSet;
	_tcscpy_s(m_lf.lfFaceName, _countof(m_lf.lfFaceName), sFontname);
	reconstruct_font(invalidate);
}

void CVtListCtrlEx::set_font_bold(int weight, bool invalidate)
{
	m_lf.lfWeight = weight;
	reconstruct_font(invalidate);
}

void CVtListCtrlEx::set_font_italic(bool italic, bool invalidate)
{
	m_lf.lfItalic = italic;
	reconstruct_font(invalidate);
}

void CVtListCtrlEx::set_log_font(LOGFONT lf, bool invalidate)
{
	memcpy(&m_lf, &lf, sizeof(LOGFONT));
	reconstruct_font(invalidate);
}

//어떤 항목을 클릭한 후 단축키 F2를 누르면 해당 텍스트를 편집하는 용도이므로
//이 때 클릭된 subItem을 기억해놓는다.
BOOL CVtListCtrlEx::OnNMClick(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMITEMACTIVATE pNMItemActivate = reinterpret_cast<LPNMITEMACTIVATE>(pNMHDR);
	int item = -1;// = pNMItemActivate->iItem;
	int subItem = -1;// = pNMItemActivate->iSubItem;	<== invalid index returned when user clicked out of columns
	int ht = LVHT_NOWHERE;
	*pResult = 0;

	if (((ht = hit_test(pNMItemActivate->ptAction, item, subItem, false)) == LVHT_NOWHERE) ||
		item < 0 || subItem < 0)
	{
		return FALSE;
	}

	if (m_use_virtual_list)
	{
		if (ht == LVHT_ONITEMSTATEICON)
		{
			m_list_db[item].checked = !m_list_db[item].checked;
			TRACE(_T("checkbox clicked. m_list_db[%d].checked = %d\n"), item, m_list_db[item].checked);
			Invalidate();
			::SendMessage(GetParent()->GetSafeHwnd(), Message_CVtListCtrlEx, (WPARAM) & (CVtListCtrlExMessage(this, message_checked_item, NULL)), item);
			return FALSE;
		}
	}

	//TRACE(_T("%d, %d\n"), item, subItem);

	// TODO: Add your control notification handler code here
	if (m_in_editing)
	{
		edit_end();
		//return;
	}

	if (m_allow_edit &&
		(subItem >= 0) &&
		(subItem < get_column_count()) &&
		m_allow_edit_column[subItem])
	{
		//name영역을 클릭한 후 1~2초 사이에 다시 클릭하면 name 편집모드로 전환된다.
		/*
		if ((m_edit_item == item) &&
			(m_edit_subItem == subItem) &&
			(abs(t1 - m_last_clicked) >= 800) &&
			(abs(t1 - m_last_clicked) < 1600))
		*/
		//선택된 항목을 다시 원클릭하면 편집모드로 전환한다.
		if (m_allow_one_click_edit &&
			(m_edit_item == item) &&
			(m_edit_subItem == subItem) &&
			(GetSelectedCount() == 1) &&
			(clock() - m_last_clicked_time > 500) &&	//이 값이 작으면 더블클릭에도 편집되고
			(clock() - m_last_clicked_time < 2000))
		{
			//트리에서 "내 PC"를 선택하여 리스트에 드라이브 리스트가 표시된 상태에서는 시간차 클릭에 의한 편집은 지원하지 않는다.
			if (m_is_shell_listctrl && (get_path() == m_pShellImageList->get_system_label(!m_is_local, CSIDL_DRIVES)))
				return TRUE;

			edit_item(m_edit_item, m_edit_subItem);
		}
		else
		{
			m_last_clicked_time = clock();
		}

		m_edit_item = item;
		m_edit_subItem = subItem;
	}

	//edit mode까지 들어가지 않고 단순 클릭이라면 해당 셀에 설정된 action control이 있다면 표시해준다.


	return FALSE;
}

//m_is_shell_list라면 이름 변경은 굳이 main이 아닌 여기서 처리해도 될듯하다.
//단, 중복되는 이름이 존재하여 실패할 경우는 main에게 이를 알려 메시지를 표시해야 한다.
void CVtListCtrlEx::edit_end(bool valid)
{
	remove_edit_mouse_hook();

	if (m_in_editing == false || m_pEdit == NULL)
		return;

	m_in_editing = false;
	m_last_clicked_time = 0;
	m_pEdit->GetWindowText(m_edit_new_text);
	m_pEdit->ShowWindow(SW_HIDE);

	//valid==false (Escape) 또는 텍스트 미변경 → rename / set_text 모두 건너뜀. 외부 클릭으로 들어온 commit
	//의 경우 텍스트 미변경이면 MoveFile(same, same) 이 실패하고 아래 edit_item() 재진입 경로로 빠져 마치
	//편집이 종료되지 않은 것처럼 보이는 버그를 회피.
	if (!valid || m_edit_new_text == m_edit_old_text)
	{
		LV_DISPINFO dispinfo;
		dispinfo.hdr.hwndFrom = m_hWnd;
		dispinfo.hdr.idFrom = GetDlgCtrlID();
		dispinfo.hdr.code = LVN_ENDLABELEDIT;
		dispinfo.item.mask = LVIF_TEXT;
		dispinfo.item.iItem = m_edit_item;
		dispinfo.item.iSubItem = m_edit_subItem;
		GetParent()->SendMessage(WM_NOTIFY, GetDlgCtrlID(), (LPARAM)&dispinfo);
		return;
	}

	//shell listctrl의 label이 변경되면 실제 파일/폴더명도 변경해줘야 한다.
	if (m_is_shell_listctrl)
	{
		CString path = m_pShellImageList->convert_special_folder_to_real_path(!m_is_local, get_path());//convert_special_folder_to_real_path(get_path(), m_pShellImageList, !m_is_local);
		bool res = false;

		CString old_path = concat_path(path, m_edit_old_text);
		CString new_path = concat_path(path, m_edit_new_text);

		if (m_is_local)
		{
			//이미 동일한 항목이 존재하면 parent에게 알려 메시지를 표시하도록 한다.
			if (new_path != old_path && PathFileExists(new_path))
			{
				::SendMessage(GetParent()->GetSafeHwnd(),
					Message_CVtListCtrlEx,
					(WPARAM) & (CVtListCtrlExMessage(this, message_rename_duplicated, NULL, old_path, new_path)),
					(LPARAM)&res);
				edit_item(m_edit_item, m_edit_subItem);
				return;
			}

			res = MoveFile(old_path, new_path);
		}
		else
		{
			::SendMessage(GetParent()->GetSafeHwnd(),
				Message_CVtListCtrlEx,
				(WPARAM) & (CVtListCtrlExMessage(this, message_request_rename, NULL, old_path, new_path)),
				(LPARAM)&res);
		}

		if (res)
		{
			m_modified = true;

			set_text(m_edit_item, m_edit_subItem, m_edit_new_text);
		}
		else
		{
			edit_item(m_edit_item, m_edit_subItem);
		}
	}
	else
	{
		if (valid)
			set_text(m_edit_item, m_edit_subItem, m_edit_new_text);
	}

	LV_DISPINFO dispinfo;
	dispinfo.hdr.hwndFrom = m_hWnd;
	dispinfo.hdr.idFrom = GetDlgCtrlID();
	dispinfo.hdr.code = LVN_ENDLABELEDIT;
	dispinfo.item.mask = LVIF_TEXT;
	dispinfo.item.iItem = m_edit_item;
	dispinfo.item.iSubItem = m_edit_subItem;

	//이 컨트롤에 LVN_ENDLABELEDIT 이벤트를 보내 기본 핸들러에서 처리할 것이 있다면 처리한다.
	GetParent()->SendMessage(WM_NOTIFY, GetDlgCtrlID(), (LPARAM)&dispinfo);
}

//어떤 항목이 특정 위치에 표시되도록 스크롤시킨다.
//mode가 visible_first이고 offset이 3이면 위에서 3인 인덱스, 즉 4번째 위치에 해당 아이템이 표시되도록 스크롤시킨다.
//mode가 visible_center이고 offset이 0이면 중앙 위치에 해당 아이템이 표시되도록 스크롤시킨다.
//mode가 visible_last이고 offset이 3이면 아래에서 -3-1인 위치에 해당 아이템이 표시되도록 스크롤시킨다.
void CVtListCtrlEx::ensure_visible(int index, int mode, int offset)
{
	//EnsureVisible(index, FALSE);

	int items_per_page = GetCountPerPage();
	int top_index = GetTopIndex();

	CRect r;
	GetItemRect(0, r, LVIR_BOUNDS);

	if (mode == visible_first)
	{
		Scroll(CSize(0, (index - offset - top_index) * r.Height()));
	}
	else if (mode == visible_center)
	{
		Scroll(CSize(0, (index - offset - items_per_page/2 - top_index) * r.Height()));
	}
	else if (mode == visible_last)
	{
		Scroll(CSize(0, (index + offset - items_per_page - top_index + 1) * r.Height()));
	}
}

bool CVtListCtrlEx::is_item_visible(int index, bool bPartial)
{
	CRect rc;
	CRect itemRect;
	
	GetClientRect(rc);
	GetItemRect(index, itemRect, LVIR_BOUNDS);

	rc.top += m_HeaderCtrlEx.get_header_height();

	if (!bPartial && (itemRect.top >= rc.top) && (itemRect.bottom <= rc.bottom))
		return true;
	else if (bPartial &&
			(((itemRect.top > rc.top) && (itemRect.top < rc.bottom)) ||
			 ((itemRect.bottom > rc.top) && (itemRect.bottom < rc.bottom))))
		return true;

	return false;
}

void CVtListCtrlEx::shuffle()
{
	std::random_device rd;
	std::default_random_engine re(rd());
	std::shuffle(m_list_db.begin(), m_list_db.end(), re);
	Invalidate();
}

void CVtListCtrlEx::OnDropFiles(HDROP hDropInfo)
{
	// TODO: 여기에 메시지 처리기 코드를 추가 및/또는 기본값을 호출합니다.
	CPoint pt;
	DragQueryPoint(hDropInfo, &pt);

	DWORD dropped_point = index_from_point(pt.x, pt.y);
	int dropped_index = HIWORD(dropped_point);
	int dropped_column = LOWORD(dropped_point);

	TRACE(_T("dropped point = %d, %d (index = %d, column = %d)\n"), pt.x, pt.y, dropped_index, dropped_column);

	CListCtrl::OnDropFiles(hDropInfo);

	::PostMessage(GetParent()->GetSafeHwnd(), WM_DROPFILES, (WPARAM)hDropInfo, (LPARAM)0);
}

//x, y위치의 item, sub_item 인덱스를 구할 수 있다. item의 인덱스만을 필요로 할 경우는 리턴값만 이용하면 된다.
int CVtListCtrlEx::index_from_point(int x, int y, int* item, int* sub_item)
{
	LVHITTESTINFO ht = {};
	ht.pt.x = x;
	ht.pt.y = y;
	SubItemHitTest(&ht);

	if (item)
		*item = ht.iItem;
	if (sub_item)
		*sub_item = ht.iSubItem;

	return ht.iItem;
	/*
	// 헤더 높이만큼 y 보정
	CHeaderCtrl* pHeader = GetHeaderCtrl();
	if (pHeader)
	{
		CRect rcHeader;
		pHeader->GetWindowRect(&rcHeader);
		ScreenToClient(&rcHeader);
		y -= rcHeader.Height();  // 헤더 높이를 빼줌
	}

	int first = GetTopIndex();
	int last = first + GetCountPerPage();
	CRect rItem;

	for (; first < last; first++)
	{
		for (int j = 0; j < get_column_count(); j++)
		{
			GetSubItemRect(first, j, LVIR_BOUNDS, rItem);
			if (j == 0)
			{
				rItem.right = rItem.left + GetColumnWidth(0);
			}

			if (rItem.PtInRect(CPoint(x, y)))
			{
				if (item)
					*item = first;
				if (sub_item)
					*sub_item = j;
				return first;
			}
		}
	}

	return -1;
	*/
}


void CVtListCtrlEx::MeasureItem(LPMEASUREITEMSTRUCT lpMeasureItemStruct)
{
	// TODO: 여기에 메시지 처리기 코드를 추가 및/또는 기본값을 호출합니다.
	lpMeasureItemStruct->itemHeight = m_line_height;
	//CListCtrl::OnMeasureItem(nIDCtl, lpMeasureItemStruct);
}

void CVtListCtrlEx::set_as_shell_listctrl(CShellImageList* pShellImageList, bool is_local, CString default_path)
{
	m_is_shell_listctrl = true;
	m_is_local = is_local;
	m_use_own_imagelist = true;

	//LVS_EX_FLATSB 사용 금지 — CSCScrollbar overlay 가 단독 결정자.
	SetExtendedStyle(LVS_EX_FULLROWSELECT);

	m_pShellImageList = pShellImageList;
	//이 설정을 해줘야 다른 클래스에서 CListCtrl* 타입으로도 GetImageList()를 통해 참조할 수 있다.
	SetImageList(m_pShellImageList->get_imagelist(), LVSIL_SMALL);

	if (GetUserDefaultUILanguage() == 1042)
		set_headings(_T("이름,200;크기,100;수정한 날짜,150"));
	else
		set_headings(_T("Name,200;Size,100;Date mofified,150"));
	set_font_size(9);
	//set_font_name(_T("맑은 고딕"));
	//set_font_size(), set_font_name()을 호출하지 않고 set_header_height()을 호출하면
	//CHeaderCtrlEx::OnLayout()에서 에러가 발생한다.
	set_header_height(24);
	set_line_height(21);

	set_column_data_type(col_filesize, column_data_type_numeric);

	set_column_text_align(col_filesize, HDF_RIGHT, false);

	allow_edit();
	allow_edit_column(col_filesize, false);
	allow_edit_column(col_filedate, false);

	if (!default_path.IsEmpty())
		set_path(default_path);
}

//list의 index를 주면 fullpath를 리턴한다. -1이면 현재 path를 리턴한다.
CString CVtListCtrlEx::get_path(int index)
{
	//"D:\Temp"와 같이 리턴해야하는데 "D:\\Temp"와 같이 리턴되는 현상이 있다.
	//우선 중복된 역슬래쉬를 제거하고 리턴해준다.
	//추후 모든 path는 반드시 '\'로 끝나도록 하는 규칙에 따라 변경 예정.
	//m_path.Replace(_T("\\\\"), _T("\\"));

	if (index < 0)
		return m_path;

	CString fullpath;

	fullpath = concat_path(m_path, get_text(index, col_filename));

	return m_pShellImageList->convert_special_folder_to_real_path(!m_is_local, fullpath);//convert_special_folder_to_real_path(fullpath, m_pShellImageList, !m_is_local);
}

//현재 선택된 항목이 폴더이면 해당 경로까지의 fullpath를, 파일이라면 현재 리스트의 경로를 리턴한다.
//m_is_shell_listctrl로 동작할때만 동작한다.
CString	CVtListCtrlEx::get_selected_path()
{
	CString path;

	if (!m_is_shell_listctrl)
		return path;

	int index = get_selected_index();
	if (index < 0)
		return path;

	path = get_path();

	//내 PC 목록이 표시되고 있거나 폴더인 경우는 선택된 항목까지의 경로를 리턴한다.
	if (path == m_pShellImageList->get_system_path(!m_is_local, CSIDL_DRIVES) ||
		get_text(index, col_filesize).IsEmpty())
	{
		if (path.Right(1) == '\\')
			path.Format(_T("%s%s\\"), get_path(), get_text(index, col_filename));
		else
			path.Format(_T("%s\\%s\\"), get_path(), get_text(index, col_filename));
	}
	else
	{
		if (path.Right(1) == '\\')
			path.Format(_T("%s%s"), get_path(), get_text(index, col_filename));
		else
			path.Format(_T("%s\\%s"), get_path(), get_text(index, col_filename));
	}

	return m_pShellImageList->convert_special_folder_to_real_path(!m_is_local, path);//convert_special_folder_to_real_path(path, m_pShellImageList, !m_is_local);
}

//해당 인덱스의 파일/폴더의 WIN32_FIND_DATA 값을 리턴한다.
//초기에는 각 항목의 label을 m_cur_folders, m_cur_files에서 찾았으나
//그 갯수가 많아지면 현저히 느려지게 된다.
//display_filelist()를 할 때 m_cur_folders, m_cur_files 항목을 추가하면서
//SetItemData(index, no);로 해당 항목이 위 두 배열에서 몇번째인지 저장한 후
//GetItemData(index);를 통해 해당 항목의 WIN32_FIND_DATA 값을 얻어오도록 수정함.
WIN32_FIND_DATA	CVtListCtrlEx::get_win32_find_data(int index)
{
	int idx = GetItemData(index);
	if (idx >= 0 && idx < m_cur_folders.size())
		return m_cur_folders[idx].data;
	return m_cur_files[idx - m_cur_folders.size()].data;
}

//해당 인덱스의 파일/폴더의 WIN32_FIND_DATA 값을 갱신한다.
void CVtListCtrlEx::set_win32_find_data(int index, WIN32_FIND_DATA data)
{
	int idx = GetItemData(index);
	if (idx >= 0 && idx < m_cur_folders.size())
		memcpy(&m_cur_folders[idx].data, &data, sizeof(WIN32_FIND_DATA));
	else
		memcpy(&m_cur_files[idx - m_cur_folders.size()].data, &data, sizeof(WIN32_FIND_DATA));
}

void CVtListCtrlEx::set_path(CString path, bool refresh)
{
	if (m_pShellImageList == NULL)
		return;

	m_last_clicked_time = 0;

	path = m_pShellImageList->convert_special_folder_to_real_path(!m_is_local, path);//convert_special_folder_to_real_path(path, m_pShellImageList, !m_is_local);

	if (!refresh && (path == m_path))
	{
		TRACE(_T("path is same to m_path. skip."));
		return;
	}

	if (path.IsEmpty())
		path = m_pShellImageList->get_system_path(!m_is_local, CSIDL_DRIVES);

	m_path = path;

	//C 드라이브는 "C:\\"로, "C:\\temp\\"는 "C:\\temp"로 변경한다.
	//즉, 드라이브 루트는 역슬래시가 붙지만 그 외의 폴더는 붙이지 않아야 하므로 제거시켜준다.
	if (m_path.Right(1) == '\\' && m_path.GetLength() > 3)
		m_path = m_path.Left(m_path.GetLength() - 1);

	TRACE(_T("set_path(%s)\n"), m_path);

	refresh_list(refresh);
}

void CVtListCtrlEx::set_filelist(std::deque<CVtFileInfo>* pFolderList, std::deque<CVtFileInfo>* pFileList)
{
	m_cur_folders.clear();
	m_cur_files.clear();

	m_cur_folders.assign(pFolderList->begin(), pFolderList->end());
	m_cur_files.assign(pFileList->begin(), pFileList->end());

	refresh_list(false);
}

void CVtListCtrlEx::invalidate_folder_cache(CString path)
{
	//외부 훅이라 호출부가 넘기는 path 의 대소문자가 다를 수 있어 CompareNoCase 로 매칭(Windows 경로).
	for (auto it = m_folder_cache.begin(); it != m_folder_cache.end(); )
	{
		if (it->path.CompareNoCase(path) == 0)
			it = m_folder_cache.erase(it);
		else
			++it;
	}
}

void CVtListCtrlEx::refresh_list(bool reload, bool force)
{
	if (!m_is_shell_listctrl)
		return;

	int i;
	CString		sfile;

	//UI의 list의 내용은 모두 clear하지만
	//remote의 경우는 reload가 false이고 폴더/파일 목록이 채워진채로 이 함수안으로 들어오므로
	//폴더/파일 목록까지 clear해선 안된다.
	delete_all_items(reload);

	//local일 경우는 파일목록을 다시 읽어서 표시한다.
	//sort할 경우 또는 remote일 경우는 변경된 m_cur_folders, m_cur_files를 새로 표시하면 된다.
	if (reload)
	{
		m_cur_folders.clear();
		m_cur_files.clear();

		if (m_is_local)
		{
			//이번 방문이 캐시 miss(실제 폴더)라 display_filelist 로 아이콘까지 채운 뒤 캐시에 저장해야 하는지.
			bool store_to_cache = false;
			FILETIME cur_mtime = {};

			if (m_path == get_system_label(CSIDL_DRIVES))
			{
				std::deque<CDiskDriveInfo>* drive_list = m_pShellImageList->get_drive_list(!m_is_local);
				for (i = 0; i < drive_list->size(); i++)
				{
					CVtFileInfo fi;
					_tcscpy_s(fi.data.cFileName, _countof(fi.data.cFileName), drive_list->at(i).label);
					fi.data.dwFileAttributes = FILE_ATTRIBUTE_DIRECTORY;
					fi.is_remote = false;
					m_cur_folders.push_back(fi);
				}
			}
			else
			{
				//force(명시적 새로고침 등)면 이 폴더의 캐시를 먼저 무효화해 아래 조회가 반드시 miss → 디스크 재열거되게 한다.
				if (force)
					invalidate_folder_cache(m_path);

				//폴더 콘텐츠 캐시 — 디렉토리 mtime 이 캐시와 같으면 디스크 재-enumerate(+아이콘)를 건너뛰고 캐시 사용.
				//mtime(LastWriteTime)은 항목 추가/삭제/이름변경 시에만 갱신된다. 기존 파일의 제자리 내용 편집(크기/날짜만
				//변경)은 dir mtime 을 안 바꾸므로 이 캐시로 감지 못한다 — 그래서 (1) 큰 폴더만 캐시(작은 폴더는 매번 최신),
				//(2) 명시적 새로고침/전송 완료 시 force/invalidate 로 우회하도록 설계했다.
				bool have_mtime = false;
				WIN32_FILE_ATTRIBUTE_DATA fad;
				if (::GetFileAttributesEx(m_path, GetFileExInfoStandard, &fad))
				{
					cur_mtime = fad.ftLastWriteTime;
					have_mtime = true;
				}

				auto hit = m_folder_cache.end();
				if (have_mtime)
				{
					for (auto it = m_folder_cache.begin(); it != m_folder_cache.end(); ++it)
					{
						if (it->path == m_path && ::CompareFileTime(&it->mtime, &cur_mtime) == 0)
						{
							hit = it;
							break;
						}
					}
				}

				if (hit != m_folder_cache.end())
				{
					//캐시 히트 — enumerate·아이콘 모두 생략. img_idx 가 이미 채워져 있어 display_filelist 가 디스크를 안 탄다.
					m_cur_folders = hit->folders;
					m_cur_files   = hit->files;
					m_folder_cache.splice(m_folder_cache.begin(), m_folder_cache, hit);	//LRU — 최근 사용 front 로.
				}
				else
				{
					std::deque<WIN32_FIND_DATA> dq;
					find_all_files(m_path, &dq, _T("*"), true, false);
					for (int i = 0; i < dq.size(); i++)
					{
						if (dq[i].dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
							m_cur_folders.push_back(CVtFileInfo(dq[i]));
						else
							m_cur_files.push_back(CVtFileInfo(dq[i]));
					}
					//캐시 조건: (1) mtime 을 읽었고(무효화 기준 존재), (2) 항목 수가 임계값 이상(큰 폴더만 — 작은 폴더는
					//재열거가 싸고 제자리 편집 stale 위험이 커 캐시하지 않는다).
					store_to_cache = have_mtime &&
						(int)(m_cur_folders.size() + m_cur_files.size()) >= m_folder_cache_min_items;
				}
			}
			TRACE(_T("cur folders[0] = %s\n"), m_cur_folders.size() > 0 ? m_cur_folders[0].data.cFileName : _T(""));
			TRACE(_T("cur files[0] = %s\n"), m_cur_files.size() > 0 ? m_cur_files[0].data.cFileName : _T(""));

			//display_filelist 가 각 항목의 img_idx 를 1회 계산해 m_cur_folders/files 에 채운다(캐시 저장 전에 호출돼야
			//아이콘까지 캐시된다).
			display_filelist(m_path);

			if (store_to_cache)
			{
				folder_cache_entry entry;
				entry.path    = m_path;
				entry.mtime   = cur_mtime;
				entry.folders = m_cur_folders;	//img_idx 채워진 상태로 저장.
				entry.files   = m_cur_files;
				m_folder_cache.push_front(std::move(entry));
				while ((int)m_folder_cache.size() > m_folder_cache_max)
					m_folder_cache.pop_back();
			}
		}
		else
		{
			//message_path_changed를 보내면 main에서 remote 파일목록을 얻어서 display_filelist(m_path);까지 호출한다.
			::SendMessage(GetParent()->GetSafeHwnd(), Message_CVtListCtrlEx, (WPARAM) & (CVtListCtrlExMessage(this, message_path_changed, NULL)), (LPARAM)&m_path);
		}
	}
	else
	{
		//sort()에 의해 여기까지 온 경우 목록만 다시 표시한다.
		display_filelist(m_path);
	}
}

//m_cur_folders와 m_cur_files에 채워진 정보대로 리스트에 출력시킨다.
void CVtListCtrlEx::display_filelist(CString cur_path)
{
	int i;
	int index;
	int insert_index = -1;
	int img_idx = -1;

	m_path = cur_path;

	SetRedraw(FALSE);
	::SetCursor(AfxGetApp()->LoadStandardCursor(IDC_WAIT));

	//아직 준비되지 않은 상태에서 호출될 경우는 그냥 리턴.
	if (m_cur_sort_column >= m_column_sort_type.size())
		return;

	if (cur_path == m_pShellImageList->m_volume[!m_is_local].get_label(CSIDL_DRIVES))
	{
		set_header_text(col_filesize, GetUserDefaultUILanguage() == 1042 ? _T("사용 가능한 공간") : _T("Free Space"));
		set_header_text(col_filedate, GetUserDefaultUILanguage() == 1042 ? _T("전체 크기") : _T("Total Size"));
		set_column_text_align(col_filedate, LVCFMT_RIGHT);
	}
	else
	{
		set_header_text(col_filesize, GetUserDefaultUILanguage() == 1042 ? _T("크기") : _T("Size"));
		set_header_text(col_filedate, GetUserDefaultUILanguage() == 1042 ? _T("수정한 날짜") : _T("Date modified"));
		set_column_text_align(col_filedate, LVCFMT_LEFT);
	}

	//해당 폴더내에 폴더, 파일이 전혀 없으면 이 메시지를 보낼 필요도 없다.
	if (m_cur_folders.size() + m_cur_files.size() > 0)
	{
		::SendMessage(GetParent()->GetSafeHwnd(), Message_CVtListCtrlEx,
			(WPARAM) & (CVtListCtrlExMessage(this, message_list_processing, NULL, _T(""), _T(""), WPARAM(m_cur_folders.size() + m_cur_files.size()))), (LPARAM)(-1));
	}


	//대량 populate — insert_item/set_text 가 매 항목 SetItemCountEx/sync_scrollbar/InvalidateRect 하던 것을 차단.
	//수천 항목 폴더(System32 등) 선택 시 N 회 sync_scrollbar(컬럼폭 합산+scrollbar MoveWindow)가 주 병목이었다.
	//루프 종료 후 항목수·scrollbar 를 한 번만 갱신한다.
	m_in_bulk_insert = true;

	//드라이브 목록(My PC) 화면 여부 — 이때만 폴더(=드라이브)에 real_path/개별 아이콘/여유공간이 필요하다.
	bool is_drives_view = (m_path == m_pShellImageList->m_volume[!m_is_local].get_label(CSIDL_DRIVES));

	//asc일 경우		: 폴더 먼저 0번 위치부터 표시하고 파일은 현재 리스트의 맨 끝에 추가한다.
	//desc일 경우	: 폴더는 역시 0번부터 표시하고 파일은 i번째에 추가하면 된다. 폴더는 자연히 뒤로 밀리면서 파일들보다 아래에	 표시된다.
	for (i = 0; i < m_cur_folders.size(); i++)
	{
		//진행률 통지는 256 항목마다만 — WinSxS(약 3만) 류에서 항목마다 동기 SendMessage 가 cross-window
		//dispatch 비용으로 누적됐다. 진행률은 근사값이라 스로틀해도 무방.
		if ((i & 0xFF) == 0)
			::SendMessage(GetParent()->GetSafeHwnd(), Message_CVtListCtrlEx,
				(WPARAM) & (CVtListCtrlExMessage(this, message_list_processing, NULL, _T(""), _T(""), WPARAM(m_cur_folders.size() + m_cur_files.size()))), (LPARAM)(i + 1));

		//img_idx 캐시 — 캐시 히트면 그대로. 미스면: 일반 로컬 폴더는 공통 폴더 아이콘(디스크 미접근) 재사용 +
		//real_path 변환(PathFileExists/get_original_path = 디스크) 자체를 생략 → 대량 폴더에서 항목당 디스크 접근 0.
		//드라이브 목록(My PC) / remote 만 real_path 가 필요하므로 그 분기에서만 변환한다.
		if (m_cur_folders[i].img_idx >= 0)
		{
			img_idx = m_cur_folders[i].img_idx;
		}
		else if (m_is_local && !is_drives_view)
		{
			img_idx = m_pShellImageList->get_folder_icon();
			m_cur_folders[i].img_idx = img_idx;
		}
		else
		{
			CString real_path = m_pShellImageList->convert_special_folder_to_real_path(!m_is_local, m_cur_folders[i].data.cFileName);
			if (m_is_local)
			{
				img_idx = m_pShellImageList->GetSystemImageListIcon(!m_is_local, real_path, true);
			}
			else
			{
				if (is_drive_root(real_path))
					img_idx = m_pShellImageList->get_drive_icon(!m_is_local, real_path);
				else
					img_idx = m_pShellImageList->GetSystemImageListIcon(!m_is_local, _T("c:\\windows"), true);
			}
			m_cur_folders[i].img_idx = img_idx;
		}

		index = insert_item(i, get_part(m_cur_folders[i].data.cFileName, fn_name), img_idx, false, false);
		SetItemData(index, i);

		if (is_drives_view)
		{
			CString real_path = m_pShellImageList->convert_special_folder_to_real_path(!m_is_local, m_cur_folders[i].data.cFileName);
			ULARGE_INTEGER ul_free_space;
			ULARGE_INTEGER ul_total_space;

			ul_free_space.QuadPart = 0;
			ul_total_space.QuadPart = 0;

			if (m_is_local)
			{
				ul_free_space.QuadPart = get_disk_free_size(real_path);
				ul_total_space.QuadPart = get_disk_total_size(real_path);
			}
			else
			{
				m_pShellImageList->m_volume[!m_is_local].get_drive_space(real_path, &ul_total_space, &ul_free_space);
				//::SendMessage(GetParent()->GetSafeHwnd(), Message_CVtListCtrlEx, (WPARAM) & (CVtListCtrlExMessage(this, message_get_remote_free_space, NULL, real_path)), (LPARAM)&ul_free_space);
				//::SendMessage(GetParent()->GetSafeHwnd(), Message_CVtListCtrlEx, (WPARAM) & (CVtListCtrlExMessage(this, message_get_remote_total_space, NULL, real_path)), (LPARAM)&ul_total_space);
			}

			//드라이브 용량은 탐색기와 동일하게 내림(size_round_down). unit=-1(auto) 자리를 채우려 floats/unit_string/comma 는 기본값 명시.
			if (ul_free_space.QuadPart > 0)
				set_text(index, col_filesize, get_size_str(ul_free_space.QuadPart, -1, 0, true, true, size_round_down));
			if (ul_total_space.QuadPart > 0)
				set_text(index, col_filedate, get_size_str(ul_total_space.QuadPart, -1, 0, true, true, size_round_down));

			set_text_color(index, col_filesize, listctrlex_weak_color);
			set_text_color(index, col_filedate, listctrlex_weak_color);
		}
		else
		{
			set_text(index, col_filesize, _T(""));
			set_text(index, col_filedate, get_file_time_str(m_cur_folders[i].data.ftLastWriteTime));
			set_text_color(index, col_filedate, listctrlex_weak_color);
		}
	}

	for (i = 0; i < m_cur_files.size(); i++)
	{
		if ((i & 0xFF) == 0)
			::SendMessage(GetParent()->GetSafeHwnd(), Message_CVtListCtrlEx,
				(WPARAM) & (CVtListCtrlExMessage(this, message_list_processing, NULL, _T(""), _T(""), WPARAM(m_cur_folders.size() + m_cur_files.size()))), (LPARAM)(m_cur_folders.size() + i + 1));

		//img_idx 캐시 — 파일은 확장자 기반이라 GetSystemImageListIcon 내부에도 ext 캐시가 있으나, 항목별 보관으로
		//캐시 히트 시 그 조회마저 생략.
		if (m_cur_files[i].img_idx >= 0)
		{
			img_idx = m_cur_files[i].img_idx;
		}
		else
		{
			img_idx = m_pShellImageList->GetSystemImageListIcon(!m_is_local, m_cur_files[i].data.cFileName, false);
			m_cur_files[i].img_idx = img_idx;
		}
		index = insert_item(m_column_sort_type[m_cur_sort_column] == sort_descending ? i : -1, get_part(m_cur_files[i].data.cFileName, fn_name), img_idx, false, false);
		SetItemData(index, m_cur_folders.size() + i);

		set_text(index, col_filesize, get_size_str(get_file_size(m_cur_files[i].data)));
		set_text(index, col_filedate, m_cur_files[i].get_file_time_str());
		set_text_color(index, col_filesize, listctrlex_weak_color);
		set_text_color(index, col_filedate, listctrlex_weak_color);
	}

	//완료 통지 — 진행률을 256 항목마다로 스로틀했으므로 마지막 항목(lParam == 총개수) 통지가 루프에서 누락될 수 있다.
	//parent 핸들러는 lParam == reserved(총개수) 일 때만 프로그레스바를 숨기므로, 완료 통지를 스로틀과 무관하게
	//한 번 더 보내 총개수가 256n+1 이 아닌 경우에도 항상 숨겨지도록 한다.
	if (m_cur_folders.size() + m_cur_files.size() > 0)
		::SendMessage(GetParent()->GetSafeHwnd(), Message_CVtListCtrlEx,
			(WPARAM) & (CVtListCtrlExMessage(this, message_list_processing, NULL, _T(""), _T(""), WPARAM(m_cur_folders.size() + m_cur_files.size()))), (LPARAM)(m_cur_folders.size() + m_cur_files.size()));

	//bulk 종료 — 항목수·scrollbar 를 한 번만 갱신.
	m_in_bulk_insert = false;
	if (m_use_virtual_list)
		SetItemCountEx(m_list_db.size(), LVSICF_NOSCROLL);
	sync_scrollbar();

	SetRedraw(TRUE);
	::SetCursor(AfxGetApp()->LoadStandardCursor(IDC_ARROW));
}

void CVtListCtrlEx::add_file(WIN32_FIND_DATA* data, bool is_remote)
{
	if (data->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
		m_cur_folders.push_back(CVtFileInfo(*data, is_remote));
	else
		m_cur_files.push_back(CVtFileInfo(*data, is_remote));
	return;
	/*
	bool file_is_folder = ((pFindFileData->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == FILE_ATTRIBUTE_DIRECTORY);

	ULARGE_INTEGER ulInt;
	ulInt.LowPart = pFindFileData->nFileSizeLow;
	ulInt.HighPart = pFindFileData->nFileSizeHigh;

	// date
	SYSTEMTIME st;
	FILETIME ftLocal;
	CString filedate;

	FileTimeToLocalFileTime(&(pFindFileData->ftLastWriteTime), &ftLocal);
	FileTimeToSystemTime(&ftLocal, &st);

	//filedate.Format(_T("%d-%02d-%02d %02d:%02d:%02d"), st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond);
	filedate = get_datetime_string(st, 2, true, _T(" "), false, false, false);
	//SetItemText(iIndex, 2, GetDateTimeStringFromTime(GetFileLastModifiedTime(pFindFileData->cFileName), true, false, false));
	add_file(pFindFileData->cFileName, file_is_folder ? 0 : ulInt.QuadPart, filedate, true, file_is_folder);
	*/
}

//파일 또는 폴더를 해당하는 멤버 리스트에 추가한다.
//local인 경우 크기와 날짜가 비어있다면 자동 채워주고 remote라면 비어있으면 안된다.
/*
void CVtListCtrlEx::add_file(CString path, uint64_t size, CString date, bool is_remote, bool is_folder)
{
	bool file_is_folder = is_folder;

	if (!is_remote)
		file_is_folder = PathIsDirectory(path);

	if (file_is_folder)
	{
		m_cur_folders.push_back(CVtFileInfo(path, size, date, is_remote, is_folder));
	}
	else
	{
		m_cur_files.push_back(CVtFileInfo(path, size, date, is_remote, is_folder));
	}
}
*/

//https://jiniya.net/tt/594/
//이 함수는 드래그 이미지를 직접 생성해주는 코드지만 취약점이 많은 코드이므로 사용 중지! 참고만 할것.
CImageList* CVtListCtrlEx::create_drag_image(CListCtrl* pList, LPPOINT lpPoint)
{
	if (pList->GetSelectedCount() <= 0)
		return NULL; // no row selected  


	DWORD dwStyle = GetWindowLong(pList->m_hWnd, GWL_STYLE) & LVS_TYPEMASK;

	CRect rectComplete(0, 0, 0, 0);

	// Determine List Control Client width size  
	CRect rectClient;
	pList->GetClientRect(rectClient);
	int nWidth = rectClient.Width() + 50;

	// Start and Stop index in view area  
	int nIndex = pList->GetTopIndex() - 1;
	int nBottomIndex = pList->GetTopIndex() + pList->GetCountPerPage();
	if (nBottomIndex > (pList->GetItemCount() - 1))
		nBottomIndex = pList->GetItemCount() - 1;

	while ((nIndex = pList->GetNextItem(nIndex, LVNI_SELECTED)) != -1)
	{
		if (nIndex > nBottomIndex)
			break;

		CRect rectItem;
		pList->GetItemRect(nIndex, rectItem, LVIR_BOUNDS);

		if (rectItem.left < 0)
			rectItem.left = 0;

		if (rectItem.right > nWidth)
			rectItem.right = nWidth;

		rectComplete.UnionRect(rectComplete, rectItem);
	}

	// Create memory device context  
	CClientDC dcClient(this);
	CDC dcMem;
	CBitmap Bitmap;

	if (!dcMem.CreateCompatibleDC(&dcClient))
		return NULL;

	if (!Bitmap.CreateCompatibleBitmap(&dcClient
		, rectComplete.Width()
		, rectComplete.Height()))
		return NULL;

	CBitmap* pOldMemDCBitmap = dcMem.SelectObject(&Bitmap);
	// Use green as mask color  
	dcMem.FillSolidRect(0
		, 0
		, rectComplete.Width()
		, rectComplete.Height()
		, RGB(0, 255, 0));

	// 안티알리아스 안된 폰트를 사용하는게 핵심
	CFont* pFont = pList->GetFont();
	LOGFONT lf;
	pFont->GetLogFont(&lf);
	//lf.lfQuality = NONANTIALIASED_QUALITY;
	CFont newFont;
	newFont.CreateFontIndirect(&lf);

	CFont* oldFont = dcMem.SelectObject(&newFont);
	////////////////////////////////////////////////  

	// Paint each DragImage in the DC  
	nIndex = pList->GetTopIndex() - 1;
	while ((nIndex = pList->GetNextItem(nIndex, LVNI_SELECTED)) != -1)
	{
		if (nIndex > nBottomIndex)
			break;

		TCHAR buffer[1000];
		LVITEM item = { 0 };
		item.mask = LVIF_TEXT | LVIF_IMAGE;
		item.iItem = nIndex;
		item.pszText = buffer;
		item.cchTextMax = 999;

		pList->GetItem(&item);

		// Draw the icon  
		
		CImageList* pSingleImageList = pList->GetImageList((dwStyle & LVS_ICON)
			? LVSIL_NORMAL : LVSIL_SMALL);

		if (pSingleImageList)
		{
			CRect rectIcon;
			pList->GetItemRect(nIndex, rectIcon, LVIR_ICON);

			IMAGEINFO info;
			pSingleImageList->GetImageInfo(m_list_db[nIndex].img_idx, &info);
			CPoint p((rectIcon.left - rectComplete.left
				+ rectIcon.right - rectComplete.left) / 2
				- (info.rcImage.right - info.rcImage.left) / 2,
				(rectIcon.top - rectComplete.top
					+ rectIcon.bottom - rectComplete.top) / 2
				- (info.rcImage.bottom - info.rcImage.top) / 2
				+ ((dwStyle == LVS_ICON) ? 2 : 0));

			pSingleImageList->Draw(&dcMem, m_list_db[nIndex].img_idx, p, ILD_TRANSPARENT);
		}

		// Draw the text  
		CString text;
		text = item.pszText;
		CRect textRect;
		pList->GetItemRect(nIndex, textRect, LVIR_LABEL);

		textRect.top -= rectComplete.top - 2;
		textRect.bottom -= rectComplete.top + 1;
		textRect.left -= rectComplete.left - 2;
		textRect.right -= rectComplete.left;

		//dcMem.FillSolidRect(textRect, RGB(255, 0, 0));
		dcMem.SetTextColor(RGB(255, 0, 0));
		DWORD flags = DT_END_ELLIPSIS | /*DT_MODIFYSTRING | */DT_NOCLIP;
		if (dwStyle == LVS_ICON)
			flags |= DT_CENTER | DT_WORDBREAK;
		dcMem.DrawText(text, -1, textRect, flags);
	}

	dcMem.SelectObject(oldFont);

	dcMem.SelectObject(pOldMemDCBitmap);

	// Create drag image(list)  
	CImageList* pCompleteImageList = new CImageList;
	pCompleteImageList->Create(rectComplete.Width()
		, rectComplete.Height()
		, ILC_COLOR32 | ILC_MASK
		, 0
		, 1);
	pCompleteImageList->Add(&Bitmap, RGB(0, 255, 0));
	Bitmap.DeleteObject();

	if (lpPoint)
	{
		lpPoint->x = rectComplete.left;
		lpPoint->y = rectComplete.top;
	}

	return pCompleteImageList;
}

void CVtListCtrlEx::OnLvnBeginDrag(NMHDR* pNMHDR, LRESULT* pResult)
{
	if (!m_use_drag_and_drop)
		return;

	LPNMLISTVIEW pNMLV = reinterpret_cast<LPNMLISTVIEW>(pNMHDR);
	// TODO: 여기에 컨트롤 알림 처리기 코드를 추가합니다.
	LPNMITEMACTIVATE pNMItemActivate = reinterpret_cast<LPNMITEMACTIVATE>(pNMHDR);
	int item = -1;// = pNMItemActivate->iItem;
	int subItem = -1;// = pNMItemActivate->iSubItem;	<== invalid index returned when user clicked out of columns

	if (hit_test(pNMItemActivate->ptAction, item, subItem, false) == LVHT_NOWHERE ||
		item < 0 || subItem < 0)
		return;

	m_nDragIndex = pNMLV->iItem;

	CPoint pt;
	int nOffset = -10; //offset in pixels for drag image (positive is up and to the left; neg is down and to the right)

	int sel_count = GetSelectedCount();

	//focus가 없거나 선택되지 않은 상태에서 바로 drag가 시작되면
	//drag 이미지만 표시되므로 focus를 주고 drag하고 있는 아이템을 선택상태로 표시해줘야 한다.
	//"선택 영역 항상 표시" 속성 또한 true여야 한다.
	if (sel_count == 1)
	{
		::SetFocus(m_hWnd);
		select_item(m_nDragIndex);
	}

	//if (m_pDragImage && m_pDragImage->GetSafeHandle())
	//{
	//	m_pDragImage->DeleteImageList();
	//	m_pDragImage = NULL;
	//}

	CSCGdiplusBitmap bmpRes;// (64, 64, PixelFormat32bppARGB, Gdiplus::Color(128, 255, 0, 0));
	
	//drag_image가 없다면 노드 자체 아이콘 및 레이블을 이용한다.
	//GDI를 이용해서 create_drag_image()를 사용했으나 아이콘과 함께 레이블을 출력할 때 오동작함. 수정 필요.
	//GDIPlus를 이용한 create_drag_image()를 직접 만드는 것도 좋을듯함.
	if (m_pDragImage == NULL || m_pDragImage->m_hImageList == NULL)
	{
		if (m_drag_images_id.size() == 0)
		{
			//bmpRes.create_drag_image(this);
			m_pDragImage = create_drag_image((CListCtrl*)this, &pNMLV->ptAction);
		}
		else
		{
			if (m_drag_images_id.size() == 1)
			{
				bmpRes.load(m_drag_images_id[0]);
			}
			else if (m_drag_images_id.size() > 1)
			{
				bmpRes.load(sel_count == 1 ? m_drag_images_id[0] : m_drag_images_id[1]);
			}

			//bmpRes.draw_text(bmpRes.width / 2 - 4, bmpRes.height / 2 + 4, i2S(sel_count), 20, 2,
			//	_T("Arial"), Gdiplus::Color(255, 0, 0, 0), Gdiplus::Color(255, 255, 128, 128), DT_CENTER | DT_VCENTER);

			m_pDragImage = new CImageList();
			m_pDragImage->Create(bmpRes.width, bmpRes.height, ILC_COLOR32, 1, 1);

			HICON hicon;
			bmpRes.m_pBitmap->GetHICON(&hicon);
			m_pDragImage->Add(hicon);
		}
	}

//	SendMessageToDescendants(WM_SETFONT, (WPARAM)pFontDefault->GetSafeHandle(), 1, TRUE, FALSE);

	//CreateDragImage(m_nDragIndex, &pt);
	ASSERT(m_pDragImage); //make sure it was created

	//// Set dragging flag and others
	m_bDragging = TRUE;	//we are in a drag and drop operation
	m_nDropIndex = -1;	//we don't have a drop index yet
	m_pDragWnd = this; //make note of which list we are dragging from
	m_pDropWnd = this;	//at present the drag list is the drop list

	//// Capture all mouse messages
	SetCapture();

	//// Change the cursor to the drag image
	////	(still must perform DragMove() in OnMouseMove() to show it moving)
	m_pDragImage->BeginDrag(0, CPoint(nOffset, nOffset - 4));
	m_pDragImage->DragEnter(GetDesktopWindow(), pNMLV->ptAction);

	TRACE(_T("start drag...\n"));

	*pResult = 0;
}


void CVtListCtrlEx::OnMouseMove(UINT nFlags, CPoint point)
{
	// TODO: 여기에 메시지 처리기 코드를 추가 및/또는 기본값을 호출합니다.
	//While the mouse is moving, this routine is called.
	//This routine will redraw the drag image at the present
	// mouse location to display the dragging.
	//Also, while over a CListCtrl, this routine will highlight
	// the item we are hovering over.

	if (!m_is_hovering)
	{
		//TRACE(_T("list. move\n"));
		TRACKMOUSEEVENT tme;
		tme.cbSize = sizeof(tme);
		tme.hwndTrack = m_hWnd;
		//TME_HOVER를 넣으면 마우스가 hover되면 자동으로 focus를 가진다.
		tme.dwFlags = TME_LEAVE;// | TME_HOVER;
		tme.dwHoverTime = 1;
		m_is_hovering = true;
		/*m_bIsTracking = */_TrackMouseEvent(&tme);
		//RedrawWindow();
	}


	//// If we are in a drag/drop procedure (m_bDragging is true)
	if (m_bDragging)
	{
		//// Move the drag image
		CPoint pt(point);	//get our current mouse coordinates
		ClientToScreen(&pt); //convert to screen coordinates
		m_pDragImage->DragMove(pt); //move the drag image to those coordinates
		// Unlock window updates (this allows the dragging image to be shown smoothly)
		//m_pDragImage->DragShowNolock(false);

		//// Get the CWnd pointer of the window that is under the mouse cursor
		CWnd* pDropWnd = WindowFromPoint(pt);
		ASSERT(pDropWnd); //make sure we have a window

		CVtListCtrlEx* pList = NULL;
		CTreeCtrl* pTree = NULL;

		//// If we drag outside current window we need to adjust the highlights displayed
		if (pDropWnd != m_pDropWnd)
		{
			TRACE(_T("pDropWnd != m_pDropWnd\n"));

			if (pDropWnd->IsKindOf(RUNTIME_CLASS(CVtListCtrlEx)) && m_pDropWnd->IsKindOf(RUNTIME_CLASS(CVtListCtrlEx)))
			{
				pList = (CVtListCtrlEx*)m_pDropWnd;

				if (m_nDropIndex != -1) //If we drag over the CListCtrl header, turn off the hover highlight
				{
					TRACE(_T("m_nDropIndex != -1\n"));
					VERIFY(pList->SetItemState(m_nDropIndex, 0, LVIS_DROPHILITED));
					// redraw item
					VERIFY(pList->RedrawItems(m_nDropIndex, m_nDropIndex));
					pList->UpdateWindow();
					m_nDropIndex = -1;
				}
				else //If we drag out of the CListCtrl altogether
				{
					int i = 0;
					int nCount = pList->GetItemCount();

					TRACE(_T("m_nDropIndex is not -1, nCount = %d\n"), nCount);

					for (i = 0; i < nCount; i++)
					{
						pList->SetItemState(i, 0, LVIS_DROPHILITED);
					}
					pList->RedrawItems(0, nCount);
					pList->UpdateWindow();
				}
			}
			else if (pDropWnd->IsKindOf(RUNTIME_CLASS(CTreeCtrl)))
			{
				pTree = (CTreeCtrl*)m_pDropWnd;
				UINT uFlags;

				HTREEITEM hItem = pTree->HitTest(pt, &uFlags);

				if ((hItem != NULL) && (TVHT_ONITEM & uFlags))
				{
					pTree->SelectDropTarget(hItem);
					ASSERT(pTree->GetDropHilightItem() == hItem);
				}
			}
			else
			{
				if (pList && m_nDropIndex >= 0)
				{
					pList->SetItemState(m_nDropIndex, 0, LVIS_DROPHILITED);
				}

				if (pTree)
				{
					pTree->SelectDropTarget(NULL);
				}
			}
		}

		// Save current window pointer as the CListCtrl we are dropping onto
		//if (pDropWnd->IsKindOf(RUNTIME_CLASS(CListCtrl)))
		m_pDropWnd = pDropWnd;

		//드롭 대상 컨트롤(트리/리스트) 가장자리에 마우스가 hovering 되면 자동 스크롤.
		//가장자리와의 거리에 비례해 속도 조절 + 타이머로 연속 스크롤(마우스가 멈춰 있어도) — update_drag_auto_scroll() 로 위임.
		update_drag_auto_scroll(pt);	//pt = screen 좌표

		// Convert from screen coordinates to drop target client coordinates
		pDropWnd->ScreenToClient(&pt);

		//If we are hovering over a CListCtrl we need to adjust the highlights
		if (pDropWnd->IsKindOf(RUNTIME_CLASS(CVtListCtrlEx)))
		{
			UINT uFlags;
			pList = (CVtListCtrlEx*)pDropWnd;

			//target listctrl이 drag&drop 가능이 아니면 그냥 리턴.
			if (!pList->get_use_drag_and_drop())
			{
				::SetCursor(AfxGetApp()->LoadStandardCursor(MAKEINTRESOURCE(IDC_NO)));
				return;
			}

			//Note that we can drop here
			::SetCursor(AfxGetApp()->LoadStandardCursor(MAKEINTRESOURCE(IDC_ARROW)));

			// Turn off hilight for previous drop target
			pList->SetItemState(m_nDropIndex, 0, LVIS_DROPHILITED);
			// Redraw previous item
			pList->RedrawItems(m_nDropIndex, m_nDropIndex);

			// Get the item that is below cursor
			m_nDropIndex = ((CVtListCtrlEx*)pDropWnd)->HitTest(pt, &uFlags);
			// Highlight it (폴더인 경우에만 hilite시킨다)
			if (m_nDropIndex >= 0 && ((CVtListCtrlEx*)pDropWnd)->GetItemText(m_nDropIndex, col_filesize) == _T(""))
				pList->SetItemState(m_nDropIndex, LVIS_DROPHILITED, LVIS_DROPHILITED);
			// Redraw item
			pList->RedrawItems(m_nDropIndex, m_nDropIndex);
			pList->UpdateWindow();
		}
		else if (pDropWnd->IsKindOf(RUNTIME_CLASS(CTreeCtrl)))
		{
			::SetCursor(AfxGetApp()->LoadStandardCursor(MAKEINTRESOURCE(IDC_ARROW)));
			UINT uFlags;
			pTree = (CTreeCtrl*)pDropWnd;

			// Get the item that is below cursor
			HTREEITEM hItem = ((CTreeCtrl*)pDropWnd)->HitTest(pt, &uFlags);
			pTree->SetItemState(hItem, TVIS_DROPHILITED, TVIF_STATE);
			pTree->SelectDropTarget(hItem);
			ASSERT(hItem == pTree->GetDropHilightItem());
		}
		else
		{
			if (pList && m_nDropIndex >= 0)
			{
				pList->SetItemState(m_nDropIndex, 0, LVIS_DROPHILITED);
			}

			if (pTree)
			{
				pTree->SelectDropTarget(NULL);
			}

			//If we are not hovering over a CListCtrl, change the cursor
			// to note that we cannot drop here
			::SetCursor(AfxGetApp()->LoadStandardCursor(MAKEINTRESOURCE(IDC_NO)));
		}
		// Lock window updates
		m_pDragImage->DragShowNolock(true);
	}

	CListCtrl::OnMouseMove(nFlags, point);
}


//드래그 중, 대상 컨트롤(m_pDropWnd) 가장자리 근처면 자동 스크롤 방향/속도(level)를 정하고 타이머로 연속 스크롤한다.
//속도는 가장자리와의 '거리에 비례'(가까울수록 빠름). 상/하/좌/우 4방향 각각. screen_pt = 스크린 좌표.
void CVtListCtrlEx::update_drag_auto_scroll(CPoint screen_pt)
{
	m_drag_scroll_vx = 0;
	m_drag_scroll_vy = 0;

	if (m_pDropWnd)
	{
		CRect rw;
		m_pDropWnd->GetWindowRect(rw);

		const int MARGIN    = 48;	//가장자리 감지 폭(px)
		const int MAX_LEVEL = 5;	//tick 당 최대 스크롤 단위(level). 가장자리 접할수록 이 값에 근접.

		if (rw.PtInRect(screen_pt))
		{
			//가장자리로부터의 거리 d(0=가장자리) → level = 1 + (MARGIN-d)*(MAX-1)/MARGIN. d=0 → MAX, d=MARGIN → 1.
			if (screen_pt.x < rw.left + MARGIN)
				m_drag_scroll_vx = -(1 + (MARGIN - (screen_pt.x - rw.left)) * (MAX_LEVEL - 1) / MARGIN);
			else if (screen_pt.x > rw.right - MARGIN)
				m_drag_scroll_vx =  (1 + (MARGIN - (rw.right - screen_pt.x)) * (MAX_LEVEL - 1) / MARGIN);

			if (screen_pt.y < rw.top + MARGIN)
				m_drag_scroll_vy = -(1 + (MARGIN - (screen_pt.y - rw.top)) * (MAX_LEVEL - 1) / MARGIN);
			else if (screen_pt.y > rw.bottom - MARGIN)
				m_drag_scroll_vy =  (1 + (MARGIN - (rw.bottom - screen_pt.y)) * (MAX_LEVEL - 1) / MARGIN);
		}
	}

	if (m_drag_scroll_vx != 0 || m_drag_scroll_vy != 0)
		SetTimer(TIMER_ID_DRAG_AUTO_SCROLL, 70, NULL);	//~14fps 연속 스크롤(마우스가 멈춰 있어도 계속)
	else
		KillTimer(TIMER_ID_DRAG_AUTO_SCROLL);
}

void CVtListCtrlEx::OnTimer(UINT_PTR nIDEvent)
{
	if (nIDEvent == TIMER_ID_DRAG_AUTO_SCROLL)
	{
		if (!m_bDragging || m_pDropWnd == NULL || (m_drag_scroll_vx == 0 && m_drag_scroll_vy == 0))
		{
			KillTimer(TIMER_ID_DRAG_AUTO_SCROLL);
			return;
		}

		//level 만큼 SB_LINE 을 반복 전송 → 대상이 트리면 m_h_scroll_pos/native V 로, 리스트면 native 로 스크롤되고 오버레이 스크롤바도 연동.
		for (int i = 0; i < abs(m_drag_scroll_vy); i++)
			m_pDropWnd->SendMessage(WM_VSCROLL, (m_drag_scroll_vy < 0) ? SB_LINEUP   : SB_LINEDOWN);
		for (int i = 0; i < abs(m_drag_scroll_vx); i++)
			m_pDropWnd->SendMessage(WM_HSCROLL, (m_drag_scroll_vx < 0) ? SB_LINELEFT : SB_LINERIGHT);

		return;
	}

	__super::OnTimer(nIDEvent);
}

void CVtListCtrlEx::OnLButtonUp(UINT nFlags, CPoint point)
{
	// TODO: 여기에 메시지 처리기 코드를 추가 및/또는 기본값을 호출합니다.
	//This routine is the end of the drag/drop operation.
	//When the button is released, we are to drop the item.
	//There are a few things we need to do to clean up and
	// finalize the drop:
	//	1) Release the mouse capture
	//	2) Set m_bDragging to false to signify we are not dragging
	//	3) Actually drop the item (we call a separate function to do that)

	//If we are in a drag and drop operation (otherwise we don't do anything)
	if (m_bDragging)
	{
		TRACE(_T("OnLButtonUp\n"));
		// Release mouse capture, so that other controls can get control/messages
		ReleaseCapture();

		// Note that we are NOT in a drag operation
		m_bDragging = FALSE;

		//드래그 자동 스크롤 타이머 정지.
		KillTimer(TIMER_ID_DRAG_AUTO_SCROLL);
		m_drag_scroll_vx = 0;
		m_drag_scroll_vy = 0;

		// End dragging image
		m_pDragImage->DragLeave(GetDesktopWindow());
		m_pDragImage->EndDrag();
		//delete m_pDragImage; //must delete it because it was created at the beginning of the drag

		CPoint pt(point); //Get current mouse coordinates
		ClientToScreen(&pt); //Convert to screen coordinates
		// Get the CWnd pointer of the window that is under the mouse cursor
		CWnd* pDropWnd = WindowFromPoint(pt);
		ASSERT(pDropWnd); //make sure we have a window pointer
		// If window is CListCtrl, we perform the drop
		if (pDropWnd->IsKindOf(RUNTIME_CLASS(CVtListCtrlEx)) ||
			pDropWnd->IsKindOf(RUNTIME_CLASS(CTreeCtrl)))
		{
			m_pDropWnd = pDropWnd; //Set pointer to the list we are dropping on
			DroppedHandler(m_pDragWnd, m_pDropWnd); //Call routine to perform the actual drop
		}

		//ListCtrl에서 drag하여 drophilited가 표시된 상태에서 빠르게 마우스를 밖으로 이동시키면
		//마우스를 떼도 drophilited된 항목 표시가 여전히 남는다.
		//메인에 메시지를 보내서 해당 컨트롤들의 아이템에서 drophilited를 제거시켜줘야 한다.
		::SendMessage(GetParent()->GetSafeHwnd(), Message_CVtListCtrlEx, (WPARAM) & (CVtListCtrlExMessage(this, message_drag_and_drop, NULL)), (LPARAM)0);
	}

	CListCtrl::OnLButtonUp(nFlags, point);
}

void CVtListCtrlEx::DroppedHandler(CWnd* pDragWnd, CWnd* pDropWnd)
{
	//drop되면 그 이벤트만 메인에 알리고
	//메인에서는 drag관련 정보와 drop정보를 이용해서 원하는 처리만 한다.
	//따라서 맨 아래 ::SendMessage만 필요하며
	//중간 코드들은 메인에서 활용하는 방법에 대한 예시임.

	CString droppedItem;

	if (pDropWnd->IsKindOf(RUNTIME_CLASS(CVtListCtrlEx)))
	{
		CVtListCtrlEx* pDropListCtrl = (CVtListCtrlEx*)pDropWnd;
		
		if (!pDropListCtrl->get_use_drag_and_drop())
			return;

		if (m_nDropIndex >= 0)
			droppedItem = pDropListCtrl->GetItemText(m_nDropIndex, col_filename);

		std::deque<int> dq;
		get_selected_items(&dq);

		for (int i = 0; i < dq.size(); i++)
			TRACE(_T("drag item %d of %p = %s\n"), i, pDragWnd, GetItemText(dq[i], col_filename));

		TRACE(_T("dropped on %p = %s\n"), pDropWnd, (droppedItem.IsEmpty() ? _T("same ctrl") : droppedItem));
	}
	else if (pDropWnd->IsKindOf(RUNTIME_CLASS(CTreeCtrl)))
	{

	}

	::SendMessage(GetParent()->GetSafeHwnd(), Message_CVtListCtrlEx, (WPARAM) & (CVtListCtrlExMessage(this, message_drag_and_drop, pDropWnd)), (LPARAM)0);
}

//https://learn.microsoft.com/en-us/windows/win32/controls/lvn-itemchanging
//LVS_OWNERDATA style, LVN_ITEMCHANGING notification codes are not sent.
BOOL CVtListCtrlEx::OnLvnItemchanging(NMHDR* pNMHDR, LRESULT* pResult)
{
	LPNMLISTVIEW pNMLV = reinterpret_cast<LPNMLISTVIEW>(pNMHDR);
	// TODO: 여기에 컨트롤 알림 처리기 코드를 추가합니다.
	*pResult = 0;

	return FALSE;
}

BOOL CVtListCtrlEx::OnLvnItemchanged(NMHDR* pNMHDR, LRESULT* pResult)
{
	LPNMLISTVIEW pNMLV = reinterpret_cast<LPNMLISTVIEW>(pNMHDR);
	// TODO: 여기에 컨트롤 알림 처리기 코드를 추가합니다.
	NM_LISTVIEW* pNMListView = (NM_LISTVIEW*)pNMHDR;

	if (pNMListView->uNewState & LVIS_SELECTED)
		set_auto_scroll(false);

	//trace(pNMListView->iItem);

	if (pNMListView->iItem < 0)
		return FALSE;

	//키보드 navigation 으로 selection 이동 시 base 가 internal top 갱신 → overlay scrollbar 도 sync.
	//LVN_ENDSCROLL 이 키보드 auto-scroll 에선 발화 누락 → top 변경 시에만 UpdateWindow 강제.
	//(매 selection 변경마다 호출하면 자막 highlight 같은 빈번한 변경에 깜빡임 발생.)
	if (m_scrollbar_setup)
	{
		int top = GetTopIndex();
		if (top != m_last_top_index)
		{
			UpdateWindow();
			m_last_top_index = top;
		}
	}
	sync_scrollbar();

	//if (!m_use_virtual_list)
	//	return FALSE;

	if (pNMListView->uChanged & LVIF_STATE)
	{
		//체크박스 상태가 변경된 경우에만 처리하기 위해.
		UINT old_check = pNMLV->uOldState & LVIS_STATEIMAGEMASK;
		UINT new_check = pNMLV->uNewState & LVIS_STATEIMAGEMASK;
		//TRACE(_T("pNMListView->uChanged = %d, old = %d, new = %d\n"), pNMListView->uChanged, old_check, new_check);

		/*
		if (m_use_virtual_list)
		{
			m_list_db[pNMListView->iItem].checked = !m_list_db[pNMListView->iItem].checked;
			//Invalidate();
			TRACE(_T("list item changed: %d, checked = %d\n"), pNMListView->iItem, m_list_db[pNMListView->iItem].checked);
		}
		else 
		*/
		//if (old_check && new_check && old_check != new_check)
		{
			//else
			{
				if ((pNMListView->uNewState & LVIS_STATEIMAGEMASK) == 0x1000)
				{
					SetCheck(pNMListView->iItem, false);
				}
				else if ((pNMListView->uNewState & LVIS_STATEIMAGEMASK) == 0x2000)
				{
					SetCheck(pNMListView->iItem, true);
				}
			}
		}
	}

	return FALSE;

	*pResult = 0;
}
/*
void CVtListCtrlEx::capture_selected_items_to_bitmap(CSCGdiplusBitmap* bmp)
{
	int i;
	CRect r, rc;
	CRect rTotal;
	std::deque<int> dq;
	std::deque<CRect> rItem;

	GetClientRect(rc);
	get_selected_items(&dq);

	//선택 항목들의 합집합으로 최대 사각형을 구하고
	for (i = 0; i < dq.size(); i++)
	{
		GetItemRect(dq[i], r, LVIR_LABEL);
		//r.right = rc.right;
		//if (RectInRect(rc, r))
		{
			rItem.push_back(r);
			//ClientToScreen(r);

			if (rTotal.IsRectNull())
				rTotal = r;
			else
				rTotal.UnionRect(rTotal, r);


		}
	}

	//if (rTotal.Height() > 48)
	//	rTotal.bottom = rTotal.top + 48;

	bmp->resize(rTotal.Width(), rTotal.Height());

	CPoint offset(-rItem[0].left, -rItem[0].top);

	//각 항목 영역을 캡처해서 bmp에 붙여준다.
	for (i = 0; i < rItem.size(); i++)
	{
		r = rItem[i];
		ClientToScreen(r);
		HBITMAP hBitmap = capture_screen_to_bitmap(r);
		CSCGdiplusBitmap sub(hBitmap);

		rItem[i].OffsetRect(offset);
		bmp->draw(&sub, &rItem[i]);
	}
}
*/

//ListCtrl 의 우클릭이 WM_CONTEXTMENU 를 항상 self 로 보내지는 않으므로(NM_RCLICK 만 발생하는 경우가 있음),
//우클릭 메뉴 경로를 WM_CONTEXTMENU 하나로 정규화하기 위해 명시적으로 self 에 SendMessage 한다 (CSCTreeCtrl 과 동일 패턴).
//return TRUE 로 NM_RCLICK 을 소비하여 parent 의 NM_RCLICK 핸들러가 이중 호출되지 않게 한다.
BOOL CVtListCtrlEx::OnNMRClick(NMHDR* /*pNMHDR*/, LRESULT* pResult)
{
	CPoint pt_screen;
	GetCursorPos(&pt_screen);
	SendMessage(WM_CONTEXTMENU, (WPARAM)m_hWnd, MAKELPARAM(pt_screen.x, pt_screen.y));

	*pResult = 1;
	return TRUE;
}

void CVtListCtrlEx::OnContextMenu(CWnd* /*pWnd*/, CPoint point)
{
	//키보드 컨텍스트 메뉴 키는 (-1,-1) 로 오므로 커서 위치로 정규화.
	if (point.x == -1 && point.y == -1)
		point = (CPoint)GetMessagePos();

	//자체 메뉴 미사용 — parent 로 위임. wParam = 소스 hwnd 로 parent 가 어느 리스트인지 구분 가능.
	if (!m_use_own_context_menu)
	{
		CWnd* parent = GetParent();
		if (parent)
			parent->SendMessage(WM_CONTEXTMENU, (WPARAM)GetSafeHwnd(), MAKELPARAM(point.x, point.y));
		return;
	}

	//자체 내장 메뉴 — 파일 개념이 없는 범용 리스트이므로 범용 항목(모두 선택 / 선택 해제)만 제공.
	CMenu menu;
	menu.CreatePopupMenu();
	menu.AppendMenu(MF_STRING, menu_select_all, _T("모두 선택(&A)\tCtrl+A"));
	menu.AppendMenu(MF_STRING, menu_unselect_all, _T("선택 해제(&U)"));
	menu.EnableMenuItem(menu_select_all, MF_BYCOMMAND | (GetItemCount() > 0 ? MF_ENABLED : MF_GRAYED));
	menu.EnableMenuItem(menu_unselect_all, MF_BYCOMMAND | (GetSelectedCount() > 0 ? MF_ENABLED : MF_GRAYED));
	menu.TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON, point.x, point.y, this);
}

void CVtListCtrlEx::OnPopupMenu(UINT nID)
{
	switch (nID)
	{
	case menu_select_all:	select_item(-1, true, false, false); break;	//SetItemState(-1) 은 전체 항목에 적용됨
	case menu_unselect_all:	select_item(-1, false, false, false); break;
	}
}


void CVtListCtrlEx::OnMouseHover(UINT nFlags, CPoint point)
{
	// TODO: 여기에 메시지 처리기 코드를 추가 및/또는 기본값을 호출합니다.
	TRACE(_T("list. hover\n"));
	CListCtrl::OnMouseHover(nFlags, point);
}


void CVtListCtrlEx::OnMouseLeave()
{
	// TODO: 여기에 메시지 처리기 코드를 추가 및/또는 기본값을 호출합니다.
	//TRACE(_T("list. leave\n"));
	m_is_hovering = false;
	CListCtrl::OnMouseLeave();
}

//HAS_STRING, OWNER_DRAW_FIXED 속성을 가지면 CListCtrl의 Get/SetItemData() 함수를 사용할 수 없다.
//이 두 함수를 사용할 수 있도록 CListCtrlData에 data 멤버를 추가하고 다음 함수들을 override하여 선언함.
DWORD_PTR CVtListCtrlEx::GetItemData(int index)
{
	if (index < 0 || index >= m_list_db.size())
		return NULL;

	return m_list_db[index].data;
}

BOOL CVtListCtrlEx::SetItemData(int index, DWORD_PTR dwData)
{
	if (index < 0 || index >= m_list_db.size())
		return FALSE;

	m_list_db[index].data = dwData;
	return TRUE;
}


void CVtListCtrlEx::OnSetFocus(CWnd* pOldWnd)
{
	CListCtrl::OnSetFocus(pOldWnd);

	// TODO: 여기에 메시지 처리기 코드를 추가합니다.
	m_has_focus = true;
}


void CVtListCtrlEx::OnKillFocus(CWnd* pNewWnd)
{
	CListCtrl::OnKillFocus(pNewWnd);

	// TODO: 여기에 메시지 처리기 코드를 추가합니다.
	m_has_focus = false;
}

//remote일 경우는 fullpath로 해당 파일의 WIN32_FIND_DATA값을 얻어야 할 경우가 있다.
void CVtListCtrlEx::get_remote_file_info(CString fullpath, WIN32_FIND_DATA* data)
{
	int i;

	//lambda를 이용한 방법인데 우선 보류.
	//std::find(m_cur_folders.begin(), m_cur_folders.end(), [&fullpath](const CVtFileInfo& x)
	//	{
	//		)

	fullpath = m_pShellImageList->convert_special_folder_to_real_path(!m_is_local, fullpath);//convert_special_folder_to_real_path(fullpath, m_pShellImageList, !m_is_local);

	for (i = 0; i < m_cur_folders.size(); i++)
	{
		if (get_part(fullpath, fn_name) == get_part(m_cur_folders[i].data.cFileName, fn_name))
		{
			memcpy(data, &m_cur_folders[i].data, sizeof(WIN32_FIND_DATA));
			//_tcscpy(data->cFileName, m_cur_folders[i].data.cFileName);
			//data->dwFileAttributes = FILE_ATTRIBUTE_DIRECTORY;
			//data->nFileSizeHigh = 0;
			//data->nFileSizeLow = 0;
			//data->ftLastWriteTime = m_cur_folders[i].get_file_time_str();
			return;
		}
	}

	for (i = 0; i < m_cur_files.size(); i++)
	{
		if (get_part(fullpath, fn_name) == get_part(m_cur_files[i].data.cFileName, fn_name))
		{
			memcpy(data, &m_cur_files[i].data, sizeof(WIN32_FIND_DATA));
			//_tcscpy(data->cFileName, m_cur_files[i].data.cFileName);
			//data->dwFileAttributes = FILE_ATTRIBUTE_NORMAL;
			//data->nFileSizeHigh = m_cur_files[i].filesize.HighPart;
			//data->nFileSizeLow = m_cur_files[i].filesize.LowPart;
			return;
		}
	}
}


void CVtListCtrlEx::OnLButtonDown(UINT nFlags, CPoint point)
{
	// TODO: 여기에 메시지 처리기 코드를 추가 및/또는 기본값을 호출합니다.
	CListCtrl::OnLButtonDown(nFlags, point);
}


LRESULT CVtListCtrlEx::OnNcHitTest(CPoint point)
{
	// TODO: 여기에 메시지 처리기 코드를 추가 및/또는 기본값을 호출합니다.
	return CListCtrl::OnNcHitTest(point);
}


void CVtListCtrlEx::OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar)
{
	// TODO: 여기에 메시지 처리기 코드를 추가 및/또는 기본값을 호출합니다.
	//TRACE(_T("CVtListCtrlEx::OnHScroll\n"));
	if (m_in_editing)
		edit_end();
	CListCtrl::OnHScroll(nSBCode, nPos, pScrollBar);
}


void CVtListCtrlEx::OnVScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar)
{
	// TODO: 여기에 메시지 처리기 코드를 추가 및/또는 기본값을 호출합니다.
	TRACE(_T("CVtListCtrlEx::OnVScroll\n"));
	if (m_in_editing)
		edit_end();

	CListCtrl::OnVScroll(nSBCode, nPos, pScrollBar);
	sync_scrollbar();
}


void CVtListCtrlEx::OnLvnBeginScroll(NMHDR* pNMHDR, LRESULT* pResult)
{
	// 이 기능을 사용하려면 Internet Explorer 5.5 이상이 필요합니다.
	// _WIN32_IE 기호는 0x0560보다 크거나 같아야 합니다.
	LPNMLVSCROLL pStateChanged = reinterpret_cast<LPNMLVSCROLL>(pNMHDR);
	// TODO: 여기에 컨트롤 알림 처리기 코드를 추가합니다.
	//TRACE(_T("CVtListCtrlEx::OnLvnBeginScroll\n"));
	*pResult = 0;
}


void CVtListCtrlEx::OnLvnEndScroll(NMHDR* pNMHDR, LRESULT* pResult)
{
	// 이 기능을 사용하려면 Internet Explorer 5.5 이상이 필요합니다.
	// _WIN32_IE 기호는 0x0560보다 크거나 같아야 합니다.
	LPNMLVSCROLL pStateChanged = reinterpret_cast<LPNMLVSCROLL>(pNMHDR);
	// TODO: 여기에 컨트롤 알림 처리기 코드를 추가합니다.
	//TRACE(_T("CVtListCtrlEx::OnLvnEndScroll\n"));

	//모든 scroll source (휠, 가로 휠, 키보드 navigation, internal) 가 LVN_ENDSCROLL 발화 →
	//여기서 UpdateWindow + sync 강제하면 erase ↔ paint 사이 디스플레이 refresh 차단.
	if (m_scrollbar_setup)
	{
		UpdateWindow();
		sync_scrollbar();

		//top 항목을 헤더 하단에 스냅 — header_height ≠ line_height 인 경우 native 가 클릭/EnsureVisible 시
		//top 항목을 헤더에 딱 안 맞춰 sub-pixel 어긋남(gap/overlap)이 생긴다. 모든 scroll source 가 수렴하는
		//이 지점에서 보정. Scroll 이 다시 LVN_ENDSCROLL 을 유발하므로 m_snapping 으로 재진입 차단(수렴 후 dy=0).
		if (!m_snapping && m_HeaderCtrlEx.GetSafeHwnd() && GetItemCount() > 0)
		{
			int top = GetTopIndex();
			CRect ri;
			if (top >= 0 && GetItemRect(top, &ri, LVIR_BOUNDS))
			{
				int dy = ri.top - get_header_height();
				if (dy != 0)
				{
					m_snapping = true;
					Scroll(CSize(0, dy));
					m_snapping = false;
				}
			}
		}
	}

	*pResult = 0;
}

BOOL CVtListCtrlEx::OnNotify(WPARAM wParam, LPARAM lParam, LRESULT* pResult)
{
	//listview 가 native header 의 raw HDN_* notification 을 parent 로 reflect 하지 않아
	//ON_NOTIFY_REFLECT(HDN_TRACK...) 매크로는 작동하지 않는다. WM_NOTIFY 는 child→parent 직진 경로라
	//listview 의 OnNotify override 에서 직접 가로채는 것이 정석.
	NMHDR* nm = reinterpret_cast<NMHDR*>(lParam);
	if (nm)
	{
		CHeaderCtrl* hdr = GetHeaderCtrl();
		HWND hdr_hwnd = hdr ? hdr->GetSafeHwnd() : NULL;

		if (hdr && nm->hwndFrom == hdr_hwnd)
		{
			//HDN_DIVIDERDBLCLICK 자동맞춤 — m_scrollbar_setup 가드 *밖*. 스크롤바 인프라와 무관하게 항상 처리.
			//과거 회귀 원인: 가드 안에 묶여 있어 scrollbar 미설정 시 우리 분기 미진입 → 시스템 default 가 동작.
			if (nm->code == HDN_DIVIDERDBLCLICKW || nm->code == HDN_DIVIDERDBLCLICKA)
			{
				NMHEADER* nmh = (NMHEADER*)nm;
				int col = nmh->iItem;
				int w = get_column_max_text_width(col);
				if (w > 0)
					set_column_width(col, w, true);

				if (m_scrollbar_setup)
					sync_scrollbar();
				if (pResult) *pResult = 1;
				return TRUE;   //base OnNotify 호출 차단 — Default 가 LVSCW_AUTOSIZE 로 덮어쓰는 것 방지.
			}

			//나머지 트랙/체인지 분기는 sync_scrollbar 호출이라 m_scrollbar_setup 가드 안에서.
			if (m_scrollbar_setup)
			{
				switch (nm->code)
				{
				case HDN_TRACKW: case HDN_TRACKA:                       //드래그 진행 중 매 이동 (일반 컬럼)
				case HDN_ENDTRACKW: case HDN_ENDTRACKA:                 //release
				case HDN_ITEMCHANGEDW: case HDN_ITEMCHANGEDA:           //마지막 컬럼 right divider 드래그는
					//HDN_TRACK 을 발화시키지 않고 HDN_ITEMCHANGED 로만 width 변경을 알린다. 따라서
					//마지막 컬럼 drag 실시간 반영을 위해 ITEMCHANGED 도 받아야 한다. OnPaint 가 더블 버퍼
					//상태라 중복 sync 호출에도 flicker 없음.
					sync_scrollbar();
					break;
				}
			}
		}
	}
	return CListCtrl::OnNotify(wParam, lParam, pResult);
}

void CVtListCtrlEx::OnSize(UINT nType, int cx, int cy)
{
	CListCtrl::OnSize(nType, cx, cy);

	// TODO: 여기에 메시지 처리기 코드를 추가합니다.
	CRect rc;
	GetClientRect(rc);

	if (m_button_scroll_to_end)
	{
		m_button_scroll_to_end->MoveWindow(CRect(rc.right - 5 - m_auto_scroll_button_size, rc.bottom - 5 - m_auto_scroll_button_size, rc.right - 5, rc.bottom - 5));
	}

	//특정 컬럼 너비를 고정 처리
	if (m_fixed_width_column >= 0 && m_fixed_width_column < get_column_count())
	{
		std::vector<int> width(get_column_count());
		int total_column_width = 0;
		for (int i = 0; i < get_column_count(); i++)
		{
			if (i != m_fixed_width_column)
				total_column_width += GetColumnWidth(i);
		}
		set_column_width(m_fixed_width_column, rc.Width() - total_column_width - 2);
	}

	sync_scrollbar();

	//WS_CLIPCHILDREN + 자식 스크롤바 환경에서 대각 resize 로 새로 확장된 client 영역이
	//listctrl native invalidate 만으로는 stale pixel 남음 — 부모가 그릴 영역 강제 invalidate.
	if (m_scrollbar_setup)
		Invalidate(FALSE);
}

void CVtListCtrlEx::OnWindowPosChanged(WINDOWPOS* lpwndpos)
{
	CListCtrl::OnWindowPosChanged(lpwndpos);

	//하단 remainder 예약으로 client 높이는 itemH 배수 단위로만 바뀐다 → sub-row 만큼 window 가 커질 땐 client 가
	//안 변해 WM_SIZE 가 안 떠 sync 가 호출되지 않는다(스크롤바가 window 를 못 따라옴). window 크기 변화에서도
	//sync 를 돌려 바가 매 순간 window 에 맞게 따라오게 한다. m_syncing 으로 framechange 재진입은 차단.
	//
	//*위치(move)만 바뀌어도* sync 해야 한다 — overlay 스크롤바는 부모 dialog 의 child 라 sync_scrollbar 가 listctrl 의
	//현재 window 위치(ClientToScreen)로 매번 바를 재배치한다. CResizeCtrl 가 리스트를 *크기변경 없이 이동*만 시키는
	//경우(예: 하단 anchor 의 즐겨찾기 리스트, move 0/100/0/0)는 SWP_NOSIZE 가 켜져 예전 조건(!SWP_NOSIZE)으로는
	//sync 가 누락되어 바가 안 따라왔다(클릭 시에야 제자리로 점프). 따라서 size *또는* move 변화 모두에서 sync 한다.
	//(size·move 둘 다 억제된 순수 z-order/framechange 만 제외 → sync_scrollbar 의 SWP_NOMOVE|SWP_NOSIZE framechange 재진입 무해.)
	if (m_scrollbar_setup && lpwndpos && !m_syncing &&
		!((lpwndpos->flags & SWP_NOSIZE) && (lpwndpos->flags & SWP_NOMOVE)))
		sync_scrollbar();
}

void CVtListCtrlEx::show_auto_scroll_button(bool show)
{
	m_show_auto_scroll_button = show;

	if (!m_button_scroll_to_end)
	{
		init_auto_scroll_button();
	}
}

void CVtListCtrlEx::set_auto_scroll(int auto_scroll)
{
	if (auto_scroll == -1)
		m_auto_scroll = !m_auto_scroll;
	else
		m_auto_scroll = auto_scroll;

	if (m_button_scroll_to_end)
		m_button_scroll_to_end->ShowWindow(m_auto_scroll ? SW_HIDE: SW_SHOW);
}

LRESULT CVtListCtrlEx::on_message_CGdiButton(WPARAM wParam, LPARAM lParam)
{
	auto msg = (CGdiButtonMessage*)wParam;

	if (msg->message == WM_LBUTTONUP)
	{
		if (msg->pThis == m_button_scroll_to_end)
		{
			ensure_visible(m_list_db.size() - 1, visible_last);
			set_auto_scroll(true);
		}
	}

	return 0;
}

void CVtListCtrlEx::init_auto_scroll_button()
{
	//우측 하단에 auto_scroll 버튼을 표시한다.
	CRect r(0, 0, m_auto_scroll_button_size, m_auto_scroll_button_size);
	r.DeflateRect(4, 4);

	CSCGdiplusBitmap img(m_auto_scroll_button_size, m_auto_scroll_button_size);
	Gdiplus::Graphics g(img.m_pBitmap);
	Gdiplus::Pen pen(Gdiplus::Color::RoyalBlue, 2.0f);
	pen.SetLineCap(Gdiplus::LineCapRound, Gdiplus::LineCapRound, Gdiplus::DashCapRound);

	g.DrawLine(&pen, r.CenterPoint().x, r.top + 2, r.CenterPoint().x, r.bottom);
	g.DrawLine(&pen, r.left, r.bottom + 1, r.right, r.bottom + 1);
	g.DrawLine(&pen, r.CenterPoint().x, r.bottom, r.CenterPoint().x - 6, r.bottom - 6);
	g.DrawLine(&pen, r.CenterPoint().x - 1, r.bottom, r.CenterPoint().x + 5, r.bottom - 6);

	img.set_alpha(128);

	m_button_scroll_to_end = new CGdiButton();
	m_button_scroll_to_end->create(_T("end"), WS_CHILD | BS_PUSHLIKE, CRect(0, 0, m_auto_scroll_button_size, m_auto_scroll_button_size), this, WM_USER + 3872);
	m_button_scroll_to_end->add_image(&img);
}

LRESULT	CVtListCtrlEx::on_message_CHeaderCtrlEx(WPARAM wParam, LPARAM lParam)
{
	//CHeaderCtrlEx::OnLButtonDblClk 가 분리자 더블클릭 시 SendMessage(Message_CHeaderCtrlEx, column, header_text_width).
	//기존 로직은 GetStringWidth(row text) 만으로 max 산출 + 컬럼0 의 *small image 아이콘 폭 미반영* —
	//사용자 보고 "아이콘 너비가 반영 안 된 듯한 너비, 항목이 일부 안 보이고 말줄임표" 의 직접 원인.
	//get_column_max_text_width 는 헤더+row text+icon+padding 모두 포함하므로 그것을 사용 (단일 진입점).
	int column = (int)wParam;
	int passed_header_w = (int)lParam;
	int computed = get_column_max_text_width(column);
	int new_w = max(passed_header_w, computed);

	set_column_width(column, new_w);

	return 0;
}

void CVtListCtrlEx::OnNcPaint()
{
	//기본 테두리 색상을 별도로 지정한 색으로 그리기 위해 OnNcPaint() 함수를 override.
	//아래 CListCtrl::OnNcPaint();를 호출하지 않으면 스크롤바 등의 일부 영역이 제대로 그려지지 않게 되므로 반드시 기본 핸들러 호출 필요.
	CListCtrl::OnNcPaint();

	//[self-heal] 임베드(child 로 심어 생성 타이밍이 다른 경우) 등으로 PreSubclassWindow 의 frame change 가
	//WM_NCCALCSIZE 를 못 띄워 1px NC 가 미확보된 채 paint 되는 경우 — 표시 시점(여기, 실제 window 상태)에서 frame
	//change 를 1회 강제한다. 그러면 이번엔 OnNcCalcSize 가 불려 1px 를 확보(m_border_nc_reserved=true)하고, 곧바로
	//재진입한 OnNcPaint 가 정상 그린다. 정상 list 는 PreSubclass 에서 이미 확보(reserved=true)라 이 분기에 안 들어감.
	if (m_draw_border && !m_border_nc_reserved && !m_border_recalc_tried)
	{
		m_border_recalc_tried = true;
		SetWindowPos(NULL, 0, 0, 0, 0,
			SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE | SWP_FRAMECHANGED);
		return;
	}

	CWindowDC dc(this);

	//가로바 출현 시 OnNcCalcSize 가 예약한 하단 gw NC 띠. 가로바(dialog child)는 [0, content_view_w]만 덮고
	//그 우측 gw 코너(세로바 바로 아래)는 listctrl NC 영역으로 남아 stale 픽셀이 보인다. 띠 전체를 cr_back 으로
	//채우면 가로바가 좌측을 덮고 코너는 listctrl 배경색으로 남는다. (가로/세로바가 만나는 코너 = 컨트롤 배경색.)
	//하단 NC 띠 = 가로바 gw(있을 때) + 세로바 시 예약한 partial row remainder(m_bottom_reserve). 둘 다 cr_back 으로
	//채워 — 가로바/세로바가 그 위에 그려지고, 마지막 행 아래 gap 과 코너는 listctrl 배경색으로 남는다.
	int bottom_band = (m_h_visible_state ? m_scrollbar.get_width() : 0) + m_bottom_reserve;
	if (bottom_band > 0)
	{
		int b = m_draw_border ? 1 : 0;
		CRect rcWin;
		GetWindowRect(&rcWin);
		rcWin.OffsetRect(-rcWin.TopLeft());

		CRect band(b, rcWin.Height() - b - bottom_band, rcWin.Width() - b, rcWin.Height() - b);
		CBrush br(m_theme.cr_back.ToCOLORREF());
		dc.FillRect(&band, &br);
	}

	//PreSubclassWindow 에서 WS_BORDER/WS_EX_CLIENTEDGE 를 제거하고 m_draw_border 로 옮겨놨다 — GetStyle 직접 검사는 작동 안 함.
	if (m_draw_border)
	{
		CRect rc;
		GetWindowRect(&rc);
		rc.OffsetRect(-rc.TopLeft());
		draw_rect(&dc, rc, m_theme.cr_border_inactive);
	}
}

void CVtListCtrlEx::OnStyleChanging(int nStyleType, LPSTYLESTRUCT lpStyleStruct)
{
	//CSCScrollbar overlay 가 native scrollbar 를 대체. native 가 overflow 감지 시 WS_HSCROLL/WS_VSCROLL 을 다시
	//붙이려 함 → 여기서 강제 strip. WS_HSCROLL 이 살아있으면 native 가 자기 hsb 만 예약하고 OnNcCalcSize 의 하단 NC
	//예약을 무시해 두꺼운 가로바가 마지막 항목을 가린다(CSCTreeCtrl 동일 패턴). 가로 스크롤 시 헤더는 수동 동기화.
	//WS_VSCROLL 은 유지(세로 bottom-align 위해). WS_HSCROLL 만 strip.
	if (m_scrollbar_setup && nStyleType == GWL_STYLE && lpStyleStruct)
		lpStyleStruct->styleNew &= ~WS_HSCROLL;

	CListCtrl::OnStyleChanging(nStyleType, lpStyleStruct);
}

void CVtListCtrlEx::OnNcCalcSize(BOOL bCalcValidRects, NCCALCSIZE_PARAMS* lpncsp)
{
	//CSCScrollbar overlay 모드에선 native scrollbar NC 영역 미할당 — base 호출 안 해 client = window.
	if (m_scrollbar_setup && lpncsp)
	{
		//단 m_draw_border 시 자체 border 그릴 1px 공간만 확보 (4변 각 1px).
		if (m_draw_border && bCalcValidRects)
		{
			lpncsp->rgrc[0].left   += 1;
			lpncsp->rgrc[0].top    += 1;
			lpncsp->rgrc[0].right  -= 1;
			lpncsp->rgrc[0].bottom -= 1;
			m_border_nc_reserved = true;	//1px 확보됨 — OnNcPaint 의 self-heal 강제 recalc 불필요.
		}
		//세로바 보이면 우측 gw, 가로바 보이면 하단 gw 를 NC 로 예약 → native content 영역 = 실제 가시 영역.
		//그러면 native 가 컬럼을 content 폭까지만 그리고(마지막 divider 가 세로바 밑에 안 가림), 가로 scroll 한계도
		//content 폭 기준으로 정확히 잡아 가려진 부분을 스크롤로 꺼낼 수 있다. WS_VSCROLL/HSCROLL strip(OnStyleChanging)
		//이 전제 — 비트가 살아있으면 native 가 자기 바만 보고 NC 무시. 바는 이 예약 띠에 dialog-child 로 놓인다.
		int gw = m_scrollbar.get_width();
		if (m_v_visible_state && bCalcValidRects &&
			lpncsp->rgrc[0].right - lpncsp->rgrc[0].left > gw)
			lpncsp->rgrc[0].right -= gw;
		if (m_h_visible_state && bCalcValidRects &&
			lpncsp->rgrc[0].bottom - lpncsp->rgrc[0].top > gw)
			lpncsp->rgrc[0].bottom -= gw;

		//세로바가 있을 때 하단의 partial row(remainder)를 NC 로 추가 예약 → native 의 item 영역이 itemH 의 정확한
		//배수가 되어 nPage 의 ceil==floor. 그러면 native 가 nPage 를 어떻게 재계산하든 max scroll 이 마지막 항목을
		//완전히 보여준다(끝 항목 잘림·thumb 튐의 근본 해결). 예약한 remainder 는 마지막 행 아래의 빈 영역(cr_back).
		m_bottom_reserve = 0;
		if (m_v_visible_state && bCalcValidRects && m_line_height > 0)
		{
			int hdr = get_header_height();
			int item_area = (lpncsp->rgrc[0].bottom - lpncsp->rgrc[0].top) - hdr;
			if (item_area > m_line_height)
			{
				int rem = item_area % m_line_height;
				if (rem > 0)
				{
					lpncsp->rgrc[0].bottom -= rem;
					m_bottom_reserve = rem;
				}
			}
		}
		return;
	}

	CListCtrl::OnNcCalcSize(bCalcValidRects, lpncsp);
}

void CVtListCtrlEx::setup_scrollbar()
{
	if (m_scrollbar_setup || !::IsWindow(m_hWnd))
		return;

	m_scrollbar_setup = true;

	//WS_HSCROLL strip — native 가 살려두면 OnNcCalcSize 하단 NC 예약을 무시(자기 hsb 만 봄)해 두꺼운
	//가로바가 마지막 항목을 가린다(CSCTreeCtrl 동일). OnStyleChanging 이 재부착도 차단. 가로 스크롤 시 헤더는
	//수동 동기화(sync_header_h_offset). native scrollbar 시각화는 OnNcCalcSize override 가 단독 차단.
	//WS_VSCROLL 은 유지 — strip 하면 native 가 세로 max scroll 을 top-align(첫 항목 flush)에서 클램프해
	//마지막 항목이 항상 partial 로 잘린다. 유지하면 native 가 정상 bottom-align(마지막 항목 flush)한다.
	//WS_CLIPSIBLINGS 추가 — overlay 스크롤바는 부모 dialog 의 child(=listctrl 의 형제)다. 이 비트가 없으면
	//listctrl 자신의 paint(예: set_color_theme 의 RedrawWindow)가 형제 바 영역까지 그려 덮는다 → 테마 변경 시
	//바가 사라지고 resize/hover 전까지 안 보이던 증상. 켜면 listctrl paint 가 visible 한 형제 바를 자동 clip.
	ModifyStyle(WS_HSCROLL, WS_CLIPCHILDREN | WS_CLIPSIBLINGS, SWP_FRAMECHANGED);

	//빈 영역 cr_back 처리 — native 가 자체 erase 시 사용.
	SetBkColor(m_theme.cr_back.ToCOLORREF());
	SetTextBkColor(m_theme.cr_back.ToCOLORREF());

	//세로바·가로바 모두 *부모 dialog 의 child*. NC 로 예약한 우측/하단 띠(=client 밖)에 놓이므로 listctrl child 면
	//클리핑된다. 위치는 sync_scrollbar 가 parent 좌표로, 통지는 set_message_target(this) 로 회수.
	CWnd* pParent = GetParent();
	CWnd* pHost   = pParent ? pParent : (CWnd*)this;

	//부모가 자기 client 를 repaint(예: 리사이즈 시 CSCThemeDlg::OnSize 의 Invalidate)할 때, child 인 스크롤바 overlay
	//위를 덮어 그려 리사이즈 중 바가 사라지는 회귀(자식이 늦게 repaint 돼 hover 전까지 안 보임)를 막는다.
	//WS_CLIPCHILDREN 을 부모에 부여하면 부모 paint 가 child window 영역을 제외 → 바가 덮이지 않는다.
	if (pParent)
		pParent->ModifyStyle(0, WS_CLIPCHILDREN);

	m_scrollbar.create(pHost, CSCScrollbar::vertical,
		0, 0, m_scrollbar.get_width(), 10);
	m_scrollbar.ModifyStyle(0, WS_CLIPSIBLINGS);
	m_scrollbar.set_message_target(this);
	m_scrollbar.set_color_theme(m_theme, false);
	m_scrollbar.set_line(3);		//화살표 클릭 = 3 라인 (휠 single notch 와 동일).
	m_scrollbar.ShowWindow(SW_HIDE);

	m_scrollbar_h.create(pHost, CSCScrollbar::horizontal,
		0, 0, 10, m_scrollbar_h.get_width());
	m_scrollbar_h.ModifyStyle(0, WS_CLIPSIBLINGS);
	m_scrollbar_h.set_message_target(this);
	m_scrollbar_h.set_color_theme(m_theme, false);
	m_scrollbar_h.set_line(30);		//화살표 클릭 = 30px (average char width ~ 7 의 약 4 배).
	m_scrollbar_h.ShowWindow(SW_HIDE);

	//header 와 overlay 의 sibling clipping — header 가 paint 시 자기 rect 전체를 그리면 overlay 위까지
	//덮어쓰는 깨짐(image #5)이 발생. WS_CLIPSIBLINGS 가 켜지면 자기보다 z-order 위인 sibling 의
	//visible region 이 자동 clip 되어 침범 차단. PreSubclassWindow 시점에 native header 는 이미 존재.
	if (CHeaderCtrl* hdr = GetHeaderCtrl())
		hdr->ModifyStyle(0, WS_CLIPSIBLINGS);
	m_scrollbar.ModifyStyle(0, WS_CLIPSIBLINGS);
	m_scrollbar_h.ModifyStyle(0, WS_CLIPSIBLINGS);

	//overlay 의 z-order 를 header 위로 강제. created order(나중 생성 = top) 만 의존하지 않고 명시 보증.
	m_scrollbar.SetWindowPos(&CWnd::wndTop, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
	m_scrollbar_h.SetWindowPos(&CWnd::wndTop, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
}

void CVtListCtrlEx::begin_bulk_insert()
{
	m_in_bulk_insert = true;
	SetRedraw(FALSE);
}

void CVtListCtrlEx::end_bulk_insert()
{
	m_in_bulk_insert = false;
	//bulk 중 생략했던 가상리스트 항목수를 한 번에 반영.
	if (m_use_virtual_list)
		SetItemCountEx(m_list_db.size(), LVSICF_NOSCROLL);
	SetRedraw(TRUE);
	sync_scrollbar();
	Invalidate();
}

void CVtListCtrlEx::sync_scrollbar()
{
	if (!m_scrollbar_setup || !::IsWindow(m_scrollbar.m_hWnd))
		return;

	//native scrollbar 의 시각화는 setup_scrollbar 의 SWP_FRAMECHANGED + OnNcCalcSize(NC=0) 가
	//영구 차단 — sync 마다 ShowScrollBar 토글은 무용하면서 OS 가 NCCALCSIZE/paint 사이클을 발화시켜
	//컬럼 폭 드래그 중 변경 컬럼이 추가 repaint 되는 flicker 원인. 따라서 sync 진입부에선 호출 X.

	int total = GetItemCount();

	CRect rc;
	GetClientRect(&rc);

	//visible(세로 가시 항목 수): 커스텀 헤더 height(get_header_height) 와 실제 행 높이로 직접 계산한다.
	//GetCountPerPage 는 커스텀 헤더(CHeaderCtrlEx, set_header_height) 높이를 반영하지 못해 세로 스크롤바
	//출현 시점이 헤더 높이만큼 어긋난다.
	int total_col_width = 0;
	int col_count = get_column_count();
	for (int i = 0; i < col_count; i++)
		total_col_width += GetColumnWidth(i);

	int gw     = m_scrollbar.get_width();
	int header = get_header_height();

	//세로바·가로바 필요 여부는 상호의존적이다 (둘 다 NC 로 content 영역을 줄인다):
	// - 세로바: 우측 gw 를 NC 예약 → 가용 컬럼 폭이 gw 만큼 줄어 가로바를 유발.
	// - 가로바: 하단 gw 를 NC 예약 → 가시 행수가 줄어 세로바를 유발.
	//따라서 둘을 수렴 반복으로 함께 푼다. 진입 rc 는 현재 NC 상태(현재 m_v/h_visible_state)를 반영하므로,
	//바가 없을 때의 full 치수를 복원해 기준으로 둔다.
	int full_w = rc.Width()  + (m_v_visible_state ? gw : 0);
	int full_h = rc.Height() + (m_h_visible_state ? gw : 0);

	bool need_v = false;
	bool need_h = false;
	for (int pass = 0; pass < 3; pass++)
	{
		int usable_h = full_h - (need_h ? gw : 0);
		int avail = (m_line_height > 0) ? max(0, (usable_h - header) / m_line_height) : GetCountPerPage();
		bool nv = total > avail;
		int usable_w = full_w - (nv ? gw : 0);
		bool nh = (col_count > 0) && (total_col_width > usable_w);
		if (nv == need_v && nh == need_h)
			break;
		need_v = nv;
		need_h = nh;
	}

	//need_v / need_h 상태가 바뀌면 framechange 로 우측/하단 NC 재적용. 두 플래그는 OnNcCalcSize 가 읽으므로
	//framechange *전* 에 갱신. 창 크기 불변 → 리사이즈 헬퍼 충돌·렌더 깨짐 없음.
	bool old_v = m_v_visible_state;
	bool old_h = m_h_visible_state;
	m_v_visible_state = need_v;
	m_h_visible_state = need_h;
	if ((need_v != old_v || need_h != old_h) && !m_syncing)
	{
		m_syncing = true;
		SetWindowPos(NULL, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE | SWP_FRAMECHANGED);
		m_syncing = false;
		GetClientRect(&rc);	//NC 적용된 content 영역(우측/하단 gw 빠짐)

		//전환 순간 화면갱신 보강 — NC 띠가 parent↔listctrl 사이를 오가고 바가 재배치되며, 우하단 코너(두 띠가
		//만나는 곳)는 어느 바도 안 덮는다. self + parent 의 우리 window 영역 전체를 invalidate 해 stale 제거.
		Invalidate(FALSE);
		if (CWnd* pParent = GetParent())
		{
			CRect rWin;
			GetWindowRect(&rWin);
			pParent->ScreenToClient(&rWin);
			pParent->InvalidateRect(&rWin, TRUE);
		}
	}

	//최종 content 영역 기준 — NC 가 이미 우측/하단 gw 를 뺐으므로 rc 가 곧 가시 영역.
	//가로 폭 단일 출처(회귀 방지): content_view_w = rc.Width(). need_h 판정·가로바 길이·max scroll·page·range 가 모두 사용.
	int content_view_w = rc.Width();
	int avail_rows = (m_line_height > 0) ? max(0, (rc.Height() - header) / m_line_height) : GetCountPerPage();
	int visible = avail_rows;

	//세로 scrollbar — 우측 NC 띠(parent child). [rc.right, rc.right+gw] × content 높이를 parent 좌표로 배치.
	if (::IsWindow(m_scrollbar.m_hWnd))
	{
		CPoint strip_tl(rc.right, rc.top);
		ClientToScreen(&strip_tl);
		//rc.bottom 은 remainder 예약으로 마지막 행 바로 아래까지만. 세로바는 그 아래 gap(remainder)까지 덮어 가로바 윗변에 닿게 연장.
		CRect rTargetV(strip_tl.x, strip_tl.y, strip_tl.x + gw, strip_tl.y + rc.Height() + m_bottom_reserve);
		CRect rCurV;
		m_scrollbar.GetWindowRect(&rCurV);
		if (CWnd* pParent = GetParent())
		{
			pParent->ScreenToClient(&rTargetV);
			pParent->ScreenToClient(&rCurV);
		}
		if (rCurV != rTargetV)
			m_scrollbar.MoveWindow(rTargetV);
	}

	//가로 scrollbar — 하단 NC 띠(parent child). [rc.left, rc.bottom] × (content 폭 × gw)를 parent 좌표로 배치.
	if (::IsWindow(m_scrollbar_h.m_hWnd))
	{
		//rc.bottom 위에 remainder gap 이 있으므로 가로바는 그만큼 더 내려 window 바닥(gap 아래)에 놓는다.
		CPoint band_tl(rc.left, rc.bottom + m_bottom_reserve);
		ClientToScreen(&band_tl);
		CRect rTargetH(band_tl.x, band_tl.y, band_tl.x + content_view_w, band_tl.y + gw);
		CRect rCurH;
		m_scrollbar_h.GetWindowRect(&rCurH);
		if (CWnd* pParent = GetParent())
		{
			pParent->ScreenToClient(&rTargetH);
			pParent->ScreenToClient(&rCurH);
		}
		if (rCurH != rTargetH)
			m_scrollbar_h.MoveWindow(rTargetH);
	}

	//IsWindowVisible() 는 부모 chain 의 WS_VISIBLE 까지 검사 — OnInitDialog 안에서는
	//dialog 가 아직 SW_SHOW 전이라 false 반환 → toggle 가드용으로 쓰면 자식의 WS_VISIBLE
	//비트가 켜진 채로 hide 가 스킵되어 dialog 가 보이는 순간 overlay 가 그대로 남는다.
	//ShowWindow 는 같은 상태면 no-op (메시지 미발송) 이므로 무조건 호출이 정석.
	if (!need_v)
	{
		m_scrollbar.ShowWindow(SW_HIDE);
	}
	else
	{
		int top = GetTopIndex();
		if (top < 0) top = 0;
		int page = visible;
		int max_pos = (total - 1) - page + 1;
		//마지막 항목이 content 바닥 안에 완전히 들어왔으면(=바닥까지 스크롤됨) thumb 은 GetTopIndex 대신 max_pos 로 pin —
		//트랙 끝에 닿게 하고 튐 제거. 그 외엔 GetTopIndex 그대로.
		int thumb_pos = top;
		CRect rLast;
		if (total > 0 && GetItemRect(total - 1, &rLast, LVIR_BOUNDS) && rLast.bottom <= rc.Height())
			thumb_pos = max_pos;
		m_scrollbar.set_range(0, total - 1);
		m_scrollbar.set_page(page);
		m_scrollbar.set_pos(thumb_pos);
		m_scrollbar.ShowWindow(SW_SHOW);
		//parent dialog 의 child 라 형제인 listctrl 위로 z-order 올려야 보인다.
		m_scrollbar.SetWindowPos(&CWnd::wndTop, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
	}

	if (!need_h)
	{
		m_scrollbar_h.ShowWindow(SW_HIDE);
		m_h_scroll_pos = 0;
	}
	else
	{
		//TODO(미해결): 가로 끝까지 스크롤해도 마지막 컬럼 우측 구분자가 v-bar 밑으로 ~native_margin(측정 5px) 가린다.
		//원인 — native 가로 스크롤 viewport = 헤더 폭이고, 헤더가 그리기 client 보다 그만큼 넓다. (technical_memo 참조.)
		int max_h = max(0, total_col_width - content_view_w);
		if (m_h_scroll_pos > max_h)
			m_h_scroll_pos = max_h;
		if (m_h_scroll_pos < 0)
			m_h_scroll_pos = 0;
		int total_h_range = max_h + content_view_w;
		m_scrollbar_h.set_range(0, total_h_range - 1);
		m_scrollbar_h.set_page(content_view_w);
		m_scrollbar_h.set_pos(m_h_scroll_pos);
		m_scrollbar_h.ShowWindow(SW_SHOW);
		//parent dialog 의 child 라 형제인 listctrl 위로 z-order 올려야 보인다.
		m_scrollbar_h.SetWindowPos(&CWnd::wndTop, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
		//drag-during 에는 listview 의 paint cycle 이 멈춰 있어 자식 ShowWindow(SW_SHOW) 가 invalidate
		//만 마크하고 즉시 그려지지 않는다 (release 후에야 보이는 현상). 즉시 paint 강제.
		m_scrollbar_h.RedrawWindow(NULL, NULL, RDW_INVALIDATE | RDW_UPDATENOW);
	}

}

LRESULT CVtListCtrlEx::on_message_CSCScrollbar(WPARAM wParam, LPARAM lParam)
{
	CSCScrollbarMsg* msg = (CSCScrollbarMsg*)wParam;
	if (msg == nullptr)
		return 0;

	if (msg->msg != CSCScrollbarMsg::msg_scrollbar_pos_changed)
		return 0;

	m_last_user_scroll_at = GetTickCount();

	if (msg->pThis == &m_scrollbar)
	{
		//msg->pos 는 이미 scrollbar 가 max_pos(=total - sync 에서 계산한 visible) 로 클램프해 보낸다. 여기서
		//GetCountPerPage 로 다시 클램프하면 가로바·헤더 차감을 반영 못해 new_top 이 강제로 작아져, 끝까지
		//드래그해도 마지막 항목이 보이지 않는다. msg->pos 를 그대로 쓰고 total-1 안전 클램프만 둔다.
		int total = GetItemCount();
		int new_top = max(0, min(msg->pos, total - 1));
		int cur_top = GetTopIndex();

		CRect rc;
		GetClientRect(&rc);
		int header = m_HeaderCtrlEx.GetSafeHwnd() ? get_header_height() : 0;
		int visible = (m_line_height > 0) ? max(0, (rc.Height() - header) / m_line_height) : GetCountPerPage();
		int max_top = max(0, total - visible);

		if (new_top >= max_top && total > 0)
		{
			//바닥 — Scroll()/EnsureVisible 은 item-aligned(첫 항목 flush)에서 멈춰 마지막 항목이 잘린다.
			//native 자체 스크롤바 메시지 SB_BOTTOM 은 bottom-align(마지막 항목 flush)한다. WS_VSCROLL 유지가 전제.
			SendMessage(WM_VSCROLL, MAKEWPARAM(SB_BOTTOM, 0), 0);
		}
		else
		{
			if (new_top != cur_top && total > 0)
			{
				CRect rItem;
				if (GetItemRect(0, &rItem, LVIR_BOUNDS))
				{
					int item_h = rItem.Height();
					if (item_h > 0)
						Scroll(CSize(0, (new_top - cur_top) * item_h));
				}
			}

			//pixel 정렬 보정 — ensure_visible(visible_center) 등이 top 한계를 넘는 Scroll 을 호출하면
			//listctrl 이 부분 스크롤로 끝내 item 0 top y 가 header_bottom 와 어긋난 채 정착할 수 있음.
			//top_index 가 같아 위 Scroll 이 발화 안 해도 잔여 offset 을 여기서 보정.
			CRect rItem;
			if (GetItemRect(new_top, &rItem, LVIR_BOUNDS))
			{
				int dy = rItem.top - header;
				if (dy != 0)
					Scroll(CSize(0, dy));
			}
		}
	}
	else if (msg->pThis == &m_scrollbar_h)
	{
		int new_pos = max(0, msg->pos);
		int delta = new_pos - m_h_scroll_pos;
		if (delta != 0)
		{
			//Scroll() 이 동기적으로 LVN_ENDSCROLL → sync_scrollbar 호출 → m_h_scroll_pos 가 stale 이면 thumb reset.
			//그래서 Scroll() 전에 새 값으로 갱신.
			m_h_scroll_pos = new_pos;
			Scroll(CSize(delta, 0));
			UpdateWindow();
		}
	}
	return 0;
}

BOOL CVtListCtrlEx::OnMouseWheel(UINT nFlags, short zDelta, CPoint pt)
{
	if (m_scrollbar_setup && GetItemCount() > 0)
	{
		m_last_user_scroll_at = GetTickCount();
		bool shift = (nFlags & MK_SHIFT) != 0;
		if (shift)
		{
			int dx = -zDelta / WHEEL_DELTA * 60;
			if (dx == 0)
				dx = (zDelta > 0) ? -60 : 60;
			m_h_scroll_pos += dx;
			Scroll(CSize(dx, 0));
			UpdateWindow();
			sync_scrollbar();
			return TRUE;
		}

		int lines = -zDelta / WHEEL_DELTA * 3;
		if (lines == 0)
			lines = (zDelta > 0) ? -1 : 1;

		CRect rItem;
		if (GetItemRect(0, &rItem, LVIR_BOUNDS))
		{
			int item_h = rItem.Height();
			if (item_h > 0)
			{
				int total = GetItemCount();
				CRect rc;
				GetClientRect(&rc);
				int header = m_HeaderCtrlEx.GetSafeHwnd() ? get_header_height() : 0;
				int visible = (m_line_height > 0) ? max(0, (rc.Height() - header) / m_line_height) : GetCountPerPage();
				int max_top = max(0, total - visible);
				int target_top = GetTopIndex() + lines;

				if (lines > 0 && target_top >= max_top)
					SendMessage(WM_VSCROLL, MAKEWPARAM(SB_BOTTOM, 0), 0);	//바닥 — native bottom-align 시도
				else
					Scroll(CSize(0, lines * item_h));
				UpdateWindow();
				sync_scrollbar();
				return TRUE;
			}
		}
	}
	return CListCtrl::OnMouseWheel(nFlags, zDelta, pt);
}

void CVtListCtrlEx::OnMouseHWheel(UINT nFlags, short zDelta, CPoint pt)
{
	if (m_scrollbar_setup)
	{
		m_last_user_scroll_at = GetTickCount();
		int dx = zDelta / WHEEL_DELTA * 60;
		if (dx == 0)
			dx = (zDelta > 0) ? 60 : -60;
		m_h_scroll_pos += dx;
		Scroll(CSize(dx, 0));
		UpdateWindow();
		sync_scrollbar();
		return;
	}
	CListCtrl::OnMouseHWheel(nFlags, zDelta, pt);
}
