#pragma once

#include <deque>
#include <Afxwin.h>
#include "list_data.h"
#include "HeaderCtrlEx.h"
#include "../../Functions.h"

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
*/
// CVtListCtrlEx

#define LCSB_CLIENTDATA 1
#define LCSB_NCOVERRIDE 2

#define MESSAGE_VTLISTCTRLEX	WM_USER + 0x7FFF - 0x7464

class CVtListCtrlEx : public CListCtrl
{
	DECLARE_DYNAMIC(CVtListCtrlEx)

public:
	CVtListCtrlEx();
	virtual ~CVtListCtrlEx();

	BOOL NCOverride;
	int Who;

	BOOL m_bHorizLine = TRUE;
	void HideScrollBars(int Type, int Which = SB_BOTH);

	enum VtListCtrlExMsgs
	{
		message_progress_pos = 0,
	};


//컬럼 관련
	//ex. "No,20;Item1,50;Item2,50"
	//컬럼 설정과 관련된 변수의 저장 크기는 컬럼의 수가 결정되면 정해진다.
	//따라서 set_headings()함수를 가장 먼저 호출하고 그 후에 컬럼 세팅을 하도록 하자.
	bool		set_headings(const CString& strHeadings);
	
	int			get_column_count();

	//default = LVCFMT_LEFT
	void		set_column_text_align(int column, int format = LVCFMT_LEFT, bool set_to_header = true);
	int			get_column_text_align(int column);

	CString		get_header_text(int column);
	void		set_header_text(int column, CString text);
	//default = HDF_LEFT
	void		set_header_text_align(int header, int format = HDF_LEFT);
	int			get_header_text_align(int header);

	enum
	{
		column_data_type_unknown = 0,
		column_data_type_text,
		column_data_type_numeric,
		//실제값은 정수지만 그려질땐 막대그래프로 그려짐. 100%가 최대값이므로 100넘는 값도 100%로 표시된다.
		//만약 최대값을 지정하거나 등의 다른 처리가 필요하다면 부가적인 코드가 필요하다.
		//일단 이 컬럼의 데이터 타입은 percentage를 막대그래프로 그려주는 것이 목적이므로
		//값은 0~100까지의 비율로 아예 넣어주자. 110으로 넣어도 관계없다. 다만 100%로 그려질 것이다.
		column_data_type_percentage_bar,
		column_data_type_percentage_grid,
	};
	int			get_column_data_type(int column);
	void		set_column_data_type(int column, int nType = column_data_type_text, bool invalidate = false);

//추가 관련
	//index 위치에 0번 컬럼이 text인 라인을 추가한다.(-1이면 맨 마지막에 추가)
	int			add_item(CString text = _T(""), bool ensureVisible = true, bool invalidate = true);
	int			add_line_string_item(CString line_string, CString separator = _T("|"), bool ensureVisible = true, bool invalidate = true);
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
	void		delete_all_items();
	void		DeleteAllItems() { AfxMessageBox(_T("use delete_all_items() function in virtual list control.")); }

//텍스트 관련
	CString		get_text(int item, int subItem);
	void		set_text(int item, int subItem, CString text, bool invalidate = true);
	//from ~ to 컬럼까지의 텍스트를 하나로 합쳐서 리턴해준다.
	CString		get_line_text(int index, int from = 0, int to = -1, CString sep = _T("|"));
	CString		get_line_text(int index, std::deque<int>* dqColumn = NULL, CString sep = _T("|"));
	std::deque<CString> get_line_text_list(int index, int from = 0, int to = -1);
	std::deque<CString> get_line_text_list(int index, std::deque<int>* dqColumn = NULL);
	

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
	void		sort_by_text_color(int column, int ascending);
	int			last_sorted_column_index() { return m_last_sorted_column; }

//컬러 관련
	enum listctrlex_color_theme
	{
		color_theme_default = 0,
		color_theme_light_blue,
		color_theme_navy_blue,
		color_theme_dark_blue,
		color_theme_dark_gray,
	};

