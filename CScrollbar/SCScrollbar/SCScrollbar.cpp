#include "SCScrollbar.h"

#include "../../Functions.h"
#include "../../MemoryDC.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


IMPLEMENT_DYNAMIC(CSCScrollbar, CWnd)

CSCScrollbar::CSCScrollbar()
{
	m_theme.set_color_theme(CSCColorTheme::color_theme_dark_gray);
}

CSCScrollbar::~CSCScrollbar()
{
}

BEGIN_MESSAGE_MAP(CSCScrollbar, CWnd)
	ON_WM_PAINT()
	ON_WM_ERASEBKGND()
	ON_WM_SIZE()
	ON_WM_LBUTTONDOWN()
	ON_WM_LBUTTONUP()
	ON_WM_MOUSEMOVE()
	ON_MESSAGE(WM_MOUSELEAVE, &CSCScrollbar::OnMouseLeave)
	ON_WM_TIMER()
END_MESSAGE_MAP()


bool CSCScrollbar::create(CWnd* parent, ORIENTATION orient, int x, int y, int cx, int cy, UINT id /*=0*/)
{
	if (!parent || !::IsWindow(parent->GetSafeHwnd()))
		return false;

	m_orient = orient;

	//WS_CHILD + WS_VISIBLE — host 의 client 영역에 배치. 자체 paint 라 WS_CLIPCHILDREN 이슈 없음.
	const DWORD style = WS_CHILD | WS_VISIBLE;
	if (!CWnd::CreateEx(0, AfxRegisterWndClass(CS_DBLCLKS | CS_HREDRAW | CS_VREDRAW),
		_T(""), style, CRect(x, y, x + cx, y + cy), parent, id))
	{
		return false;
	}

	return true;
}

//---- 모델 ------------------------------------------------------------------

void CSCScrollbar::set_range(int min_pos, int max_pos)
{
	int new_max = max(min_pos, max_pos);
	if (m_range_min == min_pos && m_range_max == new_max)
		return;
	m_range_min = min_pos;
	m_range_max = new_max;
	clamp_pos();
	if (::IsWindow(m_hWnd))
		Invalidate();
}

void CSCScrollbar::set_page(int page_size)
{
	int new_page = max(1, page_size);
	if (m_page == new_page)
		return;
	m_page = new_page;
	clamp_pos();
	if (::IsWindow(m_hWnd))
		Invalidate();
}

void CSCScrollbar::set_pos(int pos, bool emit_message /*=false*/)
{
	int old = m_pos;
	m_pos = pos;
	clamp_pos();
	if (m_pos == old)
		return;

	if (::IsWindow(m_hWnd))
	{
		//RDW_UPDATENOW — message queue 통한 비동기 WM_PAINT 가 parent paint 보다 1 frame 지연돼 visual lag 가 나는 현상 회피.
		//RDW_NOERASE — OnEraseBkgnd 가 어차피 TRUE 반환 (no-op) 이라 erase 단계 skip.
		::RedrawWindow(m_hWnd, NULL, NULL, RDW_INVALIDATE | RDW_NOERASE | RDW_UPDATENOW);
	}
	if (emit_message)
		emit(CSCScrollbarMsg::msg_scrollbar_pos_changed);
}

void CSCScrollbar::scroll_by_lines(int lines, bool emit_message /*=true*/)
{
	set_pos(m_pos + lines * m_line, emit_message);
}

void CSCScrollbar::scroll_by_pages(int pages, bool emit_message /*=true*/)
{
	set_pos(m_pos + pages * m_page, emit_message);
}

void CSCScrollbar::clamp_pos()
{
	int max_pos = m_range_max - m_page + 1;
	if (max_pos < m_range_min)
		max_pos = m_range_min;
	Clamp(m_pos, m_range_min, max_pos);
}

//---- 시각 ------------------------------------------------------------------

