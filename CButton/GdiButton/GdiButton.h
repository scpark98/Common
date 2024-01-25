#pragma once

#include <afxwin.h>
#include <afxcmn.h>
#include <deque>
#include <gdiplus.h>

#include "../../GdiplusBitmap.h"

using namespace Gdiplus;

/*
- gdi+�� �̿��� ��ư Ŭ����
- ���� png �̹����� ����� ��� �̹��� ������� ��ư�� ǥ�õ�.
- ��� ����� ���� �Ϲ� push button, toggle button(as a checkbox) ������ ����� �� ����.

[usage]
* Gdiplus ����� ���� ����
* GdiplusBitmap.cpp�� Dummy Ŭ�������� �ڵ����� �ʱ�ȭ �� �����ϹǷ� ���� ���� ���ʿ�.
* ���� ���� ��� :
- stdafx.h
	#include <gdiplus.h>
	using namespace Gdiplus;

- App.h
	ULONG_PTR	m_gdiplusToken;
	and add ExitInstance() virtual function using class wizard.

- in App.cpp

	in InitInstance() function, add below lines before calling main dlg.

		Gdiplus::GdiplusStartupInput gdiplusStartupInput;
		Gdiplus::GdiplusStartup(&m_gdiplusToken, &gdiplusStartupInput, NULL);


	in ExitInstance() function, add below.
	
	Gdiplus::GdiplusShutdown(m_gdiplusToken);



* add_image(...)�� �� ��ư�� ���� 4���� �����̹���, �� normal, over, down, disabled�� ������ �� ����ϰ�
  add_images(...)�� �� ��ư�� ���� �̹������� �߰��� ���� active_index()�� �̿��Ͽ� ���ϴ� �̹����� ǥ���ϴ� �뵵�� ����� �� �ִ�
  (on/off, checked/unchecked, play/pause, img0/img1/img2...)

* set_auto_repeat()���� �ݺ� ����Ǵ� ��ư���� ���� ����.
* 
*/

//��ư �ڵ� ���� �����̸� Ư�� ��ǥ�� ������ �ƴ�
//��ü ��� �̹���(Ŭ���̾�Ʈ ����)�� �������� �� �����̴�.
#define ANCHOR_NONE			0x00000000
#define ANCHOR_LEFT			0x00000001
#define ANCHOR_TOP			0x00000010
#define ANCHOR_RIGHT		0x00000100
#define ANCHOR_BOTTOM		0x00001000
#define ANCHOR_HCENTER		0x00010000
#define ANCHOR_VCENTER		0x00100000
#define ANCHOR_CENTER		ANCHOR_HCENTER | ANCHOR_VCENTER

class CButtonImage
{
public:
	CButtonImage() {};

	CGdiplusBitmap normal;
	CGdiplusBitmap over;
	CGdiplusBitmap down;
	CGdiplusBitmap disabled;
};

// CGdiButton

class CGdiButton : public CButton, CGdiplusBitmap
{
	DECLARE_DYNAMIC(CGdiButton)

public:
	CGdiButton();
	virtual ~CGdiButton();

	//���� CButton::SetButtonStyle �Լ��� overriding�Ͽ� OWNER_DRAW�� �߰�������� �Ѵ�.
	void		SetButtonStyle(UINT nStyle, BOOL bRedraw = 1);
	//enum 

	//pBack�� ��ư�� ����� �Ǵ� �׸����� parent â�� ����̹����� ���� Bitmap*�̴�.
	//�� ��濡�� ��ư�� ����� �����Ͽ� DrawItem���� ���+���� �̹����� �׷��ְ� �ȴ�.
	//��, parentâ���� ��� �׸��� �ݵ�� ���� ũ��� ������� �׷����� �Ѵ�.
	//��ư�� ��ġ ��ǥ�� �ش��ϴ� ��� �׸��� �߶�ͼ� ����ϹǷ� ��浵 1:1�� �ѷ����� �Ѵ�.
	
	//GetParent()->UpdateWindow()�� �̿��Ͽ� �ѷ��ִ� ����� ��� �����͸� ������� �ʾƵ� �����ϰ� �ѷ����� ������ ������
	//UpdateWindow�� ���� �̹����� �����ϴ� �̺�Ʈ�� �߻��ϸ� �����̴� ������ �����Ѵ�.

