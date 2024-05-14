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
}


BEGIN_MESSAGE_MAP(CSCTreeCtrl, CTreeCtrl)
	ON_WM_ACTIVATE()
	ON_WM_ERASEBKGND()
	ON_WM_KEYDOWN()
	ON_WM_PAINT()
	ON_NOTIFY_REFLECT(TVN_SELCHANGED, &CSCTreeCtrl::OnTvnSelchanged)
	ON_WM_WINDOWPOSCHANGED()
	ON_NOTIFY_REFLECT(TVN_ITEMEXPANDING, &CSCTreeCtrl::OnTvnItemexpanding)
	ON_NOTIFY_REFLECT_EX(NM_CLICK, &CSCTreeCtrl::OnNMClick)
	ON_NOTIFY_REFLECT(TVN_BEGINDRAG, &CSCTreeCtrl::OnTvnBegindrag)
	ON_NOTIFY_REFLECT(NM_DBLCLK, &CSCTreeCtrl::OnNMDblclk)
	ON_WM_MOUSEMOVE()
	ON_WM_LBUTTONUP()
	ON_WM_HSCROLL()
END_MESSAGE_MAP()



// CSCTreeCtrl 메시지 처리기
void CSCTreeCtrl::PreSubclassWindow()
{
	// TODO: 여기에 특수화된 코드를 추가 및/또는 기본 클래스를 호출합니다.
	SetItemHeight(22);

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

	ASSERT(bCreated);
}