void CSCScrollbar::set_thickness(int thickness)
{
	int t = max(1, thickness);
	m_thickness_normal = t;
	if (m_thickness_hover < t + 2)
		m_thickness_hover = t + 3;
	if (!m_window_hovering)
		apply_visible_thickness(t);
	else if (::IsWindow(m_hWnd))
		Invalidate();
}

void CSCScrollbar::set_hover_thickness(int thickness)
{
	m_thickness_hover = max(m_thickness_normal, thickness);
	if (m_window_hovering)
		apply_visible_thickness(m_thickness_hover);
}

void CSCScrollbar::apply_visible_thickness(int new_thickness)
{
	if (m_thickness == new_thickness)
		return;
	m_thickness = new_thickness;

	//윈도우 크기는 변경하지 않음 — 호스트가 reserve 한 영역 (hover thickness) 그대로.
	//track/thumb 의 가운데 정렬 폭만 m_thickness 로 갱신해 그림.
	if (::IsWindow(m_hWnd))
		Invalidate();
}

CRect CSCScrollbar::get_visible_rect() const
{
	//track 영역 = window 전체. thumb 의 cross 정렬은 calc_thumb_rect 에서 따로 처리.
	CRect rc;
	GetClientRect(&rc);
	return rc;
}

void CSCScrollbar::set_show_arrows(bool show)
{
	m_show_arrows = show;
	if (::IsWindow(m_hWnd))
		Invalidate();
}

void CSCScrollbar::set_color_theme(int theme, bool invalidate /*=true*/)
{
	m_theme.set_color_theme(theme);
	if (invalidate && ::IsWindow(m_hWnd))
		Invalidate();
}

void CSCScrollbar::set_color_theme(const CSCColorTheme& theme, bool invalidate /*=false*/)
{
	m_theme = theme;
	if (invalidate && ::IsWindow(m_hWnd))
		Invalidate();
}

//---- 기하 ------------------------------------------------------------------

CRect CSCScrollbar::calc_arrow_lt_rect() const
{
	if (!m_show_arrows)
		return CRect(0, 0, 0, 0);

	CRect rc = get_visible_rect();
	if (m_orient == vertical)
		return CRect(rc.left, rc.top, rc.right, rc.top + m_arrow_size);
	return CRect(rc.left, rc.top, rc.left + m_arrow_size, rc.bottom);
}

CRect CSCScrollbar::calc_arrow_rb_rect() const
{
	if (!m_show_arrows)
		return CRect(0, 0, 0, 0);

	CRect rc = get_visible_rect();
	if (m_orient == vertical)
		return CRect(rc.left, rc.bottom - m_arrow_size, rc.right, rc.bottom);
	return CRect(rc.right - m_arrow_size, rc.top, rc.right, rc.bottom);
}

CRect CSCScrollbar::calc_track_rect() const
{
	CRect rc = get_visible_rect();
	if (!m_show_arrows)
		return rc;

	if (m_orient == vertical)
	{
		rc.top += m_arrow_size;
		rc.bottom -= m_arrow_size;
	}
	else
	{
		rc.left += m_arrow_size;
		rc.right -= m_arrow_size;
	}
	return rc;
}

