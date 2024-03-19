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

#include "../../GdiplusBitmap.h"

#define	SCSLIDERCTRL_STYLE_SLIDER		0
#define SCSLIDERCTRL_STYLE_PROGRESS		1
#define SCSLIDERCTRL_STYLE_VOLUME		2

#define MESSAGE_SCSLIDERCTRL			WM_USER + 9424

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
//scpark 2019 08 23 10
//결국 timer 방식으로 변경하고 timer 방식의 문제점이었던 재생 중 클릭시 트랙이동이 안되던 문제는
//PostMessage를 SendMessage로 변경하고
//msg를 세분화해서 grab, move, release에 따라 동작 방식을 달리 구현하여 해결되었다.
//우선 이렇게 timer 방식으로 계속 사용해보고 문제가 없다면 코드를 다시 정리하자.

/////////////////////////////////////////////////////////////////////////////
// CSCSliderCtrl window
class CSCSliderCtrlMsg
{
public:
	CSCSliderCtrlMsg(int _msg, int _ctrl_id, int _pos)
	{
		ctrl_id = _ctrl_id;
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

	int		msg;
	int		ctrl_id;
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
	CGdiplusBitmap m_img;
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
		slider_normal = 0,
		slider_thumb,
		slider_value,
		slider_progress,
		slider_track,
		slider_step,		//진행 단계를 표시. 1-2-3-4 과 같은 형태. 
	};

	enum SCSliderCtrlValueStyle
	{
		none = 0,
		value,
		percentage,
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

	int		GetStyle() { return m_style; }

	//slider_thumb,	slider_value, slider_progress, slider_track, slider_step, 
	void	SetStyle(int nStyle);
	void	SetTrackHeight(int height) { m_nTrackHeight = height; }
	void	SetPos(int nPos);
	int		GetPos();
	void	EnableSlide(bool enable = true) { m_enable_slide = enable; }
	//void	set_enable_bottom_slide(bool enable) { m_enable_bottom_slide = enable; }
	int		GetLower() { return m_lower; }
	int		GetUpper() { return m_upper; }
	void	SetRange(int lower, int upper, BOOL bRedraw = FALSE);
	BOOL	SetBitmapChannel(UINT nChannelID, UINT nActiveID = NULL);
	BOOL	SetBitmapThumb(	UINT nThumbID, UINT nActiveID = NULL, BOOL bTrans = FALSE, COLORREF crTrans = RGB(0,0,0));
	void	DrawFocusRect(BOOL bDraw = TRUE, BOOL bRedraw = FALSE);
	void	SetValueStyle(int nValueStyle = value, COLORREF crText = RGB(64, 64, 64)) { m_nValueStyle = nValueStyle; m_crValueText = crText; }

	void	SetBackColor(COLORREF crBack);
	void	SetActiveColor(COLORREF crActive);
	void	SetInActiveColor(COLORREF crInActive);
	void	SetThumbColor(COLORREF crThumb);

	//현재 위치를 북마크에 추가한다. 만약 해당 위치가 이미 북마크라면 삭제한다.
	void	use_bookmark(bool use = true) { m_use_bookmark = use; }
	void	set_bookmark_color(COLORREF cr);
	void	set_bookmark_current_color(COLORREF cr);
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
	void	use_tooltip(bool use = true) { m_use_tooltip = use; }
	void	set_tooltip_format(int format = tooltip_value) { m_tooltip_format = format; }

//구간 반복 관련(slider_track style일 경우)
	int		m_repeat_start;
	int		m_repeat_end;
	//둘 다 -1이면 반복 해제.
	//하나만 -1이면 시작 또는 끝은 미디어의 시작 또는 끝으로
	void	set_repeat_range(int start, int end);

	//pos가 0이상이면 그 트랙 위치를 start 또는 end로 설정하고
	//-2이면 현재 위치를, -1면 해제의 의미로 처리한다.
	void	set_repeat_start(int pos = -2);
	//pos가 0이상이면 그 트랙 위치를 start 또는 end로 설정하고
	//-2이면 현재 위치를, -1면 해제의 의미로 처리한다.
	void	set_repeat_end(int pos = -2);



protected:
	// Attributes

