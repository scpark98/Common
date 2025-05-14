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

		//���� ���� ����, focus���� � ���� ���� �׷��� ���ڻ�, ������ ����ְ�...

		//LVIR_BOUNDS�� ���� ��� 0�� �÷��� �� ������ �簢�� ������ �����Ѵ�.
		//0�� �÷� ���ĺ��ʹ� �ش� ���� �簢�� �������� �����Ѵ�.
		GetSubItemRect(iItem, iSubItem, LVIR_BOUNDS, itemRect);

		if (iSubItem == 0)
		{
			itemRect.right = itemRect.left + GetColumnWidth(0);
		}

		//if(lpDIS->itemState & ODS_SELECTED) //ok
		//��Ŀ���� �����ų� Always Show Selection�̶�� ���� �׸��� ������ ǥ�����ְ�
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
		//drophilited��� active�� ������� drop hilited �������� ǥ���Ѵ�.
		//�� ��� �׸��� ������ ���� drop hilited ǥ�ø� ���� �ʴ´�.
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

		//���� �׸��� �ؽ�Ʈ ������ ������ �÷� ��Ŵ�� �����°� �ƴ϶�
		//������ ���� �ִٸ� �� ���� �켱���� �ؾ� �Ѵ�.
		if (m_list_db[iItem].crText[iSubItem].GetValue() != listctrlex_unused_color.GetValue())
			crText = m_list_db[iItem].crText[iSubItem];
		if (m_list_db[iItem].crBack[iSubItem].GetValue() != listctrlex_unused_color.GetValue())
			crBack = m_list_db[iItem].crBack[iSubItem];
	
		pDC->FillSolidRect(itemRect, crBack.ToCOLORREF());

		//percentage Ÿ���̸� �ٱ׷��� ���·� �׷��ְ�
		if (get_column_data_type(iSubItem) == column_data_type_percentage_bar)
		{
			CRect r = itemRect;

			//�ٱ׷����� ���� ����, �� ���� ���ݰ��� �������. ��Ʈ�� ���̰��� ����ϴ� ���̷� �׷�����.
			int cy = r.CenterPoint().y;
			r.DeflateRect(4, 0);
			r.top = cy + m_lf.lfHeight/3.4;		//�� ���� Ű��� �ٱ׷����� ���̰� ��������.
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
				//���� ������ �´� �ܻ����� ä�� ���
				if (false)
				{
					pDC->FillSolidRect(r, get_color(m_theme.cr_percentage_bar[0], m_theme.cr_percentage_bar[1], d).ToCOLORREF());
				}
				//���� �������� �׶���Ʈ�� ä�� ���
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

			//�ٱ׷����� ���� ����, �� ���� ���ݰ��� �������. ��Ʈ�� ���̰��� ����ϴ� ���̷� �׷�����.
			int cy = r.CenterPoint().y;
			r.DeflateRect(4, 0);
			r.top = cy + m_lf.lfHeight / 3.4;		//�� ���� Ű��� �ٱ׷����� ���̰� ��������.
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

			//�ٱ׷����� ���� ����, �� ���� ���ݿ� ��������� �ʰ� ��Ʈ�� ���̰��� ����ϴ� ���̷� �׷���� �Ѵ�.
			int cy = r.CenterPoint().y;
			r.top = cy + (double)m_lf.lfHeight / 1.8;		//������ ���� Ŀ���� �ٱ׷����� ���̰� ��������.
			r.bottom = cy - (double)m_lf.lfHeight / 1.8;	//m_lf.lfHeight�� �����̹Ƿ� -,+�� �ƴ϶� +,-�� ���� ����

			double d = _ttof(m_list_db[iItem].text[iSubItem]);
			d /= 100.0;
			Clamp(d, 0.0, 1.0);

			r.right = r.left + (double)(r.Width()) * d;
			pDC->FillSolidRect(r, m_theme.cr_progress.ToCOLORREF());

			//20231102 CSCSliderCtrl������ �����ϰ� progress ��� ��ġ�� ���� ���ʰ� �������� ���� �ٸ� ������ ǥ���ϰ���
			//�Ʒ� �ڵ带 ��������� �ؽ�Ʈ�� ���� ��µ��� �ʴ´�.
			//pDC�� �ƴ� CMemoryDC�� �� ����ϴ� ���ϴ´�� ��µ����� �ٸ� row�� ���� �׷����� �ʴ´�.
			//Rgn�� �����Ͽ� OnPaint()�� DrawItem�� ���� ���̰� �ִ�.
			if (m_show_progress_text)
			{
				CString text = m_list_db[iItem].text[iSubItem];
				CString sPercent;
				
				if (!text.IsEmpty())
				{
					//progress type�̶� ������ %�� �ٿ����� �ȵȴ�. text�� �����̸� ���� %�� ���� ��쿡�� �ڵ����� �ٿ��ش�.
					if (IsNumericString(text) && (text.GetLength() > 0) && (text.Right(1) != '%'))
						text += _T("%");

					//text color�� ������ �� ���� �� �÷��� ���� �׷��� ������ m_theme�� ���ǵ� �÷��� ����.
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
		//�ؽ�Ʈ �����̸� ���ڿ��� ������ش�.
		else
		{
			pDC->SetTextColor(crText.ToCOLORREF());

			//�ؽ�Ʈ�� �׷��� �� itemRect�� �׸��� �¿� ������ ��� ������ �������δ�.
			//�ణ �ٿ��� ����ؾ� ���� ����.
			textRect = itemRect;
			CString text = get_text(iItem, iSubItem);

			//0�� �÷��� text �տ� ������ ���� ��� �̸� �鿩����� ó���ϴ� ���
			if (iSubItem == 0 && m_use_indent_from_prefix_space)
			{
				int space_count = get_char_count(text, ' ', true);
				textRect.left += (space_count * 16);
				text = text.Mid(space_count);
			}

			//������ ǥ��
			if (iSubItem == 0 &&
				(m_is_shell_listctrl ||
				(m_use_own_imagelist && m_pShellImageList && m_pShellImageList->get_imagelist() && m_pShellImageList->get_imagelist()->GetImageCount())))
			{
				//16x16 �������� 22x21 ������ ǥ���Ѵ�. (21�� �⺻ height�̸� m_line_height�� ���� �޶�����.)
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

			//���̺� ���
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

	//���õ� �׸��� ���� ���󺸴� ���� ������ �׵θ��� �׷�����.
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
		//rowRect.bottom���� ���ָ� ������ �������̹Ƿ� �׷����� �ʴ´�. �ݵ�� -1�� �ؾ� ��.
		draw_line(pDC, rowRect.left, rowRect.bottom - 1, rowRect.right, rowRect.bottom - 1, m_cr_bottom_line);
	}
}

// ex. "No,20;Item1,50;Item2,50"
//scpark 20230801
//��� ������Ʈ���� �� �Լ��� 2�� ȣ��Ǿ� ���� �÷��� 3���ε�
//6���� ���õǾ� ������ �߻���.
//�Ź� ȣ��� �����ϰ� ó���ص� �ǰ� �ٸ� ó�� ����� ������
//��Ȯ�� �ѹ��� ȣ���Ͽ� ����ϴ� ���� �����̹Ƿ� �켱 ������ ó���� ���� ����.
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

	//�÷��� ���� �����Ǹ� �÷��� ���õ� ���� ���� ������ ũ�⸦ �������ش�.
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

//�ش� �÷����� ���ڿ� ���� �ִ밪�� ����.
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

//�ش� �÷����� ��½ÿ� width �ִ밪�� ����.(�̱���)
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

	//��ü������ imagelist�� ������� �ʴ� ���� �Ʒ� �ڵ常���ε�
	//height�� ����ȴ�.
	if (!m_use_own_imagelist)
	{
		CImageList gapImage;
		gapImage.Create(1, height, ILC_COLORDDB, 1, 0); //2��° �Ķ���ͷ� ��������.....
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
			Rect.left += 19;	//editbox��ü�� left-margin�� �����Ƿ� 22�� �ƴ� 19�� �����ش�.
			Rect.OffsetRect(0, -1);
		}
	}

	return Rect;
}

//Ŭ����ġ�� ���� item�� �ùٸ��� �Ǻ��ǳ� subItem�� �׷��� �ʾƼ�(���콺 �̺�Ʈ �ڵ鷯 �Լ�����) ���� �߰���.
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
		//LVIR_BOUNDS�� itemRect�� ���ϸ� �� �� �÷��� ���� Ŭ�� �� �����ȿ� ������ �����Ƿ�
		//item�� -1�� �����Ѵ�. itemRect.right�� rc.right�� �˻������ subItem�� -1�̴��� item�� �� �� �ִ�.
		GetItemRect(i, &itemRect, LVIR_BOUNDS);
		itemRect.right = rc.right;

		if (itemRect.PtInRect(pt))
		{
			item = i;

			//��� �÷������� ã�´�.
			for (int j = get_column_count() - 1; j >= 0; j--)
			{
				GetSubItemRect(i, j, LVIR_BOUNDS, itemRect);

				//0�� �÷��� �� ���� ��ü�� ������ �����ϹǷ�
				if (j == 0)
				{
					//0�� �÷��� width�� �������ְ�
					itemRect.right = itemRect.left + GetColumnWidth(0);
					//imagelist�� ����Ѵٸ� �̹��� ������ŭ �ٽ� ��������� �Ѵ�.
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

//debug��忡���� �ſ� ������. lambda�������� �𸣰ڴ�.
//0�� �÷��� �ƴ� �ٸ� �÷����� ������ �� �� ���� ������ 0�� �÷����� �ѹ� �� �˻��Ѵ�.
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

	//shelllist�� ���� ������ ������ ������ �����Ѵ�.
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

					//����ũ�Ⱑ ���ٸ� �̸������� �����Ѵ�.
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

					//����ũ�Ⱑ ���ٸ� �̸������� �����Ѵ�.
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
			//������ ������ ���ϸ����� �����Ϸ� ������
			//���� ���ϴ´�� ǥ�õ��� �ʰ� �ð��� �� ����ȴ�.
			//�ϴ� �������� �����Ѵ�.
			//������ ���� ��� ���ϸ����� ������ ��
			//�Ϲ� ���� �׸��� ���ܽ�Ű�� ���ϴ´�� ���۵�.
			//return (a.crText[iSub] > b.crText[iSub]);
			
			if (a.crText[iSub].GetValue() > b.crText[iSub].GetValue())
				return true;
			else if (a.crText[iSub].GetValue() < b.crText[iSub].GetValue())
				return false;
			//�ؽ�Ʈ ������ ������ �ؽ�Ʈ�� ���ϵ� �Ϲ� �׸��� ���ܽ�Ų��.
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
			else //�ؽ�Ʈ ������ ������ �ؽ�Ʈ�� ��
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
	//��Ʈ�� Ű�� �������� �������� �����Ѵ�.
	if (IsCtrlPressed())
		sort_by_text_color(pNMLV->iSubItem, -1);
	else
		sort(pNMLV->iSubItem, -1);

	*pResult = 0;
}


BOOL CVtListCtrlEx::PreCreateWindow(CREATESTRUCT& cs)
{
	// TODO: ���⿡ Ư��ȭ�� �ڵ带 �߰� ��/�Ǵ� �⺻ Ŭ������ ȣ���մϴ�.
	//AfxMessageBox(_T("sldkf"));
	return CListCtrl::PreCreateWindow(cs);
}

void CVtListCtrlEx::PreSubclassWindow()
{
	// TODO: Add your specialized code here and/or call the base class
	//�������� ������ ��� PreSubclassWindow()�Լ�������
	//GetHeaderCtrl()�� �׻� NULL�� �����Ѵ�.
	//�̸� �����ϱ� ���� Ÿ�̸�, SendMessage���� ����� ����غ����� ���� �ʴ�.



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

	//�����Ʈ���� ������ ���� �ִ��� Ȯ�� �ʿ�.
#if 1
	// get original view style
	//LVS_OWNERDATA�� �������� �������� �ʴµ��ϴ�. resource editor���� �ְ� ��������.
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

	//������ ��Ÿ���� �����ϹǷ� ���⼭�� ����.
	/*
	LONG lStyleOld = GetWindowLongPtr(GetSafeHwnd(), GWL_STYLE);
	lStyleOld &= ~(LVS_TYPEMASK);
	lStyleOld |= (LVS_REPORT | LVS_OWNERDRAWFIXED | LVS_OWNERDATA);

	SetWindowLongPtr(GetSafeHwnd(), GWL_STYLE, lStyleOld | LVS_NOCOLUMNHEADER | LVS_NOSORTHEADER);
	*/

	//ASSERT(pHeader->m_hWnd != NULL);
#endif
	//������ headerCtrl���� ���� �� font ������ �ؾ� ������� �����ϰ� ����ȴ�.
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

		//����̺��� �� ��ǻ�ͷ� back (path = "C:", go to ThisPC)
		if (is_drive_root(path))
		{
			path = thisPC;
		}
		//�Ϲ� ������� ���������� �̵�
		else
		{
			path = GetParentDirectory(path);
		}

		if (m_is_local)
			set_path(path);

		//VtListCtrlEx ���ο��� � �̺�Ʈ�� ���� ��ΰ� ����Ǵ� ����� parent���� �̸� �˷����Ѵ�.
		//set_path���� �޽��� ������ ���Խ�Ű�� recursive call�� �߻��ϹǷ� ������ ȣ���Ѵ�.
		::SendMessage(GetParent()->GetSafeHwnd(), Message_CVtListCtrlEx, (WPARAM) & (CVtListCtrlExMessage(this, message_path_changed, NULL)), (LPARAM)&path);
	}
}

BOOL CVtListCtrlEx::PreTranslateMessage(MSG* pMsg)
{
	// TODO: Add your specialized code here and/or call the base class
	// TODO: ���⿡ Ư��ȭ�� �ڵ带 �߰� ��/�Ǵ� �⺻ Ŭ������ ȣ���մϴ�.

	//���콺 back button up
	if (pMsg->message == WM_XBUTTONUP)
	{
		move_parent_folder();
	}
	else if (pMsg->message == WM_MOUSEWHEEL && m_in_editing)
	{
		return true;

		/*
		//�����߿� ��ũ���� �Ǹ�E�������� ���� �簢?E������ ���ؼ�
		//edit�� �� ��ǥ�� �̵�������� �Ѵ�. client������ ����Ϫ��Eȭ�鿡�� ��������E�ȵȴ�.
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

			//������ ��� BackŰ�� ���� ���� ������ �����Ű�� ���� ������ ����� �� ���� ������
			//main���� ��� ����ϴ��Ŀ� ���� ���ذ� �� ���� �ִ�.
			if (m_is_shell_listctrl)
			{
				if (m_path == m_pShellImageList->m_volume[!m_is_local].get_label(CSIDL_DRIVES))
					return true;

				CString new_path;

				//����̺�� �� PC�� ����
				if (is_drive_root(m_path))
					new_path = m_pShellImageList->m_volume[!m_is_local].get_label(CSIDL_DRIVES);
				//�׷��� ������ ���� ���丮�� �̵�
				else
					new_path = GetParentDirectory(m_path);

				set_path(new_path);
				
				//VtListCtrlEx ���ο��� � �̺�Ʈ�� ���� ��ΰ� ����Ǵ� ����� parent���� �̸� �˷����Ѵ�.
				//set_path���� �޽��� ������ ���Խ�Ű�� recursive call�� �߻��ϹǷ� ������ ȣ���Ѵ�.
				::SendMessage(GetParent()->GetSafeHwnd(), Message_CVtListCtrlEx, (WPARAM) & (CVtListCtrlExMessage(this, message_path_changed, NULL)), (LPARAM)&new_path);
			}
				
			break;
		}

		case 220		:	return true;	//'\'Ű�� ������ ����Ʈ �� ó������ �̵��Ǵ� ���� ����.
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
								//���� ��ΰ� "�� PC"�� ���� �켱 ������ �����Ѵ�.
								if (!m_is_shell_listctrl || get_path() != m_pShellImageList->get_system_label(!m_is_local, CSIDL_DRIVES))
									edit_item(get_selected_index(), m_edit_subItem);
								return true;
							}
							break;
		case VK_F5		:	
							//editing�� ���� F5Ű�� CEdit���� �߻��ϹǷ� ���� ���� �ʴ´�.
							//CEdit�� �Ļ��ؼ� F5 �̺�Ʈ�� �߻��ϸ� ������ �����Ű�� ���ΰ�ħ ����� �Ѵ�.
							if (m_in_editing)
								edit_end();

							refresh_list();
							return true;
		case VK_END :		if (!m_in_editing && GetKeyState(VK_CONTROL) & 0x8000)
							{
								set_auto_scroll(true);
								return true;	//���⼭ true�� �������� ������ CListCtrl�� �⺻ end Ű ó���� ����ǰ� ���� ȭ�鿡�� �� �Ʒ� �׸��� ���õǾ� m_auto_scroll�� �ٽ� false�� ���Ѵ�.
							}
							return false;
		/*
		//Ű���忡 ���� �׸� ���� ó���� ���ο��� �ؾ� �����ϴ�.
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

	//CTreeCtrl, CListCtrl�� ������
	//dc.FillSolidRect(rc, m_theme.cr_back.ToCOLORREF());
	dc.FillSolidRect(rc, m_theme.cr_back.ToCOLORREF());

	if (m_list_db.size() == 0 && !m_text_on_empty.IsEmpty())
	{
		draw_text(g, rc, m_text_on_empty, 10, false, 0, 0, _T("���� ���"));
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
	//�̹� edit_end()���� �ʿ��� ��� ó���� ��ģ �����̹Ƿ� ���⼭�� ���� ó������
	//return FALSE;�Ͽ� main dlg���� LVN_ENDLABELEDIT�� ���� �޽����� ó���ϵ��� �ϸ� �ȴ�.
	/*
	LV_DISPINFO *plvDispInfo = (LV_DISPINFO *)pNMHDR;
	LV_ITEM	*plvItem = &plvDispInfo->item;

	if (plvItem->pszText != NULL && plvItem->pszText != m_edit_old_text)
	{
		m_modified = true;

		//��Ƽ ���õ� ���¿��ٸ� ���õ� �׸� ��� �ش� �÷����� ���� �������ش�.
		//���� ���ο����� OnLvnEndlabeledit()�� ȣ���ؼ�
		//ini�� �ٸ� ���Ͽ��� ����ؾ� �Ѵٸ� �ű⼭�� �� ó���� �� ������Ѵ�.
		//��, ���콺�� �ٸ� ���� Ŭ���ϸ鼭 ������ �����ϴ� ���
		//���⼭ get_selected_items()�� �̿��Ͽ� ���ϸ� �ε����� �޶����� ������ �ȴ�.
		//������ ���۵� �� �̸� ���õ� �׸���� ��ȣ�� ����� ���� �Ѵ�.
		//������ �Ϸ�Ǹ� ���õ� �׸���� ��� ����������
		//������ ���� ������������ ������� �ִ�.
		//���� CtrlŰ�� ������ ������ �����ϴ� ��쿡�� ���õ� �׸���� ��� �����ϵ��� �����Ѵ�.
		if (IsShiftPressed())
		{
			for (int i = 0; i < m_dqSelected_list_for_edit.size(); i++)
				set_text(m_dqSelected_list_for_edit[i], plvItem->iSubItem, plvItem->pszText);
		}
		else
		{
			bool res = true;
			set_text(m_edit_item, m_edit_subItem, plvItem->pszText);

			//local�� ���� MoveFile()�� �����ϸ� undo��Ų��.
			if (m_is_shell_listctrl && m_is_local)
			{
				//���� ���ϸ� ������ �����ؾ� �������� �ؽ�Ʈ�� �����Ѵ�.
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

	//�� �÷��̶� allow_edit�̶�� m_allow_edit�� true�̾�� �Ѵ�.
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

	//�� ó�� subItem�� ������ ���� ���ٸ� -1�� �Ѿ���µ�
	//�׷� ���� 0�� �÷��� �����̶�� �����Ѵ�.
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

	//edit�� �������� ������ �� ES_MULTILINE �Ӽ��� ���� ���
	//edit�� ���̴� 1���̰� �ʺ� �ؽ�Ʈ ���̺��� ���� ��� word wrap�Ǿ� �Ϻ� �ؽ�Ʈ�� �����ٷ� �Ѿ�� ������ �ʰ� �ȴ�.
	//���� SetRect�� �̿��ؼ� ���� �߾ӿ� ǥ���ϴ� �� ES_MULTILINE�� �ݵ�� �ʿ��� ��쿡�� ����Ѵ�.
	//�� CVtListCtrlEx�� single line�� �⺻���� �ϹǷ� ES_MULTILINE�� �����Ѵ�.
	if (m_pEdit == NULL)
	{
		m_pEdit = new CSCEdit;
		m_pEdit->Create(dwStyle | WS_BORDER | WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL /* | ES_MULTILINE*/, r, this, IDC_EDIT_CELL);
	}

	//ES_MULTILINE, ES_LEFT, ES_CENTER, ES_RIGHT ���� ���������� �Ұ����� ��Ÿ���̹Ƿ�
	//edit_end()���� �������� hide�������� delete���� ������.
	//BOOL res = m_pEdit->ModifyStyle(0, dwStyle);
	m_pEdit->MoveWindow(r);
	m_pEdit->SetRect(&r);
	m_pEdit->SetFont(&m_font, true);
	m_pEdit->SetWindowText(m_edit_old_text);

	m_pEdit->ShowWindow(SW_SHOW);
	m_pEdit->SetSel(0, -1);
	m_pEdit->SetFocus();

	//ES_READONLY�� ModifyStyle()�δ� ���������� �� ����. �Ʒ��� ���� EM_SETREADONLY�� ������ �Ѵ�.
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
	//�⺻ �����Ǵ� ���� ����� �̿�E���E�ʰ�EEditSubItem�̶�� �Լ��� �����ؼ� �翁E߱�E������
	//parent���Դ� LVN_BEGINLABELEDIT �޽����� ��E޵���E�ʴ´�.
	//�Ʒ� �޽����� �������� ��E��Ͽ?parent���� �ش�E����Ʈ ��Ʈ���� ������ ���۵Ǿ����� �˸���.
	//parent�� OnLvnBeginlabeleditListCtrl() �޽���E�ڵ鷯���� �ʿ��� ó���� ������.
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

//���� ���� �ؽ�Ʈ�� �ǵ�����.(���� ���̺��� ���ϸ��̰� ���ϸ� ������ ������ ��� �� �� �ִ�.)
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
	//item�� -1�̸� �ش� ���� ������ �ƴ϶� �⺻ �ؽ�Ʈ ������ ���ϴ� ���̴�.
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

//index ��ġ�� 0�� �÷��� text�� ������ �߰��Ѵ�.(-1�̸� �� �������� �߰�)
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
	//�̹� �ش� index�� row�� �����ϴ� ���� ���� �����Ѵ�. => ���� �־�� �ȵǴ� �ڵ���. �� �� ����� �����ߴ��� Ȯ�� �ʿ�!
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

	//LVSICF_NOSCROLL �ɼ��� ���� ������ Ư�� �׸� ���� �� �ش� �׸��� ������ �ʵ��� ��ũ���Ϸ� �ص�
	//�����Ͱ� ��� �߰��Ǵ� ��Ȳ������ ���õ� �׸��� ������ �ʴ� ���������� ��ũ���� ���� �ʴ� ������ �ִ�.
	SetItemCountEx(m_list_db.size(), LVSICF_NOSCROLL);

	//ensureVisible�̸� Invalidate()�� �����ص� �ȴ�.
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

	//������ �����ϴ� �����̶�� ũ��, ������ ��¥�� �������ְ�
	//���ٸ� ����Ʈ�� �߰��Ѵ�.
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

//���� ������ �� ������ �����ϰ� �������� ǥ���Ѵ�.
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

	//���� ������ ������ �� ����Ʈ�� ����� �߰��Ѵ�.
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

//���� �������� "�� ����" ���� �� �ε����� ���Ѵ�. ex. "�� ���� (2)"
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

		//�� ')'�� ã��
		int start_paren = -1;
		int end_paren = folder.ReverseFind(')');
		int found_index = -1;

		if (end_paren > 0)
		{
			//���� '('�� ã�Ƽ� �� ������ ���ڸ� ����
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

	//set �׸� �� ����ִ� �ε����� �������ش�.
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

	//���� 1 ~ n���� ��� ������ ���������� ����ִٸ� 1 ������ ���� �������ָ� �ȴ�.
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

//����� �� Ȯ��â�� ȣ���ƾ���� ó���ؾ� ��
//shell�� ���� ���� ���� �Ǵ� ������ ������ �Ŀ� ��Ͽ����� �������Ѵ�.
//local �����̶�� ���⼭ ó���ص� ��������� remote�� �����ϼ��� ������ ���ο��� ó������.
//���� ������ �����ϴ� �ڵ�� ���ο��� ó���� �� ������ �����ϸ� ����Ʈ������ �����.
void CVtListCtrlEx::delete_selected_items()
{
	int i;
	int index;
	std::deque<int> dqSelected;
	get_selected_items(&dqSelected);

	//�ڿ������� ���� �ָ�.
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

//shell_listctrl�̶�� ���� ���� ������ �� �Ķ���͸� �� �� �ְ� ���� ������ ���� �Ŀ� ����Ʈ������ �����ؾ� �Ѵ�.
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
	//shell_listctrl�� ��� fullpath��� ���ϸ� �Ǵ� ������ �����Ѵ�.
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

		//����Ʈ���� �����
		delete_item(index);

		//����, ���� ��Ͽ����� �����ش�.
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

	//����� ��������2���� ���� �� ��Ҹ� ������ ��Ȥ m_list_db�� size�� �ſ� ū ���� ������ ��찡 �ִ�.
	//������ �ľ����̸� �켱 �ӽ÷� ó���Ѵ�.
	if (m_list_db.size() > 10000)
		return;

	m_list_db[item].text[subItem] = text;
	if (invalidate)
		InvalidateRect(get_item_rect(item, subItem), false);
}

//�� ������ �� �÷����� separator�� ���еǾ� �ִ� ���� ���� �����ͷ� index��ġ���� ����Ʈ�� ä���.
//reset = true�̸� ��� �����͸� ������ �� ���� ä���. �� �� index�� 0���� ���µȴ�.
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

//txt ��� �� �÷� ���� ������ �°� ����ϵ��� ������ �����Ͽ� ����
//CString�� �ִ� 64K������ ���� �� �����Ƿ� vector�� �̿���.
void CVtListCtrlEx::get_line_text_list(std::vector<CString>* vt)
{

}

//shell_listctrl�� �� ������ Ž���⿡�� ����/������ ���̺��� �����ϴ� �̺�Ʈ�� �߻��ϸ�
//main���� �� �Լ��� ȣ���Ͽ� ���̺��� �����Ѵ�.
//shell_listctrl�� �ƴϾ ��� �����ϳ� ���̺�� ���ϹǷ� ������ �׸��� ���� ���� ���� ó������ �����Ƿ�
//������� �� ��!
void CVtListCtrlEx::rename(CString old_text, CString new_text)
{
	//shell_listctrl�� ��� fullpath��� ���ϸ� �Ǵ� ������ �����Ѵ�.
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

		//����Ʈ���� label�� �����Ű��
		set_text(index, col_filename, new_label);

		//����, ���� ��Ͽ����� ��������ش�.

		//�켱 ���� ��Ͽ��� ã�ƺ���
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

		//������ ���� ��Ͽ��� ã�Ƽ� �������ش�.
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

//���õ� �׸���� dqSelected�� ��´�. dqSelected�� null�̸� �׳� ���� ������ ���Ϲ޾� ����Ѵ�.
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

//���õ� �׸���� ����� dq�� ��´�. shelllist�� ��� fullpath = true�̸� �� �׸��� ��ü��θ� ��´�.
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

//���õ� �׸���� ����� dq�� ��´�. shelllist������ ���Ǹ� cFileName�� �̹� ��ü��θ� ������ �ִ�.
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

//�������� ���°��� Ư�� ���°��� �׸� �Ǵ� �� ���� ���ϱ�
//LVIS_DROPHILITED or LVIS_SELECTED �׸��� ���� �� �ִ�.
//drag ���߿� ���콺�� �ٸ� �� �������� ������ WM_LBUTTONUP �� ��� drophilited ���·� �������� ���� ������ �����ϱ� ����.
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

//dq ����� �����۵��� state ����. dq�� null�̸� ��� �׸� ���� ����
//���� : set_items_with_state(LVIS_SELECTED|LVIS_FOCUSED, LVIS_SELECTED|LVIS_FOCUSED, dq);
//���� : set_items_with_state(0, LVIS_SELECTED|LVIS_FOCUSED, dq);
int CVtListCtrlEx::set_items_with_state(UINT state, UINT mask, std::deque<int>* dq)
{
	//��� �׸� ���� ����
	if (dq == NULL)
	{
		for (int i = 0; i < size(); i++)
		{
			SetItemState(i, state, mask);
		}
	}
	//�ش� �׸�鿡 ���� ����
	else
	{
		for (int i = 0; i < dq->size(); i++)
		{
			SetItemState(dq->at(i), state, mask);
		}
	}
	
	return 0;
}

//0�� �÷������� �����͸� ã�´�.
int CVtListCtrlEx::find_string(CString str, int start, bool bWholeWord, bool bCaseSensitive)
{
	/*
	LVFINDINFO info;

	info.flags = (bWholeWord ? LVFI_STRING : LVFI_PARTIAL|LVFI_STRING);
	info.psz = str;

	// Delete all of the items that begin with the string.
	return FindItem(&info, indexFrom);
	*/
	//virtual list�� �ƴ϶�� �� �ڵ尡 �����ϰ�����
	//virtual list��� OnLvnOdfinditem()�Լ��� �ٽ� ȣ��ǰ�
	//�ű⼭ �ᱹ ã�� �ڵ尡 ���� �Ѵ�.
	//�⺻ FindItem�Լ��� �̿��Ѵٸ� �� �Լ� �ȿ� �˻� �ڵ带 �߰��ؾ߰�����
	//���⼭ ������ �����ؼ� �������.
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

//CListCtrl�� FindItem���ε� ��� ������ ��� �÷��� �˻��� �� ������
//�ϴ� ���� �����ؼ� ����.
//�� ��� result���� �÷���ȣ�� ������� �ʰ� ���ι�ȣ�� ����ǹǷ�
//�� ���ο��� �̹� ã�Ҵٸ� �� ������ �ٸ� �÷������� ã�� �ʰ� �׳� break�ؾ� �Ѵ�.
//�׷��� ������ ������ ���� ��ȣ�� result�� ��� ����ȴ�.
//result_reset�� true�̸� �˻� ����� ������ �� �˻��ϰ� false�̸� �߰��� �ִ´�.
//'|'�� �����ϸ� �� �ܾ���� �ϳ��� ���Ե� ����� �����ϰ�(OR)
//'&'�� �����ϸ� �� �ܾ ��� �� �����(AND)
//���� ��ȣ�� ������ wholeword�� �˻��Ѵ�.
//��ǰ���� ��� �����빮��-������ ������ �����Ƿ� '-'�� ���ٸ� ���� �տ� �ڵ� �־��ش�.
int CVtListCtrlEx::find_string(CString find_target, std::deque<int>* result,
								int start_idx, int end_idx,
								std::deque<int>* dqColumn, bool stop_first_found)
{
	int		i;
	TCHAR	op;
	CString sText;
	std::deque<CString> dqTarget;	//separator�� ""�� �ƴ� ���� ��ū���� �и��Ͽ� ��� ã�´�.
	std::deque<int> dq_columns;

	if (start_idx < 0)
		start_idx = 0;

	if (end_idx < 0 || end_idx >= size())
		end_idx = size() - 1;
	if (end_idx < 0)
		return 0;

	//ã�� �׸��� �������� ǥ���ϴ� ��� ���� ���� ���õ� �׸���� �ʱ�ȭ�����ش�.
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
		//sline ���ڿ����� dqTarget ���ڿ����� �����ϴ��� op��Ŀ� ���� �˻�.
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
		//�� ������ �����͸� �ϳ��� ��Ʈ�� ����Ʈ�� ���ͼ� ��
		std::deque<CString> dqLine = get_line_text_list(i, column_start, column_end);

		//functions�� find_string���� �����Ͽ����� �ν��� �ȵǼ� find_dqstring���� �̸� ����
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

	//dbclick�� �������� ����ϰų� � �׼����� ����ϴ� ����
	//�� Ŭ������ ����ϴ� ���ο��� �����ϴ� ���� �´�.
	//���⼭ �����ϸ� ���� ��쵵 ������ ���뼺�� ��������.
	//=>shell_listctrl�̶�� ���⼭ ó���ϴ� ���� �� �������ε��ϴ�.
	//�ٸ� ����Ŭ������ ��ΰ� ����Ǵ� �̺�Ʈ�� �߻��� ���̹Ƿ�
	//�̸� parent���� �˷���� CPathCtrl, CSCTreeCtrl�� ��ΰ� ����� ���̴�.
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
			//������ ���� ����δ� �ƹ�ó������ �ʴ´�.
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

		//shiftŰ��?E���յǸ�E?E���݁E�����Ѵ�.
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
				int result = AfxMessageBox(_T("�ٿ����� �������� �÷� ������ ��E�E����Ʈ�� �÷� ������ �ٸ��ϴ�.\n�״�E?�ٿ��ֱ�E�ұ�?E"), MB_YESNO);
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

//���Ͽ��� �ҷ��ͼ� ����Ʈ�� ä����. �÷��� ���� �����ؾ� �Ѵ�.
//�÷� ������ �ٸ� ������ ���ϵ��� �˾Ƽ� �ҷ�����E�ʴ´�.
//�̹� �÷��� ������ �Ƚ��Ǿ�E�ְ�E�� �������� ����� ���ϸ� �ҷ������� �Ǿ�E�ִ�.
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

//����Ʈ�� ����E?���Ϸ� �����Ѵ�.
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

//�� �Լ������� m_lf ������ �̿��ؼ� ��Ʈ�� ������Ѵ�.
//��, m_lf.lfHeight ���� �̿��ؼ� ��Ʈ�� ��������Ƿ�
//m_font_size ��� ������ ���� ����Ǿ��ٸ�
//������ �̿��� �̸� m_lf.lfHeight ������ ������ �� �� �Լ��� ȣ��Ǿ�� �Ѵ�.
//m_lf.lfHeight���� ���밪�� MM_TEXT����� ���� DC������ ���� �ȼ�ũ�Ⱑ �ȴ�.
//���� ��ũ�� ũ�� ���� ����Ҷ��� m_font_size�� �̿��ϴ°� �ƴ϶�
//m_lf.lfHeight���� �̿��ؾ� ��Ȯ�� ��ũ�� ũ�Ⱑ ���ȴ�.
//m_font_size�� ���� ����ڿ��� �Ϲ����� ��Ʈ ũ�� ���� ��ġ�� ���̴� �������� ��ġ�̴�.
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

//� �׸��� Ŭ���� �� ����Ű F2�� ������ �ش� �ؽ�Ʈ�� �����ϴ� �뵵�̹Ƿ�
//�� �� Ŭ���� subItem�� ����س��´�.
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
		//name������ Ŭ���� �� 1~2�� ���̿� �ٽ� Ŭ���ϸ� name �������� ��ȯ�ȴ�.
		/*
		if ((m_edit_item == item) &&
			(m_edit_subItem == subItem) &&
			(abs(t1 - m_last_clicked) >= 800) &&
			(abs(t1 - m_last_clicked) < 1600))
		*/
		//���õ� �׸��� �ٽ� ��Ŭ���ϸ� �������� ��ȯ�Ѵ�.
		if (m_allow_one_click_edit &&
			(m_edit_item == item) &&
			(m_edit_subItem == subItem) &&
			(GetSelectedCount() == 1) &&
			(clock() - m_last_clicked_time > 500) &&	//�� ���� ������ ����Ŭ������ �����ǰ�
			(clock() - m_last_clicked_time < 2000))
		{
			//Ʈ������ "�� PC"�� �����Ͽ� ����Ʈ�� ����̺� ����Ʈ�� ǥ�õ� ���¿����� �ð��� Ŭ���� ���� ������ �������� �ʴ´�.
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

//m_is_shell_list��� �̸� ������ ���� main�� �ƴ� ���⼭ ó���ص� �ɵ��ϴ�.
//��, �ߺ��Ǵ� �̸��� �����Ͽ� ������ ���� main���� �̸� �˷� �޽����� ǥ���ؾ� �Ѵ�.
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

	//shell listctrl�� label�� ����Ǹ� ���� ����/������ ��������� �Ѵ�.
	if (m_is_shell_listctrl)
	{
		CString path = m_pShellImageList->convert_special_folder_to_real_path(!m_is_local, get_path());//convert_special_folder_to_real_path(get_path(), m_pShellImageList, !m_is_local);
		bool res = false;

		CString old_path = concat_path(path, m_edit_old_text);
		CString new_path = concat_path(path, m_edit_new_text);

		if (m_is_local)
		{
			//�̹� ������ �׸��� �����ϸ� parent���� �˷� �޽����� ǥ���ϵ��� �Ѵ�.
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

	//�� ��Ʈ�ѿ� LVN_ENDLABELEDIT �̺�Ʈ�� ���� �⺻ �ڵ鷯���� ó���� ���� �ִٸ� ó���Ѵ�.
	GetParent()->SendMessage(WM_NOTIFY, GetDlgCtrlID(), (LPARAM)&dispinfo);
}

//� �׸��� Ư�� ��ġ�� ǥ�õǵ��� ��ũ�ѽ�Ų��.
//mode�� visible_first�̰� offset�� 3�̸� ������ 3�� �ε���, �� 4��° ��ġ�� �ش� �������� ǥ�õǵ��� ��ũ�ѽ�Ų��.
//mode�� visible_center�̰� offset�� 0�̸� �߾� ��ġ�� �ش� �������� ǥ�õǵ��� ��ũ�ѽ�Ų��.
//mode�� visible_last�̰� offset�� 3�̸� �Ʒ����� -3-1�� ��ġ�� �ش� �������� ǥ�õǵ��� ��ũ�ѽ�Ų��.
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
	// TODO: ���⿡ �޽��� ó���� �ڵ带 �߰� ��/�Ǵ� �⺻���� ȣ���մϴ�.
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
	// TODO: ���⿡ �޽��� ó���� �ڵ带 �߰� ��/�Ǵ� �⺻���� ȣ���մϴ�.
	lpMeasureItemStruct->itemHeight = m_line_height;
	//CListCtrl::OnMeasureItem(nIDCtl, lpMeasureItemStruct);
}

void CVtListCtrlEx::set_as_shell_listctrl(CShellImageList* pShellImageList, bool is_local, CString default_path)
{
	m_is_shell_listctrl = true;
	m_is_local = is_local;
	m_use_own_imagelist = true;

	m_pShellImageList = pShellImageList;
	//�� ������ ����� �ٸ� Ŭ�������� CListCtrl* Ÿ�����ε� GetImageList()�� ���� ������ �� �ִ�.
	SetImageList(m_pShellImageList->get_imagelist(), LVSIL_SMALL);

	if (GetUserDefaultUILanguage() == 1042)
		set_headings(_T("�̸�,200;ũ��,100;������ ��¥,150"));
	else
		set_headings(_T("Name,200;Size,100;Date mofified,150"));
	set_font_size(9);
	//set_font_name(_T("���� ���"));
	//set_font_size(), set_font_name()�� ȣ������ �ʰ� set_header_height()�� ȣ���ϸ�
	//CHeaderCtrlEx::OnLayout()���� ������ �߻��Ѵ�.
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

//list�� index�� �ָ� fullpath�� �����Ѵ�. -1�̸� ���� path�� �����Ѵ�.
CString CVtListCtrlEx::get_path(int index)
{
	//"D:\Temp"�� ���� �����ؾ��ϴµ� "D:\\Temp"�� ���� ���ϵǴ� ������ �ִ�.
	//�켱 �ߺ��� ���������� �����ϰ� �������ش�.
	//���� ��� path�� �ݵ�� '\'�� �������� �ϴ� ��Ģ�� ���� ���� ����.
	//m_path.Replace(_T("\\\\"), _T("\\"));

	if (index < 0)
		return m_path;

	CString fullpath;

	fullpath = concat_path(m_path, get_text(index, col_filename));

	return m_pShellImageList->convert_special_folder_to_real_path(!m_is_local, fullpath);//convert_special_folder_to_real_path(fullpath, m_pShellImageList, !m_is_local);
}

//���� ���õ� �׸��� �����̸� �ش� ��α����� fullpath��, �����̶�� ���� ����Ʈ�� ��θ� �����Ѵ�.
CString	CVtListCtrlEx::get_selected_path()
{
	CString path;

	if (!m_is_shell_listctrl)
		return path;

	int index = get_selected_index();
	if (index < 0)
		return path;

	path = get_path();

	//�� PC ����� ǥ�õǰ� �ְų� ������ ���� ���õ� �׸������ ��θ� �����Ѵ�.
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

//�ش� �ε����� ����/������ WIN32_FIND_DATA ���� �����Ѵ�.
//�ʱ⿡�� �� �׸��� label�� m_cur_folders, m_cur_files���� ã������
//�� ������ �������� ������ �������� �ȴ�.
//display_filelist()�� �� �� m_cur_folders, m_cur_files �׸��� �߰��ϸ鼭
//SetItemData(index, no);�� �ش� �׸��� �� �� �迭���� ���°���� ������ ��
//GetItemData(index);�� ���� �ش� �׸��� WIN32_FIND_DATA ���� �������� ������.
WIN32_FIND_DATA	CVtListCtrlEx::get_win32_find_data(int index)
{
	int idx = GetItemData(index);
	if (idx >= 0 && idx < m_cur_folders.size())
		return m_cur_folders[idx].data;
	return m_cur_files[idx - m_cur_folders.size()].data;
}

//�ش� �ε����� ����/������ WIN32_FIND_DATA ���� �����Ѵ�.
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

	//C ����̺�� "C:\\"��, "C:\\temp\\"�� "C:\\temp"�� �����Ѵ�.
	//��, ����̺� ��Ʈ�� �������ð� ������ �� ���� ������ ������ �ʾƾ� �ϹǷ� ���Ž����ش�.
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

	//UI�� list�� ������ ��� clear������
	//remote�� ���� reload�� false�̰� ����/���� ����� ä����ä�� �� �Լ������� �����Ƿ�
	//����/���� ��ϱ��� clear�ؼ� �ȵȴ�.
	delete_all_items(reload);

	//local�� ���� ���ϸ���� �ٽ� �о ǥ���Ѵ�.
	//sort�� ��� �Ǵ� remote�� ���� ����� m_cur_folders, m_cur_files�� ���� ǥ���ϸ� �ȴ�.
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
			//message_path_changed�� ������ main���� remote ���ϸ���� �� display_filelist(m_path);���� ȣ���Ѵ�.
			::SendMessage(GetParent()->GetSafeHwnd(), Message_CVtListCtrlEx, (WPARAM) & (CVtListCtrlExMessage(this, message_path_changed, NULL)), (LPARAM)&m_path);
		}
	}
	else
	{
		//sort()�� ���� ������� �� ��� ��ϸ� �ٽ� ǥ���Ѵ�.
		display_filelist(m_path);
	}
}

//m_cur_folders�� m_cur_files�� ä���� ������� ����Ʈ�� ��½�Ų��.
void CVtListCtrlEx::display_filelist(CString cur_path)
{
	int i;
	int index;
	int insert_index = -1;
	int img_idx = -1;

	m_path = cur_path;

	SetRedraw(FALSE);
	::SetCursor(AfxGetApp()->LoadStandardCursor(IDC_WAIT));

	//���� �غ���� ���� ���¿��� ȣ��� ���� �׳� ����.
	if (m_cur_sort_column >= m_column_sort_type.size())
		return;

	if (cur_path == m_pShellImageList->m_volume[!m_is_local].get_label(CSIDL_DRIVES))
	{
		set_header_text(col_filesize, GetUserDefaultUILanguage() == 1042 ? _T("��� ������ ����") : _T("Free Space"));
		set_header_text(col_filedate, GetUserDefaultUILanguage() == 1042 ? _T("��ü ũ��") : _T("Total Size"));
		set_column_text_align(col_filedate, LVCFMT_RIGHT);
	}
	else
	{
		set_header_text(col_filesize, GetUserDefaultUILanguage() == 1042 ? _T("ũ��") : _T("Size"));
		set_header_text(col_filedate, GetUserDefaultUILanguage() == 1042 ? _T("������ ��¥") : _T("Date modified"));
		set_column_text_align(col_filedate, LVCFMT_LEFT);
	}

	//�ش� �������� ����, ������ ���� ������ �� �޽����� ���� �ʿ䵵 ����.
	if (m_cur_folders.size() + m_cur_files.size() > 0)
	{
		::SendMessage(GetParent()->GetSafeHwnd(), Message_CVtListCtrlEx,
			(WPARAM) & (CVtListCtrlExMessage(this, message_list_processing, NULL, _T(""), _T(""), WPARAM(m_cur_folders.size() + m_cur_files.size()))), (LPARAM)(-1));
	}


	//asc�� ���		: ���� ���� 0�� ��ġ���� ǥ���ϰ� ������ ���� ����Ʈ�� �� ���� �߰��Ѵ�.
	//desc�� ���	: ������ ���� 0������ ǥ���ϰ� ������ i��°�� �߰��ϸ� �ȴ�. ������ �ڿ��� �ڷ� �и��鼭 ���ϵ麸�� �Ʒ���	 ǥ�õȴ�.
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

//���� �Ǵ� ������ �ش��ϴ� ��� ����Ʈ�� �߰��Ѵ�.
//local�� ��� ũ��� ��¥�� ����ִٸ� �ڵ� ä���ְ� remote��� ��������� �ȵȴ�.
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
//�� �Լ��� �巡�� �̹����� ���� �������ִ� �ڵ����� ������� ���� �ڵ��̹Ƿ� ��� ����! ���� �Ұ�.
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

	// ��Ƽ�˸��ƽ� �ȵ� ��Ʈ�� ����ϴ°� �ٽ�
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
	// TODO: ���⿡ ��Ʈ�� �˸� ó���� �ڵ带 �߰��մϴ�.
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

	//focus�� ���ų� ���õ��� ���� ���¿��� �ٷ� drag�� ���۵Ǹ�
	//drag �̹����� ǥ�õǹǷ� focus�� �ְ� drag�ϰ� �ִ� �������� ���û��·� ǥ������� �Ѵ�.
	//"���� ���� �׻� ǥ��" �Ӽ� ���� true���� �Ѵ�.
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
	
	//drag_image�� ���ٸ� ��� ��ü ������ �� ���̺��� �̿��Ѵ�.
	//GDI�� �̿��ؼ� create_drag_image()�� ��������� �����ܰ� �Բ� ���̺��� ����� �� ��������. ���� �ʿ�.
	//GDIPlus�� �̿��� create_drag_image()�� ���� ����� �͵� ��������.
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
	// TODO: ���⿡ �޽��� ó���� �ڵ带 �߰� ��/�Ǵ� �⺻���� ȣ���մϴ�.
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
		//TME_HOVER�� ������ ���콺�� hover�Ǹ� �ڵ����� focus�� ������.
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

		//��� ��� ��Ʈ���� ���, �ϴܿ� ���콺�� hovering�Ǹ� �ڵ� ��ũ�� ������� �Ѵ�.
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

			//target listctrl�� drag&drop ������ �ƴϸ� �׳� ����.
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
			// Highlight it (������ ��쿡�� hilite��Ų��)
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
	// TODO: ���⿡ �޽��� ó���� �ڵ带 �߰� ��/�Ǵ� �⺻���� ȣ���մϴ�.
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

		//ListCtrl���� drag�Ͽ� drophilited�� ǥ�õ� ���¿��� ������ ���콺�� ������ �̵���Ű��
		//���콺�� ���� drophilited�� �׸� ǥ�ð� ������ ���´�.
		//���ο� �޽����� ������ �ش� ��Ʈ�ѵ��� �����ۿ��� drophilited�� ���Ž������ �Ѵ�.
		::SendMessage(GetParent()->GetSafeHwnd(), Message_CVtListCtrlEx, (WPARAM) & (CVtListCtrlExMessage(this, message_drag_and_drop, NULL)), (LPARAM)0);
	}

	CListCtrl::OnLButtonUp(nFlags, point);
}

void CVtListCtrlEx::DroppedHandler(CWnd* pDragWnd, CWnd* pDropWnd)
{
	//drop�Ǹ� �� �̺�Ʈ�� ���ο� �˸���
	//���ο����� drag���� ������ drop������ �̿��ؼ� ���ϴ� ó���� �Ѵ�.
	//���� �� �Ʒ� ::SendMessage�� �ʿ��ϸ�
	//�߰� �ڵ���� ���ο��� Ȱ���ϴ� ����� ���� ������.

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
	// TODO: ���⿡ ��Ʈ�� �˸� ó���� �ڵ带 �߰��մϴ�.
	*pResult = 0;

	return FALSE;
}

BOOL CVtListCtrlEx::OnLvnItemchanged(NMHDR* pNMHDR, LRESULT* pResult)
{
	LPNMLISTVIEW pNMLV = reinterpret_cast<LPNMLISTVIEW>(pNMHDR);
	// TODO: ���⿡ ��Ʈ�� �˸� ó���� �ڵ带 �߰��մϴ�.
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

	//���� �׸���� ���������� �ִ� �簢���� ���ϰ�
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

	//�� �׸� ������ ĸó�ؼ� bmp�� �ٿ��ش�.
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
	// TODO: ���⿡ �޽��� ó���� �ڵ带 �߰��մϴ�.
}


void CVtListCtrlEx::OnMouseHover(UINT nFlags, CPoint point)
{
	// TODO: ���⿡ �޽��� ó���� �ڵ带 �߰� ��/�Ǵ� �⺻���� ȣ���մϴ�.
	TRACE(_T("list. hover\n"));
	CListCtrl::OnMouseHover(nFlags, point);
}


void CVtListCtrlEx::OnMouseLeave()
{
	// TODO: ���⿡ �޽��� ó���� �ڵ带 �߰� ��/�Ǵ� �⺻���� ȣ���մϴ�.
	//TRACE(_T("list. leave\n"));
	m_is_hovering = false;
	CListCtrl::OnMouseLeave();
}

//HAS_STRING, OWNER_DRAW_FIXED �Ӽ��� ������ CListCtrl�� Get/SetItemData() �Լ��� ����� �� ����.
//�� �� �Լ��� ����� �� �ֵ��� CListCtrlData�� data ����� �߰��ϰ� ���� �Լ����� override�Ͽ� ������.
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

	// TODO: ���⿡ �޽��� ó���� �ڵ带 �߰��մϴ�.
	m_has_focus = true;
}


void CVtListCtrlEx::OnKillFocus(CWnd* pNewWnd)
{
	CListCtrl::OnKillFocus(pNewWnd);

	// TODO: ���⿡ �޽��� ó���� �ڵ带 �߰��մϴ�.
	m_has_focus = false;
}

//remote�� ���� fullpath�� �ش� ������ WIN32_FIND_DATA���� ���� �� ��찡 �ִ�.
void CVtListCtrlEx::get_remote_file_info(CString fullpath, WIN32_FIND_DATA* data)
{
	int i;

	//lambda�� �̿��� ����ε� �켱 ����.
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
	// TODO: ���⿡ �޽��� ó���� �ڵ带 �߰� ��/�Ǵ� �⺻���� ȣ���մϴ�.
	UINT uFlags = 0;
	//int item = HitTest(point, &uFlags);

	int item = -1;// = pNMItemActivate->iItem;
	int subItem = -1;// = pNMItemActivate->iSubItem;	<== invalid index returned when user clicked out of columns

	//include_icon = false�� �ָ� �������� Ŭ���ص� �ش� �׸��� ���õ��� �ʴ´�.
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
	// TODO: ���⿡ �޽��� ó���� �ڵ带 �߰� ��/�Ǵ� �⺻���� ȣ���մϴ�.
	//TRACE(_T("OnNcHitTest\n"));
	return CListCtrl::OnNcHitTest(point);
}


void CVtListCtrlEx::OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar)
{
	// TODO: ���⿡ �޽��� ó���� �ڵ带 �߰� ��/�Ǵ� �⺻���� ȣ���մϴ�.
	TRACE(_T("CVtListCtrlEx::OnHScroll\n"));
	if (m_in_editing)
		edit_end();
	CListCtrl::OnHScroll(nSBCode, nPos, pScrollBar);
}


void CVtListCtrlEx::OnVScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar)
{
	// TODO: ���⿡ �޽��� ó���� �ڵ带 �߰� ��/�Ǵ� �⺻���� ȣ���մϴ�.
	TRACE(_T("CVtListCtrlEx::OnVScroll\n"));
	if (m_in_editing)
		edit_end();
	CListCtrl::OnVScroll(nSBCode, nPos, pScrollBar);
}


void CVtListCtrlEx::OnLvnBeginScroll(NMHDR* pNMHDR, LRESULT* pResult)
{
	// �� ����� ����Ϸ��� Internet Explorer 5.5 �̻��� �ʿ��մϴ�.
	// _WIN32_IE ��ȣ�� 0x0560���� ũ�ų� ���ƾ� �մϴ�.
	LPNMLVSCROLL pStateChanged = reinterpret_cast<LPNMLVSCROLL>(pNMHDR);
	// TODO: ���⿡ ��Ʈ�� �˸� ó���� �ڵ带 �߰��մϴ�.
	//TRACE(_T("CVtListCtrlEx::OnLvnBeginScroll\n"));
	*pResult = 0;
}


void CVtListCtrlEx::OnLvnEndScroll(NMHDR* pNMHDR, LRESULT* pResult)
{
	// �� ����� ����Ϸ��� Internet Explorer 5.5 �̻��� �ʿ��մϴ�.
	// _WIN32_IE ��ȣ�� 0x0560���� ũ�ų� ���ƾ� �մϴ�.
	LPNMLVSCROLL pStateChanged = reinterpret_cast<LPNMLVSCROLL>(pNMHDR);
	// TODO: ���⿡ ��Ʈ�� �˸� ó���� �ڵ带 �߰��մϴ�.
	//TRACE(_T("CVtListCtrlEx::OnLvnEndScroll\n"));
	*pResult = 0;
}

void CVtListCtrlEx::OnSize(UINT nType, int cx, int cy)
{
	CListCtrl::OnSize(nType, cx, cy);

	// TODO: ���⿡ �޽��� ó���� �ڵ带 �߰��մϴ�.
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
	//���� �ϴܿ� auto_scroll ��ư�� ǥ���Ѵ�.
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
