// GdiButton.cpp : ���� �����Դϴ�.
//

#include "stdafx.h"
#include "GdiButton.h"

#include "../Functions.h"
#include "../MemoryDC.h"
#include "../GdiPlusBitmap.h"

#pragma warning(disable: 4305)	//'argument': truncation from 'double' to 'Gdiplus::REAL'

#define	TIMER_BLINK		0

ColorMatrix GrayMat = {	0.30f, 0.30f, 0.30f, 0.00f, 0.00f,
						0.59f, 0.59f, 0.59f, 0.00f, 0.00f,
						0.11f, 0.11f, 0.11f, 0.00f, 0.00f,
						0.00f, 0.00f, 0.00f, 1.00f, 0.00f,
						0.00f, 0.00f, 0.00f, 0.00f, 1.00f	};

ColorMatrix HotMat = {	1.00f, 0.00f, 0.00f, 0.00f, 0.00f,		// red scaling factor
						0.00f, 1.00f, 0.00f, 0.00f, 0.00f,		// green scaling factor 
						0.00f, 0.00f, 1.00f, 0.00f, 0.00f,		// blue scaling factor 
						0.00f, 0.00f, 0.00f, 0.80f, 0.00f,		// alpha scaling factor 
						0.00f, 0.00f, 0.00f, 0.00f, 1.00f	};	// three translations 

// CGdiButton

IMPLEMENT_DYNAMIC(CGdiButton, CButton)

CGdiButton::CGdiButton()
{
	m_button_style		= BS_PUSHBUTTON;

	m_pBack				= NULL;
	m_pBackOrigin		= NULL;
	m_bAsStatic			= false;

	m_nIndex			= -1;
	m_nImages			= 0;

	m_nAnchor			= ANCHOR_NONE;
	m_nAnchorMarginX	= 0;
	m_nAnchorMarginY	= 0;

	for ( int i = 0; i < MAX_BUTTON_IMAGE; i++ )
		m_pImage[i] = NULL;

	m_bToggleButton		= false;
	m_bPushed			= false;
	m_use_hover			= true;
	m_bHover			= false;
	m_bIsTracking		= false;
	m_down_offset		= CPoint(1, 1);

	m_crText			= ::GetSysColor( COLOR_BTNTEXT );
	m_crTextHover		= ::GetSysColor( COLOR_HOTLIGHT );
	m_crBack			= ::GetSysColor( COLOR_3DFACE );
	m_crBackHover		= ::GetSysColor( COLOR_3DSHADOW );

	m_bHasFocus			= false;
	m_bShowFocusRect	= false;
	m_crFocusRect		= RGB( 6, 205, 255 );
	m_nFocusRectWidth	= 2;

	m_b3DRect			= true;

	m_bBlink		= FALSE;
	m_bBlinkStatus	= FALSE;
	m_nBlinkTime0	= 400;
	m_nBlinkTime1	= 1200;

	m_bFitToImage	= true;

	memset( &m_lf, 0, sizeof( LOGFONT ) );
}

CGdiButton::~CGdiButton()
{
	ReleaseAll();
}

void CGdiButton::SafeRelease( Bitmap** pBitmap )
{
	if ( *pBitmap == NULL )
		return;

	delete *pBitmap;
	*pBitmap = NULL;
}

void CGdiButton::ReleaseAll()
{
	SafeRelease( &m_pBack );
	SafeRelease( &m_pBackOrigin );

	for ( int i = 0; i < MAX_BUTTON_IMAGE; i++ )
		SafeRelease( &m_pImage[i] );

	m_nIndex	= -1;
	m_nImages	= 0;
}

BEGIN_MESSAGE_MAP(CGdiButton, CButton)
	ON_WM_ERASEBKGND()
	ON_WM_MOUSEMOVE()
	ON_WM_MOUSEHOVER()
	ON_WM_MOUSELEAVE()
	ON_WM_TIMER()
	ON_WM_SETFOCUS()
	ON_WM_KILLFOCUS()
	ON_WM_SETCURSOR()
	ON_WM_LBUTTONUP()
END_MESSAGE_MAP()



// CGdiButton �޽��� ó�����Դϴ�.

//���� CButton::SetButtonStyle �Լ��� overriding�Ͽ� OWNER_DRAW�� �߰�������� �Ѵ�.
void CGdiButton::SetButtonStyle( UINT nStyle, BOOL bRedraw )
{
	DWORD dwStyle = ::GetWindowLong(m_hWnd, GWL_STYLE);

	CButton::SetButtonStyle( nStyle, bRedraw );

	// BS_OWNERDRAW �Ӽ��� �����Ѵ�.
	//���� : dwStyle�� �ݵ�� CButton::SetButtonStyle() �Լ� ȣ�� ���� ���� ���� ����ؾ� �Ѵ�.
	::SetWindowLong(m_hWnd, GWL_STYLE, dwStyle | BS_OWNERDRAW);
	m_button_style = nStyle;
}


