#pragma once

#include <deque>
#include <Afxwin.h>
#include "list_data.h"
#include "HeaderCtrlEx.h"
#include "../../GdiplusBitmap.h"
#include "../../Functions.h"
#include "../../system/ShellImageList/ShellImageList.h"

/*
* CVtListCtrlEx를 사용하기 위해 프로젝트에 추가해야 하는 소스들
* Common/CListCtrl/CVtListCtrlEx/*
* Common/Functions.*
* Common/colors.*
* Common/system/ShellImageList.*
* Common/GdiPlusBitmap.*
*/

/*
CListCtrl의 각 라인을 CListCtrlData라는 클래스의 인스턴스로 생성하여
virtual list 기능을 이용하도록 작성된 클래스.
resource editor에서
virtual list 기능을 이용하기 위해 Owner Data를 true로,
DrawItem을 이용해서 직접 그리기 위해 Owner Draw Fixed를 true로 해줘야 한다.

<주의!>
각 라인의 데이터가 m_list_db라는 deque로 저장되고 이를 이용하여 virtual list로 동작하는 것이므로
데이터 추가, 삭제시에 반드시 m_list_db 또한 같이 변경되어야 한다.
따라서 기존 CListCtrl의 InsertItem, DeleteItem, DeleteAllItems등과 같은 함수는
그대로 사용해서는 안된다.
반드시 새로 정의한 insert_item, add_item, delete_item, delete_all_items와 같은 함수를 이용해서
추가, 삭제가 되어야 한다.
또한 GetItemText는 정해진 텍스트를 읽어오는 함수이므로 써도 관계없으나
SetItemText는 단지 리스트에만 쓰고 실제 m_list_db에는 써지지 않는 함수이니
반드시 set_text 함수를 이용해서 써야 한다.

<주의!>
리스트 컨트롤의 column과 header는 사실 같은 개념일 수 있지만
text align과 같이 리스트의 정렬 방식과 header의 정렬 방식을 다르게 설정할 수 있듯이
리스트 컨트롤의 column과 관련된 것은 column으로,
헤더 컨트롤의 column과 관련된 것은 header라는 키워드를 사용하자.

[편집 완료 시]
-멀티 선택 상태에서 편집 완료될 때 shift가 눌린채로 완료되면 멀티 항목 모두 수정된 내용으로 변경된다.

<사용법 예시>
m_list.SetExtendedStyle(LVS_EX_GRIDLINES | LVS_EX_FLATSB | LVS_EX_CHECKBOXES | LVS_EX_FULLROWSELECT | LVS_EX_INFOTIP);
m_list.UsePopupMenu(false);
m_list.AllowSort();
m_list.set_color_theme(CVtListCtrlEx::color_theme_dark_gray);
m_list.set_progress_color(RGB(128, 255, 128));
m_list.set_headings(_T("파일,200;스냅샷,60;분류,60;평점,60;크기,200;메모,100;경로,300;날짜,100"));
m_list.set_header_height(24);

<수정될 내용>
-어떤 컬럼이 이미지만 표시할 경우가 있으므로 column type을 추가하고 sort도 그에 맞게 처리되어야 한다.
*/
// CVtListCtrlEx

#define LCSB_CLIENTDATA 1
#define LCSB_NCOVERRIDE 2

static const UINT Message_CVtListCtrlEx = ::RegisterWindowMessage(_T("MessageString_CVtListCtrlEx"));

class CVtListCtrlExMessage
{
public :
	CVtListCtrlExMessage(CWnd* _this, int _message, CWnd* _pTarget = NULL)
	{
		pThis = _this;
		message = _message;
		pTarget = _pTarget;
	}

	CWnd*	pThis = NULL;
	CWnd*	pTarget = NULL;
	int		message;
};

class CVtFileInfo
{
public:
	CVtFileInfo(CString _path, uint64_t _size = 0, CString _date = _T(""))
	{
		if (_size == 0)
		{
			if (PathIsDirectory(_path))
				_size = 0;
			else
				_size = get_file_size(_path);
		}

		if (_date.IsEmpty())
		{
			_date = get_datetime_string(GetFileLastModifiedTime(_path), 2, true, _T(" "), false, false);
		}

		path = _path;
		size = _size;
		date = _date;
	}

