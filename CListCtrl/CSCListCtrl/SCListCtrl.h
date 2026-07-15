#pragma once

#include <deque>
#include <list>
#include <functional>
#include <Afxwin.h>
//20260707 by claude. HeaderCtrlEx/list_data 세트를 CSCListCtrl 폴더로 복사(HeaderCtrlEx→SCHeaderCtrl, class CSCHeaderCtrl).
//원본(CVtListCtrlEx 폴더)은 다른 프로젝트가 아직 사용하므로 이 프로젝트 빌드에서만 제외하고 이 복사본을 쓴다(전 프로젝트 이행 완료 후 원본 폴더 제거 예정).
#include "list_data.h"
#include "SCHeaderCtrl.h"
//#include "../../GdiplusBitmap.h"
#include "../../CButton/GdiButton/GdiButton.h"
#include "../../Functions.h"
#include "../../colors.h"
#include "../../system/ShellImageList/ShellImageList.h"
#include "../../CEdit/CSCStaticEdit/SCStaticEdit.h"
#include "../../CScrollbar/SCScrollbar/SCScrollbar.h"
#include "../../CDialog/SCShapeDlg/SCShapeDlg.h"		//20260704 by claude. 드래그 이미지를 레이어드 팝업으로(깜빡임 없음)

/*
* CSCListCtrl를 사용하기 위해 프로젝트에 추가해야 하는 소스들
* Common/CListCtrl/CSCListCtrl/*
* Common/Functions.*
* Common/colors.*
* Common/system/ShellImageList.*
* Common/SCGdiPlusBitmap.*
*/

/*
CListCtrl의 각 라인을 CSCListCtrlData라는 클래스의 인스턴스로 생성하여
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
m_list.set_color_theme(CSCListCtrl::color_theme_dark_gray);
m_list.set_progress_color(RGB(128, 255, 128));
m_list.set_headings(_T("파일,200;스냅샷,60;분류,60;평점,60;크기,200;메모,100;경로,300;날짜,100"));
m_list.set_header_height(24);

<멀티레벨 구현 관련 방향>
- 간혹 트리+리스트의 형태로 멀티레벨이 구현된 컨트롤도 있으나 이 컨트롤만으로 심플한 정도에서 멀티레벨을 구현하고자 함
  (확장, 축소 기능 지원안함)
- CSCListCtrlData에 level 멤버를 추가하여 해당 라인의 레벨을 표시한다.
  0레벨
	1레벨
		2레벨
			3레벨
- 각 레벨에 따라 체크박스도 같이 들여쓰기 할 지, 체크박스는 항상 맨 왼쪽에만 표시할지는 검토 필요.
- 상위 레벨과 하위 레벨은 ㄴ 과 같이 선으로 그려준다.
- DrawItem()에서 level에 따라 들여쓰기를 적용한다.

<수정될 내용>
- 어떤 컬럼이 이미지만 표시할 경우가 있으므로 column type을 추가하고 sort도 그에 맞게 처리되어야 한다.

- 특정 셀에는 특정 액션을 수행하는 컨트롤을 표시할 수 있도록 한다.
  예를 들어 파일 또는 폴더 선택 버튼을 표시하는데 항상 표시하는 것이 아니라
  해당 셀이 선택되면 컨트롤을 표시해준다. (button, combobox 등)
  이러한 컨트롤들은 하나만 동적으로 생성하고 선택 상태에 따라 show/hide 시킨다.
  m_list.set_control(int item, int subItem, ctrl_folder_browse);
  m_list.set_control(int item, int subItem, ctrl_file_browse);
*/
// CSCListCtrl

#define LCSB_CLIENTDATA 1
#define LCSB_NCOVERRIDE 2

static const UINT Message_CSCListCtrl = ::RegisterWindowMessage(_T("MessageString_CSCListCtrl"));

class CSCListCtrlMessage
{
public :
	CSCListCtrlMessage(CWnd* _this, int _message, CWnd* _pTarget = NULL, CString _param0 = _T(""), CString _param1 = _T(""), int _reserved = 0)
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
	int		message;
	CString param0 = _T("");
	CString param1 = _T("");
	int		reserved = 0;
};

class CVtFileInfo
{
public:
	CVtFileInfo()
	{
		memset(&data, 0, sizeof(data));
		is_remote = false;
	};

	CVtFileInfo(WIN32_FIND_DATA _data, bool _is_remote = false)
	{
		data = _data;
		is_remote = _is_remote;
	}

	CString get_file_time_str()
	{
		return ::get_file_time_str(data.ftLastWriteTime);
	}

	WIN32_FIND_DATA	data;
	//sort의 lamda등에서도 인덱스로 접근하도록 하기 위해 text[3];과 같이 배열로 선언했었으나
	//3개밖에 되지 않고 명확히 사용하고자 타입에 맞게 선언함
	//CString			path;
	//ULARGE_INTEGER	filesize;
	//FILETIME		filetime;
	bool			is_remote = false;
	//bool			is_folder = false;
	int				img_idx = -1;	//system image list 아이콘 index 캐시(-1 = 미계산). display_filelist 가 1회 계산 후 보관 → 폴더 캐시 재방문 시 SHGetFileInfo 디스크 접근 생략.
};


class CSCListCtrl : public CListCtrl
{
	DECLARE_DYNAMIC(CSCListCtrl)

public:
	CSCListCtrl();
	virtual ~CSCListCtrl();

	//virtual list 기능을 사용하기 위해서는 Owner Data, Owner Draw Fixed를 모두 true로 설정해야 한다.
	//이 옵션은 동적으로 변경 불가하고 WM_DRAWITEM 핸들러인 OnDrawItem()에서 그리게 된다.
	//단, virtual list에서 checkbox가 올바르게 표시되지 않는 문제가 있었으나 check = !check로 m_list_db에서 처리하고
	//OnDrawItem()에서 체크여부에 따라 직접 그리도록 수정하였다.
	//간혹 OnDrawItem()으로 직접 그리는 등의 세세한 기능들을 사용하지 않고 기본 CListCtrl과 같이 동작시키고 싶다면
	//Owner Data, Owner Draw Fixed를 모두 false로 설정하고 왠만한 method는 공통적으로 사용 가능하다.
	void			set_use_virtual_list(bool use_virtual = true) { m_use_virtual_list = use_virtual; }

	//대량 insert/set 사이 sync_scrollbar/Invalidate/InvalidateRect 호출 일괄 차단.
	//N row populate 시 매 호출마다 GetClientRect+GetColumnWidth*col+MoveWindow 가 N 회 누적 → 성능 저하.
	//사용 패턴: begin_bulk_insert() → loop { insert_item + set_text + set_text_color + set_text_style } → end_bulk_insert().
	//end 가 SetRedraw(TRUE) + sync_scrollbar() + Invalidate() 한 번씩 호출.
	void			begin_bulk_insert();
	void			end_bulk_insert();

	CSCColorTheme	m_theme = CSCColorTheme(this);

	void			modify_style();

	BOOL NCOverride;
	int Who;

	BOOL			m_bHorizLine = TRUE;
	void			HideScrollBars(int Type, int Which = SB_BOTH);

	enum CSCListCtrlMsgs
	{
		message_progress_pos = 0,

		//OnLvnGetdispinfo(), OnLvnItemchanged() 등에서 뭔가 세팅만 해주면
		//virtual list일때도 OnLvnItemChanged()에서 체크가 변경될때만 이벤트를 처리할 수 있을듯한데
		//아직은 그 해결책을 찾지 못했다.
		//우선 체크 변경 시 사용자 정의 메시지로 parent에 알린다.
		message_checked_item,

		message_drag_and_drop,
		message_path_changed,
		//20260708 by claude. 드라이브 볼륨 레이블 변경 통지 — param0=드라이브 root("C:\\"), param1=새 볼륨 레이블. watcher·일반 rename 통지로는
		//커버 안 되는 드라이브 전용 이벤트(파일/폴더 rename 은 LVN_ENDLABELEDIT 로 별도 처리). parent 가 받아 형제 컨트롤(트리 등) 드라이브 표시 동기화.
		message_drive_volume_changed,
		message_rename_duplicated,
		message_request_rename,
		message_request_new_folder,
		message_request_new_folder_index,
		message_get_remote_free_space,
		message_get_remote_total_space,
		message_list_processing,		//C:\Windows, C:\Windows\WinSxS 등과 같은 폴더는 그 갯수가 많으므로 parent에게 이를 알린다.
		//20260715 by claude. 리모트 드라이브 볼륨 레이블 변경 요청 — param0=드라이브 root("C:\\"), param1=새 볼륨 레이블, lParam=(bool*)결과.
		//로컬은 컨트롤이 직접 SetVolumeLabel 하지만 리모트는 소켓 명령이라 parent 가 수행해야 한다(rename 과 동일 구조).
		message_request_set_volume_label,
	};


//ShellList로 동작시키기
	bool		m_is_shell_listctrl = false;
	bool		m_is_local = true;
	bool		is_shell_listctrl() { return m_is_shell_listctrl; }
	bool		is_local() { return m_is_local; }
	//맨 처음 이 명령을 주면 모든 세팅과 동작이 ShellListCtrl로 동작된다.
	//세팅 이후 ShellList가 아닌 형태로 동작시키는 등은 허용하지 않는다.
	//is_local이 true이면 파일목록을 직접 얻어와서 표시하지만
	//false, 즉 remote일 경우는 파일목록을 받아서 표시해야 한다.
	void		set_as_shell_listctrl(CShellImageList* pShellImageList, bool is_local = true, CString default_path = _T("C:\\"));
	
