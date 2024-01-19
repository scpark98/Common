/*===========================================================================
====                                                                     ====
====    File name           :  EditTrans.h                               ====
====    Creation date       :  7/10/2001                                 ====
====    Author(s)           :  Dany Cantin                               ====
====                                                                     ====
===========================================================================*/
#include <Afxwin.h>

#ifndef EDITTRANS_H
#define EDITTRANS_H

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#define MSG_MOUSE_EVENT		WM_APP + 100

/////////////////////////////////////////////////////////////////////////////
// CEditTrans window

#define TRANS_BACK -1

class CEditTrans : public CEdit
{
// Construction
public:
	CEditTrans();
	void	SetParenthWnd( HWND hWnd )
	{
		m_hParent = hWnd;
	}

// Attributes
private:
    COLORREF m_TextColor;
    COLORREF m_BackColor;
    CBrush   m_Brush;

	HWND		m_hParent;
	BOOL		m_bClicked;
	CPoint		m_ptClicked;
	
// Operations
public:
    void SetTextColor(COLORREF col) { m_TextColor = col;
                                      UpdateCtrl();      }
    void SetBackColor(COLORREF col) { m_BackColor = col;
                                      UpdateCtrl();      }
private:
// 	LOGFONT		m_lf;
// 	CFont		m_font;
// 	void		update_surface();
// 	void		ReconstructFont();

    void		UpdateCtrl();

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CEditTrans)
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CEditTrans();

	// Generated message map functions
protected:
	//{{AFX_MSG(CEditTrans)
	afx_msg HBRUSH CtlColor(CDC* pDC, UINT nCtlColor);
	afx_msg void OnUpdate();
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnKillfocus();
	afx_msg void OnWindowPosChanged(WINDOWPOS FAR* lpwndpos);
	afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
	//}}AFX_MSG

	DECLARE_MESSAGE_MAP()
};


#endif // EDITTRANS_H