//�� ��ư�� �ִ� 10���� �̹����� ���� �� ������ 0���� normal���� �̹���, �׸��� checked ���·� ���۵ȴ�.
bool CGdiButton::AddImage( CString sfile, Bitmap* pBack, COLORREF crBack, bool bFitToImage, int dx, int dy, UINT nAnchor, bool bAsStatic, bool bShowError )
{
	if ( m_nImages >= MAX_BUTTON_IMAGE )
		return false;

	Bitmap	*bitmap = Load( sfile );

	if ( bitmap == NULL )
	{
		if ( bShowError )
			AfxMessageBox( sfile + "\n\n�� ������ �������� �ʰų� �� �� �����ϴ�." );
		return false;
	}

	return AddImage( bitmap, pBack, crBack, bFitToImage, dx, dy, nAnchor, bAsStatic, bShowError );
}

Bitmap* CGdiButton::GdiplusImageToBitmap(Image* img, Color bkgd)
{
	Bitmap* bmp = nullptr;
	try {
		int wd = img->GetWidth();
		int hgt = img->GetHeight();
		auto format = img->GetPixelFormat();
		bmp = new Bitmap(wd, hgt, format);
		auto g = std::unique_ptr<Graphics>(Graphics::FromImage(bmp));
		g->Clear(bkgd);
		g->DrawImage(img, 0, 0, wd, hgt);
	} catch(...) {
		// this might happen if img->GetPixelFormat() is something exotic
		// ... not sure
	}
	return bmp;
}

bool CGdiButton::AddImage(  HINSTANCE hInst, LPCTSTR lpName, LPCTSTR lpType, Bitmap* pBack, COLORREF crBack, bool bFitToImage, int dx, int dy, UINT nAnchor, bool bAsStatic, bool bShowError )
{
	if ( m_nImages >= MAX_BUTTON_IMAGE )
		return false;

	Bitmap	*bitmap = NULL;
	CString str = lpType;

	if ( str == "PNG" )
		bitmap = GetImageFromResource( hInst, lpName, lpType );
	else if ( str == "JPG" )
	{
		HRSRC hResource = FindResource(hInst, lpName, TEXT("JPG"));

		if (!hResource) 
			return false;

		DWORD imageSize = SizeofResource(hInst, hResource);
		HGLOBAL hGlobal = LoadResource(hInst, hResource);
		LPVOID pData = LockResource(hGlobal);
		HGLOBAL hBuffer = GlobalAlloc(GMEM_MOVEABLE,imageSize);
		LPVOID pBuffer = GlobalLock(hBuffer);

		CopyMemory(pBuffer,pData,imageSize);
		GlobalUnlock(hBuffer);
		IStream *pStream;

		HRESULT hr=CreateStreamOnHGlobal(hBuffer,TRUE,&pStream);
		Gdiplus::Image I(pStream);
		pStream->Release();

		if (I.GetLastStatus() != Gdiplus::Ok)
			return false;
		bitmap = GdiplusImageToBitmap(&I);
	}
	else
		bitmap = Bitmap::FromResource( hInst, (WCHAR*)lpName );

	if ( bitmap == NULL )
	{
		if ( bShowError )
		{
			str.Format( _T("%s : fail to read resource image.") );
			AfxMessageBox( str );
		}
		return false;
	}

	return AddImage( bitmap, pBack, crBack, bFitToImage, dx, dy, nAnchor, bAsStatic, bShowError );

}

bool CGdiButton::AddImage( Bitmap *bitmap, Bitmap* pBack, COLORREF crBack, bool bFitToImage, int dx, int dy, UINT nAnchor, bool bAsStatic, bool bShowError )
{
	if ( m_nImages >= MAX_BUTTON_IMAGE )
		return false;


	//m_pImage[m_nImages] = new Bitmap( bitmap->GetWidth(), bitmap->GetHeight(), PixelFormat32bppARGB );
	Rect sourceRect( 0, 0, bitmap->GetWidth(), bitmap->GetHeight() );
	m_pImage[m_nImages] = bitmap->Clone( sourceRect, PixelFormatDontCare );
	delete bitmap;

	m_nAnchor = nAnchor;

	CRect	rc;
	GetWindowRect( rc );

	m_crBack = crBack;
	m_bFitToImage = bFitToImage;

	if ( m_nImages == 0 )
	{
		if ( m_bFitToImage )
		{
			m_width	= m_pImage[0]->GetWidth();
			m_height = m_pImage[0]->GetHeight();

			ResizeControl( dx, dy );
		}
		else
		{
			//m_pImage[m_nImages] = (Bitmap*)(m_pImage[m_nImages]->GetThumbnailImage( rc.Width(), rc.Height() ));
			m_width = rc.Width();// m_pImage[0]->GetWidth();
			m_height = rc.Height();// m_pImage[0]->GetHeight();
		}

		SafeRelease( &m_pBack );
		SafeRelease( &m_pBackOrigin );

		if ( pBack )
		{
			Rect rc( 0, 0, pBack->GetWidth(), pBack->GetHeight() );
			m_pBackOrigin = pBack->Clone( rc, PixelFormatDontCare );
			SetBackImage( pBack );
		}

		m_nIndex = 0;
		m_bAsStatic = bAsStatic;
	}
	else
	{
		//�߰��Ǵ� �̹������� ��� 0�� �̹����� ������ ũ�⿩�� �Ѵ�.
		//m_pImage[m_nImages] = (Bitmap*)(m_pImage[m_nImages]->GetThumbnailImage( m_width, m_height ));
	}

	m_nImages++;

	//radio ��ư�� 1���� �̹����� ������ ó�� ���۽� unchecked�� ���۵ȴ�.
	//unchecked �̹����� setcheck���� �ڵ����� �������ش�.
	if ( (GetButtonStyle() & BS_RADIOBUTTON) && m_nImages == 1 )
		SetCheck( false );

	return true;
}