	//sort의 lamda등에서도 인덱스로 접근하도록 하기 위해 text[3];과 같이 배열로 선언했었으나
	//3개밖에 되지 않고 명확히 사용하고자 타입에 맞게 선언함
	CString		path;
	uint64_t	size;
	CString		date;
};


class CVtListCtrlEx : public CListCtrl
{
	DECLARE_DYNAMIC(CVtListCtrlEx)

public:
	CVtListCtrlEx();
	virtual ~CVtListCtrlEx();

	void		modify_style();

	BOOL NCOverride;
	int Who;

	BOOL m_bHorizLine = TRUE;
	void HideScrollBars(int Type, int Which = SB_BOTH);

	enum CVtListCtrlExMsgs
	{
		message_progress_pos = 0,
		message_drag_and_drop,
	};


//ShellList로 동작시키기
	bool		m_is_shell_listctrl = false;
	bool		m_is_shell_listctrl_local = true;
	bool		is_shell_listctrl() { return m_is_shell_listctrl; }
	bool		is_shell_listctrl_local() { return m_is_shell_listctrl_local; }
	//맨 처음 이 명령을 주면 모든 세팅과 동작이 ShellListCtrl로 동작된다.
	//세팅 이후 ShellList가 아닌 형태로 동작시키는 등은 허용하지 않는다.
	//is_local이 true이면 파일목록을 직접 얻어와서 표시하지만
	//false, 즉 remote일 경우는 파일목록을 받아서 표시해야 한다.
	void		set_as_shell_listctrl(bool is_local = true);
	//local일 경우는 경로만 주면 자동으로 폴더목록 표시
	CString		get_path() { return m_path; }
	//path를 받아 m_path에 저장하고 refresh_list()를 호출한다.
	void		set_path(CString path, bool refresh = true);
	//local이 아닐 경우는 파일목록을 받아서 표시한다.
	void		set_filelist(std::deque<CVtFileInfo>* pFolderList, std::deque<CVtFileInfo>* pFileList);

	//reload가 true라면 local의 파일목록을 다시 불러오지만 false라면 목록을 다시 리스트에 표시한다
	//(sort의 경우 폴더/파일 목록을 sort하여 보여줄 뿐 파일목록을 reload하진 않는다)
	//다른 코드에 의해 이미 m_cur_folders/m_cur_files 가 채워졌다면 reload를 false로 호출한다.
	void		refresh_list(bool reload = true);

	//폴더와 파일을 별도로 처리한 이유는 정렬시에 파일과 폴더가 별도 처리되기 때문
	std::deque<CVtFileInfo> m_cur_folders;
	std::deque<CVtFileInfo> m_cur_files;
	//파일 또는 폴더를 해당하는 멤버 리스트에 추가한다.
	//local인 경우 크기와 날짜등이 비어있다면 자동 채워주고 remote라면 비어있으면 안된다.
	void		add_file(CString path, uint64_t size, CString date = _T(""), bool is_remote = false, bool is_folder = false);
	//local이 아닌 remote의 경우 넘겨받은 구조체값으로 목록을 추가한다.
	void		add_file(WIN32_FIND_DATA* pFindFileData);

	enum SHELL_LIST_COLUMN
	{
		col_filename = 0,
		col_filesize,
		col_filedate,
	};

	CString		m_path;
	CShellImageList*	m_pShellImageList = NULL;
	void		set_shell_imagelist(CShellImageList* pShellImageList) { m_pShellImageList = pShellImageList; }

//컬럼 관련
	//ex. "No,20;Item1,50;Item2,50"
	//컬럼 설정과 관련된 변수의 저장 크기는 컬럼의 수가 결정되면 정해진다.
	//따라서 set_headings()함수를 가장 먼저 호출하고 그 후에 컬럼 세팅을 하도록 하자.
	bool		set_headings(const CString& strHeadings);
	
