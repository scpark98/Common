/*
********** Author : Mohammed Nisamudheen S.  **********
********** Email  : nizam_clt@rediffmail.com **********
********** Date   : 21.10.2004               **********
********** URL    : www.uslindia.com         **********
*/

#include "stdafx.h"
#include "SideBarMenu.h"

//Constants used for drawing
const int SEPERATOR_HEIGHT = 8;
const int HILITE_HGAP = 10;
const int HILITE_VGAP = 5;
const int SIDEBAR_WIDTH = 30;
const int TEXT_MARGIN = 5;


CSideBarMenu::CSideBarMenu()
{
	m_SideBarStartColor = RGB(128, 128, 128);
	m_SideBarEndColor = RGB(200, 0, 0);
	m_SideBarTextColor = RGB(255, 255, 250);
	m_MenuBkgColor = RGB(255, 0, 0);
	m_HiLitColor = RGB(128, 128, 128);
	m_TextColor = RGB(128, 128, 128);
	m_HiLitTextColor = RGB(225, 225, 225);

}




void CSideBarMenu::MeasureItem(LPMEASUREITEMSTRUCT lpMIS)
{

	LPCTSTR itemData = (LPCTSTR)lpMIS->itemData;
	TEXTMETRIC tm;

	CWindowDC dc(NULL);//Just for geting the text metrics

	GetTextMetrics(dc, &tm);

	if (lstrlen(itemData) != 0)
	{
		lpMIS->itemHeight = tm.tmHeight + HILITE_VGAP; /* for making the height of the hilit bar
												a little greater */
	}
	else
	{
		//For menu seperator we use a fixed height
		lpMIS->itemHeight = SEPERATOR_HEIGHT;
	}


	lpMIS->itemWidth = tm.tmAveCharWidth * lstrlen(itemData) + TEXT_MARGIN;

}

void CSideBarMenu::DrawItem(LPDRAWITEMSTRUCT lpDIS)
{

	CDC dc;
	int oldMode;
	int oldTextColor;
	COLORREF cr;
	LPCTSTR lpszText = (LPCTSTR)lpDIS->itemData;



	CRect rectFull(lpDIS->rcItem);
	CRect rectText(rectFull.left + SIDEBAR_WIDTH, rectFull.top, rectFull.right, rectFull.bottom);
	CRect rectHiLite = rectText;

	rectHiLite.left -= HILITE_HGAP;

	dc.Attach(lpDIS->hDC);
	oldMode = dc.SetBkMode(TRANSPARENT);


	if (lpDIS->itemAction & ODA_DRAWENTIRE)
	{
		// Paint the color item in the color requested
		cr = m_TextColor;
		CBrush br(m_MenuBkgColor);
		dc.FillRect(&rectFull, &br);
	}

	if ((lpDIS->itemState & ODS_SELECTED) &&
		(lpDIS->itemAction & (ODA_SELECT | ODA_DRAWENTIRE)))
	{
		// item has been selected - hilite frame

		cr = m_HiLitTextColor;
		CBrush br(m_HiLitColor);
		CBrush *old = dc.SelectObject(&br);
		dc.FillRect(&rectHiLite, &br);
		//dc.RoundRect(&rectHiLite,CPoint(16,16));
		dc.SelectObject(old);
	}

	if (!(lpDIS->itemState & ODS_SELECTED) &&
		(lpDIS->itemAction & ODA_SELECT))
	{
		// Item has been de-selected 
		cr = m_TextColor;
		CBrush br(m_MenuBkgColor);
		dc.FillRect(&rectHiLite, &br);
	}

	oldTextColor = dc.SetTextColor(cr);

	dc.DrawText(
		lpszText,
		_tcslen(lpszText),
		&rectText,
		DT_SINGLELINE | DT_VCENTER);


	//Generating the sidebar with gradient
	if (lpDIS->itemAction & ODA_DRAWENTIRE)
	{

		CRect rectG;
		dc.GetClipBox(rectG);


		int p, q;
		int r1, g1, b1;
		int r2, g2, b2;
		int r, g, b;
		r1 = GetRValue(m_SideBarStartColor);
		g1 = GetGValue(m_SideBarStartColor);
		b1 = GetBValue(m_SideBarStartColor);

		r2 = GetRValue(m_SideBarEndColor);
		g2 = GetGValue(m_SideBarEndColor);
		b2 = GetBValue(m_SideBarEndColor);


		for (p = rectG.bottom; p > rectG.top; p--)
		{

			r = r2 + (p * (r1 - r2) / rectG.Height());
			g = g2 + (p * (g1 - g2) / rectG.Height());
			b = b2 + (p * (b1 - b2) / rectG.Height());
			for (q = 0; q < 20; q++)
				dc.SetPixel(q, p, RGB(r, g, b));

		}

		LOGFONT lf;
		NONCLIENTMETRICS ncm;
		CFont font;

		ncm.cbSize = sizeof(ncm);
		//Retrieving the system font settings
		SystemParametersInfo(SPI_GETNONCLIENTMETRICS,
			ncm.cbSize, &ncm, 0);

		memset(&lf, 0, sizeof(LOGFONT));

		lf = ncm.lfMenuFont; //storing all default menu font attributes
		lf.lfOrientation = lf.lfEscapement = 900;// 90 degree rotation
		lf.lfWeight = FW_BOLD;

		font.CreateFontIndirect(&lf);
		CFont * OldFont = dc.SelectObject(&font);
		COLORREF oldColor = dc.SetTextColor(RGB(128, 128, 128));
		rectG.left += 3;
		dc.SetTextColor(m_SideBarTextColor);
		dc.DrawText(m_SideBarText, &rectG, DT_BOTTOM | DT_LEFT | DT_SINGLELINE);
		dc.SetTextColor(oldColor);
		dc.SelectObject(OldFont);
	}

	dc.SetBkMode(oldMode);
	dc.SetTextColor(oldTextColor);

	//Handling the menu separators if any
	if (lstrlen(lpszText) == 0)
	{

		CBrush b(RGB(255, 0, 0));
		CPen p;

		COLORREF color = RGB(255, 255, 255); //separator color

		color -= m_MenuBkgColor; // to make it visible on all backgound colors
		p.CreatePen(PS_SOLID, 1, color);
		CBrush *b1 = dc.SelectObject(&b);
		CPen *p1 = dc.SelectObject(&p);


		dc.MoveTo(rectHiLite.left + 2, rectHiLite.top + 2);
		dc.LineTo(rectHiLite.left + rectHiLite.Width(),
			rectHiLite.top + 2);

		dc.SelectObject(b1);
		dc.SelectObject(p1);

	}


	dc.Detach();


}


