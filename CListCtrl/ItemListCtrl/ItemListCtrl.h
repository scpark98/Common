#if !defined(AFX_PHOTOLISTCTRL_H__15C82E64_0D7E_450E_BB9F_5B7F6CEE85FE__INCLUDED_)
#define AFX_PHOTOLISTCTRL_H__15C82E64_0D7E_450E_BB9F_5B7F6CEE85FE__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// PhotoListCtrl.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CItemListCtrl window
#include <afxtempl.h>

class CItem;

typedef CList<CItem *, CItem *> CItemList;

class CItemListCtrl : public CWnd
{
// Construction
public:
	CItemListCtrl();
	virtual ~CItemListCtrl();

// Attributes
protected:
	CItemList m_listItem;

protected:
	CItem * m_pSelectedItem;

protected:
	CPoint m_ptStart;
	CPoint m_ptEnd;

	HCURSOR m_hCursorHand;
	HCURSOR m_hCursorArrow;

	UINT m_nLButtonDown;
	UINT m_nLButtonDblClk;

public:
	void SetLButtonDblClk(UINT nMessageID);
	void SetLButtonDown( UINT nMessageID );
// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CItemListCtrl)
	//}}AFX_VIRTUAL

// Implementation
protected:
	int m_nViewHeight;

protected:
	int m_nVScrollPos;
	int m_nVPageSize;

protected:
	CBrush m_brBackground;

public:
	void SetBgColor( COLORREF clrBgColor );

public:
	void InitItemList();
	void InitItemPos();

protected:
	void InitScrollSize();
	void InitVirtualSpace(CRect rcClient);

public:
	CItemList * GetItemList();
	long	  GetItemCount();

public:
	CItem * GetClickItem( CPoint point );
	CItem * GetSelectedItem();

	void ReDraw();

protected:
	virtual int CalcVerticalSpace(CRect rcClient);
	virtual CRect CalcItemRect(int nItem);

public:
	virtual bool AddTail( CItem * pNewItem );
	virtual bool AddHead( CItem * pNewItem );

	virtual void DeleteAllItems();
	virtual bool DeleteItem( CItem * pDeleteItem );

protected:
	virtual int GetItemWidth() = 0;
	virtual int GetItemHeight() = 0;

	virtual int GetItemVGap() = 0;
	virtual int GetItemHGap() = 0;

	virtual int GetVGap() = 0;
	virtual int GetHGap() = 0;

public:
	virtual void Draw(CDC * pDC, CRect rcInvalid);
	
	// Generated message map functions
protected:
	//{{AFX_MSG(CItemListCtrl)
	afx_msg void OnPaint();
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg BOOL OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message);
	afx_msg void OnVScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnLButtonDblClk(UINT nFlags, CPoint point);
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
	afx_msg HBRUSH CtlColor(CDC* pDC, UINT nCtlColor);
	afx_msg BOOL OnMouseWheel(UINT nFlags, short zDelta, CPoint pt);
	//}}AFX_MSG

	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_PHOTOLISTCTRL_H__15C82E64_0D7E_450E_BB9F_5B7F6CEE85FE__INCLUDED_)