	//list의 index를 주면 fullpath를 리턴한다. -1이면 현재 path를 리턴한다.
	//만약 선택된 항목이 폴더이고 해당 폴더까지의 경로를 원한다면 get_selected_path()를 사용해야 한다.
	CString		get_path(int index = -1);

	//현재 선택된 항목이 폴더이면 해당 경로까지의 fullpath를, 파일이라면 현재 리스트의 경로를 리턴한다.('\' 포함)
	//m_is_shell_listctrl로 동작할때만 동작한다.
	CString		get_selected_path();

	//해당 인덱스의 파일/폴더의 WIN32_FIND_DATA 값을 리턴한다.
	WIN32_FIND_DATA	get_win32_find_data(int index);
	//해당 인덱스의 파일/폴더의 WIN32_FIND_DATA 값을 갱신한다.
	void		set_win32_find_data(int index, WIN32_FIND_DATA data);

	//path를 받아 m_path에 저장하고 refresh_list()를 호출한다.
	//local일 경우는 경로만 주면 자동으로 폴더목록 표시
	//m_path를 변경한 후 set_path(m_path)와 같이 절대 호출하지 말것. set_path()내부에서 변경하려는 path와 m_path가 같으면 스킵되므로.
	void		set_path(CString path, bool refresh = true);

	//20260707 by claude. shell 리스트 뒤로가기 — 방문 폴더 히스토리(deque). 앞으로 이동 시 set_path 가 이전 폴더를 push,
	//go_back() 이 마지막 폴더로 이동(그때는 push 안 함). 마우스 back 버튼(XBUTTON1)·Backspace 가 자체적으로 호출한다.
	void		go_back();
	//20260709 by claude. 앞으로가기 — go_back 으로 물러난 폴더로 다시 진행. 마우스 forward 버튼(XBUTTON2)·Alt+Right 가 호출.
	void		go_forward();
	//remote일 경우는 파일목록을 받아서 표시한다.
	void		set_filelist(std::deque<CVtFileInfo>* pFolderList, std::deque<CVtFileInfo>* pFileList);

	//reload가 true라면 local의 파일목록을 다시 불러오지만 false라면 목록을 다시 리스트에 표시한다
	//(sort의 경우 폴더/파일 목록을 sort하여 보여줄 뿐 파일목록을 reload하진 않는다)
	//다른 코드에 의해 이미 m_cur_folders/m_cur_files 가 채워졌다면 reload를 false로 호출한다.
	//force=true 면 폴더 콘텐츠 캐시를 우회(해당 폴더 엔트리 무효화 후 디스크 재열거) — 명시적 새로고침/전송 완료 등
	//"반드시 최신"이 필요한 호출부에서 사용. 캐시는 local 폴더에만 적용되므로 remote/sort 경로에선 force 가 무해.
	void		refresh_list(bool reload = true, bool force = false);

	//m_cur_folders와 m_cur_files에 채워진 정보대로 리스트에 출력시킨다.
	void		display_filelist(CString cur_path);


	//리스트에 파일/폴더는 분리되어 표시/정렬되므로 나눠서 저장시켜야 한다.
	//추가/삭제시에도 이 목록에 추가/삭제시켜야 하는 불편함이 있다.
	std::deque<CVtFileInfo> m_cur_folders;
	std::deque<CVtFileInfo> m_cur_files;
	//remote일 경우는 fullpath로 해당 파일의 WIN32_FIND_DATA값을 얻어야 할 경우가 있다.
	void		get_remote_file_info(CString fullpath, WIN32_FIND_DATA* data);

	//폴더 콘텐츠 캐시 — local 폴더 재방문 시 디스크 재-enumerate(find_all_files) 를 건너뛴다. 무효화는 디렉토리
	//mtime(LastWriteTime — 항목 추가/삭제/이름변경 시 갱신) 기준. front = 최근 사용(LRU), 초과 시 back 제거.
	struct folder_cache_entry
	{
		CString					path;
		FILETIME				mtime = {};
		std::deque<CVtFileInfo>	folders;
		std::deque<CVtFileInfo>	files;
	};
	std::list<folder_cache_entry>	m_folder_cache;
	static const int				m_folder_cache_max = 32;
	//이 개수 미만 폴더는 캐시하지 않는다 — 작은 폴더는 재열거가 싸고, 파일 제자리 편집(내용 수정 → 크기/날짜 변경,
	//부모 dir mtime 은 불변)이 흔해 stale 위험이 크다. 반대로 거대 폴더(System32/WinSxS 등)는 열거가 비싸고 제자리
	//편집이 드물어(업데이트는 대개 추가/삭제/교체 → mtime 갱신) 캐시 이득은 크고 위험은 작다. 그래서 큰 폴더만 캐시한다.
	static const int				m_folder_cache_min_items = 1000;

	//폴더 콘텐츠 캐시 무효화 — dir mtime 은 파일 제자리 편집을 감지 못하므로, 앱은 명시적 새로고침 / 전송 완료(덮어쓰기) /
	//변경 감지 시 아래를 호출해 강제로 다시 읽게 해야 한다. refresh_list(reload, force=true) 도 내부적으로 이를 쓴다.
	void		clear_folder_cache() { m_folder_cache.clear(); }
	void		invalidate_folder_cache(CString path);

	//파일 또는 폴더를 해당하는 멤버 리스트에 추가한다.
	//local인 경우 크기와 날짜등이 비어있다면 자동 채워주고 remote라면 비어있으면 안된다.
	//void		add_file(CString path, uint64_t size, CString date = _T(""), bool is_remote = false, bool is_folder = false);
	//local이 아닌 remote의 경우 넘겨받은 구조체값으로 목록을 추가한다.
	void		add_file(WIN32_FIND_DATA* data, bool is_remote);

	//단일 파일을 리스트에 추가한다. 하나의 파일 전송이 끝나면 바로 리스트를 갱신하기 위해 사용.
	//list에만 추가할 뿐 m_cur_folders, m_cur_files에는 추가하지 않는다.
	//void		add_file(WIN32_FIND_DATA* data, bool is_remote);

	enum SHELL_LIST_COLUMN
	{
		col_filename = 0,
		col_filesize,
		col_filedate,
	};

	CString		m_path;
	//20260707 by claude. shell 리스트 뒤로가기 히스토리. set_path(앞으로 이동) 시 이전 m_path 를 push, go_back 시 pop.
	std::deque<CString>	m_folder_history;
	bool				m_navigating_back = false;	//go_back 실행 중이면 set_path 가 히스토리에 push 하지 않도록.
	//20260709 by claude. 앞으로가기 스택. go_back 시 떠난 폴더를 push, go_forward 시 pop. 새 폴더로 정상 이동하면 clear(분기 무효화).
	std::deque<CString>	m_folder_forward;
	bool				m_navigating_forward = false;	//go_forward 실행 중이면 set_path 가 히스토리 조작을 건너뛰도록.
	CShellImageList*	m_pShellImageList = NULL;
	CShellImageList*	get_shell_imagelist() { return m_pShellImageList; }
	void				set_shell_imagelist(CShellImageList* pShellImageList) { m_pShellImageList = pShellImageList; }

	void				move_parent_folder();

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

	//column = -1이면 전체 헤더의 텍스트를 '|'로 구분자로 하여 리턴한다.
	CString		get_header_text(int column = -1);
	void		set_header_text(int column, CString text);
	//default = HDF_LEFT
	void		set_header_text_align(int header, int format = HDF_LEFT);
	int			get_header_text_align(int header);

	//WM_SIZE시에 특정 컬럼은 그 너비를 가변(남는 폭을 흡수)으로 처리한다. min_width = 그 컬럼이 가질 수 있는 최소 폭(음수/과소 방지).
	//20260714 by claude. min_width 클램프 추가 — 다른 컬럼을 client 폭보다 넓게 끌면 채울 폭이 음수가 될 수 있고, 그 값이 native 특수값
	//(-1=LVSCW_AUTOSIZE, -2=LVSCW_AUTOSIZE_USEHEADER)과 겹치면 오토사이즈 이상동작을 한다. 못 채우면 가로 스크롤바가 자연히 뜬다.
	void		set_fixed_width_column(int column, int min_width = 60) { m_fixed_width_column = column; m_fixed_width_column_min = min_width; }

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
		column_data_type_combo,
	};
	int			get_column_data_type(int column);
	void		set_column_data_type(int column, int nType = column_data_type_text, bool invalidate = false);

