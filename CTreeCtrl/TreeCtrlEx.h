#pragma once


// CTreeCtrlEx
#include <afxwin.h>
#include <afxcmn.h>

#include "../system/ShellImageList/ShellImageList.h"

#define MESSAGE_TREECTRLEX			WM_USER + 0x7FFF - 0x7462

class CTreeCtrlExMessage
{
public:
	CTreeCtrlExMessage(CWnd* _this, int _message, CWnd* _pTarget = NULL)
	{
		pThis = _this;
		message = _message;
		pTarget = _pTarget;
	}

	CWnd*	pThis = NULL;
	CWnd*	pTarget = NULL;
	int		message;
};

class CTreeCtrlEx : public CTreeCtrl
{
	DECLARE_DYNAMIC(CTreeCtrlEx)

public:
	CTreeCtrlEx();
	virtual ~CTreeCtrlEx();

	enum TreeCtrlExMsgs
	{
		message_selchanged = 0,
	};

	CShellImageList* m_pShellImageList = NULL;
	void		set_shell_imagelist(CShellImageList* pShellImageList) { m_pShellImageList = pShellImageList; }

	bool		m_use_own_imagelist = true;
	void		set_use_own_imagelist(bool use) { m_use_own_imagelist = use; }

	bool		m_is_shell_treectrl = false;
	bool		m_is_shell_treectrl_local = true;
	bool		is_is_shell_treectrl() { return m_is_shell_treectrl; }
	bool		is_is_shell_treectrl_local() { return m_is_shell_treectrl_local; }

	void		set_as_shell_treectrl(bool is_local = true);

	HTREEITEM	insert_special_folder(int csidl);
	void		insert_drive(CString driveName);
	void		insert_folder(WIN32_FIND_DATA* pFindFileData);
	void		insert_folder(HTREEITEM hParent, CString sParentPath);
	CString		get_fullpath(HTREEITEM hItem);
	void		add_children_folders(HTREEITEM hParent);

private:
	HTREEITEM	m_expandItem;	// 마지막으로 확장한 아이템
	HTREEITEM	m_desktopItem;	// 바탕화면 아이템
	HTREEITEM	m_documentItem;	// 문서 아이템
	HTREEITEM	m_computerItem;	// 내 PC 아이템

protected:
	DECLARE_MESSAGE_MAP()
	virtual void PreSubclassWindow();
public:
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	afx_msg void OnActivate(UINT nState, CWnd* pWndOther, BOOL bMinimized);
	afx_msg BOOL OnEraseBkgnd(CDC* pDC);
	afx_msg void OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags);
	//afx_msg void OnPaint();
	afx_msg void OnTvnSelchanged(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnWindowPosChanged(WINDOWPOS* lpwndpos);
	afx_msg void OnTvnItemexpanding(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnNMClick(NMHDR* pNMHDR, LRESULT* pResult);
};


