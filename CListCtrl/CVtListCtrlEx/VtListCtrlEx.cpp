// VtListCtrlEx.cpp : implementation file
//

//#include "stdafx.h"
#include "VtListCtrlEx.h"

#include <algorithm>
#include <set>

#include <locale>

#include "../../colors.h"
#include "../../MemoryDC.h"
#include "../../CEdit/SCEdit/SCEdit.h"
#include <afxvslistbox.h>

#define IDC_EDIT_CELL	1001

// CVtListCtrlEx

IMPLEMENT_DYNAMIC(CVtListCtrlEx, CListCtrl)

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
	safe_release_gradient_rect_handle();

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
	ON_NOTIFY_REFLECT(LVN_BEGINLABELEDIT, &CVtListCtrlEx::OnLvnBeginlabeledit)
	ON_NOTIFY_REFLECT_EX(LVN_ENDLABELEDIT, &CVtListCtrlEx::OnLvnEndlabeledit)
	ON_NOTIFY_REFLECT_EX(NM_DBLCLK, &CVtListCtrlEx::OnNMDblclk)
	ON_NOTIFY_REFLECT_EX(NM_CLICK, &CVtListCtrlEx::OnNMClick)
	ON_WM_DROPFILES()
	ON_WM_MEASUREITEM_REFLECT()
	ON_NOTIFY_REFLECT(LVN_BEGINDRAG, &CVtListCtrlEx::OnLvnBeginDrag)
	ON_WM_MOUSEMOVE()
	ON_WM_LBUTTONUP()
	ON_NOTIFY_REFLECT_EX(LVN_ITEMCHANGED, &CVtListCtrlEx::OnLvnItemchanged)
	//ON_WM_CONTEXTMENU()
	ON_WM_MOUSEHOVER()
	ON_WM_MOUSELEAVE()
	ON_WM_SETFOCUS()
	ON_WM_KILLFOCUS()
	ON_REGISTERED_MESSAGE(Message_CSCEdit, &CVtListCtrlEx::on_message_CSCEdit)
	ON_NOTIFY_REFLECT_EX(LVN_ITEMCHANGING, &CVtListCtrlEx::OnLvnItemchanging)
	ON_WM_LBUTTONDOWN()
	ON_WM_NCHITTEST()
	ON_WM_HSCROLL()
	ON_WM_VSCROLL()
	ON_NOTIFY_REFLECT(LVN_BEGINSCROLL, &CVtListCtrlEx::OnLvnBeginScroll)
	ON_NOTIFY_REFLECT(LVN_ENDSCROLL, &CVtListCtrlEx::OnLvnEndScroll)
	ON_WM_SIZE()
	ON_REGISTERED_MESSAGE(Message_CGdiButton, &CVtListCtrlEx::on_message_CGdiButton)
END_MESSAGE_MAP()



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
		text = m_list_db[pItem->iItem].text[pItem->iSubItem];

		//Copy the text to the LV_ITEM structure
		//Maximum number of characters is in pItem->cchTextMax
		lstrcpyn(pItem->pszText, text, pItem->cchTextMax);
	}
	/*
	//Do the list need image information?
	if(pItem->mask & LVIF_IMAGE) 
	{
		//Set which image to use
		pItem->iImage=*m_list_db[itemid].m_image;

		//Show check box?
		if(IsCheckBoxesVisible())
		{
			//To enable check box, we have to enable state mask...
			pItem->mask |= LVIF_STATE;
			pItem->stateMask = LVIS_STATEIMAGEMASK;

			if(m_database[itemid].m_checked)
			{
				//Turn check box on..
				pItem->state = INDEXTOSTATEIMAGEMASK(2);
			}
			else
			{
				//Turn check box off
				pItem->state = INDEXTOSTATEIMAGEMASK(1);
			}
		}
	}
	*/
	*pResult = 0;
}


void CVtListCtrlEx::OnLvnOdfinditem(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMLVFINDITEM pFindInfo = reinterpret_cast<LPNMLVFINDITEM>(pNMHDR);
	// TODO: Add your control notification handler code here
	*pResult = 0;
}


void CVtListCtrlEx::OnLvnOdcachehint(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMLVCACHEHINT pCacheHint = reinterpret_cast<LPNMLVCACHEHINT>(pNMHDR);
	// TODO: Add your control notification handler code here
	*pResult = 0;
}


