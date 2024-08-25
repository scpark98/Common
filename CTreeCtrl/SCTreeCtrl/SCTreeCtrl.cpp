// CSCTreeCtrl.cpp: 구현 파일
//

#include "SCTreeCtrl.h"
#include <thread>
#include "../../Functions.h"
#include "../../MemoryDC.h"
#include "../../GdiPlusBitmap.h"

// CSCTreeCtrl

IMPLEMENT_DYNAMIC(CSCTreeCtrl, CTreeCtrl)

CSCTreeCtrl::CSCTreeCtrl()
{
	set_color_theme(color_theme_default, false);

	memset(&m_lf, 0, sizeof(LOGFONT));
}

CSCTreeCtrl::~CSCTreeCtrl()
{
	if (m_pEdit)
	{
		m_pEdit->DestroyWindow();
		delete m_pEdit;
	}
}


BEGIN_MESSAGE_MAP(CSCTreeCtrl, CTreeCtrl)
	ON_WM_ACTIVATE()
	ON_WM_ERASEBKGND()
	ON_WM_KEYDOWN()
	//ON_WM_PAINT()
	ON_NOTIFY_REFLECT_EX(TVN_SELCHANGED, &CSCTreeCtrl::OnTvnSelchanged)
	ON_WM_WINDOWPOSCHANGED()
	ON_NOTIFY_REFLECT(TVN_ITEMEXPANDING, &CSCTreeCtrl::OnTvnItemexpanding)
	ON_NOTIFY_REFLECT_EX(NM_CLICK, &CSCTreeCtrl::OnNMClick)
	ON_NOTIFY_REFLECT(TVN_BEGINDRAG, &CSCTreeCtrl::OnTvnBegindrag)
	ON_NOTIFY_REFLECT(NM_DBLCLK, &CSCTreeCtrl::OnNMDblclk)
	ON_WM_MOUSEMOVE()
	ON_WM_LBUTTONUP()
	ON_WM_HSCROLL()
	ON_NOTIFY_REFLECT_EX(TVN_BEGINLABELEDIT, &CSCTreeCtrl::OnTvnBeginlabeledit)
	ON_NOTIFY_REFLECT_EX(TVN_ENDLABELEDIT, &CSCTreeCtrl::OnTvnEndlabeledit)
	ON_NOTIFY_REFLECT(TVN_ITEMEXPANDED, &CSCTreeCtrl::OnTvnItemexpanded)
	ON_NOTIFY_REFLECT(NM_CUSTOMDRAW, &CSCTreeCtrl::OnNMCustomDraw)
	ON_WM_TIMER()
	ON_WM_CONTEXTMENU()
	ON_NOTIFY_REFLECT(NM_RCLICK, &CSCTreeCtrl::OnNMRClick)
	ON_REGISTERED_MESSAGE(Message_CSCMenu, &CSCTreeCtrl::OnMessageCSCMenu)
	ON_WM_VSCROLL()
	ON_WM_MOUSEHWHEEL()
	ON_WM_MOUSEWHEEL()
END_MESSAGE_MAP()



// CSCTreeCtrl 메시지 처리기
void CSCTreeCtrl::PreSubclassWindow()
{
	// TODO: 여기에 특수화된 코드를 추가 및/또는 기본 클래스를 호출합니다.
	CFont* font = GetParent()->GetFont();

	if (font != NULL)
		font->GetObject(sizeof(m_lf), &m_lf);
	else
		GetObject(GetStockObject(SYSTEM_FONT), sizeof(m_lf), &m_lf);

	reconstruct_font();

	CTreeCtrl::PreSubclassWindow();
}

void CSCTreeCtrl::reconstruct_font()
{
	m_font.DeleteObject();
	BOOL bCreated = m_font.CreateFontIndirect(&m_lf);
	SetFont(&m_font, true);

	m_font_size = get_font_size_from_logical_size(m_hWnd, m_lf.lfHeight);

	//폰트 height와 line height중에 큰 값으로 조정
	int height = GetItemHeight();
	SetItemHeight(MAX(height, -m_lf.lfHeight + 12));

	ASSERT(bCreated);
}

BOOL CSCTreeCtrl::PreTranslateMessage(MSG* pMsg)
{
	// TODO: 여기에 특수화된 코드를 추가 및/또는 기본 클래스를 호출합니다.
	if (pMsg->message == WM_KEYDOWN)
	{
		switch (pMsg->wParam)
		{
			case VK_F2:
			{
				edit_item(NULL);
				return TRUE;
			}

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
			case VK_DELETE :
			{
				if (m_in_editing)
					return FALSE;

				delete_item();

				return TRUE;
			}
		}
	}

	return CTreeCtrl::PreTranslateMessage(pMsg);
}


void CSCTreeCtrl::OnActivate(UINT nState, CWnd* pWndOther, BOOL bMinimized)
{
	CTreeCtrl::OnActivate(nState, pWndOther, bMinimized);

	// TODO: 여기에 메시지 처리기 코드를 추가합니다.
}


BOOL CSCTreeCtrl::OnEraseBkgnd(CDC* pDC)
{
	// TODO: 여기에 메시지 처리기 코드를 추가 및/또는 기본값을 호출합니다.
	CBrush backBrush(m_crBack);
	CBrush* pPrevBrush = pDC->SelectObject(&backBrush);
	CRect rect;
	pDC->GetClipBox(&rect);
	pDC->PatBlt(rect.left, rect.top, rect.Width(), rect.Height(),
		PATCOPY);
	pDC->SelectObject(backBrush);
	return TRUE;
	return CTreeCtrl::OnEraseBkgnd(pDC);
}


void CSCTreeCtrl::OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags)
{
	// TODO: 여기에 메시지 처리기 코드를 추가 및/또는 기본값을 호출합니다.

	CTreeCtrl::OnKeyDown(nChar, nRepCnt, nFlags);
}

//수동으로 직접 그려줄 경우는 BEGIN_MESSAGE_MAP에서 ON_WM_PAINT() 주석을 해제하면
//OnPaint()가 호출되고 여기서 직접 그릴 수 있다.
//폰트 색상 등의 UI를 자유롭게 변경할 수 있으나 스크롤 처리 등이 복잡해진다.
//GetItemRect()를 이용해서 그려주니 스크롤도 자동 처리된다.
//하지만 여전히 확장버튼, 체크박스, 아이콘 등 그 위치를 정확히 얻기 힘들고
//스크롤 처리도 힘들다. 이를 처리하기 위해서는 많은 처리가 필요하게 되어
//WM_PAINT를 사용하지 않고 CustomDraw 방식으로 변경함
/*
void CSCTreeCtrl::OnPaint()
{
	//tree가 제대로 구성되어 있는지 기본 OnPaint()로 확인 목적 코드
	if (m_use_custom_draw == false)
	{
		CTreeCtrl::OnPaint();
		return;
	}

	CPaintDC dc1(this); // device context for painting
					   // TODO: 여기에 메시지 처리기 코드를 추가합니다.
					   // 그리기 메시지에 대해서는 CTreeCtrl::OnPaint()을(를) 호출하지 마십시오.
	CRect rc;
	GetClientRect(rc);

	CMemoryDC dc(&dc1, &rc);
	Gdiplus::Graphics g(dc.m_hDC, rc);

	COLORREF	crText = m_crText;
	COLORREF	crBack = m_crBack;

	dc.FillSolidRect(rc, m_crBack);

	CFont font;
	CFont* pOldFont;

	font.CreateFontIndirect(&m_lf);
	pOldFont = (CFont*)dc.SelectObject(&font);

	dc.SetBkMode(TRANSPARENT);

	HTREEITEM hItem = GetRootItem();

	int		tab_count = 0;
	int		image_count = 0;
	int		image_index = -1, image_selected_index = -1;
	CRect	rItem[5];
	CString label;

	if (!m_is_shell_treectrl && m_use_own_imagelist)
	{
		if (m_imagelist.GetSafeHandle())
			image_count = m_imagelist.GetImageCount();
	}

	while (hItem)
	{
		crText = m_crText;

		//GetItemRect(hItem, rRow, FALSE);
		get_item_rect(hItem, rItem);
		label = GetItemText(hItem);

		//배경색을 그려주고
		if (GetItemState(hItem, TVIS_SELECTED) & TVIS_SELECTED)
		{
			dc.SetTextColor(red);

			//포커스에 따라 다르다. 단, 편집중일때도 CEdit이 focus를 가지게 되는데 inactive 색상 대신 원래의 선택배경색을 그대로 사용하자.
			if (GetFocus() == this || m_in_editing)
			{
				crText = m_crTextSelected;
				DrawRectangle(&dc, (GetStyle() & TVS_FULLROWSELECT) ? rItem[rect_row] : rItem[rect_label], m_crSelectedBorder, m_crBackSelected);
			}
			else if (GetStyle() & TVS_SHOWSELALWAYS)
			{
				crText = m_crTextSelectedInactive;
				dc.FillSolidRect((GetStyle() & TVS_FULLROWSELECT) ? rItem[rect_row] : rItem[rect_label], m_crBackSelectedInactive);
			}
		}


		//확장버튼을 그려주고
		if (rItem[rect_button].Width() > 0)//GetStyle() & TVS_HASBUTTONS)
		{
			//SetItemHeight() 또는 m_image_size가 16이 아니어도 확장버튼의 크기는 동일해야 한다.
			CRect expand_rect = makeCenterRect(rItem[rect_button].left + 16 / 2 - 1, rItem[rect_button].CenterPoint().y, 16, 16);
			expand_rect.DeflateRect(4, 4, 3, 3);

			if (ItemHasChildren(hItem))
			{
				//확장되어 있으면 '-' 버튼을 
				if (GetItemState(hItem, TVIF_STATE) & TVIS_EXPANDED)
				{
					DrawRectangle(&dc, expand_rect, GRAY(128));
					expand_rect.DeflateRect(3, 3);
					DrawLine(&dc, expand_rect.left - 1, expand_rect.CenterPoint().y, expand_rect.right, expand_rect.CenterPoint().y, GRAY(128));
				}
				//축소되어 있으면 '+' 버튼을 그려준다.
				else
				{
					DrawRectangle(&dc, expand_rect, GRAY(128));//, GRAY(128));
					expand_rect.DeflateRect(3, 3);
					DrawLine(&dc, expand_rect.left - 1, expand_rect.CenterPoint().y, expand_rect.right, expand_rect.CenterPoint().y, GRAY(128));
					DrawLine(&dc, expand_rect.CenterPoint().x, expand_rect.top - 1, expand_rect.CenterPoint().x, expand_rect.bottom, GRAY(128));
				}
			}
		}

		//체크박스를 그려주고
		if (rItem[rect_check].Width() > 0)//GetStyle() & TVS_CHECKBOXES)
		{
			//체크박스도 그 크기는 16으로 동일하다.
			CRect rCheck = makeCenterRect(rItem[rect_check].left + 16 / 2, rItem[rect_check].CenterPoint().y, 16, 16);
			rCheck.DeflateRect(2, 2, 2, 2);

#if 1
			draw_checkbox(&dc, rCheck, GetCheck(hItem));
#else
			DrawRectangle(&dc, rCheck, GRAY128, white);

			//3D border가 그려져서 별로다. <uxtheme.h>를 이용하여 윈도우에서 기본 제공하는 모습으로 표시 가능하나
			//우선 gdiplus로 직접 그려준다.
			//dc.DrawFrameControl(rCheck, DFC_BUTTON, GetCheck(hItem) ? DFCS_BUTTONCHECK | DFCS_CHECKED : DFCS_BUTTONCHECK);
			if (GetCheck(hItem))
			{
				Pen pen(Color(255, 64, 64, 64), 1.51);

				g.DrawLine(&pen, rCheck.left + 1, rCheck.CenterPoint().y - 2, rCheck.left + 4, rCheck.CenterPoint().y + 2);
				g.DrawLine(&pen, rCheck.left + 4, rCheck.CenterPoint().y + 3, rCheck.right - 2, rCheck.top + 2);
			}
#endif
		}

		//아이콘을 그려주고
		if (m_is_shell_treectrl)
		{
			GetItemImage(hItem, image_index, image_selected_index);
			image_index = m_pShellImageList->GetSystemImageListIcon(get_fullpath(GetSelectedItem()), true);
			m_pShellImageList->m_imagelist_small.Draw(&dc, image_index,
				CPoint(rItem[rect_icon].left, rItem[rect_icon].CenterPoint().y - m_image_size / 2), ILD_TRANSPARENT);
		}
		else if (m_use_own_imagelist && m_imagelist.GetSafeHandle() && (image_index < m_imagelist.GetImageCount()))
		{
			GetItemImage(hItem, image_index, image_selected_index);
			if (image_index >= 0 && image_index < image_count)
			{
				m_imagelist.Draw(&dc, image_index, CPoint(rItem[rect_icon].left, rItem[rect_icon].CenterPoint().y - m_image_size / 2), ILD_TRANSPARENT);
				rItem[rect_icon].left += m_image_size;
			}
		}

		dc.SetTextColor(crText);
		dc.DrawText(label, rItem[rect_label], DT_LEFT | DT_SINGLELINE | DT_VCENTER);

		//현재 노드가 EXPAND 상태이고 child가 존재한다면 child로 이동
		if (get_item_state(hItem, TVIS_EXPANDED) && ItemHasChildren(hItem))
		{
			tab_count++;
			hItem = GetChildItem(hItem);

			continue;
		}
		//EXPAND상태가 아니거나 child가 없다면 현재 node의 sibling으로 이동한다.
		else
		{
			HTREEITEM hSiblingItem = GetNextSiblingItem(hItem);
			if (hSiblingItem)
			{
				hItem = hSiblingItem;
				continue;
			}
			else
			{
				//만약 sibling도 없다면 parent의 sibling부터 다시 탐색한다.
				while (true)
				{
					tab_count--;
					hItem = GetParentItem(hItem);

					//parent가 NULL인 경우는 최상위 node들인 경우이므로 모든 탐색을 중지한다.
					if (hItem == NULL)
						return;

					//sibling이 있으면 그 node부터 다시 탐색을 시작하고 없다면 다시 parent의 sibling을 찾는다.
					hSiblingItem = GetNextSiblingItem(hItem);
					if (hSiblingItem)
					{
						hItem = hSiblingItem;
						break;
					}
				}
			}
		}
	}
#if 0
	for (int i = 0; i < m_folder_list.size(); i++)
	{
		GetItemRect(m_folder_list[i].item, &r, true);
		GetItemRect(m_folder_list[i].item, &rRow, false);

		r.right = rc.right - 1;

		//rc와 일부분도 겹치지 않는 아이템은 그리지 않는다.
		if (getIntersectionRect(rc, CRect(r.left - 32, r.top, r.right, r.bottom)).IsRectEmpty())
			continue;

		long t0 = getClock();

		indent = get_char_count(m_folder_list[i].fullpath, '\\');
		
		if (indent > 0)
		{
			if (m_folder_list[i].fullpath.GetLength() > 3)
				indent++;
		}

		//선택된 항목은 다른 색상으로 표시한다.
		//if (GetItemState(m_folder_list[i].item, TVIS_DROPHILITED) & TVIS_DROPHILITED)//m_folder_list[i].item == GetDropHilightItem())
		if (GetDropHilightItem() == m_folder_list[i].item)
		{
			TRACE(_T("drophilited\n"));
			//GetItemState(m_folder_list[i].item, TVIS_DROPHILITED))
			crText = m_crTextDropHilited;
			dc.FillSolidRect(rRow, m_crBackDropHilited);
		}
		else if (GetSelectedItem() == m_folder_list[i].item)
		//else if (GetItemState(m_folder_list[i].item, TVIS_SELECTED) & TVIS_SELECTED)
		{
			TRACE(_T("selected\n"));

			//포커스에 따라 다르다.
			if (GetFocus() == this)
			{
				crText = m_crTextSelected;
				DrawRectangle(&dc, rRow, m_crSelectedBorder, m_crBackSelected);
			}
			else if ((GetStyle() & TVS_SHOWSELALWAYS))
			{
				crText = m_crTextSelectedInactive;
				dc.FillSolidRect(rRow, m_crBackSelectedInactive);
			}
		}


		//텍스트를 출력할 때 DT_NOCLIP을 주지 않으면 가로 스크롤시에 잔상이 남게된다.
		dc.SetTextColor(crText);
		dc.DrawText(_T(" ") + m_folder_list[i].folder, r, DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_NOCLIP);

		//텍스트 왼쪽에 아이콘을 그려주고
		icon_index = m_pShellImageList->GetSystemImageListIcon(m_folder_list[i].fullpath, true);
		m_pShellImageList->m_imagelist_small.Draw(&dc, icon_index,
						CPoint(r.left - 16, r.CenterPoint().y - 8), ILD_TRANSPARENT);

		//그 왼쪽에는 확장/축소 버튼을 그려준다.
		if (has_sub_folders(m_folder_list[i].fullpath))
		{
			flags = GetItemState(m_folder_list[i].item, TVIF_STATE);
			CRect rArrow = r;
			rArrow.left = rArrow.left - 16 - 16;
			rArrow.right = rArrow.left + 16;
			CPoint cp(rArrow.CenterPoint());

			cp.Offset(-6, 0);

			//아래로 향한 화살표 표시
			if (flags & TVIS_EXPANDED)
			{
				DrawLine(&dc, cp.x - 4, cp.y - 2, cp.x + 1, cp.y + 3, GRAY192, 2);
				DrawLine(&dc, cp.x + 5, cp.y - 2, cp.x - 0, cp.y + 3, GRAY192, 2);
			}
			//일반상태의 > 화살표 표시
			else
			{
				DrawLine(&dc, cp.x - 2, cp.y - 4, cp.x + 3, cp.y + 1, GRAY192, 2);
				DrawLine(&dc, cp.x - 2, cp.y + 4, cp.x + 3, cp.y - 1, GRAY192, 2);
			}
		}

		long t1 = getClock();
		tTotal += (t1 - t0);
		//trace(_T("%ld : %s\n"), t1 - t0, m_folder_list[i].folder);
	}
#endif

	dc.SelectObject(pOldFont);

	//TRACE(_T("total %ld\n"), tTotal);
}
*/