void CGdiButton::SelectImage( int nIndex )
{
	if ( m_nImages <= 0 || nIndex < 0 || nIndex >= m_nImages )
		return;

	m_nIndex = nIndex;
	Invalidate();
}

//1�� �̹����� true, 0�� �̹����� false (checkbox�� radiobutton�� ���� �뵵�� ����� �� �ִ�)
bool CGdiButton::GetCheck()
{
	return (m_nIndex == 1);
}

//true�̸� 0�� �̹����� ǥ�����ش�. false�� ���� 1���� ǥ���ϴµ� ���� ������ AddGrayImage�� ȸ�� �̹����� �ڵ� �߰��� �� �������ش�.
void CGdiButton::SetCheck( bool bCheck )
{
	if ( m_pImage[0] == NULL )
	{
	}
 	else if ( m_pImage[1] == NULL )
	{
		AddGrayImage();

		Rect rc( 0, 0, m_width, m_height );

		Bitmap* temp = m_pImage[0]->Clone( rc, PixelFormatDontCare );
		SafeRelease( &m_pImage[0] );

		m_pImage[0] = m_pImage[1]->Clone( rc, PixelFormatDontCare );
		SafeRelease( &m_pImage[1] );

		m_pImage[1] = temp->Clone( rc, PixelFormatDontCare );
		SafeRelease( &temp );
	}

	m_nIndex = bCheck;

	Invalidate();

	//radio ��ư�� �������ų� SetCheck( true )�� ȣ��Ǹ�
	//���� group ���� �ٸ� ��ư���� unchecked�� ����� ����Ѵ�.
	//owner draw �Ӽ������� WindowProc�� ������ �⺻ �޽����� �޾Ƽ� ó���� �� ����.
	if ( (m_button_style == BS_RADIOBUTTON) && bCheck )
	{
		CWnd *pWndParent = GetParent(); 
		CWnd *pWnd = pWndParent->GetNextDlgGroupItem(this); 

		while (pWnd && (pWnd != this)) 
		{ 
			if (pWnd->IsKindOf(RUNTIME_CLASS(CGdiButton))) 
			{ 
				if ( ((CGdiButton*)pWnd)->m_button_style == BS_RADIOBUTTON )
				{ 
					((CGdiButton*)pWnd)->SetCheck(0); 
					pWnd->Invalidate(); 
				} 
			} 

			pWnd = pWndParent->GetNextDlgGroupItem(pWnd); 
		}
	}
}

Bitmap* CGdiButton::Load( CString sfile )
{
	Bitmap*	bitmap = NULL;

	BSTR bstrFilename = sfile.AllocSysString();;
	bitmap = Bitmap::FromFile( bstrFilename );
	::SysFreeString(bstrFilename);

	if ( bitmap->GetLastStatus() == Gdiplus::Ok )
		return bitmap;

	return NULL;
}

//�̰� ������ �ȸ�����. FindResource���� null���� �����Ѵ�.
/*
Bitmap* CGdiButton::Load( UINT nResourceID )
{
	HINSTANCE hInst = AfxGetApp()->m_hInstance;
	HRSRC hResource = FindResource(AfxGetApp()->m_hInstance, MAKEINTRESOURCE(nResourceID), _T("PNG") );
	if ( !hResource )
		return NULL;

	DWORD imageSize = SizeofResource(AfxGetApp()->m_hInstance, hResource);
	HGLOBAL hGlobal = LoadResource(AfxGetApp()->m_hInstance, hResource);
	LPVOID pData = LockResource(hGlobal);

	HGLOBAL hBuffer = GlobalAlloc(GMEM_MOVEABLE, imageSize);
	LPVOID pBuffer = GlobalLock(hBuffer);

	CopyMemory(pBuffer,pData,imageSize);
	GlobalUnlock(hBuffer);

	IStream *pStream;
	HRESULT hr = CreateStreamOnHGlobal(hBuffer, TRUE, &pStream);

	Image imagePNG(pStream);

	pStream->Release();

	if ( imagePNG.GetLastStatus() != Ok )
		return NULL;

	Bitmap* pBitmap = static_cast<Bitmap*>(imagePNG.Clone());

	return pBitmap;
}
*/
Bitmap*	CGdiButton::GetImageFromResource( HINSTANCE hInst, LPCTSTR lpName, LPCTSTR lpType)
{
	HRSRC hResource = FindResource(hInst, lpName, lpType );

	if ( !hResource )
		return NULL;

	DWORD imageSize = SizeofResource(AfxGetApp()->m_hInstance, hResource);
	HGLOBAL hGlobal = LoadResource(AfxGetApp()->m_hInstance, hResource);
	LPVOID pData = LockResource(hGlobal);

	HGLOBAL hBuffer = GlobalAlloc(GMEM_MOVEABLE, imageSize);
	LPVOID pBuffer = GlobalLock(hBuffer);

	CopyMemory(pBuffer,pData,imageSize);
	GlobalUnlock(hBuffer);

	IStream *pStream;
	HRESULT hr = CreateStreamOnHGlobal(hBuffer, TRUE, &pStream);

	Image imagePNG(pStream);

	pStream->Release();

	if ( imagePNG.GetLastStatus() != Ok )
		return NULL;

	Bitmap* pBitmap = static_cast<Bitmap*>(imagePNG.Clone());

	return pBitmap;
}

