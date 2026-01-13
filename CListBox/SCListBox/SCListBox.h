#if !defined(AFX_SCLISTBOX_H__5529A6B1_584A_11D2_A41A_006097BD277B__INCLUDED_)
#define AFX_SCLISTBOX_H__5529A6B1_584A_11D2_A41A_006097BD277B__INCLUDED_


/*
* [개발 배경]
* 로그 등을 출력하는 용도로 기존 CRichEditCtrlEx를 사용했으나
* 맨 마지막을 SetSel하고 이를 ReplaceSel을 하여 출력하는 방식이다보니
* 로그가 계속 추가되는 상태에서는 스크롤 및 텍스트 선택 등이 매우 불편했다.
* 그래서 CStatic을 상속받은 COutputStatic을 제작해봤으나
  CStatic은 키보드 입력이 처리되지 않고 스크롤 관련 처리도 번거롭다.
  CListBox를 상속받아 위 단점들을 없애기 위해 제작함.

* DWORD의 컬러값을 SetItemData()로 저장하여 컬러 텍스트를 출력.
* log 출력등에 CRichEditCtrlEx를 대신하기 위해 수정.
* (임의 항목을 클릭하면 auto_scroll이 off되고 Ctrl+End 등에 의해 auto_scroll이 on됨.
* CPathCtrl에서 동적 팝업 리스트창으로 표시하기 위해서도 사용됨.
* 
* [warning]
* - 컨트롤의 너비, 높이가 어느 이상되면 표시 속도가 현저히 느려진다.
* 
* [setting]
* - 필수 : Has String = true, Owner Draw = Fixed, No Integral Height = false
* - 선택 : Multiline, Sort
* 
* [수정될 내용]
* - text color를 SetItemData()를 이용하여 지정하고 있으나 image index 등을 저장하려면
*   struct를 사용해야 한다.
*   단, 이 클래스를 사용하는 모든 프로젝트에서 이렇게 할 경우 불필요한 리소스를 할당해야 하므로
*   flag처리로 꼭 필요할 경우에만 사용하도록 수정되어야 한다.
*/



#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#include <afxwin.h>
#include <deque>
#include "../../Functions.h"
#include "../../system/ShellImageList/ShellImageList.h"
#include "../../SCGdiplusBitmap.h"
#include "../../colors.h"
#include "../../CEdit/SCEdit/SCEdit.h"


//ROOT_LABEL은 PathCtrl에서 최상위를 표시하기 위한 용도임.
//#define ROOT_LABEL _T("_r")

// SCListBox.h : header file

//-------------------------------------------------------------------
//
//	CSCListBox class - 
//		A CListBox-derived class with optional colored items.
//
//		Version: 1.0	01/10/1998 Copyright ?Patrice Godard
//
//		Version: 2.0	09/17/1999 Copyright ?Paul M. Meidinger
//
//-------------------------------------------------------------------

/////////////////////////////////////////////////////////////////////////////
// CSCListBox window

static const UINT Message_CSCListBox = ::RegisterWindowMessage(_T("MessageString_CSCListBox"));

class CSCListBoxMessage
{
public:
	CSCListBoxMessage(CWnd* _this, int _message)
	{
		pThis = _this;
		message = _message;
	}

	CWnd*	pThis = NULL;
	int		message;
};


class CSCListBoxItem
{
	CSCListBoxItem(WIN32_FIND_DATA _data, UINT _icon_index)
	{
		data = _data;
		icon_index = _icon_index;
	}

	WIN32_FIND_DATA		data;
	UINT				icon_index = 0;	//remote의 파일/폴더목록을 표시할 경우 이 값
};

class CSCListBox : public CListBox
{
// Construction
public:
	CSCListBox();
	virtual ~CSCListBox();

	CSCColorTheme	m_theme = CSCColorTheme(this);