/*
void CSCTreeCtrl::OnPaint()
{
	CPaintDC dc(this); // device context for painting

	CRect rcClip, rcClient;
	dc.GetClipBox(&rcClip);
	GetClientRect(&rcClient);

	// Set clip region to be same as that in paint DC
	CRgn rgn;
	rgn.CreateRectRgnIndirect(&rcClip);
	dc.SelectClipRgn(&rgn);
	rgn.DeleteObject();

	//COLORREF m_wndColor = m_crBack;// GetSysColor(COLOR_WINDOW);

	dc.SetViewportOrg(0, 0);
	//dc.SetTextColor(m_wndColor);

	// First let the control do its default drawing.
	CWnd::DefWindowProc(WM_PAINT, (WPARAM)dc.m_hDC, 0);

	HTREEITEM hItem = GetFirstVisibleItem();
	int n = GetVisibleCount();

	// the most urgent thing is to erase the labels that were drawn by the tree
	while (hItem != NULL && n >= 0)
	{
		CRect rect;
		GetItemRect(hItem, &rect, TRUE);

		//dc.FillSolidRect(rect, RGB(255, 0, 0));// m_crBack);
		hItem = GetNextVisibleItem(hItem);
		n--;
	}

	// create the font
	CFont* pFontDC;
	CFont fontDC, boldFontDC;
	LOGFONT logfont;

	CFont* pFont = GetFont();
	pFont->GetLogFont(&logfont);

	fontDC.CreateFontIndirect(&logfont);
	pFontDC = dc.SelectObject(&fontDC);

	logfont.lfWeight = 700;
	boldFontDC.CreateFontIndirect(&logfont);

	// and now let's get to the painting itself
	UINT nDrawFormat = DT_CENTER;

	BOOL bFoundSpl = FALSE;
	int m_nSplIndex = 0;

	hItem = GetFirstVisibleItem();
	n = GetVisibleCount();
	while (hItem != NULL && n >= 0)
	{
		CRect rect;

		BOOL bBold = (GetParentItem(hItem) == NULL);
		BOOL bSplColor = FALSE;
		if (bBold && !bFoundSpl && m_nSplIndex >= 0)
		{
			HTREEITEM hTmpItem = GetRootItem();
			int index = 0;
			while (hTmpItem != NULL)
			{
				if (index == m_nSplIndex && hTmpItem == hItem)
				{
					bFoundSpl = TRUE;
					bSplColor = TRUE;
				}
				hTmpItem = GetNextSiblingItem(hTmpItem);
				index++;
			}
		}

		UINT selflag = TVIS_DROPHILITED | TVIS_SELECTED;

		if (!(GetItemState(hItem, selflag) & selflag))
		{
			dc.SetBkMode(TRANSPARENT);

			CString text = GetItemText(hItem);
			GetItemRect(hItem, &rect, TRUE);


			//dc.SetBkColor(m_crBack);
			dc.SetTextColor(bSplColor ? RGB(255, 0, 0) : RGB(0, 0, 255));//m_crText);

			if (bBold)
				dc.SelectObject(&boldFontDC);

			dc.DrawText(text, rect, nDrawFormat);

			if (bBold)
				dc.SelectObject(&fontDC);
			//dc.SetTextColor(::GetSysColor(COLOR_WINDOWTEXT));
		}
		else
		{
			GetItemRect(hItem, &rect, TRUE);

			COLORREF m_highlightColor = m_crBackSelected;// ::GetSysColor(COLOR_HIGHLIGHT);
			CBrush brush(m_highlightColor);

			//dc.FillRect(rect, &brush);

			// draw a dotted focus rectangle
			dc.DrawFocusRect(rect);

			CString text = GetItemText(hItem);

			//dc.SetBkColor(m_highlightColor);
			dc.SetTextColor(m_crTextSelected);// ::GetSysColor(COLOR_HIGHLIGHTTEXT));

			if (bBold)
				dc.SelectObject(&boldFontDC);

			dc.DrawText(text, rect, nDrawFormat);

			if (bBold)
				dc.SelectObject(&fontDC);
		}

		hItem = GetNextVisibleItem(hItem);
		n--;
	}

	dc.SelectObject(pFontDC);
}
*/
BOOL CSCTreeCtrl::OnTvnSelchanged(NMHDR* pNMHDR, LRESULT* pResult)
{
	LPNMTREEVIEW pNMTreeView = reinterpret_cast<LPNMTREEVIEW>(pNMHDR);
	// TODO: 여기에 컨트롤 알림 처리기 코드를 추가합니다.
	//HWND hWnd = GetParent()->GetSafeHwnd();
	//::SendMessage(GetParent()->GetSafeHwnd(), Message_CSCTreeCtrl,
	//			(WPARAM)&CSCTreeCtrlMessage(this, message_selchanged),
	//			(LPARAM)&CString(get_fullpath(GetSelectedItem())));

	*pResult = 0;

	return FALSE;
}


void CSCTreeCtrl::OnWindowPosChanged(WINDOWPOS* lpwndpos)
{
	CTreeCtrl::OnWindowPosChanged(lpwndpos);

	// TODO: 여기에 메시지 처리기 코드를 추가합니다.
}

void CSCTreeCtrl::set_as_shell_treectrl(CShellImageList* pShellImageList, bool is_local)
{
	DeleteAllItems();
	//m_folder_list.clear();

	m_is_shell_treectrl = true;
	m_is_shell_treectrl_local = is_local;
	m_use_own_imagelist = true;
	m_image_size = 16;
	m_pShellImageList = pShellImageList;

	SetImageList(m_pShellImageList->get_imagelist(true), TVSIL_NORMAL);
	m_desktopItem = insert_special_folder(CSIDL_DESKTOP);
	m_documentItem = insert_special_folder(CSIDL_MYDOCUMENTS);
	m_computerItem = insert_special_folder(CSIDL_DRIVES);

	for (std::map<TCHAR, CString>::iterator it = m_pShellImageList->get_drive_map()->begin(); it != m_pShellImageList->get_drive_map()->end(); it++)
		insert_drive(it->second);

	//std::thread t(&CSCTreeCtrl::thread_insert_folders, this, GetRootItem());
	//t.detach();

	//m_folder_list = iterate_tree_with_no_recursion();

	//for test
	//expand_all();
}

void CSCTreeCtrl::thread_insert_folders(HTREEITEM hItem)
{
	if (hItem)
	{
		TRACE(_T("%s\n"), GetItemText(hItem));
		hItem = GetNextItem(hItem, TVGN_CHILD);
		while (hItem)
		{
			iterate_tree(hItem);
			hItem = GetNextItem(hItem, TVGN_NEXT);
		}
	}
	else
	{
		HTREEITEM hItem = GetNextItem(NULL, TVGN_ROOT);
		while (hItem)
		{
			iterate_tree(hItem);
			hItem = GetNextItem(hItem, TVGN_NEXT);
		}
	}
}

