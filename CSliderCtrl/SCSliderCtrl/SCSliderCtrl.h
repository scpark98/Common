#if !defined(AFX_SCSLIDERCTRL_H__70340BD3_CE86_4EE4_A00B_5A4EC2B6A92D__INCLUDED_)
#define AFX_SCSLIDERCTRL_H__70340BD3_CE86_4EE4_A00B_5A4EC2B6A92D__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// SCSliderCtrl.h : header file
//

#include <afxwin.h>
#include <Afxdisp.h>
#include <afxext.h>         // MFC 확장입니다.
#include <afxcmn.h>             // Windows 공용 컨트롤에 대한 MFC 지원입니다.

#include <deque>
#include <algorithm>
#include <vector>

#include "../../colors.h"
#include "../../SCGdiplusBitmap.h"

/*
[주의]
이벤트 메시지 처리 방식 주의!
일반적인 CSliderCtrl은 parent에서 WM_HSCROLL 이벤트로 처리하지만
이 CSCSliderCtrl에서는 grab, release 등의 세밀한 이벤트 처리가 필요하므로 WM_HSCROLL을 써서는 안된다.
반드시 아래 메시지를 수신받아 처리할 것!

LRESULT CTest_SliderCtrlExDlg::on_message_CSCSliderCtrl(WPARAM wParam, LPARAM lParam)
{
	CSCSliderCtrlMsg *msg = (CSCSliderCtrlMsg*)wParam;
	...
}
*/

static const UINT Message_CSCSliderCtrl = ::RegisterWindowMessage(_T("MessageString_CSCSliderCtrl"));

//트랙 이벤트가 발생했을 때 바로 PostMessage를 호출하게 되면
//미세한 마우스 움직임들에 대해 무수히 많은 이벤트가 발생하게 되므로
//딜레이가 많이 발생된다. 타이머를 이용하여 매우 짧은 시간에 발생하는 중복된 이벤트는 스킵되도록 하면
//트랙 이동이 매우 부드럽게 처리된다.
//단, 이럴 경우 message queue에 메시지가 쌓이거나 타이머를 만나는데까지 아주 짧은 시간에도
//다른 트랙 이동과 관련된 코드를 만날 경우 현재 변경된 트랙으로 이동되지 않는 현상이 발생한다.
//예를 들어 동영상 재생기를 만들었고 재생중에 트랙의 임의 위치를 클릭하는 순간
//해당 위치로 이동해야하는데 재생 정보에 의해 갱신되는 SetPos에 의해
//클릭된 위치로 이동되었다가 바로 재생위치로 되돌아가는 현상이 발생한다.
//settimer방식일 경우 트랙 이동이 매우 부드러운 장점은 있으나 위의 단점이 크므로
//일단 콜백방식으로도 변경해 보았으나 PostMessage방식과 유사한 단점이 존재한다.
//
//scpark 20190823
//결국 timer 방식으로 변경하고 timer 방식의 문제점이었던 재생 중 클릭시 트랙이동이 안되던 문제는
//PostMessage를 SendMessage로 변경하고
//msg를 세분화해서 grab, move, release에 따라 동작 방식을 달리 구현하여 해결되었다.
//우선 이렇게 timer 방식으로 계속 사용해보고 문제가 없다면 코드를 다시 정리하자.

/*
* scpark 20240501
* bitmap 이미지들로 slidectrl를 표현하는 코드에서 시작됐으나 좋은 디자인 이미지가 필요하고 크기조절 지원도 쉽지 않다.
* 최근 동향은 이미지를 이용한 스타일은 거의 사용되지 않고 gdiplus로 깔끔하게 그려주는 것이 좋다.
* 우선 이미지 설정관련 코드들은 살려두되 사용되지 않으며 삭제 예정임.
* 
* scpark 20240902
* WM_LBUTTONDOWN, WM_MOUSEMOVE와 같은 핸들러의 맨 마지막줄에 보면 CSliderCtrl::OnLButtonDown(nFlags, point);과 같이
* 기본 핸들러를 호출하는데 이 코드를 살려두면 클릭시, 드래그시에 WM_PAINT가 호출되면서 pos가 달라지는 부작용이 발생했다.
* 이 기본 핸들러를 주석처리하니 원하는 형태로 잘 동작한다.
* CMacProgress에서는 주석처리하지 않아도 잘 동작한다. 뭔가 차이가 있는 듯 하다.
*/