	enum MESSAGES
	{
		message_selchanged = 0,
		message_edit_end,
	};

// Attributes

// Operations
public:
	//가변 인자를 사용하면 CString::FormatV()를 이용하는데 문자열에 %가 포함되면 format specifier로 인식되므로 에러가 발생한다.
	//add 대신 insert_string()을 이용하자.
	/*
	//기본 글자색으로 한 줄 추가
	int			add(LPCTSTR lpszFormat, ...);

	//지정 글자색으로 한 줄 추가.
	//cr = Gdiplus::Color::Transparent일 경우는 기본 글자색을 사용한다. 이때에는 위의 add()를 사용하면 된다.
	int			add(Gdiplus::Color cr, LPCTSTR lpszFormat, ...);

	//마지막 라인의 데이터에 텍스트 append
	int			append(LPCTSTR lpszFormat, ...);
	*/

	//한줄의 내용을 추가할 경우 (printf()와 같이 사용 가능)
	int			add(LPCTSTR text, ...);

	//여러줄의 문자열을 한번에 추가(shell_listbox에 사용)
	int			add(std::deque<CString>* lists, Gdiplus::Color cr = Gdiplus::Color::Transparent);

	int			insert_string(int nIndex, CString lpszItem, Gdiplus::Color rgb = Gdiplus::Color::Transparent);	// Inserts a colored string to the list box
	void		set_item_color(int nIndex, Gdiplus::Color rgb, bool invalidate = true);	// Sets the color of an item in the list box
	Gdiplus::Color	get_item_color(int nIndex);

	CString		get_text(int index);
	void		set_text(int index, CString text, Gdiplus::Color cr = Gdiplus::Color::Transparent);

	void		clear_all() { ResetContent(); Invalidate(); }

	CSize		resizeToFit(bool bHori = true, bool bVert = true);			//변경된 크기를 리턴한다.
	void		set_minimum_lines(int lines) { m_nMinimumLines = lines; }

	//선택 관련
	//선택된 항목 리스트 및 선택된 개수를 리턴
	int			get_selected_items(std::deque<int>* selected = NULL);
	void		set_show_selection_always(bool always_show) { m_show_selection_always = always_show; }
	int			get_item_from_pos(int x, int y);

	//팝업 메뉴. 맨 처음/끝 항목이 변경되면 반드시 아래의 명령범위 코드도 변경시켜줘야 한다.
	//ON_COMMAND_RANGE(menu_selected_count, menu_delete_selected, OnPopupMenu)
	enum CONTEXT_MENU
	{
		menu_selected_count = WM_USER + 1234,
		menu_clear_all,
		menu_show_log,
		menu_show_date,
		menu_show_time,
		menu_auto_scroll,
		menu_copy_selected_to_clipboard,
		menu_copy_all_to_clipboard,
		menu_save_selected,				//선택 로그만 파일로 저장
		menu_save_all,					//전체 로그 파일로 저장
		menu_delete_selected,
	};

	bool		m_use_popup_menu = true;
	void		use_popup_menu(bool use) { m_use_popup_menu = use; }
	void		OnPopupMenu(UINT nID);

	//1:show, 0:hide, -1:no change
	void		show_date_time(int date = -1, int time = -1);

	//라인 간격
	int			get_line_height() { return m_line_height; }
	void		set_line_height(int _line_height);

	void		use_over(bool use = true) { m_use_over = use; }
	int			get_over_item() { return (m_use_over ? m_over_item : -1); }

	//색상
	void		set_text_color(Gdiplus::Color cr) { m_theme.cr_text = cr; Invalidate(); }
	void		set_back_color(Gdiplus::Color cr) { m_theme.cr_back = cr; Invalidate(); }

	void		set_draw_border(bool draw = true) { m_draw_border = draw; }

	int			GetGutterCharNumber() { return m_nGutterCharNumber; }
	void		SetGutterCharNumber(int chars) { m_nGutterCharNumber = chars; }

