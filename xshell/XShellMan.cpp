#include "stdafx.h"
#include "XShellMan.h"


DWORD WM_XSHELL_NOTIFY_PATHCHANGED  = ::RegisterWindowMessage(_T("WM_XSHELL_NOTIFY_PATHCHANGED"));
DWORD WM_XSHELL_NOTIFY_SHELLCOMMAND = ::RegisterWindowMessage(_T("WM_XSHELL_NOTIFY_SHELLCOMMAND"));
DWORD WM_XSHELL_NOTIFY_LISTSELCHANGED = ::RegisterWindowMessage(_T("WM_XSHELL_NOTIFY_LISTSELCHANGED"));

static BOOL IsFileOrFolder(LPCTSTR szPathName)
{
	DWORD dwRet;
	dwRet = GetFileAttributes(szPathName);
	if(dwRet==0xffffffff) return FALSE;					// ���� �߻�
	return TRUE;
}


XShellMan::XShellMan()
{
	m_created = FALSE;
	m_malloc = NULL;

	m_root = NULL;
	m_currentItem = NULL;
	m_tree = NULL;
	m_list = NULL;

	m_pidlRecycleBin = NULL;
	//m_pidlControlPanel = NULL;

	m_hWndParent = NULL;
	m_bIgnoreNotify = FALSE;
	m_ulSHChangeNotifyRegister = 0;
}

XShellMan::~XShellMan()
{
	Destroy();

	if(m_pidlRecycleBin)
		::CoTaskMemFree(m_pidlRecycleBin);
	//if(m_pidlControlPanel)
	//	::CoTaskMemFree(m_pidlControlPanel);
}


