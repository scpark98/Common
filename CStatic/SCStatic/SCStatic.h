//Test Project: https://github.com/scpark98/Test_CEdit.git

#if !defined(AFX_SCSTATIC_INCLUDED_)
#define AFX_SCSTATIC_INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#pragma once

#include <afxwin.h>
#include <deque>

//.ico가 아닌 png 이미지들을 앞에 그려주고 필요에 따라 변경되도록 하기 위해 사용.
#include "../../SCGdiplusBitmap.h"
#include "../../data_structure/SCParagraph/SCParagraph.h"
#include "../../CEdit/SCEdit/SCEdit.h"
/*
//scpark

[gradient usage]
m_static1.set_gradient();
m_static1.set_gradient_color(RED);
//m_static1.set_vertical_gradient);
//m_static1.set_gradient_color(4, RED, GREEN, YELLOW, BLUE);

[주의사항]
- parent의 WM_SIZE에 의해 CSCStatic의 인스턴스도 resize할 경우 깜빡인다면
  parent 속성에서 Clip_Children을 true로 설정해야 깜빡이지 않는다.

[투명처리 규칙]
- 사각형 영역 배경색이 정해지면 불투명으로 처리된다.
- m_round > 0 이거나 투명 이미지를 표시할 경우는 먼저 parent_back을 칠해줘야 한다.

- parent에 배경 그림이 있을 경우는 parent_back_image를 알고 있어야 하고 컨트롤의 위치에 해당하는 이미지를 crop해서
  먼저 그려준 후에 static을 표현해야 한다.
  물론 parent가 resize를 지원하거나 parent_back_image가 1:1크기로 그려진 게 아니라면 이마저도 불가하다.
  이럴 경우 parent DC를 얻어와서 해당 배경 정보를 추출하고 이를 그려준 후 static을 그려주는 코드를 사용했었으나
  깜빡임이 발생하기도 하고 투명 이미지를 덮어써서 그리는 이벤트가 발생되면 투명정보가 점점 불투명해지는 부작용이 있다.
  배경 그림이 있거나 그림의 크기가 1:1이 아닌 경우는 우선 현재 시점에서는 배재시킨다.

[이미지 추가 시]
- set_icon()을 이용해서 텍스트 앞에 아이콘을 표시할 수 있다.
- add_header_image()를 이용해서 텍스트 앞에 이미지를 표시할 수 있다.
  set_icon()과 다른 점은 .ico 파일이 아닌 png 등의 이미지 포맷을 지원하고
  2장 이상의 이미지를 추가한 후 필요에 따라 이미지를 변경할 수도 있다.
- 두 함수 모두 left_align_fix를 true로 설정하면 이미지는 왼쪽 정렬로 고정되고 텍스트는 설정된 정렬방식으로 표시된다.
  이 파라미터를 추가한 이유 CSCMessageBox에서 이 CSCStatic을 사용하는데 메시지박스를 보면 아이콘은 항상 왼쪽에, 텍스트는 왼쪽 또는 중앙정렬로 표시된다.
  false일 경우는 텍스트의 위치에 따라 아이콘과 이미지의 위치가 변경된다.

[label + value 형태로 사용할 때]
- set_use_edit(true)를 하면 click으로 편집이 가능해진다. 편집이 시작되면 내부적으로 CEdit 컨트롤이 생성되고 편집이 끝나면 소멸된다.
- Resource Editor에서 해당 static의 속성에서 "이미지 가운데 맞춤" 선택, 컨트롤의 높이는 default로 8 DLU로 생성되지만 12 DLU로 조정해준다.


[2014-11-26]

[수정될 내용]
- CSCStatic과 CSCParagraphStatic을 합친다? CSCTagStatic 이름변경?
  tag가 포함되었느냐에 따라 OnPaint() 등이 달라진다.

- 배경이 있는 앱에서는 투명이 잘 적용되지만 기본 스타일의 dlg에서는 화면 갱신이 잘 되지 않는다.
  우선 기본 스타일의 dlg에서는 배경색을 지정해주자.
- 출력크기보다 rc가 작으면 키워줘야 한다?
- "\r\n"으로 multiline을 출력할 경우 center image, no wrap, simple 속성은 반드시 false가 되어야 한다.
  따라서 이러한 속성을 살리기 위해서는 파싱하여 각 라인을 출력하도록 수정이 필요하다.

- set_text(_T("<cr=red>test</cr>"))와 같이 html 태그를 지원하려면? <b>, <i>, <u>, <s> 정도 지원 예정.
  태그가 있는 텍스트와 없는 텍스트를 혼합하여 출력할 경우는 paragraph 구조체가 필요하다.

[2014-11-27]
- blink시에 아이콘도 함께 적용되지 않던 문제 수정
- blink를 false로 할 경우 숨겨진 상태로 blink가 종료되던 문제 수정

*/

