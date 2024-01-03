// This file was created on March 21st 2001. By Robert Brault
// scpark. 20240103. EditTrans를 참조하여 transparent edit 기능 추가 예정.
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

	BOOL					set_read_only(BOOL bReadOnly = TRUE);

	virtual CColorEdit&		set_text_color(COLORREF crText); // This Function is to set the Color for the Text.
	virtual CColorEdit&		set_back_color(COLORREF crBack); // This Function is to set the BackGround Color for the Text and the Edit Box.
	virtual CColorEdit&		set_text_color_disabled(COLORREF cr_text_disabled);
	virtual CColorEdit&		set_back_color_disabled(COLORREF cr_back_disabled);
	virtual	CColorEdit&		set_font_name(LPCTSTR sFontname, BYTE byCharSet = DEFAULT_CHARSET);
	virtual CColorEdit&		set_font_size(int nSize);
	virtual CColorEdit&		set_font_bold(bool bBold = true);
	virtual CColorEdit&		set_auto_font_size(bool bAuto = true, double ratio = 0.6);	//resize font depending on control's height, not width.
	void					recalc_font_size();						//recalculate font height when control size is changed.
	int						get_font_size(bool pixel_size = false);

	// Generated message map functions
protected:
	COLORREF	m_cr_text;
	COLORREF	m_cr_back;
	COLORREF	m_cr_text_disabled;	//간혹 disabled일때 윈도우 기본 회색이 아닌 특정색으로 표현해야 할 필요가 있다.
	COLORREF	m_cr_back_disabled;	//간혹 disabled일때 윈도우 기본 회색이 아닌 특정색으로 표현해야 할 필요가 있다.
	CBrush		m_br_back;
	CBrush		m_br_back_disabled;

	LOGFONT		m_lf;
	CFont		m_font;
	int			m_font_size;
	bool		m_auto_resize_font;	//default = false
	double		m_auto_resize_ratio;
	int			m_default_height;
	void		reconstruct_font();
	void		update_ctrl();

	CRect		m_rect_NCbottom;
	CRect		m_rect_NCtop;

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
	afx_msg void OnEnKillfocus();
	afx_msg void OnEnUpdate();
	afx_msg void OnWindowPosChanged(WINDOWPOS* lpwndpos);
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_ColorEdit_H__E889B47D_AF6B_4066_B055_967508314A88__INCLUDED_)
