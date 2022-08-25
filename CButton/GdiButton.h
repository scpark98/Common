#pragma once

#include <afxwin.h>
#include <vector>
#include <gdiplus.h>

using namespace Gdiplus;

/* scpark
- gdi+�� �̿��� png ���� ��ư Ŭ����
- �ܻ� �Ǵ� ��� �̹��� ���� ����.
- ��� �̹����� �����͸� �Ѱܹ޾� ��ư�� ������� �̿���.
  ���� ����� (0,0)���� ���۵ǰ� resize���� ���� ����� ��쿡�� ������.
- ��� ����� ���� �Ϲ� push button, toggle button(as a checkbox) ������ ����� �� ����.

[usage]
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


- load button image
	from external file	:	m_Button_GDI.AddImage( _T("f:\\test_images\\etc\\red_checked.png"), NULL, ::GetSysColor(COLOR_3DFACE) );
	from resource png	:	m_Button_GDI.AddImage( AfxGetApp()->m_hInstance, MAKEINTRESOURCE(IDB_RED_CAR), _T("PNG"), NULL, ::GetSysColor(COLOR_3DFACE));
	from resource jpg	:	m_Button_GDI.AddImage( AfxGetApp()->m_hInstance, MAKEINTRESOURCE(IDB_RED_CAR), _T("JPG"), NULL, ::GetSysColor(COLOR_3DFACE));
	from resource bmp	:	m_Button_GDI.AddImage( AfxGetApp()->m_hInstance, MAKEINTRESOURCE(IDB_HOUSE), _T("Bitmap"), NULL, ::GetSysColor(COLOR_3DFACE));

- push button
	normal, over, down, disabled
- check or radio button
	checked, unchecked
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
	CButtonImage()
	{
		normal = over = down = disabled = NULL;
	}

	//~CButtonImage();

	Bitmap* normal;
	Bitmap* over;
	Bitmap* down;
	Bitmap* disabled;
};

// CGdiButton

class CGdiButton : public CButton
{
	DECLARE_DYNAMIC(CGdiButton)

public:
	CGdiButton();
	virtual ~CGdiButton();

	//���� CButton::SetButtonStyle �Լ��� overriding�Ͽ� OWNER_DRAW�� �߰�������� �Ѵ�.
	void		SetButtonStyle( UINT nStyle, BOOL bRedraw = 1 );
	//enum 

	//pBack�� ��ư�� ����� �Ǵ� �׸����� parent â�� ����̹����� ���� Bitmap*�̴�.
	//�� ��濡�� ��ư�� ����� �����Ͽ� DrawItem���� ���+���� �̹����� �׷��ְ� �ȴ�.
	//��, parentâ���� ��� �׸��� �ݵ�� ���� ũ��� ������� �׷����� �Ѵ�.
	//��ư�� ��ġ ��ǥ�� �ش��ϴ� ��� �׸��� �߶�ͼ� ����ϹǷ� ��浵 1:1�� �ѷ����� �Ѵ�.
	
	//GetParent()->UpdateWindow()�� �̿��Ͽ� �ѷ��ִ� ����� ��� �����͸� ������� �ʾƵ� �����ϰ� �ѷ����� ������ ������
	//UpdateWindow�� ���� �̹����� �����ϴ� �̺�Ʈ�� �߻��ϸ� �����̴� ������ �����Ѵ�.

	//add_images�� �ϳ��� ��ư�� �������� �̹����� �߰��� �� ����Ѵ�.
	//��, add_image�� �̹��� ������ŭ ȣ���Ѵ�.
	//Ư�� check, radioó�� checked, unchecked image�� ������ ������ �� ����� �� �ְ�
	//�ϳ��� ��ư�� �������� �̹����� �������� �� �ʿ䰡 ���� ��쿡�� ���ȴ�.
	//on/off, play/pause, img0/img1/img2...
	template <typename ... Types>
	void add_images(HINSTANCE hInst, LPCTSTR lpType, Types... args)
	{
		int n = sizeof...(args);
		int arg[] = { args... };

		for (auto id : arg)
			add_image(hInst, lpType, id);
	}

	//��ư�� 4���� ���� �̹������� �����Ѵ�. UINT�� 0�̸� �ڵ� �������ش�.
	//�� ��ư�� ���� normal, over, down, disabled �̹������� ���� ������ �� ���ȴ�.
	bool		add_image(HINSTANCE hInst, LPCTSTR lpType, UINT normal, UINT over = 0, UINT down = 0, UINT disabled = 0);

	//fit = true�̸� ��Ʈ���� ũ�⸦ �̹��� ũ��� resize�Ѵ�. false�̸� ��Ʈ���� ũ�⿡ �°� �̹����� �׷��ش�.
	void		fit_to_image(bool fit = true);

	//over �̹����� �ڵ� �����Ѵ�.
	Bitmap*		gen_over_image(Bitmap* img);
	//down �̹����� �ڵ� �����Ѵ�.
	Bitmap*		gen_down_image(Bitmap* img);
	//disabled �̹����� �ڵ� �����Ѵ�.
	Bitmap*		gen_disabled_image(Bitmap* img);

	void		select(int index);


	Bitmap*		Load(CString sfile);
	Bitmap*		get_bitmap(HINSTANCE hInst, LPCTSTR lpType, LPCTSTR lpName, bool show_error = false);

	static void	safe_release(Bitmap** pBitmap);

	static Bitmap* GetImageFromResource( HINSTANCE hInst, LPCTSTR lpName, LPCTSTR lpType);
	static Bitmap* GdiplusImageToBitmap(Image* img, Color bkgd = Color::Transparent);

	void		release_all();

	void		SetBackImage(Bitmap* pBack);		//����� ����, ������ ��� ���
	void		set_back_color(COLORREF normal, COLORREF hover, COLORREF down, COLORREF disabled);
	void		SetBrightnessHoverEffect(float fScale);	//1.0f = no effect.


	virtual	CGdiButton&		SetFontName(LPCTSTR sFontname, BYTE byCharSet = DEFAULT_CHARSET);
	virtual CGdiButton&		SetFontSize( int nSize );
	virtual CGdiButton&		SetFontBold( bool bBold = true );

	bool		GetCheck();
	void		SetCheck( bool bCkeck );
	void		Toggle();

	//m_bAsStatic�� true�� ��� hover�� down���� ������ ���� �ʴ´�.
	void		SetAsStatic( bool bAsStatic = true ) { m_bAsStatic = bAsStatic; }


	//��ư�� ũ��, ��ġ ����
	int			width() { return m_width; }
	int			height() { return m_height; }
	void		SetAnchor( UINT nAnchor ) { m_nAnchor = nAnchor; }	//���� ��� ����
	void		SetAnchorMargin( int x, int y ) { m_nAnchorMarginX = x; m_nAnchorMarginY = y; }
	void		ReAlign();	//parent�� ũ�Ⱑ ���ϸ� ������ align���� ���� ��ġ�� ���������ش�.
	void		Offset( int x, int y );
	void		Inflate( int cx, int cy );
	void		Inflate( int l, int t, int r, int b );

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
	void		SetBlinkTime( int nTime0 = 400, int nTime1 = 1200 );	//nTime0:hidden, nTime1:shown
	void		SetBlink( BOOL bBlink = TRUE );

protected:
	UINT		m_button_style;				//pushbutton(default) or checkbox or radiobutton

	std::vector<CButtonImage> m_image;
	int			m_idx = 0;					//���� ǥ���� m_image�� �ε��� (checkbox�� radio�� �̼���=0, ����=1)
	bool		m_fit2image = true;			//true : �̹��� ũ���� ��Ʈ�� ũ�� ����, false : ���� ��Ʈ�� ũ��� �̹��� ǥ��
	CRect		m_rwOrigin = 0;				//��Ʈ���� ���� ũ�� ����

	Bitmap*		m_pBack;					//��ư�� ��� �̹���, NULL�̸� m_crBack�� ����
	Bitmap*		m_pBackOrigin;

	COLORREF	m_cr_text[4];
	COLORREF	m_cr_back[4];

	int			m_width;
	int			m_height;

	UINT		m_nAnchor;
	int			m_nAnchorMarginX;
	int			m_nAnchorMarginY;

	bool		m_bAsStatic;			//�ܼ� �̹��� ǥ�� �뵵�� ���ǰ� Ŭ���ص� ��ȭ�� ����. �⺻�� false.
	bool		m_use_hover = true;		//default = true;
	bool		m_hover_rect = false;	//hover �׵θ� �簢�� ǥ�� ����
	int			m_hover_rect_thick = 2;
	COLORREF	m_hover_rect_color = RGB(128, 128, 255);
	bool		m_bHover;
	bool		m_bIsTracking;
	bool		m_bPushed;
	bool		m_bHasFocus;
	bool		m_bShowFocusRect;		//��Ŀ�� �簢�� ǥ�� ����(�⺻�� false)
	COLORREF	m_crFocusRect;			//����
	int			m_nFocusRectWidth;		//�β�
	bool		m_b3DRect;				//��ü ������ 3D, ������ sunken. default = true;
	bool		m_is_down = false;
	CPoint		m_down_offset;			//������ �� �׷��� ��ġ(�⺻��=1);

	BOOL		m_bBlink;
	BOOL		m_bBlinkStatus;
	int			m_nBlinkTime0;			//blink type is Show/Hide, time0 = shown duration, time1 = hidden duration in millisecond.
	int			m_nBlinkTime1;


	void		resize_control(int cx, int cy);

	LOGFONT		m_lf;
	CFont		m_font;

	void		ReconstructFont();

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
};


