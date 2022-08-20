#include "stdafx.h"
#include "XShellListCtrl.h"
#include "XShellMan.h"

DWORD WM_USER_SHELL_CHANGE_NOTIFY = ::RegisterWindowMessage(_T("WM_USER_SHELL_CHANGE_NOTIFY"));

XShellListCtrl::XShellListCtrl()
{
	m_man = NULL;
	m_column2sort = 0;
	m_pContextMenu2 = NULL;
	m_editingPos = -1;
	m_editing = FALSE;
	m_tick2IgnoreShallChangeNotify = 0;
	m_sortBy = ShellList_ColumnName;
	m_sortAscending = TRUE;
}

XShellListCtrl::~XShellListCtrl()
{
}

BOOL XShellListCtrl::Attach(XShellMan* man, HWND hWndList)
{
	if(__super::SubclassWindow(hWndList)==FALSE)  { ASSERT(0); return FALSE;}
	m_man = man;

	InitList();

	return TRUE;
}

LRESULT XShellListCtrl::OnDestroy(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
	m_man->RegisterChangeNotify(FALSE);

	return 0;
}


void XShellListCtrl::InitList()
{
	ModifyStyle(0, LVS_SHAREIMAGELISTS);

	// Set shell images:
	ListView_SetImageList(m_hWnd, GetShellImageList(TRUE), LVSIL_NORMAL);
	ListView_SetImageList(m_hWnd, GetShellImageList(FALSE), LVSIL_SMALL);

	OnSetColumns();

}

HIMAGELIST XShellListCtrl::GetShellImageList(BOOL bLarge)
{
	TCHAR szWinDir [MAX_PATH + 1];
	if (GetWindowsDirectory(szWinDir, MAX_PATH) == 0)
	{
		return NULL;
	}

	SHFILEINFO sfi;
	HIMAGELIST hImageList = (HIMAGELIST) SHGetFileInfo(szWinDir, 0, &sfi, sizeof(SHFILEINFO), SHGFI_SYSICONINDEX |(bLarge ? 0 : SHGFI_SMALLICON));
	return hImageList;
}

void XShellListCtrl::OnSetColumns()
{
	const TCHAR* szName [] = 
	{	m_man->m_opt.textListColumnName.GetBuffer(),
		m_man->m_opt.textListColumnSize.GetBuffer(),
		m_man->m_opt.textListColumnType.GetBuffer(),
		m_man->m_opt.textListColumnModified.GetBuffer(),};

	const int width[] = {
		m_man->m_opt.columnWidthName, 
		m_man->m_opt.columnWidthSize, 
		m_man->m_opt.columnWidthType, 
		m_man->m_opt.columnWidthModified,
	};


	for (int iColumn = 0; iColumn < 4; iColumn++)
	{
		int nFormat = (iColumn == ShellList_ColumnSize) ? LVCFMT_RIGHT : LVCFMT_LEFT;

		LVCOLUMN	lvcolumn;
		lvcolumn.mask = LVCF_FMT | LVCF_SUBITEM | LVCF_TEXT | LVCF_WIDTH;
		lvcolumn.fmt = nFormat;
		lvcolumn.iSubItem = iColumn;
		lvcolumn.pszText = (LPWSTR)szName[iColumn];
		lvcolumn.cx = width[iColumn];

		ListView_InsertColumn(m_hWnd, iColumn, &lvcolumn);  //InsertColumn(m_hWnd, iColumn, szName [iColumn], nFormat, 100, iColumn);
	}
}



////////////////////////////////////////////////////////////////////////////////////////////////////
///         WM_NOTIFY 처리...
/// @param  
/// @return 
/// @date   Monday, October 24, 2011  11:40:03 AM
////////////////////////////////////////////////////////////////////////////////////////////////////
LRESULT XShellListCtrl::OnNotify(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
	NMHDR	*pNotify = (NMHDR*)lParam;

	//Debug(L"notify: %d", pNotify->code - LVN_FIRST);

	if(pNotify->code==LVN_BEGINLABELEDIT)
	{
		if(m_editing) return FALSE;		// F2 키 눌렀을때..

		if(IsEditable(m_editingPos)==FALSE)
			return TRUE;				// edit 안함

		m_editing = TRUE;				// 편집 시작
		return FALSE;
	}
	else if(pNotify->code==LVN_ENDLABELEDIT)
	{
		NMLVDISPINFO* pdi = (NMLVDISPINFO*)lParam;
		BOOL ret = EndLabelEdit(pdi);
		m_editing = FALSE;
		return ret;
	}
	else if(pNotify->code==LVN_ITEMCHANGED)
	{
		// 아이템 선택이 바뀌었다.
		SetTimer(TIMER_DELAYPOSTSELCHANGE, 100);
	}
	else if(pNotify->code==LVN_COLUMNCLICK)
	{
		int column = ((NMLISTVIEW*)pNotify)->iSubItem;
		OnSortColumn(column);
	}
	else
		bHandled = FALSE;
	
	return 0;
}