void CGdiButton::SetBackImage( Bitmap* pBack )
{
	if ( pBack == NULL )
		return;

	if ( m_pBack )
		SafeRelease( &m_pBack );

	CRect	rc;
	CPoint	pt;

	GetWindowRect( rc );
	pt = rc.TopLeft();
	::ScreenToClient( GetParent()->m_hWnd, &pt );

	Rect cutRect( pt.x, pt.y, rc.Width(), rc.Height() );
	m_pBack = pBack->Clone( cutRect, pBack->GetPixelFormat() );

	Invalidate();
}

void CGdiButton::SetBackColor( COLORREF crBack )
{
	if ( m_pBack )
		SafeRelease( &m_pBack );

	m_crBack = crBack;
	Invalidate();
}

void CGdiButton::SetBackHoverColor(COLORREF crBackHover)
{
	m_crBackHover = crBackHover;
	Invalidate();
}

void CGdiButton::SetBrightnessHoverEffect( float fScale )	//1.0f = no effect.
{
	HotMat.m[0][0] = fScale;
	HotMat.m[1][1] = fScale;
	HotMat.m[2][2] = fScale;
}

//�׸��� ũ�⿡ �°� ��Ʈ���� resize�ϰ� dx, dy, nAnchor�� ���� move���ش�.
void CGdiButton::ResizeControl( int& dx, int& dy )
{
	CRect	rc, rParentRect;

	GetParent()->GetClientRect( &rParentRect );

	//parent�� Ŭ���̾�Ʈ ���� ������ �� ��Ʈ���� ��ǥ�� ���Ѵ�.
	//ScreenToClient( rc );�� ::ScreenToClient( GetParent()->m_hWnd, &pt );�� rc.topleft�� pt�� �ٸ� ����� ���δ�.
	GetWindowRect( rc );
	//ScreenToClient( rc );
	CPoint	pt = rc.TopLeft();
	::ScreenToClient( GetParent()->m_hWnd, &pt );

	if ( dx < 0 )
		dx = pt.x;

	if ( dy < 0 )
		dy = pt.y;

	if ( m_nAnchor > ANCHOR_NONE )
	{
		if ( m_nAnchor & ANCHOR_LEFT )
			dx = m_nAnchorMarginX;

		if ( m_nAnchor & ANCHOR_TOP )
			dy = m_nAnchorMarginY;

		if ( m_nAnchor & ANCHOR_RIGHT )
			dx = rParentRect.right - m_width - m_nAnchorMarginX;

		if ( m_nAnchor & ANCHOR_BOTTOM )
			dy = rParentRect.bottom - m_height - m_nAnchorMarginY;

		if ( m_nAnchor & ANCHOR_HCENTER )
			dx = rParentRect.CenterPoint().x - m_width / 2;

		if ( m_nAnchor & ANCHOR_VCENTER )
			dy = rParentRect.CenterPoint().y - m_height / 2;
	}

	//w, h�� +1�� �� �� ������ ��ư�� ũ�Ⱑ fit�ϸ� ������ �� (x+1), (y+1)�� �׷����Ƿ�
	//�����ʰ� �ϴ��� 1�ȼ��� �Ⱥ��̰� �ȴ�. ���� ��ư�� ũ��� ���� �̹����� w, h���� 1�� �� Ŀ�� �Ѵ�.
	SetWindowPos( NULL, dx, dy, m_width + 1, m_height + 1, SWP_NOZORDER );
}

bool CGdiButton::AddGrayImage( bool bSelectGray )
{
	if ( m_nImages <= 0 )
		return false;

	Rect rc( 0, 0, m_width, m_height );

	//gray image�� 1���� �����Ѵ�. ���� 1���� �����ϸ� �����...
	if ( m_nImages == 2 && m_pImage[1] )
	{
		SafeRelease( &m_pImage[1] );
	}
	else
	{
		m_pImage[1] = m_pImage[0]->Clone( rc, PixelFormatDontCare );
	}

	Graphics		g( m_pImage[1] );
	ImageAttributes ia;

	ia.SetColorMatrix( &GrayMat,ColorMatrixFlagsDefault, ColorAdjustTypeBitmap);
	g.DrawImage( m_pImage[0], rc, 0, 0, m_width, m_height, UnitPixel, &ia);

	m_nImages++;

	if ( bSelectGray )
		SelectImage( 1 );

	return true;
}

void CGdiButton::PreSubclassWindow()
{
	// TODO: ���⿡ Ư��ȭ�� �ڵ带 �߰� ��/�Ǵ� �⺻ Ŭ������ ȣ���մϴ�.
	// Set control to owner draw
	m_button_style = getButtonStyle( m_hWnd );
	
	CString str;
	GetWindowText( str );
	//TRACE( _T("%s = %d\n"), str, m_button_style );
	
	ModifyStyle( 0, BS_OWNERDRAW, SWP_FRAMECHANGED );
	//ModifyStyle( 0, WS_TABSTOP, WS_TABSTOP );



	CFont* font = GetFont();

	if ( font != NULL )
		font->GetObject(sizeof(m_lf),&m_lf);
	else
		GetObject(GetStockObject(SYSTEM_FONT),sizeof(m_lf),&m_lf);

	ReconstructFont();

	CButton::PreSubclassWindow();
}

