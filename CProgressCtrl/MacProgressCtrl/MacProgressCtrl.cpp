// MacProgressCtrl.cpp : implementation file
//
//	CMacProgressCtrl class, version 1.0
//
//	Copyright (c) 1999 Paul M. Meidinger (pmmeidinger@yahoo.com)
//
// Feel free to modifiy and/or distribute this file, but
// do not remove this header.
//
// I would appreciate a notification of any bugs discovered or 
// improvements that could be made.
//
// This file is provided "as is" with no expressed or implied warranty.
//
//	History:
//		PMM	12/21/1999		Initial implementation.		


#include "MacProgressCtrl.h"
#include <thread>
#include "../../Functions.h"
#include "../../MemoryDC.h"


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#define	IDT_INDETERMINATE		100
#define	IND_BAND_WIDTH			20

/////////////////////////////////////////////////////////////////////////////
// CMacProgressCtrl

//-------------------------------------------------------------------
//
CMacProgressCtrl::CMacProgressCtrl()
//
// Return Value:	None.
//
// Parameters	:	None.
//
// Remarks		:	Standard constructor.
//
{
	m_nCtrlID			= 0;

	m_auto_hide			= false;

	m_use_slider		= false;
	m_bLButtonDown		= false;

	m_bIndeterminate	= FALSE;
	m_nIndOffset		= 0;
	m_cr_track			= RGB(0, 255, 0);//::GetSysColor(COLOR_HIGHLIGHT);

	m_sText				= "";
	m_cr_text0			= ::GetSysColor(COLOR_BTNTEXT);
	m_cr_text1			= RGB(0, 0, 0);
	m_cr_back			= ::GetSysColor(COLOR_3DFACE);
	m_cr_back_track		= ::GetSysColor(COLOR_BTNSHADOW);
	m_bTransparent		= false;
	m_bGradient			= false;
	
	//m_dq_cr_indeterminate.push_back(m_cr_backColor);
	m_dq_cr_indeterminate.push_back(::GetSysColor(COLOR_3DFACE));
	m_dq_cr_indeterminate.push_back(RGB(64, 212, 32));

	GetColors();
	CreatePens();

	memset(&m_lf, 0, sizeof(LOGFONT));
}	// CMacProgressCtrl

//-------------------------------------------------------------------
//
CMacProgressCtrl::~CMacProgressCtrl()
//
// Return Value:	None.
//
// Parameters	:	None.
//
// Remarks		:	None.
//
{
	m_bIndeterminate = false;
	DeletePens();
}	// ~CMacProgressCtrl


BEGIN_MESSAGE_MAP(CMacProgressCtrl, CProgressCtrl)
	//{{AFX_MSG_MAP(CMacProgressCtrl)
	ON_WM_PAINT()
	ON_WM_TIMER()
	ON_WM_ERASEBKGND()
	ON_WM_LBUTTONDOWN()
	ON_WM_LBUTTONUP()
	ON_WM_MOUSEMOVE()
	ON_WM_NCPAINT()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CMacProgressCtrl message handlers