////////////////////////////////////////////////////////////////////////////////////////////////////
///         현재 아이템 목록 보여주기
/// @param  
/// @return 
/// @date   Wednesday, October 19, 2011  2:20:55 PM
////////////////////////////////////////////////////////////////////////////////////////////////////
void XShellListCtrl::RefreshScreen()
{
	// 기존꺼 다 지우고
	ListView_DeleteAllItems(m_hWnd);

	if(m_man->m_currentItem==NULL) return;

	//////////////////////////////////////
	//
	//
	m_man->RegisterChangeNotify(TRUE);



	//////////////////////////////////////
	//
	//

	CShellItem* pParentItem = m_man->GetCurrentItem();
	if(pParentItem==NULL){ASSERT(0); return;}


	XWaitCursor wait;
	SetRedraw(FALSE);

	// .. 폴더
	if(m_man->m_opt.showDotDotAtListControl)
	{
		if(pParentItem->parent)		// 부모 폴더가 있을 경우..
			InsertItem(NULL);
	}


	InsertItems(pParentItem->childrenFolder);
	InsertItems(pParentItem->childrenNonFolder);

	Sort();
	ProcessSortIcon();

	SetRedraw(TRUE);
	RedrawWindow();
}

void XShellListCtrl::ClearItems()
{
	// 기존꺼 다 지우고
	ListView_DeleteAllItems(m_hWnd);
}


////////////////////////////////////////////////////////////////////////////////////////////////////
///         리스트를 가지고 목록에 추가
/// @param  
/// @return 
/// @date   Wednesday, October 19, 2011  3:57:08 PM
////////////////////////////////////////////////////////////////////////////////////////////////////
void XShellListCtrl::InsertItems(ShellItemList& list)
{
	POSITION p = list.GetHeadPosition();

	SetRedraw(FALSE);
	while(p)
	{
		CShellItem* pItem = list.GetNext(p);
		InsertItem(pItem);
	}
	SetRedraw(TRUE);
}

int XShellListCtrl::InsertItem(CShellItem* pItem)
{
	LVITEM lvItem;
	ZeroMemory(&lvItem, sizeof(lvItem));

	lvItem.mask = LVIF_PARAM | LVIF_TEXT | LVIF_IMAGE | LVIF_STATE;
	lvItem.pszText = _T("");			// 텍스트 세팅은 OnSetItemText() 에서 처리..
	lvItem.iImage = GetItemIcon(pItem);
	lvItem.lParam = (LPARAM)pItem;		// 아이템 데이타

	if(pItem)
	{
		//determine if the item is shared
		DWORD dwAttr = SFGAO_DISPLAYATTRMASK;
		pItem->pParentFolder->GetAttributesOf(1, (LPCITEMIDLIST*)&(pItem->pidlRel), &dwAttr);

		if (dwAttr & SFGAO_SHARE)
		{
			lvItem.mask |= LVIF_STATE;
			lvItem.stateMask |= LVIS_OVERLAYMASK;
			lvItem.state |= INDEXTOOVERLAYMASK(1); //1 is the index for the shared overlay image
		}

		if (dwAttr & SFGAO_GHOSTED)
		{
			lvItem.mask |= LVIF_STATE;
			lvItem.stateMask |= LVIS_CUT;
			lvItem.state |= LVIS_CUT;
		}
	}

	int iItem = ListView_InsertItem(m_hWnd, &lvItem);

	if(iItem>=0)
		OnSetItemText(iItem, pItem);

	return iItem;
}


int XShellListCtrl::GetItemIcon(CShellItem* pItem)
{
	// .. 폴더?
	if (pItem == NULL) { return 1;}

	SHFILEINFO sfi;
	int iIcon = -1;

	if (SHGetFileInfo((LPCTSTR)pItem->pidlFQ.GetPIDL(), 0, &sfi, sizeof(sfi), SHGFI_PIDL | SHGFI_SYSICONINDEX | SHGFI_SMALLICON | SHGFI_LINKOVERLAY))
		iIcon = sfi.iIcon;

	return iIcon;
}



