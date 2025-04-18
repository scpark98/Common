#if !defined(AFX_SCSTATIC_INCLUDED_)
#define AFX_SCSTATIC_INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#pragma once

#include <afxwin.h>
#include <deque>

//.ico가 아닌 png 이미지들을 앞에 그려주고 필요에 따라 변경되도록 하기 위해 사용.
#include "../../GdiplusBitmap.h"

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


[2014-11-26]

[수정될 내용]
- 배경이 있는 앱에서는 투명이 잘 적용되지만 기본 스타일의 dlg에서는 화면 갱신이 잘 되지 않는다.
  우선 기본 스타일의 dlg에서는 배경색을 지정해주자.
- 출력크기보다 rc가 작으면 키워줘야 한다?
- "\r\n"으로 multiline을 출력할 경우 center image, no wrap, simple 속성은 반드시 false가 되어야 한다.
  따라서 이러한 속성을 살리기 위해서는 파싱하여 각 라인을 출력하도록 수정이 필요하다.

[2014-11-27]
- blink시에 아이콘도 함께 적용되지 않던 문제 수정
- blink를 false로 할 경우 숨겨진 상태로 blink가 종료되던 문제 수정

*/

// CSCStatic

typedef UINT (CALLBACK* LPFNDLLFUNC1)(HDC,CONST PTRIVERTEX,DWORD,CONST PVOID,DWORD,DWORD);

class CSCStatic : public CStatic
{
	DECLARE_DYNAMIC(CSCStatic)

public:
	CSCStatic();
	~CSCStatic();

	//CParagraph

	//동적생성시에 사용
	BOOL			create(LPCTSTR lpszText, DWORD dwStyle, const RECT& rect, CWnd* pParentWnd, UINT nID = 0xffff);
	
	void			set_transparent(bool bTransparent = true, Gdiplus::Color cr_parent_back = Gdiplus::Color::Transparent);
	void			SetWindowText(CString sText) { set_text(sText); }
	CRect			set_text(CString sText, Gdiplus::Color cr_text = Gdiplus::Color::Transparent);

	//printf()와 같이 변수값을 바로 문자열로 세팅할 수 있다.
	//컬러값을 변경하지 않으려면 cr_text = -1로 넘겨준다.
	void			set_textf(Gdiplus::Color cr_text, LPCTSTR format, ...);

	//CStatic의 SS_LEFT, SS_RIGHT 등의 align 설정값을 DrawText()에서 사용하는 DT_LEFT, DT_RIGHT 등의
	DWORD			get_text_align();

	CRect			get_text_rect() { return m_rect_text; }

	//글자색, 배경색 동시 설정
	void			set_color(Gdiplus::Color cr_text, Gdiplus::Color cr_back = Gdiplus::Color::Transparent);

	//글자색만 변경할 경우
	void			set_text_color(Gdiplus::Color crTextColor);

	Gdiplus::Color	get_back_color() { return m_cr_back; }

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

	//배경 이미지를 좌우대칭하는데 만약 animated gif라면 역재생처럼 동작시킬 수 있다.
	void			set_back_image_mirror(bool is_mirror);

	//배경 이미지 크기에 맞게 컨트롤을 resize한다.
	//fit_to_image = false이면 컨트롤의 크기에 맞게 이미지를 resize한다.
	void			fit_to_back_image(bool fit_to_image = true);
	//m_fit_to_back_image = false이면 컨트롤이 resize될 때 이미지도 resize된다.
	bool			m_fit_to_back_image = false;

	int				m_round = 0;				//round rect
	void			set_round(int round, Gdiplus::Color gcr_border = Gdiplus::Color::Transparent, Gdiplus::Color gcr_parent_back = Gdiplus::Color::Transparent);

	void			get_auto_font_size(CWnd* pWnd, CRect r, CString text, LOGFONT *lf);

	void			set_font_name(const CString& strFont, BYTE byCharSet = DEFAULT_CHARSET);
	void			set_font_size(int nFontSize);
	void			set_auto_font_size(bool auto_font_size = true);
	void			set_font_bold(int weight = FW_BOLD);
	void			set_font_underline(bool underline = true);
	void			set_font_italic(bool italic = true);
	void			set_font_antialiased(bool antiAliased = true);

	void			set_font_width(int nFontWidth) { m_nFontWidth = nFontWidth; Invalidate(); }
	bool			is_bold() { return m_bFontBold; }
	void			set_font(CFont* font);

	void			set_border_thick(int thick) { m_border_thick = thick; Invalidate(); }
	void			draw_border(bool draw, int thick, Gdiplus::Color gcr_outline) { m_draw_border = draw; m_border_thick = thick; m_gcr_border = gcr_outline; Invalidate(); }

