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
	set_style(m_style);

	m_nEventMsgStyle = AfxGetApp()->GetProfileInt(_T("setting"), _T("event msg style"), msg_style_timer);
	int theme = m_theme.get_color_theme();
	set_color_theme(CSCColorTheme::color_theme_default);
	memset(&m_lf, 0, sizeof(LOGFONT));
}

CSCSliderCtrl::~CSCSliderCtrl()
{
	//m_penThumb.DeleteObject();
	//m_penThumbLight.DeleteObject();
	//m_penThumbLighter.DeleteObject();
	//m_penThumbDark.DeleteObject();
	//m_penThumbDarker.DeleteObject();
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
	if (m_style == style_normal)
	{
		CSliderCtrl::OnPaint();
		return;
	}

	int lower = GetRangeMin();
	int upper = GetRangeMax();
	GetRange(lower, upper);
	int pos = GetPos();
	//TRACE(_T("GetPos() OnPaint() = %d\n"), pos);

	if (m_style == style_step && m_steps.size() < upper - lower)
		m_steps.resize(upper - lower + 1);

	CPaintDC	dc1(this); // device context for painting
	CPen*		pOldPen = NULL;
	CBrush*		pOldBrush = NULL;
	CString		str, str_dual;

	//Gdiplus::Color	cr_back = (m_forced_gray_include_back ? enable_color(m_theme.cr_back) : m_theme.cr_back);
	Gdiplus::Color	cr_active = enable_color(m_cr_active);
	Gdiplus::Color	cr_inactive = enable_color(m_cr_inactive, 32);
	Gdiplus::Color	cr_text = enable_color(m_theme.cr_text);
	Gdiplus::Color	cr_thumb = enable_color(m_cr_thumb);
	Gdiplus::Color	cr_tic = enable_color(m_cr_tic);

	// TODO: Add your message handler code here
	GetClientRect(m_rc);
	CMemoryDC	dc(&dc1, &m_rc);

	Gdiplus::Graphics g(dc.m_hDC);
	g.SetSmoothingMode(Gdiplus::SmoothingMode::SmoothingModeAntiAlias);
	g.SetInterpolationMode(Gdiplus::InterpolationModeHighQualityBicubic);

	CFont* pOldFont = (CFont*)dc.SelectObject(&m_font);

	//���� �׷����� ������ ������ thumb_size/2�� ���ʿ��� ���� �Ѵ�.
	CRect rtrack = m_rc;

	if (m_style <= style_thumb_round)
	{
		if (m_is_vertical)
			rtrack.DeflateRect((m_rc.Width() - m_track_thick) / 2, m_thumb.cy / 2);
		else
			rtrack.DeflateRect(m_thumb.cx / 2, (m_rc.Height() - m_track_thick) / 2);
	}

	//m_rc�� y������ǥ
	int	cy = m_rc.CenterPoint().y;
	//���� ��ġ�� ���� �ȼ���ǥ
	int	pxpos = Pos2Pixel(pos);
	//int	pxpos = int(((float)track.Width() * float(pos - lower)) / float(upper - lower));
	int i;

	//��ü �����̵� �簢�� ������ �������� �׸�
	if (m_transparent)
	{
		CRect Rect;
		GetWindowRect(&Rect);
		CWnd* pParent = GetParent();
		ASSERT(pParent);
		pParent->ScreenToClient(&Rect);  //convert our corrdinates to our parents
		//copy what's on the parents at this point
		CDC* pDC = pParent->GetDC();
		CDC MemDC;
		CBitmap bmp;
		MemDC.CreateCompatibleDC(pDC);
		bmp.CreateCompatibleBitmap(pDC, Rect.Width(), Rect.Height());
		CBitmap* pOldBmp = MemDC.SelectObject(&bmp);
		MemDC.BitBlt(0, 0, Rect.Width(), Rect.Height(), pDC, Rect.left, Rect.top, SRCCOPY);
		dc.BitBlt(0, 0, Rect.Width(), Rect.Height(), &MemDC, 0, 0, SRCCOPY);
		MemDC.SelectObject(pOldBmp);
		pParent->ReleaseDC(pDC);
		MemDC.DeleteDC();
	}
	else
	{
		//dc.FillSolidRect(m_rc, cr_back.ToCOLORREF());
	}


	//inactive ������ ���� �׸���
	if (m_style <= style_value)
	{
		//tic text�� ǥ�õǴ� ��쿡�� gap + m_thumb.cy + 4 + text_height(12) ��ŭ�� ���Ϳ� ��ġ�ϵ��� �������ش�.
		if (m_tic_show_text)
		{
			rtrack.top = (m_rc.Height() - (2 + m_thumb.cy + 10)) / 2;
			rtrack.bottom = rtrack.top + m_thumb.cy;
			dc.FillSolidRect(pxpos, rtrack.CenterPoint().y - m_track_height / 2, rtrack.right - pxpos, m_track_height, cr_inactive.ToCOLORREF());
		}
		else
		{
			dc.FillSolidRect(pxpos, cy - m_track_height / 2, rtrack.right - pxpos, m_track_height, cr_inactive.ToCOLORREF());
		}
		
		if (m_style == style_value)
		{
			CPen	penDark(PS_SOLID, 1, get_color(cr_inactive, -24).ToCOLORREF());
			CPen	penLight(PS_SOLID, 1, get_color(cr_inactive, 36).ToCOLORREF());

			dc.SelectObject(&penDark);
			dc.MoveTo(pxpos, cy - m_track_height / 2);
			dc.LineTo(rtrack.right, cy - m_track_height / 2);
			//dc.LineTo(m_rc.right - 1, cy + m_track_height / 2);

			dc.SelectObject(&penLight);
			dc.MoveTo(pxpos, cy + m_track_height / 2);
			dc.LineTo(rtrack.right - 1, cy + m_track_height / 2);
			dc.LineTo(rtrack.right - 1, cy - m_track_height / 2);

			dc.SelectObject(pOldPen);
			penDark.DeleteObject();
			penLight.DeleteObject();
		}
	}
	else if (m_style == style_progress)
	{
		rtrack = CRect(pxpos, m_rc.CenterPoint().y - m_track_height / 2, m_rc.right, m_rc.CenterPoint().y + m_track_height / 2);
		dc.FillSolidRect(rtrack, cr_inactive.ToCOLORREF());
	}
	else if (m_style == style_progress_line)
	{
		int track_height = 10;
		int marginy = MAX(0, (m_rc.Height() - track_height) / 2);
		CRect rtrack = m_rc;
		rtrack.top = m_rc.CenterPoint().y - marginy;
		rtrack.bottom = m_rc.CenterPoint().y + marginy;
		Gdiplus::Pen pen(m_cr_inactive, track_height);
		pen.SetLineCap(Gdiplus::LineCapRound, Gdiplus::LineCapRound, Gdiplus::DashCapRound);
		g.DrawLine(&pen, rtrack.left + rtrack.Height() / 2, rtrack.CenterPoint().y, m_rc.right - rtrack.Height() / 2 - 1, rtrack.CenterPoint().y);
	}
	else if (m_style == style_track)
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
		draw_sunken_rect(&dc, r, true, get_color(m_theme.cr_back, -32), get_color(m_theme.cr_back, 32), 1);
		r.DeflateRect(1, 1);
		draw_sunken_rect(&dc, r, true, get_color(m_theme.cr_back, -32), get_color(m_theme.cr_back, 32), 1);

	}
	else if (m_style == style_step)
	{
		CRect r = m_rc;
		int thumb_size = m_thumb.cx;// MIN(r.Width(), r.Height()); //���� ��ġ���� �׷��� ������ ũ��
		int interval;
		CSize gap(0, 0);	//������ ���۰� ���� ����
			
		Gdiplus::Color gcr_line = Gdiplus::Color::RoyalBlue;
		Gdiplus::Color gcr_thumb_inner(255, 255, 255, 255);
		Gdiplus::Pen pen_line(gcr_line, 1.7f);
		Gdiplus::Pen pen_gray(Gdiplus::Color::Color(192, 192, 192), 1.7f);
		Gdiplus::Pen pen_thumb(cr_thumb, 1.0f);


		m_is_vertical = (m_rc.Height() > m_rc.Width());

		if (m_is_vertical)
		{
			r.DeflateRect(0, gap.cy + thumb_size);
			interval = r.Height() / (upper - lower);

			//thumb
			for (i = lower; i <= upper; i++)
			{
				//�� step�� ������ �̸� �������� ���ٸ�
				if (m_steps[i].r.IsRectEmpty())
				{
					m_steps[i].r = CRect(r.CenterPoint().x - thumb_size / 2, gap.cy + interval * i, r.CenterPoint().x + thumb_size / 2, gap.cy + interval * i + thumb_size);
					m_steps[i].r.DeflateRect(2, 2);	
				}

				//�̹����� �����Ǿ� �ִٸ�
				if (m_steps[i].m_img.is_valid())
				{
					m_steps[i].m_img.draw(g, m_steps[i].r);
				}
				else
				{
					//g.DrawEllipse(&pen_thumb, CRect2GpRect(m_steps[i].r));
					g.FillEllipse(&Gdiplus::SolidBrush(cr_thumb), CRect2GpRect(m_steps[i].r));

					//pos �̸��� �Ķ��� ���� üũ ǥ�ø�
					if (i < pos)
					{
						draw_line(&dc, m_steps[i].r.CenterPoint().x - 3, m_steps[i].r.CenterPoint().y - 0, m_steps[i].r.CenterPoint().x - 1, m_steps[i].r.CenterPoint().y + 2, Gdiplus::Color::White, 1.0f);
						draw_line(&dc, m_steps[i].r.CenterPoint().x - 1, m_steps[i].r.CenterPoint().y + 2, m_steps[i].r.CenterPoint().x + 3, m_steps[i].r.CenterPoint().y - 2, Gdiplus::Color::White, 1.0f);
					}
					else if (i == pos)
					{
						CRect rthumb_small = m_steps[i].r;
						rthumb_small.DeflateRect(2, 2);
						g.FillEllipse(&Gdiplus::SolidBrush(Gdiplus::Color::White), CRect2GpRect(rthumb_small));
					}
					else
					{
						CRect rthumb_small = m_steps[i].r;
						rthumb_small.DeflateRect(1, 1);
						g.FillEllipse(&Gdiplus::SolidBrush(Gdiplus::Color::White), CRect2GpRect(rthumb_small));
					}
				}
			}

			//�� thumb�� ���̿� ������ �׷��ش�.
			pen_line.SetLineCap(Gdiplus::LineCapRound, Gdiplus::LineCapRound, Gdiplus::DashCapRound);
			for (i = 1; i <= upper - lower; i++)
			{
				g.DrawLine(i > pos ? &pen_gray : &pen_line, Gdiplus::Point(r.CenterPoint().x, m_steps[i - 1].r.bottom + 0),
					Gdiplus::Point(r.CenterPoint().x, m_steps[i].r.top - 0));
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

				//�̹����� �����Ǿ� �ִٸ�
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

			//����
			pen_line.SetLineCap(Gdiplus::LineCapRound, Gdiplus::LineCapRound, Gdiplus::DashCapRound);
			for (i = 1; i <= upper - lower; i++)
			{
				g.DrawLine(i > pos ? &pen_gray : &pen_line, Gdiplus::Point(m_steps[i - 1].r.right + 4, r.CenterPoint().y),
					Gdiplus::Point(m_steps[i].r.left - 4, r.CenterPoint().y));
			}
		}
	}

	//����� ����(active area) ǥ��
#if 1
	if (m_style <= style_value)
	{
		//tic text�� ǥ�õǴ� ��쿡�� m_thumb.cy + 2(gap) + text_height(10) ��ŭ�� ���Ϳ� ��ġ�ϵ��� �������ش�.
		if (m_tic_show_text)
		{
			dc.FillSolidRect(rtrack.left, rtrack.CenterPoint().y - m_track_height / 2, pxpos - rtrack.left, m_track_height, cr_active.ToCOLORREF());
		}
		else
		{
			dc.FillSolidRect(rtrack.left, cy - m_track_height / 2, pxpos - rtrack.left, m_track_height, cr_active.ToCOLORREF());
		}
		
		if (m_style == style_value)
		{
			CPen	penDark(PS_SOLID, 1, get_color(cr_active, -64).ToCOLORREF());
			CPen	penLight(PS_SOLID, 1, get_color(cr_active, 64).ToCOLORREF());

			dc.SelectObject(&penDark);
			dc.MoveTo(pxpos, cy - m_track_height / 2);
			dc.LineTo(rtrack.left, cy - m_track_height / 2);
			dc.LineTo(rtrack.left, cy + m_track_height / 2);

			dc.SelectObject(&penLight);
			dc.MoveTo(rtrack.left + 1, cy + m_track_height / 2);
			dc.LineTo(pxpos, cy + m_track_height / 2);

			dc.SelectObject(pOldPen);
			penDark.DeleteObject();
			penLight.DeleteObject();
		}
	}
	else if (m_style == style_progress)
	{
		rtrack = CRect(0, m_rc.CenterPoint().y - m_track_height / 2, pxpos, m_rc.CenterPoint().y + m_track_height / 2);
		dc.FillSolidRect(rtrack, cr_active.ToCOLORREF());

		//m_crValueText = RGB(12, 162, 255);
		//m_cr_active = RGB(128,255,128);
		if (m_text_style > text_style_none)
		{
			dc.SetTextColor(cr_text.ToCOLORREF());
			dc.SetBkMode(TRANSPARENT);

			if (m_text_style == text_style_value)
				str.Format(_T("%ld / %ld"), pos, (upper > 0 ? upper : 0));
			else if (m_text_style == text_style_user_defined)
				str = m_text;
			else if (m_text_style == text_style_dual_text)
			{
				str = m_text;
				str_dual = m_text_dual;
			}
			else if (upper == lower)
				str = _T("0.0%");
			else
				str.Format(_T("%.1f%%"), (double)(pos - lower) / (double)(upper - lower) * 100.0);
			//dc.DrawText(str, m_rc, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
		}

		//�ؽ�Ʈ�� active, inactive ������ ���� �����ǵ��� �׷�����Ѵ�.
		CRect rcLeft, rcRight;
		rcLeft = rcRight = m_rc;
		rcRight.left = rcLeft.right = pxpos;

		//�� ������ �����Ͽ� �ش� �������� �׷����� ������ ���ڸ� 2���� ������ �׸���.
		CRgn rgn;
		rgn.CreateRectRgnIndirect(rcLeft);
		dc.SelectClipRgn(&rgn);
		dc.SetTextColor(cr_text.ToCOLORREF());

		if ((m_text_style == text_style_dual_text))
		{
			dc.DrawText(_T("  ") + str, m_rc, DT_VCENTER | DT_LEFT | DT_SINGLELINE);
			//dc.SetTextColor(m_cr_active);// m_crBack);
			dc.DrawText(str_dual + _T("  "), m_rc, DT_VCENTER | DT_RIGHT | DT_SINGLELINE);
		}
		else
		{
			dc.DrawText(str, m_rc, DT_VCENTER | DT_CENTER | DT_SINGLELINE);
		}

		rgn.SetRectRgn(rcRight);
		dc.SelectClipRgn(&rgn);
		dc.SetTextColor(cr_active.ToCOLORREF()); //m_crText);

		if ((m_text_style == text_style_dual_text))
		{
			dc.DrawText(_T("  ") + str, m_rc, DT_VCENTER | DT_LEFT | DT_SINGLELINE);
			//dc.SetTextColor(m_cr_text);
			dc.DrawText(str_dual + _T("  "), m_rc, DT_VCENTER | DT_RIGHT | DT_SINGLELINE);
		}
		else
		{
			dc.DrawText(str, m_rc, DT_VCENTER | DT_CENTER | DT_SINGLELINE);
		}

		dc.SelectClipRgn(NULL);
	}
	else if (m_style == style_progress_line)
	{
		if (GetPos() > lower)
		{
			int track_height = 10;
			int marginy = MAX(0, (m_rc.Height() - track_height) / 2);
			CRect rtrack = m_rc;
			rtrack.top = m_rc.CenterPoint().y - marginy;
			rtrack.bottom = m_rc.CenterPoint().y + marginy;

			Gdiplus::Pen pen(m_cr_active, track_height);
			pen.SetLineCap(Gdiplus::LineCapRound, Gdiplus::LineCapRound, Gdiplus::DashCapRound);

			int end = MAX(0, pxpos - rtrack.Height() / 2 - 1);
			g.DrawLine(&pen, rtrack.left + rtrack.Height() / 2, rtrack.CenterPoint().y, end, rtrack.CenterPoint().y);
		}
	}
	else if (m_style == style_track)
	{
		CRect r = rtrack;
		int cy = r.CenterPoint().y;
		r.top = cy - 2;
		r.bottom = cy + 2;
		r.left += 2;
		r.right = pxpos;

		if (r.right > rtrack.right - 2)
			r.right = rtrack.right - 2;

		//CRect	rActive(0, m_rc.top + 2, pxpos, m_rc.bottom - 2);
		if (r.right > r.left)
			dc.FillSolidRect(r, cr_active.ToCOLORREF());
	}
	else if (m_style == style_step)
	{
		CRect r = m_steps[pos].r;

		if (m_step_completed)
		{
			g.FillEllipse(&Gdiplus::SolidBrush(cr_thumb), CRect2GpRect(m_steps[pos].r));
			draw_line(&dc, r.CenterPoint().x - 3, r.CenterPoint().y - 0, r.CenterPoint().x - 1, r.CenterPoint().y + 2, Gdiplus::Color::White, 1.0f);
			draw_line(&dc, r.CenterPoint().x - 1, r.CenterPoint().y + 2, r.CenterPoint().x + 3, r.CenterPoint().y - 2, Gdiplus::Color::White, 1.0f);
		}
		else
		{
			r.DeflateRect(4, 4);
			g.FillEllipse(&Gdiplus::SolidBrush(cr_thumb), CRect2GpRect(r));
		}
	}
#endif
	
	//for test
	//DrawRectangle(&dc, track, red);
	//DrawRectangle(&dc, m_rc, red);


	//tic ǥ��
	if (m_tic_freq > 0)
	{
		int	tic = lower;
		int	tic_pos = 0;
		CRect rtext;
		CString text;

		if (m_is_vertical)
		{
			while (tic <= upper)
			{
				tic_pos = Pos2Pixel(tic);
				//dc.FillSolidRect(m_rc.left, tic_pos - 1, m_rc.Width(), 2, cr_text);
				//g.FillEllipse(&Gdiplus::SolidBrush(gcr_thumb), CRectTogpRect(r));
				tic += m_tic_freq;
			}
		}
		else
		{
			dc.SetBkMode(TRANSPARENT);
			dc.SetTextColor(cr_text.ToCOLORREF());

			while (tic <= upper)
			{
				tic_pos = Pos2Pixel(tic);
				CRect rtic = make_center_rect(tic_pos, rtrack.CenterPoint().y - 1, 7, 7);
				g.FillEllipse(&Gdiplus::SolidBrush(cr_tic), CRect2GpRect(rtic));

				if (m_tic_show_text)
				{
					//range�� 0 ~ 100�� �� 0�� ����� ǥ�õ����� 100�� ������ �߸��� �ȴ�.
					//�̸� �����Ϸ��� Ʈ�� ������ �ؽ�Ʈ�� �߸��� �ʰ� �ٿ���� ������ ��ǥ ������ �������� ����������.
					//���� �ؽ�Ʈ�� -6 ~ +6���� 12�� width�� �ְ� center �����Ͽ� ǥ���ϰ� �� ������ ƽ�� ������ ���ķ� ó���Ѵ�.
					rtext = CRect(tic_pos - 6, rtrack.bottom, tic_pos + 6, m_rc.bottom);

					if (m_text_style == text_style_percentage)
						text.Format(_T("%d%%"), tic);
					else
						text.Format(_T("%d"), tic);

					if (tic == lower)
						dc.DrawText(text, rtext, DT_LEFT | DT_TOP | DT_NOCLIP);
					else if (tic == upper)
						dc.DrawText(text, rtext, DT_RIGHT | DT_TOP | DT_NOCLIP);
					else
						dc.DrawText(text, rtext, DT_CENTER | DT_TOP | DT_NOCLIP);
				}
				tic += m_tic_freq;
			}

			//������ ƽ�� �׸���.
			if ((tic > upper) && (tic < upper + m_tic_freq))
			{
				tic = upper;
				tic_pos = Pos2Pixel(upper);
				CRect rtic = make_center_rect(tic_pos, rtrack.CenterPoint().y - 1, 7, 7);
				g.FillEllipse(&Gdiplus::SolidBrush(cr_thumb), CRect2GpRect(rtic));

				if (m_tic_show_text)
				{
					rtext = CRect(tic_pos - 6, rtrack.bottom, tic_pos + 6, m_rc.bottom);

					if (m_text_style == text_style_percentage)
						text.Format(_T("%d%%"), tic);
					else
						text.Format(_T("%d"), tic);

					dc.DrawText(text, rtext, DT_RIGHT | DT_TOP | DT_NOCLIP);
				}
			}
		}
	}

	// ������(thumb)�� �׸���
#if 1
	if (!m_thumb_hide && m_style <= style_value)
	{
		CRect	rThumb = make_center_rect(pxpos, rtrack.CenterPoint().y, m_thumb.cx, m_thumb.cy);

		if (false)//!IsWindowEnabled())
		{
		}
		else if (m_style == style_thumb)
		{
			CBrush br(cr_thumb.ToCOLORREF());

			int		n = 4;										//���� ���� ����
			double	dx = (double)m_thumb.cx * 0.84 / double(n + 1);	//���� ����
			double	sx = pxpos - 0.5 * double(n - 1) * dx;		//���� ���� ǥ�� ���� ��ǥ

			CPen pen(PS_SOLID, 1, m_cr_thumb_darker.ToCOLORREF());
			pOldPen = (CPen*)dc.SelectObject(&pen);//NULL_PEN);
			pOldBrush = (CBrush*)dc.SelectObject(&br);

			dc.RoundRect(rThumb, CPoint(6, 6));

			for (i = 0; i < n; i++)
			{
				//dc.SelectObject(&m_penThumbDarker);
				//dc.MoveTo(sx + (double)i * dx + 1, cy - 3);
				//dc.LineTo(sx + (double)i * dx + 1, cy + 4);
				draw_line(&dc, (int)(sx + (double)i * dx + 1), cy - 4, (int)(sx + (double)i * dx + 1), cy + 3, m_cr_thumb_darker, 1.0f);

				//dc.SelectObject(&m_penThumbLighter);
				//dc.MoveTo(sx + (double)i * dx, cy - 3);
				//dc.LineTo(sx + (double)i * dx, cy + 4);
				draw_line(&dc, (int)(sx + (double)i * dx), cy - 4, (int)(sx + (double)i * dx), cy + 3, m_cr_thumb_lighter, 1.0f);
			}

			dc.SelectObject(pOldPen);
			dc.SelectObject(pOldBrush);
			br.DeleteObject();
			pen.DeleteObject();
		}
		else if (m_style == style_thumb_round)
		{
			//dc.FillSolidRect(rThumb, m_cr_thumb);
			Gdiplus::Color cr_pen(255, 255, 0, 0);
			Gdiplus::Pen pen(enable_color(cr_pen), 1.0f);
			Gdiplus::Color cr_brush_outer(96, m_cr_thumb.GetRed(),m_cr_thumb.GetGreen(), m_cr_thumb.GetBlue());
			Gdiplus::SolidBrush brush_outer(enable_color(cr_brush_outer));
			Gdiplus::Color cr_brush_inner = get_color(m_cr_thumb, -64);// (255, GetRValue(m_cr_thumb - 64), GetGValue(m_cr_thumb - 64), GetBValue(m_cr_thumb - 64));
			Gdiplus::SolidBrush brush_inner(enable_color(cr_brush_inner));

			CRect r = rThumb;
			r.DeflateRect(4, 4);

			g.FillEllipse(&brush_outer, Gdiplus::Rect(rThumb.left, rThumb.top, rThumb.Width(), rThumb.Height()));
			g.FillEllipse(&brush_inner, Gdiplus::Rect(r.left, r.top, r.Width(), r.Height()));
			//g.DrawEllipse(&pen, Gdiplus::Rect(rThumb.left, rThumb.top, rThumb.Width(), rThumb.Height()));
		}
		else if (m_style == style_value)
		{
			CBrush br(cr_thumb.ToCOLORREF());
			pOldBrush = (CBrush*)dc.SelectObject(&br);
			CPen pen(PS_SOLID, 1, m_cr_thumb_darker.ToCOLORREF());
			pOldPen = (CPen*)dc.SelectObject(&pen);//NULL_PEN);

			rThumb.top = m_rc.top;
			rThumb.bottom = m_rc.bottom;

			if (m_text_style > text_style_none)
			{
				dc.SetTextColor(cr_text.ToCOLORREF());
				dc.SetBkMode(TRANSPARENT);

				if (m_text_style == text_style_value)
					str.Format(_T("%ld / %ld"), pos, (upper > 0 ? upper : 0));
				else if (m_text_style == text_style_user_defined)
					str = m_text;
				else if (upper == lower)
					str = _T("0.0%");
				else
					str.Format(_T("%.1f%%"), (double)(pos - lower) / (double)(upper - lower) * 100.0);

				//�ؽ�Ʈ�� thumb�� ����ϴµ� �� �ʺ� �۴ٸ� �÷��ش�.
				int		margin = 8;
				int		minWidth;
				CSize	sz;
				sz = dc.GetTextExtent(str);

				//if (rText.Width() + margin > minWidth)
					minWidth = sz.cx + margin;

				if (m_thumb.cx != minWidth)
				{
					m_thumb.cx = minWidth;

					rThumb = CRect(pxpos - m_thumb.cx / 2, m_rc.top, pxpos + m_thumb.cx / 2, m_rc.bottom);
				}					

				dc.RoundRect(rThumb, CPoint(6, 6));
			}
			else
			{
				dc.RoundRect(rThumb, CPoint(6, 6));
			}

			dc.DrawText(str, rThumb, DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_NOCLIP);

			dc.SelectObject(pOldPen);
			dc.SelectObject(pOldBrush);
			br.DeleteObject();
			pen.DeleteObject();
		}
	}
	//Ʈ���� �ϸ�ũ�� �׸���.
	else if (m_style == style_track)
	{
		if (m_use_bookmark)
		{
			for (i = 0; i < m_bookmark.size(); i++)
			{
				pxpos = Pos2Pixel(m_bookmark[i].pos);
				//dc.FillSolidRect(pxpos-2, cy-8, 4, 4, (i == m_cur_bookmark ? m_crBookmarkCurrent : m_crBookmark));
				CPen pen(PS_SOLID, 1, (i == m_cur_bookmark ? enable_color(m_cr_bookmark_current).ToCOLORREF() : enable_color(m_cr_bookmark).ToCOLORREF()));
				CPen* pOldPen = (CPen*)dc.SelectObject(&pen);
				dc.SetPixel(pxpos, cy - 5, (i == m_cur_bookmark ? enable_color(m_cr_bookmark_current).ToCOLORREF() : enable_color(m_cr_bookmark).ToCOLORREF()));

				for (int j = 1; j < 5; j++)
				{
					dc.MoveTo(pxpos - j, cy - 5 - j);
					dc.LineTo(pxpos + j + 1, cy - 5 - j);	//���ﰢ��
					//dc.LineTo(pxpos + 1, cy - 5 - j);		//�������ﰢ��
				}
				dc.SelectObject(pOldPen);
			}
		}

		dc.SetBkMode(TRANSPARENT);
		dc.SetTextColor(RGB(0,255,0));

		LOGFONT lf;
		memcpy(&lf, &m_lf, sizeof(LOGFONT));
		lf.lfHeight = -8;
		lf.lfWeight = FW_ULTRABOLD;
		//lf.lfQuality = ANTIALIASED_QUALITY;
		lf.lfWidth = 8;

		CFont font_bracket;
		font_bracket.CreateFontIndirect(&lf);
		pOldFont = (CFont*)dc.SelectObject(&font_bracket);
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

		font_bracket.DeleteObject();
		dc.SelectObject(pOldFont);
	}
#endif
	// ��Ŀ�� �簢���� �׸���
	if(m_draw_focus_rect && m_has_focus && !IsWindowEnabled())
	{
		dc.DrawFocusRect(m_rc);
	}

	if (m_draw_progress_border)
	{
		rtrack.left = 0;
		rtrack.right = m_rc.right;
		draw_rectangle(&dc, rtrack, m_cr_progress_border);// , NULL_BRUSH, m_border_width, m_border_pen_style);
	}

	dc.SelectObject(pOldFont);
	dc.SelectObject(pOldFont);
	dc.SelectObject(pOldPen);

	//���� Ȯ�ο�
	//draw_rectangle(g, m_rc, Gdiplus::Color::Blue);

	// Do not call CSliderCtrl::OnPaint() for painting messages
}