	void	set_color_theme(int theme, bool apply_now = true);
	COLORREF get_text_color(int item, int subItem);
	COLORREF get_back_color(int item, int subItem);
	//특정 항목의 글자색 설정. erase가 true이면 crText 인자를 무시하고 기본 글자색으로 되돌린다.
	void	set_text_color(int item, int subItem, COLORREF crText, bool erase = false, bool invalidate = true);
	//특정 항목의 배경색 설정. erase가 true이면 crText 인자를 무시하고 기본 글자색으로 되돌린다.
	void	set_back_color(int item, int subItem, COLORREF crBack, bool erase = false);
	void	set_item_color(int item, int subItem, COLORREF crText, COLORREF crBack);
	void	set_default_item_color(COLORREF crText, COLORREF crBack);	//기본 글자색, 배경색을 설정한다.
	//컬럼이 percentage를 표시하는 컬럼일 경우 그 표시 색상을 설정한다.
	void	set_progress_color(COLORREF crProgress);

//편집 관련
	//리소스의 ListCtrl 속성에서 "레이블 편집" 속성은 default값인 false로 하고 이 함수로 편집 가능 여부를 설정해야 한다.
	void	allow_edit(bool allow_edit = true, bool one_click_edit = true);
	void	allow_one_click_edit(bool allow_one_click_edit) { m_allow_one_click_edit = allow_one_click_edit; }
	void	allow_edit_column(int column, bool allow_edit = true);
	bool	is_in_editing()	{ return m_in_editing; }	//편집중인지
	void	set_flag_in_editing(bool in_editing) { edit_end(); m_in_editing = in_editing; }
	int		get_recent_edit_item() { return m_edit_item; }
	int		get_recent_edit_subitem() { return m_edit_subItem; }
	void	set_recent_edit_subitem(int subItem) { m_edit_subItem = subItem; }
	std::deque<int>* get_selected_list_for_edit() { return &m_dqSelected_list_for_edit; }
	std::deque<int> m_dqSelected_list_for_edit;
	CString get_old_text() { return m_old_text; }
	CEdit*	edit_item(int item, int subItem);
	void	edit_end(bool valid = true);
	void	undo_edit_label();		//편집 전의 텍스트로 되돌린다.(예를 들어 편집 레이블이 파일명이고 파일명 변경이 실패한 경우 쓸 수 있다.)
	bool	is_modified() { return m_modified; }
	void	reset_modified_flag() { m_modified = false; }

//폰트 관련
	int		get_font_size();
	void	set_font_name(LPCTSTR sFontname, BYTE byCharSet = DEFAULT_CHARSET);
	//-1 : reduce, +1 : enlarge
	void	set_font_size(int font_size);
	void	enlarge_font_size(bool enlarge);
	void	set_font_bold(bool bold = true);
	void	set_font_italic(bool italic = true);
	LOGFONT	get_log_font() { return m_lf; }
	void	set_log_font(LOGFONT lf);

//파일, 클립보드 관련
	//Ctrl+C키로 선택된 항목을 클립보드로 복사할 수 있다. shift를 조합하면 헤더까지 포함된다.
	bool	list_copy_to_clipboard(bool selected_only = true, CString sSeparator = _T("|"), bool bHead = false);
	//void	paste_from_clipboard();
	bool	save(CString file, CString separator = _T("|"), bool selected_only = false);
	bool	load(CString sfile, CString separator = _T("|"), bool add_index = false, bool match_column_count = true, bool reset_before_load = true);

//기타 레이아웃 관련
	//header_height는 헤더클래스에서 HDM_LAYOUT 메시지가 발생해야 적용되는데
	//그렇게 되기 위해서는 SetFont / MoveWindow / SetWindowPos 등이 필요하므로
	//VtListCtrlEx에게 set_font_name() 호출시에 헤더도 SetFont를 적용하여 해결함.
	void	set_header_height(int height);

	//line height를 변경하는 방법은 가상의 이미지리스트를 이용하는 방법과
	//(실제 사용할 이미지리스트가 있는 경우는 위 방법을 사용할 수 없다)
	//MeasureItem을 이용하는 방법(OwnerDrawFixed only)이 있다.
	//(강제 갱신 방법 아직 미해결이며 resize를 하면 자동 적용되므로
	//우선은 mainDlg에서 이 함수 호출후에 RestoreWindowPosition()등과 같이
	//CVtListCtrlEx에 WM_SIZE가 발생하도록 하는 함수를 사용하면 된다)
	//CVtListCtrlEx가 아닌 그냥 mainDlg만 resize한다고 해서 되지 않는다.
	//자체 imagelist를 사용하지 않는 컨트롤이라면 쉽게 lineheight를 조절할 수 있다.
	bool	use_own_imagelist = true;
	void	set_use_own_imagelist(bool use) { use_own_imagelist = use; }
	void	set_line_height(int height);

	void	set_column_width(int nCol, int cx);
	void	load_column_width(CWinApp* pApp, CString sSection);
	void	save_column_width(CWinApp* pApp, CString sSection);
	CRect	get_item_rect(int item, int subItem);

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

	void random();

	DWORD index_from_point(int x, int y);

protected:
//메인 데이터
	std::deque<CListCtrlData> m_list_db;

//컬럼 관련
	CHeaderCtrlEx	m_HeaderCtrlEx;
	std::deque<int> m_column_data_type;
	std::deque<int> m_column_text_align;

	int				m_line_height = 16;

//정렬 관련
	bool			m_allow_sort;				//default = true
	std::deque<int> m_column_sort_type;			//asceding or descending
	int				m_last_sorted_column;		//마지막 정렬된 컬럼 인덱스(색상 정렬은 제외)

//컬러 관련
	COLORREF		m_crText;					//기본 글자색
	COLORREF		m_crTextSelected;			//선택 항목의 활성화(active) 글자색
	COLORREF		m_crTextSelectedInactive;	//선택 항목의 비활성화(inactive) 글자색
	COLORREF		m_crBack;					//기본 배경색
	COLORREF		m_crBackSelected;
	COLORREF		m_crBackSelectedInactive;
	COLORREF		m_crHeaderBack;
	COLORREF		m_crHeaderText;
	COLORREF		m_crProgress;				//percentage bar graph color

//편집기능 관련
	bool			m_allow_edit;			//항목 편집이 가능한지... 기본 false
	bool			m_allow_one_click_edit;	//항목을 클릭한 후 정해진 시간내에 다시 클릭될 때 편집모드로 할지
	std::deque<bool> m_allow_edit_column;	//각 컬럼마다 편집가능 여부를 설정할 수 있다. 단, m_allow_edit이 변경되면 모두 그 값으로 변경됨에 주의.
	bool			m_modified;				//항목의 차례 또는 내용이 수정된 경우 true
	bool			m_in_editing;			//편집중인지
	int				m_edit_item;			//편집중인 아이템 인덱스
	int				m_edit_subItem;			//편집중인 아이템 서브인덱스
	CString			m_old_text;				//편집 전 텍스트
	CEdit*			m_pEdit;			//
	long			m_last_clicked;

//폰트 관련
	LOGFONT			m_lf;
	CFont			m_font;
	int				m_font_size;
	void			reconstruct_font();


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
};


