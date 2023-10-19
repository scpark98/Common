// SliderCtrlEx.cpp : implementation file
//

//#include "stdafx.h"
#include "SliderCtrlEx.h"

#include "../Functions.h"
#include "../MemoryDC.h"
#include "../Functions.h"
#include "../AutoFont.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#define THUMB_WIDTH	14

/////////////////////////////////////////////////////////////////////////////
// CSliderCtrlEx

CSliderCtrlEx::CSliderCtrlEx()
{
	m_pCallback_func = NULL;

	m_nStyle = slider_thumb;
	m_nEventMsgStyle = AfxGetApp()->GetProfileInt(_T("setting"), _T("event msg style"), msg_style_timer);

	m_bEnableSlide	= true;
	m_use_bookmark = false;
	m_cur_bookmark = -1;

	m_use_tooltip = false;
	m_tooltip_format = tooltip_value;


	m_nMin		= 0;
	m_nMax		= 0;
	m_nPos		= 0;

	m_nMarginLeft = m_nMarginRight = m_nMarginTop = m_nMarginBottom = 0;
	m_nThumbWidth = THUMB_WIDTH * 2;
	m_nThumbHeight = 0;

	m_nTrackHeight = 5;

	m_bChannel = m_bChannelActive = m_bVertical = m_bThumb = m_bLButtonDown = false;
	m_bTransparentChannel = false;
	m_bDrawFocusRect = false;

	m_nValueStyle = value;
	m_crValueText = RGB(32, 32, 32);

	m_dcBk.m_hDC = NULL;

	m_crBack	= ::GetSysColor(COLOR_3DFACE);
	m_crActive	= RGB(128, 192, 255);
	m_crInActive= get_color(m_crBack, -64);
	m_crThumb	= RGB(124, 192, 232);

	//북마크 컬러는 처음에만 배경의 보색으로 설정되지만
	//차후 배경이 바뀌더라도 북마크의 색상까지 자동으로 보색으로 바꾸는 것은 좋지 않다.
	m_crBookmark = color_complementary(m_crBack);
	m_crBookmarkCurrent = green;

	m_repeat_start = -1;
	m_repeat_end = -1;

	SetThumbColor(m_crThumb);
}

CSliderCtrlEx::~CSliderCtrlEx()
{
	DeleteObject(m_dcBk.SelectObject(&m_bmpBkOld));
	DeleteDC(m_dcBk);

	m_penThumb.DeleteObject();
	m_penThumbLight.DeleteObject();
	m_penThumbLighter.DeleteObject();
	m_penThumbDark.DeleteObject();
	m_penThumbDarker.DeleteObject();
}


BEGIN_MESSAGE_MAP(CSliderCtrlEx, CSliderCtrl)
	//{{AFX_MSG_MAP(CSliderCtrlEx)
	ON_WM_PAINT()
	ON_NOTIFY_REFLECT(NM_CUSTOMDRAW, OnCustomDraw)
	ON_WM_ERASEBKGND()
	ON_WM_LBUTTONDOWN()
	ON_WM_LBUTTONUP()
	ON_WM_MOUSEMOVE()
	ON_WM_SETFOCUS()
	ON_WM_KILLFOCUS()
	//}}AFX_MSG_MAP
	//ON_WM_KEYDOWN()
	ON_WM_TIMER()
	ON_WM_SIZE()
	ON_WM_WINDOWPOSCHANGED()
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CSliderCtrlEx message handlers
void CSliderCtrlEx::OnCustomDraw(NMHDR* pNMHDR, LRESULT* pResult) 
//
// Description	:	Sent by the slider control to notify the parent window 
//					about drawing operations. This notification is sent in 
//					the form of a WM_NOTIFY message.
// Parameters	:	pNMHDR - a pointer to a NM_CUSTOMDRAW structure.
//					pResult - value depends on the current drawing state.
{
	LPNMCUSTOMDRAW lpcd = (LPNMCUSTOMDRAW)pNMHDR;
	CDC *pDC = CDC::FromHandle(lpcd->hdc);

	GetClientRect(m_rc);

	switch(lpcd->dwDrawStage)
	{
		case CDDS_PREPAINT:
			*pResult = CDRF_NOTIFYITEMDRAW ;
			break;
			//return;  
		case CDDS_ITEMPREPAINT:
		if (lpcd->dwItemSpec == TBCD_THUMB)
		{
			*pResult = CDRF_DODEFAULT;
			break;
		}
		if (lpcd->dwItemSpec == TBCD_CHANNEL)
		{
			CClientDC clientDC(GetParent());
			CRect crect;
			CRect wrect;
			GetClientRect(crect);
			GetWindowRect(wrect);
			GetParent()->ScreenToClient(wrect);
			if (m_dcBk.m_hDC == NULL)
			{
				m_dcBk.CreateCompatibleDC(&clientDC);
				m_bmpBk.CreateCompatibleBitmap(&clientDC, crect.Width(), crect.Height());
				m_bmpBkOld = m_dcBk.SelectObject(&m_bmpBk);
				m_dcBk.BitBlt(0, 0, crect.Width(), crect.Height(), &clientDC, wrect.left, wrect.top, SRCCOPY);
			}
			//This bit does the tics marks transparently.
			//create a memory dc to hold a copy of the oldbitmap data that includes the tics,
			//because when we add the background in we will lose the tic marks
			CDC SaveCDC;
			CBitmap SaveCBmp, maskBitmap;
			//set the colours for the monochrome mask bitmap
			COLORREF crOldBack = pDC->SetBkColor(RGB(0,0,0));
			COLORREF crOldText = pDC->SetTextColor(RGB(255,255,255));
			CDC maskDC;
			int iWidth = crect.Width();
			int iHeight = crect.Height();
			SaveCDC.CreateCompatibleDC(pDC);
			SaveCBmp.CreateCompatibleBitmap(&SaveCDC, iWidth, iHeight);
			CBitmap* SaveCBmpOld = (CBitmap *)SaveCDC.SelectObject(SaveCBmp);
			//fill in the memory dc for the mask
			maskDC.CreateCompatibleDC(&SaveCDC);
			//create a monochrome bitmap
			maskBitmap.CreateBitmap(iWidth, iHeight, 1, 1, NULL);
			//select the mask bitmap into the dc
			CBitmap* OldmaskBitmap = maskDC.SelectObject(&maskBitmap);
			//copy the oldbitmap data into the bitmap, this includes the tics.
			SaveCDC.BitBlt(0, 0, iWidth, iHeight, pDC, crect.left, crect.top, SRCCOPY);
			//now copy the background into the slider
			BitBlt(lpcd->hdc, 0, 0, iWidth, iHeight, m_dcBk.m_hDC, 0, 0, SRCCOPY);
			// Blit the mask based on background colour
			maskDC.BitBlt(0, 0, iWidth, iHeight, &SaveCDC, 0, 0, SRCCOPY);
			// Blit the image using the mask
			pDC->BitBlt(0, 0, iWidth, iHeight, &SaveCDC, 0, 0, SRCINVERT);
			pDC->BitBlt(0, 0, iWidth, iHeight, &maskDC, 0, 0, SRCAND);
			pDC->BitBlt(0, 0, iWidth, iHeight, &SaveCDC, 0, 0, SRCINVERT);
			//restore and clean up
			pDC->SetBkColor(crOldBack);
			pDC->SetTextColor(crOldText);
			DeleteObject(SelectObject(SaveCDC, SaveCBmpOld));
			DeleteDC(SaveCDC);
			DeleteObject(maskDC.SelectObject(OldmaskBitmap));
			DeleteDC(maskDC);
			*pResult = 0;
			break;
		}
	}
}