	//slider_thumb,	slider_value, slider_progress, slider_track,
	int			m_style;

	int			m_nEventMsgStyle;

	int			m_lower;
	int			m_upper;
	int			m_pos;

	CRect		m_rc;

	CToolTipCtrl	m_tooltip;
	bool		m_use_tooltip;	//default = false
	int			m_tooltip_format;	//default = tooltip_value


	int			m_nChannelWidth, m_nChannelHeight;
	int			m_nThumbWidth, m_nThumbHeight;
	int			m_nMarginLeft, m_nMarginRight, m_nMarginTop, m_nMarginBottom;
	int			m_nMouseOffset;

	BOOL		m_is_vertical;
	bool		m_enable_slide;	//enable move the current pos by click or drag even though progress style. default = true


	//특정 위치들을 기억해두자. 북마크처럼.
	bool		m_use_bookmark;	//default = false;
	std::deque<CSCSliderCtrlBookmark> m_bookmark;
	//pos의 위치에 있는 북마크의 인덱스를 리턴한다.
	int			find_index_bookmark(int pos);
	//mouse move, sliding할 때 북마크 근처에 가면 이 값이 세팅된다. 근처가 아니면 -1.
	int			m_cur_bookmark;
	//마우스 위치에서 가장 가까운 북마크를 찾는데 그 허용 오차를 미리 구해놓는다.
	int			m_bookmark_near_tolerance;
	//현재 위치에서 가장 가까운 이전/다음 북마크 위치를 찾는다.
	int			get_near_bookmark(int pos, bool forward);


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
	BOOL		m_bChannelActive, m_bThumbActive;
	BOOL		m_bTransparentChannel, m_bTransparentThumb, m_bThumb, m_bChannel;
	BOOL		m_bLButtonDown, m_bHasFocus, m_bDrawFocusRect;

	int			m_nValueStyle;	//only applies to SCSLIDERCTRL_STYLE_PROGRESS. default = value;
	COLORREF	m_crValueText;	//text color of value. background is transparent. default = RGB(64,64,64);

	CBitmap		m_bmChannel, m_bmChannelMask, m_bmChannelActive, m_bmChannelActiveMask;
	CBitmap		m_bmThumb, m_bmThumbMask, m_bmThumbActive, m_bmThumbActiveMask, m_bmThumbBg;


	CDC			m_dcBk;
	CBitmap		m_bmpBk;
	CBitmap     *m_bmpBkOld;

	COLORREF	m_crBack;		// back color of control
	COLORREF	m_crActive;			//processed area
	COLORREF	m_crInActive;		//not processed area
	int			m_nTrackHeight;		//processing area height. odd recommend between 3 ~ 11.
	CPen		m_penThumb;
	CPen		m_penThumbLight;
	CPen		m_penThumbLighter;
	CPen		m_penThumbDark;
	CPen		m_penThumbDarker;
	COLORREF	m_crThumb;
	COLORREF	m_crThumbLight;
	COLORREF	m_crThumbLighter;
	COLORREF	m_crThumbDark;
	COLORREF	m_crThumbDarker;
	COLORREF	m_crBookmark;
	COLORREF	m_crBookmarkCurrent;
	//컨트롤의 enable, disable 상태에 따라 그려지는 색상이 달라지므로 사용
	COLORREF	enable_color(COLORREF cr, int offset = 0);

	CWnd*		m_pParentWnd;
	void		(*m_pCallback_func)(CWnd* pParent, CWnd* pWnd, DWORD msg, UINT pos);

	double		Pixel2Pos(int nPixel);
	int			Pos2Pixel(int nPos);
	void		PrepareMask(CBitmap *pBmpSource, CBitmap *pBmpMask,
							 COLORREF clrpTransColor);
	void		DrawTransparentBitmap(	CDC *pDC, int xStart, int yStart, int wWidth, int wHeight,
										CDC *pTmpDC, int xSource, int ySource, CBitmap *bmMask);

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
	afx_msg void OnCustomDraw(NMHDR* pNMHDR, LRESULT* pResult);
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
