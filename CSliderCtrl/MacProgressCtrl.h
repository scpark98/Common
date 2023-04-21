#if !defined(AFX_MACPROGRESSCTRL_H__603BBF44_B19C_11D3_90FA_0020AFBC499D__INCLUDED_)
#define AFX_MACPROGRESSCTRL_H__603BBF44_B19C_11D3_90FA_0020AFBC499D__INCLUDED_

#include <Afxwin.h>
#include <Afxdisp.h>
#include <afxext.h>         // MFC 확장입니다.
#include <afxcmn.h>             // Windows 공용 컨트롤에 대한 MFC 지원입니다.

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// MacProgressCtrl.h : header file
//
//	CMacProgressCtrl class, version 1.0
//
//	Copyright (c) 1999 Paul M. Meidinger (pmmeidinger@yahoo.com)
//
// Feel free to modifiy and/or distribute this file, but
// do not remove this header.
//
// I would appreciate a notification of any bugs discovered or 
// improvements that could be made.
//
// This file is provided "as is" with no expressed or implied warranty.
//
//	History:
//		PMM	12/21/1999		Initial implementation.		

/////////////////////////////////////////////////////////////////////////////
// CMacProgressCtrl window


#define MESSAGE_MACPROGRESSCTRL_MOVED		WM_USER + 100

class CMacProgressCtrl : public CProgressCtrl
{
// Construction
public:
	CMacProgressCtrl();

// Attributes
public:

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CMacProgressCtrl)
	//}}AFX_VIRTUAL

// Implementation
public:
	bool		m_bLButtonDown;

	void		SetCtrlID( int nID ) { m_nCtrlID = nID; }
	void		UseSlider( bool bUseSlider = true ) { m_bUseSlider = bUseSlider; }
	int			GetRangeMin();
	int			GetRangeMax();
	BOOL		GetIndeterminate();
	void		SetIndeterminate(BOOL bIndeterminate = TRUE);
	void		SetColor(COLORREF crColor, COLORREF crBackColor = -1, BOOL bGradient = TRUE );
	void		SetTransparent( bool bTransparent = true ) { m_bTransparent = bTransparent; Invalidate(); }
	void		SetGradient( bool bGradient = true ) { m_bGradient = bGradient; Invalidate(); }
	void		SetText( CString sText );
	void		SetTextColor( COLORREF cTextColor );
	void		SetTextShadow( BOOL bShadow = TRUE );
	void		ShowPercent( BOOL bShow = TRUE )
				{
					m_bShowPercent = bShow;
					Invalidate( FALSE );
				}

	void		show_text(bool show = true) { m_show_text = show; m_bShowPercent = false; Invalidate(); }
	
	COLORREF	GetColor( COLORREF crOrigin, int nOffset );
	void		SetRange(int min, int max);
	void		SetRange32(int min, int max);
	void		SetPos(int pos);
	void		SetAutoHide(bool auto_hide) { m_auto_hide = auto_hide; }

	virtual		~CMacProgressCtrl();

	// Generated message map functions
protected:
	//{{AFX_MSG(CMacProgressCtrl)
	afx_msg void OnPaint();
	afx_msg void OnTimer(UINT_PTR nIDEvent);
	afx_msg BOOL OnEraseBkgnd(CDC* pDC);
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
	afx_msg void OnNcPaint();

	//}}AFX_MSG

	DECLARE_MESSAGE_MAP()
private:
	int			m_nCtrlID;			//parent에 progress가 여러개 생성될 경우 메시지를 구분하기 위한 ID
	int			m_lower;
	int			m_upper;
	bool		m_bUseSlider;		//true이면 슬라이드 컨트롤과 같이 조정할 수 있고 콜백함수를 호출한다.
	bool		m_bDrawOutline;
	BOOL		m_bShowPercent;
	bool		m_show_text;
	CString		m_sText;
	COLORREF	m_cTextColor;
	BOOL		m_bGradient;		//default = false
	BOOL		m_bTransparent;
	BOOL		m_bTextShadow;
	int			m_nIndOffset;
	BOOL		m_bIndeterminate;
	void		DrawVerticalBar(CDC *pDC, const CRect rect);
	void		DrawHorizontalBar(CDC *pDC, const CRect rect);
	void		DeletePens();
	void		CreatePens();
	CPen		m_penColor;
	CPen		m_penColorLight;
	CPen		m_penColorLighter;
	CPen		m_penColorDark;
	CPen		m_penColorDarker;
	CPen		m_penDkShadow;
	CPen		m_penShadow;
	CPen		m_penLiteShadow;
	void		GetColors();
	COLORREF	m_crColor;
	COLORREF	m_crColorLight;
	COLORREF	m_crColorLighter;
	COLORREF	m_crColorLightest;
	COLORREF	m_crColorDark;
	COLORREF	m_crColorDarker;
	COLORREF	m_crDkShadow;
	COLORREF	m_crShadow;
	COLORREF	m_crLiteShadow;
	COLORREF	m_crBackColor;

	bool		m_auto_hide;	//default : false
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_MACPROGRESSCTRL_H__603BBF44_B19C_11D3_90FA_0020AFBC499D__INCLUDED_)
