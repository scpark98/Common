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
	ON_WM_MOUSELEAVE()
	ON_WM_SETFOCUS()
	ON_WM_KILLFOCUS()
	//}}AFX_MSG_MAP
	ON_WM_KEYDOWN()
	ON_WM_TIMER()
	ON_WM_SIZE()
	ON_WM_WINDOWPOSCHANGED()
	ON_WM_MOUSEWHEEL()
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

	//style_step일 경우는 upper, lower에 따라 m_steps의 크기가 결정된다.
	if (m_style == style_step && m_steps.size() < upper - lower)
		m_steps.resize(upper - lower + 1);

	CPaintDC	dc1(this); // device context for painting
	CPen*		pOldPen = NULL;
	CBrush*		pOldBrush = NULL;
	CString		str, str_dual;

	//배경색은 OS 기본 disable 비주얼 (gray-out) 을 적용하지 않음 — customized 컨트롤이라
	//EnableWindow(FALSE) 만으로는 배경 유지. set_forced_gray(true, include_back=true) 로 호출자가 명시 요청한 경우에만 gray.
	Gdiplus::Color	cr_back = (m_forced_gray && m_forced_gray_include_back) ? get_color(get_gray_color(m_theme.cr_back), 64) : m_theme.cr_back;
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

	//실제 그려지는 게이지 영역은 thumb_size/2씩 좌우에서 빼야 한다.
	CRect rtrack = m_rc;

	if (m_style <= style_thumb_round_alpha || is_round_pill_style())
	{
		if (m_is_vertical)
		{
			rtrack.DeflateRect((m_rc.Width() - m_track_thick) / 2, m_thumb.cy / 2);
		}
		else
		{
			// style_thumb_round_alpha / hue / gradient: rtrack 전체가 큰 썸이므로
			// m_track_thick 대신 m_track_height의 높이를 사용한다
			const int track_h = (is_round_pill_style() ? (int)m_track_height : m_track_thick);
			// is_round_pill_style(): 트랙이 컨트롤 전체 너비를 사용한다.
			// → lower에서 thumb.left == track.left == 0
			// → upper에서 thumb.right == track.right == rc.Width()
			// Pos2Pixel/Pixel2Pos는 use_thumb_offset=true로 이미 올바르게 처리되므로 변경 불필요.
			const int h_margin = is_round_pill_style() ? 0 : m_thumb.cx / 2;
			rtrack.DeflateRect(h_margin, (m_rc.Height() - track_h) / 2);
		}
	}

	//m_rc의 y센터좌표
	int	cy = m_rc.CenterPoint().y;
	//현재 위치의 실제 픽셀좌표
	int	pxpos = Pos2Pixel(pos);
	//int	pxpos = int(((float)track.Width() * float(pos - lower)) / float(upper - lower));
	int i;

	//전체 슬라이드 사각형 영역을 배경색으로 그림
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
		dc.FillSolidRect(m_rc, cr_back.ToCOLORREF());
	}


	//inactive 영역을 먼저 그리고
	if (m_style <= style_value && !is_round_pill_style())
	{
		//tic text 표시 시 트랙 중심을 thumb 영역(+텍스트 공간) 기준으로 보정.
		float fill_cy = (float)cy;
		if (m_tic_show_text)
		{
			rtrack.top = (m_rc.Height() - (2 + m_thumb.cy + 10)) / 2;
			rtrack.bottom = rtrack.top + m_thumb.cy;
			fill_cy = (float)rtrack.CenterPoint().y;
		}

		//GDI+ float fill — m_track_height(float) 두께를 fill_cy 중심에 AA 로. 정수 FillSolidRect 와 달리 짝수/소수 두께도 cy 에 정확히 센터.
		g.SetSmoothingMode(Gdiplus::SmoothingModeAntiAlias);
		g.FillRectangle(&Gdiplus::SolidBrush(cr_inactive),
			Gdiplus::RectF((Gdiplus::REAL)pxpos, fill_cy - m_track_height / 2.0f, (Gdiplus::REAL)(rtrack.right - pxpos), m_track_height));

		if (m_style == style_value)
		{
			const int half = (int)(m_track_height / 2.0f);
			CPen	penDark(PS_SOLID, 1, get_color(cr_inactive, -24).ToCOLORREF());
			CPen	penLight(PS_SOLID, 1, get_color(cr_inactive, 36).ToCOLORREF());

			dc.SelectObject(&penDark);
			dc.MoveTo(pxpos, cy - half);
			dc.LineTo(rtrack.right, cy - half);

			dc.SelectObject(&penLight);
			dc.MoveTo(pxpos, cy + half);
			dc.LineTo(rtrack.right - 1, cy + half);
			dc.LineTo(rtrack.right - 1, cy - half);

			dc.SelectObject(pOldPen);
			penDark.DeleteObject();
			penLight.DeleteObject();
		}
	}
	else if (m_style == style_thumb_round_alpha)//투명도 선택 슬라이더
	{
		const Gdiplus::RectF track_rf(
			static_cast<Gdiplus::REAL>(rtrack.left),
			static_cast<Gdiplus::REAL>(rtrack.top),
			static_cast<Gdiplus::REAL>(rtrack.Width()),
			static_cast<Gdiplus::REAL>(rtrack.Height())
		);

		Gdiplus::GraphicsPath track_path;
		get_round_rect_path(&track_path,
			Gdiplus::Rect(rtrack.left, rtrack.top, rtrack.Width(), rtrack.Height()),
			static_cast<float>(rtrack.Height()) * 0.5f, 1);

		// rtrack 범위로 clip: fill/border의 antialiasing bleed 방지
		g.SetClip(Gdiplus::Rect(rtrack.left, rtrack.top, rtrack.Width(), rtrack.Height()));

		// ① 체커보드: rect clip과 path clip의 교집합으로 pill 형태 유지
		{
			Gdiplus::GraphicsState state = g.Save();
			g.SetClip(&track_path, Gdiplus::CombineModeIntersect);
			Gdiplus::TextureBrush tb(CSCGdiplusBitmap::checker_bmp(5), Gdiplus::WrapModeTile);
			g.FillRectangle(&tb,
				track_rf.X - 1.f, track_rf.Y - 1.f,
				track_rf.Width + 2.f, track_rf.Height + 2.f);
			g.Restore(state);	// rect clip으로 복원
		}

		// ② 좌(투명) → 우(불투명) 선형 그라디언트
		{
			const Gdiplus::Color cr_from(0, cr_active.GetR(), cr_active.GetG(), cr_active.GetB());
			const Gdiplus::Color cr_to(255, cr_active.GetR(), cr_active.GetG(), cr_active.GetB());
			Gdiplus::LinearGradientBrush grad(
				Gdiplus::PointF(track_rf.X, track_rf.Y),
				Gdiplus::PointF(track_rf.X + track_rf.Width, track_rf.Y),
				cr_from, cr_to
			);
			g.FillPath(&grad, &track_path);
		}

		// ③ 트랙 테두리
		{
			Gdiplus::Pen track_border(Gdiplus::Color(60, 0, 0, 0), 1.0f);
			g.DrawPath(&track_border, &track_path);
		}

		g.ResetClip();
	}
	else if (m_style == style_thumb_round_hue)	//무지개 hue 그라디언트
	{
		const Gdiplus::RectF track_rf(
			static_cast<Gdiplus::REAL>(rtrack.left),
			static_cast<Gdiplus::REAL>(rtrack.top),
			static_cast<Gdiplus::REAL>(rtrack.Width()),
			static_cast<Gdiplus::REAL>(rtrack.Height())
		);

		Gdiplus::GraphicsPath track_path;
		get_round_rect_path(&track_path,
			Gdiplus::Rect(rtrack.left, rtrack.top, rtrack.Width(), rtrack.Height()),
			static_cast<float>(rtrack.Height()) * 0.5f, 1);

		// rtrack 범위로 clip
		g.SetClip(Gdiplus::Rect(rtrack.left, rtrack.top, rtrack.Width(), rtrack.Height()));

		Gdiplus::LinearGradientBrush grad(
			Gdiplus::PointF(track_rf.X, track_rf.Y),
			Gdiplus::PointF(track_rf.X + track_rf.Width, track_rf.Y),
			Gdiplus::Color::Red, Gdiplus::Color::Red);

		Gdiplus::Color hue_colors[] = {
			Gdiplus::Color(255, 255,   0,   0),
			Gdiplus::Color(255, 255, 255,   0),
			Gdiplus::Color(255,   0, 255,   0),
			Gdiplus::Color(255,   0, 255, 255),
			Gdiplus::Color(255,   0,   0, 255),
			Gdiplus::Color(255, 255,   0, 255),
			Gdiplus::Color(255, 255,   0,   0),
		};
		Gdiplus::REAL positions[] = { 0.f, 1.f / 6.f, 2.f / 6.f, 3.f / 6.f, 4.f / 6.f, 5.f / 6.f, 1.f };
		grad.SetInterpolationColors(hue_colors, positions, 7);
		g.FillPath(&grad, &track_path);

		Gdiplus::Pen track_border(Gdiplus::Color(60, 0, 0, 0), 1.0f);
		g.DrawPath(&track_border, &track_path);

		g.ResetClip();
	}
	else if (m_style == style_thumb_round_gradient)
	{
		const Gdiplus::RectF track_rf(
			static_cast<Gdiplus::REAL>(rtrack.left),
			static_cast<Gdiplus::REAL>(rtrack.top),
			static_cast<Gdiplus::REAL>(rtrack.Width()),
			static_cast<Gdiplus::REAL>(rtrack.Height())
		);

		Gdiplus::GraphicsPath track_path;
		get_round_rect_path(&track_path,
			Gdiplus::Rect(rtrack.left, rtrack.top, rtrack.Width(), rtrack.Height()),
			static_cast<float>(rtrack.Height()) * 0.5f, 1);

		g.SetClip(Gdiplus::Rect(rtrack.left, rtrack.top, rtrack.Width(), rtrack.Height()));

		if (m_gradient_colors.size() >= 2)
		{
			// ── N-stop 그라디언트 (hue 슬라이더와 동일한 패턴) ──────
			const int n = static_cast<int>(m_gradient_colors.size());
			Gdiplus::LinearGradientBrush grad(
				Gdiplus::PointF(track_rf.X, track_rf.Y),
				Gdiplus::PointF(track_rf.X + track_rf.Width, track_rf.Y),
				m_gradient_colors.front(), m_gradient_colors.back());

			std::vector<Gdiplus::Color> colors(m_gradient_colors.begin(), m_gradient_colors.end());
			std::vector<Gdiplus::REAL>  positions(n);
			for (int i = 0; i < n; i++)
				positions[i] = static_cast<Gdiplus::REAL>(i) / static_cast<Gdiplus::REAL>(n - 1);

			grad.SetInterpolationColors(colors.data(), positions.data(), n);
			g.FillPath(&grad, &track_path);
		}
		else
		{
			// ── 폴백: 기존 2색 (cr_inactive → cr_active) ─────────
			Gdiplus::LinearGradientBrush grad(
				Gdiplus::PointF(track_rf.X, track_rf.Y),
				Gdiplus::PointF(track_rf.X + track_rf.Width, track_rf.Y),
				cr_inactive, cr_active);
			g.FillPath(&grad, &track_path);
		}

		Gdiplus::Pen track_border(Gdiplus::Color(60, 0, 0, 0), 1.0f);
		g.DrawPath(&track_border, &track_path);

		g.ResetClip();
	}
	else if (m_style == style_progress)
	{
		//트랙 두께는 정수 CRect+1 대신 float 중심 정렬(cy 기준)로 그린다. 기존 (int)(th/2)+1 은
		//bottom 을 1px 더 내려 위/아래가 비대칭(아래로 1px 더 칠해짐)이었다. style_value 등과 동일 방식으로 통일.
		rtrack = CRect(pxpos, m_rc.CenterPoint().y - (int)(m_track_height / 2), m_rc.right, m_rc.CenterPoint().y + (int)(m_track_height / 2));
		g.FillRectangle(&Gdiplus::SolidBrush(cr_inactive),
			Gdiplus::RectF((Gdiplus::REAL)rtrack.left, (float)cy - m_track_height / 2.0f, (Gdiplus::REAL)rtrack.Width(), m_track_height));
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
		r.bottom = cy + 5;

		//3D sunken bevel(좌상 그림자/우하 하이라이트)은 중간 명도 배경을 가정한 기법이라, 배경이 거의 검정/흰색이면
		//한쪽이 clamp 돼 묻히거나 방향이 뒤집혀(embossed) 테마마다 인상이 갈린다. → 방향성 없는 *평면 채널*로 그린다:
		//배경에서 살짝 벗어난(어두운 테마=조금 밝게/밝은 테마=조금 어둡게) 균일 톤의 둥근 홈 fill + 약한 1px 테두리.
		//어떤 테마에서도 일관된 "들어간 홈"으로 읽히고 양 극단에서도 항상 보인다(검정엔 더 어둡게 못 가니 밝게, 반대도).
		//(slider 는 cr_back 만 갱신되고 cr_text 는 stale 일 수 있어 cr_back 명도만 사용.)
		int dir = (get_luminance(m_theme.cr_back) < 128) ? +1 : -1;
		Gdiplus::Color cr_fill   = get_color(m_theme.cr_back, dir * 12);	//채널 바탕 — 아주 미세.
		Gdiplus::Color cr_border = get_color(m_theme.cr_back, dir * 28);	//1px 윤곽 — 살짝만.

		g.SetSmoothingMode(Gdiplus::SmoothingModeAntiAlias);
		draw_round_rect(&g, Gdiplus::Rect(r.left, r.top, r.Width(), r.Height()),
			cr_border, cr_fill, r.Height() / 2, 1);
	}
	else if (m_style == style_step)
	{
		CRect r = m_rc;
		int thumb_size = m_thumb.cx;// MIN(r.Width(), r.Height()); //스텝 위치마다 그려질 도형의 크기
		int interval;
		CSize gap(0, 0);	//스텝의 시작과 끝의 여백
			
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
				//각 step의 영역이 미리 계산된적이 없다면
				if (m_steps[i].r.IsRectEmpty())
				{
					m_steps[i].r = CRect(r.CenterPoint().x - thumb_size / 2, gap.cy + interval * i, r.CenterPoint().x + thumb_size / 2, gap.cy + interval * i + thumb_size);
					m_steps[i].r.DeflateRect(2, 2);	
				}

				//이미지가 지정되어 있다면
				if (m_steps[i].m_img.is_valid())
				{
					m_steps[i].m_img.draw(g, m_steps[i].r);
				}
				else
				{
					//g.DrawEllipse(&pen_thumb, CRect_to_gpRect(m_steps[i].r));
					g.FillEllipse(&Gdiplus::SolidBrush(cr_thumb), CRect_to_gpRect(m_steps[i].r));

					//pos 미만은 파란색 원에 체크 표시를
					if (i < pos)
					{
						draw_line(g, m_steps[i].r.CenterPoint().x - 3, m_steps[i].r.CenterPoint().y - 0, m_steps[i].r.CenterPoint().x - 1, m_steps[i].r.CenterPoint().y + 2, Gdiplus::Color::White, 1.0f);
						draw_line(g, m_steps[i].r.CenterPoint().x - 1, m_steps[i].r.CenterPoint().y + 2, m_steps[i].r.CenterPoint().x + 3, m_steps[i].r.CenterPoint().y - 2, Gdiplus::Color::White, 1.0f);
					}
					else if (i == pos)
					{
						CRect rthumb_small = m_steps[i].r;
						rthumb_small.DeflateRect(2, 2);
						g.FillEllipse(&Gdiplus::SolidBrush(Gdiplus::Color::White), CRect_to_gpRect(rthumb_small));
					}
					else
					{
						CRect rthumb_small = m_steps[i].r;
						rthumb_small.DeflateRect(1, 1);
						g.FillEllipse(&Gdiplus::SolidBrush(Gdiplus::Color::White), CRect_to_gpRect(rthumb_small));
					}
				}
			}

			//각 thumb들 사이에 라인을 그려준다.
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

				//이미지가 지정되어 있다면
				if (m_steps[i].m_img.is_valid())
				{
					m_steps[i].m_img.draw(g, m_steps[i].r);
				}
				else
				{
					//g.DrawEllipse(&pen_thumb, CRect_to_gpRect(m_steps[i].r));
					g.FillEllipse(&Gdiplus::SolidBrush(Gdiplus::Color(62, 134, 193)), CRect_to_gpRect(m_steps[i].r));
					CRect rthumb_small = m_steps[i].r;
					rthumb_small.DeflateRect(4, 4);
					g.FillEllipse(&Gdiplus::SolidBrush(Gdiplus::Color(255, 255, 255)), CRect_to_gpRect(rthumb_small));
				}
			}

			//라인
			pen_line.SetLineCap(Gdiplus::LineCapRound, Gdiplus::LineCapRound, Gdiplus::DashCapRound);
			for (i = 1; i <= upper - lower; i++)
			{
				g.DrawLine(i > pos ? &pen_gray : &pen_line, Gdiplus::Point(m_steps[i - 1].r.right + 4, r.CenterPoint().y),
					Gdiplus::Point(m_steps[i].r.left - 4, r.CenterPoint().y));
			}
		}
	}

	//경과된 영역(active area) 표시