HBITMAP CSideBarMenu::GetGradientBitmap(HDC hDC, COLORREF clStart, COLORREF clEnd, int nWidth, int nHeight)
{

	LPDWORD			pColorData;
	BITMAPINFO		BitmpInfo;


	pColorData = new DWORD[nWidth * nHeight];

	BitmpInfo.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	BitmpInfo.bmiHeader.biWidth = nWidth;
	BitmpInfo.bmiHeader.biHeight = nHeight;
	BitmpInfo.bmiHeader.biPlanes = 1;
	BitmpInfo.bmiHeader.biBitCount = 32;
	BitmpInfo.bmiHeader.biCompression = BI_RGB;

	int p, q;
	int r1, g1, b1;
	int r2, g2, b2;
	int r, g, b;
	r1 = GetRValue(m_SideBarStartColor);
	g1 = GetGValue(m_SideBarStartColor);
	b1 = GetBValue(m_SideBarStartColor);

	r2 = GetRValue(m_SideBarEndColor);
	g2 = GetGValue(m_SideBarEndColor);
	b2 = GetBValue(m_SideBarEndColor);


	for (p = 0; p < nHeight; p++)
	{

		r = r2 + (p * (r1 - r2) / nHeight);
		g = g2 + (p * (g1 - g2) / nHeight);
		b = b2 + (p * (b1 - b2) / nHeight);
		for (q = 0; q < nWidth; q++)
			*(pColorData + p + q) = RGB(r, g, b);

	}


	HBITMAP hBmp = CreateDIBitmap(hDC, &BitmpInfo.bmiHeader, CBM_INIT,
		pColorData, &BitmpInfo, DIB_RGB_COLORS);

	free(pColorData);

	return hBmp;


}


void CSideBarMenu::SetSideBarText(LPCSTR Text)
{

	m_SideBarText = Text;
}

void CSideBarMenu::SetSideBarTextColor(COLORREF color)
{

	m_SideBarTextColor = color;

}



void CSideBarMenu::SetHiLitColor(COLORREF color)
{
	m_HiLitColor = color;
}



void CSideBarMenu::SetMenuBkgColor(COLORREF color)
{
	m_MenuBkgColor = color;

}

void CSideBarMenu::SetSideBarColor(COLORREF start, COLORREF end)
{

	m_SideBarStartColor = start;
	m_SideBarEndColor = end;


}


void CSideBarMenu::SetMenuTextColor(COLORREF color)
{
	m_TextColor = color;
}


void CSideBarMenu::SetTextHiLitColor(COLORREF color)
{
	m_HiLitTextColor = color;
}
