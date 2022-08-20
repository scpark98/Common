#include "stdafx.h"
#include "XShellTreeCtrl.h"
#include "XShellMan.h"

XShellTreeCtrl::XShellTreeCtrl()
{
	m_editing = FALSE;
	m_ignorePaint = FALSE;
}

XShellTreeCtrl::~XShellTreeCtrl()
{
}


BOOL XShellTreeCtrl::Attach(XShellMan* man, HWND hWndTree)
{
	if(__super::SubclassWindow(hWndTree)==FALSE)  { ASSERT(0); return FALSE;}

	m_man = man;

	InitTree();

	return TRUE;
}

void XShellTreeCtrl::InitTree()
{
	TCHAR szWinDir [MAX_PATH + 1];
	if (GetWindowsDirectory(szWinDir, MAX_PATH) > 0)
	{
		SHFILEINFO sfi;
		TreeView_SetImageList(m_hWnd, SHGetFileInfo(szWinDir, 0, &sfi, sizeof(SHFILEINFO), SHGFI_SYSICONINDEX | SHGFI_SMALLICON), 0);
	}
	Refresh();
}

void XShellTreeCtrl::Refresh()
{
	TreeView_DeleteAllItems(m_hWnd);
	TreeView_SetScrollTime(m_hWnd, 100);
}


////////////////////////////////////////////////////////////////////////////////////////////////////
///         ������ �߰�
/// @param  
/// @return 
/// @date   Monday, October 17, 2011  3:03:44 PM
////////////////////////////////////////////////////////////////////////////////////////////////////
HTREEITEM XShellTreeCtrl::InsertItem(HTREEITEM hParentItem, CShellItem* pItem)
{
	if(pItem->isTreeItem==FALSE) return 0;		// Ʈ���� �������� �ƴϸ� �н�.

	TV_ITEM tvItem;
	tvItem.mask = TVIF_PARAM | TVIF_TEXT | TVIF_IMAGE | TVIF_SELECTEDIMAGE | TVIF_CHILDREN;

	if(pItem->dispName.IsEmpty())
		pItem->dispName = pItem->pidlFQ.GetDispName();

	tvItem.lParam = (LPARAM)(pItem);
	tvItem.pszText = pItem->dispName.GetBuffer();
	tvItem.iImage = OnGetItemIcon(pItem, FALSE);
	tvItem.iSelectedImage = OnGetItemIcon(pItem, TRUE);
	tvItem.cChildren = pItem->hasChildren;

	TV_INSERTSTRUCT tvInsert;
	tvInsert.item = tvItem;
	tvInsert.hInsertAfter = TVI_LAST;
	tvInsert.hParent = hParentItem;

	// ������ �߰�
	HTREEITEM hItem = TreeView_InsertItem(m_hWnd, &tvInsert);
	return hItem;
}

BOOL XShellTreeCtrl::RemoveItem(HTREEITEM hTreeItem)
{
	return TreeView_DeleteItem(m_hWnd, hTreeItem);
}

int XShellTreeCtrl::OnGetItemIcon(CShellItem* pItem, BOOL bSelected)
{
	if(pItem==NULL){ASSERT(0); return -1;}

	SHFILEINFO sfi;

	UINT uiFlags = SHGFI_PIDL | SHGFI_SYSICONINDEX | SHGFI_SMALLICON;

	if (bSelected)
	{
		uiFlags |= SHGFI_OPENICON;
	}
	else
	{
		uiFlags |= SHGFI_LINKOVERLAY;
	}

	if (SHGetFileInfo((LPCTSTR)pItem->pidlFQ.GetPIDL(), 0, &sfi, sizeof(sfi), uiFlags))
	{
		return sfi.iIcon;
	}

	return -1;
}



