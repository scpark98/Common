// VtListCtrlEx.cpp : implementation file
//

//#include "stdafx.h"
#include "VtListCtrlEx.h"

#include <algorithm>

#include "../../colors.h"
#include "../../MemoryDC.h"
#include "../../GdiPlusBitmap.h"
#include "EditCell.h"
#include <afxvslistbox.h>

#define IDC_EDIT_CELL	1001

// CVtListCtrlEx

IMPLEMENT_DYNAMIC(CVtListCtrlEx, CListCtrl)

CVtListCtrlEx::CVtListCtrlEx()
{
	set_color_theme(color_theme_default, false);

	memset(&m_lf, 0, sizeof(LOGFONT));

	NCOverride = FALSE; //False as default...
	Who = SB_BOTH; //Default remove both...
}

CVtListCtrlEx::~CVtListCtrlEx()
{
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
	ON_NOTIFY_REFLECT_EX(NM_CLICK, &CVtListCtrlEx::OnNMClickList)
	ON_WM_DROPFILES()
	ON_WM_MEASUREITEM_REFLECT()
	ON_NOTIFY_REFLECT(LVN_BEGINDRAG, &CVtListCtrlEx::OnLvnBeginDrag)
	ON_WM_MOUSEMOVE()
	ON_WM_LBUTTONUP()
	ON_NOTIFY_REFLECT_EX(LVN_ITEMCHANGED, &CVtListCtrlEx::OnLvnItemchanged)
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
	//int			nSavedDC	= pDC->SaveDC();
	CRect		rowRect;
	CRect		itemRect;
	CRect		textRect;
	COLORREF	crText = m_crText;
	COLORREF	crBack = m_crBack;
	//COLORREF	crProgress = m_crProgress;
	bool		is_show_selection_always = (GetStyle() & LVS_SHOWSELALWAYS);
	//TRACE(_T("is_show_selection_always = %d\n"), is_show_selection_always);

	for (iSubItem = 0; iSubItem < get_column_count(); iSubItem++)
	{
		//���� ���� ����, focus���� � ���� ���� �׷��� ���ڻ�, ������ ����ְ�...

		//LVIR_BOUNDS�� ���� ��� 0�� �÷��� �� ������ �簢�� ������ �����Ѵ�.
		//0�� �÷� ���ĺ��ʹ� �ش� ���� �簢�� �������� �����Ѵ�.
		GetSubItemRect(iItem, iSubItem, LVIR_BOUNDS, rowRect);
		itemRect = rowRect;

		if (iSubItem == 0)
		{
			itemRect.right = itemRect.left + GetColumnWidth(0);
		}

		//if(lpDIS->itemState & ODS_SELECTED) //ok
		//��Ŀ���� �����ų� Always Show Selection�̶�� ���� �׸��� ������ ǥ�����ְ�
		if (((GetFocus() == this) || is_show_selection_always) && GetItemState(iItem, LVIS_SELECTED)) //ok
		{
			if (GetFocus() == this)
			{
				crText = m_crTextSelected;
				crBack = m_crBackSelected;
			}
			else
			{
				crText = m_crTextSelectedInactive;
				crBack = m_crBackSelectedInactive;
			}

			//���� �׸��� �ؽ�Ʈ ������ ������ �÷� ��Ŵ�� �����°� �ƴ϶�
			//������ ���� ������ �� ������ ǥ���ϴ°� �������ϴ�.
			//������ ������ �׳� �÷� ��Ŵ�� ������.
			if (m_list_db[iItem].crText[iSubItem] != listctrlex_unused_color)
				crText = m_list_db[iItem].crText[iSubItem];
		}
		//drophilited��� active�� ������� drop hilited �������� ǥ���Ѵ�.
		//�� ��� �׸��� ������ ���� drop hilited ǥ�ø� ���� �ʴ´�.
		else if (GetItemState(iItem, LVIS_DROPHILITED)) //ok
		{
			crText = m_crTextSelected;
			crBack = m_crBackSelected;
		}
		else
		{
			crText = m_list_db[iItem].crText[iSubItem];
			if (crText == listctrlex_unused_color)
				crText = m_crText;

			crBack = m_list_db[iItem].crBack[iSubItem];
			if (crBack == listctrlex_unused_color)
			{
				if (iItem % 2)
					crBack = m_crBackAlt;
				else
					crBack = m_crBack;
			}
		}

	
		pDC->FillSolidRect(itemRect, crBack);

		//percentage Ÿ���̸� �ٱ׷��� ���·� �׷��ְ�
		if (get_column_data_type(iSubItem) == column_data_type_percentage_bar)
		{
			//pDC->FillSolidRect(itemRect, crBack);

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

			if (m_crPercentage.size() == 1)
			{
				pDC->FillSolidRect(r, m_crPercentage[0]);
			}
			else if (m_crPercentage.size() > 1)
			{
				//���� ������ �´� �ܻ����� ä�� ���
				if (false)
				{
					pDC->FillSolidRect(r, get_color(m_crPercentage[0], m_crPercentage[1], d));
				}
				//���� �������� �׶���Ʈ�� ä�� ���
				else
				{
					std::deque<COLORREF> dqColor;
					dqColor.push_back(m_crPercentage[0]);
					dqColor.push_back(get_color(m_crPercentage[0], m_crPercentage[1], d));
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
			//std::deque<COLORREF> dqColor(m_crPercentage);
			//dqColor.push_back(crBack);
			//gradient_rect(pDC, r, dqColor, false);

			if (m_crPercentage.size() == 1)
			{
				pDC->FillSolidRect(r, m_crPercentage[0]);
			}
			else if (m_crPercentage.size() > 1)
			{
				std::deque<COLORREF> dqColor;
				dqColor.push_back(m_crPercentage[0]);
				int hue0 = get_hue(m_crPercentage[0]);
				int hue1 = get_hue(m_crPercentage[1]);
				dqColor.push_back(get_color(hue0, hue1, (int)(d * 100.0), 1.0f, 1.0f));
				gradient_rect(pDC, r, dqColor, false);
			}

			int i;
			int half = (int)((double)(r.Height()) / 2.2);
			for (i = r.left + half; i < itemRect.right - half; i += (half * 1.5))
			{
				pDC->FillSolidRect(i, r.top, half / 2, r.Height(), crBack);
			}
		}
		else if (get_column_data_type(iSubItem) == column_data_type_progress)
		{
			//pDC->FillSolidRect(itemRect, crBack);

			CRect r = itemRect;

			//�ٱ׷����� ���� ����, �� ���� ���ݿ� ��������� �ʰ� ��Ʈ�� ���̰��� ����ϴ� ���̷� �׷���� �Ѵ�.
			int cy = r.CenterPoint().y;
			r.top = cy + (double)m_lf.lfHeight / 1.4 + 1;		//�� ���� Ű��� �ٱ׷����� ���̰� ��������.
			r.bottom = cy - (double)m_lf.lfHeight / 1.4;	//m_lf.lfHeight�� �����̹Ƿ� -,+�� �ƴ϶� +,-�� ���� ����

			double d = _ttof(m_list_db[iItem].text[iSubItem]);
			d /= 100.0;
			Clamp(d, 0.0, 1.0);

			r.right = r.left + (double)(r.Width()) * d;
			pDC->FillSolidRect(r, m_crProgress);

			//20231102 CSCSliderCtrl������ �����ϰ� progress ��� ��ġ�� ���� ���ʰ� �������� ���� �ٸ� ������ ǥ���ϰ���
			//�Ʒ� �ڵ带 ��������� �ؽ�Ʈ�� ���� ��µ��� �ʴ´�.
			//pDC�� �ƴ� CMemoryDC�� �� ����ϴ� ���ϴ´�� ��µ����� �ٸ� row�� ���� �׷����� �ʴ´�.
			//Rgn�� �����Ͽ� OnPaint()�� DrawItem�� ���� ���̰� �ִ�.
			if (m_show_progress_text)
			{
				CString sPercent = m_list_db[iItem].text[iSubItem] + _T("%");
				pDC->SetTextColor(m_crProgressText);// m_crBack);
#if 1
				pDC->DrawText(sPercent, itemRect, DT_VCENTER | DT_CENTER | DT_SINGLELINE);

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

				pDC->SetTextColor(RGB(255, 0, 0));// m_crBack);
				pDC->SelectClipRgn(&rgnLeft);
				pDC->DrawText(sPercent, itemRect, DT_VCENTER | DT_CENTER | DT_SINGLELINE);
				//pDC->TextOut(itemRect.CenterPoint().x, itemRect.CenterPoint().y+4, sPercent);

				//rgnRight.SetRectRgn(rcRight);
				pDC->SetTextColor(RGB(0, 0, 255)); //m_crText);
				pDC->SelectClipRgn(&rgnRight);
				pDC->DrawText(sPercent, itemRect, DT_VCENTER | DT_CENTER | DT_SINGLELINE);
				//pDC->TextOut(itemRect.CenterPoint().x, itemRect.CenterPoint().y+4, sPercent);

				rgnLeft.DeleteObject();
				rgnRight.DeleteObject();
#endif
				pDC->SelectClipRgn(NULL);
				
			}			
		}
		//�ؽ�Ʈ �����̸� ���ڿ��� ������ش�.
		else
		{
			pDC->SetTextColor(crText);

			//�ؽ�Ʈ�� �׷��� �� itemRect�� �׸��� �¿� ������ ��� ������ �������δ�.
			//�ణ �ٿ��� ����ؾ� ���� ����.
			textRect = itemRect;

			if (iSubItem == 0 && m_is_shell_listctrl)
			{
				//16x16 �������� 22x21 ������ ǥ���Ѵ�. (21�� �⺻ height�̸� m_line_height�� ���� �޶�����.)
				textRect.left += 3;
				CString text = get_text(iItem, iSubItem);
				CString real_path = convert_special_folder_to_real_path(m_path + _T("\\") + get_text(iItem, iSubItem),
																		m_pShellImageList->get_csidl_map());
				int icon_index;

				if (m_is_shell_listctrl)
				{
					if (get_text(iItem, col_filesize).IsEmpty())
					{
						real_path = _T("c:\\Windows");
						icon_index = m_pShellImageList->GetSystemImageListIcon(real_path, true);
					}
					else
					{
						icon_index = m_pShellImageList->GetSystemImageListIcon(real_path, false);
					}

					m_pShellImageList->m_imagelist_small.Draw(pDC, icon_index,
						CPoint(textRect.left, textRect.CenterPoint().y - 8), ILD_TRANSPARENT);
					/*
					if (m_shell_list_local)
					{
						m_imagelist_small.Draw(pDC, icon_index, false),
							CPoint(textRect.left, textRect.CenterPoint().y - 8), ILD_TRANSPARENT);
					}
					else
					{
						m_imagelist_small.Draw(pDC, icon_index,
							CPoint(textRect.left, textRect.CenterPoint().y - 8), ILD_TRANSPARENT);
					}
					*/
				}

				textRect.left += 16;	//small icon width
				textRect.left += 3;		//margin between icon and text
			}
			else
			{
				textRect.DeflateRect(4, 0);
			}

			UINT format = 0;

			if (get_column_text_align(iSubItem) == LVCFMT_LEFT)
				format = DT_LEFT;
			else if (get_column_text_align(iSubItem) == LVCFMT_CENTER)
				format = DT_CENTER;
			else if (get_column_text_align(iSubItem) == LVCFMT_RIGHT)
				format = DT_RIGHT;

			format = format | DT_SINGLELINE | DT_VCENTER | DT_WORD_ELLIPSIS;
			pDC->DrawText(m_list_db[iItem].text[iSubItem], textRect, format);
		}
	}

	//���õ� �׸��� ���� ���󺸴� ���� ������ �׵θ��� �׷�����.
	if (m_is_shell_listctrl && !m_in_editing && (GetFocus() == this))
	{
		for (int i = 0; i < size(); i++)
		{
			if (GetItemState(i, LVIS_SELECTED))
			{
				GetSubItemRect(i, 0, LVIR_BOUNDS, rowRect);
				DrawRectangle(pDC, rowRect, m_crSelectedBorder);
			}
		}
	}

	//GetSubItemRect(iItem, 0, LVIR_BOUNDS, rowRect);
	//DrawLine(pDC, rowRect.left, rowRect.bottom-1, rowRect.right, rowRect.bottom-1, GRAY(232));
}

// ex. "No,20;Item1,50;Item2,50"
//scpark 20230801
//��� ������Ʈ���� �� �Լ��� 2�� ȣ��Ǿ� ���� �÷��� 3���ε�
//6���� ���õǾ� ������ �߻���.
//�Ź� ȣ��� �����ϰ� ó���ص� �ǰ� �ٸ� ó�� ����� ������
//��Ȯ�� �ѹ��� ȣ���Ͽ� ����ϴ� ���� �����̹Ƿ� �켱 ������ ó���� ���� ����.
bool CVtListCtrlEx::set_headings(const CString& strHeadings)
{
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

	m_HeaderCtrlEx.m_header_text_align.resize(column);
	m_HeaderCtrlEx.m_header_text_align.assign(column, HDF_LEFT);

	m_allow_edit_column.resize(column);

	modify_style();

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
		//������ ���� ã��
		GetItemRect(i, &itemRect, LVIR_BOUNDS);

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
		}
	}

	return false;
}