HTREEITEM CSCTreeCtrl::insert_special_folder(int csidl)
{
	if (false)//m_osType == OS_MAC)
	{
		//TV_INSERTSTRUCT tvInsert;
		//tvInsert.item.mask = TVIF_TEXT | TVIF_IMAGE | TVIF_SELECTEDIMAGE | TVIF_CHILDREN;
		//tvInsert.item.iImage = m_pShellList->GetSystemImageListIcon(GetStringById(NFTD_IDS_COMPUTER));
		//tvInsert.item.iSelectedImage = m_pShellList->GetSystemImageListIcon(GetStringById(NFTD_IDS_COMPUTER));
		//tvInsert.item.cChildren = TRUE;
		//tvInsert.hInsertAfter = TVI_LAST;
		//tvInsert.hParent = TVI_ROOT;
		//tvInsert.item.pszText = (LPWSTR)(LPCWSTR)GetStringById(NFTD_IDS_ROOT);
		//return InsertItem(&tvInsert);
	}
	else
	{
		CString path = get_known_folder(csidl);
		CString text = m_pShellImageList->get_shell_known_string_by_csidl(csidl);

		TV_INSERTSTRUCT tvInsert;
		tvInsert.item.mask = TVIF_TEXT | TVIF_IMAGE | TVIF_SELECTEDIMAGE;
		tvInsert.item.iImage = m_pShellImageList->GetSystemImageListIcon(csidl);
		tvInsert.item.iSelectedImage = m_pShellImageList->GetSystemImageListIcon(csidl);
		tvInsert.hInsertAfter = TVI_LAST;
		tvInsert.hParent = NULL;
		tvInsert.item.pszText = (LPTSTR)(LPCTSTR)text;
		return InsertItem(&tvInsert);
	}
}

void CSCTreeCtrl::insert_drive(CString driveName)
{
	CString real_path = convert_special_folder_to_real_path(driveName);

	TV_INSERTSTRUCT tvInsert;
	tvInsert.item.mask = TVIF_TEXT | TVIF_IMAGE | TVIF_SELECTEDIMAGE | TVIF_CHILDREN;
	tvInsert.item.iImage = m_pShellImageList->GetSystemImageListIcon(real_path);
	tvInsert.item.iSelectedImage = m_pShellImageList->GetSystemImageListIcon(real_path);
	//tvInsert.item.cChildren = 0;
	tvInsert.hInsertAfter = TVI_LAST;
	tvInsert.hParent = m_computerItem;
	tvInsert.item.pszText = (LPTSTR)(LPCTSTR)driveName;
	HTREEITEM hItem = InsertItem(&tvInsert);

	//아래 코드는 드라이브 루트를 추가할 때 1레벨 하위 폴더까지 기본으로 추가
	//insert_folder(hItem, real_path);
}

void CSCTreeCtrl::insert_folder(HTREEITEM hParent, CString sParentPath)
{
	CFileFind	FileFind;
	bool		bWorking;
	CString		curFolder;

	sParentPath = convert_special_folder_to_real_path(sParentPath, m_pShellImageList->get_csidl_map());

	if (sParentPath.Right(1) != "\\")
		sParentPath += "\\";

	bWorking = FileFind.FindFile(sParentPath + _T("*"));

	while (bWorking)
	{
		bWorking = FileFind.FindNextFile();

		if (!FileFind.IsDots() && !FileFind.IsHidden() && FileFind.IsDirectory())
		{
			curFolder = FileFind.GetFileName();
			TV_INSERTSTRUCT tvItem;
			tvItem.item.mask = TVIF_TEXT | TVIF_IMAGE | TVIF_SELECTEDIMAGE | TVIF_CHILDREN;
			tvItem.item.iImage = m_pShellImageList->GetSystemImageListIcon(_T("C:\\windows"));
			tvItem.item.iSelectedImage = m_pShellImageList->GetSystemImageListIcon(_T("C:\\windows")) + 1;
			tvItem.hInsertAfter = TVI_LAST;
			tvItem.hParent = hParent;
			tvItem.item.pszText = (LPTSTR)(LPCTSTR)curFolder;
			//하위 폴더가 있을때만 확장버튼이 표시되도록.
			tvItem.item.cChildren = get_sub_folders(FileFind.GetFilePath());
			HTREEITEM hItem = InsertItem(&tvItem);
		}
	}
}

void CSCTreeCtrl::insert_folder(WIN32_FIND_DATA* pFindFileData)
{

}
/*
HTREEITEM CSCTreeCtrl::find_item(const CString& name)
{
	HTREEITEM root = GetRootItem();
	while (root)
	{
		HTREEITEM item = find_item(name, root);
		if (item)
			return item;

		root = GetNextSiblingItem(root);
	}

	return NULL;
}
*/
HTREEITEM CSCTreeCtrl::find_item(const CString& name, HTREEITEM root)
{
	if (root == NULL)
		root = GetRootItem();

	CString text = GetItemText(root);
	
	if (text == name)
		return root;

	HTREEITEM child = GetChildItem(root);

	while (child)
	{
		HTREEITEM item = find_item(name, child);
		if (item)
			return item;

		child = GetNextSiblingItem(child);
	}

	return NULL;
}

CString CSCTreeCtrl::get_selected_item_text(bool include_parent)
{
	CString label;
	HTREEITEM hItem = GetSelectedItem();

	if (hItem == NULL)
		return label;

	label = GetItemText(hItem);

	if (!include_parent)
		return label;

	while (hItem)
	{
		hItem = GetParentItem(hItem);

		if (hItem == NULL)
			break;

		label = GetItemText(hItem) + _T("/") + label;
	}

	return label;
}

//해당 아이템이 축소되서 보이지 않는 상태인지(height가 음수로 리턴된다.)
bool CSCTreeCtrl::is_visible_item(HTREEITEM hItem)
{
	CRect r;
	GetItemRect(hItem, r, FALSE);
	return (r.Height() > 0);
}

void CSCTreeCtrl::OnTvnItemexpanding(NMHDR* pNMHDR, LRESULT* pResult)
{
	LPNMTREEVIEW pNMTreeView = reinterpret_cast<LPNMTREEVIEW>(pNMHDR);
	// TODO: 여기에 컨트롤 알림 처리기 코드를 추가합니다.
	TRACE(_T("%s\n"), __function__);
	HTREEITEM hItem = pNMTreeView->itemNew.hItem;
	if (m_is_shell_treectrl)
	{
		//만약 child가 없다면 아직 로딩되지 않은 노드이므로 검색해서 추가한다.
		//물론 실제 child가 없는 폴더일수도 있다.
		if (GetChildItem(hItem) == NULL)
			insert_folder(hItem, get_fullpath(hItem));

		//m_folder_list = iterate_tree_with_no_recursion();
	}

	*pResult = 0;
}

//check, has button등에 의해 HitTest가 알아서 해당 영역을 알려주리라 기대했으나
//hit 영역을 정확히 리턴해주지 않는다.
//button, check, text 영역을 내가 정해서 그려주고 이벤트도 수동으로 처리해주는 것이 나을듯하다.
//이것을 수동으로 해보니 절대 정확히 알아낼 수 없다.
//결국 OnPaint()에서 직접 그려주는 방법이 아닌 OnCustomDraw()로 전환함.
BOOL CSCTreeCtrl::OnNMClick(NMHDR* pNMHDR, LRESULT* pResult)
{
	if (m_in_editing)
	{
		edit_end(true);
	}

	LPNMTVDISPINFO pTVDispInfo = reinterpret_cast<LPNMTVDISPINFO>(pNMHDR);

	long t0 = clock();

	CPoint pt;
	GetCursorPos(&pt);
	ScreenToClient(&pt);
	HTREEITEM hCurItem = HitTest(pt);
	HTREEITEM hDispItem = pTVDispInfo->item.hItem;

	CRect r;
	GetItemRect(hCurItem, r, TRUE);

	TRACE(_T("cur = %ld, last = %ld, m_last_clicked_item = %p, hCurItem = %p, hDispItem = %p\n"),
		t0, m_last_clicked_time, m_last_clicked_item, hCurItem, hDispItem);

	if (r.PtInRect(pt))
	{
		if ((t0 - m_last_clicked_time > 500) && (t0 - m_last_clicked_time < 2000))
		{
			if (hCurItem == m_last_clicked_item)
			{
				TRACE(_T("edit start\n"));
				edit_item();
			}
			else
			{
				TRACE(_T("diff item\n"));
				m_last_clicked_time = t0;
				m_last_clicked_item = hCurItem;
			}
		}
		else
		{
			TRACE(_T("time over\n"));
			m_last_clicked_time = t0;
			m_last_clicked_item = hCurItem;
		}
	}
	else
	{
		m_last_clicked_time = 0;
		m_last_clicked_item = NULL;
		TRACE(_T("m_last_clicked_time = %ld\n"), m_last_clicked_time);
	}

	/*
	CPoint pt;

	GetCursorPos(&pt);
	ScreenToClient(&pt);


	// TODO: 여기에 컨트롤 알림 처리기 코드를 추가합니다.
	UINT nFlags = 0;
	CString str;
	CString fullpath;

	HTREEITEM hItem = hit_test(&nFlags);
	if (hItem == NULL)
		return TRUE;

	SelectItem(hItem);

	//확장버튼을 누르면 (확장 단추 표시 속성이 TRUE일 경우에만 처리됨)
	if (nFlags & TVHT_ONITEMBUTTON)
	{
		nFlags = GetItemState(hItem, TVIS_EXPANDED);

		TRACE(_T("[%s] expanded = %d\n"), GetItemText(hItem), nFlags & TVIS_EXPANDED);

		if (m_is_shell_treectrl)
		{
			//만약 child가 없다면 아직 로딩되지 않은 노드이므로 검색해서 추가한다.
			//물론 실제 child가 없는 폴더일수도 있다.
			if (GetChildItem(hItem) == NULL)
				insert_folder(hItem, get_fullpath(hItem));

			m_folder_list = iterate_tree_with_no_recursion();
		}
		else
		{
			if (nFlags & TVIS_EXPANDED)
				Expand(hItem, TVE_COLLAPSE);
			else// if (nFlags & TVIS_EXPANDED)
				Expand(hItem, TVE_EXPAND);
		}
	}
	else if (nFlags & TVHT_ONITEMSTATEICON)
	{
		//BOOL checked = GetCheck(hItem);
		//SetCheck(hItem, !checked);
	}
	*/
	*pResult = 0;

	return FALSE;
}

CString CSCTreeCtrl::get_fullpath(HTREEITEM hItem)
{
	if (hItem == NULL)
	{
		hItem = GetSelectedItem();
		if (hItem == NULL)
			return _T("");
	}

	CString fullpath;
	CString folder;

	while (hItem)
	{
		folder = GetItemText(hItem);
		//AfxMessageBox(folder);
		fullpath = folder + _T("\\") + fullpath;

		if (folder.Right(2) == _T(":)"))
		{
			break;
		}

		hItem = GetParentItem(hItem);
	}

	return convert_special_folder_to_real_path(fullpath, m_pShellImageList->get_csidl_map());
}

void CSCTreeCtrl::select_folder(CString fullpath)
{
	fullpath = m_pShellImageList->get_shell_known_string_by_csidl(CSIDL_DRIVES) + _T("\\") + convert_real_path_to_special_folder(fullpath);
	//AfxMessageBox(fullpath);

	std::deque<CString> dq;
	get_token_string(fullpath, dq, '\\');

	HTREEITEM item = NULL;

	for (int i = 0; i < dq.size(); i++)
	{
		TRACE(_T("finding [%s] from [%s] node...\n"), dq[i], (item ? GetItemText(item) : _T("root")));

		//만약 현재 노드에 아직 child가 추가된 상태가 아니라면 우선 children을 넣어준 후 검색해야 한다.
		if (GetChildItem(item) == NULL)
		{
			insert_folder(item, get_fullpath(item));
			Expand(item, TVE_EXPAND);
			//m_folder_list = iterate_tree_with_no_recursion();
		}

		if (item)
			item = find_item(dq[i], item);
		else
			item = find_item(dq[i]);

		if (!item)
			return;

		Expand(item, TVE_EXPAND);
	}

	if (item)
		SelectItem(item);
}

void CSCTreeCtrl::iterate_tree(HTREEITEM hItem)
{
	if (hItem)
	{
		TRACE(_T("%s\n"), GetItemText(hItem));
		hItem = GetNextItem(hItem, TVGN_CHILD);
		while (hItem)
		{
			iterate_tree(hItem);
			hItem = GetNextItem(hItem, TVGN_NEXT);
		}
	}
	else
	{
		HTREEITEM hItem = GetNextItem(NULL, TVGN_ROOT);
		while (hItem)
		{
			iterate_tree(hItem);
			hItem = GetNextItem(hItem, TVGN_NEXT);
		}
	}
}

