// This file was created on March 21st 2001. By Robert Brault
// scpark. 20240103.
// DimEditCtrl, EditTrans 기능 추가 진행 중...
//
#if !defined(AFX_SCEdit_H__E889B47D_AF6B_4066_B055_967508314A88__INCLUDED_)
#define AFX_SCEdit_H__E889B47D_AF6B_4066_B055_967508314A88__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// SCEdit.h : header file
//

#include <Afxwin.h>

/////////////////////////////////////////////////////////////////////////////
// CSCEdit window
//color, font

class CSCEdit : public CEdit
{
// Construction
public:
	CSCEdit();
	virtual ~CSCEdit();

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CSCEdit)
	//}}AFX_VIRTUAL

	bool					set_read_only(bool readonly = true);

	//Test_GdiButton 프로젝트에서는 CSCEdit이 투명하게 잘 표시되나
	//Test_SCThemeDialog 프로젝트에서는 투명하게 표시되지 않는다.
	//차이점은 전자의 경우 parent가 순수 CDialog를 상속받은 dlg이고
	//후자는 SCThemeDialog를 상속받은 dlg라는 점이다. 수정 필요.
	virtual CSCEdit&		set_transparent(bool transparent = true);

	virtual CSCEdit&		set_text_color(COLORREF crText); // This Function is to set the Color for the Text.
	virtual CSCEdit&		set_back_color(COLORREF crBack); // This Function is to set the BackGround Color for the Text and the Edit Box.
	//아직 set_text_color_disabled()는 효과가 적용되고 있지 않다. 수정 필요.
	virtual CSCEdit&		set_text_color_disabled(COLORREF cr_text_disabled);
	virtual CSCEdit&		set_back_color_disabled(COLORREF cr_back_disabled);

	virtual	CSCEdit&		set_font_name(LPCTSTR sFontname, BYTE byCharSet = DEFAULT_CHARSET);
	virtual CSCEdit&		set_font_size(int nSize);
	virtual CSCEdit&		set_font_bold(bool bBold = true);
	virtual CSCEdit&		set_auto_font_size(bool bAuto = true, double ratio = 0.6);	//resize font depending on control's height, not width.
	void					recalc_font_size();						//recalculate font height when control size is changed.
	int						get_font_size(bool pixel_size = false);

	// Generated message map functions
protected:
	bool		m_transparent = false;
	COLORREF	m_cr_text;
	COLORREF	m_cr_back;
	COLORREF	m_cr_text_disabled;	//배경은 변경되나 text색상은 COLOR_GREYTEXT로 고정된듯하다. 현재로는 변경 불가.
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

	//{{AFX_MSG(CSCEdit)
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
	//afx_msg HBRUSH OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor);
	afx_msg void OnEnKillfocus();
	afx_msg void OnEnUpdate();
	afx_msg void OnWindowPosChanged(WINDOWPOS* lpwndpos);
	afx_msg void OnEnSetfocus();
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_SCEdit_H__E889B47D_AF6B_4066_B055_967508314A88__INCLUDED_)
