#if !defined(AFX_SCSTATIC_INCLUDED_)
#define AFX_SCSTATIC_INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#pragma once

#include <afxwin.h>
#include <deque>

//.ico�� �ƴ� png �̹������� �տ� �׷��ְ� �ʿ信 ���� ����ǵ��� �ϱ� ���� ���.
#include "../../SCGdiplusBitmap.h"
#include "../../data_structure/SCParagraph/SCParagraph.h"

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

[����ó�� ��Ģ]
- �簢�� ���� ������ �������� ���������� ó���ȴ�.
- m_round > 0 �̰ų� ���� �̹����� ǥ���� ���� ���� parent_back�� ĥ����� �Ѵ�.

- parent�� ��� �׸��� ���� ���� parent_back_image�� �˰� �־�� �ϰ� ��Ʈ���� ��ġ�� �ش��ϴ� �̹����� crop�ؼ�
  ���� �׷��� �Ŀ� static�� ǥ���ؾ� �Ѵ�.
  ���� parent�� resize�� �����ϰų� parent_back_image�� 1:1ũ��� �׷��� �� �ƴ϶�� �̸����� �Ұ��ϴ�.
  �̷� ��� parent DC�� ���ͼ� �ش� ��� ������ �����ϰ� �̸� �׷��� �� static�� �׷��ִ� �ڵ带 ����߾�����
  �������� �߻��ϱ⵵ �ϰ� ���� �̹����� ����Ἥ �׸��� �̺�Ʈ�� �߻��Ǹ� ���������� ���� ������������ ���ۿ��� �ִ�.
  ��� �׸��� �ְų� �׸��� ũ�Ⱑ 1:1�� �ƴ� ���� �켱 ���� ���������� �����Ų��.

[�̹��� �߰� ��]
- set_icon()�� �̿��ؼ� �ؽ�Ʈ �տ� �������� ǥ���� �� �ִ�.
- add_header_image()�� �̿��ؼ� �ؽ�Ʈ �տ� �̹����� ǥ���� �� �ִ�.
  set_icon()�� �ٸ� ���� .ico ������ �ƴ� png ���� �̹��� ������ �����ϰ�
  2�� �̻��� �̹����� �߰��� �� �ʿ信 ���� �̹����� ������ ���� �ִ�.
- �� �Լ� ��� left_align_fix�� true�� �����ϸ� �̹����� ���� ���ķ� �����ǰ� �ؽ�Ʈ�� ������ ���Ĺ������ ǥ�õȴ�.
  �� �Ķ���͸� �߰��� ���� CSCMessageBox���� �� CSCStatic�� ����ϴµ� �޽����ڽ��� ���� �������� �׻� ���ʿ�, �ؽ�Ʈ�� ���� �Ǵ� �߾����ķ� ǥ�õȴ�.
  false�� ���� �ؽ�Ʈ�� ��ġ�� ���� �����ܰ� �̹����� ��ġ�� ����ȴ�.



[2014-11-26]

[������ ����]
- CSCStatic�� CSCParagraphStatic�� ��ģ��? CSCTagStatic �̸�����?
  tag�� ���ԵǾ����Ŀ� ���� OnPaint() ���� �޶�����.

- ����� �ִ� �ۿ����� ������ �� ��������� �⺻ ��Ÿ���� dlg������ ȭ�� ������ �� ���� �ʴ´�.
  �켱 �⺻ ��Ÿ���� dlg������ ������ ����������.
- ���ũ�⺸�� rc�� ������ Ű����� �Ѵ�?
- "\r\n"���� multiline�� ����� ��� center image, no wrap, simple �Ӽ��� �ݵ�� false�� �Ǿ�� �Ѵ�.
  ���� �̷��� �Ӽ��� �츮�� ���ؼ��� �Ľ��Ͽ� �� ������ ����ϵ��� ������ �ʿ��ϴ�.