#if 1
	if (m_style <= style_value && !is_round_pill_style())
	{
		//active 구간도 inactive 와 동일 중심. tic text 보정 시 rtrack 은 위 inactive 블록에서 이미 조정됨.
		float fill_cy = m_tic_show_text ? (float)rtrack.CenterPoint().y : (float)cy;
		g.SetSmoothingMode(Gdiplus::SmoothingModeAntiAlias);
		g.FillRectangle(&Gdiplus::SolidBrush(cr_active),
			Gdiplus::RectF((Gdiplus::REAL)rtrack.left, fill_cy - m_track_height / 2.0f, (Gdiplus::REAL)(pxpos - rtrack.left), m_track_height));

		if (m_style == style_value)
		{
			const int half = (int)(m_track_height / 2.0f);
			CPen	penDark(PS_SOLID, 1, get_color(cr_active, -64).ToCOLORREF());
			CPen	penLight(PS_SOLID, 1, get_color(cr_active, 64).ToCOLORREF());

			dc.SelectObject(&penDark);
			dc.MoveTo(pxpos, cy - half);
			dc.LineTo(rtrack.left, cy - half);
			dc.LineTo(rtrack.left, cy + half);

			dc.SelectObject(&penLight);
			dc.MoveTo(rtrack.left + 1, cy + half);
			dc.LineTo(pxpos, cy + half);

			dc.SelectObject(pOldPen);
			penDark.DeleteObject();
			penLight.DeleteObject();
		}
	}
	else if (m_style == style_progress)
	{
		//inactive 와 동일하게 float 중심 정렬. +1 제거로 active/inactive 의 상하 범위가 정확히 일치.
		rtrack = CRect(0, m_rc.CenterPoint().y - (int)(m_track_height / 2), pxpos, m_rc.CenterPoint().y + (int)(m_track_height / 2));
		g.FillRectangle(&Gdiplus::SolidBrush(cr_active),
			Gdiplus::RectF((Gdiplus::REAL)rtrack.left, (float)cy - m_track_height / 2.0f, (Gdiplus::REAL)rtrack.Width(), m_track_height));

		//m_crValueText = RGB(12, 162, 255);
		//m_cr_active = RGB(128,255,128);
		if (m_text_style > text_style_none)
		{
			dc.SetTextColor(cr_text.ToCOLORREF());
			dc.SetBkMode(TRANSPARENT);

			if (m_text_style == text_style_value)
			{
				str.Format(_T("%ld / %ld"), pos, (upper > 0 ? upper : 0));
			}
			else if (m_text_style == text_style_percentage)
			{
				if (upper == lower)
					str = _T("0%");
				else
					str.Format(_T("%.0f%%"), (double)(pos - lower) / (double)(upper - lower) * 100.0);
			}
			else if (m_text_style == text_style_user_defined)
			{
				str = m_text;
			}
			else if (m_text_style == text_style_dual_text)
			{
				str = m_text;
				str_dual = m_text_dual;
			}
			else if (upper == lower)
			{
				str = _T("0.0%");
			}
			else
			{
				str.Format(_T("%.1f%%"), (double)(pos - lower) / (double)(upper - lower) * 100.0);
			}
			//dc.DrawText(str, m_rc, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
		}

		//텍스트가 active, inactive 구간의 색상에 반전되도록 그려줘야한다.
		CRect rcLeft, rcRight;
		rcLeft = rcRight = m_rc;
		rcRight.left = rcLeft.right = pxpos;

		//두 영역을 생성하여 해당 영역에만 그려지는 원리로 글자를 2가지 색으로 그린다.
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

		//채널이 라운드(pill)라 진행 fill 도 라운드로 맞춘다 — plain rect 면 둥근 홈 안에 각진 막대가 들어가 어색.
		if (r.right > r.left)
		{
			g.SetSmoothingMode(Gdiplus::SmoothingModeAntiAlias);
			draw_round_rect(&g, Gdiplus::Rect(r.left, r.top, r.Width(), r.Height()),
				Gdiplus::Color::Transparent, cr_active, r.Height() / 2, 0);
		}
	}
	else if (m_style == style_step)
	{
		CRect r = m_steps[pos].r;

		if (m_step_completed)
		{
			g.FillEllipse(&Gdiplus::SolidBrush(cr_thumb), CRect_to_gpRect(m_steps[pos].r));
			draw_line(g, r.CenterPoint().x - 3, r.CenterPoint().y - 0, r.CenterPoint().x - 1, r.CenterPoint().y + 2, Gdiplus::Color::White, 1.0f);
			draw_line(g, r.CenterPoint().x - 1, r.CenterPoint().y + 2, r.CenterPoint().x + 3, r.CenterPoint().y - 2, Gdiplus::Color::White, 1.0f);
		}
		else
		{
			r.DeflateRect(4, 4);
			g.FillEllipse(&Gdiplus::SolidBrush(cr_thumb), CRect_to_gpRect(r));
		}
	}
