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
/* custom draw�� �Ἥ �ϸ� checkbox�� �ڵ����� �׷��ְ� ��� ������ �����Ӱ� ������ �� �ִµ�
virtual list�� ����ϴ� OnGetdispinfoList�� ��ȯ ȣ���� �߻��Ͽ� �ε尡 ����� �����ȴ�.
custom draw���� �׳� �����ϸ� �����׸��� ���� �ܿ��� ��� ���ϴ� ������ �׷�����.
DrawItem�� �̿��ϸ� virtual list�� ��ȯ ȣ�⵵ �߻����� �ʰ� ��� ���� ǥ���� �� ������ checkbox�� ���� �׷���� �Ѵ�.
�׷����� �ӵ� ���� �ټ� ���� �����̴�.
*/

#define MAX_COLUMN				50
#define SORT_NONE				0
//#define SORT_ASCENDING		1	//already defined in ShObjIdl.h
//#define SORT_DESCENDING		-1	//already defined in ShObjIdl.h

#define MAX_COLOR_THEME			4

//�������� ���ڻ�, ������ DWORD_PTR�� SetItemData�� ���� �����صд�.
//��, ��ȿ���� ���� ������ ���� �⺻ �÷��� ����ϰ� �Ǵµ�
//�̸� ���� ������� ���� ���� �ϳ��� �����ؼ� ����Ѵ�.
//��, �� �����̸� ��ĥ���� �ʴ´ٴ� ���̴�.
#define LISTCTRLEX_UNUSED_COLOR	RGB(17,235,53)
class CListCtrlExItemColor
{
public:
	//dqColor�� column�� ���� ��ŭ ���Ҹ� ������ �ȴ�.
	//5�� �÷��� ������ �����Ϸ��� dqColor�� 5���� ũ��� Ȯ��Ǿ�� �ϰ�
	//�� �� ũ�� Ȯ���� ���� ä���� 0~4�� �� �������� �����Ѱ� �ƴϹǷ� ��ŵ�Ǿ�� �Ѵ�.
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
- SetItemData�� �׸��� ���ڻ� �� ������ ��� ǥ���� �� �ֵ��� ����(RGB24�� RGB565�� ����Ͽ� DWORD�� �����ϰ� ���ÿ� �ٽ� 2���� DWORD�� �и��Ͽ� ���� ���ڻ�, �������� ���

[2014-12-02]
- RGB565�� ���� ������ϴ� Ư�� ������� ��� ������ ������� �������� �ʾ� LISTCTRL_ITEM_COLOR Ÿ���� DWORD_PTR�� �����Ͽ� SetItemData�� �Ѱ��ֵ��� ����.
  ���� ������ ���� �� OnDestroy���� new�� �Ҵ���� �޸𸮸� delete �ϵ��� ����.
- ���� �׸��� ���� �������� �⺻ UI�� �����ϰ� �ϰ� Always Show Selection �ɼǵ� ����ǵ��� ����.
*/

/* ���� ���� �׼�
- F2 �Ǵ� �׸� LButtonDbClick : ���� ����
- Enter, Escape : ���� �Ϸ�, ���
- ���� �� Tab : ���� �׸����� ���� ��Ŀ�� �̵�(Shift : ���� �׸����� �̵�)
- �� ���� LButtonDbClick : �׸� �߰�
*/

/* ��� ���
in resource editor���� ListCtrl�� �Ӽ� :
	- View : Report (�ʼ�)
	- Edit Labels : false (����Ŭ�� �� ��ü ������� ����)�� ����ؼ� ��� �ɼ��� �⺻������ ����Ѵ�.


	int		i;
	//m_List.SetFontName("Consolas");
	//m_List.SetFontSize(14);

	m_List.SetHeadings("�׸�,50;����,50;");

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

	//�÷��� ������ ���� ������ �׸��� �߰��Ҷ��� �ݵ�� �� ���ڴ� NULL�� �־���� �Ѵ�.
	int		AddItem(LPCTSTR pszText, ...);
	//�÷��� ������ ���� ������ �׸��� �߰��Ҷ��� �ݵ�� �� ���ڴ� NULL�� �־���� �Ѵ�.
	int		AddItem(int nIndex, LPCTSTR pszText, ...);
	//�÷��� ���� ���ų� �� ������ �׸���� separator�� ���еǾ� �ִٸ� �ڵ� �Ľ��Ͽ� ����Ʈ�� �־��ش�.
	int		AddLineStringItem(CString sLine, TCHAR separator = '|');

	//�÷��� �̸� �״�� �� ������ �߰��Ѵ�. ����Ʈ �߰��� �÷��� �ٽ� ǥ���ϰ��� �� �� �����ϴ�.
	int		AddItemWithColumnText();
	int		AddItemWithColumnText(int iItem);

	//���� 0�� �÷��� ������ ǥ���ϴ� �뵵�� ���� ���ȴ�.
	//�׷��� �����Ͱ� �߰� �����Ǹ鼭 �� ��ȣ�� �ڼ��̰� �ȴ�.
	//�ܼ� ���� ��ȣ��� ��ȣ ������� ���Ž����ش�.
	void	RefreshDefaultIndexColumn();

	enum POPUP_MENU_ITEM
	{
		listctrlex_menu_add_prior = 1,			//��Ȥ ������� �ʰ� ���������� 0���� �����ϸ� 0�� ����� �������� �ʴ� ��찡 �ִ�. �⺻ 1���� ����.
		listctrlex_menu_copy_prior,
		listctrlex_menu_add_next,
		listctrlex_menu_copy_next,
		listctrlex_menu_move_up,
		listctrlex_menu_move_down,
		listctrlex_menu_delete,
		listctrlex_menu_copy_clipboard,
		listctrlex_menu_copy_clipboard_head,
		listctrlex_menu_paste_insert,			//������ �� ����
		listctrlex_menu_checkall,
	};

	void	OnPopupMenu(UINT nMenuID);
	// ����� �� Ȯ��â�� ȣ���ƾ���� ó���ؾ� ��
	void	DeleteSelectedItems();

	int		GetSelectedItem(int nStart = 0);
	int		GetLastSelectedItem();
	std::deque<int>	GetSelectedItems(int nStart = 0);

	//nIndex = -1 : ��ü����
	void	SelectItem(int nIndex, bool bSelect = true);

	void	SetItemDataReset(int start = -1, DWORD data = 0);

	//from ~ to �÷������� �ؽ�Ʈ�� �ϳ��� ���ļ� �������ش�.
	CString GetText(int nIndex, int from = 0, int to = -1, CString sep = _T("|"));

	//0�� �÷������� �����͸� ã�´�.
	//int		FindString(CString str, int indexFrom = -1, bool bWholeWord = true);
	//column�� ����� �÷��鿡�� �ش� ���ڿ��� ���Ե� ���ι�ȣ���� ã�´�.
	//column�� NULL���ؼ� ȣ���ϸ� ��� �÷��� ��� �˻��Ѵ�.
	//result�� NULL�̸� �� ó�� �߰ߵ� �ε����� �����Ѵ�.
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
	void	AutoAdjustColumnWidth();					// �������� ���̿� ���� �÷��ʺ� �ڵ� ����
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

	//���Ĺ�� ���� ����
	void	Sort(int nColumn, bool bAscending);
	//���� ���Ĺ�� ���
	void	Sort(int nColumn);



	void	UsePopupMenu(bool bUse = true) { m_bUsePopupMenu = bUse; }
	bool	IsPopupMenuDisplayed() { return m_bPopupMenuDisplayed; }

//���� ����
	void	AllowEdit(bool bAllowEdit = true) { m_bAllowEdit = bAllowEdit; }
	bool	IsInEditing()	{ return m_bInEditing; }	//����������
	void	SetFlagInEditing(bool bInEditing) { m_bInEditing = bInEditing; }
	int		GetRecentEditItem() { return m_nEditItem; }
	int		GetRecentEditSubItem() { return m_nEditSubItem; }
	CString GetOldText() { return m_sOldText; }
	CEdit*	EditSubItem(int Item, int Column);
	void	UndoEditLabel();		//���� ���� �ؽ�Ʈ�� �ǵ�����.(���� ���̺��� ���ϸ��̰� ���ϸ� ������ ������ ��� �� �� �ִ�.)
	bool	IsModified() { return m_bModified; }
	void	ResetModifiedFlag() { m_bModified = false; }


	void	InvalidateItem(int nIndex, bool bErase = TRUE);	//Ư�� �׸� �ٽ� �׷��ְ��� �� ��.
	void	SwapItem(int nIndex1, int nIndex2);
	void	MoveItem(int nFrom, int nTo);
	bool	MoveUp(int nIndex);
	bool	MoveDown(int nIndex);
	bool	MoveTop(int nIndex);
	bool	MoveBottom(int nIndex);
	void	SetTopIndex(int nIndex);	//���ϴ� �׸��� �� ���� ��ġ�ϵ��� ��ũ��

	bool	CopyToClipboard(LPCTSTR lpszSeparator = _T("|"), bool bHead = false);	//Ctrl+CŰ�� ���õ� �׸��� Ŭ������� ������ �� �ִ�. shift�� �����ϸ� ������� ���Եȴ�.
	void	PasteInsertFromClipboard();

	//���Ͽ��� �ҷ��ͼ� ����Ʈ�� ä���. �÷��� ���� �����ؾ� �Ѵ�.
	//���� �������� ù��° �÷��� �̹� �ε��� ������ ������ ��쿡�� add_index�� false�� �ؾ� �Ѵ�.
	bool	load_from_file(CString sfile, TCHAR separator = '|', bool match_column_count = true, bool reset_before_load = true, bool add_index = false);
	//����Ʈ�� ������ ���Ϸ� �����Ѵ�.
	bool	save_to_file(CString sfile, TCHAR separator = '|', bool includeHeader = false);

	enum COLOR_THEME
	{
		color_theme_default = 0,
		color_theme_blue,
		color_theme_dark_blue,
		color_theme_dark_gray,
	};
	void	SetColorTheme(int nTheme);

	//������ �ؽ�Ʈ �� ���� ���� ����
	COLORREF get_text_color(int iItem, int iSubItem);
	void	set_text_color(int iItem, int iSubItem, COLORREF crText);					//Ư�� �׸��� ���ڻ� ����
	void	set_back_color(int iItem, int iSubItem, COLORREF crBack);					//Ư�� �׸��� ���� ����
	void	set_item_color(int iItem, int iSubItem, COLORREF crText, COLORREF crBack);
	void	SetDefaultItemColor(COLORREF crText, COLORREF crBack);	//�⺻ ���ڻ�, ������ �����Ѵ�.
	void	use_data_item_color(bool bUse) { m_use_item_color = bUse; Invalidate(); }

	//32��Ʈ�� dw�� ����ִ� R, G, B�� �����Ͽ� 16��Ʈ(5+6+5) �÷��� �����Ѵ�.
	WORD	RGB24ToRGB565(DWORD dw);
	//�� DWORD�� WORD�� ��ȯ�Ͽ� �ϳ��� DWORD�� �����ش�.
	DWORD	RGB24ToRGB565(DWORD rgb1, DWORD rgb2);
	//565�� ��ȯ�� WORD�� �ٽ� 24��Ʈ RGB�� ����ϱ� ���� DWORD�� ������Ų��.
	DWORD	RGB565ToRGB24(WORD wd);

	int		HitTestEx(CPoint &point, int *pColumn);

	//��Ʈ ����
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
	//� �׸��� Ư�� ��ġ�� ǥ�õǵ��� ��ũ�ѽ�Ų��.
	//mode�� visible_first�̰� offset�� 3�̸� ������ 3+1�� ��ġ�� �ش� �������� ǥ�õǵ��� ��ũ�ѽ�Ų��.
	//mode�� visible_center�̰� offset�� 0�̸� �߾� ��ġ�� �ش� �������� ǥ�õǵ��� ��ũ�ѽ�Ų��.
	//mode�� visible_last�̰� offset�� 3�̸� �Ʒ����� -3-1�� ��ġ�� �ش� �������� ǥ�õǵ��� ��ũ�ѽ�Ų��.
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

	//������ Ÿ���� ������ְ� ���Ľÿ� ����Ѵ�.
	//��� �ؽ�Ʈ�� �Ϲ� ���ڿ��� ó���ȴ�.
	//���� � �÷��� ��� �����Ͱ� �����̰� ���� ũ��� �����ϰ��� �Ѵٸ�
	//�ش� �÷��� DataType�� �ݵ�� NUMERIC���� ��������� �Ѵ�.
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
	bool			m_bPopupMenuDisplayed;	//�˾� �޴��� ȭ�鿡 ���ִ� �������� �Ǻ�

	COLORREF		m_crText;						//�⺻ ���ڻ� (SetItemData�� ���ڻ��̳� ������ Ư�������� �������� ���� ��쿡�� ���)
	COLORREF		m_crTextSelected;				//���� �׸��� Ȱ��ȭ(active) ���ڻ�
	COLORREF		m_crTextSelectedInactive;		//���� �׸��� ��Ȱ��ȭ(inactive) ���ڻ�
	COLORREF		m_crBack;						//�⺻ ����
	COLORREF		m_crBackSelected;
	COLORREF		m_crBackSelectedInactive;
	COLORREF		m_crHeaderBack;
	COLORREF		m_crHeaderText;
	bool			m_use_item_color;		//Data���� ���ڻ� �� ������ �����ϰ� �ִ� deque�� �����͸� ������ �ִ�.

//������� ����
	bool			m_bAllowEdit;			//�׸� ������ ��������... �⺻ false
	bool			m_bModified;			//�׸��� ���� �Ǵ� ������ ������ ��� true
	bool			m_bInEditing;			//����������
	int				m_nEditItem;			//�������� ������ �ε���
	int				m_nEditSubItem;			//�������� ������ �����ε���
	CString			m_sOldText;				//���� �� �ؽ�Ʈ
	CEdit*			m_pEditCtrl;			//

	//��Ʈ ����
	LOGFONT			m_lf;
	CFont			m_font;
	int				m_font_size;
	void			reconstruct_font();

	//OnLvnItemchangedList �ڵ鷯�� parent���� ó���ϵ��� ����������
	//�� ���� unselect, select, unfocused, focus �� �ߺ��� ȣ���� �߻��Ѵ�.
	//���� �ܼ��� �����ϸ� ������ ó���ϴ� �ڵ鷯�� parent�� �־��� ���
	//�׸� ���ø� ����ǵ� ����� �ڵ鷯 �Լ� ȣ���� �Ͼ�� �ȴ�.
	//�׷��� ���ýÿ� ª�� �ð����� �ߺ��� �̺�Ʈ �ڵ鷯�� ȣ��Ǹ� ��ŵ�ǵ��� �Ѵ�.
	//NMCLICK�� �̿��ϸ� �̷� �ߺ��� ȣ�� ������ ������ 


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
