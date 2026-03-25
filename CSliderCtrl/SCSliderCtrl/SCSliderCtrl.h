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

	//slider_thumb,	slider_value, slider_progress, slider_track, slider_step, 
	void	set_style(int nStyle);
	void	set_track_height(int height) { m_track_height = height; }

	void	set_use_slide(bool enable = true) { m_use_slide = enable; }
	//void	set_enable_bottom_slide(bool enable) { m_enable_bottom_slide = enable; }
	void	DrawFocusRect(BOOL bDraw = TRUE, BOOL bRedraw = FALSE);

	//set_track_color()는 active 영역과 inactive 영역을 지정하므로
	void	set_track_color(Gdiplus::Color cr_active, Gdiplus::Color cr_inactive) { m_cr_active = cr_active; m_cr_inactive = cr_inactive; }
	void	set_active_color(Gdiplus::Color cr_active);
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
	//freq는 CSliderCtrl::SetTicFreq()와 동일하게 lower ~ upper 사이의 구간을 몇 등분할 것인지가 아닌 간격을 의미한다.
	//즉, freq = 23이면 23 등분이 아니라 0 ~ 23 ~ 46 ~ 69 ~ 92와 같이 틱이 표시된다.
	void	set_tic_freq(int freq, bool show_text = false);

	//현재 위치를 북마크에 추가한다. 만약 해당 위치가 이미 북마크라면 삭제한다.
	void	use_bookmark(bool use = true) { m_use_bookmark = use; }
	void	set_bookmark_color(Gdiplus::Color cr);
	void	set_bookmark_current_color(Gdiplus::Color cr);
	enum BOOKMARK
	{
		bookmark_add_current = 0,
		bookmark_add,
		bookmark_delete_current,
		bookmark_delete,
		bookmark_move,				//pos < 0 ? go to previous bookmark : go to next bookmark
		bookmark_reset,
	};
	void	bookmark(int mode = bookmark_add_current, int pos = -1, CString name = _T(""));
	std::deque<CSCSliderCtrlBookmark> get_bookmark_list() { return m_bookmark; }

	enum TOOLTIP_FORMAT
	{
		tooltip_value = 0,
		tooltip_time,
		tooltip_time_ms,
	};
	void			use_tooltip(bool use = true) { m_use_tooltip = use; }
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

	//disable이면 gray로 표시되지만 때로는 disable이 아니어도 gray로 표시해야 하는 경우도 있다.
	void			set_forced_gray(bool forced_gray, bool include_back = true) { m_forced_gray = forced_gray; m_forced_gray_include_back = include_back; Invalidate(); }

//layout
	//slider의 thumb는 거의 대부분 표시되는 상태지만 간혹 어떤 조건에 따라 pos는 그대로지만 thumb를 감출 필요가 있을 경우도 있다.
	//ex. zoom slider인 경우 80% 위치로 수동 설정했으나 auto fit을 클릭하면 화면 크기에 따라 그 값이 달라지는데
	//그 배율을 slider의 pos로 설정할 수도 있지만 auto fit을 토글하여 수동 설정된 배율과 auto fit을 토글할 수도 있다.
	void			hide_thumb(bool hide = true) { m_thumb_hide = hide; Invalidate(); }

protected:
	// Attributes

	//slider_thumb,	slider_value, slider_progress, slider_track,
	int				m_style = style_normal;

	int				m_nEventMsgStyle;

	CRect			m_rc;

	CToolTipCtrl	m_tooltip;
	bool			m_use_tooltip = false;	//default = false
	int				m_tooltip_format = tooltip_value;	//default = tooltip_value


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
	std::deque<CSCSliderCtrlBookmark> m_bookmark;
	//pos의 위치에 있는 북마크의 인덱스를 리턴한다.
	int				find_index_bookmark(int pos);
	//mouse move, sliding할 때 북마크 근처에 가면 이 값이 세팅된다. 근처가 아니면 -1.
	int				m_cur_bookmark = -1;
	//마우스 위치에서 가장 가까운 북마크를 찾는데 그 허용 오차를 미리 구해놓는다.
	int				m_bookmark_near_tolerance;
	//현재 위치에서 가장 가까운 이전/다음 북마크 위치를 찾는다.
	int				get_near_bookmark(int pos, bool forward);


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

	CString			m_text;			//m_text_style == text_style_user_defined일 경우 표시되는 텍스트
	CString			m_text_dual;	//m_text_style == text_style_dual_text일 경우 오른쪽에 표시되는 텍스트
	int				m_text_style = text_style_value;
	//COLORREF		m_cr_text = RGB(192, 192, 192);

	bool			m_transparent = false;
	//COLORREF		m_cr_back;				// back color of control
	Gdiplus::Color	m_cr_active;			//processed area
	Gdiplus::Color	m_cr_inactive;			//not processed area
	int				m_track_height = 14;	//rc.CenterPoint().y +- m_track_height / 2. ex) 6 and 7 is equal height
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

	//컨트롤의 enable, disable 상태에 따라 그려지는 색상이 달라지므로 사용
	Gdiplus::Color	enable_color(Gdiplus::Color cr, int offset = 64);

	CWnd*			m_pParentWnd;
	void			(*m_pCallback_func)(CWnd* pParent, CWnd* pWnd, DWORD msg, UINT pos) = NULL;

	int				Pixel2Pos(int pixel);
	int				Pos2Pixel(int pos);
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
	afx_msg void OnSetFocus(CWnd* pOldWnd);
	afx_msg void OnKillFocus(CWnd* pNewWnd);
	//}}AFX_MSG

	DECLARE_MESSAGE_MAP()
public:
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	//afx_msg void OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags);
	afx_msg void OnTimer(UINT_PTR nIDEvent);
	afx_msg void OnSize(UINT nType, int cx, int cy);
	virtual void PreSubclassWindow();
	afx_msg void OnWindowPosChanged(WINDOWPOS* lpwndpos);
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_SCSLIDERCTRL_H__70340BD3_CE86_4EE4_A00B_5A4EC2B6A92D__INCLUDED_)
