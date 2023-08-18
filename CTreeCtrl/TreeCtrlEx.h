#pragma once


// CTreeCtrlEx
#include <afxwin.h>
#include <afxcmn.h>
//#include <afxole.h>

#include <deque>
#include "../system/ShellImageList/ShellImageList.h"

#define MESSAGE_TREECTRLEX			WM_USER + 0x7FFF - 0x7462

class CTreeCtrlFolder
{
public:
	CTreeCtrlFolder(HTREEITEM _item, CString _fullpath, CString _folder)
	{
		item = _item;
		fullpath = _fullpath;
		folder = _folder;
	}

	HTREEITEM item;
	CString fullpath;
	CString folder;
};

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

	CString		get_fullpath(HTREEITEM hItem);
	void		select_item(CString fullpath);

	HTREEITEM	insert_special_folder(int csidl);
	void		insert_drive(CString driveName);
	void		insert_folder(HTREEITEM hParent, CString sParentPath);
	void		insert_folder(WIN32_FIND_DATA* pFindFileData);

	HTREEITEM	find_item(const CString& name);
	HTREEITEM	find_item(const CString& name, HTREEITEM root);

	void		iterate_tree(HTREEITEM hItem = NULL);
	std::deque<CTreeCtrlFolder>	iterate_tree_with_no_recursion(HTREEITEM hItem = NULL);

	void		expand_all(bool expand = true);

	//�÷� ����
	enum listctrlex_color_theme
	{
		color_theme_default = 0,
		color_theme_light_blue,
		color_theme_navy_blue,
		color_theme_dark_blue,
		color_theme_dark_gray,
	};

	void	set_color_theme(int theme, bool apply_now = true);

	//Drag&Drop �巡�� ����
	template <typename ... Types> void add_drag_images(Types... args) //(�������Ͽ� �̹���, �̱����Ͽ� �̹����� ���ʴ�� �ְ� drag�Ǵ� ������ ���� �´� �̹����� ����Ѵ�)
	{
		int n = sizeof...(args);
		int arg[] = { args... };

		for (auto id : arg)
			m_drag_images_id.push_back(id);
	}

	bool			use_drag_and_drop() { return m_use_drag_and_drop; }
	void			use_drag_and_drop(bool use_drag) { m_use_drag_and_drop = use_drag; }

protected:
	HTREEITEM	m_expandItem;	// ���������� Ȯ���� ������
	HTREEITEM	m_desktopItem;	// ����ȭ�� ������
	HTREEITEM	m_documentItem;	// ���� ������
	HTREEITEM	m_computerItem;	// �� PC ������
	std::deque<CTreeCtrlFolder> m_folder_list;

	void			thread_insert_folders(HTREEITEM hItem);

	//��Ʈ ����
	LOGFONT			m_lf;
	CFont			m_font;
	int				m_font_size;
	void			reconstruct_font();

	//�÷� ����
	COLORREF		m_crText;					//�⺻ ���ڻ�
	COLORREF		m_crTextSelected;			//���� �׸��� Ȱ��ȭ(active) ���ڻ�
	COLORREF		m_crTextSelectedInactive;	//���� �׸��� ��Ȱ��ȭ(inactive) ���ڻ�
	COLORREF		m_crTextDropHilited;
	COLORREF		m_crBack;					//�⺻ ����
	COLORREF		m_crBackSelected;
	COLORREF		m_crBackSelectedInactive;
	COLORREF		m_crBackDropHilited;
	COLORREF		m_crSelectedBorder;

	//Drag&Drop �巡�� ����
	bool			m_use_drag_and_drop = false;
	CWnd*			m_pDragWnd = NULL;			//Which ListCtrl we are dragging FROM
	CWnd*			m_pDropWnd = NULL;			//Which ListCtrl we are dropping ON
	CImageList*		m_pDragImage = NULL;		//For creating and managing the drag-image
	bool			m_bDragging = false;		//T during a drag operation
	HTREEITEM		m_DragItem = NULL;			//Index of selected item in the Tree we are dragging FROM
	//int				m_nDropIndex = -1;			//Index at which to drop item in the List we are dropping ON(drag�� ������ ��Ʈ���� ������� �����, ��ӵ� Ŭ�������� ������� ����)
	std::deque<UINT> m_drag_images_id;		//drag�� �� ����ϴ� �̹������� resource id ����(�������Ͽ� �̹���, �̱����Ͽ� �̹����� ���ʴ�� �ְ� drag�Ǵ� ������ ���� �´� �̹����� ����Ѵ�)
	void			DroppedHandler(CWnd* pDragWnd, CWnd* pDropWnd);

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
	afx_msg void OnTvnItemexpanding(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnNMClick(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnTvnBegindrag(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnNMDblclk(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
	afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
};


