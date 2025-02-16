#if !defined(AFX_XPGROUPBOX_H__F70D755B_9C4B_4F4A_A1FB_AFF720C29717__INCLUDED_)
#define AFX_XPGROUPBOX_H__F70D755B_9C4B_4F4A_A1FB_AFF720C29717__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// XPGroupBox.h : header file
//

//2025.02.16 scpark COLORREF를 Gdiplus::Color로 타입은 변경했으나
//아직 실제 코드들을 완전히 변경하지 않은 상태임.

#include <afxwin.h>
#include "../../Functions.h"
#include "../../colors.h"

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
	virtual CXPGroupBox& set_base_color(Gdiplus::Color cr_base);
	virtual CXPGroupBox& set_border_color(Gdiplus::Color cr_border);
	virtual CXPGroupBox& set_title_text_color(Gdiplus::Color cr_text);
	virtual CXPGroupBox& set_back_color(Gdiplus::Color cr_back_client);
	virtual CXPGroupBox& set_back_color(Gdiplus::Color cr_back_title, Gdiplus::Color cr_back_client);
	virtual CXPGroupBox& set_XPGroup_style(XPGroupBoxStyle style); 

	virtual CXPGroupBox& set_text(LPCTSTR lpszTitle);
	virtual CXPGroupBox& set_font_bold(bool bold = true);
	virtual CXPGroupBox& set_font_name(const CString& font_name, BYTE byCharSet = DEFAULT_CHARSET);
	virtual CXPGroupBox& set_font_underline(bool underline);
	virtual CXPGroupBox& set_font_italic(bool italic);
	virtual CXPGroupBox& set_font_size(int nSize);
	virtual CXPGroupBox& set_font(LOGFONT lf);

	virtual CXPGroupBox& set_alignment(DWORD dwType);


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
	void update_surface();
	void reconstruct_font();
	//{{AFX_MSG(CXPGroupBox)
	afx_msg void OnPaint();
	//}}AFX_MSG

	DECLARE_MESSAGE_MAP()

private:
	CString			m_title;
	CRect			m_rt_title;

	Gdiplus::Color	m_crBase;		//타이틀이 roundrect라서 양쪽 모퉁이의 색이 그려지지 않게 된다. parent배경색으로 칠해줘야 roundrect가 온전히 표시된다.
	Gdiplus::Color	m_cr_border = Gdiplus::Color::Gray;// ::GetSysColor(COLOR_3DSHADOW);
	Gdiplus::Color	m_cr_title_text = gRGB(0, 0, 0);
	Gdiplus::Color	m_cr_title_back = gRGB(249, 236, 192);
	Gdiplus::Color	m_cr_client_back = gRGB(255, 249, 228);

	XPGroupBoxStyle	m_type = XPGB_FRAME;
	DWORD			m_alignment = SS_LEFT;

	LOGFONT			m_lf;
	CFont			m_font;
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_XPGROUPBOX_H__F70D755B_9C4B_4F4A_A1FB_AFF720C29717__INCLUDED_)
