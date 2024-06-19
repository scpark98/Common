#pragma once

/*
* 기본 CTreeCtrl에서 제공하지 않는 기능들을 확장하기 위해 제작.
	- 파일 load, save (txt, json?)
	  (txt의 경우 depth는 tab으로 표현함)

[윈도우 탐색기와 같은 폴더 트리로 사용할 경우]
	- dlg.h에 CShellImageList의 인스턴스를 선언하고,
	  dlg.cpp에서 m_ShellImageList.Initialize(); 과 같이 초기화 한 후,
	  m_tree0.set_as_shell_treectrl(&m_ShellImageList, true); 로 세팅하면 끝. 2번째 인자는 local인지 remote인지 명시.

	- 만약 drag&drop 기능이 필요하다면
	  m_tree0.use_drag_and_drop(true);
	  m_tree0.add_drag_images(IDB_DRAG_ONE_FILE, IDB_DRAG_MULTI_FILES); 과 같이 드래그에 사용될 이미지 파일 세팅.

	- C드라이브가 선택되게 하려면
	  m_tree0.select_item(_T("C:\\"));

[일반 데이터를 트리에 표현할 경우]

*/


// CSCTreeCtrl
#include <afxwin.h>
#include <afxcmn.h>
#include <deque>

#include "../../system/ShellImageList/ShellImageList.h"
#include "../../ui/theme/theme.h"

static const UINT Message_CSCTreeCtrl = ::RegisterWindowMessage(_T("MessageString_CSCTreeCtrl"));

//tree의 한 항목인 folder 정보 저장 목적
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

	//자체 이미지리스트를 쓸 것인지
	void		set_use_own_imagelist(bool use, int image_size = 16);
	void		set_image_size(int image_size = 16);

	void		set_use_checkbox(bool checkbox = true);

	//ico 파일들을 imagelist에 추가
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

	//HitTest()는 클릭된 hItem과 expand button, check, icon, label 영역을 판별하는 함수인데
	//OnPaint()에서 CTreeCtrl::OnPaint()를 사용하지 않고 직접 그려줄 경우는
	//HitTest()를 그대로 사용할 수 없으므로 별도 함수로 작성.
	//만약 OnPaint()에서 그려주는 것이 아닌 CustonDraw()에서 그려주는 경우도 역시 동일한지는 확인 필요.
	HTREEITEM	hit_test(UINT* nFlags = NULL);

	//컬러 관련
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

	//Drag&Drop 드래깅 관련
	template <typename ... Types> void add_drag_images(Types... args) //(단일파일용 이미지, 싱글파일용 이미지를 차례대로 넣고 drag되는 개수에 따라 맞는 이미지를 사용한다)
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
	int				m_nDropIndex = -1;			//Index at which to drop item in the List we are dropping ON(drag를 시작한 컨트롤의 멤버값에 저장됨, 드롭된 클래스에는 저장되지 않음)

	BOOL			MoveTreeItem(CTreeCtrl* pTree, HTREEITEM hSrcItem, HTREEITEM hDestItem);
	BOOL			MoveChildTreeItem(CTreeCtrl* pTree, HTREEITEM hChildItem, HTREEITEM hDestItem);

protected:
	CTheme			m_theme;
	bool			m_theme_initialized = false;
	void			theme_init();
	void			draw_checkbox(CDC* pDC, CRect r, int check_state);

	CImageList		m_imagelist;			//자체 이미지 리스트
	std::deque<UINT>m_image_IDs;			//이미지 리스트에 추가한 resource id. 이를 저장하는 이유는 icon size 동적 변경도 지원하기 위해.
	int				m_image_size = -1;		//-1(not yet set), 16 or 32?
	bool			m_use_own_imagelist = false;	//자체 이미지리스트를 쓸 것인지
	void			create_imagelist();

	HTREEITEM		m_expandItem;			// 마지막으로 확장한 아이템
	HTREEITEM		m_desktopItem;			// 바탕화면 아이템
	HTREEITEM		m_documentItem;			// 문서 아이템
	HTREEITEM		m_computerItem;			// 내 PC 아이템
	std::deque<CSCTreeCtrlFolder> m_folder_list;

	void			thread_insert_folders(HTREEITEM hItem);

	//폰트 관련
	LOGFONT			m_lf;
	CFont			m_font;
	int				m_font_size;
	void			reconstruct_font();

	//컬러 관련
	COLORREF		m_crText;					//기본 글자색
	COLORREF		m_crTextSelected;			//선택 항목의 활성화(active) 글자색
	COLORREF		m_crTextSelectedInactive;	//선택 항목의 비활성화(inactive) 글자색
	COLORREF		m_crTextDropHilited;
	COLORREF		m_crBack;					//기본 배경색
	COLORREF		m_crBackSelected;
	COLORREF		m_crBackSelectedInactive;
	COLORREF		m_crBackDropHilited;
	COLORREF		m_crSelectedBorder;

	//Drag&Drop 드래깅 관련
	bool			m_use_drag_and_drop = false;
	CWnd*			m_pDragWnd = NULL;			//Which wnd we are dragging FROM
	CWnd*			m_pDropWnd = NULL;			//Which wnd we are dropping ON
	CImageList*		m_pDragImage = NULL;		//For creating and managing the drag-image
	bool			m_bDragging = false;		//T during a drag operation
	std::deque<UINT> m_drag_images_id;			//drag할 때 사용하는 이미지들의 resource id 저장(단일파일용 이미지, 싱글파일용 이미지를 차례대로 넣고 drag되는 개수에 따라 맞는 이미지를 사용한다)
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


