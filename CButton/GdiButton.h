#pragma once

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

#define MAX_BUTTON_IMAGE	10


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

	//ex. ANCHOR_RIGHT �ɼ��� �ָ� dx ��ǥ������ �����ϰ� �θ�â�� �����ʿ� ��ư�� ��ġ�ϰ� �ȴ�.

	//�� ��ư�� �ִ� 10���� �̹����� ���� �� ������ 0���� normal���� �̹���, �׸��� checked ���·� ���۵ȴ�.
	bool		AddImage( CString sfile, Bitmap* pBack = NULL, COLORREF crBack = 0, bool bFitToImage = true, int dx = -1, int dy = -1, UINT nAnchor = ANCHOR_NONE, bool bAsStatic = false, bool bShowError = true );
	bool		AddImage( HINSTANCE hInst, LPCTSTR lpName, LPCTSTR lpType, Bitmap* pBack = NULL, COLORREF crBack = 0, bool bFitToImage = true, int dx = -1, int dy = -1, UINT nAnchor = ANCHOR_NONE, bool bAsStatic = false, bool bShowError = true );
	bool		AddImage( Bitmap* bmpImage, Bitmap* pBack = NULL, COLORREF crBack = 0, bool bFitToImage = true, int dx = -1, int dy = -1, UINT nAnchor = ANCHOR_NONE, bool bAsStatic = false, bool bShowError = true );

	void		SelectImage( int nIndex );

	//gray �̹����� �ڵ� �����ؼ� �߰����ش�. (� �̹����� ������ ���� �÷���, ������ ���� ������� ǥ���ϴ� �뵵�� �����ϴ�)
	//�������� �̹����� ǥ���ϴ� ��쿡�� ������� ������ ���� 1���� �̹��� ��ư�� ��� disable �Ǵ� unchecked�� ǥ���ϱ� ���� ���ȴ�.
	//���� normal(checked)�� 0��, gray(unchecked)�� 1�� �ε����� ���´�.
	bool		AddGrayImage( bool bSelectGray = false );

	Bitmap*		Load( CString sfile );
	//Bitmap*		Load( UINT nResourceID );
	Bitmap*		GetImageFromResource( HINSTANCE hInst, LPCTSTR lpName, LPCTSTR lpType);
	Bitmap*		GdiplusImageToBitmap(Image* img, Color bkgd = Color::Transparent);

	void		ReleaseAll();

	void		SetBackImage(Bitmap* pBack);		//����� ����, ������ ��� ���
	void		SetBackColor(COLORREF crBack);	//������ ����, ������ ��� ���, ���� ��� �׸��� �־��� ���� ��� �׸��� ������.
	void		SetBackHoverColor(COLORREF crBackHover);
	void		SetBrightnessHoverEffect(float fScale);	//1.0f = no effect.


	virtual CGdiButton&		SetTextColor(COLORREF crText); // This Function is to set the Color for the Text.
	virtual CGdiButton&		SetTextHoverColor(COLORREF crTextHover); // This Function is to set the Color for hovering.
	virtual	CGdiButton&		SetFontName(LPCTSTR sFontname, BYTE byCharSet = DEFAULT_CHARSET);
	virtual CGdiButton&		SetFontSize( int nSize );
	virtual CGdiButton&		SetFontBold( bool bBold = true );

	//1�� �̹����� true, 0�� �̹����� false (checkbox�� radiobutton�� ���� �뵵�� ����� �� �ִ�)
	bool		GetCheck();
	//true�̸� 1�� �̹����� ǥ�����ش�. ���� 1���� ���ٸ� normal �̹����� ���� ����̹Ƿ�
	//AddGrayImage�� ȸ�� �̹����� 1���� �߰����ְ� 0���� 1���� �ٲ��ش�.
	void		SetCheck( bool bCkeck );
	void		Toggle();

	//m_bAsStatic�� true�� ��� hover�� down���� ������ ���� �ʴ´�.
	void		SetAsStatic( bool bAsStatic = true ) { m_bAsStatic = bAsStatic; }


	//��ư�� ũ��, ��ġ ����
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
	void		down_offset(CPoint offset) { m_down_offset = offset; Invalidate(); }
	void		down_offset(int offset) { m_down_offset = CPoint(offset, offset); Invalidate(); }

	//3D, sunken 
	void		use_3D_rect(bool use) { m_b3DRect = use; Invalidate(); }

	//blink
	void		SetBlinkTime( int nTime0 = 400, int nTime1 = 1200 );	//nTime0:hidden, nTime1:shown
	void		SetBlink( BOOL bBlink = TRUE );

protected:
	UINT		m_button_style;					//pushbutton(default) or checkbox or radiobutton

	Bitmap*		m_pImage[MAX_BUTTON_IMAGE];		//�� ��ư�� �߰��� �̹�����
	int			m_nImages;						//��� �ִ� �̹����� ��
	int			m_nIndex;						//���� ǥ���ϰ��� �ϴ� �̹��� �ε���

	Bitmap*		m_pBack;					//��ư�� ��� �̹���, NULL�̸� m_crBack�� ����
	Bitmap*		m_pBackOrigin;
	COLORREF	m_crBack;					//��ư�� ����
	COLORREF	m_crBackHover;				//��ư�� ����


	int			m_width;
	int			m_height;

	UINT		m_nAnchor;
	int			m_nAnchorMarginX;
	int			m_nAnchorMarginY;

	bool		m_bToggleButton;		//�⺻ �̹����� ǥ������, �ΰ� �̹����� ǥ������
	bool		m_bAsStatic;			//�ܼ� �̹��� ǥ�� �뵵�� ���ǰ� Ŭ���ص� ��ȭ�� ����. �⺻�� false.
	bool		m_use_hover;			//default = true;
	bool		m_bHover;
	bool		m_bIsTracking;
	bool		m_bPushed;
	bool		m_bHasFocus;
	bool		m_bShowFocusRect;		//��Ŀ�� �簢�� ǥ�� ����(�⺻�� false)
	COLORREF	m_crFocusRect;			//����
	int			m_nFocusRectWidth;		//�β�
	bool		m_b3DRect;				//��ü ������ 3D, ������ sunken. default = true;
	bool		m_bFitToImage;			//true�̸� �̹��� ũ���� ��Ʈ�� ũ�� ����, false�̸� ��Ʈ�� ũ��� �̹��� ������ ����. �⺻��=true
	CPoint		m_down_offset;			//������ �� �׷��� ��ġ(�⺻��=1);

	BOOL		m_bBlink;
	BOOL		m_bBlinkStatus;
	int			m_nBlinkTime0;		//blink type is Show/Hide, time0 = shown duration, time1 = hidden duration in millisecond.
	int			m_nBlinkTime1;


	void		ResizeControl( int& dx, int& dy );
	void		SafeRelease( Bitmap** pBitmap );

	LOGFONT		m_lf;
	CFont		m_font;
	COLORREF	m_crText;
	COLORREF	m_crTextHover;

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
};


