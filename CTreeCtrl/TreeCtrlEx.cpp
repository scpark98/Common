// CTreeCtrlEx.cpp: 구현 파일
//

#include "TreeCtrlEx.h"
#include "../Functions.h"

// CTreeCtrlEx

IMPLEMENT_DYNAMIC(CTreeCtrlEx, CTreeCtrl)

CTreeCtrlEx::CTreeCtrlEx()
{

}

CTreeCtrlEx::~CTreeCtrlEx()
{
}


BEGIN_MESSAGE_MAP(CTreeCtrlEx, CTreeCtrl)
	ON_WM_ACTIVATE()
	ON_WM_ERASEBKGND()
	ON_WM_KEYDOWN()
	//ON_WM_PAINT()
	ON_NOTIFY_REFLECT(TVN_SELCHANGED, &CTreeCtrlEx::OnTvnSelchanged)
	ON_WM_WINDOWPOSCHANGED()
	ON_NOTIFY_REFLECT(TVN_ITEMEXPANDING, &CTreeCtrlEx::OnTvnItemexpanding)
	ON_NOTIFY_REFLECT(NM_CLICK, &CTreeCtrlEx::OnNMClick)
END_MESSAGE_MAP()



// CTreeCtrlEx 메시지 처리기
void CTreeCtrlEx::PreSubclassWindow()
{
	// TODO: 여기에 특수화된 코드를 추가 및/또는 기본 클래스를 호출합니다.
	SetItemHeight(20);
	CTreeCtrl::PreSubclassWindow();
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

	return CTreeCtrl::OnEraseBkgnd(pDC);
}


void CTreeCtrlEx::OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags)
{
	// TODO: 여기에 메시지 처리기 코드를 추가 및/또는 기본값을 호출합니다.

	CTreeCtrl::OnKeyDown(nChar, nRepCnt, nFlags);
}

/*
void CTreeCtrlEx::OnPaint()
{
	CPaintDC dc(this); // device context for painting
					   // TODO: 여기에 메시지 처리기 코드를 추가합니다.
					   // 그리기 메시지에 대해서는 CTreeCtrl::OnPaint()을(를) 호출하지 마십시오.
}
*/

void CTreeCtrlEx::OnTvnSelchanged(NMHDR* pNMHDR, LRESULT* pResult)
{
	LPNMTREEVIEW pNMTreeView = reinterpret_cast<LPNMTREEVIEW>(pNMHDR);
	// TODO: 여기에 컨트롤 알림 처리기 코드를 추가합니다.
	HWND hWnd = GetParent()->GetSafeHwnd();
	::SendMessage(GetParent()->GetSafeHwnd(), MESSAGE_TREECTRLEX,
				(WPARAM)&CTreeCtrlExMessage(this, message_selchanged),
				(LPARAM)&CString(get_fullpath(GetSelectedItem())));

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
	TV_INSERTSTRUCT tvInsert;
	tvInsert.item.mask = TVIF_TEXT | TVIF_IMAGE | TVIF_SELECTEDIMAGE | TVIF_CHILDREN;
	tvInsert.item.iImage = m_pShellImageList->GetSystemImageListIcon(driveName);
	tvInsert.item.iSelectedImage = m_pShellImageList->GetSystemImageListIcon(driveName);
	//tvInsert.item.cChildren = TRUE;
	tvInsert.hInsertAfter = TVI_LAST;
	tvInsert.hParent = m_computerItem;
	tvInsert.item.pszText = (LPWSTR)(LPCWSTR)driveName;
	HTREEITEM hItem = InsertItem(&tvInsert);

	insert_folder(hItem, driveName);
}

void CTreeCtrlEx::insert_folder(HTREEITEM hParent, CString sParentPath)
{
	CFileFind	FileFind;
	bool		bWorking;
	CString		curFolder;

	sParentPath = convert_special_folder_to_real_path(sParentPath);

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

	return convert_special_folder_to_real_path(fullpath);
}

void CTreeCtrlEx::add_children_folders(HTREEITEM hParent)
{

}