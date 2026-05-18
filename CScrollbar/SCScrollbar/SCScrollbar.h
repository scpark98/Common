#pragma once

#include <afxwin.h>
#include <afxext.h>
#include <gdiplus.h>

#include "../../colors.h"

/*
CSCScrollbar — 자체 그리기 스크롤바.
 - OS native 스크롤바 색을 못 바꾸는 한계를 우회. 어떤 host 에든 부착 가능.
 - 모델 (range/page/pos/line) 은 host 가 push, host 는 Message_CSCScrollbar 로 pos 변경 통지 수신.
 - 그리기는 CSCColorTheme 기반 — track/thumb/hover/pressed 색을 m_theme.cr_back / cr_text 로부터 derive.
   호스트가 set_color_theme 한 번만 호출하면 일관된 컬러.
 - XP 호환 — GDI+ 그리기 + 표준 WM_* 메시지만. SetWindowTheme / DWM 미사용.

[수신 패턴]
ON_REGISTERED_MESSAGE(Message_CSCScrollbar, &CHostDlg::on_message_CSCScrollbar)
LRESULT CHostDlg::on_message_CSCScrollbar(WPARAM wParam, LPARAM lParam) {
    CSCScrollbarMsg* msg = (CSCScrollbarMsg*)wParam;
    //msg->msg : msg_scrollbar_pos_changed / drag_start / drag_end
    //msg->pos : 새 pos
    return 0;
}
*/

static const UINT Message_CSCScrollbar = ::RegisterWindowMessage(_T("MessageString_CSCScrollbar"));

class CSCScrollbar;

class CSCScrollbarMsg
{
public:
	CSCScrollbarMsg(int _msg, CSCScrollbar* _this, int _pos)
		: pThis(_this), msg(_msg), pos(_pos)
	{
	}

	enum SCScrollbarMsgs
	{
		msg_scrollbar_pos_changed = 0,
		msg_scrollbar_drag_start,
		msg_scrollbar_drag_end,
	};

	CSCScrollbar*	pThis;
	int				msg;
	int				pos;
};


class CSCScrollbar : public CWnd
{
	DECLARE_DYNAMIC(CSCScrollbar)

public:
	CSCScrollbar();
	virtual ~CSCScrollbar();

	enum ORIENTATION { vertical = 0, horizontal };

	bool	create(CWnd* parent, ORIENTATION orient, int x, int y, int cx, int cy, UINT id = 0);

//모델 — host 가 set_range/set_page/set_pos 로 push, set_line 으로 단일 step 정의.
	void	set_range(int min_pos, int max_pos);
	void	set_page(int page_size);
	void	set_pos(int pos, bool emit_message = false);
	void	set_line(int line_step) { m_line = max(1, line_step); }
	int		get_pos() const { return m_pos; }
	int		get_range_min() const { return m_range_min; }
	int		get_range_max() const { return m_range_max; }
	int		get_page() const { return m_page; }

//스크롤 보조 — host 의 OnMouseWheel / 키보드 Page Up/Down 등에서 직접 호출.
	void	scroll_by_lines(int lines, bool emit_message = true);
	void	scroll_by_pages(int pages, bool emit_message = true);

//시각 옵션
	void	set_thickness(int thickness);					//평상시 두께 (resting).
	void	set_hover_thickness(int thickness);				//mouse 호버시 두께 — overlay 로 확장 (host 콘텐츠 위 살짝 덮음). 기본 normal+4.
	void	set_show_arrows(bool show);
	bool	get_show_arrows() const { return m_show_arrows; }
	int		get_thickness_normal() const { return m_thickness_normal; }
	int		get_thickness_hover() const { return m_thickness_hover; }	//host 가 layout 시 reserve 할 두께 — 윈도우 width 가 항상 hover thickness.

//테마 — int preset 또는 외부 CSCColorTheme 그대로 가져오기.
	void	set_color_theme(int theme, bool invalidate = true);
	void	set_color_theme(const CSCColorTheme& theme, bool invalidate = false);

	CSCColorTheme	m_theme = CSCColorTheme(this);

//Message_CSCScrollbar 발신 대상 — 미지정 시 GetParent() 로 fallback.
//overlay 가 host 의 형제로 (다른 window 의 child 로) 부착될 때 host 가 자기 자신을 target 으로 지정 가능.
	void			set_message_target(CWnd* target) { m_message_target = target; }

protected:
	CWnd*			m_message_target = nullptr;

