#pragma once


// CTreeCtrlEx
#include <afxwin.h>
#include <afxcmn.h>

#include "../system/ShellImageList/ShellImageList.h"

class CTreeCtrlEx : public CTreeCtrl
{
	DECLARE_DYNAMIC(CTreeCtrlEx)

public:
	CTreeCtrlEx();
	virtual ~CTreeCtrlEx();

	CShellImageList* m_pShellImageList = NULL;
	void		SetCShellImageList(CShellImageList* pShellImageList) { m_pShellImageList = pShellImageList; }

	bool		m_use_own_imagelist = true;
	void		set_use_own_imagelist(bool use) { m_use_own_imagelist = use; }

	bool		m_is_shell_tree = false;
	bool		m_is_shell_tree_local = true;
	bool		is_is_shell_tree() { return m_is_shell_tree; }
	bool		is_is_shell_tree_local() { return m_is_shell_tree_local; }

	void		set_as_shell_tree(bool is_local = true);

protected:
	DECLARE_MESSAGE_MAP()
	virtual void PreSubclassWindow();
public:
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	afx_msg void OnActivate(UINT nState, CWnd* pWndOther, BOOL bMinimized);
	afx_msg BOOL OnEraseBkgnd(CDC* pDC);
	afx_msg void OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags);
	afx_msg void OnPaint();
	afx_msg void OnTvnSelchanged(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnWindowPosChanged(WINDOWPOS* lpwndpos);
};


