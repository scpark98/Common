#pragma once

#include <afxwin.h>
#include <deque>
#include <gdiplus.h>
#include "../../directx/CSCD2Image/SCD2Image.h"

class CSCUIElement
{
public:
	CSCUIElement() {}
	CSCUIElement(float left, float top, float right, float bottom, CString label = _T(""));
	CSCUIElement(Gdiplus::RectF r, CString label = _T(""));
	CSCUIElement(CString label);

	~CSCUIElement();

	void	set(float left, float top, float right, float bottom, CString label = _T(""));
	void	setwh(float left, float top, float width, float height, CString label = _T(""));

	void	set(CRect r, CString label = _T(""));
	void	set(Gdiplus::RectF r, CString label = _T(""));

	bool	pt_in_rect(float x, float y);

	void	copy(CSCUIElement* dst);

	//m_image_path가 유효한 경우 해당 이미지를 불러온다.
	HRESULT			load_image(IWICImagingFactory2* WICfactory, ID2D1DeviceContext* d2context);

	int				m_type = 0;
	Gdiplus::RectF	m_r;
	float			m_round[4] = { 0.f, 0.f, 0.f, 0.f };

	CString			m_text;
	CSCD2Image*		m_image = NULL;
	CString			m_image_path;
	int				m_text_align = DWRITE_TEXT_ALIGNMENT_CENTER;
	int				m_text_valign = DWRITE_PARAGRAPH_ALIGNMENT_CENTER;
	bool			m_text_visible = true;
	Gdiplus::Color	m_cr_text = Gdiplus::Color::Black;
	Gdiplus::Color	m_cr_back = Gdiplus::Color::Transparent;

	CString			m_font_name = _T("Arial");
	int				m_font_size = 10;
	int				m_font_weight = DWRITE_FONT_WEIGHT_BOLD;
	bool			m_font_italic = false;

	float			m_stroke_thickness = 1.0f;
	Gdiplus::Color	m_cr_stroke = Gdiplus::Color::RoyalBlue;// LightGray;
	Gdiplus::Color	m_cr_fill = Gdiplus::Color::Gray;// Transparent;

	bool			m_selected = false;

protected:
};
