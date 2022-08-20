////////////////////////////////////////////////////////////////////////////////////////////////////
/// 
/// 셸 탐색기 컨트롤 처리 모듈
/// 
/// - 리스트와 트리 컨트롤은 UI 처리 부분에만 관여하고, 실제 셸의 트리 구조등 데이타는 전부
///	  이 클래스에서 처리한다.
///
/// 참고	
///		- CMFCShellManager class
///		- http://www.codeproject.com/KB/shell/citemidlist.aspx
///		- fly explorer 1.0 src
/// 
/// 
/// @author   parkkh
/// @date     Friday, October 14, 2011  11:36:24 AM
/// 
/// Copyright(C) 2011 Bandisoft, All rights reserved.
/// 
////////////////////////////////////////////////////////////////////////////////////////////////////

#pragma once

#include "XShellListCtrl.h"
#include "XShellTreeCtrl.h"
#include "XShellPidl.h"


extern DWORD WM_XSHELL_NOTIFY_PATHCHANGED;		// 경로가 바뀌었을때 PARENT 로 NOTIFY
extern DWORD WM_XSHELL_NOTIFY_SHELLCOMMAND;		// 탐색기 메뉴를 좀전에 처리했음
extern DWORD WM_XSHELL_NOTIFY_LISTSELCHANGED;	// 리스트 컨트롤의 선택이 바뀌었다.


class CShellItem;
typedef CAtlList<CShellItem*> ShellItemList;

class CShellItem
{
public :
	CShellItem()
	{
		hTree = NULL;
		parent = NULL;
		pParentFolder = NULL;
		hasChildren = FALSE;
		isShared = FALSE;
		isTreeItem = FALSE;
		bDeletedItem = FALSE;
		isEditable = FALSE;
		fileSize = -1;
		fileTime.dwHighDateTime = fileTime.dwLowDateTime = 0;
	}

	~CShellItem()
	{
		Clear();
	}
	void Clear()
	{
		// 차일드 아이템 전부 삭제
		POSITION p = childrenFolder.GetHeadPosition();
		while(p)
			delete childrenFolder.GetNext(p);
		childrenFolder.RemoveAll();

		ClearNonFolder();

		hTree = NULL;
		pidlRel.Clear();
		pidlFQ.Clear();
		dispName.Empty();
		if(pParentFolder)
		{pParentFolder->Release(); pParentFolder = NULL;}
	}

	void			ClearNonFolder()
	{
		POSITION p = childrenNonFolder.GetHeadPosition();
		while(p)
			delete childrenNonFolder.GetNext(p);
		childrenNonFolder.RemoveAll();
	}

	// 하위 폴더중에서 이름이 같은걸 찾는다.
	CShellItem*		FindChildrenFolder(LPCWSTR dispName)
	{
		POSITION p;
		p = this->childrenFolder.GetHeadPosition();
		while(p)
		{
			CShellItem* pItem = this->childrenFolder.GetNext(p);

			// 이름으로 비교해서 같으면 찾은거다..
			if(pItem->dispName==dispName)
			{
				return pItem;
			}
		}
		return NULL;
	}

	// 이름등을 바꾸었을경우 바뀐 pidl 로 바꾸기.
	BOOL			ChangePidl(LPITEMIDLIST newPidl)
	{
		if(newPidl==NULL){ASSERT(0); return FALSE;}

		// 현재 pidl
		pidlRel.Attach(newPidl);

		// 상위 폴더가 없는 경우? (설마 desktop?)
		if(parent==NULL)
		{pidlFQ = pidlRel;return TRUE;}

		pidlFQ.Attach(XPidl::ConcatPidls(parent->pidlFQ, newPidl));
		return TRUE;
	}


	HTREEITEM		hTree;
	ShellItemList	childrenFolder;		// 폴더 아이템 - 트리컨트롤에는 이놈들만 표시
	ShellItemList	childrenNonFolder;
	XPidl			pidlRel;			// pParentFolder 에 대한 relative 값
	XPidl			pidlFQ;				// full path ?
	CShellItem*		parent;
	CString			dispName;
	IShellFolder*	pParentFolder;		// parent 의 shell folder
	BOOL			hasChildren;		// 차일드 아이템이 있나? (tree 에 넣을때 + 표시 넣을까 말까 할때 사용)
	BOOL			isShared;			// SFGAO_SHARE 속성을 가지고 있나?
	BOOL			isTreeItem;			// tree 컨트롤에 보여줄 아이템인가(폴더인가?)
	BOOL			isEditable;
	BOOL			bDeletedItem;		// 삭제 체크용 임시 변수

	INT64			fileSize;			// 파일일 경우 파일 크기 - 리스트 컨트롤에서 소트할때만 사용되기 때문에 평소에는 유효한 값이 들어 있지 않다.
	CString			typeName;			// 파일 설명 - ""
	FILETIME		fileTime;			// 파일 시간 - ""
};

// 유틸
class XWaitCursor
{
public :
	XWaitCursor()
	{
		hcurWait = ::LoadCursor(NULL, IDC_WAIT);
		hcurPrev = ::SetCursor(hcurWait);
	}
	~XWaitCursor()
	{Restore();}
	void	Restore()
	{SetCursor(hcurPrev);}
	HCURSOR hcurWait;
	HCURSOR hcurPrev;
};

