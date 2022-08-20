#if !defined(AFX_STATICRANGE_H__A1021082_2739_4889_853C_30BC90C027A7__INCLUDED_)
#define AFX_STATICRANGE_H__A1021082_2739_4889_853C_30BC90C027A7__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// StaticRange.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CStaticRange window

class CStaticRange : public CStatic
{
// Construction
public:
	CStaticRange();

// Attributes
public:
	BOOL		bInitialized;
	int			m_nLower;//double		dMin;
	int			m_nUpper;//double		dMax;
	int			m_nSelLower;
	int			m_nSelUpper;
	BOOL		bLClicked;
	BOOL		bRClicked;

	COLORREF	ArrowColor;
	COLORREF	UnderColor;

	CImageList	m_ImageBack;
	
	void		SetBackImage( UINT nIDBack );
	
	void	SetRange( int nLower, int nUpper )
	{
		m_nLower = m_nSelLower = nLower;
		m_nUpper = m_nSelUpper = nUpper;
		bInitialized = TRUE;
	}

	void	SetSelRange( int nSelLower, int nSelUpper );
	int		GetSelLower() { return m_nSelLower; }
	int		GetSelUpper() { return m_nSelUpper; }

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CStaticRange)
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CStaticRange();

	// Generated message map functions
protected:
	//{{AFX_MSG(CStaticRange)
	afx_msg void OnPaint();
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnRButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnRButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnMButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnMButtonUp(UINT nFlags, CPoint point);
	//}}AFX_MSG

	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_STATICRANGE_H__A1021082_2739_4889_853C_30BC90C027A7__INCLUDED_)