//-------------------------------------------------------------------
//
void CMacProgressCtrl::OnPaint() 
//
// Return Value:	None.
//
// Parameters	:	None.
//
// Remarks		:	The framework calls this member function when Windows 
//						or an application makes a request to repaint a portion 
//						of an application뭩 window.
//
{
	CPaintDC dcPaint(this); // device context for painting
	CRect rtrack, rc;
	GetClientRect(rc);
	rtrack = rc;
	BOOL bVertical = GetStyle() & PBS_VERTICAL;

	CMemoryDC	dc(&dcPaint, &rc);
	Gdiplus::Graphics g(dc.m_hDC);
	g.SetSmoothingMode(Gdiplus::SmoothingMode::SmoothingModeAntiAlias);
	g.SetInterpolationMode(Gdiplus::InterpolationModeHighQualityBicubic);

	CFont* pOldFont = dc.SelectObject(&m_font);

	//track의 width는 rc와 동일하나 height는 작은 경우도 있다.(특히 style_round_line과 같은 경우)
	//실제 트랙이 그려지는 영역
	if (m_style == style_round_line)
	{
		rtrack.top = rtrack.CenterPoint().y - 4;
		rtrack.bottom = rtrack.CenterPoint().y + 4;
	}

	CBrush brLightest(m_cr_track_Lightest);
	CBrush brColor(m_cr_track);
	CBrush brBack(m_cr_back);
	//TRACE(_T("%ld : %s\n"), GetTickCount(), __function__);

	int pos = GetPos();
	//TRACE(_T("pos = %d\n"), pos);

	//우선 투명은 패스하자.
	if (m_bTransparent)
	{
		if (bVertical)
		{
			if (!m_bIndeterminate)
				rtrack.top = rtrack.bottom - int(((float)rtrack.Height() * float(pos - m_lower)) / float(m_upper - m_lower));
			dc.FillRect(rtrack, &brLightest);
			DrawVerticalBar(&dc, rtrack);
		}
		else
		{
			if (!m_bIndeterminate)
				rtrack.right = int(((float)rtrack.Width() * float(pos - m_lower)) / float(m_upper - m_lower));

			//CBrush brTrans;
			//brTrans.FromHandle((HBRUSH)GetStockObject(NULL_BRUSH));
			//dc.FillRect(rect, &brTrans);
			//dcPaint.FillRect(rect, &brLightest);
			//DrawHorizontalBar(&dc, rect);
		}
	}
	else
	{
		//active track과 inactive track을 사각형으로 칠하므로
		//style_round_line이 아니면 굳이 배경색을 중복해서 칠할 필요는 없다.
		//style_round_line일때만 배경색을 칠한다.
		if (m_style == style_round_line)
			dc.FillSolidRect(rc, m_cr_back);
	}
	

	//indeterminate일때는 배경색으로 그리는게 아니라 전경색보다 밝은 색으로 그린다.
	if (m_bIndeterminate)
	{
		CRect gradRect;
			
		if (m_indeterminate_forward)
			gradRect = CRect(m_nIndOffset - m_indeterminate_width, rc.top, m_nIndOffset, rc.bottom);
		else
			gradRect = CRect(m_nIndOffset, rc.top, m_nIndOffset + m_indeterminate_width, rc.bottom);
		//gradRect.left = m_nIndOffset;
		gradient_rect(dc, gradRect, m_dq_cr_indeterminate);
	}

	// Determine the size of the bar and draw it.
	//현재 vertical일 경우는 코드 완성되지 않음.
	if (bVertical)
	{
		if (!m_bIndeterminate)
			rtrack.top = rtrack.bottom - int(((float)rtrack.Height() * float(pos - m_lower)) / float(m_upper - m_lower));

		if (m_bGradient)
		{
			dc.FillRect(rtrack, &brLightest);
			DrawVerticalBar(&dc, rtrack);
		}
		else
		{
			dc.FillRect(rtrack, &brColor);
		}
	}
	else
  	{
		//draw inactive track
		if (!m_bIndeterminate)
		{
			//rect.right는 경과된 오른쪽 끝.
			rtrack.right = int(((float)rtrack.Width() * float(pos - m_lower)) / float(m_upper - m_lower));

			if (m_bGradient)
			{
				dc.FillRect(rtrack, &brLightest);
			}
			else
			{
				if (m_style == style_round_line)
				{
					Gdiplus::Pen pen(RGB2gpColor(m_cr_back_track), rtrack.Height());
					pen.SetLineCap(Gdiplus::LineCapRound, Gdiplus::LineCapRound, Gdiplus::DashCapRound);
					g.DrawLine(&pen, rc.left + rtrack.Height() / 2, rtrack.CenterPoint().y, rc.right - rtrack.Height() / 2 - 1, rtrack.CenterPoint().y);
				}
				else
				{
					dc.FillSolidRect(rc, m_cr_back_track);
				}
			}
		}

		DrawHorizontalBar(&dc, rtrack);
	}

	if (m_text_show)
	{
		if (m_text_format == text_format_percent)
		{
			if (m_upper - m_lower == 0)
				m_sText = _T("0.0 %%");
			else
				m_sText.Format(_T("%.1f %%"), (double)(pos + 0) / (double)(m_upper - m_lower) * 100.0);
		}
		else
		{
			m_sText.Format(_T("%ld / %ld"), pos, m_upper - m_lower);
		}

		if (m_sText != _T(""))
		{
			CRect	rText = rc;

			dc.SetBkMode(TRANSPARENT);

			if (m_text_shadow)
			{
				rText.OffsetRect(1, 0);
				dc.SetTextColor(m_crShadow);
				dc.DrawText(m_sText, rText, DT_SINGLELINE | DT_CENTER | DT_VCENTER);
				rText.OffsetRect(-1, -1);
			}

			if (m_use_invert_text_color)
			{
				CRect rcLeft, rcRight;
				rcLeft = rcRight = rText;
				rcRight.left = rcLeft.right = rtrack.right;

				//두 영역을 생성하여 해당 영역에만 그려지는 원리로 글자를 2가지 색으로 그린다.
				CRgn rgn;
				rgn.CreateRectRgnIndirect(rcLeft);
				dc.SelectClipRgn(&rgn);
				dc.SetTextColor(m_cr_text0);
				dc.DrawText(m_sText, rText, DT_VCENTER | DT_CENTER | DT_SINGLELINE);

				rgn.SetRectRgn(rcRight);
				dc.SelectClipRgn(&rgn);
				dc.SetTextColor(m_cr_text1);
				dc.DrawText(m_sText, rText, DT_VCENTER | DT_CENTER | DT_SINGLELINE);

				dc.SelectClipRgn(NULL);
			}
			else
			{
				dc.SetTextColor(m_cr_text0);
				dc.DrawText(m_sText, rText, DT_SINGLELINE | DT_CENTER | DT_VCENTER);
			}
		}
	}//if (m_text_show)

	if (m_draw_border)
	{
		draw_rect(&dc, rc, RGB2gpColor(m_border_color));// , NULL_BRUSH, m_border_width, m_border_pen_style);
	}

	dc.SelectObject(pOldFont);
}	// OnPaint	