////////////////////////////////////////////////////////////////////////////////////////////////////
///         PARENT ���� ������ WM_NOTIFY (OCM_NOTIFY) ó���ϱ�
///			parent ������ ������ ���� REFLECT_NOTIFICATIONS() �� �־�� �Ѵ�.
/* 
			BEGIN_MSG_MAP(CMainDlg)
				// ...
				REFLECT_NOTIFICATIONS()		// ���ϵ� ��Ʈ���� �޽��� ���÷��� ó��
				// ...
			END_MSG_MAP()
*/
/// @param  
/// @return 
/// @date   Wednesday, October 19, 2011  11:34:17 AM
////////////////////////////////////////////////////////////////////////////////////////////////////
LRESULT	XShellTreeCtrl::OnNotify(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
	NMHDR	*pNotify = (NMHDR*)lParam;
	if(pNotify->code==TVN_ITEMEXPANDING)
	{
		OnItemexpanding(pNotify);
	}
	else if(pNotify->code==TVN_SELCHANGED)
	{
		NMTREEVIEW* pNmTree = (NMTREEVIEW*)pNotify;
		m_man->OnTreeSelChanged(GetItemData(pNmTree->itemNew.hItem));
	}
	else if(pNotify->code==TVN_BEGINLABELEDIT)
	{
		if(m_editing)
			return FALSE;		// ����

		HTREEITEM hTreeItem;
		if(IsEditable(hTreeItem)==FALSE)
			return TRUE;				// edit ����

		m_editing = TRUE;
		return FALSE;
	}
	else if(pNotify->code==TVN_ENDLABELEDIT)
	{
		NMTVDISPINFO* ptvdi = (NMTVDISPINFO*)lParam;
		BOOL ret = EndLabelEdit(ptvdi);
		m_editing = FALSE;
		return ret;
	}
	else
		bHandled = FALSE;

	return 0;
}

void XShellTreeCtrl::OnItemexpanding(NMHDR* pNMHDR)
{
	NM_TREEVIEW* pNMTreeView = (NM_TREEVIEW*)pNMHDR;

	HTREEITEM hItem = pNMTreeView->itemNew.hItem;
	if(hItem ==NULL){ASSERT(0); return;}

	switch (pNMTreeView->action)
	{
	case TVE_EXPAND:
		m_man->OnTreeExpand(hItem);		// parent ���� ó��
		break;

	case TVE_COLLAPSE:
		break;
	}

}

void XShellTreeCtrl::Expand(HTREEITEM hItem)
{
	TreeView_Expand(m_hWnd, hItem, TVE_EXPAND);
}


////////////////////////////////////////////////////////////////////////////////////////////////////
///         '+' ��� ���ֱ�
/// @param  
/// @return 
/// @date   Wednesday, October 19, 2011  10:51:41 AM
////////////////////////////////////////////////////////////////////////////////////////////////////
void XShellTreeCtrl::RemovePlusMark(HTREEITEM hItem, BOOL remove)
{
	TV_ITEM tvItem;
	ZeroMemory(&tvItem, sizeof(tvItem));

	tvItem.hItem = hItem;
	tvItem.mask = TVIF_CHILDREN;

	// �߰�!
	if(remove==FALSE)
		tvItem.cChildren = 1;

	TreeView_SetItem(m_hWnd, &tvItem);
}


void XShellTreeCtrl::SortChildren(HTREEITEM hItem)
{
	// Sort the new items:
	TV_SORTCB tvSort;

	tvSort.hParent = hItem;
	tvSort.lpfnCompare = CompareProc;
	tvSort.lParam = 0;

	m_man->m_bIgnoreNotify = TRUE;
	TreeView_SortChildrenCB(m_hWnd, &tvSort, 0);
	m_man->m_bIgnoreNotify = FALSE;
}

int CALLBACK XShellTreeCtrl::CompareProc(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort)
{
	UNREFERENCED_PARAMETER(lParamSort);

	CShellItem* pItem1 = (CShellItem*)lParam1;
	CShellItem* pItem2 = (CShellItem*)lParam2;

	HRESULT hr = pItem1->pParentFolder->CompareIDs(0, pItem1->pidlRel, pItem2->pidlRel);

	if (FAILED(hr))
	{
		return 0;
	}

	return(short)SCODE_CODE(GetScode(hr));
}

