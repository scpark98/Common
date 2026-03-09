#pragma once

#include <afxwin.h>
#include <afxcmn.h>
#include <deque>
#include <gdiplus.h>

#include "../../colors.h"
#include "../../SCGdiplusBitmap.h"

//using namespace Gdiplus;

/*
* https://github.com/scpark98/Test_GdiButton.git
- gdi+를 이용한 버튼 클래스
- 투명 png 이미지를 사용할 경우 이미지 모양으로 버튼이 표시됨.
- 사용 방법에 따라 일반 push button, toggle button(as a checkbox) 등으로 사용할 수 있음.
- 2024.08.23 COLORREF -> Gdiplus::Color 로 변경

[개선 방향 정리]
- 지원되는 버튼 종류 m_button_shape?
	- PushButton, CheckBox, RadioButton + PushLike
	- 이미지가 있을 경우
		.이미지만 있는 경우
		.이미지 + 텍스트 => m_img_header
	- 이미지가 없을 경우
		.기본 MFC 버튼과 동일하게 표시
- 이미지 설정
	-

[draw_shadow 관련]
- 이미지로 표현되는 버튼의 경우는 그림자를 생성해서 표현하는 것은 간단하나
  이미지를 지정하지 않고 기본 CButton 처럼 일반 버튼 모양으로 그려지는 버튼의 경우는
  WM_NCPAINT에서 그림자를 그려주면 된다.
  만약 그림이 '<'과 같은 투명 배경에 도형만 있는 경우라면 drop_shadow를 그려주고
  기본 CButton과 같이 사각형 형태로 그려지는 버튼이라면 back_shadow를 그려준다.
  이 두 shadow를 별도로 세팅 및 처리하느냐, 하나의 변수로 처리하느냐는 아직 미정임.

	그림자는 다음과 같이 2가지가 존재한다.
	1. 일반 사각형 버튼의 우측 하단에 표시하는 그림자
	2. "<" 모양의 투명 PNG 이미지를 그릴 때 그림자
	즉, 이미지의 모양에 따라 그림자의 종류가 달라지므로 정의 및 구현 또한 별도로 한다.
	1번 그림자는 back shadow (OnNcPaint에서 그림)
	2번 그림자는 drop shadow (DrawItem에서 그림)
	둘 다 우측 하단에 그려지지만 구현되는 함수 위치가 다르다.

- 버튼 모양의 이미지를 사용하는 경우의 back_shadow를 표시하기 위해서는 전처리가 필요하다.
  버튼 이미지가 꽉 찬 이미지는 blur가 제대로 나타나지 않는다.
  따라서 


[기본값 설정 기준]
- png를 주로 사용하기 위한 클래스를 목적으로 제작되었지만
  이미지를 사용하지 않고 CButton과 유사하게 표현하여 사용하는 경우도 많다.
  따라서 기본 배경색도 윈도우 기본값으로 설정되어 있고, m_transparent = false로 시작한다.
  그래야만 이미지를 사용하지 않는 버튼들도 별다른 세팅없이 일반 버튼처럼 표시된다.

- png를 사용한다면 add_image()등의 멤버함수를 이용해서 세팅하므로 이 세팅을 할 경우는
  m_transparent는 자동으로 true로 변경된다.
  단, 투명 png라도 배경색을 투명색이 아닌 다른 색으로도 설정할 수 있으므로
  back_color()를 이용해 배경색을 칠할 경우에는 m_transparent는 false로 변경된다.
  따라서 배경색을 별도로 지정할 경우에는 반드시 add_image() 설정후에 해야 한다.

[투명 png 또는 round button의 깜빡임 원인 및 현재 조치 내용]
- 버튼이 그려지기 전에 parent에서 버튼 영역을 invalidate() 시킨 후 버튼을 그려야만 투명도가 중첩되는 부작용을 막을 수 있다.
  장점은 배경에 어떤 이미지를 깔아도 버튼이 투명하게 표시된다.
  근데 이렇게하면 깜빡임이 발생했다.
  일반적으로 parent에 배경 이미지를 사용하는 경우는 거의 없다보니 단색으로 칠한 후 버튼을 그리게하도록 수정하여
  깜빡임을 없앴다.
- 만약 parent에 배경 이미지를 표시할 경우 resize를 지원하지 않는 경우라면 *m_img_parent_back에 저장하고
  버튼 영역만큼 이미지를 잘라와서 그려준 후 버튼을 그리면 된다.
  parent가 resize를 지원해야 한다면 매우 복잡해진다.

[fit_to_image 관련]
- 사용할 이미지를 실제 UI design에 맞게 resize해서 사용하는 것을 원칙으로 한다.
1. 컨트롤의 크기에 맞춰 이미지의 크기를 자동 조정해서 그려줄 경우
   resource editor에서 그린 크기대로 표시된다.
   단, resize에 의해 해당 컨트롤이 동적으로 변경되는 경우는 이미지까지 자동 resize가 되지 않으므로 문제된다.
2. 이미지의 크기대로 컨트롤의 크기를 변경시킬 경우
   이미지의 실제크기를 직관적으로 알 수 없으므로 레이아웃이 design단계와 다르게 나타날 수 있다.

[usage]
* Gdiplus 사용을 위한 세팅
* GdiplusBitmap.cpp의 Dummy 클래스에서 자동으로 초기화 및 해제하므로 별도 세팅 불필요.
* 기존 세팅 방법 :
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



* add_image(...)는 한 버튼에 대한 4가지 상태이미지, 즉 normal, over, down, disabled를 설정할 때 사용하고
  add_images(...)는 한 버튼에 여러 이미지들을 추가해 놓고 active_index()를 이용하여 원하는 이미지를 표시하는 용도로 사용할 수 있다
  (on/off, checked/unchecked, play/pause, img0/img1/img2...)

* set_auto_repeat()으로 반복 실행되는 버튼으로 동작 가능.

* 지원되는 버튼 종류
	[투명 png 버튼]
	- 
*/