//-------------------------------------------------------------------
//
void CMacProgressCtrl::DrawHorizontalBar(CDC *pDC, const CRect rtrack)
//
// Return Value:	None.
//
// Parameters	:	pDC - Specifies the device context object.
//						rect - Specifies the rectangle of the progess bar.
//
// Remarks		:	Draws a horizontal progress bar.
//
{
	if (!rtrack.Width())
		return;

	Gdiplus::Graphics g(pDC->m_hDC);
	g.SetSmoothingMode(Gdiplus::SmoothingMode::SmoothingModeAntiAlias);
	g.SetInterpolationMode(Gdiplus::InterpolationModeHighQualityBicubic);

	int nLeft = rtrack.left;
	int nTop = rtrack.top;
	int nBottom = rtrack.bottom;

	// Assume we're not drawing the indeterminate state.
	CPen *pOldPen = pDC->SelectObject(&m_penColorLight);

	if (m_bIndeterminate)
	{
		/*
		pOldPen = pDC->SelectObject(&m_penColor);
		int nNumBands = (rect.Width() / IND_BAND_WIDTH) + 2;
		int nHeight = rect.Height() + 1;

		int nAdjust = nLeft - IND_BAND_WIDTH + m_nIndOffset;
		int nXpos = 0;
		int nYpos1 = nTop + 1;
		int nYpos2 = nBottom - 2;

		for (int i = 0; i < nNumBands; i++)
		{
			nXpos = nAdjust + (i * IND_BAND_WIDTH);

			pDC->SelectObject(&m_penColorDarker);
			pDC->MoveTo(nXpos + 1, nTop);
			pDC->LineTo(nXpos + nHeight, nBottom);

			pDC->SelectObject(&m_penColorDark);
			pDC->MoveTo(nXpos + 2, nTop);
			pDC->LineTo(nXpos + nHeight + 1, nBottom);
			pDC->MoveTo(nXpos + 10, nTop);
			pDC->LineTo(nXpos + nHeight + 9, nBottom);

			pDC->SelectObject(&m_penColor);
			pDC->MoveTo(nXpos + 3, nTop);
			pDC->LineTo(nXpos + nHeight + 2, nBottom);
			pDC->MoveTo(nXpos + 9, nTop);
			pDC->LineTo(nXpos + nHeight + 8, nBottom);

			pDC->SelectObject(&m_penColorLight);
			pDC->MoveTo(nXpos + 4, nTop);
			pDC->LineTo(nXpos + nHeight + 3, nBottom);
			pDC->MoveTo(nXpos + 8, nTop);
			pDC->LineTo(nXpos + nHeight + 7, nBottom);

			pDC->SelectObject(&m_penColorLighter);
			pDC->MoveTo(nXpos + 5, nTop);
			pDC->LineTo(nXpos + nHeight + 4, nBottom);
			pDC->MoveTo(nXpos + 7, nTop);
			pDC->LineTo(nXpos + nHeight + 6, nBottom);
		}	// for the number of bands
		*/
	}	// if indeterminate
	else if (m_bGradient)
	{
		int nRight = rtrack.right;
	
		pDC->MoveTo(nLeft + 2, nBottom - 4);
		pDC->LineTo(nRight - 2, nBottom - 4);
		pDC->MoveTo(nLeft + 2, nTop + 2);
		pDC->LineTo(nRight - 2, nTop + 2);
		pDC->SetPixel(nLeft + 1, nBottom - 3, m_cr_track);
		pDC->SetPixel(nLeft + 1, nTop + 1, m_cr_track);

		pDC->SelectObject(&m_penColorLighter);
		pDC->MoveTo(nLeft + 2, nBottom - 5);
		pDC->LineTo(nRight - 3, nBottom - 5);
		pDC->LineTo(nRight - 3, nTop + 3);
		pDC->LineTo(nLeft + 1, nTop + 3);
		pDC->SetPixel(nLeft + 1, nBottom - 4, m_cr_track_Lighter);
		pDC->SetPixel(nLeft + 1, nTop + 2, m_cr_track_Lighter);

		pDC->SelectObject(&m_penColor);
		pDC->MoveTo(nLeft, nBottom - 1);
		pDC->LineTo(nLeft, nTop);
		pDC->LineTo(nLeft + 2, nTop);
		pDC->SetPixel(nLeft + 1, nBottom - 2, m_cr_track);
		pDC->MoveTo(nLeft + 2, nBottom - 3);
		pDC->LineTo(nRight - 2, nBottom - 3);
		pDC->MoveTo(nLeft + 2, nTop + 1);
		pDC->LineTo(nRight - 1, nTop + 1);
		
		pDC->SelectObject(&m_penColorDark);
		pDC->MoveTo(nLeft + 2, nBottom - 2);
		pDC->LineTo(nRight - 2, nBottom - 2);
		pDC->LineTo(nRight - 2, nTop + 1);
		pDC->MoveTo(nLeft + 2, nTop);
		pDC->LineTo(nRight, nTop);
		pDC->SetPixel(nLeft + 1, nBottom - 1, m_cr_track_Dark);

		pDC->SelectObject(&m_penColorDarker);
		pDC->MoveTo(nLeft + 2, nBottom - 1);
		pDC->LineTo(nRight - 1, nBottom - 1);
		pDC->LineTo(nRight - 1, nTop);
	}	// if not indeterminate
	else
	{
		//TRACE(_T("m_style = %d, rtrack.right = %d\n"), m_style, rtrack.right);

		if (m_style == style_default)
		{
			pDC->FillSolidRect(rtrack, m_cr_track);
		}
		else if (m_style == style_round_line)
		{
			Gdiplus::Pen pen(RGB2gpColor(m_cr_track), rtrack.Height());
			pen.SetLineCap(Gdiplus::LineCapRound, Gdiplus::LineCapRound, Gdiplus::DashCapRound);

			int end = MAX(0, rtrack.right - rtrack.Height() / 2);
			//TRACE(_T("end = %d\n"), end);
			g.DrawLine(&pen, rtrack.Height() / 2, rtrack.CenterPoint().y, end, rtrack.CenterPoint().y);
		}
	}

	pDC->SelectObject(pOldPen);
}	// DrawHorizontalBar

