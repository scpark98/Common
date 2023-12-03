#if !defined(AFX_LISTCTRLEX_H__EBDEA0CC_AC57_45C7_AE82_C5EF410F6061__INCLUDED_)
#define AFX_LISTCTRLEX_H__EBDEA0CC_AC57_45C7_AE82_C5EF410F6061__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// ListCtrlEx.h : header file
//

#include <afxwin.h>
#include <vector>
#include <deque>
#include "HeaderCtrlEx.h"

enum MESSAGE_LISTCTRLEX
{
	MESSAGE_LISTCTRLEX_ITEM_CHANGED = WM_USER + 1287,
	MESSAGE_LISTCTRLEX_ITEM_ADDED,
};

enum STATE_LISTCTRLEX
{
	ITEM_UNSELECTED = 0,
	ITEM_SELECTED,
	ITEM_UNFOCUSED,
	ITEM_FOCUSED,
	ITEM_UNCHECKED,
	ITEM_CHECKED,
};

class CListItemState
{
public:
	CListItemState(int nItem, int nSubItem, int state)
	{
		iItem = nItem;
		iSubItem = nSubItem;
		nState = state;
	}

	int		iItem;
	int		iSubItem;
	int		nState;
};

#define USE_CUSTOMDRAW			1 //USE_CUSTOMDRAW = !DRAWITEM
/* custom draw를 써서 하면 checkbox도 자동으로 그려주고 모든 색상을 자유롭게 설정할 수 있는데
virtual list를 사용하니 OnGetdispinfoList와 순환 호출이 발생하여 로드가 상당히 증가된다.
custom draw에서 그냥 리턴하면 선택항목의 색상 외에는 모두 원하는 색으로 그려진다.
DrawItem을 이용하면 virtual list와 순환 호출도 발생하지 않고 모든 색을 표현할 수 있으나 checkbox도 직접 그려줘야 한다.
그려지는 속도 또한 다소 느린 느낌이다.
*/

#define MAX_COLUMN				50
#define SORT_NONE				0
//#define SORT_ASCENDING		1	//already defined in ShObjIdl.h
//#define SORT_DESCENDING		-1	//already defined in ShObjIdl.h

#define MAX_COLOR_THEME			4

//아이템의 글자색, 배경색을 DWORD_PTR로 SetItemData를 통해 저장해둔다.
//단, 유효하지 않은 색상일 경우는 기본 컬러를 사용하게 되는데
//이를 위해 사용하지 않을 색상 하나를 선정해서 사용한다.
//즉, 이 색상이면 색칠하지 않는다는 뜻이다.
#define LISTCTRLEX_UNUSED_COLOR	RGB(17,235,53)
class CListCtrlExItemColor
{
public:
	//dqColor는 column의 갯수 만큼 원소를 가지게 된다.
	//5번 컬럼의 색상을 지정하려면 dqColor는 5개의 크기로 확장되어야 하고
	//이 때 크기 확장을 위해 채워진 0~4는 그 색생값을 지정한게 아니므로 스킵되어야 한다.
	//
	CListCtrlExItemColor(COLORREF _text = LISTCTRLEX_UNUSED_COLOR, COLORREF _back = LISTCTRLEX_UNUSED_COLOR)
	{
		text = _text;
		back = _back;
	}

	DWORD	text;
	DWORD	back;
};

class CListCtrlExItemColorDeque
{
public:
	std::deque<CListCtrlExItemColor> dqColor;
};




/*
//scpark

[2014-11-26]
- SetItemData로 항목의 글자색 및 배경색을 모두 표현할 수 있도록 수정(RGB24를 RGB565로 축소하여 DWORD로 저장하고 사용시에 다시 2개의 DWORD로 분리하여 각각 글자색, 배경색으로 사용

[2014-12-02]
- RGB565로 색을 디더링하니 특히 흰색같은 경우 온전한 흰색으로 유지되지 않아 LISTCTRL_ITEM_COLOR 타입의 DWORD_PTR을 생성하여 SetItemData에 넘겨주도록 수정.
  따라서 아이템 삭제 및 OnDestroy에서 new로 할당받은 메모리를 delete 하도록 수정.
- 선택 항목의 색을 윈도우의 기본 UI와 동일하게 하고 Always Show Selection 옵션도 적용되도록 수정.
*/

