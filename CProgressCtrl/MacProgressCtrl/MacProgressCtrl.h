#if !defined(AFX_MACPROGRESSCTRL_H__603BBF44_B19C_11D3_90FA_0020AFBC499D__INCLUDED_)
#define AFX_MACPROGRESSCTRL_H__603BBF44_B19C_11D3_90FA_0020AFBC499D__INCLUDED_

#include <Afxwin.h>
#include <Afxdisp.h>
#include <afxext.h>         // MFC 확장입니다.
#include <afxcmn.h>             // Windows 공용 컨트롤에 대한 MFC 지원입니다.
#include <deque>
#include <gdiplus.h>

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

	bool		m_bLButtonDown;

	void		set_style(int style) { m_style = style; Invalidate(); }
	enum MACPROGRESS_STYLE
	{
		style_default = 0,
		style_round_line,
	};

	void		SetCtrlID(int nID) { m_nCtrlID = nID; }

	//true로 하면 CSliderCtrl과 같이 위치를 조정할 수 있다.
	void		use_slider(bool use = true) { m_use_slider = use; }

	int			get_lower() { return m_lower; }
	int			get_upper() { return m_upper; }
	BOOL		GetIndeterminate();
	void		SetIndeterminate(BOOL bIndeterminate = TRUE);
	void		SetColor(COLORREF crColor, COLORREF crBackColor = -1);
	void		set_back_color(COLORREF cr_back);
	void		set_back_track_color(COLORREF cr_back);
	void		SetTransparent(bool bTransparent = true) { m_bTransparent = bTransparent; Invalidate(); }
	void		SetGradient(bool bGradient = true) { m_bGradient = bGradient; Invalidate(); }
	void		SetText(CString sText);
	void		set_text_color(COLORREF crText0, COLORREF crText1 = -1);
	void		use_invert_text_color(bool use = true);
	void		use_text_shadow(bool shadow = true);
	void		draw_border(bool border = true, int width = 1, COLORREF cr = RGB(188, 188, 188), int pen_style = PS_SOLID);

	enum MAC_PROGRESS_TEXT_FORMAT
	{
		text_format_percent = 0,
		text_format_value,
	};

	void		show_text(bool show = true, int text_format = text_format_percent) { m_text_show = show; m_text_format = text_format; }
	void		set_text_format(int text_format) { m_text_format = text_format; }
	
	COLORREF	GetColor(COLORREF crOrigin, int nOffset);
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
	int			m_style = style_default;
	int			m_nCtrlID;			//parent에 progress가 여러개 생성될 경우 메시지를 구분하기 위한 ID
	int			m_lower = 0;
	int			m_upper = 100;
	bool		m_use_slider;		//true이면 슬라이드 컨트롤과 같이 조정할 수 있고 콜백함수를 호출한다.
	bool		m_bDrawOutline;
	bool		m_text_show = false;
	int			m_text_format = text_format_percent;
	CString		m_sText;
	COLORREF	m_cr_text0;
	COLORREF	m_cr_text1;
	bool		m_use_invert_text_color = false;
	bool		m_text_shadow = false;

	bool		m_draw_border = false;
	int			m_border_width = 1;
	int			m_border_pen_style = PS_SOLID;
	COLORREF	m_border_color = RGB(188, 188, 188);

	bool		m_bGradient = false;


	bool		m_bIndeterminate;
	bool		m_indeterminate_forward = true;
	int			m_indeterminate_width = 120;
	std::deque<Gdiplus::Color> m_dq_cr_indeterminate;
	void		thread_indeterminate();

	bool		m_bTransparent;
	int			m_nIndOffset;
	void		DrawVerticalBar(CDC *pDC, const CRect rtrack);
	void		DrawHorizontalBar(CDC *pDC, const CRect rtrack);
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
	COLORREF	m_cr_track;
	COLORREF	m_cr_track_Light;
	COLORREF	m_cr_track_Lighter;
	COLORREF	m_cr_track_Lightest;
	COLORREF	m_cr_track_Dark;
	COLORREF	m_cr_track_Darker;
	COLORREF	m_crDkShadow;
	COLORREF	m_crShadow;
	COLORREF	m_crLiteShadow;
	COLORREF	m_cr_back;			//back color of client area
	COLORREF	m_cr_back_track;	//back color of inactive track

	bool		m_auto_hide;	//default : false
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_MACPROGRESSCTRL_H__603BBF44_B19C_11D3_90FA_0020AFBC499D__INCLUDED_)
