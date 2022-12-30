#pragma once

#include <afxwin.h>
#include <deque>
#include <gdiplus.h>

#include "../GdiplusBitmap.h"

using namespace Gdiplus;

/* scpark
- gdi+�� �̿��� png ���� ��ư Ŭ����
- �ܻ� �Ǵ� ��� �̹��� ���� ����.
- ��� �̹����� �����͸� �Ѱܹ޾� ��ư�� ������� �̿���.
  ���� ����� (0,0)���� ���۵ǰ� resize���� ���� ����� ��쿡�� ������.
- ��� ����� ���� �Ϲ� push button, toggle button(as a checkbox) ������ ����� �� ����.

[usage]
* Gdiplus ����� ���� ����
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
	...

- push button
	m_btnOk.add_image(_T("PNG"), IDB_BTN72_NORMAL, 0, IDB_BTN72_DOWN, IDB_BTN72_DISABLED); //�Ǵ�
	m_btnOk.add_image(_T("PNG"), IDB_BTN72_NORMAL); //over, down, disabled �̹����� ���� �������� ������ �ڵ� ��������.
	m_btnOk.text(_T("Ȯ��")).text_color(black, blue, red, gray).SetFontSize(12).SetFontBold();

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
	CButtonImage() {};

	CGdiplusBitmap normal;
	CGdiplusBitmap over;
	CGdiplusBitmap down;
	CGdiplusBitmap disabled;
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
	void add_images(LPCTSTR lpType, Types... args)
	{
		int n = sizeof...(args);
		int arg[] = { args... };

		for (auto id : arg)
			add_image(lpType, id);
	}

	//��ư�� 4���� ���� �̹������� �����Ѵ�. UINT�� 0�̸� �ڵ� �������ش�.
	//�� ��ư�� ���� normal, over, down, disabled �̹������� ���� ������ �� ���ȴ�.
	bool		add_image(LPCTSTR lpType, UINT normal, UINT over = 0, UINT down = 0, UINT disabled = 0);

	//fit = true�̸� ��Ʈ���� ũ�⸦ �̹��� ũ��� resize�Ѵ�. false�̸� ��Ʈ���� ũ�⿡ �°� �̹����� �׷��ش�.
	void		fit_to_image(bool fit = true);

	void		select(int index);

	void		release_all();

	//void		SetBackImage(Bitmap* pBack);		//����� ����, ������ ��� ���
	CGdiButton& text(CString text);
	CGdiButton& text_color(COLORREF normal, COLORREF hover, COLORREF down, COLORREF disabled);
	CGdiButton& text_color(COLORREF normal);
	CGdiButton& back_color(COLORREF normal, COLORREF hover, COLORREF down, COLORREF disabled);
	CGdiButton& back_color(COLORREF normal);
	CGdiButton& text_color() { m_cr_text.clear(); }
	CGdiButton& back_color() { m_cr_back.clear(); }
	//reassign [0,0] [1,1] [2,2]
	//hover, down�� ��� ���� ��ȭ�� �ְ��� �� ��� ���.(fScale�� 1.0���� ũ���ָ� ���, �۰��ָ� ��Ӱ� ����ȴ�.
	CGdiButton& set_hover_color_matrix(float fScale);	//1.0f = no effect.
	CGdiButton& set_down_color_matrix(float fScale);	//1.0f = no effect.

	virtual	CGdiButton&		SetFontName(LPCTSTR sFontname, BYTE byCharSet = DEFAULT_CHARSET);
	virtual CGdiButton&		SetFontSize( int nSize );
	virtual CGdiButton&		SetFontBold( bool bBold = true );

	void		UpdateSurface();

	bool		GetCheck();
	void		SetCheck( bool bCkeck );
	void		Toggle();

	//m_bAsStatic�� true�� ��� hover�� down���� ������ ���� �ʴ´�.
	void		SetAsStatic( bool bAsStatic = true ) { m_bAsStatic = bAsStatic; }


	//��ư�� ũ��, ��ġ ����
	int			width();
	int			height();
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

	//static void	rotate(Bitmap* bitmap, Gdiplus::RotateFlipType type);
	//static void	rotate(Bitmap** bitmap, float angle);

protected:
	UINT		m_button_style;				//pushbutton(default) or checkbox or radiobutton

	std::deque<CButtonImage*> m_image;
	int			m_idx = 0;					//���� ǥ���� m_image�� �ε��� (checkbox�� radio�� �̼���=0, ����=1)
	bool		m_fit2image = true;			//true : �̹��� ũ���� ��Ʈ�� ũ�� ����, false : ���� ��Ʈ�� ũ��� �̹��� ǥ��
	CRect		m_rOrigin = 0;				//��Ʈ���� ���� ũ�� ����

	//����� �ܻ��� �ƴ� �׸��̰� ���� PNG�� �׸��� ���, resize���� �� ���� true�� �Ѵ�.
	//��, �� ��� ���� �ϼ��� ����� �ƴ϶� �ణ �����̴� ������ �ִ�.
	bool		m_transparent = true;

	CGdiplusBitmap	m_back;					//��ư�� ��� �̹���, NULL�̸� m_crBack�� ����
	CGdiplusBitmap	m_back_origin;

	CString		m_text = _T("");
	std::deque <COLORREF>	m_cr_text;
	std::deque <COLORREF>	m_cr_back;		//���� PNG�� ������ �����ߴٸ� ����� �׷�����.

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
	CPoint		m_down_offset;			//������ �� �׷��� ��ġ(�⺻��=1);

	BOOL		m_bBlink;
	BOOL		m_bBlinkStatus;
	int			m_nBlinkTime0;			//blink type is Show/Hide, time0 = shown duration, time1 = hidden duration in millisecond.
	int			m_nBlinkTime1;

	ColorMatrix m_grayMatrix;
	ColorMatrix m_hoverMatrix;			//hover�̹����� ������ ��Ʈ����
	ColorMatrix m_downMatrix;			//down�̹����� ������ ��Ʈ����

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