/* 편집 관련 액션
- F2 또는 항목 LButtonDbClick : 편집 시작
- Enter, Escape : 편집 완료, 취소
- 편집 중 Tab : 다음 항목으로 편집 포커스 이동(Shift : 이전 항목으로 이동)
- 빈 공간 LButtonDbClick : 항목 추가
*/

/* 사용 방법
in resource editor에서 ListCtrl의 속성 :
	- View : Report (필수)
	- Edit Labels : false (더블클릭 시 자체 편집모드 지원)를 비롯해서 모든 옵션은 기본값으로 사용한다.


	int		i;
	//m_List.SetFontName("Consolas");
	//m_List.SetFontSize(14);

	m_List.SetHeadings("항목,50;상태,50;");

	for (i = 0; i < m_List.GetColumnCount(); i++)
		m_List.SetColumnTextAlign(i, LVCFMT_CENTER);

	//m_List.SetColumnWidth(0, 91);
	//m_List.SetColumnWidth(1, 142);
	m_List.LoadColumnWidth(&theApp);
	m_List.SetExtendedStyle(LVS_EX_GRIDLINES | LVS_EX_FULLROWSELECT | LVS_EX_FLATSB);

	int index = m_List.AddItem("Item0", NULL);
	m_List.SetItemText(index, 1, "1st column text");
	....

	//on app exit..(ex. CDialog::OnCancel())
	m_List.SaveColumnWidth(&theApp)
*/

/////////////////////////////////////////////////////////////////////////////
// CListCtrlEx window

class CListCtrlEx : public CListCtrl
{
// Construction
public:
	CListCtrlEx();

	CHeaderCtrlEx	m_HeaderCtrlEx;
	void			ModifyExtendedStyle(DWORD dwExStyle, bool bAdd);

// Attributes
public:

// Operations
public:

//item

	//컬럼의 수보다 적은 개수의 항목을 추가할때는 반드시 끝 인자는 NULL을 넣어줘야 한다.
	int		AddItem(LPCTSTR pszText, ...);
	//컬럼의 수보다 적은 개수의 항목을 추가할때는 반드시 끝 인자는 NULL을 넣어줘야 한다.
	int		AddItem(int nIndex, LPCTSTR pszText, ...);
	//컬럼의 수와 같거나 그 이하인 항목들이 separator로 구분되어 있다면 자동 파싱하여 리스트에 넣어준다.
	int		AddLineStringItem(CString sLine, TCHAR separator = '|');

	//컬럼의 이름 그대로 한 라인을 추가한다. 리스트 중간에 컬럼을 다시 표시하고자 할 때 유용하다.
	int		AddItemWithColumnText();
	int		AddItemWithColumnText(int iItem);

	//보통 0번 컬럼은 순번을 표시하는 용도로 많이 사용된다.
	//그런데 데이터가 추가 삭제되면서 그 번호가 뒤섞이게 된다.
	//단순 순번 번호라면 번호 순서대로 갱신시켜준다.
	void	RefreshDefaultIndexColumn();

	enum POPUP_MENU_ITEM
	{
		listctrlex_menu_add_prior = 1,			//간혹 명시하지 않고 묵시적으로 0부터 시작하면 0번 명령이 동작하지 않는 경우가 있다. 기본 1부터 주자.
		listctrlex_menu_copy_prior,
		listctrlex_menu_add_next,
		listctrlex_menu_copy_next,
		listctrlex_menu_move_up,
		listctrlex_menu_move_down,
		listctrlex_menu_delete,
		listctrlex_menu_copy_clipboard,
		listctrlex_menu_copy_clipboard_head,
		listctrlex_menu_paste_insert,			//복사한 셀 삽입
		listctrlex_menu_checkall,
	};

	void	OnPopupMenu(UINT nMenuID);
	// 지우기 전 확인창은 호출루틴에서 처리해야 함
	void	DeleteSelectedItems();

	int		GetSelectedItem(int nStart = 0);
	int		GetLastSelectedItem();
	std::deque<int>	GetSelectedItems(int nStart = 0);

	//nIndex = -1 : 전체선택
	void	SelectItem(int nIndex, bool bSelect = true);

	void	SetItemDataReset(int start = -1, DWORD data = 0);