/////////////////////////////////////////////////////////////////////////////
// CSCSliderCtrl window

class CSCSliderCtrl;

class CSCSliderCtrlMsg
{
public:
	CSCSliderCtrlMsg(int _msg, CSCSliderCtrl* _this, int _pos)
	{
		//ctrl_id = _ctrl_id;
		pThis = _this;
		msg = _msg;
		pos = _pos;
	}

	enum SCSliderCtrlMsgs
	{
		msg_thumb_grab = 0,
		msg_thumb_move,
		msg_thumb_release,
		msg_thumb_track_bottom_slide,
		msg_hover,			//hover 프리뷰 — pos = hover 위치 값(슬라이더 단위). m_use_hover_preview 일 때만 발송.
		msg_hover_leave,	//hover 종료(마우스가 슬라이더 밖으로). pos 무의미.
	};

	CSCSliderCtrl* pThis = NULL;
	int		msg;
	//int		ctrl_id;
	int		pos;
};

class CSCSliderCtrlBookmark
{
public:
	CSCSliderCtrlBookmark(int _pos, CString _name)
	{
		pos = _pos;
		name = _name;
	}

	int pos;
	CString name;
};

class CSCSliderCtrlSteps
{
public:
	CSCSliderCtrlSteps() {};

	CRect	r;
	CSCGdiplusBitmap m_img;
};

class CSCSliderCtrl : public CSliderCtrl
{
// Construction
public:
	CSCSliderCtrl();

//slider_step 관련
	std::vector<CSCSliderCtrlSteps> m_steps;
//slider_step 관련
	//step 이미지에 사용될 이미지들을 세팅한다.
	//void	set_step_images();
	//pos 위치에 resource id 이미지를 표시한다.
	void	set_step_image(int pos, int id);

	enum SCSliderCtrlStyle
	{
		style_normal = 0,
		style_thumb,
		style_thumb_round,
		style_thumb_round_alpha,	//색상 선택 시 alpha값 조절 용도로 사용. 트랙 배경에 격자패턴을 깔고 thumb에 alpha를 적용해서 그린다.
		style_thumb_round_hue,		//hue 그라디언트 트랙 + 둥근 썸
		style_thumb_round_gradient,	//밝기 선택 그라디언트 트랙 + 둥근 썸
		style_value,
		style_progress,
		style_progress_line,
		style_track,
		style_step,		//진행 단계를 표시. 1-2-3-4 과 같은 형태. 
		style_koino,
	};

	enum SCSliderCtrlTextStyle
	{
		text_style_none = 0,
		text_style_value,			//GetPos()의 값으로 표시
		text_style_percentage,		//percentage 값으로 표시
		text_style_user_defined,	//중앙에 사용자가 직접 설정한 값으로 표시
		text_style_dual_text,		//양쪽에 사용자가 직접 설정한 값으로 표시
	};

	enum TIMER
	{
		timer_post_pos = 0,
	};

	enum SCSliderCtrlEventMsgStyle
	{
		msg_style_timer = 0,
		msg_style_post,
		msg_style_callback,
	};


// Operations
public:
	void	SetEventMsgStyle(int style);
	void	SetCallbackFunction(void (*p_func)(CWnd* pParent, CWnd* pWnd, DWORD msg, UINT pos))
	{
		m_pParentWnd = GetParent();
		m_pCallback_func = p_func;
	}	//콜백함수 콜 방식으로 이벤트를 전달할 경우 사용.

	//int		GetPos();
	//m_hWnd 유효한지 검사, 특정 스타일에서의 별도 처리등이 포함되어 있으므로 CSliderCtrl::SetPos() 대신 이 함수를 사용한다.
	//SetPos()를 사용해도 set_pos()가 호출되도록 함.
	void	set_pos(int pos);
	void	SetPos(int pos) { set_pos(pos); }
	void	set_range(int lower, int upper) { SetRange(lower, upper); }
	int		get_lower();
	int		get_upper();
	void	set_step_completed() { m_step_completed = true; Invalidate(); }
	int		get_style() { return m_style; }
	// style_thumb_round_alpha ~ style_thumb_round_gradient: pill 트랙 + 별도 둥근 썸
	bool	is_round_pill_style() const { return m_style >= style_thumb_round_alpha && m_style <= style_thumb_round_gradient; }

