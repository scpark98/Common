#pragma once

#include <afxwin.h>
#include <deque>
#include <gdiplus.h>

class CSCUIElement
{
public:
	CSCUIElement() {}
	CSCUIElement(float left, float top, float right, float bottom, CString label = _T(""));
	CSCUIElement(Gdiplus::RectF r, CString label = _T(""));
	CSCUIElement(CString label);

	~CSCUIElement() {}

	void	set(float left, float top, float right, float bottom, CString label = _T(""));
	void	setwh(float left, float top, float width, float height, CString label = _T(""));

	void	set(CRect r, CString label = _T(""));
	void	set(Gdiplus::RectF r, CString label = _T(""));

	bool	pt_in_rect(float x, float y);


	int				m_type = 0;
	Gdiplus::RectF	m_r;
	int				m_round = 0;
	CString			m_label;
	bool			m_label_visible = true;
	UINT			m_label_align = DT_CENTER | DT_VCENTER | DT_SINGLELINE;
	Gdiplus::Color	m_cr_stroke = Gdiplus::Color::RoyalBlue;// LightGray;
	Gdiplus::Color	m_cr_fill = Gdiplus::Color::Transparent;// Transparent;

	bool			m_selected = false;

protected:
};
