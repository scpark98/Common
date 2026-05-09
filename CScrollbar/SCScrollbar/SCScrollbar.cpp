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
	int t = max(4, thickness);
	m_thickness_normal = t;
	if (m_thickness_hover < t + 2)
		m_thickness_hover = t + 4;
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
	CRect rc;
	GetClientRect(&rc);
	if (m_orient == vertical)
	{
		int margin = (rc.Width() - m_thickness) / 2;
		if (margin > 0)
		{
			rc.left += margin;
			rc.right -= margin;
		}
	}
	else
	{
		int margin = (rc.Height() - m_thickness) / 2;
		if (margin > 0)
		{
			rc.top += margin;
			rc.bottom -= margin;
		}
	}
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

	if (m_orient == vertical)
		return CRect(rTrack.left, rTrack.top + thumb_offset, rTrack.right, rTrack.top + thumb_offset + thumb_extent);
	return CRect(rTrack.left + thumb_offset, rTrack.top, rTrack.left + thumb_offset + thumb_extent, rTrack.bottom);
}

CSCScrollbar::SCROLL_PART CSCScrollbar::hit_test(CPoint pt) const
{
	if (m_show_arrows)
	{
		if (calc_arrow_lt_rect().PtInRect(pt)) return part_arrow_lt;
		if (calc_arrow_rb_rect().PtInRect(pt)) return part_arrow_rb;
	}

	CRect rThumb = calc_thumb_rect();
	if (!rThumb.IsRectEmpty() && rThumb.PtInRect(pt))
		return part_thumb;

	CRect rTrack = calc_track_rect();
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
	if (CWnd* parent = GetParent())
	{
		CSCScrollbarMsg payload(msg, this, m_pos);
		::SendMessage(parent->GetSafeHwnd(), Message_CSCScrollbar, (WPARAM)&payload, 0);
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

	//track 색은 cr_back 에서 약간 톤 변경 — light theme 면 어둡게, dark theme 면 밝게.
	Gdiplus::Color cr_track = get_color(m_theme.cr_back, 8);

	Gdiplus::SolidBrush br(cr_track);
	g.FillRectangle(&br, rTrack.left, rTrack.top, rTrack.Width(), rTrack.Height());
}

void CSCScrollbar::draw_thumb(Gdiplus::Graphics& g, const CRect& rThumb)
{
	if (rThumb.IsRectEmpty())
		return;

	//thumb base color = cr_text 와 cr_back 의 중간 (약 25% text). hover/pressed 시 점진적으로 cr_text 에 가까워짐.
	double ratio = 0.30;
	if (m_hover == part_thumb)		ratio = 0.50;
	if (m_pressed == part_thumb)	ratio = 0.70;

	Gdiplus::Color cr_thumb = get_color(m_theme.cr_back, m_theme.cr_text, ratio);

	int radius = max(2, m_thickness / 4);
	CRect rDraw = rThumb;
	//thumb 양옆 1px 여백 — track 바닥과 시각 구분.
	if (m_orient == vertical)
		rDraw.DeflateRect(2, 1);
	else
		rDraw.DeflateRect(1, 2);

	draw_round_rect(&g, Gdiplus::Rect(rDraw.left, rDraw.top, rDraw.Width(), rDraw.Height()),
		Gdiplus::Color::Transparent, cr_thumb, radius, 0);
}

void CSCScrollbar::draw_arrow(Gdiplus::Graphics& g, const CRect& rArrow, bool toward_lt, SCROLL_PART part)
{
	if (rArrow.IsRectEmpty())
		return;

	//배경 — hover/pressed 면 약간 강조.
	Gdiplus::Color cr_bg = m_theme.cr_back;
	if (m_hover == part)	cr_bg = get_color(m_theme.cr_back, 16);
	if (m_pressed == part)	cr_bg = get_color(m_theme.cr_back, 32);
	Gdiplus::SolidBrush br_bg(cr_bg);
	g.FillRectangle(&br_bg, rArrow.left, rArrow.top, rArrow.Width(), rArrow.Height());

	//화살표 글리프 — 작은 삼각형. orient 와 toward_lt 조합으로 4 방향.
	Gdiplus::Color cr_glyph = (m_pressed == part) ? m_theme.cr_text : get_color(m_theme.cr_back, m_theme.cr_text, 0.55);
	Gdiplus::SolidBrush br_glyph(cr_glyph);

	CPoint c = rArrow.CenterPoint();
	int s = max(3, min(rArrow.Width(), rArrow.Height()) / 4);

	Gdiplus::Point pts[3];
	if (m_orient == vertical)
	{
		if (toward_lt)	{ pts[0] = Gdiplus::Point(c.x - s, c.y + s/2); pts[1] = Gdiplus::Point(c.x + s, c.y + s/2); pts[2] = Gdiplus::Point(c.x, c.y - s/2); }
		else			{ pts[0] = Gdiplus::Point(c.x - s, c.y - s/2); pts[1] = Gdiplus::Point(c.x + s, c.y - s/2); pts[2] = Gdiplus::Point(c.x, c.y + s/2); }
	}
	else
	{
		if (toward_lt)	{ pts[0] = Gdiplus::Point(c.x + s/2, c.y - s); pts[1] = Gdiplus::Point(c.x + s/2, c.y + s); pts[2] = Gdiplus::Point(c.x - s/2, c.y); }
		else			{ pts[0] = Gdiplus::Point(c.x - s/2, c.y - s); pts[1] = Gdiplus::Point(c.x - s/2, c.y + s); pts[2] = Gdiplus::Point(c.x + s/2, c.y); }
	}
	g.FillPolygon(&br_glyph, pts, 3);
}

void CSCScrollbar::OnPaint()
{
	CPaintDC dc1(this);
	CRect rc;
	GetClientRect(&rc);

	CMemoryDC dc(&dc1, &rc);
	dc.FillSolidRect(rc, m_theme.cr_back.ToCOLORREF());

	Gdiplus::Graphics g(dc);
	g.SetSmoothingMode(Gdiplus::SmoothingModeHighQuality);

	draw_track(g, calc_track_rect());

	if (m_show_arrows)
	{
		draw_arrow(g, calc_arrow_lt_rect(), true, part_arrow_lt);
		draw_arrow(g, calc_arrow_rb_rect(), false, part_arrow_rb);
	}

	draw_thumb(g, calc_thumb_rect());
}
