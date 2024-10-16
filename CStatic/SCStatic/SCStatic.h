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

[수정될 내용]
- 배경이 있는 앱에서는 투명이 잘 적용되지만 기본 스타일의 dlg에서는 화면 갱신이 잘 되지 않는다.
  우선 기본 스타일의 dlg에서는 배경색을 지정해주자.
- 출력크기보다 rc가 작으면 키워줘야 한다.
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
	BOOL		create(LPCTSTR lpszText, DWORD dwStyle, const RECT& rect, CWnd* pParentWnd, UINT nID = 0xffff);
	
	void		set_transparent(bool bTransparent = true) { m_transparent = bTransparent; Invalidate(); }
	void		SetWindowText(CString sText) { set_text(sText); }
	void		set_text(CString sText, Gdiplus::Color cr_text = Gdiplus::Color::Transparent);
	void		set_textf(Gdiplus::Color crTextColor, LPCTSTR format, ...);

	//글자색, 배경색 동시 설정
	void		set_color(Gdiplus::Color cr_text, Gdiplus::Color cr_back = Gdiplus::Color::Transparent);

	//글자색만 변경할 경우
	void		set_text_color(Gdiplus::Color crTextColor);

	Gdiplus::Color	get_back_color() { return m_cr_back; }

	//배경색을 지정하면 투명 옵션은 off됨.
	void		set_back_color(Gdiplus::Color cBackColor) { m_cr_back = cBackColor; m_transparent = false; Invalidate(); }

	//gradient
	//m_crGradient deque에 색상들을 넣고 n등분하여 gradient fill을 해서 표시한다.
	CSCStatic&		set_gradient(bool bGradient = true);
	CSCStatic&		set_gradient_color(Gdiplus::Color crGradient);			//배경~crGradient 2컬러로 표현됨.
	CSCStatic&		set_gradient_color(int idx, Gdiplus::Color crGradient);	//idx의 색상을 crGradient로 변경. idx가 없으면 idx까지 crGradient로 채움.

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

	CSCStatic&		add_gradient_color(Gdiplus::Color crGradient);			//색상 추가
	CSCStatic&		insert_gradient_color(int idx, Gdiplus::Color crGradient);//idx번째 색상 추가
	CSCStatic&		set_vertical_gradient(bool bVertical = true);

	void			sunken(bool sunken = true) { m_bSunken = sunken; Invalidate(); }


//marquee효과를 위해서 간단히 테스트해 보았으나 부드럽게 표현하기 위해서는
//rc의 width 또는 height 크기에 적당할 정도로 m_crGradient를 자동으로 채워준 후에 돌려야한다.
//필요성이 있나 싶어서 일단 보류한다.
	//start = 1:start, 0:stop, -1:toggle
	bool			m_start_marquee;
	//void			start_marquee(int start);

	void			set_back_image(UINT nIDBack);

	void			set_round_head(bool bRound);

	void			get_auto_font_size(CWnd* pWnd, CRect r, CString text, LOGFONT *lf);

	virtual CSCStatic& set_font_name(const CString& strFont, BYTE byCharSet = DEFAULT_CHARSET);
	virtual CSCStatic& set_font_size(int nFontSize);
	virtual CSCStatic& set_auto_font_size(bool auto_font_size = true);
	virtual CSCStatic& set_font_bold(bool bold = true);
	virtual CSCStatic& set_font_underline(bool underline = true);
	virtual CSCStatic& set_font_italic(bool italic = true);
	virtual CSCStatic& set_font_antialiased(bool antiAliased = true);

	void		set_font_width(int nFontWidth) { m_nFontWidth = nFontWidth; Invalidate(); }
	bool		is_bold() { return m_bFontBold; }
	void		set_font(CFont* font);

	void		set_outline_width(int nWidth) { m_nOutlineWidth = nWidth; Invalidate(); }
	void		set_outline(int nWidth, Gdiplus::Color crOutline) { m_nOutlineWidth = nWidth; m_crOutline = crOutline; Invalidate(); }

	void		set_blink_time(int nTime0 = 400, int nTime1 = 1200);
	void		set_blink(BOOL bBlink = TRUE);
	void		set_text_alignment(DWORD dwStyle) { m_dwStyle = dwStyle; }

	//static의 맨 앞에 nSpace 개수만큼의 공백을 추가하여 출력한다.(= left margin)
	void		set_prefix_space(int nSpace = 1) { m_nPrefixSpace = nSpace; Invalidate(); }

	void		set_icon(UINT nIDResource, int nSize = 16);

	//png 이미지를 label의 앞에 표시한다. 2장 이상일 경우 alt효과를 줄 수 있다. id가 0이면 clear()로 동작한다.
	void		add_header_image(UINT id);
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

protected:
	//SetWindowText(), GetWindowText()를 쓰면 m_text를 굳이 선언해서 사용안해도 될 듯 하지만
	//SetWindowText()를 호출하는 순간 화면갱신이 일어나고 MFC내부적으로는 많은 처리를 할 것이다.
	//특히 transparent일 경우는 그 깜빡임이 크므로 m_text를 별도 선언하여 사용한다.
	CString		m_text;

	enum ENUM_TIMER
	{
		TIMER_BLINK = 0,
		TIMER_MARQUEE,
	};

	CBitmap		m_Bmp;
	CString		m_sFontName;
	int			m_nFontSize;
	bool		m_auto_font_size = false;
	int			m_nFontWidth;
	bool		m_bFontBold;
	bool		m_bFontUnderline;
	bool		m_bFontAntiAliased;

	//gradient background fill
	std::deque<Gdiplus::Color> m_crGradient;
	bool		m_bGradient;		//gradient background?
	bool		m_bVertical;		//vertical gradient?


	//this function will be used only if msimg32.dll library is not available
	void		draw_gradient_rect(CDC *pDC, CRect r, Gdiplus::Color cLeft, Gdiplus::Color cRight, BOOL a_bVertical);


 	int			m_nOutlineWidth;	//두께가 1 이상이면 외곽선으로 그려진다.
	Gdiplus::Color	m_crOutline;		//외곽선 색상
	bool		m_bSunken;			//default = false;

	Gdiplus::Color	m_cr_text;
	Gdiplus::Color	m_cr_back;
	bool		m_transparent;		//default = false

	BOOL		m_bBlink;
	BOOL		m_bBlinkStatus;
	int			m_nBlinkTime0;		//blink type is Show/Hide, time0 = shown duration, time1 = hidden duration in millisecond.
	int			m_nBlinkTime1;

	DWORD		m_dwStyle;
	int			m_nPrefixSpace;
	CImageList	m_ImageBack;


	LOGFONT		m_lf;
	CFont		m_font;
	void		update_surface();
	void		reconstruct_font();


	HICON		m_hIcon;
	CSize		m_szIcon;

	//label의 앞에 그려질 이미지이며 만약 2개 이상일 경우 타이머에 의해 alt되기도 한다.
	std::deque<CGdiplusBitmap*> m_header_images;
	int			m_header_image_index;

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
};
#endif