CRect CSCScrollbar::calc_thumb_rect() const
{
	CRect rTrack = calc_track_rect();
	if (rTrack.IsRectEmpty())
		return CRect(0, 0, 0, 0);

	int range = m_range_max - m_range_min + 1;
	if (range <= m_page)
		return CRect(0, 0, 0, 0);	//전체 보임 → thumb 표시 안 함.

	int track_extent = (m_orient == vertical) ? rTrack.Height() : rTrack.Width();
	int thumb_extent = (int)((double)track_extent * m_page / range + 0.5);
	if (thumb_extent < thumb_min_extent())
		thumb_extent = thumb_min_extent();
	if (thumb_extent > track_extent)
		thumb_extent = track_extent;

	int max_pos = m_range_max - m_page + 1;
	int span = max_pos - m_range_min;	//스크롤 가능 범위.
	if (span <= 0)
		span = 1;

	int max_thumb_offset = track_extent - thumb_extent;
	int thumb_offset = (int)((double)(m_pos - m_range_min) * max_thumb_offset / span + 0.5);
	Clamp(thumb_offset, 0, max_thumb_offset);

	//even thickness + floor cx → FillRectangle 의 visual center sub-pixel = rTrack.left + Width()/2 = client 정중앙 9.0 (width 18 기준). arrow chevron sub-pixel center 와 일치.
	if (m_orient == vertical)
	{
		int cx = rTrack.left + (rTrack.Width() - m_thickness) / 2;
		return CRect(cx, rTrack.top + thumb_offset, cx + m_thickness, rTrack.top + thumb_offset + thumb_extent);
	}
	int cy = rTrack.top + (rTrack.Height() - m_thickness) / 2;
	return CRect(rTrack.left + thumb_offset, cy, rTrack.left + thumb_offset + thumb_extent, cy + m_thickness);
}

CSCScrollbar::SCROLL_PART CSCScrollbar::hit_test(CPoint pt) const
{
	if (m_show_arrows)
	{
		if (calc_arrow_lt_rect().PtInRect(pt)) return part_arrow_lt;
		if (calc_arrow_rb_rect().PtInRect(pt)) return part_arrow_rb;
	}

	CRect rThumb = calc_thumb_rect();
	CRect rTrack = calc_track_rect();

	//thumb hit 영역 — cross 폭은 track 전체로 확장. 얇은 thumb 라도 scrollbar 전체 폭에서 hover/클릭 인식.
	if (!rThumb.IsRectEmpty())
	{
		CRect rHit = rThumb;
		if (m_orient == vertical)
		{
			rHit.left = rTrack.left;
			rHit.right = rTrack.right;
		}
		else
		{
			rHit.top = rTrack.top;
			rHit.bottom = rTrack.bottom;
		}
		if (rHit.PtInRect(pt))
			return part_thumb;
	}

	if (rTrack.PtInRect(pt))
	{
		if (rThumb.IsRectEmpty())
			return part_none;
		if (m_orient == vertical)
			return (pt.y < rThumb.top) ? part_track_lt : part_track_rb;
		return (pt.x < rThumb.left) ? part_track_lt : part_track_rb;
	}
	return part_none;
}

int CSCScrollbar::pos_from_thumb_offset(int track_origin, int track_extent, int thumb_extent, int mouse_along) const
{
	int max_thumb_offset = track_extent - thumb_extent;
	if (max_thumb_offset <= 0)
		return m_range_min;

	int thumb_offset = mouse_along - track_origin - m_drag_offset;
	Clamp(thumb_offset, 0, max_thumb_offset);

	int max_pos = m_range_max - m_page + 1;
	int span = max_pos - m_range_min;
	if (span <= 0)
		return m_range_min;

	return m_range_min + (int)((double)thumb_offset * span / max_thumb_offset + 0.5);
}

//---- 이벤트 ----------------------------------------------------------------

void CSCScrollbar::update_hover(CPoint pt)
{
	SCROLL_PART new_hover = hit_test(pt);
	if (new_hover == m_hover)
		return;
	m_hover = new_hover;
	Invalidate();
}

void CSCScrollbar::emit(int msg)
{
	CWnd* dst = m_message_target ? m_message_target : GetParent();
	if (dst)
	{
		CSCScrollbarMsg payload(msg, this, m_pos);
		::SendMessage(dst->GetSafeHwnd(), Message_CSCScrollbar, (WPARAM)&payload, 0);
	}
}