#endif
	
	//tic 표시
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
				bool is_end_tic = (tic == lower || tic == upper);
				int tic_d = is_end_tic ? 8 : 6;
				CRect rtic = make_center_rect(tic_pos, rtrack.CenterPoint().y, tic_d, tic_d);
				g.FillEllipse(&Gdiplus::SolidBrush(is_end_tic ? cr_tic : get_weak_color(cr_tic)), CRect_to_gpRect(rtic));

				if (m_tic_show_text)
				{
					//range가 0 ~ 100일 때 0은 충분히 표시되지만 100은 우측이 잘리게 된다.
					//이를 보정하려면 트랙 양쪽을 텍스트가 잘리지 않게 줄여줘야 하지만 좌표 계산들이 여기저기 복잡해진다.
					//따라서 텍스트를 -6 ~ +6까지 12의 width를 주고 center 정렬하여 표시하고 맨 마지막 틱만 오른쪽 정렬로 처리한다.
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

			//마지막 틱을 그린다.
			if ((tic > upper) && (tic < upper + m_tic_freq))
			{
				tic = upper;
				tic_pos = Pos2Pixel(upper);
				CRect rtic = make_center_rect(tic_pos, rtrack.CenterPoint().y, 8, 8);
				g.FillEllipse(&Gdiplus::SolidBrush(cr_thumb), CRect_to_gpRect(rtic));

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

	// 손잡이(thumb)를 그린다
	if (!m_thumb_hide && (m_style <= style_value || is_round_pill_style()))	// ← 새 스타일 포함
	{
		CRect	rThumb = make_center_rect(pxpos, rtrack.CenterPoint().y, m_thumb.cx, m_thumb.cy);

		if (false)//!IsWindowEnabled())
		{
		}
		else if (m_style == style_thumb)
		{
			CBrush br(cr_thumb.ToCOLORREF());

			int		n = 4;										//세로 라인 개수
			double	dx = (double)m_thumb.cx * 0.84 / double(n + 1);	//라인 간격
			double	sx = pxpos - 0.5 * double(n - 1) * dx;		//수직 라인 표시 시작 좌표

			CPen pen(PS_SOLID, 1, m_cr_thumb_darker.ToCOLORREF());
			pOldPen = (CPen*)dc.SelectObject(&pen);//NULL_PEN);
			pOldBrush = (CBrush*)dc.SelectObject(&br);

			dc.RoundRect(rThumb, CPoint(6, 6));

			for (i = 0; i < n; i++)
			{
				//dc.SelectObject(&m_penThumbDarker);
				//dc.MoveTo(sx + (double)i * dx + 1, cy - 3);
				//dc.LineTo(sx + (double)i * dx + 1, cy + 4);
				draw_line(g, (int)(sx + (double)i * dx + 1), cy - 4, (int)(sx + (double)i * dx + 1), cy + 3, m_cr_thumb_darker, 1.0f);

				//dc.SelectObject(&m_penThumbLighter);
				//dc.MoveTo(sx + (double)i * dx, cy - 3);
				//dc.LineTo(sx + (double)i * dx, cy + 4);
				draw_line(g, (int)(sx + (double)i * dx), cy - 4, (int)(sx + (double)i * dx), cy + 3, m_cr_thumb_lighter, 1.0f);
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
		}
		else if (is_round_pill_style())
		{
			//제일 바깥쪽 테두리 원. 1을 줄이고 2두께로 그리는 것은 선과 선사이 공백이 나타나므로 겹치도록 그리기 위함.
			rThumb.DeflateRect(1, 1);
			g.DrawEllipse(&Gdiplus::Pen(Gdiplus::Color(255, 160, 160, 160), 2.0f),
				rThumb.left, rThumb.top, rThumb.Width(), rThumb.Height());

			//그 다음 테두리 원
			rThumb.DeflateRect(1, 1);
			g.DrawEllipse(&Gdiplus::Pen(Gdiplus::Color(255, 232, 232, 232), 3.0f),
				rThumb.left, rThumb.top, rThumb.Width(), rThumb.Height());

			// ③ 스타일별 내부 채우기 색상 결정
			rThumb.DeflateRect(2, 2);

			Gdiplus::Color cr_fill;
			if (m_style == style_thumb_round_alpha)
			{
				// 현재 pos = alpha 값 (0~255)
				const BYTE cur_alpha = static_cast<BYTE>(max(0, min(255, pos)));
				cr_fill = Gdiplus::Color(cur_alpha, cr_active.GetR(), cr_active.GetG(), cr_active.GetB());
			}
			else if (m_style == style_thumb_round_hue)
			{
				// 현재 hue 위치의 순색
				cr_fill = get_color(static_cast<float>(pos));
			}
			else if (m_style == style_thumb_round_gradient)
			{
				// N-stop 그라디언트에서 현재 위치의 색상을 샘플링
				const float t = (upper > lower)
					? static_cast<float>(pos - lower) / static_cast<float>(upper - lower)
					: 0.f;
				cr_fill = sample_gradient(t);
			}

			g.FillEllipse(&Gdiplus::SolidBrush(cr_fill),
				rThumb.left, rThumb.top, rThumb.Width(), rThumb.Height());
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

				//텍스트를 thumb에 출력하는데 그 너비가 작다면 늘려준다.
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
	//트랙의 북마크를 그린다.
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
					dc.LineTo(pxpos + j + 1, cy - 5 - j);	//역삼각형
					//dc.LineTo(pxpos + 1, cy - 5 - j);		//역직각삼각형
				}
				dc.SelectObject(pOldPen);
			}
		}

		//구간 반복 bracket — 다른 element 가 덮지 못하도록 OnPaint 의 *맨 마지막* 으로 이동됨 (아래 progress_border 다음 블록 참조).
	}

	// 포커스 사각형을 그린다
	if(m_draw_focus_rect && m_has_focus && !IsWindowEnabled())
	{
		dc.DrawFocusRect(m_rc);
	}

	if (m_draw_progress_border)
	{
		//채움은 float 중심 정렬로 rows [cy-th/2, cy+th/2) 를 칠한다(예: th=14 → cy-7..cy+6).
		//draw_rect(CDC) 는 내부에서 Height-- 후 PenAlignmentInset 으로 그리므로, 채움의 가장자리 행에
		//1px 테두리를 정확히 얹으려면 bottom 을 +1 보정해야 한다. 이 보정이 없으면 테두리 bottom 이
		//한 행 위로 올라가 채움 bottom 이 테두리 밖으로 1px 넘쳐 보인다.
		CRect rborder(0, cy - (int)(m_track_height / 2), m_rc.right, cy + (int)(m_track_height / 2) + 1);
		draw_rect(&dc, rborder, m_cr_progress_border);// , NULL_BRUSH, m_border_width, m_border_pen_style);
	}

	//구간 반복 bracket — 다른 모든 element 가 그려진 *후 맨 마지막* 에. FillSolidRect 직접.
	//세로와 가로의 x 좌표 완전 분리 — 세로 (cx ~ cx+VW), 가로 (cx+VW ~ cx+VW+HW). 겹침 없음.
	//bracket 세로 height = 트랙 두께 + 2 (위/아래 1px 씩 확장).
	if (m_style == style_track)
	{
		const COLORREF cr = RGB(64, 160, 64);
		const int VW = 1;                       //세로 두께
		const int HH = 1;                       //가로 두께
		const int HW = 2;                       //가로 폭 (세로 옆으로)
		const int cy = m_rc.CenterPoint().y;
		const int top = cy - (int)(m_track_height / 2) + 2;
		const int bot = cy + (int)(m_track_height / 2) - 2;
		const int h   = bot - top;

		if (m_repeat_start >= 0)
		{
			int cx = Pos2Pixel(m_repeat_start);
			dc.FillSolidRect(cx,        top,            VW, h,  cr);   //세로
			dc.FillSolidRect(cx + VW,   top,            HW, HH, cr);   //위 가로 (세로 *옆부터*)
			dc.FillSolidRect(cx + VW,   bot - HH,       HW, HH, cr);   //아래 가로 (세로 *옆부터*)
		}
		if (m_repeat_end >= 0)
		{
			int cx = Pos2Pixel(m_repeat_end);
			dc.FillSolidRect(cx - VW,       top,            VW, h,  cr);   //세로
			dc.FillSolidRect(cx - VW - HW,  top,            HW, HH, cr);   //위 가로 (세로 *옆부터, 왼쪽*)
			dc.FillSolidRect(cx - VW - HW,  bot - HH,       HW, HH, cr);   //아래 가로
		}
	}

	//[sub track] 다운로드된 seek 가능 frontier — 빨간 1px 세로선. style_track 전용. -1 이면 숨김.
	if (m_style == style_track && m_sub_track_pos >= 0)
	{
		int cx   = Pos2Pixel(m_sub_track_pos);
		int cyc  = m_rc.CenterPoint().y;
		int half = (int)(m_track_height / 2) + 2;	//그루브보다 살짝 돌출시켜 눈에 띄게.
		dc.FillSolidRect(cx, cyc - half, 1, half * 2, RGB(255, 0, 0));
	}

	dc.SelectObject(pOldFont);
	dc.SelectObject(pOldPen);

	//영역 확인용
	//draw_rect(g, m_rc, Gdiplus::Color::Blue);

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

	const bool use_thumb_offset = (m_style <= style_value || is_round_pill_style());

	if (upper == lower)
		return (use_thumb_offset ? m_thumb.cx / 2 : 0);

	if (m_is_vertical)
	{
		return
			m_margin.top + m_thumb.cy / 2 +
			(int)round(
				(double)(m_rc.Height() - m_margin.top - m_margin.bottom - m_thumb.cy) *
				((double)(nPos - lower) / (double)(upper - lower))
			);
	}
	else
	{
		return
			m_margin.left + (use_thumb_offset ? m_thumb.cx / 2 : 0) +
			(int)round(
				(double)(m_rc.Width() - m_margin.left - m_margin.right - (use_thumb_offset ? m_thumb.cx : 0)) *
				((double)(nPos - lower) / (double)(upper - lower))
			);
	}
}

