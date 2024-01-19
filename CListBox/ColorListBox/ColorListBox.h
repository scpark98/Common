#if !defined(AFX_COLORLISTBOX_H__5529A6B1_584A_11D2_A41A_006097BD277B__INCLUDED_)
#define AFX_COLORLISTBOX_H__5529A6B1_584A_11D2_A41A_006097BD277B__INCLUDED_


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
* [caution]
* - 컨트롤의 너비, 높이가 어느 이상되면 표시 속도가 현저히 느려진다.
*/



#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#include <afxwin.h>
#include <deque>
#include "../../Functions.h"
#include "../../system/ShellImageList/ShellImageList.h"


//ROOT_LABEL은 PathCtrl에서 최상위를 표시하기 위한 용도임.
//#define ROOT_LABEL _T("_r")

// ColorListBox.h : header file

//-------------------------------------------------------------------
//
//	CColorListBox class - 
//		A CListBox-derived class with optional colored items.
//
//		Version: 1.0	01/10/1998 Copyright ?Patrice Godard
//
//		Version: 2.0	09/17/1999 Copyright ?Paul M. Meidinger
//
//-------------------------------------------------------------------

/////////////////////////////////////////////////////////////////////////////
// CColorListBox window

static const UINT Message_CColorListBox = ::RegisterWindowMessage(_T("MessageString_CColorListBox"));

class CColorListBoxMessage
{
public:
	CColorListBoxMessage(CWnd* _this, int _message)
	{
		pThis = _this;
		message = _message;
	}

	CWnd*	pThis = NULL;
	int		message;
};

class CColorListBox : public CListBox
{
// Construction
public:
	CColorListBox();

	enum MESSAGES
	{
		message_colorlistbox_selchange = 0,
	};

// Attributes

// Operations
public:
	//기본 글자색으로 한 줄 추가
	int			add(LPCTSTR lpszFormat, ...);

	//지정 글자색으로 한 줄 추가
	int			add(COLORREF cr, LPCTSTR lpszFormat, ...);

	//마지막 라인의 데이터에 텍스트 append
	int			append(LPCTSTR lpszFormat, ...);

	//여러줄의 문자열을 한번에 추가(shell_listbox에 사용)
	int			add(std::deque<CString>* lists, COLORREF cr = -1);

	int			insert_string(int nIndex, CString lpszItem);				// Inserts a string to the list box
	int			insert_string(int nIndex, CString lpszItem, COLORREF rgb = -1);	// Inserts a colored string to the list box
	void		set_item_color(int nIndex, COLORREF rgb, bool invalidate = true);	// Sets the color of an item in the list box
	COLORREF	get_item_color(int nIndex);

	void		clear_all() { ResetContent(); Invalidate(); }

	CSize		resizeToFit(bool bHori = true, bool bVert = true);			//변경된 크기를 리턴한다.
	void		set_minimum_lines(int lines) { m_nMinimumLines = lines; }

	//선택 관련
	//선택된 항목 리스트 및 선택된 개수를 리턴
	int			get_selected_items(std::vector<int>* selected = NULL);

	//팝업 메뉴
	enum CONTEXT_MENU
	{
		menu_selected_count = WM_USER + 1234,
		menu_show_log,
		menu_show_date,
		menu_show_time,
		menu_auto_scroll,
		menu_clear_all,
		menu_copy_selected_to_clipboard,
		menu_copy_all_to_clipboard,
		menu_save_selected,				//선택 로그만 파일로 저장
		menu_save_all,					//전체 로그 파일로 저장
	};

	bool		m_use_popup_menu = true;
	void		use_popup_menu(bool use) { m_use_popup_menu = use; }
	void		OnPopupMenu(UINT nID);


	//라인 간격
	int			get_line_height() { return m_line_height; }
	void		set_line_height(int _line_height);

	void		use_over(bool use = true) { m_use_over = use; }
	int			get_over_item() { return (m_use_over ? m_over_item : -1); }

	//색상
	void		set_text_color(COLORREF cr) { m_cr_text = cr; Invalidate(); }
	void		set_back_color(COLORREF cr) { m_cr_back = cr; Invalidate(); }

	int			GetGutterCharNumber() { return m_nGutterCharNumber; }
	void		SetGutterCharNumber(int chars) { m_nGutterCharNumber = chars; }

