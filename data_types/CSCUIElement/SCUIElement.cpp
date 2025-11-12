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