	//add_images�� �ϳ��� ��ư�� �������� �̹����� �߰��� �� ����Ѵ�.
	//��, �̹��� ������ŭ add_image�� ȣ���Ѵ�.
	//Ư�� push, check, radio button ó�� checked, unchecked ���� ���� image�� ������ ������ �� ����� �� �ְ�
	//�ϳ��� ��ư�� �������� �̹����� �������� �� �ʿ䰡 ���� ��쿡�� ���ȴ�.
	//on/off, play/pause, img0/img1/img2...
	template <typename ... Types> void add_images(CString type, Types... args)
	{
		int n = sizeof...(args);
		int arg[] = { args... };

		for (auto id : arg)
			add_image(type, id);
	}

	//�ϳ��� ��ư�� ���� normal, over, down, disabled �̹������� ���� ������ �� ���ȴ�.
	//UINT�� 0�̸� �ڵ� �������ش�.
	//Ÿ���� ������ �⺻ _T("PNG")�� ó���Ѵ�.
	bool		add_image(CString type, UINT normal, UINT over = 0, UINT down = 0, UINT disabled = 0);
	bool		add_image(UINT normal, UINT over = 0, UINT down = 0, UINT disabled = 0);
	bool		add_image(CString normal, CString over = _T(""), CString down = _T(""), CString disabled = _T(""));
	bool		add_image(CGdiplusBitmap *img);
	bool		add_image(Gdiplus::Bitmap *img);
	void		use_normal_image_on_disabled(bool use = true);

	//fit = true�̸� ��Ʈ���� ũ�⸦ �̹��� ũ��� resize�Ѵ�. false�̸� ��Ʈ���� ũ�⿡ �°� �̹����� �׷��ش�.
	void		fit_to_image(bool fit = true);

	//��Ȥ ����PNG�� ��� parent���� � ����� �׸���Ŀ� ���� ��� ������ ����� �ȵɶ��� �ִ�.
	void		active_index(int index, bool bErase = false);
	int			active_index() { return m_idx; }

	void		release_all();

	void		set_alpha(float alpha);
	void		add_rgb(int red, int green, int blue, COLORREF crExcept);

	//void		set_back_imageBitmap* pBack);		//����� ����, ������ ��� ���
	CGdiButton& text(CString text);
	CGdiButton& text_color(COLORREF normal, COLORREF hover, COLORREF down, COLORREF disabled);
	CGdiButton& text_color(COLORREF normal);
	CGdiButton& back_color(COLORREF normal, COLORREF hover, COLORREF down, COLORREF disabled);
	//����png�� ����� �� �ʿ䰡 ������ ��Ȥ ����� refresh�� ����� �������� �ʾƼ� �ʿ��� ��쵵 �����Ѵ�.
	//(NH������Ʈ���� ���ȣ ������ �ۼ��� CBaseDialog�� ��ӹ��� CDialog ����)
	//auto_set_color�� true�� �ָ� over, down�϶��� ������ �ڵ����� �������ش�.
	CGdiButton& back_color(COLORREF normal, bool auto_set_color = false);
	CGdiButton& text_color() { m_cr_text.clear(); }
	CGdiButton& back_color() { m_cr_back.clear(); }
	//reassign [0,0] [1,1] [2,2]
	//hover, down�� ��� ���� ��ȭ�� �ְ��� �� ��� ���.(fScale�� 1.0���� ũ���ָ� ���, �۰��ָ� ��Ӱ� ����ȴ�.
	CGdiButton& set_hover_color_matrix(float fScale);	//1.0f = no effect.
	CGdiButton& set_down_color_matrix(float fScale);	//1.0f = no effect.

	//n��° �̹����� m��° ���� �̹����� x, y �ȼ� �÷��� �����Ѵ�. ��, disable�� ���ܵȴ�.
	void		replace_color(int index, int state_index, int x, int y, Gdiplus::Color newColor);

	//hue : -180 ~ 180, sat : -100 ~ 100, light : -100 ~ 100
	void		apply_effect_hsl(int hue, int sat = 0, int light = 0);