void CSCTreeCtrl::iterate_tree_in_order(HTREEITEM hItem)
{
	Trace(_T("\n"));

	if (hItem == NULL)
		hItem = GetRootItem();

	int tab_count = 0;
	CString label;

	while (hItem)
	{
		label = GetItemText(hItem);
		
		Trace_only(make_string(_T("\t"), tab_count));

		Trace_only(_T("%s\n"), label);

		//child가 있다면 child로 이동하고
		if (ItemHasChildren(hItem))
		{
			tab_count++;
			hItem = GetChildItem(hItem);
			continue;
		}
		//child가 없다면 현재 node의 sibling으로 이동한다.
		else
		{
			HTREEITEM hSiblingItem = GetNextSiblingItem(hItem);
			if (hSiblingItem)
			{
				hItem = hSiblingItem;
				continue;
			}
			else
			{
				//만약 sibling도 없다면 parent의 sibling부터 다시 탐색한다.
				while (true)
				{
					tab_count--;
					hItem = GetParentItem(hItem);

					//parent가 NULL인 경우는 최상위 node들인 경우이므로 모든 탐색을 중지한다.
					if (hItem == NULL)
						return;

					//sibling이 있으면 그 node부터 다시 탐색을 시작하고 없다면 다시 parent의 sibling을 찾는다.
					hSiblingItem = GetNextSiblingItem(hItem);
					if (hSiblingItem)
					{
						hItem = hSiblingItem;
						break;
					}
				}
			}
		}
	}

	//expand 상태가 아닌 node의 children은 옵션으로 탐색여부가 달라진다.
}

//recursion을 사용하지 않고 모든 node를 검색한다.
//stack을 이용하므로 그 차례가 실제 UI와 다르므로 deque에 넣은 후 sort를 이용한다.
//탐색기의 트리일 경우에 특화된 코드가 있으므로 범용으로 사용하려면 좀 더 보완이 필요함.
std::deque<CSCTreeCtrlFolder> CSCTreeCtrl::iterate_tree_with_no_recursion(HTREEITEM hItem)
{
	std::deque<HTREEITEM> s;
	std::deque<CSCTreeCtrlFolder> folders;
	CString fullpath;
	CString text;

	HTREEITEM item = (hItem ? hItem : GetRootItem());
	while (item != NULL || s.size())
	{
		while (item != NULL)
		{
			s.push_back(item);
			item = GetChildItem(item);
		}

		item = s[0];
		s.pop_front();

		text = GetItemText(item);
		//trace(_T("%s\n"), text);
		if (text == m_pShellImageList->get_shell_known_string_by_csidl(CSIDL_DESKTOP) ||
			text == m_pShellImageList->get_shell_known_string_by_csidl(CSIDL_PERSONAL))
		{
			fullpath = GetItemText(item);
		}
		else
		{
			fullpath = get_fullpath(item);
		}

		//내 PC일 경우는 ""로 리턴되므로
		if (fullpath.IsEmpty())
			folders.push_back(CSCTreeCtrlFolder(item, fullpath, m_pShellImageList->get_shell_known_string_by_csidl(CSIDL_DRIVES)));
		else
			folders.push_back(CSCTreeCtrlFolder(item, fullpath, get_part(fullpath, fn_last_folder)));
		item = GetNextSiblingItem(item);
	}

	//바탕 화면, 문서, 내 PC 3개 항목을 제외하고 정렬.
	//기본 정렬과 탐색기의 정렬은 약간 다르므로 탐색기와 같은 정렬이 되도록.
	std::sort(folders.begin() +3, folders.end(),
		[](CSCTreeCtrlFolder a, CSCTreeCtrlFolder b)
		{
			return !is_greater_with_numeric(a.fullpath, b.fullpath);
		}
	);

	for (int i = 0; i < folders.size(); i++)
	{
		//trace(_T("%s\n"), folders[i]);

		//"C:\\" => "로컬 디스크 (C:)"로 변경해준다.
		if (folders[i].fullpath.Right(2) == _T(":\\"))
		{
			folders[i].folder = get_drive_volume(folders[i].fullpath[0]);
		}
	}

	return folders;
}

void CSCTreeCtrl::expand_all(bool expand)
{
	HTREEITEM hItem;

	hItem = GetFirstVisibleItem();

	while (hItem != NULL)
	{
		Expand(hItem, expand ? TVE_EXPAND : TVE_COLLAPSE);
		hItem = GetNextItem(hItem, TVGN_NEXTVISIBLE);
	}
}
/*
HTREEITEM CSCTreeCtrl::hit_test(UINT* nFlags)
{
	CPoint pt;

	GetCursorPos(&pt);
	ScreenToClient(&pt);

	HTREEITEM hItem = HitTest(pt, nFlags);

	CRect r[5];

	get_item_rect(hItem, r);

	if ((GetStyle() & TVS_HASBUTTONS) && r[rect_button].PtInRect(pt))
	{
		*nFlags = TVHT_ONITEMBUTTON;
		TRACE(_T("hItem = %p, nFlags = %s\n"), hItem, ENUM_TO_CSTRING(TVHT_ONITEMBUTTON));
	}
	else if ((GetStyle() & TVS_CHECKBOXES) && r[rect_check].PtInRect(pt))
	{
		*nFlags = TVHT_ONITEMSTATEICON;
		TRACE(_T("hItem = %p, nFlags = %s\n"), hItem, ENUM_TO_CSTRING(TVHT_ONITEMSTATEICON));
	}
	else if (r[rect_icon].PtInRect(pt))
	{
		*nFlags = TVHT_ONITEMICON;
		TRACE(_T("hItem = %p, nFlags = %s\n"), hItem, ENUM_TO_CSTRING(TVHT_ONITEMICON));
	}
	else if (r[rect_label].PtInRect(pt))
	{
		*nFlags = TVHT_ONITEMLABEL;
		TRACE(_T("hItem = %p, nFlags = %s\n"), hItem, ENUM_TO_CSTRING(TVHT_ONITEMLABEL));
	}
	else if (pt.x > r[rect_label].right)
	{
		*nFlags = TVHT_ONITEMRIGHT;
		TRACE(_T("hItem = %p, nFlags = %s\n"), hItem, ENUM_TO_CSTRING(TVHT_ONITEMRIGHT));
	}
	else
	{
		*nFlags = TVHT_ONITEMINDENT;
		TRACE(_T("hItem = %p, nFlags = %d (%s)\n"), hItem, *nFlags, ENUM_TO_CSTRING(TVHT_ONITEMINDENT));
	}

	return hItem;
}
*/

void CSCTreeCtrl::set_color_theme(int theme, bool apply_now)
{
	switch (theme)
	{
		//최근 윈도우 탐색기의 색상을 보면 텍스트 색상은 선택여부, inactive에 무관하게 동일하다.
	case color_theme_default:
		m_crText = ::GetSysColor(COLOR_BTNTEXT);
		m_crTextSelected = m_crText;// ::GetSysColor(COLOR_HIGHLIGHTTEXT);
		m_crTextSelectedInactive = m_crText;// ::GetSysColor(COLOR_INACTIVECAPTIONTEXT);
		m_crTextDropHilited = ::GetSysColor(COLOR_HIGHLIGHTTEXT);;
		m_crBack = ::GetSysColor(COLOR_WINDOW);
		m_crBackSelected = RGB(204, 232, 255);// ::GetSysColor(COLOR_HIGHLIGHT);
		m_crBackSelectedInactive = RGB(217, 217, 217);// ::GetSysColor(COLOR_HIGHLIGHT);
		m_crBackDropHilited = ::GetSysColor(COLOR_HIGHLIGHT);
		m_crSelectedBorder = RGB(153, 209, 255);
		break;
	case color_theme_light_blue:
		m_crText = ::GetSysColor(COLOR_BTNTEXT);
		m_crTextSelected = RGB(65, 102, 146);
		m_crTextSelectedInactive = RGB(65, 102, 146);
		m_crTextDropHilited = get_color(m_crText, -48);
		m_crBack = RGB(193, 219, 252);
		m_crBackSelected = get_color(m_crBack, -48);
		m_crBackSelectedInactive = get_color(m_crBack, -48);
		m_crBackDropHilited = get_color(m_crBack, -48);;
		m_crSelectedBorder = RGB(153, 209, 255);
		break;
	case color_theme_navy_blue:
		m_crText = RGB(204, 216, 225);
		m_crTextSelected = RGB(234, 246, 255);
		m_crTextSelectedInactive = RGB(105, 142, 186);
		m_crTextDropHilited = get_color(m_crText, 48);
		m_crBack = RGB(74, 94, 127);
		m_crBackSelected = RGB(15, 36, 41);
		m_crBackSelectedInactive = RGB(15, 36, 41);
		m_crBackDropHilited = get_color(m_crBack, 48);
		m_crSelectedBorder = RGB(153, 209, 255);
		break;
	case color_theme_dark_blue:
		m_crText = RGB(16, 177, 224);
		m_crTextSelected = RGB(224, 180, 59);
		m_crTextSelectedInactive = RGB(105, 142, 186);
		m_crTextDropHilited = get_color(m_crText, 48);
		m_crBack = RGB(2, 21, 36);
		m_crBackSelected = RGB(3, 42, 59);
		m_crBackSelectedInactive = RGB(15, 36, 41);
		m_crBackDropHilited = blue;
		m_crSelectedBorder = RGB(153, 209, 255);
		break;
	case color_theme_dark_gray:
		m_crText = RGB(164, 164, 164);
		m_crTextSelected = RGB(241, 241, 241);
		m_crTextSelectedInactive = get_color(m_crTextSelected, -36);
		m_crTextDropHilited = get_color(m_crText, 255);
		m_crBack = RGB(64, 64, 64);
		m_crBackSelected = get_color(m_crBack, -32);
		m_crBackSelectedInactive = get_color(m_crBack, -32);
		m_crBackDropHilited = blue;
		m_crSelectedBorder = RGB(128, 128, 128);
		break;
	case color_theme_dark:
		m_crText = RGB(212, 212, 212);
		m_crTextSelected = RGB(241, 241, 241);
		m_crTextSelectedInactive = get_color(m_crTextSelected, -36);
		m_crTextDropHilited = white;
		m_crBack = RGB(37, 37, 38);
		m_crBackSelected = RGB(0, 120, 215);
		m_crBackSelectedInactive = RGB(0, 120, 215);
		m_crBackDropHilited = blue;
		m_crSelectedBorder = m_crBackSelected;// RGB(128, 128, 128);
		break;
	}

	if (apply_now)
		Invalidate();
}


void CSCTreeCtrl::OnNMDblclk(NMHDR* pNMHDR, LRESULT* pResult)
{
	// TODO: 여기에 컨트롤 알림 처리기 코드를 추가합니다.
	/*
	CPoint pt;
	UINT nFlags = 0;

	GetCursorPos(&pt);
	ScreenToClient(&pt);

	HTREEITEM hItem = HitTest(pt, &nFlags);
	TRACE(_T("%s, %d, %d, %d\n"), get_fullpath(hItem), pt.x, pt.y, nFlags);

	//아이콘+레이블 영역이라면
	if (nFlags & TVHT_ONITEM)
	{
		TRACE(_T("button\n"));
		nFlags = GetItemState(hItem, TVIF_STATE);
		TRACE(_T("expanded = %d\n"), nFlags & TVIS_EXPANDED);

		//if (nFlags & TVIS_EXPANDED)
		{
			//만약 child가 없다면 아직 로딩되지 않은 노드이므로 검색해서 추가한다.
			//물론 실제 child가 없는 폴더일수도 있다.
			if (GetChildItem(hItem) == NULL)
			{
				insert_folder(hItem, get_fullpath(hItem));
				m_folder_list = iterate_tree_with_no_recursion();
			}
		}
	}
	*/
	*pResult = 0;
}