//버튼 자동 정렬 정의이며 특정 좌표값 기준이 아닌
//전체 배경 이미지(클라이언트 영역)을 기준으로 한 정렬이다.
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
	CGdiButtonMessage(CWnd* _this, UINT _ctrl_id, int _message, CPoint _pt = CPoint(-1, -1))
	{
		pThis = _this;
		ctrl_id = _ctrl_id;
		message = _message;
		pt = _pt;
	}

	CWnd*		pThis = NULL;
	UINT		ctrl_id;
	int			message;
	CPoint		pt;
};

class CGdiButtonImage
{
public:
	CGdiButtonImage() {};

	CSCGdiplusBitmap img[4];	//normal, over, down, disabled
};

class CGdiButtonParagragh
{
public:
	CGdiButtonParagragh();


};

// CGdiButton

class CGdiButton : public CButton//, CSCGdiplusBitmap
{
	DECLARE_DYNAMIC(CGdiButton)

public:
	CGdiButton();
	virtual ~CGdiButton();

	enum GdiButtonMessage
	{
	};

	//동적 생성시에만 사용.
	BOOL		create(CString caption, DWORD dwStyle, CRect r, CWnd* parent, UINT button_id);

	//기존 CButton::SetButtonStyle 함수를 overriding하여 OWNER_DRAW를 추가시켜줘야 한다.
	void		SetButtonStyle(UINT nStyle, BOOL bRedraw = 1);

	//설정된 속성들을 동일한 종류의 대상 컨트롤에 그대로 적용할 때 사용된다. text는 제외된다.
	void		copy_properties(CGdiButton& dst);