//추가 관련
	//index 위치에 0번 컬럼이 text인 라인을 추가한다.(-1이면 맨 마지막에 추가)
	//실행 후 UI가 모두 표시되기 전에 insert_item() 또는 Invalidate(), EnsureVisible() 등이 호출되면
	//간혹 리스트의 top item이 반칸 정도 잘린 상태로 표시되는 경우가 있는데
	//set_line_height(), set_header_height() 등이 영향을 줬을 수 있다.
	//일단, UI가 화면에 표시되기 전까지는 Invalidate()이 되지 않도록 하니 이러한 현상은 사라짐.
	int			add_item(CString text = _T(""), int image_index = -1, bool ensureVisible = true, bool invalidate = true);
	int			add_item(std::deque<CString> dqText, int image_index = -1, bool ensureVisible = true, bool invalidate = true);
	int			insert_item(int index, CString text = _T(""), int img_idx = -1, bool ensureVisible = true, bool invalidate = true);
	int			insert_item(int index, std::deque<CString> dqText, int img_idx = -1, bool ensureVisible = true, bool invalidate = true);
	int			insert_item(int index, WIN32_FIND_DATA data, bool ensureVisible = true, bool invalidate = true);
	int			update_item(int index, WIN32_FIND_DATA data, bool ensureVisible = true, bool invalidate = true);
	//default separator = _T("|")
	int			insert_line(int index, CString line_string, CString separator = _T("|"), int img_idx = -1, bool ensureVisible = true, bool invalidate = true);

	//현재 폴더에 폴더를 추가한다.
	int			insert_folder(int index, CString new_folder_name, bool is_remote);

	//현재 폴더에 새 폴더를 생성하고 편집모드로 표시한다. 생성된 폴더명을 리턴한다.
	//"새 폴더" or "New Folder" 등 다국어까지 고려하여 타이틀을 받는다.
	CString		new_folder(CString &new_folder_title);

	//현재 폴더에서 "새 폴더" 생성 시 인덱스를 구한다. ex. "새 폴더 (2)"
	int			get_file_index(CString path, CString new_folder_title);

	//여러개의 인자를 args에 주니 모두 기록되지 않는 현상이 있다. 확인 필요!!
	//또한 실행 후 UI가 모두 표시되기 전에 insert_item() 또는 Invalidate(), EnsureVisible() 등이 호출되면
	//간혹 리스트의 top item이 반칸 정도 잘린 상태로 표시되는 경우가 있는데
	//set_line_height(), set_header_height() 등이 영향을 줬을 수 있다.
	//일단, UI가 화면에 표시되기 전까지는 Invalidate()이 되지 않도록 하니 이러한 현상은 사라짐.
	template <typename ... Types> int insert_item(int index, int img_idx, Types... args)
	{
		int n = sizeof...(args);
		CString arg[] = { args... };

		int subItem = 0;
		if (index < 0)
			index = size();

		for (auto item : arg)
		{
			if (subItem == 0)
				index = insert_item(index, item, img_idx, false, false);
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

	//shell_listctrl이라면 파일 또한 삭제할 지 파라미터를 줄 수 있고 실제 파일을 지운 후에 리스트에서도 삭제해야 한다.
	bool		delete_item(int index, bool delete_physical_file = false);
	bool		delete_item(CString label);

	//라인 전체가 공백이면 해당 라인을 삭제한다.
	void		delete_empty_lines();
	//sync=false 면 clear 후 sync_scrollbar 를 건너뛴다(폴더 전환처럼 곧바로 refill+최종 sync 가 뒤따르는 경로용 — clear 시점의
	//total=0 sync 가 세로/가로바를 잠깐 숨겼다 되살려 flicker 를 유발하므로). standalone clear(=refill 없음)는 기본 sync=true 로 둔다.
	void		delete_all_items(bool delete_file_list = true, bool sync = true);
	void		DeleteAllItems() { delete_all_items(); }

//텍스트 관련
	CString		get_text(int item, int subItem);
	void		set_text(int item, int subItem, CString text, bool invalidate = true);

	//한 라인의 각 컬럼값은 separator로 구분되어 있는 여러 라인 데이터로 index위치부터 리스트를 채운다.
	//reset = true이면 모든 데이터를 삭제한 후 새로 채운다. 이 때 index는 0으로 리셋된다.
	void		set_text(int index, CString text, CString separator = _T("|"), bool reset = true);

	//from ~ to 컬럼까지의 텍스트를 하나로 합쳐서 리턴해준다.
	CString		get_line_text(int index, int from = 0, int to = -1, CString sep = _T("|"));
	CString		get_line_text(int index, std::deque<int>* dqColumn = NULL, CString sep = _T("|"));
	std::deque<CString> get_line_text_list(int index, int from = 0, int to = -1);
	std::deque<CString> get_line_text_list(int index, std::deque<int>* dqColumn = NULL);
	//txt 출력 시 컬럼 세로 정렬을 맞게 출력하도록 공백을 보정하여 리턴
	//CString은 최대 64K까지만 담을 수 있으므로 vector를 이용함.
	void		get_line_text_list(std::vector<CString> *vt);

	//리스트에 표시할 항목이 없을 경우 표시할 텍스트 설정
	void		set_text_on_empty(CString text, int font_size = 9, Gdiplus::Color cr = Gdiplus::Color::DimGray);

	//shell_listctrl일 때 윈도우 탐색기에서 파일/폴더의 레이블을 변경하는 이벤트가 발생하면
	//main에서 이 함수를 호출하여 레이블을 변경한다.
	//shell_listctrl이 아니어도 사용 가능하나 레이블로 비교하므로 동일한 항목이 있을 경우는 정상 처리되지 않으므로
	//사용하지 말 것!
	void		rename(CString old_text, CString new_text);

//선택 관련
	//start부터 맨 처음 선택된 항목의 인덱스를 리턴한다.
	int			get_selected_index(int start = 0);
	//맨 끝에서부터 선택된 한 항목의 인덱스를 리턴한다.
	int			get_last_selected_item();
	int			get_selected_count() { return GetSelectedCount(); }
	//선택된 항목들을 dq에 담는다. dqSelected가 null이면 그냥 선택 갯수를 리턴받아 사용한다.
	int			get_selected_items(std::deque<int> *dq);
	//선택된 항목들의 목록을 dq에 담는다. shelllist일 경우 is_fullpath = true이면 각 항목의 전체경로를 담는다.
	int			get_selected_items(std::deque<CString>* dq, bool is_fullpath = false);
	//선택된 항목들의 목록을 dq에 담는다. shelllist에서만 사용되며 cFileName은 이미 전체경로를 가지고 있다.
	int			get_selected_items(std::deque<WIN32_FIND_DATA>* dq);

	//index = -1 : 전체선택
	void		select_item(int index, bool select = true, bool after_unselect = false, bool insure_visible = true);
	void		unselect_selected_item();
	//names(각 항목의 leaf 파일/폴더명)에 매칭되는 항목만 선택하고, 마지막 매칭 항목으로 스크롤한다(기존 선택은 모두 해제).
	//리로드(refresh_list) 후 선택/스크롤 복원, 전송된 항목 재선택 등에 사용. 반환: 마지막으로 선택된 인덱스(없으면 -1).
	int			select_items_by_names(const std::deque<CString>& names, int visible_mode = visible_last);

	int			get_check(int index);
	//CListCtrl::GetCheck() override.
	int			GetCheck(int index) { return get_check(index); }

	//index = -1 : 전체선택
	void		set_check(int index, bool check = true);
	void		set_check(std::deque<int> indices, bool check);
	//CListCtrl::SetCheck() override.
	void		SetCheck(int index, BOOL bCheck) { set_check(index, bCheck); }


	//check된 항목 리스트를 dq에 담고 체크된 항목의 개수를 리턴한다.
	int			get_checked_items(std::deque<int>* dq = NULL);

	//아이템의 상태값이 특정 상태값이 항목 또는 그 개수 구하기
	//LVIS_DROPHILITED or LVIS_SELECTED 항목을 구할 수 있다.
	//drag 도중에 마우스가 다른 앱 영역으로 나가서 WM_LBUTTONUP 될 경우 drophilited 상태로 아이템이 남는 문제를 제거하기 위해.
	int			get_items_by_state(UINT state, std::deque<int>* dq = NULL);
	//dq 목록의 아이템들의 state 세팅. dq가 null이면 모든 항목에 대해 실행
	//선택 : set_items_with_state(LVIS_SELECTED|LVIS_FOCUSED, LVIS_SELECTED|LVIS_FOCUSED, dq);
	//해제 : set_items_with_state(0, LVIS_SELECTED|LVIS_FOCUSED, dq);
	int			set_items_with_state(UINT state, UINT mask, std::deque<int>* dq = NULL);

//검색 관련
	//기본 검색함수인 FindItem()을 이용해서 0번 컬럼에서만 데이터를 찾는다. virtual list이므로 OnLvnOdfinditem() 함수 수정 필수.
	int			find(CString str, int start = -1, bool whole_word_olny = false, bool case_sensitive = false);

	//separator가 ""이 아닌 공백 또는 다른 문자열일 경우는
	//str을 분리해서 모두 찾는다.
	int			find(CString find_target, std::deque<int>* result,
					 int start_idx = 0, int end_idx = -1,
					 std::deque<int>* dqColumn = NULL, bool stop_first_found = false, bool whole_word_olny = false, bool case_sensitive = false);

	//target_columns에 검색 대상 컬럼들을 나열해준다.
	//위의 find()함수와 std::deque<int>* dqColumn 유무와 끝의 T... target_columns 파라미터 때문에
	//int arg[] = { target_columns... }; 문장에서 컴파일 타임에 에러가 표시되기도 한다.
	//이는 이 템플릿 함수를 호출하며 파라미터가 일부 생략됐을 때 발생하므로 이럴 경우는 파라미터를 정확히 명시해야 한다.
	template <typename ... T> int find(CString find_target, std::deque<int>* result, int start_idx = 0, int end_idx = -1,
					bool stop_first_found = false, bool whole_word_olny = false, bool case_sensitive = false,
					T... target_columns)
	{
		int n = sizeof...(target_columns);
		int arg[] = { target_columns... };

		std::deque<int> dqColumn;
		for (auto column : arg)
			dqColumn.push_back(column);

		return find(find_target, result, start_idx, end_idx, &dqColumn, stop_first_found, whole_word_olny, case_sensitive);
	}

//정렬 관련
	enum listctrlex_sort_type
	{
		sort_none = -1,
		sort_descending,
		sort_ascending,
	};
	void			allow_sort(bool allow);
	void			sort(int column, int ascending);
	void			sort_by_text_color(int column, int ascending, bool text_sort_on_same_color = true);
	int				cur_sorted_column_index() { return m_cur_sort_column; }
	//20260715 by claude. 현재 정렬 기준(컬럼·오름/내림)을 그대로 다시 적용 — 항목 텍스트가 바뀌어(이름변경) 순서가 어긋났을 때 쓴다.
	//sort() 는 목록을 reload 하지 않고 정렬만 다시 하므로 refresh_list 보다 싸고, 선택 인덱스는 바뀌므로 호출측이 이름으로 재선택해야 한다.
	void			resort();

//컬러 관련
	void			set_color_theme(int theme, bool invalidate = true);
	void			set_color_theme(const CSCColorTheme& theme, bool invalidate = true);
	Gdiplus::Color	get_text_color(int item, int subItem);
	Gdiplus::Color	get_back_color(int item, int subItem);
	//특정 셀이 아닌 기본 배경색을 리턴.
	Gdiplus::Color	get_back_color() { return m_theme.cr_back; }
	Gdiplus::Color	get_back_alternate_color(int item, int subItem) { return m_theme.cr_back_alternate; }
	void			set_back_alternate_color(bool use = true, Gdiplus::Color cr = Gdiplus::Color::Transparent);

	//특정 항목의 글자색 설정. erase가 true이면 crText 인자를 무시하고 기본 글자색으로 되돌린다.
	//item이 -1이면 모든 라인에, subItem이 -1이면 모든 컬럼에 적용.
	//set_default_text_color(), set_default_back_color()를 통해 기본 글자색과 배경색을 설정할 수 있다.
	void		set_text_color(int item, int subItem, Gdiplus::Color crText, bool erase = false, bool invalidate = true);
	//default text color로 되돌린다.
	void		reset_text_color(int item, int subItem) { set_text_color(item, subItem, 0, true); }
	//특정 항목의 배경색 설정. erase가 true이면 crText 인자를 무시하고 기본 글자색으로 되돌린다.
	void		set_back_color(int item, int subItem, Gdiplus::Color crBack, bool erase = false, bool invalidate = true);
	//default back color로 되돌린다.
	void		reset_back_color(int item, int subItem) { set_back_color(item, subItem, 0, true); }
	//글자색과 배경색 동시 변경
	void		set_item_color(int item, int subItem, Gdiplus::Color crText, Gdiplus::Color crBack);

	//셀별 font style. item/subItem 이 -1 이면 모든 행/모든 컬럼에 일괄 적용.
	//weight: LOGFONT.lfWeight (FW_DONTCARE=0/THIN=100/.../NORMAL=400/SEMIBOLD=600/BOLD=700/HEAVY=900). 0 = base font 그대로.
	//italic / underline / strikeout: 0=off, 1=on.
	void		set_text_weight   (int item, int subItem, int  weight,    bool invalidate = true);
	void		set_text_italic   (int item, int subItem, bool italic,    bool invalidate = true);
	void		set_text_underline(int item, int subItem, bool underline, bool invalidate = true);
	void		set_text_strikeout(int item, int subItem, bool strikeout, bool invalidate = true);
	//4 속성 한 번에 — invalidate 한 번만.
	void		set_text_style    (int item, int subItem, int  weight, bool italic, bool underline = false, bool strikeout = false, bool invalidate = true);

	int			get_text_weight   (int item, int subItem) const;
	bool		get_text_italic   (int item, int subItem) const;
	bool		get_text_underline(int item, int subItem) const;
	bool		get_text_strikeout(int item, int subItem) const;

	//기본 글자색을 설정한다.
	void		set_default_text_color(Gdiplus::Color cr_text) { m_theme.cr_text = cr_text; Invalidate(); }
	//기본 배경색을 설정한다.
	void		set_default_back_color(Gdiplus::Color cr_back) { m_theme.cr_back = cr_back; Invalidate(); }

	//컬럼이 percentage를 표시하는 컬럼일 경우 그 표시 색상을 설정한다.
	void		set_progress_color(Gdiplus::Color crProgress);
	template <typename ... Types> void set_percentage_gradient(Types... args)
	{
		int n = sizeof...(args);
		Gdiplus::Color arg[] = { args... };

		m_cr_percentage_bar.clear();

		for (auto item : arg)
		{
			m_cr_percentage_bar.push_back(item);
		}
	}


//편집 관련
	//리소스의 ListCtrl 속성에서 "레이블 편집" 속성은 default값인 false로 하고 이 함수로 편집 가능 여부를 설정해야 한다.
	void		allow_edit(bool allow_edit = true, bool one_click_edit = true);
	void		allow_one_click_edit(bool allow_one_click_edit) { m_allow_one_click_edit = allow_one_click_edit; }
	void		allow_edit_column(int column, bool allow_edit = true);
	void		set_edit_readonly(bool readonly = true) { m_edit_readonly = readonly; }
	bool		is_in_editing()	{ return m_in_editing; }	//편집중인지
	void		set_flag_in_editing(bool in_editing) { edit_end(); m_in_editing = in_editing; }
	int			get_recent_edit_item() { return m_edit_item; }
	int			get_recent_edit_subitem() { return m_edit_subItem; }
	void		set_recent_edit_subitem(int subItem) { m_edit_subItem = subItem; }
	std::deque<int>* get_selected_list_for_edit() { return &m_dqSelected_list_for_edit; }
	std::deque<int> m_dqSelected_list_for_edit;
	CString		get_edit_old_text() { return m_edit_old_text; }
	CString		get_edit_new_text() { return m_edit_new_text; }
	//item = -1이면 선택된 항목을, subItem = -1이면 0번 컬럼을 편집 시작한다.
	//return되는 편집 컨트롤 포인터를 이용할 경우도 필요하여 리턴함.
	CSCStaticEdit*	edit_item(int item = -1, int subItem = -1);
	void		edit_end(bool valid = true);
	void		undo_edit_label();		//편집 전의 텍스트로 되돌린다.(예를 들어 편집 레이블이 파일명이고 파일명 변경이 실패한 경우 쓸 수 있다.)
	bool		is_modified() { return m_modified; }
	void		reset_modified_flag() { m_modified = false; }
	LRESULT		on_message_CSCStaticEdit(WPARAM wParam, LPARAM lParam);

	//편집 모드 중 *다른 컨트롤·다른 앱·다이얼로그 빈 영역* 어디를 클릭해도 편집을 정상 종료(commit) 시키기 위한 훅.
	//CSCListBox / CSCTreeCtrl / CPathCtrl 과 동일 패턴 — WH_MOUSE 로 같은 스레드 외부 클릭 감지 → 메시지 post → on_end_edit_posted → edit_end(true).
	//다른 앱으로의 포커스 이동은 CSCStaticEdit 의 killfocus 가 on_message_CSCStaticEdit 로 전달돼 처리 (기존 경로 유지).
	void			install_edit_mouse_hook();
	void			remove_edit_mouse_hook();
	static LRESULT CALLBACK	edit_mouse_hook_proc(int code, WPARAM wParam, LPARAM lParam);
	LRESULT			on_end_edit_posted(WPARAM wParam, LPARAM lParam);
	static CSCListCtrl*	s_editing_list;
	HHOOK			m_mouse_hook = NULL;

//폰트 관련. 반드시 set_headings() 후에 호출할것.
	void		set_font_name(LPCTSTR sFontname, BYTE byCharSet = DEFAULT_CHARSET, bool invalidate = false);
	//-1 : reduce, +1 : enlarge
	int			get_font_size();
	//반드시 set_headings() 후에 호출할것.
	void		set_font_size(int font_size, bool invalidate = false);
	void		enlarge_font_size(bool enlarge, bool invalidate = false);
	void		set_font_bold(int weight = FW_BOLD, bool invalidate = false);
	void		set_font_italic(bool italic = true, bool invalidate = false);
	LOGFONT		get_log_font() { return m_lf; }
	void		set_log_font(LOGFONT lf, bool invalidate = false);

//파일, 클립보드 관련
	//Ctrl+C키로 선택된 항목을 클립보드로 복사할 수 있다. shift를 조합하면 헤더까지 포함된다.
	bool		list_copy_to_clipboard(bool selected_only = true, TCHAR separator = '|', bool bHead = false);
	//void	paste_from_clipboard();
	bool		save(CString file, TCHAR separator = '|', bool selected_only = false);
	bool		load(CString sfile, TCHAR separator = '|', bool add_index = false, bool match_column_count = true, bool reset_before_load = true);

//기타 레이아웃 관련
	int			get_header_height();
	//header_height는 헤더클래스에서 HDM_LAYOUT 메시지가 발생해야 적용되는데
	//그렇게 되기 위해서는 SetFont / MoveWindow / SetWindowPos 등이 필요하므로
	//CSCListCtrl에게 set_font_name() 호출시에 헤더도 SetFont를 적용하여 해결함.
	//=> SetFont()를 통해서 header의 height를 변경한다.
	void		set_header_height(int height, bool invalidate = false);

	LRESULT		on_message_CSCHeaderCtrl(WPARAM wParam, LPARAM lParam);

	//line height를 변경하는 방법은 가상의 이미지리스트를 이용하는 방법과
	//(실제 사용할 이미지리스트가 있는 경우는 위 방법을 사용할 수 없다)
	//MeasureItem을 이용하는 방법(OwnerDrawFixed only)이 있다.
	//(강제 갱신 방법 아직 미해결이며 resize를 하면 자동 적용되므로
	//우선은 mainDlg에서 이 함수 호출후에 RestoreWindowPosition()등과 같이
	//CSCListCtrl에 WM_SIZE가 발생하도록 하는 함수를 사용하면 된다)
	//CSCListCtrl가 아닌 그냥 mainDlg만 resize한다고 해서 되지 않는다.
	//자체 imagelist를 사용하지 않는 컨트롤이라면 쉽게 lineheight를 조절할 수 있다.
	bool		m_use_own_imagelist = true;
	void		set_use_own_imagelist(bool use) { m_use_own_imagelist = use; }

	int			get_line_height() { return m_line_height; }
	void		set_line_height(int height, bool invalidate = false);

	//20260706 by claude. 픽셀 페인트(부분행/여백0) 모드 on/off. off(기본) 면 기존 native 위임 동작 그대로.
	void		set_smooth_scroll(bool smooth = true) { m_smooth_scroll = smooth; if (::IsWindow(m_hWnd)) { sync_scrollbar(); Invalidate(); } }
	bool		is_smooth_scroll() { return m_smooth_scroll; }

	void		set_column_width(int nCol, int cx, bool invalidate = false);
	//레지스트리에 저장된 각 컬럼너비를 복원한다.
	void		restore_column_width(CWinApp* pApp, CString sSection, bool invalidate = false);
	//각 컬럼너비를 레지스트리에 저장한다. 프로그램이 시작될 때 restore_column_width()를 통해 그 너비대로 복원할 수 있다.
	void		save_column_width(CWinApp* pApp, CString sSection);
	CRect		get_item_rect(int item, int subItem);
	//클릭위치에 따라 item은 올바르게 판별되나 subItem은 그렇지 않아서(마우스 이벤트 핸들러 함수에서) 새로 추가함.
	//return value : checkbox 클릭 시 LVHT_ONITEMSTATEICON, 이미지 클릭 시 LVHT_ONITEMICON, 그 외에는 LVHT_ONITEMLABEL을 리턴한다.
	int			hit_test(CPoint pt, int& item, int& subItem, bool include_icon);

	//20260714 by claude. [smooth API 가드] 이 컨트롤은 사실상 smooth 스크롤 고정이라, native CListCtrl::HitTest/SubItemHitTest 는
	//세로 스크롤(m_scroll_y)을 무시해 화면 위치와 다른 엉뚱한 항목을 돌려준다. smooth 를 모르고 native API 를 그냥 호출해도
	//올바른 항목이 나오도록 base 를 가려(shadow) smooth-aware hit_test 로 위임한다. (base 시그니처가 const 라 맞추되, hit_test 는
	//순수 조회지만 비-const 로 선언돼 있어 const_cast 가 필요하다 — 상태 변경 없음.)
	int HitTest(CPoint pt, UINT* pFlags = NULL) const
	{
		int item = -1, sub_item = -1;
		UINT flags = (UINT)const_cast<CSCListCtrl*>(this)->hit_test(pt, item, sub_item, true);
		if (pFlags)
			*pFlags = flags;
		return item;
	}
	int HitTest(LVHITTESTINFO* pInfo) const
	{
		if (pInfo == NULL)
			return -1;
		int item = -1, sub_item = -1;
		pInfo->flags = (UINT)const_cast<CSCListCtrl*>(this)->hit_test(pInfo->pt, item, sub_item, true);
		pInfo->iItem = item;
		pInfo->iSubItem = sub_item;
		return item;
	}
	int SubItemHitTest(LVHITTESTINFO* pInfo)
	{
		if (pInfo == NULL)
			return -1;
		int item = -1, sub_item = -1;
		pInfo->flags = (UINT)hit_test(pInfo->pt, item, sub_item, true);
		pInfo->iItem = item;
		pInfo->iSubItem = sub_item;
		return item;
	}

	void		show_progress_text(bool show = true) { m_show_progress_text = show; Invalidate(); }
	//void		set_progress_text_color(Gdiplus::Color cr) { m_theme.cr_progress_text = cr; }

	//cr_border가 Transparent라면 cr_border의 값을 적용하지 않는다는 의미다. 기본 m_theme.cr_selected_border 색상을 사용한다.
	//단, set_use_distinct_border_color(true);로 설정되면 지정된 cr_border를 무시하고 해당 아이템의 배경색과 구분되는 색으로 그린다.(black or white)
	//selected_border_width <= 0이면 m_selected_border_width를 변경하지 않는다는 의미다. 기본 m_selected_border_width 값이 사용된다.
	void		set_draw_selected_border(bool draw, Gdiplus::Color cr_border = Gdiplus::Color::Transparent, int selected_border_width = 0, int pen_style = -1);
	//border color를 특정색이 아닌 배경색과 구분되는 색으로 그린다.
	void		set_use_distinct_border_color(bool use_distinct = true) { m_use_distinct_border_color = use_distinct; }

	//한 라인에 대한 상단 라인 표시 여부 설정. m_draw_top_line
	void		draw_top_line(bool draw, Gdiplus::Color cr = Gdiplus::Color::LightGray) { m_draw_top_line = draw; m_cr_top_line = cr; }
	//한 라인에 대한 하단 라인 표시 여부 설정. m_draw_top_line
	void		draw_bottom_line(bool draw, Gdiplus::Color cr = Gdiplus::Color::LightGray) { m_draw_bottom_line = draw; m_cr_bottom_line = cr; }

	//선택항목등의 border가 아닌 ctrl 자체의 cr_border. OnNcPaint()에서 border 속성유무를 판단하여 테두리를 그린다.
	void		set_border_color(Gdiplus::Color cr_border) { m_theme.cr_border_inactive = cr_border; Invalidate(); }

//scroll
	enum CLISTCTRLEX_ENSURE_VISIBLE_MODE
	{
		visible_first = 0,
		visible_center,
		visible_last,
	};
	//어떤 항목이 특정 위치에 표시되도록 스크롤시킨다.
	//mode가 visible_first(0)이고 offset이 3이면 위에서 3+1인 위치에 해당 아이템이 표시되도록 스크롤시킨다.
	//mode가 visible_center(1)이고 offset이 0이면 중앙 위치에 해당 아이템이 표시되도록 스크롤시킨다.
	//mode가 visible_last(2)이고 offset이 3이면 아래에서 -3-1인 위치에 해당 아이템이 표시되도록 스크롤시킨다.
	//mode default = visible_center
	void			ensure_visible(int index, int mode = visible_center, int offset = 0);
	//original IsItemVisible method does not check partial. only check vertical axis.
	bool			is_item_visible(int index, bool bPartial = false);

	//20260710 by claude. 표준 CListCtrl geometry/scroll API 를 '같은 이름'으로 smooth-aware shadow(override).
	//소비자가 표준 이름만 써도 smooth(픽셀 뷰포트 m_scroll_y)/native 무관 정상 동작한다. 예전엔 커스텀 이름(is_item_visible/ensure_visible)만 있어
	//표준 API 호출이 native=smooth에서 깨진 값을 받았고(자막 선택 미추적·EnsureVisible 루프 freeze 원인), 소비자가 어느 이름을 써야 하는지도 혼란스러웠다.
	//CListCtrl 은 non-virtual 이라 CSCListCtrl* 로 호출할 때만 이 shadow 가 적용된다(기반 포인터론 native — 필요시 CListCtrl::… 명시).
	BOOL			EnsureVisible(int nItem, BOOL bPartialOK);
	int				GetTopIndex();
	int				GetCountPerPage();
	BOOL			IsItemVisible(int nItem, BOOL bPartial = FALSE);

	//항목이 추가되면 auto scroll되지만 특정 항목을 선택하면 false로 된다.
	//scroll_end_button은 m_auto_scroll = false 일때만 화면에 표시된다.
	bool			m_auto_scroll = true;
	bool			m_show_auto_scroll_button = false;
	void			show_auto_scroll_button(bool show = true);
	bool			get_auto_scroll() { return m_auto_scroll; }
	//0:auto scroll off, 1:auto scroll on, -1:toggle auto scroll
	void			set_auto_scroll(int auto_scroll);
	LRESULT			on_message_CGdiButton(WPARAM wParam, LPARAM lParam);

	//리스트 항목들을 무작위로 섞는다.
	void			shuffle();

	//x, y위치의 item, sub_item 인덱스를 구할 수 있다. item의 인덱스만을 필요로 할 경우는 리턴값만 이용하면 된다.
	int				index_from_point(int x, int y, int* item = NULL, int* sub_item = NULL);

	//Drag&Drop 드래깅 관련.
	template <typename ... Types> void add_drag_images(Types... args) //(단일파일용 이미지, 싱글파일용 이미지를 차례대로 넣고 drag되는 개수에 따라 맞는 이미지를 사용한다)
	{
		int n = sizeof...(args);
		int arg[] = { args... };

		for (auto id : arg)
			m_drag_images_id.push_back(id);
	}

	//20260705 by claude. 드래그 중 드래그 이미지 하단 문구(예: "+ 대상폴더(으)로 복사")를 app 이 계산해 돌려주는 provider(§2.3:
	//메커니즘은 컨트롤, 정책은 app). 빈 문자열이면 밴드 없이 원본만. 미설정이면 문구 없음(하위호환). pt 는 screen 좌표.
	void			set_drag_hint_provider(std::function<CString(CWnd* pDropWnd, CPoint pt_screen)> fn) { m_fn_drag_hint = fn; }
	//20260705 by claude. 드래그 이미지 문구 갱신. hint 가 비면 원본만, 아니면 원본 하단에 밴드+문구 합성. 값이 이전과 같으면 재합성 안 함(캐시).
	void			update_drag_hint(const CString& hint_text);
	//20260705 by claude. 현재 커서 위치 기준으로 provider 를 재호출해 문구 갱신(드래그 중 Ctrl/Shift 토글처럼 마우스가 안 움직여도).
	void			refresh_drag_hint();

	bool			get_use_drag_and_drop() { return m_use_drag_and_drop; }
	void			set_use_drag_and_drop(bool use_drag = true) { m_use_drag_and_drop = use_drag; }
	int				get_drop_index() { return m_nDropIndex; }

	//드래그 자동 스크롤 등 '외부(드래그 소스 컨트롤)'가 이 리스트를 스크롤할 때 쓰는 정상 경로 진입점.
	//가로는 shift+휠/가로휠과 동일하게 m_h_scroll_pos 갱신+sync_scrollbar 로 가로바·헤더까지 동기화한다.
	//(raw Scroll() 만 호출하면 LVN_ENDSCROLL→sync_scrollbar 가 stale m_h_scroll_pos 로 thumb 을 리셋해 가로바가 안 따라옴.)
	//dx_px: +오른쪽/-왼쪽(px). dy_lines: +아래/-위(라인 수).
	void			drag_scroll_by(int dx_px, int dy_lines);
	//20260707 by claude. SCTreeCtrl 등에서 드래그 자동스크롤이 이 리스트로 위임될 때 받는 메시지 핸들러. drag_scroll_by 후 1 반환.
	afx_msg LRESULT	on_message_DragScrollBy(WPARAM wParam, LPARAM lParam);

	//text = "     some text"일 경우 앞의 공백을 들여쓰기 용도로 사용한다.
	//"     " 만큼 들여써서 "some text"를 출력한다. 아이콘도 함께 적용된다.
	void			use_indent_from_prefix_space(bool use_indent) { m_use_indent_from_prefix_space = use_indent; }

	//HAS_STRING, OWNER_DRAW_FIXED 속성을 가지면 Get/SetItemData() 함수를 사용할 수 없다.
	//이 두 함수를 사용할 수 있도록 CSCListCtrlData에 data 멤버를 추가하고 다음 함수들을 override하여 선언함.
	DWORD_PTR		GetItemData(int index);
	BOOL			SetItemData(int index, DWORD_PTR dwData);

	CSCHeaderCtrl*	get_header_ctrl() { return &m_HeaderCtrlEx; }

//cell action
	enum CELL_ACTION
	{
		cell_action_browse_folder,
		cell_action_browse_file,
	};
	void			set_action(int item, int subItem, int action);

protected:
	bool			m_use_virtual_list = true;

	//begin_bulk_insert/end_bulk_insert 사이는 true — insert_item / set_text* 의 sync_scrollbar/Invalidate 차단.
	bool			m_in_bulk_insert = false;

//메인 데이터
	std::deque<CSCListCtrlData> m_list_db;
	
	CString			m_text_on_empty;
	int				m_text_on_empty_size = 9;
	Gdiplus::Color	m_text_on_empty_color = Gdiplus::Color::DimGray;

//컬럼 관련
	CSCHeaderCtrl	m_HeaderCtrlEx;
	std::deque<int> m_column_data_type;
	std::deque<int> m_column_text_align;
	int				m_fixed_width_column = -1;
	int				m_fixed_width_column_min = 60;	//20260714 by claude. 가변 컬럼 최소 폭(음수/과소 방지 클램프). set_fixed_width_column 에서 설정.
	int				m_line_height = 16;

//정렬 관련
	bool			m_allow_sort = true;
	std::deque<int> m_column_sort_type;				//asceding or descending
	int				m_cur_sort_column = 0;			//정렬된 컬럼 인덱스(색상 정렬은 제외)

//컬러 관련
	bool			m_use_alternate_back_color = false;

//편집기능 관련
	bool			m_allow_edit = false;			//항목 편집이 가능한지...
	bool			m_allow_one_click_edit = false;	//항목을 클릭한 후 정해진 시간내에 다시 클릭될 때 편집모드로 할지
	std::deque<bool> m_allow_edit_column;			//각 컬럼마다 편집가능 여부를 설정할 수 있다. 단, m_allow_edit이 변경되면 모두 그 값으로 변경됨에 주의.
	bool			m_modified = false;				//항목의 차례 또는 내용이 수정된 경우 true
	bool			m_in_editing = false;			//편집중인지
	int				m_edit_item = -1;				//편집중인 아이템 인덱스
	int				m_edit_subItem = -1;			//편집중인 아이템 서브인덱스
	bool			m_edit_readonly = false;		//편집모드로는 들어가지만 편집해서는 안되는 데이터인 경우 set_edit_readonly(true);를 호출한다.
	CString			m_edit_old_text;				//편집 전 텍스트
	CString			m_edit_new_text;				//편집 후 텍스트
	//20260708 by claude. 드라이브 루트 편집 상태 — 편집 박스엔 볼륨명만 보이고(드라이브 문자 " (X:)" 제거), 커밋 때 SetVolumeLabel + 표시에 재부착.
	bool			m_edit_is_drive = false;		//현재 편집 대상이 드라이브 루트인가.
	CString			m_edit_drive_root;				//그 드라이브 root("C:\\") — edit_end 가 m_edit_old_text(접미 제거본)에 의존하지 않게 저장.
	CSCStaticEdit*	m_pEdit = NULL;
	long			m_last_clicked_time = 0;		//one_click으로 편집모드 진입 시 마지막 클릭 시각
	int				m_last_clicked_index = -1;		//one_click으로 편집모드 진입 시 마지막 클릭 인덱스

//폰트 관련
	LOGFONT			m_lf;
	CFont			m_font;
	int				m_font_size;
	void			reconstruct_font(bool invalidate = false);

	//셀별 font style 캐시 — (weight, italic, underline, strikeout) 조합당 1 CFont. lazy 생성.
	//draw 단계의 셀 당 비용 = std::map::find + SelectObject. weight 0 = base font (캐시 우회).
	//base font (m_font) 가 바뀌면 (reconstruct_font) 캐시 무효화.
	std::map<DWORD, CFont*>	m_font_style_cache;
	void			clear_font_style_cache();
	CFont*			get_styled_font(int weight, bool italic, bool underline, bool strikeout);

	bool			m_show_progress_text = true;	//default = true

	//마우스가 컨트롤 안에 들어온 경우 true
	bool			m_is_hovering = false;

//Drag&Drop 드래깅 관련
	bool			m_use_drag_and_drop = false;//default = false
	CWnd*			m_pDragWnd = NULL;			//Which ListCtrl we are dragging FROM
	CWnd*			m_pDropWnd = NULL;			//Which ListCtrl we are dropping ON
	CImageList*		m_pDragImage = NULL;		//For creating and managing the drag-image
	//20260704 by claude. 드래그 이미지를 CImageList(화면 lock→스크롤 시 깜빡임) 대신 레이어드 팝업으로 표시. set_image 가 창을 지연 생성.
	CSCShapeDlg		m_drag_shape;
	CPoint			m_drag_shape_offset = CPoint(-10, -14);		//커서 → 이미지 좌상단 오프셋
	CSCGdiplusBitmap m_drag_base_img;			//20260705 by claude. 밴드(문구) 없는 원본 드래그 이미지 — 문구 갱신 때 base+band 재합성용
	CString			m_drag_hint_text;			//20260705 by claude. 현재 표시 중인 문구(캐시) — 값이 바뀔 때만 재합성
	DWORD			m_last_drag_hint_tick = 0;	//20260713 by claude. 마지막 재합성 시각(GetTickCount) — 재합성 스로틀용.
	std::function<CString(CWnd* pDropWnd, CPoint pt_screen)> m_fn_drag_hint;		//드래그 문구 provider(app 설정)
	//20260715 by claude. provider 를 마지막으로 부른 '드롭 대상'(창 + 커서 밑 항목). 문구는 대상이 바뀔 때만 달라지므로
	//같은 항목 위에서 움직이는 동안은 provider 호출 자체를 건너뛴다(대다수 이동이 여기 해당).
	HWND			m_drag_hint_wnd = NULL;
	int				m_drag_hint_item = -2;		//-1 = 빈 공간, -2 = 아직 계산 안 함(드래그 시작 시 리셋)
	//20260705 by claude. base 이미지 하단에 문구 밴드를 붙인 새 비트맵을 out 에 합성. XP 호환 위해 리스트 자체 폰트(m_lf) 사용.
	void			compose_drag_image_with_hint(CSCGdiplusBitmap& base, const CString& text, CSCGdiplusBitmap& out);
	bool			m_bDragging = false;		//T during a drag operation
	//20260714 by claude. [탐색기 방식 재클릭 편집] 선택된 항목을 다시 '눌렀다 뗐을 때만'(드래그 없이) 편집 진입하도록,
	//OnNMClick(버튼 DOWN)에서 조건 만족 시 예약만 하고 OnLButtonUp(뗄 때)에서 실제 진입. 드래그로 이어지면 OnMouseMove 가 취소.
	bool			m_pending_reclick_edit = false;
	//20260713 by claude. 소수 level 속도 + 누적기. tick 당 정수 라인만 스크롤 가능하므로 소수 속도를 누적해 1.0 을 넘을 때만
	//그 정수만큼 스크롤 → level<1(가장자리 존 바깥쪽)이면 여러 tick 에 한 번씩 스크롤되어 '시작이 천천히'가 표현된다.
	float			m_drag_scroll_fx = 0.f;		//드래그 자동 스크롤 목표속도(가로 level, 부호=방향). 0=안 함.
	float			m_drag_scroll_fy = 0.f;		//세로 목표속도.
	//20260715 by claude. 가로 누적기의 단위는 level 이 아니라 px — level 을 정수 절삭 후 *30 하면 30px 단위로만 튀어(툭툭툭), *30 을 먼저 곱해 px 로 누적한다.
	float			m_drag_scroll_ax = 0.f;		//가로 누적기(px).
	float			m_drag_scroll_ay = 0.f;		//세로 누적기(라인).
	//20260713 by claude. 시간 램프 — 스크롤 에피소드 시작 시각. 시작 직후 아주 느리게 → 유지하면 최대속도로 가속(탐색기식). 존 벗어나면 0.
	DWORD			m_drag_scroll_start_tick = 0;
	CWnd*			m_drag_scroll_target = NULL;	//자동 스크롤을 실제로 보낼 리스트/트리(오버레이 스크롤바 위여도 이 컨트롤로 전송).
	void			update_drag_auto_scroll(CPoint screen_pt);	//드래그 중 대상 가장자리 거리로 속도 산출 + 타이머 관리.
	//20260709 by claude. 자동 스크롤 타이머는 zone 진입 시 1회만 SetTimer 해야 한다. 매 mousemove 마다 SetTimer 하면 같은 ID 라 카운트다운이
	//리셋돼(마우스가 계속 움직이면) 70ms 타이머가 영영 발화하지 못해 '이동 중엔 스크롤 안 되고 멈춰야만 스크롤' 되던 버그. 플래그로 중복 SetTimer 차단.
	bool			m_auto_scroll_timer_on = false;
	void			start_auto_scroll_timer();
	void			stop_auto_scroll_timer();
	//20260709 by claude. 자동스크롤 속도 레벨 — 가장자리 거리(안쪽 양수/넘으면 음수)에 비례해 1..MAX_LEVEL 가속(멀면 0). 드래그·마퀴 공통.
	float			auto_scroll_level(int dist, bool cap_overshoot = true);	//20260714 by claude. cap_overshoot=false 면 오버슛 상한 무시(전환 대상 없을 때 계속 스크롤).
	void			cancel_drag();								//드래그 중 ESC 등으로 드롭 없이 완전 취소.
	bool			m_swallow_rbutton = false;					//드래그 취소용 우클릭의 RBUTTONUP 을 소비할지(팝업 메뉴 방지).
	int				m_nDragIndex = -1;			//drag되는 컨트롤이 CListCtrl일 때 그 인덱스(drag를 시작한 컨트롤의 멤버값에 저장됨, 드롭된 클래스에는 저장되지 않음)
	int				m_nDropIndex = -1;			//drop된 컨트롤이 CListCtrl일 때 그 인덱스(drag를 시작한 컨트롤의 멤버값에 저장됨, 드롭된 클래스에는 저장되지 않음)
	std::deque<UINT> m_drag_images_id;			//drag할 때 사용하는 이미지들의 resource id 저장(단일파일용 이미지, 싱글파일용 이미지를 차례대로 넣고 drag되는 개수에 따라 맞는 이미지를 사용한다)
	void			DroppedHandler(CWnd* pDragWnd, CWnd* pDropWnd);

	//20260706 by claude. 마퀴(rubber-band) 선택 — 빈 공간을 드래그해 사각형에 겹치는 항목을 선택. 아이템 드래그(m_bDragging)와 별개.
	bool			m_marquee_active = false;			//마퀴 드래그 중.
	CPoint			m_marquee_start = CPoint(0, 0);		//드래그 시작(client 좌표).
	CPoint			m_marquee_cur = CPoint(0, 0);		//현재 커서(client 좌표).
	bool			m_marquee_ctrl = false;				//드래그 시작 시 Ctrl 눌림 여부(추가 모드) — 드래그 내내 고정.
	std::deque<int>	m_marquee_base;						//드래그 시작 시점의 선택 스냅샷(Ctrl 드래그 = 기존 선택에 추가).
	//20260707 by claude. 직전 apply 에서 마퀴에 세로로 걸친 연속 항목 구간(빈=hi<lo). 매 이동마다 전체 N 을 훑지 않고
	//이 구간과 새 구간의 합집합만 재평가 → 대량 리스트에서 드래그 선택 성능 확보(항목 균일 높이라 겹치는 항목은 항상 연속 구간).
	int				m_marquee_lo = 0;
	int				m_marquee_hi = -1;
	void			start_marquee(CPoint point, bool ctrl);	//빈 공간 눌림 → 마퀴 시작(capture, 스냅샷, plain 이면 선택 해제).
	void			end_marquee();						//마퀴 종료(capture 해제, 타이머 정지, 사각형 지우기).
	int				m_marquee_scroll_vx = 0;			//마퀴 자동 스크롤 속도(가로, tick당 level, 부호=방향). 거리비례 가속.
	int				m_marquee_scroll_vy = 0;			//마퀴 자동 스크롤 속도(세로).
	void			update_marquee_auto_scroll(CPoint client_pt);	//커서의 client 가장자리 거리로 속도 산출 + 타이머 관리.
	void			apply_marquee_selection(bool ctrl);	//현재 마퀴 사각형으로 선택 갱신.
	void			row_screen_rect(int item, CRect& out);	//item 행의 화면 rect(smooth=m_scroll_y 기준, native=GetItemRect).
	void			draw_marquee(CDC* pDC);				//OnPaint 에서 마퀴 사각형 렌더.

	//20260707 by claude. [smooth] 드래그 제스처 감지 — native 모드는 리스트뷰가 LVN_BEGINDRAG 를 쏘지만 smooth 는
	//OnLButtonDown 이 native 를 우회 소비하므로 직접 감지한다. 항목 위 LButton 누름 후 문턱 이상 이동하면 LVN_BEGINDRAG 를 합성.
	bool			m_smooth_drag_pending = false;		//항목 위 LButton 눌림 — 드래그 제스처 대기 중.
	CPoint			m_smooth_drag_pt = CPoint(0, 0);	//누른 지점(client).
	int				m_smooth_drag_item = -1;			//누른 항목.
	int				m_smooth_click_defer = -1;			//다중선택 상태에서 이미 선택된 항목을 plain 클릭 → 드래그가 아니면 LButtonUp 에서 single 로 축소.

	//https://jiniya.net/tt/594/
	//이 함수는 드래그 이미지를 직접 생성해주는 코드지만 취약점이 많은 코드이므로 참고만 할것.
	//20260704 by claude. out_bmp 를 주면 합성한 드래그 비트맵을 deep_copy 로 반환(CSCShapeDlg::set_image 용). CImageList 반환은 하위호환.
	CImageList* create_drag_image(CListCtrl* pList, LPPOINT lpPoint, CSCGdiplusBitmap* out_bmp = nullptr);

	bool			m_use_indent_from_prefix_space = false;

	bool			m_draw_selected_border = true;
	bool			m_use_distinct_border_color = false;
	int				m_selected_border_width = 1;
	int				m_selected_border_style = Gdiplus::DashStyleSolid;

	bool			m_has_focus = false;

	bool			m_draw_top_line = false;
	Gdiplus::Color	m_cr_top_line = Gdiplus::Color::LightGray;

	bool			m_draw_bottom_line = false;
	Gdiplus::Color	m_cr_bottom_line = Gdiplus::Color::LightGray;

//스크롤 버튼
	CGdiButton*		m_button_scroll_to_end = NULL;
	int				m_auto_scroll_button_size = 24;
	void			init_auto_scroll_button();

//popup(context) menu
	//true=자체 내장 컨텍스트 메뉴 표시, false=parent(OnContextMenu)로 위임. default = true
	bool			m_use_own_context_menu = true;
	//자체 내장 메뉴 command id. 범용 컨트롤이므로 파일 개념 없는 범용 항목만 제공.
	enum CVTLISTCTRLEX_POPUP_MENU
	{
		menu_select_all = WM_USER + 1600,
		menu_unselect_all,
	};
	afx_msg void	OnPopupMenu(UINT nID);

protected:
	DECLARE_MESSAGE_MAP()
public:
	void			set_use_own_context_menu(bool use = true) { m_use_own_context_menu = use; }
	afx_msg void OnLvnGetdispinfo(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnLvnOdfinditem(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnLvnOdcachehint(NMHDR *pNMHDR, LRESULT *pResult);
	virtual void DrawItem(LPDRAWITEMSTRUCT /*lpDrawItemStruct*/);
	//20260706 by claude. 한 행(모든 셀)을 주어진 픽셀 rect 에 그린다 — DrawItem(native 위임) 과 smooth 픽셀 페인트가 공용.
	void		draw_row(CDC* pDC, int iItem, const CRect& row_bounds);
	//20260706 by claude. [smooth §3] 키보드 네비게이션 — 방향키/PageUp·Down/Home·End 로 포커스 이동+선택+가시유지. 처리하면 true.
	bool		smooth_key_navigate(UINT vk);
	//20260706 by claude. 행 높이(픽셀) — m_line_height 미설정(0 이하) 시 16 기본. 픽셀 페인트/히트테스트/스크롤 계산이 공유.
	int			row_height() const { return (m_line_height > 0) ? m_line_height : 16; }
	//20260706 by claude. [smooth §3] item 이 항목 영역에 완전히 보이도록 m_scroll_y 최소 조정(native EnsureVisible 의 픽셀판).
	void		smooth_ensure_visible(int item);
	//20260706 by claude. subItem 컬럼이 가로 뷰포트에 완전히 들어오도록 가로 스크롤(편집 진입 전 셀 노출). smooth/native 공용.
	void		ensure_column_visible(int subItem);
	//20260706 by claude. 선택 원자연산 — 마우스(L/R)·키보드 핸들러가 공유(중복 방지). 정책(ctrl/shift 의미)은 각 핸들러가 결정하고 연산만 공용.
	void		select_single(int item);			//item 만 선택, 나머지 해제, m_focus_anchor=item.
	void		select_range(int anchor, int item);	//[anchor,item] 구간 선택, 나머지 해제(anchor 불변).
	void		select_range_add(int anchor, int item);	//[anchor,item] 구간을 기존 선택에 추가(나머지 유지) — Ctrl+Shift.
	void		toggle_item_select(int item);		//item 선택 토글, m_focus_anchor=item.
	//20260706 by claude. 체크박스 토글 — 키보드(Space)·마우스(OnNMClick) 공유. 토글 + 재그리기 + parent(message_checked_item) 통지.
	void		toggle_check(int item);
	afx_msg void OnLvnColumnclick(NMHDR *pNMHDR, LRESULT *pResult);
	virtual void PreSubclassWindow();
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	afx_msg void OnLvnOdstatechanged(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg BOOL OnEraseBkgnd(CDC* pDC);
	afx_msg void OnPaint();
	afx_msg BOOL OnLvnBeginLabelEdit(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg BOOL OnLvnEndLabelEdit(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg BOOL OnNMClick(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg BOOL OnNMDblclk(NMHDR *pNMHDR, LRESULT *pResult);
	//afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	//afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
//	afx_msg void OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
	afx_msg void OnDropFiles(HDROP hDropInfo);
	afx_msg void MeasureItem(LPMEASUREITEMSTRUCT lpMeasureItemStruct);
	afx_msg void OnLvnBeginDrag(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
	afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnCaptureChanged(CWnd* pWnd);	//20260714 by claude. 캡처 상실(윈도우 키 등) 시 드래그 취소.
	afx_msg void OnRButtonDown(UINT nFlags, CPoint point);	//드래그 중 우클릭 = 취소.
	afx_msg void OnRButtonUp(UINT nFlags, CPoint point);	//취소용 우클릭의 UP 소비(팝업 메뉴 방지).
	afx_msg void OnTimer(UINT_PTR nIDEvent);	//드래그 자동 스크롤 연속 tick.
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
	afx_msg BOOL OnLvnItemchanged(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnContextMenu(CWnd* pWnd, CPoint point);
	afx_msg BOOL OnNMRClick(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnMouseHover(UINT nFlags, CPoint point);
	afx_msg void OnMouseLeave();
	afx_msg void OnSetFocus(CWnd* pOldWnd);
	afx_msg void OnKillFocus(CWnd* pNewWnd);
	afx_msg BOOL OnLvnItemchanging(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg LRESULT OnNcHitTest(CPoint point);
	afx_msg void OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
	afx_msg void OnVScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
	afx_msg void OnLvnBeginScroll(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnLvnEndScroll(NMHDR* pNMHDR, LRESULT* pResult);
	//native header 의 raw HDN_* notification 직접 가로채기 — listview 가 reflect 하지 않는 경로.
	virtual BOOL OnNotify(WPARAM wParam, LPARAM lParam, LRESULT* pResult) override;
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnWindowPosChanged(WINDOWPOS* lpwndpos);
	afx_msg void OnNcPaint();
	afx_msg void OnNcCalcSize(BOOL bCalcValidRects, NCCALCSIZE_PARAMS* lpncsp);
	afx_msg void OnStyleChanging(int nStyleType, LPSTYLESTRUCT lpStyleStruct);
	afx_msg BOOL OnMouseWheel(UINT nFlags, short zDelta, CPoint pt);
	afx_msg void OnMouseHWheel(UINT nFlags, short zDelta, CPoint pt);

	//자체 스크롤바 — 세로/가로 모두 *부모 dialog 의 child*. OnNcCalcSize 가 세로바는 우측 gw, 가로바는 하단 gw 를
	//NC 로 예약해 native content 영역을 실제 가시 영역과 일치시킨다 → native 의 스크롤 한계·divider hit-test·count
	//per page 가 자동으로 맞아 overlay-vs-native 불일치(컬럼이 바 밑에 가림, 마지막 항목 가림 등)가 구조적으로 제거됨.
	//바는 NC 영역(=client 밖)에 놓이므로 listctrl child 면 클리핑된다 → 둘 다 dialog child + set_message_target(this).
	CSCScrollbar	m_scrollbar;		//세로 — 우측 NC 띠
	CSCScrollbar	m_scrollbar_h;		//가로 — 하단 NC 띠 (column 가로폭 합계 > content 폭일 때)
	bool			m_scrollbar_setup = false;
	bool			m_v_visible_state = false;	//세로바 필요 상태 — OnNcCalcSize 가 읽어 우측 gw 예약
	bool			m_h_visible_state = false;	//가로바 필요 상태 — OnNcCalcSize 가 읽어 하단 gw 예약
	int				m_bottom_reserve = 0;		//세로바 시 하단 partial row 예약 px — OnNcCalcSize 가 설정, sync(바 위치)·OnNcPaint(빈영역 fill)가 읽음
	bool			m_syncing = false;			//framechange 재진입 가드 (SWP_FRAMECHANGED → OnSize → sync 재진입 차단)
	bool			m_snapping = false;			//top 항목 헤더 스냅 재진입 가드 (Scroll → LVN_ENDSCROLL 재귀 차단)
	//20260706 by claude. [smooth scroll — 픽셀 페인트 모드] 항목을 직접 픽셀 위치에 그려 부분행/하단여백0 을 구현.
	//20260708 by claude. 기본값을 true 로 — CSCListCtrl 은 native 위임의 근본적 하단여백 한계를 없애려 만든 픽셀 페인트 리스트라, smooth 가 기본이다.
	//(CVtListCtrlEx 를 이걸로 대체 중. 다른 소비자는 아직 CVtListCtrlEx 를 쓰므로 기본 변경은 안전.) set_smooth_scroll(false) 로 native 위임 복귀 가능.
	//m_scroll_y = 픽셀 세로 오프셋(항목 영역 기준, 헤더 아래).
	bool			m_smooth_scroll = true;
	int				m_scroll_y = 0;
	int				m_focus_anchor = -1;		//smooth 모드 shift-range 선택 기준 항목.
	//WS_BORDER/WS_EX_CLIENTEDGE 가 켜져 있으면 PreSubclassWindow 에서 native border 제거하고 이 플래그를 켠 뒤
	//OnNcCalcSize 에서 1px NC 확보 → OnNcPaint 가 theme 색으로 직접 그린다. (CSCTreeCtrl 와 동일 패턴.)
	bool			m_draw_border = false;
	//임베드(child 로 심어 생성 타이밍이 다른 경우) 등으로 PreSubclassWindow 의 frame change 가 WM_NCCALCSIZE 를
	//못 띄워 1px NC 가 확보 안 되면 OnNcPaint 가 테두리를 그려도 client 가 덮어 안 보인다. 확보 여부를 추적해,
	//미확보 상태로 paint 되면 표시 시점에 frame change 를 1회 강제해 NCCALCSIZE 가 1px 를 확보하게 한다.
	bool			m_border_nc_reserved = false;	//OnNcCalcSize 가 1px 를 실제 확보했는지.
	bool			m_border_recalc_tried = false;	//미확보 시 강제 recalc 를 이미 시도했는지(무한 재시도 방지).
	int				m_last_top_index = -1;
	int				m_h_scroll_pos = 0;		//가로 scroll 누적 pixel offset — WS_HSCROLL 제거 후 GetScrollPos(SB_HORZ) 가 stale 이라 자체 추적.
	DWORD			m_last_user_scroll_at = 0;	//사용자 입력에 의한 scroll tick — 자동 ensure_visible 호출 측에서 sticky timeout 으로 사용.
	DWORD			m_last_hsync_tick = 0;		//20260706 by claude. 가로 스크롤 sync 스로틀용 마지막 sync tick(WM_HSCROLL 폭주 완화).
	void			setup_scrollbar();
	void			sync_scrollbar();
	LRESULT			on_message_CSCScrollbar(WPARAM wParam, LPARAM lParam);

	//20260712 by claude. 리사이즈 드래그(WM_ENTER/EXITSIZEMOVE) 동안 오버레이 바 window 조작(MoveWindow/SetWindowPos —
	//dialog WS_CLIPCHILDREN 형제 clip 재계산으로 매우 비쌈, 리스트당 5~10ms)을 전부 건너뛰어 리사이즈 성능을 확보한다.
	//app 이 ENTER 시 set_live_resize(true)(바 숨김), EXIT 시 set_live_resize(false) 후 각 컨트롤 sync_scrollbar 로 복원.
	//모든 인스턴스가 같은 창에서 함께 리사이즈되므로 static 공유.
	static void		set_live_resize(bool b) { s_in_live_resize = b; }
	static bool		s_in_live_resize;
	//20260712 by claude. 리사이즈 중 '바 숨김' 최적화를 이 컨트롤에 적용할지. 기본 false(바가 계속 보임 — 자연스러움). 컨트롤이
	//많아 리사이즈가 정말 느린 앱에서만 set_hide_scroll_when_resize(true) 로 켜면 리사이즈 드래그 중 바를 숨겨 리스트당 5~10ms 를 절감.
	//(true 면 드래그 중 바가 잠깐 사라졌다 놓으면 복원. false 면 바는 유지되나 리사이즈가 느림.) 컨트롤별 선택 가능.
	void			set_hide_scroll_when_resize(bool b) { m_hide_scroll_when_resize = b; }
	bool			m_hide_scroll_when_resize = false;

public:
	DWORD			get_last_user_scroll_at() const { return m_last_user_scroll_at; }
};