void CVtListCtrlEx::DrawItem(LPDRAWITEMSTRUCT lpDIS/*lpDrawItemStruct*/)
{
	int			iItem		= (int)lpDIS->itemID;
	int			iSubItem;
	CDC			*pDC		= CDC::FromHandle(lpDIS->hDC);
	CRect		rowRect;
	CRect		itemRect;
	CRect		textRect;
	bool		is_show_selection_always = (GetStyle() & LVS_SHOWSELALWAYS);
	//TRACE(_T("is_show_selection_always = %d\n"), is_show_selection_always);
	bool		is_selected = GetItemState(iItem, LVIS_SELECTED);
	bool		is_drophilited = GetItemState(iItem, LVIS_DROPHILITED);
	Gdiplus::Color	crText = m_theme.cr_text;
	Gdiplus::Color	crBack = m_theme.cr_back;

	GetItemRect(iItem, &rowRect, LVIR_BOUNDS);

	for (iSubItem = 0; iSubItem < get_column_count(); iSubItem++)
	{
		//TRACE(_T("in DrawItem. %d, %d\n"), iItem, iSubItem);

		crText = m_list_db[iItem].crText[iSubItem];
		crBack = m_list_db[iItem].crBack[iSubItem];

		//먼저 선택 여부, focus여부 등에 따라 셀이 그려질 글자색, 배경색을 골라주고...

		//LVIR_BOUNDS로 구할 경우 0번 컬럼은 한 라인의 사각형 영역을 리턴한다.
		//0번 컬럼 이후부터는 해당 셀의 사각형 영역만을 리턴한다.
		GetSubItemRect(iItem, iSubItem, LVIR_BOUNDS, itemRect);

		if (iSubItem == 0)
		{
			itemRect.right = itemRect.left + GetColumnWidth(0);
		}

		//if(lpDIS->itemState & ODS_SELECTED) //ok
		//포커스를 가졌거나 Always Show Selection이라면 선택 항목의 색상을 표시해주고
		if ((m_has_focus || is_show_selection_always) && is_selected) //ok
		{
			if (m_has_focus)
			{
				//TRACE(_T("active\n"));
				crText = m_theme.cr_text_selected;
				crBack = m_theme.cr_back_selected;
			}
			else
			{
				//TRACE(_T("inactive\n"));
				crText = m_theme.cr_text_selected_inactive;
				crBack = m_theme.cr_back_selected_inactive;
			}
		}
		//drophilited라면 active에 관계없이 drop hilited 색상으로 표시한다.
		//단 대상 항목이 파일인 경우는 drop hilited 표시를 하지 않는다.
		else if (is_drophilited) //ok
		{
			crText = m_theme.cr_text_selected;
			crBack = m_theme.cr_back_selected;
		}
		else
		{
			if (crText.GetValue() == listctrlex_unused_color.GetValue())
				crText = m_theme.cr_text;
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
		if (m_list_db[iItem].crText[iSubItem].GetValue() != listctrlex_unused_color.GetValue())
			crText = m_list_db[iItem].crText[iSubItem];
		if (m_list_db[iItem].crBack[iSubItem].GetValue() != listctrlex_unused_color.GetValue())
			crBack = m_list_db[iItem].crBack[iSubItem];
	
		pDC->FillSolidRect(itemRect, crBack.ToCOLORREF());

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
			//pDC->FillSolidRect(itemRect, crBack);

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
			//std::deque<COLORREF> dqColor(m_cr_percentage_bar);
			//dqColor.push_back(crBack);
			//gradient_rect(pDC, r, dqColor, false);

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
			//pDC->FillSolidRect(itemRect, crBack);

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
#if 1
					pDC->DrawText(text, itemRect, DT_VCENTER | DT_CENTER | DT_SINGLELINE);

					//for region test
#elif 0
					CRgn rgn;
					CRect r = itemRect;
					//r.DeflateRect(30, 10);
					rgn.CreateRectRgnIndirect(&r);
					int res = ::SelectClipRgn(pDC->GetSafeHdc(), (HRGN)rgn.GetSafeHandle());
					ASSERT(res == SIMPLEREGION);
					pDC->DrawText(sPercent, itemRect, DT_VCENTER | DT_CENTER | DT_SINGLELINE | DT_NOCLIP);
					pDC->TextOut(itemRect.CenterPoint().x, itemRect.CenterPoint().y + 4, sPercent);
					rgn.DeleteObject();
#else
					CRect rcLeft, rcRight;
					rcLeft = rcRight = itemRect;
					rcRight.left = rcLeft.right = r.right;

					CRgn rgnLeft, rgnRight;
					rgnLeft.CreateRectRgnIndirect(&rcLeft);
					rgnRight.CreateRectRgnIndirect(&rcRight);

					pDC->SetTextColor(RGB(255, 0, 0));// m_cr_back);
					pDC->SelectClipRgn(&rgnLeft);
					pDC->DrawText(sPercent, itemRect, DT_VCENTER | DT_CENTER | DT_SINGLELINE);
					//pDC->TextOut(itemRect.CenterPoint().x, itemRect.CenterPoint().y+4, sPercent);

					//rgnRight.SetRectRgn(rcRight);
					pDC->SetTextColor(RGB(0, 0, 255)); //m_cr_text);
					pDC->SelectClipRgn(&rgnRight);
					pDC->DrawText(sPercent, itemRect, DT_VCENTER | DT_CENTER | DT_SINGLELINE);
					//pDC->TextOut(itemRect.CenterPoint().x, itemRect.CenterPoint().y+4, sPercent);

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

			format = format | DT_SINGLELINE | DT_VCENTER | DT_WORD_ELLIPSIS;
			pDC->DrawText(text, textRect, format);
		}
	}

	//선택된 항목은 선택 색상보다 진한 색으로 테두리가 그려진다.
	if (m_draw_selected_border && !m_in_editing && m_has_focus && is_selected)
	{
		GetSubItemRect(iItem, 0, LVIR_BOUNDS, rowRect);
		draw_rectangle(pDC, rowRect, m_theme.cr_selected_border);
	}

	if (m_draw_top_line)
	{
		draw_line(pDC, rowRect.left, rowRect.top, rowRect.right, rowRect.top, m_cr_top_line);
	}

	if (m_draw_bottom_line)
	{
		//rowRect.bottom으로 써주면 아이템 영역밖이므로 그려지지 않는다. 반드시 -1을 해야 함.
		draw_line(pDC, rowRect.left, rowRect.bottom - 1, rowRect.right, rowRect.bottom - 1, m_cr_bottom_line);
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

	modify_style();

	if (m_HeaderCtrlEx && m_HeaderCtrlEx.m_hWnd)
	{
		m_HeaderCtrlEx.m_header_text_align.resize(column);
		m_HeaderCtrlEx.m_header_text_align.assign(column, HDF_LEFT);
	}

	m_allow_edit_column.resize(column);

	return TRUE;
}

CString	CVtListCtrlEx::get_header_text(int column)
{
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
	return 0;
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

	if (invalidate)
		Invalidate();
}

void CVtListCtrlEx::set_header_height(int height)
{
	if (m_HeaderCtrlEx.m_hWnd != NULL)
		m_HeaderCtrlEx.set_header_height(height);
}

void CVtListCtrlEx::set_line_height(int height)
{
	m_line_height = height;
	//reconstruct_font();
	//RedrawItems(0, -1);
	//UpdateWindow();
	//RedrawWindow();

	//자체적으로 imagelist를 사용하지 않는 경우는 아래 코드만으로도
	//height가 적용된다.
	if (!m_use_own_imagelist)
	{
		CImageList gapImage;
		gapImage.Create(1, height, ILC_COLORDDB, 1, 0); //2번째 파라미터로 높이조절.....
		SetImageList(&gapImage, LVSIL_SMALL);
	}
}

void CVtListCtrlEx::set_column_width(int nCol, int cx)
{
	if (nCol >= get_column_count())
		return;

	SetColumnWidth(nCol, cx);
}

void CVtListCtrlEx::load_column_width(CWinApp* pApp, CString sSection)
{
	int		i, width;
	CString str;

	for (i = 0; i < m_HeaderCtrlEx.GetItemCount(); i++)
	{
		width = pApp->GetProfileInt(sSection + _T("\\column width"), i2S(i), 0);
		if (width > 0)
			SetColumnWidth(i, width);
	}
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
		//DWORD dwExStyle = ListView_GetExtendedListViewStyle(GetSafeHwnd());

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
bool CVtListCtrlEx::get_index_from_point(CPoint pt, int& item, int& subItem, bool include_icon)
{
	CRect rc;
	CRect itemRect;
	
	item = subItem = -1;

	// Now scroll if we need to expose the column
	GetClientRect(&rc);

	if (!rc.PtInRect(pt))
		return false;

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
					//imagelist를 사용한다면 이미지 영역만큼 다시 보정해줘야 한다.
					if (!include_icon && m_use_own_imagelist)
						itemRect.left += 22;
				}

				if (itemRect.PtInRect(pt))
				{
					subItem = j;
					return true;
				}
			}

			return true;
		}
	}

	return false;
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
		std::sort(m_list_db.begin(), m_list_db.end(),
			[sort_asc, iSub, data_type, include_null](CListCtrlData a, CListCtrlData b)
			{
				if (sort_asc)
				{
					if (data_type == column_data_type_text)
					{
						if (iSub != 0)
						{
							if (!include_null)
							{
								if (a.text[iSub].IsEmpty() && b.text[iSub].IsEmpty())
									return true;
								else if (a.text[iSub].IsEmpty() && !b.text[iSub].IsEmpty())
									return false;
								else if (!a.text[iSub].IsEmpty() && b.text[iSub].IsEmpty())
									return true;
							}

							if (a.text[iSub].MakeLower() > b.text[iSub].MakeLower())
								return false;
							else if (a.text[iSub].MakeLower() < b.text[iSub].MakeLower())
								return true;
							else
							{
								if (a.text[0].MakeLower() > b.text[0].MakeLower())
									return false;
								else
									return true;
							}
						}
						else
						{
							return (a.text[iSub].MakeLower() < b.text[iSub].MakeLower());
						}
						return (a.text[iSub].MakeLower() < b.text[iSub].MakeLower());
					}
					else if (data_type == column_data_type_text_ip)
					{
						if (iSub != 0)
						{
							if (!include_null)
							{
								return (compare_string(a.text[iSub], b.text[iSub]) == 1);
								//if (a.text[iSub].IsEmpty() && b.text[iSub].IsEmpty())
								//	return true;
								//else if (a.text[iSub].IsEmpty() && !b.text[iSub].IsEmpty())
								//	return false;
								//else if (!a.text[iSub].IsEmpty() && b.text[iSub].IsEmpty())
								//	return true;
							}
							else
							{
								return (compare_string(a.text[iSub], b.text[iSub]) == 1);
								//if (a.text[iSub].MakeLower() > b.text[iSub].MakeLower())
								//	return false;
								//else if (a.text[iSub].MakeLower() < b.text[iSub].MakeLower())
								//	return true;
								//else
								//{
								//	if (a.text[0].MakeLower() > b.text[0].MakeLower())
								//		return false;
								//	else
								//		return true;
								//}
							}
						}
						else
						{
							//return (a.text[iSub].MakeLower() < b.text[iSub].MakeLower());
							return (compare_string(a.text[iSub], b.text[iSub]) == 1);
						}

						//return (a.text[iSub].MakeLower() < b.text[iSub].MakeLower());
						return (compare_string(a.text[iSub], b.text[iSub]) == 1);
					}
					else
					{
						//a.text[iSub].Replace(_T(","), _T(""));
						a.text[iSub].Remove(',');
						b.text[iSub].Remove(',');
						return (_ttof(a.text[iSub]) < _ttof(b.text[iSub]));
					}
				}
				else
				{
					if (data_type == column_data_type_text)
					{
						if (iSub != 0)
						{
							if (!include_null)
							{
								if (a.text[iSub].IsEmpty() && b.text[iSub].IsEmpty())
									return true;
								else if (a.text[iSub].IsEmpty() && !b.text[iSub].IsEmpty())
									return false;
								else if (!a.text[iSub].IsEmpty() && b.text[iSub].IsEmpty())
									return true;
							}

							if (a.text[iSub].MakeLower() > b.text[iSub].MakeLower())
								return true;
							else if (a.text[iSub].MakeLower() < b.text[iSub].MakeLower())
								return false;
							else
							{
								if (a.text[0].MakeLower() > b.text[0].MakeLower())
									return true;
								else
									return false;
							}
						}
						else
						{
							return (a.text[iSub].MakeLower() > b.text[iSub].MakeLower());
						}
						return (_ttof(a.text[iSub].MakeLower()) > _ttof(b.text[iSub].MakeLower()));
					}
					else if (data_type == column_data_type_text_ip)
					{
						if (iSub != 0)
						{
							if (!include_null)
							{
								return (compare_string(a.text[iSub], b.text[iSub]) == -1);
								//if (a.text[iSub].IsEmpty() && b.text[iSub].IsEmpty())
								//	return true;
								//else if (a.text[iSub].IsEmpty() && !b.text[iSub].IsEmpty())
								//	return false;
								//else if (!a.text[iSub].IsEmpty() && b.text[iSub].IsEmpty())
								//	return true;
							}
							else
							{
								return (compare_string(a.text[iSub], b.text[iSub]) == -1);
								//if (a.text[iSub].MakeLower() > b.text[iSub].MakeLower())
								//	return false;
								//else if (a.text[iSub].MakeLower() < b.text[iSub].MakeLower())
								//	return true;
								//else
								//{
								//	if (a.text[0].MakeLower() > b.text[0].MakeLower())
								//		return false;
								//	else
								//		return true;
								//}
							}
						}
						else
						{
							//return (a.text[iSub].MakeLower() < b.text[iSub].MakeLower());
							return (compare_string(a.text[iSub], b.text[iSub]) == -1);
						}

						//return (a.text[iSub].MakeLower() < b.text[iSub].MakeLower());
						return (compare_string(a.text[iSub], b.text[iSub]) == -1);
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
}

void CVtListCtrlEx::modify_style()
{
	//get parent font
	CWnd* pWnd = GetParent();
	CFont* font = NULL;

	if (pWnd)
		font = pWnd->GetFont();

	if (font == NULL)
		GetObject(GetStockObject(SYSTEM_FONT), sizeof(m_lf), &m_lf);
	else
		font->GetObject(sizeof(m_lf), &m_lf);

	//헤더컨트롤을 제어할 일이 있는지 확인 필요.
#if 1
	// get original view style
	//LVS_OWNERDATA는 동적으로 설정되지 않는듯하다. resource editor에서 주고 시작하자.
	DWORD dwStyle = GetStyle() & LVS_TYPEMASK;
	//ModifyStyle(LVS_TYPEMASK, dwStyle | LVS_REPORT | LVS_OWNERDRAWFIXED | LVS_OWNERDATA | LVS_NOSORTHEADER);
	BOOL b = ModifyStyle(0, dwStyle | LVS_REPORT | LVS_OWNERDRAWFIXED | LVS_OWNERDATA | LVS_NOSORTHEADER);

	// if view style is other than LVS_REPORT 
	// returned pointer will be NULL
	CHeaderCtrl* pHeader = GetHeaderCtrl();

	if (pHeader)
	{
		// voila!
		m_HeaderCtrlEx.SubclassWindow(pHeader->m_hWnd);
	}

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
			path = GetParentDirectory(path);
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
	else if (pMsg->message == WM_KEYDOWN)
	{
		//TRACE(_T("listctrl key = %d\n"), pMsg->wParam);
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
		case VK_BACK	:
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
					new_path = GetParentDirectory(m_path);

				set_path(new_path);
				
				//VtListCtrlEx 내부에서 어떤 이벤트에 의해 경로가 변경되는 경우라면 parent에게 이를 알려야한다.
				//set_path에서 메시지 전송을 포함시키면 recursive call이 발생하므로 별도로 호출한다.
				::SendMessage(GetParent()->GetSafeHwnd(), Message_CVtListCtrlEx, (WPARAM) & (CVtListCtrlExMessage(this, message_path_changed, NULL)), (LPARAM)&new_path);
			}
				
			break;
		}

		case 220		:	return true;	//'\'키를 누르면 리스트 맨 처음으로 이동되는 현상 방지.
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
		case VK_END :		if (!m_in_editing && GetKeyState(VK_CONTROL) & 0x8000)
							{
								set_auto_scroll(true);
								return true;	//여기서 true를 리턴하지 않으면 CListCtrl의 기본 end 키 처리가 수행되고 현재 화면에서 맨 아래 항목이 선택되어 m_auto_scroll이 다시 false로 변한다.
							}
							return false;
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

	if (pHeaderCtrl)
	{
		pHeaderCtrl->GetClientRect(&rcHeader);
		rc.top += rcHeader.Height();
	}

	CMemoryDC dc(&dc1, &rc, true);
	Gdiplus::Graphics g(dc.m_hDC);

	//CTreeCtrl, CListCtrl의 배경색은
	//dc.FillSolidRect(rc, m_theme.cr_back.ToCOLORREF());
	dc.FillSolidRect(rc, m_theme.cr_back.ToCOLORREF());

	if (m_list_db.size() == 0 && !m_text_on_empty.IsEmpty())
	{
		draw_text(g, rc, m_text_on_empty, 10, false, 0, 0, _T("맑은 고딕"));
	}

	//CListCtrl::OnPaint();
	DefWindowProc(WM_PAINT, (WPARAM)dc.m_hDC, (LPARAM)0);
}


void CVtListCtrlEx::OnLvnBeginlabeledit(NMHDR *pNMHDR, LRESULT *pResult)
{
	NMLVDISPINFO *pDispInfo = reinterpret_cast<NMLVDISPINFO*>(pNMHDR);
	// TODO: Add your control notification handler code here
	*pResult = 0;
}


BOOL CVtListCtrlEx::OnLvnEndlabeledit(NMHDR *pNMHDR, LRESULT *pResult)
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


CEdit* CVtListCtrlEx::edit_item(int item, int subItem)
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

	r.OffsetRect(0, 1);

	GetClientRect(rc);

	// Get Column alignment
	DWORD dwStyle = ES_LEFT;
	int align = get_column_text_align(subItem);
	if (align == HDF_CENTER)
		dwStyle = ES_CENTER;
	else if (align == HDF_RIGHT)
		dwStyle = ES_RIGHT;

	TRACE(_T("subItem = %d, dwStyle = %d\n"), subItem, dwStyle);
	/*
	LV_COLUMN	lvCol;

	lvCol.mask = LVCF_FMT;
	GetColumn (subItem, &lvCol);


	if ((lvCol.fmt & LVCFMT_JUSTIFYMASK) == LVCFMT_LEFT)
		dwStyle = ES_LEFT;
	else if ((lvCol.fmt & LVCFMT_JUSTIFYMASK) == LVCFMT_RIGHT)
		dwStyle = ES_RIGHT;
	else
		dwStyle = ES_CENTER;
	*/

	if (r.right > rc.right)
		r.right = rc.right;

	m_edit_old_text = GetItemText(item, subItem);

	//edit을 동적으로 생성할 때 ES_MULTILINE 속성이 있을 경우
	//edit의 높이는 1줄이고 너비가 텍스트 길이보다 작을 경우 word wrap되어 일부 텍스트가 다음줄로 넘어가서 보이지 않게 된다.
	//따라서 SetRect를 이용해서 세로 중앙에 표시하는 등 ES_MULTILINE이 반드시 필요한 경우에만 명시한다.
	//이 CVtListCtrlEx는 single line을 기본으로 하므로 ES_MULTILINE을 생략한다.
	if (m_pEdit == NULL)
	{
		m_pEdit = new CSCEdit;
		m_pEdit->Create(dwStyle | WS_BORDER | WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL /* | ES_MULTILINE*/, r, this, IDC_EDIT_CELL);
	}

	//ES_MULTILINE, ES_LEFT, ES_CENTER, ES_RIGHT 등은 동적변경이 불가능한 스타일이므로
	//edit_end()에서 기존에는 hide시켰으나 delete으로 변경함.
	//BOOL res = m_pEdit->ModifyStyle(0, dwStyle);
	m_pEdit->MoveWindow(r);
	m_pEdit->SetRect(&r);
	m_pEdit->SetFont(&m_font, true);
	m_pEdit->SetWindowText(m_edit_old_text);

	m_pEdit->ShowWindow(SW_SHOW);
	m_pEdit->SetSel(0, -1);
	m_pEdit->SetFocus();

	//ES_READONLY는 ModifyStyle()로는 동적변경할 수 없다. 아래와 같이 EM_SETREADONLY를 보내야 한다.
	//m_pEdit->ModifyStyle(m_edit_readonly ? 0 : ES_READONLY, m_edit_readonly ? ES_READONLY : 0);
	m_pEdit->SendMessage(EM_SETREADONLY, m_edit_readonly ? TRUE : FALSE, 0);

	CString ext = get_part(m_edit_old_text, fn_ext);
	if ((ext.GetLength() == 3 || ext.GetLength() == 4) && IsAlphaNumeric(ext))
	{
		m_pEdit->SetSel(0, m_edit_old_text.GetLength() - ext.GetLength() - 1);
	}

	m_in_editing = true;
	m_edit_item = item;
	m_edit_subItem = subItem;
	get_selected_items(&m_dqSelected_list_for_edit);

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

LRESULT CVtListCtrlEx::on_message_CSCEdit(WPARAM wParam, LPARAM lParam)
{
	CSCEdit* pEdit = (CSCEdit*)wParam;
	int	msg = (int)lParam;

	if (!pEdit->IsWindowVisible())
		return 0;

	TRACE(_T("message(%d) from CSCEdit(%p)\n"), (int)lParam, pEdit);
	if (msg == WM_KILLFOCUS)
		edit_end();

	Invalidate();

	return 0;
}

void CVtListCtrlEx::set_color_theme(int theme, bool apply_now)
{
	m_theme.set_color_theme(theme);

	m_HeaderCtrlEx.set_color(m_theme.cr_header_text, m_theme.cr_header_back);

	if (m_hWnd)
		Invalidate();
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

void CVtListCtrlEx::set_text_color(int item, int subItem, Gdiplus::Color crText, bool erase, bool invalidate)
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

	if (invalidate)
		Invalidate();
}

void CVtListCtrlEx::set_back_color(int item, int subItem, Gdiplus::Color crBack, bool erase)
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

void CVtListCtrlEx::set_progress_color(Gdiplus::Color crProgress)
{
	m_theme.cr_progress = crProgress;
	Invalidate();
}

int CVtListCtrlEx::add_item(CString text, int image_index, bool ensureVisible, bool invalidate)
{
	return insert_item(-1, text, image_index, ensureVisible, invalidate);
}

int CVtListCtrlEx::insert_line(int index, CString line_string, CString separator, int image_index, bool ensureVisible, bool invalidate)
{
	std::deque<CString> dq;
	get_token_string(line_string, dq, separator);
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

	m_list_db.insert(m_list_db.begin() + index, CListCtrlData(text, image_index, m_HeaderCtrlEx.GetItemCount()));

	//LVSICF_NOSCROLL 옵션을 주지 않으면 특정 항목 선택 후 해당 항목이 보이지 않도록 스크롤하려 해도
	//데이터가 계속 추가되는 상황에서는 선택된 항목이 보이지 않는 영역으로의 스크롤이 되지 않는 현상이 있다.
	SetItemCountEx(m_list_db.size(), LVSICF_NOSCROLL);

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
	int old_index = find_string(filename, 0, true);
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
	set_text_color(index, col_filedate, RGB(109, 109, 109));

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
		folder.Format(_T("%s\\%s"), folder, new_folder_title);
	else
		folder.Format(_T("%s\\%s (%d)"), folder, new_folder_title, index);

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
	return m_list_db.size();
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
}

CString CVtListCtrlEx::get_text(int item, int subItem)
{
	return m_list_db[item].text[subItem];
}

void CVtListCtrlEx::set_text(int item, int subItem, CString text, bool invalidate)
{
	if (item < 0 || item >= size())
		return;

	if (!m_hWnd || m_hWnd == INVALID_HANDLE_VALUE)
		return;

	if (m_is_shell_listctrl && !m_pShellImageList)
		return;

	//양방향 파일전송2에서 전송 중 취소를 누르면 간혹 m_list_db의 size가 매우 큰 수로 나오는 경우가 있다.
	//원인을 파악중이며 우선 임시로 처리한다.
	if (m_list_db.size() > 10000)
		return;

	m_list_db[item].text[subItem] = text;
	if (invalidate)
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
	get_token_string(text, dqlines, _T("\n"));

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
		linetext = linetext + get_text(index, dqColumn->at(i)) + sep;

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
	std::deque<int> dq_index;
	get_selected_items(&dq_index);

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

void CVtListCtrlEx::select_item(int nIndex, bool bSelect /*= true*/, bool after_unselect, bool make_visible)
{
	if (after_unselect)
		unselect_selected_item();

	SetItemState(nIndex, bSelect ? LVIS_SELECTED : 0, LVIS_SELECTED);
	SetItemState(nIndex, bSelect ? LVIS_FOCUSED : 0, LVIS_FOCUSED);

	if (make_visible)
		ensure_visible(nIndex, visible_last);
}

void CVtListCtrlEx::unselect_selected_item()
{
	std::deque<int> dqSelected;
	get_selected_items(&dqSelected);
	for (int i = 0; i < dqSelected.size(); i++)
		select_item(dqSelected[i], false);
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

//0번 컬럼에서만 데이터를 찾는다.
int CVtListCtrlEx::find_string(CString str, int start, bool bWholeWord, bool bCaseSensitive)
{
	/*
	LVFINDINFO info;

	info.flags = (bWholeWord ? LVFI_STRING : LVFI_PARTIAL|LVFI_STRING);
	info.psz = str;

	// Delete all of the items that begin with the string.
	return FindItem(&info, indexFrom);
	*/
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
int CVtListCtrlEx::find_string(CString find_target, std::deque<int>* result,
								int start_idx, int end_idx,
								std::deque<int>* dqColumn, bool stop_first_found)
{
	int		i;
	TCHAR	op;
	CString sText;
	std::deque<CString> dqTarget;	//separator가 ""이 아닐 경우는 토큰으로 분리하여 모두 찾는다.
	std::deque<int> dq_columns;

	if (start_idx < 0)
		start_idx = 0;

	if (end_idx < 0 || end_idx >= size())
		end_idx = size() - 1;
	if (end_idx < 0)
		return 0;

	//찾은 항목을 선택으로 표시하는 경우 먼저 기존 선택된 항목들을 초기화시켜준다.
	//if (select)
	//	select_item(-1, false);

	if (find_target.Find('&') > 0)
		op = '&';
	else
		op = '|';

	get_token_string(find_target, dqTarget, op, false);
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
		if (find_dqstring(dqLine, dqTarget, op) >= 0)
		{
			result->push_back(cur_idx);

			if (stop_first_found)
			{
				::SendMessage(GetParent()->GetSafeHwnd(), Message_CVtListCtrlEx, (WPARAM)&CVtListCtrlExMessage(this, message_progress_pos), (LPARAM)-1);
				return 1;
			}
		}

		if (start_idx < end_idx)
		{
			cur_idx++;
			if (cur_idx > end_idx)
				return result->size();
		}
		else
		{
			cur_idx--;
			if (cur_idx < end_idx)
				return result->size();
		}

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

	return result->size();
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

		if (!get_index_from_point(pNMItemActivate->ptAction, item, subItem, true) ||
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
void CVtListCtrlEx::reconstruct_font()
{
	m_font.DeleteObject();
	BOOL bCreated = m_font.CreateFontIndirect(&m_lf);
	SetFont(&m_font, false);

	if (m_HeaderCtrlEx)
		m_HeaderCtrlEx.set_font(&m_lf);

	m_font_size = get_font_size();
	
	//set_line_height(4-m_lf.lfHeight);
	//if (m_auto_line_height)
		//recalculate_line_height();k
	
	ASSERT(bCreated);
}

int CVtListCtrlEx::get_font_size()
{
	m_font_size = get_font_size_from_pixel_size(GetParent()->GetSafeHwnd(), m_lf.lfHeight);
	return m_font_size;
}

//-1 : reduce, +1 : enlarge
void CVtListCtrlEx::set_font_size(int font_size)
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

	reconstruct_font();
}

void CVtListCtrlEx::enlarge_font_size(bool enlarge)
{
	m_font_size = get_font_size_from_pixel_size(m_hWnd, m_lf.lfHeight);
	enlarge ? m_font_size++ : m_font_size--;

	if (m_font_size < 4)
		m_font_size = 4;
	if (m_font_size > 40)
		m_font_size = 40;

	m_lf.lfHeight = get_pixel_size_from_font_size(GetParent()->GetSafeHwnd(), m_font_size);

	reconstruct_font();
}

void CVtListCtrlEx::set_font_name(LPCTSTR sFontname, BYTE byCharSet)
{
	if (sFontname == _T(""))
		return;
	m_lf.lfCharSet = byCharSet;
	_tcscpy_s(m_lf.lfFaceName, _countof(m_lf.lfFaceName), sFontname);
	reconstruct_font();
}

void CVtListCtrlEx::set_font_bold(int weight)
{
	m_lf.lfWeight = weight;
	reconstruct_font();
}

void CVtListCtrlEx::set_font_italic(bool italic)
{
	m_lf.lfItalic = italic;
	reconstruct_font();
}

void CVtListCtrlEx::set_log_font(LOGFONT lf)
{
	memcpy(&m_lf, &lf, sizeof(LOGFONT));
	reconstruct_font();
	Invalidate();
}

//어떤 항목을 클릭한 후 단축키 F2를 누르면 해당 텍스트를 편집하는 용도이므로
//이 때 클릭된 subItem을 기억해놓는다.
BOOL CVtListCtrlEx::OnNMClick(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMITEMACTIVATE pNMItemActivate = reinterpret_cast<LPNMITEMACTIVATE>(pNMHDR);
	int item = -1;// = pNMItemActivate->iItem;
	int subItem = -1;// = pNMItemActivate->iSubItem;	<== invalid index returned when user clicked out of columns

	if (!get_index_from_point(pNMItemActivate->ptAction, item, subItem, false) ||
		item < 0 || subItem < 0)
	{
		*pResult = 1;
		return TRUE;
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

	return FALSE;
	*pResult = 0;
}

//m_is_shell_list라면 이름 변경은 굳이 main이 아닌 여기서 처리해도 될듯하다.
//단, 중복되는 이름이 존재하여 실패할 경우는 main에게 이를 알려 메시지를 표시해야 한다.
void CVtListCtrlEx::edit_end(bool valid)
{
	if (m_in_editing == false || m_pEdit == NULL)
		return;

	m_in_editing = false;
	m_last_clicked_time = 0;
	m_pEdit->GetWindowText(m_edit_new_text);
	//m_pEdit->ShowWindow(SW_HIDE);
	m_pEdit->DestroyWindow();
	delete m_pEdit;
	m_pEdit = NULL;

	//if (m_edit_new_text == m_edit_old_text)
	//	return;

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

DWORD CVtListCtrlEx::index_from_point(int x, int y)
{
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
				return MAKELONG(j, first);
			}
		}
	}

	return MAKELONG(-1, -1);
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

void CVtListCtrlEx::refresh_list(bool reload)
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
				std::deque<WIN32_FIND_DATA> dq;
				find_all_files(m_path, &dq, _T("*"), true, false);
				for (int i = 0; i < dq.size(); i++)
				{
					if (dq[i].dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
						m_cur_folders.push_back(CVtFileInfo(dq[i]));
					else
						m_cur_files.push_back(CVtFileInfo(dq[i]));
				}
			}
			TRACE(_T("cur folders[0] = %s\n"), m_cur_folders.size() > 0 ? m_cur_folders[0].data.cFileName : _T(""));
			TRACE(_T("cur files[0] = %s\n"), m_cur_files.size() > 0 ? m_cur_files[0].data.cFileName : _T(""));

			display_filelist(m_path);
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


	//asc일 경우		: 폴더 먼저 0번 위치부터 표시하고 파일은 현재 리스트의 맨 끝에 추가한다.
	//desc일 경우	: 폴더는 역시 0번부터 표시하고 파일은 i번째에 추가하면 된다. 폴더는 자연히 뒤로 밀리면서 파일들보다 아래에	 표시된다.
	for (i = 0; i < m_cur_folders.size(); i++)
	{
		::SendMessage(GetParent()->GetSafeHwnd(), Message_CVtListCtrlEx,
			(WPARAM) & (CVtListCtrlExMessage(this, message_list_processing, NULL, _T(""), _T(""), WPARAM(m_cur_folders.size() + m_cur_files.size()))), (LPARAM)(i + 1));

		CString real_path = m_pShellImageList->convert_special_folder_to_real_path(!m_is_local, m_cur_folders[i].data.cFileName);//convert_special_folder_to_real_path(m_cur_folders[i].data.cFileName, m_pShellImageList, !m_is_local);

		if (m_is_local)
		{
			img_idx = m_pShellImageList->GetSystemImageListIcon(!m_is_local, real_path, true);
		}
		else
		{
			if (is_drive_root(real_path))
			{
				img_idx = m_pShellImageList->get_drive_icon(!m_is_local, real_path);
			}
			else
			{
				img_idx = m_pShellImageList->GetSystemImageListIcon(!m_is_local, _T("c:\\windows"), true);
			}
		}

		index = insert_item(i, get_part(m_cur_folders[i].data.cFileName, fn_name), img_idx, false, false);
		SetItemData(index, i);

		if (m_path == m_pShellImageList->m_volume[!m_is_local].get_label(CSIDL_DRIVES))
		{
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

			if (ul_free_space.QuadPart > 0)
				set_text(index, col_filesize, get_size_str(ul_free_space.QuadPart, -1));
			if (ul_total_space.QuadPart > 0)
				set_text(index, col_filedate, get_size_str(ul_total_space.QuadPart, -1));

			set_text_color(index, col_filesize, RGB(109, 109, 109));
			set_text_color(index, col_filedate, RGB(109, 109, 109));
		}
		else
		{
			set_text(index, col_filesize, _T(""));
			set_text(index, col_filedate, get_file_time_str(m_cur_folders[i].data.ftLastWriteTime));
			set_text_color(index, col_filedate, RGB(109, 109, 109));
		}
	}

	for (i = 0; i < m_cur_files.size(); i++)
	{
		::SendMessage(GetParent()->GetSafeHwnd(), Message_CVtListCtrlEx,
			(WPARAM) & (CVtListCtrlExMessage(this, message_list_processing, NULL, _T(""), _T(""), WPARAM(m_cur_folders.size() + m_cur_files.size()))), (LPARAM)(m_cur_folders.size() + i + 1));

		img_idx = m_pShellImageList->GetSystemImageListIcon(!m_is_local, m_cur_files[i].data.cFileName, false);
		index = insert_item(m_column_sort_type[m_cur_sort_column] == sort_descending ? i : -1, get_part(m_cur_files[i].data.cFileName, fn_name), img_idx, false, false);
		SetItemData(index, m_cur_folders.size() + i);

		set_text(index, col_filesize, get_size_str(get_file_size(m_cur_files[i].data)));
		set_text(index, col_filedate, m_cur_files[i].get_file_time_str());
		set_text_color(index, col_filesize, RGB(109, 109, 109));
		set_text_color(index, col_filedate, RGB(109, 109, 109));
	}

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

	if (!get_index_from_point(pNMItemActivate->ptAction, item, subItem, false) ||
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

	CGdiplusBitmap bmpRes;// (64, 64, PixelFormat32bppARGB, Gdiplus::Color(128, 255, 0, 0));
	
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

		//드롭 대상 컨트롤의 상단, 하단에 마우스가 hovering되면 자동 스크롤 시켜줘야 한다.
		if (m_pDropWnd)
		{
			CRect rw;
			m_pDropWnd->GetWindowRect(rw);

			if (rw.PtInRect(pt))
			{
				if (pt.x < rw.left + 32)
					m_pDropWnd->SendMessage(WM_HSCROLL, SB_LINELEFT);
				else if (pt.x > rw.right - 32)
					m_pDropWnd->SendMessage(WM_HSCROLL, SB_LINERIGHT);

				if (pt.y < rw.top + 32)
					m_pDropWnd->SendMessage(WM_VSCROLL, SB_LINEUP);
				else if (pt.y > rw.bottom - 32)
					m_pDropWnd->SendMessage(WM_VSCROLL, SB_LINEDOWN);
			}
		}

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

	if ((pNMListView->uChanged & LVIF_STATE)
		&& (pNMListView->uNewState & LVIS_SELECTED))
	{
		set_auto_scroll(false);
	}

	return FALSE;

	*pResult = 0;
}
/*
void CVtListCtrlEx::capture_selected_items_to_bitmap(CGdiplusBitmap* bmp)
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
		CGdiplusBitmap sub(hBitmap);

		rItem[i].OffsetRect(offset);
		bmp->draw(&sub, &rItem[i]);
	}
}
*/

void CVtListCtrlEx::OnContextMenu(CWnd* /*pWnd*/, CPoint /*point*/)
{
	// TODO: 여기에 메시지 처리기 코드를 추가합니다.
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
	UINT uFlags = 0;
	//int item = HitTest(point, &uFlags);

	int item = -1;// = pNMItemActivate->iItem;
	int subItem = -1;// = pNMItemActivate->iSubItem;	<== invalid index returned when user clicked out of columns

	//include_icon = false로 주면 아이콘을 클릭해도 해당 항목이 선택되지 않는다.
	get_index_from_point(point, item, subItem, true);

	//TRACE(_T("%d, %d, uFlags = %d\n"), item, subItem, uFlags);

	if (item >= 0 && subItem < 0)
		return;

	//if (uFlags == 14)
	//	//nFlags = 1;
	//	return; LVHT_ONITEMLABEL
	CListCtrl::OnLButtonDown(nFlags, point);
}


LRESULT CVtListCtrlEx::OnNcHitTest(CPoint point)
{
	// TODO: 여기에 메시지 처리기 코드를 추가 및/또는 기본값을 호출합니다.
	//TRACE(_T("OnNcHitTest\n"));
	return CListCtrl::OnNcHitTest(point);
}


void CVtListCtrlEx::OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar)
{
	// TODO: 여기에 메시지 처리기 코드를 추가 및/또는 기본값을 호출합니다.
	TRACE(_T("CVtListCtrlEx::OnHScroll\n"));
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
	*pResult = 0;
}

void CVtListCtrlEx::OnSize(UINT nType, int cx, int cy)
{
	CListCtrl::OnSize(nType, cx, cy);

	// TODO: 여기에 메시지 처리기 코드를 추가합니다.
	if (!m_button_scroll_to_end)
		return;

	CRect rc;
	GetClientRect(rc);
	m_button_scroll_to_end->MoveWindow(CRect(rc.right - 5 - m_auto_scroll_button_size, rc.bottom - 5 - m_auto_scroll_button_size, rc.right - 5, rc.bottom - 5));
}

void CVtListCtrlEx::show_auto_scroll_button(bool show)
{
	m_show_auto_scroll_button = show;

	if (!m_button_scroll_to_end)
	{
		init_auto_scroll_button();
	}
}

void CVtListCtrlEx::set_auto_scroll(bool auto_scroll)
{
	m_auto_scroll = auto_scroll;

	if (m_button_scroll_to_end)
		m_button_scroll_to_end->ShowWindow(m_auto_scroll ? SW_HIDE: SW_SHOW);
}

LRESULT CVtListCtrlEx::on_message_CGdiButton(WPARAM wParam, LPARAM lParam)
{
	auto msg = (CGdiButtonMessage*)wParam;

	if (msg->msg == WM_LBUTTONUP)
	{
		if (msg->pWnd == m_button_scroll_to_end)
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

	CGdiplusBitmap img(m_auto_scroll_button_size, m_auto_scroll_button_size);
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