	int32_t	step(int step = 1);

	//void	SetPos(int pos) { CSliderCtrl::SetPos(pos); Invalidate(); /*redraw_window();*/ }
	void	redraw_window(bool bErase = false);

	//style_thumb,	style_value, style_progress, style_track, style_step, 
	void	set_style(int nStyle);
	void	set_track_height(float height) { m_track_height = height; }

	void	set_use_slide(bool enable = true) { m_use_slide = enable; }
	//void	set_enable_bottom_slide(bool enable) { m_enable_bottom_slide = enable; }
	void	DrawFocusRect(BOOL bDraw = TRUE, BOOL bRedraw = FALSE);

	//set_track_color()는 active 영역과 inactive 영역을 지정하므로
	void	set_track_color(Gdiplus::Color cr_active, Gdiplus::Color cr_inactive) { m_cr_active = cr_active; m_cr_inactive = cr_inactive; }
	void	set_active_color(Gdiplus::Color cr_active);	//단색(solid).

	//active(processed) 영역을 현재 위치 %로 stops 보간한 '단색'으로 채운다 (0%=첫색 ~ 100%=끝색).
	//예: 볼륨 — set_active_color_by_pos(Orange, Red) → 작을 땐 주황, 클수록 빨강.
	void	set_active_color_by_pos(const std::vector<Gdiplus::Color>& colors);
	template<typename... TRest>
	void	set_active_color_by_pos(Gdiplus::Color cr_first, Gdiplus::Color cr_second, TRest... cr_rest)
	{
		set_active_color_by_pos(std::vector<Gdiplus::Color>{ cr_first, cr_second, Gdiplus::Color(cr_rest)... });
	}

	//active(processed) 영역 자체를 좌→우 N-stop 그라디언트로 채운다.
	//예: set_active_gradient(Black, Red, Yellow).
	void	set_active_gradient(const std::vector<Gdiplus::Color>& colors);
	template<typename... TRest>
	void	set_active_gradient(Gdiplus::Color cr_first, Gdiplus::Color cr_second, TRest... cr_rest)
	{
		set_active_gradient(std::vector<Gdiplus::Color>{ cr_first, cr_second, Gdiplus::Color(cr_rest)... });
	}

	void	set_inactive_color(Gdiplus::Color cr_inactive);
	void	set_back_color(Gdiplus::Color cr_back);
	void	set_thumb_color(Gdiplus::Color cr_thumb);
	void	set_tic_color(Gdiplus::Color cr_tic);

	// ── N-stop 그라디언트 (style_thumb_round_gradient 전용) ──────────
	// 비어 있으면 기존 cr_inactive→cr_active 2색 폴백.
	// 호출 예: set_gradient_colors({ Black, PureColor, White });
	void	set_gradient_colors(const std::vector<Gdiplus::Color>& colors);
	void	set_gradient_colors(std::initializer_list<Gdiplus::Color> colors);

	void	set_text_color(Gdiplus::Color cr_text) { m_theme.cr_text = cr_text; Invalidate(); }
	void	set_text_style(int text_style) { m_text_style = text_style; Invalidate(); }
	void	set_text(LPCTSTR text, ...);
	void	set_text_dual(LPCTSTR text_dual, ...);

	int		m_tic_freq = 0;
	bool	m_tic_show_text = false;	//default = false
	//틱 동작 3모드 (m_tic_freq > 0 일 때만 의미):
	// 1) 둘 다 false : 틱은 표시만, 위치 자유.
	// 2) m_auto_snap : 틱 ±m_snap_tolerance(자석 범위) 안이면 그 틱으로 끌어당김, 밖은 자유. (자석식)
	// 3) m_tic_only  : 틱 위치만 선택 가능, 항상 가장 가까운 틱으로 강제. (이산값 — 해상도 목록처럼)
	// 둘 다 켜지면 m_tic_only 우선.
	bool	m_auto_snap = false;
	bool	m_tic_only = false;
	int		m_snap_tolerance = 2;		//m_auto_snap 자석 범위 (pos 값 단위, ±). 틱과의 거리가 이 이하면 스냅.
	//freq는 CSliderCtrl::SetTicFreq()와 동일하게 lower ~ upper 사이의 구간을 몇 등분할 것인지가 아닌 간격을 의미한다.
	//즉, freq = 23이면 23 등분이 아니라 0 ~ 23 ~ 46 ~ 69 ~ 92와 같이 틱이 표시된다.
	void	set_tic_freq(int freq, bool show_text = false);

