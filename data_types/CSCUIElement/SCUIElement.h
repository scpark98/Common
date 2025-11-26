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

	void	copy(CSCUIElement* dst);


	int				m_type = 0;
	Gdiplus::RectF	m_r;
	float			m_round[4] = { 0.f, 0.f, 0.f, 0.f };

	CString			m_label;
	UINT			m_label_align = DT_CENTER | DT_VCENTER | DT_SINGLELINE;
	bool			m_label_visible = true;

	CString			m_font_name = _T("Arial");
	int				m_font_size = 10;
	int				m_font_weight = FW_NORMAL;
	Gdiplus::Color	m_cr_label = Gdiplus::Color::Black;
	Gdiplus::Color	m_cr_label_back = Gdiplus::Color::Transparent;

	float			m_stroke_thickness = 1.0f;
	Gdiplus::Color	m_cr_stroke = Gdiplus::Color::RoyalBlue;// LightGray;
	Gdiplus::Color	m_cr_fill = Gdiplus::Color::Transparent;// Transparent;

	bool			m_selected = false;

protected:
};
