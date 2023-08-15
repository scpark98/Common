// CTreeCtrlEx.cpp: 구현 파일
//

#include "TreeCtrlEx.h"
#include "../Functions.h"
#include "../MemoryDC.h"

// CTreeCtrlEx

IMPLEMENT_DYNAMIC(CTreeCtrlEx, CTreeCtrl)

CTreeCtrlEx::CTreeCtrlEx()
{
	memset(&m_lf, 0, sizeof(LOGFONT));
}

CTreeCtrlEx::~CTreeCtrlEx()
{
}


BEGIN_MESSAGE_MAP(CTreeCtrlEx, CTreeCtrl)
	ON_WM_ACTIVATE()
	ON_WM_ERASEBKGND()
	ON_WM_KEYDOWN()
	ON_WM_PAINT()
	ON_NOTIFY_REFLECT(TVN_SELCHANGED, &CTreeCtrlEx::OnTvnSelchanged)
	ON_WM_WINDOWPOSCHANGED()
	ON_NOTIFY_REFLECT(TVN_ITEMEXPANDING, &CTreeCtrlEx::OnTvnItemexpanding)
	ON_NOTIFY_REFLECT(NM_CLICK, &CTreeCtrlEx::OnNMClick)
	ON_NOTIFY_REFLECT(TVN_BEGINDRAG, &CTreeCtrlEx::OnTvnBegindrag)
END_MESSAGE_MAP()



// CTreeCtrlEx 메시지 처리기
void CTreeCtrlEx::PreSubclassWindow()
{
	// TODO: 여기에 특수화된 코드를 추가 및/또는 기본 클래스를 호출합니다.
	SetItemHeight(20);

	CFont* font = GetParent()->GetFont();

	if (font != NULL)
		font->GetObject(sizeof(m_lf), &m_lf);
	else
		GetObject(GetStockObject(SYSTEM_FONT), sizeof(m_lf), &m_lf);

	reconstruct_font();

	CTreeCtrl::PreSubclassWindow();
}

void CTreeCtrlEx::reconstruct_font()
{
	m_font.DeleteObject();
	BOOL bCreated = m_font.CreateFontIndirect(&m_lf);
	SetFont(&m_font, true);

	m_font_size = get_font_size_from_logical_size(m_hWnd, m_lf.lfHeight);

	ASSERT(bCreated);
}

BOOL CTreeCtrlEx::PreTranslateMessage(MSG* pMsg)
{
	// TODO: 여기에 특수화된 코드를 추가 및/또는 기본 클래스를 호출합니다.

	return CTreeCtrl::PreTranslateMessage(pMsg);
}


void CTreeCtrlEx::OnActivate(UINT nState, CWnd* pWndOther, BOOL bMinimized)
{
	CTreeCtrl::OnActivate(nState, pWndOther, bMinimized);

	// TODO: 여기에 메시지 처리기 코드를 추가합니다.
}


BOOL CTreeCtrlEx::OnEraseBkgnd(CDC* pDC)
{
	// TODO: 여기에 메시지 처리기 코드를 추가 및/또는 기본값을 호출합니다.
	return FALSE;
	return CTreeCtrl::OnEraseBkgnd(pDC);
}


void CTreeCtrlEx::OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags)
{
	// TODO: 여기에 메시지 처리기 코드를 추가 및/또는 기본값을 호출합니다.

	CTreeCtrl::OnKeyDown(nChar, nRepCnt, nFlags);
}

