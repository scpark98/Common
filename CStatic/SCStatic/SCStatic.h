#if !defined(AFX_SCSTATIC_INCLUDED_)
#define AFX_SCSTATIC_INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#pragma once

#include <afxwin.h>
#include <deque>

//.ico�� �ƴ� png �̹������� �տ� �׷��ְ� �ʿ信 ���� ����ǵ��� �ϱ� ���� ���.
#include "../../GdiplusBitmap.h"

/*
//scpark

[gradient usage]
m_static1.set_gradient();
m_static1.set_gradient_color(RED);
//m_static1.set_vertical_gradient);
//m_static1.set_gradient_color(4, RED, GREEN, YELLOW, BLUE);

[���ǻ���]
- parent�� WM_SIZE�� ���� CSCStatic�� �ν��Ͻ��� resize�� ��� �����δٸ�
  parent �Ӽ����� Clip_Children�� true�� �����ؾ� �������� �ʴ´�.

[������ ����]
- ����� �ִ� �ۿ����� ������ �� ��������� �⺻ ��Ÿ���� dlg������ ȭ�� ������ �� ���� �ʴ´�.
  �켱 �⺻ ��Ÿ���� dlg������ ������ ����������.
- ���ũ�⺸�� rc�� ������ Ű����� �Ѵ�.
- "\r\n"���� multiline�� ����� ��� center image, no wrap, simple �Ӽ��� �ݵ�� false�� �Ǿ�� �Ѵ�.
  ���� �̷��� �Ӽ��� �츮�� ���ؼ��� �Ľ��Ͽ� �� ������ ����ϵ��� ������ �ʿ��ϴ�.

[2014-11-27]
- blink�ÿ� �����ܵ� �Բ� ������� �ʴ� ���� ����
- blink�� false�� �� ��� ������ ���·� blink�� ����Ǵ� ���� ����

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

	//���������ÿ� ���
	BOOL			create(LPCTSTR lpszText, DWORD dwStyle, const RECT& rect, CWnd* pParentWnd, UINT nID = 0xffff);
	
	void			set_transparent(bool bTransparent = true) { m_transparent = bTransparent; Invalidate(); }
	void			SetWindowText(CString sText) { set_text(sText); }
	void			set_text(CString sText, Gdiplus::Color cr_text = Gdiplus::Color::Transparent);
	void			set_textf(Gdiplus::Color crTextColor, LPCTSTR format, ...);

	//���ڻ�, ���� ���� ����
	void			set_color(Gdiplus::Color cr_text, Gdiplus::Color cr_back = Gdiplus::Color::Transparent);

	//���ڻ��� ������ ���
	void			set_text_color(Gdiplus::Color crTextColor);

	Gdiplus::Color	get_back_color() { return m_cr_back; }

	//������ �����ϸ� ���� �ɼ��� off��.
	void			set_back_color(Gdiplus::Color cBackColor) { m_cr_back = cBackColor; m_transparent = false; Invalidate(); }

	//gradient
	//m_crGradient deque�� ������� �ְ� n����Ͽ� gradient fill�� �ؼ� ǥ���Ѵ�.
	void			set_gradient(bool bGradient = true);
	void			set_gradient_color(Gdiplus::Color crGradient);			//���~crGradient 2�÷��� ǥ����.
	void			set_gradient_color(int idx, Gdiplus::Color crGradient);	//idx�� ������ crGradient�� ����. idx�� ������ idx���� crGradient�� ä��.

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

	void			add_gradient_color(Gdiplus::Color crGradient);			//���� �߰�
	void			insert_gradient_color(int idx, Gdiplus::Color crGradient);//idx��° ���� �߰�
	void			set_vertical_gradient(bool bVertical = true);

	void			sunken(bool sunken = true) { m_bSunken = sunken; Invalidate(); }


//marqueeȿ���� ���ؼ� ������ �׽�Ʈ�� �������� �ε巴�� ǥ���ϱ� ���ؼ���
//rc�� width �Ǵ� height ũ�⿡ ������ ������ m_crGradient�� �ڵ����� ä���� �Ŀ� �������Ѵ�.
//�ʿ伺�� �ֳ� �; �ϴ� �����Ѵ�.
	//start = 1:start, 0:stop, -1:toggle
	bool			m_start_marquee;
	//void			start_marquee(int start);

	//gif�� ����.
	//m_static_copy.set_back_image(_T("GIF"), IDR_GIF_COPY, Gdiplus::Color::White);
	void			set_back_image(CString type, UINT nIDBack, Gdiplus::Color cr_back = Gdiplus::Color::Transparent);

	//��� �̹����� �¿��Ī�ϴµ� ���� animated gif��� �����ó�� ���۽�ų �� �ִ�.
	void			set_back_image_mirror(bool is_mirror);

	//��� �̹��� ũ�⿡ �°� ��Ʈ���� resize�Ѵ�.
	//fit_to_image = false�̸� ��Ʈ���� ũ�⿡ �°� �̹����� resize�Ѵ�.
	void			fit_to_back_image(bool fit_to_image = true);
	//m_fit_to_back_image = false�̸� ��Ʈ���� resize�� �� �̹����� resize�ȴ�.
	bool			m_fit_to_back_image = false;

	void			set_round_head(bool bRound);

	void			get_auto_font_size(CWnd* pWnd, CRect r, CString text, LOGFONT *lf);

	void			set_font_name(const CString& strFont, BYTE byCharSet = DEFAULT_CHARSET);
	void			set_font_size(int nFontSize);
	void			set_auto_font_size(bool auto_font_size = true);
	void			set_font_bold(bool bold = true);
	void			set_font_underline(bool underline = true);
	void			set_font_italic(bool italic = true);
	void			set_font_antialiased(bool antiAliased = true);

	void			set_font_width(int nFontWidth) { m_nFontWidth = nFontWidth; Invalidate(); }
	bool			is_bold() { return m_bFontBold; }
	void			set_font(CFont* font);

	void			set_outline_width(int nWidth) { m_nOutlineWidth = nWidth; Invalidate(); }
	void			set_outline(int nWidth, Gdiplus::Color crOutline) { m_nOutlineWidth = nWidth; m_crOutline = crOutline; Invalidate(); }

	void			set_blink_time(int nTime0 = 400, int nTime1 = 1200);
	void			set_blink(BOOL bBlink = TRUE);
	void			set_text_alignment(DWORD dwStyle) { m_dwStyle = dwStyle; }

	//CStatic�� Ŭ�� �̺�Ʈ�� ó���Ϸ��� SS_NOTIFY �Ӽ� �ʼ��̹Ƿ� �ڵ� �����ǵ��� ����
	void			set_link(CString url, Gdiplus::Color cr_link = Gdiplus::Color::RoyalBlue);

	//static�� �� �տ� nSpace ������ŭ�� ������ �߰��Ͽ� ����Ѵ�.(= left margin)
	void			set_prefix_space(int nSpace = 1) { m_nPrefixSpace = nSpace; Invalidate(); }

	void			set_icon(UINT nIDResource, int nSize = 16);

	//png �̹����� label�� �տ� ǥ���Ѵ�. 2�� �̻��� ��� altȿ���� �� �� �ִ�. id�� 0�̸� clear()�� �����Ѵ�.
	void			add_header_image(UINT id);
	//png �̹����� label�� �տ� ǥ���Ѵ�. 2�� �̻��� ��� altȿ���� �� �� �ִ�.
	template <typename ... T> void set_header_images(T... args)
	{
		int n = sizeof...(args);
		int arg[] = { args... };

		//set_�̹Ƿ� ���� �׸���� ��� ������ �� �߰��ؾ� �Ѵ�.
		//�׳� �ϳ��� �߰��Ѵٸ� add_header_image()�� ����Ѵ�.
		for (int i = 0; i < m_header_images.size(); i++)
		{
			CGdiplusBitmap* img = m_header_images[i];
			delete img;
		}

		m_header_images.clear();

		for (auto id : arg)
		{
			//���⼭ ���� new�� �Ҵ�ް� load�Ͽ� deque�� ������������
			//���⼭�� �� ��ó�� �������� ���� OnPaint()���� ���� deque�� ����־���.
			//template�̶� �׷��� ���⼭ �ٷδ� ���� �ʴ´�.
			//����Լ��� ���� �־�� �Ѵ�.
			add_header_image(id);
		}
	}

//animated gif ����
	void	play_animation();
	//pos��ġ�� �̵��� �� �Ͻ������Ѵ�. -1�̸� pause <-> play�� ����Ѵ�.
	void	pause_animation(int pos = 0);
	//animation thread�� ����ǰ� ȭ�鿡�� �� �̻� ǥ�õ��� �ʴ´�. ���� �״�� ���߱� ���Ѵٸ� pause_animation()�� ȣ���Ѵ�.
	void	stop_animation();


protected:
	//SetWindowText(), GetWindowText()�� ���� m_text�� ���� �����ؼ� �����ص� �� �� ������
	//SetWindowText()�� ȣ���ϴ� ���� ȭ�鰻���� �Ͼ�� MFC���������δ� ���� ó���� �� ���̴�.
	//Ư�� transparent�� ���� �� �������� ũ�Ƿ� m_text�� ���� �����Ͽ� ����Ѵ�.
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


 	int			m_nOutlineWidth;	//�β��� 1 �̻��̸� �ܰ������� �׷�����.
	Gdiplus::Color	m_crOutline;		//�ܰ��� ����
	bool		m_bSunken;			//default = false;

	Gdiplus::Color	m_cr_text;
	Gdiplus::Color	m_cr_back;
	bool		m_transparent;		//default = false

	BOOL		m_bBlink;
	BOOL		m_bBlinkStatus;
	int			m_nBlinkTime0;		//blink type is Show/Hide, time0 = shown duration, time1 = hidden duration in millisecond.
	int			m_nBlinkTime1;

	CString		m_link_url;
	Gdiplus::Color m_cr_link = Gdiplus::Color::RoyalBlue;

	DWORD		m_dwStyle;
	int			m_nPrefixSpace;
	CGdiplusBitmap	m_img_back;


	LOGFONT		m_lf;
	CFont		m_font;
	void		update_surface();
	void		reconstruct_font();


	HICON		m_hIcon;
	CSize		m_szIcon;

	//label�� �տ� �׷��� �̹����̸� ���� 2�� �̻��� ��� Ÿ�̸ӿ� ���� alt�Ǳ⵵ �Ѵ�.
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
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg BOOL OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message);
};
#endif
