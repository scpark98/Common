// SCSliderCtrl.cpp : implementation file
//

//#include "stdafx.h"
#include "SCSliderCtrl.h"

#include "../../Functions.h"
#include "../../MemoryDC.h"
#include "../../AutoFont.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CSCSliderCtrl

CSCSliderCtrl::CSCSliderCtrl()
{
	m_pCallback_func = NULL;

	m_style = slider_thumb;
	set_style(m_style);

	m_nEventMsgStyle = AfxGetApp()->GetProfileInt(_T("setting"), _T("event msg style"), msg_style_timer);

	m_enable_slide	= true;
	m_use_bookmark = false;
	m_cur_bookmark = -1;

	m_use_tooltip = false;
	m_tooltip_format = tooltip_value;

	m_margin = CRect(0, 0, 0, 0);

	m_track_height = 5;

	m_draw_focus_rect = false;

	m_nValueStyle = value;
	m_crValueText = RGB(32, 32, 32);

	m_cr_back	= ::GetSysColor(COLOR_3DFACE);
	m_cr_active	= RGB(64, 80, 181);//RGB(128, 192, 255);
	m_cr_inactive= get_color(m_cr_back, -64);
	m_cr_thumb	= RGB(64, 80, 181); //RGB(124, 192, 232);

	//북마크 컬러는 처음에만 배경의 보색으로 설정되지만
	//차후 배경이 바뀌더라도 북마크의 색상까지 자동으로 보색으로 바꾸는 것은 좋지 않다.
	m_crBookmark = color_complementary(m_cr_back);
	m_crBookmarkCurrent = green;

	m_repeat_start = -1;
	m_repeat_end = -1;

	set_thumb_color(m_cr_thumb);
}

CSCSliderCtrl::~CSCSliderCtrl()
{
	m_penThumb.DeleteObject();
	m_penThumbLight.DeleteObject();
	m_penThumbLighter.DeleteObject();
	m_penThumbDark.DeleteObject();
	m_penThumbDarker.DeleteObject();
}


BEGIN_MESSAGE_MAP(CSCSliderCtrl, CSliderCtrl)
	//{{AFX_MSG_MAP(CSCSliderCtrl)
	ON_WM_PAINT()
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
// CSCSliderCtrl message handlers