void CSCScrollbar::OnLButtonDown(UINT nFlags, CPoint point)
{
	SCROLL_PART part = hit_test(point);
	if (part == part_none)
		return;

	SetCapture();
	m_pressed = part;

	switch (part)
	{
	case part_thumb:
	{
		CRect rThumb = calc_thumb_rect();
		m_drag_offset = (m_orient == vertical) ? (point.y - rThumb.top) : (point.x - rThumb.left);
		m_dragging = true;
		emit(CSCScrollbarMsg::msg_scrollbar_drag_start);
		break;
	}
	case part_arrow_lt:
		scroll_by_lines(-1);
		SetTimer(timer_repeat_initial, m_repeat_initial_ms, NULL);
		break;
	case part_arrow_rb:
		scroll_by_lines(+1);
		SetTimer(timer_repeat_initial, m_repeat_initial_ms, NULL);
		break;
	case part_track_lt:
		scroll_by_pages(-1);
		SetTimer(timer_repeat_initial, m_repeat_initial_ms, NULL);
		break;
	case part_track_rb:
		scroll_by_pages(+1);
		SetTimer(timer_repeat_initial, m_repeat_initial_ms, NULL);
		break;
	default:
		break;
	}

	Invalidate();
}

void CSCScrollbar::OnMouseMove(UINT nFlags, CPoint point)
{
	if (!m_tracking_leave)
	{
		TRACKMOUSEEVENT tme = { sizeof(TRACKMOUSEEVENT), TME_LEAVE, m_hWnd, 0 };
		::TrackMouseEvent(&tme);
		m_tracking_leave = true;

		//호버 진입 — visible 두께를 hover 로 확장 (윈도우는 그대로, 안의 track/thumb 만 굵어짐).
		m_window_hovering = true;
		apply_visible_thickness(m_thickness_hover);
	}

	if (m_dragging && m_pressed == part_thumb)
	{
		CRect rTrack = calc_track_rect();
		CRect rThumb = calc_thumb_rect();
		int track_origin = (m_orient == vertical) ? rTrack.top : rTrack.left;
		int track_extent = (m_orient == vertical) ? rTrack.Height() : rTrack.Width();
		int thumb_extent = (m_orient == vertical) ? rThumb.Height() : rThumb.Width();
		int mouse_along = (m_orient == vertical) ? point.y : point.x;

		int new_pos = pos_from_thumb_offset(track_origin, track_extent, thumb_extent, mouse_along);
		set_pos(new_pos, true);
		return;
	}

	update_hover(point);
}

void CSCScrollbar::OnLButtonUp(UINT nFlags, CPoint point)
{
	if (::GetCapture() == m_hWnd)
		ReleaseCapture();

	KillTimer(timer_repeat_initial);
	KillTimer(timer_repeat);

	bool was_dragging = m_dragging;
	m_pressed = part_none;
	m_dragging = false;

	if (was_dragging)
		emit(CSCScrollbarMsg::msg_scrollbar_drag_end);

	update_hover(point);
	Invalidate();
}

LRESULT CSCScrollbar::OnMouseLeave(WPARAM, LPARAM)
{
	m_tracking_leave = false;

	//drag 중이면 leave 취소 — capture 가 풀리지 않은 한 윈도우 밖이라도 hover 유지.
	if (!m_dragging)
	{
		m_window_hovering = false;
		apply_visible_thickness(m_thickness_normal);
	}

	if (m_hover != part_none)
	{
		m_hover = part_none;
		Invalidate();
	}
	return 0;
}

void CSCScrollbar::OnTimer(UINT_PTR nIDEvent)
{
	if (nIDEvent == timer_repeat_initial)
	{
		KillTimer(timer_repeat_initial);
		SetTimer(timer_repeat, m_repeat_ms, NULL);
		nIDEvent = timer_repeat;
	}

	if (nIDEvent == timer_repeat)
	{
		switch (m_pressed)
		{
		case part_arrow_lt:		scroll_by_lines(-1); break;
		case part_arrow_rb:		scroll_by_lines(+1); break;
		case part_track_lt:		scroll_by_pages(-1); break;
		case part_track_rb:		scroll_by_pages(+1); break;
		default:				KillTimer(timer_repeat); break;
		}
		return;
	}

	CWnd::OnTimer(nIDEvent);
}

