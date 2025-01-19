#pragma once

#include <afxwin.h>
#include <afxcmn.h>
#include <deque>
#include <gdiplus.h>

#include "../../colors.h"
#include "../../GdiplusBitmap.h"

using namespace Gdiplus;

/*
- gdi+�� �̿��� ��ư Ŭ����
- ���� png �̹����� ����� ��� �̹��� ������� ��ư�� ǥ�õ�.
- ��� ����� ���� �Ϲ� push button, toggle button(as a checkbox) ������ ����� �� ����.
- 2024.08.23 COLORREF -> Gdiplus::Color �� ����

[�⺻�� ���� ����]
- png�� �ַ� ����ϱ� ���� Ŭ������ �������� ���۵Ǿ�����
  �̹����� ������� �ʰ� CButton�� �����ϰ� ǥ���Ͽ� ����ϴ� ��쵵 ����.
  ���� �⺻ ������ ������ �⺻������ �����Ǿ� �ְ�, m_transparent = false�� �����Ѵ�.
  �׷��߸� �̹����� ������� �ʴ� ��ư�鵵 ���ٸ� ���þ��� �Ϲ� ��ưó�� ǥ�õȴ�.

- png�� ����Ѵٸ� add_image()���� ����Լ��� �̿��ؼ� �����ϹǷ� �� ������ �� ����
  m_transparent�� �ڵ����� true�� ����ȴ�.
  ��, ���� png�� ������ ������� �ƴ� �ٸ� �����ε� ������ �� �����Ƿ�
  back_color()�� �̿��� ������ ĥ�� ��쿡�� m_transparent�� false�� ����ȴ�.
  ���� ������ ������ ������ ��쿡�� �ݵ�� add_image() �����Ŀ� �ؾ� �Ѵ�.

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

* �����Ǵ� ��ư ����
	[���� png ��ư]
	- 
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

static const UINT Message_CGdiButton = ::RegisterWindowMessage(_T("MessageString_CGdiButton"));

class CGdiButtonMessage
{
public:
	CGdiButtonMessage(CWnd* _this, UINT ctrl_id, int _message)
	{
		m_pWnd = _this;
		m_ctrl_id = ctrl_id;
		m_message = _message;
	}

	CWnd*		m_pWnd = NULL;
	UINT		m_ctrl_id;
	int			m_message;
};

class CGdiButtonImage
{
public:
	CGdiButtonImage() {};

	CGdiplusBitmap img[4];	//normal, over, down, disabled
};

class CGdiButtonParagragh
{
public:
	CGdiButtonParagragh();


};

// CGdiButton

class CGdiButton : public CButton//, CGdiplusBitmap
{
	DECLARE_DYNAMIC(CGdiButton)

public:
	CGdiButton();
	virtual ~CGdiButton();

	enum GdiButtonMessage
	{
	};

	//���� �����ÿ��� ���.
	BOOL		create(CString caption, DWORD dwStyle, CRect r, CWnd* parent, UINT button_id);

	//���� CButton::SetButtonStyle �Լ��� overriding�Ͽ� OWNER_DRAW�� �߰�������� �Ѵ�.
	void		SetButtonStyle(UINT nStyle, BOOL bRedraw = 1);
	
	//�⺻ PUSH_BUTTON, CHECKBOX, RADIOBUTTON�� ���� style���� �޸�
	//PUSH_BUTTON�ε� ǥ�� ����� ���� ���·� ǥ�õǴ� mobile ui�� �ַ� ���Ǵ� �׸� ǥ�ÿ����� �߰�(Tile UI)
	//ex. Visual Studio ���� �� "�������丮 ����", "������Ʈ �Ǵ� �ַ�� ����", "���� ���� ����", "�� ������Ʈ �����"
	//ex. ���� ���� �� �����ϴ� ������ ���� ��� "Copy and Replace", "Don't copy", "Copy, but keep both files"
	//�⺻ hoverȿ��, round corner, �Ӽ��� flat = true�� ������ ��
	enum BUTTON_TYPE
	{
		type_default = 0,
		type_paragraph,		//1st line : header img, title, 2nd line : comment, other lines : multiline body text
	};
	//1st line : header img, title, 2nd line : comment, other lines : multiline body text
	void		set_paragraph(int header_icon_id, CString title, CString comment, int main_icon_id = -1, CString body_text = _T(""))
	{

	}
	

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
	template <typename ... T> void add_images(CString type, T... args)
	{
		int n = sizeof...(args);
		int arg[] = { args... };

		for (auto id : arg)
			add_image(type, id);
	}

	//�ϳ��� ��ư�� ���� normal, over, down, disabled �̹������� ���� ������ �� ���ȴ�.
	//UINT�� 0�̸� �ڵ� �������ش�.
	//Ÿ���� ������ �⺻ _T("PNG")�� ó���Ѵ�.
	//������ ������ ������ ��쿡�� �ݵ�� add_image()�Ŀ� set_back_color()�� ȣ������� �Ѵ�.
	bool		add_image(CString type, UINT normal, UINT over = 0, UINT down = 0, UINT disabled = 0);
	bool		add_image(UINT normal, UINT over = 0, UINT down = 0, UINT disabled = 0);
	//�⺻ �̹����� ������ �� resize�� �� ����
	bool		add_image_resize(UINT normal, float ratio = 1.0f);
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
	//void		add_rgb(int red, int green, int blue, COLORREF crExcept);

	void		set_transparent(bool trans = true);

	//void		set_back_imageBitmap* pBack);		//����� ����, ������ ��� ���
	void		set_text_color(Gdiplus::Color normal, Gdiplus::Color hover, Gdiplus::Color down, Gdiplus::Color disabled);
	void		set_text_color(Gdiplus::Color normal);
	void		set_back_color(Gdiplus::Color normal, Gdiplus::Color hover, Gdiplus::Color down, Gdiplus::Color disabled);
	//����png�� ����� �� �ʿ䰡 ������ ��Ȥ ����� refresh�� ����� �������� �ʾƼ� �ʿ��� ��쵵 �����Ѵ�.
	//(NH ������Ʈ���� ���ȣ ������ �ۼ��� CBaseDialog�� ��ӹ��� CDialog ����)
	//auto_set_color�� true�� �ָ� over, down�϶��� ������ �ڵ����� �������ش�.
	void		set_back_color(Gdiplus::Color normal, bool auto_set_color = true);
	//CGdiButton& text_color() { m_cr_text.clear(); }
	//CGdiButton& back_color() { m_cr_back.clear(); }
	//reassign [0,0] [1,1] [2,2]
	//hover, down�� ��� ���� ��ȭ�� �ְ��� �� ��� ���.(fScale�� 1.0���� ũ���ָ� ���, �۰��ָ� ��Ӱ� ����ȴ�.
	void		set_hover_color_matrix(float fScale);	//1.0f = no effect.
	void		set_down_color_matrix(float fScale);	//1.0f = no effect.

	//n��° �̹����� m��° ���� �̹����� x, y �ȼ� �÷��� �����Ѵ�. ��, disable�� ���ܵȴ�.
	void		replace_color(int index, int state_index, int x, int y, Gdiplus::Color newColor);

	//��ư �̹����� ȿ���� ����. state_index�� -1�̸� ��� �����̹����� ȿ�� ����. 0�̸� normal image�� ����.
	//hue : -180 ~ 180, sat : -100 ~ 100, light : -100 ~ 100
	void		apply_effect_hsl(int state_index, int hue, int sat = 0, int light = 0);
	void		apply_effect_rgba(int state_index, float r, float g, float b, float a = 1.0);
	void		apply_effect_blur(int state_index, float radius, BOOL expandEdge);


	void		set_font_name(LPCTSTR sFontname, BYTE byCharSet = DEFAULT_CHARSET);
	void		set_font_size( int nSize );
	void		set_font_bold( bool bBold = true );

	//����� ������ ���� parent�� ����� Invalidate()����� �ϹǷ� �׳� Invalidate()�����δ� �ȵȴ�.
	void		redraw_window(bool bErase = false);

	bool		GetCheck();
	void		SetCheck( bool bCkeck );
	void		Toggle();

	//m_bAsStatic�� true�� ��� hover�� down���� ������ ���� �ʴ´�.
	void		SetAsStatic( bool bAsStatic = true ) { m_bAsStatic = bAsStatic; }


	//�̹��� �� ��ư�� ũ�⸦ �����Ѵ�.
	void		resize(int cx, int cy);

	//�̹����� ũ�⿡ �°� ��Ʈ���� resize�ϰ� dx, dy, nAnchor�� ���� move���ش�.(move�� ���� ����)
	void		resize_control(int cx, int cy);

	//�̹����� ������� �ʰ� ���� �׷��ִ� ��ư�� ��� width�� ��Ȯ�� ���ؾ��ϴ� ��찡 �ִ�.
	CRect		calc_rect();

	int			width();
	int			height();
	void		SetAnchor( UINT nAnchor ) { m_nAnchor = nAnchor; }	//���� ��� ����
	void		SetAnchorMargin( int x, int y ) { m_nAnchorMarginX = x; m_nAnchorMarginY = y; }
	void		ReAlign();	//parent�� ũ�Ⱑ ���ϸ� ������ align���� ���� ��ġ�� ���������ش�.
	void		Offset(int x, int y);
	void		Inflate(int cx, int cy);
	void		Inflate(int l, int t, int r, int b);
	void		set_round(int round);

	//��Ŀ�� �簢�� ����
	void		ShowFocusRect( bool bShow = true ) { m_draw_focus_rect = bShow; Invalidate(); }
	void		SetFocusRectColor(Gdiplus::Color crFocus ) { m_crFocusRect = crFocus; Invalidate(); }
	void		SetFocusRectWidth( int nWidth ) { m_nFocusRectWidth = nWidth; Invalidate(); }

	void		use_hover(bool use);
	void		draw_hover_rect(bool draw = true, Gdiplus::Color cr = Gdiplus::Color::DimGray, int thick = 1);
	void		set_hover_rect_thick(int thick);
	void		set_hover_rect_color(Gdiplus::Color cr);
	void		down_offset(CPoint offset) { m_down_offset = offset; Invalidate(); }
	void		down_offset(int offset) { m_down_offset = CPoint(offset, offset); Invalidate(); }

	//border
	void		draw_border(bool draw = true, Gdiplus::Color cr = Gdiplus::Color::DimGray, int thick = 1);

	//3D, sunken 
	void		use_3D_rect(bool use = true) { m_b3DRect = use; Invalidate(); }

	//���� ��ư�� ��� �׸��ڸ� ǥ���Ѵ�.
	//weight�� 1.0���� ũ�� ����, ������ ��ο� �׸��ڰ� �׷�����.
	//�̹��� ������ ���� shadow�� ��Ⱑ �ٸ��Ƿ� �� ������ �����ϰ� �����Ѵ�.
	void		draw_shadow(bool draw = true, float weight = 1.0f);

	//blink
	void		set_blink_time(int nTime0 = 400, int nTime1 = 1200);	//nTime0:hidden, nTime1:shown
	void		set_blink(bool blink = true);

	void		use_tooltip(bool use) { m_use_tooltip = use; }
	//disabled�� ��Ʈ���� main�� PreTranslateMessage()���� ó������ ������ ��Ÿ���� �ʴ´�.
	//���� tooltip�� ������ �� main���� ó���ϵ��� �Ѵ�.
	void		set_tooltip_text(CString text);

	//auto repeat
	void		set_auto_repeat(bool use = true);
	void		set_auto_repeat_delay(int initial_delay = 1, int repeat_delay = 500);

	//public���� �Ͽ� CGdiplusBitmap�� effect���� �Լ����� ����� �� �ֵ��� ��.
	//�ϳ��� ��ư���� n���� �̹����� ���� �� �ְ� �� �̹����� 4���� state image�� ���� ������ �� �ִ�.
	std::deque<CGdiButtonImage*> m_image;

protected:
	BOOL		RegisterWindowClass();

	enum TIMER_ID
	{
		timer_blink = 0,
		timer_auto_repeat,
	};

	UINT		m_button_type;				//BS_PUSHBUTTON(default) or BS_CHECKBOX or BS_RADIOBUTTON
	UINT		m_button_style;				//BS_PUSHLIKE, BS_MULTILINE, BS_FLAT
	//���ŵ� style���� ���� m_button_style�� �Ǻ����� m_button_type���� �Ǻ����� �����Ͽ� �Ǻ���
	//ex. bool b = is_button_style(BS_PUSHBUTTON, BS_DEFPUSHBUTTON);
	template <typename ... Types> bool is_button_style(Types... args)
	{
		UINT styles[] = { args... };

		for (auto style : styles)
		{
			if (style >= BS_PUSHBUTTON && style <= BS_PUSHBOX)
			{
				if (m_button_type == style)
					return true;
			}
			else if (style >= BS_LEFTTEXT && style <= BS_FLAT)
			{
				if ((m_button_style & style) == style)
					return true;
			}
		}

		return false;
	}


	int			m_idx = 0;					//���� ǥ���� m_image�� �ε��� (checkbox�� radio�� �̼���=0, ����=1)
	bool		m_fit2image = true;			//true : �̹��� ũ���� ��Ʈ�� ũ�� ����, false : ���� ��Ʈ�� ũ��� �̹��� ǥ��
	CRect		m_rOrigin = 0;				//��Ʈ���� ���� ũ�� ����

	//����� ������ �⺻���� �ƴ� �׸��̰� ���� PNG�� �׸��� ���, resize���� �� ���� true�� �Ѵ�.
	//��, �� ��� ���� �ϼ��� ����� �ƴ϶� �ణ �����̴� ������ �ִ�.
	bool		m_transparent = false;

	CGdiplusBitmap	m_back_img;					//��ư�� ��� �̹���, NULL�̸� m_cr_back�� ����
	CGdiplusBitmap	m_back_img_origin;

	CString		m_text = _T("");
	std::deque <Gdiplus::Color>	m_cr_text;
	std::deque <Gdiplus::Color>	m_cr_back;		//���� PNG�� ������ �����ߴٸ� ����� �׷�����.

	int			m_width = 0;
	int			m_height = 0;

	int			m_round = 0;				//round rect

	UINT		m_nAnchor = 0;
	int			m_nAnchorMarginX = 0;
	int			m_nAnchorMarginY = 0;

	bool		m_bAsStatic = false;		//�ܼ� �̹��� ǥ�� �뵵�� ���ǰ� Ŭ���ص� ��ȭ�� ����. �⺻�� false.
	bool		m_use_hover = true;			//default = true
	bool		m_draw_hover_rect = false;	//hover �׵θ� �簢�� ǥ�� ����. default = false
	int			m_hover_rect_thick = 2;
	Gdiplus::Color	m_hover_rect_color = gRGB(128, 128, 255);
	bool		m_is_hover = false;			//���� hover ��������
	bool		m_bIsTracking = false;
	bool		m_bPushed = false;

	bool		m_bHasFocus = false;
	bool		m_draw_focus_rect;		//��Ŀ�� �簢�� ǥ�� ����(�⺻�� false)
	Gdiplus::Color	m_crFocusRect;			//����
	int			m_nFocusRectWidth;		//�β�

	bool		m_b3DRect;				//��ü ������ 3D, ������ sunken. default = true;
	CPoint		m_down_offset;			//������ �� �׷��� ��ġ(�⺻��=1);
	bool		m_use_normal_image_on_disabled = false;	//disabled�� �⺻ ȸ������ �ڵ� ���������� �׷��� ���� �ʴ� ��쵵 ���� �� �ִ�.

	//���� ��ư�� ��� �׸��ڸ� ǥ���Ѵ�.
	bool		m_draw_shadow = false;
	//weight�� 1.0���� ũ�� ����, ������ ��ο� �׸��ڰ� �׷�����.
	float		m_shadow_weight = 1.0f;

	bool		m_draw_border = false;
	Gdiplus::Color m_cr_border = Gdiplus::Color::DimGray;
	int			m_border_thick = 1;

	bool		m_blink = false;
	bool		m_blink_status = false;
	int			m_blink_time0 = 400;	//blink type is Show/Hide, time0 = shown duration, time1 = hidden duration in millisecond.
	int			m_blink_time1 = 1200;

	ColorMatrix m_grayMatrix;
	ColorMatrix m_hoverMatrix;			//hover�̹����� ������ ��Ʈ����
	ColorMatrix m_downMatrix;			//down�̹����� ������ ��Ʈ����

	LOGFONT		m_lf;
	CFont		m_font;

	void		reconstruct_font();

	//enable�����϶��� �� ǥ�õǳ� disable�϶��� ǥ�õ��� �ʴ´�.
	//�̸� �ذ��Ϸ��� parent�� PreTranslateMessage()���� ó���ؾ� �Ѵ�.
	CToolTipCtrl *m_tooltip = NULL;
	CToolTipCtrl m_tooltip1;
	bool		m_use_tooltip = true;
	CString		m_tooltip_text = _T("");
	//�������� ���� ��ư�� ���������� �������� ��ư�� ����� ���
	//PreSubclassWindow()���� ������ �ʱ�ȭ�Ϸ��� ���ܰ� �߻���.
	//�׷��� Create()�Ŀ� ������ prepare_tooltip()�� ȣ���Ͽ� �غ�ǵ��� ����.
	void		prepare_tooltip();


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