// 마우스의 좌표값에 해당하는 컨트롤의 위치값을 구한다
//
int CSCSliderCtrl::Pixel2Pos(int nPixel)
{
	int lower = GetRangeMin();
	int upper = GetRangeMax();
	int pos;

	const bool use_thumb_offset = (m_style <= style_value || is_round_pill_style());

	if (m_is_vertical)
	{
		pos = int(
			lower +
			(double)(nPixel - m_margin.top - (double)m_thumb.cy / 2.0) /
			(double)(m_rc.Height() - m_margin.bottom - m_margin.top - (use_thumb_offset ? m_thumb.cy : 0)) *
			(double)(upper - lower)
			);
		Clamp(pos, lower, upper);
	}
	else
	{
		pos = int(
			lower +
			(double)(nPixel - m_margin.left - (use_thumb_offset ? m_thumb.cx / 2 : 0)) /
			(double)(m_rc.Width() - m_margin.left - m_margin.right - (use_thumb_offset ? m_thumb.cx : 0)) *
			(double)(upper - lower)
			);
		Clamp(pos, lower, upper);
	}

	return pos;
}

int CSCSliderCtrl::snap_to_tic(int pos)
{
	if (m_tic_freq <= 0 || (!m_tic_only && !m_auto_snap))
		return pos;

	int lower = GetRangeMin();
	int upper = GetRangeMax();

	int rel = pos - lower;
	int nearest = lower + ((rel + m_tic_freq / 2) / m_tic_freq) * m_tic_freq;

	//마지막 틱은 freq 배수가 아닐 수 있어 upper 도 스냅 후보 — upper 가 더 가까우면 upper 로.
	if (abs(upper - pos) < abs(nearest - pos))
		nearest = upper;

	Clamp(nearest, lower, upper);

	if (m_tic_only)				//모드3 — 항상 가장 가까운 틱.
		return nearest;

	//모드2 — 틱 tolerance 안일 때만 끌어당기고, 그 밖은 자유 이동.
	if (abs(nearest - pos) <= m_snap_tolerance)
		return nearest;

	return pos;
}