	//resource editor에서 버튼의 caption을 입력하면 그대로 출력된다.
	//단, 이미지가 있을 경우는 caption은 추가로 표시하지 않았으나 공용 버튼 이미지를 사용한다면 지정된 텍스트를 출력해줘야 한다.
	//버튼의 caption은 resource editor 또는 set_text() 또는 SetWindowText()로 설정할 수 있다.
	//이미지가 있음에도 텍스트를 출력하고자 한다면 set_draw_own_text() 함수를 호출하여 m_draw_own_text를 true로 설정해야 한다.
	void		set_draw_own_text(bool draw = true) { m_draw_own_text = draw; Invalidate(); }

	void		set_text(CString text) { SetWindowText(text); }
	void		set_text_empty() { SetWindowText(_T("")); }

	void		set_3state(bool tri_state = true) { m_is_3state = tri_state; }
	bool		is_3state() { return m_is_3state; }

	//SC_HELP, SC_PIN, SC_MINIMIZE, SC_MAXIMIZE, SC_CLOSE 등의 한 값으로 줄 경우 해당 버튼이 그려진다.
	void		set_button_cmd(UINT cmd) { m_button_cmd = cmd; }
	
	//기본 PUSH_BUTTON, CHECKBOX, RADIOBUTTON과 같은 style과는 달리
	//PUSH_BUTTON인데 표시 모양이 문단 형태로 표시되는 mobile ui에 주로 사용되는 항목 표시용으로 추가(Tile UI)
	//ex. Visual Studio 시작 시 "리포지토리 복제", "프로젝트 또는 솔루션 열기", "로컬 폴더 열기", "새 프로젝트 만들기"
	//ex. 파일 복사 시 존재하는 파일이 있을 경우 "Copy and Replace", "Don't copy", "Copy, but keep both files"
	//기본 hover효과, round corner, 속성의 flat = true로 설정할 것
	enum BUTTON_TYPE
	{
		type_default = 0,
		type_paragraph,		//1st line : header img, title, 2nd line : comment, other lines : multiline body text
	};
	//1st line : header img, title, 2nd line : comment, other lines : multiline body text
	void		set_paragraph(int header_icon_id, CString title, CString comment, int main_icon_id = -1, CString body_text = _T(""))
	{

	}
	

	//pBack은 버튼의 배경이 되는 그림으로 parent 창의 배경이미지에 사용된 Bitmap*이다.
	//그 배경에서 버튼의 배경을 추출하여 DrawItem에서 배경+전경 이미지를 그려주게 된다.
	//단, parent창에서 배경 그림은 반드시 원래 크기와 비율대로 그려져야 한다.
	//버튼의 위치 좌표에 해당하는 배경 그림을 잘라와서 사용하므로 배경도 1:1로 뿌려져야 한다.
	
	//GetParent()->UpdateWindow()를 이용하여 뿌려주는 방식은 배경 데이터를 명시하지 않아도 투명하게 뿌려지는 장점은 있으나
	//UpdateWindow로 인해 이미지를 변경하는 이벤트가 발생하면 깜빡이는 단점이 존재한다.

	//add_images는 하나의 버튼에 여러개의 resouce 이미지를 추가할 때 사용한다.
	//즉, 이미지 개수만큼 add_image를 호출한다.
	//특히 push, check, radio button 처럼 checked, unchecked 등의 상태 image를 별도로 세팅할 때 사용할 수 있고
	//하나의 버튼이 여러개의 이미지를 가지도록 할 필요가 있을 경우에도 사용된다.
	//on/off, play/pause, img0/img1/img2...
	//이미지를 지정하면 기본적으로 컨트롤의 크기에 맞게 이미지가 자동으로 그려진다.
	//이미지 원본 크기대로 그려지길 원하는 경우는 fit_to_image(true);를 호출해준다.
	template <typename ... T> void add_images(CString type, T... args)
	{
		int n = sizeof...(args);
		int arg[] = { args... };

		for (auto id : arg)
			add_image(type, id);
	}

