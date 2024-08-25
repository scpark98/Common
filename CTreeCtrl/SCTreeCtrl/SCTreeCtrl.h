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

#include "../../system/ShellImageList/ShellImageList.h"
#include "../../ui/theme/theme.h"
#include "../../CMenu/SCMenuBar/SCMenu.h"

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


	int			get_indent_size() { return m_indent_size; }
	void		set_indent_size(int size) { m_indent_size = size; Invalidate(); }

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
	bool		m_is_shell_treectrl_local = true;
	bool		is_is_shell_treectrl() { return m_is_shell_treectrl; }
	bool		is_is_shell_treectrl_local() { return m_is_shell_treectrl_local; }

	void		set_as_shell_treectrl(CShellImageList* pShellImageList, bool is_local);

	//hItem이 NULL이면 현재 선택된 폴더의 fullpath return.
	CString		get_fullpath(HTREEITEM hItem = NULL);
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

	//해당 아이템이 축소되서 보이지 않는 상태인지(height가 음수로 리턴된다.)
	bool		is_visible_item(HTREEITEM hItem);

	//recursive traverse
	void		iterate_tree(HTREEITEM hItem = NULL);
	//not recursive, top-down traverse
	void		iterate_tree_in_order(HTREEITEM hItem = NULL);
	//not recursive traverse using stack, not guarantee the original order
	std::deque<CSCTreeCtrlFolder>	iterate_tree_with_no_recursion(HTREEITEM hItem = NULL);

	void		expand_all(bool expand = true);

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
	void	set_text_color(COLORREF text_color) { m_crText = text_color; Invalidate(); }
	void	set_back_color(COLORREF back_color) { m_crBack = back_color; Invalidate(); }
	//CTreeCtrl에서 지원하는 기본 함수 override
	void	SetTextColor(COLORREF text_color) { set_text_color(text_color); }
	void	SetBkColor(COLORREF back_color) { set_back_color(back_color); }

	//Drag&Drop 드래깅 관련
	template <typename ... Types> void add_drag_images(Types... args) //(단일파일용 이미지, 싱글파일용 이미지를 차례대로 넣고 drag되는 개수에 따라 맞는 이미지를 사용한다)
	{
		int n = sizeof...(args);
		int arg[] = { args... };

		for (auto id : arg)
			m_drag_images_id.push_back(id);
	}

	bool			get_use_drag_and_drop() { return m_use_drag_and_drop; }
	void			set_use_drag_and_drop(bool use_drag) { m_use_drag_and_drop = use_drag; }
	HTREEITEM		m_DragItem = NULL;			//drag되는 아이템
	HTREEITEM		m_DropItem = NULL;			//drop된 아이템
	int				m_nDropIndex = -1;			//drop된 컨트롤이 CListCtrl일 때 그 인덱스(drag를 시작한 컨트롤의 멤버값에 저장됨, 드롭된 클래스에는 저장되지 않음)

	BOOL			move_tree_item(CTreeCtrl* pTree, HTREEITEM hSrcItem, HTREEITEM hDestItem);
	BOOL			move_child_tree_item(CTreeCtrl* pTree, HTREEITEM hChildItem, HTREEITEM hDestItem);

	//20240801 scpark 편집과 관계된 코드 추가
	void			edit_item(HTREEITEM hItem = NULL);
	void			edit_end(bool valid = true);
	HTREEITEM		get_recent_edit_item() { return m_edit_item; }
	CString			get_edit_old_text() { return m_edit_old_text; }		//편집 후 텍스트
	CString			get_edit_new_text() { return m_edit_new_text; }	//편집 후 텍스트
	void			undo_edit_label();									//편집 전의 텍스트로 되돌린다.(예를 들어 편집 레이블이 파일명이고 파일명 변경이 실패한 경우 쓸 수 있다.)
	//CTreeCtrl::GetEditControl()을 override.
	CEdit*			GetEditControl() { return get_edit_control(); }
	CEdit*			get_edit_control() { return m_pEdit; }
	long			m_last_clicked_time = 0;
	HTREEITEM		m_last_clicked_item;

	//폰트 관련
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
		//Resource의 menu id가 아닌 사용자 정의 id를 사용한다면 주의할 것.
		//특히 Resource의 menu에서는 separator의 id가 0이므로
		//사용자가 추가한 메뉴 id도 절대 0부터 시작하지 말것!
		menu_add_item = WM_USER + 473,
		menu_rename_item,
		menu_delete_item,
	};
	CSCMenu			m_menu;
	LRESULT			OnMessageCSCMenu(WPARAM wParam, LPARAM lParam);

	//하위 항목을 추가한다. label이 ""이면 기본이름으로 추가한 후 edit_item() 호출.
	//새 폴더, 새 항목이 이미 존재하면 뒤에 숫자를 증가시켜 붙여줘야 한다.
	void			add_sub_item(HTREEITEM hParent = NULL, CString label = _T(""));
	//주어진 항목의 label을 변경한다.
	void			rename_item(HTREEITEM hItem = NULL, CString new_label = _T(""));
	void			delete_item(HTREEITEM hItem = NULL, bool confirm = true);


	//들여쓰기 크기
	int				m_indent_size = 16;
	bool			m_use_checkbox = false;
	bool			m_use_expand_button = true;

	bool			m_show_area = false;

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
	//std::deque<CSCTreeCtrlFolder> m_folder_list;

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
	//https://jiniya.net/tt/594/
	//이 함수는 드래그 이미지를 직접 생성해주는 코드지만 취약점이 많은 코드이므로 참고만 할것.
	CImageList*		create_drag_image(CTreeCtrl* pList, LPPOINT lpPoint);

	//편집 관련
	CEdit*			m_pEdit = NULL;
	CString			m_edit_old_text;
	CString			m_edit_new_text;
	HTREEITEM		m_edit_item = NULL;			//편집중인 아이템 인덱스
	bool			m_in_editing = false;		//편집중인지

	enum ROW_ITEM_RECT
	{
		rect_row = 0,
		rect_button,
		rect_check,
		rect_icon,
		rect_label,
	};
	//fullRow(0), 확장버튼(1), 체크박스(2), 아이콘(3), 레이블(4) 총 5개의 CRect를 구한다.
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