	virtual	CGdiButton&		set_font_name(LPCTSTR sFontname, BYTE byCharSet = DEFAULT_CHARSET);
	virtual CGdiButton&		set_font_size( int nSize );
	virtual CGdiButton&		set_font_bold( bool bBold = true );

	void		update_surface(bool bErase = false);

	bool		GetCheck();
	void		SetCheck( bool bCkeck );
	void		Toggle();

	//m_bAsStatic�� true�� ��� hover�� down���� ������ ���� �ʴ´�.
	void		SetAsStatic( bool bAsStatic = true ) { m_bAsStatic = bAsStatic; }


	//��ư�� ũ��, ��ġ ����
	int			width();
	int			height();
	void		SetAnchor( UINT nAnchor ) { m_nAnchor = nAnchor; }	//���� ��� ����
	void		SetAnchorMargin( int x, int y ) { m_nAnchorMarginX = x; m_nAnchorMarginY = y; }
	void		ReAlign();	//parent�� ũ�Ⱑ ���ϸ� ������ align���� ���� ��ġ�� ���������ش�.
	void		Offset( int x, int y );
	void		Inflate( int cx, int cy );
	void		Inflate( int l, int t, int r, int b );
	virtual CGdiButton& set_round(int round);

	//��Ŀ�� �簢�� ����
	void		ShowFocusRect( bool bShow = true ) { m_bShowFocusRect = bShow; Invalidate(); }
	void		SetFocusRectColor( COLORREF crFocus ) { m_crFocusRect = crFocus; Invalidate(); }
	void		SetFocusRectWidth( int nWidth ) { m_nFocusRectWidth = nWidth; Invalidate(); }

	void		use_hover(bool use);
	void		set_hover_rect(int thick = 2, COLORREF cr = RGB(128, 128, 255));
	void		set_hover_rect_thick(int thick);
	void		set_hover_rect_color(COLORREF cr);
	void		down_offset(CPoint offset) { m_down_offset = offset; Invalidate(); }
	void		down_offset(int offset) { m_down_offset = CPoint(offset, offset); Invalidate(); }

	//3D, sunken 
	void		use_3D_rect(bool use) { m_b3DRect = use; Invalidate(); }

	//blink
	void		set_blink_time( int nTime0 = 400, int nTime1 = 1200 );	//nTime0:hidden, nTime1:shown
	void		set_blink( BOOL bBlink = TRUE );

	void		use_tooltip(bool use) { m_use_tooltip = use; }
	//disabled�� ��Ʈ���� main�� PreTranslateMessage()���� ó������ ������ ��Ÿ���� �ʴ´�.
	//���� tooltip�� ������ �� main���� ó���ϵ��� �Ѵ�.
	void		set_tooltip_text(CString text);

	//auto repeat
	void		set_auto_repeat(bool use = true);
	void		set_auto_repeat_delay(int initial_delay = 1, int repeat_delay = 500);

	//public���� �Ͽ� CGdiplusBitmap�� effect���� �Լ����� ����� �� �ֵ��� ��.
	std::deque<CButtonImage*> m_image;

protected:
	BOOL	RegisterWindowClass();

	enum TIMER_ID
	{
		timer_blink = 0,
		timer_auto_repeat,
	};

	UINT		m_button_style;				//pushbutton(default) or checkbox or radiobutton

	int			m_idx = 0;					//���� ǥ���� m_image�� �ε��� (checkbox�� radio�� �̼���=0, ����=1)
	bool		m_fit2image = true;			//true : �̹��� ũ���� ��Ʈ�� ũ�� ����, false : ���� ��Ʈ�� ũ��� �̹��� ǥ��
	CRect		m_rOrigin = 0;				//��Ʈ���� ���� ũ�� ����

	//����� �ܻ��� �ƴ� �׸��̰� ���� PNG�� �׸��� ���, resize���� �� ���� true�� �Ѵ�.
	//��, �� ��� ���� �ϼ��� ����� �ƴ϶� �ణ �����̴� ������ �ִ�.
	bool		m_transparent = true;