	//from ~ to 컬럼까지의 텍스트를 하나로 합쳐서 리턴해준다.
	CString GetText(int nIndex, int from = 0, int to = -1, CString sep = _T("|"));

	//0번 컬럼에서만 데이터를 찾는다.
	//int		FindString(CString str, int indexFrom = -1, bool bWholeWord = true);
	//column에 저장된 컬럼들에서 해당 문자열이 포함된 라인번호들을 찾는다.
	//column을 NULL로해서 호출하면 모든 컬럼을 모두 검색한다.
	//result가 NULL이면 맨 처음 발견된 인덱스를 리턴한다.
	int		FindString(CString str, int indexFrom = -1, std::vector<int> *column = NULL, std::vector<int> *result = NULL, bool bWholeWord = TRUE, bool bCaseSensitive = FALSE);

	bool	DeleteItem(int nItem);
	bool	DeleteAllItems();

	bool	m_bCheckAll;
	void	CheckAll(bool bCheck = true);
	int		GetCheckedItemCount();

//column

	bool	SetHeadings(UINT uiStringID);

	// ex. "No,20;Item1,50;Item2,50"
	bool	SetHeadings(const CString& strHeadings);	
	
	int		GetColumnDataType(int nColumn);
	void	SetColumnDataType(int nColumn, int nType);	

	int		GetColumnTextAlign(int nColumn);
	//format : HDF_LEFT ~ HDF_RIGHT
	void	SetColumnTextAlign(int nColumn, int format);
	void	SetColumnText(int nColumn, CString sText);
	CString	GetColumnText(int nColumn);
	int		GetColumnCount();
	void	DeleteColumn(int nColumn);				// -1 = delete all columns
	void	OnColumnClickFunction(NMHDR* pNMHDR, LRESULT* pResult);
	void	AutoAdjustColumnWidth();					// 데이터의 길이에 따라 컬럼너비 자동 조정
	void	LoadColumnWidth(CWinApp* pApp, CString sSection = _T("listctrl column width"));
	void	SaveColumnWidth(CWinApp* pApp, CString sSection = _T("listctrl column width"));
	bool	GetCellRect(int nRow, int nCol, CRect& rect);

//sort
	//0:no sort, 1:ascending, -1:descending
	bool	m_bAllowSort;			//default false
	void	AllowSort(bool bAllow = true) { m_bAllowSort = bAllow; }
	int		m_nSorted;
	bool	IsSorted();
	void	SetBitmapImageList(UINT nBitmapIDLarge, UINT nBitmapIDSmall, COLORREF cTransparent, int nInitial, int nGrow = 1);
	bool	SortTextItems(int nCol, bool bAscending, int low = 0, int high = -1);
	bool	SortNumericItems(int nCol, bool bAscending,int low = 0, int high = -1);
	int		GetItemImageIndex(int nRow, int nCol = 0);
	bool	SortInImageOrder(int nCol, bool bAscending, int nLow = 0, int nHigh = -1);
	bool	sort_in_text_color(int nSubItem, bool bAscending, int nLow = 0, int nHigh = -1);
	//void	SetSortArrow(int colIndex, bool ascending);

	//정렬방식 직접 지정
	void	Sort(int nColumn, bool bAscending);
	//현재 정렬방식 토글
	void	Sort(int nColumn);



	void	UsePopupMenu(bool bUse = true) { m_bUsePopupMenu = bUse; }
	bool	IsPopupMenuDisplayed() { return m_bPopupMenuDisplayed; }

//편집 관련
	void	AllowEdit(bool bAllowEdit = true) { m_bAllowEdit = bAllowEdit; }
	bool	IsInEditing()	{ return m_bInEditing; }	//편집중인지
	void	SetFlagInEditing(bool bInEditing) { m_bInEditing = bInEditing; }
	int		GetRecentEditItem() { return m_nEditItem; }
	int		GetRecentEditSubItem() { return m_nEditSubItem; }
	CString GetOldText() { return m_sOldText; }
	CEdit*	EditSubItem(int Item, int Column);
	void	UndoEditLabel();		//편집 전의 텍스트로 되돌린다.(편집 레이블이 파일명이고 파일명 변경이 실패한 경우 쓸 수 있다.)
	bool	IsModified() { return m_bModified; }
	void	ResetModifiedFlag() { m_bModified = false; }