// 옵션 처리
struct XShellManOpt
{
	XShellManOpt()
	{
		treatCompressedFolderAsFile = TRUE;
		hideFloppy = TRUE;
		hideRecycleBin = TRUE;
		hideControlPanel = TRUE;		
		executeFile = TRUE;
		showDotDotAtListControl = FALSE;

		enableListControlContextMenu = TRUE;
		enableTreeControlContextMenu = TRUE;

		enableListControlRename = TRUE;
		enableTreeControlRename = TRUE;

		enableListControlDelete = TRUE;
		enableTreeControlDelete = TRUE;

		textListColumnName = _T("Name");
		textListColumnSize = _T("Size");
		textListColumnType = _T("Type");
		textListColumnModified = _T("Modified");

		columnWidthName = 200;
		columnWidthSize = 70;
		columnWidthType = 100;
		columnWidthModified = 200;

	}

	BOOL	treatCompressedFolderAsFile;	// 압축된 폴더(.zip, .cab)는 폴더가 아닌 파일로 처리하기
	BOOL	executeFile;					// 더블 클릭시이나 엔터누를때 파일 실행시키기
	BOOL	showDotDotAtListControl;		// .. 폴더 보여주까?

	BOOL	hideFloppy;						// 플로피 드라이브 숨기기
	BOOL	hideRecycleBin;					// 휴지통 숨기기
	BOOL	hideControlPanel;				// 제어판 숨기기 - 정상 작동하지 않음.

	BOOL	enableListControlContextMenu;	// 마우스 우클릭 가능?
	BOOL	enableTreeControlContextMenu;

	BOOL	enableListControlRename;		// 이름 바꾸기 가능?
	BOOL	enableTreeControlRename;		// 

	BOOL	enableListControlDelete;		// 삭제
	BOOL	enableTreeControlDelete;

	CString	textListColumnName;				// 리스트 컨트롤 컬럼 이름
	CString	textListColumnSize;
	CString	textListColumnType;
	CString	textListColumnModified;

	int		columnWidthName;				// 리스트 컨트롤 컬럼 크기
	int		columnWidthSize;
	int		columnWidthType;
	int		columnWidthModified;
};


class XShellMan
{
	friend XShellTreeCtrl;
	friend XShellListCtrl;

public :
	XShellMan();
	~XShellMan();

	void				SetOpt(XShellManOpt opt) { m_opt = opt; }
	BOOL				Create(HWND hWndParent, HWND hWndTreeCtrl=NULL, HWND hWndListCtrl=NULL);
	void				Destroy();
	LPCTSTR				GetCurrentPath();
	LPCTSTR				GetCurrentDispName();
	BOOL				SetPath(LPCTSTR pathName);
	BOOL				SetPath2Desktop();
	BOOL				SetPath(XPidl& pidl);
	BOOL				MoveToParent();
	BOOL				IsTreeCtrlEditMessage(WPARAM keyCode);
	XShellListCtrl*		GetListCtrl() { return m_list; }
	XShellTreeCtrl*		GetTreeCtrl() { return m_tree; }
	BOOL				CreateNewFolderAtTree(LPCTSTR folderName);
	BOOL				CreateNewFolderAtList(LPCTSTR folderName);

private :				// friend 에서 호출하는 놈들
	BOOL				ReloadCurrentChildItemAndUpdateTree();
	BOOL				ReloadChildItemAndUpdateTree(CShellItem* pItem);
	BOOL				OnTreeExpand(HTREEITEM hParentItem);
	void				OnTreeSelChanged(CShellItem* pItem);
	CShellItem*			GetCurrentItem() { return m_currentItem; }
	LRESULT				SendMessageToParent(UINT msg);
	void				PostMessageToParent(UINT msg);
	void				RegisterChangeNotify(BOOL reg);
	void				GetCurrentPathFromCurPidl();
	void				ApplyItemNameChangeToTree(CShellItem* pItem);

private :
	BOOL				CreateRoot();
	BOOL				EnumObjects(ShellItemList& listFolder, ShellItemList& listNonFolder, CShellItem* parent, LPSHELLFOLDER pParentFolder, XPidl& pidlParent);
	CShellItem*			AddToList(ShellItemList& list, CShellItem* parent, LPSHELLFOLDER pParentFolder, XPidl& pidlParent, LPITEMIDLIST pidlTemp, DWORD dwAttribs, BOOL isTreeItem);
	BOOL				UpdateTreeItemList(CShellItem* pParentItem, IShellFolder* pParentFolder, ShellItemList& newList);
	void				OnSelChanged(CShellItem* pItem);
	void				SetDialogFocus(HWND hwndControl);
	CString				AddPath(CString sLeft, CString sRight);
	LPSHELLFOLDER		GetFolder(LPSHELLFOLDER pParentFolder, LPITEMIDLIST pidl);
	CShellItem*			CreateNewFolder(LPCTSTR folderName);


private :
	BOOL				m_created;
	XShellListCtrl*		m_list;
	XShellTreeCtrl*		m_tree;
	LPMALLOC			m_malloc;
	CShellItem*			m_root;
	CShellItem*			m_currentItem;		// 현재 포커스 받고 있는 아이템
	XShellManOpt		m_opt;
	HWND				m_hWndParent;		// notify 받을 parent 윈도우
	CString				m_sCurPath;			// 현재 폴더 전체 경로 ( ""일 수 있다.)
	CString				m_sCurDispName;		// 현재 폴더의 이름

	LPITEMIDLIST		m_pidlRecycleBin;	// 휴지통
	XPidl				m_pidlControlPanel;	// 제어판

	BOOL				m_bIgnoreNotify;	// tree, list 가 보내온 notify 를 무시한다.
	ULONG				m_ulSHChangeNotifyRegister;

};


