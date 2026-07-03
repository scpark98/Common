#pragma once

/*
* 기본 CTreeCtrl에서 제공하지 않는 기능들을 확장하기 위해 제작.
	- 파일 load, save (txt, json?)
	  (txt의 경우 depth는 tab으로 표현함)
- git : https://github.com/scpark98/Test_CSCTreeCtrl.git
- test project : D:\1.Projects_C++\1.test\Test_CSCSliderCtrl

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
	- 아래와 같이 raw text를 \n과 tab으로 표현할 수 있다.
	m_tree.load_from_string(_T("\
1\n\
	11\n\
	12\n\
		121\n\
			1211\n\
				12111 긴 텍스트 테스트 12111 긴 텍스트 테스트 12111 긴 텍스트 테스트 12111 긴 텍스트 테스트 12111 긴 텍스트 테스트\n\
	13\n\
2\n\
	21\n\
		211\n\
		212한글\n\
		"));


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
#include <vector>
#include <map>
#include <unordered_map>

#include "../../system/ShellImageList/ShellImageList.h"
#include "../../ui/theme/theme.h"
#include "../../CMenu/CSCMenuBar/SCMenu.h"
#include "../../colors.h"
#include "../../CScrollbar/SCScrollbar/SCScrollbar.h"
#include "../../CEdit/CSCStaticEdit/SCStaticEdit.h"
#include "../../Json/rapid_json/json.h"


static const UINT Message_CSCTreeCtrl = ::RegisterWindowMessage(_T("MessageString_CSCTreeCtrl"));

//특정 트리 항목에만 적용할 스타일 override. 지정 안 한 필드는 테마 기본값 사용.
//속성이 늘어나도 map 을 추가하지 않고 이 struct 에 필드만 더하면 됨.
struct CSCTreeItemStyle
{
	bool			use_text_color = false;
	Gdiplus::Color	text_color = Gdiplus::Color::Black;
	bool			use_back_color = false;
	Gdiplus::Color	back_color = Gdiplus::Color::White;
	int				font_style = -1;	//-1 = 테마 기본. >=0 = Gdiplus::FontStyle 비트 OR (Bold=1|Italic=2|Underline=4|Strikeout=8)
};

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
		message_expand_changed,			//노드 확장/축소로 '펼쳐진 폴더 집합'이 바뀌었음을 parent에게 알린다(dir watcher 재설정용).
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

	//HICON 리스트로 imagelist 구성. 리소스 ID 가 아닌 동적 아이콘 (SHGetStockIconInfo,
	//ExtractIconEx 등) 을 imagelist 에 넣을 때 사용.
	//호출 측 HICON 은 imagelist 가 내부 복사하므로 호출 후 ::DestroyIcon 으로 해제 가능.
	void			set_imagelist(const std::vector<HICON>& icons, int image_size = 16);

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
	//delete_data = true로 주면 동적으로 할당된 data도 delete해주지만 이는 매우 위험하다.
	//만약 data를 다른 thread에서 할당했는데 여기서 delete하면 문제가 되고
	//또 main에서 delete했는데 여기서 다시 delete한다면 double delete가 발생할 수 있다. 따라서 delete_data는 정말 필요한 경우에만 true로 주도록 한다.
	void		delete_item(HTREEITEM hItem = NULL, bool only_children = false, bool confirm = false, bool delete_data = false);

	void		delete_all_items(bool confirm = false, bool delete_data = false);
	//CTreeCtrl::DeleteAllItems() override.
	//void		DeleteAllItems(bool confirm = false, bool delete_data = false);

	//현재 폴더에서 "새 폴더" 생성 시 인덱스를 구한다. ex. "새 폴더 (2)"
	int			get_file_index(CString path, CString new_folder_title);


	//hItem이 NULL이면 현재 선택된 폴더의 fullpath return.
	CString		get_path(HTREEITEM hItem = NULL);
	void		set_path(CString fullpath, bool expand = true);
	//real 또는 특수폴더 표시형 fullpath 에 해당하는 노드를 찾는다. 이미 로드(펼쳐진 경로상)된 노드만 반환하며
	//중간 노드의 자식이 로드 안 됐으면 강제 확장하지 않고 NULL 을 반환한다(감시 콜백 등에서 '보이는 노드만' 갱신용).
	HTREEITEM	get_item_by_fullpath(CString fullpath);

	HTREEITEM	insert_special_folder(int csidl);
	void		insert_drive(CDiskDriveInfo drive_info);
	void		insert_folder(HTREEITEM hParent, CString sParentPath);
	HTREEITEM	insert_folder(HTREEITEM hParent, WIN32_FIND_DATA* pFindFileData, bool has_children = true, HTREEITEM hInsertAfter = TVI_LAST);
	//is_greater_with_numeric(탐색기식 숫자 인식) 정렬 위치에 폴더 노드 1개를 삽입 — 단일 노드 추가 시에도 오름차순 유지.
	HTREEITEM	insert_folder_sorted(HTREEITEM hParent, WIN32_FIND_DATA* pFindFileData);

	//local이면 drive_list를 NULL로 주고 remote이면 실제 리스트를 주고 갱신시킨다.
	void		update_drive_list(CString thisPC, std::deque<CDiskDriveInfo>* drive_list = NULL);

	//탭을 이용해서 작성된 트리 구조 문자열을 파싱하여 트리로 표현함.
	//각 노드의 이미지는 그 depth에 따라 m_imagelist의 인덱스를 사용함.
	bool		load(CString file);
	//탭으로 구분되는 텍스트 파일로 저장
	bool		save(CString file);
	//탭으로 구분된 텍스트 파일 로딩
	bool		load_from_string(CString text);


	//load from json file
	HTREEITEM	(*function_set_node_data_on_loading)(CTreeCtrl* pTree, DWORD** node_data, const rapidjson::Value& nodeValue, HTREEITEM hParent, HTREEITEM hInsertAfter) = nullptr;
	void		set_function_set_node_data_on_loading(HTREEITEM (*func)(CTreeCtrl* pTree, DWORD** node, const rapidjson::Value& nodeValue, HTREEITEM hParent, HTREEITEM hInsertAfter)) { function_set_node_data_on_loading = func; }
	bool		load_tree_from_json_file(CString json_file_path);

	//save to json file
	bool		(*function_set_node_value_on_saving)(CTreeCtrl* pTree, HTREEITEM hItem, rapidjson::Value& nodeValue, rapidjson::Document::AllocatorType& alloc) = nullptr;
	void		set_function_set_node_value_on_saving(bool (*func)(CTreeCtrl* pTree, HTREEITEM hItem, rapidjson::Value& nodeValue, rapidjson::Document::AllocatorType& alloc)) { function_set_node_value_on_saving = func; }
	bool		save_tree_to_json_file(CString json_file_path);


	//HTREEITEM	find_item(const CString& name);
	//hItem 위치부터 child, sibling들을 recursive하게 모두 탐색하여 label을 찾는다. 
	HTREEITEM	find_item(const CString& label, HTREEITEM hItem = NULL);
	//hParentItem의 첫 레벨 children만 검사한다. recursive를 사용하지 않는다.
	HTREEITEM	find_children_item(const CString& label, HTREEITEM hParentItem = NULL);
	CString		get_selected_item_text(bool include_parent = false);
	//hItem 의 라벨을 리턴. include_parent=true 면 root 까지 walk-up + sep 로 join. parent 라벨이 이미 sep 로 끝나면 (drive "C:\" 등) 추가 sep 안 넣음 (smart-join).
	CString		get_item_text(HTREEITEM hItem, bool include_parent = false, CString sep = _T("\\"));

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

	void		set_color_theme(int theme, bool invalidate = true);
	void		set_color_theme(const CSCColorTheme& theme, bool invalidate = true);
	void		set_text_color(Gdiplus::Color text_color) { m_theme.cr_text = text_color; Invalidate(); }
	void		set_back_color(Gdiplus::Color back_color) { m_theme.cr_back = back_color; Invalidate(); }
	//CTreeCtrl에서 지원하는 기본 함수 override
	void		SetTextColor(Gdiplus::Color text_color) { set_text_color(text_color); }
	void		SetBkColor(Gdiplus::Color back_color) { set_back_color(back_color); }

	//항목별 스타일 override — 지정한 항목만 테마 기본값 대신 이 스타일로 그림. 미지정 필드는 테마 그대로.
	//HTREEITEM 핸들은 DeleteAllItems 시 무효화·재사용될 수 있어, DeleteAllItems 에서 맵을 자동 clear 한다
	//(rebuild 후 호출측이 재지정). 따라서 트리 다시 채운 뒤 매번 set_item_*_color/style 호출.
	std::map<HTREEITEM, CSCTreeItemStyle>	m_item_styles;
	void		set_item_style(HTREEITEM hItem, const CSCTreeItemStyle& style) { m_item_styles[hItem] = style; Invalidate(); }
	void		set_item_text_color(HTREEITEM hItem, Gdiplus::Color color) { CSCTreeItemStyle& s = m_item_styles[hItem]; s.use_text_color = true; s.text_color = color; Invalidate(); }
	void		set_item_back_color(HTREEITEM hItem, Gdiplus::Color color) { CSCTreeItemStyle& s = m_item_styles[hItem]; s.use_back_color = true; s.back_color = color; Invalidate(); }
	void		set_item_font_style(HTREEITEM hItem, int gdiplus_font_style) { m_item_styles[hItem].font_style = gdiplus_font_style; Invalidate(); }
	CSCTreeItemStyle*	get_item_style(HTREEITEM hItem) { auto it = m_item_styles.find(hItem); return (it == m_item_styles.end()) ? nullptr : &it->second; }
	void		clear_item_style(HTREEITEM hItem) { m_item_styles.erase(hItem); Invalidate(); }
	void		clear_all_item_styles() { m_item_styles.clear(); Invalidate(); }
	//항목 삭제 시 stale handle 의 스타일 잔재 제거 — 핸들 재사용으로 엉뚱한 항목이 칠해지는 것 방지.
	BOOL		DeleteAllItems() { m_item_styles.clear(); return CTreeCtrl::DeleteAllItems(); }
	BOOL		DeleteItem(HTREEITEM hItem) { m_item_styles.erase(hItem); return CTreeCtrl::DeleteItem(hItem); }


	//Drag&Drop 드래깅 관련
	template <typename ... Types> void add_drag_images(Types... args) //(단일파일용 이미지, 싱글파일용 이미지를 차례대로 넣고 drag되는 개수에 따라 맞는 이미지를 사용한다)
	{
		int n = sizeof...(args);
		int arg[] = { args... };

		for (auto id : arg)
			m_drag_images_id.push_back(id);
	}

	void			set_use_own_context_menu(bool use = true) { m_use_own_context_menu = use; }

	//드래그앤드롭 기능을 사용할 것인지. 드래그앤드롭 기능이 false이면 순서 변경도 의미가 없다.
	bool			get_use_drag_and_drop() { return m_use_drag_and_drop; }
	void			set_use_drag_and_drop(bool use_drag = true) { m_use_drag_and_drop = use_drag; }
	//순서도 변경 가능하도록 할것인지. m_use_drag_and_drop = false이면 무의미하다.
	bool			get_use_rearrange_order() { return m_use_rearrange_order; }
	void			set_use_rearrange_order(bool use_rearrange = true) { m_use_rearrange_order = use_rearrange; }
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
	//CSCStaticEdit 으로 통일 — 본 컨트롤은 base CTreeCtrl 의 native edit 을 안 쓰고 자체 m_pEdit 만 운용.
	CSCStaticEdit*	get_edit_control() { return m_pEdit; }
	LRESULT			on_message_CSCStaticEdit(WPARAM wParam, LPARAM lParam);

	//편집 모드 중 *다른 컨트롤·다른 앱·다이얼로그 빈 영역* 어디를 클릭해도 편집을 정상 종료(commit) 시키기 위한 훅.
	//CSCListBox / CPathCtrl 과 동일 패턴 — WH_MOUSE 로 같은 스레드 외부 클릭 감지 → 메시지 post → on_end_edit_posted → edit_end(true).
	//다른 앱으로의 포커스 이동은 CSCEdit 의 WM_KILLFOCUS 가 on_message_CSCEdit 로 전달돼 처리 (기존 경로 유지).
	void			install_edit_mouse_hook();
	void			remove_edit_mouse_hook();
	static LRESULT CALLBACK	edit_mouse_hook_proc(int code, WPARAM wParam, LPARAM lParam);
	LRESULT			on_end_edit_posted(WPARAM wParam, LPARAM lParam);
	static CSCTreeCtrl*	s_editing_tree;
	HHOOK			m_mouse_hook = NULL;


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
	//이 함수를 parent에서 호출하면서 판별함수를 지정할 수 있다. (LMM CSManager에서 속한 device가 없는 그룹의 색상을 dim color로 처리)
	bool			(*function_check_is_dim_text)(CWnd* pTree, HTREEITEM hItem) = NULL;
	void			set_function_check_is_dim_text(bool (*func)(CWnd* pTree, HTREEITEM hItem)) { function_check_is_dim_text = func; }

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

	//선택항목등의 border가 아닌 ctrl 자체의 cr_border. OnNcPaint()에서 border 속성유무를 판단하여 테두리를 그린다.
	void			set_border_color(Gdiplus::Color cr_border) { m_theme.cr_border_inactive = cr_border; Invalidate(); }

	//resource editor에서 border를 true로 하면 m_draw_border = true;로 자동 설정된다.
	//그런데 만약 사용자가 set_draw_border(false)로 하면 border는 그려지지 않아야 한다.
	//
	//border 는 OnNcPaint 에서 CWindowDC 로 그리므로 m_draw_border 변경 후 WM_NCPAINT 를 발생시켜야
	//한다. Invalidate() 는 client area 만 갱신하므로 부족 → SWP_FRAMECHANGED 로 NC 재계산 + 재그리기.
	//또한 native WS_BORDER/WS_EX_CLIENTEDGE 가 남아 있으면 OnNcPaint 직후 native NC paint 가 덮어쓰므로 함께 제거.
	void			set_draw_border(bool draw_border = true)
	{
		m_draw_border = draw_border;
		if (m_hWnd)
		{
			if (draw_border)
			{
				if (GetStyle()   & WS_BORDER)        ModifyStyle(WS_BORDER, 0);
				if (GetExStyle() & WS_EX_CLIENTEDGE) ModifyStyleEx(WS_EX_CLIENTEDGE, 0);
				if (GetExStyle() & WS_EX_STATICEDGE) ModifyStyleEx(WS_EX_STATICEDGE, 0);
			}
			SetWindowPos(NULL, 0, 0, 0, 0,
				SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE | SWP_FRAMECHANGED);
			Invalidate();
		}
	}

//자체 overlay scrollbar — native WS_VSCROLL / WS_HSCROLL 제거 + 우측 / 하단 child 로 그림.
	//트리 mutation 후 sync_scrollbar() 외부 호출 가능.
	CSCScrollbar	m_scrollbar;	//vertical
	CSCScrollbar	m_scrollbar_h;	//horizontal
	int				m_h_wheel_accum = 0;	//mouse driver 가 한 번 굴림에 작은 zDelta 다수 메시지 보낼 때 누적해서 WHEEL_DELTA 단위로 process.
	int				m_h_scroll_pos = 0;		//사용자 H scroll 위치(px). 0..(m_h_content_width - viewport). 전량 customdraw 의 over_shift paint-shift 로 시각화.
	int				m_h_content_width = 0;	//sync_scrollbar 가 가시 항목 라벨 right 최댓값으로 측정한 자연 콘텐츠 폭. native si 대신 H scroll 의 source of truth.
	//콘텐츠 폭 캐시 — measure_content_width 는 전체 항목을 순회하므로 WinSxS(2.9만 개) 류에서 비싸다.
	//sync_scrollbar 는 스크롤·페인트마다 호출되므로 매번 재측정하면 세로 스크롤이 막힌다. expand/collapse·폰트
	//변경 시 dirty 로, insert/delete 는 GetCount() 변화로 감지해 *변경 시에만* 재측정하고 그 외엔 캐시를 쓴다.
	bool			m_content_width_dirty = true;
	int				m_content_width_count = -1;	//마지막 측정 시점의 GetCount().
	int				m_content_width_cache = 0;
	int				get_content_width();		//캐시 반환 — dirty 거나 항목 수가 바뀌었을 때만 measure_content_width() 재호출.

	//가시(펼쳐진/non-collapsed) 항목 캐시 — 세로 스크롤(휠/드래그/sync_scrollbar)이 visible-index ↔ HTREEITEM
	//매핑을 GetNextVisibleItem O(n) walk(각 SendMessage)로 하던 것을 O(1) 로 대체. WinSxS(2.9만) 류에서 매 스크롤
	//수만 SendMessage 가 세로 스크롤을 멈추게 했다. expand/collapse·폰트 변경(dirty)·insert/delete(GetCount 변화)
	//시에만 1회 재빌드.
	std::vector<HTREEITEM>				m_visible_items;	//display 순서(펼쳐진 항목만).
	std::unordered_map<HTREEITEM, int>	m_visible_index;	//item → index.
	bool			m_visible_cache_dirty = true;
	int				m_visible_cache_count = -1;
	void			ensure_visible_cache();					//dirty/항목수 변화 시 재빌드.
	int				get_visible_total();					//펼쳐진 항목 총수 (O(1) after cache).
	int				index_of_visible(HTREEITEM hItem);		//item → visible index, 없으면 -1.
	HTREEITEM		visible_at(int index);					//visible index → item, 범위 밖이면 NULL. 숨은 항목이면 재빌드 후 보이는 항목 반환(자가치유).
	bool			is_item_displayable(HTREEITEM hItem);	//모든 조상이 펼쳐져 현재 스크롤로 도달 가능한 항목인지.
	int				m_h_natural_max = -1;	//over_shift 활성 게이트(>=0 이면 활성). native 스크롤을 안 쓰므로 활성 시 0.
	bool			m_h_internal_thumb = false;	//우리 코드가 발사한 SB_THUMBPOSITION 인지 마킹 (외부 driver / 자체 발사 구분).
	bool			m_v_visible_state = false;	//sync_scrollbar 가 결정한 V overlay visible — customdraw 가 timing 무관하게 정확한 값 사용.
	bool			m_h_visible_state = false;	//H overlay visible 동일. OnNcCalcSize 가 이 값으로 하단 NC(가로바 자리) 예약 여부 결정.
	bool			m_scrollbar_setup = false;
	bool			m_syncing = false;		//sync_scrollbar 재진입 가드 — need_h 변화로 SWP_FRAMECHANGED 발사 시 중첩 sync 가 또 발사하지 않도록.
	void			setup_scrollbar();		//PreSubclassWindow 끝에서 호출 — WS_VSCROLL 제거 + scrollbar 생성.
	void			sync_scrollbar();		//트리 scroll state → scrollbar 모델 push. 외부에서도 batch insert/delete 후 호출 가능.
	LRESULT			on_message_CSCScrollbar(WPARAM wParam, LPARAM lParam);

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
		timer_vscroll_apply,				//세로 스크롤바 드래그 coalesce — 연속 pos 는 목표값만 갱신, 타이머가 마지막만 적용.
		timer_drag_auto_scroll,				//트리에서 드래그 중 대상 컨트롤 가장자리 호버 시 연속 자동 스크롤.
	};

	int				m_drag_scroll_vx = 0;		//드래그 자동 스크롤 속도(가로, tick당 level, 부호=방향). 0=안 함.
	int				m_drag_scroll_vy = 0;		//드래그 자동 스크롤 속도(세로).
	void			update_drag_auto_scroll(CPoint screen_pt);	//드래그 중 대상(m_pDropWnd) 가장자리 거리로 속도 산출 + 타이머 관리.
	void			cancel_drag();								//드래그 중 ESC 등으로 드롭 없이 완전 취소.

	int				m_pending_vscroll_pos = -1;	//timer_vscroll_apply 가 적용할 최신 V 스크롤 목표 pos(-1 = 없음).

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
	bool			m_use_own_context_menu = true;	//true=자체 내장 컨텍스트 메뉴 표시, false=parent(OnContextMenu)로 위임. default = true
	void			OnPopupMenu(UINT nID);
	LRESULT			OnMessageCSCMenu(WPARAM wParam, LPARAM lParam);

	bool			m_draw_border = false;

//마우스가 컨트롤 안에 들어온 경우 true
	bool			m_is_hovering = false;
	HTREEITEM		m_hot_item = NULL;	//Y 좌표 기준 row 의 어디든 hover 시 hot 으로 인식 — native HitTest 가 label 외 영역에서 hItem 반환 안 하는 케이스 보완.


//들여쓰기 크기
	int				m_indent_size = 16;
	bool			m_use_checkbox = false;
	bool			m_use_expand_button = true;

	bool			m_show_area = false;

	CTheme			m_winctrl_theme;
	bool			m_winctrl_theme_initialized = false;
	void			winctrl_theme_init();
	void			draw_checkbox(CDC* pDC, CRect r, int check_state);

	//tree 자체 H scroll max 초과 over-scroll 픽셀 수 (>=0). customdraw 가 콘텐츠를 이만큼 좌측으로 추가
	//shift 한다. get_checkbox_rect / get_expand_button_rect 는 unshifted GetItemRect 기준이라 이 값을 따로
	//빼줘야 over-scroll 끝에서 체크박스·버튼이 콘텐츠와 함께 스크롤된다.
	int				get_over_shift() const;

	//전체 펼침(expanded) 항목 기준 자연 콘텐츠 폭(px) — H 스크롤바 표시/범위의 source of truth.
	//수직 스크롤 위치와 무관하게 동일 값(고정). 가시 항목 하나로 text-left 의 level 선형관계를 보정한 뒤,
	//모든 펼침 항목의 라벨 폭을 DC 로 측정해 최댓값을 취한다. (native si 는 WS_HSCROLL strip 으로 신뢰 불가.)
	int				measure_content_width();

	//customdraw 의 체크박스 그리기 위치 — OnLButtonDown 의 hit-test 와 일관성 위해 helper. TVS_CHECKBOXES 없을 시 empty rect.
	CRect			get_checkbox_rect(HTREEITEM hItem);

	//customdraw 의 expand button (▶/▼) 그리기 위치 + hit-test padding. TVS_HASBUTTONS 없거나 children 없을 시 empty rect.
	CRect			get_expand_button_rect(HTREEITEM hItem);

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
	//drag&drop으로 노드 순서변경
	enum DropPosition
	{
		drop_on_item = 0,		// 노드 위에 놓기 → parent 변경 (child로 이동)
		drop_before_item,		// 노드 앞에 놓기 → 같은 레벨에서 순서 변경
		drop_after_item,		// 노드 뒤에 놓기 → 같은 레벨에서 순서 변경
	};

	bool			m_use_drag_and_drop = false;	//노드 이동기능 사용여부. default = false
	bool			m_use_rearrange_order = false;	//노드 위치조정기능 사용여부. default = false
	CWnd*			m_pDragWnd = nullptr;		//Which wnd we are dragging FROM
	CWnd*			m_pDropWnd = nullptr;		//Which wnd we are dropping ON
	CImageList*		m_pDragImage = nullptr;		//For creating and managing the drag-image
	bool			m_bDragging = false;		//T during a drag operation
	std::deque<UINT> m_drag_images_id;			//drag할 때 사용하는 이미지들의 resource id 저장(단일파일용 이미지, 싱글파일용 이미지를 차례대로 넣고 drag되는 개수에 따라 맞는 이미지를 사용한다)
	void			DroppedHandler(CWnd* pDragWnd, CWnd* pDropWnd);
	//https://jiniya.net/tt/594/
	//이 함수는 드래그 이미지를 직접 생성해주는 코드지만 취약점이 많은 코드이므로 참고만 할것.
	CImageList*		create_drag_image(CTreeCtrl* pList, LPPOINT lpPoint);
	void			create_drag_image(CSCGdiplusBitmap& drag_img);

	// 드롭 위치 판별 및 삽입 마크 관련
	DropPosition	m_dropPosition = drop_on_item;
	HTREEITEM		m_hInsertMarkItem = NULL;		// 삽입 마크가 표시될 아이템
	DropPosition	hit_test_drop_position(CPoint ptClient, HTREEITEM hItem);
	void			draw_insert_mark(CDC* pDC);
	void			clear_insert_mark();
	BOOL			move_tree_item_as_sibling(CTreeCtrl* pTree, HTREEITEM hSrcItem, HTREEITEM hRefItem, bool bAfter);

//편집 관련
	bool			m_allow_edit = true;
	bool			m_in_editing = false;		//편집중인지
	bool			m_in_context_menu = false;	//우클릭 메뉴 표시 중 — focus 가 menu 로 가도 selected 항목을 active 색으로 그리기 위한 가드
	CSCStaticEdit*	m_pEdit = NULL;
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

//json load
	bool			load_tree_from_json_document(const rapidjson::Document& doc);
	bool			ReadFileToString(const CString& filePath, std::string& outText);
	// JSON 객체 1개를 Tree 노드로 재귀 복원
	HTREEITEM		JsonValueToTreeItem(const rapidjson::Value& nodeValue, HTREEITEM hParent, HTREEITEM hInsertAfter);

//json save
	void			tree_to_json_document(rapidjson::Document& doc);
	// Tree 노드 1개를 JSON 객체로 재귀 변환
	rapidjson::Value tree_item_to_json_value(HTREEITEM hItem, rapidjson::Document::AllocatorType& alloc);

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
	afx_msg BOOL OnTvnItemexpanding(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg BOOL OnNMClick(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg BOOL OnTvnBegindrag(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg BOOL OnNMDblclk(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
	afx_msg BOOL OnTvnBeginlabeledit(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg BOOL OnTvnEndlabeledit(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg BOOL OnTvnItemexpanded(NMHDR* pNMHDR, LRESULT* pResult);
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
	afx_msg void OnNcPaint();
	afx_msg void OnNcCalcSize(BOOL bCalcValidRects, NCCALCSIZE_PARAMS* lpncsp);
	afx_msg void OnStyleChanging(int nStyleType, LPSTYLESTRUCT lpStyleStruct);
};


