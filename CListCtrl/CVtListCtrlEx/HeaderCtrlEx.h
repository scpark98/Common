#if !defined(AFX_SKINHEADERCTRL_H__8B0847B1_B4E6_4372_A62D_038582FFEA5C__INCLUDED_)
#define AFX_SKINHEADERCTRL_H__8B0847B1_B4E6_4372_A62D_038582FFEA5C__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// SkinHeaderCtrl.h : header file
//

#include <deque>
#include <Afxwin.h>
#include <afxheaderctrl.h>

#include <gdiplus.h>


/////////////////////////////////////////////////////////////////////////////
// CHeaderCtrlEx window

class CHeaderCtrlEx : public CMFCHeaderCtrl
{
// Construction
public:
	CHeaderCtrlEx();

// Attributes
public:
	Gdiplus::Color	m_cr_text;
	Gdiplus::Color	m_cr_back;
	Gdiplus::Color	m_cr_separator;

	void			set_color(Gdiplus::Color cr_text, Gdiplus::Color cr_back, Gdiplus::Color cr_separator = Gdiplus::Color::Transparent);
	void			set_text_color(Gdiplus::Color cr_text);
	void			set_back_color(Gdiplus::Color cr_back);

	bool			m_header_is_clicked = false;
	int				m_header_clicked_index = -1;
	int				get_clicked_header(CPoint point);

	//m_cr_sort_arrow 기본값은 DarkGray이며 이 값을 변경하려면 Transparent가 아닌 다른색을, 변경하지 않을 경우는 해당 파라미터를 생략하면 된다.
	void			set_sort_arrow(int column, bool sort_asc, Gdiplus::Color cr_sort_arrow = Gdiplus::Color::Transparent);
	void			allow_sort(bool allow) { m_allow_sort = allow; }

	//pt가 separator 위치인지 판별
	bool			is_separator(CPoint pt);

// Operations
	CString			get_header_text(int column);
	void			set_header_text(int column, CString sText);

//align.
	//리스트컨트롤과는 별도로 설정할 수 있도록 한다. 기본값 = HDF_LEFT
	std::deque<int> m_header_text_align;
	int				get_header_text_align(int column);
	void			set_header_text_align(int column, int format = HDF_LEFT);

	int				m_header_height;
	int				get_header_height();
	void			set_header_height(int height);

	bool			m_flat_style = true;
	void			set_header_flat_style(bool flat = true);

	void			use_header_separator(bool use) { m_use_header_separator = use; }

//폰트 관련
	void			set_font(LOGFONT* lf);
	void			set_font_name(LPCTSTR sFontname, BYTE byCharSet = DEFAULT_CHARSET);
	void			set_font_bold(int weight = FW_BOLD);
	void			set_font_italic(bool italic = true);


protected:
	HDLAYOUT		m_HDLayout;
	bool			m_use_header_separator = true;

	bool			m_allow_sort = true;
	int				m_cur_sort_column = -1;
	bool			m_cur_sort_asc = true;
	Gdiplus::Color	m_cr_sort_arrow = Gdiplus::Color::DarkGray;

//폰트 관련
	LOGFONT			m_lf;
	CFont			m_font;
	void			reconstruct_font();

public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CHeaderCtrlEx)
	//}}AFX_VIRTUAL

// Implementation
public:
	//virtual void DrawItem(LPDRAWITEMSTRUCT lpDrawItemStruct);
	virtual ~CHeaderCtrlEx();

	// Generated message map functions
protected:
	afx_msg LRESULT OnLayout(WPARAM wparam, LPARAM lparam);

protected:
	//{{AFX_MSG(CHeaderCtrlEx)
	afx_msg void OnPaint();
	afx_msg BOOL OnEraseBkgnd(CDC* pDC);
	//}}AFX_MSG

	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_SKINHEADERCTRL_H__8B0847B1_B4E6_4372_A62D_038582FFEA5C__INCLUDED_)
