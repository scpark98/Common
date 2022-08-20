#if !defined(AFX_HEADERCTRLEX_H__821C8276_D91E_4EA5_8BAA_C30567625D84__INCLUDED_)
#define AFX_HEADERCTRLEX_H__821C8276_D91E_4EA5_8BAA_C30567625D84__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// HeaderCtrlEx.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CHeaderCtrlEx window

class CHeaderCtrlEx : public CHeaderCtrl
{
// Construction
public:
	CHeaderCtrlEx();
	CHeaderCtrlEx(CWnd* pWnd, void (CWnd::*fpDragCol)(int, int));


// Attributes
public:

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CHeaderCtrlEx)
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CHeaderCtrlEx();
	void SetCallback(CWnd* pWnd, void (CWnd::*fpDragCol)(int, int));

protected:
	BOOL	m_bCheckForDrag;
	BOOL	m_bDragging;
	int		*m_pWidth;
	int		m_nDragCol;
	int		m_nDropPos;
	CRect	marker_rect;
	void	(CWnd::*m_fpDragCol)(int, int);
	CWnd	*m_pOwnerWnd;


	// Generated message map functions
protected:
	//{{AFX_MSG(CHeaderCtrlEx)
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
	afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	//}}AFX_MSG

	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_HEADERCTRLEX_H__821C8276_D91E_4EA5_8BAA_C30567625D84__INCLUDED_)
