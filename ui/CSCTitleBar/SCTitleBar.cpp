#include "SCTitleBar.h"

void CSCTitleBar::draw(CDC* pDC, CRect rc)
{
	Gdiplus::Graphics g(pDC->GetSafeHdc());
	draw(&g, rc);
}

void CSCTitleBar::draw(Gdiplus::Graphics* g, CRect rc)
{
	rc.bottom = rc.top + m_titlebar_height;
	g->FillRectangle(&Gdiplus::SolidBrush(Gdiplus::Color(255, 255, 0, 0)), Gdiplus::Rect(rc.left, rc.top, rc.Width(), rc.Height()));
}
