#if !defined(AFX_XPGROUPBOX_H__F70D755B_9C4B_4F4A_A1FB_AFF720C29717__INCLUDED_)
#define AFX_XPGROUPBOX_H__F70D755B_9C4B_4F4A_A1FB_AFF720C29717__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// XPGroupBox.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CXPGroupBox window

class CXPGroupBox : public CButton
{
	DECLARE_DYNAMIC(CXPGroupBox);

	// Construction
public:
	CXPGroupBox();

	enum XPGroupBoxStyle
	{ XPGB_FRAME,  
	XPGB_WINDOW};

	// Attributes
public:

	// Operations
public:
	virtual CXPGroupBox& SetBaseColor(COLORREF clrBase);
	virtual CXPGroupBox& SetBorderColor(COLORREF clrBorder);
	virtual CXPGroupBox& SetTitleTextColor(COLORREF clrText);
	virtual CXPGroupBox& SetBackgroundColor(COLORREF clrBKClient);
	virtual CXPGroupBox& SetBackgroundColor(COLORREF clrBKTitle,  COLORREF clrBKClient);
	virtual CXPGroupBox& SetXPGroupStyle(XPGroupBoxStyle eStyle); 

	virtual CXPGroupBox& SetText(LPCTSTR lpszTitle);
	virtual CXPGroupBox& SetFontBold(BOOL bBold);
	virtual CXPGroupBox& SetFontName(const CString& strFont, BYTE byCharSet = ANSI_CHARSET);
	virtual CXPGroupBox& SetFontUnderline(BOOL bSet);
	virtual CXPGroupBox& SetFontItalic(BOOL bSet);
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
	CString		m_strTitle;
	CRect		m_rectTitle;

	COLORREF	m_crBase;		//타이틀이 roundrect라서 양쪽 모퉁이의 색이 그려지지 않게 된다. parent배경색으로 칠해줘야 roundrect가 온전히 표시된다.
	COLORREF	m_clrBorder;
	COLORREF	m_crTitleBack;
	COLORREF	m_crClientBack;
	COLORREF	m_crTitleText;

	XPGroupBoxStyle		m_nType;
	DWORD       m_dwAlignment;  

	LOGFONT			m_lf;
	CFont			m_font;

};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_XPGROUPBOX_H__F70D755B_9C4B_4F4A_A1FB_AFF720C29717__INCLUDED_)