//-------------------------------------------------------------------
//
void CMacProgressCtrl::DrawVerticalBar(CDC *pDC, const CRect rect)
//
// Return Value:	None.
//
// Parameters	:	pDC - Specifies the device context object.
//						rect - Specifies the rectangle of the progess bar.
//
// Remarks		:	Draws a vertical progress bar.
//
{
	int nHeight = rect.Height();
	if (!nHeight)
		return;

	int nLeft = rect.left;
	int nTop = rect.top;
	int nRight = rect.right;
	int nBottom = rect.bottom;

	CPen *pOldPen = pDC->SelectObject(&m_penColor);

	if (m_bIndeterminate)
	{
		int nNumBands = (nHeight / IND_BAND_WIDTH) + 2;
		int nHeight = rect.Width() + 1;

		int nAdjust = nBottom - m_nIndOffset;
		int nXpos1 = nLeft;
		int nXpos2 = nRight + 1;
		int nYpos = nTop + 1;

		for (int i = 0; i < nNumBands; i++)
		{
			nYpos = nAdjust - (i * IND_BAND_WIDTH);

			pDC->SelectObject(&m_penColorDarker);
			pDC->MoveTo(nXpos1, nYpos);
			pDC->LineTo(nXpos2, nYpos + nHeight);

			pDC->SelectObject(&m_penColorDark);
			pDC->MoveTo(nXpos1, nYpos + 1);
			pDC->LineTo(nXpos2, nYpos + nHeight + 1);
			pDC->MoveTo(nXpos1, nYpos + 9);
			pDC->LineTo(nXpos2, nYpos + nHeight + 9);

			pDC->SelectObject(&m_penColor);
			pDC->MoveTo(nXpos1, nYpos + 2);
			pDC->LineTo(nXpos2, nYpos + nHeight + 2);
			pDC->MoveTo(nXpos1, nYpos + 8);
			pDC->LineTo(nXpos2, nYpos + nHeight + 8);

			pDC->SelectObject(&m_penColorLight);
			pDC->MoveTo(nXpos1, nYpos + 3);
			pDC->LineTo(nXpos2, nYpos + nHeight + 3);
			pDC->MoveTo(nXpos1, nYpos + 7);
			pDC->LineTo(nXpos2, nYpos + nHeight + 7);

			pDC->SelectObject(&m_penColorLighter);
			pDC->MoveTo(nXpos1, nYpos + 4);
			pDC->LineTo(nXpos2, nYpos + nHeight + 4);
			pDC->MoveTo(nXpos1, nYpos + 6);
			pDC->LineTo(nXpos2, nYpos + nHeight + 6);
		}	// for the number of bands
	}	// if indeterminate
	else
	{
		if (nHeight > 3)
		{
			pDC->MoveTo(nLeft, nTop + 1);
			pDC->LineTo(nLeft, nTop);
			pDC->LineTo(nRight, nTop);
			pDC->MoveTo(nLeft + 1, nBottom - 2);
			pDC->LineTo(nLeft + 1, nTop + 1);
			pDC->MoveTo(nRight - 3, nBottom - 3);
			pDC->LineTo(nRight - 3, nTop + 1);
			pDC->SetPixel(nRight - 2, nTop + 1, m_cr_track);

			pDC->SelectObject(&m_penColorLight);
			pDC->MoveTo(nLeft + 2, nBottom - 3);
			pDC->LineTo(nLeft + 2, nTop + 1);
			pDC->MoveTo(nRight - 4, nBottom - 3);
			pDC->LineTo(nRight - 4, nTop + 1);
			pDC->SetPixel(nLeft + 1, nTop + 1, m_cr_track_Light);
			pDC->SetPixel(nRight - 3, nTop + 1, m_cr_track_Light);
			
			pDC->SelectObject(&m_penColorLighter);
			pDC->MoveTo(nLeft + 3, nBottom - 3);
			pDC->LineTo(nLeft + 3, nTop + 1);
			pDC->MoveTo(nRight - 5, nBottom - 3);
			pDC->LineTo(nRight - 5, nTop + 1);
			pDC->SetPixel(nLeft + 2, nTop + 1, m_cr_track_Lighter);
			pDC->SetPixel(nRight - 4, nTop + 1, m_cr_track_Lighter);

			pDC->SelectObject(&m_penColorDark);
			pDC->MoveTo(nLeft, nBottom - 1);
			pDC->LineTo(nLeft, nTop + 1);
			pDC->MoveTo(nLeft + 2, nBottom - 2);
			pDC->LineTo(nRight - 2, nBottom - 2);
			pDC->LineTo(nRight - 2, nTop + 1);
			pDC->SetPixel(nRight - 1, nTop + 1, m_cr_track_Dark);

			pDC->SelectObject(&m_penColorDarker);
			pDC->MoveTo(nLeft + 1, nBottom - 1);
			pDC->LineTo(nRight - 1, nBottom - 1);
			pDC->LineTo(nRight - 1, nTop + 1);
		}
		else
		{
			CBrush br(m_cr_track);
			CBrush *pOldBrush = pDC->SelectObject(&br);
			pDC->SelectObject(&m_penColorDark);
			pDC->Rectangle(rect);
			pDC->SelectObject(pOldBrush);
		}
	}	// if not indeterminate

	pDC->SelectObject(pOldPen);
}	// DrawVerticalBar