	ORIENTATION		m_orient = vertical;

	//모델
	int				m_range_min = 0;
	int				m_range_max = 0;	//content 의 끝값. visible window 의 마지막 pos = max - page + 1.
	int				m_pos = 0;
	int				m_page = 1;
	int				m_line = 1;

	//시각 — m_thickness 는 *thumb 의 cross 두께만*. track 영역(=window 폭) 과 별개.
	int				m_thickness = 2;			//현재 적용된 thumb cross 두께.
	int				m_thickness_normal = 2;		//resting thumb 두께. host window 폭 (16, even) 과 parity 일치 → FillRectangle visual center sub-pixel 이 client 정중앙 (8.0) 와 일치.
	int				m_thickness_hover = 6;		//hover thumb 두께. 탐색기 reference.
	bool			m_window_hovering = false;	//마우스가 scrollbar 윈도우 안에 있는지.
	bool			m_show_arrows = true;	//arrow 영역은 항상 reserve, 그리기만 hover 시 — 탐색기 스타일.
	int				m_arrow_size = 16;	//화살표 영역 크기. host window 폭 (16) 과 동일 → 정사각형 16×16.

	//상호작용 상태
	enum SCROLL_PART { part_none = 0, part_arrow_lt, part_arrow_rb, part_thumb, part_track_lt, part_track_rb };
	SCROLL_PART		m_hover = part_none;
	SCROLL_PART		m_pressed = part_none;
	int				m_drag_offset = 0;		//drag 시작 시 마우스와 thumb 시작점 사이 오프셋 (pixel).
	bool			m_dragging = false;
	bool			m_tracking_leave = false;	//OnMouseMove 안에서 TrackMouseEvent 등록했는지.

	enum TIMER_ID { timer_repeat_initial = 1, timer_repeat = 2 };
	int				m_repeat_initial_ms = 300;
	int				m_repeat_ms = 50;

	//기하 계산
	SCROLL_PART		hit_test(CPoint pt) const;
	CRect			calc_arrow_lt_rect() const;	//vertical=top, horizontal=left
	CRect			calc_arrow_rb_rect() const;	//vertical=bottom, horizontal=right
	CRect			calc_track_rect() const;		//arrow 사이의 트랙 전체 (arrow off 면 client 전체).
	CRect			calc_thumb_rect() const;		//track 안의 thumb 영역.
	int				thumb_min_extent() const { return 60; }

	//pos 계산 보조
	void			clamp_pos();
	void			update_hover(CPoint pt);
	int				pos_from_thumb_offset(int track_origin, int track_extent, int thumb_extent, int mouse_along) const;

	//hover-expand: 윈도우는 항상 hover thickness 만큼 잡고, 안에서 track/thumb 만 m_thickness 만큼 가운데 정렬해 그림.
	//resting → hover 시 좌우 모두 (hover-normal)/2 만큼 늘어나는 시각 효과.
	void			apply_visible_thickness(int new_thickness);
	CRect			get_visible_rect() const;	//window client 안에서 m_thickness 만큼 가운데 정렬한 영역.

	//알림
	void			emit(int msg);

	//그리기 보조 — track/thumb/arrow 분리.
	void			draw_track(Gdiplus::Graphics& g, const CRect& rTrack);
	void			draw_thumb(Gdiplus::Graphics& g, const CRect& rThumb);
	void			draw_arrow(Gdiplus::Graphics& g, const CRect& rArrow, bool toward_lt, SCROLL_PART part);

	afx_msg void	OnPaint();
	afx_msg BOOL	OnEraseBkgnd(CDC* pDC);
	afx_msg void	OnSize(UINT nType, int cx, int cy);
	afx_msg void	OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void	OnLButtonUp(UINT nFlags, CPoint point);
	afx_msg void	OnMouseMove(UINT nFlags, CPoint point);
	afx_msg LRESULT	OnMouseLeave(WPARAM wParam, LPARAM lParam);
	afx_msg void	OnTimer(UINT_PTR nIDEvent);
	DECLARE_MESSAGE_MAP()
};