	//하나의 버튼에 대한 normal, over, down, disabled 이미지들을 각각 세팅할 때 사용된다.
	//UINT가 0이면 자동 생성해준다.
	//타입이 없으면 기본 _T("PNG")로 처리한다.
	//배경색을 별도로 지정할 경우에는 반드시 add_image()후에 set_back_color()를 호출해줘야 한다.
	//이미지를 지정하면 기본적으로 컨트롤의 크기에 맞게 이미지가 자동으로 그려진다.
	//이미지 원본 크기대로 그려지길 원하는 경우는 fit_to_image(true);를 호출해준다.
	bool		add_image(CString type, UINT normal, UINT over = 0, UINT down = 0, UINT disabled = 0);
	bool		add_image(UINT normal, UINT over = 0, UINT down = 0, UINT disabled = 0);
	//기본 이미지를 설정할 때 resize한 후 설정
	bool		add_image_resize(UINT normal, float ratio = 1.0f);
	bool		add_image(CString normal, CString over = _T(""), CString down = _T(""), CString disabled = _T(""));
	bool		add_image(CSCGdiplusBitmap *normal, bool add_auto_state_images = true);
	bool		add_image(Gdiplus::Bitmap *img);
	void		use_normal_image_on_disabled(bool use = true);

	CSize		get_img_size(int index = 0);
	CSize		get_img_origin_size() { return m_sz_img_origin; }

	//텍스트 버튼인 경우 텍스트 앞에 이미지를 표시한다.
	//이미지의 높이는 rc.Height()에 대한 비율로 결정되며 이미지 ratio에 맞게 width도 자동 조정된다.
	//align에 따라 이미지와 텍스트가 정렬되며 align flag는 CDC::DrawText()에서 사용하는 DT_ 정의를 사용한다.
	//이미지의 여백 정도에 따라 크기 또는 텍스트와의 갭이 적절하지 않을 수 있다.
	void		set_header_image(UINT id, float ratio = 0.7f, UINT align = DT_CENTER | DT_VCENTER);
	//이미지와 레이블 사이의 간격 픽셀 크기. m_img_header_gap. default = 4;
	void		set_header_image_gap(int gap);

	//fit = true이면 컨트롤의 크기를 이미지 크기로 resize한다. false이면 컨트롤의 크기에 맞게 이미지를 그려준다.
	void		fit_to_image(bool fit = true);

	//간혹 투명PNG의 경우 parent에서 어떤 방식의 그리기냐에 따라 배경 갱신이 제대로 안될때가 있다.
	void		active_index(int index, bool bErase = false);
	int			active_index() { return m_idx; }

	void		release_all();

	//0(transparent) ~ 255(opaque)
	void		set_alpha(int alpha);

	//void		add_rgb(int red, int green, int blue, COLORREF crExcept);

	void		set_transparent(bool trans = true, Gdiplus::Color cr_parent_back = Gdiplus::Color::Transparent);

	//auto_color를 true로 주면 over, down일때의 색상을 자동으로 설정해준다.
	void		set_color(Gdiplus::Color cr_text, Gdiplus::Color cr_back, bool auto_color = true);

	//void		set_back_imageBitmap* pBack);		//배경을 설정, 변경할 경우 사용
	void		set_text_color(Gdiplus::Color normal, bool auto_color = true);
	void		set_text_color(Gdiplus::Color normal, Gdiplus::Color hover, Gdiplus::Color down, Gdiplus::Color disabled = Gdiplus::Color::Transparent);
	void		set_hover_text_color(Gdiplus::Color hover_back);

	//투명png는 배경을 줄 필요가 없지만 간혹 배경이 refresh가 제대로 동작하지 않아서 필요한 경우도 존재한다.
	//(NH 프로젝트에서 김근호 부장이 작성한 CBaseDialog를 상속받은 CDialog 사용시)
	//auto_color를 true로 주면 over, down일때의 색상을 자동으로 설정해준다.
	void		set_back_color(Gdiplus::Color normal, bool auto_color = true);

	//3개의 색상은 지정하지만 disabled는 기본 disable color를 쓰고자 할 경우에는 Transparent로 호출한다.
	void		set_back_color(Gdiplus::Color normal, Gdiplus::Color hover, Gdiplus::Color down, Gdiplus::Color disabled = Gdiplus::Color::Transparent);
	void		set_hover_back_color(Gdiplus::Color hover_back);