void CSCTreeCtrl::OnTvnBegindrag(NMHDR* pNMHDR, LRESULT* pResult)
{
	LPNMTREEVIEW pNMTreeView = reinterpret_cast<LPNMTREEVIEW>(pNMHDR);
	// TODO: 여기에 컨트롤 알림 처리기 코드를 추가합니다.

	if (!m_use_drag_and_drop)
		return;

	CRect	rc;
	GetClientRect(rc);

	m_DragItem = pNMTreeView->itemNew.hItem;
	CString path = convert_special_folder_to_real_path(get_fullpath(m_DragItem));
	
	//focus가 없거나 선택되지 않은 상태에서 바로 drag가 시작되면
	//drag 이미지만 표시되므로 focus를 주고 drag하고 있는 아이템을 선택상태로 표시해줘야 한다.
	//"선택 영역 항상 표시" 속성 또한 true여야 한다.
	::SetFocus(m_hWnd);
	SetItemState(m_DragItem, TVIS_SELECTED, TVIF_STATE);

	int item_count = get_sub_folders(path, NULL, false, true);
	CGdiplusBitmap bmpRes(64, 64, PixelFormat32bppARGB, Gdiplus::Color(128, 255, 0, 0));

	if (m_pDragImage)
	{
		m_pDragImage->DeleteImageList();
		m_pDragImage = NULL;
	}

	//drag_image가 없다면 노드 자체 아이콘 및 레이블을 이용한다.
	//GDI를 이용해서 create_drag_image()를 사용했으나 아이콘과 함께 레이블을 출력할 때 오동작함. 수정 필요.
	//GDIPlus를 이용한 create_drag_image()를 이용해서 만드는 것도 좋을듯함.
	if (m_drag_images_id.size() == 0)
	{
		//bmpRes.create_drag_image(this);
		m_pDragImage = create_drag_image((CTreeCtrl*)this, &pNMTreeView->ptDrag);
	}
	else
	{
		//drag_image가 1개일 경우
		if (m_drag_images_id.size() == 1)
		{
			bmpRes.load(m_drag_images_id[0]);
		}
		//drag_image가 2개 이상일 경우는 drag count에 따라 0번 또는 1번 이미지를 사용한다.
		else if (m_drag_images_id.size() > 1)
		{
			bmpRes.load(item_count == 1 ? m_drag_images_id[0] : m_drag_images_id[1]);
		}

		//bmpRes.draw_text(bmpRes.width / 2 + 10, bmpRes.height / 2, i2S(item_count), 20, 2,
		//	_T("맑은 고딕"), Gdiplus::Color(192, 0, 0, 0), Gdiplus::Color(192, 255, 128, 128), DT_CENTER | DT_VCENTER);

		m_pDragImage = new CImageList();
		m_pDragImage->Create(bmpRes.width, bmpRes.height, ILC_COLOR32, 1, 1);

		HICON hicon;
		bmpRes.m_pBitmap->GetHICON(&hicon);
		m_pDragImage->Add(hicon);
	}

	ASSERT(m_pDragImage); //make sure it was created

	//// Set dragging flag and others
	m_bDragging = TRUE;	//we are in a drag and drop operation
	m_pDragWnd = this; //make note of which list we are dragging from
	m_pDropWnd = this;	//at present the drag list is the drop list

	//// Capture all mouse messages
	SetCapture();

	//// Change the cursor to the drag image
	////	(still must perform DragMove() in OnMouseMove() to show it moving)
	m_pDragImage->BeginDrag(0, CPoint(-10, -14));	//이 좌표를 주지 않으면 move할때 이미지가 잘못된 위치에 표시된다.
	m_pDragImage->DragEnter(GetDesktopWindow(), pNMTreeView->ptDrag);

	TRACE(_T("start drag...\n"));

	*pResult = 0;
}

//https://jiniya.net/tt/594/
//이 함수는 드래그 이미지를 직접 생성해주는 코드지만 취약점이 많은 코드이므로 사용 중지! 참고만 할것.
CImageList* CSCTreeCtrl::create_drag_image(CTreeCtrl* pTree, LPPOINT lpPoint)
{
	//DWORD dwStyle = GetWindowLongPtr(pTree->m_hWnd, GWL_STYLE) & LVS_TYPEMASK;
	CRect rectComplete(0, 0, 0, 0);
	HTREEITEM hItem = GetSelectedItem();

	if (hItem == NULL)
		return NULL;

	// Determine List Control Client width size  
	CRect rc;
	pTree->GetClientRect(rc);
	int nWidth = rc.Width() + 50;

	GetItemRect(hItem, rectComplete, TRUE);

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
		, RGB(255, 255, 0));

	// 안티알리아스 안된 폰트를 사용하는게 핵심
	CFont* pFont = pTree->GetFont();
	LOGFONT lf;
	pFont->GetLogFont(&lf);
	//lf.lfQuality = NONANTIALIASED_QUALITY;
	CFont newFont;
	newFont.CreateFontIndirect(&lf);

	CFont* oldFont = dcMem.SelectObject(&newFont);
	////////////////////////////////////////////////  

	// Paint each DragImage in the DC  
	TCHAR buffer[1000];
	TVITEM item = { 0 };
	item.mask = LVIF_TEXT | LVIF_IMAGE;
	item.hItem = hItem;
	item.pszText = buffer;
	item.cchTextMax = 999;

	pTree->GetItem(&item);

	// Draw the icon  

	CImageList* pSingleImageList = pTree->GetImageList(TVSIL_NORMAL);

	if (pSingleImageList)
	{
		CRect rectIcon;
		pTree->GetItemRect(hItem, rectIcon, LVIR_ICON);

		IMAGEINFO info;
		pSingleImageList->GetImageInfo(item.iImage, &info);
		CPoint p((rectIcon.left - rectComplete.left
			+ rectIcon.right - rectComplete.left) / 2
			- (info.rcImage.right - info.rcImage.left) / 2,
			(rectIcon.top - rectComplete.top
				+ rectIcon.bottom - rectComplete.top) / 2
			- (info.rcImage.bottom - info.rcImage.top) / 2);

		pSingleImageList->Draw(&dcMem, item.iImage, CPoint(0, 0), ILD_TRANSPARENT);
	}

	// Draw the text  
	CString text;
	text = item.pszText;
	CRect textRect;
	pTree->GetItemRect(hItem, textRect, LVIR_LABEL);

	textRect.top -= rectComplete.top - 2;
	textRect.bottom -= rectComplete.top + 1;
	textRect.left -= rectComplete.left - 2;
	textRect.right -= rectComplete.left;

	//dcMem.FillSolidRect(textRect, RGB(255, 0, 0));
	dcMem.SetTextColor(RGB(255, 0, 0));
	DWORD flags = DT_LEFT | DT_SINGLELINE | DT_VCENTER | DT_NOCLIP | DT_WORDBREAK;
	dcMem.DrawText(text, -1, textRect, flags);

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