//debug��忡���� �ſ� ������. lambda�������� �𸣰ڴ�.
//0�� �÷��� �ƴ� �ٸ� �÷����� ������ �� �� ���� ������ 0�� �÷����� �ѹ� �� �˻��Ѵ�.
void CVtListCtrlEx::sort(int subItem, int ascending)
{
	if (!m_allow_sort)
		return;

	m_cur_sorted_column = subItem;

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
	int data_type = get_column_data_type(subItem);
	bool include_null = false;

	//shelllist�� ���� ������ ������ ������ ������ �� ������� �ϹǷ� �Ʒ� ���ٸ� ����� �� ����.
	if (m_is_shell_listctrl)
	{
		if (iSub == col_filedate)
		{
			std::sort(m_cur_folders.begin(), m_cur_folders.end(),
				[sort_asc, iSub, data_type, include_null](CVtFileInfo a, CVtFileInfo b)
				{
					return (a.text[iSub] > b.text[iSub]);
				});

			std::sort(m_cur_files.begin(), m_cur_files.end(),
				[sort_asc, iSub, data_type](CVtFileInfo a, CVtFileInfo b)
				{
					return (a.text[iSub] > b.text[iSub]);
				});
		}

		refresh_list(false);
		return;
	}

	
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

			return true;
		}
	);

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
			
			if (a.crText[iSub] > b.crText[iSub])
				return true;
			else if (a.crText[iSub] < b.crText[iSub])
				return false;
			//�ؽ�Ʈ ������ ������ �ؽ�Ʈ�� ���ϵ� �Ϲ� �׸��� ���ܽ�Ų��.
			else if (text_sort_on_same_color_item && (a.crText[iSub] != listctrlex_unused_color))
			{
				if (a.text[iSub] < b.text[iSub])
					return true;
				else// if (a.text[iSub] < b.text[iSub])
					return false;
			}
		}
		else
		{
			return (a.crText[iSub] < b.crText[iSub]);
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
	//�ڱ� �ڽſ��� �ο��� ��Ʈ�� ���ٸ� null�� ���ϵȴ�.
	//dlg�� parent�� font�� ���;� �Ѵ�.
	CFont* font = GetFont();

	if (font == NULL)
		font = AfxGetMainWnd()->GetFont();

	if (font != NULL)
		font->GetObject(sizeof(m_lf), &m_lf);
	else
		GetObject(GetStockObject(SYSTEM_FONT), sizeof(m_lf), &m_lf);

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


BOOL CVtListCtrlEx::PreTranslateMessage(MSG* pMsg)
{
	// TODO: Add your specialized code here and/or call the base class
	// TODO: ���⿡ Ư��ȭ�� �ڵ带 �߰� ��/�Ǵ� �⺻ Ŭ������ ȣ���մϴ�.
	if (pMsg->message == WM_MOUSEWHEEL && m_in_editing)
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
		case VK_BACK	:
		{
			//������ ��� BackŰ�� ���� ���� ������ �����Ű�� ���� ������ ����� �� ���� ������
			//main���� ��� ����ϴ��Ŀ� ���� ���ذ� �� ���� �ִ�.
			if (m_is_shell_listctrl && m_is_shell_listctrl_local)
			{
				if (m_path == get_system_label(CSIDL_DRIVES))
					return true;

				//����̺�� �� PC�� ����
				if (m_path.Mid(1) == _T(":"))
					m_path = get_system_label(CSIDL_DRIVES);
				//�׷��� ������ ���� ���丮�� �̵�
				else
					m_path = GetParentDirectory(m_path);

				set_path(m_path);
				//return true;
			}
				
			break;
		}

		case 220		:	return true;	//'\'Ű�� ������ ����Ʈ �� ó������ �̵��Ǵ� ���� ����.
		case 'A'		:	if (GetKeyState(VK_CONTROL) & 0x8000)
							{
								select_item(-1);
							}
							break;
		case 'C'		:	if (GetKeyState(VK_CONTROL) & 0x8000)
							{
								//CopyToClipboard(_T("|"));
								MessageBeep(MB_ICONINFORMATION);
								return true;
							}
							break;
		case 'V'		:	if (GetKeyState(VK_CONTROL) & 0x8000)
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
								edit_item(get_selected_index(), m_edit_subItem);
								return true;
							}
							break;
		case VK_F5		:	
							//editing�� ���� F5Ű �̺�Ʈ�� �߻����� �ʴ´�.
							//if (m_in_editing)
							//{
							//	edit_end();
							//	return true;
							//}
							if (m_is_shell_listctrl && m_is_shell_listctrl_local)
							{
								refresh_list();
								return true;
							}
							else
							{
								break;
							}
		case VK_END :
							m_auto_scroll = true;
							break;
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

	/* �� �ڵ��� �ǹ̴�?
	//����� ��ĥ�������ؼ� �ȱ��ڰŸ��� �ϰ��� �ѵ��ѵ� �����δ�. �ϴ� �н�.
	CRect headerRect;
	GetDlgItem(0)->GetWindowRect(&headerRect);
	ScreenToClient(&headerRect);
	dc.ExcludeClipRect(&headerRect);

	CRect clip;
	dc.GetClipBox(&clip);
	clip.top -= 5;
	*/

	dc.FillSolidRect(rc, m_crBack);

	if (!m_text_on_empty.IsEmpty())
	{
		draw_text(&g, rc, m_text_on_empty, 10, false, 0, 0, _T("���� ���"));
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
	LV_DISPINFO *plvDispInfo = (LV_DISPINFO *)pNMHDR;
	LV_ITEM	*plvItem = &plvDispInfo->item;

	if (plvItem->pszText != NULL && plvItem->pszText != m_old_text)
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
			set_text(m_edit_item, m_edit_subItem, plvItem->pszText);
		}
	}

	m_in_editing = false;

	*pResult = FALSE;

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
}