//수동으로 직접 그려줄 경우는 BEGIN_MESSAGE_MAP에서 ON_WM_PAINT() 주석을 해제하면
//OnPaint()가 호출되고 여기서 직접 그릴 수 있다.
//폰트 색상 등의 UI를 자유롭게 변경할 수 있으나 스크롤 처리 등이 복잡해진다.
//GetItemRect()를 이용해서 그려주니 스크롤도 자동 처리된다.
void CTreeCtrlEx::OnPaint()
{
	CPaintDC dc1(this); // device context for painting
					   // TODO: 여기에 메시지 처리기 코드를 추가합니다.
					   // 그리기 메시지에 대해서는 CTreeCtrl::OnPaint()을(를) 호출하지 마십시오.
	CRect rc;
	GetClientRect(rc);

	CMemoryDC dc(&dc1, &rc);

	//dc.FillSolidRect(rc, pink);

	CFont font;
	CFont* pOldFont;

	font.CreateFontIndirect(&m_lf);
	pOldFont = (CFont*)dc.SelectObject(&font);


	int indent;
	int icon_index;
	UINT flags = 0;
	CRect r, rRow;
	long tTotal = 0;

	for (int i = 0; i < m_folder_list.size(); i++)
	{
		GetItemRect(m_folder_list[i].item, &r, true);
		r.right = rc.right;

		//rc와 일부분도 겹치지 않는 아이템은 그리지 않는다.
		if (getIntersectionRect(rc, r).IsRectEmpty())
			continue;

		long t0 = getClock();

		indent = get_char_count(m_folder_list[i].fullpath, '\\');
		
		if (indent > 0)
		{
			if (m_folder_list[i].fullpath.GetLength() > 3)
				indent++;
		}

		if (m_folder_list[i].item == m_selectedItem)
		{
			trace(_T("selected\n"));
			GetItemRect(m_folder_list[i].item, &rRow, false);
			dc.FillSolidRect(rRow, pink);
		}

		//dc.FillSolidRect(r, pink);
		
		dc.DrawText(m_folder_list[i].folder, r, DT_LEFT | DT_VCENTER | DT_SINGLELINE);

		//텍스트 왼쪽에 아이콘을 그려주고
		icon_index = m_pShellImageList->GetSystemImageListIcon(m_folder_list[i].fullpath, true);
		m_pShellImageList->m_imagelist_small.Draw(&dc, icon_index,
						CPoint(r.left - 16, r.CenterPoint().y - 8), ILD_TRANSPARENT);

		//그 왼쪽에는 확장/축소 버튼을 그려준다.
		if (get_sub_folders(m_folder_list[i].fullpath))
		//if (ItemHasChildren(m_folder_list[i].item))
		{
			flags = GetItemState(m_folder_list[i].item, TVIF_STATE);
			CRect rButton = r;
			rButton.left = rButton.left - 16 - 16;

			if (flags & TVIS_EXPANDED)
			{
				dc.DrawText(_T("^"), rButton, DT_LEFT | DT_VCENTER | DT_SINGLELINE);
			}
			else
			{
				dc.DrawText(_T(">"), rButton, DT_LEFT | DT_VCENTER | DT_SINGLELINE);
			}
		}

		long t1 = getClock();
		tTotal += (t1 - t0);
		//trace(_T("%ld : %s\n"), t1 - t0, m_folder_list[i].folder);
	}

	dc.SelectObject(pOldFont);

	trace(_T("total %ld\n"), tTotal);
}


void CTreeCtrlEx::OnTvnSelchanged(NMHDR* pNMHDR, LRESULT* pResult)
{
	LPNMTREEVIEW pNMTreeView = reinterpret_cast<LPNMTREEVIEW>(pNMHDR);
	// TODO: 여기에 컨트롤 알림 처리기 코드를 추가합니다.
	HWND hWnd = GetParent()->GetSafeHwnd();
	::SendMessage(GetParent()->GetSafeHwnd(), MESSAGE_TREECTRLEX,
				(WPARAM)&CTreeCtrlExMessage(this, message_selchanged),
				(LPARAM)&CString(get_fullpath(GetSelectedItem())));

	m_selectedItem = GetSelectedItem();

	*pResult = 0;
}