//-------------------------------------------------------------------
//
BOOL CMacProgressCtrl::OnEraseBkgnd(CDC* pDC) 
//
// Return Value:	Nonzero if it erases the background; otherwise 0.
//
// Parameters	:	pDC - Specifies the device-context object.
//
// Remarks		:	The framework calls this member function when the 
//						CWnd object background needs erasing (for example, 
//						when resized). It is called to prepare an invalidated 
//						region for painting.
//
{
		return false;
}	// OnEraseBkgnd


COLORREF CMacProgressCtrl::GetColor(COLORREF crOrigin, int nOffset)
{
	int nRed	= GetRValue(crOrigin) + nOffset;
	int nGreen	= GetGValue(crOrigin) + nOffset;
	int nBlue	= GetBValue(crOrigin) + nOffset;
	
	if (nRed < 0)
		nRed = 0;
	else if (nRed > 255)
		nRed = 255;

	if (nGreen < 0)
		nGreen = 0;
	else if (nGreen > 255)
		nGreen = 255;

	if (nBlue < 0)
		nBlue = 0;
	else if (nBlue > 255)
		nBlue = 255;
	
	return RGB(nRed, nGreen, nBlue);
}

//-------------------------------------------------------------------
//
void CMacProgressCtrl::GetColors()
//
// Return Value:	None.
//
// Parameters	:	None.
//
// Remarks		:	Calculates the lighter and darker colors, as well as 
//						the shadow colors.
//
{
	m_cr_track_Light = GetColor(m_cr_track, 51);
	m_cr_track_Lighter = GetColor(m_cr_track_Light, 51);
	m_cr_track_Lightest = GetColor(m_cr_track_Lighter, 51);
	m_cr_track_Dark = GetColor(m_cr_track, -51);
	m_cr_track_Darker = GetColor(m_cr_track_Dark, -51);
	m_crDkShadow = ::GetSysColor(COLOR_3DDKSHADOW);
	m_crLiteShadow = ::GetSysColor(COLOR_3DSHADOW);

	// Get a color halfway between COLOR_3DDKSHADOW and COLOR_3DSHADOW
	BYTE byRed3DDkShadow = GetRValue(m_crDkShadow);
	BYTE byRed3DLiteShadow = GetRValue(m_crLiteShadow);
	BYTE byGreen3DDkShadow = GetGValue(m_crDkShadow);
	BYTE byGreen3DLiteShadow = GetGValue(m_crLiteShadow);
	BYTE byBlue3DDkShadow = GetBValue(m_crDkShadow);
	BYTE byBlue3DLiteShadow = GetBValue(m_crLiteShadow);

	m_crShadow = RGB(byRed3DLiteShadow + ((byRed3DDkShadow - byRed3DLiteShadow) >> 1),
						  byGreen3DLiteShadow + ((byGreen3DDkShadow - byGreen3DLiteShadow) >> 1),
						  byBlue3DLiteShadow + ((byBlue3DDkShadow - byBlue3DLiteShadow) >> 1));
}	// GetColors