////////////////////////////////////////////////////////////////////////////////////////////////////
///         아이템 텍스트 세팅하기
/// @param  
/// @return 
/// @date   Wednesday, October 19, 2011  3:20:12 PM
////////////////////////////////////////////////////////////////////////////////////////////////////
void XShellListCtrl::OnSetItemText(int nItem, CShellItem* pItem)
{
	// .. 폴더?
	if(pItem==NULL)
	{
		ListView_SetItemText(m_hWnd, nItem, 0, _T(".."));
		return;
	}


	// 파일명
	ListView_SetItemText(m_hWnd, nItem, 0, pItem->dispName.GetBuffer());

	if(!(GetStyle() & LVS_REPORT))
		return;

	TCHAR	pathName[MAX_PATH];
	if(SHGetPathFromIDList(pItem->pidlFQ, pathName))
	{
		HANDLE   hFile;
		hFile = ::CreateFile(pathName, 0, 0 , NULL, OPEN_EXISTING, 0, NULL);
		if(hFile!=INVALID_HANDLE_VALUE)
		{
			// 파일 크기
			LARGE_INTEGER size;
			if(::GetFileSizeEx(hFile, &size))
			{
				TCHAR buf[1024];
				if(StrFormatByteSize64(size.QuadPart, buf, 1024))
					ListView_SetItemText(m_hWnd, nItem, 1, buf);
				pItem->fileSize = size.QuadPart;
			}
			else
				pItem->fileSize = -1;

			// 파일 시간
			FILETIME ftWrite;
			if(::GetFileTime(hFile, NULL, NULL, &ftWrite))
			{
			    SYSTEMTIME stUTC, stLocal;
			    FileTimeToSystemTime(&ftWrite, &stUTC);
			    SystemTimeToTzSpecificLocalTime(NULL, &stUTC, &stLocal);

				TCHAR szLocalDate[255];
				GetDateFormat( LOCALE_USER_DEFAULT, DATE_LONGDATE, &stLocal, NULL, szLocalDate, 255 );

				TCHAR szLocalTime[255];
				GetTimeFormat( LOCALE_USER_DEFAULT, 0, &stLocal, NULL, szLocalTime, 255 );

				CString str;
				str.Format(_T("%s %s"), szLocalDate, szLocalTime);
				ListView_SetItemText(m_hWnd, nItem, 3, str.GetBuffer());

				pItem->fileTime = ftWrite;
			}
			else
			{
				pItem->fileTime.dwHighDateTime = 0;
				pItem->fileTime.dwLowDateTime = 0;
			}

		}
		CloseHandle(hFile);
	}

	// 기타 설명
	SHFILEINFO sfi;

	// 파일 설명
	if (SHGetFileInfo((LPCTSTR)pItem->pidlFQ.GetPIDL(), 0, &sfi, sizeof(sfi), SHGFI_PIDL | SHGFI_TYPENAME))
	{
		ListView_SetItemText(m_hWnd, nItem, 2, sfi.szTypeName);
		pItem->typeName = sfi.szTypeName;
	}
	else
		pItem->typeName.Empty();
}

void XShellListCtrl::Sort()
{
	::SendMessage(m_hWnd, LVM_SORTITEMS, (WPARAM)this, (LPARAM)CompareProc);
}

int CALLBACK XShellListCtrl::CompareProc(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort)
{
	XShellListCtrl* pList = (XShellListCtrl*) lParamSort;
	return pList->OnCompareItems((CShellItem*)lParam1, (CShellItem*)lParam2);
}

int XShellListCtrl::OnCompareItems(CShellItem* pItem1, CShellItem* pItem2)
{
	// .. 폴더 처리
	if(pItem1==NULL) return -1;
	if(pItem2==NULL) return 1;


	int nRes = 0;


	if(m_sortBy==ShellList_ColumnName)
	{
		HRESULT hr = pItem1->pParentFolder->CompareIDs(0, pItem1->pidlRel, pItem2->pidlRel);
		if (FAILED(hr)) {ASSERT(0); return 0;}
		nRes = (short) SCODE_CODE(GetScode(hr));

		if(m_sortAscending==FALSE)
			nRes = -nRes;
	}
	else
	{
		switch(m_sortBy)
		{
		case  ShellList_ColumnSize:
			if(pItem1->fileSize > pItem2->fileSize) nRes = 1;
			else if(pItem1->fileSize < pItem2->fileSize) nRes = -1;
			break;

		case  ShellList_ColumnType:
			nRes = pItem1->typeName.Compare(pItem2->typeName);
			break;

		case  ShellList_ColumnModified:
			nRes = CompareFileTime(&pItem1->fileTime, &pItem2->fileTime);
			break;

		case  ShellList_ColumnName :
			break;
		default :
			ASSERT(0);
			break;
		}

		if(m_sortAscending==FALSE)
			nRes = -nRes;

		if(nRes==0)
		{
			HRESULT hr = pItem1->pParentFolder->CompareIDs(0, pItem1->pidlRel, pItem2->pidlRel);
			if (FAILED(hr)) {ASSERT(0); return 0;}
			nRes = (short) SCODE_CODE(GetScode(hr));
		}
	}

	return nRes;
}