// CSCStatic

typedef UINT (CALLBACK* LPFNDLLFUNC1)(HDC,CONST PTRIVERTEX,DWORD,CONST PVOID,DWORD,DWORD);

static const UINT Message_CSCStatic = ::RegisterWindowMessage(_T("MessageString_CSCStatic"));

class CSCStatic;

class CSCStaticMsg
{
public:
	CSCStaticMsg(int _msg, CSCStatic* _this, CString  _sValue = _T(""), int _nValue = 0)
	{
		pThis = _this;
		msg = _msg;
		sValue = _sValue;
		nValue = _nValue;
	}

	enum CSCStaticMsgs
	{
		msg_text_value_changed,
	};

	CSCStatic* pThis = NULL;
	int		msg;
	CString	sValue;
	int		nValue;
};

class CSCStatic : public CStatic
{
	DECLARE_DYNAMIC(CSCStatic)

public:
	CSCStatic();
	~CSCStatic();


	//동적생성시에 사용
	BOOL			create(LPCTSTR lpszText, DWORD dwStyle, const RECT& rect, CWnd* pParentWnd, UINT nID = 0xffff);
	
	//설정된 속성들을 동일한 종류의 대상 컨트롤에 그대로 적용할 때 사용된다. text는 제외된다.
	void			copy_properties(CSCStatic& dst);

	void			set_transparent(bool bTransparent = true, Gdiplus::Color cr_parent_back = Gdiplus::Color::Transparent);
	void			SetWindowText(CString sText) { set_text(sText); }

	CString			get_text();

	//text는 일반 문자열 또는 tag가 포함된 문자열이 될 수 있다.
	CRect			set_text(CString text, Gdiplus::Color cr_text = Gdiplus::Color::Transparent);

	//printf()와 같이 변수값을 바로 문자열로 세팅할 수 있다.
	//초기 버전에서는 첫번째 파라미터로 컬러를 주고 그 값이 -1이면 default text color를 사용하도록 구현했었으나
	//-1 비교가 제대로 되지 않아 우선 제거함.
	void			set_textf(LPCTSTR format, ...);


//자체 편집 기능
	//편집 기능 허용. click으로 편집시작, esc, return, 다른 항목 클릭으로 편집 종료된다.
	void			set_use_edit(bool use = true, UINT align = ES_RIGHT);
	//편집 시 표시되는 edit의 width를 지정한다. 0 또는 음수값이면 텍스트의 너비 + 8만큼 edit의 width가 지정된다. (편집이 시작될 때 계산되어 적용된다.)
	void			set_edit_width(int width);
	CString			get_text_value() { return m_text_value; }
	void			set_text_value(CString value = _T(""));
	void			set_text_value(LPCTSTR format, ...);

	//label + value로 표시하는 경우 value 편집할 때 CEdit의 색상을 지정한다.
	void			set_edit_text_color(Gdiplus::Color cr_edit_text = Gdiplus::Color::Transparent);
	void			set_edit_back_color(Gdiplus::Color cr_edit_back = Gdiplus::Color::Transparent);
	void			edit_begin();
	void			edit_end(bool valid);
	LRESULT			on_message_CSCEdit(WPARAM wParam, LPARAM lParam);


	//CStatic의 SS_LEFT, SS_RIGHT 등의 align 설정값을 DrawText()에서 사용하는 DT_LEFT, DT_RIGHT 등으로 치환하여 리턴
	DWORD			get_text_align();

	//VALIGN이 SS_CENTERIMAGE가 아니면 상단정렬 둘 뿐이므로 DT_TOP, DT_VCENTER, DT_BOTTOM으로 정렬할 수 있도록 확장한다.
	int				get_halign();
	int				get_valign();
	//아직 미구현
	//void			set_halign();
	//void			set_valign();

	//텍스트가 실제 출력될 영역의 크기를 알기 위해 m_rect_text를 추가했으나 현재 방식으로는 rc와 거의 동일한 값이 리턴된다.
	//텍스트의 너비를 알고자 할 경우는 get_text_extent()를 사용해야 한다.
	CRect			get_text_rect() { return m_rect_text; }
	//텍스트의 너비, 높이를 알고자 할 경우 사용.
	CSize			get_text_extent();

	//parent에서 현재 이 static의 위치를 리턴.
	CRect			get_rect();