	//folder list로 동작시킨다.
	void		set_as_folder_list();
	void		set_parent(HWND hWnd) { m_hParentWnd = hWnd; }
	//root라는 폴더의 하위 폴더들을 리스트에 표시한다.
	int			set_path(CString root, CString selected_text = _T(""));
	//lists를 NULL하여 호출하면 멤버변수인 m_folder_list의 내용을 표시한다.
	int			set_folder_list(std::deque<CString>* lists = NULL, CString selected_text = _T(""));


	void		set_font(LOGFONT& lf);
	void		set_font_name(CString sFontname, BYTE byCharSet = DEFAULT_CHARSET);
	void		set_font_size(int nSize);
	void		set_font_bold(int weight = FW_BOLD);

	void		set_color_theme(int theme);

	CShellImageList* m_pShellImageList = NULL;
	void		set_shell_imagelist(CShellImageList* pShellImageList, bool is_local) { m_pShellImageList = pShellImageList; m_is_local = is_local; }

	//선택 불가 리스트로 동작. disable로 한 후 색상등은 지정된대로 표시한다.
	//이렇게 해야 키보드, 마우스 이벤트들을 쉽게 차단할 수 있다.
	void		set_as_static(bool as_static = true) { EnableWindow(FALSE); m_as_static = as_static; }

	//편집 관련
	bool		get_use_edit() { return m_use_edit; }
	void		set_use_edit(bool use = true, bool readonly = false) { m_use_edit = use; m_edit_readonly = readonly; }
	//index == -1이면 선택된 첫번째 항목 편집
	void		edit(int index = -1);
	//modify가 true이면 편집된 텍스트로 변경, 그렇지 않으면 기존 텍스트 유지.
	void		edit_end(bool modify = true);
	//편집된 데이터가 empty이어도 정상 데이터로 처리할 지...
	//false일 경우는 해당 라인을 제거한다.
	void		set_accept_empty_edit_str(bool accept = true) { m_accept_empty_edit_str = accept; }

	//use own imagelist
	//png 이미지를 label의 앞에 표시한다. 2장 이상일 경우 alt효과를 줄 수 있다. id가 0이면 clear()로 동작한다.
	void		add_to_imagelist(UINT id);
	//png 이미지를 label의 앞에 표시한다. 2장 이상일 경우 alt효과를 줄 수 있다.
	template <typename ... T> void set_imagelist(T... args)
	{
		int n = sizeof...(args);
		int arg[] = { args... };

		//set_이므로 기존 항목들을 모두 제거한 후 추가해야 한다.
		//그냥 하나를 추가한다면 add_header_image()를 사용한다.
		for (int i = 0; i < m_imagelist.size(); i++)
		{
			CSCGdiplusBitmap* img = m_imagelist[i];
			delete img;
		}

		m_imagelist.clear();

		for (auto id : arg)
		{
			//여기서 직접 new로 할당받고 load하여 deque에 넣으려했으나
			//여기서는 들어간 것처럼 보였지만 실제 OnPaint()에서 보면 deque가 비어있었다.
			//template이라 그런지 여기서 바로는 들어가지 않는다.
			//멤버함수를 통해 넣어야 한다.
			add_to_imagelist(id);
		}
	}

	//for_selected = true : 선택된 항목들을 삭제, false : 전체 항목 삭제
	void		delete_items(bool for_selected = true);

	//항목 추가/삭제시에 가로 스크롤바를 재계산해준다.
	//추가시에는 추가된 항목과 최대크기 항목을 비교하고 삭제시에는 모든 항목을 재계산해야 한다.
	void		recalc_horizontal_extent(CString added_text = _T(""));

protected:
	//동적생성한 경우 GetParent등으로도 parent가 구해지지 않고 OnNotify()도 동작하지 않아서 수동으로 세팅하기 위함.
	HWND		m_hParentWnd = NULL;

	bool		m_as_static = false;				//true일 경우 키보드, 마우스에 의한 선택 불가
	bool		m_use_over = false;					//hover hilighted
	int			m_over_item = -1;
	bool		m_as_popup = false;					//팝업모드로 동작하는 리스트박스일 경우는 killfocus이면 숨겨진다.