	int			get_column_count();

	//default = LVCFMT_LEFT
	void		set_column_text_align(int column, int format = LVCFMT_LEFT, bool set_to_header = true);
	int			get_column_text_align(int column);

	//해당 컬럼에서 문자열 길이 최대값을 리턴.
	//real_
	int			get_column_max_text_length(int column, bool real_bytes);
	void		get_column_max_text_length(std::vector<int>& col_len, bool real_bytes);
	//해당 컬럼에서 출력시에 width 최대값을 리턴.(미구현)
	int			get_column_max_text_width(int column);

	CString		get_header_text(int column);
	void		set_header_text(int column, CString text);
	//default = HDF_LEFT
	void		set_header_text_align(int header, int format = HDF_LEFT);
	int			get_header_text_align(int header);

	enum
	{
		column_data_type_unknown = 0,
		column_data_type_text,
		column_data_type_text_ip,		//ip는 text와 동일하지만 sort 방식이 다르다.
		column_data_type_numeric,

		//실제값은 정수지만 그려질땐 막대그래프로 그려짐. 100%가 최대값이므로 %로 변환하여 값을 줘야 함.
		//100넘는 값도 100%로 표시된다.
		//만약 최대값을 지정하거나 등의 다른 처리가 필요하다면 부가적인 코드가 필요하므로 우선 100%를 max로 가정함.
		//일단 이 컬럼의 데이터 타입은 percentage를 막대그래프로 그려주는 것이 목적이므로
		//값은 0~100까지의 비율로 아예 넣어주자. 110으로 넣어도 관계없다. 다만 100%로 그려질 것이다.
		column_data_type_percentage_bar,
		column_data_type_percentage_grid,
		//progress는 막대그래프 모양이 아닌 실제 진행상태를 표시하는 ProgressBar에 가깝다.
		column_data_type_progress,
	};
	int			get_column_data_type(int column);
	void		set_column_data_type(int column, int nType = column_data_type_text, bool invalidate = false);

//추가 관련
	//index 위치에 0번 컬럼이 text인 라인을 추가한다.(-1이면 맨 마지막에 추가)
	int			add_item(CString text = _T(""), bool ensureVisible = true, bool invalidate = true);
	int			add_line_string_item(CString line_string, TCHAR separator = '|', bool ensureVisible = true, bool invalidate = true);
	int			insert_item(int index, CString text = _T(""), bool ensureVisible = true, bool invalidate = true);
	int			insert_item(int index, std::deque<CString> dqText, bool ensureVisible = true, bool invalidate = true);

	//여러개의 인자를 args에 주니 모두 기록되지 않는 현상이 있다. 확인 필요!!
	template <typename ... Types> int insert_item(int index, Types... args)
	{
		int n = sizeof...(args);
		CString arg[] = { args... };

		int subItem = 0;
		if (index < 0)
			index = size();

		for (auto item : arg)
		{
			if (subItem == 0)
				index = insert_item(index, item, false, false);
			else
				set_text(index, subItem, item, false);
			subItem++;
		}

		return index;
	}

	int			size();

//삭제 관련
	//지우기 전 확인창은 호출루틴에서 처리해야 함
	void		delete_selected_items();
	void		delete_item(int index);
	//라인 전체가 공백이면 해당 라인을 삭제한다.
	void		delete_empty_lines();
	void		delete_all_items(bool delete_file_list = true);
	void		DeleteAllItems() { delete_all_items(); }

//텍스트 관련
	CString		get_text(int item, int subItem);
	void		set_text(int item, int subItem, CString text, bool invalidate = true);
	//from ~ to 컬럼까지의 텍스트를 하나로 합쳐서 리턴해준다.
	CString		get_line_text(int index, int from = 0, int to = -1, CString sep = _T("|"));
	CString		get_line_text(int index, std::deque<int>* dqColumn = NULL, CString sep = _T("|"));
	std::deque<CString> get_line_text_list(int index, int from = 0, int to = -1);
	std::deque<CString> get_line_text_list(int index, std::deque<int>* dqColumn = NULL);
	//txt 출력 시 컬럼 세로 정렬을 맞게 출력하도록 공백을 보정하여 리턴
	//CString은 최대 64K까지만 담을 수 있으므로 vector를 이용함.
	void		get_line_text_list(std::vector<CString> *vt);