void CSCSliderCtrl::OnLButtonDown(UINT nFlags, CPoint point)
{
	SetFocus();

	if (m_style == style_normal)
	{
		CSliderCtrl::OnLButtonDown(nFlags, point);
		return;
	}

	if (m_use_slide == false)
		return;

	//북마크 근처 click 시 정확한 bookmark 위치로 점프.
	//OnMouseMove 가 마우스 위치 ±m_bookmark_near_tolerance 안에 bookmark 있으면 m_cur_bookmark 를 미리 set.
	//그 상태에서 click 하면 바로 그 bookmark 의 pos 로 SetPos + thumb_release 통지 (트랙 점프 의미).
	if (m_use_bookmark && m_cur_bookmark >= 0)
	{
		int bm_pos = m_bookmark[m_cur_bookmark].pos;
		SetPos(bm_pos);
		CSCSliderCtrlMsg msg(CSCSliderCtrlMsg::msg_thumb_release, this, bm_pos);
		::SendMessage(GetParent()->GetSafeHwnd(), Message_CSCSliderCtrl, (WPARAM)&msg, 0);
		return;
	}
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
	m_lbutton_down_point = point;
	m_drag_started = false;
	SetCapture();

	//hover 프리뷰 모드면 grab(드래그) 시작 시 프리뷰 숨김 — 드래그 중엔 영상이 실시간 변하므로 프리뷰 무의미.
	//기존 leave 경로 재사용(프리뷰 lifecycle 을 hover/leave 이벤트로 일원화). m_last_hover_pos 리셋 → release 후 같은 위치 hover 도 재발송.
	if (m_use_hover_preview)
	{
		m_last_hover_pos = -1;
		::SendMessage(GetParent()->GetSafeHwnd(), Message_CSCSliderCtrl,
			(WPARAM)&CSCSliderCtrlMsg(CSCSliderCtrlMsg::msg_hover_leave, this, 0), 0);
	}

	//PotPlayer 식 grab — click 시 즉시 그 위치로 seek (msg_thumb_grab) + thumb 도 시각적으로 그 위치에 jump.
	//drag 중에는 OnMouseMove 가 thumb 를 다시 움직이지 않으므로 thumb 는 grab 위치에 고정 유지된다.
	//(이전: OnMouseMove(nFlags, point) 호출 → drag 첫 프레임에 msg_thumb_move 가 추가 발송되어
	// 같은 위치를 두 번 처리하는 문제가 있었음. grab 한 번이면 충분.)
	{
		int pos = snap_to_tic(Pixel2Pos(point.x));
		SetPos(pos);
		::SendMessage(GetParent()->GetSafeHwnd(), Message_CSCSliderCtrl,
			(WPARAM)&CSCSliderCtrlMsg(CSCSliderCtrlMsg::msg_thumb_grab, this, pos), 0);
	}

#if 0
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

	//이 코드를 살려두면 뭔가 WM_PAINT가 발생하여 현재의 pos가 실제 pos와 다르게 설정되는 문제가 발생한다.
	//막아둬야만 정상 동작한다.
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
	m_drag_started = false;
	ReleaseCapture();
	Invalidate();

	//drag 종료 알림 — 호출자가 timer 재시작하고 graph 를 다시 Running 으로 복귀시킬 수 있도록.
	{
		int pos = snap_to_tic(Pixel2Pos(point.x));
		::SendMessage(GetParent()->GetSafeHwnd(), Message_CSCSliderCtrl,
			(WPARAM)&CSCSliderCtrlMsg(CSCSliderCtrlMsg::msg_thumb_release, this, pos), 0);
	}

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

	//int pos = (int)((double)point.x * (double)(upper - lower) / (double)rc.right) + lower;
	//Clamp(pos, lower, upper);
	int pos = Pixel2Pos(point.x);

	CString str;

	if (m_use_bookmark)
	{
		int i;
		int pos_range[2];
		int found = false;

		if (point.y < m_rc.CenterPoint().y + 4)
		{
			pos_range[0] = Pixel2Pos(point.x - m_bookmark_near_tolerance);
			pos_range[1] = Pixel2Pos(point.x + m_bookmark_near_tolerance);
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
			//손 떨림으로 인한 micro WM_MOUSEMOVE 마다 UpdateTipText 가 호출되면 같은 텍스트로도 popup redraw 발생 → 깜빡임.
			//m_use_tooltip — 호출자가 use_tooltip(false) 로 suppress 한 경우 (미디어 미오픈 등) 북마크 위 hover 도 표시 안 함.
			if (m_use_tooltip && !m_use_hover_preview && m_tooltip.m_hWnd && str != m_tooltip_last_text)
			{
				m_tooltip.UpdateTipText(str, this);
				m_tooltip_last_text = str;
			}
		}
		redraw_window();
	}

	if (m_use_tooltip && !m_use_hover_preview && m_cur_bookmark == -1)
	{
		int pos = Pixel2Pos(point.x);

		if (m_tooltip_format == tooltip_time)
			str.Format(_T("%s"), GetTimeStringFromMilliSeconds(pos, true, false));
		else if (m_tooltip_format == tooltip_time_ms)
			str.Format(_T("%s"), GetTimeStringFromMilliSeconds(pos));
		else if (m_tooltip_format == tooltip_time_percent)
		{
			int range = upper - lower;
			int percent = (range > 0) ? (int)(((double)(pos - lower) / range) * 100.0 + 0.5) : 0;
			str.Format(_T("%s(%d%%)"), GetTimeStringFromMilliSeconds(pos, true, false), percent);
		}
		else//if (m_tooltip_format == tooltip_value)
			str.Format(_T("%d / %d"), pos, upper);

		if (m_tooltip.m_hWnd && str != m_tooltip_last_text)
		{
			m_tooltip.UpdateTipText(str, this);
			m_tooltip_last_text = str;
		}
	}


	//[hover 프리뷰] 드래그가 아닌 단순 hover 시 parent 로 hover 위치 통지 (opt-in). 호출자가 그 위치의 썸네일을 프리뷰.
	//pos 변동 시에만 발송(jitter spam 억제). TrackMouseEvent 로 leave 를 받아 프리뷰를 숨기게 함.
	if (m_use_hover_preview && !m_lbuttondown && IsWindowEnabled())
	{
		if (!m_hover_tracking)
		{
			TRACKMOUSEEVENT tme = { sizeof(tme), TME_LEAVE, m_hWnd, 0 };
			TrackMouseEvent(&tme);
			m_hover_tracking = true;
		}
		if (pos != m_last_hover_pos)
		{
			m_last_hover_pos = pos;
			::SendMessage(GetParent()->GetSafeHwnd(), Message_CSCSliderCtrl,
				(WPARAM)&CSCSliderCtrlMsg(CSCSliderCtrlMsg::msg_hover, this, pos), 0);
		}
	}

	//drag 중에는 thumb 가 마우스를 따라가고 msg_thumb_move 가 발송되어 호출자가 set_track_pos 로 seek.
	//(click+hold = mouse 가 안 움직이는 상태에서는 이 분기가 발화하지 않으므로 thumb 는 grab 위치에 고정.)
	//drag threshold 가드 — reference point 로부터 m_drag_threshold_px 이상 이동해야 emission.
	//- drag 시작 전: reference = lbutton_down_point. 손 jitter 무시.
	//- drag 중: reference = last_emit_point. drag-and-hold 시 마우스가 그 자리에 멈춰 있으면 jitter 가
	//  threshold 안이라 emission 안 됨 → 호출자의 throttle seek 도 안 일어나서 그 위치부터 정상 재생.
	if (m_lbuttondown && IsWindowEnabled())
	{
		const CPoint& ref = m_drag_started ? m_last_emit_point : m_lbutton_down_point;
		int dx = point.x - ref.x;
		int dy = point.y - ref.y;
		if (dx * dx + dy * dy < m_drag_threshold_px * m_drag_threshold_px)
			return;

		m_drag_started = true;
		m_last_emit_point = point;

		int snapped = snap_to_tic(pos);
		SetPos(snapped);
		::SendMessage(GetParent()->GetSafeHwnd(), Message_CSCSliderCtrl,
			(WPARAM)&CSCSliderCtrlMsg(CSCSliderCtrlMsg::msg_thumb_move, this, snapped), 0);
	}

	//이 코드를 살려놓으면 thumb위에서 마우스가 클릭되지 않고 움직여도 WM_PAINT가 호출되는 현상이 발생한다.
	//우선 주석처리한다.
	//CSliderCtrl::OnMouseMove(nFlags, point);
}