LRESULT XShellListCtrl::HandleMenu(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
	if (m_pContextMenu2 != NULL)
		m_pContextMenu2->HandleMenuMsg(uMsg, wParam, lParam);
	return 0;
}


////////////////////////////////////////////////////////////////////////////////////////////////////
///         context 메뉴 처리하기
/// @param  
/// @return 
/// @date   Thursday, October 20, 2011  3:40:14 PM
////////////////////////////////////////////////////////////////////////////////////////////////////
LRESULT XShellListCtrl::OnContextMenu(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
	// invoke command 수행중에 다시한번 우클릭하면 여기로 올 수 있다.
	if (m_pContextMenu2) {return 0;}

	if (m_man->m_opt.enableListControlContextMenu==FALSE)
	{bHandled = FALSE;return  0;}

	if(m_man->m_currentItem==NULL)
	{ASSERT(0); return 0;}


	// 현재 폴더에 대한 IShellFolder 구하기.
	IShellFolder* pParentFolder = m_man->m_currentItem->pParentFolder;
	IShellFolder* pCurFolder;
	HRESULT	hr;

	if(pParentFolder==NULL)
		hr = SHGetDesktopFolder(&pParentFolder);
	else
		hr = pParentFolder->BindToObject(m_man->m_currentItem->pidlRel, NULL, IID_IShellFolder, (LPVOID*) &pCurFolder);

	if(FAILED(hr) || pCurFolder==NULL)
	{ASSERT(0);return 0;}



	CPoint			point(lParam);
	UINT			nSelItems = ListView_GetSelectedCount(m_hWnd);
	int				nClickedItem = -1;
	IContextMenu*	pcm = NULL;
	CShellItem*		pItem = NULL; 


	// 아이템이 선택되었다면?
	if(nSelItems>0)
	{
		// 키보드
		if (point.x == -1 && point.y == -1)
		{
			int nCurItem = -1;
			int nLastSelItem = -1;

			for (UINT i = 0; i < nSelItems; i++)
			{
				nCurItem = ListView_GetNextItem(m_hWnd, nCurItem, LVNI_SELECTED);
				nLastSelItem = nCurItem;
			}

			CRect rectItem;
			if (ListView_GetItemRect(m_hWnd, nLastSelItem, rectItem, LVIR_BOUNDS))
			{
				point.x = rectItem.left;
				point.y = rectItem.bottom + 1;

				ClientToScreen(&point);
			}
		}
		// 마우스
		else
		{
			// Clicked on specifed item:
			LVHITTESTINFO lvhti;
			lvhti.pt = point;
			ScreenToClient(&lvhti.pt);

			lvhti.flags = LVHT_NOWHERE;

			ListView_HitTest(m_hWnd, &lvhti);

			if ((lvhti.flags & LVHT_ONITEM) == 0)
			{
				// Click ouside of items, do nothing
				return  0;
			}

			nClickedItem = lvhti.iItem;
		}


		// 선택된 파일 목록에 대한 pidl 골라내기
		LPITEMIDLIST* pPidls = (LPITEMIDLIST*) XPidl::g_malloc->Alloc(sizeof(LPITEMIDLIST) * nSelItems);
		if(pPidls==NULL){ASSERT(0); return 0;}

		// Get the selected items:
		LVITEM lvItem;
		ZeroMemory(&lvItem, sizeof(lvItem));
		lvItem.mask = LVIF_PARAM;

		// 마우스로 클릭한 경우 - 마우스 위치에 있는 놈이 첫번째..
		if (nClickedItem >= 0)
		{
			// Put the item clicked on first in the list:
			lvItem.iItem = nClickedItem;

			if (ListView_GetItem(m_hWnd, &lvItem))
			{
				pItem = (CShellItem*)lvItem.lParam;
				pPidls [0] = pItem->pidlRel;
			}
		}

		int nCurItem = -1;
		for (UINT i = nClickedItem >= 0 ? 1 : 0; i < nSelItems; i++)
		{
			nCurItem = ListView_GetNextItem(m_hWnd, nCurItem, LVNI_SELECTED);
			if (nCurItem != nClickedItem)
			{
				lvItem.iItem = nCurItem;

				if (ListView_GetItem(m_hWnd, &lvItem))
				{
					CShellItem* pItemTemp = (CShellItem*)lvItem.lParam;
					pPidls [i] = pItemTemp->pidlRel;

					if (pItem == NULL)
					{
						pItem = pItemTemp;
					}
				}
			}
			else
			{
				i--;
			}
		}

		if (pPidls [0] == NULL)
		{
			XPidl::g_malloc->Free(pPidls);
			return 0;
		}

		// context menu 얻기
		hr = pCurFolder->GetUIObjectOf(m_hWnd, nSelItems, (LPCITEMIDLIST*)pPidls, IID_IContextMenu, NULL, (LPVOID*)&pcm);

		// pidl 프리
		XPidl::g_malloc->Free(pPidls);

		if(FAILED(hr)){ASSERT(0); return 0;}
	}
	else	// 폴더 바닥인 경우
	{
		// 폴더 바닥용 context menu 얻기
		hr = pCurFolder->CreateViewObject(m_hWnd, IID_IContextMenu, (LPVOID*)&pcm);
		if (FAILED(hr) || pcm==NULL) {ASSERT(0); return 0;}


		// 키보드인 경우 위치 잡기
		if (point.x == -1 && point.y == -1)
		{
			CRect rWnd;
			GetWindowRect(&rWnd);

			point.x = (rWnd.left + rWnd.right)/2;
			point.y = (rWnd.top + rWnd.bottom)/2;
		}
	}

	if (SUCCEEDED(hr))
	{
		hr = pcm->QueryInterface(IID_IContextMenu2, (LPVOID*)&m_pContextMenu2);

		if (SUCCEEDED(hr))
		{
			HMENU hPopup = CreatePopupMenu();
			if (hPopup != NULL)
			{
				hr = m_pContextMenu2->QueryContextMenu(hPopup, 0, 1, 0x7fff, CMF_NORMAL | CMF_EXPLORE);

				if (SUCCEEDED(hr))
				{
					UINT idCmd = TrackPopupMenu(hPopup, TPM_LEFTALIGN | TPM_RETURNCMD | TPM_RIGHTBUTTON, point.x, point.y, 0, m_hWnd, NULL);

					if (idCmd != 0)
					{
						BOOL bDoNotInvokeCommand = FALSE;

						// 1개 선택 + 기본 메뉴 선택인가?
						if (nSelItems == 1 && idCmd == ::GetMenuDefaultItem(hPopup, FALSE, 0) && pItem)
						{
							// If specified element is a folder, try to display it:
							ULONG ulAttrs = SFGAO_FOLDER;
							pCurFolder->GetAttributesOf(1, (const struct _ITEMIDLIST **) &pItem->pidlRel, &ulAttrs);

							// 폴더하나 선택후 "열기" 를 선택한 경우이다.
							// 해당 폴더로 이동하면 끝.
							if (ulAttrs & SFGAO_FOLDER)
							{
								bDoNotInvokeCommand = TRUE;
								EnterFolder(pItem);
							}
						}

						// 메뉴에서 선택한 명령 수행하기.
						if (bDoNotInvokeCommand==FALSE)
						{
							CMINVOKECOMMANDINFO cmi;
							cmi.cbSize = sizeof(CMINVOKECOMMANDINFO);
							cmi.fMask = 0;
							cmi.hwnd = (HWND) GetParent();
							cmi.lpVerb = (LPCSTR)(INT_PTR)(idCmd - 1);
							cmi.lpParameters = NULL;
							cmi.lpDirectory = NULL;
							cmi.nShow = SW_SHOWNORMAL;
							cmi.dwHotKey = 0;
							cmi.hIcon = NULL;

							hr = pcm->InvokeCommand(&cmi);

							// parent 로 notify 메시지 보내기
							if (SUCCEEDED(hr))
								m_man->SendMessageToParent(WM_XSHELL_NOTIFY_SHELLCOMMAND);
						}
					}
				}
			}

			if (m_pContextMenu2 != NULL)
			{
				m_pContextMenu2->Release();
				m_pContextMenu2 = NULL;
			}
		}

		pcm->Release();
	}

	pCurFolder->Release();

	return 0;
}



