#if !defined(AFX_SCSLIDERCTRL_H__70340BD3_CE86_4EE4_A00B_5A4EC2B6A92D__INCLUDED_)
#define AFX_SCSLIDERCTRL_H__70340BD3_CE86_4EE4_A00B_5A4EC2B6A92D__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// SCSliderCtrl.h : header file
//

#include <afxwin.h>
#include <Afxdisp.h>
#include <afxext.h>         // MFC Ȯ���Դϴ�.
#include <afxcmn.h>             // Windows ���� ��Ʈ�ѿ� ���� MFC �����Դϴ�.

#include <deque>
#include <algorithm>
#include <vector>

#include "../../colors.h"
#include "../../SCGdiplusBitmap.h"

/*
[����]
�̺�Ʈ �޽��� ó�� ��� ����!
�Ϲ����� CSliderCtrl�� parent���� WM_HSCROLL �̺�Ʈ�� ó��������
�� CSCSliderCtrl������ grab, release ���� ������ �̺�Ʈ ó���� �ʿ��ϹǷ� WM_HSCROLL�� �Ἥ�� �ȵȴ�.
�ݵ�� �Ʒ� �޽����� ���Ź޾� ó���� ��!
*/

static const UINT Message_CSCSliderCtrl = ::RegisterWindowMessage(_T("MessageString_CSCSliderCtrl"));

//Ʈ�� �̺�Ʈ�� �߻����� �� �ٷ� PostMessage�� ȣ���ϰ� �Ǹ�
//�̼��� ���콺 �����ӵ鿡 ���� ������ ���� �̺�Ʈ�� �߻��ϰ� �ǹǷ�
//�����̰� ���� �߻��ȴ�. Ÿ�̸Ӹ� �̿��Ͽ� �ſ� ª�� �ð��� �߻��ϴ� �ߺ��� �̺�Ʈ�� ��ŵ�ǵ��� �ϸ�
//Ʈ�� �̵��� �ſ� �ε巴�� ó���ȴ�.
//��, �̷� ��� message queue�� �޽����� ���̰ų� Ÿ�̸Ӹ� �����µ����� ���� ª�� �ð�����
//�ٸ� Ʈ�� �̵��� ���õ� �ڵ带 ���� ��� ���� ����� Ʈ������ �̵����� �ʴ� ������ �߻��Ѵ�.
//���� ��� ������ ����⸦ ������� ����߿� Ʈ���� ���� ��ġ�� Ŭ���ϴ� ����
//�ش� ��ġ�� �̵��ؾ��ϴµ� ��� ������ ���� ���ŵǴ� SetPos�� ����
//Ŭ���� ��ġ�� �̵��Ǿ��ٰ� �ٷ� �����ġ�� �ǵ��ư��� ������ �߻��Ѵ�.
//settimer����� ��� Ʈ�� �̵��� �ſ� �ε巯�� ������ ������ ���� ������ ũ�Ƿ�
//�ϴ� �ݹ������ε� ������ �������� PostMessage��İ� ������ ������ �����Ѵ�.
//
//scpark 2019 08 23 10
//�ᱹ timer ������� �����ϰ� timer ����� �������̾��� ��� �� Ŭ���� Ʈ���̵��� �ȵǴ� ������
//PostMessage�� SendMessage�� �����ϰ�
//msg�� ����ȭ�ؼ� grab, move, release�� ���� ���� ����� �޸� �����Ͽ� �ذ�Ǿ���.
//�켱 �̷��� timer ������� ��� ����غ��� ������ ���ٸ� �ڵ带 �ٽ� ��������.