BOOL CSCSliderCtrl::OnEraseBkgnd(CDC* pDC) 
{
	// TODO: Add your message handler code here and/or call default
	return FALSE;
	
	return CSliderCtrl::OnEraseBkgnd(pDC);
}

// ��Ʈ���� ��ġ���� �ش��ϴ� ������ �߽��� ��ǥ���� ���Ѵ�
int CSCSliderCtrl::Pos2Pixel(int nPos)
{
	int lower = GetRangeMin();
	int upper = GetRangeMax();

	if (upper == lower)
		return (m_style <= style_value ? m_thumb.cx / 2 : 0);

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
		return 
			m_margin.left + (m_style <= style_value ? m_thumb.cx / 2 : 0) +
			(int)(
			(double)(m_rc.Width() - m_margin.left - m_margin.right - (m_style <= style_value ? m_thumb.cx : 0)) *
			((double)(nPos - lower) / (double)(upper - lower))
			);
	}
}

// ���콺�� ��ǥ���� �ش��ϴ� ��Ʈ���� ��ġ���� ���Ѵ�
//
int CSCSliderCtrl::Pixel2Pos(int nPixel)
{
	int lower = GetRangeMin();
	int upper = GetRangeMax();
	int pos;

	if(m_is_vertical)
	{
		pos = int(
			lower +
			(double)(nPixel - m_margin.top - (double)m_thumb.cy/2.0) /
			(double)(m_rc.Height() - m_margin.bottom - m_margin.top - (m_style <= style_value ? m_thumb.cy : 0)) *
			(double)(upper - lower)
			);
		Clamp(pos, lower, upper);
	}
	else
	{
		pos = int(
			lower +
			(double)(nPixel - m_margin.left - (m_style <= style_value ? m_thumb.cx/2 : 0)) /
			(double)(m_rc.Width() - m_margin.left - m_margin.right - (m_style <= style_value ? m_thumb.cx : 0)) *
			(double)(upper - lower)
			);
		Clamp(pos, lower, upper);
	}

	return pos;
}