- set_text(_T("<cr=red>test</cr>"))�� ���� html �±׸� �����Ϸ���? <b>, <i>, <u>, <s> ���� ���� ����.
  �±װ� �ִ� �ؽ�Ʈ�� ���� �ؽ�Ʈ�� ȥ���Ͽ� ����� ���� paragraph ����ü�� �ʿ��ϴ�.

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


	//���������ÿ� ���
	BOOL			create(LPCTSTR lpszText, DWORD dwStyle, const RECT& rect, CWnd* pParentWnd, UINT nID = 0xffff);
	
	void			set_transparent(bool bTransparent = true, Gdiplus::Color cr_parent_back = Gdiplus::Color::Transparent);
	void			SetWindowText(CString sText) { set_text(sText); }

	//text�� �Ϲ� ���ڿ� �Ǵ� tag�� ���Ե� ���ڿ��� �� �� �ִ�.
	CRect			set_text(CString text, Gdiplus::Color cr_text = Gdiplus::Color::Transparent);

	//printf()�� ���� �������� �ٷ� ���ڿ��� ������ �� �ִ�.
	//�ʱ� ���������� ù��° �Ķ���ͷ� �÷��� �ְ� �� ���� -1�̸� default text color�� ����ϵ��� �����߾�����
	//-1 �񱳰� ����� ���� �ʾ� �켱 ������.
	void			set_textf(LPCTSTR format, ...);

	//CStatic�� SS_LEFT, SS_RIGHT ���� align �������� DrawText()���� ����ϴ� DT_LEFT, DT_RIGHT ������ ġȯ�Ͽ� ����
	DWORD			get_text_align();

	//VALIGN�� SS_CENTERIMAGE�� �ƴϸ� ������� �� ���̹Ƿ� DT_TOP, DT_VCENTER, DT_BOTTOM���� ������ �� �ֵ��� Ȯ���Ѵ�.
	int				get_halign();
	int				get_valign();
	//���� �̱���
	//void			set_halign();
	//void			set_valign();

	//�ؽ�Ʈ�� ���� ��µ� ������ ũ�⸦ �˱� ���� m_rect_text�� �߰������� ���� ������δ� rc�� ���� ������ ���� ���ϵȴ�.
	//�ؽ�Ʈ�� �ʺ� �˰��� �� ���� get_text_extent()�� ����ؾ� �Ѵ�.
	CRect			get_text_rect() { return m_rect_text; }
	//�ؽ�Ʈ�� �ʺ�, ���̸� �˰��� �� ��� ���.
	CSize			get_text_extent();

	//parent���� ���� �� static�� ��ġ�� ����.
	CRect			get_rect();

	//���ڻ�, ���� ���� ����
	void			set_color(Gdiplus::Color cr_text, Gdiplus::Color cr_back = Gdiplus::Color::Transparent);

	//���ڻ��� ������ ���. �� ���� Transparent�̸� �⺻ ���ڻ����� �����Ѵٴ� �ǹ̷� ���ȴ�.
	void			set_text_color(Gdiplus::Color cr_text = Gdiplus::Color::Transparent);

	Gdiplus::Color	get_text_color() { return m_cr_text; }
	Gdiplus::Color	get_back_color() { return m_cr_back; }

	void			set_back_color(Gdiplus::Color cr_back);

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
	//���� ���� ũ�Ⱑ static ctrl�� ũ�⺸�� Ŭ ���� ������ �ʰ� �ǹǷ�
	//m_static_copy.fit_to_back_image(false);�� ȣ���ؼ� �̹��� ��� ũ�⸦ ��Ʈ�ѿ� ���缭 ��µǵ��� �ؾ� �Ѵ�.
	void			set_back_image(CString type, UINT nIDBack, Gdiplus::Color cr_back = Gdiplus::Color::Transparent);
	void			set_back_image(CSCGdiplusBitmap& img, Gdiplus::Color cr_back = Gdiplus::Color::Transparent);

	//��� �̹����� �¿��Ī�ϴµ� ���� animated gif��� �����ó�� ���۽�ų �� �ִ�.
	void			set_back_image_mirror(bool is_mirror);

	//��� �̹��� ũ�⿡ �°� ��Ʈ���� resize�Ѵ�.
	//fit_to_image = false�̸� ��Ʈ���� ũ�⿡ �°� �̹����� resize�Ѵ�.
	void			fit_to_back_image(bool fit_to_image = true);
	//m_fit_to_back_image = false�̸� ��Ʈ���� resize�� �� �̹����� resize�ȴ�.
	bool			m_fit_to_back_image = false;

	int				m_round = 0;				//round rect
	void			set_round(int round, Gdiplus::Color gcr_border = Gdiplus::Color::Transparent, Gdiplus::Color gcr_parent_back = Gdiplus::Color::Transparent);

	void			get_auto_font_size(CWnd* pWnd, CRect r, CString text, LOGFONT *lf);

	void			set_font_name(const CString& strFont, BYTE byCharSet = DEFAULT_CHARSET);
	void			set_font_size(int nFontSize);
	void			enlarge_font_size(bool enlarge);
	void			set_auto_font_size(bool auto_font_size = true);
	//default bold = FW_BOLD
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

	//CStatic�� Ŭ�� �̺�Ʈ�� ó���Ϸ��� SS_NOTIFY �Ӽ� �ʼ��̹Ƿ� �ڵ� �����ǵ��� ������.
	void			set_link(CString url, Gdiplus::Color cr_link = Gdiplus::Color::RoyalBlue);

	//static�� �� �տ� nSpace ������ŭ�� ������ �߰��Ͽ� ����Ѵ�.(= left margin)
	void			set_prefix_space(int nSpace = 1) { m_nPrefixSpace = nSpace; Invalidate(); }

	void			set_margin(int l, int t, int r, int b) { set_margin(CRect(l, t, r, b)); }
	void			set_margin(CRect margin);
	void			get_margin(CSize sz_margin) { set_margin(CRect(sz_margin.cx, sz_margin.cy, sz_margin.cx, sz_margin.cy)); }

	void			set_icon(UINT nIDResource, int nSize = 16, bool left_align_fix = false);
	void			set_icon(HICON hIcon, int nSize = 16, bool left_align_fix = false);

	//png �̹����� label�� �տ� ǥ���Ѵ�. 2�� �̻��� ��� altȿ���� �� �� �ִ�. id�� 0�̸� clear()�� �����Ѵ�.
	void			add_header_image(UINT id, bool left_align_fix = false);
	//png �̹����� label�� �տ� ǥ���Ѵ�. 2�� �̻��� ��� altȿ���� �� �� �ִ�.
	template <typename ... T> void set_header_images(T... args)
	{
		int n = sizeof...(args);
		int arg[] = { args... };

		//set_�̹Ƿ� ���� �׸���� ��� ������ �� �߰��ؾ� �Ѵ�.
		//�׳� �ϳ��� �߰��Ѵٸ� add_header_image()�� ����Ѵ�.
		for (int i = 0; i < m_header_images.size(); i++)
		{
			CSCGdiplusBitmap* img = m_header_images[i];
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
	void			play_gif();
	//pos��ġ�� �̵��� �� �Ͻ������Ѵ�. -1�̸� pause <-> play�� ����Ѵ�.
	void			pause_gif(int pos = 0);
	//animation thread�� ����ǰ� ȭ�鿡�� �� �̻� ǥ�õ��� �ʴ´�. ���� �״�� ���߱� ���Ѵٸ� pause_animation()�� ȣ���Ѵ�.
	void			stop_gif();

	HICON			m_hIcon;
	CSize			m_sz_icon;

//tooltip
	//�⺻���� ������ �� ��Ʈ�� ������ ����������
	//disabled�� ��Ʈ���� main�� PreTranslateMessage()���� ó������ ������ ��Ÿ���� �ʴ´�.
	void		use_tooltip(bool use) { m_use_tooltip = use; }
	void		set_tooltip_text(CString text);


protected:
	//SetWindowText(), GetWindowText()�� ���� m_text�� ���� �����ؼ� �����ص� �� �� ������
	//SetWindowText()�� ȣ���ϴ� ���� ȭ�鰻���� �Ͼ�� MFC���������δ� ���� ó���� �� ���̴�.
	//Ư�� transparent�� ���� �� �������� ũ�Ƿ� m_text�� ���� �����Ͽ� ����Ѵ�.
	CString			m_text;

	//�ؽ�Ʈ�� ���� ��µ� ������ ũ�⸦ �˱� ���� m_rect_text�� �߰������� ���� ������δ� rc�� ���� ����ϴ�.
	//�ؽ�Ʈ�� �ʺ� �˰��� �� ���� get_text_extent()�� ����ؾ� �Ѵ�.
	//��, CSCParagraphStatic�� m_rect_text�� ���� ��µǴ� �ؽ�Ʈ �������� �������� �����Ǿ���.
	CRect			m_rect_text;
	int				m_text_extent = 0;	//�ؽ�Ʈ�� ũ��

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

	Gdiplus::Color	m_cr_text = Gdiplus::Color::Black;
	Gdiplus::Color	m_cr_back = Gdiplus::Color::White;
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
	CSCGdiplusBitmap	m_img_back;

	int				m_halign = -1;
	int				m_valign = -1;

	LOGFONT			m_lf;
	CFont			m_font;
	void			update_surface();
	void			reconstruct_font();

	CRect			m_margin;			//�ؽ�Ʈ�� ������ �����Ѵ�. �⺻���� CStatic�� �����ϰ� 0.

	//label�� �տ� �׷��� �̹����̸� ���� 2�� �̻��� ��� Ÿ�̸ӿ� ���� alt�Ǳ⵵ �Ѵ�.
	std::deque<CSCGdiplusBitmap*> m_header_images;
	int				m_header_image_index;

	//�ؽ�Ʈ �տ� ǥ�õǴ� ������ �Ǵ� ��� �̹����� �ؽ�Ʈ�� ������ ������ ���ʿ� �׷��� ���
	//CSCMessageBox���� �������� ���ʿ�, �ؽ�Ʈ�� �߾����ĵǾ� ǥ�õǾ� �ϹǷ� �� �ɼ��� �߰���.
	bool			m_image_left_align_fix = false;

//tooltip
	//enable�����϶��� �� ǥ�õǳ� disable�϶��� ǥ�õ��� �ʴ´�.
	//�̸� �ذ��Ϸ��� parent�� PreTranslateMessage()���� ó���ؾ� �Ѵ�.
	//pointer Ÿ������ ������ ������ ���������ÿ��� ������ �����ϱ� ����.
	CToolTipCtrl*	m_tooltip = NULL;
	//default = true
	bool			m_use_tooltip = true;
	CString			m_tooltip_text = _T("");

	//�������� ���� ��Ʈ���� ���������� �������� ��Ʈ���� �����Ͽ� ����ϴ� ���
	//PreSubclassWindow()���� ������ �ʱ�ȭ�Ϸ��� ���ܰ� �߻���.
	//�׷��� Create()�Ŀ� ������ prepare_tooltip()�� ȣ���Ͽ� �غ�ǵ��� ����.
	//���� ������ ��Ʈ�ѿ����� ���� ǥ�õ��� Ȯ����.
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
};
#endif