void CTreeCtrlEx::OnWindowPosChanged(WINDOWPOS* lpwndpos)
{
	CTreeCtrl::OnWindowPosChanged(lpwndpos);

	// TODO: 여기에 메시지 처리기 코드를 추가합니다.
}

void CTreeCtrlEx::set_as_shell_treectrl(bool is_local /*= true*/)
{
	m_is_shell_treectrl = true;
	m_is_shell_treectrl_local = is_local;
	m_use_own_imagelist = true;

	SetImageList(&m_pShellImageList->m_imagelist_small, TVSIL_NORMAL);
	m_desktopItem = insert_special_folder(CSIDL_DESKTOP);
	m_documentItem = insert_special_folder(CSIDL_MYDOCUMENTS);
	m_computerItem = insert_special_folder(CSIDL_DRIVES);

	for (std::map<TCHAR, CString>::iterator it = m_pShellImageList->get_drive_map()->begin(); it != m_pShellImageList->get_drive_map()->end(); it++)
		insert_drive(it->second);

	m_folder_list = iterate_tree_with_no_recursion();

	expand_all();
}

HTREEITEM CTreeCtrlEx::insert_special_folder(int csidl)
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
		tvInsert.item.pszText = (LPWSTR)(LPCWSTR)text;
		return InsertItem(&tvInsert);
	}
}

void CTreeCtrlEx::insert_drive(CString driveName)
{
	CString real_path = convert_special_folder_to_real_path(driveName);

	TV_INSERTSTRUCT tvInsert;
	tvInsert.item.mask = TVIF_TEXT | TVIF_IMAGE | TVIF_SELECTEDIMAGE | TVIF_CHILDREN;
	tvInsert.item.iImage = m_pShellImageList->GetSystemImageListIcon(real_path);
	tvInsert.item.iSelectedImage = m_pShellImageList->GetSystemImageListIcon(real_path);
	//tvInsert.item.cChildren = TRUE;
	tvInsert.hInsertAfter = TVI_LAST;
	tvInsert.hParent = m_computerItem;
	tvInsert.item.pszText = (LPWSTR)(LPCWSTR)driveName;
	HTREEITEM hItem = InsertItem(&tvInsert);

	insert_folder(hItem, real_path);
}

void CTreeCtrlEx::insert_folder(HTREEITEM hParent, CString sParentPath)
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
			tvInsert.item.pszText = (LPWSTR)(LPCWSTR)curFolder;
			HTREEITEM hItem = InsertItem(&tvInsert);
		}
	}
}

void CTreeCtrlEx::insert_folder(WIN32_FIND_DATA* pFindFileData)
{

}

HTREEITEM CTreeCtrlEx::find_item(const CString& name)
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

