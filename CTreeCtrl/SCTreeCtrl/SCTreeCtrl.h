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

[주의사항]
	- 가로스크롤과 관련된 처리는 완전하지 않음. 특히 편집중일 때.
	- 윈도우탐색기도 트리뷰에서 가로스크롤은 세로스크롤로 동작함.
	- 
*/


// CSCTreeCtrl
#include <afxwin.h>
#include <afxcmn.h>
#include <deque>
#include <functional>

#include "../../system/ShellImageList/ShellImageList.h"
#include "../../ui/theme/theme.h"
#include "../../CMenu/CSCMenuBar/SCMenu.h"
#include "../../colors.h"
#include "../../CEdit/SCEdit/SCEdit.h"


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
	CSCTreeCtrlMessage(CWnd* _this, int _message, CWnd* _pTarget = NULL, CString _param0 = _T(""), CString _param1 = _T(""), int _reserved = 0)
	{
		pThis = _this;
		message = _message;
		pTarget = _pTarget;
		param0 = _param0;
		param1 = _param1;
		reserved = _reserved;
	}

	CWnd*	pThis = NULL;
	CWnd*	pTarget = NULL;
	int		message = 0;
	CString	param0;
	CString	param1;
	int		reserved = 0;
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
		//message_selchanged = 0,		//TVN_SELCHANGED를 이용하므로 제거한다.
		message_drag_and_drop = 0,
		message_path_changed,
		message_request_folder_list,	//remote일 경우 OnTvnItemexpanding() 메시지가 발생하면 remote의 폴더목록을 받아서 넣어줘야 한다.
		message_request_new_folder,
		message_request_new_folder_index,
		message_request_rename,
		message_request_property,
		message_rename_duplicated,		//중복된 폴더명이 존재할 경우
		message_edit_item,				//F2키를 누르면 메인에서 편집작업을 수행하기 위해.
		message_tree_processing,		//폴더들을 노드에 추가할 경우 C:\Windows, C:\Windows\WinSxS 등과 같은 폴더는 그 갯수가 많으므로 parent에게 이를 알린다.
	};

	//해당 아이템의 depth level을 리턴한다. hItem == NULL이면 -1을 리턴한다.
	int			get_indent_level(HTREEITEM hItem);

	int			get_indent_size() { return m_indent_size; }
	void		set_indent_size(int size) { m_indent_size = size; Invalidate(); }

	//해당 아이템에 지정된 이미지 인덱스를 리턴한다.
	//이 노드가 어떤 종류의 노드인지 구분하기 위해 사용된다.
	int			get_image_index(HTREEITEM hItem);

	//스타일 변경
	void		full_row_selection(bool full_row);
	void		has_line(bool line);
	void		use_checkbox(bool use = true);

	//확장, 축소 버튼 표시 여부
	void		use_expand_button(bool use = true);

	void		show_area(bool show) { m_show_area = show; Invalidate(); }

	CShellImageList* m_pShellImageList = NULL;
	void		set_shell_imagelist(CShellImageList* pShellImageList) { m_pShellImageList = pShellImageList; }

	//자체 이미지리스트를 쓸 것인지
	void		set_use_own_imagelist(bool use, int image_size = 16);
	void		set_image_size(int image_size = 16);


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
	bool		m_is_local = true;
	bool		is_shell_treectrl() { return m_is_shell_treectrl; }
	bool		is_local() { return m_is_local; }

	//윈도우 탐색기의 폴더 트리와 같이 동작한다.
	//is_local이 false일 경우는 remote의 폴더정보를 얻어와서 표시해줘야 한다.
	void		set_as_shell_treectrl(CShellImageList* pShellImageList, bool is_local = true, CString default_path = _T("C:\\"));

	//드라이브 폴더를 다시 읽어들인다.
	void		refresh(HTREEITEM hParent = NULL);

	//현재 폴더에 새 폴더를 생성하고 편집모드로 표시한다.
	//"새 폴더" or "New Folder" 등 다국어까지 고려하여 타이틀을 받는다.
	bool		add_item(HTREEITEM hParent, CString new_folder_title, bool edit_mode);

	//hParent 항목 아래 하위 항목을 추가한다. NULL이면 현재 선택된 항목이 hParent가 된다.
	//label이 ""이면 기본 "새 폴더"명으로 추가한 후 edit_item() 호출.
	//auto_index = true라면 새 폴더, 새 항목이 이미 존재할 경우 뒤에 숫자를 증가시켜 붙여줘야 한다.
	//새로 추가된 항목을 리턴한다.
	HTREEITEM	add_new_item(HTREEITEM hParent = NULL, CString label = _T(""), bool auto_index = false, bool edit_mode = false);
	//주어진 항목의 label을 변경한다. hItem == NULL이면 현재 선택된 항목을, new_label이 공백이면 편집모드로 표시한다.
	void		rename_item(HTREEITEM hItem = NULL, CString new_label = _T(""));
	//현재 선택된 노드의 하위 노드들 중에서 old_label을 찾아서 new_label로 이름을 변경한다.
	void		rename_child_item(HTREEITEM hParent, CString old_label, CString new_label);

	//hItem과 하위 노드들까지 삭제한다.
	//해당 노드에 메모리가 할당되었다면 해제후에 삭제시킨다.
	//only_children이 true이면 해당 노드의 자식들만 제거한다.
	void		delete_item(HTREEITEM hItem = NULL, bool only_children = false, bool confirm = false);

	void		delete_all_items(bool confirm = false);
	//CTreeCtrl::DeleteAllItems() override.
	void		DeleteAllItems(bool confirm = false);

	//현재 폴더에서 "새 폴더" 생성 시 인덱스를 구한다. ex. "새 폴더 (2)"
	int			get_file_index(CString path, CString new_folder_title);


	//hItem이 NULL이면 현재 선택된 폴더의 fullpath return.
	CString		get_path(HTREEITEM hItem = NULL);
	void		set_path(CString fullpath, bool expand = true);

	HTREEITEM	insert_special_folder(int csidl);
	void		insert_drive(CDiskDriveInfo drive_info);
	void		insert_folder(HTREEITEM hParent, CString sParentPath);
	void		insert_folder(HTREEITEM hParent, WIN32_FIND_DATA* pFindFileData, bool has_children = true);

	//local이면 drive_list를 NULL로 주고 remote이면 실제 리스트를 주고 갱신시킨다.
	void		update_drive_list(CString thisPC, std::deque<CDiskDriveInfo>* drive_list = NULL);

	//탭을 이용해서 작성된 트리 구조 문자열을 파싱하여 트리로 표현함.
	//각 노드의 이미지는 그 depth에 따라 m_imagelist의 인덱스를 사용함.
	bool		load(CString file);
	//탭으로 구분되는 텍스트 파일로 저장
	bool		save(CString file);
	//탭으로 구분된 텍스트 파일 로딩
	bool		load_from_string(CString text);

	//HTREEITEM	find_item(const CString& name);
	//hItem 위치부터 child, sibling들을 recursive하게 모두 탐색하여 label을 찾는다. 
	HTREEITEM	find_item(const CString& label, HTREEITEM hItem = NULL);
	//hParentItem의 첫 레벨 children만 검사한다. recursive를 사용하지 않는다.
	HTREEITEM	find_children_item(const CString& label, HTREEITEM hParentItem = NULL);
	CString		get_selected_item_text(bool include_parent = false);

	//hItem = NULL인 경우는 모든 노드를 unselect로 만드는데 사용된다.
	//NULL이 아닌 어떤 노드를 select상태로 만들지만 기존 selected 노드에는 영향을 주지 않는다.
	void		select_item(HTREEITEM hItem = NULL);
	//hItem의 하위 폴더들중에서 먼저 발견되는 label의 노드를 찾아서 선택상태로 표시한다.
	//만약 find_label = "item1\\item2"와 같이 full 경로로 전달된다면 recursive하게 경로를 찾아간다.
	HTREEITEM	select_item(CString find_label, HTREEITEM hItem = NULL, CString sep = _T("\\"));

	//폴더 펼침 이벤트가 발생한 노드 아이템 리턴
	HTREEITEM	get_expanding_item() { return m_expanding_item; }

	//해당 아이템이 축소되서 보이지 않는 상태인지(height가 음수로 리턴되는 특징으로 판별함)
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
	//CTreeCtrl에서 지원하는 기본 함수 override
	void		SetTextColor(Gdiplus::Color text_color) { set_text_color(text_color); }
	void		SetBkColor(Gdiplus::Color back_color) { set_back_color(back_color); }


	//Drag&Drop 드래깅 관련
	template <typename ... Types> void add_drag_images(Types... args) //(단일파일용 이미지, 싱글파일용 이미지를 차례대로 넣고 drag되는 개수에 따라 맞는 이미지를 사용한다)
	{
		int n = sizeof...(args);
		int arg[] = { args... };

		for (auto id : arg)
			m_drag_images_id.push_back(id);
	}

	void			set_use_popup_menu(bool use = true) { m_use_popup_menu = use; }

	bool			get_use_drag_and_drop() { return m_use_drag_and_drop; }
	void			set_use_drag_and_drop(bool use_drag = true) { m_use_drag_and_drop = use_drag; }
	HTREEITEM		m_DragItem = NULL;			//drag되는 아이템
	HTREEITEM		m_DropItem = NULL;			//drop된 아이템
	int				m_nDropIndex = -1;			//drop된 컨트롤이 CListCtrl일 때 그 인덱스(drag를 시작한 컨트롤의 멤버값에 저장됨, 드롭된 클래스에는 저장되지 않음)

	BOOL			move_tree_item(CTreeCtrl* pTree, HTREEITEM hSrcItem, HTREEITEM hDestItem);
	BOOL			move_child_tree_item(CTreeCtrl* pTree, HTREEITEM hChildItem, HTREEITEM hDestItem);

