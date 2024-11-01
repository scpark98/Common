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

[���ǻ���]
	- ���ν�ũ�Ѱ� ���õ� ó���� �������� ����. Ư�� �������� ��.
	- ������Ž���⵵ Ʈ���信�� ���ν�ũ���� ���ν�ũ�ѷ� ������.
	- 
*/


// CSCTreeCtrl
#include <afxwin.h>
#include <afxcmn.h>
#include <deque>

#include "../../system/ShellImageList/ShellImageList.h"
#include "../../ui/theme/theme.h"
#include "../../CMenu/SCMenuBar/SCMenu.h"
#include "../../colors.h"

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

	CSCColorTheme	m_theme = CSCColorTheme(this);

	enum CSCTreeCtrlMsgs
	{
		//message_selchanged = 0,		//TVN_SELCHANGED�� �̿��ϹǷ� �����Ѵ�.
		message_drag_and_drop = 0,
		message_request_folder_list,	//remote�� ��� OnTvnItemexpanding() �޽����� �߻��ϸ� remote�� ��������� �޾Ƽ� �־���� �Ѵ�.
	};


	int			get_indent_size() { return m_indent_size; }
	void		set_indent_size(int size) { m_indent_size = size; Invalidate(); }

	//��Ÿ�� ����
	void		full_row_selection(bool full_row);
	void		has_line(bool line);
	void		use_checkbox(bool use = true);

	//Ȯ��, ��� ��ư ǥ�� ����
	void		use_expand_button(bool use = true);

	void		show_area(bool show) { m_show_area = show; Invalidate(); }

	CShellImageList* m_pShellImageList = NULL;
	void		set_shell_imagelist(CShellImageList* pShellImageList) { m_pShellImageList = pShellImageList; }

	//��ü �̹�������Ʈ�� �� ������
	void		set_use_own_imagelist(bool use, int image_size = 16);
	void		set_image_size(int image_size = 16);


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
	bool		m_is_local = true;
	bool		is_shell_treectrl() { return m_is_shell_treectrl; }
	bool		is_local() { return m_is_local; }

	void		set_as_shell_treectrl(CShellImageList* pShellImageList, bool is_local);

	//����̺� ������ �ٽ� �о���δ�.
	void		refresh();

	//hItem�� NULL�̸� ���� ���õ� ������ fullpath return.
	CString		get_fullpath(HTREEITEM hItem = NULL);
	void		set_path(CString fullpath);

	HTREEITEM	insert_special_folder(int csidl);
	void		insert_drive(CString driveName);
	void		insert_folder(HTREEITEM hParent, CString sParentPath);
	void		insert_folder(WIN32_FIND_DATA* pFindFileData, bool has_children = true);

	//local�̸� drive_list�� NULL�� �ְ� remote�̸� ���� ����Ʈ�� �ְ� ���Ž�Ų��.
	void		update_drive_list(CString thisPC, std::deque<CString>* drive_list = NULL);

	bool		load(CString file);
	bool		save(CString file);
	bool		load_from_string(CString text);

	//HTREEITEM	find_item(const CString& name);
	//hItem ��ġ���� child, sibling���� Ž���Ͽ� label�� ã�´�. 
	HTREEITEM	find_item(const CString& label, HTREEITEM hItem = NULL);
	CString		get_selected_item_text(bool include_parent = false);

	//�ش� �������� ��ҵǼ� ������ �ʴ� ��������(height�� ������ ���ϵȴ�.)
	bool		is_visible_item(HTREEITEM hItem);

	//recursive traverse
	void		iterate_tree(HTREEITEM hItem = NULL);
	//not recursive, top-down traverse
	void		iterate_tree_in_order(HTREEITEM hItem = NULL);
	//not recursive traverse using stack, not guarantee the original order
	std::deque<CSCTreeCtrlFolder>	iterate_tree_with_no_recursion(HTREEITEM hItem = NULL);

	void		expand_all(bool expand = true);

	void		set_color_theme(int theme);
	void		set_text_color(Gdiplus::Color text_color) { m_theme.cr_text = text_color; Invalidate(); }
	void		set_back_color(Gdiplus::Color back_color) { m_theme.cr_back = back_color; Invalidate(); }
	//CTreeCtrl���� �����ϴ� �⺻ �Լ� override
	void		SetTextColor(Gdiplus::Color text_color) { set_text_color(text_color); }
	void		SetBkColor(Gdiplus::Color back_color) { set_back_color(back_color); }

	//Drag&Drop �巡�� ����
	template <typename ... Types> void add_drag_images(Types... args) //(�������Ͽ� �̹���, �̱����Ͽ� �̹����� ���ʴ�� �ְ� drag�Ǵ� ������ ���� �´� �̹����� ����Ѵ�)
	{
		int n = sizeof...(args);
		int arg[] = { args... };

		for (auto id : arg)
			m_drag_images_id.push_back(id);
	}

	void			set_use_popup_menu(bool use) { m_use_popup_menu = use; }

	bool			get_use_drag_and_drop() { return m_use_drag_and_drop; }
	void			set_use_drag_and_drop(bool use_drag) { m_use_drag_and_drop = use_drag; }
	HTREEITEM		m_DragItem = NULL;			//drag�Ǵ� ������
	HTREEITEM		m_DropItem = NULL;			//drop�� ������
	int				m_nDropIndex = -1;			//drop�� ��Ʈ���� CListCtrl�� �� �� �ε���(drag�� ������ ��Ʈ���� ������� �����, ��ӵ� Ŭ�������� ������� ����)

	BOOL			move_tree_item(CTreeCtrl* pTree, HTREEITEM hSrcItem, HTREEITEM hDestItem);
	BOOL			move_child_tree_item(CTreeCtrl* pTree, HTREEITEM hChildItem, HTREEITEM hDestItem);

	//20240801 scpark ������ ����� �ڵ� �߰�
	void			edit_item(HTREEITEM hItem = NULL);
	void			edit_end(bool valid = true);
	HTREEITEM		get_recent_edit_item() { return m_edit_item; }
	CString			get_edit_old_text() { return m_edit_old_text; }		//���� �� �ؽ�Ʈ
	CString			get_edit_new_text() { return m_edit_new_text; }	//���� �� �ؽ�Ʈ
	void			undo_edit_label();									//���� ���� �ؽ�Ʈ�� �ǵ�����.(���� ��� ���� ���̺��� ���ϸ��̰� ���ϸ� ������ ������ ��� �� �� �ִ�.)
	//CTreeCtrl::GetEditControl()�� override.
	CEdit*			GetEditControl() { return get_edit_control(); }
	CEdit*			get_edit_control() { return m_pEdit; }
	long			m_last_clicked_time = 0;
	HTREEITEM		m_last_clicked_item;

	//��Ʈ ����
	LOGFONT			get_log_font() { return m_lf; }
	void			set_log_font(LOGFONT lf);
	int				get_font_size();
	void			set_font_name(LPCTSTR sFontname, BYTE byCharSet = DEFAULT_CHARSET);
	void			set_font_size(int font_size);
	void			enlarge_font_size(bool enlarge);
	void			set_font_bold(bool bold = true);
	void			set_font_italic(bool italic = true);