void CSCSliderCtrl::OnPaint() 
{
	if (m_style == slider_normal)
	{
		CSliderCtrl::OnPaint();
		return;
	}

	int lower = GetRangeMin();
	int upper = GetRangeMax();
	int pos = GetPos();
	TRACE(_T("pos = %d\n"), pos);

	CPaintDC	dc1(this); // device context for painting
	CPen*		pOldPen = NULL;
	CBrush*		pOldBrush = NULL;
	CString		str;

	// TODO: Add your message handler code here
	GetClientRect(m_rc);
	CMemoryDC	dc(&dc1, &m_rc);

	Gdiplus::Graphics g(dc.m_hDC);
	g.SetSmoothingMode(Gdiplus::SmoothingMode::SmoothingModeAntiAlias);
	g.SetInterpolationMode(Gdiplus::InterpolationModeHighQualityBicubic);

	CDC		dcMem;
	dcMem.CreateCompatibleDC(&dc);

	CAutoFont font(_T("굴림"));
	font.SetHeight((double)get_logical_size_from_font_size(m_hWnd, m_rc.Height()) / 2.2);
	font.SetBold(true);
	CFont* pOldFont = (CFont*)dc.SelectObject(&font);

	//실제 그려지는 게이지 영역은 thumb_size/2씩 양쪽에서 빼야 한다.
	m_track = m_rc;

	if (m_is_vertical)
		m_track.DeflateRect((m_rc.Width() - m_track_thick) / 2, m_thumb.cy / 2);
	else
		m_track.DeflateRect(m_thumb.cx / 2, (m_rc.Height() - m_track_thick) / 2);

	//m_rc의 y센터좌표
	int	cy = m_rc.CenterPoint().y;
	//현재 위치의 실제 픽셀좌표
	int	pxpos = Pos2Pixel(pos);
	int i;

	//전체 슬라이드 사각형 영역을 배경색으로 그림
	dc.FillSolidRect(m_rc, enable_color(m_cr_back));


	//inactive 영역을 먼저 그리고
	if (m_style == slider_thumb_round)
	{
		dc.FillSolidRect(pxpos, cy - m_track_height / 2, m_track.right - pxpos, m_track_height, enable_color(m_cr_inactive));
	}
	else if (m_style == slider_thumb || m_style == slider_value)
	{
		dc.FillSolidRect(pxpos, cy - m_track_height / 2, m_track.right - pxpos, m_track_height, enable_color(m_cr_inactive));
			
		CPen	penDark(PS_SOLID, 1, get_color(enable_color(m_cr_inactive), -24));
		CPen	penLight(PS_SOLID, 1, get_color(enable_color(m_cr_inactive), 36));

		dc.SelectObject(&penDark);
		dc.MoveTo(pxpos, cy - m_track_height / 2);
		dc.LineTo(m_track.right, cy - m_track_height / 2);
		//dc.LineTo(m_rc.right - 1, cy + m_track_height / 2);

		dc.SelectObject(&penLight);
		dc.MoveTo(pxpos, cy + m_track_height / 2);
		dc.LineTo(m_track.right - 1, cy + m_track_height / 2);
		dc.LineTo(m_track.right - 1, cy - m_track_height / 2);

		dc.SelectObject(pOldPen);
		penDark.DeleteObject();
		penLight.DeleteObject();
	}
	else if (m_style == slider_progress)
	{
		CRect	rInActive(pxpos, m_rc.top + 2, m_rc.right, m_rc.bottom - 2);
		dc.FillSolidRect(rInActive, enable_color(m_cr_inactive));
	}
	else if (m_style == slider_track)
	{
		CRect r = m_rc;
		int cy = r.CenterPoint().y;
		r.top = cy - 4;
		r.bottom = cy + 4;
		/*
		dc.FillSolidRect(r.left, cy - 4, r.Width(), 2, get_color(m_cr_back, -64));
		dc.FillSolidRect(r.left, cy - 2, r.Width(), 4, m_cr_back);//GRAY192);
		dc.FillSolidRect(r.left, cy + 2, r.Width(), 2, get_color(m_cr_back, 64));
		*/
		DrawSunkenRect(&dc, r, true, get_color(m_cr_back, -32), get_color(m_cr_back, 32), 1);
		r.DeflateRect(1, 1);
		DrawSunkenRect(&dc, r, true, get_color(m_cr_back, -32), get_color(m_cr_back, 32), 1);

	}
	else if (m_style == slider_step)
	{
		CRect r = m_rc;
		int thumb_size = MIN(r.Width(), r.Height()); //스텝 위치마다 그려질 도형의 크기
		int interval;
		CSize gap(0, 0);	//스텝의 시작과 끝의 여백
			
		Gdiplus::Color cr_line(255, 255, 0, 255);
		Gdiplus::Color cr_thumb(255, 62, 134, 193);
		Gdiplus::Color cr_thumb_inner(255, 255, 255, 255);
		Gdiplus::Pen pen_line(cr_line, 2.0f);
		Gdiplus::Pen pen_thumb(cr_thumb, 1.0f);

		m_is_vertical = (m_rc.Height() > m_rc.Width());

		if (m_is_vertical)
		{
			r.DeflateRect(0, gap.cy + thumb_size / 2);
			interval = r.Height() / (upper - lower);

			//thumb
			for (i = 0; i <= upper - lower; i++)
			{
				//각 step의 영역이 미리 계산된적이 없다면
				if (m_steps[i].r.IsRectEmpty())
				{
					m_steps[i].r = CRect(r.left, gap.cy + interval * i, r.right, gap.cy + interval * i + thumb_size);
					m_steps[i].r.DeflateRect(2, 2);
				}

				//이미지가 지정되어 있다면
				if (m_steps[i].m_img.is_valid())
				{
					m_steps[i].m_img.draw(g, m_steps[i].r);
				}
				else
				{
					//g.DrawEllipse(&pen_thumb, CRect2GpRect(m_steps[i].r));
					g.FillEllipse(&Gdiplus::SolidBrush(Gdiplus::Color(62, 134, 193)), CRect2GpRect(m_steps[i].r));
					CRect rthumb_small = m_steps[i].r;
					rthumb_small.DeflateRect(4, 4);
					g.FillEllipse(&Gdiplus::SolidBrush(Gdiplus::Color(255, 255, 255)), CRect2GpRect(rthumb_small));
				}
			}

			//라인
			pen_line.SetLineCap(Gdiplus::LineCapRound, Gdiplus::LineCapRound, Gdiplus::DashCapRound);
			for (i = 1; i <= upper - lower; i++)
			{
				g.DrawLine(&pen_line, Gdiplus::Point(r.CenterPoint().x, m_steps[i - 1].r.bottom + 4),
					Gdiplus::Point(r.CenterPoint().x, m_steps[i].r.top - 4));
			}
		}
		else
		{
			r.DeflateRect(gap.cx + thumb_size / 2, 0);
			interval = r.Width() / (upper - lower);

			//thumb
			for (i = 0; i <= upper - lower; i++)
			{
				if (m_steps[i].r.IsRectEmpty())
				{
					m_steps[i].r = CRect(gap.cx + interval * i, r.top, gap.cx + interval * i + thumb_size, r.bottom);
					m_steps[i].r.DeflateRect(2, 2);
				}

				//이미지가 지정되어 있다면
				if (m_steps[i].m_img.is_valid())
				{
					m_steps[i].m_img.draw(g, m_steps[i].r);
				}
				else
				{
					//g.DrawEllipse(&pen_thumb, CRect2GpRect(m_steps[i].r));
					g.FillEllipse(&Gdiplus::SolidBrush(Gdiplus::Color(62, 134, 193)), CRect2GpRect(m_steps[i].r));
					CRect rthumb_small = m_steps[i].r;
					rthumb_small.DeflateRect(4, 4);
					g.FillEllipse(&Gdiplus::SolidBrush(Gdiplus::Color(255, 255, 255)), CRect2GpRect(rthumb_small));
				}
			}

			//라인
			pen_line.SetLineCap(Gdiplus::LineCapRound, Gdiplus::LineCapRound, Gdiplus::DashCapRound);
			for (i = 1; i <= upper - lower; i++)
			{
				g.DrawLine(&pen_line, Gdiplus::Point(m_steps[i-1].r.right + 4, r.CenterPoint().y),
					Gdiplus::Point(m_steps[i].r.left - 4, r.CenterPoint().y));
			}
		}
	}

	//경과된 영역(active area) 표시
#if 1
	if (m_style <= slider_value)
	{
		dc.FillSolidRect(m_track.left, cy - m_track_height / 2, pxpos - m_track.left, m_track_height, enable_color(m_cr_active));
			
		CPen	penDark(PS_SOLID, 1, get_color(enable_color(m_cr_active), -64));
		CPen	penLight(PS_SOLID, 1, get_color(enable_color(m_cr_active), 64));

		dc.SelectObject(&penDark);
		dc.MoveTo(pxpos, cy - m_track_height / 2);
		dc.LineTo(m_track.left, cy - m_track_height / 2);
		dc.LineTo(m_track.left, cy + m_track_height / 2);

		dc.SelectObject(&penLight);
		dc.MoveTo(m_track.left + 1, cy + m_track_height / 2);
		dc.LineTo(pxpos, cy + m_track_height / 2);

		dc.SelectObject(pOldPen);
		penDark.DeleteObject();
		penLight.DeleteObject();
			
	}
	else if (m_style == slider_progress)
	{
		CRect	rActive(0, m_track.top + 2, pxpos, m_track.bottom - 2);
		dc.FillSolidRect(rActive, enable_color(m_cr_active));

		//m_crValueText = RGB(12, 162, 255);
		//m_cr_active = RGB(128,255,128);
		if (m_nValueStyle > none)
		{
			dc.SetTextColor(enable_color(m_crValueText));
			dc.SetBkMode(TRANSPARENT);

			if (m_nValueStyle == value)
				str.Format(_T("%ld / %ld"), pos, (upper > 0 ? upper : 0));
			else if (upper == lower)
				str = _T("0.0%");
			else
				str.Format(_T("%.1f%%"), (double)(pos - lower) / (double)(upper - lower) * 100.0);
			dc.DrawText(str, m_rc, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
		}

		//텍스트가 active, inactive 구간의 색상에 반전되도록 그려줘야한다.
		//텍스트를 반전시켜서 뿌려주는 기능이 없으므로
		//텍스트를 출력하고 그 위에 구간 색상을 반전시켜서 그려줘야 한다.
		//미완성.
		//dc.SetROP2(R2_XORPEN);
		//CBrush br((m_cr_active));
		//CBrush *pOldBrush = (CBrush*)dc.SelectObject(&br);
		//dc.Rectangle(rActive);//, &CBrush(m_cr_active));
		//dc.SelectObject(pOldBrush);
		//DrawRectangle(&dc, rActive, (m_cr_active), (m_cr_active), 1, PS_SOLID, R2_XORPEN);
	}
	else if (m_style == slider_track)
	{
		CRect r = m_track;
		int cy = r.CenterPoint().y;
		r.top = cy - 2;
		r.bottom = cy + 2;
		r.left += 2;
		r.right = pxpos;

		if (r.right > m_track.right - 2)
			r.right = m_track.right - 2;

		//CRect	rActive(0, m_rc.top + 2, pxpos, m_rc.bottom - 2);
		if (r.right > r.left)
			dc.FillSolidRect(r, enable_color(m_cr_active));
	}
#endif
	
	//for test
	//DrawRectangle(&dc, m_track, red);
	DrawRectangle(&dc, m_rc, red);

	// 손잡이(thumb)를 그린다
#if 1
	if (m_style <= slider_value)
	{
		CRect	rThumb = CRect(pxpos - m_thumb.cx / 2, cy - m_thumb.cy / 2, 0, 0);
		rThumb.right = rThumb.left + m_thumb.cx;
		rThumb.bottom = rThumb.top + m_thumb.cy;

		if (!IsWindowEnabled())
		{
		}
		else if (m_style == slider_thumb)
		{
			CBrush br(enable_color(m_cr_thumb));

			int		n = 4;										//라인 개수
			double	dx = (double)m_thumb.cx * 0.84 / double(n + 1);	//라인 간격
			double	sx = pxpos - 0.5 * double(n - 1) * dx;		//수직 라인 표시 시작 좌표

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
		else if (m_style == slider_thumb_round)
		{
			//dc.FillSolidRect(rThumb, m_cr_thumb);
			Gdiplus::Color cr_pen(255, GetRValue(255), GetGValue(0), GetBValue(0));
			Gdiplus::Pen pen(cr_pen, 1.0f);
			Gdiplus::Color cr_brush_outer(128, GetRValue(m_cr_thumb), GetGValue(m_cr_thumb), GetBValue(m_cr_thumb));
			Gdiplus::SolidBrush brush_outer(cr_brush_outer);
			Gdiplus::Color cr_brush_inner(255, GetRValue(m_cr_thumb-64), GetGValue(m_cr_thumb-64), GetBValue(m_cr_thumb-64));
			Gdiplus::SolidBrush brush_inner(cr_brush_inner);

			CRect r = rThumb;
			r.DeflateRect(4, 4);

			g.FillEllipse(&brush_outer, Gdiplus::Rect(rThumb.left, rThumb.top, rThumb.Width(), rThumb.Height()));
			g.FillEllipse(&brush_inner, Gdiplus::Rect(r.left, r.top, r.Width(), r.Height()));
			//g.DrawEllipse(&pen, Gdiplus::Rect(rThumb.left, rThumb.top, rThumb.Width(), rThumb.Height()));
		}
		else if (m_style == slider_value)
		{
			CBrush br(enable_color(m_cr_thumb));
			pOldBrush = (CBrush*)dc.SelectObject(&br);
			pOldPen = (CPen*)dc.SelectObject(&m_penThumbDarker);//NULL_PEN);

			CRect	rThumb = CRect(pxpos - m_thumb.cx / 2, m_rc.top + 2, pxpos + m_thumb.cx / 2, m_rc.bottom - 2);
			dc.RoundRect(rThumb, CPoint(6, 6));

			dc.SelectObject(pOldPen);
			dc.SelectObject(pOldBrush);
			br.DeleteObject();

			if (m_nValueStyle > none)
			{
				dc.SetTextColor(enable_color(m_crValueText));
				dc.SetBkMode(TRANSPARENT);

				if (m_nValueStyle == value)
					str.Format(_T("%ld / %ld"), pos, (upper > 0 ? upper : 0));
				else if (upper == lower)
					str = _T("0.0%");
				else
					str.Format(_T("%.1f%%"), (double)(pos - lower) / (double)(upper - lower) * 100.0);

				//텍스트를 thumb에 출력하는데 그 너비가 작다면 늘려준다.
				int		margin = 12;
				int		minWidth = m_thumb.cx * 2;
				CRect	rText;
				dc.DrawText(str, rText, DT_CALCRECT);

				if (rText.Width() + margin > minWidth)
					minWidth = rText.Width() + margin;

				if (m_thumb.cx != minWidth)
				{
					m_thumb.cx = minWidth;

					rThumb = CRect(pxpos - m_thumb.cx / 2, m_rc.top, pxpos + m_thumb.cx / 2, m_rc.bottom);
					//TRACE("%ld, invalidate\n", GetTickCount());
					Invalidate();
				}					
			}

			dc.DrawText(str, rThumb, DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_NOCLIP);
		}
	}
	//트랙의 북마크를 그린다.
	else if (m_style == slider_track)
	{
		if (m_use_bookmark)
		{
			for (i = 0; i < m_bookmark.size(); i++)
			{
				pxpos = Pos2Pixel(m_bookmark[i].pos);
				//dc.FillSolidRect(pxpos-2, cy-8, 4, 4, (i == m_cur_bookmark ? m_crBookmarkCurrent : m_crBookmark));
				CPen pen(PS_SOLID, 1, (i == m_cur_bookmark ? enable_color(m_crBookmarkCurrent) : enable_color(m_crBookmark)));
				CPen* pOldPen = (CPen*)dc.SelectObject(&pen);
				dc.SetPixel(pxpos, cy - 5, (i == m_cur_bookmark ? enable_color(m_crBookmarkCurrent) : enable_color(m_crBookmark)));
				for (int j = 1; j < 5; j++)
				{
					dc.MoveTo(pxpos - j, cy - 5 - j);
					dc.LineTo(pxpos + j + 1, cy - 5 - j);	//역삼각형
					//dc.LineTo(pxpos + 1, cy - 5 - j);		//역직각삼각형
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
		int repeat_pos;

		if (m_repeat_start >= 0)
		{
			repeat_pos = Pos2Pixel(m_repeat_start);
			dc.DrawText(_T("["), CRect(repeat_pos, m_rc.top-2, repeat_pos, m_rc.bottom),
						DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_NOCLIP);
			//DrawShadowText(dc.m_hDC, _T("["), 1, CRect(pxpos, m_rc.top-2, pxpos, m_rc.bottom),
			//	DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_NOCLIP, RGB(0,255,0),
			//	RGB(0,0,0), 4, 4);
		}
		if (m_repeat_end >= 0)
		{
			repeat_pos = Pos2Pixel(m_repeat_end);
			dc.DrawText(_T("]"), CRect(repeat_pos, m_rc.top-2, repeat_pos, m_rc.bottom),
				DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_NOCLIP);
		}
	}
#endif
	// 포커스 사각형을 그린다
	if(m_draw_focus_rect && m_has_focus && !IsWindowEnabled())
	{
		dc.DrawFocusRect(m_rc);
	}

	dc.SelectObject(pOldFont);
	dc.SelectObject(pOldPen);

	// Do not call CSliderCtrl::OnPaint() for painting messages
}

BOOL CSCSliderCtrl::OnEraseBkgnd(CDC* pDC) 
{
	// TODO: Add your message handler code here and/or call default
	return FALSE;
	
	return CSliderCtrl::OnEraseBkgnd(pDC);
}

// 컨트롤의 위치값에 해당하는 손잡이 중심의 좌표값을 구한다
int CSCSliderCtrl::Pos2Pixel(int nPos)
{
	int lower = GetRangeMin();
	int upper = GetRangeMax();

	if (upper == lower)
		return (m_style <= slider_value ? m_thumb.cx / 2 : 0);

	if (m_is_vertical)
	{
		return
			m_margin.top + m_thumb.cy / 2 +
			(int)(
			(double)(m_rc.Height() - m_margin.top - m_margin.bottom - m_thumb.cy) *
			((double)(nPos - lower) / (double)(upper - lower))
			);

	}
	else
	{
		return (int)(
			(double)(m_rc.Width() - m_margin.left - m_margin.right - (m_style <= slider_value ? m_thumb.cx : 0)) *
			((double)(nPos - lower) / (double)(upper - lower))
			) + m_margin.left + (m_style <= slider_value ? m_thumb.cx/2 : 0);
	}
}

// 마우스의 좌표값에 해당하는 컨트롤의 위치값을 구한다
//
double CSCSliderCtrl::Pixel2Pos(int nPixel)
{
	int lower = GetRangeMin();
	int upper = GetRangeMax();

	if(m_is_vertical)
	{
		return (
			lower +
			(double)(nPixel - m_margin.top - (double)m_thumb.cy/2.0) /
			(double)(m_rc.Height() - m_margin.bottom - m_margin.top - (m_style <= slider_value ? m_thumb.cy : 0)) *
			(double)(upper - lower) + 0.5
			);

	}
	else
	{
		return (
			lower +
			(double)(nPixel - m_margin.left - (m_style <= slider_value ? (double)m_thumb.cx/2.0 : 0)) /
			(double)(m_rc.Width() - m_margin.left - m_margin.right - (m_style <= slider_value ? m_thumb.cx : 0)) *
			(double)(upper - lower + 1) + 0.0
			);
		//TRACE("d = %f\n", d);
		//return (int)d;
	}
}

void CSCSliderCtrl::OnLButtonDown(UINT nFlags, CPoint point) 
{
	if (m_style == slider_normal)
	{
		CSliderCtrl::OnLButtonDown(nFlags, point);
		return;
	}

	if (m_enable_slide == false)
		return;

	int pos = GetPos();

	// TODO: Add your message handler code here and/or call default
	if (m_use_bookmark && (m_cur_bookmark >= 0))
	{
		SetPos(m_bookmark[m_cur_bookmark].pos);
		::SendMessage(GetParent()->GetSafeHwnd(), Message_SCSliderCtrl, (WPARAM)&CSCSliderCtrlMsg(CSCSliderCtrlMsg::msg_thumb_move, GetDlgCtrlID(), pos), 0);
		return;
	}

	SetCapture();
	SetFocus();

	m_lbuttondown = true;

	// 손잡이를 마우스로 클릭했을 때
	// 마우스의 좌표와 손잡이 중심의 좌표를 비교하여
	// 둘 사이의 거리를 구해둔다
	if(m_is_vertical)
	{
		if(abs(point.y - Pos2Pixel(pos)) <= (m_style <= slider_value ? m_thumb.cy/2.0 : 0))
			m_nMouseOffset = point.y - Pos2Pixel(pos);
		else
			m_nMouseOffset = 0;

	}
	else
	{
		/*
		if (m_style == slider_track && !m_enable_bottom_slide)
		{
			if (point.y > m_rc.CenterPoint().y + 2)
			{
				ReleaseCapture();
				m_lbuttondown = false;
				::SendMessage(GetParent()->GetSafeHwnd(),	WM_LBUTTONDOWN,
								MK_LBUTTON, MAKELPARAM(point.x, point.y));
				return;
			}
		}
		*/
		if(abs(point.x - Pos2Pixel(pos)) <= (m_style <= slider_value ? m_thumb.cx/2.0 : 0))
			m_nMouseOffset = point.x - Pos2Pixel(pos);
		else
			m_nMouseOffset = 0;
	}

	if (m_nEventMsgStyle == msg_style_timer)
	{
		//CSCSliderCtrlMsg msg(CSCSliderCtrlMsg::msg_thumb_grab, GetDlgCtrlID(), pos);
		::SendMessage(GetParent()->GetSafeHwnd(), Message_SCSliderCtrl, (WPARAM)&CSCSliderCtrlMsg(CSCSliderCtrlMsg::msg_thumb_grab, GetDlgCtrlID(), pos), 0);
	}
	else if (m_nEventMsgStyle == msg_style_post)
	{
		::PostMessage(GetParent()->GetSafeHwnd(), Message_SCSliderCtrl, (WPARAM)&CSCSliderCtrlMsg(CSCSliderCtrlMsg::msg_thumb_grab, GetDlgCtrlID(), pos), 0);
	}
	else if (m_nEventMsgStyle == msg_style_callback)
	{
		//if (m_pCallback_func != NULL)
			//(*(m_pCallback_func))(m_pParentWnd, this, MESSAGE_SCSLIDERCTRL_THUMB_GRAB, pos);
	}

	//OnMouseMove(nFlags, point);
	
	CSliderCtrl::OnLButtonDown(nFlags, point);
}

void CSCSliderCtrl::OnLButtonUp(UINT nFlags, CPoint point) 
{
	// TODO: Add your message handler code here and/or call default
	if (m_style == slider_normal)
	{
		CSliderCtrl::OnLButtonUp(nFlags, point);
		return;
	}

	if (m_enable_slide == false)
		return;

	if(!IsWindowEnabled() || !m_lbuttondown)
		return;

	int pos = GetPos();

	ReleaseCapture();
	m_lbuttondown = false;

	if (m_nEventMsgStyle == msg_style_timer)
	{
		::SendMessage(GetParent()->GetSafeHwnd(), Message_SCSliderCtrl, (WPARAM)&CSCSliderCtrlMsg(CSCSliderCtrlMsg::msg_thumb_release, GetDlgCtrlID(), pos), 0);
	}
	else if (m_nEventMsgStyle == msg_style_post)
	{
		::PostMessage(GetParent()->GetSafeHwnd(), Message_SCSliderCtrl, (WPARAM)&CSCSliderCtrlMsg(CSCSliderCtrlMsg::msg_thumb_release, GetDlgCtrlID(), pos), 0);
	}
	else if (m_nEventMsgStyle == msg_style_callback)
	{
		//if (m_pCallback_func != NULL)
			//(*(m_pCallback_func))(m_pParentWnd, this, MESSAGE_SCSLIDERCTRL_THUMB_RELEASE, pos);
	}

	
	CSliderCtrl::OnLButtonUp(nFlags, point);
}

void CSCSliderCtrl::OnMouseMove(UINT nFlags, CPoint point) 
{
	if (m_style == slider_normal)
	{
		CSliderCtrl::OnMouseMove(nFlags, point);
		return;
	}

	// TODO: Add your message handler code here and/or call default
	int pos = GetPos();
	CString str;

	if (m_use_bookmark)
	{
		int i;
		int pos_range[2];
		int found = false;

		if (point.y < m_rc.CenterPoint().y + 4)
		{
			pos_range[0] = Pixel2Pos(point.x - 4);
			pos_range[1] = Pixel2Pos(point.x + 4);
			//TRACE(_T("0:%f, 1:%f\n"), pos[0], pos[1]);
			for (i = 0; i < m_bookmark.size(); i++)
			{
				//어떤 북마크 위치가 현재 마우스의 근처라면
				if (m_bookmark[i].pos >= pos_range[0] && m_bookmark[i].pos <= pos_range[1])
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
			if (m_tooltip.m_hWnd)
				m_tooltip.UpdateTipText(str, this);
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
			str.Format(_T("%d / %d"), pos, upper);

		if (m_tooltip.m_hWnd)
			m_tooltip.UpdateTipText(str, this);
	}


	if(!m_lbuttondown || !IsWindowEnabled())
		return;

	int nPixel;

	// 범위를 벗어났는지 검사한다
	if(m_is_vertical)
	{
		nPixel = point.y - m_nMouseOffset;

		if(nPixel > m_rc.Height() - m_margin.bottom - (m_style <= slider_value ? m_thumb.cy/2 : 0))
			nPixel = m_rc.Height() - m_margin.bottom - (m_style<= slider_value ? m_thumb.cy/2 : 0);

		if(nPixel < m_margin.top + (m_style <= slider_value ? m_thumb.cy/2 : 0))
			nPixel = m_margin.top + (m_style <= slider_value ? m_thumb.cy/2 : 0);
	}
	else
	{
		nPixel = point.x - m_nMouseOffset;

		if(nPixel < m_margin.left + (m_style <= slider_value ? m_thumb.cx/2 : 0))
			nPixel = m_margin.left + (m_style <= slider_value ? m_thumb.cx/2 : 0);

		if(nPixel > m_rc.Width() - m_margin.right - (m_style <= slider_value ? m_thumb.cx/2 : 0))
			nPixel = m_rc.Width() - m_margin.right - (m_style <= slider_value ? m_thumb.cx/2 : 0);
	}

	// 변한 내용을 적용한다
	int pix = Pos2Pixel(pos);
	TRACE(_T("pos2pix(%d) = %d, nPixel = %d\n"), pos, pix, nPixel);
	if(Pos2Pixel(pos) != nPixel)
	{
		pos = Pixel2Pos(nPixel);
		TRACE(_T("Pixel2Pos(%d) = %d, nPixel = %d\n"), nPixel, pos, nPixel);
		SetPos(pos);

		if (m_nEventMsgStyle == msg_style_timer)
		{
			SetTimer(timer_post_pos, 1, NULL);
		}
		else if (m_nEventMsgStyle == msg_style_post)
		{
			::PostMessage(GetParent()->GetSafeHwnd(), Message_SCSliderCtrl, (WPARAM)&CSCSliderCtrlMsg(CSCSliderCtrlMsg::msg_thumb_move, GetDlgCtrlID(), pos), 0);
		}
		else if (m_nEventMsgStyle == msg_style_callback)
		{
			//if (m_pCallback_func != NULL)
				//(*(m_pCallback_func))(m_pParentWnd, this, MESSAGE_SCSLIDERCTRL_MOVED, pos);
		}
	}

	CSliderCtrl::OnMouseMove(nFlags, point);
}

// PrepareMask
//
// "Drawing Transparent Bitmap with ease with on the fly masks in MFC"
// By Raja Segar
//
// clrpTransColor의 기본값을 NULL(black)에서
// 0xFF000000(RGB 값이 아닌 값)으로 바꿨습니다.
//
void CSCSliderCtrl::PrepareMask(CBitmap *pBmpSource, CBitmap *pBmpMask,
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
void CSCSliderCtrl::DrawTransparentBitmap(
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

void CSCSliderCtrl::DrawFocusRect(BOOL bDraw, BOOL bRedraw)
{
	m_draw_focus_rect = bDraw;

	if(bRedraw)
		Invalidate();
}

void CSCSliderCtrl::OnSetFocus(CWnd* pOldWnd) 
{
	CSliderCtrl::OnSetFocus(pOldWnd);
	
	// TODO: Add your message handler code here
	m_has_focus = TRUE;
	Invalidate();
	//TRACE("SetFocus\n");
}

void CSCSliderCtrl::OnKillFocus(CWnd* pNewWnd) 
{
	CSliderCtrl::OnKillFocus(pNewWnd);
	
	// TODO: Add your message handler code here
	m_has_focus = FALSE;
	Invalidate();
	//TRACE("KillFocus\n");
}

void CSCSliderCtrl::set_back_color(COLORREF crBack)
{
	m_cr_back = crBack;
	Invalidate();
}

void CSCSliderCtrl::set_active_color(COLORREF crActive)
{
	m_cr_active = crActive;
	Invalidate();
}

void CSCSliderCtrl::set_inactive_color(COLORREF crInActive)
{
	m_cr_inactive = crInActive;
	Invalidate();
}

void CSCSliderCtrl::set_thumb_color(COLORREF crThumb)
{
	int nMultiple = 32;

	m_cr_thumb		= crThumb;
	m_cr_thumbLight	= get_color(m_cr_thumb, nMultiple);
	m_cr_thumbLighter= get_color(m_cr_thumb, nMultiple * 2);
	m_cr_thumbDark	= get_color(m_cr_thumb, -nMultiple);
	m_cr_thumbDarker = get_color(m_cr_thumb, -nMultiple * 2);

	m_penThumb.DeleteObject();
	m_penThumbLight.DeleteObject();
	m_penThumbLighter.DeleteObject();
	m_penThumbDark.DeleteObject();
	m_penThumbDarker.DeleteObject();

	m_penThumb.CreatePen(PS_SOLID, 1, m_cr_thumb);
	m_penThumbLight.CreatePen(PS_SOLID, 1, m_cr_thumbLight);
	m_penThumbLighter.CreatePen(PS_SOLID, 1, m_cr_thumbLighter);
	m_penThumbDark.CreatePen(PS_SOLID, 1, m_cr_thumbDark);
	m_penThumbDarker.CreatePen(PS_SOLID, 1, m_cr_thumbDarker);
}


BOOL CSCSliderCtrl::PreTranslateMessage(MSG* pMsg)
{
	if (m_use_tooltip)
		m_tooltip.RelayEvent(pMsg);
	// TODO: Add your specialized code here and/or call the base class 
	if (pMsg->wParam == WM_KEYDOWN)
	{
		return false;
	}

	return CSliderCtrl::PreTranslateMessage(pMsg);
}

/*
void CSCSliderCtrl::OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags)
{
	// TODO: Add your message handler code here and/or call default
	TRACE(_T("CSCSliderCtrl::OnKeyDown = %d\n"), nChar);
	if (m_enable_slide == false)
		return;

	switch (nChar)
	{
		case VK_LEFT :	pos = GetPos() - 1;
						if (pos < lower)
							pos = lower;
						SetPos(pos);
						if (m_nEventMsgStyle == msg_style_timer)
						{
							SetTimer(timer_post_pos, 1, NULL);
						}
						else if (m_nEventMsgStyle == msg_style_post)
						{
							::PostMessage(GetParent()->GetSafeHwnd(),	Message_CSCSliderCtrl, (WPARAM)&CSCSliderCtrlMsg(CSCSliderCtrlMsg::msg_thumb_move, GetDlgCtrlID(), pos), 0);
						}
						else if (m_nEventMsgStyle == msg_style_callback)
						{
							//if (m_pCallback_func != NULL)
								//(*(m_pCallback_func))(m_pParentWnd, this, Message_CSCSliderCtrl, (WPARAM)&CSliderCtrlMsg(CSliderCtrlMsg::msg_thumb_move, GetDlgCtrlID(), pos), 0);
						}
						break;
		case VK_RIGHT :	pos = GetPos() + 1;
						if (pos > upper)
							pos = upper;
						SetPos(pos);
						if (m_nEventMsgStyle == msg_style_timer)
						{
							SetTimer(timer_post_pos, 1, NULL);
						}
						else if (m_nEventMsgStyle == msg_style_post)
						{
							::PostMessage(GetParent()->GetSafeHwnd(),	Message_CSCSliderCtrl, (WPARAM)&CSCSliderCtrlMsg(CSCSliderCtrlMsg::msg_thumb_move, GetDlgCtrlID(), pos), 0);
						}
						else if (m_nEventMsgStyle == msg_style_callback)
						{
							//if (m_pCallback_func != NULL)
								//(*(m_pCallback_func))(m_pParentWnd, this, MESSAGE_SCSLIDERCTRL_MOVED, pos);
						}
						break;
		default :		::PostMessage(GetParent()->GetSafeHwnd(), WM_KEYDOWN, nChar, 0);
						return;
	}
	
	CSliderCtrl::OnKeyDown(nChar, nRepCnt, nFlags);
}
*/

void CSCSliderCtrl::set_style(int nStyle)
{
	m_style = nStyle;

	m_track_height = 5;

	if (m_style == slider_thumb)
	{
		m_thumb = CSize(28, 12);
	}
	else if (m_style == slider_thumb_round)
	{
		m_thumb = CSize(16, 16);
		m_track_height = 3;
	}
	else if (m_style == slider_value)
	{
		m_thumb = CSize(56, 8);
	}
	else if (m_style == slider_progress)
	{
		m_enable_slide = false;
	}
	else if (m_style == slider_step)
	{
		m_enable_slide = false;
	}
}


void CSCSliderCtrl::OnTimer(UINT_PTR nIDEvent)
{
	// TODO: Add your message handler code here and/or call default
	if (nIDEvent == timer_post_pos)
	{
		KillTimer(timer_post_pos);
		::SendMessage(GetParent()->GetSafeHwnd(), Message_SCSliderCtrl, (WPARAM)&CSCSliderCtrlMsg(CSCSliderCtrlMsg::msg_thumb_move, GetDlgCtrlID(), GetPos()), 0);
	}

	CSliderCtrl::OnTimer(nIDEvent);
}

void CSCSliderCtrl::SetEventMsgStyle(int style)
{
	if (style < msg_style_timer || style > msg_style_callback)
		return;

	m_nEventMsgStyle = style;
	AfxGetApp()->WriteProfileInt(_T("setting"), _T("event msg style"), m_nEventMsgStyle);
}


void CSCSliderCtrl::OnSize(UINT nType, int cx, int cy)
{
	CSliderCtrl::OnSize(nType, cx, cy);

	// TODO: Add your message handler code here
	//Invalidate(true);
}

//pos의 위치에 있는 북마크의 인덱스를 리턴한다.
int CSCSliderCtrl::find_index_bookmark(int pos)
{
	int i;

	for (i = 0; i < m_bookmark.size(); i++)
	{
		if (m_bookmark[i].pos == pos)
			return i;
	}

	return -1;
}

void CSCSliderCtrl::bookmark(int mode, int pos, CString name)
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
		m_bookmark.push_back(CSCSliderCtrlBookmark(pos, name));
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
			::SendMessage(GetParent()->GetSafeHwnd(), Message_SCSliderCtrl, (WPARAM)&CSCSliderCtrlMsg(CSCSliderCtrlMsg::msg_thumb_move, GetDlgCtrlID(), pos), 0);
		}
	}

	std::sort(m_bookmark.begin(), m_bookmark.end(),
			[](CSCSliderCtrlBookmark a, CSCSliderCtrlBookmark b)
	{
		return (a.pos < b.pos);
	}
	);
	Invalidate();
}

int CSCSliderCtrl::get_near_bookmark(int pos, bool forward)
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

void CSCSliderCtrl::set_bookmark_color(COLORREF cr)
{
	m_crBookmark = cr;
	Invalidate();
}

void CSCSliderCtrl::set_bookmark_current_color(COLORREF cr)
{
	m_crBookmarkCurrent = cr;
	Invalidate();
}

void CSCSliderCtrl::PreSubclassWindow()
{
	// TODO: Add your specialized code here and/or call the base class
	m_tooltip.Create(this, TTS_ALWAYSTIP | TTS_NOPREFIX | TTS_NOANIMATE);
	m_tooltip.SetDelayTime(TTDT_AUTOPOP, -1);
	m_tooltip.SetDelayTime(TTDT_INITIAL, 0);
	m_tooltip.SetDelayTime(TTDT_RESHOW, 0);
	m_tooltip.SetMaxTipWidth(400); 
	m_tooltip.AddTool(this, _T(""));
	m_tooltip.Activate(TRUE);

	CSliderCtrl::PreSubclassWindow();
}


void CSCSliderCtrl::OnWindowPosChanged(WINDOWPOS* lpwndpos)
{
	CSliderCtrl::OnWindowPosChanged(lpwndpos);

	// TODO: Add your message handler code here
	Invalidate();
}

COLORREF CSCSliderCtrl::enable_color(COLORREF cr, int offset)
{
	if (IsWindowEnabled())
		return cr;
	return get_color(gray_color(cr), offset);
}

void CSCSliderCtrl::set_repeat_range(int start, int end)
{
	set_repeat_start(start);
	set_repeat_end(end);
}

//pos가 0이상이면 그 트랙 위치를 start 또는 end로 설정하고
//-2이면 현재 위치를, -1면 해제의 의미로 처리한다.
void CSCSliderCtrl::set_repeat_start(int pos)
{
	if (pos == -2)
	{
		m_repeat_start = pos;
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
		m_repeat_end = GetRangeMax();

	Invalidate();
}

//pos가 0이상이면 그 트랙 위치를 start 또는 end로 설정하고
//-2이면 현재 위치를, -1면 해제의 의미로 처리한다.
void CSCSliderCtrl::set_repeat_end(int pos)
{
	if (pos == -2)
	{
		m_repeat_end = pos;
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
		m_repeat_start = GetRangeMin();

	Invalidate();
}

//pos 위치에 resource id 이미지를 표시한다.
void CSCSliderCtrl::set_step_image(int pos, int id)
{
	if (pos == -1)
	{
		for (int i = 0; i < m_steps.size(); i++)
			m_steps[i].m_img.load(id);

		Invalidate();
		return;
	}

	if (pos >= m_steps.size())
	{
		TRACE(_T("invalid index = %d\n"), pos);
		return;
	}

	m_steps[pos].m_img.load(id);
	Invalidate();
}