//편집 관련
	void			allow_edit(bool allow) { m_allow_edit = allow; }
	void			edit_item(HTREEITEM hItem = NULL);
	void			edit_end(bool valid = true);
	HTREEITEM		get_recent_edit_item() { return m_edit_item; }
	CString			get_edit_old_text() { return m_edit_old_text; }		//편집 후 텍스트
	CString			get_edit_new_text() { return m_edit_new_text; }		//편집 후 텍스트
	void			undo_edit_label();									//편집 전의 텍스트로 되돌린다.(예를 들어 편집 레이블이 파일명이고 파일명 변경이 실패한 경우 쓸 수 있다.)
	//CTreeCtrl::GetEditControl()을 override.
	CEdit*			GetEditControl() { return get_edit_control(); }
	CEdit*			get_edit_control() { return m_pEdit; }
	LRESULT			on_message_CSCEdit(WPARAM wParam, LPARAM lParam);


//폰트 관련
	LOGFONT			get_log_font() { return m_lf; }
	void			set_log_font(LOGFONT lf);
	int				get_font_size();
	void			set_font_name(LPCTSTR font_name, BYTE char_set = DEFAULT_CHARSET);
	void			set_font_size(int font_size);
	void			enlarge_font_size(bool enlarge);
	void			set_font_bold(bool bold = true);
	void			set_font_italic(bool italic = true);