	CGdiplusBitmap	m_back;					//��ư�� ��� �̹���, NULL�̸� m_crBack�� ����
	CGdiplusBitmap	m_back_origin;

	CString		m_text = _T("");
	std::deque <COLORREF>	m_cr_text;
	std::deque <COLORREF>	m_cr_back;		//���� PNG�� ������ �����ߴٸ� ����� �׷�����.

	int			m_width = 0;
	int			m_height = 0;

	int			m_round = 0;				//round rect

	UINT		m_nAnchor = 0;
	int			m_nAnchorMarginX = 0;
	int			m_nAnchorMarginY = 0;

	bool		m_bAsStatic = false;		//�ܼ� �̹��� ǥ�� �뵵�� ���ǰ� Ŭ���ص� ��ȭ�� ����. �⺻�� false.
	bool		m_use_hover = true;			//default = true;
	bool		m_hover_rect = false;		//hover �׵θ� �簢�� ǥ�� ����
	int			m_hover_rect_thick = 2;
	COLORREF	m_hover_rect_color = RGB(128, 128, 255);
	bool		m_bHover = false;
	bool		m_bIsTracking = false;
	bool		m_bPushed = false;
	bool		m_bHasFocus = false;
	bool		m_bShowFocusRect;		//��Ŀ�� �簢�� ǥ�� ����(�⺻�� false)
	COLORREF	m_crFocusRect;			//����
	int			m_nFocusRectWidth;		//�β�
	bool		m_b3DRect;				//��ü ������ 3D, ������ sunken. default = true;
	CPoint		m_down_offset;			//������ �� �׷��� ��ġ(�⺻��=1);
	bool		m_use_normal_image_on_disabled = false;	//disabled�� �⺻ ȸ������ �ڵ� ���������� �׷��� ���� �ʴ� ��쵵 ���� �� �ִ�.

	bool		m_blink = false;
	bool		m_blink_status = false;
	int			m_blink_time0 = 400;	//blink type is Show/Hide, time0 = shown duration, time1 = hidden duration in millisecond.
	int			m_blink_time1 = 1200;

	ColorMatrix m_grayMatrix;
	ColorMatrix m_hoverMatrix;			//hover�̹����� ������ ��Ʈ����
	ColorMatrix m_downMatrix;			//down�̹����� ������ ��Ʈ����

	void		resize_control(int cx, int cy);

	LOGFONT		m_lf;
	CFont		m_font;

	void		reconstruct_font();

	//enable�����϶��� �� ǥ�õǳ� disable�϶��� ǥ�õ��� �ʴ´�.
	//�̸� �ذ��Ϸ��� parent�� PreTranslateMessage()���� ó���ؾ� �Ѵ�.
	CToolTipCtrl	m_tooltip;
	bool		m_use_tooltip = true;
	CString		m_tooltip_text = _T("");


	//auto repeat
	int			m_initial_delay = 1;
	int			m_repeat_delay = 500;
	bool		m_use_auto_repeat = false;
	int			m_sent_once_auto_repeat_click_message = 0;	//���� down�� initial_delay�� �Ǳ⵵ ���� up�ȴٸ� �̶����� �ѹ��� ���콺 Ŭ�� �̺�Ʈ�� ó������� �Ѵ�.

protected:
	DECLARE_MESSAGE_MAP()
	virtual void PreSubclassWindow();
public:
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	virtual void DrawItem(LPDRAWITEMSTRUCT /*lpDrawItemStruct*/);
	afx_msg BOOL OnEraseBkgnd(CDC* pDC);
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
	afx_msg void OnMouseHover(UINT nFlags, CPoint point);
	afx_msg void OnMouseLeave();
	afx_msg void OnTimer(UINT_PTR nIDEvent);
	afx_msg void OnSetFocus(CWnd* pOldWnd);
	afx_msg void OnKillFocus(CWnd* pNewWnd);
	afx_msg BOOL OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message);
	virtual LRESULT WindowProc(UINT message, WPARAM wParam, LPARAM lParam);
	afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnWindowPosChanged(WINDOWPOS* lpwndpos);
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg BOOL OnToolTipNotify(UINT id, NMHDR* pNMHDR, LRESULT* pResult);
};