void CSCSliderCtrl::OnLButtonDown(UINT nFlags, CPoint point) 
{
	if (m_style == style_normal)
	{
		CSliderCtrl::OnLButtonDown(nFlags, point);
		return;
	}

	if (m_use_slide == false)
		return;

	/*
	int pos = GetPos();

	// TODO: Add your message handler code here and/or call default
	if (m_use_bookmark && (m_cur_bookmark >= 0))
	{
		SetPos(m_bookmark[m_cur_bookmark].pos);
		::SendMessage(GetParent()->GetSafeHwnd(), Message_CSCSliderCtrl, (WPARAM)&CSCSliderCtrlMsg(CSCSliderCtrlMsg::msg_thumb_move, GetDlgCtrlID(), pos), 0);
		return;
	}
	*/
	/*
	int		lower;
	int		upper;
	CRect	rc;

	GetRange(lower, upper);
	GetClientRect(&rc);

	int pos = (int)((double)point.x * (double)(upper - lower) / (double)rc.right) + lower;
	TRACE(_T("lower = %d, upper = %d, pos = %d\n"), lower, upper, pos);
	SetPos(pos);
	*/
	m_lbuttondown = true;
	OnMouseMove(nFlags, point);
	SetCapture();
	//SetFocus();

#if 0
	// �����̸� ���콺�� Ŭ������ ��
	// ���콺�� ��ǥ�� ������ �߽��� ��ǥ�� ���Ͽ�
	// �� ������ �Ÿ��� ���صд�
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
		::SendMessage(GetParent()->GetSafeHwnd(), Message_CSCSliderCtrl, (WPARAM)&CSCSliderCtrlMsg(CSCSliderCtrlMsg::msg_thumb_grab, GetDlgCtrlID(), pos), 0);
	}
	else if (m_nEventMsgStyle == msg_style_post)
	{
		::PostMessage(GetParent()->GetSafeHwnd(), Message_CSCSliderCtrl, (WPARAM)&CSCSliderCtrlMsg(CSCSliderCtrlMsg::msg_thumb_grab, GetDlgCtrlID(), pos), 0);
	}
	else if (m_nEventMsgStyle == msg_style_callback)
	{
		//if (m_pCallback_func != NULL)
			//(*(m_pCallback_func))(m_pParentWnd, this, MESSAGE_SCSLIDERCTRL_THUMB_GRAB, pos);
	}

	//OnMouseMove(nFlags, point);