//-------------------------------------------------------------------
//
void CMacProgressCtrl::SetColor(COLORREF crColor,
								COLORREF crBackColor)
//
// Return Value:	None.
//
// Parameters	:	crColor - New color.
//
// Remarks		:	Sets the progress	bar control's color. The lighter
//						darker colors are recalculated, and the pens recreated.
//
{
	m_cr_track = crColor;
	m_cr_back_track = crBackColor;

	if (m_cr_back_track == -1)
		m_bTransparent = TRUE;
	else
		m_bTransparent = FALSE;
	
	GetColors();
	CreatePens();
	RedrawWindow();
}	// SetColor

void CMacProgressCtrl::set_back_color(COLORREF cr_back)
{
	m_cr_back = cr_back;
	Invalidate();
}

void CMacProgressCtrl::set_back_track_color(COLORREF cr_back_track)
{
	m_cr_back_track = cr_back_track;
	m_dq_cr_indeterminate[0] = cr_back_track;
	Invalidate();
}

//-------------------------------------------------------------------
//
void CMacProgressCtrl::CreatePens()
//
// Return Value:	None.
//
// Parameters	:	None.
//
// Remarks		:	Deletes the pen objects, if necessary, and creates them.
//
{
	DeletePens();

	m_penColorLight.CreatePen(PS_SOLID, 1, m_cr_track_Light);
	m_penColorLighter.CreatePen(PS_SOLID, 1, m_cr_track_Lighter);
	m_penColor.CreatePen(PS_SOLID, 1, m_cr_track);
	m_penColorDark.CreatePen(PS_SOLID, 1, m_cr_track_Dark);
	m_penColorDarker.CreatePen(PS_SOLID, 1, m_cr_track_Darker);
	m_penDkShadow.CreatePen(PS_SOLID, 1, m_crDkShadow);
	m_penShadow.CreatePen(PS_SOLID, 1, m_crShadow);
	m_penLiteShadow.CreatePen(PS_SOLID, 1, m_crLiteShadow);
}	// CreatePens