void CSCScrollbar::OnSize(UINT nType, int cx, int cy)
{
	CWnd::OnSize(nType, cx, cy);
	if (::IsWindow(m_hWnd))
		Invalidate();
}

BOOL CSCScrollbar::OnEraseBkgnd(CDC* pDC)
{
	return TRUE;	//OnPaint 가 전체 채움.
}

//---- 그리기 ----------------------------------------------------------------

void CSCScrollbar::draw_track(Gdiplus::Graphics& g, const CRect& rTrack)
{
	if (rTrack.IsRectEmpty())
		return;

	//직사각형 — cr_back 보다 약간 weak 한 톤. "여기 scrollbar 가 있다" 정도 인지.
	Gdiplus::Color cr_track = get_color(m_theme.cr_back, 24);
	Gdiplus::SolidBrush br(cr_track);
	g.FillRectangle(&br, rTrack.left, rTrack.top, rTrack.Width(), rTrack.Height());
}

void CSCScrollbar::draw_thumb(Gdiplus::Graphics& g, const CRect& rThumb)
{
	if (rThumb.IsRectEmpty())
		return;

	//thumb color = cr_text 와 cr_back 의 blend. hover/pressed 시 cr_text 에 가까워짐. base ratio 0.45 로 track 과 충분한 대비 (track 은 cr_back+24).
	double ratio = 0.45;
	if (m_hover == part_thumb)		ratio = 0.65;
	if (m_pressed == part_thumb)	ratio = 0.85;

	Gdiplus::Color cr_thumb = get_color(m_theme.cr_back, m_theme.cr_text, ratio);

	//cross 두께는 m_thickness 가 직접 결정 — DeflateRect 로 더 깎으면 얇은 thumb 가 사라짐.
	int radius = max(1, m_thickness / 2);

	//draw_round_rect → get_round_rect_path 는 stroke 여백 명목으로 fill 영역을 항상 1px deflate 한다(fill-only 라도
	//stroke_thick 기본값 1 이 전달됨). 그대로 두면 얇은 thumb 가 cross 폭 1px 좁아지고 0.5px 좌(세로)/상(가로)으로
	//밀려 track 중앙 정렬이 깨진다(세로 scrollbar 에서 좌측 1px 치우침으로 보임). width/height 에 +1 보정해 deflate
	//후 의도한 rThumb 크기·중심이 되게 한다. 공유 draw_round_rect 는 다른 UI 회귀 위험이 있어 호출부에서 보정.
	draw_round_rect(&g, Gdiplus::Rect(rThumb.left, rThumb.top, rThumb.Width() + 1, rThumb.Height() + 1),
		Gdiplus::Color::Transparent, cr_thumb, radius, 0);
}

