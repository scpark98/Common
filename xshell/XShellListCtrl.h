////////////////////////////////////////////////////////////////////////////////////////////////////
/// 
/// 
/// 
/// @author   parkkh
/// @date     Friday, October 14, 2011  11:36:24 AM
/// 
/// Copyright(C) 2011 Bandisoft, All rights reserved.
/// 
////////////////////////////////////////////////////////////////////////////////////////////////////


#pragma once


#include <shlobj.h>


class XShellMan;
class CShellItem;
class XShellTreeCtrl;
typedef CAtlList<CShellItem*> ShellItemList;

extern DWORD WM_USER_SHELL_CHANGE_NOTIFY;


class XShellListCtrl  : public CWindowImpl<XShellListCtrl, CWindow>
{
	friend class XShellMan;
	friend class XShellTreeCtrl;

public :
	XShellListCtrl();
	virtual ~XShellListCtrl();

public :
	BOOL			Attach(XShellMan* man, HWND hWndList);
	POSITION		GetFirstSelectedItemPosition();
	int				GetNextSelectedItem(POSITION& pos);
	CShellItem*		GetItemData(int item);
	void			SetStyle(int style);

public :
	BEGIN_MSG_MAP(XShellListCtrl)
		MESSAGE_HANDLER(OCM_NOTIFY, OnNotify)		// PARENT 에서 REFLECT 된 WM_NOTIFY 메시지 처리
		MESSAGE_HANDLER(WM_CONTEXTMENU, OnContextMenu)
		MESSAGE_HANDLER(WM_INITMENUPOPUP, HandleMenu)
		MESSAGE_HANDLER(WM_LBUTTONDBLCLK, OnLButtonDblClk)
		MESSAGE_HANDLER(WM_KEYDOWN, OnKeyDown)
		MESSAGE_HANDLER(WM_KEYUP, OnKeyUp)
		MESSAGE_HANDLER(WM_USER_SHELL_CHANGE_NOTIFY, OnShellChangeNotify)
		MESSAGE_HANDLER(WM_DESTROY, OnDestroy)
		MESSAGE_HANDLER(WM_TIMER, OnTimer)
		MESSAGE_HANDLER(WM_GETDLGCODE, OnGetDlgCode)
	END_MSG_MAP()

public :			// 상속 가능한 메쏘드
	virtual void	OnSetItemText(int nItem, CShellItem* pItem);
	virtual int		OnCompareItems(CShellItem* pItem1, CShellItem* pItem2);


private :
	enum ShellListColumns
	{
		ShellList_ColumnName = 0,
		ShellList_ColumnSize = 1,
		ShellList_ColumnType = 2,
		ShellList_ColumnModified = 3,
	};

	enum 
	{
		TIMER_REFRESH				= 1,
		TIMER_DELAYREGISTERCN		= 2,
		TIMER_DELAYPOSTSELCHANGE	= 3,
	};

private :			// friend class 에서 호출하는 놈들
	void			RefreshScreen();
	void			ClearItems();
	void			ResetShellNotifyTick();
	int				InsertItem(CShellItem* pItem);
	void			EditLabel(int pos);

private :
	void			InitList();
	HIMAGELIST		GetShellImageList(BOOL bLarge);
	void			OnSetColumns();
	int				GetItemIcon(CShellItem* pItem);
	void			InsertItems(ShellItemList& list);
	void			Sort();
	BOOL			EnterFolder(CShellItem* pItem);
	BOOL			DoDefault(int nItem);
	void			OnReturn();
	BOOL			IsEditable(int& pos);
	BOOL			EndLabelEdit(NMLVDISPINFO* pdi);
	BOOL			DoDeleteItem();
	void			OnSortColumn(int column);
	void			ProcessSortIcon();


private :
	static int CALLBACK	CompareProc(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort);


private :
	XShellMan*		m_man;
	int				m_column2sort;
	IContextMenu2*	m_pContextMenu2;
	BOOL			m_editing;
	int				m_editingPos;					// 현재 이름 바꾸는중인 아이템 위치
	DWORD			m_tick2IgnoreShallChangeNotify;
	ShellListColumns	m_sortBy;
	BOOL				m_sortAscending;

public:
	LRESULT OnLButtonDblClk(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
	LRESULT OnDestroy(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
	LRESULT OnTimer(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
	LRESULT	OnNotify(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
	LRESULT	OnContextMenu(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
	LRESULT	HandleMenu(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
	LRESULT	OnKeyUp(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
	LRESULT	OnKeyDown(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
	LRESULT	OnShellChangeNotify(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
	LRESULT OnGetDlgCode(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
};