////////////////////////////////////////////////////////////////////////////////////////////////////
///         폴더 이동하기
/// @param  
/// @return 
/// @date   Thursday, October 20, 2011  3:43:22 PM
////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL XShellListCtrl::EnterFolder(CShellItem* pItem)
{
	m_man->SetPath(pItem->pidlFQ);
	return TRUE;
}


////////////////////////////////////////////////////////////////////////////////////////////////////
///         마우스 더블클릭 처리
/// @param  
/// @return 
/// @date   Thursday, October 20, 2011  4:15:08 PM
////////////////////////////////////////////////////////////////////////////////////////////////////
LRESULT XShellListCtrl::OnLButtonDblClk(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
	int nItem = ListView_GetNextItem(m_hWnd, -1, LVNI_FOCUSED);
	if (nItem != -1)
		DoDefault(nItem);
 	return 0;
}
void XShellListCtrl::OnReturn()
{
	int nItem = ListView_GetNextItem(m_hWnd, -1, LVNI_FOCUSED);
	if (nItem != -1)
		DoDefault(nItem);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
///         실행..
/// @param  
/// @return 
/// @date   Thursday, October 20, 2011  4:15:26 PM
////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL XShellListCtrl::DoDefault(int nItem)
{
	LVITEM lvItem;

	ZeroMemory(&lvItem, sizeof(lvItem));
	lvItem.mask = LVIF_PARAM;
	lvItem.iItem = nItem;

	if (!ListView_GetItem(m_hWnd, &lvItem))
		return FALSE;

	CShellItem* pItem = (CShellItem*) lvItem.lParam;
	if(pItem==NULL)		// ".." 폴더
		return m_man->MoveToParent();

	if(pItem->pParentFolder==NULL || pItem->pidlRel.IsEmpty())
	{ASSERT(0); return FALSE;}


	IShellFolder *pShellFolder = pItem->pParentFolder;

	if (pShellFolder == NULL)
	{
		HRESULT hr = SHGetDesktopFolder(&pShellFolder);
		if (FAILED(hr)) {ASSERT(0);return FALSE;}
	}
	else
	{
		pShellFolder->AddRef();
	}

	if (pShellFolder == NULL)
	{return FALSE;}


	// If specified element is a folder, try to display it:
	ULONG ulAttrs = SFGAO_FOLDER | SFGAO_STREAM;
	pShellFolder->GetAttributesOf(1, (const struct _ITEMIDLIST **) &pItem->pidlRel, &ulAttrs);

	// 폴더인 경우?
	if (ulAttrs & SFGAO_FOLDER)
	{
		// 압축 폴더인가?
		if(m_man->m_opt.treatCompressedFolderAsFile && ulAttrs & SFGAO_STREAM)
		{
			// .... 파일로 처리한다.
		}
		else
		{
			EnterFolder(pItem);

			pShellFolder->Release();
			return TRUE;
		}
	}

	// 파일인가? Invoke a default menu command:
	if(m_man->m_opt.executeFile)
	{
		IContextMenu *pcm;
		HRESULT hr = pShellFolder->GetUIObjectOf(m_hWnd, 1, (LPCITEMIDLIST*)&pItem->pidlRel, IID_IContextMenu, NULL, (LPVOID*)&pcm);

		if (SUCCEEDED(hr))
		{
			HMENU hPopup = CreatePopupMenu();

			if (hPopup != NULL)
			{
				hr = pcm->QueryContextMenu(hPopup, 0, 1, 0x7fff, CMF_DEFAULTONLY | CMF_EXPLORE);

				if (SUCCEEDED(hr))
				{
					UINT idCmd = ::GetMenuDefaultItem(hPopup, FALSE, 0);
					if (idCmd != 0 && idCmd != (UINT)-1)
					{
						CMINVOKECOMMANDINFO cmi;
						cmi.cbSize = sizeof(CMINVOKECOMMANDINFO);
						cmi.fMask = 0;
						cmi.hwnd = GetParent();
						cmi.lpVerb = (LPCSTR)(INT_PTR)(idCmd - 1);
						cmi.lpParameters = NULL;
						cmi.lpDirectory = NULL;
						cmi.nShow = SW_SHOWNORMAL;
						cmi.dwHotKey = 0;
						cmi.hIcon = NULL;

						hr = pcm->InvokeCommand(&cmi);

						if (SUCCEEDED(hr))
							m_man->SendMessageToParent(WM_XSHELL_NOTIFY_SHELLCOMMAND);
					}
				}
			}
			pcm->Release();
		}
	}

	pShellFolder->Release();

	return TRUE;
}


////////////////////////////////////////////////////////////////////////////////////////////////////
///         키보드(엔터키) 처리
/// @param  
/// @return 
/// @date   Thursday, October 06, 2011  2:19:35 PM
////////////////////////////////////////////////////////////////////////////////////////////////////
LRESULT XShellListCtrl::OnKeyDown(UINT /*uMsg*/, WPARAM wParam, LPARAM /*lParam*/, BOOL& bHandled)
{
	if(wParam==VK_RETURN)
	{
		OnReturn();
		return 0;
	}
	else if(wParam==VK_F2)
	{
		m_editing = TRUE;
		if(IsEditable(m_editingPos))
			ListView_EditLabel(m_hWnd, m_editingPos);
		m_editing = FALSE;
		return 0;
	}
	else if(wParam==VK_BACK)
	{
		m_man->MoveToParent();
		return 0;
	}
	else if(wParam==VK_DELETE)
	{
		DoDeleteItem();
		return 0;
	}

	bHandled = FALSE;
	return 0;
}
LRESULT XShellListCtrl::OnKeyUp(UINT /*uMsg*/, WPARAM wParam, LPARAM /*lParam*/, BOOL& bHandled)
{
	return 0;
}


////////////////////////////////////////////////////////////////////////////////////////////////////
///         셸 변화 탐지 - 연속적인 변화가 있을경우를 대비해 타이머만 세팅한다.
/// @param  
/// @return 
/// @date   Friday, October 21, 2011  2:54:28 PM
////////////////////////////////////////////////////////////////////////////////////////////////////
LRESULT XShellListCtrl::OnShellChangeNotify(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
	DWORD tick = ::GetTickCount();

	if(tick - m_tick2IgnoreShallChangeNotify < 1000)	// 무시
		return 0;

	SetTimer(TIMER_REFRESH, 100);
	return 0;
}


LRESULT XShellListCtrl::OnTimer(UINT /*uMsg*/, WPARAM wParam, LPARAM /*lParam*/, BOOL& bHandled)
{
	if(wParam==TIMER_REFRESH)
	{
		KillTimer(TIMER_REFRESH);

		// 상위 폴더가 사라진 경우등을 처리하기 위해서, enum 실패시 상위 폴더로 이동.
		for(;;)
		{
			if(m_man->ReloadCurrentChildItemAndUpdateTree())
				break;
			if(m_man->MoveToParent()==FALSE)
				break;
		}

		RefreshScreen();
	}
	else if(wParam==TIMER_DELAYPOSTSELCHANGE)
	{
		KillTimer(TIMER_DELAYPOSTSELCHANGE);
		m_man->PostMessageToParent(WM_XSHELL_NOTIFY_LISTSELCHANGED);
	}
	else
	{
		bHandled = FALSE;
	}

	return 0;
}


////////////////////////////////////////////////////////////////////////////////////////////////////
///         VK_RETURN 처리를 위해서..
/// @param  
/// @return 
/// @date   Friday, October 21, 2011  3:15:43 PM
////////////////////////////////////////////////////////////////////////////////////////////////////
LRESULT XShellListCtrl::OnGetDlgCode(UINT /*uMsg*/, WPARAM wParam, LPARAM /*lParam*/, BOOL& bHandled)
{
	if(wParam==VK_RETURN)	
		return DLGC_WANTALLKEYS;
	bHandled = FALSE;
	return 0;
}


////////////////////////////////////////////////////////////////////////////////////////////////////
///         아이템이 편집 가능한 상태인가?
/// @param  
/// @return 
/// @date   Monday, October 24, 2011  11:53:35 AM
////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL XShellListCtrl::IsEditable(int& pos)
{
	// 설정상 편집 불가능
	if(m_man->m_opt.enableListControlRename==FALSE)
		return FALSE;

	// 여러개 선택된 상태
	UINT nSelItems = ListView_GetSelectedCount(m_hWnd);
	if(nSelItems!=1) 
		return FALSE;

	pos = -1;
	pos = ListView_GetNextItem(m_hWnd, pos, LVNI_SELECTED);

	CShellItem* pItem = GetItemData(pos);
	if(pItem==NULL){ASSERT(0); return FALSE;}

	// 편집 가능한 아이템인가?
	if(pItem->isEditable)
		return TRUE;

	return FALSE;
}


CShellItem* XShellListCtrl::GetItemData(int item)
{
	LVITEM lvi;
	memset(&lvi, 0, sizeof(LVITEM));
	lvi.iItem = item;
	lvi.mask = LVIF_PARAM;
	::SendMessage(m_hWnd, LVM_GETITEM, 0, (LPARAM)&lvi);
	return (CShellItem*)lvi.lParam;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
///         이름 바꾸기 처리하기.
/// @param  
/// @return 
/// @date   Monday, October 24, 2011  2:19:53 PM
////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL XShellListCtrl::EndLabelEdit(NMLVDISPINFO* pdi)
{
	CString newName = pdi->item.pszText;

	CShellItem* pItem = GetItemData(m_editingPos);
	if(pItem==NULL) {ASSERT(0); return FALSE;}

	LPITEMIDLIST newPidl = NULL;
	HRESULT hr;

	hr = pItem->pParentFolder->SetNameOf(m_hWnd, pItem->pidlRel, newName, SHGDN_NORMAL, &newPidl);
	if(FAILED(hr))
		return FALSE;

	// 잠시후 날아올 shell change notify 무시
	ResetShellNotifyTick();

	// 성공시.. newpidl 로 바꾼다.
	pItem->ChangePidl(newPidl);
	pItem->dispName = pItem->pidlFQ.GetDispName();

	// 실제로 바뀐 이름 다시 반영
	pdi->item.pszText = pItem->dispName.GetBuffer();

	// 트리컨트롤에도 반영시켜줘야 한다.
	m_man->ApplyItemNameChangeToTree(pItem);
	//m_man->ReloadChildItemAndUpdateTree(pItem->parent);

	return TRUE;
}

void XShellListCtrl::ResetShellNotifyTick()
{
	// 잠시후 날아올 shell change notify 무시
	m_tick2IgnoreShallChangeNotify = ::GetTickCount();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
///         아이템 삭제하기.
/// @param  
/// @return 
/// @date   Monday, October 24, 2011  5:58:45 PM
////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL XShellListCtrl::DoDeleteItem()
{

	return TRUE;
}

POSITION XShellListCtrl::GetFirstSelectedItemPosition()
{
	return (POSITION)(DWORD_PTR)(1+ListView_GetNextItem(m_hWnd, -1, LVIS_SELECTED)); 
}

int	XShellListCtrl::GetNextSelectedItem(POSITION& pos)
{
	DWORD_PTR nOldPos = (DWORD_PTR)pos-1; 
	pos = (POSITION)(DWORD_PTR)(1+ListView_GetNextItem(m_hWnd, (UINT)nOldPos, LVIS_SELECTED)); 
	return (UINT)nOldPos; 
}


////////////////////////////////////////////////////////////////////////////////////////////////////
///         특정 아이템을 편집하기 시작한다.
/// @param  
/// @return 
/// @date   Tuesday, October 25, 2011  5:12:43 PM
////////////////////////////////////////////////////////////////////////////////////////////////////
void XShellListCtrl::EditLabel(int pos) 
{ 
	int count = ListView_GetItemCount(m_hWnd);
	int i;

	for(i=0;i<count;i++)
	{
		// 선택 해제
		if(ListView_GetItemState(m_hWnd, i, LVIS_SELECTED))
			ListView_SetItemState(m_hWnd, i, 0, LVIS_SELECTED)
	}

	ListView_EnsureVisible(m_hWnd, pos, TRUE);
	ListView_EditLabel(m_hWnd, pos); 
}

////////////////////////////////////////////////////////////////////////////////////////////////////
///         리스트 컨트롤 스타일 바꾸기.
/// @param  
/// @return 
/// @date   Thursday, October 27, 2011  11:31:54 AM
////////////////////////////////////////////////////////////////////////////////////////////////////
void XShellListCtrl::SetStyle(int style)
{
	DWORD old = GetWindowLong(GWL_STYLE);
	SetWindowLong(GWL_STYLE, (old & ~LVS_TYPEMASK) | style);

	if(style==LVS_REPORT)
		RefreshScreen();
}


////////////////////////////////////////////////////////////////////////////////////////////////////
///         컬럼 헤더 클릭 처리
/// @param  
/// @return 
/// @date   Thursday, October 27, 2011  12:18:15 PM
////////////////////////////////////////////////////////////////////////////////////////////////////
void XShellListCtrl::OnSortColumn(int column)
{
	if(m_sortBy == (ShellListColumns)column)
		m_sortAscending = !m_sortAscending;
	else 
	{
		m_sortAscending = TRUE;
		m_sortBy = (ShellListColumns)column;
	}

	ProcessSortIcon();

	// 소트하기
	Sort();
}

void XShellListCtrl::ProcessSortIcon()
{
	if(!(GetStyle() & LVS_REPORT))
		return;

	int column = m_sortBy;

	// sort 화살표 모양 그리기..
	HWND hWndHeader = ListView_GetHeader(m_hWnd);
	HDITEM hd;

	memset(&hd, 0, sizeof(hd));
	for(int i=0;i<=ShellList_ColumnModified;i++)
	{
		hd.mask = HDI_FORMAT;

		if(Header_GetItem(hWndHeader, i, &hd)==FALSE)
			break;

		if(i==column)
		{
			hd.fmt = hd.fmt & ~(HDF_SORTDOWN|HDF_SORTUP);			
			hd.fmt = hd.fmt | ( m_sortAscending==FALSE ? HDF_SORTDOWN : HDF_SORTUP);
			Header_SetItem(hWndHeader, i, &hd);
		}
		else
		{
			hd.fmt = hd.fmt & ~ (HDF_SORTDOWN | HDF_SORTUP);			
			Header_SetItem(hWndHeader, i, &hd);
		}
	}
}
