// GdiButton.cpp : 구현 파일입니다.
//

#include "stdafx.h"
#include "GdiButton.h"

#include "../MemoryDC.h"

#define	TIMER_BLINK		0

ColorMatrix GrayMat = {	0.30f, 0.30f, 0.30f, 0.00f, 0.00f,
						0.59f, 0.59f, 0.59f, 0.00f, 0.00f,
						0.11f, 0.11f, 0.11f, 0.00f, 0.00f,
						0.00f, 0.00f, 0.00f, 1.00f, 0.00f,
						0.00f, 0.00f, 0.00f, 0.00f, 1.00f	};

ColorMatrix HotMat = {	1.05f, 0.00f, 0.00f, 0.00f, 0.00f,
						0.00f, 1.05f, 0.00f, 0.00f, 0.00f,
						0.00f, 0.00f, 1.05f, 0.00f, 0.00f,
						0.00f, 0.00f, 0.00f, 1.00f, 0.00f,
						0.05f, 0.05f, 0.05f, 0.00f, 1.00f	};

// CGdiButton

IMPLEMENT_DYNAMIC(CGdiButton, CButton)

CGdiButton::CGdiButton()
{
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
	m_bIsPressed		= false;
	m_bIsHovering		= false;
	m_bIsTracking		= false;

	m_crBack			= ::GetSysColor( COLOR_3DFACE );

	m_bHasFocus			= false;
	m_bShowFocusRect	= false;
	m_crFocusRect		= RGB( 6, 205, 255 );
	m_nFocusRectWidth	= 2;

	m_bBlink		= FALSE;
	m_bBlinkStatus	= FALSE;
	m_nBlinkTime0	= 400;
	m_nBlinkTime1	= 1200;

	m_bFitToImage	= true;
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
END_MESSAGE_MAP()



// CGdiButton 메시지 처리기입니다.

//한 버튼은 최대 10장의 이미지를 담을 수 있으며 0번이 normal상태 이미지, 그리고 checked 상태로 시작된다.
bool CGdiButton::AddImage( CString sfile, Bitmap* pBack, COLORREF crBack, bool bFitToImage, int dx, int dy, UINT nAnchor, bool bAsStatic, bool bShowError )
{
	if ( m_nImages >= MAX_BUTTON_IMAGE )
		return false;

	//첫 장은 반드시 배경 정보를 포함해야 한다.
// 	if ( m_nImages == 0 && pBack == NULL )
// 		return false;

	Bitmap	*bitmap = Load( sfile );

	if ( bitmap == NULL )
	{
		if ( bShowError )
			AfxMessageBox( sfile + "\n\n위 파일이 존재하지 않거나 열 수 없습니다." );
		return false;
	}

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
			m_pImage[m_nImages] = (Bitmap*)(m_pImage[m_nImages]->GetThumbnailImage( rc.Width(), rc.Height() ));
			m_width	= m_pImage[0]->GetWidth();
			m_height = m_pImage[0]->GetHeight();
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
		//추가되는 이미지들은 모두 0번 이미지와 동일한 크기여야 한다.
		m_pImage[m_nImages] = (Bitmap*)(m_pImage[m_nImages]->GetThumbnailImage( m_width, m_height ));
	}

	m_nImages++;

	//radio 버튼은 1개의 이미지를 가지며 처음 시작시 unchecked로 시작된다.
	//unchecked 이미지는 setcheck에서 자동으로 생성해준다.
	if ( m_nButtonStyle == BS_AUTORADIOBUTTON && m_nImages == 1 )
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

//1번 이미지는 true, 0번 이미지는 false (checkbox나 radiobutton과 같은 용도로 사용할 수 있다)
bool CGdiButton::GetCheck()
{
// 	if ( m_nButtonStyle != BS_CHECKBOX )
// 		m_nButtonStyle = BS_CHECKBOX;

	return (m_nIndex == 1);
}

//true이면 0번 이미지를 표시해준다. false일 경우는 1번을 표시하는데 만약 없으면 AddGrayImage로 회색 이미지를 자동 추가한 후 선택해준다.
void CGdiButton::SetCheck( bool bCheck )
{
// 	if ( this == NULL )
// 		return;

 	if ( m_nImages == 0 )
 		return;

	if ( m_pImage[1] == NULL )
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

	//radio 버튼이 눌려지거나 SetCheck( true )가 호출되면
	//같은 group 내의 다른 버튼들은 unchecked로 만들어 줘야한다.
	//owner draw 속성때문에 WindowProc의 윈도우 기본 메시지를 받아서 처리할 수 없다.
	if ( (m_nButtonStyle == BS_AUTORADIOBUTTON) && bCheck )
	{
		SetFocus();

		CGdiButton*	pButton = (CGdiButton*)( GetParent()->GetNextDlgGroupItem( this, true ) );

		while ( pButton != NULL && pButton != this )
		{
			pButton->SetCheck( false );
			pButton = (CGdiButton*)( GetParent()->GetNextDlgGroupItem( pButton, true ) );
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
}

//그림의 크기에 맞게 컨트롤을 resize하고 dx, dy, nAnchor에 따라 move해준다.
void CGdiButton::ResizeControl( int& dx, int& dy )
{
	CRect	rc, rParentRect;

	GetParent()->GetClientRect( &rParentRect );

	//parent의 클라이언트 영역 내에서 이 컨트롤의 좌표를 구한다.
	//ScreenToClient( rc );와 ::ScreenToClient( GetParent()->m_hWnd, &pt );의 rc.topleft와 pt는 다른 결과를 보인다.
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

	SetWindowPos( NULL, dx, dy, m_width, m_height, SWP_NOZORDER );
}

bool CGdiButton::AddGrayImage( bool bSelectGray )
{
	if ( m_nImages <= 0 )
		return false;

	Rect rc( 0, 0, m_width, m_height );

	//gray image는 1번에 존재한다. 만약 1번이 존재하면 지우고...
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
	// TODO: 여기에 특수화된 코드를 추가 및/또는 기본 클래스를 호출합니다.
	// Set control to owner draw
// 	UINT nBS;
// 
// 	nBS = GetButtonStyle();
// 
// 	// Set initial control type
// 	m_nButtonStyle = nBS & BS_TYPEMASK;

	CString str;
	GetWindowText( str );
	// Check if this is a checkbox
// 	if ( nBS & BS_CHECKBOX )
// 		m_nButtonStyle = BS_CHECKBOX;
// 	else if ( nBS & BS_RADIOBUTTON )
// 		m_nButtonStyle = BS_RADIOBUTTON;
// 	else
// 		m_nButtonStyle = BS_PUSHBUTTON;

	m_nButtonStyle = GetButtonStyle();

	switch ( m_nButtonStyle )
	{
		case BS_PUSHBUTTON		:
		case BS_DEFPUSHBUTTON	:	m_nButtonStyle = BS_PUSHBUTTON;
									break;

		case BS_RADIOBUTTON		:
		case BS_AUTORADIOBUTTON :	m_nButtonStyle = BS_AUTORADIOBUTTON;
									break;

		case BS_CHECKBOX		:
		case BS_AUTOCHECKBOX	:	m_nButtonStyle = BS_AUTOCHECKBOX;
									break;
	}

	SetButtonStyle( m_nButtonStyle );

	ModifyStyle( 0, BS_OWNERDRAW, SWP_FRAMECHANGED );
	ModifyStyle( 0, WS_TABSTOP, WS_TABSTOP );

	CButton::PreSubclassWindow();
}


BOOL CGdiButton::PreTranslateMessage(MSG* pMsg)
{
	// TODO: 여기에 특수화된 코드를 추가 및/또는 기본 클래스를 호출합니다.
	switch ( pMsg->message )
	{
		case WM_LBUTTONDBLCLK	:	return true;
									pMsg->message = WM_LBUTTONDOWN;
									break;

		//LButtonDown과 Up을 기본 메시지 핸들러를 통해 처리하면 메인에서 버튼 클릭 이벤트가 발생되지 않는다.
		//여기서 return false로 하면 메인에서 버튼 클릭 이벤트가 처리된다.
		//단, 여기서도 SetCapture와 ReleaseCapture를 사용하면 역시 메인에서 이벤트가 발생되지 않는다.
		case WM_LBUTTONDOWN		:	if ( m_bAsStatic )
										return true;

									m_bIsPressed = true;

									Invalidate();
									return false;

		case WM_LBUTTONUP		:	if ( m_bAsStatic )
										return true;

									if ( m_bIsPressed )
									{
										m_bIsPressed = false;

										CRect	rc;
										GetWindowRect( rc );

										//버튼 영역 내에서 떼야만 의미있다.
										//또한 버튼 스타일이 check box 이면 토글되고
										//radio button 이면 같은 그룹내의 다른 radio button들은 unchecked 시킨다.
										if ( rc.PtInRect( pMsg->pt ) )
										{
											if ( m_nButtonStyle == BS_AUTOCHECKBOX )
												Toggle();
											else if ( m_nButtonStyle == BS_AUTORADIOBUTTON )
												SetCheck( true );
										}

										Invalidate();
									}

									return false;
	}

	return CButton::PreTranslateMessage(pMsg);
}


void CGdiButton::DrawItem(LPDRAWITEMSTRUCT lpDIS/*lpDrawItemStruct*/)
{
	if ( IsWindowVisible() == false || m_nImages <= 0 || m_nIndex < 0 )
		return;

	CDC*		pDC1 = CDC::FromHandle(lpDIS->hDC);
	CRect		rc;
	CPoint		pt( 0, 0 );
	Bitmap*		pImage = m_pImage[m_nIndex];

	if ( pImage == NULL )
		return;

	GetClientRect( rc );

	//배경을 그려주고 이미지를 그리므로 여기서는 CMemoryDC를 이용하여 더블버퍼링을 해줘야 안깜빡인다.
	CMemoryDC	pDC( pDC1, &rc );//, true );
	//CDC*		pDC = pDC1;


	pt.x = ( rc.Width() - m_width ) / 2.0;
	pt.y = ( rc.Height() - m_height ) / 2.0;


	Graphics		g( pDC->m_hDC, rc );
	RectF			grect;
	ImageAttributes ia;


	//우선 배경 이미지를 뿌려주고
	if ( m_pBack )
		g.DrawImage( m_pBack, 0, 0 );
	else
		pDC->FillSolidRect( rc, m_crBack );

	pImage = m_pImage[m_nIndex];

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

			g.DrawImage( pImage, grect, 0, 0, m_width, m_height, UnitPixel, &ia);
		}
		//다운 이미지. 반드시 hovering보다 먼저 체크되어야 한다.
		else if ( /*m_bIsPressed )*/lpDIS->itemState & ODS_SELECTED )
		{
			g.DrawImage( pImage, pt.x + 1, pt.y + 1, m_width, m_height );
			//m_bAltImage = true;
		}
		//
		else if ( m_bIsHovering )
		{
			ia.SetColorMatrix( &HotMat );

			grect.X = pt.x;
			grect.Y = pt.y;
			grect.Width = (REAL)m_width;
			grect.Height = (REAL)m_height;

			g.DrawImage( pImage, grect, 0, 0, m_width, m_height, UnitPixel, &ia);
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
	// TODO: 여기에 메시지 처리기 코드를 추가 및/또는 기본값을 호출합니다.
	if ( m_pBack )
		return false;
	else
		return true;

	return CButton::OnEraseBkgnd(pDC);
}


void CGdiButton::OnMouseMove(UINT nFlags, CPoint point)
{
	// TODO: 여기에 메시지 처리기 코드를 추가 및/또는 기본값을 호출합니다.
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
	// TODO: 여기에 메시지 처리기 코드를 추가 및/또는 기본값을 호출합니다.
	m_bIsHovering = true;
	Invalidate();

	CButton::OnMouseHover(nFlags, point);
}


void CGdiButton::OnMouseLeave()
{
	// TODO: 여기에 메시지 처리기 코드를 추가 및/또는 기본값을 호출합니다.
	m_bIsTracking = false;
	m_bIsHovering = false;
	Invalidate();

	CButton::OnMouseLeave();
}

void CGdiButton::OnTimer(UINT_PTR nIDEvent)
{
	// TODO: 여기에 메시지 처리기 코드를 추가 및/또는 기본값을 호출합니다.
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

	// TODO: 여기에 메시지 처리기 코드를 추가합니다.
	//TRACE( "set focus\n" );
	m_bHasFocus = true;
	Invalidate();
}


void CGdiButton::OnKillFocus(CWnd* pNewWnd)
{
	CButton::OnKillFocus(pNewWnd);

	// TODO: 여기에 메시지 처리기 코드를 추가합니다.
	//TRACE( "kill focus\n" );
	m_bHasFocus = false;
	Invalidate();
}


BOOL CGdiButton::OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message)
{
	// TODO: 여기에 메시지 처리기 코드를 추가 및/또는 기본값을 호출합니다.

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

//버튼의 위치 변경
void CGdiButton::Offset( int x, int y )
{
	CRect	rc;

	//parent의 클라이언트 영역 내에서 이 컨트롤의 좌표를 구한다.
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

//버튼 크기를 늘리거나 줄인다.
//버튼 이미지는 항상 버튼 영역의 중앙에 그려준다.
void CGdiButton::Inflate( int l, int t, int r, int b )
{
	CRect	rc;

	//parent의 클라이언트 영역 내에서 이 컨트롤의 좌표를 구한다.
	GetWindowRect( rc );
	CPoint	pt = rc.TopLeft();
	::ScreenToClient( GetParent()->m_hWnd, &pt );

	rc.MoveToXY( pt.x, pt.y );
	rc.InflateRect( l, t, r, b );

	//배경 그림이 존재했다면 배경 또한 새로 따와야 한다.
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
	ASSERT( m_nImages >= 2 );

	bool b = GetCheck();
	SetCheck( b ? false : true );
}


LRESULT CGdiButton::WindowProc(UINT message, WPARAM wParam, LPARAM lParam)
{
	// TODO: 여기에 특수화된 코드를 추가 및/또는 기본 클래스를 호출합니다.

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