	//color theme 설정
	CSCColorTheme	m_theme = CSCColorTheme(this);
	void			set_color_theme(int theme, bool invalidate = false);

	//글자색, 배경색 동시 설정
	void			set_color(Gdiplus::Color cr_text, Gdiplus::Color cr_back);
	void			set_text_color(Gdiplus::Color cr_text);
	Gdiplus::Color	get_text_color() { return m_theme.cr_text; }
	Gdiplus::Color	get_back_color() { return m_theme.cr_back; }
	void			set_back_color(Gdiplus::Color cr_back);

	//gradient
	//m_crGradient deque에 색상들을 넣고 n등분하여 gradient fill을 해서 표시한다.
	void			set_gradient(bool bGradient = true);
	void			set_gradient_color(Gdiplus::Color crGradient);			//배경~crGradient 2컬러로 표현됨.
	void			set_gradient_color(int idx, Gdiplus::Color crGradient);	//idx의 색상을 crGradient로 변경. idx가 없으면 idx까지 crGradient로 채움.

	template <typename ... T> CSCStatic& set_gradient_color(T... colors)
	{
		set_gradient();
		m_crGradient.clear();

		int n = sizeof...(colors);
		int arg[] = { colors... };
		for (auto cr : arg)
		{
			m_crGradient.push_back(cr);
		}

		return *this;
	}

	void			add_gradient_color(Gdiplus::Color crGradient);			//색상 추가
	void			insert_gradient_color(int idx, Gdiplus::Color crGradient);//idx번째 색상 추가
	void			set_vertical_gradient(bool bVertical = true);

	void			sunken(bool sunken = true) { m_bSunken = sunken; Invalidate(); }


//marquee효과를 위해서 간단히 테스트해 보았으나 부드럽게 표현하기 위해서는
//rc의 width 또는 height 크기에 적당할 정도로 m_crGradient를 자동으로 채워준 후에 돌려야한다.
//필요성이 있나 싶어서 일단 보류한다.
	//start = 1:start, 0:stop, -1:toggle
	bool			m_start_marquee;
	//void			start_marquee(int start);

	//gif도 가능.
	//m_static_copy.set_back_image(_T("GIF"), IDR_GIF_COPY, Gdiplus::Color::White);
	//만약 원본 크기가 static ctrl의 크기보다 클 경우는 보이지 않게 되므로
	//m_static_copy.fit_to_back_image(false);를 호출해서 이미지 출력 크기를 컨트롤에 맞춰서 출력되도록 해야 한다.
	void			set_back_image(CString type, UINT nIDBack, Gdiplus::Color cr_back = Gdiplus::Color::Transparent);
	void			set_back_image(CSCGdiplusBitmap& img, Gdiplus::Color cr_back = Gdiplus::Color::Transparent);

	//배경 이미지를 좌우대칭하는데 만약 animated gif라면 역재생처럼 동작시킬 수 있다.
	void			set_back_image_mirror(bool is_mirror);

	//배경 이미지 크기에 맞게 컨트롤을 resize한다.
	//fit_to_image = false이면 컨트롤의 크기에 맞게 이미지를 resize한다.
	void			fit_to_back_image(bool fit_to_image = true);
	//m_fit_to_back_image = false이면 컨트롤이 resize될 때 이미지도 resize된다.
	bool			m_fit_to_back_image = false;

	int				m_round = 0;				//round rect
	void			set_round(int round, Gdiplus::Color cr_border = Gdiplus::Color::Transparent, Gdiplus::Color gcr_parent_back = Gdiplus::Color::Transparent);

	void			get_auto_font_size(CWnd* pWnd, CRect r, CString text, LOGFONT *lf);

	//기본 CWnd::SetFont() override
	void			SetFont(CFont* font, BOOL bRedraw = TRUE);
	void			set_font_name(const CString& strFont, BYTE byCharSet = DEFAULT_CHARSET);
	void			set_font_size(int nFontSize);
	void			enlarge_font_size(bool enlarge);
	void			set_auto_font_size(bool auto_font_size = true);
	//default bold = FW_BOLD
	void			set_font_weight(int weight = FW_BOLD);
	void			set_font_underline(bool underline = true);
	void			set_font_italic(bool italic = true);
	void			set_font_antialiased(bool antiAliased = true);

	//void			set_font_width(int nFontWidth) { m_nFontWidth = nFontWidth; Invalidate(); }
	//bool			is_bold() { return m_bFontBold; }
	void			set_font(CFont* font);

