/*****************************************************************************
* CColumnTreeCtrl
* Version: 1.1 
* Date: February 18, 2008
* Author: Oleg A. Krivtsov
* E-mail: olegkrivtsov@mail.ru
* Based on ideas implemented in Michal Mecinski's CColumnTreeCtrl class 
* (see copyright note below).
*
*****************************************************************************/

/*********************************************************
* Multi-Column Tree View
* Version: 1.1
* Date: October 22, 2003
* Author: Michal Mecinski
* E-mail: mimec@mimec.w.pl
* WWW: http://www.mimec.w.pl
*
* You may freely use and modify this code, but don't remove
* this copyright note.
*
* There is no warranty of any kind, express or implied, for this class.
* The author does not take the responsibility for any damage
* resulting from the use of it.
*
* Let me know if you find this code useful, and
* send me any modifications and bug reports.
*
* Copyright (C) 2003 by Michal Mecinski
*********************************************************/
#pragma once

#define MSG_COLUMNTREECTRL_CHECKED_CHANGED	WM_APP + 8273
//#define _OWNER_DRAWN_TREE  // comment this line if you want to use standard drawing code

// #ifdef _OWNER_DRAWN_TREE
// #ifndef IDB_TREEBTNS
// 	#error You should insert IDB_TREEBTNS bitmap to project resources. See control documentation for more info.
// #endif //IDB_TREEBTNS
// #endif //_OWNER_DRAWN_TREE

typedef struct _CTVHITTESTINFO { 
  POINT pt; 
  UINT flags; 
  HTREEITEM hItem; 
  int iSubItem;
} CTVHITTESTINFO;


class CCustomTreeChildCtrl : public CTreeCtrl
{
	friend class CColumnTreeCtrl;

	DECLARE_DYNAMIC(CCustomTreeChildCtrl)

public:

	/*
	 *  Construction/destruction
	 */
	
	CCustomTreeChildCtrl();
	virtual ~CCustomTreeChildCtrl();

	//Tab indentation을 이용한 저장방식(checkAtRoot:check tree일 경우 root에도 checkbox를 표시할 지)
	void		LoadSaveTreeTabData( BOOL bLoad, CString sFileName, bool checkAtRoot = false );
	int			GetIndentLevel( HTREEITEM hItem );
	HTREEITEM	GetNextItem( HTREEITEM hItem );

	//아래 두 FindData는 트리 형태에 따라 결과가 다른지 Elysium프로젝트에 사용하니 StackOverflow가 발생함. 우선 제거.
	//HTREEITEM	FindData(HTREEITEM hti, DWORD dwData, BOOL bAutoSelect = TRUE );
	//HTREEITEM	FindData(HTREEITEM hti, CString sData, BOOL bAutoSelect = TRUE );

	//바로 하위의 자식노드들중에 해당 문자열 항목을 찾는다. 비재귀호출. 다음 하위는 안찾음.
	HTREEITEM	FindChildItem( HTREEITEM hti, CString sString );
	
	void		SetTreeButtonImage( UINT nButtonID );
	BOOL		GetBkImage(LVBKIMAGE* plvbkImage) const;
	BOOL		SetBkImage(LVBKIMAGE* plvbkImage);

	//findText가 ""이면 모든 노드를 순회해서 check가 0또는 1이라면 체크 변경.
	//findText가 ""이 아니면 해당 아이템만 찾아서 check.
	//단, check가 0또는 1이 아니라면 단순히 findText의 노드를 찾고자 하는 목적으로 이용된다.
	HTREEITEM	FindItem(const CString& findText, int check = -1);
	HTREEITEM	FindItem(HTREEITEM hRoot, const CString& findText, int check = -1);

	/*
	enum
	{
		tree_find = 0,
		tree_check,
		tree_uncheck
	};

	void		Traverse(HTREEITEM hItem);//, CString itemText, );
	void		SetCheckAll(bool bCheck = true);
	*/

protected:
	DECLARE_MESSAGE_MAP()

