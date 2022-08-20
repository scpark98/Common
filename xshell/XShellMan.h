////////////////////////////////////////////////////////////////////////////////////////////////////
/// 
/// �� Ž���� ��Ʈ�� ó�� ���
/// 
/// - ����Ʈ�� Ʈ�� ��Ʈ���� UI ó�� �κп��� �����ϰ�, ���� ���� Ʈ�� ������ ����Ÿ�� ����
///	  �� Ŭ�������� ó���Ѵ�.
///
/// ����	
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


extern DWORD WM_XSHELL_NOTIFY_PATHCHANGED;		// ��ΰ� �ٲ������ PARENT �� NOTIFY
extern DWORD WM_XSHELL_NOTIFY_SHELLCOMMAND;		// Ž���� �޴��� ������ ó������
extern DWORD WM_XSHELL_NOTIFY_LISTSELCHANGED;	// ����Ʈ ��Ʈ���� ������ �ٲ����.


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
		// ���ϵ� ������ ���� ����
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

	// ���� �����߿��� �̸��� ������ ã�´�.
	CShellItem*		FindChildrenFolder(LPCWSTR dispName)
	{
		POSITION p;
		p = this->childrenFolder.GetHeadPosition();
		while(p)
		{
			CShellItem* pItem = this->childrenFolder.GetNext(p);

			// �̸����� ���ؼ� ������ ã���Ŵ�..
			if(pItem->dispName==dispName)
			{
				return pItem;
			}
		}
		return NULL;
	}

	// �̸����� �ٲپ������ �ٲ� pidl �� �ٲٱ�.
	BOOL			ChangePidl(LPITEMIDLIST newPidl)
	{
		if(newPidl==NULL){ASSERT(0); return FALSE;}

		// ���� pidl
		pidlRel.Attach(newPidl);

		// ���� ������ ���� ���? (���� desktop?)
		if(parent==NULL)
		{pidlFQ = pidlRel;return TRUE;}

		pidlFQ.Attach(XPidl::ConcatPidls(parent->pidlFQ, newPidl));
		return TRUE;
	}


	HTREEITEM		hTree;
	ShellItemList	childrenFolder;		// ���� ������ - Ʈ����Ʈ�ѿ��� �̳�鸸 ǥ��
	ShellItemList	childrenNonFolder;
	XPidl			pidlRel;			// pParentFolder �� ���� relative ��
	XPidl			pidlFQ;				// full path ?
	CShellItem*		parent;
	CString			dispName;
	IShellFolder*	pParentFolder;		// parent �� shell folder
	BOOL			hasChildren;		// ���ϵ� �������� �ֳ�? (tree �� ������ + ǥ�� ������ ���� �Ҷ� ���)
	BOOL			isShared;			// SFGAO_SHARE �Ӽ��� ������ �ֳ�?
	BOOL			isTreeItem;			// tree ��Ʈ�ѿ� ������ �������ΰ�(�����ΰ�?)
	BOOL			isEditable;
	BOOL			bDeletedItem;		// ���� üũ�� �ӽ� ����

	INT64			fileSize;			// ������ ��� ���� ũ�� - ����Ʈ ��Ʈ�ѿ��� ��Ʈ�Ҷ��� ���Ǳ� ������ ��ҿ��� ��ȿ�� ���� ��� ���� �ʴ�.
	CString			typeName;			// ���� ���� - ""
	FILETIME		fileTime;			// ���� �ð� - ""
};

// ��ƿ
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

// �ɼ� ó��
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

	BOOL	treatCompressedFolderAsFile;	// ����� ����(.zip, .cab)�� ������ �ƴ� ���Ϸ� ó���ϱ�
	BOOL	executeFile;					// ���� Ŭ�����̳� ���ʹ����� ���� �����Ű��
	BOOL	showDotDotAtListControl;		// .. ���� �����ֱ�?

	BOOL	hideFloppy;						// �÷��� ����̺� �����
	BOOL	hideRecycleBin;					// ������ �����
	BOOL	hideControlPanel;				// ������ ����� - ���� �۵����� ����.

	BOOL	enableListControlContextMenu;	// ���콺 ��Ŭ�� ����?
	BOOL	enableTreeControlContextMenu;

	BOOL	enableListControlRename;		// �̸� �ٲٱ� ����?
	BOOL	enableTreeControlRename;		// 

	BOOL	enableListControlDelete;		// ����
	BOOL	enableTreeControlDelete;

	CString	textListColumnName;				// ����Ʈ ��Ʈ�� �÷� �̸�
	CString	textListColumnSize;
	CString	textListColumnType;
	CString	textListColumnModified;

	int		columnWidthName;				// ����Ʈ ��Ʈ�� �÷� ũ��
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

private :				// friend ���� ȣ���ϴ� ���
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
	CShellItem*			m_currentItem;		// ���� ��Ŀ�� �ް� �ִ� ������
	XShellManOpt		m_opt;
	HWND				m_hWndParent;		// notify ���� parent ������
	CString				m_sCurPath;			// ���� ���� ��ü ��� ( ""�� �� �ִ�.)
	CString				m_sCurDispName;		// ���� ������ �̸�

	LPITEMIDLIST		m_pidlRecycleBin;	// ������
	XPidl				m_pidlControlPanel;	// ������

	BOOL				m_bIgnoreNotify;	// tree, list �� ������ notify �� �����Ѵ�.
	ULONG				m_ulSHChangeNotifyRegister;

};