void CGdiButton::ReconstructFont()
{
	m_font.DeleteObject();
	BOOL bCreated = m_font.CreateFontIndirect(&m_lf);

	SetFont( &m_font, true );

	ASSERT(bCreated);
}


BOOL CGdiButton::PreTranslateMessage(MSG* pMsg)
{
	// TODO: ���⿡ Ư��ȭ�� �ڵ带 �߰� ��/�Ǵ� �⺻ Ŭ������ ȣ���մϴ�.
	switch ( pMsg->message )
	{
		case WM_LBUTTONDBLCLK	:	return true;
									pMsg->message = WM_LBUTTONDOWN;
									break;

		//LButtonDown�� Up�� �⺻ �޽��� �ڵ鷯�� ���� ó���ϸ� ���ο��� ��ư Ŭ�� �̺�Ʈ�� �߻����� �ʴ´�.
		//���⼭ return false�� �ϸ� ���ο��� ��ư Ŭ�� �̺�Ʈ�� ó���ȴ�.
		//��, ���⼭�� SetCapture�� ReleaseCapture�� ����ϸ� ���� ���ο��� �̺�Ʈ�� �߻����� �ʴ´�.
		case WM_LBUTTONDOWN		:	if ( m_bAsStatic )
										return true;

									m_bPushed = true;

									Invalidate();
									return false;

		case WM_LBUTTONUP		:	if ( m_bAsStatic )
										return true;

									if ( m_bPushed )
									{
										m_bPushed = false;

										CRect	rc;
										GetWindowRect( rc );

										//��ư ���� ������ ���߸� �ǹ��ִ�.
										//���� ��ư ��Ÿ���� check box �̸� ��۵ǰ�
										//radio button �̸� ���� �׷쳻�� �ٸ� radio button���� unchecked ��Ų��.
										if ( rc.PtInRect( pMsg->pt ) )
										{
											if ( m_button_style == BS_CHECKBOX )
												Toggle();
											else if ( m_button_style == BS_RADIOBUTTON )
												SetCheck( true );
										}

										Invalidate();
									}

									return false;
	}

	return CButton::PreTranslateMessage(pMsg);
}

#define MAP_STYLE(src, dest) if(dwStyle & (src)) dwText |= (dest)