	//모드2 — 드래그 중 틱 ±tolerance 안이면 그 틱으로 끌어당김(자석), 밖은 자유 이동.
	void	set_auto_snap(bool enable = true) { m_auto_snap = enable; }
	//모드3 — 틱 위치만 선택 가능(이산값). 항상 가장 가까운 틱으로 강제. 픽셀폭보다 range 가 커도 정확히 틱에 안착.
	void	set_tic_only(bool enable = true) { m_tic_only = enable; }
	//모드2 자석 범위 (pos 값 단위, ±). 기본 2.
	void	set_snap_tolerance(int tolerance) { m_snap_tolerance = tolerance; }

	void	use_bookmark(bool use = true) { m_use_bookmark = use; }

	//트랙 hover 시 parent 로 Message_CSCSliderCtrl(msg_hover, pos) 발송 — 호버 썸네일 프리뷰용. opt-in(기본 off).
	//off 면 기존 사용처에 영향 없음. 마우스가 슬라이더를 벗어나면 msg_hover_leave 1회 발송(프리뷰 숨김용).
	//tooltip 과 상호배타 — 켜면 tooltip 강제 비활성(set_track_total 등이 use_tooltip(true) 재호출해도 안 뜸).
	void	use_hover_preview(bool use = true)
	{
		m_use_hover_preview = use;
		if (m_tooltip.GetSafeHwnd())
			m_tooltip.Activate((m_use_tooltip && !use) ? TRUE : FALSE);
	}
	void	set_bookmark_color(Gdiplus::Color cr);
	void	set_bookmark_current_color(Gdiplus::Color cr);

	//pos 에 북마크 추가 — 같은 pos 가 이미 있으면 그 항목 제거 (토글). 없으면 push.
	//N키 같은 사용자 입력의 기본 호출. 슬라이더가 데이터 own 하는 사용 패턴.
	void	add_bookmark(int pos, CString name = _T(""));

	//북마크 list 전체 교체 — 외부에서 데이터 own 하는 경우 view 갱신용.
	//입력 list 에 같은 pos 가 중복되면 마지막 항목만 남김 (방어).
	void	set_bookmarks(const std::deque<CSCSliderCtrlBookmark>& items);

	//모두 비움.
	void	clear_bookmarks();

	//read-only 조회. prev/next 등 호출자가 직접 결정해야 할 때 사용.
	const std::deque<CSCSliderCtrlBookmark>& get_bookmarks() const { return m_bookmark; }

	//현재 위치 from_pos 에서 가장 가까운 이전/다음 북마크의 index 반환. 없으면 -1.
	//호출자가 prev/next 점프 시 → idx 받아 get_bookmarks()[idx].pos 로 점프 위치 결정.
	int		get_near_bookmark(int from_pos, bool forward);

	enum TOOLTIP_FORMAT
	{
		tooltip_value = 0,
		tooltip_time,
		tooltip_time_ms,
		tooltip_time_percent,	//"HH:MM:SS(NN%)" — 시간 + 트랙 진행률.
	};
	void			use_tooltip(bool use = true);	//hWnd valid 면 즉시 tooltip 생성 (PreSubclassWindow 이후 호출에 대응).
	void			set_tooltip_format(int format = tooltip_value) { m_tooltip_format = format; }

//구간 반복 관련(slider_track style일 경우)
	int				m_repeat_start = -1;
	int				m_repeat_end = -1;
	//둘 다 -1이면 반복 해제.
	//하나만 -1이면 시작 또는 끝은 미디어의 시작 또는 끝으로
	void			set_repeat_range(int start, int end);