void CSCScrollbar::draw_arrow(Gdiplus::Graphics& g, const CRect& rArrow, bool toward_lt, SCROLL_PART part)
{
	if (rArrow.IsRectEmpty())
		return;

	//배경은 track 과 동일 — 클릭/hover 시에도 변경하지 않음.
	Gdiplus::Color cr_track = get_color(m_theme.cr_back, 24);
	Gdiplus::SolidBrush br_bg(cr_track);
	g.FillRectangle(&br_bg, rArrow.left, rArrow.top, rArrow.Width(), rArrow.Height());

	//삼각형 — 탐색기 reference. sharp triangle (FillPolygon) + 각 vertex 의 1×1 픽셀을 배경색으로 덮어 round triangle 효과.
	//세로 scrollbar 기준: 가로 8 (hw=4), 세로 6 (hh=3). 가로 scrollbar 는 90° 회전 → 가로 6, 세로 8 (코드상 hw/hh swap 적용).
	//PointF sub-pixel center 사용 — visual mass center 가 client 정중앙 (width 16 → sub-pixel 8.0) 에 정확 align.
	Gdiplus::Color cr_glyph = get_color(m_theme.cr_back, m_theme.cr_text, 0.75);
	Gdiplus::SolidBrush br_glyph(cr_glyph);

	Gdiplus::REAL cx = (rArrow.left + rArrow.right) * 0.5f;
	Gdiplus::REAL cy = (rArrow.top + rArrow.bottom) * 0.5f;

	Gdiplus::REAL hw = 4.0f;	//세로 scrollbar 기준: 삼각형 가로 반폭 (전체 8). 가로 scrollbar 에선 세로 반폭 (전체 8).
	Gdiplus::REAL hh = 3.0f;	//세로 scrollbar 기준: 삼각형 세로 반높이 (전체 6). 가로 scrollbar 에선 가로 반폭 (전체 6).
	if (m_pressed == part)
	{
		hw *= 0.85f;
		hh *= 0.85f;
	}

	Gdiplus::PointF pts[3];
	if (m_orient == vertical)
	{
		if (toward_lt)	{ pts[0] = Gdiplus::PointF(cx - hw, cy + hh); pts[1] = Gdiplus::PointF(cx, cy - hh); pts[2] = Gdiplus::PointF(cx + hw, cy + hh); }
		else			{ pts[0] = Gdiplus::PointF(cx - hw, cy - hh); pts[1] = Gdiplus::PointF(cx, cy + hh); pts[2] = Gdiplus::PointF(cx + hw, cy - hh); }
	}
	else
	{
		if (toward_lt)	{ pts[0] = Gdiplus::PointF(cx + hh, cy - hw); pts[1] = Gdiplus::PointF(cx - hh, cy); pts[2] = Gdiplus::PointF(cx + hh, cy + hw); }
		else			{ pts[0] = Gdiplus::PointF(cx - hh, cy - hw); pts[1] = Gdiplus::PointF(cx + hh, cy); pts[2] = Gdiplus::PointF(cx - hh, cy + hw); }
	}
	g.FillPolygon(&br_glyph, pts, 3);

	//각 vertex 의 1×1 픽셀을 배경색으로 덮음 → 1px 뾰족점 제거, 라운드 삼각형 효과.
	for (int i = 0; i < 3; i++)
	{
		int vx = (int)pts[i].X;
		int vy = (int)pts[i].Y;
		g.FillRectangle(&br_bg, vx, vy, 1, 1);
	}
}

void CSCScrollbar::OnPaint()
{
	CPaintDC dc1(this);

	//BeginPaint 의 hdc 는 update region 으로 clip 됨 — 호출자가 row 단위 partial invalidate 시 BitBlt 가 dest 의 일부만 copy.
	//SelectClipRgn(NULL) 로 clip 제거 → 항상 client 전체 paint. (BeginPaint 의 ValidateRect 효과는 유지되므로 무한 paint 위험 없음.)
	::SelectClipRgn(dc1.m_hDC, NULL);

	CRect rc;
	GetClientRect(&rc);

	CMemoryDC dc(&dc1, &rc);
	dc.FillSolidRect(rc, m_theme.cr_back.ToCOLORREF());

	Gdiplus::Graphics g(dc);
	g.SetSmoothingMode(Gdiplus::SmoothingModeHighQuality);

	//track 배경은 window 전체 (arrow 영역 포함) — 평소에도 그 영역까지 시각적으로 같은 톤.
	//thumb 의 스크롤 범위는 calc_track_rect (arrow 제외) — calc_thumb_rect 가 그 위에서 계산.
	draw_track(g, get_visible_rect());

	//arrow 영역은 항상 reserve (thumb 범위 일관) — 삼각형 그리기만 hover 시.
	if (m_show_arrows && m_window_hovering)
	{
		draw_arrow(g, calc_arrow_lt_rect(), true, part_arrow_lt);
		draw_arrow(g, calc_arrow_rb_rect(), false, part_arrow_rb);
	}

	draw_thumb(g, calc_thumb_rect());
}