#endif

	//�� �ڵ带 ����θ� ���� WM_PAINT�� �߻��Ͽ� ������ pos�� ���� pos�� �ٸ��� �����Ǵ� ������ �߻��Ѵ�.
	//���Ƶ־߸� ���� �����Ѵ�.
	//CSliderCtrl::OnLButtonDown(nFlags, point);
}

void CSCSliderCtrl::OnLButtonUp(UINT nFlags, CPoint point) 
{
	// TODO: Add your message handler code here and/or call default
	if (m_style == style_normal)
	{
		CSliderCtrl::OnLButtonUp(nFlags, point);
		return;
	}

	if (m_use_slide == false)
		return;

	if(!IsWindowEnabled() || !m_lbuttondown)
		return;


	m_lbuttondown = false;
	ReleaseCapture();
	Invalidate();
	/*
	int pos = GetPos();
	if (m_nEventMsgStyle == msg_style_timer)
	{
		::SendMessage(GetParent()->GetSafeHwnd(), Message_CSCSliderCtrl, (WPARAM)&CSCSliderCtrlMsg(CSCSliderCtrlMsg::msg_thumb_release, GetDlgCtrlID(), pos), 0);
	}
	else if (m_nEventMsgStyle == msg_style_post)
	{
		::PostMessage(GetParent()->GetSafeHwnd(), Message_CSCSliderCtrl, (WPARAM)&CSCSliderCtrlMsg(CSCSliderCtrlMsg::msg_thumb_release, GetDlgCtrlID(), pos), 0);
	}
	else if (m_nEventMsgStyle == msg_style_callback)
	{
		//if (m_pCallback_func != NULL)
			//(*(m_pCallback_func))(m_pParentWnd, this, MESSAGE_SCSLIDERCTRL_THUMB_RELEASE, pos);
	}
	*/
	
	CSliderCtrl::OnLButtonUp(nFlags, point);
}