void CSCSliderCtrl::OnMouseLeave()
{
	//hover 프리뷰 종료 — TrackMouseEvent 재무장 필요(다음 진입 시 OnMouseMove 가 다시 arm), 프리뷰 숨김 통지.
	m_hover_tracking = false;
	m_last_hover_pos = -1;
	if (m_use_hover_preview)
		::SendMessage(GetParent()->GetSafeHwnd(), Message_CSCSliderCtrl,
			(WPARAM)&CSCSliderCtrlMsg(CSCSliderCtrlMsg::msg_hover_leave, this, 0), 0);

	CSliderCtrl::OnMouseLeave();
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

void CSCSliderCtrl::set_back_color(Gdiplus::Color crBack)
{
	m_theme.cr_back = crBack;
	Invalidate();
}

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

void CSCSliderCtrl::compute_thumb_shades(Gdiplus::Color cr_thumb)
{
	int nMultiple = 32;

	m_cr_thumb			= cr_thumb;
	m_cr_thumb_light	= get_color(m_cr_thumb, nMultiple);
	m_cr_thumb_lighter	= get_color(m_cr_thumb, nMultiple * 2);
	m_cr_thumb_dark		= get_color(m_cr_thumb, -nMultiple);
	m_cr_thumb_darker	= get_color(m_cr_thumb, -nMultiple * 2);
}

void CSCSliderCtrl::set_thumb_color(Gdiplus::Color cr_thumb)
{
	compute_thumb_shades(cr_thumb);
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
	if (pMsg->message == WM_KEYDOWN && IsWindowVisible())
	{
		return FALSE;
	}

	return CSliderCtrl::PreTranslateMessage(pMsg);
}

void CSCSliderCtrl::OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags)
{
	int lower = get_lower();
	int upper = get_upper();
	int pos = GetPos();

	//tic_only 모드면 키보드 이동도 틱 단위로만 — 화살표는 인접 틱(±freq), 그 외 키는 결과를 틱에 스냅.
	const bool tic_mode = (m_tic_only && m_tic_freq > 0);
	const int line_step = tic_mode ? m_tic_freq : 1;
	const int page_step = tic_mode ? max(m_tic_freq, (upper - lower) / 10) : max(1, (upper - lower) / 10);

	//네이티브 트랙바가 처리하는 네비게이션 키는 모두 여기서 가로채 set_pos 로 직접 갱신한다.
	//base CSliderCtrl::OnKeyDown 으로 넘기면 네이티브가 커스텀 paint 위에 thumb 를 또 그려 잔상이 남고,
	//parent 로 Message_CSCSliderCtrl 도 안 가 값이 갱신되지 않는다.
	switch (nChar)
	{
		case VK_LEFT:
		case VK_DOWN:	pos -= line_step;		break;
		case VK_RIGHT:
		case VK_UP:		pos += line_step;		break;
		case VK_PRIOR:	pos += page_step;		break;
		case VK_NEXT:	pos -= page_step;		break;
		case VK_HOME:	pos = lower;			break;
		case VK_END:	pos = upper;			break;
		default:
			CSliderCtrl::OnKeyDown(nChar, nRepCnt, nFlags);
			return;
	}

	pos = max(lower, min(upper, pos));
	if (tic_mode)
		pos = snap_to_tic(pos);
	set_pos(pos);
	::SendMessage(GetParent()->GetSafeHwnd(), Message_CSCSliderCtrl, (WPARAM)&CSCSliderCtrlMsg(CSCSliderCtrlMsg::msg_thumb_move, this, pos), 0);
}

