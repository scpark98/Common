#include "stdafx.h"
#include "XShellMan.h"


DWORD WM_XSHELL_NOTIFY_PATHCHANGED  = ::RegisterWindowMessage(_T("WM_XSHELL_NOTIFY_PATHCHANGED"));
DWORD WM_XSHELL_NOTIFY_SHELLCOMMAND = ::RegisterWindowMessage(_T("WM_XSHELL_NOTIFY_SHELLCOMMAND"));
DWORD WM_XSHELL_NOTIFY_LISTSELCHANGED = ::RegisterWindowMessage(_T("WM_XSHELL_NOTIFY_LISTSELCHANGED"));

static BOOL IsFileOrFolder(LPCTSTR szPathName)
{
	DWORD dwRet;
	dwRet = GetFileAttributes(szPathName);
	if(dwRet==0xffffffff) return FALSE;					// 오류 발생
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
///         초기화
/// @param  
/// @return 
/// @date   Friday, October 14, 2011  12:16:01 PM
////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL XShellMan::Create(HWND hWndParent, HWND hWndTreeCtrl, HWND hWndListCtrl)
{
	Destroy();

	if(FAILED(SHGetMalloc(&m_malloc))) {ASSERT(0); return FALSE;}


	m_hWndParent = hWndParent;

	// 컨트롤 attach
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
///         정리
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
///         최초 루트 아이템 생성
/// @param  
/// @return 
/// @date   Friday, October 14, 2011  2:24:11 PM
////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL XShellMan::CreateRoot()
{
	if(m_root) { ASSERT(0); return FALSE;}		// 발생 불가

	m_root = new CShellItem();

	// 바탕화면 구하기
	if(m_root->pidlRel.GetSpecialFolderLocation(CSIDL_DESKTOP)==FALSE)
	{ASSERT(0); return FALSE;}
	m_root->pidlFQ = m_root->pidlRel;
	m_root->hasChildren = TRUE;
	m_root->isTreeItem = TRUE;

	// 추가 Pidl 구하기
	if(m_pidlRecycleBin==NULL)	::SHGetSpecialFolderLocation(NULL, CSIDL_BITBUCKET, &m_pidlRecycleBin);
	if(m_pidlControlPanel.IsEmpty())
		m_pidlControlPanel.CreateFromFromPath(_T("::{26EE0668-A00A-44D7-9371-BEB064C98683}"));	// http://social.msdn.microsoft.com/Forums/en-US/csharpgeneral/thread/04c90891-641a-424d-b4a6-7208ba98d2d0/
		//::SHGetSpecialFolderLocation(NULL, CSIDL_CONTROLS, &m_pidlControlPanel);	// 정상 작동하지 않음

	// 현재 아이템
	m_currentItem = m_root;


	// 트리 컨트롤이 있으면?
	if(m_tree)
	{
		m_root->hTree = m_tree->InsertItem(TVI_ROOT, m_root);

		// 트리 확장
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
///			IShellFolder 구하기. 리턴된 값은 Release() 를 해줘야 한다.
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
		// 루트 아이템이다. 그냥 desktop 꺼 구하기
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
///         해당 아이템의 차일드 아이템을 리프레시 한다. 만약 처음이라면 차일드 목록을 새로 생성하는거고.
/// @param  
/// @return 
/// @date   Monday, October 17, 2011  5:02:42 PM
////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL XShellMan::ReloadChildItemAndUpdateTree(CShellItem* pParentItem)
{
	if(pParentItem==NULL){ASSERT(0); return FALSE;}

	LPSHELLFOLDER pParentFolder = NULL;

	// 부모 볼더의 IShellFolder 구하기
	pParentFolder = GetFolder(pParentItem->pParentFolder, pParentItem->pidlRel);


	// nonfolder 는 목록 삭제
	pParentItem->ClearNonFolder();

	// enum 해서 목록에 집어 넣는다.
	BOOL ret = TRUE;
	ShellItemList	newListFolder;
	if(EnumObjects(newListFolder, pParentItem->childrenNonFolder, pParentItem, pParentFolder, pParentItem->pidlFQ))
	{
		// 생성된 목록을 가지고 기존 목록과 비교를 한다.
		UpdateTreeItemList(pParentItem, pParentFolder, newListFolder);

		// 트리 소트하기
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
///         차일드 디렉토리(?)를 enum
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
	
	// 폴더 enum 
	hr = pParentFolder->EnumObjects(NULL, SHCONTF_FOLDERS, &pEnum);
	if(FAILED(hr)) return FALSE;										// 현재 폴더가 사라진 경우 등등..

	if (pEnum)
	{
		dwFetched = 1;
		// Enumerate the item's PIDLs:
		while (SUCCEEDED(pEnum->Next(1, &pidlTemp, &dwFetched)) && dwFetched)
		{
			dwFetched = 0;

			// zip(cab) 파일인지 확인하기
			DWORD dwAttribs = SFGAO_STREAM | SFGAO_REMOVABLE;		// 가져올 속성 마스크
			pParentFolder->GetAttributesOf(1, (LPCITEMIDLIST*) &pidlTemp, &dwAttribs);

			// 압축 파일(zip, cab) 폴더이다
			if(m_opt.treatCompressedFolderAsFile && (dwAttribs & SFGAO_STREAM))
			{
				if(m_list==NULL) continue;		// 그냥 skip.

				// 압축파일을 "파일 목록"에 추가한다.
				DWORD dwAttribs = SFGAO_HASSUBFOLDER | SFGAO_FOLDER | SFGAO_DISPLAYATTRMASK | SFGAO_CANRENAME;
				pParentFolder->GetAttributesOf(1, (LPCITEMIDLIST*) &pidlTemp, &dwAttribs);

				// 목록에 추가
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
						continue;										// 플로피 이다!!
				}
			}

			// 휴지통?
			if(m_opt.hideRecycleBin && m_pidlRecycleBin && ::ILIsEqual(pidlTemp, m_pidlRecycleBin))
				continue;

			// 제어판? 
			if(m_opt.hideControlPanel && m_pidlControlPanel.IsEmpty()==FALSE && ::ILIsEqual(pidlTemp, m_pidlControlPanel))
				continue;

			// 일반 폴더일 경우 상세 아이템 속성 가져오기
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


			// 목록에 추가
			AddToList(listFolder, parent, pParentFolder, pidlParent, pidlTemp, 
				dwAttribs, TRUE);
		}
		SAFE_RELEASE(pEnum);
	}


	// 파일 enum 
	if(m_list)			// 리스트 컨트롤이 없으면 이놈을 enum 할 일도 없다.
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

				// 목록에 추가
				AddToList(listNonFolder, parent, pParentFolder, pidlParent, pidlTemp, 
					dwAttribs, FALSE);
			}
			SAFE_RELEASE(pEnum);
		}
	}


	return TRUE;
}


////////////////////////////////////////////////////////////////////////////////////////////////////
///         목록에 아이템 추가하기
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

	// parent shell folder 세팅하기
	pItem->pParentFolder = pParentFolder;
	pItem->pParentFolder->AddRef();
	pItem->parent = parent;

	// pidl 세팅
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
///         트리가 확장되었을때 처리
/// @param  
/// @return 
/// @date   Monday, October 17, 2011  5:54:02 PM
////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL XShellMan::OnTreeExpand(HTREEITEM hParentItem)
{
	if(m_bIgnoreNotify) return TRUE;

	if(m_tree==NULL){ASSERT(0); return FALSE;}		// 발생 불가

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

	// 차일드 아이템에 트리 아이템이 하나도 없을 경우..
	if(pItem->childrenFolder.GetCount()==0)
		m_tree->RemovePlusMark(pItem->hTree);


	m_tree->SetRedraw(TRUE);
	return ret;
}


////////////////////////////////////////////////////////////////////////////////////////////////////
///         두개의 목록을 비교해서 사라진게 있으면 삭제하면서 트리에서도 삭제하고
///			추가된게 있으면 추가하면서 트리에도 추가하고
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
		old리스트와 newList 를 비교해서 
			1) 똑같은게 있으면 old 리스트 내용을 merged 로 옮김
			2) 똑같은게 없으면 new 리스트 내용을 merged 로 옮김
			3) old, new 에 남은건 삭제
			4) merged 를 다시 old 로 옮김
	*/

	// 새로운 리스트 순환
	p = newList.GetHeadPosition();
	while(p)
	{
		_p = p;
		CShellItem* newItem = newList.GetNext(p);
		BOOL	found = FALSE;

		// 이전 리스트 순환
		q = oldList.GetHeadPosition();
		while(q)
		{
			_q = q;
			CShellItem* oldItem = oldList.GetNext(q);

			// 1:1 비교
			if(pParentFolder->CompareIDs(0, newItem->pidlRel, oldItem->pidlRel)==0)
			{
				// 똑같은게 있다.. merged 로 옮긴다.
				mergedList.AddTail(oldItem);

				// old list 에서는 삭제한다.
				oldList.RemoveAt(_q);
				found = TRUE;
				break;
			}
		}

		// old 리스트에 있는 항목이 new 리스트에 없다 - 새로이 추가한다.
		if(found==FALSE)
		{
			CShellItem* item2move = newList.GetAt(_p);

			// merged list 로 옮긴다.
			mergedList.AddTail(item2move);
			newList.RemoveAt(_p);

			// 트리 컨트롤에 추가한다.
			if(m_tree)
			{
				item2move->hTree = m_tree->InsertItem(pParentItem->hTree, item2move);
			}
		}
	}

	// oldList 에 남은 놈들은 삭제된 아이템들이다.
	p = oldList.GetHeadPosition();
	while(p)
	{
		CShellItem* pItem = oldList.GetNext(p);

		// 트리 컨트롤에서 삭제한다.
		if(m_tree)
			m_tree->RemoveItem(pItem->hTree);

		// delete
		delete pItem;
	}
	oldList.RemoveAll();


	// merged 를 다시 oldList 로 다 옮긴다.
	p = mergedList.GetHeadPosition();
	while(p)
		oldList.AddTail(mergedList.GetNext(p));
	mergedList.RemoveAll();


	// newlist 의 중복된 항목도  삭제
	p = newList.GetHeadPosition();
	while(p) delete newList.GetNext(p);
	newList.RemoveAll();

	return TRUE;
}