void CSliderCtrlEx::OnPaint() 
{
	if (m_nStyle == slider_normal)
	{
		CSliderCtrl::OnPaint();
		return;
	}

	CPaintDC	dc1(this); // device context for painting
	CPen*		pOldPen = NULL;
	CBrush*		pOldBrush = NULL;
	CString		str;

	// TODO: Add your message handler code here
	GetClientRect(m_rc);
	CMemoryDC	dc(&dc1, &m_rc, m_bTransparentChannel);

	CDC		dcMem;
	dcMem.CreateCompatibleDC(&dc);

	CBitmap*	pbmTmp;
	//m_rc의 y센터좌표
	int			cy = m_rc.CenterPoint().y;
	//현재 위치의 실제 픽셀좌표
	int			pos = Pos2Pixel(m_nPos);
	int i;
	
	CAutoFont font(_T("굴림"));
	font.SetHeight((double)get_logical_size_from_font_size(m_hWnd, m_rc.Height()) / 2.2);
	font.SetBold(true);
	CFont* pOldFont = (CFont*)dc.SelectObject(&font);

	//background
	if(m_bChannel)
	{
		pbmTmp = dcMem.SelectObject(&m_bmChannel);

		dc.StretchBlt(0, 0, m_rc.Width(), m_rc.Height(), &dcMem,
						0, 0, m_nChannelWidth, m_nChannelHeight, SRCCOPY);
	}
	else
	{
		//전체 슬라이드 사각형 영역을 배경색으로 그림
		dc.FillSolidRect(m_rc, enable_color(m_crBack));

		if (m_nStyle <= slider_value)
		{
			dc.FillSolidRect(pos, cy - m_nTrackHeight / 2, m_rc.right - pos, m_nTrackHeight, enable_color(m_crInActive));
			
			CPen	penDark(PS_SOLID, 1, get_color(enable_color(m_crInActive), -24));
			CPen	penLight(PS_SOLID, 1, get_color(enable_color(m_crInActive), 36));

			dc.SelectObject(&penDark);
			dc.MoveTo(pos, cy - m_nTrackHeight / 2);
			dc.LineTo(m_rc.right, cy - m_nTrackHeight / 2);
			//dc.LineTo(m_rc.right - 1, cy + m_nTrackHeight / 2);

			dc.SelectObject(&penLight);
			dc.MoveTo(pos, cy + m_nTrackHeight / 2);
			dc.LineTo(m_rc.right - 1, cy + m_nTrackHeight / 2);
			dc.LineTo(m_rc.right - 1, cy - m_nTrackHeight / 2);

			dc.SelectObject(pOldPen);
			penDark.DeleteObject();
			penLight.DeleteObject();
		}
		else if (m_nStyle == slider_progress)
		{
			CRect	rInActive(pos, m_rc.top + 2, m_rc.right, m_rc.bottom - 2);
			dc.FillSolidRect(rInActive, enable_color(m_crInActive));
		}
		else if (m_nStyle == slider_track)
		{
			CRect r = m_rc;
			int cy = r.CenterPoint().y;
			r.top = cy - 4;
			r.bottom = cy + 4;
			/*
			dc.FillSolidRect(r.left, cy - 4, r.Width(), 2, get_color(m_crBack, -64));
			dc.FillSolidRect(r.left, cy - 2, r.Width(), 4, m_crBack);//GRAY192);
			dc.FillSolidRect(r.left, cy + 2, r.Width(), 2, get_color(m_crBack, 64));
			*/
			DrawSunkenRect(&dc, r, true, get_color(m_crBack, -32), get_color(m_crBack, 32), 1);
			r.DeflateRect(1, 1);
			DrawSunkenRect(&dc, r, true, get_color(m_crBack, -32), get_color(m_crBack, 32), 1);

		}
	}

	//경과된 영역(active area) 표시
	if (m_bChannelActive)
	{
		pbmTmp = dcMem.SelectObject(&m_bmChannelActive);
		dc.StretchBlt( 0, 0, pos, m_rc.Height(), &dcMem,
						0, 0, m_nChannelWidth, m_nChannelHeight, SRCCOPY);
	}
	else
	{
		if (m_nStyle <= slider_value)
		{
			dc.FillSolidRect(m_rc.left, cy - m_nTrackHeight / 2, pos - m_rc.left, m_nTrackHeight, enable_color(m_crActive));
			
			CPen	penDark(PS_SOLID, 1, get_color(enable_color(m_crActive), -64));
			CPen	penLight(PS_SOLID, 1, get_color(enable_color(m_crActive), 64));

			dc.SelectObject(&penDark);
			dc.MoveTo(pos, cy - m_nTrackHeight / 2);
			dc.LineTo(m_rc.left, cy - m_nTrackHeight / 2);
			dc.LineTo(m_rc.left, cy + m_nTrackHeight / 2);

			dc.SelectObject(&penLight);
			dc.MoveTo(m_rc.left + 1, cy + m_nTrackHeight / 2);
			dc.LineTo(pos, cy + m_nTrackHeight / 2);

			dc.SelectObject(pOldPen);
			penDark.DeleteObject();
			penLight.DeleteObject();
			
		}
		else if (m_nStyle == slider_progress)
		{
			CRect	rActive(0, m_rc.top + 2, pos, m_rc.bottom - 2);
			dc.FillSolidRect(rActive, enable_color(m_crActive));

			//m_crValueText = RGB(12, 162, 255);
			//m_crActive = RGB(128,255,128);
			if (m_nValueStyle > none)
			{
				dc.SetTextColor(enable_color(m_crValueText));
				dc.SetBkMode(TRANSPARENT);

				if (m_nValueStyle == value)
					str.Format(_T("%ld / %ld"), m_nPos, (m_nMax > 0 ? m_nMax : 0));
				else if (m_nMax == m_nMin)
					str = _T("0.0%");
				else
					str.Format(_T("%.1f%%"), (double)(m_nPos - m_nMin) / (double)(m_nMax - m_nMin) * 100.0);
				dc.DrawText(str, m_rc, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
			}

			//텍스트가 active, inactive 구간의 색상에 반전되도록 그려줘야한다.
			//텍스트를 반전시켜서 뿌려주는 기능이 없으므로
			//텍스트를 출력하고 그 위에 구간 색상을 반전시켜서 그려줘야 한다.
			//미완성.
			//dc.SetROP2(R2_XORPEN);
			//CBrush br((m_crActive));
			//CBrush *pOldBrush = (CBrush*)dc.SelectObject(&br);
			//dc.Rectangle(rActive);//, &CBrush(m_crActive));
			//dc.SelectObject(pOldBrush);
			//DrawRectangle(&dc, rActive, (m_crActive), (m_crActive), 1, PS_SOLID, R2_XORPEN);
		}
		else if (m_nStyle == slider_track)
		{
			CRect r = m_rc;
			int cy = r.CenterPoint().y;
			r.top = cy - 2;
			r.bottom = cy + 2;
			r.left += 2;
			r.right = pos;

			if (r.right > m_rc.right - 2)
				r.right = m_rc.right - 2;

			//CRect	rActive(0, m_rc.top + 2, pos, m_rc.bottom - 2);
			if (r.right > r.left)
				dc.FillSolidRect(r, enable_color(m_crActive));
		}
	}


	// 손잡이(thumb)를 그린다
	if (m_nStyle <= slider_value)
	{
		if (m_bThumb && !IsWindowEnabled())
		{
			if (m_bThumbActive && m_bLButtonDown)
				pbmTmp = dcMem.SelectObject(&m_bmThumbActive); // 활성화
			else
				pbmTmp = dcMem.SelectObject(&m_bmThumb); // 보통
	
	//		dc.BitBlt( pos - m_nThumbWidth / 2,
	//					m_nMarginTop + m_nChannelHeight / 2 - m_nThumbHeight / 2 - 1,
	//					m_nThumbWidth, m_nThumbHeight, &dcMem, 0, 0, SRCCOPY);

			DrawTransparentBitmap(&dc, pos - m_nThumbWidth / 2,
						m_nMarginTop + m_nChannelHeight / 2 - m_nThumbHeight / 2 + 1,
						m_nThumbWidth, m_nThumbHeight,
						&dcMem, 0, 0, &m_bmThumbMask);
		}
		else if (m_nStyle == slider_thumb)
		{
			CBrush br(enable_color(m_crThumb));

			int		n = 4;										//라인 개수
			double	dx = (double)m_nThumbWidth * 0.84 / double(n + 1);	//라인 간격
			double	sx = pos - 0.5 * double(n - 1) * dx;		//수직 라인 표시 시작 좌표
			CRect	rThumb = CRect(pos - THUMB_WIDTH, cy - 6, pos + THUMB_WIDTH, cy + 7);

			pOldPen = (CPen*)dc.SelectObject(&m_penThumbDarker);//NULL_PEN);
			pOldBrush = (CBrush*)dc.SelectObject(&br);

			dc.RoundRect(rThumb, CPoint(6, 6));

			for (i = 0; i < n; i++)
			{
				dc.SelectObject(&m_penThumbDarker);
				dc.MoveTo(sx + (double)i * dx + 1, cy - 3);
				dc.LineTo(sx + (double)i * dx + 1, cy + 4);

				dc.SelectObject(&m_penThumbLighter);
				dc.MoveTo(sx + (double)i * dx, cy - 3);
				dc.LineTo(sx + (double)i * dx, cy + 4);
			}

			dc.SelectObject(pOldPen);
			dc.SelectObject(pOldBrush);
			br.DeleteObject();
		}
		else if (m_nStyle == slider_value)
		{
			CBrush br(enable_color(m_crThumb));
			pOldBrush = (CBrush*)dc.SelectObject(&br);
			pOldPen = (CPen*)dc.SelectObject(&m_penThumbDarker);//NULL_PEN);

			CRect	rThumb = CRect(pos - m_nThumbWidth / 2, m_rc.top + 2, pos + m_nThumbWidth / 2, m_rc.bottom - 2);
			dc.RoundRect(rThumb, CPoint(6, 6));

			dc.SelectObject(pOldPen);
			dc.SelectObject(pOldBrush);
			br.DeleteObject();

			if (m_nValueStyle > none)
			{
				dc.SetTextColor(enable_color(m_crValueText));
				dc.SetBkMode(TRANSPARENT);

				if (m_nValueStyle == value)
					str.Format(_T("%ld / %ld"), m_nPos, (m_nMax > 0 ? m_nMax : 0));
				else if (m_nMax == m_nMin)
					str = _T("0.0%");
				else
					str.Format(_T("%.1f%%"), (double)(m_nPos - m_nMin) / (double)(m_nMax - m_nMin) * 100.0);

				//텍스트를 thumb에 출력하는데 그 너비가 작다면 늘려준다.
				int		margin = 12;
				int		minWidth = THUMB_WIDTH * 2;
				CRect	rText;
				dc.DrawText(str, rText, DT_CALCRECT);

				if (rText.Width() + margin > minWidth)
					minWidth = rText.Width() + margin;

				if (m_nThumbWidth != minWidth)
				{
					m_nThumbWidth = minWidth;

					rThumb = CRect(pos - m_nThumbWidth / 2, m_rc.top, pos + m_nThumbWidth / 2, m_rc.bottom);
					TRACE("%ld, invalidate\n", GetTickCount());
					Invalidate();
				}					
			}

			dc.DrawText(str, rThumb, DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_NOCLIP);
		}
	}
	//트랙의 북마크를 그린다.
	else if (m_nStyle == slider_track)
	{
		if (m_use_bookmark)
		{
			for (i = 0; i < m_bookmark.size(); i++)
			{
				pos = Pos2Pixel(m_bookmark[i].pos);
				//dc.FillSolidRect(pos-2, cy-8, 4, 4, (i == m_cur_bookmark ? m_crBookmarkCurrent : m_crBookmark));
				CPen pen(PS_SOLID, 1, (i == m_cur_bookmark ? enable_color(m_crBookmarkCurrent) : enable_color(m_crBookmark)));
				CPen* pOldPen = (CPen*)dc.SelectObject(&pen);
				dc.SetPixel(pos, cy - 5, (i == m_cur_bookmark ? enable_color(m_crBookmarkCurrent) : enable_color(m_crBookmark)));
				for (int j = 1; j < 5; j++)
				{
					dc.MoveTo(pos - j, cy - 5 - j);
					dc.LineTo(pos + j + 1, cy - 5 - j);	//역삼각형
					//dc.LineTo(pos + 1, cy - 5 - j);		//역직각삼각형
				}
				dc.SelectObject(pOldPen);
			}
		}

		dc.SetBkMode(TRANSPARENT);
		dc.SetTextColor(RGB(0,255,0));

		LOGFONT lf;
		font.GetLogFont(&lf);
		lf.lfHeight = -8;
		lf.lfWeight = FW_ULTRABOLD;
		//lf.lfQuality = ANTIALIASED_QUALITY;
		lf.lfWidth = 8;
		font.SetLogFont(lf);
		dc.SelectObject(&font);
		if (m_repeat_start >= 0)
		{
			pos = Pos2Pixel(m_repeat_start);
			dc.DrawText(_T("["), CRect(pos, m_rc.top-2, pos, m_rc.bottom),
						DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_NOCLIP);
			//DrawShadowText(dc.m_hDC, _T("["), 1, CRect(pos, m_rc.top-2, pos, m_rc.bottom),
			//	DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_NOCLIP, RGB(0,255,0),
			//	RGB(0,0,0), 4, 4);
		}
		if (m_repeat_end >= 0)
		{
			pos = Pos2Pixel(m_repeat_end);
			dc.DrawText(_T("]"), CRect(pos, m_rc.top-2, pos, m_rc.bottom),
				DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_NOCLIP);
		}
	}

	// 포커스 사각형을 그린다
	if(m_bDrawFocusRect && m_bHasFocus && !IsWindowEnabled())
	{
		dc.DrawFocusRect(m_rc);
	}

	dc.SelectObject(pOldFont);
	dc.SelectObject(pOldPen);

	// Do not call CSliderCtrl::OnPaint() for painting messages
}

BOOL CSliderCtrlEx::OnEraseBkgnd(CDC* pDC) 
{
	// TODO: Add your message handler code here and/or call default
	return FALSE;
	
	return CSliderCtrl::OnEraseBkgnd(pDC);
}

void CSliderCtrlEx::SetRange(int nMin, int nMax, BOOL bRedraw)
{
	m_nMin = nMin;
	m_nMax = nMax;

	if (m_nMax < m_nMin)
		m_nMax = m_nMin;

	CSliderCtrl::SetRange(nMin, nMax, bRedraw);
}

BOOL CSliderCtrlEx::SetBitmapChannel(UINT nChannelID, UINT nActiveID)
{
	// 비트맵의 ID가 정해지지 않았을 때
	if(!nChannelID)
	{
		m_bChannel = FALSE;
		m_bmChannel.DeleteObject();
		m_bmChannelMask.DeleteObject();
		m_bmChannelActive.DeleteObject();
		m_bmChannelActiveMask.DeleteObject();
	
		return TRUE;
	}

	// 비트맵을 불러온다
	m_bmChannel.DeleteObject();

	if(!m_bmChannel.LoadBitmap(nChannelID))
		return FALSE;

	// 비트맵의 크기 정보를 가져온다.
	BITMAP	bitmap;
	m_bmChannel.GetBitmap(&bitmap);

	m_nChannelWidth = bitmap.bmWidth;
	m_nChannelHeight = bitmap.bmHeight;

	// 활성화 배경 비트맵을 불러온다.
	if(nActiveID)
	{
		m_bmChannelActive.DeleteObject();
		if(!m_bmChannelActive.LoadBitmap(nActiveID))
		{
		}
		m_bChannelActive = TRUE;
	}
	else
		m_bChannelActive = FALSE;

/*	// 크기를 비교한다
	if(m_bChannelActive) {

		BITMAP	bitmap;
		m_bmChannelActive.GetBitmap(&bitmap);

		ASSERT(m_nWidth == bitmap.bmWidth && m_nHeight == bitmap.bmHeight);
	}
*/

	GetClientRect(&m_rc);

//	m_bTransparentChannel = bTransparent;
	m_bChannel = TRUE;

	return TRUE;
}

BOOL CSliderCtrlEx::SetBitmapThumb(UINT nThumbID, UINT nActiveID, BOOL bTrans, COLORREF crTrans)
{
	// 손잡이의 ID값이 NULL일때
	if(!nThumbID)
	{
		m_bThumb = FALSE;
		m_bmThumb.DeleteObject();
		m_bmThumbMask.DeleteObject();
		m_bmThumbActive.DeleteObject();
		m_bmThumbActiveMask.DeleteObject();
		m_bmThumbBg.DeleteObject();

		return TRUE;
	}

	// 비트맵을 불러온다
	m_bmThumb.DeleteObject();

	if(!m_bmThumb.LoadBitmap(nThumbID))
		return FALSE;

	// 투명한 비트맵을 그리기 위해 마스크를 만든다.
	if(bTrans) {

		PrepareMask(&m_bmThumb, &m_bmThumbMask, crTrans);
	}

	// 활성화 상태를 위한 비트맵을 불러온다.
	if(nActiveID) {

		m_bmThumbActive.DeleteObject();

		if(!m_bmThumbActive.LoadBitmap(nActiveID)) {

			m_bmThumb.DeleteObject();
			//if(bTransparent)
			//	m_bmThumbMask.DeleteObject();

			return FALSE;
		}

/*		if(bTransparent) {

			PrepareMask(&m_bmThumbActive, &m_bmThumbActiveMask,
				clrpTransColor, iTransPixelX, iTransPixelY);
		}
*/		
		m_bThumbActive = TRUE;

	// 활성화 상태의 비트맵이 존재하지 않을 때.
	} else
		m_bThumbActive = FALSE;

	// 비트맵의 크기 정보를 가져온다.
	BITMAP	bitmap;
	m_bmThumb.GetBitmap(&bitmap);

	m_nThumbWidth = bitmap.bmWidth;
	m_nThumbHeight = bitmap.bmHeight;

	// 배경을 위한 비트맵이 없으면 컨트롤의 사이즈를 기억한다.
	if(!m_bChannel)
	{
		GetClientRect(&m_rc);
		m_nChannelWidth = m_rc.Width();
		m_nChannelHeight = m_rc.Height();
	}

	ASSERT(m_nThumbWidth <= m_nChannelWidth && m_nThumbHeight <= m_nChannelHeight);

	// 크기를 비교한다
	if(m_bThumbActive) {

		BITMAP	bitmap;
		m_bmThumbActive.GetBitmap(&bitmap);

		ASSERT(
			m_nThumbWidth == bitmap.bmWidth &&
			m_nThumbHeight == bitmap.bmHeight);
	}

	// 속성값을 설정한다
	m_bTransparentThumb = 0;//bTransparent;
	m_bThumb = TRUE;

	return TRUE;
}

// 컨트롤의 위치값에 해당하는 손잡이 중심의 좌표값을 구한다
//
int CSliderCtrlEx::Pos2Pixel(int nPos)
{
	if (m_nMax == m_nMin)
		return (m_nStyle <= slider_value ? m_nThumbWidth / 2 : 0);

	if (m_bVertical)
	{
		return
			m_nMarginTop + m_nThumbHeight/2 +
			(int)(
			(double)(m_rc.Height() - m_nMarginTop - m_nMarginBottom - m_nThumbHeight) *
			((double)(nPos - m_nMin) / (double)(m_nMax - m_nMin))
			);

	}
	else
	{
		return (int)(
			(double)(m_rc.Width() - m_nMarginLeft - m_nMarginRight - (m_nStyle <= slider_value ? m_nThumbWidth : 0)) *
			((double)(nPos - m_nMin) / (double)(m_nMax - m_nMin))
			) + m_nMarginLeft + (m_nStyle <= slider_value ? m_nThumbWidth/2 : 0);
	}
}

// 마우스의 좌표값에 해당하는 컨트롤의 위치값을 구한다
//
double CSliderCtrlEx::Pixel2Pos(int nPixel)
{
	if(m_bVertical)
	{
		return (
			m_nMin +
			(double)(nPixel - m_nMarginTop - (double)m_nThumbHeight/2.0) /
			(double)(m_rc.Height() - m_nMarginBottom - m_nMarginTop - (m_nStyle <= slider_value ? m_nThumbHeight : 0)) *
			(double)(m_nMax - m_nMin) + 0.5
			);

	}
	else
	{
		return (
			m_nMin +
			(double)(nPixel - m_nMarginLeft - (m_nStyle <= slider_value ? (double)m_nThumbWidth/2.0 : 0)) /
			(double)(m_rc.Width() - m_nMarginLeft - m_nMarginRight - (m_nStyle <= slider_value ? m_nThumbWidth : 0)) *
			(double)(m_nMax - m_nMin + 1) + 0.0
			);
		//TRACE("d = %f\n", d);
		//return (int)d;
	}
}

int	CSliderCtrlEx::GetPos()
{
	return m_nPos;
}

void CSliderCtrlEx::SetPos(int nPos)
{
	m_nPos = nPos;

	if (m_nPos < m_nMin)
		m_nPos = m_nMin;
	else if (m_nPos > m_nMax)
		m_nPos = m_nMax;

	Invalidate();
}

void CSliderCtrlEx::OnLButtonDown(UINT nFlags, CPoint point) 
{
	// TODO: Add your message handler code here and/or call default
	if (m_bEnableSlide == false)
		return;

	if (m_use_bookmark && (m_cur_bookmark >= 0))
	{
		SetPos(m_bookmark[m_cur_bookmark].pos);
		::SendMessage(GetParent()->GetSafeHwnd(),	MESSAGE_SLIDERCTRLEX, (WPARAM)&CSliderCtrlExMsg(CSliderCtrlExMsg::msg_thumb_move, GetDlgCtrlID(), m_nPos), 0);
		return;
	}

	SetCapture();
	SetFocus();

	m_bLButtonDown = TRUE;

	// 손잡이를 마우스로 클릭했을 때
	// 마우스의 좌표와 손잡이 중심의 좌표를 비교하여
	// 둘 사이의 거리를 구해둔다
	if(m_bVertical)
	{
		if(abs(point.y - Pos2Pixel(m_nPos)) <= (m_nStyle <= slider_value ? m_nThumbHeight/2.0 : 0))
			m_nMouseOffset = point.y - Pos2Pixel(m_nPos);
		else
			m_nMouseOffset = 0;

	}
	else
	{
		/*
		if (m_nStyle == slider_track && !m_enable_bottom_slide)
		{
			if (point.y > m_rc.CenterPoint().y + 2)
			{
				ReleaseCapture();
				m_bLButtonDown = false;
				::SendMessage(GetParent()->GetSafeHwnd(),	WM_LBUTTONDOWN,
								MK_LBUTTON, MAKELPARAM(point.x, point.y));
				return;
			}
		}
		*/
		if(abs(point.x - Pos2Pixel(m_nPos)) <= (m_nStyle <= slider_value ? m_nThumbWidth/2.0 : 0))
			m_nMouseOffset = point.x - Pos2Pixel(m_nPos);
		else
			m_nMouseOffset = 0;
	}

	if (m_nEventMsgStyle == msg_style_timer)
	{
		//CSliderCtrlExMsg msg(CSliderCtrlExMsg::msg_thumb_grab, GetDlgCtrlID(), m_nPos);
		::SendMessage(GetParent()->GetSafeHwnd(),	MESSAGE_SLIDERCTRLEX, (WPARAM)&CSliderCtrlExMsg(CSliderCtrlExMsg::msg_thumb_grab, GetDlgCtrlID(), m_nPos), 0);
	}
	else if (m_nEventMsgStyle == msg_style_post)
	{
		::PostMessage(GetParent()->GetSafeHwnd(),	MESSAGE_SLIDERCTRLEX, (WPARAM)&CSliderCtrlExMsg(CSliderCtrlExMsg::msg_thumb_grab, GetDlgCtrlID(), m_nPos), 0);
	}
	else if (m_nEventMsgStyle == msg_style_callback)
	{
		//if (m_pCallback_func != NULL)
			//(*(m_pCallback_func))(m_pParentWnd, this, MESSAGE_SLIDERCTRLEX_THUMB_GRAB, m_nPos);
	}

	OnMouseMove(nFlags, point);
	
	CSliderCtrl::OnLButtonDown(nFlags, point);
}

void CSliderCtrlEx::OnLButtonUp(UINT nFlags, CPoint point) 
{
	// TODO: Add your message handler code here and/or call default
	if (m_bEnableSlide == false)
		return;

	if(!IsWindowEnabled() || !m_bLButtonDown)
		return;

	ReleaseCapture();
	m_bLButtonDown = FALSE;

	if (m_nEventMsgStyle == msg_style_timer)
	{
		::SendMessage(GetParent()->GetSafeHwnd(),	MESSAGE_SLIDERCTRLEX, (WPARAM)&CSliderCtrlExMsg(CSliderCtrlExMsg::msg_thumb_release, GetDlgCtrlID(), m_nPos), 0);
	}
	else if (m_nEventMsgStyle == msg_style_post)
	{
		::PostMessage(GetParent()->GetSafeHwnd(),	MESSAGE_SLIDERCTRLEX, (WPARAM)&CSliderCtrlExMsg(CSliderCtrlExMsg::msg_thumb_release, GetDlgCtrlID(), m_nPos), 0);
	}
	else if (m_nEventMsgStyle == msg_style_callback)
	{
		//if (m_pCallback_func != NULL)
			//(*(m_pCallback_func))(m_pParentWnd, this, MESSAGE_SLIDERCTRLEX_THUMB_RELEASE, m_nPos);
	}

	
	CSliderCtrl::OnLButtonUp(nFlags, point);
}

void CSliderCtrlEx::OnMouseMove(UINT nFlags, CPoint point) 
{
	// TODO: Add your message handler code here and/or call default
	CString str;

	if (m_use_bookmark)
	{
		int i;
		int pos[2];
		int found = false;

		if (point.y < m_rc.CenterPoint().y + 4)
		{
			pos[0] = Pixel2Pos(point.x - 4);
			pos[1] = Pixel2Pos(point.x + 4);
			//TRACE(_T("0:%f, 1:%f\n"), pos[0], pos[1]);
			for (i = 0; i < m_bookmark.size(); i++)
			{
				//어떤 북마크 위치가 현재 마우스의 근처라면
				if (m_bookmark[i].pos >= pos[0] && m_bookmark[i].pos <= pos[1])
				{
					m_cur_bookmark = i;
					found = true;
				}
			}
		}
		
		if (!found)
		{
			m_cur_bookmark = -1;
		}
		else
		{
			str.Format(_T("%s (%s)"), GetTimeStringFromMilliSeconds(m_bookmark[m_cur_bookmark].pos, true, false), m_bookmark[m_cur_bookmark].name);
			if (m_ToolTip.m_hWnd)
				m_ToolTip.UpdateTipText(str, this);
		}
		
		Invalidate();
	}

	if (m_use_tooltip && m_cur_bookmark == -1)
	{
		int pos = Pixel2Pos(point.x);

		if (m_tooltip_format == tooltip_time)
			str.Format(_T("%s"), GetTimeStringFromMilliSeconds(pos, true, false));
		else if (m_tooltip_format == tooltip_time_ms)
			str.Format(_T("%s"), GetTimeStringFromMilliSeconds(pos));
		else//if (m_tooltip_format == tooltip_value)
			str.Format(_T("%d / %d"), pos, m_nMax);

		if (m_ToolTip.m_hWnd)
			m_ToolTip.UpdateTipText(str, this);
	}


	if(!m_bLButtonDown || !IsWindowEnabled())
		return;

	int nPixel;

	// 범위를 벗어났는지 검사한다
	if(m_bVertical)
	{
		nPixel = point.y - m_nMouseOffset;

		if(nPixel > m_rc.Height() - m_nMarginBottom - (m_nStyle <= slider_value ? m_nThumbHeight/2 : 0))
			nPixel = m_rc.Height() - m_nMarginBottom - (m_nStyle<= slider_value ? m_nThumbHeight/2 : 0);

		if(nPixel < m_nMarginTop + (m_nStyle <= slider_value ? m_nThumbHeight/2 : 0))
			nPixel = m_nMarginTop + (m_nStyle <= slider_value ? m_nThumbHeight/2 : 0);
	}
	else
	{
		nPixel = point.x - m_nMouseOffset;

		if(nPixel < m_nMarginLeft + (m_nStyle <= slider_value ? m_nThumbWidth/2 : 0))
			nPixel = m_nMarginLeft + (m_nStyle <= slider_value ? m_nThumbWidth/2 : 0);

		if(nPixel > m_rc.Width() - m_nMarginRight - (m_nStyle <= slider_value ? m_nThumbWidth/2 : 0))
			nPixel = m_rc.Width() - m_nMarginRight - (m_nStyle <= slider_value ? m_nThumbWidth/2 : 0);
	}

	// 변한 내용을 적용한다
	if(Pos2Pixel(m_nPos) != nPixel)
	{
		SetPos(Pixel2Pos(nPixel));

		if (m_nEventMsgStyle == msg_style_timer)
		{
			SetTimer(timer_post_pos, 1, NULL);
		}
		else if (m_nEventMsgStyle == msg_style_post)
		{
			::PostMessage(GetParent()->GetSafeHwnd(),	MESSAGE_SLIDERCTRLEX, (WPARAM)&CSliderCtrlExMsg(CSliderCtrlExMsg::msg_thumb_move, GetDlgCtrlID(), m_nPos), 0);
		}
		else if (m_nEventMsgStyle == msg_style_callback)
		{
			//if (m_pCallback_func != NULL)
				//(*(m_pCallback_func))(m_pParentWnd, this, MESSAGE_SLIDERCTRLEX_MOVED, m_nPos);
		}
	}

	//CSliderCtrl::OnMouseMove(nFlags, point);
}

// PrepareMask
//
// "Drawing Transparent Bitmap with ease with on the fly masks in MFC"
// By Raja Segar
//
// clrpTransColor의 기본값을 NULL(black)에서
// 0xFF000000(RGB 값이 아닌 값)으로 바꿨습니다.
//
void CSliderCtrlEx::PrepareMask(CBitmap *pBmpSource, CBitmap *pBmpMask,
								 COLORREF clrpTransColor)
{
	BITMAP bm;

	// Get the dimensions of the source bitmap
	pBmpSource->GetObject(sizeof(BITMAP), &bm);

	// Create the mask bitmap
	pBmpMask->DeleteObject();
	pBmpMask->CreateBitmap(bm.bmWidth, bm.bmHeight, 1, 1, NULL);

	// We will need two DCs to work with. One to hold the Image
	// (the source), and one to hold the mask (destination).
	// When blitting onto a monochrome bitmap from a color, pixels
	// in the source color bitmap that are equal to the background
	// color are blitted as white. All the remaining pixels are
	// blitted as black.

	CDC hdcSrc, hdcDst;

	hdcSrc.CreateCompatibleDC(NULL);
	hdcDst.CreateCompatibleDC(NULL);

	// Load the bitmaps into memory DC
	CBitmap* hbmSrcT = (CBitmap*) hdcSrc.SelectObject(pBmpSource);
	CBitmap* hbmDstT = (CBitmap*) hdcDst.SelectObject(pBmpMask);

	// Dynamically get the transparent color
	COLORREF clrTrans = clrpTransColor;

	// Change the background to trans color
	COLORREF clrSaveBk  = hdcSrc.SetBkColor(clrTrans);

	// This call sets up the mask bitmap.
	hdcDst.BitBlt(0,0,bm.bmWidth, bm.bmHeight, &hdcSrc,0,0,SRCCOPY);

	// Now, we need to paint onto the original image, making
	// sure that the "transparent" area is set to black. What
	// we do is AND the monochrome image onto the color Image
	// first. When blitting from mono to color, the monochrome
	// pixel is first transformed as follows:
	// if  1 (black) it is mapped to the color set by SetTextColor().
	// if  0 (white) is is mapped to the color set by SetBkColor().
	// Only then is the raster operation performed.

	COLORREF clrSaveDstText = hdcSrc.SetTextColor(RGB(255,255,255));
	hdcSrc.SetBkColor(RGB(0,0,0));

	hdcSrc.BitBlt(0,0,bm.bmWidth, bm.bmHeight, &hdcDst,0,0,SRCAND);

	// Clean up by deselecting any objects, and delete the
	// DC's.
	hdcDst.SetTextColor(clrSaveDstText);

	hdcSrc.SetBkColor(clrSaveBk);
	hdcSrc.SelectObject(hbmSrcT);
	hdcDst.SelectObject(hbmDstT);

	hdcSrc.DeleteDC();
	hdcDst.DeleteDC();
}

// DrawTransparentBitmap
//
// "Drawing Transparent Bitmap with ease with on the fly masks in MFC"
// By Raja Segar
//
void CSliderCtrlEx::DrawTransparentBitmap(
	CDC *pDC, int xStart, int yStart, int wWidth, int wHeight,
	CDC *pTmpDC, int xSource, int ySource, CBitmap *bmMask)
{
	// We are going to paint the two DDB's in sequence to the destination.
	// 1st the monochrome bitmap will be blitted using an AND operation to
	// cut a hole in the destination. The color image will then be ORed
	// with the destination, filling it into the hole, but leaving the
	// surrounding area untouched.

	CDC hdcMem;
	hdcMem.CreateCompatibleDC(NULL);

	CBitmap* hbmT = hdcMem.SelectObject(bmMask);

	pDC->BitBlt(xStart, yStart, wWidth, wHeight, &hdcMem,
		xSource, ySource, SRCAND);

	// Also note the use of SRCPAINT rather than SRCCOPY.

	pDC->BitBlt(xStart, yStart, wWidth, wHeight, pTmpDC,
		xSource, ySource,SRCPAINT);

	// Now, clean up.
	hdcMem.SelectObject(hbmT);
	hdcMem.DeleteDC();
}

void CSliderCtrlEx::DrawFocusRect(BOOL bDraw, BOOL bRedraw)
{
	m_bDrawFocusRect = bDraw;

	if(bRedraw)
		Invalidate();
}

void CSliderCtrlEx::OnSetFocus(CWnd* pOldWnd) 
{
	CSliderCtrl::OnSetFocus(pOldWnd);
	
	// TODO: Add your message handler code here
	m_bHasFocus = TRUE;
	Invalidate();
	//TRACE("SetFocus\n");
}

void CSliderCtrlEx::OnKillFocus(CWnd* pNewWnd) 
{
	CSliderCtrl::OnKillFocus(pNewWnd);
	
	// TODO: Add your message handler code here
	m_bHasFocus = FALSE;
	Invalidate();
	//TRACE("KillFocus\n");
}

void CSliderCtrlEx::SetBackColor(COLORREF crBack)
{
	m_crBack = crBack;
	Invalidate();
}

void CSliderCtrlEx::SetActiveColor(COLORREF crActive)
{
	m_crActive = crActive;
	Invalidate();
}

void CSliderCtrlEx::SetInActiveColor(COLORREF crInActive)
{
	m_crInActive = crInActive;
	Invalidate();
}

void CSliderCtrlEx::SetThumbColor(COLORREF crThumb)
{
	int nMultiple = 32;

	m_crThumb		= crThumb;
	m_crThumbLight	= get_color(m_crThumb, nMultiple);
	m_crThumbLighter= get_color(m_crThumb, nMultiple * 2);
	m_crThumbDark	= get_color(m_crThumb, -nMultiple);
	m_crThumbDarker = get_color(m_crThumb, -nMultiple * 2);

	m_penThumb.DeleteObject();
	m_penThumbLight.DeleteObject();
	m_penThumbLighter.DeleteObject();
	m_penThumbDark.DeleteObject();
	m_penThumbDarker.DeleteObject();

	m_penThumb.CreatePen(PS_SOLID, 1, m_crThumb);
	m_penThumbLight.CreatePen(PS_SOLID, 1, m_crThumbLight);
	m_penThumbLighter.CreatePen(PS_SOLID, 1, m_crThumbLighter);
	m_penThumbDark.CreatePen(PS_SOLID, 1, m_crThumbDark);
	m_penThumbDarker.CreatePen(PS_SOLID, 1, m_crThumbDarker);
}


BOOL CSliderCtrlEx::PreTranslateMessage(MSG* pMsg)
{
	if (m_use_tooltip)
		m_ToolTip.RelayEvent(pMsg);
	// TODO: Add your specialized code here and/or call the base class 
	if (pMsg->wParam == WM_KEYDOWN)
	{
		return false;
	}

	return CSliderCtrl::PreTranslateMessage(pMsg);
}

/*
void CSliderCtrlEx::OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags)
{
	// TODO: Add your message handler code here and/or call default
	TRACE(_T("CSliderCtrlEx::OnKeyDown = %d\n"), nChar);
	if (m_bEnableSlide == false)
		return;

	switch (nChar)
	{
		case VK_LEFT :	m_nPos = GetPos() - 1;
						if (m_nPos < m_nMin)
							m_nPos = m_nMin;
						SetPos(m_nPos);
						if (m_nEventMsgStyle == msg_style_timer)
						{
							SetTimer(timer_post_pos, 1, NULL);
						}
						else if (m_nEventMsgStyle == msg_style_post)
						{
							::PostMessage(GetParent()->GetSafeHwnd(),	MESSAGE_SLIDERCTRLEX, (WPARAM)&CSliderCtrlExMsg(CSliderCtrlExMsg::msg_thumb_move, GetDlgCtrlID(), m_nPos), 0);
						}
						else if (m_nEventMsgStyle == msg_style_callback)
						{
							//if (m_pCallback_func != NULL)
								//(*(m_pCallback_func))(m_pParentWnd, this, MESSAGE_SLIDERCTRLEX, (WPARAM)&CSliderCtrlMsg(CSliderCtrlMsg::msg_thumb_move, GetDlgCtrlID(), m_nPos), 0);
						}
						break;
		case VK_RIGHT :	m_nPos = GetPos() + 1;
						if (m_nPos > m_nMax)
							m_nPos = m_nMax;
						SetPos(m_nPos);
						if (m_nEventMsgStyle == msg_style_timer)
						{
							SetTimer(timer_post_pos, 1, NULL);
						}
						else if (m_nEventMsgStyle == msg_style_post)
						{
							::PostMessage(GetParent()->GetSafeHwnd(),	MESSAGE_SLIDERCTRLEX, (WPARAM)&CSliderCtrlExMsg(CSliderCtrlExMsg::msg_thumb_move, GetDlgCtrlID(), m_nPos), 0);
						}
						else if (m_nEventMsgStyle == msg_style_callback)
						{
							//if (m_pCallback_func != NULL)
								//(*(m_pCallback_func))(m_pParentWnd, this, MESSAGE_SLIDERCTRLEX_MOVED, m_nPos);
						}
						break;
		default :		::PostMessage(GetParent()->GetSafeHwnd(), WM_KEYDOWN, nChar, 0);
						return;
	}
	
	CSliderCtrl::OnKeyDown(nChar, nRepCnt, nFlags);
}
*/

void CSliderCtrlEx::SetStyle(int nStyle)
{
	m_nStyle = nStyle;

	if (m_nStyle == slider_thumb)
		m_nThumbWidth = THUMB_WIDTH * 2;
	else if (m_nStyle == slider_value)
		m_nThumbWidth = THUMB_WIDTH * 4;
	else if (m_nStyle == slider_progress)
		m_bEnableSlide = false;
}


void CSliderCtrlEx::OnTimer(UINT_PTR nIDEvent)
{
	// TODO: Add your message handler code here and/or call default
	if (nIDEvent == timer_post_pos)
	{
		KillTimer(timer_post_pos);
		::SendMessage(GetParent()->GetSafeHwnd(), MESSAGE_SLIDERCTRLEX, (WPARAM)&CSliderCtrlExMsg(CSliderCtrlExMsg::msg_thumb_move, GetDlgCtrlID(), m_nPos), 0);
	}

	CSliderCtrl::OnTimer(nIDEvent);
}

void CSliderCtrlEx::SetEventMsgStyle(int style)
{
	if (style < msg_style_timer || style > msg_style_callback)
		return;

	m_nEventMsgStyle = style;
	AfxGetApp()->WriteProfileInt(_T("setting"), _T("event msg style"), m_nEventMsgStyle);
}


void CSliderCtrlEx::OnSize(UINT nType, int cx, int cy)
{
	CSliderCtrl::OnSize(nType, cx, cy);

	// TODO: Add your message handler code here
	//Invalidate(true);
}

//pos의 위치에 있는 북마크의 인덱스를 리턴한다.
int CSliderCtrlEx::find_index_bookmark(int pos)
{
	int i;

	for (i = 0; i < m_bookmark.size(); i++)
	{
		if (m_bookmark[i].pos == pos)
			return i;
	}

	return -1;
}

void CSliderCtrlEx::bookmark(int mode, int pos, CString name)
{
	if (!m_use_bookmark)
		return;

	int index;

	if (mode == bookmark_add_current || mode == bookmark_delete_current)
	{
		pos = GetPos();
	}
	else if (mode == bookmark_reset)
	{
		m_bookmark.clear();
		Invalidate();
		return;
	}

	index = find_index_bookmark(pos);

	if (index < 0 && mode <= bookmark_add)
	{
		if (name.IsEmpty())
			name.Format(_T("bookmark%02d"), m_bookmark.size());
		m_bookmark.push_back(CSliderCtrlExBookmark(pos, name));
	}
	else if (mode <= bookmark_delete)
	{
		m_bookmark.erase(m_bookmark.begin() + index);
	}
	else if (mode == bookmark_move)
	{
		index = get_near_bookmark(GetPos(), pos > 0);
		if (index >= 0)
		{
			SetPos(m_bookmark[index].pos);
			::SendMessage(GetParent()->GetSafeHwnd(),	MESSAGE_SLIDERCTRLEX, (WPARAM)&CSliderCtrlExMsg(CSliderCtrlExMsg::msg_thumb_move, GetDlgCtrlID(), m_nPos), 0);
		}
	}

	std::sort(m_bookmark.begin(), m_bookmark.end(),
			[](CSliderCtrlExBookmark a, CSliderCtrlExBookmark b)
	{
		return (a.pos < b.pos);
	}
	);
	Invalidate();
}

int CSliderCtrlEx::get_near_bookmark(int pos, bool forward)
{
	int i;
	int index = -1;

	if (m_bookmark.size() == 1)
	{
		if ((forward && (pos < m_bookmark[0].pos)) ||
			(!forward && (pos > m_bookmark[0].pos)))
			return 0;
	}

	for (i = 0; i < m_bookmark.size() - 1; i++)
	{
		//이전 북마크로 이동할 경우는 약 3초 정도 시간이 흘러도 그 이전 북마크로 이동시켜줘야 한다.
		if ((pos >= m_bookmark[i].pos + (forward ? 0 : 3000)) && (pos <= m_bookmark[i+1].pos + (forward ? 0 : 3000)))
		{
			if (forward)
			{
				//0부터 검사하므로 pos가 i+1일 경우는 i+2를 리턴해줘야 다음 북마크 위치로 이동된다.
				if (pos == m_bookmark[i+1].pos)
				{
					if (i + 2 < m_bookmark.size())
						return i + 2;
					else
						return -1;
				}
				return i + 1;
			}
			else
			{
				return i;
			}
		}
	}

	//맨 마지막 북마크보다 큰 위치에서 backward를 누른 경우
	if ((m_bookmark.size() >= 1) && (pos >= m_bookmark[m_bookmark.size() - 1].pos))
		return m_bookmark.size() - 1;

	return index;
}

void CSliderCtrlEx::set_bookmark_color(COLORREF cr)
{
	m_crBookmark = cr;
	Invalidate();
}

void CSliderCtrlEx::set_bookmark_current_color(COLORREF cr)
{
	m_crBookmarkCurrent = cr;
	Invalidate();
}

void CSliderCtrlEx::PreSubclassWindow()
{
	// TODO: Add your specialized code here and/or call the base class
	m_ToolTip.Create(this, TTS_ALWAYSTIP | TTS_NOPREFIX | TTS_NOANIMATE);
	m_ToolTip.SetDelayTime(TTDT_AUTOPOP, -1);
	m_ToolTip.SetDelayTime(TTDT_INITIAL, 0);
	m_ToolTip.SetDelayTime(TTDT_RESHOW, 0);
	m_ToolTip.SetMaxTipWidth(400); 
	m_ToolTip.AddTool(this, _T(""));
	m_ToolTip.Activate(TRUE);

	CSliderCtrl::PreSubclassWindow();
}


void CSliderCtrlEx::OnWindowPosChanged(WINDOWPOS* lpwndpos)
{
	CSliderCtrl::OnWindowPosChanged(lpwndpos);

	// TODO: Add your message handler code here
	Invalidate();
}

COLORREF CSliderCtrlEx::enable_color(COLORREF cr, int offset)
{
	if (IsWindowEnabled())
		return cr;
	return get_color(gray_color(cr), offset);
}

void CSliderCtrlEx::set_repeat_range(int start, int end)
{
	set_repeat_start(start);
	set_repeat_end(end);
}

//pos가 0이상이면 그 트랙 위치를 start 또는 end로 설정하고
//-2이면 현재 위치를, -1면 해제의 의미로 처리한다.
void CSliderCtrlEx::set_repeat_start(int pos)
{
	if (pos == -2)
	{
		m_repeat_start = m_nPos;
	}
	else if (pos == -1)
	{
		m_repeat_start = -1;
	}
	else
	{
		m_repeat_start = pos;
	}

	//시작과 끝이 역순이 되면 end를 끝으로 변경
	if ((m_repeat_start >= 0) &&
		(m_repeat_end >= 0) &&
		(m_repeat_start >= m_repeat_end))
		m_repeat_end = m_nMax;

	Invalidate();
}

//pos가 0이상이면 그 트랙 위치를 start 또는 end로 설정하고
//-2이면 현재 위치를, -1면 해제의 의미로 처리한다.
void CSliderCtrlEx::set_repeat_end(int pos)
{
	if (pos == -2)
	{
		m_repeat_end = m_nPos;
	}
	else if (pos == -1)
	{
		m_repeat_end = -1;
	}
	else
	{
		m_repeat_end = pos;
	}

	//시작과 끝이 역순이 되면 start를 시작으로 변경
	if ((m_repeat_start >= 0) &&
		(m_repeat_end >= 0) &&
		(m_repeat_end <= m_repeat_start))
		m_repeat_start = m_nMin;

	Invalidate();
}