void CSCSliderCtrl::OnMouseMove(UINT nFlags, CPoint point) 
{
	if (m_style == style_normal)
	{
		CSliderCtrl::OnMouseMove(nFlags, point);
		return;
	}

	// TODO: Add your message handler code here and/or call default
	//int pos = GetPos();

	int		lower;
	int		upper;
	CRect	rc;

	GetRange(lower, upper);
	GetClientRect(&rc);

	int pos = (int)((double)point.x * (double)(upper - lower) / (double)rc.right) + lower;
	Clamp(pos, lower, upper);

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
				//� �ϸ�ũ ��ġ�� ���� ���콺�� ��ó���
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
		
		redraw_window();
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


	if (m_lbuttondown && IsWindowEnabled())
	{
		SetPos(pos);
		::SendMessage(GetParent()->GetSafeHwnd(), Message_CSCSliderCtrl, (WPARAM)&CSCSliderCtrlMsg(CSCSliderCtrlMsg::msg_thumb_move, this, pos), 0);
	}

	//�� �ڵ带 ��������� thumb������ ���콺�� Ŭ������ �ʰ� �������� WM_PAINT�� ȣ��Ǵ� ������ �߻��Ѵ�.
	//�켱 �ּ�ó���Ѵ�.
	//CSliderCtrl::OnMouseMove(nFlags, point);
}

