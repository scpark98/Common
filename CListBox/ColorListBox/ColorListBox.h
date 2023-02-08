#if !defined(AFX_COLORLISTBOX_H__5529A6B1_584A_11D2_A41A_006097BD277B__INCLUDED_)
#define AFX_COLORLISTBOX_H__5529A6B1_584A_11D2_A41A_006097BD277B__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#include <afxwin.h>
#include "../../Functions.h"

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

class CColorListBox : public CListBox
{
// Construction
public:
	CColorListBox();

// Attributes

// Operations
public:
	int			add_string(CString text, COLORREF crText = GetSysColor(COLOR_WINDOWTEXT), COLORREF crBack = COLOR_WINDOW);
	int			add_string(CString lpszItem, COLORREF rgb);					// Adds a colored string to the list box
	int			insert_string(int nIndex, CString lpszItem);					// Inserts a string to the list box
	int			insert_string(int nIndex, CString lpszItem, COLORREF rgb);	// Inserts a colored string to the list box
	void		set_item_color(int nIndex, COLORREF rgb);						// Sets the color of an item in the list box
	COLORREF	get_item_color(int nIndex);

	CSize		resizeToFit(bool bHori = true, bool bVert = true);			//변경된 크기를 리턴한다.
	void		set_minimum_lines(int lines) { m_nMinimumLines = lines; }

	void		UseColor(bool bUse = true) { m_bUseColor = bUse; }
	void		UseHover(bool bUse = true) { m_bUseHover = bUse; }
	int			get_hover_item() { return (m_bUseHover ? m_nHoverItem : -1); }

	void		set_back_color(COLORREF cr) { m_crBack = cr; Invalidate(); }

	int			GetGutterCharNumber() { return m_nGutterCharNumber; }
	void		SetGutterCharNumber(int chars) { m_nGutterCharNumber = chars; }

	virtual		CColorListBox&	set_font_name(CString sFontname, BYTE byCharSet = DEFAULT_CHARSET);
	virtual		CColorListBox&	set_font_size(int nSize);
	virtual		CColorListBox&	set_font_bold(bool bBold = true);

protected:
	bool		m_bUseColor;		//default = false;
	bool		m_bUseHover;		//default = false;
	BOOL		m_bOutside;
	int			m_nHoverItem;

	COLORREF	m_crBack;
	COLORREF	m_crBackSelected;

	LOGFONT		m_lf;
	CFont		m_font;
	void		ReconstructFont();

	//vert fit에서 최소 표시 라인수를 정해놓는다.default = -1(정하지 않을 경우)
	int			m_nMinimumLines;

	//
	//int			m_nItemHeight?


	//항목 왼쪽에 인덱스를 표시하는 gutter 영역 설정
	//단위는 픽셀이 아닌 문자 갯수임. default = 0;
	int			m_nGutterCharNumber;

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
	afx_msg BOOL OnLbnSelchange();
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_COLORLISTBOX_H__5529A6B1_584A_11D2_A41A_006097BD277B__INCLUDED_)
