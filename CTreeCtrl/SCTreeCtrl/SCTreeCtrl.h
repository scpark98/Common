#pragma once

/*
* �⺻ CTreeCtrl���� �������� �ʴ� ��ɵ��� Ȯ���ϱ� ���� ����.
	- ���� load, save (txt, json?)
	  (txt�� ��� depth�� tab���� ǥ����)

[������ Ž����� ���� ���� Ʈ���� ����� ���]
	- dlg.h�� CShellImageList�� �ν��Ͻ��� �����ϰ�,
	  dlg.cpp���� m_ShellImageList.Initialize(); �� ���� �ʱ�ȭ �� ��,
	  m_tree0.set_as_shell_treectrl(&m_ShellImageList, true); �� �����ϸ� ��. 2��° ���ڴ� local���� remote���� ���.

	- ���� drag&drop ����� �ʿ��ϴٸ�
	  m_tree0.use_drag_and_drop(true);
	  m_tree0.add_drag_images(IDB_DRAG_ONE_FILE, IDB_DRAG_MULTI_FILES); �� ���� �巡�׿� ���� �̹��� ���� ����.

	- C����̺갡 ���õǰ� �Ϸ���
	  m_tree0.select_item(_T("C:\\"));

[�Ϲ� �����͸� Ʈ���� ǥ���� ���]

*/


// CSCTreeCtrl
#include <afxwin.h>
#include <afxcmn.h>
#include <deque>

#include "../../system/ShellImageList/ShellImageList.h"
#include "../../ui/theme/theme.h"

static const UINT Message_CSCTreeCtrl = ::RegisterWindowMessage(_T("MessageString_CSCTreeCtrl"));

//tree�� �� �׸��� folder ���� ���� ����
class CSCTreeCtrlFolder
{
public:
	CSCTreeCtrlFolder(HTREEITEM _item, CString _fullpath, CString _folder)
	{
		item = _item;
		fullpath = _fullpath;
		folder = _folder;
	}

	HTREEITEM item;
	CString fullpath;
	CString folder;
};

class CSCTreeCtrlMessage
{
public:
	CSCTreeCtrlMessage(CWnd* _this, int _message, CWnd* _pTarget = NULL)
	{
		pThis = _this;
		message = _message;
		pTarget = _pTarget;
	}

	CWnd*	pThis = NULL;
	CWnd*	pTarget = NULL;
	int		message;
};

class CSCTreeCtrl : public CTreeCtrl
{
	DECLARE_DYNAMIC(CSCTreeCtrl)

public:
	CSCTreeCtrl();
	virtual ~CSCTreeCtrl();

	enum CSCTreeCtrlMsgs
	{
		message_selchanged = 0,
		message_drag_and_drop,
	};

	//for test
	bool		use_default_paint = true;

	CShellImageList* m_pShellImageList = NULL;
	void		set_shell_imagelist(CShellImageList* pShellImageList) { m_pShellImageList = pShellImageList; }

	//��ü �̹�������Ʈ�� �� ������
	void		set_use_own_imagelist(bool use, int image_size = 16);
	void		set_image_size(int image_size = 16);

	void		set_use_checkbox(bool checkbox = true);

	//ico ���ϵ��� imagelist�� �߰�
	template <typename ... Types> void set_imagelist(Types... ids)
	{
		int n = sizeof...(ids);
		int arg[] = { ids... };

		for (auto id : arg)
		{
			m_image_IDs.push_back(id);
		}

		create_imagelist();
	}

	bool		m_is_shell_treectrl = false;
	bool		m_is_shell_treectrl_local = true;
	bool		is_is_shell_treectrl() { return m_is_shell_treectrl; }
	bool		is_is_shell_treectrl_local() { return m_is_shell_treectrl_local; }

	void		set_as_shell_treectrl(CShellImageList* pShellImageList, bool is_local);

	CString		get_fullpath(HTREEITEM hItem);
	void		select_folder(CString fullpath);

	HTREEITEM	insert_special_folder(int csidl);
	void		insert_drive(CString driveName);
	void		insert_folder(HTREEITEM hParent, CString sParentPath);
	void		insert_folder(WIN32_FIND_DATA* pFindFileData);

