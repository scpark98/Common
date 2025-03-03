#pragma once

#include <afxwin.h>
#include <afxcmn.h>
#include <deque>
#include <gdiplus.h>

#include "../../colors.h"
#include "../../GdiplusBitmap.h"

//using namespace Gdiplus;

/*
- gdi+�� �̿��� ��ư Ŭ����
- ���� png �̹����� ����� ��� �̹��� ������� ��ư�� ǥ�õ�.
- ��� ����� ���� �Ϲ� push button, toggle button(as a checkbox) ������ ����� �� ����.
- 2024.08.23 COLORREF -> Gdiplus::Color �� ����

[���� ���� ����]
- �����Ǵ� ��ư ���� m_button_shape?
	- PushButton, CheckBox, RadioButton + PushLike
	- �̹����� ���� ���
		.�̹����� �ִ� ���
		.�̹��� + �ؽ�Ʈ => m_img_header
	- �̹����� ���� ���
		.�⺻ MFC ��ư�� �����ϰ� ǥ��
- �̹��� ����
	-

[draw_shadow ����]
- �̹����� ǥ���Ǵ� ��ư�� ���� �׸��ڸ� �����ؼ� ǥ���ϴ� ���� �����ϳ�
  �⺻ MFC CButton ó�� �Ϲ� ��ư���� �׷����� ��ư�� ����
  �׸��� ������ŭ ��ư ũ�⸦ Ȯ���ϴ���, ��ư ũ�⸦ ���� ���̳Ŀ� ���� ������ �ʿ��ϴ�.
  ���� ��� +1, +1�� �׸��ڰ� �߰��ȴٸ� ���� �������� ��ư�� ����� �׸��� ������ ������ ������ �°�
  border, text align���� ǥ���Ǿ�� �Ѵ�.

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

[���� png �Ǵ� round button�� ������ ���� �� ���� ��ġ ����]
- ��ư�� �׷����� ���� parent���� ��ư ������ invalidate() ��Ų �� ��ư�� �׷��߸� ������ ��ø�Ǵ� ���ۿ��� ���� �� �ִ�.
  ������ ��濡 � �̹����� ��Ƶ� ��ư�� �����ϰ� ǥ�õȴ�.
  �ٵ� �̷����ϸ� �������� �߻��ߴ�.
  �Ϲ������� parent�� ��� �̹����� ����ϴ� ���� ���� ���ٺ��� �ܻ����� ĥ�� �� ��ư�� �׸����ϵ��� �����Ͽ�
  �������� ���ݴ�.
- ���� parent�� ��� �̹����� ǥ���� ��� resize�� �������� �ʴ� ����� *m_img_parent_back�� �����ϰ�
  ��ư ������ŭ �̹����� �߶�ͼ� �׷��� �� ��ư�� �׸��� �ȴ�.
  parent�� resize�� �����ؾ� �Ѵٸ� �ſ� ����������.

[fit_to_image ����]
- ����� �̹����� ���� UI design�� �°� resize�ؼ� ����ϴ� ���� ��Ģ���� �Ѵ�.
1. ��Ʈ���� ũ�⿡ ���� �̹����� ũ�⸦ �ڵ� �����ؼ� �׷��� ���
   resource editor���� �׸� ũ���� ǥ�õȴ�.
   ��, resize�� ���� �ش� ��Ʈ���� �������� ����Ǵ� ���� �̹������� �ڵ� resize�� ���� �����Ƿ� �����ȴ�.
2. �̹����� ũ���� ��Ʈ���� ũ�⸦ �����ų ���
   �̹����� ����ũ�⸦ ���������� �� �� �����Ƿ� ���̾ƿ��� design�ܰ�� �ٸ��� ��Ÿ�� �� �ִ�.

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

	void		set_3state(bool tri_state = true) { m_is_3state = tri_state; }
	bool		is_3state() { return m_is_3state; }
	
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

	//add_images�� �ϳ��� ��ư�� �������� resouce �̹����� �߰��� �� ����Ѵ�.
	//��, �̹��� ������ŭ add_image�� ȣ���Ѵ�.
	//Ư�� push, check, radio button ó�� checked, unchecked ���� ���� image�� ������ ������ �� ����� �� �ְ�
	//�ϳ��� ��ư�� �������� �̹����� �������� �� �ʿ䰡 ���� ��쿡�� ���ȴ�.
	//on/off, play/pause, img0/img1/img2...
	//�̹����� �����ϸ� �⺻������ ��Ʈ���� ũ�⿡ �°� �̹����� �ڵ����� �׷�����.
	//�̹��� ���� ũ���� �׷����� ���ϴ� ���� fit_to_image(true);�� ȣ�����ش�.
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
	//�̹����� �����ϸ� �⺻������ ��Ʈ���� ũ�⿡ �°� �̹����� �ڵ����� �׷�����.
	//�̹��� ���� ũ���� �׷����� ���ϴ� ���� fit_to_image(true);�� ȣ�����ش�.
	bool		add_image(CString type, UINT normal, UINT over = 0, UINT down = 0, UINT disabled = 0);
	bool		add_image(UINT normal, UINT over = 0, UINT down = 0, UINT disabled = 0);
	//�⺻ �̹����� ������ �� resize�� �� ����
	bool		add_image_resize(UINT normal, float ratio = 1.0f);
	bool		add_image(CString normal, CString over = _T(""), CString down = _T(""), CString disabled = _T(""));
	bool		add_image(CGdiplusBitmap *img);
	bool		add_image(Gdiplus::Bitmap *img);
	void		use_normal_image_on_disabled(bool use = true);

	//�ؽ�Ʈ ��ư�� ��� �ؽ�Ʈ �տ� �̹����� ǥ���Ѵ�.
	//�̹����� ���̴� rc.Height()�� ���� ������ �����Ǹ� �̹��� ratio�� �°� width�� �ڵ� �����ȴ�.
	//align�� ���� �̹����� �ؽ�Ʈ�� ���ĵǸ� align flag�� CDC::DrawText()���� ����ϴ� DT_ ���Ǹ� ����Ѵ�.
	//�̹����� ���� ������ ���� ũ�� �Ǵ� �ؽ�Ʈ���� ���� �������� ���� �� �ִ�.
	void		set_header_image(UINT id, float ratio = 0.7f, UINT align = DT_CENTER | DT_VCENTER);
	//�̹����� ���̺� ������ ���� �ȼ� ũ��. m_img_header_gap. default = 4;
	void		set_header_image_gap(int gap);

	//fit = true�̸� ��Ʈ���� ũ�⸦ �̹��� ũ��� resize�Ѵ�. false�̸� ��Ʈ���� ũ�⿡ �°� �̹����� �׷��ش�.
	void		fit_to_image(bool fit = true);

	//��Ȥ ����PNG�� ��� parent���� � ����� �׸���Ŀ� ���� ��� ������ ����� �ȵɶ��� �ִ�.
	void		active_index(int index, bool bErase = false);
	int			active_index() { return m_idx; }

	void		release_all();

	//0(transparent) ~ 255(opaque)
	void		set_alpha(int alpha);

	//void		add_rgb(int red, int green, int blue, COLORREF crExcept);

	void		set_transparent(bool trans = true, Gdiplus::Color cr_parent_back = Gdiplus::Color::Transparent);

	//void		set_back_imageBitmap* pBack);		//����� ����, ������ ��� ���
	void		set_text_color(Gdiplus::Color normal, Gdiplus::Color hover, Gdiplus::Color down, Gdiplus::Color disabled);
	void		set_text_color(Gdiplus::Color normal);
	void		set_back_color(Gdiplus::Color normal, Gdiplus::Color hover, Gdiplus::Color down, Gdiplus::Color disabled);
	//����png�� ����� �� �ʿ䰡 ������ ��Ȥ ����� refresh�� ����� �������� �ʾƼ� �ʿ��� ��쵵 �����Ѵ�.
	//(NH ������Ʈ���� ���ȣ ������ �ۼ��� CBaseDialog�� ��ӹ��� CDialog ����)
	//auto_set_color�� true�� �ָ� over, down�϶��� ������ �ڵ����� �������ش�.
	void		set_back_color(Gdiplus::Color normal, bool auto_set_color = true);

	//���� png�� �׸��ų� round button�� ���� parent back���� ĥ���ְ� �׷���� �Ѵ�. �׷��� �������� ���� �� �ִ�.
	void		set_parent_back_color(Gdiplus::Color cr_parent_back);

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
	void		set_font_size(int nSize);
	void		set_font_bold(bool bBold = true);


	int			GetCheck();
	void		SetCheck(int check_state);
	void		Toggle();

	//m_bAsStatic�� true�� ��� hover�� down���� ������ ���� �ʴ´�.
	void		SetAsStatic(bool bAsStatic = true) { m_bAsStatic = bAsStatic; }


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

	void		use_hover(bool use = true);
	//thick, round ���� -1�̸� ���� �������� ��������� �ǹ���
	void		draw_hover_rect(bool draw = true, int thick = -1, int round = -1, Gdiplus::Color cr = Gdiplus::Color::DimGray);
	void		set_hover_rect_thick(int thick);
	void		set_hover_rect_color(Gdiplus::Color cr);
	void		down_offset(CPoint offset) { m_down_offset = offset; Invalidate(); }
	void		down_offset(int offset) { m_down_offset = CPoint(offset, offset); Invalidate(); }

	//border. thick, round ���� -1�̸� ���� �������� ��������� �ǹ���
	void		draw_border(bool draw = true, int thick = -1, int round = -1, Gdiplus::Color cr = Gdiplus::Color::DimGray);

	//3D, sunken 
	void		use_3D_rect(bool use = true) { m_b3DRect = use; Invalidate(); }

	//���� ��ư�� ��� �׸��ڸ� ǥ���Ѵ�.
	//shadow_weight�� 1.0���� ũ�� ����, ������ ��ο� �׸��ڰ� �׷�����.
	//blur_sigma�� ũ�� Ŭ���� �׸����� blur�� ������
	//0.0f���� ���� ������ ���� �ش� ����������� �������� �ʴ´�.
	//draw_shadow(true, 5.0f, -1.0f);��� �ָ� m_shadow_weight�� ���ŵ����� m_blur_sigma���� ������� �ʴ´�.
	void		draw_shadow(bool draw = true, float shadow_weight = 1.0f, float blur_sigma = 4.0f);

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

	//3state ��ư�� checkbox�� �Ӽ��� �� �� �ִµ� �̷� ��� DrawItem()�� �ƿ� ȣ����� �ʴ´�.
	//���� 3stat�� �ʿ��� checkbox�� BS_CHECKBOX�� ó���ϵ� m_is_3state�� ���� �׷���� �Ѵ�.
	//���ҽ������� �ش� �ɼ��� ���� �ʾƾ� �ϰ� set_3state(true);�� �����ؾ� �Ѵ�.
	bool		m_is_3state = false;

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

	//���� ǥ���� m_image�� �ε��� (checkbox�� radio�� �̼���=0, ����=1)
	int			m_idx = 0;

	//default = false(��Ʈ�� ũ��� �̹��� �ڵ� resize), true(�̹��� ũ���� ��Ʈ�� ũ�� ����)
	bool		m_fit2image = false;

	//�̹��� ���� ũ��
	CSize		m_img_origin;

	//��Ʈ���� ���� ũ�� ����
	CRect		m_rOrigin = 0;

	//����� ������ �⺻���� �ƴ� �׸��̰� ���� PNG�� �׸��� ���, resize���� �� ���� true�� �Ѵ�.
	//��, �� ��� ���� �ϼ��� ����� �ƴ϶� �ణ �����̴� ������ �ִ�.
	bool		m_transparent = false;

	//parent�� ��濡 ���� �̹����� �����ؾ� ���� png�� �ùٸ��� ǥ���� �� �ִ�.
	//parent�� resize�� ������ ���� ó���� �ſ� ����������. �׶��� ��¿������ redraw_window()�� �����ؾ� �Ѵ�.
	//���� parent�� resize�� ������� �ʰų� ����� �ܻ��̶�� ������ ���� �׸� �� �ִ�.
	CGdiplusBitmap* m_img_parent = NULL;
	Gdiplus::Color	m_cr_parent_back = Gdiplus::Color::Transparent;

	//����� ������ ���� parent�� ������ Invalidate()����� �ϹǷ� �׳� Invalidate()�����δ� �ȵȴ�.
	//�׷��� ������ ���� �ȼ����� ��� �����Ǿ� �������� ǥ�õ��� �ʴ´�.
	void		redraw_window(bool bErase = false);


	//��ư �ؽ�Ʈ �տ� �׷��� �̹���. �̹��� ��ư�� �ƴ� �⺻ MFC ��ư ������ ��� �տ� �̹����� �߰��� ��� ���.
	CGdiplusBitmap	m_img_header;
	UINT			m_img_header_align = DT_CENTER;
	float			m_img_header_ratio = 0.6f;
	int				m_img_header_gap = 4;

	//��ư�� ��� �̹���, NULL�̸� m_cr_back�� ����
	CGdiplusBitmap	m_back_img;
	CGdiplusBitmap	m_back_img_origin;

	CString		m_text = _T("");
	std::deque <Gdiplus::Color>	m_cr_text;
	std::deque <Gdiplus::Color>	m_cr_back;		//���� PNG�� ������ �����ߴٸ� ������ �׷�����.

	//int			m_width = 0;
	//int			m_height = 0;

	int			m_round = 0;				//round rect

	UINT		m_nAnchor = ANCHOR_NONE;
	int			m_nAnchorMarginX = 0;
	int			m_nAnchorMarginY = 0;

	bool		m_bAsStatic = false;		//�ܼ� �̹��� ǥ�� �뵵�� ���ǰ� Ŭ���ص� ��ȭ�� ����. �⺻�� false.
	bool		m_use_hover = false;		//default = false
	bool		m_draw_hover_rect = false;	//hover �׵θ� �簢�� ǥ�� ����. default = false
	int			m_hover_rect_thick = 1;
	Gdiplus::Color	m_hover_rect_color = gRGB(128, 128, 255);
	bool		m_is_hover = false;			//���� hover ��������
	bool		m_bIsTracking = false;
	bool		m_bPushed = false;

	bool		m_bHasFocus = false;
	bool		m_draw_focus_rect = false;				//��Ŀ�� �簢�� ǥ�� ����(�⺻�� false)
	Gdiplus::Color	m_crFocusRect = gRGB(6, 205, 255);	//����
	int			m_nFocusRectWidth = 2;					//�β�

	bool		m_b3DRect = true;						//��ü ������ 3D, ������ sunken. default = true;
	CPoint		m_down_offset = CPoint(1, 1);			//������ �� �׷��� ��ġ. �� ���� Ŭ ��� �̹����� ������ ���ٸ� ��
	bool		m_use_normal_image_on_disabled = false;	//disabled�� �⺻ ȸ������ �ڵ� ���������� �׷��� ���� �ʴ� ��쵵 ���� �� �ִ�.

	//���� ��ư�� ��� �׸��ڸ� ǥ���Ѵ�.
	bool		m_draw_shadow = false;
	//m_shadow_weight
	float		m_shadow_weight = 1.0f;
	//m_blur_sigma�� ũ�� Ŭ���� �׸����� blur�� ������. default = 5.0f
	float		m_blur_sigma = 5.0f;

	bool		m_draw_border = false;
	Gdiplus::Color m_cr_border = Gdiplus::Color::DimGray;
	int			m_border_thick = 1;

	bool		m_blink = false;
	bool		m_blink_status = false;
	int			m_blink_time0 = 400;	//blink type is Show/Hide, time0 = shown duration, time1 = hidden duration in millisecond.
	int			m_blink_time1 = 1200;

	Gdiplus::ColorMatrix m_grayMatrix;
	Gdiplus::ColorMatrix m_hoverMatrix;			//hover�̹����� ������ ��Ʈ����
	Gdiplus::ColorMatrix m_downMatrix;			//down�̹����� ������ ��Ʈ����

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
