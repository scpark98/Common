// This file was created on March 21st 2001. By Robert Brault
//
//
#if !defined(AFX_ColorEdit_H__E889B47D_AF6B_4066_B055_967508314A88__INCLUDED_)
#define AFX_ColorEdit_H__E889B47D_AF6B_4066_B055_967508314A88__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// ColorEdit.h : header file
//

#include <Afxwin.h>

/////////////////////////////////////////////////////////////////////////////
// CColorEdit window
//color, font

class CColorEdit : public CEdit
{
// Construction
public:
	CColorEdit();
	virtual ~CColorEdit();

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CColorEdit)
	//}}AFX_VIRTUAL

	BOOL					SetReadOnly(BOOL bReadOnly = TRUE);

	virtual CColorEdit&		SetTextColor(COLORREF crText); // This Function is to set the Color for the Text.
	virtual CColorEdit&		SetBackColor(COLORREF crBack); // This Function is to set the BackGround Color for the Text and the Edit Box.
	virtual	CColorEdit&		SetFontName(LPCTSTR sFontname, BYTE byCharSet = DEFAULT_CHARSET);
	virtual CColorEdit&		SetFontSize(int nSize);
	virtual CColorEdit&		SetFontBold(bool bBold = true);
	virtual CColorEdit&		SetAutoFontSize(bool bAuto = true, double ratio = 0.6);	//resize font depending on control's height, not width.
	void					RecalcFontSize();						//recalculate font height when control size is changed.
	int						get_font_size(bool pixel_size = false);

	// Generated message map functions
protected:
	CBrush		m_brBkgnd;		// Holds Brush Color for the Edit Box
	COLORREF	m_crBack;	// Holds the Background Color for the Text
	COLORREF	m_crText;	// Holds the Color for the Text

	LOGFONT		m_lf;
	CFont		m_font;
	int			m_font_size;
	bool		m_bAutoResizeFont;	//default = false
	double		m_auto_resize_ratio;
	int			m_nDefaultHeight;
	void		ReconstructFont();

	CRect		m_rectNCBottom;
	CRect		m_rectNCTop;

	//{{AFX_MSG(CColorEdit)
	afx_msg HBRUSH CtlColor(CDC* pDC, UINT nCtlColor); // This Function Gets Called Every Time Your Window Gets Redrawn.
	//}}AFX_MSG

	DECLARE_MESSAGE_MAP()
	virtual void PreSubclassWindow();
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	afx_msg void OnSize(UINT nType, int cx, int cy);
	//afx_msg void OnWindowPosChanged(WINDOWPOS* lpwndpos);
public:
	afx_msg void OnPaint();
	afx_msg void OnNcCalcSize(BOOL bCalcValidRects, NCCALCSIZE_PARAMS* lpncsp);
	afx_msg void OnNcPaint();
	afx_msg HBRUSH OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor);
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_ColorEdit_H__E889B47D_AF6B_4066_B055_967508314A88__INCLUDED_)