// PrepareMask
//
// "Drawing Transparent Bitmap with ease with on the fly masks in MFC"
// By Raja Segar
//
// clrpTransColor�� �⺻���� NULL(black)����
// 0xFF000000(RGB ���� �ƴ� ��)���� �ٲ���ϴ�.
//
void CSCSliderCtrl::PrepareMask(CBitmap *pBmpSource, CBitmap *pBmpMask,
								 Gdiplus::Color cr_transparent)
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
	COLORREF clrTrans = cr_transparent.ToCOLORREF();

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
/*
void CSCSliderCtrl::set_back_color(Gdiplus::Color crBack)
{
	m_theme.cr_back = crBack;
	Invalidate();
}
*/
void CSCSliderCtrl::set_active_color(Gdiplus::Color cr_active)
{
	m_cr_active = cr_active;
	Invalidate();
}

void CSCSliderCtrl::set_inactive_color(Gdiplus::Color cr_inActive)
{
	m_cr_inactive = cr_inActive;
	Invalidate();
}

void CSCSliderCtrl::set_thumb_color(Gdiplus::Color cr_thumb)
{
	int nMultiple = 32;

	m_cr_thumb			= cr_thumb;
	m_cr_thumb_light	= get_color(m_cr_thumb, nMultiple);
	m_cr_thumb_lighter	= get_color(m_cr_thumb, nMultiple * 2);
	m_cr_thumb_dark		= get_color(m_cr_thumb, -nMultiple);
	m_cr_thumb_darker	= get_color(m_cr_thumb, -nMultiple * 2);
	Invalidate();
}