	//text, back color를 기준으로 hover, down 색상들을 자동 설정해준다.
	//m_use_hover = true로 자동 설정된다.
	void		set_auto_hover_down_color();

	//투명 png를 그리거나 round button일 경우는 parent back으로 칠해주고 그려줘야 한다. 그래야 깜빡임을 없앨 수 있다.
	void		set_parent_back_color(Gdiplus::Color cr_parent_back);

	//border color는 기본적으로 m_cr_back과 동일하게 설정되지만 별도로 지정할 수 있다.
	//3개의 색상은 지정하지만 disabled는 기본 disable color를 쓰고자 할 경우에는 Transparent로 호출한다.
	void		set_border_color(Gdiplus::Color normal, Gdiplus::Color hover, Gdiplus::Color down, Gdiplus::Color disabled = Gdiplus::Color::Transparent);
	//auto_set_color를 true로 주면 over, down일때의 색상을 자동으로 설정해준다.
	void		set_border_color(Gdiplus::Color normal, bool auto_set_color = true);

	//CGdiButton& text_color() { m_cr_text.clear(); }
	//CGdiButton& back_color() { m_cr_back.clear(); }
	//reassign [0,0] [1,1] [2,2]
	//hover, down일 경우 색상 변화를 주고자 할 경우 사용.(fScale을 1.0보다 크게주면 밝게, 작게주면 어둡게 변경된다.
	void		set_hover_color_matrix(float fr, float fg = 0.0f, float fb = 0.0f);	//1.0f = no effect.
	void		set_down_color_matrix(float fScale);	//1.0f = no effect.

	//n번째 이미지의 m번째 상태 이미지의 x, y 픽셀 컬러를 변경한다. 단, disable은 제외된다.
	void		replace_color(int index, int state_index, int x, int y, Gdiplus::Color newColor);

	//버튼 이미지에 효과를 적용. state_index가 -1이면 모든 상태이미지에 효과 적용. 0이면 normal image만 적용.
	//hue : -180 ~ 180, sat : -100 ~ 100, light : -100 ~ 100
	void		apply_effect_hsl(int state_index, int hue, int sat = 0, int light = 0);
	void		apply_effect_rgba(int state_index, float r, float g, float b, float a = 1.0);
	void		apply_effect_blur(int state_index, float radius, BOOL expandEdge);


	void		set_font_name(LPCTSTR sFontname, BYTE byCharSet = DEFAULT_CHARSET);
	void		set_font_size(int size);
	void		set_font_weight(int weight = FW_BOLD);
	void		set_font_antialias(bool antialias = true);
	void		set_font(CFont* font);

	int			GetCheck();
	void		SetCheck(int check_state);
	void		Toggle();

	//m_bAsStatic이 true일 경우 hover와 down에는 반응을 하지 않는다.
	void		SetAsStatic(bool bAsStatic = true) { m_bAsStatic = bAsStatic; }


	//이미지 및 버튼의 크기를 조정한다. image_only = true이면 이미지의 크기만 조정할 뿐 버튼의 크기는 조정하지 않는다.
	void		resize(bool image_only, int cx, int cy);

	//이미지의 크기에 맞게 컨트롤을 resize하고 dx, dy, nAnchor에 따라 move해준다.(move는 현재 보류)
	void		resize_control(int cx, int cy);

	//이미지를 사용하지 않고 직접 그려주는 버튼의 경우 width를 정확히 구해야하는 경우가 있다.
	CRect		calc_rect();

	int			width();
	int			height();
	void		SetAnchor( UINT nAnchor ) { m_nAnchor = nAnchor; }	//정렬 방식 설정
	void		SetAnchorMargin( int x, int y ) { m_nAnchorMarginX = x; m_nAnchorMarginY = y; }
	void		ReAlign();	//parent의 크기가 변하면 설정된 align값에 따라 위치를 재조정해준다.
	void		Offset(int x, int y);
	void		Inflate(int cx, int cy);
	void		Inflate(int l, int t, int r, int b);

