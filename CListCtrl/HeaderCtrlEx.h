#if !defined(AFX_SKINHEADERCTRL_H__8B0847B1_B4E6_4372_A62D_038582FFEA5C__INCLUDED_)
#define AFX_SKINHEADERCTRL_H__8B0847B1_B4E6_4372_A62D_038582FFEA5C__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// SkinHeaderCtrl.h : header file
//

#include <afxwin.h>
#include <afxheaderctrl.h>

/////////////////////////////////////////////////////////////////////////////
// CHeaderCtrlEx window

class CHeaderCtrlEx : public CMFCHeaderCtrl
{
// Construction
public:
	CHeaderCtrlEx();

// Attributes
public:
	COLORREF		m_crText;
	COLORREF		m_crBack;
	void			SetColor( COLORREF crText, COLORREF crBack ) { m_crText = crText; m_crBack = crBack; Invalidate(); }
	void			SetTextColor( COLORREF crText ) { m_crText = crText; Invalidate(); }
	void			SetBackColor( COLORREF crBack ) { m_crBack = crBack; Invalidate(); }

	bool			m_bColumnClicked;
	int				m_nColumnClicked;
	int				GetClickedColumn( CPoint point );

// Operations
	CString			GetColumnText( int nColumn );
	void			SetColumnText( int nColumn, CString sText );
	int				GetColumnTextAlign( int nColumn );
	void			SetColumnTextAlign( int nColumn, int format );

public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CHeaderCtrlEx)
	//}}AFX_VIRTUAL

// Implementation
public:
	//virtual void DrawItem(LPDRAWITEMSTRUCT lpDrawItemStruct);
	virtual ~CHeaderCtrlEx();

	// Generated message map functions
protected:
	//{{AFX_MSG(CHeaderCtrlEx)
	afx_msg void OnPaint();
	afx_msg BOOL OnEraseBkgnd(CDC* pDC);
	//}}AFX_MSG

	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_SKINHEADERCTRL_H__8B0847B1_B4E6_4372_A62D_038582FFEA5C__INCLUDED_)