CShellItem* XShellTreeCtrl::GetItemData(HTREEITEM hItem) const
{
	TVITEM item;
	item.hItem = hItem;
	item.mask = TVIF_PARAM;
	::SendMessage(m_hWnd, TVM_GETITEM, 0, (LPARAM)&item);
	return (CShellItem*)item.lParam;
}


BOOL XShellTreeCtrl::SelectItem(HTREEITEM hTreeItem)
{
	return TreeView_SelectItem(m_hWnd, hTreeItem);
}

LRESULT XShellTreeCtrl::OnKeyDown(UINT /*uMsg*/, WPARAM wParam, LPARAM /*lParam*/, BOOL& bHandled)
{
	if(wParam==VK_F2)
	{
		HTREEITEM hTreeItem;
		if(IsEditable(hTreeItem))
		{
			m_editing = TRUE;
			TreeView_EditLabel(m_hWnd, hTreeItem);
		}
		return 0;
	}
	else
		bHandled = FALSE;

	return 0;
}


////////////////////////////////////////////////////////////////////////////////////////////////////
///         ���� ���õ� �������� ���� ������ �������ΰ�?
/// @param  
/// @return 
/// @date   Monday, October 24, 2011  2:56:55 PM
////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL XShellTreeCtrl::IsEditable(HTREEITEM& hTreeItem)
{
	// ������ ���� �Ұ���
	if(m_man->m_opt.enableTreeControlRename==FALSE)
		return FALSE;

	hTreeItem = TreeView_GetSelection(m_hWnd);
	if(hTreeItem==NULL){ASSERT(0); return FALSE;}

	CShellItem* pItem = GetItemData(hTreeItem);
	if(pItem==NULL){ASSERT(0); return FALSE;}


	// ���� ������ �������ΰ�?
	if(pItem->isEditable)
		return TRUE;

	return FALSE;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
///         ���� �Ϸ� ó��
/// @param  
/// @return 
/// @date   Monday, October 24, 2011  3:09:13 PM
////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL XShellTreeCtrl::EndLabelEdit(NMTVDISPINFO* pdi)
{
	CString newName = pdi->item.pszText;

	CShellItem* pItem = GetItemData(pdi->item.hItem);
	if(pItem==NULL) {ASSERT(0); return FALSE;}

	LPITEMIDLIST newPidl = NULL;

	HRESULT hr = pItem->pParentFolder->SetNameOf(m_hWnd, pItem->pidlRel, newName, SHGDN_NORMAL, &newPidl);
	if(FAILED(hr))
		return FALSE;

	// shell notify �����ϵ���..
	if(m_man->m_list)
		m_man->m_list->ResetShellNotifyTick();


	// ������.. newpidl �� �ٲ۴�.
	pItem->ChangePidl(newPidl);
	pItem->dispName = pItem->pidlFQ.GetDispName();

	// ������ �ٲ� �̸� �ٽ� �ݿ�
	pdi->item.pszText = pItem->dispName.GetBuffer();

	// current path �� �ٲ����..
	m_man->GetCurrentPathFromCurPidl();

	return TRUE;
}

HWND XShellTreeCtrl::GetEditControl()
{
	return (HWND)::SendMessage(m_hWnd, TVM_GETEDITCONTROL, 0, 0);
}

LRESULT XShellTreeCtrl::OnPaint(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled)
{
	if(m_ignorePaint)
		return 0;
	bHandled = FALSE;
	return 0;
}

void XShellTreeCtrl::SetText(HTREEITEM hTree, LPCTSTR text)
{
	TV_ITEM tvItem;
	ZeroMemory(&tvItem, sizeof(tvItem));

	tvItem.hItem = hTree;
	tvItem.mask = TVIF_TEXT;
	tvItem.pszText = (LPTSTR)text;

	TreeView_SetItem(m_hWnd, &tvItem);
}

