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
	m_label = label;
}

bool CSCUIElement::pt_in_rect(float x, float y)
{
	if (x >= m_r.X && x <= m_r.GetRight() && y >= m_r.Y && y <= m_r.GetBottom())
		return true;

	return false;
}

void CSCUIElement::copy(CSCUIElement* dst)
{
	dst->m_type = m_type;
	dst->m_r = m_r;
	dst->m_round = m_round;
	dst->m_label = m_label;
	dst->m_label_visible = m_label_visible;
	dst->m_label_align = m_label_align;
	dst->m_cr_stroke = m_cr_stroke;
	dst->m_cr_fill = m_cr_fill;
}