	//리스트에 표시할 항목이 없을 경우 표시할 텍스트 설정
	void		set_text_on_empty(CString text) { m_text_on_empty = text; Invalidate(); };

//선택 관련
	//start부터 맨 처음 선택된 항목의 인덱스를 리턴한다.
	int			get_selected_index(int start = 0);
	//맨 끝에서부터 선택된 한 항목의 인덱스를 리턴한다.
	int			get_last_selected_item();
	//선택된 항목들을 dqSelected에 담는다. dqSelected가 null이면 그냥 선택 갯수를 리턴받아 사용한다.
	int			get_selected_items(std::deque<int> *dqSelected = NULL);
	//index = -1 : 전체선택
	void		select_item(int index, bool select = true, bool after_unselect = false);
	void		unselect_selected_item();

	//아이템의 상태값이 특정 상태값이 항목 또는 그 개수 구하기
	//LVIS_DROPHILITED or LVIS_SELECTED 항목을 구할 수 있다.
	//drag 도중에 마우스가 다른 앱 영역으로 나가서 WM_LBUTTONUP 될 경우 drophilited 상태로 아이템이 남는 문제를 제거하기 위해.
	int			get_items_state(UINT state, std::deque<int>* dq = NULL);
	//dq 목록의 아이템들의 state 세팅. dq가 null이면 모든 항목에 대해 실행
	//선택 : set_items_state(LVIS_SELECTED|LVIS_FOCUSED, LVIS_SELECTED|LVIS_FOCUSED, dq);
	//해제 : set_items_state(0, LVIS_SELECTED|LVIS_FOCUSED, dq);
	int			set_items_state(UINT state, UINT mask, std::deque<int>* dq = NULL);

//검색 관련
	//0번 컬럼에서만 데이터를 찾는다.
	int			find_string(CString str, int start = -1, bool bWholeWord = false, bool bCaseSensitive = false);
	//separator가 ""이 아닌 공백 또는 다른 문자열일 경우는
	//str을 분리해서 모두 찾는다.
	void		find_string(CString find_target, std::deque<int>* result,
							int start_idx = 0, int end_idx = -1, int start_column = 0, int end_column = -1,
							bool result_reset = true, bool bWholeWord = false, bool bCaseSensitive = false, bool select = false);
	int			find_string(CString find_target, std::deque<int>* result,
							int start_idx = 0, int end_idx = -1,
							std::deque<int>* dqColumn = NULL, bool stop_first_found = false);

//정렬 관련
	enum listctrlex_sort_type
	{
		sort_none = -1,
		sort_descending,
		sort_ascending,
	};
	void		allow_sort(bool allow) { m_allow_sort = allow; }
	void		sort(int column, int ascending);
	void		sort_by_text_color(int column, int ascending, bool text_sort_on_same_color = true);
	int			cur_sorted_column_index() { return m_cur_sorted_column; }

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

	void		set_color_theme(int theme, bool apply_now = true);
	COLORREF	get_text_color(int item, int subItem);
	COLORREF	get_back_color(int item, int subItem);
	//특정 셀이 아닌 기본 배경색을 리턴.
	COLORREF	get_back_color() { return m_crBack; }
	COLORREF	get_back_alt_color(int item, int subItem) { return m_crBackAlt; }
	void		set_back_alt_color(COLORREF cr) { m_crBackAlt = cr; }