void CGdiButton::DrawItem(LPDRAWITEMSTRUCT lpDIS/*lpDrawItemStruct*/)
{
	if (IsWindowVisible() == false)//|| m_nImages <= 0 || m_nIndex < 0 )
		return;

	CDC*		pDC1 = CDC::FromHandle(lpDIS->hDC);
	CRect		rc;
	CPoint		pt(0, 0);
	Bitmap*		pImage = (m_nIndex >= 0 ? m_pImage[m_nIndex] : NULL);
	CString		str;

	GetClientRect(rc);

	//����� �׷��ְ� �̹����� �׸��Ƿ� ���⼭�� CMemoryDC�� �̿��Ͽ� ������۸��� ����� �ȱ����δ�.
	CMemoryDC	dc(pDC1, &rc);//, true );
	Graphics	g(dc.m_hDC, rc);

	//�̹����� ���ٸ� �⺻ ������� �׷���� ������
	//push or radio�� ������ �׷��ֱ�� �ڵ尡 �� �ʿ��ϴ�.
	//�켱 checkbox�� �����ϰ� �����Ѵ�. 2018 09 04 12
	//2020 09 26 21
	//radio�� checkbox�� �⺻ �̹����� �ε��Ͽ� ����ϴ� �͵� �����ϳ�
	//���� ������ ������ ���� �ϴ� ���� �׷��ش�.
	if (pImage == NULL)
	{
		GetWindowText(str);

		int		size = 6;
		CRect	r = rc;
		CRect	rCaption;
		Color	color;
		//DWORD	dwText = DT_SINGLELINE | DT_VCENTER;// | DT_END_ELLIPSIS;
		//DWORD	dwStyle = ::GetWindowLong(m_hWnd, GWL_STYLE);
		//DWORD	dwText;
		DWORD dwStyle = GetStyle();
		DWORD dwText = 0;

		CString str;
		GetWindowText(str);
		TRACE(_T("%s = %d, %d\n"), str, m_button_style, dwStyle);

#if 1
		if (dwStyle & BS_LEFT)
			dwText |= DT_LEFT;
		else if (dwStyle & BS_RIGHT)
			dwText |= DT_RIGHT;
		else// if (dwStyle & BS_CENTER)
			dwText |= DT_CENTER;
#else		
		MAP_STYLE(BS_LEFT,	 DT_LEFT);
		MAP_STYLE(BS_RIGHT,	 DT_RIGHT);
		MAP_STYLE(BS_CENTER, DT_CENTER);
#endif
		dwText |= (DT_SINGLELINE | DT_VCENTER);

		if (m_use_hover && m_bHover)
		{
			dc.SetTextColor(m_crTextHover);
			color.SetFromCOLORREF(m_crTextHover);
			dc.FillSolidRect(rc, m_crBackHover);
		}
		else if (IsWindowEnabled())
		{
			dc.SetTextColor(m_crText);
			color.SetFromCOLORREF(m_crText);
			dc.FillSolidRect(rc, m_crBack);
		}
		else
		{
			dc.SetTextColor(GRAY(128));
			color.SetFromCOLORREF(GRAY(128));
			dc.FillSolidRect(rc, m_crBack);
		}

		g.SetSmoothingMode(SmoothingModeAntiAlias);
		CFont *pOldFont = dc.SelectObject(&m_font);
		dc.SetBkMode(TRANSPARENT);

		if (m_button_style == BS_CHECKBOX)
		{
			r.left += 3;
			r.right = r.left + size * 2 + 1;
			r.top = r.CenterPoint().y - size;
			r.bottom = r.top + size * 2 + 1;
			DrawRectangle(&dc, r, (m_use_hover && m_bHover) ? m_crTextHover : m_crText, RGB(255,255,255));

			Pen pen(Color(255, 32, 32, 32), 1.51);

			if (GetCheck())
			{
				g.DrawLine(&pen, r.left + 1, r.CenterPoint().y - 1, r.left + 4, r.CenterPoint().y + 3);
				g.DrawLine(&pen, r.left + 4, r.CenterPoint().y + 3, r.right - 3, r.top + 3);
			}

			rCaption = r;

			rCaption.left = rCaption.right + 4;
			rCaption.right = rc.right;
		}
		else if (m_button_style == BS_RADIOBUTTON)
		{
			r.left += 3;
			r.right = r.left + size * 2 + 1;
			r.top = r.CenterPoint().y - size;
			r.bottom = r.top + size * 2 + 1;

			Pen pen(color, 0.8);
			SolidBrush br(color);
			g.DrawEllipse(&pen, r.left, r.top, r.Width(), r.Height());

			if (GetCheck())
			{
				g.FillEllipse(&br, r.left + r.Width() / 4, r.top + r.Height() / 4, r.Width() / 1.7, r.Height() / 1.7);
			}

			rCaption = r;
			rCaption.left = rCaption.right + 4;
			rCaption.right = rc.right;
		}
		else
		{
			if (m_b3DRect)
			{
				dc.Draw3dRect(rc, m_bPushed ? GetSysColor(COLOR_3DSHADOW) : GetSysColor(COLOR_3DLIGHT),
					m_bPushed ? GetSysColor(COLOR_3DLIGHT) : GetSysColor(COLOR_3DSHADOW));
				if (m_bPushed)
					r.OffsetRect(-1, -1);
			}

			rCaption = r;
			//rCaption.left += 2;
		}

		dc.DrawText(str, rCaption, dwText);

		dc.SelectObject(pOldFont);
		
		return;
	}

	pt.x = ( rc.Width() - m_width ) / 2.0;
	pt.y = ( rc.Height() - m_height ) / 2.0;


	RectF			grect;
	ImageAttributes ia;


	//�켱 ��� �̹����� �ѷ��ְ�
	if ( m_pBack )
		g.DrawImage( m_pBack, 0, 0 );
	else
		dc.FillSolidRect( rc, m_crBack );

	pImage = m_pImage[m_nIndex];

	//g.SetInterpolationMode(InterpolationModeNearestNeighbor);
	g.SetInterpolationMode(InterpolationModeHighQualityBilinear);
	//g.SetInterpolationMode(InterpolationModeHighQualityBicubic);

	if ( m_bAsStatic )
	{
		g.DrawImage( pImage, pt.x, pt.y, m_width, m_height );
	}
	else
	{
		if ( IsWindowEnabled() == false )
		{
			ia.SetColorMatrix( &GrayMat );

			grect.X = pt.x;
			grect.Y = pt.y;
			grect.Width = (REAL)m_width;
			grect.Height = (REAL)m_height;

			g.DrawImage(pImage, grect, 0, 0, pImage->GetWidth(), pImage->GetHeight(), UnitPixel, &ia);
			//TRACE(_T("w = %d, h = %d\n"), m_width, m_height);
		}
		//�ٿ� �̹���. �ݵ�� hovering���� ���� üũ�Ǿ�� �Ѵ�.
		else if ( /*m_bIsPressed )*/lpDIS->itemState & ODS_SELECTED )
		{
			g.DrawImage( pImage, pt.x + m_down_offset.x, pt.y + m_down_offset.y, m_width, m_height );
			//m_bAltImage = true;
		}
		//
		else if (m_use_hover && m_bHover)
		{
			ia.SetColorMatrices( &HotMat, &GrayMat, ColorMatrixFlagsDefault );

			grect.X = pt.x;
			grect.Y = pt.y;
			grect.Width = (REAL)m_width;
			grect.Height = (REAL)m_height;

			g.DrawImage( pImage, grect, 0, 0, pImage->GetWidth(), pImage->GetHeight(), UnitPixel, &ia);
			//g.DrawImage(pImage, pt.x, pt.y, m_width, m_height);// , UnitPixel, & ia);
			TRACE(_T("w = %d, h = %d\n"), m_width, m_height);
		}
		else
		{
			g.DrawImage( pImage, pt.x, pt.y, m_width, m_height );
		}
	}

	if ( m_bShowFocusRect )//&& m_bHasFocus )
	{
		TRACE( "draw focus rect\n" );
		//pDC->DrawFocusRect( rc );
		Color	color;

		color.SetFromCOLORREF( m_crFocusRect );
		Pen	pen( color, m_nFocusRectWidth );
		pen.SetDashStyle( DashStyleDot );
		g.DrawRectangle( &pen, rc.left, rc.top, rc.Width(), rc.Height() );
	}
}


