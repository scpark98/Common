/*|*\
|*|  File:      DimEditCtrl.cpp
|*|  
|*|  By:        James R. Twine, TransactionWorks, Inc.
|*|             Copyright 2000, TransactionWorks, inc.
|*|  Date:      Thursday, September 21, 2000
|*|             
|*|  Notes:     This Is The Implementation Of A "Dim Edit Control".
|*|             It Provides Visual Instructions Within The Edit
|*|             Control Itself.  It Can Be Used To Indicate Special
|*|             Properties Of A Edit Control Used On A Crowded
|*|             Interface
|*|             
|*|             May Be Freely Incorporated Into Projects Of Any Type
|*|             Subject To The Following Conditions:
|*|             
|*|             o This Header Must Remain In This File, And Any
|*|               Files Derived From It
|*|             o Do Not Misrepresent The Origin Of This Code
|*|               (IOW, Do Not Claim You Wrote It)
|*|             
|*|             A "Mention In The Credits", Or Similar Acknowledgement,
|*|             Is *NOT* Required.  It Would Be Nice, Though! :)
\*|*/
#if !defined(AFX_DIMEDITCTRL_H__CF8D88FB_6945_11D4_8AC4_00C04F6092F9__INCLUDED_)
#define AFX_DIMEDITCTRL_H__CF8D88FB_6945_11D4_8AC4_00C04F6092F9__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// DimEditCtrl.h : header file
//

#include <Afxwin.h>
//
//	This Specifies The Length Of The Dim Text Buffer...
//
static	const	int		DIM_TEXT_LEN =	128;				// Dim Text Buffer Length


/////////////////////////////////////////////////////////////////////////////
// CDimEditCtrl window

class CDimEditCtrl : public CEdit
{
// Construction
public:
	CDimEditCtrl();									// Constructor

// Attributes
public:

// Operations
public:

	void	SetTextColor( COLORREF crText ) { m_crText = crText; Invalidate(); }
	void	SetShowDimControl( bool bShow );				// Show Or Hide The Dim Control
	void	SetDimText( LPCTSTR cpText );					// Set The Dim Text
	void	SetDimColor( COLORREF crDColor );				// Set The Dim Color
	void	SetDimOffset( char cRedOS, char cGreenOS, 
					char cBlueOS );							// Set The Dim Color Offset

	void	SetAutoFontSize( bool bAuto );				//EditBox의 높이만큼 폰트크기가 자동 변경됨
	virtual CDimEditCtrl& SetFontBold( bool bBold = true );

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CDimEditCtrl)
	public:
	virtual BOOL Create(LPCTSTR lpszClassName, LPCTSTR lpszWindowName, DWORD dwStyle, const RECT& rect, CWnd* pParentWnd, UINT nID, CCreateContext* pContext = NULL);
	protected:
	virtual void PreSubclassWindow();
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CDimEditCtrl();

private:
	LOGFONT		m_lf;
	CFont		m_font;
	int			m_nDefaultHeight;
	void		UpdateSurface();
	void		ReconstructFont();

	// Generated message map functions
protected:
	//{{AFX_MSG(CDimEditCtrl)
	afx_msg BOOL OnChange();
	afx_msg BOOL OnSetfocus();
	afx_msg void OnPaint();
	afx_msg BOOL OnEraseBkgnd(CDC* pDC);
	afx_msg void OnSettingChange(UINT uFlags, LPCTSTR lpszSection);
	//}}AFX_MSG
//	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
//	afx_msg void OnLButtonDblClk(UINT nFlags, CPoint point);

	DECLARE_MESSAGE_MAP()

	void		DrawDimText();				// Draw The Dim Text

	COLORREF	m_crText;
	COLORREF	m_crDimTextColor;							// "Hard" Dim Text Color
	TCHAR		m_caDimText[ DIM_TEXT_LEN + 1 ];			// Dim Text Buffer
	bool		m_bShowDimText;								// Are We Showing The Dim Text?
	bool		m_bUseDimOffset;							// Are We Using The Offset Colors (Not Hard Color)?
	char		m_cRedOS;									// Red Color Dim Offset
	char		m_cGreenOS;									// Green Color Dim Offset
	char		m_cBlueOS;									// Blue Color Dim Offset
	int			m_iDimTextLen;								// Length Of Dim Text
	DWORD		m_dwStyle;
public:
	afx_msg BOOL OnEnKillfocus();
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_DIMEDITCTRL_H__CF8D88FB_6945_11D4_8AC4_00C04F6092F9__INCLUDED_)