/*
* scpark 20240501
* bitmap �̹������ slidectrl�� ǥ���ϴ� �ڵ忡�� ���۵����� ���� ������ �̹����� �ʿ��ϰ� ũ������ ������ ���� �ʴ�.
* �ֱ� ������ �̹����� �̿��� ��Ÿ���� ���� ������ �ʰ� gdiplus�� ����ϰ� �׷��ִ� ���� ����.
* �켱 �̹��� �������� �ڵ���� ����ε� ������ ������ ���� ������.
* 
* scpark 20240902
* WM_LBUTTONDOWN, WM_MOUSEMOVE�� ���� �ڵ鷯�� �� �������ٿ� ���� CSliderCtrl::OnLButtonDown(nFlags, point);�� ����
* �⺻ �ڵ鷯�� ȣ���ϴµ� �� �ڵ带 ����θ� Ŭ����, �巡�׽ÿ� WM_PAINT�� ȣ��Ǹ鼭 pos�� �޶����� ���ۿ��� �߻��ߴ�.
* �� �⺻ �ڵ鷯�� �ּ�ó���ϴ� ���ϴ� ���·� �� �����Ѵ�.
* CMacProgress������ �ּ�ó������ �ʾƵ� �� �����Ѵ�. ���� ���̰� �ִ� �� �ϴ�.
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

//slider_step ����
	std::vector<CSCSliderCtrlSteps> m_steps;
//slider_step ����
	//step �̹����� ���� �̹������� �����Ѵ�.
	//void	set_step_images();
	//pos ��ġ�� resource id �̹����� ǥ���Ѵ�.
	void	set_step_image(int pos, int id);

	enum SCSliderCtrlStyle
	{
		style_normal = 0,
		style_thumb,
		style_thumb_round,
		style_value,
		style_progress,
		style_progress_line,
		style_track,
		style_step,		//���� �ܰ踦 ǥ��. 1-2-3-4 �� ���� ����. 
		style_koino,
	};

	enum SCSliderCtrlTextStyle
	{
		text_style_none = 0,
		text_style_value,			//GetPos()�� ������ ǥ��
		text_style_percentage,		//percentage ������ ǥ��
		text_style_user_defined,	//�߾ӿ� ����ڰ� ���� ������ ������ ǥ��
		text_style_dual_text,		//���ʿ� ����ڰ� ���� ������ ������ ǥ��
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
	}	//�ݹ��Լ� �� ������� �̺�Ʈ�� ������ ��� ���.

	//int		GetPos();
	void	set_pos(int pos);
	void	SetPos(int pos) { set_pos(pos); }
	void	set_range(int lower, int upper) { SetRange(lower, upper); }
	int		get_lower();
	int		get_upper();
	void	set_step_completed() { m_step_completed = true; Invalidate(); }
	int		get_style() { return m_style; }

	int32_t	step(int step = 1);

	//void	SetPos(int pos) { CSliderCtrl::SetPos(pos); Invalidate(); /*redraw_window();*/ }
	void	redraw_window(bool bErase = false);

	//slider_thumb,	slider_value, slider_progress, slider_track, slider_step, 
	void	set_style(int nStyle);
	void	set_track_height(int height) { m_track_height = height; }

	void	set_use_slide(bool enable = true) { m_use_slide = enable; }
	//void	set_enable_bottom_slide(bool enable) { m_enable_bottom_slide = enable; }
	void	DrawFocusRect(BOOL bDraw = TRUE, BOOL bRedraw = FALSE);

	//set_track_color()�� active ������ inactive ������ �����ϹǷ�
	void	set_track_color(Gdiplus::Color cr_active, Gdiplus::Color cr_inactive) { m_cr_active = cr_active; m_cr_inactive = cr_inactive; }
	void	set_active_color(Gdiplus::Color cr_active);
	void	set_inactive_color(Gdiplus::Color cr_inactive);
	//set_back_color()�� ���� ���ʿ��ϴ�.
	//void	set_back_color(Gdiplus::Color cr_back);
	void	set_thumb_color(Gdiplus::Color cr_thumb);
	void	set_tic_color(Gdiplus::Color cr_tic);

	void	set_text_color(Gdiplus::Color cr_text) { m_theme.cr_text = cr_text; Invalidate(); }
	void	set_text_style(int text_style) { m_text_style = text_style; Invalidate(); }
	void	set_text(LPCTSTR text, ...);
	void	set_text_dual(LPCTSTR text_dual, ...);

	int		m_tic_freq = 0;
	bool	m_tic_show_text = false;	//default = false
	//freq�� CSliderCtrl::SetTicFreq()�� �����ϰ� lower ~ upper ������ ������ �� ����� �������� �ƴ� ������ �ǹ��Ѵ�.
	//��, freq = 23�̸� 23 ����� �ƴ϶� 0 ~ 23 ~ 46 ~ 69 ~ 92�� ���� ƽ�� ǥ�õȴ�.
	void	set_tic_freq(int freq, bool show_text = false);

	//���� ��ġ�� �ϸ�ũ�� �߰��Ѵ�. ���� �ش� ��ġ�� �̹� �ϸ�ũ��� �����Ѵ�.
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

