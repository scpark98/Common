#if !defined(AFX_ITEM_H__80746B41_4825_4358_B071_0E999007ED0B__INCLUDED_)
#define AFX_ITEM_H__80746B41_4825_4358_B071_0E999007ED0B__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// Item.h : header file
//
//------------------------------------------------------------------------
// 아이템의 상태를 나타낸다.
//------------------------------------------------------------------------
#define ITEM_NORMAL		1
#define	ITEM_SELECTED	2
#define	ITEM_DELETED	3
#define ITEM_FOCUSED	4
//------------------------------------------------------------------------
/////////////////////////////////////////////////////////////////////////////
// CItem command target

class CItem
{
public:
	CItem();           
	virtual ~CItem();

protected:
	bool	m_bTitle;
	int		m_nStatus;

	CRect	m_rcItem;
	CString m_strTitle;

public:
	bool	GetTitleFlag();
	void	SetTitleFlag(bool bTitle);

	CString	GetTitle();
	void	SetTitle(CString strTitle);

	int		GetStatus();
	void	SetStatus( int nStatus );

	CRect	GetRect();
	void SetRect(CRect rcItem);

public:
	virtual void Draw(CDC * pDC);
	virtual void Draw(CDC * pDC , int nOffsetX , int nOffsetY );

public:
	virtual bool LButtonDblClk(CWnd* pWnd, UINT nFlags, CPoint point);
	virtual bool LButtonDown(CWnd * pWnd, UINT nFlags , CPoint point);
	virtual bool LButtonUp(CWnd * pWnd, UINT nFlags, CPoint point);
	virtual bool MouseMove(CWnd * pWnd, UINT nFlags, CPoint point);
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_ITEM_H__80746B41_4825_4358_B071_0E999007ED0B__INCLUDED_)