void CSCSliderCtrl::set_style(int nStyle)
{
	m_style = nStyle;

	//m_track_height = 5;

	if (m_style == style_thumb)
	{
		m_track_height = 8.0f;
		m_thumb = CSize(28, (int)m_track_height + 6);
	}
	else if (m_style == style_thumb_round)
	{
		m_thumb = CSize(16, 16);
		m_track_height = 1.6f;	//소수 두께 — g.FillRectangle(RectF) 로 cy 중심 AA 정합 (1px 은 너무 얇음).
	}
	else if (is_round_pill_style())
	{
		m_thumb = CSize(24, 24);
		m_track_height = 12.0f;
	}
	else if (m_style == style_value)
	{
		m_thumb = CSize(56, 8);
	}
	else if (m_style == style_progress)
	{
		m_use_slide = false;

		CRect rc;
		GetClientRect(rc);

		//기본 m_track_height = 14지만 rc의 height가 그보다 작을 경우는 rc.Height()로 줄여준다.
		if (rc.Height() < m_track_height)
			m_track_height = rc.Height();
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

//pos 에 북마크 추가 — 같은 pos 가 이미 있으면 그 항목 제거 (토글). 없으면 push.
void CSCSliderCtrl::add_bookmark(int pos, CString name)
{
	if (!m_use_bookmark)
		return;

	int index = find_index_bookmark(pos);
	if (index >= 0)
	{
		m_bookmark.erase(m_bookmark.begin() + index);
	}
	else
	{
		if (name.IsEmpty())
			name.Format(_T("bookmark%02d"), m_bookmark.size());
		m_bookmark.push_back(CSCSliderCtrlBookmark(pos, name));
		std::sort(m_bookmark.begin(), m_bookmark.end(),
			[](const CSCSliderCtrlBookmark& a, const CSCSliderCtrlBookmark& b) { return a.pos < b.pos; });
	}
	Invalidate();
}

//북마크 list 전체 교체 — 외부에서 데이터 own 하는 경우 view 갱신.
//입력 list 에 같은 pos 중복이 있으면 마지막 항목만 남김 (방어).
void CSCSliderCtrl::set_bookmarks(const std::deque<CSCSliderCtrlBookmark>& items)
{
	if (!m_use_bookmark)
		return;

	m_bookmark.clear();
	for (const auto& bm : items)
	{
		int index = find_index_bookmark(bm.pos);
		if (index >= 0)
			m_bookmark[index] = bm;
		else
			m_bookmark.push_back(bm);
	}
	std::sort(m_bookmark.begin(), m_bookmark.end(),
		[](const CSCSliderCtrlBookmark& a, const CSCSliderCtrlBookmark& b) { return a.pos < b.pos; });
	Invalidate();
}

//모두 비움.
void CSCSliderCtrl::clear_bookmarks()
{
	if (m_bookmark.empty())
		return;
	m_bookmark.clear();
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

	//CSCSliderCtrl을 동적으로 생성하면 font = NULL이고 m_lf는 기본값으로 채워지는데 이 때 lfWidth가 8로 정해진다.
	//이럴 경우 lfHeight를 바꿔도 lfWidth가 변경되지 않아 8이하의 작은 글꼴은 넓은 글자로 표시된다.
	//이 값을 0으로 강제 고정해야 lfHeight에 따라 글자의 width가 자동 계산되어 그려진다.
	m_lf.lfWidth = 0;

	reconstruct_font();

	use_tooltip(m_use_tooltip);
}

void CSCSliderCtrl::use_tooltip(bool use)
{
	m_use_tooltip = use;
	if (use && GetSafeHwnd() && !m_tooltip.GetSafeHwnd())
	{
		m_tooltip.Create(this, TTS_ALWAYSTIP | TTS_NOPREFIX | TTS_NOANIMATE);
		m_tooltip.SetDelayTime(TTDT_AUTOPOP, -1);
		m_tooltip.SetDelayTime(TTDT_INITIAL, 0);
		m_tooltip.SetDelayTime(TTDT_RESHOW, 0);
		m_tooltip.SetMaxTipWidth(400);
		m_tooltip.AddTool(this, _T(""));
		//hover_preview 모드면 tooltip 과 배타 — 켜지 않음.
		m_tooltip.Activate(m_use_hover_preview ? FALSE : TRUE);
		return;
	}

	//기존 tooltip 이 이미 만들어져 있으면 Activate 만 토글 — 호출자가 미디어 close 등으로 일시 suppress 가능.
	//단 hover_preview(프리뷰) 모드면 use 와 무관하게 tooltip 비활성 — 둘은 상호배타(set_track_total 이 use_tooltip(true) 재호출해도 안 뜸).
	if (m_tooltip.GetSafeHwnd())
	{
		BOOL activate = (use && !m_use_hover_preview) ? TRUE : FALSE;
		m_tooltip.Activate(activate);
		if (!activate)
			m_tooltip.Pop();
	}
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

	return get_color(get_gray_color(cr), offset);
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
		m_repeat_start = GetPos();
	else if (pos == -1)
		m_repeat_start = -1;
	else
		m_repeat_start = pos;

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
		m_repeat_end = GetPos();
	else if (pos == -1)
		m_repeat_end = -1;
	else
		m_repeat_end = pos;

	//시작과 끝이 역순이 되면 start를 시작으로 변경
	if ((m_repeat_start >= 0) &&
		(m_repeat_end >= 0) &&
		(m_repeat_end <= m_repeat_start))
		m_repeat_start = GetRangeMin();

	Invalidate();
}

//[sub track] seek 가능 frontier 위치(range 단위). -1 이면 숨김. 값이 바뀔 때만 redraw.
void CSCSliderCtrl::set_sub_track_pos(int pos)
{
	if (pos == m_sub_track_pos)
		return;
	m_sub_track_pos = pos;
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

	if (pos >= (int)m_steps.size())
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
	//CSCImageDlg에서 animated gif를 재생하다가 종료시키면 간혹 이 set_pos()를 호출하여 에러가 발생한다.
	//이미 이 슬라이더의 m_hWnd가 NULL인 상태에서는 아래 코드를 처리하지 않아야 한다.
	if (!m_hWnd)
		return;

	//style_step일 때 SetPos()를 호출한다는 얘기는 아직 처리중이고 pos == upper인 경우라도 맨 마지막 스텝을 처리중이라는 뜻이므로
	//SetPos()에서는 m_step_completed는 항상 false이며 parent에서 모든 처리가 완료된 시점에 이 값을 true로 세팅해줘야 한다.
	m_step_completed = false;
	m_thumb_hide = false;
	CSliderCtrl::SetPos(pos);

	if (!m_hWnd)
		return;

	//20260331 scpark
	//CTest_SliderCtrlExDlg 프로젝트에서 휠을 움직였을 때 CTest_SliderCtrlExDlg에도 이 이벤트 메시지를 보내고
	//CTest_SliderCtrlExDlg에서는 현재 변경된 slider의 pos로 모두 set_pos()하는 코드가 있다.
	//그런데 여기서 Invalidate()만 호출하면 다른 slider의 UI가 바로 갱신되지 않고 깨지는 현상이 발생한다.
	//UpdateWindow()까지 호출해줘야 즉시 갱신된다.
	Invalidate();
	UpdateWindow();
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

//freq는 CSliderCtrl::SetTicFreq()와 동일하게 lower ~ upper 사이의 구간을 몇 등분할 것인지가 아닌 간격을 의미한다.
//즉, freq = 23이면 23 등분이 아니라 0 ~ 23 ~ 46 ~ 69 ~ 92와 같이 틱이 표시된다.
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

//예를 들어 폰트 크기를 10으로 설정하면
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

//step 단위 증감
int32_t CSCSliderCtrl::step(int step)
{
	int pos = GetPos() + step;
	Clamp(pos, get_lower(), get_upper());

	SetPos(pos);
	Invalidate();

	::SendMessage(GetParent()->GetSafeHwnd(), Message_CSCSliderCtrl, (WPARAM)&CSCSliderCtrlMsg(CSCSliderCtrlMsg::msg_thumb_move, this, pos), 0);

	return GetPos();
}

//theme 의 accent(cr_progress)·배경에서 트랙/썸/틱 색을 파생. cr_progress 는 모든 테마에서 progress/선택 강조색
//(default=RoyalBlue, windows=COLOR_HIGHLIGHT, 커스텀 테마=cr_back_selected)으로 설정돼 슬라이더 active 트랙·썸에
//의미상 그대로 맞는다. 개별 setter(set_thumb_color 등) 로 일일이 지정하지 않아도 테마 교체만으로 채색되도록.
void CSCSliderCtrl::apply_theme_colors()
{
	Gdiplus::Color bg = m_theme.cr_back;
	int lum_bg = (int)get_luminance(bg);

	//active/thumb = 테마 accent(cr_progress). 단 에디터 import 테마는 cr_progress = 에디터 selection 색이라
	//배경과 명도 대비가 약한 경우가 많아(choco/navajo/ruby_blue 등) 트랙이 거의 안 보인다. 배경과 명도차가
	//70 미만이면 배경 반대쪽(어두운 배경=흰, 밝은 배경=검)으로 부족분에 비례(0.25~0.6)해 섞어 가시성을 확보.
	//이미 충분히 대비되는 curated 테마(windows~claude09 등)는 이 분기를 안 타므로 그대로 유지된다.
	Gdiplus::Color accent = m_theme.cr_progress;
	int diff = abs((int)get_luminance(accent) - lum_bg);
	if (diff < 70)
	{
		float t = (float)(70 - diff) / 70.0f;
		t = max(0.25f, min(0.6f, t));
		Gdiplus::Color target = (lum_bg < 128) ? Gdiplus::Color(Gdiplus::Color::White) : Gdiplus::Color(Gdiplus::Color::Black);
		accent = lerp_color(accent, target, t);
	}

	m_cr_active = accent;
	compute_thumb_shades(accent);

	//inactive 트랙·틱 = accent 의 회색 톤. 유채색 accent 는 'colored active vs gray inactive' 로 자연히 구분돼
	//양호하다(기존 동작 유지). 그러나 accent 가 무채색이면(mono_industrial/navajo) gray inactive 가 active 와
	//같은 톤이 되어 트랙이 썸 위치에서 끊긴 것처럼 보이므로, 배경~active 중간 명도의 회색으로 분리해 active 보다
	//옅게 보이게 한다.
	int chroma = (int)max(accent.GetR(), max(accent.GetG(), accent.GetB()))
			   - (int)min(accent.GetR(), min(accent.GetG(), accent.GetB()));
	if (chroma < 24)
		m_cr_inactive = get_gray_color(lerp_color(bg, accent, 0.5f));
	else
		m_cr_inactive = get_gray_color(accent);
	m_cr_tic = m_cr_inactive;
}

void CSCSliderCtrl::set_color_theme(int theme)
{
	m_theme.set_color_theme(theme);
	apply_theme_colors();
}

//다이얼로그에서 m_theme 객체를 그대로 전파받는 경로 (다른 SC* 컨트롤과 동일 시그니처).
void CSCSliderCtrl::set_color_theme(const CSCColorTheme& theme, bool invalidate)
{
	m_theme.copy_colors_from(theme);
	apply_theme_colors();
	if (invalidate && m_hWnd)
		Invalidate();
}

void CSCSliderCtrl::set_gradient_colors(const std::vector<Gdiplus::Color>& colors)
{
	m_gradient_colors = colors;
	Invalidate(FALSE);
}

void CSCSliderCtrl::set_gradient_colors(std::initializer_list<Gdiplus::Color> colors)
{
	m_gradient_colors.assign(colors);
	Invalidate(FALSE);
}

// ── N-stop 그라디언트에서 비율 t(0~1) 위치의 색상을 보간 ───────
Gdiplus::Color CSCSliderCtrl::sample_gradient(float t) const
{
	const int n = static_cast<int>(m_gradient_colors.size());
	if (n == 0)
		return lerp_color(m_cr_inactive, m_cr_active, t);
	if (n == 1)
		return m_gradient_colors[0];

	t = max(0.f, min(1.f, t));

	// t가 어느 두 색 사이에 있는지 찾기
	const float segment = 1.f / static_cast<float>(n - 1);
	const int   idx = min(static_cast<int>(t / segment), n - 2);
	const float local_t = (t - idx * segment) / segment;

	return lerp_color(m_gradient_colors[idx], m_gradient_colors[idx + 1], local_t);
}

//휠을 올리면 감소하고 내리면 증가하는 기본 동작을 반대로 동작하도록 수정.
BOOL CSCSliderCtrl::OnMouseWheel(UINT nFlags, short zDelta, CPoint pt)
{
	// TODO: 여기에 메시지 처리기 코드를 추가 및/또는 기본값을 호출합니다.
	int pos = GetPos();
	TRACE(_T("delta = %d, pos = %d\n"), zDelta, pos);

	if (zDelta > 0)
		pos += 1;
	else
		pos -= 1;

	int lower = get_lower();
	int upper = get_upper();

	Clamp(pos, lower, upper);

	set_pos(pos);

	CSCSliderCtrlMsg msg(CSCSliderCtrlMsg::msg_thumb_move, this, pos);
	::SendMessage(GetParent()->GetSafeHwnd(), Message_CSCSliderCtrl, (WPARAM)&msg, 0);

	return TRUE;
	//return CSliderCtrl::OnMouseWheel(nFlags, -zDelta, pt);
}