CEdit* CVtListCtrlEx::edit_item(int item, int subItem)
{
	// Make sure that nCol is valid
	CHeaderCtrl* pHeader = (CHeaderCtrl*)GetDlgItem(0);
	int nColumnCount = pHeader->GetItemCount();

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

	m_last_clicked = 0;


	CRect r = get_item_rect(item, subItem);
	CRect rc;

	GetClientRect(rc);

	// Get Column alignment
	LV_COLUMN	lvCol;

	lvCol.mask = LVCF_FMT;
	GetColumn (subItem, &lvCol);

	DWORD dwStyle;

	if ((lvCol.fmt & LVCFMT_JUSTIFYMASK) == LVCFMT_LEFT)
		dwStyle = ES_LEFT;
	else if ((lvCol.fmt & LVCFMT_JUSTIFYMASK) == LVCFMT_RIGHT)
		dwStyle = ES_RIGHT;
	else
		dwStyle = ES_CENTER;

	DWORD dwExStyle = ListView_GetExtendedListViewStyle(GetSafeHwnd());
	if (subItem == 0)
	{
		if (dwExStyle & LVS_EX_CHECKBOXES)
			r.left += 18;

		if (m_is_shell_listctrl)
		{
			r.left += 19;	//editbox��ü�� left-margin�� �����Ƿ� 22�� �ƴ� 19�� �����ش�.
			r.OffsetRect(0, -1);
		}
	}

	if (r.right > rc.right)
		r.right = rc.right;

	m_old_text = GetItemText(item, subItem);

	dwStyle |= WS_BORDER | WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL;
	CEdit *pEdit = new gxEditCell(this, item, subItem, GetItemText(item, subItem));
	pEdit->Create(dwStyle, r, this, IDC_EDIT_CELL);
	pEdit->SetFont(&m_font, true);
	m_pEdit = pEdit;

	CString ext = get_part(m_old_text, fn_ext);
	if ((ext.GetLength() == 3 || ext.GetLength() == 4) && IsAlphaNumeric(ext))
	{
		m_pEdit->SetSel(0, m_old_text.GetLength() - ext.GetLength() - 1);
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

	return pEdit;
}

//���� ���� �ؽ�Ʈ�� �ǵ�����.(���� ���̺��� ���ϸ��̰� ���ϸ� ������ ������ ��� �� �� �ִ�.)
void CVtListCtrlEx::undo_edit_label()
{
	set_text(m_edit_item, m_edit_subItem, m_old_text);
}

void CVtListCtrlEx::set_color_theme(int theme, bool apply_now)
{
	switch (theme)
	{
		//�ֱ� ������ Ž������ ������ ���� �ؽ�Ʈ ������ ���ÿ���, inactive�� �����ϰ� �����ϴ�.
	case color_theme_default :
		m_crText				= ::GetSysColor(COLOR_BTNTEXT);
		m_crTextSelected		= m_crText;// ::GetSysColor(COLOR_HIGHLIGHTTEXT);
		m_crTextSelectedInactive= m_crText;// ::GetSysColor(COLOR_INACTIVECAPTIONTEXT);
		m_crBack				= ::GetSysColor(COLOR_WINDOW);
		m_crBackAlt				= m_crBack;//get_color(m_crBack, -8);
		m_crBackSelected		= RGB(204, 232, 255);// ::GetSysColor(COLOR_HIGHLIGHT);
		m_crBackSelectedInactive = RGB(217, 217, 217);// ::GetSysColor(COLOR_HIGHLIGHT);
		m_crSelectedBorder		= RGB(153, 209, 255);
		m_crHeaderBack			= ::GetSysColor(COLOR_3DFACE);
		m_crHeaderText			= ::GetSysColor(COLOR_BTNTEXT);
		m_crPercentage.clear();
		m_crPercentage.push_back(GRAY192);
		m_crProgress			= RGB(49, 108, 244);
		m_crProgressText		= RGB(192, 192, 192);
		break;
	case color_theme_light_blue :
		m_crText				= ::GetSysColor(COLOR_BTNTEXT);
		m_crTextSelected		= RGB( 65, 102, 146);
		m_crTextSelectedInactive= RGB( 65, 102, 146);
		m_crBack				= RGB(193, 219, 252);
		m_crBackAlt				= m_crBack;//get_color(m_crBack, -8);
		m_crBackSelected		= get_color(m_crBack, -48);
		m_crBackSelectedInactive= get_color(m_crBack, -48);
		m_crSelectedBorder		= RGB(153, 209, 255);
		m_crHeaderBack			= get_color(m_crBack, -32);
		m_crHeaderText			= get_color(m_crText, -32);
		m_crPercentage.clear();
		m_crPercentage.push_back(get_color(m_crBack, -32));
		m_crProgress			= RGB(32, 32, 255);
		m_crProgressText		= RGB(192, 192, 192);
		break;
	case color_theme_navy_blue :
		m_crText				= RGB(204, 216, 225);
		m_crTextSelected		= RGB(234, 246, 255);
		m_crTextSelectedInactive= RGB(105, 142, 186);
		m_crBack				= RGB( 74,  94, 127);
		m_crBackAlt				= m_crBack;//get_color(m_crBack, -8);
		m_crBackSelected		= RGB( 15,  36,  41);
		m_crBackSelectedInactive= RGB( 15,  36,  41);
		m_crSelectedBorder		= RGB(153, 209, 255);
		m_crHeaderBack			= get_color(m_crBack, -32);
		m_crHeaderText			= get_color(m_crText, -32);
		m_crPercentage.clear();
		m_crPercentage.push_back(get_color(m_crBack, -32));
		m_crProgress			= RGB(32, 32, 255);
		m_crProgressText		= RGB(192, 192, 192);
		break; 
	case color_theme_dark_blue :
		m_crText				= RGB( 16, 177, 224);
		m_crTextSelected		= RGB(224, 180,  59);
		m_crTextSelectedInactive= RGB(105, 142, 186);
		m_crBack				= RGB(  2,  21,  36);
		m_crBackAlt				= m_crBack;//get_color(m_crBack, -8);
		m_crBackSelected		= RGB(  3,  42,  59);
		m_crBackSelectedInactive= RGB( 15,  36,  41);
		m_crSelectedBorder		= RGB(153, 209, 255);
		m_crHeaderBack			= RGB(  0,  13,  22);
		m_crHeaderText			= RGB(  0, 180, 228);
		m_crPercentage.clear();
		m_crPercentage.push_back(get_color(m_crBack, 32));
		m_crProgress			= RGB(32, 32, 255);
		m_crProgressText		= RGB(192, 192, 192);
		break; 
	case color_theme_dark_gray :
		m_crText				= RGB(164, 164, 164);
		m_crTextSelected		= RGB(241, 241, 241);
		m_crTextSelectedInactive= get_color(m_crTextSelected, -36);
		m_crBack				= RGB( 64,  64,  64);
		m_crBackAlt				= m_crBack;//get_color(m_crBack, -8);
		m_crBackSelected		= get_color(m_crBack, -32);
		m_crBackSelectedInactive= get_color(m_crBack, -32);
		m_crSelectedBorder		= RGB(128, 128, 128);
		m_crHeaderBack			= get_color(m_crBack, -16);
		m_crHeaderText			= get_color(m_crText, -16);
		m_crPercentage.clear();
		m_crPercentage.push_back(get_color(m_crBack, 32));
		m_crProgress			= RGB(32, 32, 255);
		m_crProgressText		= RGB(192, 192, 192);
		break;
	case color_theme_dark :
		m_crText = RGB(212, 212, 212);
		m_crTextSelected = RGB(255, 255, 255);
		m_crTextSelectedInactive = get_color(m_crTextSelected, -36);
		m_crBack = RGB(37, 37, 38);
		m_crBackAlt = m_crBack;//get_color(m_crBack, -8);
		m_crBackSelected = get_color(m_crBack, -16);
		m_crBackSelectedInactive = get_color(m_crBack, -16);
		m_crSelectedBorder = m_crBackSelected;
		m_crHeaderBack = get_color(m_crBack, 16);
		m_crHeaderText = get_color(m_crText, -32);
		m_crPercentage.clear();
		m_crPercentage.push_back(get_color(m_crBack, 32));
		m_crProgress = RGB(32, 32, 255);
		m_crProgressText = RGB(192, 192, 192);
		break;
	}

	m_HeaderCtrlEx.set_color(m_crHeaderText, m_crHeaderBack);

	if (apply_now)
		Invalidate();
}

COLORREF CVtListCtrlEx::get_text_color(int item, int subItem)
{
	//item�� -1�̸� �ش� ���� ������ �ƴ϶� �⺻ �ؽ�Ʈ ������ ���ϴ� ���̴�.
	if (item < 0)
		return m_crText;

	COLORREF cr = m_list_db[item].crText[subItem];
	if (cr == listctrlex_unused_color)
		return m_crText;
	return cr;
}

COLORREF CVtListCtrlEx::get_back_color(int item, int subItem)
{
	COLORREF cr = m_list_db[item].crBack[subItem];
	if (cr == listctrlex_unused_color)
		return m_crBack;
	return cr;
}

void CVtListCtrlEx::set_text_color(int item, int subItem, COLORREF crText, bool erase, bool invalidate)
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

void CVtListCtrlEx::set_back_color(int item, int subItem, COLORREF crBack, bool erase)
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

void CVtListCtrlEx::set_item_color(int item, int subItem, COLORREF crText, COLORREF crBack)
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

void CVtListCtrlEx::set_progress_color(COLORREF crProgress)
{
	m_crProgress = crProgress;
	Invalidate();
}

int CVtListCtrlEx::add_item(CString text, bool ensureVisible, bool invalidate)
{
	return insert_item(-1, text, ensureVisible, invalidate);
}

int CVtListCtrlEx::add_line_string_item(CString line_string, TCHAR separator, bool ensureVisible, bool invalidate)
{
	std::deque<CString> dq;
	get_token_string(line_string, dq, separator);
	return insert_item(-1, dq, ensureVisible, invalidate);
}

//index ��ġ�� 0�� �÷��� text�� ������ �߰��Ѵ�.(-1�̸� �� �������� �߰�)
int CVtListCtrlEx::insert_item(int index, CString text, bool ensureVisible, bool invalidate)
{
	if (get_column_count() <= 0)
	{
		TRACE(_T("column count is 0. use set_headings(...) first.\n"));
		return -1;
	}

	if (index < 0)
		index = size();

	//�̹� �ش� index�� row�� �����ϴ� ���� ���� �����Ѵ�.
	if (index < m_list_db.size())
	{
		set_text(index, 0, text, false);
	}
	else
	{
		m_list_db.insert(m_list_db.begin() + index, CListCtrlData(text, m_HeaderCtrlEx.GetItemCount()));
		SetItemCountEx(m_list_db.size());
	}

	//ensureVisible�̸� Invalidate()�� �����ص� �ȴ�.
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


int CVtListCtrlEx::insert_item(int index, std::deque<CString> dqText, bool ensureVisible, bool invalidate)
{
	if (dqText.size() == 0)
		return -1;

	int count = MIN(get_column_count(), dqText.size());
	
	index = insert_item(index, dqText[0], ensureVisible, invalidate);

	for (int i = 1; i < count; i++)
		set_text(index, i, dqText[i]);

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

		if (m_is_shell_listctrl && m_is_shell_listctrl_local)
		{
			CString file;
			file.Format(_T("%s\\%s"), m_path, get_text(index, col_filename));
			if (!delete_file(file, true))
			{
				get_last_error_string(true);
				break;
			}
		}

		CListCtrl::DeleteItem(index);
		m_list_db.erase(m_list_db.begin() + index);
	}

	SetItemCount(m_list_db.size());
}

void CVtListCtrlEx::delete_item(int index)
{
	CListCtrl::DeleteItem(index);
	m_list_db.erase(m_list_db.begin() + index);
	SetItemCount(m_list_db.size());
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
	CListCtrl::DeleteAllItems();
	m_list_db.clear();
	SetItemCount(0);

	if (delete_file_list)
	{
		m_cur_files.clear();
		m_cur_folders.clear();
	}
}

CString CVtListCtrlEx::get_text(int item, int subItem)
{
	return m_list_db[item].text[subItem];
}

void CVtListCtrlEx::set_text(int item, int subItem, CString text, bool invalidate)
{
	if (item < 0)
		return;

	if (!m_hWnd)
		return;

	m_list_db[item].text[subItem] = text;
	if (invalidate)
		InvalidateRect(get_item_rect(item, subItem), false);
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
int CVtListCtrlEx::get_selected_items(std::deque<int> *dqSelected)
{
	if (dqSelected != NULL)
		dqSelected->clear();

	int index;
	int selected_count = 0;
	POSITION pos = GetFirstSelectedItemPosition();

	while (pos)
	{
		index = GetNextSelectedItem(pos);
		selected_count++;
		if (dqSelected != NULL)
			dqSelected->push_back(index);
	}

	return selected_count;
}


void CVtListCtrlEx::select_item(int nIndex, bool bSelect /*= true*/, bool after_unselect)
{
	if (after_unselect)
		unselect_selected_item();

	SetItemState(nIndex, bSelect ? LVIS_SELECTED : 0, LVIS_SELECTED);
	SetItemState(nIndex, bSelect ? LVIS_FOCUSED : 0, LVIS_FOCUSED);
}

void CVtListCtrlEx::unselect_selected_item()
{
	std::deque<int> dqSelected;
	get_selected_items(&dqSelected);
	for (int i = 0; i < dqSelected.size(); i++)
		select_item(dqSelected[i], false);
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
void CVtListCtrlEx::find_string(CString find_target, std::deque<int>* result,
								int start_idx, int end_idx, int column_start, int column_end,
								bool result_reset, bool bWholeWord, bool bCaseSensitive, bool select)
{
	if (column_start < 0)
		column_start = 0;
	if (column_end < 0)
		column_end = get_column_count() - 1;

	if (column_start > column_end)
		return;

	if (result_reset && result->size())
		result->clear();

	int i;
	std::deque<int> dqColumn;
	for (i = column_start; i <= column_end; i++)
		dqColumn.push_back(i);

	find_string(find_target, result, start_idx, end_idx, &dqColumn);
}

int CVtListCtrlEx::find_string(CString find_target, std::deque<int>* result, int start_idx, int end_idx, std::deque<int>* dqColumn, bool stop_first_found)
{
	int		i;
	TCHAR	op;
	CString sText;
	std::deque<CString> dqTarget;	//separator�� ""�� �ƴ� ���� ��ū���� �и��Ͽ� ��� ã�´�.

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
	return FALSE;

	LPNMITEMACTIVATE pNMItemActivate = reinterpret_cast<LPNMITEMACTIVATE>(pNMHDR);
	// TODO: Add your control notification handler code here
	int item;// = pNMItemActivate->iItem;
	int subItem;// = pNMItemActivate->iSubItem;

	if (!get_index_from_point(pNMItemActivate->ptAction, item, subItem, true) ||
		item < 0 || subItem < 0)
		return TRUE;

	TRACE(_T("%d, %d\n"), item, subItem);

	if (m_is_shell_listctrl && m_is_shell_listctrl_local)
	{
		if (item < 0 || item >= size() || subItem < 0 || subItem >= get_column_count())
			return TRUE;

		if (m_path == get_system_label(CSIDL_DRIVES))
		{
			m_path = convert_special_folder_to_real_path(get_text(item, col_filename), m_pShellImageList->get_csidl_map());
		}
		else
		{
			if (!PathIsDirectory(m_path + _T("\\") + get_text(item, col_filename)))
				return TRUE;

			m_path = m_path + _T("\\") + get_text(item, col_filename);
		}

		set_path(m_path);
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

	if (onlySelected && get_selected_items() == 0)
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
		add_line_string_item(CString(sLine), separator);
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
	SetFont(&m_font, true);

	if (m_HeaderCtrlEx)
		m_HeaderCtrlEx.SetFont(&m_font, true);

	m_font_size = get_font_size();
	
	//set_line_height(4-m_lf.lfHeight);
	//if (m_auto_line_height)
		//recalculate_line_height();k
	
	ASSERT(bCreated);
}

int CVtListCtrlEx::get_font_size()
{
	m_font_size = -MulDiv(m_lf.lfHeight, 72, GetDeviceCaps(::GetDC(GetParent()->GetSafeHwnd()), LOGPIXELSY));
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
	m_lf.lfHeight = -MulDiv(m_font_size, GetDeviceCaps(::GetDC(GetParent()->GetSafeHwnd()), LOGPIXELSY), 72);
	reconstruct_font();
}

void CVtListCtrlEx::enlarge_font_size(bool enlarge)
{
	m_font_size = get_font_size_from_logical_size(m_hWnd, m_lf.lfHeight);
	enlarge ? m_font_size++ : m_font_size--;

	if (m_font_size < 4)
		m_font_size = 4;
	if (m_font_size > 40)
		m_font_size = 40;

	m_lf.lfHeight = -MulDiv(m_font_size, GetDeviceCaps(::GetDC(GetParent()->GetSafeHwnd()), LOGPIXELSY), 72);
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

void CVtListCtrlEx::set_font_bold(bool bBold)
{
	m_lf.lfWeight = (bBold ? FW_BOLD : FW_NORMAL);
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
BOOL CVtListCtrlEx::OnNMClickList(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMITEMACTIVATE pNMItemActivate = reinterpret_cast<LPNMITEMACTIVATE>(pNMHDR);
	int item = -1;// = pNMItemActivate->iItem;
	int subItem = -1;// = pNMItemActivate->iSubItem;	<== invalid index returned when user clicked out of columns

	if (!get_index_from_point(pNMItemActivate->ptAction, item, subItem, false) ||
		item < 0 || subItem < 0)
		return TRUE;

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
			(get_selected_items() == 1) &&
			(clock() - m_last_clicked > 800) &&	//�� ���� ������ ����Ŭ������ �����ǰ�
			(clock() - m_last_clicked < 1600))
		{
			edit_item(m_edit_item, m_edit_subItem);
		}
		else
		{
			m_last_clicked = clock();
		}

		m_edit_item = item;
		m_edit_subItem = subItem;
	}

	return FALSE;
	*pResult = 0;
}

void CVtListCtrlEx::edit_end(bool valid)
{
	m_in_editing = false;
	m_last_clicked = 0;
	m_pEdit->ShowWindow(SW_HIDE);
	Invalidate();

	/*
	CString sText;

	if (valid)
	{
		m_pEdit->GetWindowText(sText);

		if (sText == m_old_title)
			return;

		TRACE(_T("editted text = %s\n"), sText);
		sText.Trim();

		if (sText.IsEmpty())
			return;

		m_dqThumb[m_editing_index].title = sText;
		Invalidate();

		::SendMessage(GetParent()->GetSafeHwnd(), Message_CThumbCtrl,
			(WPARAM)&CThumbCtrlMsg(GetDlgCtrlID(), CThumbCtrlMsg::message_thumb_rename, m_editing_index), 0);
	}
	*/
}

//� �׸��� Ư�� ��ġ�� ǥ�õǵ��� ��ũ�ѽ�Ų��.
//mode�� visible_first�̰� offset�� 3�̸� ������ 3�� �ε���, �� 4��° ��ġ�� �ش� �������� ǥ�õǵ��� ��ũ�ѽ�Ų��.
//mode�� visible_center�̰� offset�� 0�̸� �߾� ��ġ�� �ش� �������� ǥ�õǵ��� ��ũ�ѽ�Ų��.
//mode�� visible_last�̰� offset�� 3�̸� �Ʒ����� -3-1�� ��ġ�� �ش� �������� ǥ�õǵ��� ��ũ�ѽ�Ų��.
void CVtListCtrlEx::ensure_visible(int index, int mode, int offset)
{
	EnsureVisible(index, FALSE);

	/*
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
	*/
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

void CVtListCtrlEx::random()
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

void CVtListCtrlEx::set_as_shell_listctrl(bool is_local)
{
	m_is_shell_listctrl = true;
	m_is_shell_listctrl_local = is_local;
	m_use_own_imagelist = true;

	set_headings(_T("�̸�,200;ũ��,100;������ ��¥,150"));
	set_font_size(9);
	set_font_name(_T("���� ���"));
	//set_font_size(), set_font_name()�� ȣ������ �ʰ� set_header_height()�� ȣ���ϸ�
	//CHeaderCtrlEx::OnLayout()���� ������ �߻��Ѵ�.
	set_header_height(24);
	set_line_height(21);

	set_column_data_type(col_filesize, column_data_type_numeric);

	set_column_text_align(col_filesize, HDF_RIGHT, false);

	allow_edit();
	allow_edit_column(col_filesize, false);
	allow_edit_column(col_filedate, false);
}

void CVtListCtrlEx::set_path(CString path, bool refresh)
{
	m_last_clicked = 0;

	path = convert_special_folder_to_real_path(path, m_pShellImageList->get_csidl_map());

	m_path = path;

	if (m_path.Right(1) == '\\')
		m_path = m_path.Left(m_path.GetLength() - 1);

	TRACE(_T("current path = %s\n"), m_path);

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
	if (m_is_shell_listctrl_local && reload)
	{
		if (m_path == get_system_label(CSIDL_DRIVES))
		{
			std::map<TCHAR, CString> drive_map;
			get_drive_map(&drive_map);
			for (std::map<TCHAR, CString>::iterator it = drive_map.begin(); it != drive_map.end(); it++)
				m_cur_folders.push_back(CVtFileInfo(it->second));
		}
		else
		{
			CFileFind	finder;

			bool bWorking = finder.FindFile(m_path + _T("\\*"));

			while (bWorking)
			{
				bWorking = finder.FindNextFile();
				sfile = finder.GetFilePath();

				if (finder.IsDots() || finder.IsHidden() || finder.IsSystem())
					continue;

				if (finder.IsDirectory())
				{
					m_cur_folders.push_back(CVtFileInfo(sfile));
					continue;
				}

				m_cur_files.push_back(CVtFileInfo(sfile));
			}
		}
	}

	int index;
	int insert_index = -1;

	if (m_column_sort_type[m_cur_sorted_column] == sort_descending)
		insert_index = 0;

	if (m_path == get_system_label(CSIDL_DRIVES))
	{
		set_header_text(col_filesize, _T("��� ������ ����"));
		set_header_text(col_filedate, _T("��ü ũ��"));
		set_column_text_align(col_filedate, LVCFMT_RIGHT);
	}
	else
	{
		set_header_text(col_filesize, _T("ũ��"));
		set_header_text(col_filedate, _T("������ ��¥"));
		set_column_text_align(col_filedate, LVCFMT_LEFT);
	}

	//asce�� ��������, desc�� ���ϸ��� ǥ�õȴ�.
	for (i = 0; i < m_cur_folders.size(); i++)
	{
		index = insert_item(insert_index, get_part(m_cur_folders[i].text[0], fn_name), false, false);

		if (m_path == get_system_label(CSIDL_DRIVES))
		{
			CString drive = convert_special_folder_to_real_path(m_cur_folders[i].text[0], m_pShellImageList->get_csidl_map());
			
			uint64_t disk_size = get_disk_free_size(drive);
			if (disk_size > 0)
				set_text(index, col_filesize, get_size_string(disk_size, 3, 1));

			disk_size = get_disk_total_size(drive);
			if (disk_size > 0)
				set_text(index, col_filedate, get_size_string(get_disk_total_size(drive), 3, 1));

			set_text_color(index, col_filesize, RGB(109, 109, 109));
			set_text_color(index, col_filedate, RGB(109, 109, 109));
		}
		else
		{
			set_text(index, col_filedate, m_cur_folders[i].text[2]);
			set_text_color(index, col_filedate, RGB(109, 109, 109));
		}
	}

	for (i = 0; i < m_cur_files.size(); i++)
	{
		index = insert_item(insert_index, get_part(m_cur_files[i].text[0], fn_name), false, false);

		set_text(index, col_filesize, m_cur_files[i].text[1]);
		set_text(index, col_filedate, m_cur_files[i].text[2]);
		set_text_color(index, col_filesize, RGB(109, 109, 109));
		set_text_color(index, col_filedate, RGB(109, 109, 109));
	}
}

//���� �Ǵ� ������ �ش��ϴ� ��� ����Ʈ�� �߰��Ѵ�.
//local�� ��� ũ��� ��¥�� ����ִٸ� �ڵ� ä���ְ� remote��� ��������� �ȵȴ�.
void CVtListCtrlEx::add_file(CString filename, CString filesize, CString filedate, bool is_remote, bool is_folder)
{
	bool file_is_folder = is_folder;

	if (!is_remote)
		file_is_folder = PathIsDirectory(filename);

	if (file_is_folder)
	{
		m_cur_folders.push_back(CVtFileInfo(filename, filesize, filedate));
	}
	else
	{
		m_cur_files.push_back(CVtFileInfo(filename, filesize, filedate));
	}
}

void CVtListCtrlEx::add_file(WIN32_FIND_DATA* pFindFileData)
{
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
	add_file(pFindFileData->cFileName, file_is_folder ? _T("-") : get_size_string(ulInt.QuadPart), filedate, true, file_is_folder);
}

CImageList* CVtListCtrlEx::CreateDragImageEx(LPPOINT lpPoint)
{
	CRect	cSingleRect;
	CRect	cCompleteRect(0, 0, 0, 0);
	int	nIdx;
	BOOL	bFirst = TRUE;
	//
	// Determine the size of the drag image
	//
	POSITION pos = GetFirstSelectedItemPosition();
	while (pos)
	{
		nIdx = GetNextSelectedItem(pos);
		GetItemRect(nIdx, cSingleRect, LVIR_BOUNDS);
		if (bFirst)
		{
			// Initialize the CompleteRect
			GetItemRect(nIdx, cCompleteRect, LVIR_BOUNDS);
			bFirst = FALSE;
		}
		cCompleteRect.UnionRect(cCompleteRect, cSingleRect);
	}

	//
	// Create bitmap in memory DC
	//
	CClientDC	cDc(this);
	CDC		cMemDC;
	CBitmap   	cBitmap;

	if (!cMemDC.CreateCompatibleDC(&cDc))
		return NULL;

	if (!cBitmap.CreateCompatibleBitmap(&cDc, cCompleteRect.Width(), cCompleteRect.Height()))
		return NULL;

	CBitmap* pOldMemDCBitmap = cMemDC.SelectObject(&cBitmap);
	// Here green is used as mask color
	cMemDC.FillSolidRect(0, 0, cCompleteRect.Width(), cCompleteRect.Height(), RGB(0, 255, 0));

	//
	// Paint each DragImage in the DC
	//
	CImageList* pSingleImageList;
	CPoint		cPt;

	pos = GetFirstSelectedItemPosition();
	while (pos)
	{
		nIdx = GetNextSelectedItem(pos);
		GetItemRect(nIdx, cSingleRect, LVIR_BOUNDS);

		pSingleImageList = CreateDragImage(nIdx, &cPt);
		if (pSingleImageList)
		{
			pSingleImageList->DrawIndirect(&cMemDC,
				0,
				CPoint(cSingleRect.left - cCompleteRect.left,
					cSingleRect.top - cCompleteRect.top),
				cSingleRect.Size(),
				CPoint(0, 0));
			delete pSingleImageList;
		}
	}

	cMemDC.SelectObject(pOldMemDCBitmap);
	//
	// Create the imagelist	with the merged drag images
	//
	CImageList* pCompleteImageList = new CImageList;

	pCompleteImageList->Create(cCompleteRect.Width(),
		cCompleteRect.Height(),
		ILC_COLOR | ILC_MASK, 0, 1);
	// Here green is used as mask color
	pCompleteImageList->Add(&cBitmap, RGB(0, 255, 0));

	cBitmap.DeleteObject();
	//
	// as an optional service:
	// Find the offset of the current mouse cursor to the imagelist
	// this we can use in BeginDrag()
	//
	if (lpPoint)
	{
		CPoint cCursorPos;
		GetCursorPos(&cCursorPos);
		ScreenToClient(&cCursorPos);
		lpPoint->x = cCursorPos.x - cCompleteRect.left;
		lpPoint->y = cCursorPos.y - cCompleteRect.top;
	}

	return(pCompleteImageList);
}

CImageList* CVtListCtrlEx::CreateDragImageEx(CListCtrl *pList, LPPOINT lpPoint)
{
	if (pList->GetSelectedCount() <= 0) return NULL; // no row selected

	CFont* pFontDrag = NULL;

	pFontDrag = new CFont();
	pFontDrag->CreateFont(19, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET
		, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, FF_DONTCARE, _T("���� ���")
	);

	SendMessageToDescendants(WM_SETFONT, (WPARAM)pFontDrag->GetSafeHandle(), 1, TRUE, FALSE);


	CRect rectSingle;
	CRect rectComplete(0, 0, 0, 0);

	// Determine List Control Client width size
	pList->GetClientRect(rectSingle);
	int nWidth = rectSingle.Width();

	// Start and Stop index in view area
	int nIndex = pList->GetTopIndex() - 1;
	int nBottomIndex = pList->GetTopIndex() + pList->GetCountPerPage() - 1;
	if (nBottomIndex > (pList->GetItemCount() - 1))
		nBottomIndex = pList->GetItemCount() - 1;

	// Determine the size of the drag image (limite for rows visibled and Client width)
	while ((nIndex = pList->GetNextItem(nIndex, LVNI_SELECTED)) != -1)
	{
		if (nIndex > nBottomIndex)
			break;

		pList->GetItemRect(nIndex, rectSingle, LVIR_BOUNDS);

		if (rectSingle.left < 0)
			rectSingle.left = 0;

		if (rectSingle.right > nWidth)
			rectSingle.right = nWidth;

		rectComplete.UnionRect(rectComplete, rectSingle);
	}

	CClientDC dcClient(this);
	CDC dcMem;
	CBitmap Bitmap;

	if (!dcMem.CreateCompatibleDC(&dcClient))
		return NULL;

	if (!Bitmap.CreateCompatibleBitmap(&dcClient, rectComplete.Width(), rectComplete.Height()))
		return NULL;

	CBitmap *pOldMemDCBitmap = dcMem.SelectObject(&Bitmap);
	// Use green as mask color
	dcMem.FillSolidRect(0, 0, rectComplete.Width(), rectComplete.Height(), RGB(0, 255, 0));

	// Paint each DragImage in the DC
	nIndex = pList->GetTopIndex() - 1;
	while ((nIndex = pList->GetNextItem(nIndex, LVNI_SELECTED)) != -1)
	{
		if (nIndex > nBottomIndex)
			break;

		CPoint pt;
		CImageList* pSingleImageList = pList->CreateDragImage(nIndex, &pt);

		if (pSingleImageList)
		{
			pList->GetItemRect(nIndex, rectSingle, LVIR_BOUNDS);
			pSingleImageList->Draw(&dcMem,
				0,
				CPoint(rectSingle.left - rectComplete.left,
					rectSingle.top - rectComplete.top),
				ILD_MASK);
			pSingleImageList->DeleteImageList();
			delete pSingleImageList;
		}
	}


	dcMem.SelectObject(pOldMemDCBitmap);
	CImageList* pCompleteImageList = new CImageList;
	pCompleteImageList->Create(rectComplete.Width(), rectComplete.Height(), ILC_COLOR | ILC_MASK, 0, 1);
	pCompleteImageList->Add(&Bitmap, RGB(0, 255, 0)); // Green is used as mask color
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
	m_nDragIndex = pNMLV->iItem;

	CPoint pt;
	int nOffset = -10; //offset in pixels for drag image (positive is up and to the left; neg is down and to the right)

	//CFont* pFontDefault = NULL;
	//pFontDefault = new CFont();
	//pFontDefault = this->GetFont(); //

	//m_pDragImage = CreateDragImageEx(this, &pt);	<= �� �Լ� �ȵȴ�.

	int sel_count = get_selected_items();
	CGdiplusBitmap bmpRes(64, 64, PixelFormat32bppARGB, Gdiplus::Color(128, 255, 0, 0));
	
	if (m_drag_images_id.size() == 1)
		bmpRes.load(m_drag_images_id[0]);
	else if (m_drag_images_id.size() > 1)
		bmpRes.load(sel_count == 1 ? m_drag_images_id[0] : m_drag_images_id[1]);

	bmpRes.draw_text(bmpRes.width / 2 + 10, bmpRes.height / 2, i2S(sel_count), 20, 2,
					_T("���� ���"), Gdiplus::Color(192, 0, 0, 0), Gdiplus::Color(192, 255, 128, 128), DT_CENTER | DT_VCENTER);

	m_pDragImage = new CImageList();
	m_pDragImage->Create(bmpRes.width, bmpRes.height, ILC_COLOR32, 1, 1);

	HICON hicon;
	bmpRes.m_pBitmap->GetHICON(&hicon);
	m_pDragImage->Add(hicon);


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

		//// If we drag outside current window we need to adjust the highlights displayed
		if (pDropWnd != m_pDropWnd)
		{
			TRACE(_T("pDropWnd != m_pDropWnd\n"));

			if (pDropWnd->IsKindOf(RUNTIME_CLASS(CListCtrl)) && m_pDropWnd->IsKindOf(RUNTIME_CLASS(CListCtrl)))
			{
				if (m_nDropIndex != -1) //If we drag over the CListCtrl header, turn off the hover highlight
				{
					TRACE(_T("m_nDropIndex != -1\n"));
					CListCtrl* pList = (CListCtrl*)m_pDropWnd;
					VERIFY(pList->SetItemState(m_nDropIndex, 0, LVIS_DROPHILITED));
					// redraw item
					VERIFY(pList->RedrawItems(m_nDropIndex, m_nDropIndex));
					pList->UpdateWindow();
					m_nDropIndex = -1;
				}
				else //If we drag out of the CListCtrl altogether
				{
					CListCtrl* pList = (CListCtrl*)m_pDropWnd;
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
				/*
				CTreeCtrl* pTree = (CTreeCtrl*)m_pDropWnd;
				UINT uFlags;

				HTREEITEM hItem = pTree->HitTest(pt, &uFlags);

				if ((hItem != NULL) && (TVHT_ONITEM & uFlags))
				{
					pTree->SelectDropTarget(hItem);
					ASSERT(pTree->GetDropHilightItem() == hItem);
				}
				*/
			}
		}

		// Save current window pointer as the CListCtrl we are dropping onto
		//if (pDropWnd->IsKindOf(RUNTIME_CLASS(CListCtrl)))
		m_pDropWnd = pDropWnd;

		// Convert from screen coordinates to drop target client coordinates
		pDropWnd->ScreenToClient(&pt);

		//If we are hovering over a CListCtrl we need to adjust the highlights
		if (pDropWnd->IsKindOf(RUNTIME_CLASS(CListCtrl)))
		{
			//Note that we can drop here
			::SetCursor(AfxGetApp()->LoadStandardCursor(MAKEINTRESOURCE(IDC_ARROW)));
			UINT uFlags;
			CListCtrl* pList = (CListCtrl*)pDropWnd;

			// Turn off hilight for previous drop target
			pList->SetItemState(m_nDropIndex, 0, LVIS_DROPHILITED);
			// Redraw previous item
			pList->RedrawItems(m_nDropIndex, m_nDropIndex);

			// Get the item that is below cursor
			m_nDropIndex = ((CListCtrl*)pDropWnd)->HitTest(pt, &uFlags);
			// Highlight it (������ ��쿡�� hilite��Ų��)
			if (m_nDropIndex >= 0 && ((CListCtrl*)pDropWnd)->GetItemText(m_nDropIndex, col_filesize) == _T(""))
				pList->SetItemState(m_nDropIndex, LVIS_DROPHILITED, LVIS_DROPHILITED);
			// Redraw item
			pList->RedrawItems(m_nDropIndex, m_nDropIndex);
			pList->UpdateWindow();
		}
		else if (pDropWnd->IsKindOf(RUNTIME_CLASS(CTreeCtrl)))
		{
			::SetCursor(AfxGetApp()->LoadStandardCursor(MAKEINTRESOURCE(IDC_ARROW)));
			UINT uFlags;
			CTreeCtrl* pTree = (CTreeCtrl*)pDropWnd;

			// Get the item that is below cursor
			HTREEITEM hItem = ((CTreeCtrl*)pDropWnd)->HitTest(pt, &uFlags);
			TRACE(_T("%d, %d, hItem = %p\n"), pt.x, pt.y, hItem);
			pTree->SelectDropTarget(hItem);
			ASSERT(hItem == pTree->GetDropHilightItem());
		}
		else
		{
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
		delete m_pDragImage; //must delete it because it was created at the beginning of the drag

		CPoint pt(point); //Get current mouse coordinates
		ClientToScreen(&pt); //Convert to screen coordinates
		// Get the CWnd pointer of the window that is under the mouse cursor
		CWnd* pDropWnd = WindowFromPoint(pt);
		ASSERT(pDropWnd); //make sure we have a window pointer
		// If window is CListCtrl, we perform the drop
		if (pDropWnd->IsKindOf(RUNTIME_CLASS(CListCtrl)) ||
			pDropWnd->IsKindOf(RUNTIME_CLASS(CTreeCtrl)))
		{
			m_pDropWnd = pDropWnd; //Set pointer to the list we are dropping on
			DroppedHandler(m_pDragWnd, m_pDropWnd); //Call routine to perform the actual drop
		}
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

	if (pDropWnd->IsKindOf(RUNTIME_CLASS(CListCtrl)))
	{
		CListCtrl* pDropListCtrl = (CListCtrl*)pDropWnd;

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


BOOL CVtListCtrlEx::OnLvnItemchanged(NMHDR* pNMHDR, LRESULT* pResult)
{
	LPNMLISTVIEW pNMLV = reinterpret_cast<LPNMLISTVIEW>(pNMHDR);
	// TODO: ���⿡ ��Ʈ�� �˸� ó���� �ڵ带 �߰��մϴ�.
	NM_LISTVIEW* pNMListView = (NM_LISTVIEW*)pNMHDR;

	if ((pNMListView->uChanged & LVIF_STATE)
		&& (pNMListView->uNewState & LVIS_SELECTED))
	{
		m_auto_scroll = false;
	}

	return FALSE;

	*pResult = 0;
}