	void			set_border_thick(int thick) { m_border_thick = thick; Invalidate(); }
	void			draw_border(bool draw, int thick, Gdiplus::Color cr_outline) { m_draw_border = draw; m_border_thick = thick; m_theme.cr_border = cr_outline; Invalidate(); }

	void			set_blink_time(int nTime0 = 400, int nTime1 = 1200);
	void			set_blink(BOOL bBlink = TRUE);
	void			set_text_alignment(DWORD dwStyle) { m_dwStyle = dwStyle; }

	//CStatic의 클릭 이벤트를 처리하려면 SS_NOTIFY 속성 필수이므로 자동 설정되도록 수정함.
	void			set_link(CString url, Gdiplus::Color cr_link = Gdiplus::Color::RoyalBlue);

	//static의 맨 앞에 nSpace 개수만큼의 공백을 추가하여 출력한다.(= left margin)
	void			set_prefix_space(int nSpace = 1) { m_nPrefixSpace = nSpace; Invalidate(); }

	void			set_margin(int l, int t, int r, int b) { set_margin(CRect(l, t, r, b)); }
	void			set_margin(CRect margin);
	void			get_margin(CSize sz_margin) { set_margin(CRect(sz_margin.cx, sz_margin.cy, sz_margin.cx, sz_margin.cy)); }

	void			set_icon(UINT nIDResource, int nSize = 16, bool left_align_fix = false);
	void			set_icon(HICON hIcon, int nSize = 16, bool left_align_fix = false);

	//png 이미지를 label의 앞에 표시한다. 2장 이상일 경우 alt효과를 줄 수 있다. id가 0이면 clear()로 동작한다.
	void			add_header_image(UINT id, bool left_align_fix = false);
	//png 이미지를 label의 앞에 표시한다. 2장 이상일 경우 alt효과를 줄 수 있다.
	template <typename ... T> void set_header_images(T... args)
	{
		int n = sizeof...(args);
		int arg[] = { args... };

		//set_이므로 기존 항목들을 모두 제거한 후 추가해야 한다.
		//그냥 하나를 추가한다면 add_header_image()를 사용한다.
		for (int i = 0; i < m_header_images.size(); i++)
		{
			CSCGdiplusBitmap* img = m_header_images[i];
			delete img;
		}

		m_header_images.clear();

		for (auto id : arg)
		{
			//여기서 직접 new로 할당받고 load하여 deque에 넣으려했으나
			//여기서는 들어간 것처럼 보였지만 실제 OnPaint()에서 보면 deque가 비어있었다.
			//template이라 그런지 여기서 바로는 들어가지 않는다.
			//멤버함수를 통해 넣어야 한다.
			add_header_image(id);
		}
	}

//animated gif 관련
	void			play_gif();
	//pos위치로 이동한 후 일시정지한다. -1이면 pause <-> play를 토글한다.
	void			pause_gif(int pos = 0);
	//animation thread가 종료되고 화면에도 더 이상 표시되지 않는다. 만약 그대로 멈추길 원한다면 pause_animation()을 호출한다.
	void			stop_gif();

	HICON			m_hIcon;
	CSize			m_sz_icon;

//tooltip
	//기본적인 툴팁은 이 컨트롤 내에서 지원하지만
	//disabled인 컨트롤은 main의 PreTranslateMessage()에서 처리하지 않으면 나타나지 않는다.
	void			use_tooltip(bool use) { m_use_tooltip = use; }
	void			set_tooltip_text(CString text);


protected:
	//SetWindowText(), GetWindowText()를 쓰면 m_text를 굳이 선언해서 사용안해도 될 듯 하지만
	//SetWindowText()를 호출하는 순간 화면갱신이 일어나고 MFC내부적으로는 많은 처리를 할 것이다.
	//특히 transparent일 경우는 그 깜빡임이 크므로 m_text를 별도 선언하여 사용한다.
	CString			m_text;

	//텍스트가 실제 출력될 영역의 크기를 알기 위해 m_rect_text를 추가했으나 현재 방식으로는 rc와 거의 흡사하다.
	//텍스트의 너비를 알고자 할 경우는 get_text_extent()를 사용해야 한다.
	//단, CSCParagraphStatic은 m_rect_text가 실제 출력되는 텍스트 영역값을 가지도록 구현되었다.
	CRect			m_rect_text;
	int				m_text_extent = 0;	//텍스트의 크기

	CSCEdit			m_edit;
	bool			m_use_edit = false;
	CString			m_text_value;		//m_use_edit = true이면 label + value로 표시되는데 이 때 value의 내용이 저장된다.
	int				m_edit_width = 0;	//편집모드일 때 edit의 너비. 이 값이 0이면 (m_text_extent + 8)부터 edit이 시작된다.