	//pos가 0이상이면 그 트랙 위치를 start 또는 end로 설정하고
	//-2이면 현재 위치를, -1면 해제의 의미로 처리한다.
	void			set_repeat_start(int pos = -2);
	//pos가 0이상이면 그 트랙 위치를 start 또는 end로 설정하고
	//-2이면 현재 위치를, -1면 해제의 의미로 처리한다.
	void			set_repeat_end(int pos = -2);

//[sub track] seek 가능 구간 표시(style_track 전용) — 부분 다운로드/스트리밍 미디어에서 현재 받아진 frontier 를
//트랙에 빨간 1px 세로선으로 표시한다. range 단위(set_pos 와 동일)로 지정. -1 이면 숨김(완전 다운로드/일반 미디어).
	int				m_sub_track_pos = -1;
	void			set_sub_track_pos(int pos);

//폰트 관련
	LOGFONT			get_log_font() { return m_lf; }
	void			set_log_font(LOGFONT lf);
	int				get_font_size();
	void			set_font_size(int font_size);
	void			set_font_name(LPCTSTR font_name, BYTE char_set = DEFAULT_CHARSET);
	void			set_font_bold(int weight = FW_BOLD);
	void			set_font_italic(bool italic = true);
	void			enlarge_font_size(bool enlarge);

//border
	void			draw_progress_border(bool draw = true) { m_draw_progress_border = draw; }
	void			set_progress_border_color(Gdiplus::Color cr) { m_cr_progress_border = cr; }

//color theme 관련
	CSCColorTheme	m_theme = CSCColorTheme(this);	//m_theme(this); 는 오류.
	void			set_color_theme(int theme);
	void			set_color_theme(const CSCColorTheme& theme, bool invalidate = true);

	//disable이면 gray로 표시되지만 때로는 disable이 아니어도 gray로 표시해야 하는 경우도 있다.
	void			set_forced_gray(bool forced_gray, bool include_back = true) { m_forced_gray = forced_gray; m_forced_gray_include_back = include_back; Invalidate(); }

//layout
	//slider의 thumb는 거의 대부분 표시되는 상태지만 간혹 어떤 조건에 따라 pos는 그대로지만 thumb를 감출 필요가 있을 경우도 있다.
	//ex. zoom slider인 경우 80% 위치로 수동 설정했으나 auto fit을 클릭하면 화면 크기에 따라 그 값이 달라지는데
	//그 배율을 slider의 pos로 설정할 수도 있지만 auto fit을 토글하여 수동 설정된 배율과 auto fit을 토글할 수도 있다.
	void			hide_thumb(bool hide = true) { m_thumb_hide = hide; Invalidate(); }

	//drag/click+hold 중 여부 — 외부 timer 가 thumb 자동 갱신을 skip 할지 판단하는 용도.
	bool			is_lbutton_down() const { return m_lbuttondown; }

protected:
	// Attributes

	//slider_thumb,	slider_value, slider_progress, slider_track,
	int				m_style = style_normal;

	int				m_nEventMsgStyle;

	CRect			m_rc;

	CToolTipCtrl	m_tooltip;
	bool			m_use_tooltip = false;	//default = false
	int				m_tooltip_format = tooltip_value;	//default = tooltip_value
	CString			m_tooltip_last_text;	//OnMouseMove 마다 UpdateTipText 호출 시 같은 텍스트면 skip — popup redraw 깜빡임 회피.


	//실제 전체 구간 게이지 영역
	int				m_track_thick = 8;	//트랙 두께

	//잡고 움직이는 영역
	CSize			m_thumb;
	//4방향의 여백
	CRect			m_margin = CRect(0, 0, 0, 0);
	int				m_nMouseOffset;

	//현재는 세로모드에 대한 코드가 거의 구현되어 있지 않다.
	bool			m_is_vertical = false;

	//enable move the current pos by click or drag even though progress style. default = true
	bool			m_use_slide = true;

	//다른 슬라이더와는 다르게 style_step일 경우 pos를 기준으로 처리 - 현재 처리중인 위치 - 아직 처리하지 않은 위치
	//이렇게 3단계로 구분해서 그려주는데 pos가 upper step을 처리 완료한 후, 즉 모든 스텝이 끝나면 upper 또한 처리된 곳으로 표시해야 한다.
	bool			m_step_completed = false;

	//특정 위치들을 기억해두자. 북마크처럼.
	bool			m_use_bookmark = false;	//default = false;