//-------------------------------------------------------------------
//
void CMacProgressCtrl::DeletePens()
//
// Return Value:	None.
//
// Parameters	:	None.
//
// Remarks		:	Deletes the pen objects.
//
{
	if (m_penColorLight.m_hObject)
		m_penColorLight.DeleteObject();
	if (m_penColorLighter.m_hObject)
		m_penColorLighter.DeleteObject();
	if (m_penColor.m_hObject)
		m_penColor.DeleteObject();
	if (m_penColorDark.m_hObject)
		m_penColorDark.DeleteObject();
	if (m_penColorDarker.m_hObject)
		m_penColorDarker.DeleteObject();
	if (m_penDkShadow.m_hObject)
		m_penDkShadow.DeleteObject();
	if (m_penShadow.m_hObject)
		m_penShadow.DeleteObject();
	if (m_penLiteShadow.m_hObject)
		m_penLiteShadow.DeleteObject();
}	// DeletePens

//-------------------------------------------------------------------
//
void CMacProgressCtrl::SetIndeterminate(BOOL bIndeterminate)
//
// Return Value:	None.
//
// Parameters	:	bIndeterminate - Specifies the indeterminate state.
//
// Remarks		:	Sets the indeterminate flag.
//
{
	m_bIndeterminate = bIndeterminate;

	if (m_bIndeterminate)
	{
		CRect rect;
		GetClientRect(rect);
		m_nIndOffset = 0;

		m_bGradient = false;
		RedrawWindow();
		std::thread t(&CMacProgressCtrl::thread_indeterminate, this);
		t.detach();
		//SetTimer(IDT_INDETERMINATE, 25, NULL);
	}
	else
	{
		//KillTimer(IDT_INDETERMINATE);
		RedrawWindow();
	}
}	// SetIndeterminate

void CMacProgressCtrl::thread_indeterminate()
{
	m_nIndOffset = 0;

	int frame = 0;

	while (m_bIndeterminate)
	{
		if (!m_hWnd)
			break;

		CRect rc;
		GetClientRect(rc);

		if (m_indeterminate_forward)
		{
			m_nIndOffset += 6;// m_nIndOffset + ((double)m_nIndOffset + 1.0) * 1.2;
			if (m_nIndOffset > rc.right + m_indeterminate_width / 1.1)
			{
				m_indeterminate_forward = false;
				std::reverse(m_dq_cr_indeterminate.begin(), m_dq_cr_indeterminate.end());
			}
		}
		else
		{
			m_nIndOffset -= 6;// m_nIndOffset - ((double)m_nIndOffset + 1.0) * 1.2;
			if (m_nIndOffset < rc.left - m_indeterminate_width / 1.1)
			{
				m_indeterminate_forward = true;
				std::reverse(m_dq_cr_indeterminate.begin(), m_dq_cr_indeterminate.end());
			}
		}
		//TRACE(_T("m_nIndOffset = %d\n"), m_nIndOffset);
		std::this_thread::sleep_for(std::chrono::milliseconds(10));
		//Invalidate();

		if (!m_hWnd)
			return;

		RedrawWindow();

		//for test
		//CString str;
		//str.Format(_T("d:\\temp\\progressDlg\\%03d.bmp"), frame++);
		//save_bitmap(CaptureWindowToBitmap(GetParent()->m_hWnd), str);
	}
}

//-------------------------------------------------------------------
//
BOOL CMacProgressCtrl::GetIndeterminate()
//
// Return Value:	m_bIndeterminate.
//
// Parameters	:	None.
//
// Remarks		:	Returns m_bIndeterminate.
//
{
	return m_bIndeterminate;
}	// GetIndeterminate

//-------------------------------------------------------------------
//
void CMacProgressCtrl::OnTimer(UINT_PTR nIDEvent)
//
// Return Value:	None.
//
// Parameters	:	nIDEvent - Specifies the identifier of the timer.
//
// Remarks		:	The framework calls this member function after each 
//						interval specified in the SetTimer member function used 
//						to install a timer.
//
{
	// Increment the indeterminate bar offset and redraw the window.
	if (nIDEvent == IDT_INDETERMINATE)
	{
		KillTimer(nIDEvent);

		if (++m_nIndOffset > 300)
		{
			m_nIndOffset = 0;
		}

		TRACE(_T("m_nIndOffset = %d\n"), m_nIndOffset);

		RedrawWindow();

		SetTimer(IDT_INDETERMINATE, 10, NULL);
	}
}	// OnTimer