BOOL CSCTreeCtrl::PreTranslateMessage(MSG* pMsg)
{
	// TODO: 여기에 특수화된 코드를 추가 및/또는 기본 클래스를 호출합니다.
	if (pMsg->message == WM_KEYDOWN)
	{
		switch (pMsg->wParam)
		{
			case VK_DELETE :
			{
				CString label = get_selected_item_text(false);
				if (AfxMessageBox(label + _T("\ndelete this node?"), MB_YESNO) == IDNO)
					return TRUE;
				DeleteItem(GetSelectedItem());
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
	return FALSE;
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
void CSCTreeCtrl::OnPaint()
{
	//CTreeCtrl::OnPaint();
	//return;

	CPaintDC dc1(this); // device context for painting
					   // TODO: 여기에 메시지 처리기 코드를 추가합니다.
					   // 그리기 메시지에 대해서는 CTreeCtrl::OnPaint()을(를) 호출하지 마십시오.
	CRect rc;
	GetClientRect(rc);

	CMemoryDC	dc(&dc1, &rc);
	Graphics	g(dc.m_hDC, rc);

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
	CRect	rRow, rItem;
	CString label;

	if (!m_is_shell_treectrl && m_use_own_imagelist)
	{
		if (m_imagelist.GetSafeHandle())
			image_count = m_imagelist.GetImageCount();
	}

	while (hItem)
	{
		GetItemRect(hItem, rRow, FALSE);

		//배경색을 그려주고
		if (GetItemState(hItem, TVIS_SELECTED) & TVIS_SELECTED)
		{
			dc.SetTextColor(red);

			//포커스에 따라 다르다.
			if (GetFocus() == this)
			{
				crText = m_crTextSelected;
				DrawRectangle(&dc, rRow, m_crSelectedBorder, m_crBackSelected);
			}
			else if (GetStyle() & TVS_SHOWSELALWAYS)
			{
				crText = m_crTextSelectedInactive;
				dc.FillSolidRect(rRow, m_crBackSelectedInactive);
			}
		}
		else
		{
			dc.SetTextColor(m_crText);
		}

		//label영역의 left는 margin, button, checkbox, icon 유무에 관계없이 동일하게 리턴된다.
		//Lines at Root에 대해서만 자동 반영된다. false일 경우는 2, true일 경우는 22.
		GetItemRect(hItem, rItem, TRUE);
		rItem.right = rc.right - 1;

		label = GetItemText(hItem);
		//Trace(_T("%s, rItem.left = %d\n"), label, rItem.left);

		//확장버튼을 그려주고
		if (GetStyle() & TVS_HASBUTTONS)
		{
			//SetItemHeight() 또는 m_image_size가 16이 아니어도 확장버튼의 크기는 동일해야 한다.
			CRect expand_rect = makeCenterRect(rItem.left + 16 / 2, rItem.CenterPoint().y, 16, 16);
			expand_rect.DeflateRect(4, 4, 3, 3);

			if (ItemHasChildren(hItem))
			{
				if (GetItemState(hItem, TVIF_STATE) & TVIS_EXPANDED)
				{
					DrawRectangle(&dc, expand_rect, GRAY(128));
					expand_rect.DeflateRect(3, 3);
					DrawLine(&dc, expand_rect.left - 1, expand_rect.CenterPoint().y, expand_rect.right, expand_rect.CenterPoint().y, GRAY(128));
				}
				else
				{
					DrawRectangle(&dc, expand_rect, GRAY(128));//, GRAY(128));
					expand_rect.DeflateRect(3, 3);
					DrawLine(&dc, expand_rect.left - 1, expand_rect.CenterPoint().y, expand_rect.right, expand_rect.CenterPoint().y, GRAY(128));
					DrawLine(&dc, expand_rect.CenterPoint().x, expand_rect.top - 1, expand_rect.CenterPoint().x, expand_rect.bottom, GRAY(128));
				}
			}

			//child 유무와 관계없이 너비만큼 left를 이동시켜준다.
			rItem.left += 16;
		}

		//체크박스를 그려주고
		if (GetStyle() & TVS_CHECKBOXES)
		{
			//체크박스도 그 크기는 16으로 동일하다.
			CRect rCheck = makeCenterRect(rItem.left + 16 / 2, rItem.CenterPoint().y, 16, 16);
			rCheck.DeflateRect(2, 2, 2, 2);

#if 1
			draw_checkbox(&dc, rCheck, GetCheck(hItem));
#else
			DrawRectangle(&dc, rCheck, GRAY128, white);

			/*
			dc.SetTextColor(GRAY64);
			dc.DrawText(GetCheck(hItem) ? _T("√") : _T(""), rCheck, DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_NOCLIP);
			*/

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
			rItem.left += 16;
		}

		//아이콘을 그려주고
		if (m_use_own_imagelist && m_imagelist.GetSafeHandle() && (image_index < m_imagelist.GetImageCount()))
		{
			GetItemImage(hItem, image_index, image_selected_index);
			if (image_index >= 0 && image_index < image_count)
			{
				m_imagelist.Draw(&dc, image_index, CPoint(rItem.left, rItem.CenterPoint().y - m_image_size / 2), ILD_TRANSPARENT);
				rItem.left += m_image_size;
			}
		}
		else if (m_is_shell_treectrl)
		{
			GetItemImage(hItem, image_index, image_selected_index);
			image_index = m_pShellImageList->GetSystemImageListIcon(get_fullpath(GetSelectedItem()), true);
			m_pShellImageList->m_imagelist_small.Draw(&dc, image_index,
				CPoint(rItem.left, rItem.CenterPoint().y - m_image_size / 2), ILD_TRANSPARENT);
		}

		//레이블을 그려준다.
		rItem.left += 4;	//여백을 두고 label을 그려준다.
		dc.SetTextColor(crText);
		dc.DrawText(label, rItem, DT_LEFT | DT_SINGLELINE | DT_VCENTER);

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


void CSCTreeCtrl::OnTvnSelchanged(NMHDR* pNMHDR, LRESULT* pResult)
{
	LPNMTREEVIEW pNMTreeView = reinterpret_cast<LPNMTREEVIEW>(pNMHDR);
	// TODO: 여기에 컨트롤 알림 처리기 코드를 추가합니다.
	HWND hWnd = GetParent()->GetSafeHwnd();
	::SendMessage(GetParent()->GetSafeHwnd(), Message_CSCTreeCtrl,
				(WPARAM)&CSCTreeCtrlMessage(this, message_selchanged),
				(LPARAM)&CString(get_fullpath(GetSelectedItem())));

	*pResult = 0;
}


void CSCTreeCtrl::OnWindowPosChanged(WINDOWPOS* lpwndpos)
{
	CTreeCtrl::OnWindowPosChanged(lpwndpos);

	// TODO: 여기에 메시지 처리기 코드를 추가합니다.
}

void CSCTreeCtrl::set_as_shell_treectrl(CShellImageList* pShellImageList, bool is_local)
{
	DeleteAllItems();
	m_folder_list.clear();

	m_is_shell_treectrl = true;
	m_is_shell_treectrl_local = is_local;
	m_use_own_imagelist = true;
	m_pShellImageList = pShellImageList;

	SetImageList(&m_pShellImageList->m_imagelist_small, TVSIL_NORMAL);
	m_desktopItem = insert_special_folder(CSIDL_DESKTOP);
	m_documentItem = insert_special_folder(CSIDL_MYDOCUMENTS);
	m_computerItem = insert_special_folder(CSIDL_DRIVES);

	for (std::map<TCHAR, CString>::iterator it = m_pShellImageList->get_drive_map()->begin(); it != m_pShellImageList->get_drive_map()->end(); it++)
		insert_drive(it->second);

	//std::thread t(&CSCTreeCtrl::thread_insert_folders, this, GetRootItem());
	//t.detach();

	m_folder_list = iterate_tree_with_no_recursion();

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
	//tvInsert.item.cChildren = TRUE;
	tvInsert.hInsertAfter = TVI_LAST;
	tvInsert.hParent = m_computerItem;
	tvInsert.item.pszText = (LPTSTR)(LPCTSTR)driveName;
	HTREEITEM hItem = InsertItem(&tvInsert);

	insert_folder(hItem, real_path);
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
			TV_INSERTSTRUCT tvInsert;
			tvInsert.item.mask = TVIF_TEXT | TVIF_IMAGE | TVIF_SELECTEDIMAGE | TVIF_CHILDREN;
			tvInsert.item.iImage = m_pShellImageList->GetSystemImageListIcon(_T("C:\\windows"));
			tvInsert.item.iSelectedImage = m_pShellImageList->GetSystemImageListIcon(_T("C:\\windows")) + 1;
			//tvInsert.item.cChildren = TRUE;
			tvInsert.hInsertAfter = TVI_LAST;
			tvInsert.hParent = hParent;
			tvInsert.item.pszText = (LPTSTR)(LPCTSTR)curFolder;
			HTREEITEM hItem = InsertItem(&tvInsert);
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

void CSCTreeCtrl::OnTvnItemexpanding(NMHDR* pNMHDR, LRESULT* pResult)
{
	LPNMTREEVIEW pNMTreeView = reinterpret_cast<LPNMTREEVIEW>(pNMHDR);
	// TODO: 여기에 컨트롤 알림 처리기 코드를 추가합니다.
	*pResult = 0;
}

//check, has button등에 의해 HitTest가 알아서 해당 영역을 알려주리라 기대했으나
//hit 영역을 정확히 리턴해주지 않는다.
//button, check, text 영역을 내가 정해서 그려주고 이벤트도 수동으로 처리해주는 것이 나을듯하다.
BOOL CSCTreeCtrl::OnNMClick(NMHDR* pNMHDR, LRESULT* pResult)
{
#if 0
	CPoint pt;
	UINT nFlags;

	GetCursorPos(&pt);
	ScreenToClient(&pt);

	HTREEITEM hItem = HitTest(pt, &nFlags);

	TRACE(_T("nFlags = %d\n"), nFlags);
	return TRUE;
#else
	// TODO: 여기에 컨트롤 알림 처리기 코드를 추가합니다.
	UINT nFlags = 0;
	CString str;
	CString fullpath;

	HTREEITEM hItem = hit_test(&nFlags);

	fullpath = get_fullpath(hItem);

	SelectItem(hItem);

	//확장버튼을 누르면 (확장 단추 표시 속성이 TRUE일 경우에만 처리됨)
	if (nFlags & TVHT_ONITEMBUTTON)
	{
		//TRACE(_T("button\n"));
		nFlags = GetItemState(hItem, TVIF_STATE);
		//TRACE(_T("expanded = %d\n"), nFlags & TVIS_EXPANDED);

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
			Expand(hItem, TVE_TOGGLE);
		}
	}
	else if (nFlags & TVHT_ONITEMSTATEICON)
	{
		BOOL checked = GetCheck(hItem);
		SetCheck(hItem, !checked);
	}

	return TRUE;

	*pResult = 0;

#endif
}

CString CSCTreeCtrl::get_fullpath(HTREEITEM hItem)
{
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
			m_folder_list = iterate_tree_with_no_recursion();
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

HTREEITEM CSCTreeCtrl::hit_test(UINT* nFlags)
{
	CPoint pt;

	GetCursorPos(&pt);
	ScreenToClient(&pt);

	HTREEITEM hItem = HitTest(pt, nFlags);

	CRect rc, r;
	int label_width;

	GetClientRect(rc);
	GetItemRect(hItem, &r, TRUE);
	label_width = r.Width();

	if (pt.x < r.left)
	{
		*nFlags = TVHT_ONITEMINDENT;
		TRACE(_T("hItem = %p, nFlags = %d (%s)\n"), hItem, *nFlags, ENUM_TO_CSTRING(TVHT_ONITEMINDENT));
		return hItem;
	}

	r.right = rc.right - 1;

	CRect button, check, icon;

	if (GetStyle() & TVS_HASBUTTONS)
	{
		button = r;
		button.right = button.left + 16;
		r.left += 16;
	}

	if (GetStyle() & TVS_CHECKBOXES)
	{
		check = r;
		check.right = check.left + 16;
		r.left += 16;
	}

	if (m_is_shell_treectrl || m_imagelist.GetImageCount())
	{
		icon = r;
		icon.right = icon.left + m_image_size;
		r.left += m_image_size;
	}

	if (button.PtInRect(pt))
	{
		*nFlags = TVHT_ONITEMBUTTON;
		TRACE(_T("hItem = %p, nFlags = %s\n"), hItem, ENUM_TO_CSTRING(TVHT_ONITEMBUTTON));
	}
	else if (check.PtInRect(pt))
	{
		*nFlags = TVHT_ONITEMSTATEICON;
		TRACE(_T("hItem = %p, nFlags = %s\n"), hItem, ENUM_TO_CSTRING(TVHT_ONITEMSTATEICON));
	}
	else if (icon.PtInRect(pt))
	{
		*nFlags = TVHT_ONITEMICON;
		TRACE(_T("hItem = %p, nFlags = %s\n"), hItem, ENUM_TO_CSTRING(TVHT_ONITEMICON));
	}
	else if (pt.x > r.left && pt.x <= r.left + label_width)
	{
		*nFlags = TVHT_ONITEMLABEL;
		TRACE(_T("hItem = %p, nFlags = %s\n"), hItem, ENUM_TO_CSTRING(TVHT_ONITEMLABEL));
	}
	else
	{
		*nFlags = TVHT_ONITEMRIGHT;
		TRACE(_T("hItem = %p, nFlags = %s\n"), hItem, ENUM_TO_CSTRING(TVHT_ONITEMRIGHT));
	}

	return hItem;
}

void CSCTreeCtrl::set_color_theme(int theme, bool apply_now)
{
	switch (theme)
	{
		//최근 윈도우 탐색기의 색상을 보면 텍스트 색상은 선택여부, inactive에 무관하게 동일하다.
	case color_theme_default:
		m_crText = ::GetSysColor(COLOR_BTNTEXT);
		m_crTextSelected = m_crText;// ::GetSysColor(COLOR_HIGHLIGHTTEXT);
		m_crTextSelectedInactive = m_crText;// ::GetSysColor(COLOR_INACTIVECAPTIONTEXT);
		m_crTextDropHilited = m_crText;
		m_crBack = ::GetSysColor(COLOR_WINDOW);
		m_crBackSelected = RGB(204, 232, 255);// ::GetSysColor(COLOR_HIGHLIGHT);
		m_crBackSelectedInactive = RGB(217, 217, 217);// ::GetSysColor(COLOR_HIGHLIGHT);
		m_crBackDropHilited = m_crBackSelected;
		m_crSelectedBorder = RGB(153, 209, 255);
		break;
	case color_theme_light_blue:
		m_crText = ::GetSysColor(COLOR_BTNTEXT);
		m_crTextSelected = RGB(65, 102, 146);
		m_crTextSelectedInactive = RGB(65, 102, 146);
		m_crTextDropHilited = m_crText;
		m_crBack = RGB(193, 219, 252);
		m_crBackSelected = get_color(m_crBack, -48);
		m_crBackSelectedInactive = get_color(m_crBack, -48);
		m_crBackDropHilited = m_crBackSelected;
		m_crSelectedBorder = RGB(153, 209, 255);
		break;
	case color_theme_navy_blue:
		m_crText = RGB(204, 216, 225);
		m_crTextSelected = RGB(234, 246, 255);
		m_crTextSelectedInactive = RGB(105, 142, 186);
		m_crBack = RGB(74, 94, 127);
		m_crBackSelected = RGB(15, 36, 41);
		m_crBackSelectedInactive = RGB(15, 36, 41);
		m_crSelectedBorder = RGB(153, 209, 255);
		break;
	case color_theme_dark_blue:
		m_crText = RGB(16, 177, 224);
		m_crTextSelected = RGB(224, 180, 59);
		m_crTextSelectedInactive = RGB(105, 142, 186);
		m_crBack = RGB(2, 21, 36);
		m_crBackSelected = RGB(3, 42, 59);
		m_crBackSelectedInactive = RGB(15, 36, 41);
		m_crSelectedBorder = RGB(153, 209, 255);
		break;
	case color_theme_dark_gray:
		m_crText = RGB(164, 164, 164);
		m_crTextSelected = RGB(241, 241, 241);
		m_crTextSelectedInactive = get_color(m_crTextSelected, -36);
		m_crBack = RGB(64, 64, 64);
		m_crBackSelected = get_color(m_crBack, -32);
		m_crBackSelectedInactive = get_color(m_crBack, -32);
		m_crSelectedBorder = RGB(128, 128, 128);
		break;
	case color_theme_dark:
		m_crText = RGB(212, 212, 212);
		m_crTextSelected = RGB(241, 241, 241);
		m_crTextSelectedInactive = get_color(m_crTextSelected, -36);
		m_crBack = RGB(37, 37, 38);
		m_crBackSelected = get_color(m_crBack, -32);
		m_crBackSelectedInactive = get_color(m_crBack, -32);
		m_crSelectedBorder = m_crBackSelected;// RGB(128, 128, 128);
		break;
	}

	if (apply_now)
		Invalidate();
}


void CSCTreeCtrl::OnNMDblclk(NMHDR* pNMHDR, LRESULT* pResult)
{
	// TODO: 여기에 컨트롤 알림 처리기 코드를 추가합니다.
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
	

	int item_count = get_sub_folders(path, NULL, false, true);
	CGdiplusBitmap bmpRes(64, 64, PixelFormat32bppARGB, Gdiplus::Color(128, 255, 0, 0));

	if (m_drag_images_id.size() == 1)
		bmpRes.load(m_drag_images_id[0]);
	else if (m_drag_images_id.size() > 1)
		bmpRes.load(item_count == 1 ? m_drag_images_id[0] : m_drag_images_id[1]);

	bmpRes.draw_text(bmpRes.width / 2 + 10, bmpRes.height / 2, i2S(item_count), 20, 2,
		_T("맑은 고딕"), Gdiplus::Color(192, 0, 0, 0), Gdiplus::Color(192, 255, 128, 128), DT_CENTER | DT_VCENTER);

	m_pDragImage = new CImageList();
	m_pDragImage->Create(bmpRes.width, bmpRes.height, ILC_COLOR32, 1, 1);

	HICON hicon;
	bmpRes.m_pBitmap->GetHICON(&hicon);
	m_pDragImage->Add(hicon);

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
	int			img_count = m_imagelist.GetImageCount();
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
	m_imagelist.DeleteImageList();
	m_imagelist.Create(m_image_size, m_image_size, ILC_COLOR32 | ILC_MASK, 0, 1);

	for (int i = 0; i < m_image_IDs.size(); i++)
		m_imagelist.Add(AfxGetApp()->LoadIcon(m_image_IDs[i]));

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

	SetItemHeight(MAX(16 + 2, m_image_size + 2));
	Invalidate();
}

void CSCTreeCtrl::set_image_size(int image_size)
{
	set_use_own_imagelist(true, image_size);
}

// 아이템 데이터 이동

BOOL CSCTreeCtrl::MoveTreeItem(CTreeCtrl* pTree, HTREEITEM hSrcItem, HTREEITEM hDestItem)
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

	if (hChildItem)
	{
		// 자식 아이템이 있다면 같이 이동하자.
		MoveChildTreeItem(pTree, hChildItem, hItem);
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

	return TRUE;
}

// 현재 트리의 모든 아이템 데이터 이동
BOOL CSCTreeCtrl::MoveChildTreeItem(CTreeCtrl* pTree, HTREEITEM hChildItem, HTREEITEM hDestItem)
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
			MoveChildTreeItem(pTree, hChildItem, hItem);
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

void CSCTreeCtrl::set_use_checkbox(bool checkbox)
{
	ModifyStyle(TVS_CHECKBOXES, 0);

	if (checkbox)
		ModifyStyle(0, TVS_CHECKBOXES);

	Invalidate();
}