	//특정 항목의 글자색 설정. erase가 true이면 crText 인자를 무시하고 기본 글자색으로 되돌린다.
	//item이 -1이면 모든 라인에, subItem이 -1이면 모든 컬럼에 적용.
	void		set_text_color(int item, int subItem, COLORREF crText, bool erase = false, bool invalidate = true);
	//특정 항목의 배경색 설정. erase가 true이면 crText 인자를 무시하고 기본 글자색으로 되돌린다.
	void		set_back_color(int item, int subItem, COLORREF crBack, bool erase = false);
	void		set_item_color(int item, int subItem, COLORREF crText, COLORREF crBack);

	//기본 글자색을 설정한다.
	void		set_default_text_color(COLORREF cr_text) { m_crText = cr_text; Invalidate(); }
	//기본 배경색을 설정한다.
	void		set_default_back_color(COLORREF cr_back) { m_crBack = cr_back; Invalidate(); }

	//컬럼이 percentage를 표시하는 컬럼일 경우 그 표시 색상을 설정한다.
	void		set_progress_color(COLORREF crProgress);
	template <typename ... Types> void set_percentage_gradient(Types... args)
	{
		int n = sizeof...(args);
		COLORREF arg[] = { args... };

		m_crPercentage.clear();

		for (auto item : arg)
		{
			m_crPercentage.push_back(item);
		}
	}


//편집 관련
	//리소스의 ListCtrl 속성에서 "레이블 편집" 속성은 default값인 false로 하고 이 함수로 편집 가능 여부를 설정해야 한다.
	void		allow_edit(bool allow_edit = true, bool one_click_edit = true);
	void		allow_one_click_edit(bool allow_one_click_edit) { m_allow_one_click_edit = allow_one_click_edit; }
	void		allow_edit_column(int column, bool allow_edit = true);
	bool		is_in_editing()	{ return m_in_editing; }	//편집중인지
	void		set_flag_in_editing(bool in_editing) { edit_end(); m_in_editing = in_editing; }
	int			get_recent_edit_item() { return m_edit_item; }
	int			get_recent_edit_subitem() { return m_edit_subItem; }
	void		set_recent_edit_subitem(int subItem) { m_edit_subItem = subItem; }
	std::deque<int>* get_selected_list_for_edit() { return &m_dqSelected_list_for_edit; }
	std::deque<int> m_dqSelected_list_for_edit;
	CString		get_old_text() { return m_old_text; }
	CEdit*		edit_item(int item, int subItem);
	void		edit_end(bool valid = true);
	void		undo_edit_label();		//편집 전의 텍스트로 되돌린다.(예를 들어 편집 레이블이 파일명이고 파일명 변경이 실패한 경우 쓸 수 있다.)
	bool		is_modified() { return m_modified; }
	void		reset_modified_flag() { m_modified = false; }

//폰트 관련. 반드시 set_headings() 후에 호출할것.
	void		set_font_name(LPCTSTR sFontname, BYTE byCharSet = DEFAULT_CHARSET);
	//-1 : reduce, +1 : enlarge
	int			get_font_size();
	//반드시 set_headings() 후에 호출할것.
	void		set_font_size(int font_size);
	void		enlarge_font_size(bool enlarge);
	void		set_font_bold(bool bold = true);
	void		set_font_italic(bool italic = true);
	LOGFONT		get_log_font() { return m_lf; }
	void		set_log_font(LOGFONT lf);

//파일, 클립보드 관련
	//Ctrl+C키로 선택된 항목을 클립보드로 복사할 수 있다. shift를 조합하면 헤더까지 포함된다.
	bool		list_copy_to_clipboard(bool selected_only = true, TCHAR separator = '|', bool bHead = false);
	//void	paste_from_clipboard();
	bool		save(CString file, TCHAR separator = '|', bool selected_only = false);
	bool		load(CString sfile, TCHAR separator = '|', bool add_index = false, bool match_column_count = true, bool reset_before_load = true);

//기타 레이아웃 관련
	//header_height는 헤더클래스에서 HDM_LAYOUT 메시지가 발생해야 적용되는데
	//그렇게 되기 위해서는 SetFont / MoveWindow / SetWindowPos 등이 필요하므로
	//VtListCtrlEx에게 set_font_name() 호출시에 헤더도 SetFont를 적용하여 해결함.
	//=> SetFont()를 통해서 header의 height를 변경한다.
	void		set_header_height(int height);

