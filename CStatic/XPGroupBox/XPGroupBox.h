#if !defined(AFX_XPGROUPBOX_H__F70D755B_9C4B_4F4A_A1FB_AFF720C29717__INCLUDED_)
#define AFX_XPGROUPBOX_H__F70D755B_9C4B_4F4A_A1FB_AFF720C29717__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// XPGroupBox.h : header file
//

#include <afxwin.h>

/////////////////////////////////////////////////////////////////////////////
// CXPGroupBox window

class CXPGroupBox : public CButton
{
	DECLARE_DYNAMIC(CXPGroupBox);

	// Construction
public:
	CXPGroupBox();

	enum XPGroupBoxStyle
	{
		XPGB_FRAME,
		XPGB_WINDOW
	};

	// Attributes
public:

	// Operations
public:
	virtual CXPGroupBox& set_base_color(COLORREF cr_base);
	virtual CXPGroupBox& set_border_color(COLORREF cr_border);
	virtual CXPGroupBox& set_title_text_color(COLORREF cr_text);
	virtual CXPGroupBox& set_back_color(COLORREF cr_back_client);
	virtual CXPGroupBox& set_back_color(COLORREF cr_back_title,  COLORREF cr_back_client);
	virtual CXPGroupBox& set_XPGroup_style(XPGroupBoxStyle style); 

	virtual CXPGroupBox& set_text(LPCTSTR lpszTitle);
	virtual CXPGroupBox& set_font_bold(bool bold = true);
	virtual CXPGroupBox& set_font_name(const CString& font_name, BYTE byCharSet = DEFAULT_CHARSET);
	virtual CXPGroupBox& set_font_underline(bool underline);
	virtual CXPGroupBox& set_font_italic(bool italic);
	virtual CXPGroupBox& SetFontSize(int nSize);
	virtual CXPGroupBox& SetFont(LOGFONT lf);

	virtual CXPGroupBox& SetAlignment(DWORD dwType);


	// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CXPGroupBox)
public:
	virtual BOOL Create(LPCTSTR lpszClassName, LPCTSTR lpszWindowName, DWORD dwStyle, const RECT& rect, CWnd* pParentWnd, UINT nID, CCreateContext* pContext = NULL);
protected:
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
	virtual void PreSubclassWindow();
	//}}AFX_VIRTUAL

	// Implementation
public:
	virtual ~CXPGroupBox();

	// Generated message map functions
protected:
	void UpdateSurface();
	void ReconstructFont();
	//{{AFX_MSG(CXPGroupBox)
	afx_msg void OnPaint();
	//}}AFX_MSG

	DECLARE_MESSAGE_MAP()

private:
	CString			m_strTitle;
	CRect			m_rectTitle;

	COLORREF		m_crBase;		//타이틀이 roundrect라서 양쪽 모퉁이의 색이 그려지지 않게 된다. parent배경색으로 칠해줘야 roundrect가 온전히 표시된다.
	COLORREF		m_clrBorder = ::GetSysColor(COLOR_3DSHADOW);
	COLORREF		m_crTitleText = RGB(0, 0, 0);
	COLORREF		m_crTitleBack = RGB(249, 236, 192);
	COLORREF		m_crClientBack = RGB(255, 249, 228);

	XPGroupBoxStyle	m_nType = XPGB_FRAME;
	DWORD			m_dwAlignment = SS_LEFT;

	LOGFONT			m_lf;
	CFont			m_font;
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_XPGROUPBOX_H__F70D755B_9C4B_4F4A_A1FB_AFF720C29717__INCLUDED_)