BOOL CGdiButton::OnEraseBkgnd(CDC* pDC)
{
	// TODO: ���⿡ �޽��� ó���� �ڵ带 �߰� ��/�Ǵ� �⺻���� ȣ���մϴ�.
	if ( m_pBack )
		return false;
	else
		return true;

	return CButton::OnEraseBkgnd(pDC);
}


void CGdiButton::OnMouseMove(UINT nFlags, CPoint point)
{
	// TODO: ���⿡ �޽��� ó���� �ڵ带 �߰� ��/�Ǵ� �⺻���� ȣ���մϴ�.
	if (!m_bIsTracking)
	{
		TRACKMOUSEEVENT tme;
		tme.cbSize = sizeof(tme);
		tme.hwndTrack = m_hWnd;
		tme.dwFlags = TME_LEAVE|TME_HOVER;
		tme.dwHoverTime = 1;
		m_bIsTracking = _TrackMouseEvent(&tme);
	}

	CButton::OnMouseMove(nFlags, point);
}


void CGdiButton::OnMouseHover(UINT nFlags, CPoint point)
{
	// TODO: ���⿡ �޽��� ó���� �ڵ带 �߰� ��/�Ǵ� �⺻���� ȣ���մϴ�.
	if (!m_use_hover)
		return;

	m_bHover = true;
	Invalidate();

	CButton::OnMouseHover(nFlags, point);
}


void CGdiButton::OnMouseLeave()
{
	// TODO: ���⿡ �޽��� ó���� �ڵ带 �߰� ��/�Ǵ� �⺻���� ȣ���մϴ�.
	if (!m_use_hover)
		return;

	m_bIsTracking = false;
	m_bHover = false;
	Invalidate();

	CButton::OnMouseLeave();
}

void CGdiButton::OnTimer(UINT_PTR nIDEvent)
{
	// TODO: ���⿡ �޽��� ó���� �ڵ带 �߰� ��/�Ǵ� �⺻���� ȣ���մϴ�.
	if ( nIDEvent == TIMER_BLINK )
	{
		m_bBlinkStatus = !m_bBlinkStatus;
		ShowWindow( m_bBlinkStatus ? SW_SHOW : SW_HIDE );

		KillTimer( TIMER_BLINK );

		if ( m_bBlinkStatus )
			SetTimer( TIMER_BLINK, m_nBlinkTime1, NULL );
		else
			SetTimer( TIMER_BLINK, m_nBlinkTime0, NULL );
	}

	CButton::OnTimer(nIDEvent);
}


void CGdiButton::OnSetFocus(CWnd* pOldWnd)
{
	CButton::OnSetFocus(pOldWnd);

	// TODO: ���⿡ �޽��� ó���� �ڵ带 �߰��մϴ�.
	//TRACE( "set focus\n" );
	m_bHasFocus = true;
	Invalidate();
}


void CGdiButton::OnKillFocus(CWnd* pNewWnd)
{
	CButton::OnKillFocus(pNewWnd);

	// TODO: ���⿡ �޽��� ó���� �ڵ带 �߰��մϴ�.
	//TRACE( "kill focus\n" );
	m_bHasFocus = false;
	Invalidate();
}


BOOL CGdiButton::OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message)
{
	// TODO: ���⿡ �޽��� ó���� �ڵ带 �߰� ��/�Ǵ� �⺻���� ȣ���մϴ�.

	return CButton::OnSetCursor(pWnd, nHitTest, message);
}

void CGdiButton::ReAlign()
{
	int dx, dy;
	CRect	rc, rParentRect;

	GetParent()->GetClientRect( &rParentRect );

	if ( m_nAnchor > ANCHOR_NONE )
	{
		if ( m_nAnchor & ANCHOR_LEFT )
			dx = m_nAnchorMarginX;

		if ( m_nAnchor & ANCHOR_TOP )
			dy = m_nAnchorMarginY;

		if ( m_nAnchor & ANCHOR_RIGHT )
			dx = rParentRect.right - m_width - m_nAnchorMarginX;

		if ( m_nAnchor & ANCHOR_BOTTOM )
			dy = rParentRect.bottom - m_height - m_nAnchorMarginY;

		if ( m_nAnchor & ANCHOR_HCENTER )
			dx = rParentRect.CenterPoint().x - m_width / 2;

		if ( m_nAnchor & ANCHOR_VCENTER )
			dy = rParentRect.CenterPoint().y - m_height / 2;
	
		SetWindowPos( NULL, dx, dy, m_width, m_height, SWP_NOZORDER | SWP_NOSIZE );
		SetBackImage( m_pBackOrigin );
	}
}