////////////////////////////////////////////////////////////////////////////////////////////////////
///         �ʱ�ȭ
/// @param  
/// @return 
/// @date   Friday, October 14, 2011  12:16:01 PM
////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL XShellMan::Create(HWND hWndParent, HWND hWndTreeCtrl, HWND hWndListCtrl)
{
	Destroy();

	if(FAILED(SHGetMalloc(&m_malloc))) {ASSERT(0); return FALSE;}


	m_hWndParent = hWndParent;

	// ��Ʈ�� attach
	if(hWndTreeCtrl)
	{
		m_tree = new XShellTreeCtrl;
		if(m_tree->Attach(this, hWndTreeCtrl)==FALSE)
		{ASSERT(0); return FALSE;}
	}

	if(hWndListCtrl)
	{
		m_list = new XShellListCtrl;

		if(m_list->Attach(this, hWndListCtrl)==FALSE)
		{ASSERT(0); return FALSE;}
	}

	
	if(CreateRoot()==FALSE) return FALSE;

	m_created = TRUE;


	return TRUE;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
///         ����
/// @param  
/// @return 
/// @date   Friday, October 14, 2011  12:30:03 PM
////////////////////////////////////////////////////////////////////////////////////////////////////
void XShellMan::Destroy()
{
	if(m_malloc)
	{
		m_malloc->Release();
		m_malloc = NULL;
	}

	if(m_root)
	{
		delete m_root;
		m_root = NULL;
	}
	m_currentItem = NULL;

	if(m_tree)
	{
		m_tree->Detach();
		delete m_tree;
		m_tree = NULL;
	}

	if(m_list)
	{
		m_list->Detach();
		delete m_list;
		m_list = NULL;
	}


	m_hWndParent = NULL;
	m_created = FALSE;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
///         ���� ��Ʈ ������ ����
/// @param  
/// @return 
/// @date   Friday, October 14, 2011  2:24:11 PM
////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL XShellMan::CreateRoot()
{
	if(m_root) { ASSERT(0); return FALSE;}		// �߻� �Ұ�

	m_root = new CShellItem();

	// ����ȭ�� ���ϱ�
	if(m_root->pidlRel.GetSpecialFolderLocation(CSIDL_DESKTOP)==FALSE)
	{ASSERT(0); return FALSE;}
	m_root->pidlFQ = m_root->pidlRel;
	m_root->hasChildren = TRUE;
	m_root->isTreeItem = TRUE;

	// �߰� Pidl ���ϱ�
	if(m_pidlRecycleBin==NULL)	::SHGetSpecialFolderLocation(NULL, CSIDL_BITBUCKET, &m_pidlRecycleBin);
	if(m_pidlControlPanel.IsEmpty())
		m_pidlControlPanel.CreateFromFromPath(_T("::{26EE0668-A00A-44D7-9371-BEB064C98683}"));	// http://social.msdn.microsoft.com/Forums/en-US/csharpgeneral/thread/04c90891-641a-424d-b4a6-7208ba98d2d0/
		//::SHGetSpecialFolderLocation(NULL, CSIDL_CONTROLS, &m_pidlControlPanel);	// ���� �۵����� ����

	// ���� ������
	m_currentItem = m_root;


	// Ʈ�� ��Ʈ���� ������?
	if(m_tree)
	{
		m_root->hTree = m_tree->InsertItem(TVI_ROOT, m_root);

		// Ʈ�� Ȯ��
		m_tree->Expand(m_root->hTree);
	}

	return TRUE;
}



BOOL XShellMan::ReloadCurrentChildItemAndUpdateTree()
{
	if(m_currentItem==NULL) return FALSE;
	return ReloadChildItemAndUpdateTree(m_currentItem);
}


////////////////////////////////////////////////////////////////////////////////////////////////////
///			IShellFolder ���ϱ�. ���ϵ� ���� Release() �� ����� �Ѵ�.
/// @param  
/// @return 
/// @date   Tuesday, October 25, 2011  4:36:06 PM
////////////////////////////////////////////////////////////////////////////////////////////////////
LPSHELLFOLDER XShellMan::GetFolder(LPSHELLFOLDER pParentFolder, LPITEMIDLIST pidl)
{
	LPSHELLFOLDER ret=NULL;
	HRESULT hr;
	if (pParentFolder==NULL)
	{
		// ��Ʈ �������̴�. �׳� desktop �� ���ϱ�
		hr = SHGetDesktopFolder(&ret);
	}
	else
	{
		hr = pParentFolder->BindToObject(pidl, NULL, IID_IShellFolder, (LPVOID*) &ret);
	}
	if(FAILED(hr)) {ASSERT(0); return NULL;}
	return ret;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
///         �ش� �������� ���ϵ� �������� �������� �Ѵ�. ���� ó���̶�� ���ϵ� ����� ���� �����ϴ°Ű�.
/// @param  
/// @return 
/// @date   Monday, October 17, 2011  5:02:42 PM
////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL XShellMan::ReloadChildItemAndUpdateTree(CShellItem* pParentItem)
{
	if(pParentItem==NULL){ASSERT(0); return FALSE;}

	LPSHELLFOLDER pParentFolder = NULL;

	// �θ� ������ IShellFolder ���ϱ�
	pParentFolder = GetFolder(pParentItem->pParentFolder, pParentItem->pidlRel);


	// nonfolder �� ��� ����
	pParentItem->ClearNonFolder();

	// enum �ؼ� ��Ͽ� ���� �ִ´�.
	BOOL ret = TRUE;
	ShellItemList	newListFolder;
	if(EnumObjects(newListFolder, pParentItem->childrenNonFolder, pParentItem, pParentFolder, pParentItem->pidlFQ))
	{
		// ������ ����� ������ ���� ��ϰ� �񱳸� �Ѵ�.
		UpdateTreeItemList(pParentItem, pParentFolder, newListFolder);

		// Ʈ�� ��Ʈ�ϱ�
		if(m_tree)
			m_tree->SortChildren(pParentItem->hTree);
	}
	else
		ret = FALSE;

	if(pParentFolder)
		pParentFolder->Release();

	return ret;
}


////////////////////////////////////////////////////////////////////////////////////////////////////
///         ���ϵ� ���丮(?)�� enum
/// @param  
/// @return 
/// @date   Monday, October 17, 2011  5:10:41 PM
////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL XShellMan::EnumObjects(ShellItemList& listFolder, ShellItemList& listNonFolder, CShellItem* parent, LPSHELLFOLDER pParentFolder, XPidl& pidlParent)
{
	//Debug(L"@@rXShellMan::EnumObjects");

	LPENUMIDLIST pEnum;

	LPITEMIDLIST pidlTemp;
	DWORD dwFetched ;
	SHDESCRIPTIONID desc;
	HRESULT hr;
	
	// ���� enum 
	hr = pParentFolder->EnumObjects(NULL, SHCONTF_FOLDERS, &pEnum);
	if(FAILED(hr)) return FALSE;										// ���� ������ ����� ��� ���..

	if (pEnum)
	{
		dwFetched = 1;
		// Enumerate the item's PIDLs:
		while (SUCCEEDED(pEnum->Next(1, &pidlTemp, &dwFetched)) && dwFetched)
		{
			dwFetched = 0;

			// zip(cab) �������� Ȯ���ϱ�
			DWORD dwAttribs = SFGAO_STREAM | SFGAO_REMOVABLE;		// ������ �Ӽ� ����ũ
			pParentFolder->GetAttributesOf(1, (LPCITEMIDLIST*) &pidlTemp, &dwAttribs);

			// ���� ����(zip, cab) �����̴�
			if(m_opt.treatCompressedFolderAsFile && (dwAttribs & SFGAO_STREAM))
			{
				if(m_list==NULL) continue;		// �׳� skip.

				// ���������� "���� ���"�� �߰��Ѵ�.
				DWORD dwAttribs = SFGAO_HASSUBFOLDER | SFGAO_FOLDER | SFGAO_DISPLAYATTRMASK | SFGAO_CANRENAME;
				pParentFolder->GetAttributesOf(1, (LPCITEMIDLIST*) &pidlTemp, &dwAttribs);

				// ��Ͽ� �߰�
				AddToList(listNonFolder, parent, pParentFolder, pidlParent, pidlTemp, 
					dwAttribs, FALSE);
				continue;
			}

			// removable disk ? (floppy, cdrom..)
			if(m_opt.hideFloppy && (dwAttribs & SFGAO_REMOVABLE))
			{
				if(SUCCEEDED(SHGetDataFromIDList(pParentFolder, pidlTemp, SHGDFIL_DESCRIPTIONID, (void*)&desc, sizeof(desc))))
				{
					if(	desc.dwDescriptionId==SHDID_COMPUTER_DRIVE35 || desc.dwDescriptionId==SHDID_COMPUTER_DRIVE525)
						continue;										// �÷��� �̴�!!
				}
			}

			// ������?
			if(m_opt.hideRecycleBin && m_pidlRecycleBin && ::ILIsEqual(pidlTemp, m_pidlRecycleBin))
				continue;

			// ������? 
			if(m_opt.hideControlPanel && m_pidlControlPanel.IsEmpty()==FALSE && ::ILIsEqual(pidlTemp, m_pidlControlPanel))
				continue;

			// �Ϲ� ������ ��� �� ������ �Ӽ� ��������
			dwAttribs = SFGAO_HASSUBFOLDER | SFGAO_FOLDER | SFGAO_DISPLAYATTRMASK | SFGAO_CANRENAME;
			pParentFolder->GetAttributesOf(1, (LPCITEMIDLIST*) &pidlTemp, &dwAttribs);
			/*
			Debug(L"@@r dwAttribs: 0x%x  pidl size:%d - 0x%x 0x%x  ", dwAttribs, (int)pidlTemp->mkid.cb, (int)pidlTemp->mkid.abID[0],  (int)pidlTemp->mkid.abID[1]  );
			CString str, temp;
			for(int i=0;i<pidlTemp->mkid.cb; i++)
			{
				temp.Format(L"%x ", pidlTemp->mkid.abID[i]);
				str += temp;
			}
			Debug(L"  - %s", str);
			*/


			// ��Ͽ� �߰�
			AddToList(listFolder, parent, pParentFolder, pidlParent, pidlTemp, 
				dwAttribs, TRUE);
		}
		SAFE_RELEASE(pEnum);
	}


	// ���� enum 
	if(m_list)			// ����Ʈ ��Ʈ���� ������ �̳��� enum �� �ϵ� ����.
	{
		hr = pParentFolder->EnumObjects(NULL, SHCONTF_NONFOLDERS, &pEnum);
		if (SUCCEEDED(hr)&& pEnum)
		{
			dwFetched = 1;

			// Enumerate the item's PIDLs:
			while (SUCCEEDED(pEnum->Next(1, &pidlTemp, &dwFetched)) && dwFetched)
			{
				dwFetched = 0;

				DWORD dwAttribs = SFGAO_HASSUBFOLDER | SFGAO_FOLDER | SFGAO_DISPLAYATTRMASK | SFGAO_CANRENAME;
				pParentFolder->GetAttributesOf(1, (LPCITEMIDLIST*) &pidlTemp, &dwAttribs);

				// ��Ͽ� �߰�
				AddToList(listNonFolder, parent, pParentFolder, pidlParent, pidlTemp, 
					dwAttribs, FALSE);
			}
			SAFE_RELEASE(pEnum);
		}
	}


	return TRUE;
}


////////////////////////////////////////////////////////////////////////////////////////////////////
///         ��Ͽ� ������ �߰��ϱ�
/// @param  
/// @return 
/// @date   Tuesday, October 18, 2011  10:12:50 AM
////////////////////////////////////////////////////////////////////////////////////////////////////
CShellItem* XShellMan::AddToList(ShellItemList& list, CShellItem* parent, LPSHELLFOLDER pParentFolder, XPidl& pidlParent, LPITEMIDLIST pidlTemp, DWORD dwAttribs, BOOL isTreeItem)
{
	BOOL hasChildren = (dwAttribs & SFGAO_HASSUBFOLDER) ? TRUE : FALSE;
	BOOL isShared	 = (dwAttribs & SFGAO_SHARE) ? TRUE : FALSE;
	BOOL isEditable	 = (dwAttribs & SFGAO_CANRENAME) ? TRUE : FALSE;

	CShellItem* pItem = new CShellItem;

	// parent shell folder �����ϱ�
	pItem->pParentFolder = pParentFolder;
	pItem->pParentFolder->AddRef();
	pItem->parent = parent;

	// pidl ����
	pItem->pidlRel.Attach(pidlTemp);
	pItem->pidlFQ.Attach(XPidl::ConcatPidls(pidlParent, pidlTemp));

	pItem->dispName = pItem->pidlFQ.GetDispName();
	pItem->hasChildren = hasChildren;
	pItem->isShared = isShared;
	pItem->isTreeItem = isTreeItem;
	pItem->isEditable = isEditable;

	list.AddTail(pItem);

	return pItem;
}


////////////////////////////////////////////////////////////////////////////////////////////////////
///         Ʈ���� Ȯ��Ǿ����� ó��
/// @param  
/// @return 
/// @date   Monday, October 17, 2011  5:54:02 PM
////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL XShellMan::OnTreeExpand(HTREEITEM hParentItem)
{
	if(m_bIgnoreNotify) return TRUE;

	if(m_tree==NULL){ASSERT(0); return FALSE;}		// �߻� �Ұ�

	XWaitCursor wait;

	TVITEM tvItem;
	ZeroMemory(&tvItem, sizeof(tvItem));

	tvItem.mask = TVIF_PARAM;
	tvItem.hItem = hParentItem;

	if (!TreeView_GetItem(m_tree->m_hWnd, &tvItem))
		return FALSE;

	m_tree->SetRedraw(FALSE);

	CShellItem* pItem = (CShellItem*) tvItem.lParam;
	BOOL ret = ReloadChildItemAndUpdateTree(pItem);

	// ���ϵ� �����ۿ� Ʈ�� �������� �ϳ��� ���� ���..
	if(pItem->childrenFolder.GetCount()==0)
		m_tree->RemovePlusMark(pItem->hTree);


	m_tree->SetRedraw(TRUE);
	return ret;
}


////////////////////////////////////////////////////////////////////////////////////////////////////
///         �ΰ��� ����� ���ؼ� ������� ������ �����ϸ鼭 Ʈ�������� �����ϰ�
///			�߰��Ȱ� ������ �߰��ϸ鼭 Ʈ������ �߰��ϰ�
/// @param  
/// @return 
/// @date   Tuesday, October 18, 2011  3:47:04 PM
////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL XShellMan::UpdateTreeItemList(CShellItem* pParentItem, IShellFolder* pParentFolder , ShellItemList& newList)
{
	ShellItemList&	oldList = pParentItem->childrenFolder;
	ShellItemList	mergedList;

	POSITION p, q;
	POSITION _p, _q;

	/*
		old����Ʈ�� newList �� ���ؼ� 
			1) �Ȱ����� ������ old ����Ʈ ������ merged �� �ű�
			2) �Ȱ����� ������ new ����Ʈ ������ merged �� �ű�
			3) old, new �� ������ ����
			4) merged �� �ٽ� old �� �ű�
	*/

	// ���ο� ����Ʈ ��ȯ
	p = newList.GetHeadPosition();
	while(p)
	{
		_p = p;
		CShellItem* newItem = newList.GetNext(p);
		BOOL	found = FALSE;

		// ���� ����Ʈ ��ȯ
		q = oldList.GetHeadPosition();
		while(q)
		{
			_q = q;
			CShellItem* oldItem = oldList.GetNext(q);

			// 1:1 ��
			if(pParentFolder->CompareIDs(0, newItem->pidlRel, oldItem->pidlRel)==0)
			{
				// �Ȱ����� �ִ�.. merged �� �ű��.
				mergedList.AddTail(oldItem);

				// old list ������ �����Ѵ�.
				oldList.RemoveAt(_q);
				found = TRUE;
				break;
			}
		}

		// old ����Ʈ�� �ִ� �׸��� new ����Ʈ�� ���� - ������ �߰��Ѵ�.
		if(found==FALSE)
		{
			CShellItem* item2move = newList.GetAt(_p);

			// merged list �� �ű��.
			mergedList.AddTail(item2move);
			newList.RemoveAt(_p);

			// Ʈ�� ��Ʈ�ѿ� �߰��Ѵ�.
			if(m_tree)
			{
				item2move->hTree = m_tree->InsertItem(pParentItem->hTree, item2move);
			}
		}
	}

	// oldList �� ���� ����� ������ �����۵��̴�.
	p = oldList.GetHeadPosition();
	while(p)
	{
		CShellItem* pItem = oldList.GetNext(p);

		// Ʈ�� ��Ʈ�ѿ��� �����Ѵ�.
		if(m_tree)
			m_tree->RemoveItem(pItem->hTree);

		// delete
		delete pItem;
	}
	oldList.RemoveAll();


	// merged �� �ٽ� oldList �� �� �ű��.
	p = mergedList.GetHeadPosition();
	while(p)
		oldList.AddTail(mergedList.GetNext(p));
	mergedList.RemoveAll();


	// newlist �� �ߺ��� �׸�  ����
	p = newList.GetHeadPosition();
	while(p) delete newList.GetNext(p);
	newList.RemoveAll();

	return TRUE;
}


////////////////////////////////////////////////////////////////////////////////////////////////////
///         ���� ������ �ٲ� ���
/// @param  
/// @return 
/// @date   Wednesday, October 19, 2011  11:31:58 AM
////////////////////////////////////////////////////////////////////////////////////////////////////
void XShellMan::OnTreeSelChanged(CShellItem* pItem)
{
	if(m_bIgnoreNotify) return;

	XWaitCursor wait;

	OnSelChanged(pItem);

	// ����Ʈ�� �ִٸ�..
	if(m_list)
	{
		// ���� ������ ��� �ٽ� �о� ����
		ReloadChildItemAndUpdateTree(pItem);

		// ����Ʈ ��Ʈ�� ��������
		m_list->RefreshScreen();

		// Ʈ�� ���� �������� �ִ� ���� �ϴ� ��� ó��
		if(pItem->childrenFolder.GetCount()==0)
			m_tree->RemovePlusMark(pItem->hTree, TRUE);
		else
			m_tree->RemovePlusMark(pItem->hTree, FALSE);

	}
}

void XShellMan::OnSelChanged(CShellItem* pItem)
{
	if(pItem==NULL){ASSERT(0); return;}		// �߻� �Ұ�.

	// ���� item
	m_currentItem = pItem;

	// pidl ���� ��� ��������
	GetCurrentPathFromCurPidl();
}

// m_currentItem ���� ���� ��� ��������
void XShellMan::GetCurrentPathFromCurPidl()
{
	m_sCurPath.Empty();
	m_currentItem->pidlFQ.GetPath(m_sCurPath);
	m_sCurDispName = m_currentItem->pidlFQ.GetDispName();

	// parent notify
	if(m_hWndParent)
		PostMessage(m_hWndParent, WM_XSHELL_NOTIFY_PATHCHANGED,0,0);
}


////////////////////////////////////////////////////////////////////////////////////////////////////
///         ���� ��Ŀ���� �� �ִ� ��� ����
/// @param  
/// @return 
/// @date   Wednesday, October 19, 2011  11:55:27 AM
////////////////////////////////////////////////////////////////////////////////////////////////////
LPCTSTR	XShellMan::GetCurrentPath()
{	
	return m_sCurPath;
}

LPCTSTR XShellMan::GetCurrentDispName()
{
	return m_sCurDispName;
}


////////////////////////////////////////////////////////////////////////////////////////////////////
///         Ư�� ������ �̵��ϱ�
/// @param  
/// @return 
/// @date   Wednesday, October 19, 2011  5:16:32 PM
////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL XShellMan::SetPath(LPCTSTR pathName)
{
	if(pathName==NULL){ASSERT(0); return FALSE;}

	XPidl pidl;
	if(pidl.CreateFromFromPath(pathName)==FALSE)
		return FALSE;

	return SetPath(pidl);
}

BOOL XShellMan::SetPath2Desktop()
{
	XPidl pidl;
	pidl.GetSpecialFolderLocation(CSIDL_DESKTOP);
	return SetPath(pidl);
}

BOOL XShellMan::SetPath(XPidl& pidl)
{
	if(pidl.IsEmpty()){ASSERT(0); return FALSE;}

	CShellItem*		pItemCurrent = m_root;
	int				pidlCount = pidl.GetItemCount();
	int				i;

	// "���� ȭ��"�� ���� �����Ѵ�.
	for(i=1;i<=pidlCount;i++)
	{
		XPidl	pidlTemp;

		// �տ��� �Ϻκи� �����.
		pidlTemp.Attach(pidl.Left(i));

		SHFILEINFO sfi1;
		CShellItem* pItem;

		// �̸� ���ϱ�
		if (SHGetFileInfo((LPCTSTR) pidlTemp.GetPIDL(), 0, &sfi1, sizeof(sfi1), SHGFI_PIDL | SHGFI_DISPLAYNAME) ==0)
		{ASSERT(0); return FALSE;}

		// Ʈ�� �����ۿ��� ���� ������ �ִ��� ã�ƺ���.
		pItem = pItemCurrent->FindChildrenFolder(sfi1.szDisplayName);
		if(pItem==NULL)
		{
			// ���ε� �ϰ� �ٽ��ѹ� �� ã�ƺ���.
			ReloadChildItemAndUpdateTree(pItemCurrent);
			pItem = pItemCurrent->FindChildrenFolder(sfi1.szDisplayName);
		}

		pItemCurrent = pItem;

		// ã�� ���� ��� - �����.
		if(pItemCurrent==NULL)
			break;
	}

	// ã�� ���ߴ�.
	if(pItemCurrent==NULL)
	{
		if(m_list)
			m_list->ClearItems();
		return FALSE;
	}

	// ���� �������� ã�Ҵ�.

	// parent �� notify
	OnSelChanged(pItemCurrent);

	// �ٽ� �� Ʈ�� ��� ������ �һ�縦 ���� ���ؼ� ���� ����
	m_bIgnoreNotify = TRUE;

	// Ʈ�� ��Ʈ�� ���� �ٲٱ�
	if(m_tree)
		m_tree->SelectItem(pItemCurrent->hTree);

	// ����Ʈ ��Ʈ�� ���� ���..
	if(m_list)
	{
		// ����Ʈ ��Ʈ���� ���ؼ� ���� ��ϱ��� ������Ʈ �ϱ� ���ؼ� ȣ��.
		ReloadChildItemAndUpdateTree(pItemCurrent);
		m_list->RefreshScreen();
	}

	m_bIgnoreNotify = FALSE;

	return TRUE;
}


////////////////////////////////////////////////////////////////////////////////////////////////////
///         �θ� ������� ��Ƽ���� �޽����� ������.
/// @param  
/// @return 
/// @date   Thursday, October 20, 2011  12:09:43 PM
////////////////////////////////////////////////////////////////////////////////////////////////////
LRESULT	 XShellMan::SendMessageToParent(UINT msg)
{
	// parent notify
	if(m_hWndParent)
		return SendMessage(m_hWndParent, msg,0,0);
	return 0;
}

void	 XShellMan::PostMessageToParent(UINT msg)
{
	// parent notify
	if(m_hWndParent)
		PostMessage(m_hWndParent, msg,0,0);
}


////////////////////////////////////////////////////////////////////////////////////////////////////
///         ���� ������ �̵��ϱ�
/// @param  
/// @return 
/// @date   Thursday, October 20, 2011  5:16:55 PM
////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL XShellMan::MoveToParent()
{
	if(m_currentItem==NULL || m_currentItem->parent==NULL)
		return FALSE;

	if(m_tree)
	{
		m_tree->SelectItem(m_currentItem->parent->hTree);
	}
	else
	{
		OnTreeSelChanged(m_currentItem->parent);
	}

	return TRUE;
}



////////////////////////////////////////////////////////////////////////////////////////////////////
///			���̾˷α� �ڽ��� ���ϵ��� Ʈ�� ��Ʈ���� �������϶� enter, esc Ű�� ������
///			�޽����� ���� ���޵��� �����Ƿ� OnCancel() �̳� OnOK() ���� ������ ���� ȣ���� ��� �Ѵ�.
/*

LRESULT CMainDlg::OnBnClickedOk(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
	if(m_shellMan.IsTreeCtrlEditMessage(VK_RETURN))
		return 0;

	EndDialog(IDOK);
	return 0;
}

LRESULT CMainDlg::OnBnClickedCancel(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
	if(m_shellMan.IsTreeCtrlEditMessage(VK_ESCAPE))
		return 0;

	EndDialog(IDCANCEL);
	return 0;
}

*/
///         http://support.microsoft.com/kb/167960/en-us ����
/// @param  
/// @return 
/// @date   Monday, October 24, 2011  4:01:44 PM
////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL XShellMan::IsTreeCtrlEditMessage(WPARAM keyCode)
{
	if(m_tree==NULL) return FALSE;		// Ʈ���� ���� ���
	if(m_tree->IsEditing()==FALSE) return FALSE;		// �������ΰ�?

	HWND edit = m_tree->GetEditControl();
	if(edit==NULL) return FALSE;

	::SendMessage(edit, WM_KEYDOWN, keyCode, 0);

	return TRUE;
}


////////////////////////////////////////////////////////////////////////////////////////////////////
///         �� ��ȭ�� �����ұ� ����.
/// @param  
/// @return 
/// @date   Tuesday, October 25, 2011  10:50:03 AM
////////////////////////////////////////////////////////////////////////////////////////////////////
void XShellMan::RegisterChangeNotify(BOOL reg)
{
	// �ϴ� ������ ������ ����
	if(m_ulSHChangeNotifyRegister)
		SHChangeNotifyDeregister(m_ulSHChangeNotifyRegister);
	m_ulSHChangeNotifyRegister = 0;

	if(reg==FALSE)
		return;

	///////////////////////////////////////
	//
	// http://www.codeproject.com/KB/shell/shchangenotifyregister.aspx ����
	//
	SHChangeNotifyEntry shCNE;
	shCNE.pidl = m_currentItem->pidlFQ.GetPIDL();
    shCNE.fRecursive = TRUE;

	HWND hWnd= NULL;
	if(m_list) hWnd = m_list->m_hWnd;
	else if(m_tree) hWnd = m_tree->m_hWnd;

	m_ulSHChangeNotifyRegister  = SHChangeNotifyRegister(hWnd, SHCNE_DISKEVENTS, 
                SHCNE_ALLEVENTS, WM_USER_SHELL_CHANGE_NOTIFY, 1, &shCNE);
}


////////////////////////////////////////////////////////////////////////////////////////////////////
///         Ʈ�� ��Ʈ�ѿ� ������ �����.
/// @param  
/// @return 
/// @date   Tuesday, October 25, 2011  2:51:22 PM
////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL XShellMan::CreateNewFolderAtTree(LPCTSTR folderName)
{
	if(m_tree==NULL){ASSERT(0); return FALSE;}

	// Ʈ�� ��Ʈ�� ó���� �ݹ����� EnumObject �� ȣ��Ǵ°��� ���´�.
	m_bIgnoreNotify = TRUE;
	TreeView_Expand(m_tree->m_hWnd, m_currentItem->hTree, TVE_EXPAND);
	m_bIgnoreNotify = FALSE;


	// ���� ����
	CShellItem* pItem = CreateNewFolder(folderName);
	if(pItem==NULL) return FALSE;

	// ��Ŀ�� �ְ�
	SetDialogFocus(m_tree->m_hWnd);

	// Ʈ�� ������ �����ϰ�
	HTREEITEM hNewItem = m_tree->InsertItem(m_currentItem->hTree, pItem);
	if(hNewItem==NULL) {ASSERT(0); return FALSE;}
	m_tree->RemovePlusMark(m_currentItem->hTree, FALSE);	// ���� ������ ������� + �߰�
	pItem->hTree = hNewItem;

	// ������ ���� ����
	TreeView_Select(m_tree->m_hWnd, hNewItem, TVGN_CARET);
	TreeView_EditLabel(m_tree->m_hWnd, hNewItem);

	return TRUE;
}


////////////////////////////////////////////////////////////////////////////////////////////////////
///         ����Ʈ ��Ʈ�ѿ� �� ���� �����
/// @param  
/// @return 
/// @date   Tuesday, October 25, 2011  5:07:09 PM
////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL XShellMan::CreateNewFolderAtList(LPCTSTR folderName)
{
	if(m_list==NULL){ASSERT(0); return FALSE;}

	// ���� ����
	CShellItem* pItem = CreateNewFolder(folderName);
	if(pItem==NULL) return FALSE;

	// ��Ŀ�� �ְ�
	SetDialogFocus(m_list->m_hWnd);

	// ����Ʈ ������ �����ϰ�
	int nPos = m_list->InsertItem(pItem);

	// Ʈ�� ������ Ʈ������ �߰��ϰ�
	if(m_tree)
	{
		pItem->hTree = m_tree->InsertItem(m_currentItem->hTree, pItem);
		m_tree->RemovePlusMark(m_currentItem->hTree, FALSE);	// ���� ������ ������� + �߰�
	}

	// ���� ����
	m_list->EditLabel(nPos);

	return TRUE;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
///         Ʈ���� ����Ʈ���� ������ ����� �Ҷ� ���� ȣ��Ǵ� �޽��
/// @param  
/// @return 
/// @date   Tuesday, October 25, 2011  5:07:15 PM
////////////////////////////////////////////////////////////////////////////////////////////////////
CShellItem*	XShellMan::CreateNewFolder(LPCTSTR folderName)
{
	// ���� ������ �ٸ��� ���� ���?
	if(m_currentItem==NULL || m_sCurPath.IsEmpty() || PathIsDirectory(m_sCurPath)==FALSE) {ASSERT(0); return NULL;}

	CString newFolderName = folderName;
	CString path;

	// ��� ���
	CString sCurPath = m_sCurPath;
	if(sCurPath.Right(1)!=_T("\\"))
		sCurPath += _T("\\");

	CString sDstPath = sCurPath + newFolderName;

	// �̹� ��ΰ� ������ ���..
	if(::PathIsDirectory(sDstPath))
	{
		BOOL found = FALSE;
		for(int i=2;i<=10000;i++)
		{
			newFolderName.Format(_T("%s (%d)"), folderName, i);
			sDstPath = sCurPath + newFolderName;;
			if(IsFileOrFolder(sDstPath)==FALSE) 
			{found = TRUE; break;}
		}
		if(found==FALSE)
		{ASSERT(0); return NULL;}
	}

	// notify �����ϱ�
	if(m_list) m_list->ResetShellNotifyTick();

	// "�� ����" ���� �����
	DWORD dwError = ::SHCreateDirectoryEx(m_hWndParent, sDstPath, NULL);
	if(dwError!=ERROR_SUCCESS) {ASSERT(0); return NULL;}

	// ������ �߰�
	LPITEMIDLIST pidlNew;
	LPITEMIDLIST pidlNewFolder;
	HRESULT hr;
	pidlNew = ILCreateFromPath( sDstPath );
	if(pidlNew==NULL){ASSERT(0); return NULL;}

	pidlNewFolder = XPidl::CopyItemIDList(ILFindLastID( pidlNew ));
	ILFree( pidlNew );

	// ���� ���� ������ �Ӽ� ��������
	DWORD dwAttribs = SFGAO_CANRENAME;
	dwAttribs = SFGAO_HASSUBFOLDER | SFGAO_FOLDER | SFGAO_DISPLAYATTRMASK | SFGAO_CANRENAME;

	LPSHELLFOLDER pParentFolder = GetFolder(m_currentItem->pParentFolder, m_currentItem->pidlRel);

	hr = pParentFolder->GetAttributesOf(1, (LPCITEMIDLIST*) &pidlNewFolder, &dwAttribs);

	CShellItem* pItem = AddToList(m_currentItem->childrenFolder, m_currentItem, pParentFolder, m_currentItem->pidlFQ, pidlNewFolder, dwAttribs, TRUE);

	pParentFolder->Release();
	if(pItem==NULL){ASSERT(0); XPidl::FreePidl(pidlNewFolder); return NULL;}

	return pItem;
}

void XShellMan::SetDialogFocus(HWND hwndControl)
{
	::SendMessage(m_hWndParent, WM_NEXTDLGCTL, (WPARAM)hwndControl, TRUE);
}

CString XShellMan::AddPath(CString sLeft, CString sRight)
{
	CString sRet;
	sRight = 
		sRight.GetLength()==0 ? _T("") : 
		(sRight.GetLength()==1 && sRight.Left(1)==_T("\\")) ? _T("") :  
		sRight.Left(1)==_T("\\") ? sRight.Mid(1) : sRight;

	sRet = (sLeft.Right(1)==_T("\\")) ?	sLeft + sRight : sLeft + _T("\\") + sRight;
	return sRet;
}



////////////////////////////////////////////////////////////////////////////////////////////////////
///         ������ �̸��� �ٲ���..
/// @param  
/// @return 
/// @date   Tuesday, October 25, 2011  5:26:11 PM
////////////////////////////////////////////////////////////////////////////////////////////////////
void XShellMan::ApplyItemNameChangeToTree(CShellItem* pItem)
{
	if(m_tree==NULL || pItem==NULL) return;
	if(pItem->hTree==NULL) return;

	m_tree->SetText(pItem->hTree, pItem->dispName);

}