	//cr_border, cr_parent_back은 그 값이 Gdiplus::Color::Transparent가 아닐 경우에만 유효하다.
	//round = radius. 음수일 경우는 height의 1/2로 세팅되고 트랙 모양의 버튼이 된다.
	void		set_round(int round,
							Gdiplus::Color cr_border = Gdiplus::Color::Transparent,
							Gdiplus::Color cr_parent_back = Gdiplus::Color::Transparent);

	//포커스 사각형 관련
	void		draw_focus_rect(bool draw = true, Gdiplus::Color cr_focus = Gdiplus::Color::LightGray) { m_draw_focus_rect = draw; m_cr_focus_rect = cr_focus; Invalidate(); }
	void		set_focus_rect_color(Gdiplus::Color cr_focus) { m_cr_focus_rect = cr_focus; Invalidate(); }
	void		set_focus_rect_width(int nWidth) { m_focus_rect_width = nWidth; Invalidate(); }

	void		use_hover(bool use = true);
	//thick, round 값이 -1이면 기존 설정값의 변경없음의 의미임
	void		draw_hover_rect(bool draw = true, int thick = -1, int round = -1, Gdiplus::Color cr = Gdiplus::Color::Transparent);
	void		set_hover_rect_thick(int thick);
	void		set_hover_rect_color(Gdiplus::Color cr);
	void		set_down_offset(int ox, int oy) { m_down_offset = CPoint(ox, oy); Invalidate(); }

//border. thick, round 값이 -1이면 기존 설정값의 변경없음의 의미임
	void		draw_border(bool draw = true, int thick = -1, int round = -1, Gdiplus::Color cr_border = Gdiplus::Color::Transparent);

//3D, sunken 
	void		draw_3D_rect(bool use = true) { m_draw_3D_rect = use; Invalidate(); }

	//투명 버튼의 경우 그림자를 표시한다.
	//shadow_weight가 1.0보다 크면 밝은, 작으면 어두운 그림자가 그려진다.
	//blur_sigma가 크면 클수록 그림자의 blur가 강해짐
	//0.0f보다 작은 음수일 경우는 해당 멤버변수값을 갱신하지 않는다.
	//draw_shadow(true, 5.0f, -1.0f);라고 주면 m_shadow_weight는 갱신되지만 m_blur_sigma값은 변경되지 않는다.
	//이 두 옵션값은 이미지 모양에 따라 적절한 옵션값이 달라지므로 이미지에 맞게 적정값을 찾아야 함.
	void		draw_drop_shadow(bool draw = true, float shadow_weight = 1.0f, float blur_sigma = 4.0f);

	void		draw_back_shadow(bool draw = true, float shadow_weight = 1.0f, float blur_sigma = 4.0f);

//blink
	void		set_blink_time(int nTime0 = 400, int nTime1 = 1200);	//nTime0:hidden, nTime1:shown
	void		set_blink(bool blink = true);

//tooltip
	//기본적인 툴팁은 이 컨트롤 내에서 지원하지만
	//disabled인 컨트롤은 main의 PreTranslateMessage()에서 처리하지 않으면 나타나지 않는다.
	void		use_tooltip(bool use) { m_use_tooltip = use; }
	void		set_tooltip_text(CString text);

//auto repeat
	void		set_auto_repeat(bool use = true);
	void		set_auto_repeat_delay(int initial_delay = 1, int repeat_delay = 500);

	//public으로 하여 CSCGdiplusBitmap의 effect등의 함수등을 사용할 수 있도록 함.
	//하나의 버튼에는 n개의 이미지를 담을 수 있고 각 이미지는 4개의 state image를 각각 설정할 수 있다.
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

	//set_button_action()으로 SC_HELP, SC_PIN, SC_MINIMIZE, SC_MAXIMIZE, SC_CLOSE 중의 한 값으로 줄 경우 해당 버튼이 그려진다.
	int			m_button_cmd = -1;