	bool			m_use_hover_preview = false;	//opt-in hover 프리뷰 통지.
	int				m_last_hover_pos = -1;			//같은 pos 반복 발송 억제.
	bool			m_hover_tracking = false;		//TrackMouseEvent(TME_LEAVE) 무장 상태(중복 무장 방지).
	std::deque<CSCSliderCtrlBookmark> m_bookmark;
	//pos의 위치에 있는 북마크의 인덱스를 리턴한다.
	int				find_index_bookmark(int pos);
	//mouse move, sliding할 때 북마크 근처에 가면 이 값이 세팅된다. 근처가 아니면 -1.
	int				m_cur_bookmark = -1;
	//마우스 위치에서 가장 가까운 북마크를 찾는데 그 허용 오차를 미리 구해놓는다.
	int				m_bookmark_near_tolerance = 8;	//px
	//get_near_bookmark 는 public 섹션으로 이동 — 호출자가 prev/next 결정 시 사용.


	/*
	//m_enable_bottom_slide는 slider_track 스타일일 경우 미디어 플레이어의 트랙바로 쓰일 경우
	//전체화면에서는 cy의 아래 부분을 클릭해서 트랙이동해도 되지만
	//일반화면에서는 그 부분이 main dlg의 크기조절 영역과 겹치게 되므로
	//크기조절을 하고 싶어도 트랙이 이동되는 불편함이 있다.
	//따라서 옵션으로 cy의 아래 부분을 클릭해서 slide하는 것을 허용/불가 상태로 변경할 수 있도록 한다.
	//default = true;
	=> main dlg에서 먼저 WM_LBUTTONDOWN을 가로채서 크기조절 조건이라면 그 코드로 처리하고,
	그렇지 않은 조건일 경우만 이쪽으로 넘겨서 처리하도록 수정 완료.
	bool		m_enable_bottom_slide;
	*/
	bool			m_lbuttondown = false;
	bool			m_has_focus = false;
	bool			m_draw_focus_rect = false;

	//drag threshold — click+hold 중 손 jitter 가 미세한 px 변동을 만들면 매번 msg_thumb_move 가 발송되어
	//호출자가 grab 위치 근처로 반복 seek 하게 됨 (영상이 그 자리로 계속 되돌아가는 증상). reference point 가
	//drag 시작 전엔 lbutton_down_point, 시작 후엔 last_emit_point — 이 점으로부터 m_drag_threshold_px 이상
	//이동해야 emission. drag 후 mouse 가 멈춘 채 jitter 만 있으면 last_emit 의 threshold 안이라 emission 안 됨.
	CPoint			m_lbutton_down_point = CPoint(0, 0);
	CPoint			m_last_emit_point = CPoint(0, 0);
	bool			m_drag_started = false;
	static const int m_drag_threshold_px = 5;

	CString			m_text;			//m_text_style == text_style_user_defined일 경우 표시되는 텍스트
	CString			m_text_dual;	//m_text_style == text_style_dual_text일 경우 오른쪽에 표시되는 텍스트
	int				m_text_style = text_style_value;
	//COLORREF		m_cr_text = RGB(192, 192, 192);

	bool			m_transparent = false;
	//COLORREF		m_cr_back;				// back color of control
	Gdiplus::Color	m_cr_active;			//processed area (대표색 — _by_pos/_gradient 시 m_active_colors.front())
	enum active_fill_mode { active_solid, active_pos_color, active_gradient_fill };
	int				m_active_fill_mode = active_solid;	//active 영역 채우기 모드.
	std::vector<Gdiplus::Color>	m_active_colors;	//_by_pos / _gradient_fill 의 색 stops (2색 이상)
	Gdiplus::Color	m_cr_inactive;			//not processed area
	float			m_track_height = 14.0f;	//트랙 두께(px). float — g.FillRectangle(RectF) 에 그대로 사용해 짝수/소수 두께도 cy 에 정확히 센터 정렬.
	//CPen			m_penThumb;
	//CPen			m_penThumbLight;
	//CPen			m_penThumbLighter;
	//CPen			m_penThumbDark;
	//CPen			m_penThumbDarker;
	Gdiplus::Color	m_cr_thumb;
	Gdiplus::Color	m_cr_tic;
	Gdiplus::Color	m_cr_thumb_light;
	Gdiplus::Color	m_cr_thumb_lighter;
	Gdiplus::Color	m_cr_thumb_dark;
	Gdiplus::Color	m_cr_thumb_darker;
	Gdiplus::Color	m_cr_bookmark;
	Gdiplus::Color	m_cr_bookmark_current = gRGB(0, 255, 0);