	void			set_blink_time(int nTime0 = 400, int nTime1 = 1200);
	void			set_blink(BOOL bBlink = TRUE);
	void			set_text_alignment(DWORD dwStyle) { m_dwStyle = dwStyle; }

	//CStatic의 클릭 이벤트를 처리하려면 SS_NOTIFY 속성 필수이므로 자동 설정되도록 수정함.
	void			set_link(CString url, Gdiplus::Color cr_link = Gdiplus::Color::RoyalBlue);

	//static의 맨 앞에 nSpace 개수만큼의 공백을 추가하여 출력한다.(= left margin)
	void			set_prefix_space(int nSpace = 1) { m_nPrefixSpace = nSpace; Invalidate(); }

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
			CGdiplusBitmap* img = m_header_images[i];
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
	void	play_gif();
	//pos위치로 이동한 후 일시정지한다. -1이면 pause <-> play를 토글한다.
	void	pause_gif(int pos = 0);
	//animation thread가 종료되고 화면에도 더 이상 표시되지 않는다. 만약 그대로 멈추길 원한다면 pause_animation()을 호출한다.
	void	stop_gif();


protected:
	//SetWindowText(), GetWindowText()를 쓰면 m_text를 굳이 선언해서 사용안해도 될 듯 하지만
	//SetWindowText()를 호출하는 순간 화면갱신이 일어나고 MFC내부적으로는 많은 처리를 할 것이다.
	//특히 transparent일 경우는 그 깜빡임이 크므로 m_text를 별도 선언하여 사용한다.
	CString			m_text;

	//텍스트가 실제 출력될 영역의 크기인데 이 CSCStatic을 사용하는 dlg에서 텍스트 출력크기에 따라 dlg의 크기가 변경되어야 하는
	//CSCMessageBox와 같은 클래스에서 필요하다.
	//CSCMessageBox에서 DT_CALC로 그 출력 크기를 얻어오려 했으나 관련 자료를 검색해 본 결과
	//이는 불가능하며 직접 출력시킨 후 그 크기를 알 수 있다.
	//get_text_rect() 함수를 사용하여 실제 출력되는 사각형 영역 정보를 알 수 있다.
	CRect			m_rect_text;

	enum ENUM_TIMER
	{
		TIMER_BLINK = 0,
		TIMER_MARQUEE,
	};

	CBitmap			m_Bmp;
	CString			m_sFontName;
	int				m_nFontSize;
	bool			m_auto_font_size = false;
	int				m_nFontWidth;
	bool			m_bFontBold;
	bool			m_bFontUnderline;
	bool			m_bFontAntiAliased;

	//gradient background fill
	std::deque<Gdiplus::Color> m_crGradient;
	bool			m_bGradient;		//gradient background?
	bool			m_bVertical;		//vertical gradient?


	//this function will be used only if msimg32.dll library is not available
	void			draw_gradient_rect(CDC *pDC, CRect r, Gdiplus::Color cLeft, Gdiplus::Color cRight, BOOL a_bVertical);


	bool			m_draw_border = false;
 	int				m_border_thick;
	Gdiplus::Color	m_gcr_border;

	bool			m_bSunken;			//default = false;

	Gdiplus::Color	m_cr_text;
	Gdiplus::Color	m_cr_back;
	bool			m_transparent;		//default = false
	Gdiplus::Color	m_cr_parent_back = Gdiplus::Color::Transparent;

	BOOL			m_bBlink;
	BOOL			m_bBlinkStatus;
	int				m_nBlinkTime0;		//blink type is Show/Hide, time0 = shown duration, time1 = hidden duration in millisecond.
	int				m_nBlinkTime1;

	CString			m_link_url;
	Gdiplus::Color	m_cr_link = Gdiplus::Color::RoyalBlue;

	DWORD			m_dwStyle;
	int				m_nPrefixSpace;
	CGdiplusBitmap	m_img_back;


	LOGFONT			m_lf;
	CFont			m_font;
	void			update_surface();
	void			reconstruct_font();


	HICON			m_hIcon;
	CSize			m_szIcon;

	//label의 앞에 그려질 이미지이며 만약 2개 이상일 경우 타이머에 의해 alt되기도 한다.
	std::deque<CGdiplusBitmap*> m_header_images;
	int				m_header_image_index;

	//텍스트 앞에 표시되는 아이콘 또는 헤더 이미지를 텍스트와 별개로 무조건 왼쪽에 그려줄 경우
	//CSCMessageBox에서 아이콘은 왼쪽에, 텍스트는 중앙정렬되어 표시되야 하므로 이 옵션이 추가됨.
	bool			m_image_left_align_fix = false;

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
};
#endif