////////////////////////////////////////////////////////////////////////////////////////////////////
///         현재 폴더가 바뀐 경우
/// @param  
/// @return 
/// @date   Wednesday, October 19, 2011  11:31:58 AM
////////////////////////////////////////////////////////////////////////////////////////////////////
void XShellMan::OnTreeSelChanged(CShellItem* pItem)
{
	if(m_bIgnoreNotify) return;

	XWaitCursor wait;

	OnSelChanged(pItem);

	// 리스트가 있다면..
	if(m_list)
	{
		// 현재 폴더의 목록 다시 읽어 오고
		ReloadChildItemAndUpdateTree(pItem);

		// 리스트 컨트롤 리프레시
		m_list->RefreshScreen();

		// 트리 하위 아이템이 있다 없고 하는 경우 처리
		if(pItem->childrenFolder.GetCount()==0)
			m_tree->RemovePlusMark(pItem->hTree, TRUE);
		else
			m_tree->RemovePlusMark(pItem->hTree, FALSE);

	}
}

void XShellMan::OnSelChanged(CShellItem* pItem)
{
	if(pItem==NULL){ASSERT(0); return;}		// 발생 불가.

	// 현재 item
	m_currentItem = pItem;

	// pidl 에서 경로 가져오기
	GetCurrentPathFromCurPidl();
}