	void	InvalidateItem(int nIndex, bool bErase = TRUE);	//특정 항목만 다시 그려주고자 할 때.
	void	SwapItem(int nIndex1, int nIndex2);
	void	MoveItem(int nFrom, int nTo);
	bool	MoveUp(int nIndex);
	bool	MoveDown(int nIndex);
	bool	MoveTop(int nIndex);
	bool	MoveBottom(int nIndex);
	void	SetTopIndex(int nIndex);	//원하는 항목을 맨 위에 위치하도록 스크롤

	bool	CopyToClipboard(LPCTSTR lpszSeparator = _T("|"), bool bHead = false);	//Ctrl+C키로 선택된 항목을 클립보드로 복사할 수 있다. shift를 조합하면 헤더까지 포함된다.
	void	PasteInsertFromClipboard();

	//파일에서 불러와서 리스트를 채운다. 컬럼의 수가 동일해야 한다.
	//또한 데이터의 첫번째 컬럼이 이미 인덱스 정보를 포함한 경우에는 add_index를 false로 해야 한다.
	bool	load_from_file(CString sfile, TCHAR separator = '|', bool match_column_count = true, bool reset_before_load = true, bool add_index = false);
	//리스트의 내용을 파일로 저장한다.
	bool	save_to_file(CString sfile, TCHAR separator = '|', bool includeHeader = false);

	enum COLOR_THEME
	{
		color_theme_default = 0,
		color_theme_blue,
		color_theme_dark_blue,
		color_theme_dark_gray,
	};
	void	SetColorTheme(int nTheme);

	//아이템 텍스트 및 배경색 설정 관련
	COLORREF get_text_color(int iItem, int iSubItem);
	void	set_text_color(int iItem, int iSubItem, COLORREF crText);					//특정 항목의 글자색 설정
	void	set_back_color(int iItem, int iSubItem, COLORREF crBack);					//특정 항목의 배경색 설정
	void	set_item_color(int iItem, int iSubItem, COLORREF crText, COLORREF crBack);
	void	SetDefaultItemColor(COLORREF crText, COLORREF crBack);	//기본 글자색, 배경색을 설정한다.
	void	use_data_item_color(bool bUse) { m_use_item_color = bUse; Invalidate(); }

	//32비트인 dw에 들어있는 R, G, B를 추출하여 16비트(5+6+5) 컬러로 리턴한다.
	WORD	RGB24ToRGB565(DWORD dw);
	//두 DWORD를 WORD로 변환하여 하나의 DWORD로 합쳐준다.
	DWORD	RGB24ToRGB565(DWORD rgb1, DWORD rgb2);
	//565로 변환된 WORD를 다시 24비트 RGB로 사용하기 위해 DWORD로 복원시킨다.
	DWORD	RGB565ToRGB24(WORD wd);

	int		HitTestEx(CPoint &point, int *pColumn);

	//폰트 관련
	int		GetFontSize() { return m_font_size; }
	void	SetFontSize(int font_size);
	void	EnlargeFontSize(bool enlarge);
	void	SetFontName(LPCTSTR sFontname, BYTE byCharSet = DEFAULT_CHARSET);
	void	SetFontBold(bool bBold = true);
	void	SetLineHeight(int height);

	bool	IsRowHighlighted(int row);
	void	EnableHighlighting(int row, bool enable);

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
	void	ensure_visible(int index, int mode = visible_first, int offset = 0);

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CListCtrlEx)
	protected:
#if !USE_CUSTOMDRAW
	virtual void DrawItem(LPDRAWITEMSTRUCT lpDrawItemStruct);
#endif
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CListCtrlEx();

	//데이터 타입을 명시해주고 정렬시에 사용한다.
	//모든 텍스트는 일반 문자열로 처리된다.
	//만약 어떤 컬럼의 모든 데이터가 숫자이고 숫자 크기로 정렬하고자 한다면
	//해당 컬럼의 DataType을 반드시 NUMERIC으로 설정해줘야 한다.
	enum
	{
		COLUMN_DATA_TYPE_UNKNOWN = 0,
		COLUMN_DATA_TYPE_TEXT,
		COLUMN_DATA_TYPE_NUMERIC,
	};

	// Generated message map functions