	int			m_nFirstColumnWidth; // the width of the first column 
	int			m_nOffsetX;      	 // offset of this window inside the parent 
	LVBKIMAGE	m_bkImage;			// information about background image
	CImageList	m_imgBtns;			// tree buttons images (IDB_TREEBTNS)

	BOOL		CheckHit(CPoint point);

	
	/*
	 * Message Handlers
	 */

	afx_msg void OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags);
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnLButtonDblClk(UINT nFlags, CPoint point);
	afx_msg void OnPaint();
	afx_msg void OnTimer(UINT_PTR nIDEvent);
	afx_msg BOOL OnEraseBkgnd(CDC* pDC);
	afx_msg void OnTvnSelchanged(NMHDR *pNMHDR, LRESULT *pResult);
	BOOL OnToolTipNeedText( UINT id, NMHDR * pTTTStruct, LRESULT * pResult );

	/*
	 * Custom drawing related methods
	 */

#ifdef _OWNER_DRAWN_TREE
	LRESULT CustomDrawNotify(LPNMTVCUSTOMDRAW lpnm);
	LRESULT OwnerDraw(CDC* pDC);
	int OnMouseWheel(UINT nFlags, short zDelta, CPoint pt);
	void OnVScroll(UINT nSBCode, UINT nPos, CScrollBar *pScrollBar);
#endif //_OWNER_DRAWN_TREE

};


class CColumnTreeCtrl : public CStatic
{
public:
	DECLARE_DYNCREATE(CColumnTreeCtrl)
	
	/*
	 * Construction/destruction
	 */
	 
	CColumnTreeCtrl();
	virtual ~CColumnTreeCtrl();

	// explicit construction 
	BOOL Create(DWORD dwStyle , const RECT& rect, CWnd* pParentWnd, UINT nID);

	virtual void PreSubclassWindow();

		/*
	 *  Operations
	 */

	virtual void AssertValid( ) const;

	CCustomTreeChildCtrl& GetTreeCtrl() { return m_Tree; }
	CHeaderCtrl& GetHeaderCtrl() { return m_Header; }

	int InsertColumn(int nCol,LPCTSTR lpszColumnHeading, int nFormat=0, int nWidth=-1, int nSubItem=-1);
	BOOL DeleteColumn(int nCol);

	void SetFirstColumnMinWidth(UINT uMinWidth);
		
	CString GetItemText(HTREEITEM hItem, int nSubItem);
	void SetItemText(HTREEITEM hItem, int nSubItem, LPCTSTR lpszText);

	HTREEITEM HitTest(CPoint pt, UINT* pFlags=NULL) const;
	HTREEITEM HitTest(CTVHITTESTINFO* pHitTestInfo) const;
	
protected:
	
	DECLARE_MESSAGE_MAP()

	enum ChildrenIDs { HeaderID = 1, TreeID = 2, HScrollID = 3, Header2ID = 4};
	
	CCustomTreeChildCtrl m_Tree;
	CScrollBar m_horScroll;
	CHeaderCtrl m_Header;
	CHeaderCtrl m_Header2;
	
	int m_cyHeader;
	int m_cxTotal;
	int m_xPos;
	int m_xOffset;
	int m_uMinFirstColWidth;
	BOOL m_bHeaderChangesBlocked;

	enum{MAX_COLUMN_COUNT=16}; // change this value if you need more than 16 columns

	int m_arrColWidths[MAX_COLUMN_COUNT];
	DWORD m_arrColFormats[MAX_COLUMN_COUNT];
	
	virtual void Initialize();
	void UpdateColumns();
	void RepositionControls();


	virtual void OnDraw(CDC* pDC) {}
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnPaint();
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
	afx_msg void OnHeaderItemChanging(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnHeaderItemChanged(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnTreeCustomDraw(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnCancelMode();
	afx_msg void OnEnable(BOOL bEnable);
	afx_msg void OnSettingChange(UINT uFlags, LPCTSTR lpszSection);
	afx_msg BOOL OnNotify(WPARAM wParam, LPARAM lParam, LRESULT *pResult);
	afx_msg BOOL OnEraseBkgnd(CDC* pDC);
};