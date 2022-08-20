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

class XShellMan;
class CShellItem;

class XShellTreeCtrl : public CWindowImpl<XShellTreeCtrl, CWindow>
{
	friend class XShellMan;

public :
	XShellTreeCtrl();
	virtual ~XShellTreeCtrl();

public :
	BEGIN_MSG_MAP(XShellTreeCtrl)
		MESSAGE_HANDLER(OCM_NOTIFY, OnNotify)		// PARENT 에서 REFLECT 된 WM_NOTIFY 메시지 처리
		MESSAGE_HANDLER(WM_KEYDOWN, OnKeyDown)
		MESSAGE_HANDLER(WM_PAINT, OnPaint)
	END_MSG_MAP()

private :			// friend 클래스에서 호출
	BOOL			Attach(XShellMan* man, HWND hWndList);
	HTREEITEM		InsertItem(HTREEITEM hParentItem, CShellItem* pItem);
	BOOL			RemoveItem(HTREEITEM hTreeItem);
	void			Expand(HTREEITEM hItem);
	void			RemovePlusMark(HTREEITEM hItem, BOOL remove=TRUE);
	void			SortChildren(HTREEITEM hItem);
	CShellItem*		GetItemData(HTREEITEM hItem) const;
	BOOL			SelectItem(HTREEITEM hTreeItem);
	BOOL			IsEditing() { return m_editing;}
	HWND			GetEditControl();
	void			SetText(HTREEITEM hTree, LPCTSTR text);

private :
	void			InitTree();
	int				OnGetItemIcon(CShellItem* pItem, BOOL bSelected);
	void			Refresh();
	void			OnItemexpanding(NMHDR* pNMHDR);
	BOOL			IsEditable(HTREEITEM& hTreeItem);
	BOOL			EndLabelEdit(NMTVDISPINFO* pdi);

private :			// 소트 처리용
	static int CALLBACK CompareProc(LPARAM, LPARAM, LPARAM);

private :
	LRESULT			OnNotify(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
	LRESULT			OnKeyDown(UINT /*uMsg*/, WPARAM wParam, LPARAM /*lParam*/, BOOL& bHandled);

private :
	XShellMan*		m_man;
	BOOL			m_editing;
	BOOL			m_ignorePaint;


public:
	LRESULT OnPaint(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
};