protected:
	CImageList		m_cImageListLarge, m_cImageListSmall;

	std::vector<int> m_vtDataType;


	bool			m_bUsePopupMenu;
	bool			m_bPopupMenuDisplayed;	//팝업 메뉴가 화면에 떠있는 상태인지 판별

	COLORREF		m_crText;						//기본 글자색 (SetItemData로 글자색이나 배경색을 특정색으로 설정하지 않은 경우에만 사용)
	COLORREF		m_crTextSelected;				//선택 항목의 활성화(active) 글자색
	COLORREF		m_crTextSelectedInactive;		//선택 항목의 비활성화(inactive) 글자색
	COLORREF		m_crBack;						//기본 배경색
	COLORREF		m_crBackSelected;
	COLORREF		m_crBackSelectedInactive;
	COLORREF		m_crHeaderBack;
	COLORREF		m_crHeaderText;
	bool			m_use_item_color;		//Data값은 글자색 및 배경색을 저장하고 있는 deque의 포인터를 가지고 있다.

//편집기능 관련
	bool			m_bAllowEdit;			//항목 편집이 가능한지... 기본 false
	bool			m_bModified;			//항목의 차례 또는 내용이 수정된 경우 true
	bool			m_bInEditing;			//편집중인지
	int				m_nEditItem;			//편집중인 아이템 인덱스
	int				m_nEditSubItem;			//편집중인 아이템 서브인덱스
	CString			m_sOldText;				//편집 전 텍스트
	CEdit*			m_pEditCtrl;			//

	//폰트 관련
	LOGFONT			m_lf;
	CFont			m_font;
	int				m_font_size;
	void			reconstruct_font();

	//OnLvnItemchangedList 핸들러를 parent에서 처리하도록 수정했으나
	//이 역시 unselect, select, unfocused, focus 등 중복된 호출이 발생한다.
	//따라서 단순히 선택하면 뭔가를 처리하는 핸들러를 parent에 넣었을 경우
	//항목 선택만 변경되도 몇번의 핸들러 함수 호출이 일어나게 된다.
	//그래서 선택시에 짧은 시간내에 중복된 이벤트 핸들러가 호출되면 스킵되도록 한다.
	//NMCLICK을 이용하면 이런 중복된 호출 문제는 없으나 


	//static int CALLBACK CompareFunction(LPARAM lParam1, LPARAM lParam2, LPARAM lParamData);
	//{{AFX_MSG(CListCtrlEx)
	//}}AFX_MSG
	//afx_msg LRESULT OnSetFont(WPARAM wParam, LPARAM);
	afx_msg void MeasureItem(LPMEASUREITEMSTRUCT lpMeasureItemStruct);
	//afx_msg void OnMeasureItem(int nIDCtl, LPMEASUREITEMSTRUCT lpMeasureItemStruct);
	afx_msg void OnCustomDraw(NMHDR* pNMHDR, LRESULT* pResult);
	DECLARE_MESSAGE_MAP()

public:
	afx_msg void OnLvnColumnclick(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg BOOL OnNMRClick(NMHDR *pNMHDR, LRESULT *pResult);
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	afx_msg BOOL OnLvnEndlabeledit(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnLButtonDblClk(UINT nFlags, CPoint point);
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
	afx_msg void OnVScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
	afx_msg void OnKillFocus(CWnd* pNewWnd);
	afx_msg LRESULT OnNcHitTest(CPoint point);
	afx_msg void OnPaint();
	afx_msg void OnDestroy();
	virtual void PreSubclassWindow();
	afx_msg BOOL OnEraseBkgnd(CDC* pDC);
	afx_msg BOOL OnLvnItemchanged(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnNcCalcSize(BOOL bCalcValidRects, NCCALCSIZE_PARAMS* lpncsp);
#if USE_CUSTOMDRAW
	afx_msg void OnNMCustomdraw(NMHDR *pNMHDR, LRESULT *pResult);
#endif
	afx_msg BOOL OnNMDblclk(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnNMClick(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnMeasureItem(int nIDCtl, LPMEASUREITEMSTRUCT lpMeasureItemStruct);
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_LISTCTRLEX_H__EBDEA0CC_AC57_45C7_AE82_C5EF410F6061__INCLUDED_)