	//folder list로 동작시킨다.
	void		set_as_folder_list();
	void		set_parent(HWND hWnd) { m_hParentWnd = hWnd; }
	//root라는 폴더의 하위 폴더들을 리스트에 표시한다.
	int			set_path(CString root, CString selected_text = _T(""));
	//lists를 NULL하여 호출하면 멤버변수인 m_folder_list의 내용을 표시한다.
	int			set_folder_list(std::deque<CString>* lists = NULL, CString selected_text = _T(""));


	virtual		CColorListBox&	set_font(LOGFONT& lf);
	virtual		CColorListBox&	set_font_name(CString sFontname, BYTE byCharSet = DEFAULT_CHARSET);
	virtual		CColorListBox&	set_font_size(int nSize);
	virtual		CColorListBox&	set_font_bold(bool bBold = true);

	//color setting
	enum listctrlex_color_theme
	{
		color_theme_default = 0,
		color_theme_explorer,
		color_theme_popup_folder_list,
		//color_theme_navy_blue,
		//color_theme_dark_blue,
		//color_theme_dark_gray,
	};

	void		set_color_theme(int theme, bool apply_now = true);

	CShellImageList* m_pShellImageList = NULL;
	void		set_shell_imagelist(CShellImageList* pShellImageList) { m_pShellImageList = pShellImageList; }

	//편집 관련
	bool		get_use_edit() { return m_use_edit; }
	void		set_use_edit(bool use = true, bool readonly = false) { m_use_edit = use; m_edit_readonly = readonly; }
	//index == -1이면 선택된 첫번째 항목 편집
	void		edit(int index = -1);
	//modify가 true이면 편집된 텍스트로 변경, 그렇지 않으면 기존 텍스트 유지.
	void		edit_end(bool modify = true);

protected:
	//동적생성한 경우 GetParent등으로도 parent가 구해지지 않고 OnNotify()도 동작하지 않아서 수동으로 세팅하기 위함.
	HWND		m_hParentWnd = NULL;

	bool		m_use_over = false;			//hover hilighted
	BOOL		m_bOutside;
	int			m_over_item = -1;
	bool		m_as_popup = false;			//팝업모드로 동작하는 리스트박스일 경우는 killfocus이면 숨겨진다.
	bool		m_as_folder_list = false;	//폴더목록을 표시하는 목적으로 동작하는 경우
	std::deque<CString> m_folder_list;

	bool		m_show_log = true;
	bool		m_show_date = false;
	bool		m_show_time = true;
	bool		m_dim_time_str = true;		//시간 문자열은 연한 회색으로 비강조되도록 표시

	COLORREF	m_cr_text;					//기본 글자색
	COLORREF	m_cr_textSelected;			//선택 항목의 활성화(active) 글자색
	COLORREF	m_cr_textSelectedInactive;	//선택 항목의 비활성화(inactive) 글자색
	COLORREF	m_cr_textOver;
	COLORREF	m_cr_back;					//기본 배경색
	COLORREF	m_cr_backSelected;			//선택 항목 배경색
	COLORREF	m_cr_backSelectedRect;		//선택 항목 테두리(focus()가 있을 경우에만)
	COLORREF	m_cr_backSelectedInactive;
	COLORREF	m_cr_backOver;


	LOGFONT		m_lf;
	CFont		m_font;
	void		ReconstructFont();

	int			m_line_height;

	//편집 관련
	bool		m_use_edit = false;
	bool		m_in_editing = false;
	bool		m_edit_readonly = false;
	int			m_edit_index = -1;
	CEdit*		m_pEdit = NULL;

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


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CColorListBox)
	public:
	//afx_msg LRESULT OnSetFont(WPARAM wParam, LPARAM);
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CColorListBox();

	// Generated message map functions
protected:
	//{{AFX_MSG(CColorListBox)
	//}}AFX_MSG

	DECLARE_MESSAGE_MAP()
public:
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
	afx_msg void OnDrawItem(int nIDCtl, LPDRAWITEMSTRUCT lpDrawItemStruct);
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_COLORLISTBOX_H__5529A6B1_584A_11D2_A41A_006097BD277B__INCLUDED_)