	//line height를 변경하는 방법은 가상의 이미지리스트를 이용하는 방법과
	//(실제 사용할 이미지리스트가 있는 경우는 위 방법을 사용할 수 없다)
	//MeasureItem을 이용하는 방법(OwnerDrawFixed only)이 있다.
	//(강제 갱신 방법 아직 미해결이며 resize를 하면 자동 적용되므로
	//우선은 mainDlg에서 이 함수 호출후에 RestoreWindowPosition()등과 같이
	//CVtListCtrlEx에 WM_SIZE가 발생하도록 하는 함수를 사용하면 된다)
	//CVtListCtrlEx가 아닌 그냥 mainDlg만 resize한다고 해서 되지 않는다.
	//자체 imagelist를 사용하지 않는 컨트롤이라면 쉽게 lineheight를 조절할 수 있다.
	bool		m_use_own_imagelist = true;
	void		set_use_own_imagelist(bool use) { m_use_own_imagelist = use; }
	void		set_line_height(int height);

	void		set_column_width(int nCol, int cx);
	void		load_column_width(CWinApp* pApp, CString sSection);
	void		save_column_width(CWinApp* pApp, CString sSection);
	CRect		get_item_rect(int item, int subItem);
	//클릭위치에 따라 item은 올바르게 판별되나 subItem은 그렇지 않아서(마우스 이벤트 핸들러 함수에서) 새로 추가함.
	bool		get_index_from_point(CPoint pt, int& item, int& subItem, bool include_icon);

	void		show_progress_text(bool show = true) { m_show_progress_text = show; Invalidate(); }
	void		progress_text_color(COLORREF cr) { m_crProgressText = cr; }

//scroll
	enum CLISTCTRLEX_ENSURE_VISIBLE_MODE
	{
		visible_first = 0,
		visible_center,
		visible_last,
	};
	//어떤 항목이 특정 위치에 표시되도록 스크롤시킨다.
	//mode가 visible_first이고 offset이 3이면 위에서 3+1인 위치에 해당 아이템이 표시되도록 스크롤시킨다.
	//mode가 visible_center이고 offset이 0이면 중앙 위치에 해당 아이템이 표시되도록 스크롤시킨다.
	//mode가 visible_last이고 offset이 3이면 아래에서 -3-1인 위치에 해당 아이템이 표시되도록 스크롤시킨다.
	//mode default = visible_center
	void			ensure_visible(int index, int mode = visible_center, int offset = 0);
	//original IsItemVisible method does not check partial. only check vertical axis.
	bool			is_item_visible(int index, bool bPartial = false);

	//항목이 추가되면 auto scroll되지만 특정 항목을 선택하면 false로 된다.
	bool			m_auto_scroll = true;


	void random();

	DWORD index_from_point(int x, int y);

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
	int				get_drop_index() { return m_nDropIndex; }
	void			capture_selected_items_to_bitmap(CGdiplusBitmap* bmp);

protected:

//메인 데이터
	std::deque<CListCtrlData> m_list_db;
	
	CString			m_text_on_empty;

//컬럼 관련
	CHeaderCtrlEx	m_HeaderCtrlEx;
	std::deque<int> m_column_data_type;
	std::deque<int> m_column_text_align;