	//3state 버튼은 checkbox의 속성에 줄 수 있는데 이럴 경우 DrawItem()이 아예 호출되지 않는다.
	//따라서 3stat가 필요한 checkbox는 BS_CHECKBOX로 처리하되 m_is_3state로 별도 그려줘야 한다.
	//리소스에서는 해당 옵션을 주지 않아야 하고 set_3state(true);로 세팅해야 한다.
	bool		m_is_3state = false;

	//열거된 style값에 따라 m_button_style로 판별할지 m_button_type으로 판별할지 구분하여 판별함
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

	//현재 표시할 m_image의 인덱스 (checkbox나 radio는 미선택=0, 선택=1)
	int			m_idx = 0;

	//default = false(컨트롤 크기로 이미지 자동 resize), true(이미지 크기대로 컨트롤 크기 변경)
	bool		m_fit2image = false;

	//이미지 원본 크기
	CSize		m_sz_img_origin;

	//컨트롤의 원래 크기 정보
	CRect		m_rOrigin = 0;

	//배경이 윈도우 기본값이 아닌 그림이고 투명 PNG를 그리는 경우, resize까지 할 경우는 true로 한다.
	//단, 이 경우 아직 완성된 기능이 아니라서 약간 깜빡이는 현상이 있다.
	bool		m_transparent = false;

	//parent의 배경에 사용된 이미지를 설정해야 투명 png를 올바르게 표시할 수 있다.
	//parent가 resize가 가능한 경우는 처리가 매우 복잡해진다. 그때는 어쩔수없이 redraw_window()로 갱신해야 한다.
	//만약 parent가 resize를 사용하지 않거나 배경이 단색이라면 깜빡임 없이 그릴 수 있다.
	CSCGdiplusBitmap* m_img_parent = NULL;
	Gdiplus::Color	m_cr_parent_back = Gdiplus::Color::Transparent;

	//배경이 투명인 경우는 parent의 배경까지 Invalidate()해줘야 하므로 그냥 Invalidate()만으로는 안된다.
	//그렇지 않으면 투명 픽셀들이 계속 누적되어 투명으로 표시되지 않는다.
	void		redraw_window(bool bErase = false);


	//버튼 텍스트 앞에 그려질 이미지. 이미지 버튼이 아닌 기본 MFC 버튼 형태일 경우 앞에 이미지를 추가할 경우 사용.
	CSCGdiplusBitmap	m_img_header;
	UINT			m_img_header_align = DT_CENTER;
	float			m_img_header_ratio = 0.6f;
	int				m_img_header_gap = 2;

	//버튼의 배경 이미지, NULL이면 m_cr_back이 배경색
	CSCGdiplusBitmap	m_back_img;
	CSCGdiplusBitmap	m_back_img_origin;

	CString		m_text = _T("");
	std::deque <Gdiplus::Color>	m_cr_text;
	std::deque <Gdiplus::Color>	m_cr_back;		//투명 PNG라도 배경색을 설정했다면 배경색이 그려진다.
	std::deque <Gdiplus::Color> m_cr_border;	//border의 색상은 기본적으로 m_cr_back과 동일하게 세팅되지만 개별 설정도 가능하다.

	//int			m_width = 0;
	//int			m_height = 0;

	int			m_round = 0;				//round rect

	UINT		m_nAnchor = ANCHOR_NONE;
	int			m_nAnchorMarginX = 0;
	int			m_nAnchorMarginY = 0;

	bool		m_bAsStatic = false;		//단순 이미지 표시 용도로 사용되고 클릭해도 변화가 없다. 기본값 false.
	bool		m_use_hover = false;		//default = false
	bool		m_draw_hover_rect = false;	//hover 테두리 사각형 표시 여부. default = false
	int			m_hover_rect_thick = 1;
	Gdiplus::Color	m_hover_rect_color = gRGB(128, 128, 255);
	bool		m_is_hover = false;			//현재 hover 상태인지
	bool		m_bIsTracking = false;
	bool		m_bPushed = false;