void CSCSliderCtrl::set_tic_color(Gdiplus::Color cr_tic)
{
	m_cr_tic = cr_tic;
	Invalidate();
}

BOOL CSCSliderCtrl::PreTranslateMessage(MSG* pMsg)
{
	if (m_use_tooltip && m_tooltip.m_hWnd)
		m_tooltip.RelayEvent(pMsg);
	// TODO: Add your specialized code here and/or call the base class 
	if (pMsg->message == WM_KEYDOWN)
	{
		switch (pMsg->wParam)
		{
			case VK_LEFT :
				step(-1);
				Invalidate();
				return TRUE;
			case VK_RIGHT :
				step(1);
				Invalidate();
				return TRUE;
		}

		return false;
	}

	return CSliderCtrl::PreTranslateMessage(pMsg);
}

/*
void CSCSliderCtrl::OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags)
{
	// TODO: Add your message handler code here and/or call default
	TRACE(_T("CSCSliderCtrl::OnKeyDown = %d\n"), nChar);
	if (m_use_slide == false)
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

	//m_track_height = 5;

	if (m_style == style_thumb)
	{
		m_track_height = 8;
		m_thumb = CSize(28, m_track_height + 6);
	}
	else if (m_style == style_thumb_round)
	{
		m_thumb = CSize(16, 16);
		m_track_height = 2;
	}
	else if (m_style == style_value)
	{
		m_thumb = CSize(56, 8);
	}
	else if (m_style == style_progress)
	{
		m_use_slide = false;
	}
	else if (m_style == style_step)
	{
		m_thumb = CSize(22, 22);
		m_use_slide = false;
	}
}


void CSCSliderCtrl::OnTimer(UINT_PTR nIDEvent)
{
	// TODO: Add your message handler code here and/or call default
	if (nIDEvent == timer_post_pos)
	{
		KillTimer(timer_post_pos);
		::SendMessage(GetParent()->GetSafeHwnd(), Message_CSCSliderCtrl, (WPARAM)&CSCSliderCtrlMsg(CSCSliderCtrlMsg::msg_thumb_move, this, GetPos()), 0);
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

//pos�� ��ġ�� �ִ� �ϸ�ũ�� �ε����� �����Ѵ�.
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
			::SendMessage(GetParent()->GetSafeHwnd(), Message_CSCSliderCtrl, (WPARAM)&CSCSliderCtrlMsg(CSCSliderCtrlMsg::msg_thumb_move, this, pos), 0);
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
		//���� �ϸ�ũ�� �̵��� ���� �� 3�� ���� �ð��� �귯�� �� ���� �ϸ�ũ�� �̵�������� �Ѵ�.
		if ((pos >= m_bookmark[i].pos + (forward ? 0 : 3000)) && (pos <= m_bookmark[i+1].pos + (forward ? 0 : 3000)))
		{
			if (forward)
			{
				//0���� �˻��ϹǷ� pos�� i+1�� ���� i+2�� ��������� ���� �ϸ�ũ ��ġ�� �̵��ȴ�.
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

	//�� ������ �ϸ�ũ���� ū ��ġ���� backward�� ���� ���
	if ((m_bookmark.size() >= 1) && (pos >= m_bookmark[m_bookmark.size() - 1].pos))
		return m_bookmark.size() - 1;

	return index;
}

void CSCSliderCtrl::set_bookmark_color(Gdiplus::Color cr)
{
	m_cr_bookmark = cr;
	Invalidate();
}

void CSCSliderCtrl::set_bookmark_current_color(Gdiplus::Color cr)
{
	m_cr_bookmark_current = cr;
	Invalidate();
}

void CSCSliderCtrl::PreSubclassWindow()
{
	// TODO: Add your specialized code here and/or call the base class
	CSliderCtrl::PreSubclassWindow();

	CWnd* pWnd = GetParent();
	CFont* font = NULL;

	if (pWnd)
		font = pWnd->GetFont();

	if (font == NULL)
		GetObject(GetStockObject(SYSTEM_FONT), sizeof(m_lf), &m_lf);
	else
		font->GetObject(sizeof(m_lf), &m_lf);

	//CSCSliderCtrl�� �������� �����ϸ� font = NULL�̰� m_lf�� �⺻������ ä�����µ� �� �� lfWidth�� 8�� ��������.
	//�̷� ��� lfHeight�� �ٲ㵵 lfWidth�� ������� �ʾ� 8������ ���� �۲��� ���� ���ڷ� ǥ�õȴ�.
	//�� ���� 0���� ���� �����ؾ� lfHeight�� ���� ������ width�� �ڵ� ���Ǿ� �׷�����.
	m_lf.lfWidth = 0;

	reconstruct_font();

	/*
	m_tooltip.Create(this, TTS_ALWAYSTIP | TTS_NOPREFIX | TTS_NOANIMATE);
	m_tooltip.SetDelayTime(TTDT_AUTOPOP, -1);
	m_tooltip.SetDelayTime(TTDT_INITIAL, 0);
	m_tooltip.SetDelayTime(TTDT_RESHOW, 0);
	m_tooltip.SetMaxTipWidth(400); 
	m_tooltip.AddTool(this, _T(""));
	m_tooltip.Activate(TRUE);
	*/
}

void CSCSliderCtrl::reconstruct_font()
{
	m_font.DeleteObject();
	BOOL bCreated = m_font.CreateFontIndirect(&m_lf);

	ASSERT(bCreated);
}

void CSCSliderCtrl::OnWindowPosChanged(WINDOWPOS* lpwndpos)
{
	CSliderCtrl::OnWindowPosChanged(lpwndpos);

	// TODO: Add your message handler code here
	Invalidate();
}

Gdiplus::Color CSCSliderCtrl::enable_color(Gdiplus::Color cr, int offset)
{
	if (IsWindowEnabled() && !m_forced_gray)
		return cr;

	return get_color(gray_color(cr), offset);
}

void CSCSliderCtrl::set_repeat_range(int start, int end)
{
	set_repeat_start(start);
	set_repeat_end(end);
}

