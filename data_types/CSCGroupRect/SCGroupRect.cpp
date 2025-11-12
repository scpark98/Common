#include "SCGroupRect.h"

CSCGroupRect::CSCGroupRect(float left, float top, float right, float bottom)
{
	clear();
	add(Gdiplus::RectF(left, top, right - left, bottom - top));
}

void CSCGroupRect::add(float left, float top, float right, float bottom)
{
	add(Gdiplus::RectF(left, top, right - left, bottom - top));
}

void CSCGroupRect::addwh(float left, float top, float width, float height)
{
	add(Gdiplus::RectF(left, top, width, height));
}

void CSCGroupRect::add(CRect r)
{
	add(Gdiplus::RectF(r.left, r.top, r.Width(), r.Height()));
}

void CSCGroupRect::add(Gdiplus::RectF r)
{
	m_r.push_back(r);
}