// m_currentItem 에서 현재 경로 가져오기
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
///         현재 포커스가 가 있는 경로 리턴
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
///         특정 폴더로 이동하기
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

	// "바탕 화면"은 빼고 시작한다.
	for(i=1;i<=pidlCount;i++)
	{
		XPidl	pidlTemp;

		// 앞에서 일부분만 떼어낸다.
		pidlTemp.Attach(pidl.Left(i));

		SHFILEINFO sfi1;
		CShellItem* pItem;

		// 이름 구하기
		if (SHGetFileInfo((LPCTSTR) pidlTemp.GetPIDL(), 0, &sfi1, sizeof(sfi1), SHGFI_PIDL | SHGFI_DISPLAYNAME) ==0)
		{ASSERT(0); return FALSE;}

		// 트리 아이템에서 같은 폴더가 있는지 찾아본다.
		pItem = pItemCurrent->FindChildrenFolder(sfi1.szDisplayName);
		if(pItem==NULL)
		{
			// 리로드 하고 다시한번 더 찾아본다.
			ReloadChildItemAndUpdateTree(pItemCurrent);
			pItem = pItemCurrent->FindChildrenFolder(sfi1.szDisplayName);
		}

		pItemCurrent = pItem;

		// 찾지 못한 경우 - 멈춘다.
		if(pItemCurrent==NULL)
			break;
	}

	// 찾지 못했다.
	if(pItemCurrent==NULL)
	{
		if(m_list)
			m_list->ClearItems();
		return FALSE;
	}

	// 최종 목적지를 찾았다.

	// parent 로 notify
	OnSelChanged(pItemCurrent);

	// 다시 또 트리 목록 얻어오는 불상사를 막기 위해서 변수 세팅
	m_bIgnoreNotify = TRUE;

	// 트리 컨트롤 선택 바꾸기
	if(m_tree)
		m_tree->SelectItem(pItemCurrent->hTree);

	// 리스트 컨트롤 있을 경우..
	if(m_list)
	{
		// 리스트 컨트롤을 위해서 파일 목록까지 업데이트 하기 위해서 호출.
		ReloadChildItemAndUpdateTree(pItemCurrent);
		m_list->RefreshScreen();
	}

	m_bIgnoreNotify = FALSE;

	return TRUE;
}