void CSCTreeCtrl::OnMouseMove(UINT nFlags, CPoint point)
{
	// TODO: 여기에 메시지 처리기 코드를 추가 및/또는 기본값을 호출합니다.
	if (m_bDragging)
	{
		GetCursorPos(&point);
		TRACE(_T("cursor = %d, %d\n"), point.x, point.y);
		m_pDragImage->DragMove(point); //move the drag image to those coordinates

		// Unlock window updates (this allows the dragging image to be shown smoothly)
		//m_pDragImage->DragShowNolock(false);

		//// Get the CWnd pointer of the window that is under the mouse cursor
		CWnd* pDropWnd = WindowFromPoint(point);
		TRACE(_T("this = %p, pDropWnd = %p\n"), this, pDropWnd);
		ASSERT(pDropWnd); //make sure we have a window

		m_pDropWnd = pDropWnd;

		pDropWnd->ScreenToClient(&point);

		//drag되는 위치가 컨트롤의 상하끝단에 위치하면 스크롤시켜줘야 한다.

		//If we are hovering over a CListCtrl we need to adjust the highlights
		if (pDropWnd->IsKindOf(RUNTIME_CLASS(CListCtrl)))
		{
			//Note that we can drop here
			::SetCursor(AfxGetApp()->LoadStandardCursor(MAKEINTRESOURCE(IDC_ARROW)));
			UINT uFlags;
			CListCtrl* pList = (CListCtrl*)pDropWnd;

			//기존 LVIS_DROPHILITED를 해제해주지 않으니 아래 새로 LVIS_DROPHILITED하는 항목이 갱신되지 않는다.
			if (m_nDropIndex >= 0)
			{
				// Turn off hilight for previous drop target
				pList->SetItemState(m_nDropIndex, 0, LVIS_DROPHILITED);
				// Redraw previous item
				pList->RedrawItems(m_nDropIndex, m_nDropIndex);
			}

			// Get the item that is below cursor
			m_nDropIndex = ((CListCtrl*)pDropWnd)->HitTest(point, &uFlags);
			TRACE(_T("nDropIndex in ListCtrl = %d\n"), m_nDropIndex);
			// Highlight it (폴더인 경우에만 hilite시킨다. 폴더는 크기 컬럼이 empty임)
			if (m_nDropIndex >= 0 && ((CListCtrl*)pDropWnd)->GetItemText(m_nDropIndex, 1) == _T(""))
			{
				TRACE(_T("new LVIS_DROPHILITED\n"), m_nDropIndex);
				pList->SetItemState(m_nDropIndex, LVIS_DROPHILITED, LVIS_DROPHILITED);
			}

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
			m_DropItem = ((CTreeCtrl*)pDropWnd)->HitTest(point, &uFlags);
			TRACE(_T("%d, %d, hItem = %p\n"), point.x, point.y, m_DropItem);
			pTree->SelectDropTarget(m_DropItem);
			ASSERT(m_DropItem == pTree->GetDropHilightItem());
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

	CTreeCtrl::OnMouseMove(nFlags, point);
}


void CSCTreeCtrl::OnLButtonUp(UINT nFlags, CPoint point)
{
	// TODO: 여기에 메시지 처리기 코드를 추가 및/또는 기본값을 호출합니다.
	if (m_bDragging)
	{
		TRACE(_T("OnLButtonUp\n"));
		// Release mouse capture, so that other controls can get control/messages
		ReleaseCapture();

		// Note that we are NOT in a drag operation
		m_bDragging = false;

		// End dragging image
		m_pDragImage->DragLeave(GetDesktopWindow());
		m_pDragImage->EndDrag();
		delete m_pDragImage; //must delete it because it was created at the beginning of the drag

		CPoint pt(point); //Get current mouse coordinates
		ClientToScreen(&pt); //Convert to screen coordinates

		// Get the CWnd pointer of the window that is under the mouse cursor
		CWnd* pDropWnd = WindowFromPoint(pt);
		ASSERT(pDropWnd); //make sure we have a window pointer

		if (pDropWnd->IsKindOf(RUNTIME_CLASS(CListCtrl)) ||
			pDropWnd->IsKindOf(RUNTIME_CLASS(CTreeCtrl)))
		{
			m_pDropWnd = pDropWnd; //Set pointer to the list we are dropping on
			DroppedHandler(m_pDragWnd, m_pDropWnd); //Call routine to perform the actual drop
		}

		//ListCtrl에서 drag하여 drophilited가 표시된 상태에서 빠르게 마우스를 밖으로 이동시키면
	//마우스를 떼도 drophilited된 항목 표시가 여전히 남는다.
	//메인에 메시지를 보내서 해당 컨트롤들의 아이템에서 drophilited를 제거시켜준다.
		::SendMessage(GetParent()->GetSafeHwnd(), Message_CSCTreeCtrl, (WPARAM) & (CSCTreeCtrlMessage(this, message_drag_and_drop, NULL)), (LPARAM)0);
	}
	else
	{
		return;
	}

	CTreeCtrl::OnLButtonUp(nFlags, point);
}

void CSCTreeCtrl::DroppedHandler(CWnd* pDragWnd, CWnd* pDropWnd)
{
	//drop되면 그 이벤트만 메인에 알리고
	//메인에서는 drag관련 정보와 drop정보를 이용해서 원하는 처리만 한다.
	//따라서 맨 아래 ::SendMessage만 필요하며
	//중간 코드들은 메인에서 활용하는 방법에 대한 예시임.

	CString droppedItem;

	if (pDropWnd->IsKindOf(RUNTIME_CLASS(CListCtrl)))
	{
		CListCtrl* pDropListCtrl = (CListCtrl*)pDropWnd;

		if (m_nDropIndex >= 0)
			droppedItem = pDropListCtrl->GetItemText(m_nDropIndex, 0);

		TRACE(_T("drag item = %s\n"), GetItemText(m_DragItem));

		TRACE(_T("dropped on = %s\n"), (droppedItem.IsEmpty() ? _T("dropped on ListCtrl") : droppedItem));
	}
	else if (pDropWnd->IsKindOf(RUNTIME_CLASS(CTreeCtrl)))
	{
		CTreeCtrl* pDropTreeCtrl = (CTreeCtrl*)pDropWnd;

		//dragItem, dropItem 정보는 drag가 시작된 컨트롤이 기억하고 있다.
		TRACE(_T("drag item = %s\n"), GetItemText(m_DragItem));
		TRACE(_T("dropped on = %s\n"), pDropTreeCtrl->GetItemText(m_DropItem));

		//같은 CTreeCtrl내에서 노드 이동, 같은 item이면 skip.
		if ((pDragWnd == pDropWnd) && (m_DragItem != m_DropItem))
		{
			move_tree_item(this, m_DragItem, m_DropItem);
		}
	}

	::SendMessage(GetParent()->GetSafeHwnd(), Message_CSCTreeCtrl, (WPARAM)&(CSCTreeCtrlMessage(this, message_drag_and_drop, pDropWnd)), (LPARAM)0);
}


void CSCTreeCtrl::OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar)
{
	// TODO: 여기에 메시지 처리기 코드를 추가 및/또는 기본값을 호출합니다.

	//가로 스크롤시에 선택된 항목 사각형이 제대로 그려지지 않는 문제가 있어서 Invalidate()을 호출함.
	Invalidate();

	CTreeCtrl::OnHScroll(nSBCode, nPos, pScrollBar);
}

//file의 내용을 읽어서 load_from_string()를 호출한다.
bool CSCTreeCtrl::load(CString file)
{
	/*
	FILE* fp = NULL;
	CTreeItem item(_T("top"));

	TCHAR	chLine[1000];
	CString sline;

	//encoding 방식을 읽어온다.
	int	text_encoding = get_text_encoding(sfile);
	if (text_encoding < 0)
		return false;

	//encoding 방식을 판별하여 read/write했으나
	//utf8만 다루도록 통일한다.

	fp = _tfopen(sfile, _T("rt")CHARSET);
	//m_unicode_file = true;
//}

	if (fp == NULL)
		return false;

	m_tr.clear();


	tree<CTreeItem>::iterator top = m_tr.set_head(item);
	tree<CTreeItem>::iterator it;
	int prev_indent, tabCount;

	while (_fgetts(chLine, 1000, fp) != NULL)
	{
		sline = chLine;
		sline.TrimRight();

		//빈 라인이 있을 경우는 패스
		if (sline.GetLength() == 0)
			continue;

		//맨 처음 노드이면 루트로 추가
		if (it == NULL)
		{
			item = CTreeItem(sline);
			it = m_tr.append_child(top, item);
			prev_indent = 0;
			continue;
		}

		//읽어온 문자열이 몇 탭 들어간 항목인지 세고
		tabCount = 0;
		while (sline[tabCount] == '\t')
			tabCount++;

		sline.Trim();
		//prev_indent와 비교하여 레벨에 맞게 추가시킨다.

		item = CTreeItem(sline);

		//전보다 1개 많으면 child이고
		if (tabCount == prev_indent + 1)
		{
			it = m_tr.append_child(it, item);
			prev_indent = tabCount;
		}
		//전과 같으면 sibling이고
		else if (tabCount == prev_indent)
		{
			it = m_tr.parent(it);
			it = m_tr.append_child(it, item);
		}
		//전보다 적으면 (prev_index-tabCount) * parent의 sibling이다.
		else if (tabCount < prev_indent)
		{
			while (prev_indent >= tabCount)
			{
				it = m_tr.parent(it);
				prev_indent--;
			}

			if (it == NULL)
			{
				it = m_tr.insert(m_tr.begin(), item);
			}
			else
			{
				it = m_tr.append_child(it, item);
			}
			prev_indent++;
		}
		else
		{
			AfxMessageBox(sline + _T("\ntext 파일 입력 오류"));
			break;
		}
	}

	int res = fclose(fp);
	recalculate_scroll_size();
	Invalidate();
	*/
	return true;
}

bool CSCTreeCtrl::save(CString file)
{
	return true;
}

bool CSCTreeCtrl::load_from_string(CString text)
{
	DeleteAllItems();

	text.Replace(_T("\r\n"), _T("\n"));
	text.Trim();

	std::deque<CString> lines;
	get_token_string(text, lines, '\n', false);

	int			img_index = -1;
	int			img_count = (m_imagelist.GetSafeHandle() ? m_imagelist.GetImageCount() : 0);
	int			tab_count = 0;
	int			prev_indent = 0;
	CString		label;
	HTREEITEM	hItem = TVI_ROOT;

	Trace(_T("\n"));

	for (int i = 0; i < lines.size(); i++)
	{
		label = lines[i];
		label.TrimRight();
		tab_count = get_char_count(label, '\t');

		label.TrimLeft();

		//Trace_only(make_string(_T("\t"), tab_count));
		//Trace_only(_T("%s\n"), label);

		if (tab_count < img_count)
			img_index = tab_count;
		else
			img_index = -1;

		if (tab_count == 0)
		{
			if (img_index >= 0)
				hItem = InsertItem(label, img_index, img_index, TVI_ROOT);
			else
				hItem = InsertItem(label, TVI_ROOT);
			Trace(_T("hItem = %p, label = %s\n"), hItem, label);

			prev_indent = 0;
			continue;
		}
		//전보다 1개 많으면 child이고
		if (tab_count == prev_indent + 1)
		{
			if (img_index >= 0)
				hItem = InsertItem(label, img_index, img_index, hItem);
			else
				hItem = InsertItem(label, hItem);
			Trace(_T("hItem = %p, label = %s\n"), hItem, label);

			prev_indent = tab_count;
		}
		//전과 같으면 sibling이고
		else if (tab_count == prev_indent)
		{
			hItem = GetParentItem(hItem);
			if (img_index >= 0)
				hItem = InsertItem(label, img_index, img_index, hItem);
			else
				hItem = InsertItem(label, hItem);
			Trace(_T("hItem = %p, label = %s\n"), hItem, label);
		}
		//전보다 적으면 (prev_index-tabCount) * parent의 sibling이다.
		else if (tab_count < prev_indent)
		{
			while (prev_indent >= tab_count)
			{
				hItem = GetParentItem(hItem);
				prev_indent--;
			}

			if (img_index >= 0)
				hItem = InsertItem(label, img_index, img_index, hItem);
			else
				hItem = InsertItem(label, hItem);
			Trace(_T("hItem = %p, label = %s\n"), hItem, label);

			prev_indent++;
		}
		else
		{
			AfxMessageBox(lines[i] + _T("\n문자열이 올바른 트리 구조 형식이 아닙니다."));
			break;
		}
	}

	return true;
}

void CSCTreeCtrl::create_imagelist()
{
	m_use_own_imagelist = true;

	if (m_image_size < 0)
		m_image_size = 16;

	//SetImageList(NULL, TVSIL_NORMAL);

	m_imagelist.DeleteImageList();
	m_imagelist.Create(m_image_size, m_image_size, ILC_COLOR32 | ILC_MASK, 0, 1);

	for (int i = 0; i < m_image_IDs.size(); i++)
		m_imagelist.Add(AfxGetApp()->LoadIcon(m_image_IDs[i]));

	SetImageList(&m_imagelist, TVSIL_NORMAL);
	
	int height = GetItemHeight();
	SetItemHeight(MAX(m_image_size, height));

	Invalidate();
}

void CSCTreeCtrl::set_use_own_imagelist(bool use, int image_size)
{
	m_use_own_imagelist = use;
	m_imagelist.DeleteImageList();

	if (m_use_own_imagelist)
	{
		m_image_size = image_size;
		create_imagelist();
	}

	Invalidate();
}

void CSCTreeCtrl::set_image_size(int image_size)
{
	set_use_own_imagelist(true, image_size);
}

// 아이템 데이터 이동

BOOL CSCTreeCtrl::move_tree_item(CTreeCtrl* pTree, HTREEITEM hSrcItem, HTREEITEM hDestItem)
{
	//src의 parent가 dst이면 스킵.
	if (GetParentItem(hSrcItem) == hDestItem)
	{
		TRACE(_T("move to parent node is meaningless. skip.\n"));
		return TRUE;
	}

	//ShellTreeCtrl이고 destItem이 아직 확장되기 전의 상태라면 확장시켜준 후 추가해야 한다.
	if (m_is_shell_treectrl)
	{
		if ((GetItemState(hDestItem, TVIF_STATE) & TVIS_EXPANDED) == false)
		{
			insert_folder(hDestItem, get_fullpath(hDestItem));
			Expand(hDestItem, TVE_EXPAND);
			//m_folder_list = iterate_tree_with_no_recursion();
		}
	}

	bool folder_operation = true;

	//ShellTreeCtrl이면 폴더를 move or copy가 성공한 후에 UI를 갱신시킨다.
	if (m_is_shell_treectrl)
	{
		CString srcPath = get_fullpath(hSrcItem);
		CString dstPath = get_fullpath(hDestItem);

		if (IsCtrlPressed())
		{
			TRACE(_T("copy node. %s to %s\n"), srcPath, dstPath);
			//folder_operation = CopyFile(srcPath, dstPath,);
		}
		else
		{
			TRACE(_T("move node. %s to %s\n"), srcPath, dstPath);
			//folder_operation = MoveFile(srcPath, dstPath,);
		}
	}


	// 이동할 아이템의 정보를 알아내자.
	TVITEM	TV;
	TCHAR	str[256];
	ZeroMemory(str, sizeof(str));

	TV.hItem = hSrcItem;
	TV.mask = TVIF_TEXT | TVIF_IMAGE | TVIF_SELECTEDIMAGE;
	TV.pszText = str;
	TV.cchTextMax = sizeof(str);
	GetItem(&TV);

	DWORD dwData = pTree->GetItemData(hSrcItem);

	// 아이템을 추가 하자.
	TVINSERTSTRUCT  TI;

	TI.hParent = hDestItem;
	TI.hInsertAfter = TVI_LAST;
	TI.item.mask = TVIF_TEXT | TVIF_IMAGE | TVIF_SELECTEDIMAGE;
	TI.item.iImage = TV.iImage;
	TI.item.iSelectedImage = TV.iSelectedImage;
	TI.item.pszText = TV.pszText;

	HTREEITEM hItem = pTree->InsertItem(&TI);

	pTree->SetItemData(hItem, dwData);

	// 현재 아이템에 자식 아이템이 있다면
	HTREEITEM hChildItem = pTree->GetChildItem(hSrcItem);

	if (hChildItem)
	{
		// 자식 아이템이 있다면 같이 이동하자.
		move_child_tree_item(pTree, hChildItem, hItem);
	}

	// 확장 여부를 알아서 똑같이 하자.
	TVITEM  item;

	item.mask = TVIF_HANDLE;
	item.hItem = hSrcItem;

	pTree->GetItem(&item);

	if (item.state & TVIS_EXPANDED)
	{
		pTree->Expand(hItem, TVE_EXPAND);
	}

	// 아이템을 선택하자.
	pTree->SelectItem(hItem);

	// 기존 아이템을 제거한다.
	pTree->DeleteItem(hSrcItem);

	//노드 이동후에는 dropItem의 children을 정렬시켜줘야 한다.
	pTree->SortChildren(hDestItem);

	return TRUE;
}

// 현재 트리의 모든 아이템 데이터 이동
BOOL CSCTreeCtrl::move_child_tree_item(CTreeCtrl* pTree, HTREEITEM hChildItem, HTREEITEM hDestItem)
{
	HTREEITEM hSrcItem = hChildItem;

	while (hSrcItem)
	{
		// 이동할 아이템의 정보를 알아내자.
		TVITEM	TV;
		TCHAR	str[256];

		ZeroMemory(str, sizeof(str));

		TV.hItem = hSrcItem;
		TV.mask = TVIF_TEXT | TVIF_IMAGE | TVIF_SELECTEDIMAGE;
		TV.pszText = str;
		TV.cchTextMax = sizeof(str);
		GetItem(&TV);

		DWORD dwData = pTree->GetItemData(hSrcItem);

		// 아이템을 추가 하자.
		TVINSERTSTRUCT  TI;

		TI.hParent = hDestItem;
		TI.hInsertAfter = TVI_LAST;
		TI.item.mask = TVIF_TEXT | TVIF_IMAGE | TVIF_SELECTEDIMAGE;
		TI.item.iImage = TV.iImage;
		TI.item.iSelectedImage = TV.iSelectedImage;
		TI.item.pszText = TV.pszText;

		HTREEITEM hItem = pTree->InsertItem(&TI);

		pTree->SetItemData(hItem, dwData);

		// 현재 아이템에 자식 아이템이 있다면
		HTREEITEM hChildItem = pTree->GetChildItem(hSrcItem);

		// pTree->GetNextItem(hSrcItem, TVGN_CHILD;
		if (hChildItem)
		{
			move_child_tree_item(pTree, hChildItem, hItem);
		}

		// 확장 여부를 알아서 똑같이 하자.
		TVITEM  item;

		item.mask = TVIF_HANDLE;
		item.hItem = hSrcItem;

		pTree->GetItem(&item);

		if (item.state & TVIS_EXPANDED)
		{
			pTree->Expand(hItem, TVE_EXPAND);
		}

		// 다음 아이템을 알아보자.
		hSrcItem = pTree->GetNextItem(hSrcItem, TVGN_NEXT);
	}

	// 기존 아이템을 제거한다.
	pTree->DeleteItem(hChildItem);

	return TRUE;
}

void CSCTreeCtrl::theme_init()
{
	if (!m_theme_initialized)
	{
		m_theme.Init(m_hWnd);
		m_theme_initialized = true;
	}
}

void CSCTreeCtrl::draw_checkbox(CDC* pDC, CRect r, int check_state)
{
	theme_init();

	if (m_theme.GetAppearance())
	{
		int	nState = 0;

		// Translate from the button state to the appropriate draw state flags
		switch (check_state)
		{
		case BST_CHECKED:
			nState = CBS_CHECKEDNORMAL;
			break;

		case BST_UNCHECKED:
			nState = CBS_UNCHECKEDNORMAL;
			break;

		default:
			nState = CBS_MIXEDNORMAL;
			break;
		}

		// Now do the actual drawing...
		m_theme.DrawThemeBackground(pDC->GetSafeHdc(), r, BP_CHECKBOX, nState);
	}
	else // No themes - just draw it conventionally
	{
		UINT nState = 0;

		// Translate from the button state to the appropriate draw state flags
		switch (check_state)
		{
		case BST_CHECKED:
			nState = DFCS_BUTTONCHECK | DFCS_CHECKED;
			break;

		case BST_UNCHECKED:
			nState = DFCS_BUTTONCHECK;
			break;

		default:
			nState = DFCS_BUTTON3STATE;
			break;
		}

		// Now do the actual drawing...
		pDC->DrawFrameControl(r, DFC_BUTTON, nState);
	}
}

void CSCTreeCtrl::use_checkbox(bool use)
{
	m_use_checkbox = use;

	ModifyStyle(TVS_CHECKBOXES, 0);

	if (use)
		ModifyStyle(0, TVS_CHECKBOXES);

	Invalidate();
}

void CSCTreeCtrl::use_expand_button(bool use)
{
	m_use_expand_button = use;

	ModifyStyle(TVS_HASBUTTONS, 0);

	if (use)
		ModifyStyle(0, TVS_HASBUTTONS);

	Invalidate();
}

BOOL CSCTreeCtrl::OnTvnBeginlabeledit(NMHDR* pNMHDR, LRESULT* pResult)
{
	LPNMTVDISPINFO pTVDispInfo = reinterpret_cast<LPNMTVDISPINFO>(pNMHDR);
	// TODO: 여기에 컨트롤 알림 처리기 코드를 추가합니다.
	*pResult = 0;

	TRACE(_T("%s\n"), __function__);

	return FALSE;
}


BOOL CSCTreeCtrl::OnTvnEndlabeledit(NMHDR* pNMHDR, LRESULT* pResult)
{
	LPNMTVDISPINFO pTVDispInfo = reinterpret_cast<LPNMTVDISPINFO>(pNMHDR);
	// TODO: 여기에 컨트롤 알림 처리기 코드를 추가합니다.
	*pResult = 0;

	TRACE(_T("%s\n"), __function__);

	return FALSE;
}


void CSCTreeCtrl::edit_item(HTREEITEM hItem)
{
	if (hItem == NULL)
	{
		hItem = GetSelectedItem();
		if (hItem == NULL)
			return;
	}

	EnsureVisible(hItem);

	m_edit_item = hItem;
	m_edit_old_text = GetItemText(hItem);
	m_last_clicked_item = NULL;
	m_last_clicked_time = 0;

	//편집 사각형의 right는 레이블 너비보다 좀 더 넉넉하게 준다. 단, rc.right를 넘어가서는 안된다.
	CRect rc;
	CRect r;
	//get_item_rect(hItem, r);
	GetItemRect(hItem, r, TRUE);


	//label영역은 실제 텍스트 너비보다 24픽셀 더 크게 잡아준다.(윈도우 탐색기와 동일)
	r.right += 24;

	//단, 그 오른쪽 끝이 rc.right를 넘어가지 않게 보정한다.
	GetClientRect(rc);
	if (r.right > rc.right - 1)
		r.right = rc.right - 1;

	r.DeflateRect(0, 0);

	if (m_pEdit == NULL)
	{
		m_pEdit = new CEdit;
		m_pEdit->Create(WS_CHILD | WS_BORDER | WS_VISIBLE | ES_AUTOHSCROLL | ES_MULTILINE, r, this, 1004);
	}
	else
	{
	}

	m_pEdit->MoveWindow(r);
	m_pEdit->SetFont(&m_font, true);

	//ES_MULTILINE을 준 이유는 editbox가 세로 중앙정렬되어 표시되지 않으므로
	//이를 보정해주기 위해서 CEdit::SetRect()를 사용하는데 반드시 ES_MULTILINE 속성이 있어야만 적용되기 때문이다.
	CRect new_rect;
	LOGFONT lf;

	m_pEdit->GetRect(new_rect);
	//new_rect.right += 4;
	new_rect.left = 2;
	m_pEdit->GetFont()->GetLogFont(&lf);
	//new_rect.top = new_rect.top + (new_rect.Height() + lf.lfHeight) / 2 - 1;
	m_pEdit->SetRect(&new_rect);

	m_pEdit->SetWindowText(m_edit_old_text);

	m_pEdit->ShowWindow(SW_SHOW);
	m_pEdit->SetSel(0, -1);
	m_pEdit->SetFocus();

	m_in_editing = true;

	TV_DISPINFO dispinfo;
	dispinfo.hdr.hwndFrom = m_hWnd;
	dispinfo.hdr.idFrom = GetDlgCtrlID();
	dispinfo.hdr.code = TVN_BEGINLABELEDIT;

	dispinfo.item.mask = LVIF_TEXT;
	dispinfo.item.hItem = hItem;
	//dispinfo.item.pszText = bEscape ? NULL : LPTSTR((LPCTSTR)Text);
	//dispinfo.item.cchTextMax = Text.GetLength();

	GetParent()->SendMessage(WM_NOTIFY, GetDlgCtrlID(), (LPARAM)&dispinfo);
}

void CSCTreeCtrl::edit_end(bool valid)
{
	if (m_in_editing == false || m_pEdit == NULL)
		return;

	//편집 후 Enter키 또는 리스트의 다른곳을 클릭하여 편집을 종료할 때
	//변경된 text를 변경할 지 아닐지는 mainDlg에게 맡겨야 한다.
	//여기서는 편집모드만 종료시켜준다.
	m_in_editing = false;
	
	m_pEdit->GetWindowText(m_edit_new_text);
	m_pEdit->ShowWindow(SW_HIDE);
	
	CRect r;
	GetItemRect(m_edit_item, r, FALSE);
	Invalidate();

	if (m_edit_old_text == m_edit_new_text)
		return;

	//실제 변경된 텍스트로 해당 아이템의 레이블을 변경하는 것은 main에 맞겨야 한다.
	if (valid)
	{
		TV_DISPINFO dispinfo;
		dispinfo.hdr.hwndFrom = m_hWnd;
		dispinfo.hdr.idFrom = GetDlgCtrlID();
		dispinfo.hdr.code = TVN_ENDLABELEDIT;

		dispinfo.item.mask = TVIF_TEXT;
		dispinfo.item.hItem = m_edit_item;
		//dispinfo.item.pszText = bEscape ? NULL : LPTSTR((LPCTSTR)Text);
		//dispinfo.item.cchTextMax = Text.GetLength();

		GetParent()->SendMessage(WM_NOTIFY, GetDlgCtrlID(), (LPARAM)&dispinfo);
	}


	Invalidate();
}

//custom draw일 경우는 직접 계산해야 한다?
void CSCTreeCtrl::get_item_rect(HTREEITEM hItem, CRect r[])
{
	CRect rc, temp, row;
	int label_width;

	GetClientRect(rc);
	GetItemRect(hItem, &row, FALSE);

	CFont font;
	CFont* pOldFont;

	CClientDC dc(this);
	font.CreateFontIndirect(&m_lf);
	pOldFont = (CFont*)dc.SelectObject(&font);
	CSize sz = dc.GetTextExtent(GetItemText(hItem));
	dc.SelectObject(pOldFont);

	temp = row;
	temp.right = rc.right - 1;
	r[rect_row] = temp;

	//indent 적용
	int total_indent = 0;
	HTREEITEM hParent = hItem;
	while (hParent)
	{
		total_indent += m_indent_size;
		hParent = GetParentItem(hParent);
	}

	temp.left = total_indent - m_indent_size;


	if (GetStyle() & TVS_HASBUTTONS)
	{
		r[rect_button] = temp;
		r[rect_button].right = r[rect_button].left + 16;
		temp.left += 16;
	}

	if (GetStyle() & TVS_CHECKBOXES)
	{
		r[rect_check] = temp;
		r[rect_check].right = r[rect_check].left + 16;
		temp.left += 16;
	}

	if (m_is_shell_treectrl || (m_imagelist.GetSafeHandle() && m_imagelist.GetImageCount()))
	{
		r[rect_icon] = temp;
		r[rect_icon].right = r[rect_icon].left + m_image_size;
		temp.left += m_image_size;
	}

	r[rect_label] = temp;
	r[rect_label].left += 4;
	r[rect_label].right = r[rect_label].left + sz.cx + 8;

	//영역 확인용 코드
	if (m_show_area)
	{
		Trace(_T("\nrow = %s\nbutton = %s\ncheck = %s\nicon = %s\nlabel = %s\n"),
			get_rect_info_string(r[rect_row]),
			get_rect_info_string(r[rect_button]),
			get_rect_info_string(r[rect_check]),
			get_rect_info_string(r[rect_icon]),
			get_rect_info_string(r[rect_label]));

		DrawRectangle(&dc, r[rect_button], red);
		DrawRectangle(&dc, r[rect_check], skyblue);
		DrawRectangle(&dc, r[rect_icon], orange);
		DrawRectangle(&dc, r[rect_label], cyan);
	}
}

void CSCTreeCtrl::set_log_font(LOGFONT lf)
{
	if (_tcslen(lf.lfFaceName) == 0)
		return;

	memcpy(&m_lf, &lf, sizeof(LOGFONT));
	reconstruct_font();
}

int CSCTreeCtrl::get_font_size()
{
	m_font_size = get_font_size_from_logical_size(m_hWnd, m_lf.lfHeight);
	return m_font_size;
}

//예를 들어 폰트 크기를 10으로 설정하면
void CSCTreeCtrl::set_font_size(int font_size)
{
	if (font_size == 0)
		return;

	m_font_size = font_size;
	//For the MM_TEXT mapping mode,
	//you can use the following formula to specify 
	//a height for a font with a specified point size:
	m_lf.lfHeight = get_logical_size_from_font_size(m_hWnd, m_font_size);
	reconstruct_font();
}

void CSCTreeCtrl::enlarge_font_size(bool enlarge)
{
	m_font_size = get_font_size();
	enlarge ? m_font_size++ : m_font_size--;
	m_lf.lfHeight = get_logical_size_from_font_size(m_hWnd, m_font_size);
	reconstruct_font();
}


void CSCTreeCtrl::OnTvnItemexpanded(NMHDR* pNMHDR, LRESULT* pResult)
{
	LPNMTREEVIEW pNMTreeView = reinterpret_cast<LPNMTREEVIEW>(pNMHDR);
	// TODO: 여기에 컨트롤 알림 처리기 코드를 추가합니다.
	/*
	UINT nFlags = GetItemState(pNMTreeView->itemNew.hItem, TVIS_EXPANDED);

	TRACE(_T("[%s] expanded = %d\n"), GetItemText(pNMTreeView->itemNew.hItem), nFlags & TVIS_EXPANDED);

	if (nFlags & TVIS_EXPANDED)
		Expand(pNMTreeView->itemNew.hItem, TVE_COLLAPSE);
	else// if (nFlags & TVIS_EXPANDED)
		Expand(pNMTreeView->itemNew.hItem, TVE_EXPAND);
	*/
	*pResult = 0;
}

void CSCTreeCtrl::full_row_selection(bool full_row)
{
	ModifyStyle(full_row ? 0 : TVS_FULLROWSELECT, full_row ? TVS_FULLROWSELECT : 0, 0);
	if (GetStyle() & TVS_FULLROWSELECT)
		TRACE(_T("full row\n"));
	else
		TRACE(_T("no full row\n"));
}

void CSCTreeCtrl::has_line(bool line)
{
	ModifyStyle(line ? 0 : TVS_HASLINES, line ? TVS_HASLINES : 0, 0);
}


void CSCTreeCtrl::OnNMCustomDraw(NMHDR* pNMHDR, LRESULT* pResult)
{
	LPNMCUSTOMDRAW pNMCD = reinterpret_cast<LPNMCUSTOMDRAW>(pNMHDR);
	// TODO: 여기에 컨트롤 알림 처리기 코드를 추가합니다.
	LPNMCUSTOMDRAW pNMCustomDraw = (LPNMCUSTOMDRAW)pNMHDR;
	LPNMTVCUSTOMDRAW pNMTVCustomDraw = (LPNMTVCUSTOMDRAW)pNMHDR;
	HTREEITEM hItem = (HTREEITEM)pNMCustomDraw->dwItemSpec;
	*pResult = CDRF_DODEFAULT;

	CDC dc;
	dc.Attach(pNMCustomDraw->hdc);

	CRect rtemp, rcItem;
	switch (pNMCustomDraw->dwDrawStage)
	{
		case CDDS_PREPAINT:
			*pResult = (CDRF_NOTIFYPOSTPAINT | CDRF_NOTIFYITEMDRAW);
			break;

		case CDDS_ITEMPREPAINT:
			rcItem = pNMCustomDraw->rc;
			GetItemRect(hItem, &rtemp, TRUE);
			if (rcItem.IsRectEmpty())
			{
				*pResult = CDRF_DODEFAULT;
				break;
			}

			*pResult = CDRF_NOTIFYPOSTPAINT;
			break;

		case CDDS_ITEMPOSTPAINT:
		{
			GetItemRect(hItem, &rcItem, TRUE);

			COLORREF crText = m_crText;
			COLORREF crBack = m_crBack;

			int nOldBkMode = dc.SetBkMode(TRANSPARENT);

			if (pNMCustomDraw->uItemState & CDIS_HOT)
			{
				TRACE(_T("CDIS_HOT\n"));
				crText = m_crTextSelected;
				crBack = m_crBackSelected;
			}
			//else if (pNMCustomDraw->uItemState & CDIS_DROPHILITED)	//이건 동작안한다.
			else if (hItem == GetDropHilightItem())// */pNMCustomDraw->uItemState & CDIS_DROPHILITED)
			{
				//drop을 위해 폴더위에 머무를 경우 해당 폴더가 expand가 아니면 expand시켜준다.
				SetTimer(timer_expand_for_drop, 1000, NULL);

				TRACE(_T("CDIS_DROPHILITED\n"));
				crText = m_crTextDropHilited;//VSLC_TREEVIEW_FOCUS_FONT_COLOR;
				crBack = m_crBackDropHilited;
			}
			else if (pNMCustomDraw->uItemState & CDIS_FOCUS)
			{
				TRACE(_T("CDIS_FOCUS\n"));
				crText = m_crTextSelected;
				crBack = m_crBackSelected;
			}
			else if (pNMCustomDraw->uItemState & CDIS_SELECTED)
			{
				TRACE(_T("CDIS_SELECTED\n"));
				crText = m_crTextSelected;
				crBack = m_crBackSelected;
			}

			dc.SetTextColor(crText);
			dc.FillSolidRect(&rcItem, crBack);

			/*
			switch (pIData->level)
			{
			case 0:
			case 1:
				hOldFont = (HFONT)dc.SelectObject(pMainWnd->m_fontBold);
				break;
			default:
				hOldFont = (HFONT)dc.SelectObject(pMainWnd->m_fontRegular);
				break;
			}
			*/

			//CRect rIcon = rcItem;
			////rIcon.left = rcItem.left - m_image_size;
			//rIcon.right = rIcon.left + m_image_size;// VSLC_TREEVIEW_ICON_WIDTH;
			//m_imagelist.Draw(&dc, 0, CPoint(rIcon.CenterPoint().x - m_image_size / 2, rIcon.CenterPoint().y - m_image_size / 2), ILD_TRANSPARENT);

			CRect rText = rcItem;
			//CSize szText = dc.GetTextExtent(GetItemText(hItem));
			//rText.OffsetRect(m_image_size, 0);
			//rText.right = rText.left + szText.cx;
			dc.SetBkMode(TRANSPARENT);
			dc.DrawText(GetItemText(hItem), &rText, DT_LEFT | DT_VCENTER | DT_SINGLELINE);

			//dc.DrawText(GetItemText(hItem), &rcItem, DT_LEFT | DT_VCENTER | DT_SINGLELINE);

			/*
			for (int j = 0; j < m_ImageInfo.GetCount(); j++)
			{
				if (pIData->eStStyle & m_ImageInfo.GetAt(m_ImageInfo.FindIndex(j))->eKind)
				{
					m_ImageList->Draw(&dc, j, rcIcon.TopLeft(), ILD_TRANSPARENT);
					rcIcon.left = rcIcon.right;
					rcIcon.right = rcIcon.right + VSLC_TREEVIEW_ICON_WIDTH;
				}
			}
			*/

			//dc.SelectObject(hOldFont);
			break;
		}
	}

	dc.Detach();

	//*pResult = 0;
}


void CSCTreeCtrl::OnTimer(UINT_PTR nIDEvent)
{
	// TODO: 여기에 메시지 처리기 코드를 추가 및/또는 기본값을 호출합니다.
	if (nIDEvent == timer_expand_for_drop)
	{
		KillTimer(timer_expand_for_drop);
		Expand(GetDropHilightItem(), TVE_EXPAND);
	}

	CTreeCtrl::OnTimer(nIDEvent);
}


void CSCTreeCtrl::OnNMRClick(NMHDR* pNMHDR, LRESULT* pResult)
{
	//context menu를 띠우기 위해 OnContextMenu()를 추가했으나
	//우클릭으로는 OnContextMenu()가 호출되지 않았고 더블우클릭을 해야 OnContextMenu()가 호출되는 현상이 있다.
	//검색해보니 NM_RCLICK에서도 아래와 같은 처리를 해줘야 한다.

	TRACE0("CMyTreeCtrl::OnRClick()\n");
	// Send WM_CONTEXTMENU to self
	SendMessage(WM_CONTEXTMENU, (WPARAM)m_hWnd, GetMessagePos());
	// Mark message as handled and suppress default handling
	*pResult = 1;
}

void CSCTreeCtrl::OnContextMenu(CWnd* pWnd, CPoint point)
{
	//우클릭되면 해당 아이템을 SELECTED로 수동 설정해야 한다.
	// if Shift-F10
	if (point.x == -1 && point.y == -1)
		point = (CPoint)GetMessagePos();

	UINT uFlags;
	HTREEITEM hItem;

	ScreenToClient(&point);
	hItem = HitTest(point, &uFlags);

	if (hItem == NULL)
		return;

	SelectItem(hItem);

	LANGID langID = GetSystemDefaultUILanguage();

	if (m_menu.m_hWnd == NULL)
	{
		m_menu.create(this, 160);
		m_menu.add(menu_add_item, (m_is_shell_treectrl ? _T("새 폴더(&N)") : _T("새 항목(&N)")));
		m_menu.add(menu_rename_item, (m_is_shell_treectrl ? _T("이름 바꾸기(&M)") : _T("이름 바꾸기(&M)")));
		m_menu.add(-1);
		m_menu.add(menu_delete_item, (m_is_shell_treectrl ? _T("삭제(&D)") : _T("삭제(&D)")));
	}

	ClientToScreen(&point);
	m_menu.popup_menu(point.x, point.y);
}


LRESULT CSCTreeCtrl::OnMessageCSCMenu(WPARAM wParam, LPARAM lParam)
{
	CSCMenuMessage* msg = (CSCMenuMessage*)wParam;

	if (msg->m_message == CSCMenu::message_scmenu_selchanged)
	{
		TRACE(_T("m_message = %d, m_menu_item->m_id = %d\n"), msg->m_message, msg->m_menu_item->m_id);
		switch (msg->m_menu_item->m_id)
		{
			case menu_add_item :
			{
				add_sub_item();
				break;
			}
			case menu_rename_item:
			{
				rename_item();
				break;
			}
			case menu_delete_item:
			{
				delete_item();
				break;
			}
		}
	}

	return 0;
}

//하위 항목을 추가한다. label이 ""이면 기본이름으로 추가한 후 edit_item() 호출.
//새 폴더, 새 항목이 이미 존재하면 뒤에 숫자를 증가시켜 붙여줘야 한다.
void CSCTreeCtrl::add_sub_item(HTREEITEM hParent, CString label)
{
	if (hParent == NULL)
		hParent = GetSelectedItem();
	if (hParent == NULL)
		return;

	HTREEITEM hItem;

	if (m_is_shell_treectrl)
	{
		TRACE(_T("parent = %s\n"), get_fullpath(hParent));

		//아직 펼쳐진 적이 없는 폴더라면 우선 서브 폴더들을 추가해준 후 새 폴더를 추가해야 한다.
		//if (GetChildItem(hParent) == NULL)
		//	insert_folder(hParent, get_fullpath(hParent));
		Expand(hParent, TVE_EXPAND);

		hItem = InsertItem(label.IsEmpty() ? _T("새 폴더") : label,
			m_pShellImageList->GetSystemImageListIcon(_T("C:\\windows")),
			m_pShellImageList->GetSystemImageListIcon(_T("C:\\windows")) + 1, hParent);

		CreateDirectory(get_fullpath(hItem), NULL);
	}
	else
	{
		hItem = InsertItem(label.IsEmpty() ? _T("새 항목") : label, hParent);
	}

	if (hItem)
		edit_item(hItem);
}

//주어진 항목의 label을 변경한다.
void CSCTreeCtrl::rename_item(HTREEITEM hItem, CString new_label)
{
	if (hItem == NULL)
		hItem = GetSelectedItem();
	if (hItem == NULL)
		return;

	if (new_label.IsEmpty())
	{
		//shell_tree_ctrl이라고 해도 여기서는 편집모드까지만 제공할 뿐
		//편집 완료 후의 처리는 main dlg에서 처리하는 것이 맞다.
		edit_item(hItem);
	}
	else
	{
		SetItemText(hItem, new_label);
	}
}

void CSCTreeCtrl::delete_item(HTREEITEM hItem, bool confirm)
{
	if (hItem == NULL)
		hItem = GetSelectedItem();
	if (hItem == NULL)
		return;

	if (confirm)
	{
		CString msg;

		msg.Format(_T("%s\n\n이 항목을 완전히 삭제하시겠습니까?"), GetItemText(hItem));
		int res = AfxMessageBox(msg, MB_YESNO);
		if (res == IDNO)
			return;
	}

	//shell_tree_ctrl일 경우는 SHDeleteFolder()를 이용해서 폴더 및 하위폴더까지 모두 삭제시켜야 하지만
	//위험한 동작이므로 우선 노드만 삭제한다.
	DeleteItem(hItem);
}


void CSCTreeCtrl::OnVScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar)
{
	// TODO: 여기에 메시지 처리기 코드를 추가 및/또는 기본값을 호출합니다.
	//기대 : 편집중에 스크롤 할 경우는 편집 취소처리한다.
	//결과 : 편집중일때는 여기로 오지 않는다. editbox가 가로채는듯하다.
	edit_end(false);

	CTreeCtrl::OnVScroll(nSBCode, nPos, pScrollBar);
}

//윈도우 탐색기의 폴더뷰를 보면 마우스 휠의 vscroll, hscroll을 모두 vscroll로 처리하고 있으며
//실제 가로 스크롤
void CSCTreeCtrl::OnMouseHWheel(UINT nFlags, short zDelta, CPoint pt)
{
	// 이 기능을 사용하려면 Windows Vista 이상이 있어야 합니다.
	// _WIN32_WINNT 기호는 0x0600보다 크거나 같아야 합니다.
	// TODO: 여기에 메시지 처리기 코드를 추가 및/또는 기본값을 호출합니다.
	edit_end(false);
	CTreeCtrl::OnMouseHWheel(nFlags, zDelta, pt);
}


BOOL CSCTreeCtrl::OnMouseWheel(UINT nFlags, short zDelta, CPoint pt)
{
	// TODO: 여기에 메시지 처리기 코드를 추가 및/또는 기본값을 호출합니다.
	edit_end(false);
	return CTreeCtrl::OnMouseWheel(nFlags, zDelta, pt);
}