	bool		m_is_local = true;
	bool		m_as_folder_list = false;			//폴더목록을 표시하는 목적으로 동작하는 경우
	std::deque<CString> m_folder_list;

	bool		m_show_log = true;
	bool		m_show_date = false;
	bool		m_show_time = false;
	bool		m_dim_time_str = true;				//시간 문자열은 연한 회색으로 비강조되도록 표시

	bool		m_draw_border = true;				//border

	//LBS_SHOWSELALWAYS 라는 속성이 없으므로 직접 추가
	bool		m_show_selection_always = true;

	LOGFONT		m_lf;
	CFont		m_font;
	void		ReconstructFont();

	//라인 높이는 글꼴 높이에 따라 자동 계산된다.
	//만약 수동으로 라인 간격을 변경하려면 set_line_height(32); 함수를 이용해야 한다.
	//font_size를 증감했는데 라인 간격이 변경되지 않는 것도 문제가 되니 필요할 경우
	//해당 함수를 통해 설정하자.
	int			m_line_height;

//편집 관련
	bool		m_use_edit = false;
	bool		m_in_editing = false;
	bool		m_edit_readonly = false;
	int			m_edit_index = -1;
	//편집된 데이터가 empty이어도 정상 데이터로 처리할 지...
	//false일 경우는 해당 라인을 제거한다.
	bool		m_accept_empty_edit_str = true;
	CSCEdit*	m_pEdit = NULL;
	LRESULT		on_message_CSCEdit(WPARAM wParam, LPARAM lParam);

	//선택 관련
	//std::deque<int>	 m_selected;
	//int			m_last_selected = -1;	//마지막 선택한 항목(범위 선택시 필요)

	//vert fit에서 최소 표시 라인수를 정해놓는다.default = -1(정하지 않을 경우)
	int			m_nMinimumLines;



	//항목 왼쪽에 인덱스를 표시하는 gutter 영역 설정
	//단위는 픽셀이 아닌 문자 갯수임. default = 0;
	int			m_nGutterCharNumber = 0;

	//스크롤 기능 관련
	bool		m_auto_scroll = true;
	bool		m_is_thumb_tracking = false;
	int			m_max_horizontal_extent = 0;

	CString		get_all_text(bool selected_only = false);
	void		copy_selected_to_clipboard();
	void		copy_all_to_clipboard();
	void		save_selected_to_file();
	void		save_all_to_file();

	std::deque<CSCGdiplusBitmap*> m_imagelist;

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CSCListBox)
	public:
	//afx_msg LRESULT OnSetFont(WPARAM wParam, LPARAM);
	//}}AFX_VIRTUAL


	// Generated message map functions
protected:
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
	afx_msg BOOL OnEraseBkgnd(CDC* pDC);
	virtual void PreSubclassWindow();
	afx_msg void DrawItem(LPDRAWITEMSTRUCT /*lpDrawItemStruct*/);
	afx_msg void OnPaint();
	//afx_msg void OnNMCustomdraw(NMHDR *pNMHDR, LRESULT *pResult);
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	afx_msg void OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags);
	afx_msg void OnKillFocus(CWnd* pNewWnd);
	afx_msg BOOL OnLbnSelchange();
	afx_msg void OnContextMenu(CWnd* /*pWnd*/, CPoint /*point*/);
	afx_msg BOOL OnMouseWheel(UINT nFlags, short zDelta, CPoint pt);
	afx_msg void OnMouseHWheel(UINT nFlags, short zDelta, CPoint pt);
	//	afx_msg void OnDrawItem(int nIDCtl, LPDRAWITEMSTRUCT lpDrawItemStruct);
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnLButtonUp(UINT nFlags, CPoint point);

	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnWindowPosChanged(WINDOWPOS* lpwndpos);
	afx_msg void OnNcPaint();
	afx_msg HBRUSH OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor);
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_SCLISTBOX_H__5529A6B1_584A_11D2_A41A_006097BD277B__INCLUDED_)
