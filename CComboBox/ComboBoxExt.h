#pragma once

/*
- CComboBoxEx라는 클래스를 MFC에서 기본 제공하므로 CComboBoxExt로 파생.
- 리소스 속성에서 Has String, Owner Draw Fixed를 true로 변경.
*/

#include <afxwin.h>

// CColorComboBox

class CComboBoxExt : public CComboBox
{
	DECLARE_DYNAMIC(CComboBoxExt)

public:
	CComboBoxExt();
	virtual ~CComboBoxExt();

	COLORREF		m_crText;
	COLORREF		m_crBack;
	COLORREF		m_crHighlightText;
	COLORREF		m_crHighlightTextBack;

	void			SetTextColor( COLORREF crText ) { m_crText = crText; RedrawWindow(); }
	void			SetBackColor( COLORREF crBack ) { m_crBack = crBack; RedrawWindow(); }
	void			SetHighlightTextColor( COLORREF cr ) { m_crHighlightText = cr; RedrawWindow(); }
	void			SetHighlightTextBackColor( COLORREF cr ) { m_crHighlightTextBack = cr; RedrawWindow(); }

	void			set_font_name(LPCTSTR sFontname, BYTE byCharSet = DEFAULT_CHARSET);
	//-1 : reduce, +1 : enlarge
	void			set_font_size(int font_size);
	//void	enlarge_font_size(bool enlarge);
	//void	set_font_bold(bool bold = true);
	//void	set_font_italic(bool italic = true);
	//LOGFONT	get_log_font() { return m_lf; }
	//void	set_log_font(LOGFONT lf);

	void			load_history(CWinApp* app, CString section);
	void			save_history(CWinApp* app, CString section);

	//src내에 존재하는 콤보박스 아이템의 인덱스를 리턴.
	int				find_string(CString src);

protected:

	//폰트 관련
	LOGFONT			m_lf;
	CFont			m_font;
	int				m_font_size;
	void			reconstruct_font();

	DECLARE_MESSAGE_MAP()
public:
	virtual void DrawItem(LPDRAWITEMSTRUCT /*lpDrawItemStruct*/);
	virtual void MeasureItem(LPMEASUREITEMSTRUCT /*lpMeasureItemStruct*/);
	virtual int CompareItem(LPCOMPAREITEMSTRUCT /*lpCompareItemStruct*/);
	//afx_msg void OnNcPaint();
	//afx_msg void OnPaint();
	//afx_msg void OnEditUpdate();
	afx_msg void OnSetFocus(CWnd* pOldWnd);
	afx_msg void OnKillFocus(CWnd* pNewWnd);
	afx_msg void OnCbnDropdown();
	virtual void PreSubclassWindow();
	afx_msg void OnPaint();
	afx_msg BOOL OnEraseBkgnd(CDC* pDC);
};