	bool		m_bHasFocus = false;
	bool		m_draw_focus_rect = false;				//포커스 사각형 표시 여부(기본값 false)
	Gdiplus::Color	m_cr_focus_rect = Gdiplus::Color::DimGray;	//색상
	int			m_focus_rect_width = 1;					//두께

	bool		m_draw_3D_rect = false;					//입체 느낌의 3D, 누르면 sunken. default = true;
	CPoint		m_down_offset = CPoint(0, 0);			//눌렸을 때 그려질 위치. default는 offset=0. 이 값이 클 경우 여백이 없는 이미지라면 잘릴 수 있다.
	bool		m_use_normal_image_on_disabled = false;	//disabled는 기본 회색으로 자동 생성하지만 그렇게 하지 않는 경우도 있을 수 있다. default = false;

	//이미지를 사용하는 버튼이라도 자신에게 세팅된 텍스트를 표시해줘야 할 경우도 존재한다.
	//ex. 공용 버튼 이미지를 사용하는 경우
	bool		m_draw_own_text = false;

	//버튼의 배경 그림자를 표시한다. default = false
	bool		m_draw_back_shadow = false;
	float		m_back_shadow_weight = 1.0f;
	//blur sigma가 크면 클수록 그림자의 blur가 강해짐. default = 5.0f
	float		m_back_shadow_blur_sigma = 4.0f;

	//투명 이미지 버튼의 경우 그림자를 표시한다. default = false
	bool		m_draw_drop_shadow = false;
	float		m_drop_shadow_weight = 1.0f;
	//blur sigma가 크면 클수록 그림자의 blur가 강해짐. default = 5.0f
	float		m_drop_shadow_blur_sigma = 5.0f;


	//default = false
	bool		m_draw_border = false;
	//default = LightGray
	//Gdiplus::Color m_cr_border = Gdiplus::Color::LightGray;
	//default = 1
	int			m_border_thick = 1;

	bool		m_blink = false;
	bool		m_blink_status = false;
	int			m_blink_time0 = 400;	//blink type is Show/Hide, time0 = shown duration, time1 = hidden duration in millisecond.
	int			m_blink_time1 = 1200;

	Gdiplus::ColorMatrix m_grayMatrix;
	Gdiplus::ColorMatrix m_hoverMatrix;			//hover이미지에 적용할 매트릭스
	Gdiplus::ColorMatrix m_downMatrix;			//down이미지에 적용할 매트릭스

	LOGFONT		m_lf;
	CFont		m_font;

	void		reconstruct_font();

//tooltip
	//enable상태일때는 잘 표시되나 disable일때는 표시되지 않는다.
	//이를 해결하려면 parent의 PreTranslateMessage()에서 처리해야 한다.
	CToolTipCtrl* m_tooltip = NULL;
	//default = true
	bool		m_use_tooltip = true;
	CString		m_tooltip_text = _T("");
	//정적으로 만든 컨트롤은 문제없으나 동적으로 컨트롤을 생성하여 사용하는 경우
	//PreSubclassWindow()에서 툴팁을 초기화하려니 예외가 발생함.
	//그래서 Create()후에 별도로 prepare_tooltip()을 호출하여 준비되도록 수정.
	//동적 생성한 컨트롤에서도 정상 표시됨을 확인함.
	void		prepare_tooltip();


//auto repeat
	int			m_repeat_initial_delay = 1;
	int			m_repeat_delay = 500;
	bool		m_use_auto_repeat = false;
	int			m_sent_once_auto_repeat_click_message = 0;	//만약 down후 initial_delay가 되기도 전에 up된다면 이때에도 한번은 마우스 클릭 이벤트를 처리해줘야 한다.

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
	afx_msg void OnNcPaint();
	afx_msg void OnRButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnRButtonUp(UINT nFlags, CPoint point);
};