//pos�� 0�̻��̸� �� Ʈ�� ��ġ�� start �Ǵ� end�� �����ϰ�
//-2�̸� ���� ��ġ��, -1�� ������ �ǹ̷� ó���Ѵ�.
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

	//���۰� ���� ������ �Ǹ� end�� ������ ����
	if ((m_repeat_start >= 0) &&
		(m_repeat_end >= 0) &&
		(m_repeat_start >= m_repeat_end))
		m_repeat_end = GetRangeMax();

	Invalidate();
}

//pos�� 0�̻��̸� �� Ʈ�� ��ġ�� start �Ǵ� end�� �����ϰ�
//-2�̸� ���� ��ġ��, -1�� ������ �ǹ̷� ó���Ѵ�.
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

	//���۰� ���� ������ �Ǹ� start�� �������� ����
	if ((m_repeat_start >= 0) &&
		(m_repeat_end >= 0) &&
		(m_repeat_end <= m_repeat_start))
		m_repeat_start = GetRangeMin();

	Invalidate();
}

//pos ��ġ�� resource id �̹����� ǥ���Ѵ�.
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

void CSCSliderCtrl::redraw_window(bool bErase)
{
	if (m_transparent)
	{
		CRect rc;

		GetWindowRect(&rc);

		GetParent()->ScreenToClient(&rc);
		GetParent()->InvalidateRect(rc, bErase);
	}
	//else
	{
		Invalidate();
	}

	return;
}
/*
int CSCSliderCtrl::GetPos()
{

}
*/
void CSCSliderCtrl::set_pos(int pos)
{
	//CSCImageDlg���� animated gif�� ����ϴٰ� �����Ű�� ��Ȥ �� set_pos()�� ȣ���Ͽ� ������ �߻��Ѵ�.
	//�̹� �� �����̴��� m_hWnd�� NULL�� ���¿����� �Ʒ� �ڵ带 ó������ �ʾƾ� �Ѵ�.
	if (!m_hWnd)
		return;

	//style_step�� �� SetPos()�� ȣ���Ѵٴ� ���� ���� ó�����̰� pos == upper�� ���� �� ������ ������ ó�����̶�� ���̹Ƿ�
	//SetPos()������ m_step_completed�� �׻� false�̸� parent���� ��� ó���� �Ϸ�� ������ �� ���� true�� ��������� �Ѵ�.
	m_step_completed = false;
	m_thumb_hide = false;
	CSliderCtrl::SetPos(pos);

	if (!m_hWnd)
		return;

	Invalidate();
}

int CSCSliderCtrl::get_lower()
{
	int min, max;
	GetRange(min, max);
	return min;
}

int CSCSliderCtrl::get_upper()
{
	int min, max;
	GetRange(min, max);
	return max;
}

void CSCSliderCtrl::set_text(LPCTSTR text, ...)
{
	va_list args;
	va_start(args, text);

	m_text.FormatV(text, args);
	Invalidate();
}

void CSCSliderCtrl::set_text_dual(LPCTSTR text_dual, ...)
{
	va_list args;
	va_start(args, text_dual);

	m_text_dual.FormatV(text_dual, args);
	Invalidate();
}

//freq�� CSliderCtrl::SetTicFreq()�� �����ϰ� lower ~ upper ������ ������ �� ����� �������� �ƴ� ������ �ǹ��Ѵ�.
//��, freq = 23�̸� 23 ����� �ƴ϶� 0 ~ 23 ~ 46 ~ 69 ~ 92�� ���� ƽ�� ǥ�õȴ�.
void CSCSliderCtrl::set_tic_freq(int freq, bool show_text)
{
	m_tic_freq = freq;
	m_tic_show_text = show_text;

	if (style_normal)
		SetTicFreq(m_tic_freq);
}

void CSCSliderCtrl::set_log_font(LOGFONT lf)
{
	if (_tcslen(lf.lfFaceName) == 0)
		return;

	memcpy(&m_lf, &lf, sizeof(LOGFONT));
	reconstruct_font();
}

int CSCSliderCtrl::get_font_size()
{
	m_font_size = get_font_size_from_pixel_size(m_hWnd, m_lf.lfHeight);
	return m_font_size;
}

//���� ��� ��Ʈ ũ�⸦ 10���� �����ϸ�
void CSCSliderCtrl::set_font_size(int font_size)
{
	if (font_size == 0)
		return;

	m_font_size = font_size;
	//For the MM_TEXT mapping mode,
	//you can use the following formula to specify 
	//a height for a font with a specified point size:
	m_lf.lfHeight = get_pixel_size_from_font_size(m_hWnd, m_font_size);
	reconstruct_font();
}

void CSCSliderCtrl::set_font_name(LPCTSTR sFontname, BYTE byCharSet)
{
	m_lf.lfCharSet = byCharSet;
	_tcscpy_s(m_lf.lfFaceName, _countof(m_lf.lfFaceName), sFontname);
	reconstruct_font();
}

void CSCSliderCtrl::enlarge_font_size(bool enlarge)
{
	m_font_size = get_font_size();
	enlarge ? m_font_size++ : m_font_size--;
	m_lf.lfHeight = get_pixel_size_from_font_size(m_hWnd, m_font_size);
	reconstruct_font();
}

void CSCSliderCtrl::set_font_bold(int weight)
{
	m_lf.lfWeight = weight;
	reconstruct_font();
}

void CSCSliderCtrl::set_font_italic(bool italic)
{
	m_lf.lfItalic = italic;
	reconstruct_font();
}

//step ���� ����
int32_t CSCSliderCtrl::step(int step)
{
	int pos = GetPos() + step;
	SetPos(pos);
	::SendMessage(GetParent()->GetSafeHwnd(), Message_CSCSliderCtrl, (WPARAM)&CSCSliderCtrlMsg(CSCSliderCtrlMsg::msg_thumb_move, this, pos), 0);
	return GetPos();
}

void CSCSliderCtrl::set_color_theme(int theme)
{
	m_theme.set_color_theme(theme);
	m_cr_inactive = get_color(m_theme.cr_back, -64);

	//�⺻ m_theme���� ������ CSCSlider���� �ʿ��� �ٸ� �÷��鿡 ���� ���� ����
	m_cr_active = gRGB(64, 80, 181);//RGB(128, 192, 255);
	m_cr_inactive = gray_color(m_cr_active);
	m_cr_thumb = gRGB(64, 80, 181); //RGB(124, 192, 232);
	m_cr_tic = gray_color(m_cr_thumb);
}