protected:
	enum TIMER_ID
	{
		timer_expand_for_drop = 0,
	};

	//popup menu
	enum POPUP_MENU_ID
	{
		//Resource�� menu id�� �ƴ� ����� ���� id�� ����Ѵٸ� ������ ��.
		//Ư�� Resource�� menu������ separator�� id�� 0�̹Ƿ�
		//����ڰ� �߰��� �޴� id�� ���� 0���� �������� ����!
		menu_add_item = WM_USER + 473,
		menu_rename_item,
		menu_delete_item,
		menu_property,
	};
	CSCMenu			m_menu;
	bool			m_use_popup_menu = false;
	LRESULT			OnMessageCSCMenu(WPARAM wParam, LPARAM lParam);

	//���� �׸��� �߰��Ѵ�. label�� ""�̸� �⺻�̸����� �߰��� �� edit_item() ȣ��.
	//�� ����, �� �׸��� �̹� �����ϸ� �ڿ� ���ڸ� �������� �ٿ���� �Ѵ�.
	void			add_sub_item(HTREEITEM hParent = NULL, CString label = _T(""));
	//�־��� �׸��� label�� �����Ѵ�.
	void			rename_item(HTREEITEM hItem = NULL, CString new_label = _T(""));
	void			delete_item(HTREEITEM hItem = NULL, bool confirm = true);


	//�鿩���� ũ��
	int				m_indent_size = 16;
	bool			m_use_checkbox = false;
	bool			m_use_expand_button = true;

	bool			m_show_area = false;

	CTheme			m_winctrl_theme;
	bool			m_winctrl_theme_initialized = false;
	void			winctrl_theme_init();
	void			draw_checkbox(CDC* pDC, CRect r, int check_state);

	CImageList		m_imagelist;			//��ü �̹��� ����Ʈ
	std::deque<UINT>m_image_IDs;			//�̹��� ����Ʈ�� �߰��� resource id. �̸� �����ϴ� ������ icon size ���� ���浵 �����ϱ� ����.
	int				m_image_size = -1;		//-1(not yet set), 16 or 32?
	bool			m_use_own_imagelist = false;	//��ü �̹�������Ʈ�� �� ������
	void			create_imagelist();

	HTREEITEM		m_expanding_item;		// Ȯ���ư�� ������ ������. �̸� ����ؼ� remote�� ��������� �־��ش�.
	HTREEITEM		m_desktopItem;			// ����ȭ�� ������
	HTREEITEM		m_documentItem;			// ���� ������
	HTREEITEM		m_computerItem;			// �� PC ������
	//std::deque<CSCTreeCtrlFolder> m_folder_list;

	void			thread_insert_folders(HTREEITEM hItem);

	//��Ʈ ����
	LOGFONT			m_lf;
	CFont			m_font;
	int				m_font_size;
	void			reconstruct_font();

	//�÷� ����
	/*
	Gdiplus::Color	m_cr_text;					//�⺻ ���ڻ�
	Gdiplus::Color	m_cr_text_selected;			//���� �׸��� Ȱ��ȭ(active) ���ڻ�
	Gdiplus::Color	m_cr_text_selected_inactive;	//���� �׸��� ��Ȱ��ȭ(inactive) ���ڻ�
	Gdiplus::Color	m_cr_text_dropHilited;
	Gdiplus::Color	m_cr_back;					//�⺻ ����
	Gdiplus::Color	m_cr_back_selected;
	Gdiplus::Color	m_cr_back_selected_inactive;
	Gdiplus::Color	m_crBackTrackSelect;		//hot tracking item back color
	Gdiplus::Color	m_cr_back_dropHilited;
	Gdiplus::Color	m_cr_selected_border;
	*/

	//Drag&Drop �巡�� ����
	bool			m_use_drag_and_drop = false;
	CWnd*			m_pDragWnd = NULL;			//Which wnd we are dragging FROM
	CWnd*			m_pDropWnd = NULL;			//Which wnd we are dropping ON
	CImageList*		m_pDragImage = NULL;		//For creating and managing the drag-image
	bool			m_bDragging = false;		//T during a drag operation
	std::deque<UINT> m_drag_images_id;			//drag�� �� ����ϴ� �̹������� resource id ����(�������Ͽ� �̹���, �̱����Ͽ� �̹����� ���ʴ�� �ְ� drag�Ǵ� ������ ���� �´� �̹����� ����Ѵ�)
	void			DroppedHandler(CWnd* pDragWnd, CWnd* pDropWnd);
	//https://jiniya.net/tt/594/
	//�� �Լ��� �巡�� �̹����� ���� �������ִ� �ڵ����� ������� ���� �ڵ��̹Ƿ� ���� �Ұ�.
	CImageList*		create_drag_image(CTreeCtrl* pList, LPPOINT lpPoint);

	//���� ����
	CEdit*			m_pEdit = NULL;
	CString			m_edit_old_text;
	CString			m_edit_new_text;
	HTREEITEM		m_edit_item = NULL;			//�������� ������ �ε���
	bool			m_in_editing = false;		//����������

	enum ROW_ITEM_RECT
	{
		rect_row = 0,
		rect_button,
		rect_check,
		rect_icon,
		rect_label,
	};
	//fullRow(0), Ȯ���ư(1), üũ�ڽ�(2), ������(3), ���̺�(4) �� 5���� CRect�� ���Ѵ�.
	void			get_item_rect(HTREEITEM hItem, CRect r[]);

protected:
	DECLARE_MESSAGE_MAP()
	virtual void PreSubclassWindow();
public:
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	afx_msg void OnActivate(UINT nState, CWnd* pWndOther, BOOL bMinimized);
	afx_msg BOOL OnEraseBkgnd(CDC* pDC);
	afx_msg void OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags);
	afx_msg void OnPaint();
	afx_msg BOOL OnTvnSelchanged(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnWindowPosChanged(WINDOWPOS* lpwndpos);
	afx_msg void OnTvnItemexpanding(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg BOOL OnNMClick(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnTvnBegindrag(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnNMDblclk(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
	afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
	afx_msg BOOL OnTvnBeginlabeledit(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg BOOL OnTvnEndlabeledit(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnTvnItemexpanded(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnNMCustomDraw(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnTimer(UINT_PTR nIDEvent);
	afx_msg void OnContextMenu(CWnd* /*pWnd*/, CPoint /*point*/);
	afx_msg void OnNMRClick(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnVScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
	afx_msg void OnMouseHWheel(UINT nFlags, short zDelta, CPoint pt);
	afx_msg BOOL OnMouseWheel(UINT nFlags, short zDelta, CPoint pt);
};