void CMacProgressCtrl::SetText(CString sText)
{
	m_sText	= sText;
	RedrawWindow();
}

void CMacProgressCtrl::set_text_color(COLORREF crText0, COLORREF crText1)
{
	m_cr_text0 = crText0;
	m_cr_text1 = (crText1 == -1 ? crText0 : crText1);

	RedrawWindow();
}

void CMacProgressCtrl::use_text_shadow(bool shadow)
{
	m_text_shadow = shadow;

	RedrawWindow();
}

void CMacProgressCtrl::draw_border(bool border, int width, COLORREF cr, int pen_style)
{
	m_draw_border = border;
	m_border_width = width;
	m_border_color = cr;
	m_border_pen_style = pen_style;

	RedrawWindow();
}


void CMacProgressCtrl::use_invert_text_color(bool use)
{
	m_use_invert_text_color = use;
	Invalidate();
}

void CMacProgressCtrl::OnLButtonDown(UINT nFlags, CPoint point)
{
	// TODO: 여기에 메시지 처리기 코드를 추가 및/또는 기본값을 호출합니다.
	if (!m_use_slider || m_bIndeterminate)
		return;

	m_bLButtonDown = true;
	OnMouseMove(nFlags, point);
	SetCapture();

	CProgressCtrl::OnLButtonDown(nFlags, point);
}


void CMacProgressCtrl::OnLButtonUp(UINT nFlags, CPoint point)
{
	// TODO: 여기에 메시지 처리기 코드를 추가 및/또는 기본값을 호출합니다.
	if (!m_use_slider)
		return;

	m_bLButtonDown = false;
	ReleaseCapture();

	CProgressCtrl::OnLButtonUp(nFlags, point);
}


void CMacProgressCtrl::OnMouseMove(UINT nFlags, CPoint point)
{
	// TODO: 여기에 메시지 처리기 코드를 추가 및/또는 기본값을 호출합니다.
	if (!m_use_slider)
		return;

	if (m_bLButtonDown)
	{
		int		m_lower;
		int		m_upper;
		CRect	Rect;

		GetRange(m_lower, m_upper); 
		GetClientRect(&Rect);

		int nPos = (int)((double)point.x * (double)(m_upper - m_lower) / (double)Rect.right) + m_lower;
		SetPos(nPos);
		//Invalidate(false);

		::SendMessage( GetParent()->GetSafeHwnd(), MESSAGE_MACPROGRESSCTRL_MOVED, m_nCtrlID, nPos);
		TRACE("%d\n", nPos);
	}

	CProgressCtrl::OnMouseMove(nFlags, point);
}

//이 함수 핸들러를 추가해야 기본 테두리가 표시되지 않는다.
void CMacProgressCtrl::OnNcPaint()
{
	// TODO: 여기에 메시지 처리기 코드를 추가합니다.
	// 그리기 메시지에 대해서는 CComboBox::OnNcPaint()을(를) 호출하지 마십시오.
}

void CMacProgressCtrl::SetRange32(int lower, int upper)
{
	SetRange(lower, upper);
}

void CMacProgressCtrl::SetRange(int lower, int upper)
{
	CProgressCtrl::SetRange32(lower, upper);

	m_lower = lower;
	m_upper = upper;

	if (m_auto_hide)
		ShowWindow(SW_SHOW);
}

void CMacProgressCtrl::SetPos(int pos)
{
	CProgressCtrl::SetPos(pos);
	pos = GetPos();
	Invalidate();

	if (m_auto_hide && (pos >= m_upper || pos < 0))
		ShowWindow(SW_HIDE);
}


void CMacProgressCtrl::PreSubclassWindow()
{
	// TODO: 여기에 특수화된 코드를 추가 및/또는 기본 클래스를 호출합니다.
	CWnd* pWnd = AfxGetMainWnd();
	CFont* font = NULL;
	
	if (pWnd)
		font = pWnd->GetFont();

	if (font == NULL)
		GetObject(GetStockObject(SYSTEM_FONT), sizeof(m_lf), &m_lf);
	else
		font->GetObject(sizeof(m_lf), &m_lf);

	reconstruct_font();

	CProgressCtrl::PreSubclassWindow();
}

void CMacProgressCtrl::reconstruct_font()
{
	m_font.DeleteObject();
	BOOL bCreated = m_font.CreateFontIndirect(&m_lf);

	ASSERT(bCreated);
}

