#include "SCUIElement.h"

CSCUIElement::CSCUIElement(float left, float top, float right, float bottom, CString label)
{
	set(Gdiplus::RectF(left, top, right - left, bottom - top), label);
}

CSCUIElement::CSCUIElement(Gdiplus::RectF r, CString label)
{
	set(r, label);
}

CSCUIElement::CSCUIElement(CString label)
{
	set(Gdiplus::RectF(0, 0, 0, 0), label);
}

void CSCUIElement::set(float left, float top, float right, float bottom, CString label)
{
	set(Gdiplus::RectF(left, top, right - left, bottom - top), label);
}

void CSCUIElement::setwh(float left, float top, float width, float height, CString label)
{
	set(Gdiplus::RectF(left, top, width, height), label);
}

void CSCUIElement::set(CRect r, CString label)
{
	set(Gdiplus::RectF(r.left, r.top, r.Width(), r.Height()), label);
}

void CSCUIElement::set(Gdiplus::RectF r, CString label)
{
	m_r = r;
	m_text = label;
}

bool CSCUIElement::pt_in_rect(float x, float y)
{
	float r = x <= m_r.GetRight();
	float b = x <= m_r.GetBottom();

	if ((x >= m_r.X) && (x <= m_r.GetRight()) && (y >= m_r.Y) && (y <= m_r.GetBottom()))
		return true;

	return false;
}

void CSCUIElement::copy(CSCUIElement* dst)
{
	dst->m_type = m_type;
	dst->m_r = m_r;
	memcpy(&dst->m_round, &m_round, sizeof(float) * 4);

	dst->m_text = m_text;
	dst->m_text_visible = m_text_visible;
	dst->m_text_align = m_text_align;
	dst->m_text_valign = m_text_valign;
	dst->m_cr_text = m_cr_text;
	dst->m_cr_back = m_cr_back;

	dst->m_font_name = m_font_name;
	dst->m_font_size = m_font_size;
	dst->m_font_weight = m_font_weight;

	dst->m_stroke_thickness = m_stroke_thickness;
	dst->m_cr_stroke = m_cr_stroke;
	dst->m_cr_fill = m_cr_fill;
}