	// N-stop 그라디언트 색상 (비어 있으면 2색 폴백)
	std::vector<Gdiplus::Color>	m_gradient_colors;
	// m_gradient_colors에서 비율 t(0~1)에 해당하는 색상을 보간하여 반환
	Gdiplus::Color	sample_gradient(float t) const;
	// 임의 stops(N개)에서 비율 t(0~1) 위치 색 보간 (active pos-color 모드용)
	static Gdiplus::Color	sample_stops(const std::vector<Gdiplus::Color>& stops, float t);

	//컨트롤의 enable, disable 상태에 따라 그려지는 색상이 달라지므로 사용
	Gdiplus::Color	enable_color(Gdiplus::Color cr, int offset = 64);

	//theme 의 accent(cr_progress)·배경에서 트랙/썸/틱 색을 파생한다. 개별 setter 로 일일이 지정하지 않아도
	//테마 교체만으로 슬라이더가 그 테마 톤으로 채색되도록 set_color_theme 의 양쪽 진입점이 공통 호출.
	void			apply_theme_colors();
	//썸 본색 + bevel 음영 4색 파생. Invalidate 안 함 — 생성자(m_hWnd NULL) 단계에서도 안전. set_thumb_color 의 코어.
	void			compute_thumb_shades(Gdiplus::Color cr_thumb);

	CWnd*			m_pParentWnd;
	void			(*m_pCallback_func)(CWnd* pParent, CWnd* pWnd, DWORD msg, UINT pos) = NULL;

	int				Pixel2Pos(int pixel);
	int				Pos2Pixel(int pos);
	int				snap_to_tic(int pos);	//m_auto_snap 시 pos 를 가장 가까운 틱(또는 끝점)으로 반올림. off 면 pos 그대로.
	void			PrepareMask(CBitmap *pBmpSource, CBitmap *pBmpMask, Gdiplus::Color clrpTransColor);
	void			DrawTransparentBitmap(	CDC *pDC, int xStart, int yStart, int wWidth, int wHeight,
											CDC *pTmpDC, int xSource, int ySource, CBitmap *bmMask);

	//폰트 관련
	LOGFONT			m_lf;
	CFont			m_font;
	int				m_font_size;
	void			reconstruct_font();

	//border
	Gdiplus::Color	m_cr_progress_border = Gdiplus::Color(188, 188, 188);
	bool			m_draw_progress_border = false;	//ctrl의 border가 아닌 progress style에서 progress bar의 border

//layout
	bool			m_thumb_hide = false;				//thumb을 감출 것인지 여부. default = false
	bool			m_forced_gray = false;				//강제로 gray로 표시할 것인지 여부. default = false
	bool			m_forced_gray_include_back = true;	//강제로 gray로 표시할 경우 배경까지도 강제 gray로 표시할 것인지. default = true;

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CSCSliderCtrl)
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CSCSliderCtrl();

	// Generated message map functions
protected:
	//{{AFX_MSG(CSCSliderCtrl)
	afx_msg void OnPaint();
	afx_msg BOOL OnEraseBkgnd(CDC* pDC);
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
	afx_msg void OnMouseLeave();
	afx_msg void OnSetFocus(CWnd* pOldWnd);
	afx_msg void OnKillFocus(CWnd* pNewWnd);
	//}}AFX_MSG

	DECLARE_MESSAGE_MAP()
public:
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	afx_msg void OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags);
	afx_msg void OnTimer(UINT_PTR nIDEvent);
	afx_msg void OnSize(UINT nType, int cx, int cy);
	virtual void PreSubclassWindow();
	afx_msg void OnWindowPosChanged(WINDOWPOS* lpwndpos);
	afx_msg BOOL OnMouseWheel(UINT nFlags, short zDelta, CPoint pt);
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_SCSLIDERCTRL_H__70340BD3_CE86_4EE4_A00B_5A4EC2B6A92D__INCLUDED_)
