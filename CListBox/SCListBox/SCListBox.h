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
#include "../../CScrollbar/SCScrollbar/SCScrollbar.h"
#include "../../win_compat/dwm.h"


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
public:
	CSCListBoxItem() {}
	//Transparent일 경우는 기본 글자색, 배경색을 사용하는 의미
	CSCListBoxItem(CString _text, Gdiplus::Color _cr_text = Gdiplus::Color::Transparent, Gdiplus::Color _cr_back = Gdiplus::Color::Transparent)
	{
		text = _text;
		cr_text = _cr_text;
		cr_back = _cr_back;
	}

	CString			text;
	CString			alt_text;		//추가적인 str 저장
	Gdiplus::Color	cr_text;
	Gdiplus::Color	cr_back;
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
	//add 대신 insert()를 이용하자.
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

	int			insert(int index, CString text, Gdiplus::Color cr_text = Gdiplus::Color::Transparent);	// Inserts a colored string to the list box
	void		set_item_color(int nIndex, Gdiplus::Color rgb, bool invalidate = true);	// Sets the color of an item in the list box
	Gdiplus::Color	get_item_color(int nIndex);

	CString		get_text(int index);
	void		set_text(int index, CString text, Gdiplus::Color cr = Gdiplus::Color::Transparent);
	CString		get_alt_text(int index);
	void		set_alt_text(int index, CString _alt_text);

	//find_main_text = false인 경우는 alt_text에서 찾는다.
	int			find(int start, CString find_str, bool find_main_text = true, bool whole_word = true);

	int			size() { return GetCount(); }
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

	//popup 모드에서 N 라인을 모두 보여주려면 필요한 전체 window height. NC padding 포함.
	int			calc_popup_height_for_lines(int lines) { return lines * m_line_height + (m_as_popup ? 2 * m_popup_padding : 0); }

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
	void		set_font_weight(int weight = FW_BOLD);
	void		set_font_quality(int quality);
	//aa_from_pt 미만 → GDI 내장 비트맵(작은 글씨 또렷), 이상 → GDI+ 외곽선 렌더(매끈, embedded bitmap 회피). set_font_quality() 로 해제.
	void		set_font_quality_auto(bool on = true, int aa_from_pt = 11);

	void		set_color_theme(int theme, bool invalidate = false);
	//external CSCColorTheme 의 색을 그대로 가져와 적용 — parent dlg 가 dlg 전체 theme 을 자식 컨트롤들에 일관 전파하는 패턴.
	void		set_color_theme(const CSCColorTheme& theme, bool invalidate = false);

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

	//편집 모드 중 *다른 컨트롤·다른 앱·다이얼로그 빈 영역* 어디를 클릭해도 편집을 정상 종료(commit) 시키기 위한 훅.
	//CPathCtrl 과 동일 패턴 — WH_MOUSE 로 같은 스레드 외부 클릭 감지 → 메시지 post → on_end_edit_posted → edit_end(true).
	//다른 앱으로의 포커스 이동은 CSCEdit 의 WM_KILLFOCUS 가 on_message_CSCEdit 로 전달돼 처리 (기존 경로 유지).
	void		install_edit_mouse_hook();
	void		remove_edit_mouse_hook();
	static LRESULT CALLBACK	edit_mouse_hook_proc(int code, WPARAM wParam, LPARAM lParam);
	LRESULT		on_end_edit_posted(WPARAM wParam, LPARAM lParam);
	static CSCListBox*	s_editing_listbox;
	HHOOK		m_mouse_hook = NULL;

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

	//항목 이동. single selection이고 auto sort가 아닐 경우에만 정상 동작함.
	void		move_item(int from_index, int to_index);
	void		move_item_up(int index);
	void		move_item_down(int index);
	bool		is_available_move_item_up(int index);
	bool		is_available_move_item_down(int index);

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

	//true 면 m_aa_from_pt 미만은 GDI DrawText + 내장 비트맵(작은 글씨 또렷), 이상은 GDI+ 외곽선 렌더(매끈)로
	//자동 분기. set_font_quality() 로 명시 지정하면 해제.
	bool		m_auto_font_quality = true;
	//이 pt 이상부터 GDI+ 로 그린다. 굴림/돋움/궁서는 ~20px 까지 embedded bitmap 을 내장하고 GDI 는 lfQuality 가
	//무엇이든 그 비트맵을 써버려 큰 크기에서 톱니가 남는다. GDI+ 는 자체 래스터라이저라 내장 비트맵을 무시하고
	//외곽선을 그려 매끈해진다.
	int			m_aa_from_pt = 11;
	//ReconstructFont 가 계산: 현재 크기가 m_aa_from_pt 이상이면 DrawItem 이 본문을 GDI+ 로 그린다.
	bool		m_text_smooth = false;

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
	afx_msg void OnNcCalcSize(BOOL bCalcValidRects, NCCALCSIZE_PARAMS* lpncsp);
	afx_msg HBRUSH OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor);
	//reflected: 부모 (PlaylistDlg 등) 가 WM_CTLCOLORLISTBOX 를 처리 안 하면 MFC 가 child 에 reflect.
	//owner-draw listbox 의 빈 영역 (항목 없는 공간) 도 이 brush 로 칠해짐 — 흰색 default 회피.
	afx_msg HBRUSH CtlColor(CDC* pDC, UINT nCtlColor);
	afx_msg void OnDestroy();
	afx_msg void OnVScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);

	//base 의 ResetContent / DeleteString 을 hide — itemData 에 저장된 CSCListBoxItem* 를 먼저 해제해 leak 회피.
	void			ResetContent();
	int				DeleteString(UINT nIndex);
	//SetCurSel / SetTopIndex 도 hide — auto-scroll 로 top 이 바뀌면 sync_scrollbar 호출해 CSCScrollbar 위치 갱신.
	int				SetCurSel(int nSelect);
	int				SetTopIndex(int nIndex);

	//자체 스크롤바 — CSCTreeCtrl / CSCThumbCtrl 와 동일 패턴. WS_VSCROLL 제거 + OnNcCalcSize 가 NC 영역 0 → native scrollbar 안 보임.
	CSCScrollbar	m_scrollbar;
	int				m_scrollbar_width = 14;	//track(window) 폭 — 18→16→14 로 양쪽 1px씩 축소(CVtListCtrlEx 와 통일).
	bool			m_scrollbar_setup = false;

	//가로 themed 오버레이 — 네이티브 가로 스크롤바는 일절 쓰지 않고 이 컨트롤로만 가로 스크롤한다.
	//(CVtListCtrlEx / CSCTreeCtrl 와 동일한 m_scrollbar_h / m_h_scroll_pos 네이밍 — 컨트롤 간 일관성.)
	//표시 조건: m_use_hscroll(기본 on) 이고 콘텐츠 폭이 client 를 넘을 때만. tree/listctrl 처럼 overflow 시 항상 표시.
	//Why default on / no WS_HSCROLL gate: 네이티브 listbox 가 비어있을 때 WS_HSCROLL 비트를 스스로 제거하고
	//LB_SETHORIZONTALEXTENT 없이는 복원하지 않아, 리소스의 WS_HSCROLL 플래그를 런타임 GetStyle 로 읽는 것이 불가능하다.
	//끄려면 응용단에서 set_use_hscroll(false) 호출.
	CSCScrollbar	m_scrollbar_h;
	bool			m_use_hscroll = true;	//가로 스크롤 허용 여부(기본 on). 신뢰 불가한 WS_HSCROLL 대신 명시 API 로 제어.
	int				m_h_scroll_pos = 0;		//가로 스크롤 위치(px). DrawItem 이 텍스트/아이콘을 이만큼 왼쪽으로 이동.
	bool			m_show_corner = false;	//세로·가로 바가 둘 다 보일 때 우측 하단 corner 를 cr_back 으로 칠할지(sync_scrollbar 가 설정).
	//paint shift Y — 네이티브 SetTopIndex 의 내부 max top 은 H-bar 영역을 차감하지 않아 max 스크롤에서 마지막
	//항목 bottom 이 H-bar 영역과 겹친다. DrawItem 진입 시 DC window origin 을 이만큼 위로 이동시켜 시각적으로만
	//항목들을 끌어올려 마지막 항목이 H-bar 위로 완전히 보이게 한다. hit-test 는 window 좌표 기준이라 영향 없음.
	//max 스크롤(GetTopIndex == max_top) 이고 need_h 일 때만 양수. sync_scrollbar 가 갱신.
	int				m_v_paint_shift = 0;

	//popup mode (m_as_popup) 에서 4면 NC padding — items 가 border 에 붙지 않도록.
	//OnNcCalcSize 가 client 를 deflate, OnNcPaint 가 padding band 를 cr_back 으로 fill.
	int				m_popup_padding = 4;
	void			setup_scrollbar();
	void			sync_scrollbar();		//세로/가로 오버레이 위치·범위·표시를 한 번에 동기화(CVtListCtrlEx 패턴).

	//가로 스크롤 허용 여부(기본 on). false 면 넘쳐도 가로 바 미표시(한 줄 + 우측 clip).
	//리소스 WS_HSCROLL 은 네이티브 listbox 가 런타임에 동적 제거해 신뢰 불가하므로 이 API 로 제어한다.
	void			set_use_hscroll(bool on) { m_use_hscroll = on; if (m_scrollbar_setup) sync_scrollbar(); }
	LRESULT			on_message_CSCScrollbar(WPARAM wParam, LPARAM lParam);
	//PreSubclassWindow 에서 PostMessage 로 지연된 setup_scrollbar 처리 핸들러. CBT hook 종료 후 큐에서 호출됨.
	LRESULT			on_setup_scrollbar_deferred(WPARAM wParam, LPARAM lParam);

protected:
	CBrush			m_br_back;	//cr_back 색 brush — set_color_theme 에서 재생성, CtlColor 에서 반환.
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_SCLISTBOX_H__5529A6B1_584A_11D2_A41A_006097BD277B__INCLUDED_)