//색상 관련
	//std::function<void()>	function_check_dim_text;
	//void			set_dim_text_function(std::function<void()> func) { function_check_dim_text = func; }
	//특정 노드는 어떤 조건에 의해 다른색으로 표현하고 싶은데 그 판별은 parent에서 해야 할 때
	//이 함수를 parent에서 호출하면서 판별함수를 지정할 수 있다. (LMM CSManager에서 속한 device가 없는 그룹의 색상을 dim처리)
	bool			(*check_is_dim_text)(CWnd* pTree, HTREEITEM hItem) = NULL;
	void			set_function_check_is_dim_text(bool (*func)(CWnd* pTree, HTREEITEM hItem)) { check_is_dim_text = func; }

	//가상 루트 항목 설정.
	//가상 루트가 없는 상태에서 노드들이 추가된 경우, 가상 루트를 새로 추가할 경우
	//가상 루트를 추가하고 기존 노드들을 모두 가상루트의 child로 이동시킨다.
	//image_index, selected_image_index는 미리 정의한 imagelist에 추가된 아이콘의 index이므로
	//반드시 set_imagelist()로 이미지들을 지정한 후에 호출해야 한다.
	//이미 root_item을 지정한 상태에서 DeleteAllItems()로 실제 노드들을 모두 삭제하는 경우가 있는데
	//이 때 다시 가상 루트를 넣을 때에는 파라미터 없이 set_root_item();을 호출하여 저장된 정보를 이용하도록 함.
	bool			get_use_root() { return m_use_root; }
	void			set_root_item(CString label = _T(""), int image_index = -1, int selected_image_index = -1);
	HTREEITEM		get_root_item() { return (m_use_root ? m_root_item : GetRootItem()); }
	TV_INSERTSTRUCT get_root_tvItem() { return m_root_tvItem; }

protected:
	//root 항목은 실제 또는 가상의 root일 수 있다.
	//탐색기의 "내 PC"는 가상의 root이고 "로컬 디스크 (C:)"는 C드라이브의 실제적인 root다.
	//트리 데이터에서 루트를 정한 경우는 m_use_root를 false로 사용하면 되고
	//가상의 루트 노드가 필요한 경우에는 set_root_item(CString label, UINT img_id);으로 호출해서 설정한다.
	//이런 루트를 표시할 것인지에 관련한 항목 정의.
	bool			m_use_root = false;
	HTREEITEM		m_root_item = NULL;
	TV_INSERTSTRUCT m_root_tvItem;		//기억시켜놔야 DeleteAllItem()후에도 다시 추가할 수 있다.


	enum TIMER_ID
	{
		timer_expand_for_drag_hover = 0,	//drag하여 트리 항목위에 머물경우 해당 트리를 expand시켜준다.
	};

	//popup menu
	enum POPUP_MENU_ID
	{
		//Resource의 menu id가 아닌 사용자 정의 id를 사용한다면 주의할 것.
		//특히 Resource의 menu에서는 separator의 id가 0이므로
		//사용자가 추가한 메뉴 id도 절대 0부터 시작하지 말것!
		menu_add_item = WM_USER + 473,
		menu_rename_item,
		menu_delete_item,
		menu_property,
		menu_refresh,
		menu_favorite,
	};
	CSCMenu			m_menu;
	bool			m_use_popup_menu = false;	//팝업메뉴 사용 여부. default = false
	void			OnPopupMenu(UINT nID);
	LRESULT			OnMessageCSCMenu(WPARAM wParam, LPARAM lParam);