//���� �ݺ� ����(slider_track style�� ���)
	int				m_repeat_start = -1;
	int				m_repeat_end = -1;
	//�� �� -1�̸� �ݺ� ����.
	//�ϳ��� -1�̸� ���� �Ǵ� ���� �̵���� ���� �Ǵ� ������
	void			set_repeat_range(int start, int end);

	//pos�� 0�̻��̸� �� Ʈ�� ��ġ�� start �Ǵ� end�� �����ϰ�
	//-2�̸� ���� ��ġ��, -1�� ������ �ǹ̷� ó���Ѵ�.
	void			set_repeat_start(int pos = -2);
	//pos�� 0�̻��̸� �� Ʈ�� ��ġ�� start �Ǵ� end�� �����ϰ�
	//-2�̸� ���� ��ġ��, -1�� ������ �ǹ̷� ó���Ѵ�.
	void			set_repeat_end(int pos = -2);

//��Ʈ ����
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

//color theme ����
	CSCColorTheme	m_theme = CSCColorTheme(this);	//m_theme(this); �� ����.
	void			set_color_theme(int theme);

	//disable�̸� gray�� ǥ�õ����� ���δ� disable�� �ƴϾ gray�� ǥ���ؾ� �ϴ� ��쵵 �ִ�.
	void			set_forced_gray(bool forced_gray, bool include_back = true) { m_forced_gray = forced_gray; m_forced_gray_include_back = include_back; Invalidate(); }

