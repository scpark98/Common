#pragma once

#include <afxwin.h>
#include <afxcmn.h>
#include <deque>
#include <gdiplus.h>

#include "../../GdiplusBitmap.h"

using namespace Gdiplus;

/*
- gdi+를 이용한 버튼 클래스
- 투명 png 이미지를 사용할 경우 이미지 모양으로 버튼이 표시됨.
- 사용 방법에 따라 일반 push button, toggle button(as a checkbox) 등으로 사용할 수 있음.

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
* 
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

	//기존 CButton::SetButtonStyle 함수를 overriding하여 OWNER_DRAW를 추가시켜줘야 한다.
	void		SetButtonStyle(UINT nStyle, BOOL bRedraw = 1);
	//enum 

	//pBack은 버튼의 배경이 되는 그림으로 parent 창의 배경이미지에 사용된 Bitmap*이다.
	//그 배경에서 버튼의 배경을 추출하여 DrawItem에서 배경+전경 이미지를 그려주게 된다.
	//단, parent창에서 배경 그림은 반드시 원래 크기와 비율대로 그려져야 한다.
	//버튼의 위치 좌표에 해당하는 배경 그림을 잘라와서 사용하므로 배경도 1:1로 뿌려져야 한다.
	
	//GetParent()->UpdateWindow()를 이용하여 뿌려주는 방식은 배경 데이터를 명시하지 않아도 투명하게 뿌려지는 장점은 있으나
	//UpdateWindow로 인해 이미지를 변경하는 이벤트가 발생하면 깜빡이는 단점이 존재한다.

	//add_images는 하나의 버튼에 여러개의 이미지를 추가할 때 사용한다.
	//즉, 이미지 개수만큼 add_image를 호출한다.
	//특히 push, check, radio button 처럼 checked, unchecked 등의 상태 image를 별도로 세팅할 때 사용할 수 있고
	//하나의 버튼이 여러개의 이미지를 가지도록 할 필요가 있을 경우에도 사용된다.
	//on/off, play/pause, img0/img1/img2...
	template <typename ... Types> void add_images(CString type, Types... args)
	{
		int n = sizeof...(args);
		int arg[] = { args... };

		for (auto id : arg)
			add_image(type, id);
	}

	//하나의 버튼에 대한 normal, over, down, disabled 이미지들을 각각 세팅할 때 사용된다.
	//UINT가 0이면 자동 생성해준다.
	//타입이 없으면 기본 _T("PNG")로 처리한다.
	bool		add_image(CString type, UINT normal, UINT over = 0, UINT down = 0, UINT disabled = 0);
	bool		add_image(UINT normal, UINT over = 0, UINT down = 0, UINT disabled = 0);
	bool		add_image(CString normal, CString over = _T(""), CString down = _T(""), CString disabled = _T(""));
	bool		add_image(CGdiplusBitmap *img);
	bool		add_image(Gdiplus::Bitmap *img);
	void		use_normal_image_on_disabled(bool use = true);

	//fit = true이면 컨트롤의 크기를 이미지 크기로 resize한다. false이면 컨트롤의 크기에 맞게 이미지를 그려준다.
	void		fit_to_image(bool fit = true);

	//간혹 투명PNG의 경우 parent에서 어떤 방식의 그리기냐에 따라 배경 갱신이 제대로 안될때가 있다.
	void		active_index(int index, bool bErase = false);
	int			active_index() { return m_idx; }

	void		release_all();

	void		set_alpha(float alpha);
	void		add_rgb(int red, int green, int blue, COLORREF crExcept);

	//void		set_back_imageBitmap* pBack);		//배경을 설정, 변경할 경우 사용
	CGdiButton& text(CString text);
	CGdiButton& text_color(COLORREF normal, COLORREF hover, COLORREF down, COLORREF disabled);
	CGdiButton& text_color(COLORREF normal);
	CGdiButton& back_color(COLORREF normal, COLORREF hover, COLORREF down, COLORREF disabled);
	//투명png는 배경을 줄 필요가 없지만 간혹 배경이 refresh가 제대로 동작하지 않아서 필요한 경우도 존재한다.
	//(NH프로젝트에서 김근호 부장이 작성한 CBaseDialog를 상속받은 CDialog 사용시)
	//auto_set_color를 true로 주면 over, down일때의 색상을 자동으로 설정해준다.
	CGdiButton& back_color(COLORREF normal, bool auto_set_color = false);
	CGdiButton& text_color() { m_cr_text.clear(); }
	CGdiButton& back_color() { m_cr_back.clear(); }
	//reassign [0,0] [1,1] [2,2]
	//hover, down일 경우 색상 변화를 주고자 할 경우 사용.(fScale을 1.0보다 크게주면 밝게, 작게주면 어둡게 변경된다.
	CGdiButton& set_hover_color_matrix(float fScale);	//1.0f = no effect.
	CGdiButton& set_down_color_matrix(float fScale);	//1.0f = no effect.

	//n번째 이미지의 m번째 상태 이미지의 x, y 픽셀 컬러를 변경한다. 단, disable은 제외된다.
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

	//m_bAsStatic이 true일 경우 hover와 down에는 반응을 하지 않는다.
	void		SetAsStatic( bool bAsStatic = true ) { m_bAsStatic = bAsStatic; }


	//버튼의 크기, 위치 변경
	int			width();
	int			height();
	void		SetAnchor( UINT nAnchor ) { m_nAnchor = nAnchor; }	//정렬 방식 설정
	void		SetAnchorMargin( int x, int y ) { m_nAnchorMarginX = x; m_nAnchorMarginY = y; }
	void		ReAlign();	//parent의 크기가 변하면 설정된 align값에 따라 위치를 재조정해준다.
	void		Offset( int x, int y );
	void		Inflate( int cx, int cy );
	void		Inflate( int l, int t, int r, int b );
	virtual CGdiButton& set_round(int round);

	//포커스 사각형 관련
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
	//disabled인 컨트롤은 main의 PreTranslateMessage()에서 처리하지 않으면 나타나지 않는다.
	//따라서 tooltip은 가능한 한 main에서 처리하도록 한다.
	void		set_tooltip_text(CString text);

	//auto repeat
	void		set_auto_repeat(bool use = true);
	void		set_auto_repeat_delay(int initial_delay = 1, int repeat_delay = 500);

	//public으로 하여 CGdiplusBitmap의 effect등의 함수등을 사용할 수 있도록 함.
	std::deque<CButtonImage*> m_image;

protected:
	BOOL	RegisterWindowClass();

	enum TIMER_ID
	{
		timer_blink = 0,
		timer_auto_repeat,
	};

	UINT		m_button_style;				//pushbutton(default) or checkbox or radiobutton

	int			m_idx = 0;					//현재 표시할 m_image의 인덱스 (checkbox나 radio는 미선택=0, 선택=1)
	bool		m_fit2image = true;			//true : 이미지 크기대로 컨트롤 크기 변경, false : 원래 컨트롤 크기로 이미지 표시
	CRect		m_rOrigin = 0;				//컨트롤의 원래 크기 정보

	//배경이 단색이 아닌 그림이고 투명 PNG를 그리는 경우, resize까지 할 경우는 true로 한다.
	//단, 이 경우 아직 완성된 기능이 아니라서 약간 깜빡이는 현상이 있다.
	bool		m_transparent = true;

	CGdiplusBitmap	m_back;					//버튼의 배경 이미지, NULL이면 m_crBack이 배경색
	CGdiplusBitmap	m_back_origin;

	CString		m_text = _T("");
	std::deque <COLORREF>	m_cr_text;
	std::deque <COLORREF>	m_cr_back;		//투명 PNG라도 배경색을 설정했다면 배경이 그려진다.

	int			m_width = 0;
	int			m_height = 0;

	int			m_round = 0;				//round rect

	UINT		m_nAnchor = 0;
	int			m_nAnchorMarginX = 0;
	int			m_nAnchorMarginY = 0;

	bool		m_bAsStatic = false;		//단순 이미지 표시 용도로 사용되고 클릭해도 변화가 없다. 기본값 false.
	bool		m_use_hover = true;			//default = true;
	bool		m_hover_rect = false;		//hover 테두리 사각형 표시 여부
	int			m_hover_rect_thick = 2;
	COLORREF	m_hover_rect_color = RGB(128, 128, 255);
	bool		m_bHover = false;
	bool		m_bIsTracking = false;
	bool		m_bPushed = false;
	bool		m_bHasFocus = false;
	bool		m_bShowFocusRect;		//포커스 사각형 표시 여부(기본값 false)
	COLORREF	m_crFocusRect;			//색상
	int			m_nFocusRectWidth;		//두께
	bool		m_b3DRect;				//입체 느낌의 3D, 누르면 sunken. default = true;
	CPoint		m_down_offset;			//눌렸을 때 그려질 위치(기본값=1);
	bool		m_use_normal_image_on_disabled = false;	//disabled는 기본 회색으로 자동 생성하지만 그렇게 하지 않는 경우도 있을 수 있다.

	bool		m_blink = false;
	bool		m_blink_status = false;
	int			m_blink_time0 = 400;	//blink type is Show/Hide, time0 = shown duration, time1 = hidden duration in millisecond.
	int			m_blink_time1 = 1200;

	ColorMatrix m_grayMatrix;
	ColorMatrix m_hoverMatrix;			//hover이미지에 적용할 매트릭스
	ColorMatrix m_downMatrix;			//down이미지에 적용할 매트릭스

	void		resize_control(int cx, int cy);

	LOGFONT		m_lf;
	CFont		m_font;

	void		reconstruct_font();

	//enable상태일때는 잘 표시되나 disable일때는 표시되지 않는다.
	//이를 해결하려면 parent의 PreTranslateMessage()에서 처리해야 한다.
	CToolTipCtrl	m_tooltip;
	bool		m_use_tooltip = true;
	CString		m_tooltip_text = _T("");


	//auto repeat
	int			m_initial_delay = 1;
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
	afx_msg BOOL OnToolTipNotify(UINT id, NMHDR* pNMHDR, LRESULT* pResult);
};