	enum ENUM_TIMER
	{
		TIMER_BLINK = 0,
		TIMER_MARQUEE,
	};

	CBitmap			m_Bmp;

	bool			m_auto_font_size = false;
	
	//font관련 변수는 모두 m_lf로 대체함.
	//CString			m_sFontName;
	//int				m_nFontSize;
	//int				m_nFontWidth;
	//bool			m_bFontBold;
	//bool			m_bFontUnderline;
	//bool			m_bFontAntiAliased;

	//gradient background fill
	std::deque<Gdiplus::Color> m_crGradient;
	bool			m_bGradient;		//gradient background?
	bool			m_bVertical;		//vertical gradient?


	//this function will be used only if msimg32.dll library is not available
	void			draw_gradient_rect(CDC *pDC, CRect r, Gdiplus::Color cLeft, Gdiplus::Color cRight, BOOL a_bVertical);


	bool			m_draw_border = false;
 	int				m_border_thick = 1;
	//Gdiplus::Color	m_cr_border = Gdiplus::Color::DimGray;

	bool			m_bSunken;			//default = false;

	//Gdiplus::Color	m_cr_text = Gdiplus::Color::Black;
	//Gdiplus::Color	m_cr_back = Gdiplus::Color::White;
	//Gdiplus::Color	m_cr_edit_text = gGRAY(32);	//편집모드일때 edit의 text color
	//Gdiplus::Color	m_cr_edit_back = m_cr_back;	//편집모드일때 edit의 back color. 기본값은 m_cr_back과 동일하다.
	bool			m_transparent;		//default = false
	//Gdiplus::Color	m_cr_parent_back = Gdiplus::Color::Transparent;

	BOOL			m_bBlink;
	BOOL			m_bBlinkStatus;
	int				m_nBlinkTime0;		//blink type is Show/Hide, time0 = shown duration, time1 = hidden duration in millisecond.
	int				m_nBlinkTime1;

	CString			m_link_url;
	Gdiplus::Color	m_cr_link = Gdiplus::Color::RoyalBlue;

	DWORD			m_dwStyle;
	int				m_nPrefixSpace;
	CSCGdiplusBitmap	m_img_back;

	int				m_halign = -1;
	int				m_valign = -1;

	LOGFONT			m_lf;
	CFont			m_font;
	void			update_surface();
	void			reconstruct_font();

	CRect			m_margin;			//텍스트의 여백을 설정한다. 기본값은 CStatic과 동일하게 0.

	//label의 앞에 그려질 이미지이며 만약 2개 이상일 경우 타이머에 의해 alt되기도 한다.
	std::deque<CSCGdiplusBitmap*> m_header_images;
	int				m_header_image_index;

	//텍스트 앞에 표시되는 아이콘 또는 헤더 이미지를 텍스트와 별개로 무조건 왼쪽에 그려줄 경우
	//CSCMessageBox에서 아이콘은 왼쪽에, 텍스트는 중앙정렬되어 표시되야 하므로 이 옵션이 추가됨.
	bool			m_image_left_align_fix = false;

//tooltip
	//enable상태일때는 잘 표시되나 disable일때는 표시되지 않는다.
	//이를 해결하려면 parent의 PreTranslateMessage()에서 처리해야 한다.
	//pointer 타입으로 선언한 이유는 동적생성시에도 툴팁을 적용하기 위해.
	CToolTipCtrl*	m_tooltip = NULL;
	//default = true
	bool			m_use_tooltip = true;
	CString			m_tooltip_text = _T("");

	//정적으로 만든 컨트롤은 문제없으나 동적으로 컨트롤을 생성하여 사용하는 경우
	//PreSubclassWindow()에서 툴팁을 초기화하려니 예외가 발생함.
	//그래서 Create()후에 별도로 prepare_tooltip()을 호출하여 준비되도록 수정.
	//동적 생성한 컨트롤에서도 정상 표시됨을 확인함.
	void			prepare_tooltip();


protected:
	//{{AFX_MSG(CSCStatic)
	afx_msg void OnPaint();
	afx_msg BOOL OnEraseBkgnd(CDC* pDC);
	afx_msg void OnTimer(UINT_PTR nIDEvent);
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	virtual void PreSubclassWindow();

	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

public:
	afx_msg void OnWindowPosChanged(WINDOWPOS* lpwndpos);
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg BOOL OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message);
	afx_msg void OnDestroy();
};
#endif