//layout
	//slider�� thumb�� ���� ��κ� ǥ�õǴ� �������� ��Ȥ � ���ǿ� ���� pos�� �״������ thumb�� ���� �ʿ䰡 ���� ��쵵 �ִ�.
	//ex. zoom slider�� ��� 80% ��ġ�� ���� ���������� auto fit�� Ŭ���ϸ� ȭ�� ũ�⿡ ���� �� ���� �޶����µ�
	//�� ������ slider�� pos�� ������ ���� ������ auto fit�� ����Ͽ� ���� ������ ������ auto fit�� ����� ���� �ִ�.
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


	//���� ��ü ���� ������ ����
	int				m_track_thick = 8;	//Ʈ�� �β�

	//��� �����̴� ����
	CSize			m_thumb;
	//4������ ����
	CRect			m_margin = CRect(0, 0, 0, 0);
	int				m_nMouseOffset;

	//����� ���θ�忡 ���� �ڵ尡 ���� �����Ǿ� ���� �ʴ�.
	bool			m_is_vertical = false;

	//enable move the current pos by click or drag even though progress style. default = true
	bool			m_use_slide = true;

	//�ٸ� �����̴��ʹ� �ٸ��� style_step�� ��� pos�� �������� ó�� - ���� ó������ ��ġ - ���� ó������ ���� ��ġ
	//�̷��� 3�ܰ�� �����ؼ� �׷��ִµ� pos�� upper step�� ó�� �Ϸ��� ��, �� ��� ������ ������ upper ���� ó���� ������ ǥ���ؾ� �Ѵ�.
	bool			m_step_completed = false;

	//Ư�� ��ġ���� ����ص���. �ϸ�ũó��.
	bool			m_use_bookmark = false;	//default = false;
	std::deque<CSCSliderCtrlBookmark> m_bookmark;
	//pos�� ��ġ�� �ִ� �ϸ�ũ�� �ε����� �����Ѵ�.
	int				find_index_bookmark(int pos);
	//mouse move, sliding�� �� �ϸ�ũ ��ó�� ���� �� ���� ���õȴ�. ��ó�� �ƴϸ� -1.
	int				m_cur_bookmark = -1;
	//���콺 ��ġ���� ���� ����� �ϸ�ũ�� ã�µ� �� ��� ������ �̸� ���س��´�.
	int				m_bookmark_near_tolerance;
	//���� ��ġ���� ���� ����� ����/���� �ϸ�ũ ��ġ�� ã�´�.
	int				get_near_bookmark(int pos, bool forward);


	/*
	//m_enable_bottom_slide�� slider_track ��Ÿ���� ��� �̵�� �÷��̾��� Ʈ���ٷ� ���� ���
	//��üȭ�鿡���� cy�� �Ʒ� �κ��� Ŭ���ؼ� Ʈ���̵��ص� ������
	//�Ϲ�ȭ�鿡���� �� �κ��� main dlg�� ũ������ ������ ��ġ�� �ǹǷ�
	//ũ�������� �ϰ� �; Ʈ���� �̵��Ǵ� �������� �ִ�.
	//���� �ɼ����� cy�� �Ʒ� �κ��� Ŭ���ؼ� slide�ϴ� ���� ���/�Ұ� ���·� ������ �� �ֵ��� �Ѵ�.
	//default = true;
	=> main dlg���� ���� WM_LBUTTONDOWN�� ����ä�� ũ������ �����̶�� �� �ڵ�� ó���ϰ�,
	�׷��� ���� ������ ��츸 �������� �Ѱܼ� ó���ϵ��� ���� �Ϸ�.
	bool		m_enable_bottom_slide;
	*/
	bool			m_lbuttondown = false;
	bool			m_has_focus = false;
	bool			m_draw_focus_rect = false;

	CString			m_text;			//m_text_style == text_style_user_defined�� ��� ǥ�õǴ� �ؽ�Ʈ
	CString			m_text_dual;	//m_text_style == text_style_dual_text�� ��� �����ʿ� ǥ�õǴ� �ؽ�Ʈ
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

	//��Ʈ���� enable, disable ���¿� ���� �׷����� ������ �޶����Ƿ� ���
	Gdiplus::Color	enable_color(Gdiplus::Color cr, int offset = 64);

	CWnd*			m_pParentWnd;
	void			(*m_pCallback_func)(CWnd* pParent, CWnd* pWnd, DWORD msg, UINT pos) = NULL;

	int				Pixel2Pos(int pixel);
	int				Pos2Pixel(int pos);
	void			PrepareMask(CBitmap *pBmpSource, CBitmap *pBmpMask, Gdiplus::Color clrpTransColor);
	void			DrawTransparentBitmap(	CDC *pDC, int xStart, int yStart, int wWidth, int wHeight,
											CDC *pTmpDC, int xSource, int ySource, CBitmap *bmMask);

	//��Ʈ ����
	LOGFONT			m_lf;
	CFont			m_font;
	int				m_font_size;
	void			reconstruct_font();

	//border
	Gdiplus::Color	m_cr_progress_border = Gdiplus::Color(188, 188, 188);
	bool			m_draw_progress_border = false;	//ctrl�� border�� �ƴ� progress style���� progress bar�� border

//layout
	bool			m_thumb_hide = false;				//thumb�� ���� ������ ����. default = false
	bool			m_forced_gray = false;				//������ gray�� ǥ���� ������ ����. default = false
	bool			m_forced_gray_include_back = true;	//������ gray�� ǥ���� ��� �������� ���� gray�� ǥ���� ������. default = true;

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