	bool		load(CString file);
	bool		save(CString file);
	bool		load_from_string(CString text);

	//HTREEITEM	find_item(const CString& name);
	HTREEITEM	find_item(const CString& name, HTREEITEM root = NULL);
	CString		get_selected_item_text(bool include_parent = false);

	//recursive traverse
	void		iterate_tree(HTREEITEM hItem = NULL);
	//not recursive, top-down traverse
	void		iterate_tree_in_order(HTREEITEM hItem = NULL);
	//not recursive traverse using stack, not guarantee the original order
	std::deque<CSCTreeCtrlFolder>	iterate_tree_with_no_recursion(HTREEITEM hItem = NULL);

	void		expand_all(bool expand = true);

	//HitTest()�� Ŭ���� hItem�� expand button, check, icon, label ������ �Ǻ��ϴ� �Լ��ε�
	//OnPaint()���� CTreeCtrl::OnPaint()�� ������� �ʰ� ���� �׷��� ����
	//HitTest()�� �״�� ����� �� �����Ƿ� ���� �Լ��� �ۼ�.
	//���� OnPaint()���� �׷��ִ� ���� �ƴ� CustonDraw()���� �׷��ִ� ��쵵 ���� ���������� Ȯ�� �ʿ�.
	HTREEITEM	hit_test(UINT* nFlags = NULL);

	//�÷� ����
	enum listctrlex_color_theme
	{
		color_theme_default = 0,
		color_theme_light_blue,
		color_theme_navy_blue,
		color_theme_dark_blue,
		color_theme_dark_gray,
		color_theme_dark,
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
	HTREEITEM		m_DragItem = NULL;			//drag item in the Tree we are dragging FROM
	HTREEITEM		m_DropItem = NULL;			//dropped item on the Tree we dropped ON
	int				m_nDropIndex = -1;			//Index at which to drop item in the List we are dropping ON(drag�� ������ ��Ʈ���� ������� �����, ��ӵ� Ŭ�������� ������� ����)

	BOOL			MoveTreeItem(CTreeCtrl* pTree, HTREEITEM hSrcItem, HTREEITEM hDestItem);
	BOOL			MoveChildTreeItem(CTreeCtrl* pTree, HTREEITEM hChildItem, HTREEITEM hDestItem);

protected:
	CTheme			m_theme;
	bool			m_theme_initialized = false;
	void			theme_init();
	void			draw_checkbox(CDC* pDC, CRect r, int check_state);

	CImageList		m_imagelist;			//��ü �̹��� ����Ʈ
	std::deque<UINT>m_image_IDs;			//�̹��� ����Ʈ�� �߰��� resource id. �̸� �����ϴ� ������ icon size ���� ���浵 �����ϱ� ����.
	int				m_image_size = -1;		//-1(not yet set), 16 or 32?
	bool			m_use_own_imagelist = false;	//��ü �̹�������Ʈ�� �� ������
	void			create_imagelist();

	HTREEITEM		m_expandItem;			// ���������� Ȯ���� ������
	HTREEITEM		m_desktopItem;			// ����ȭ�� ������
	HTREEITEM		m_documentItem;			// ���� ������
	HTREEITEM		m_computerItem;			// �� PC ������
	std::deque<CSCTreeCtrlFolder> m_folder_list;

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
	CWnd*			m_pDragWnd = NULL;			//Which wnd we are dragging FROM
	CWnd*			m_pDropWnd = NULL;			//Which wnd we are dropping ON
	CImageList*		m_pDragImage = NULL;		//For creating and managing the drag-image
	bool			m_bDragging = false;		//T during a drag operation
	std::deque<UINT> m_drag_images_id;			//drag�� �� ����ϴ� �̹������� resource id ����(�������Ͽ� �̹���, �̱����Ͽ� �̹����� ���ʴ�� �ְ� drag�Ǵ� ������ ���� �´� �̹����� ����Ѵ�)
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
	afx_msg BOOL OnNMClick(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnTvnBegindrag(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnNMDblclk(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
	afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
};