HTREEITEM CTreeCtrlEx::find_item(const CString& name, HTREEITEM root)
{
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


void CTreeCtrlEx::OnTvnItemexpanding(NMHDR* pNMHDR, LRESULT* pResult)
{
	LPNMTREEVIEW pNMTreeView = reinterpret_cast<LPNMTREEVIEW>(pNMHDR);
	// TODO: 여기에 컨트롤 알림 처리기 코드를 추가합니다.
	*pResult = 0;
}


void CTreeCtrlEx::OnNMClick(NMHDR* pNMHDR, LRESULT* pResult)
{
	// TODO: 여기에 컨트롤 알림 처리기 코드를 추가합니다.
	CPoint pt;
	UINT nFlags = 0;

	GetCursorPos(&pt);
	ScreenToClient(&pt);

	HTREEITEM hItem = HitTest(pt, &nFlags);
	TRACE(_T("%s, %d, %d, %d\n"), get_fullpath(hItem), pt.x, pt.y, nFlags);

	//확장버튼을 누르면
	if (nFlags & TVHT_ONITEMBUTTON)
	{
		TRACE(_T("button\n"));
		nFlags = GetItemState(hItem, TVIF_STATE);
		TRACE(_T("expanded = %d\n"), nFlags & TVIS_EXPANDED);

		//if (nFlags & TVIS_EXPANDED)
		{
			//만약 child가 없다면 아직 로딩되지 않은 노드이므로 검색해서 추가한다.
			//물론 실제 child가 없는 폴더일수도 있다.
			if (GetChildItem(hItem) == NULL)
				insert_folder(hItem, get_fullpath(hItem));
		}

		m_folder_list = iterate_tree_with_no_recursion();
	}


	*pResult = 0;
}

CString CTreeCtrlEx::get_fullpath(HTREEITEM hItem)
{
	CString fullpath;
	CString folder;

	while (hItem)
	{
		folder = GetItemText(hItem);
		fullpath = folder + _T("\\") + fullpath;

		if (folder.Right(2) == _T(":)"))
		{
			break;
		}

		hItem = GetParentItem(hItem);
	}

	return convert_special_folder_to_real_path(fullpath, m_pShellImageList->get_csidl_map());
}

void CTreeCtrlEx::select_item(CString fullpath)
{
	fullpath = m_pShellImageList->get_shell_known_string_by_csidl(CSIDL_DRIVES) + _T("\\") + convert_real_path_to_special_folder(fullpath);

	std::deque<CString> dq;
	get_token_string(fullpath, dq, '\\');

	HTREEITEM item = NULL;

	for (int i = 0; i < dq.size(); i++)
	{
		trace(_T("finding [%s] from [%s] node...\n"), dq[i], (item ? GetItemText(item) : _T("root")));

		//만약 현재 노드에 아직 child가 추가된 상태가 아니라면 우선 children을 넣어준 후 검색해야 한다.
		if (GetChildItem(item) == NULL)
			insert_folder(item, get_fullpath(item));

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

void CTreeCtrlEx::OnTvnBegindrag(NMHDR* pNMHDR, LRESULT* pResult)
{
	LPNMTREEVIEW pNMTreeView = reinterpret_cast<LPNMTREEVIEW>(pNMHDR);
	// TODO: 여기에 컨트롤 알림 처리기 코드를 추가합니다.
	*pResult = 0;
}

void CTreeCtrlEx::iterate_tree(HTREEITEM hItem)
{
	if (hItem)
	{
		trace(_T("%s\n"), GetItemText(hItem));
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

//recursion을 사용하지 않고 모든 node를 검색한다.
//stack을 이용하므로 그 차례가 실제 UI와 다르므로 deque에 넣은 후 sort를 이용한다.
//탐색기의 트리일 경우에 특화된 코드가 있으므로 범용으로 사용하려면 좀 더 보완이 필요함.
std::deque<CTreeCtrlFolder> CTreeCtrlEx::iterate_tree_with_no_recursion(HTREEITEM hItem)
{
	std::deque<HTREEITEM> s;
	std::deque<CTreeCtrlFolder> folders;
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
			folders.push_back(CTreeCtrlFolder(item, fullpath, m_pShellImageList->get_shell_known_string_by_csidl(CSIDL_DRIVES)));
		else
			folders.push_back(CTreeCtrlFolder(item, fullpath, GetFileNameFromFullPath(fullpath)));
		item = GetNextSiblingItem(item);
	}

	//바탕 화면, 문서, 내 PC 3개 항목을 제외하고 정렬.
	//기본 정렬과 탐색기의 정렬은 약간 다르므로 탐색기와 같은 정렬이 되도록.
	std::sort(folders.begin() +3, folders.end(),
		[](CTreeCtrlFolder a, CTreeCtrlFolder b)
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

void CTreeCtrlEx::expand_all(bool expand)
{
	HTREEITEM hItem;
	HTREEITEM hCurrent;

	hItem = GetFirstVisibleItem();

	while (hItem != NULL)
	{
		Expand(hItem, expand ? TVE_EXPAND : TVE_COLLAPSE);
		hItem = GetNextItem(hItem, TVGN_NEXTVISIBLE);
	}
}