////////////////////////////////////////////////////////////////////////////////////////////////////
///         부모 윈도우로 노티파이 메시지를 보낸다.
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
///         상위 폴더로 이동하기
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
///			다이알로그 박스의 차일드인 트리 컨트롤을 편집중일때 enter, esc 키를 누르면
///			메시지가 정상 전달되지 않으므로 OnCancel() 이나 OnOK() 에서 다음과 같이 호출해 줘야 한다.
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
///         http://support.microsoft.com/kb/167960/en-us 참고
/// @param  
/// @return 
/// @date   Monday, October 24, 2011  4:01:44 PM
////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL XShellMan::IsTreeCtrlEditMessage(WPARAM keyCode)
{
	if(m_tree==NULL) return FALSE;		// 트리가 없는 경우
	if(m_tree->IsEditing()==FALSE) return FALSE;		// 편집중인가?

	HWND edit = m_tree->GetEditControl();
	if(edit==NULL) return FALSE;

	::SendMessage(edit, WM_KEYDOWN, keyCode, 0);

	return TRUE;
}


////////////////////////////////////////////////////////////////////////////////////////////////////
///         셸 변화를 감지할까 말까.
/// @param  
/// @return 
/// @date   Tuesday, October 25, 2011  10:50:03 AM
////////////////////////////////////////////////////////////////////////////////////////////////////
void XShellMan::RegisterChangeNotify(BOOL reg)
{
	// 일단 무조건 기존껀 삭제
	if(m_ulSHChangeNotifyRegister)
		SHChangeNotifyDeregister(m_ulSHChangeNotifyRegister);
	m_ulSHChangeNotifyRegister = 0;

	if(reg==FALSE)
		return;

	///////////////////////////////////////
	//
	// http://www.codeproject.com/KB/shell/shchangenotifyregister.aspx 참고
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
///         트리 컨트롤에 새폴더 만들기.
/// @param  
/// @return 
/// @date   Tuesday, October 25, 2011  2:51:22 PM
////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL XShellMan::CreateNewFolderAtTree(LPCTSTR folderName)
{
	if(m_tree==NULL){ASSERT(0); return FALSE;}

	// 트리 컨트롤 처리시 콜백으로 EnumObject 가 호출되는것을 막는다.
	m_bIgnoreNotify = TRUE;
	TreeView_Expand(m_tree->m_hWnd, m_currentItem->hTree, TVE_EXPAND);
	m_bIgnoreNotify = FALSE;


	// 폴더 생성
	CShellItem* pItem = CreateNewFolder(folderName);
	if(pItem==NULL) return FALSE;

	// 포커스 주고
	SetDialogFocus(m_tree->m_hWnd);

	// 트리 아이템 생성하고
	HTREEITEM hNewItem = m_tree->InsertItem(m_currentItem->hTree, pItem);
	if(hNewItem==NULL) {ASSERT(0); return FALSE;}
	m_tree->RemovePlusMark(m_currentItem->hTree, FALSE);	// 하위 폴더가 없을경우 + 추가
	pItem->hTree = hNewItem;

	// 선택후 편집 시작
	TreeView_Select(m_tree->m_hWnd, hNewItem, TVGN_CARET);
	TreeView_EditLabel(m_tree->m_hWnd, hNewItem);

	return TRUE;
}


////////////////////////////////////////////////////////////////////////////////////////////////////
///         리스트 컨트롤에 새 폴더 만들기
/// @param  
/// @return 
/// @date   Tuesday, October 25, 2011  5:07:09 PM
////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL XShellMan::CreateNewFolderAtList(LPCTSTR folderName)
{
	if(m_list==NULL){ASSERT(0); return FALSE;}

	// 폴더 생성
	CShellItem* pItem = CreateNewFolder(folderName);
	if(pItem==NULL) return FALSE;

	// 포커스 주고
	SetDialogFocus(m_list->m_hWnd);

	// 리스트 아이템 생성하고
	int nPos = m_list->InsertItem(pItem);

	// 트리 있으면 트리에도 추가하고
	if(m_tree)
	{
		pItem->hTree = m_tree->InsertItem(m_currentItem->hTree, pItem);
		m_tree->RemovePlusMark(m_currentItem->hTree, FALSE);	// 하위 폴더가 없을경우 + 추가
	}

	// 편집 시작
	m_list->EditLabel(nPos);

	return TRUE;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
///         트리와 리스트에서 새폴더 만들기 할때 공통 호출되는 메쏘드
/// @param  
/// @return 
/// @date   Tuesday, October 25, 2011  5:07:15 PM
////////////////////////////////////////////////////////////////////////////////////////////////////
CShellItem*	XShellMan::CreateNewFolder(LPCTSTR folderName)
{
	// 현재 폴더가 바르지 않은 경우?
	if(m_currentItem==NULL || m_sCurPath.IsEmpty() || PathIsDirectory(m_sCurPath)==FALSE) {ASSERT(0); return NULL;}

	CString newFolderName = folderName;
	CString path;

	// 대상 경로
	CString sCurPath = m_sCurPath;
	if(sCurPath.Right(1)!=_T("\\"))
		sCurPath += _T("\\");

	CString sDstPath = sCurPath + newFolderName;

	// 이미 경로가 존재할 경우..
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

	// notify 무시하기
	if(m_list) m_list->ResetShellNotifyTick();

	// "새 폴더" 폴더 만들기
	DWORD dwError = ::SHCreateDirectoryEx(m_hWndParent, sDstPath, NULL);
	if(dwError!=ERROR_SUCCESS) {ASSERT(0); return NULL;}

	// 아이템 추가
	LPITEMIDLIST pidlNew;
	LPITEMIDLIST pidlNewFolder;
	HRESULT hr;
	pidlNew = ILCreateFromPath( sDstPath );
	if(pidlNew==NULL){ASSERT(0); return NULL;}

	pidlNewFolder = XPidl::CopyItemIDList(ILFindLastID( pidlNew ));
	ILFree( pidlNew );

	// 새로 만든 폴더의 속성 가져오기
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
///         아이템 이름이 바뀐경우..
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