	int				m_line_height = 16;

//정렬 관련
	bool			m_allow_sort = true;
	std::deque<int> m_column_sort_type;			//asceding or descending
	int				m_cur_sorted_column = 0;	//정렬된 컬럼 인덱스(색상 정렬은 제외)

//컬러 관련
	COLORREF		m_crText;					//기본 글자색
	COLORREF		m_crTextSelected;			//선택 항목의 활성화(active) 글자색
	COLORREF		m_crTextSelectedInactive;	//선택 항목의 비활성화(inactive) 글자색
	COLORREF		m_crTextDropHilited;
	COLORREF		m_crBack;					//기본 배경색
	COLORREF		m_crBackAlt;				//줄 구분용
	COLORREF		m_crBackSelected;
	COLORREF		m_crBackSelectedInactive;
	COLORREF		m_crBackDropHilited;
	COLORREF		m_crSelectedBorder;
	COLORREF		m_crHeaderBack;
	COLORREF		m_crHeaderText;
	std::deque<COLORREF> m_crPercentage;		//percentage bar graph color
	COLORREF		m_crProgress;				//progress bar
	COLORREF		m_crProgressText;			//progress text

//편집기능 관련
	bool			m_allow_edit = false;			//항목 편집이 가능한지...
	bool			m_allow_one_click_edit = false;	//항목을 클릭한 후 정해진 시간내에 다시 클릭될 때 편집모드로 할지
	std::deque<bool> m_allow_edit_column;			//각 컬럼마다 편집가능 여부를 설정할 수 있다. 단, m_allow_edit이 변경되면 모두 그 값으로 변경됨에 주의.
	bool			m_modified = false;				//항목의 차례 또는 내용이 수정된 경우 true
	bool			m_in_editing = false;			//편집중인지
	int				m_edit_item = -1;				//편집중인 아이템 인덱스
	int				m_edit_subItem = -1;			//편집중인 아이템 서브인덱스
	CString			m_old_text = _T("");			//편집 전 텍스트
	CEdit*			m_pEdit = NULL;
	long			m_last_clicked_time = 0;		//one_click으로 편집모드 진입 시 마지막 클릭 시각
	int				m_last_clicked_index = -1;		//one_click으로 편집모드 진입 시 마지막 클릭 인덱스

//폰트 관련
	LOGFONT			m_lf;
	CFont			m_font;
	int				m_font_size;
	void			reconstruct_font();

	bool			m_show_progress_text = true;

//Drag&Drop 드래깅 관련
	bool			m_use_drag_and_drop = false;
	CWnd*			m_pDragWnd = NULL;			//Which ListCtrl we are dragging FROM
	CWnd*			m_pDropWnd = NULL;			//Which ListCtrl we are dropping ON
	CImageList*		m_pDragImage = NULL;		//For creating and managing the drag-image
	bool			m_bDragging = false;		//T during a drag operation
	int				m_nDragIndex = -1;			//Index of selected item in the List we are dragging FROM
	int				m_nDropIndex = -1;			//Index at which to drop item in the List we are dropping ON(drag를 시작한 컨트롤의 멤버값에 저장됨, 드롭된 클래스에는 저장되지 않음)
	std::deque<UINT> m_drag_images_id;			//drag할 때 사용하는 이미지들의 resource id 저장(단일파일용 이미지, 싱글파일용 이미지를 차례대로 넣고 drag되는 개수에 따라 맞는 이미지를 사용한다)
	void			DroppedHandler(CWnd* pDragWnd, CWnd* pDropWnd);

protected:
	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnLvnGetdispinfo(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnLvnOdfinditem(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnLvnOdcachehint(NMHDR *pNMHDR, LRESULT *pResult);
	virtual void DrawItem(LPDRAWITEMSTRUCT /*lpDrawItemStruct*/);
	afx_msg void OnLvnColumnclick(NMHDR *pNMHDR, LRESULT *pResult);
	virtual void PreSubclassWindow();
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	afx_msg void OnLvnOdstatechanged(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg BOOL OnEraseBkgnd(CDC* pDC);
	afx_msg void OnPaint();
	afx_msg void OnLvnBeginlabeledit(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg BOOL OnLvnEndlabeledit(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg BOOL OnNMClickList(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg BOOL OnNMDblclk(NMHDR *pNMHDR, LRESULT *pResult);
	//afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	//afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
//	afx_msg void OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
	afx_msg void OnDropFiles(HDROP hDropInfo);
	afx_msg void MeasureItem(LPMEASUREITEMSTRUCT lpMeasureItemStruct);
	afx_msg void OnLvnBeginDrag(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
	afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
	afx_msg BOOL OnLvnItemchanged(NMHDR* pNMHDR, LRESULT* pResult);
};