//마우스가 컨트롤 안에 들어온 경우 true
	bool			m_is_hovering = false;


//들여쓰기 크기
	int				m_indent_size = 16;
	bool			m_use_checkbox = false;
	bool			m_use_expand_button = true;

	bool			m_show_area = false;

	CTheme			m_winctrl_theme;
	bool			m_winctrl_theme_initialized = false;
	void			winctrl_theme_init();
	void			draw_checkbox(CDC* pDC, CRect r, int check_state);

	CImageList		m_imagelist;			//자체 이미지 리스트
	std::deque<UINT>m_image_IDs;			//이미지 리스트에 추가한 resource id. 이를 저장하는 이유는 icon size 동적 변경도 지원하기 위해.
	int				m_image_size = -1;		//-1(not yet set), 16 or 32?
	bool			m_use_own_imagelist = false;	//자체 이미지리스트를 쓸 것인지
	void			create_imagelist();

	HTREEITEM		m_expanding_item;		// 확장버튼이 눌려진 아이템. 이를 기억해서 remote의 폴더목록을 넣어준다.
	HTREEITEM		m_desktopItem;			// 바탕화면 아이템
	HTREEITEM		m_documentItem;			// 문서 아이템
	HTREEITEM		m_computerItem;			// 내 PC 아이템
	//std::deque<CSCTreeCtrlFolder> m_folder_list;

	void			thread_insert_folders(HTREEITEM hItem);

//폰트 관련
	LOGFONT			m_lf;
	CFont			m_font;
	int				m_font_size;
	void			reconstruct_font();

//Drag&Drop 드래깅 관련
	bool			m_use_drag_and_drop = false;//default = false
	CWnd*			m_pDragWnd = NULL;			//Which wnd we are dragging FROM
	CWnd*			m_pDropWnd = NULL;			//Which wnd we are dropping ON
	CImageList*		m_pDragImage = NULL;		//For creating and managing the drag-image
	bool			m_bDragging = false;		//T during a drag operation
	std::deque<UINT> m_drag_images_id;			//drag할 때 사용하는 이미지들의 resource id 저장(단일파일용 이미지, 싱글파일용 이미지를 차례대로 넣고 drag되는 개수에 따라 맞는 이미지를 사용한다)
	void			DroppedHandler(CWnd* pDragWnd, CWnd* pDropWnd);
	//https://jiniya.net/tt/594/
	//이 함수는 드래그 이미지를 직접 생성해주는 코드지만 취약점이 많은 코드이므로 참고만 할것.
	CImageList*		create_drag_image(CTreeCtrl* pList, LPPOINT lpPoint);

//편집 관련
	bool			m_allow_edit = true;
	bool			m_in_editing = false;		//편집중인지
	CSCEdit*		m_pEdit = NULL;
	CString			m_edit_old_text;
	CString			m_edit_new_text;
	HTREEITEM		m_edit_item = NULL;			//편집중인 아이템 인덱스

	long			m_last_clicked_time = 0;
	HTREEITEM		m_last_clicked_item;

	enum ROW_ITEM_RECT
	{
		rect_row = 0,
		rect_button,
		rect_check,
		rect_icon,
		rect_label,
	};

//Serialize 관련
	void			serialize_item(CArchive& ar, HTREEITEM hItem = NULL);

	void			release_iterator(HTREEITEM hItem = NULL);

protected:
	DECLARE_MESSAGE_MAP()
	virtual void PreSubclassWindow();
public:
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	afx_msg void OnActivate(UINT nState, CWnd* pWndOther, BOOL bMinimized);
	afx_msg BOOL OnEraseBkgnd(CDC* pDC);
	afx_msg void OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags);
	afx_msg void OnPaint();
	afx_msg BOOL OnTvnSelchanging(NMHDR* pNMHDR, LRESULT* pResult);
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
	afx_msg BOOL OnNMRClick(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnVScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
	afx_msg void OnMouseHWheel(UINT nFlags, short zDelta, CPoint pt);
	afx_msg BOOL OnMouseWheel(UINT nFlags, short zDelta, CPoint pt);
	afx_msg void OnMouseHover(UINT nFlags, CPoint point);
	afx_msg void OnMouseLeave();
	afx_msg void OnGetMinMaxInfo(MINMAXINFO* lpMMI);
	virtual void Serialize(CArchive& ar);
	afx_msg void OnDestroy();
};


