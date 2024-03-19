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

#include "../../GdiplusBitmap.h"

#define	SCSLIDERCTRL_STYLE_SLIDER		0
#define SCSLIDERCTRL_STYLE_PROGRESS		1
#define SCSLIDERCTRL_STYLE_VOLUME		2

#define MESSAGE_SCSLIDERCTRL			WM_USER + 9424

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

//slider_step ����
	std::vector<CSCSliderCtrlSteps> m_steps;
//slider_step ����
	//step �̹����� ���� �̹������� �����Ѵ�.
	//void	set_step_images();
	//pos ��ġ�� resource id �̹����� ǥ���Ѵ�.
	void	set_step_image(int pos, int id);


	enum SCSliderCtrlStyle
	{
		slider_normal = 0,
		slider_thumb,
		slider_value,
		slider_progress,
		slider_track,
		slider_step,		//���� �ܰ踦 ǥ��. 1-2-3-4 �� ���� ����. 
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
	}	//�ݹ��Լ� �� ������� �̺�Ʈ�� ������ ��� ���.

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

	//���� ��ġ�� �ϸ�ũ�� �߰��Ѵ�. ���� �ش� ��ġ�� �̹� �ϸ�ũ��� �����Ѵ�.
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

//���� �ݺ� ����(slider_track style�� ���)
	int		m_repeat_start;
	int		m_repeat_end;
	//�� �� -1�̸� �ݺ� ����.
	//�ϳ��� -1�̸� ���� �Ǵ� ���� �̵���� ���� �Ǵ� ������
	void	set_repeat_range(int start, int end);

	//pos�� 0�̻��̸� �� Ʈ�� ��ġ�� start �Ǵ� end�� �����ϰ�
	//-2�̸� ���� ��ġ��, -1�� ������ �ǹ̷� ó���Ѵ�.
	void	set_repeat_start(int pos = -2);
	//pos�� 0�̻��̸� �� Ʈ�� ��ġ�� start �Ǵ� end�� �����ϰ�
	//-2�̸� ���� ��ġ��, -1�� ������ �ǹ̷� ó���Ѵ�.
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


	//Ư�� ��ġ���� ����ص���. �ϸ�ũó��.
	bool		m_use_bookmark;	//default = false;
	std::deque<CSCSliderCtrlBookmark> m_bookmark;
	//pos�� ��ġ�� �ִ� �ϸ�ũ�� �ε����� �����Ѵ�.
	int			find_index_bookmark(int pos);
	//mouse move, sliding�� �� �ϸ�ũ ��ó�� ���� �� ���� ���õȴ�. ��ó�� �ƴϸ� -1.
	int			m_cur_bookmark;
	//���콺 ��ġ���� ���� ����� �ϸ�ũ�� ã�µ� �� ��� ������ �̸� ���س��´�.
	int			m_bookmark_near_tolerance;
	//���� ��ġ���� ���� ����� ����/���� �ϸ�ũ ��ġ�� ã�´�.
	int			get_near_bookmark(int pos, bool forward);


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
	//��Ʈ���� enable, disable ���¿� ���� �׷����� ������ �޶����Ƿ� ���
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