//��ư�� ��ġ ����
void CGdiButton::Offset( int x, int y )
{
	CRect	rc;

	//parent�� Ŭ���̾�Ʈ ���� ������ �� ��Ʈ���� ��ǥ�� ���Ѵ�.
	GetWindowRect( rc );
	CPoint	pt = rc.TopLeft();
	::ScreenToClient( GetParent()->m_hWnd, &pt );

	pt.x += x;
	pt.y += y;

	if ( pt.x < 0 )
		pt.x = 0;

	if ( pt.y < 0 )
		pt.y = 0;

	SetWindowPos( &wndNoTopMost, pt.x, pt.y, 0, 0, SWP_NOSIZE | SWP_NOZORDER );	

	SetBackImage( m_pBackOrigin );
}

void CGdiButton::Inflate( int cx, int cy )
{
	Inflate( cx, cy, cx, cy );
}

//��ư ũ�⸦ �ø��ų� ���δ�.
//��ư �̹����� �׻� ��ư ������ �߾ӿ� �׷��ش�.
void CGdiButton::Inflate( int l, int t, int r, int b )
{
	CRect	rc;

	//parent�� Ŭ���̾�Ʈ ���� ������ �� ��Ʈ���� ��ǥ�� ���Ѵ�.
	GetWindowRect( rc );
	CPoint	pt = rc.TopLeft();
	::ScreenToClient( GetParent()->m_hWnd, &pt );

	rc.MoveToXY( pt.x, pt.y );
	rc.InflateRect( l, t, r, b );

	//��� �׸��� �����ߴٸ� ��� ���� ���� ���;� �Ѵ�.
	if ( m_pBack && m_pBackOrigin )
	{
		SafeRelease( &m_pBack );

		Rect cutRect( rc.left, rc.top, rc.Width(), rc.Height() );
		m_pBack = m_pBackOrigin->Clone( cutRect, m_pBackOrigin->GetPixelFormat() );
	}


	SetWindowPos( &wndNoTopMost, rc.left, rc.top, rc.Width(), rc.Height(), SWP_NOZORDER );	
	Invalidate();
}

void CGdiButton::Toggle()
{
	bool b = GetCheck();
	SetCheck( b ? false : true );
}


LRESULT CGdiButton::WindowProc(UINT message, WPARAM wParam, LPARAM lParam)
{
	// TODO: ���⿡ Ư��ȭ�� �ڵ带 �߰� ��/�Ǵ� �⺻ Ŭ������ ȣ���մϴ�.

// 	CString str;
// 	GetWindowText( str );

	//TRACE( "%s\n", str );
	//if ( str == "Radio3" )

/*
	if ( message == WM_GETDLGCODE )
		return (DLGC_BUTTON | DLGC_RADIOBUTTON);

	if ( message == BM_SETCHECK )
	{
		if ( wParam == BST_CHECKED )
		//TRACE( "%d, %d, %d\n", message, wParam, lParam );
		TRACE( "checked\n" );

	 	else if ( message == BST_UNCHECKED )
		{
			TRACE( "unchecked\n" );
		}
	}
*/


	return CButton::WindowProc(message, wParam, lParam);
}

void CGdiButton::SetBlinkTime( int nTime0 /*= 500*/, int nTime1 /*= 500*/ )
{
	KillTimer( TIMER_BLINK );

	m_nBlinkTime0		= nTime0;
	m_nBlinkTime1		= nTime1;

	SetBlink( m_bBlink );
}

void CGdiButton::SetBlink( BOOL bBlink /*= TRUE*/ )
{
	m_bBlink = bBlink;
	m_bBlinkStatus = FALSE;

	if ( m_bBlink )
	{
		SetTimer( TIMER_BLINK, m_nBlinkTime0, NULL );
	}
	else
	{
		m_bBlink = false;
		KillTimer( TIMER_BLINK );
		ShowWindow( SW_SHOW );
		//UpdateSurface();
	}
}

CGdiButton& CGdiButton::SetTextColor(COLORREF crColor)
{
	m_crText = crColor; // Passing the value passed by the dialog to the member varaible for Text Color
	RedrawWindow();

	return *this;
}

CGdiButton& CGdiButton::SetTextHoverColor(COLORREF crTextHover)
{
	m_crTextHover = crTextHover; // Passing the value passed by the dialog to the member varaible for Text Color
	RedrawWindow();

	return *this;
}

CGdiButton& CGdiButton::SetFontName(LPCTSTR sFontname, BYTE byCharSet)
{
	m_lf.lfCharSet = byCharSet;
	_tcscpy_s(m_lf.lfFaceName, _countof(m_lf.lfFaceName), sFontname);
	ReconstructFont();

	return *this;
}

CGdiButton& CGdiButton::SetFontSize( int nSize )
{
	m_lf.lfHeight = nSize;
	ReconstructFont();

	return *this;
}

CGdiButton& CGdiButton::SetFontBold( bool bBold )
{
	m_lf.lfWeight = ( bBold ? FW_BOLD : FW_NORMAL );
	ReconstructFont();

	return *this;
}


void CGdiButton::OnLButtonUp(UINT nFlags, CPoint point)
{
	// TODO: Add your message handler code here and/or call default
	//if ( m_button_style == BS_CHECKBOX )
		//Toggle();
		Invalidate(false);

	CButton::OnLButtonUp(nFlags, point);
}

void CGdiButton::use_hover(bool use)
{
	m_use_hover = use;
}
