// VideoWnd.cpp : implementation file
//

#include "VideoWnd.h"

#include "MemoryDC.h"
#include "AutoFont.h"

#include "messagebox/Win32InputBox/Win32InputBox.h"

#define CVIDEOWND_CLASSNAME _T("CVideoWnd")


#define TIMER_PLAY_VIDEO		0

// CVideoWnd

IMPLEMENT_DYNAMIC(CVideoWnd, CWnd)

CVideoWnd::CVideoWnd()
{
	if( !RegisterWindowClass() )
		return;

	m_nVideoWndID			= 0;
	m_hParentWnd			= NULL;

	m_bAutoPlayWhenDrop		= true;

	m_bUsePopupMenu			= true;
	m_bPopupMenu			= false;
	m_bSaveRecentFrame		= AfxGetApp()->GetProfileInt( _T("setting\\video"), _T("save recent frame"), 0 );;
	m_nSaveIndex			= 0;
	m_bImageFromClipboard	= false;

	m_sVideoFileName		= _T("");
	m_nFileType				= FILE_TYPE_UNKNOWN;

	m_szImageSize			= CSize(0, 0);
	m_szImageSizeOrigin		= CSize(0, 0);

	m_nTotalFrame			= 0;
	m_nCurrentFrame			= 0;

	//bin or stereo image
	//mando rear cam
	m_bMandoRearCam			= false;
	if ( m_bMandoRearCam )
	{
		m_bStereoBin		= false;
		m_nBinWidth			= 1280;
		m_nBinHeight		= 724;
	}
	else
	{
		m_bStereoBin		= AfxGetApp()->GetProfileInt( _T("setting\\video"), _T("is stereo bin"), false );
		m_nBinWidth			= AfxGetApp()->GetProfileInt( _T("setting\\video"), _T("bin width"), 1920 );
		m_nBinHeight		= AfxGetApp()->GetProfileInt( _T("setting\\video"), _T("bin height"), 1080 );
	}

	imgYUV					= NULL;
	m_yuv_origin			= NULL;
	m_yuv_format			= AfxGetApp()->GetProfileInt( _T("setting\\video"), _T("yuv format"), yuv422_uyvy );

	//현재 stereo bin은 uyvy만 지원된다.
	if ( m_bStereoBin )
	{
		m_yuv_format = yuv422_uyvy;
		AfxGetApp()->WriteProfileInt( _T("setting\\video"), _T("yuv format"), m_yuv_format );
	}


	m_rImageROI.SetRectEmpty();

	m_dResizeRatio			= 1.0;

	//opencv 기본값은 INTER_LINEAR(1)이지만 확대, 축소에 따라, 목적에 따라 결과가 달라진다.
	//확대 : INTER_AREA, INTER_NEAREST는 원본과 유사하고 LINEAR는 부드러운 엣지, CUBIC과 LANZCOS4는 선명한 엣지를 만듬.
	//축소 : INTER_AREA는 Anti-Aliasing 처리되고 나머지는 거친 엣지를 만듬.
	//따라서 원본의 엣지 특징을 그대로 살릴 것인지(영상처리용),
	//부드럽게 조정할 것인지(사진용) 선택해야 한다.
	//http://tanbakuchi.com/posts/comparison-of-openv-interpolation-algorithms/
	m_nInterpolation		= INTER_LINEAR;
	//m_mat.data				= NULL;

	m_VideoFPS				= 0.0;

	m_bUseOwnFPS			= true;
	m_dTime0 = m_dTime1		= 0.0;
	m_dCurrentFPS			= 0.0;
	m_dCurrentProcessingTime= 0.0;


	m_bSendPlayWndInfo		= false;

	m_bRepeat				= AfxGetApp()->GetProfileInt(_T("setting\\video"), _T("repeat"), false);
;
	m_nPlayState			= STOP;
	m_bReverse				= false;

	m_bLButtonDown			= false;
	m_bROI_Set				= false;
	m_bROI_Move				= false;
	m_bLButtonDown4ROIMove	= false;

	m_bGetVanishingPoint	= false;
	m_bShowVanishingPoint	= false;
	m_nVanishingPointCount	= 0;
	m_ptVanishingPoint		= CPoint(0, 0);

	m_bMeasureSize			= false;

	m_hCursor				= NULL;
	m_bUseCrossCursor		= false;

	m_crTextColor			= RGB( 64, 255, 64 );
	m_crTextBkColor			= RGB( 32, 32, 32 );

	m_bDrawFocusRect		= false;
	m_bTrackBarDown			= false;

	for ( int i = 0; i < 4; i++ )
	{
		m_bShowInfoText[i] = true;
		m_crInfoText[i] = -1;
	}

	m_bUsePatternBackground	= false;
	m_crBackground			= GRAY(64);
	m_crBorder				= -1;	//-1 : no border

	for ( int i = 0; i < BMPX; i++ )
	for ( int j = 0; j < BMPX; j++ )
	{
		if ( i < BMPX/2 )
		{
			if ( j < BMPX/2 )
				m_Bitmap_Pattern[i*BMPX + j] = 128;
			else
				m_Bitmap_Pattern[i*BMPX + j] = 140;
		}
		else
		{
			if ( j < BMPX/2 )
				m_Bitmap_Pattern[i*BMPX + j] = 140;
			else
				m_Bitmap_Pattern[i*BMPX + j] = 128;
		}
	}

	m_bUseImageArray = false;

	m_auto_next_video = AfxGetApp()->GetProfileInt( _T("setting\\video"), _T("auto next video"), false );
}

CVideoWnd::~CVideoWnd()
{
}

BOOL CVideoWnd::RegisterWindowClass()
{
	WNDCLASS wndcls;
	HINSTANCE hInst = AfxGetInstanceHandle();

	if (!(::GetClassInfo(hInst, CVIDEOWND_CLASSNAME, &wndcls)))
	{
		// otherwise we need to register a new class
		CBrush	brush;
		brush.CreateSolidBrush( ::GetSysColor( COLOR_3DFACE ) );

		wndcls.style            = CS_DBLCLKS | CS_HREDRAW | CS_VREDRAW ;
		wndcls.lpfnWndProc      = ::DefWindowProc;
		wndcls.cbClsExtra       = wndcls.cbWndExtra = 0;
		wndcls.hInstance        = hInst;
		wndcls.hIcon            = NULL;
		wndcls.hCursor          = AfxGetApp()->LoadStandardCursor(IDC_ARROW);
		wndcls.hbrBackground    = (HBRUSH) brush.GetSafeHandle();
		wndcls.lpszMenuName     = NULL;
		wndcls.lpszClassName    = CVIDEOWND_CLASSNAME;

		if (!AfxRegisterClass(&wndcls))
		{
			AfxThrowResourceException();
			return FALSE;
		}
	}

	return TRUE;
}

BEGIN_MESSAGE_MAP(CVideoWnd, CWnd)
	ON_WM_PAINT()
	ON_WM_ERASEBKGND()
	ON_WM_LBUTTONDOWN()
	ON_WM_LBUTTONUP()
	ON_WM_LBUTTONDBLCLK()
	ON_WM_MOUSEMOVE()
	ON_WM_MOUSEHOVER()
	ON_WM_DESTROY()
	ON_WM_TIMER()
	ON_WM_CREATE()
	ON_WM_SETCURSOR()
	ON_WM_RBUTTONDOWN()
	ON_WM_RBUTTONUP()
	ON_WM_MBUTTONDOWN()
	ON_WM_MBUTTONUP()
	ON_WM_MOUSEWHEEL()
	ON_WM_DROPFILES()

	ON_COMMAND_RANGE(id_menu_image_info, id_menu_save_as_displayed_size, OnPopupMenu)
	ON_WM_SETFOCUS()
	ON_WM_KILLFOCUS()
	ON_WM_ACTIVATE()
	ON_WM_ACTIVATEAPP()
	ON_WM_EXITMENULOOP()
	ON_WM_WINDOWPOSCHANGED()
	ON_WM_KEYDOWN()
END_MESSAGE_MAP()



// CVideoWnd message handlers
void CVideoWnd::OnPaint()
{
	CPaintDC dc1(this); // device context for painting
	// TODO: Add your message handler code here
	// Do not call CWnd::OnPaint() for painting messages

	int			i, j;
	CRect		rc;

	GetClientRect( rc );
	CMemoryDC	dc( &dc1, &rc, false );

	m_rDisplayedImageRect = rc;

	//단순색으로 채우기보다는 패턴으로 채워줘야 영상 영역과 배경이 확연히 구분된다.
	//창의 크기에 따라 로드가 많아진다. m_bUsePatternBackground를 default로 false로 변경한다.

	if (m_bUsePatternBackground )
	{
		CDC			bitmapDC;
		CBitmap		bmp;
		CBitmap*	pOldBmp = NULL;

		//비트맵 격자 무늬를 영상 배경으로 깔아준다.
		bitmapDC.CreateCompatibleDC( &dc );
		bmp.m_hObject = MakeDIBSection( bitmapDC, BMPX, BMPX );
		bmp.SetBitmapBits( BMPX * BMPX, m_Bitmap_Pattern );

		pOldBmp = bitmapDC.SelectObject( &bmp );
		for ( i = 0; i < rc.bottom; i += BMPX )
		for ( j = 0; j < rc.right; j += BMPX )
			dc.BitBlt( j, i, BMPX, BMPX, &bitmapDC, 0, 0, SRCCOPY );
	}
	else
	{
		dc.FillSolidRect( rc, m_crBackground );
	}

	//미디어를 안열었거나 클립보드에서 복사된 이미지가 아니라도
	//m_mat이 유효하면 무조건 화면에 뿌려야하지 않을까? (재생용이 아닌 이미지 표시용 VideoWnd일 경우)
	//왜 아래와 같은 체크를 했는지 확인해야 한다.
	//if (IsVideoFileOpened() || m_bImageFromClipboard)
	{
		if (m_mat.data)
		{
			m_rDisplayedImageRect = GetRatioRect(rc, m_mat.cols, m_mat.rows);
			cv_draw(&dc, m_mat, &m_rDisplayedImageRect, GRAY32, -1.0);
		}
		else if ( m_sVideoFileName != _T("") )
		{
			dc.SetBkMode(TRANSPARENT);
			dc.SetTextColor( RGB(242, 231, 202) );
			draw_center_text( &dc, _T("File not found or corrupted image file.\n\nOr check the size and format of raw, yuv image."), rc );
			return;
		}
	}

	if ( IsVideoFileOpened() )
	{
		//vanishing point 계산을 위한 포인트 및 라인 표시
		if (m_bGetVanishingPoint)
		{
			for ( i = 0; i < m_nVanishingPointCount; i++ )
			{
				if ( i == 1 )
					DrawLinePt( &dc, m_ptVanishingPointClicked[0], m_ptVanishingPointClicked[1], orange, 2 );
				else if ( i == 3 )
					DrawLinePt( &dc, m_ptVanishingPointClicked[2], m_ptVanishingPointClicked[3], orange, 2 );

				CRect	r(m_ptVanishingPointClicked[i], m_ptVanishingPointClicked[i]);
				r.InflateRect( 4, 4 );
				DrawRectangle( &dc, r, red, NULL_BRUSH );
			}

			if ( m_nVanishingPointCount % 2 == 1 )
			{
				CPoint ptCur;
				GetCursorPos( &ptCur );
				ScreenToClient( &ptCur );
				DrawLinePt( &dc, m_ptVanishingPointClicked[m_nVanishingPointCount - 1], ptCur, orange, 1 );
			}
		}

		if ( m_bShowVanishingPoint )//&& (m_ptVanishingPoint != CPoint(0,0)) )
		{
			CPoint pt;
			get_screen_coord_from_real_coord( m_rDisplayedImageRect, m_mat.cols, m_ptVanishingPoint, &pt);
			DrawLine( &dc, m_rDisplayedImageRect.left, pt.y, m_rDisplayedImageRect.right, pt.y, yellow, 1, PS_DASH );
			DrawLine( &dc, pt.x, m_rDisplayedImageRect.top, pt.x, m_rDisplayedImageRect.bottom, yellow, 1, PS_DASH );
		}

		//설정중인 ROI 사각형 표시
		TRACE(_T("%s\n"), get_rect_info_string(m_rScreenROI, 2));
		DrawRectangle( &dc, m_rScreenROI, red, NULL_BRUSH, 2 );

		dc.SetBkMode( TRANSPARENT );

		UINT uOldFormat;
		CRect	r = m_rSizeInfo;
		CString	str;
		CPoint	pt;

		//TRACE( "%s\n", GetRectInfoString( m_rSizeInfo, 1 ) );

		//크기 및 위치 측정 사각형 표시
		if ( m_mat.data && m_bMeasureSize && (m_rSizeInfo.TopLeft() != CPoint(0,0)) )
		{
			COLORREF crText1 = RGB(255, 255, 0);
			COLORREF crText2 = RGB(0, 255, 0);
			COLORREF crShadow = RGB( 64, 64,  64);
			//ConvertCoordinateScreen2Image( r );
			get_real_coord_from_screen_coord(m_rDisplayedImageRect, m_mat.cols, m_rSizeInfo, &r);
			r.OffsetRect( m_rImageROI.TopLeft() );
			DrawRectangle( &dc, m_rSizeInfo, crText1, NULL_BRUSH, 1, PS_DASH, R2_XORPEN );

			uOldFormat = dc.SetTextAlign( TA_RIGHT | TA_BOTTOM );
			str.Format(_T("(%d, %d)"), r.left, r.top );
			TextOutOutline( &dc, m_rSizeInfo.left, m_rSizeInfo.top, str, crText1, crShadow);

			dc.SetTextAlign( TA_LEFT | TA_TOP );
			str.Format(_T("(%d, %d)"), r.right, r.bottom );
			TextOutOutline( &dc, m_rSizeInfo.right, m_rSizeInfo.bottom, str, crText1, crShadow);

			dc.SetTextAlign( uOldFormat );
			str.Format(_T("%.1f (%d x %d)"), GetDistance( r.TopLeft(), r.BottomRight() ), r.Width(), r.Height() );
			DrawTextOutline( &dc, str, m_rSizeInfo, DT_CENTER | DT_SINGLELINE | DT_VCENTER | DT_NOCLIP, crText2, crShadow);
		}

		CAutoFont	Font( _T("FixedSys") );
		Font.SetHeight( 20 );
		//Font.SetBold( false );
		CFont *pOldFont = (CFont*)dc.SelectObject( &Font );

		//정보 문자열을 표시한다.
		int gap = 3;
		int w = rc.Width() / 2, h = 20;
		UINT nFormat;
		CRect rText;
		make_rect( m_rInfoText[0], rc.left + gap, rc.top + gap, w, h );
		make_rect( m_rInfoText[1], rc.right - gap - w, gap, w, h );
		make_rect( m_rInfoText[2], rc.left + gap, rc.bottom - gap - h, w, h );
		make_rect( m_rInfoText[3], rc.right - gap - w, rc.bottom - gap - h, w, h );

		for ( i = 0; i < 4; i++ )
		{
			if ( m_bShowInfoText[i] == false )
				continue;

			dc.SetTextColor( RGB( 64, 64, 64 ) );

			if ( i % 2 == 0 )
				nFormat = DT_LEFT | DT_SINGLELINE | DT_VCENTER;
			else
				nFormat = DT_RIGHT | DT_SINGLELINE | DT_VCENTER;

			rText = m_rInfoText[i];
			rText.OffsetRect( 1, 1 );
			dc.DrawText( m_sInfoText[i], rText, nFormat );

			if ( m_crInfoText[i] == -1 )
				dc.SetTextColor( m_crTextColor );
			else
				dc.SetTextColor( m_crInfoText[i] );
			rText.OffsetRect( -1, -1 );
			dc.DrawText( m_sInfoText[i], rText, nFormat );
		}

		dc.SelectObject( pOldFont );
	}

	//border 표시
	if ( m_crBorder != -1 )
		DrawRectangle( &dc, rc, m_crBorder, NULL_BRUSH, m_nBorderWidth, PS_SOLID );//, R2_XORPEN );

	//focus rect 표시
	if ( m_bDrawFocusRect && (this == GetFocus()) )
		DrawRectangle( &dc, rc, RGB( 212, 208, 193 ), NULL_BRUSH, 1, PS_DASH );//, R2_XORPEN );

	//trackbar를 표시해준다.
	if ( !IsVideoFileOpened() || m_nTotalFrame <= 1 || m_bImageFromClipboard)
		return;

	int yoffset = (m_rDisplayedImageRect.Height() > 1050 ? 7 : 5);
	double x = (double)m_nCurrentFrame * (double)(m_rDisplayedImageRect.Width()) / (double)(m_nTotalFrame);
	//dc.FillSolidRect( m_rDisplayedImageRect.left + 1, m_rDisplayedImageRect.bottom - 4, x, 2, RGB(212, 255, 212) );
	//DrawLine( &dc, m_rDisplayedImageRect.left, m_rDisplayedImageRect.bottom - yoffset + 1, m_rDisplayedImageRect.left + x, m_rDisplayedImageRect.bottom - yoffset + 1, GRAY(183) );
	//DrawLine( &dc, m_rDisplayedImageRect.left, m_rDisplayedImageRect.bottom - yoffset + 0, m_rDisplayedImageRect.left + x, m_rDisplayedImageRect.bottom - yoffset + 0, GRAY(96) );
	//DrawLine( &dc, m_rDisplayedImageRect.left, m_rDisplayedImageRect.bottom - yoffset + 1, m_rDisplayedImageRect.left + x, m_rDisplayedImageRect.bottom - yoffset + 1, GRAY(192) );
	//DrawLine( &dc, m_rDisplayedImageRect.left, m_rDisplayedImageRect.bottom - yoffset + 2, m_rDisplayedImageRect.left + x, m_rDisplayedImageRect.bottom - yoffset + 2, RGB(255, 255, 255) );
	//DrawLine( &dc, m_rDisplayedImageRect.left, m_rDisplayedImageRect.bottom - yoffset + 3, m_rDisplayedImageRect.left + x, m_rDisplayedImageRect.bottom - yoffset + 3, RGB(255, 255, 255) );
	//DrawLine( &dc, m_rDisplayedImageRect.left, m_rDisplayedImageRect.bottom - yoffset + 4, m_rDisplayedImageRect.left + x, m_rDisplayedImageRect.bottom - yoffset + 4, RGB(255, 255, 255) );
	//DrawLine( &dc, m_rDisplayedImageRect.left, m_rDisplayedImageRect.bottom - yoffset + 5, m_rDisplayedImageRect.left + x, m_rDisplayedImageRect.bottom - yoffset + 5, GRAY(212) );
	//DrawLine( &dc, m_rDisplayedImageRect.left, m_rDisplayedImageRect.bottom - yoffset + 6, m_rDisplayedImageRect.left + x, m_rDisplayedImageRect.bottom - yoffset + 6, GRAY(164) );
	for ( i = 0; i < 5; i++ )
		DrawLine(	&dc,
					m_rDisplayedImageRect.left,
					m_rDisplayedImageRect.bottom - yoffset + i,
					m_rDisplayedImageRect.left + x,
					m_rDisplayedImageRect.bottom - yoffset + i,
					(i % 4 == 0 ? (i == 0 ? GRAY128 : GRAY(212)) : RGB(0, 121, 212)) );
	DrawLine( &dc, m_rDisplayedImageRect.left + x, m_rDisplayedImageRect.bottom - yoffset, m_rDisplayedImageRect.left + x, m_rDisplayedImageRect.bottom - yoffset + 4, GRAY128 );
	//for resize area test
	/*CRect	side[8];
	GetSideRect( rc, side, 10 );
	for ( i = 0; i < 8; i++ )
		dc.FillSolidRect( side[i], RGB(i*16+128,128,128) );
	*/
}


BOOL CVideoWnd::OnEraseBkgnd(CDC* pDC)
{
	// TODO: Add your message handler code here and/or call default
	return false;
}


void CVideoWnd::OnLButtonDown(UINT nFlags, CPoint point)
{
	SetCapture();

	//shift + LButton = 소실점 설정 임시 기능
	if ( IsShiftPressed() || IsCtrlPressed() )
	{
		get_real_coord_from_screen_coord( m_rDisplayedImageRect, m_mat.cols, point, &m_ptVanishingPoint);

		if ( m_hParentWnd && m_bSendPlayWndInfo )
			::SendMessage( m_hParentWnd, message_video_wnd_vanishingpoint_changed, m_nVideoWndID, 0 );

		if ( m_nPlayState != PLAY )
			Invalidate();
		return;
	}

	//영상내의 크기 측정.
	if ( !m_bLButtonDown && m_mat.data && m_bMeasureSize && m_rDisplayedImageRect.PtInRect(point) )
	{
		m_bLButtonDown = true;
		m_rSizeInfo = CRect( point, point );
		Invalidate();
		return;
	}
	//vanishing point 계산.
	else if ( !m_bLButtonDown && m_mat.data && m_bGetVanishingPoint && m_rDisplayedImageRect.PtInRect(point) )
	{
		//4개의 점을 찍어서 vp좌표를 계산하고 이를 메시지박스로 표시한 후에
		//메시지박스를 닫아도 두 직선의 표시는 일단 유지시킨다.
		//이 때 다시 vp를 측정하기 위해 화면 임의 포인트를 찍으면 그 때 reset시킨다.
		if ( m_nVanishingPointCount == 4 )
		{
			m_nVanishingPointCount = 0;
			Invalidate();
			return;
		}

		m_ptVanishingPointClicked[m_nVanishingPointCount++] = point;
		Invalidate();
		Wait( 100 );

		if ( m_nVanishingPointCount == 4 )
		{
			int isx, isy;
			GetIntersectionPoint(	m_ptVanishingPointClicked[0].x, m_ptVanishingPointClicked[0].y,
									m_ptVanishingPointClicked[1].x, m_ptVanishingPointClicked[1].y,
									m_ptVanishingPointClicked[2].x, m_ptVanishingPointClicked[2].y,
									m_ptVanishingPointClicked[3].x, m_ptVanishingPointClicked[3].y, isx, isy );
			CString str;
			get_real_coord_from_screen_coord(m_rDisplayedImageRect, m_mat.cols, CPoint(isx, isy), &m_ptVanishingPoint);
			if ( m_hParentWnd && m_bSendPlayWndInfo )
				::SendMessage( m_hParentWnd, message_video_wnd_vanishingpoint_changed, m_nVideoWndID, 0 );

			str.Format(_T("vanishing point = %d, %d"), m_ptVanishingPoint.x, m_ptVanishingPoint.y );
			AfxMessageBox( str );
			//m_nVanishingPointCount = 0;
			//Invalidate();
		}
		return;
	}

 	if ( m_sVideoFileName == "" && !m_bImageFromClipboard )
		return;

	//이미 ROI가 설정되어 있는 상태라면 ROI move 기능으로 전환.
	if ( IsROIExist() && m_bROI_Move && !m_bLButtonDown4ROIMove && m_rDisplayedImageRect.PtInRect( point ) )
	{
		//크기가 달라짐으로 인해 영상처리에 오류가 발생할 수 있으므로 일시정지 시킨다.
		m_nPlayState4ROIMove = GetPlayState();
		Play( PAUSE );

		cv::resize( m_matOrigin, m_mat, cv::Size( m_szImageSize.cx, m_szImageSize.cy ), 0.0, 0.0, m_nInterpolation );
		m_bLButtonDown4ROIMove = true;

		CRect	rc;
		GetClientRect( rc );
		m_rDisplayedImageRect = GetRatioRect( rc, m_mat.cols, m_mat.rows );
		m_rScreenROI = m_rImageROI;
		ConvertCoordinateImage2Screen( m_rScreenROI );
		Invalidate();

		//커서를 resize축의 중심 또는 ROI의 중심으로 옮겨준다.
		TRACE(_T("m_nROIResizeDirection = %d\n"), m_nROIResizeDirection );
		if ( m_nROIResizeDirection == 0 )
			m_ptLButtonDown4ROIMove = point = CPoint( m_rScreenROI.left, m_rScreenROI.CenterPoint().y );
		else if ( m_nROIResizeDirection == 1 )
			m_ptLButtonDown4ROIMove = point = CPoint( m_rScreenROI.CenterPoint().x, m_rScreenROI.top );
		else if ( m_nROIResizeDirection == 2 )
			m_ptLButtonDown4ROIMove = point = CPoint( m_rScreenROI.right, m_rScreenROI.CenterPoint().y );
		else if ( m_nROIResizeDirection == 3 )
			m_ptLButtonDown4ROIMove = point = CPoint( m_rScreenROI.CenterPoint().x, m_rScreenROI.bottom );
		else if ( m_nROIResizeDirection == 4 )
			m_ptLButtonDown4ROIMove = point = m_rScreenROI.CenterPoint();

		ClientToScreen( &point );
		SetCursorPos( point.x, point.y );

		return;
	}

	if ( m_rDisplayedImageRect.PtInRect( point ) == false )
		return;

	//ROI 설정 시작
	if ( m_bROI_Set )
	{
		Play( PAUSE );
		m_bLButtonDown = true;

		SetCapture();

		m_rScreenROI.left = m_rScreenROI.right = point.x;
		m_rScreenROI.top = m_rScreenROI.bottom = point.y;
		return;
	}

	//트랙바가 클릭되면
	if ( m_rDisplayedImageRect.PtInRect( point ) && point.y >= m_rDisplayedImageRect.bottom - 20 )
	{
		m_bTrackBarDown = true;
		//SetCapture();

		double frame = (double)m_nTotalFrame * (double)(point.x - m_rDisplayedImageRect.left - 1) / (double)(m_rDisplayedImageRect.Width() - 2);
		GotoFrame( (int)frame );
		return;
	}

	CWnd::OnLButtonDown(nFlags, point);
}


void CVideoWnd::OnLButtonUp(UINT nFlags, CPoint point)
{
	// TODO: Add your message handler code here and/or call default
	ReleaseCapture();

	if ( m_bMeasureSize && m_bLButtonDown && m_rSizeInfo.TopLeft() != CPoint(0,0) )
	{
		m_bLButtonDown = false;
		m_rSizeInfo.SetRectEmpty();
		Invalidate();
		return;
	}

	//동영상 파일이 열린 상태가 아니면 리턴
 	if ( m_sVideoFileName == "" && !m_bImageFromClipboard )
		return;

	//드래그로 ROI 설정중도 아니고 트랙바 다운도 아니면 리턴
	if ( !m_bLButtonDown && !m_bLButtonDown4ROIMove && !m_bTrackBarDown )
		return;


	if ( m_bTrackBarDown )
	{
		m_bTrackBarDown = false;
		return;
	}

	if ( m_bLButtonDown4ROIMove )
	{
		m_bLButtonDown4ROIMove = false;

		//변경된 screen roi를 image roi로 변경해준다.
		CheckScreenROIValidation();
		m_rImageROI = m_rScreenROI;
		ConvertCoordinateScreen2Image( m_rImageROI );
		m_rScreenROI.SetRectEmpty();

		Play( m_nPlayState4ROIMove );
		if ( m_nPlayState4ROIMove == PAUSE || m_nFileType == FILE_TYPE_IMAGE )
		{
			cv::resize( m_matOrigin, m_mat, cv::Size( m_szImageSize.cx, m_szImageSize.cy ), 0.0, 0.0, m_nInterpolation );
			m_mat( cv::Rect(m_rImageROI.left, m_rImageROI.top, m_rImageROI.Width(), m_rImageROI.Height()) ).copyTo( m_mat );
			updateMainMat( &m_mat );
		}

		if ( m_hParentWnd && m_bSendPlayWndInfo )
		{
			::SendMessage( m_hParentWnd, message_video_wnd_roi_changed, m_nVideoWndID, 0 );
			SendTrackChanged();
		}

		return;
	}

	m_bLButtonDown = false;

	//SetCapture상태이므로 마우스는 client영역을 벗어날 수 있다. 보정해준다.
	Clamp( point.x, m_rDisplayedImageRect.left, m_rDisplayedImageRect.right );
	Clamp( point.y, m_rDisplayedImageRect.top, m_rDisplayedImageRect.bottom );

	m_rScreenROI.right		= point.x;
	m_rScreenROI.bottom		= point.y;

	CheckScreenROIValidation();
	if ( m_rScreenROI.IsRectEmpty() )
		return;

	get_real_coord_from_screen_coord(m_rDisplayedImageRect, m_mat.cols, m_rScreenROI, &m_rImageROI);

	//ROI가 설정되면 ROI 영역이 zoom되서 전체 영상으로 표시되므로
	//화면상의 ROI 좌표정보는 초기화시켜야 한다.
	m_rScreenROI.SetRectEmpty();
	m_bROI_Set = false;

	//ROI의 width는 4의 배수로 맞춰준다.
	//=>CopyToClipboard에서 문제가 있었으나 일단 해결되었으므로
	//ROI의 width를 반드시 4의 배수로 보정하지 않아도 된다.
	//다른 곳에서 어떤 오류가 발생할 수 있는지도 검증이 필요하다.
	//m_rImageROI.right	= m_rImageROI.right - m_rImageROI.Width() % 4;

	//pause일때 ROI가 변경되면 영상을 직접 갱신시켜줘야 한다.
	if ( m_nPlayState == PAUSE || m_nFileType == FILE_TYPE_IMAGE || m_bImageFromClipboard)
	{
		if ((m_nFileType > FILE_TYPE_IMAGE) && (m_matOrigin.data == NULL) )
		{
			if ( m_nFileType == FILE_TYPE_VIDEO_BIN )
				ReadFrameFromBinFile();
			else
				m_capture.retrieve( m_matOrigin );
		}
		
		if (m_mat.size() != m_matOrigin.size())
			resize(m_matOrigin, m_mat, cv::Size(m_szImageSize.cx, m_szImageSize.cy), 0.0, 0.0, m_nInterpolation);

		m_mat(cv::Rect(m_rImageROI.left, m_rImageROI.top, m_rImageROI.Width(), m_rImageROI.Height())).copyTo(m_mat);
		updateMainMat(&m_mat);
	}

	if ( m_hParentWnd && m_bSendPlayWndInfo )
	{
		::SendMessage( m_hParentWnd, message_video_wnd_roi_changed, m_nVideoWndID, 0 );
		SendTrackChanged();
	}

	CWnd::OnLButtonUp(nFlags, point);
}


void CVideoWnd::OnLButtonDblClk(UINT nFlags, CPoint point)
{
	Play( PLAY_TOGGLE );
	CWnd::OnLButtonDblClk(nFlags, point);
}


void CVideoWnd::OnMouseMove(UINT nFlags, CPoint point)
{
	// TODO: Add your message handler code here and/or call default
	if ( IsVideoFileOpened() )
	{
		CPoint ptOnRealImage;
		get_real_coord_from_screen_coord(m_rDisplayedImageRect, m_mat.cols, point, &ptOnRealImage);
		CMouseEvent me( ptOnRealImage, WM_MOUSEMOVE );
		::SendMessage( m_hParentWnd, message_video_wnd_mouse_event, m_nVideoWndID, (LPARAM)&me );
	}

	/*
	if ( m_bShowInfoText[2] )
	{
		if ( IsROIExist() )
			m_sInfoText[2].Format( "(%d, %d) (%d, %d)", ptOnRealImage.x, ptOnRealImage.y, ptOnRealImage.x + GetImageROI().left, ptOnRealImage.y + GetImageROI().top );
		else
			m_sInfoText[2].Format( "(%d, %d)", ptOnRealImage.x, ptOnRealImage.y );
		m_sInfoText[2].Replace( "-1", "-" );
		DisplayInfoText( 2 );
	}
	*/
	if ( m_bMeasureSize && m_bLButtonDown )
	{
		Clamp( point.x, m_rDisplayedImageRect.left, m_rDisplayedImageRect.right );
		Clamp( point.y, m_rDisplayedImageRect.top, m_rDisplayedImageRect.bottom );
		m_rSizeInfo.right = point.x;
		m_rSizeInfo.bottom= point.y;
		Invalidate();
		return;
	}


	if ( m_sVideoFileName == "" && !m_bImageFromClipboard )
		return;

	//
	if ( m_bGetVanishingPoint )
	{
		Invalidate();
		return;
	}

	//ROI가 설정된 상태이고 드래그로 ROI를 resize 또는 move하는 상태...
	if ( m_bLButtonDown4ROIMove )
	{
		//이미지 표시 영역을 벗어나지 않도록 보정해야 한다.
		CRect rScreenROI = m_rScreenROI;

		if ( m_nROIResizeDirection == 0 )
			rScreenROI.left += ( point.x - m_ptLButtonDown4ROIMove.x );
		else if ( m_nROIResizeDirection == 1 )
			rScreenROI.top += ( point.y - m_ptLButtonDown4ROIMove.y );
		else if ( m_nROIResizeDirection == 2 )
			rScreenROI.right += ( point.x - m_ptLButtonDown4ROIMove.x );
		else if ( m_nROIResizeDirection == 3 )
			rScreenROI.bottom += ( point.y - m_ptLButtonDown4ROIMove.y );
		else if ( m_nROIResizeDirection == 4 )
			rScreenROI.OffsetRect( point - m_ptLButtonDown4ROIMove );

		if ( m_rDisplayedImageRect.PtInRect( point ) == false )
		{
			Clamp( point.x, m_rDisplayedImageRect.left, m_rDisplayedImageRect.right - 1 );
			Clamp( point.y, m_rDisplayedImageRect.top, m_rDisplayedImageRect.bottom - 1 );
			ClientToScreen( &point );
			SetCursorPos( point.x, point.y );
		}

		m_ptLButtonDown4ROIMove = point;

		//point = rScreenROI.CenterPoint();
		//ClientToScreen( &point );
		//SetCursorPos( point.x, point.y );


		if ( RectInRect( m_rDisplayedImageRect, rScreenROI ) )
		{
			m_rScreenROI = rScreenROI;
			Invalidate();
		}
		return;
	}

	//트랙바가 클릭되어 이동중이라면
	if ( m_bTrackBarDown )//m_rDisplayedImageRect.PtInRect( point ) && point.y >= m_rDisplayedImageRect.bottom - 15 )
	{
		double frame = (double)m_nTotalFrame * (double)(point.x - m_rDisplayedImageRect.left) / (double)(m_rDisplayedImageRect.Width());

		//if ( m_nPlayState == PAUSE || m_nFileType == FILE_TYPE_IMAGE )
		{
			CString str;

			str.Format(_T("%d / %d"), (int)frame, m_nTotalFrame );//m_nCurrentFrame );
			if ( m_ToolTip.m_hWnd )
				m_ToolTip.UpdateTipText( str, this );
		}

		//if ( m_bTrackBarDown )
			GotoFrame( (int)frame );

		m_ptMousePosOld = point;

		return;
	}

	//if ( m_ToolTip.m_hWnd )
		//m_ToolTip.UpdateTipText( "", this );


	//ROI 영역 설정중인 상태...
	if ( m_bLButtonDown )
	{
		//SetCapture상태이므로 마우스는 이미지 영역을 벗어날 수 있다. 보정해준다.
		Clamp( point.x, m_rDisplayedImageRect.left, m_rDisplayedImageRect.right );
		Clamp( point.y, m_rDisplayedImageRect.top, m_rDisplayedImageRect.bottom );

		m_rScreenROI.right		= point.x;
		m_rScreenROI.bottom		= point.y;

		//TRACE( "%d, %d -> %d, %d\n", m_rScreenROI.left, m_rScreenROI.top, m_rScreenROI.right, m_rScreenROI.bottom );

		if ( m_nPlayState != PLAY )
			Invalidate( false );
	}

	CWnd::OnMouseMove(nFlags, point);
}

//t0, t1은 정수파트가 sec, 실수파트가 usec까지 표현된 clock값이다.
//ms값이 아님에 주의할 것!
double CVideoWnd::calcFPS(double t0, double t1, int infoIndex)
{
	CString str;
	double fps = 1000.0 / (t1 - t0);

	if ( infoIndex >= 0 )
	{
		str.Format(_T("%.1ffps (%.0fms)"), fps, t1 - t0);
		SetInfoText(infoIndex, str);
	}

	return fps;
}

void CVideoWnd::OnMouseHover(UINT nFlags, CPoint point)
{
	// TODO: Add your message handler code here and/or call default

	CWnd::OnMouseHover(nFlags, point);
}


void CVideoWnd::OnDestroy()
{
	CWnd::OnDestroy();

	// TODO: Add your message handler code here
	CloseVideoFile();
}

//만약 동적으로 m_dResizeRatio가 변경되면
//ROI 크기도 변화량만큼 resize시켜줘야 한다.
//따라서 수동으로 m_dResizeRatio만 변경한다고 해서 영상의 확대비율이
//그냥 적용되지 않으므로 반드시 아래 함수 호출을 통해서 m_dResizeRatio를 적용해야 한다.
//또한 이미 구해진 vp가 있다면 ratio에 맞게 변환시켜주자.
void CVideoWnd::SetResizeRatio( double dRatio )
{
	ASSERT( dRatio >= 0.0 );

	double dRatioOld = m_dResizeRatio;

	m_dResizeRatio = dRatio;
	WriteProfileDouble(AfxGetApp(), _T("setting\\video"), _T("resize ratio"), m_dResizeRatio);

	m_szImageSize.cx = ROUND( (double)m_szImageSizeOrigin.cx * m_dResizeRatio, 0 );
	m_szImageSize.cy = ROUND( (double)m_szImageSizeOrigin.cy * m_dResizeRatio, 0 );

	//ROI도 비율조정해줘야 한다.
	m_rImageROI.left	= (double)m_rImageROI.left * (m_dResizeRatio / dRatioOld);
	m_rImageROI.top		= (double)m_rImageROI.top * (m_dResizeRatio / dRatioOld);
	m_rImageROI.right	= ROUND((double)m_rImageROI.right * (m_dResizeRatio / dRatioOld), 0);
	m_rImageROI.bottom	= ROUND((double)m_rImageROI.bottom * (m_dResizeRatio / dRatioOld), 0);


	if ( m_matOrigin.data == NULL )
		return;

	cv::resize( m_matOrigin, m_mat, cv::Size( m_szImageSize.cx, m_szImageSize.cy ), 0.0, 0.0, m_nInterpolation );

	if (m_rImageROI.right > m_mat.cols)
		m_rImageROI.right = m_mat.cols;
	if (m_rImageROI.bottom > m_mat.rows)
		m_rImageROI.bottom = m_mat.rows;

	m_mat( cv::Rect(m_rImageROI.left, m_rImageROI.top, m_rImageROI.Width(), m_rImageROI.Height()) ).copyTo( m_mat );

	if ( m_ptVanishingPoint != CPoint(0,0) )
	{
		m_ptVanishingPoint.x *= (m_dResizeRatio / dRatioOld);
		m_ptVanishingPoint.y *= (m_dResizeRatio / dRatioOld);
	}

	if ( m_hParentWnd && m_bSendPlayWndInfo )
		::SendMessage( m_hParentWnd, message_video_wnd_roi_changed, m_nVideoWndID, NULL );

	//아래 updateMainMat은 위의 message_video_wnd_roi_changed 메시지 다음에 호출해야 한다.
	//message_video_wnd_roi_changed 메시지 핸들러에 의해 main에서 관련 코드를 모두 처리한 후에
	//updateMainMat에서 main에 track_changed message를 보내야 하기 때문이다.
	updateMainMat( &m_mat );

	//Invalidate();
}

bool CVideoWnd::open_cam(int index, bool auto_start)
{
	m_cam_index = index;
	return OpenVideoFile(_T("cam"), false, _T(""), auto_start);
}

bool CVideoWnd::OpenVideoFile( CString sfile, bool bRunFileDialog, CString sRecentFile, bool bPlay )
{
	CString str;
	CString sExt;

	Play( STOP );

	m_hParentWnd = GetParent()->GetSafeHwnd();
	m_nFileType = FILE_TYPE_UNKNOWN;
	m_sVideoFileName = _T("");	//정상적으로 열렸을 때만 파일명을 저장해준다.

	if ( imgYUV != NULL )
	{
		delete [] imgYUV;
		imgYUV = NULL;
	}

	if ( sfile == "" )
	{
		if (bRunFileDialog)
		{
			if ( sRecentFile == "" )
				sRecentFile = GetExeDirectory() + _T("\\*.*");

			TCHAR			szFilter[] = _T("Media (*.mp4,*.wmv,*.avi,*.bin,*.yuv,*.bmp,*.jpg,*.png)|*.mp4;*.wmv;*.avi;*.bin;*.yuv;*.bmp;*.jpg,*.png|All Files(*.*)|*.*||");
			CFileDialog		dlg(true, _T("*.*"), sRecentFile, OFN_FILEMUSTEXIST, szFilter, this);

			if ( dlg.DoModal() == IDCANCEL )
				return false;

			sfile = dlg.GetPathName();

			if ( sfile == "" )
				return false;
		}
		else
		{
			m_nFileType = FILE_TYPE_IMAGE;
		}
	}

	sExt = get_part(sfile, fn_ext).MakeLower();

	if ( sfile != "" )
		m_nFileType = GetFileTypeFromExtension( sExt );

	if (m_nFileType != FILE_TYPE_IMAGE || !m_bUseImageArray)
		m_nCurrentFrame = 0;

	//이미지 파일인 경우
	if ( m_nFileType == FILE_TYPE_IMAGE )
	{
		m_bStereoBin = false;

		if ( sfile == "" )
		{
			m_matOrigin = Mat( 360, 640, CV_8UC3, Scalar(0,0,0) );
			m_sVideoFileName = sfile = _T("untitled");
		}
		else
		{
			m_sVideoFileName = sfile;

			m_matOrigin = loadImage( sfile, IMREAD_UNCHANGED, false, m_nBinWidth, m_nBinHeight, m_yuv_format );

			if (m_matOrigin.data == NULL)
			{
				m_mat.release();
				Invalidate();
				::SendMessage(m_hParentWnd, message_video_wnd_video_opened, m_nVideoWndID, 0);
				SendTrackChanged();
				return false;
				//m_matOrigin = Mat(360, 640, CV_8UC3, Scalar(0, 0, 0));
				//drawText(m_matOrigin, "File not found or corrupted image file.", cv::Point(-1, -1), SCV_DT_CENTER | SCV_DT_VCENTER, 0, 2, 0.8, cv::Scalar(0, 64, 255));
			}
		}

		m_szImageSizeOrigin.cx = m_matOrigin.cols;
		m_szImageSizeOrigin.cy = m_matOrigin.rows;
		m_szImageSize.cx = ROUND( (double)( m_szImageSizeOrigin.cx ) * m_dResizeRatio, 0 );
		m_szImageSize.cy = ROUND( (double)( m_szImageSizeOrigin.cy ) * m_dResizeRatio, 0 );

		if (m_bUseImageArray == false)
			m_nTotalFrame = 1;

		m_VideoFPS = 0;
	}
	//raw 영상인 경우
	else if ( m_nFileType == FILE_TYPE_VIDEO_BIN )
	{
		m_szImageSizeOrigin.cx = m_nBinWidth;
		m_szImageSizeOrigin.cy = m_nBinHeight;
		m_szImageSize.cx = m_nBinWidth * m_dResizeRatio;

		m_szImageSize.cy = m_nBinHeight * m_dResizeRatio;

		m_matOrigin.release();
		m_matOrigin.create( m_nBinHeight, m_nBinWidth, CV_8UC3 );
		m_matOrigin.copyTo(	m_matOrigin2 );

		if ( m_FileBin.m_hFile != CFile::hFileNull )
			m_FileBin.Close();

		if ( m_FileBin.Open( sfile, CFile::modeRead, NULL ) )
			m_nTotalFrame = m_FileBin.GetLength();
		else
		{
			AfxMessageBox( _T("Can't open bin file or maybe already opened by another program."), MB_ICONEXCLAMATION );
			return false;
		}

		m_nTotalFrame /= ( m_nBinWidth * m_nBinHeight * 2 * (m_bStereoBin ? 2 : 1) );

		if ( m_bSaveRecentFrame )
		{
			CString sRecentFile = AfxGetApp()->GetProfileString(_T("setting\\video"), _T("recent file"), _T("") );
			if (get_part(sRecentFile, fn_name).MakeLower() == get_part(sfile, fn_name).MakeLower() )
			{
				m_nCurrentFrame = AfxGetApp()->GetProfileInt( _T("setting\\video"), _T("recent frame"), 0 );
				DEFAULT_RANGE( m_nCurrentFrame, 0, m_nTotalFrame - 1, 0 );
			}
		}

		m_sVideoFileName = _T("");
		if ( ReadFrameFromBinFile() )
			m_sVideoFileName = sfile;
	}
	//일반 비디오 파일인 경우
	else
	{
 		if ( m_capture.isOpened() )
 			m_capture.release();

		//mp4 동영상 파일을 열기 위해서는 통합코덱을 설치하거나 opencv의 opencv_ffmpeg2xxx.dll 파일을 실행파일 경로에 복사해 줘야 한다.
		if ( sfile == "cam" )
		{
			//cam일 경우 frame정보만 없을 뿐 일반 비디오 파일과 동일하다.
			//별도의 file type을 정의하기 보다는 비디오 파일과 같은 타입으로 하고
			//필요한 곳에서 예외인 액션만 별도 처리하자.
			m_nFileType = FILE_TYPE_VIDEO;

			if (m_cam_index < 0)
				m_cam_index = 0;

			//윈도우에서 CAP_DSHOW를 주지 않고 기본값인 0이면 다른 방식으로 캠을 열기 위한 시간이 많이 소모된다.
			if ( m_capture.open(m_cam_index, CAP_DSHOW) == false )
			{
				str.Format(_T("Can't open cam%d device."), m_cam_index);
				AfxMessageBox(str, MB_ICONSTOP);
				return false;
			}
		}
		else
		{
			if ( m_capture.open( CString2string(sfile) ) == false )
			{
				str.Format(_T("%s\n올바른 동영상 파일이 아니거나 실행파일과 같은 경로에 opencv_ffmpeg???.dll 파일이 존재하지 않습니다."), sfile);
				AfxMessageBox(str, MB_ICONSTOP);
				return false;
			}
		}

		m_szImageSizeOrigin.cx = m_capture.get(CAP_PROP_FRAME_WIDTH);
		m_szImageSizeOrigin.cy = m_capture.get(CAP_PROP_FRAME_HEIGHT);
		m_VideoFPS = m_capture.get(CAP_PROP_FPS);
		//AfxMessageBox( i2S( fps ) );

		m_szImageSize.cx = ROUND( (double)( m_szImageSizeOrigin.cx ) * m_dResizeRatio, 0 );
		//m_dResizeRatio에 의해 변경된 width를 4의 배수로 맞춰줘야 한다.
		//if ( m_szImageSize.cx % 4 != 0 )
		//{
		//	m_szImageSize.cx = m_szImageSize.cx - ( (m_szImageSize.cx + 4) % 4 );
		//	m_dResizeRatio = (double)m_szImageSize.cx / (double)m_szImageSizeOrigin.cx;
		//}
		m_szImageSize.cy = ROUND( (double)( m_szImageSizeOrigin.cy ) * m_dResizeRatio, 0 );

		m_nTotalFrame = m_capture.get(CAP_PROP_FRAME_COUNT);

		if ( m_bSaveRecentFrame )
		{
			CString sRecentFile = AfxGetApp()->GetProfileString(_T("setting\\video"), _T("recent file"), _T("") );
			if (get_part(sRecentFile, fn_name).MakeLower() == get_part(sfile, fn_name).MakeLower() )
			{
				m_nCurrentFrame = AfxGetApp()->GetProfileInt( _T("setting\\video"), _T("recent frame"), 0 );
				DEFAULT_RANGE( m_nCurrentFrame, 0, m_nTotalFrame - 1, 0 );
			}
		}

		bool set_res = m_capture.set(CAP_PROP_POS_FRAMES, m_nCurrentFrame );
		//m_capture.retrieve( m_matOrigin );	//cam 일 경우 이미지를 못가져옴
		m_capture >> m_matOrigin;

		if ( sfile == "cam" && m_matOrigin.empty() )
		{
			AfxMessageBox( _T("Can't retrieve images from WebCam. Maybe webcam is used by another apps."), MB_ICONEXCLAMATION );
			return false;
		}

		m_sVideoFileName = sfile;
	}

	if (m_matOrigin.empty())
	{
		AfxMessageBox(sfile + _T("\nFail to open."));
		return false;
	}

	SetWindowText( m_sVideoFileName );

	if ( m_sVideoFileName != _T("cam") || m_sVideoFileName != _T("untitled") )
		build_file_list();

	m_rScreenROI.SetRectEmpty();
	m_rImageROI = CRect( 0, 0, m_szImageSize.cx, m_szImageSize.cy );

	cv::resize( m_matOrigin, m_mat, cv::Size( m_szImageSize.cx, m_szImageSize.cy ), 0.0, 0.0, m_nInterpolation );
	if ( IsROIExist() )
		m_mat( cv::Rect(m_rImageROI.left, m_rImageROI.top, m_rImageROI.Width(), m_rImageROI.Height()) ).copyTo( m_mat );

	CString recentFile = AfxGetApp()->GetProfileString(_T("setting\\video"), _T("recent file"), _T(""));
	AfxGetApp()->WriteProfileString(_T("setting\\video"), _T("recent file"), m_sVideoFileName );

	if (GetParent()->m_hWnd && m_bSendPlayWndInfo)
		::SendMessage(GetParent()->m_hWnd, message_video_wnd_video_opened, m_nVideoWndID, 0);

	if (recentFile == m_sVideoFileName)
		SetResizeRatio(GetProfileDouble(AfxGetApp(), _T("setting\\video"), _T("resize ratio"), 1.0));

	GotoFrame(AfxGetApp()->GetProfileInt(_T("setting\\video"), _T("recent frame"), 0) - 60);

	if (GetParent()->m_hWnd && m_bSendPlayWndInfo)
	{
		::SendMessage(GetParent()->m_hWnd, message_video_wnd_play_state_changed, m_nVideoWndID, m_nPlayState );
	}
	else
	{
		//위의 message_video_wnd_video_opened에 의해서 만약 m_Video.SetResizeRatio()를 호출하면
		//updateMainMat이 수행되므로 중복 수행이 된다.
		//video 파일이고 자동 재생되면 관계없으나 pause상태 또는 이미지 파일이면
		//OnMessageTrackChanged 함수가 두 번 호출되게 된다.
		//위에서 수행했다면 여기서는 스킵되어야 한다.
		updateMainMat( &m_mat, false, true );
	}

	if ( m_bUseOwnFPS )
	{
		m_dTime0 = getClock();
	}

	if ( m_nFileType >= FILE_TYPE_VIDEO && m_nFileType <= FILE_TYPE_VIDEO_BIN )
	{
		Play( bPlay ? PLAY : PAUSE );
	}

	return true;
}

void CVideoWnd::CloseVideoFile()
{
	KillTimer( TIMER_PLAY_VIDEO );

	if ( m_hParentWnd && m_bSendPlayWndInfo && IsVideoFileOpened() )
	{
		::SendMessage(m_hParentWnd, message_video_wnd_track_changing, m_nVideoWndID, m_nCurrentFrame);

		WriteProfileDouble( AfxGetApp(), _T("setting\\video"), _T("resize ratio"), GetResizeRatio() );

		::SendMessage(m_hParentWnd, message_video_wnd_video_closing, m_nVideoWndID, 0);
	}

	if ( m_nFileType == FILE_TYPE_VIDEO || m_nFileType == FILE_TYPE_VIDEO_BIN )
	{
		AfxGetApp()->WriteProfileInt( _T("setting\\video"), _T("recent frame"), m_bSaveRecentFrame ? m_nCurrentFrame : 0 );
	}

	if ( m_capture.isOpened() )
 		m_capture.release();

	if ( m_FileBin.m_hFile != CFile::hFileNull )
		m_FileBin.Close();

	if ( imgYUV != NULL )
	{
		delete [] imgYUV;
		imgYUV = NULL;
	}

	if ( m_yuv_origin != NULL )
	{
		delete [] m_yuv_origin;
		m_yuv_origin = NULL;
	}

	m_mat.release();
	m_bImageFromClipboard = false;
	m_nFileType = FILE_TYPE_UNKNOWN;

	//반드시 파일명을 초기화시켜줘야 한다.
	//이 클래스는 파일명을 가지고 동영상 파일이 열렸는지를 판단하기 때문이다.
	//파일명은 동영상 파일이 올바르게 열린 경우에만 저장된다.
	m_sVideoFileName = _T("");

	for ( int i = 0; i < 4; i++ )
		m_sInfoText[i] = _T("");

	Invalidate();
}

void CVideoWnd::Play( int nState )
{
	if ( m_nFileType < FILE_TYPE_VIDEO )
		return;

 	if ( m_sVideoFileName == "" && nState != STOP )
		OpenVideoFile(_T(""));

	if ( nState == PLAY_TOGGLE )
	{
		if ( m_nPlayState == PLAY || m_nPlayState == PAUSE )
			m_nPlayState = !m_nPlayState;
	}
	else
	{
		m_nPlayState = nState;
	}

	//아래 변경된 재생 상태를 반영하기 전에 track changing 메시지는 미리 처리되어야 한다.
	if (m_hParentWnd && m_bSendPlayWndInfo)
		::SendMessage(m_hParentWnd, message_video_wnd_track_changing, m_nVideoWndID, m_nCurrentFrame);

	if ( m_nPlayState == PLAY )
	{
		if ( m_nCurrentFrame == m_nTotalFrame )
			GotoFrame( 0 );

		SetTimer( TIMER_PLAY_VIDEO, 1, NULL );
	}
	else if ( m_nPlayState == PAUSE )
	{
		KillTimer( TIMER_PLAY_VIDEO );
	}
	else if ( m_nPlayState == STOP )
	{
		KillTimer( TIMER_PLAY_VIDEO );

		CloseVideoFile();
	}

		//::SendMessage(m_hParentWnd, MESSAGE_VIDEO_WND_PLAY_STATE_CHANGED, m_nVideoWndID, m_nPlayState);
}


CSize CVideoWnd::GetImageSizeOrigin()
{
 	if ( m_sVideoFileName == "" )
		return 0;

	return m_szImageSizeOrigin;
}

CSize CVideoWnd::GetImageSize()
{
 	if ( m_sVideoFileName == "" )
		return 0;

	return m_szImageSize;
}

double CVideoWnd::GetImageRatio()//cols / rows
{
	if (m_sVideoFileName == "" || m_mat.rows <= 0 )
		return 0.0;

	return (double)m_mat.cols / (double)m_mat.rows;
}

void CVideoWnd::OnTimer(UINT_PTR nIDEvent)
{
	// TODO: Add your message handler code here and/or call default
	if ( nIDEvent == TIMER_PLAY_VIDEO )
	{
		if ( m_nFileType != FILE_TYPE_VIDEO && m_nFileType != FILE_TYPE_VIDEO_BIN )
			return;


		if ( m_bReverse )
		{
			m_nCurrentFrame--;

			if ( m_nCurrentFrame < 0 )
			{
				m_bReverse = false;
				return;

				if ( m_bRepeat )
				{
					GotoFrame( m_nTotalFrame - 1 );
				}
				else
				{
					KillTimer( TIMER_PLAY_VIDEO );
					Play( PAUSE );
			
					if ( m_hParentWnd && m_bSendPlayWndInfo )
						::SendMessage( m_hParentWnd, message_video_wnd_end_of_video, m_nVideoWndID, 0 );
				}
				return;
			}
		}
		else
		{
			m_nCurrentFrame++;

			//재생이 끝까지 도달한 경우.
			//간혹 끝 프레임 즈음이 깨진 동영상도 존재한다.
			//예외가 발생하므로 이에 대한 예외처리도 필요하다.
			//만약 그런 경우 우선 임시적으로 (m_nTotalFrame - 25)프레임이면 스킵되도록 처리한다.
			if ( m_nCurrentFrame >= m_nTotalFrame )
			{
				if ( m_bRepeat )
				{
					GotoFrame( 0 );
				}
				else
				{
					if ( m_auto_next_video )
					{
						Play( PAUSE );
						open_next_video();
						return;
					}

					KillTimer( TIMER_PLAY_VIDEO );
					Play( PAUSE );
			
					if ( m_hParentWnd && m_bSendPlayWndInfo )
						::SendMessage( m_hParentWnd, message_video_wnd_end_of_video, m_nVideoWndID, 0 );
				}
				return;
			}
		}

		//TRACE( "m_nCurrentFrame = %d\n", m_nCurrentFrame );

		if ( m_nFileType == FILE_TYPE_VIDEO_BIN )
		{
			ReadFrameFromBinFile();	//m_matOrigin 데이터가 채워진다.
		}
		else
		{
			//m_nCurrentFrame을 늘리고 m_capture.set을 한 후
			//현재 프레임을 retrieve해서 가져오는 방식으로 구현했었으나
			//2배 가까이 느린 현상이 발생한다. 아마도 set에서 속도 저하가 발생하는 듯하다.
			//따라서 GotoFrame과 같은 함수에서는 어쩔수 없이 set을 사용하여 프레임을 이동시켜야 하지만
			//재생 타이머에서는 >>를 이용해서 프레임을 가져온다.

			//set 방식은 9fps, >>방식은 12fps.
			//m_capture.set( CV_CAP_PROP_POS_FRAMES, m_nCurrentFrame );
			//m_capture.retrieve( m_matOrigin );

			if ( m_bReverse )
			{
				m_capture.set(CAP_PROP_POS_FRAMES, m_nCurrentFrame );
				m_capture.retrieve( m_matOrigin );
			}
			else
			{
				//30frame 당 영상 추출용이므로 그 외에는 아래 한줄은 반드시 제거해야 한다.
				//m_capture.set( CV_CAP_PROP_POS_FRAMES, m_nCurrentFrame );

				m_capture >> m_matOrigin;

				if ( m_matOrigin.empty() )
				{
					KillTimer( TIMER_PLAY_VIDEO );
					Play( PAUSE );
					//GotoFrame( 0 );
					return;
				}
			}
		}

		if ( m_bImageFromClipboard )
		{
			m_matOrigin.copyTo(m_mat);
			m_szImageSize = m_szImageSizeOrigin = CSize(m_mat.cols, m_mat.rows);
			m_rImageROI = CRect(0, 0, m_mat.cols, m_mat.rows);

			m_bImageFromClipboard = false;
		}

		if ( m_dResizeRatio == 1.0 )
			m_matOrigin.copyTo( m_mat );
		else
			cv::resize( m_matOrigin, m_mat, cv::Size( m_szImageSize.cx, m_szImageSize.cy ), 0.0, 0.0, m_nInterpolation );

		//roi가 설정된 상태라면 roi의 크기로 잘라준다.
		//단, roi 관련 명령이 모두 false인 경우에만.
		if ( m_bROI_Move && m_bLButtonDown4ROIMove )
			;
		else
			m_mat( cv::Rect(m_rImageROI.left, m_rImageROI.top, m_rImageROI.Width(), m_rImageROI.Height()) ).copyTo( m_mat );

		if ( m_hParentWnd && m_bSendPlayWndInfo )
			SendTrackChanged();

		if ( m_bSaveRecentFrame )
			AfxGetApp()->WriteProfileInt( _T("setting\\video"), _T("recent frame"), GetCurrentFrame() );

		CString str;

		//str.Format( "%d / %d", (int)m_nCurrentFrame, m_nTotalFrame );//m_nCurrentFrame );
		//if ( m_ToolTip.IsWindowVisible() )
		//	m_ToolTip.UpdateTipText( str, this );

		if ( m_bUseOwnFPS )
		{
			m_dTime1 = getClock();
			calcFPS( m_dTime0, m_dTime1 );
			//m_dCurrentProcessingTime = m_dTime1 - m_dTime0;
			//m_dCurrentFPS = 1000.0 / (double)(m_dTime1 - m_dTime0);
			//str.Format(_T("%.1ffps (%5ldms)"), m_dCurrentFPS, m_dCurrentProcessingTime );
			//SetInfoText( 1, str );
			m_dTime0 = m_dTime1;
		}

		updateMainMat( &m_mat, false, false );
	}

	CWnd::OnTimer(nIDEvent);
}

void CVideoWnd::GotoFrame(int nFrame)
{
	if ( m_nTotalFrame == 0 )
		return;

	if ( m_nFileType == FILE_TYPE_IMAGE && !m_bUseImageArray )
		return;

	if ((m_nFileType == FILE_TYPE_VIDEO || m_nFileType == FILE_TYPE_VIDEO_BIN) && (m_sVideoFileName == "" || m_sVideoFileName == "cam"))
		return;


	if ( nFrame < 0 )
		nFrame = 0;
	else if ( nFrame >= m_nTotalFrame )
		nFrame = m_nTotalFrame - 1;

	//트랙이 변경되기 전에 알려준다.
	::SendMessage(m_hParentWnd, message_video_wnd_track_changing, m_nVideoWndID, m_nCurrentFrame);

	m_nCurrentFrame = nFrame;


	if (m_nFileType == FILE_TYPE_VIDEO || m_nFileType == FILE_TYPE_VIDEO_BIN)
	{
		//play인 경우 GotoFrame은 영상 갱신이 timer에 의해 자동으로 진행되므로 굳이 목적 프레임의 영상을
		//수동으로 갱신해 줄 필요는 없다.
		//play상태가 아닌 경우에만 해당 프레임의 영상으로 갱신해주자.
		if (m_nFileType == FILE_TYPE_VIDEO)
		{
			bool b = m_capture.set(CAP_PROP_POS_FRAMES, (double)m_nCurrentFrame);
			m_nCurrentFrame = m_capture.get(CAP_PROP_POS_FRAMES);
		}

		if ( m_nPlayState != PLAY || m_nCurrentFrame == m_nTotalFrame - 1 )
		{
			if (m_nFileType == FILE_TYPE_VIDEO_BIN)
				ReadFrameFromBinFile();
			else if (m_nFileType == FILE_TYPE_VIDEO)
				m_capture >> m_matOrigin;

			if ( m_matOrigin.empty() )
				return;

			if ( m_bImageFromClipboard )
			{
				m_matOrigin.copyTo(m_mat);
				m_szImageSize = m_szImageSizeOrigin = CSize(m_mat.cols, m_mat.rows);
				m_rImageROI = CRect(0, 0, m_mat.cols, m_mat.rows);

				m_bImageFromClipboard = false;
			}

			cv::resize(m_matOrigin, m_mat, cv::Size(m_szImageSize.cx, m_szImageSize.cy), 0.0, 0.0, m_nInterpolation);
			m_mat(cv::Rect(m_rImageROI.left, m_rImageROI.top, m_rImageROI.Width(), m_rImageROI.Height())).copyTo(m_mat);
			updateMainMat(&m_mat, false, false);

			if (m_hParentWnd && m_bSendPlayWndInfo)
				SendTrackChanged();
		}
	}
	else if (m_bUseImageArray && m_nFileType == FILE_TYPE_IMAGE)
	{
		if (m_dqImageFiles.size() > 0)
			OpenVideoFile(m_dqImageFiles[m_nCurrentFrame]);
	}
}

void CVideoWnd::GotoNextFrame(bool bNext, int nStep, int newState)
{
	if ( m_nTotalFrame == 0 )
		return;

	if ((m_nFileType == FILE_TYPE_VIDEO || m_nFileType == FILE_TYPE_VIDEO_BIN) && (m_sVideoFileName == "" || m_sVideoFileName == "cam"))
		return;

	if (/*(m_nFileType == FILE_TYPE_VIDEO || m_nFileType == FILE_TYPE_VIDEO_BIN) && */m_sVideoFileName == "")
		return;

	if (newState != NO_CHANGE)
		Play(newState);

	::SendMessage(m_hParentWnd, message_video_wnd_track_changing, m_nVideoWndID, m_nCurrentFrame);

	//TRACE(_T("before frame = %d\n"), m_nCurrentFrame);
	if (bNext)
	{
		if ( (m_nCurrentFrame + nStep) >= m_nTotalFrame )
			m_nCurrentFrame = m_nTotalFrame - 1;
		else
			m_nCurrentFrame += nStep;
	}
	else
	{
		m_nCurrentFrame = m_nCurrentFrame - nStep;
		if (m_nCurrentFrame < 0)
			m_nCurrentFrame = 0;
	}
	//TRACE(_T("after  frame = %d\n"), m_nCurrentFrame);

	if (m_nFileType == FILE_TYPE_VIDEO || m_nFileType == FILE_TYPE_VIDEO_BIN)
	{
		if (m_nFileType == FILE_TYPE_VIDEO)
		{
			m_capture.set(CAP_PROP_POS_FRAMES, m_nCurrentFrame);
			m_capture >> m_matOrigin;
		}
		else if (m_nFileType == FILE_TYPE_VIDEO_BIN)
		{
			ReadFrameFromBinFile();
		}

		if ( m_bImageFromClipboard )
		{
			m_matOrigin.copyTo(m_mat);
			m_szImageSize = m_szImageSizeOrigin = CSize(m_mat.cols, m_mat.rows);
			m_rImageROI = CRect(0, 0, m_mat.cols, m_mat.rows);

			m_bImageFromClipboard = false;
		}

		cv::resize(m_matOrigin, m_mat, cv::Size(m_szImageSize.cx, m_szImageSize.cy), 0.0, 0.0, m_nInterpolation);
		m_mat(cv::Rect(m_rImageROI.left, m_rImageROI.top, m_rImageROI.Width(), m_rImageROI.Height())).copyTo(m_mat);
		updateMainMat(&m_mat, false, true);
	}
	else if (m_nFileType == FILE_TYPE_IMAGE && m_bUseImageArray)
	{
		OpenVideoFile(m_dqImageFiles[m_nCurrentFrame]);
	}

	//if (m_hParentWnd && m_bSendPlayWndInfo)
		//SendTrackChanged();

	return;
}

void CVideoWnd::GotoNextFrameAr(CStringArray *ar, int *nCurrent, bool bNext, int nStep, int newState)
{
	if (ar == NULL || ar->GetCount() == 0)
	{
		GotoNextFrame(bNext, nStep, newState);
		return;
	}

	if (bNext)
	{
		if (*nCurrent >= ar->GetCount() - nStep)
			*nCurrent = ar->GetCount() - 1;
		else
			*nCurrent += nStep;
	}
	else
	{
		*nCurrent -= nStep;
		if (*nCurrent < 0)
			*nCurrent = 0;
	}

	OpenVideoFile(ar->GetAt(*nCurrent));
}

//resize ratio와 roi만 적용된 소스 영상을 리턴.
Mat CVideoWnd::GetNonFilteredMat()
{
	Mat src;

	if ( m_dResizeRatio != 1.0 )
		resize( m_matOrigin, m_mat, cv::Size( m_matOrigin.cols, m_matOrigin.rows ), 0.0, 0.0, m_nInterpolation );

	if ( IsROIExist() )
		m_matOrigin( cv::Rect(m_rImageROI.left, m_rImageROI.top, m_rImageROI.Width(), m_rImageROI.Height()) ).copyTo( m_mat );

	return m_mat;
}

void CVideoWnd::updateMainMat( Mat* mat, bool bReload, bool bSendToParent )
{
	if ( m_sVideoFileName == "" )
		return;

	if ( mat != NULL )
	{
		if ( (m_mat.data != NULL) && (mat->size() != m_mat.size()) )
		{
			m_szImageSizeOrigin.cx = m_szImageSize.cx = mat->cols;
			m_szImageSizeOrigin.cy = m_szImageSize.cy = mat->rows;
		}
		mat->copyTo( m_mat );
	}
	else if ( mat == NULL || bReload )
	{
		if ( m_matOrigin.empty() == false )
		{
			m_szImageSizeOrigin.cx = m_matOrigin.cols;
			m_szImageSizeOrigin.cy = m_matOrigin.rows;

			if (m_dResizeRatio == 1.0)
			{
				m_szImageSize = m_szImageSizeOrigin;
				m_matOrigin.copyTo(m_mat);
			}
			else
			{
				m_szImageSize.cx = ROUND((double)(m_szImageSizeOrigin.cx) * m_dResizeRatio, 0);
				m_szImageSize.cy = ROUND((double)(m_szImageSizeOrigin.cy) * m_dResizeRatio, 0);
				resize(m_matOrigin, m_mat, cv::Size(m_szImageSize.cx, m_szImageSize.cy), 0.0, 0.0, m_nInterpolation);
			}

			if ( IsROIExist() )
				m_mat( cv::Rect(m_rImageROI.left, m_rImageROI.top, m_rImageROI.Width(), m_rImageROI.Height()) ).copyTo( m_mat );
		}

		//if ( IsROIExist() )
	}

	//동영상 파일을 재생하는 VideoWnd가 아닌
	//단순히 m_mat을 전달받아 재생만 해주는 VideoWnd는
	//이미지 크기 정보가 세팅되어 있지 않다.
	//간혹 그 크기값을 참조하는 코드가 존재하므로
	//m_mat을 받았다면 그 크기를 세팅해주자.
	if ( (m_sVideoFileName == "" || m_sVideoFileName == "Untitled") && !m_bImageFromClipboard)
	{
		m_szImageSizeOrigin = m_szImageSize = CSize( m_mat.cols, m_mat.rows );
		m_rImageROI = CRect( 0, 0, m_szImageSize.cx, m_szImageSize.cy );
	}

	if ( bSendToParent )
		SendTrackChanged();

	Invalidate(FALSE);
}


int CVideoWnd::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (CWnd::OnCreate(lpCreateStruct) == -1)
		return -1;

	// TODO:  Add your specialized creation code here
	if ( m_bUseCrossCursor )
		m_hCursor = (HCURSOR)AfxGetApp()->LoadStandardCursor( IDC_CROSS );

	return 0;
}


BOOL CVideoWnd::OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message)
{
	// TODO: Add your message handler code here and/or call default
	HCURSOR	hCursor = (HCURSOR)AfxGetApp()->LoadStandardCursor( IDC_ARROW );;

	if ( m_bPopupMenu )
	{
		SetCursor(hCursor);
		return TRUE;
	}

	int		nThickness = 10;
	CRect	r[4];
	CPoint	pt;
	GetCursorPos( &pt );
	ScreenToClient( &pt );

	if ( m_bROI_Set )
	{
		m_hCursor = (HCURSOR)AfxGetApp()->LoadStandardCursor( IDC_CROSS );
		SetCursor( m_hCursor );
		return false;
	}
	else if ( IsROIExist() && m_bROI_Move )
	{
		//m_bROI_Move가 안눌려진 상태에서는 커서의 위치에 따라 커서의 모양과 direction이 정해진다.
		//그 후 roi resize 또는 move를 위해 눌려진 다음에는 m_rDisplayedImageRect 크기가 변하므로
		//direction을 다시 판별하거나 커서 모양을 변경해선 안된다.
		if ( !m_bLButtonDown4ROIMove )
		{
			r[0] = m_rDisplayedImageRect;
			r[0].right = r[0].left + nThickness;

			r[1] = m_rDisplayedImageRect;
			r[1].bottom = r[1].top + nThickness;

			r[2] = m_rDisplayedImageRect;
			r[2].left = r[2].right - nThickness;

			r[3] = m_rDisplayedImageRect;
			r[3].top = r[3].bottom - nThickness;

			if ( r[0].PtInRect( pt ) )		//left
			{
				m_nROIResizeDirection = 0;
				m_hCursor = (HCURSOR)AfxGetApp()->LoadStandardCursor( IDC_SIZEWE );
			}
			else if ( r[1].PtInRect( pt ) )	//top
			{
				m_nROIResizeDirection = 1;
				m_hCursor = (HCURSOR)AfxGetApp()->LoadStandardCursor( IDC_SIZENS );
			}
			else if ( r[2].PtInRect( pt ) )	//right
			{
				if ( !m_bLButtonDown4ROIMove ) m_nROIResizeDirection = 2;
				m_hCursor = (HCURSOR)AfxGetApp()->LoadStandardCursor( IDC_SIZEWE );
			}
			else if ( r[3].PtInRect( pt ) )	//bottom
			{
				if ( !m_bLButtonDown4ROIMove ) m_nROIResizeDirection = 3;
				m_hCursor = (HCURSOR)AfxGetApp()->LoadStandardCursor( IDC_SIZENS );
			}
			else if ( m_rDisplayedImageRect.PtInRect(pt) )	//all
			{
				if ( !m_bLButtonDown4ROIMove ) m_nROIResizeDirection = 4;
				m_hCursor = (HCURSOR)AfxGetApp()->LoadStandardCursor( IDC_SIZEALL );
			}
			else
			{
				if ( !m_bLButtonDown4ROIMove ) m_nROIResizeDirection = -1;
				m_hCursor = (HCURSOR)AfxGetApp()->LoadStandardCursor( IDC_ARROW );
			}
			TRACE(_T("OnSetCursor::m_nROIResizeDirection = %d\n"), m_nROIResizeDirection );
		}
		else
		{
			if ( m_nROIResizeDirection == 4 )
				m_hCursor = (HCURSOR)AfxGetApp()->LoadStandardCursor( IDC_SIZEALL );
			else if ( m_nROIResizeDirection % 2 == 0 )
				m_hCursor = (HCURSOR)AfxGetApp()->LoadStandardCursor( IDC_SIZEWE );
			else if ( m_nROIResizeDirection % 2 == 1 )
				m_hCursor = (HCURSOR)AfxGetApp()->LoadStandardCursor( IDC_SIZENS );
		}

		::SetCursor( m_hCursor );

		return true;
	}
	//치수 측정 메뉴가 눌려진 경우
	else if (m_bGetVanishingPoint || m_bMeasureSize )
	{
		m_hCursor = (HCURSOR)AfxGetApp()->LoadStandardCursor( IDC_CROSS );
		::SetCursor( m_hCursor );
		return true;
	}

	if ( m_bUseCrossCursor )
	{
		if ( m_hCursor == NULL )
			m_hCursor = (HCURSOR)AfxGetApp()->LoadStandardCursor( IDC_CROSS );

		::SetCursor( m_hCursor );

		return true;
	}

	//SetCursor( m_hCursor );
	//return true;
	return CWnd::OnSetCursor(pWnd, nHitTest, message);
}

void CVideoWnd::OnExitMenuLoop( BOOL bIsTrackPopupMenu )
{
	CWnd::OnExitMenuLoop(bIsTrackPopupMenu);
	m_bPopupMenu = false;
}

//전체 화면 영상에서 roi를 설정한 후 전체 화면 영상이 resize될 때 roi영역 또한 자동 resize되어야 하므로
//아래 함수에서 screenROI를 새로이 계산하도록 했으나
//현재는 roi를 설정하면 그 영역이 m_mat 자체가되어 표시되므로 roi를 더 이상 표시하지 않아도 된다.
void CVideoWnd::RecalcScreenROI()
{
 	if ( m_sVideoFileName == "" )
		return;

	CRect	rc;
	GetClientRect( rc );

	//ROI가 별도로 정해지지 않은 전체화면인 경우라면 패스.
	if ( m_rScreenROI == rc )
		return;

	if ( m_rImageROI == CRect( 0, 0, m_szImageSize.cx, m_szImageSize.cy ) )
		return;

	double dZoom = (double)m_szImageSize.cx / (double)rc.Width();

	m_rScreenROI.left		= m_rImageROI.left / dZoom;
	m_rScreenROI.top		= m_rImageROI.top / dZoom;
	m_rScreenROI.right		= m_rImageROI.right / dZoom;
	m_rScreenROI.bottom		= m_rImageROI.bottom / dZoom;

	Invalidate();
}

void CVideoWnd::SetImageROI( CRect roi )
{ 
 	if ( m_sVideoFileName == "" )
		return;

	if ( CheckROIValidation( roi ) == false )
		return;

	m_rImageROI = roi;

	//pause일때 ROI가 변경되면 영상을 직접 갱신시켜줘야 한다.
	if ( m_nPlayState == PAUSE || m_nFileType == FILE_TYPE_IMAGE )
	{
		if ( m_matOrigin.data == NULL )
		{
			if ( m_nFileType == FILE_TYPE_VIDEO_BIN )
				ReadFrameFromBinFile();
			else
				m_capture.retrieve( m_matOrigin );
		}
		
		resize( m_matOrigin, m_mat, cv::Size( m_szImageSize.cx, m_szImageSize.cy ), 0.0, 0.0, m_nInterpolation );
		m_mat( cv::Rect(m_rImageROI.left, m_rImageROI.top, m_rImageROI.Width(), m_rImageROI.Height()) ).copyTo( m_mat );
		updateMainMat( &m_mat );
	}
}

void CVideoWnd::OnRButtonDown(UINT nFlags, CPoint point)
{
	// TODO: Add your message handler code here and/or call default

	CWnd::OnRButtonDown(nFlags, point);
}


void CVideoWnd::OnRButtonUp(UINT nFlags, CPoint point)
{
	// TODO: Add your message handler code here and/or call default
	if ( !m_bUsePopupMenu )
		return;

	CMenu	menu, menuYUV;

	menu.CreatePopupMenu();

	menuYUV.CreatePopupMenu();
	menuYUV.AppendMenu( MF_STRING, id_menu_yuv_stereo, _T("Stereo") );
	menuYUV.AppendMenu( MF_SEPARATOR );
	menuYUV.AppendMenu( MF_STRING, id_menu_yuv_size, _T("Video Size...") );
	menuYUV.AppendMenu( MF_SEPARATOR );
	menuYUV.AppendMenu( MF_STRING, id_menu_yuv422_uyvy, _T("YUV422_UYVY") );
	menuYUV.AppendMenu( MF_STRING, id_menu_yuv422_yuyv, _T("YUV422_YUYV") );
	menuYUV.AppendMenu( MF_STRING, id_menu_yuv420_nv12, _T("YUV420_NV12") );
	menuYUV.AppendMenu( MF_STRING, id_menu_yuv420_yv12, _T("YUV420_YV12") );

	menuYUV.EnableMenuItem( id_menu_yuv_stereo, (m_nFileType == FILE_TYPE_VIDEO_BIN) ? MF_ENABLED : MF_DISABLED );
	menuYUV.EnableMenuItem( id_menu_yuv422_yuyv, MF_DISABLED );	//not implemented
	//stereo bin은 uyvy만 지원된다.
	menuYUV.EnableMenuItem( id_menu_yuv420_nv12, m_bStereoBin ? MF_DISABLED : MF_ENABLED );
	menuYUV.EnableMenuItem( id_menu_yuv420_yv12, m_bStereoBin ? MF_DISABLED : MF_ENABLED );

	menuYUV.CheckMenuItem( id_menu_yuv_stereo, m_bStereoBin ? MF_CHECKED : MF_UNCHECKED );
	menuYUV.CheckMenuRadioItem( id_menu_yuv422_uyvy, id_menu_yuv420_yv12, id_menu_yuv422_uyvy + m_yuv_format, MF_BYCOMMAND );

	CPoint	pt = point;
	CString str;

	HBITMAP	bitmap = NULL;
	if ( OpenClipboard() == false )
		return;

	bitmap = (HBITMAP)GetClipboardData(CF_BITMAP);
	CloseClipboard();

	if ( m_mat.data )
		str.Format(_T("%d x %d x %db"), m_mat.cols, m_mat.rows, m_mat.channels() * 8 );
	else
		str = _T("No image infomation");
	menu.AppendMenu( MF_STRING, id_menu_image_info, str);
	menu.AppendMenu( MF_SEPARATOR );

	menu.AppendMenu(MF_STRING, id_menu_file_open, _T("Open(&O)..."));
	menu.AppendMenu(MF_STRING, id_menu_file_open_sequence, _T("Open sequence images..."));
	menu.AppendMenu(MF_SEPARATOR);
	menu.AppendMenu(MF_STRING, id_menu_do_not_image_processing, _T("Do not image processing"));
	menu.CheckMenuItem(id_menu_do_not_image_processing, m_do_not_image_processing ? MF_CHECKED : MF_UNCHECKED);
	menu.AppendMenu(MF_SEPARATOR);

	menu.AppendMenu( MF_POPUP, (UINT_PTR)menuYUV.m_hMenu, _T("YUV format") );
	menu.AppendMenu( MF_SEPARATOR );

	m_dResizeRatio = GetProfileDouble(AfxGetApp(), _T("setting\\video"), _T("resize ratio"), 1.0);
	str.Format(_T("Resize ratio : %f"), m_dResizeRatio);
	menu.AppendMenu(MF_STRING, id_menu_resize_ratio, str);
	//menu.EnableMenuItem(id_menu_resize_ratio, IsVideoFileOpened() ? MF_ENABLED : MF_DISABLED);
	menu.AppendMenu(MF_SEPARATOR);

	menu.AppendMenu( MF_STRING, id_menu_roi_set, _T("Set ROI(&R)") );
	menu.AppendMenu( MF_STRING, id_menu_roi_reset, _T("Reset ROI(&E)") );
	menu.AppendMenu( MF_STRING, id_menu_roi_move, _T("Move && Resize ROI(&M)") );
	menu.AppendMenu( MF_SEPARATOR );

	menu.EnableMenuItem( id_menu_roi_set, m_mat.data && !IsROIExist() && !m_bMeasureSize ? MF_ENABLED : MF_DISABLED );
	menu.EnableMenuItem( id_menu_roi_reset, m_mat.data && IsROIExist() && !m_bMeasureSize ? MF_ENABLED : MF_DISABLED );
	menu.EnableMenuItem( id_menu_roi_move, m_mat.data && IsROIExist() && !m_bMeasureSize ? MF_ENABLED : MF_DISABLED );

	menu.CheckMenuItem( id_menu_roi_set, m_bROI_Set ? MF_CHECKED : MF_UNCHECKED );
	menu.CheckMenuItem( id_menu_roi_move, m_bROI_Move ? MF_CHECKED : MF_UNCHECKED );

	menu.AppendMenu(MF_STRING, id_menu_goto_frame, _T("Go to...(&G)"));
	menu.EnableMenuItem(id_menu_goto_frame, (GetTotalFrame() > 0) ? MF_ENABLED : MF_DISABLED);
	menu.AppendMenu(MF_SEPARATOR);

	menu.AppendMenu( MF_STRING, id_menu_repeat, _T("Repeat") );
	menu.CheckMenuItem( id_menu_repeat, m_bRepeat ? MF_CHECKED : MF_UNCHECKED );
	menu.EnableMenuItem(id_menu_repeat, IsVideoFileOpened() ? MF_ENABLED : MF_DISABLED);

	menu.AppendMenu( MF_STRING, id_menu_auto_next_video, _T("Auto next video play") );
	menu.CheckMenuItem( id_menu_auto_next_video, m_auto_next_video ? MF_CHECKED : MF_UNCHECKED );
	menu.EnableMenuItem(id_menu_auto_next_video, IsVideoFileOpened() ? MF_ENABLED : MF_DISABLED);

	menu.AppendMenu( MF_STRING, id_menu_save_recent_frame, _T("Save recent frame position") );
	menu.CheckMenuItem( id_menu_save_recent_frame, m_bSaveRecentFrame ? MF_CHECKED : MF_UNCHECKED );
	menu.EnableMenuItem(id_menu_save_recent_frame, IsVideoFileOpened() ? MF_ENABLED : MF_DISABLED);
	menu.AppendMenu( MF_SEPARATOR );

	str.Format( _T("Current vanishing point (%d, %d)"), m_ptVanishingPoint.x, m_ptVanishingPoint.y );
	menu.AppendMenu( MF_STRING, id_menu_current_vanishing_point, str );
	menu.AppendMenu( MF_STRING, id_menu_show_vanishing_point, _T("Show vanishing point") );
	menu.AppendMenu( MF_STRING, id_menu_get_vanishing_point, _T("Get vanishing point") );
	menu.AppendMenu( MF_STRING, id_menu_set_vanishing_point, _T("Set vanishing point...") );
	menu.EnableMenuItem( id_menu_current_vanishing_point, MF_DISABLED );
	menu.CheckMenuItem( id_menu_show_vanishing_point, m_bShowVanishingPoint ? MF_CHECKED : MF_UNCHECKED );
	menu.EnableMenuItem( id_menu_show_vanishing_point, m_bShowVanishingPoint && m_mat.data && !m_bROI_Set && !m_bROI_Move && !m_bMeasureSize ? MF_ENABLED : MF_DISABLED );
	menu.EnableMenuItem( id_menu_show_vanishing_point, (m_ptVanishingPoint != CPoint(0,0) ? MF_ENABLED : MF_DISABLED) );
	menu.EnableMenuItem( id_menu_set_vanishing_point, m_mat.data && !m_bROI_Set && !m_bROI_Move && !m_bMeasureSize ? MF_ENABLED : MF_DISABLED );
	menu.EnableMenuItem( id_menu_get_vanishing_point, m_mat.data && !m_bROI_Set && !m_bROI_Move && !m_bMeasureSize ? MF_ENABLED : MF_DISABLED );
	menu.AppendMenu( MF_SEPARATOR );



	menu.AppendMenu( MF_STRING, id_menu_measure_size, _T("Measure the size or distance(&M)") );
	menu.AppendMenu( MF_SEPARATOR );

	menu.EnableMenuItem( id_menu_measure_size, m_mat.data && !m_bROI_Set && !m_bROI_Move && !m_bGetVanishingPoint ? MF_ENABLED : MF_DISABLED );
	menu.CheckMenuItem( id_menu_get_vanishing_point, m_bGetVanishingPoint ? MF_CHECKED : MF_UNCHECKED );
	menu.CheckMenuItem( id_menu_measure_size, m_bMeasureSize ? MF_CHECKED : MF_UNCHECKED );



	menu.AppendMenu( MF_STRING, id_menu_copy_to_clipboard, _T("&Copy(&C)") );
	menu.AppendMenu( MF_STRING, id_menu_paste_from_clipboard, _T("Paste(&V)") );
	menu.AppendMenu( MF_SEPARATOR );
	menu.AppendMenu( MF_STRING, id_menu_save_as_original_size, _T("Save image as original size(&S)...") );
	menu.AppendMenu( MF_STRING, id_menu_save_as_displayed_size, _T("Save image as displayed size(&D)...") );

	menu.EnableMenuItem( id_menu_image_info, MF_DISABLED );
	menu.EnableMenuItem( id_menu_save_as_displayed_size, m_mat.data ? MF_ENABLED : MF_DISABLED );
	menu.EnableMenuItem( id_menu_save_as_original_size, m_mat.data ? MF_ENABLED : MF_DISABLED );
	menu.EnableMenuItem( id_menu_copy_to_clipboard, m_mat.data ? MF_ENABLED : MF_DISABLED );
	menu.EnableMenuItem( id_menu_paste_from_clipboard, bitmap ? MF_ENABLED : MF_DISABLED );

	ClientToScreen( &pt );
	m_bPopupMenu = true;
	menu.TrackPopupMenu( TPM_LEFTALIGN, pt.x, pt.y, this );

	menuYUV.DestroyMenu();
	menu.DestroyMenu();


	//CWnd::OnRButtonUp(nFlags, point);
}

BOOL CVideoWnd::PreTranslateMessage(MSG* pMsg)
{
	// TODO: Add your specialized code here and/or call the base class
	if (pMsg->message == WM_KEYDOWN)
	{
		switch (pMsg->wParam)
		{
		case VK_SPACE:
			m_bROI_Set = m_bROI_Move = m_bMeasureSize = m_bGetVanishingPoint = false;
			m_nVanishingPointCount = 0;

			if (IsVideoFileOpened())
				Play(PLAY_TOGGLE);
			else
				OpenVideoFile(AfxGetApp()->GetProfileString(_T("setting\\video"), _T("recent file"), _T("")));
			break;
		case VK_BACK:
			GotoFrame(0);
			return true;
		case VK_LEFT:
		case VK_RIGHT:
			if (IsShiftPressed())
				GotoNextFrame(pMsg->wParam == VK_RIGHT, 33);
			else if (IsCtrlPressed())
				GotoNextFrame(pMsg->wParam == VK_RIGHT, 330);
			else
				GotoNextFrame(pMsg->wParam == VK_RIGHT, 33 * 5);
			break;
		case 'C':
			if (IsCtrlPressed())
			{
				if (IsShiftPressed())
					CopyVideonameAndFrameToClipboard();
				else
					CopyToClipboard();
				return true;
			}
			break;
		case 'V':
			if (IsCtrlPressed())
			{
				PasteFromClipboard();
				return true;
			}
			break;
		case 'F':
			GotoNextFrame(true, 1, PAUSE);
			return true;
		case 'D':
			GotoNextFrame(false, 1, PAUSE);
			return true;
		case VK_ESCAPE:
			if (m_bGetVanishingPoint)
			{
				if (m_nVanishingPointCount > 0)
					m_nVanishingPointCount = 0;
				else
					m_bGetVanishingPoint = false;
			}
			else
			{
				m_bROI_Set = m_bROI_Move = m_bMeasureSize = false;
			}

			Invalidate();
			break;
		//default:
			//AfxGetMainWnd()->SendMessage(WM_KEYDOWN, pMsg->wParam, pMsg->lParam);
			//return true;
		}
	}
	else if (pMsg->message == WM_LBUTTONDOWN)
	{
		//TRACE(_T("%s\n"), _T("CVideoWnd::WM_LBUTTONDOWN") );
		if ( m_bROI_Set || m_bROI_Move || m_bMeasureSize || m_bGetVanishingPoint)
		{
			SendMessage( pMsg->message, pMsg->wParam, pMsg->lParam );
			return true;
		}
		return false;
	}
	else if (pMsg->message == WM_LBUTTONUP)
	{
		//SendMessage( pMsg->message, pMsg->wParam, pMsg->lParam );
		return false;
	}
	/*
	else if (pMsg->message == WM_RBUTTONUP)
	{
		if ( m_bUsePopupMenu )
		{
			SendMessage( pMsg->message, pMsg->wParam, pMsg->lParam );
			return true;
		}
	}
	*/

	return CWnd::PreTranslateMessage(pMsg);
}

bool CVideoWnd::ReadFrameFromBinFile()
{
	if ( m_FileBin.m_hFile == CFile::hFileNull )
		return false;

	int stereo = (m_bStereoBin ? 2 : 1);
	int Y, U, V;
	unsigned char R,G,B;
	unsigned __int64 SeekIndex = 0;

	int k = 0, j = 0;
	int widthStep, channel;

	if ( imgYUV == NULL )
		imgYUV = new uint8_t[m_nBinWidth * m_nBinHeight * 2 * stereo]; 

	widthStep = m_matOrigin.step;
	channel = m_matOrigin.channels();

	SeekIndex = (unsigned __int64)m_nCurrentFrame * (m_nBinWidth * m_nBinHeight * 2 * stereo);

	//for Mando rear cam
	if ( m_bMandoRearCam )
	{
		m_nHeaderSize = 1000;
		m_nTailSize = 11288;
		SeekIndex += (m_nHeaderSize * (m_nCurrentFrame + 1) + m_nTailSize * m_nCurrentFrame );
	}

	if ( m_FileBin.Seek( SeekIndex, CFile::begin ) >= 0 )
	{
		m_FileBin.Read( imgYUV, m_nBinWidth * m_nBinHeight * 2 * stereo );
#if 0
		char sfile[128];
		sprintf( sfile, "f:\\temp\\side_%06d.raw", m_nCurrentFrame );
		FILE* fp = fopen( sfile, "wb" );
		fwrite( imgYUV, m_nBinWidth * m_nBinHeight * 2 * stereo, 1, fp );
		fclose( fp );
#endif
	}

	//m_yuv_origin은 yuv 영상의 원본 데이터를 저장하기 위해 선언하였으나
	//아직은 완료된 기능이 아님.
	if ( m_yuv_origin == NULL )
		m_yuv_origin = new uint8_t[m_nBinWidth * m_nBinHeight * 2];

	if ( !m_bStereoBin )
	{	
		if ( m_yuv_format == yuv422_yuyv )
			;//yuv422_yuyv_to_bgr( imgYUV, m_matOrigin.data, m_nBinWidth, m_nBinHeight );
		else if ( m_yuv_format == yuv420_nv12 )
			yuv420_nv12_to_bgr( imgYUV, m_matOrigin.data, m_nBinWidth, m_nBinHeight );
		else if ( m_yuv_format == yuv420_yv12 )
			yuv420_yv12_to_bgr( imgYUV, m_matOrigin.data, m_nBinWidth, m_nBinHeight );
		else//if ( m_yuv_format == yuv422_uyvy )
			yuv422_uyvy_to_bgr( imgYUV, m_matOrigin.data, m_nBinWidth, m_nBinHeight );
	}
	else
	{
		for(k = 0; k < m_nBinHeight ; k++)
		{
			for(j = 0; j < m_nBinWidth ; j += 2)
			{
				if ( m_bMandoRearCam )
				{
					Y = imgYUV[k*m_nBinWidth*2 + j * 2 + 0];
					U = imgYUV[k*m_nBinWidth*2 + (j + 1) * 2 + 1];
					V = imgYUV[k*m_nBinWidth*2 + j * 2 + 1 + 0];
				}
				else
				{
					Y = imgYUV[k*m_nBinWidth*4 + j*4 + 2];
					U = imgYUV[k*m_nBinWidth*4 + j*4 + 0];
					V = imgYUV[k*m_nBinWidth*4 + j*4 + 4];
				}
				/*
				if ( m_bStereoBin )
				{
					m_yuv_origin[k*m_nBinWidth*2 + j/2 + 0] = U;
					m_yuv_origin[k*m_nBinWidth*2 + j/2 + 1] = Y;
					m_yuv_origin[k*m_nBinWidth*2 + j/2 + 2] = V;
				}
				*/

				double temp = 0;
				temp = (Y-16)*1.164+(V-128)*2.017;  
				if(temp < 0)	temp = 0;
				if(temp > 255)	temp = 255;
				R = (unsigned char)temp;

				temp = (Y-16)*1.164-(U-128)*0.813-(V-128)*0.392; 
				if(temp < 0)	temp = 0;
				if(temp > 255)	temp = 255;
				G = (unsigned char)temp;

				temp = (Y-16)*1.164+(U-128)*1.596; 
				if(temp < 0)	temp = 0;
				if(temp > 255)	temp = 255;
				B = (unsigned char)temp;

				m_matOrigin.data[k*widthStep + j*channel + 0] = B;
				m_matOrigin.data[k*widthStep + j*channel + 1] = G;
				m_matOrigin.data[k*widthStep + j*channel + 2] = R;

				if ( m_bStereoBin )
				{
					m_matOrigin2.data[k*widthStep + j*channel + 0] = imgYUV[k*m_nBinWidth*4 + j*4 + 1];
					m_matOrigin2.data[k*widthStep + j*channel + 1] = imgYUV[k*m_nBinWidth*4 + j*4 + 1];
					m_matOrigin2.data[k*widthStep + j*channel + 2] = imgYUV[k*m_nBinWidth*4 + j*4 + 1];
				}

				if ( m_bMandoRearCam )
					Y = imgYUV[k*m_nBinWidth * 2 + (j + 1) * 2 + 0];
				else
					Y = imgYUV[k*m_nBinWidth * 4 + (j+1)*4 + 2];

				if ( m_bStereoBin )
				{
					//m_yuv_origin[k*m_nBinWidth*2 + j/2 + 3] = Y;
				}

				temp = (Y-16)*1.164+(V-128)*2.017;  
				if(temp < 0)	temp = 0;
				if(temp > 255)	temp = 255;
				R = (unsigned char)temp;

				temp = (Y-16)*1.164-(U-128)*0.813-(V-128)*0.392; 
				if(temp < 0)	temp = 0;
				if(temp > 255)	temp = 255;
				G = (unsigned char)temp;

				temp = (Y-16)*1.164+(U-128)*1.596; 
				if(temp < 0)	temp = 0;
				if(temp > 255)	temp = 255;
				B = (unsigned char)temp;

				m_matOrigin.data[k*widthStep + (j+1)*channel + 0] = B;
				m_matOrigin.data[k*widthStep + (j+1)*channel + 1] = G;
				m_matOrigin.data[k*widthStep + (j+1)*channel + 2] = R;

				if ( m_bStereoBin )
				{
					m_matOrigin2.data[k*widthStep + (j+1)*channel + 0] = imgYUV[k*m_nBinWidth*4 + (j+1)*4 + 1];
					m_matOrigin2.data[k*widthStep + (j+1)*channel + 1] = imgYUV[k*m_nBinWidth*4 + (j+1)*4 + 1];
					m_matOrigin2.data[k*widthStep + (j+1)*channel + 2] = imgYUV[k*m_nBinWidth*4 + (j+1)*4 + 1];
				}
			}
		}
	}

	return true;
}

/*
//for mando rear cam
//non stereo image
//Y0 V0 Y1 U0   Y2 V1 Y3 U1
bool CVideoWnd::ReadFrameFromBinFile()
{
	if ( m_FileBin.m_hFile == CFile::hFileNull )
		return false;

	unsigned __int64 SeekIndex = 0;
	int Y, U, V;
	unsigned char R,G,B;
	unsigned char*	imgYUV = new uint8_t[m_nBinWidth * m_nBinHeight * 4]; 

	int index_y = 0, index_u = 0, index_v = 0;
	int index_qvga_y = 0, index_qvga_uv = 0;
	int k = 0, j = 0;
	int widthStep, channel;

	widthStep = m_matOrigin.step;
	channel = m_matOrigin.channels();
	index_u = m_nBinWidth * m_nBinHeight;
	index_v = m_nBinWidth * m_nBinHeight + m_nBinWidth * m_nBinHeight / 2;

	SeekIndex = (unsigned __int64)m_nCurrentFrame * (m_nBinWidth * m_nBinHeight * 2);

	SeekIndex += (1000*(m_nCurrentFrame+1) + (11288)*(m_nCurrentFrame));
	
	if ( m_FileBin.Seek( SeekIndex, CFile::begin ) >= 0 )
	{
		m_FileBin.Read( imgYUV, m_nBinWidth * m_nBinHeight * 2 );
	}		

	for(k = 0; k < m_nBinHeight ; k++)
	{
		for(j = 0; j < m_nBinWidth ; j+=2)
		{
			Y = imgYUV[k*m_nBinWidth*2 + j*2 + 0];
			U = imgYUV[k*m_nBinWidth*2 + (j+1)*2 + 1];
			V = imgYUV[k*m_nBinWidth*2 + j*2 + 1 + 0];

			double temp = 0;
			temp = (Y-16)*1.164+(V-128)*2.017;  
			if(temp < 0)	temp = 0;
			if(temp > 255)	temp = 255;
			R = (unsigned char)temp;

			temp = (Y-16)*1.164-(U-128)*0.813-(V-128)*0.392; 
			if(temp < 0)	temp = 0;
			if(temp > 255)	temp = 255;
			G = (unsigned char)temp;

			temp = (Y-16)*1.164+(U-128)*1.596; 
			if(temp < 0)	temp = 0;
			if(temp > 255)	temp = 255;
			B = (unsigned char)temp;


			m_matOrigin.data[k*widthStep + j*channel + 0] = B;
			m_matOrigin.data[k*widthStep + j*channel + 1] = G;
			m_matOrigin.data[k*widthStep + j*channel + 2] = R;

			Y = imgYUV[k*m_nBinWidth*2 + (j+1)*2 + 0];

			temp = (Y-16)*1.164+(V-128)*2.017;  
			if(temp < 0)	temp = 0;
			if(temp > 255)	temp = 255;
			R = (unsigned char)temp;

			temp = (Y-16)*1.164-(U-128)*0.813-(V-128)*0.392; 
			if(temp < 0)	temp = 0;
			if(temp > 255)	temp = 255;
			G = (unsigned char)temp;

			temp = (Y-16)*1.164+(U-128)*1.596; 
			if(temp < 0)	temp = 0;
			if(temp > 255)	temp = 255;
			B = (unsigned char)temp;

			m_matOrigin.data[k*widthStep + (j+1)*channel + 0] = B;
			m_matOrigin.data[k*widthStep + (j+1)*channel + 1] = G;
			m_matOrigin.data[k*widthStep + (j+1)*channel + 2] = R;
		}
	}

	delete [] imgYUV;

	return true;
}
*/

void CVideoWnd::OnMButtonDown(UINT nFlags, CPoint point)
{
	// TODO: Add your message handler code here and/or call default
	::SendMessage( m_hParentWnd, message_video_wnd_mouse_event, m_nVideoWndID, (LPARAM)&CMouseEvent( point, WM_MBUTTONDOWN ) );

	CWnd::OnMButtonDown(nFlags, point);
}

void CVideoWnd::OnMButtonUp(UINT nFlags, CPoint point)
{
	// TODO: Add your message handler code here and/or call default
	::SendMessage( m_hParentWnd, message_video_wnd_mouse_event, m_nVideoWndID, (LPARAM)&CMouseEvent( point, WM_MBUTTONUP ) );

	CWnd::OnMButtonUp(nFlags, point);
}


BOOL CVideoWnd::OnMouseWheel(UINT nFlags, short zDelta, CPoint point)
{
	// TODO: Add your message handler code here and/or call default
	::SendMessage( m_hParentWnd, message_video_wnd_mouse_event, m_nVideoWndID, (LPARAM)&CMouseEvent( point, WM_MOUSEWHEEL, zDelta ) );

	return CWnd::OnMouseWheel(nFlags, zDelta, point);
}


void CVideoWnd::PreSubclassWindow()
{
	// TODO: Add your specialized code here and/or call the base class
	m_hParentWnd = ::GetParent( m_hWnd );

	//만약 동적으로 CVideoWnd를 생성할 경우에는 아래 툴팁컨트롤 코드에서 에러가 발생하므로
	//주석처리 해줘야 한다.
	/*
	CVideoWnd* pWnd = this;
	m_ToolTip.Create( this, TTS_ALWAYSTIP | TTS_NOPREFIX | TTS_NOANIMATE );
	m_ToolTip.SetDelayTime( TTDT_AUTOPOP, -1 );
	m_ToolTip.SetDelayTime( TTDT_INITIAL, 0 );
	m_ToolTip.SetDelayTime( TTDT_RESHOW, 0 );
	m_ToolTip.SetMaxTipWidth(400); 
	m_ToolTip.AddTool( this, _T("") );
	m_ToolTip.Activate(TRUE);
	*/
	CWnd::PreSubclassWindow();
}


void CVideoWnd::OnDropFiles(HDROP hDropInfo)
{
	// TODO: Add your message handler code here and/or call default
	TCHAR	sfile[MAX_PATH];

	DragQueryFile( hDropInfo, 0, sfile, FILENAME_MAX );

	if ( IsCtrlPressed() )
		OpenSequenceImages( sfile );
	else
		OpenVideoFile( sfile, false, _T(""), m_bAutoPlayWhenDrop );

	CWnd::OnDropFiles(hDropInfo);
}

bool CVideoWnd::CheckROIValidation( CRect &roi )
{
	if ( m_mat.empty() )
		return false;

	roi.NormalizeRect();

	Clamp( (int&)roi.left, 0, m_mat.cols );
	Clamp( (int&)roi.top, 0, m_mat.rows );
	if ( roi.right <= roi.left || roi.right > m_mat.cols )
		roi.right = m_mat.cols;
	if ( roi.bottom <= roi.top || roi.bottom > m_mat.rows )
		roi.bottom = m_mat.rows;

	if ( roi.left < 0 || roi.top < 0 || roi.right > m_mat.cols || roi.bottom > m_mat.rows )
		return false;

	return true;
}

void CVideoWnd::CheckScreenROIValidation()
{
	//왼쪽 위로 드래그한 경우 l,t와 r,b의 값을 보정해준다.
	m_rScreenROI.NormalizeRect();

	//너무 작은 영역의 ROI는 ROI해제로 처리한다.
	if ( m_rScreenROI.Width() < 10 || m_rScreenROI.Height() < 10 )
	{
		m_rImageROI = CRect( 0, 0, m_szImageSize.cx, m_szImageSize.cy );
		m_rScreenROI.SetRectEmpty();
		Invalidate();

		return;
	}

	//screen roi는 m_rDisplayedImageRect를 넘지 않는다.
	if ( m_rScreenROI.left < m_rDisplayedImageRect.left )
		m_rScreenROI.left = m_rDisplayedImageRect.left;
	if ( m_rScreenROI.top < m_rDisplayedImageRect.top )
		m_rScreenROI.top = m_rDisplayedImageRect.top;
	if ( m_rScreenROI.right > m_rDisplayedImageRect.right )
		m_rScreenROI.right = m_rDisplayedImageRect.right;
	if ( m_rScreenROI.bottom > m_rDisplayedImageRect.bottom )
		m_rScreenROI.bottom = m_rDisplayedImageRect.bottom;
}

bool CVideoWnd::IsROIExist()
{
	return (CRect(0,0,m_szImageSize.cx,m_szImageSize.cy) != m_rImageROI);
}

void CVideoWnd::ConvertCoordinateImage2Screen( CRect &r )
{
	//이미지 원본에서의 roi를 screen 상의 roi로 변환한다.
	double dZoom = (double)m_rDisplayedImageRect.Width() / (double)(m_mat.cols);//(double)m_szImageSize.cx;

	r.left	= dZoom * (double)r.left;
	r.top	= dZoom * (double)r.top;
	r.right	= dZoom * (double)r.right;
	r.bottom= dZoom * (double)r.bottom;

	r.OffsetRect( m_rDisplayedImageRect.left, m_rDisplayedImageRect.top );
}

void CVideoWnd::ConvertCoordinateScreen2Image( CRect &r )
{
	double dZoom = (double)m_rDisplayedImageRect.Width() / (double)(m_mat.cols);
			
	//화면에 표시된 영상의 l,t값을 빼서 확대 영상내에서의 상대좌표로 변환한 후
	r.OffsetRect( -m_rDisplayedImageRect.left, -m_rDisplayedImageRect.top );

	//확대된 비율로 나누고
	r.left	= (double)r.left / dZoom;
	r.top	= (double)r.top	/ dZoom;
	r.right	= (double)r.right / dZoom;
	r.bottom= (double)r.bottom / dZoom;

	if ( r.right > m_szImageSize.cx )
		r.right = m_szImageSize.cx;
	if ( r.bottom >= m_szImageSize.cy )
		r.bottom = m_szImageSize.cy;
}

void CVideoWnd::OnPopupMenu( UINT nID )
{
	TCHAR buf[128] = _T("\0");
	TCHAR msg[128] = _T("\0");
	CString str;

	switch ( nID )
	{
		case id_menu_file_open :
				OpenVideoFile(_T(""), true, m_sVideoFileName);
				break;
		case id_menu_file_open_sequence :
				{
					CString			sfile = AfxGetApp()->GetProfileString( _T("setting\\video"), _T("recent sequence image"), _T("") );
					TCHAR			szFilter[] = _T("Media (*.mp4,*.wmv,*.avi,*.bin,*.yuv,*.bmp,*.jpg,*.png)|*.mp4;*.wmv;*.avi;*.bin;*.yuv;*.bmp;*.jpg,*.png|All Filess(*.*)|*.*||");
					CFileDialog		dlg(true, _T("*.*"), sfile, OFN_FILEMUSTEXIST, szFilter, this);

					if ( dlg.DoModal() == IDCANCEL )
						return;

					sfile = dlg.GetPathName();
					if ( sfile == "" )
						return;

					OpenSequenceImages( sfile );
				}
				break;
		case id_menu_do_not_image_processing :
			m_do_not_image_processing = !m_do_not_image_processing;
			break;
		case id_menu_yuv_stereo :
			{
				int oldState = GetPlayState();
				Play(PAUSE);
				m_bStereoBin = !m_bStereoBin;
				AfxGetApp()->WriteProfileInt( _T("setting\\video"), _T("is stereo bin"), m_bStereoBin );

				//현재 stereo bin은 uyvy만 지원된다.
				if ( m_bStereoBin )
				{
					m_yuv_format = yuv422_uyvy;
					AfxGetApp()->WriteProfileInt( _T("setting\\video"), _T("yuv format"), m_yuv_format );
				}

				delete [] imgYUV;
				imgYUV = NULL;
				Play(oldState);
				break;
			}
		case id_menu_yuv_size :
		{
			_stprintf(msg, _T("Input the video size.(current = %d x %d)\nEx) \"320 180\", \"320x180\", \"320,180\", \"320.180\""), m_nBinWidth, m_nBinHeight );
			if (CWin32InputBox::InputBox(_T("Video size"), msg, buf, 128, CWin32InputBox::NORMAL, 0, m_hWnd) == IDOK)
			{
				if ( _tcslen(buf) > 0 )
				{
					str = buf;
					str.MakeLower();
					str.Replace( _T("x"), _T(" ") );
					str.Replace( _T("."), _T(" ") );
					str.Replace( _T(","), _T(" ") );
					str.Trim();

					if ( get_char_count( str, ' ' ) == 0 )
					{
						AfxMessageBox( _T("Invalid video size."), MB_ICONEXCLAMATION );
						return;
					}

					int newW = _ttoi( str.Left( str.Find(_T(" ")) ) );
					int newH = _ttoi( str.Mid( str.Find(_T(" ")) ) );

					if ( newW % 4 != 0 || newW <= 0 || newH <= 0 )
					{
						AfxMessageBox( _T("Invalid video size."), MB_ICONEXCLAMATION );
						return;
					}

					int oldState = GetPlayState();
					Play(PAUSE);

					m_nBinWidth = newW;
					m_nBinHeight = newH;

					if ( m_matOrigin.cols != m_nBinWidth || m_matOrigin.rows != m_nBinHeight )
					{
						m_matOrigin.release();
						m_matOrigin.create( m_nBinHeight, m_nBinWidth, CV_8UC3 );

						m_matOrigin.copyTo( m_matOrigin2 );
						m_matOrigin.copyTo( m_mat );
						m_szImageSize = m_szImageSizeOrigin = CSize(m_mat.cols, m_mat.rows);
						m_rImageROI = CRect(0, 0, m_mat.cols, m_mat.rows);
					}

					AfxGetApp()->WriteProfileInt( _T("setting\\video"), _T("bin width"), m_nBinWidth );
					AfxGetApp()->WriteProfileInt( _T("setting\\video"), _T("bin height"), m_nBinHeight );

					if ( m_nFileType == FILE_TYPE_VIDEO_BIN )
					{
						delete [] imgYUV;
						imgYUV = NULL;
						Play(oldState);
					}
					else
					{
						m_matOrigin = loadImage( m_sVideoFileName, IMREAD_UNCHANGED, false, m_nBinWidth, m_nBinHeight, m_yuv_format );
						updateMainMat( NULL, true );
					}
				}
			}
			break;
		}
		case id_menu_yuv422_uyvy :
				m_yuv_format = yuv422_uyvy;
				AfxGetApp()->WriteProfileInt( _T("setting\\video"), _T("yuv format"), m_yuv_format );
				m_matOrigin = loadImage( m_sVideoFileName, IMREAD_UNCHANGED, false, m_nBinWidth, m_nBinHeight, m_yuv_format );
				updateMainMat( NULL, true );
				break;
		case id_menu_yuv422_yuyv :
				m_yuv_format = yuv422_yuyv;
				AfxGetApp()->WriteProfileInt( _T("setting\\video"), _T("yuv format"), m_yuv_format );
				m_matOrigin = loadImage( m_sVideoFileName, IMREAD_UNCHANGED, false, m_nBinWidth, m_nBinHeight, m_yuv_format );
				updateMainMat( NULL, true );
				break;
		case id_menu_yuv420_nv12 :
				m_yuv_format = yuv420_nv12;
				AfxGetApp()->WriteProfileInt( _T("setting\\video"), _T("yuv format"), m_yuv_format );
				m_matOrigin = loadImage( m_sVideoFileName, IMREAD_UNCHANGED, false, m_nBinWidth, m_nBinHeight, m_yuv_format );
				updateMainMat( NULL, true );
				break;
		case id_menu_yuv420_yv12 :
				m_yuv_format = yuv420_yv12;
				AfxGetApp()->WriteProfileInt( _T("setting\\video"), _T("yuv format"), m_yuv_format );
				m_matOrigin = loadImage( m_sVideoFileName, IMREAD_UNCHANGED, false, m_nBinWidth, m_nBinHeight, m_yuv_format );
				updateMainMat( NULL, true );
				break;

		case id_menu_resize_ratio :
				_stprintf(buf, _T("%f"), m_dResizeRatio);
				if (CWin32InputBox::InputBox(_T("Resize Ratio"), _T("Input the resize ratio in float type.(original size = 1.0)"), buf, 128, CWin32InputBox::NORMAL, 0, m_hWnd) == IDOK)
				{
					if ( _tcslen(buf) > 0 )
						SetResizeRatio(_ttof(buf));
				}
				break;
		case id_menu_roi_set :
				m_bROI_Set = !m_bROI_Set;
				m_bROI_Move = false;
				break;
		case id_menu_roi_reset :
				OnMenuROIReset();
				break;
		case id_menu_roi_move :
				m_bROI_Move = !m_bROI_Move;
				m_bROI_Set = false;
				break;
		case id_menu_goto_frame :
				if (CWin32InputBox::InputBox(_T("Go to frame"), _T("Input the frame number"), buf, 128, CWin32InputBox::NORMAL, 0, m_hWnd) == IDOK)
					GotoFrame(_ttoi(buf));
				break;
		case id_menu_repeat :
				m_bRepeat = !m_bRepeat;
				AfxGetApp()->WriteProfileInt( _T("setting\\video"), _T("repeat"), m_bRepeat );
				break;
		case id_menu_auto_next_video :
				m_auto_next_video = !m_auto_next_video;
				AfxGetApp()->WriteProfileInt( _T("setting\\video"), _T("auto next video"), m_auto_next_video );
				break;
		case id_menu_save_recent_frame :
				m_bSaveRecentFrame = !m_bSaveRecentFrame;
				AfxGetApp()->WriteProfileInt( _T("setting\\video"), _T("save recent frame"), m_bSaveRecentFrame );
				break;

		case id_menu_show_vanishing_point :
				m_bShowVanishingPoint = !m_bShowVanishingPoint;
				Invalidate();
				AfxGetApp()->WriteProfileInt( _T("setting\\video"), _T("show vanishing point"), m_bShowVanishingPoint );
				break;
		case id_menu_get_vanishing_point :
				Play(PAUSE);
				m_bGetVanishingPoint = !m_bGetVanishingPoint;
				m_nVanishingPointCount = 0;
				Invalidate();
				break;
		case id_menu_set_vanishing_point :
				_stprintf( msg, _T("Input the vanishing point.\nCurrent image size : %d x %d\nCurrent vanishing point : %d, %d"),
									m_mat.cols, m_mat.rows, m_ptVanishingPoint.x, m_ptVanishingPoint.y );
				if (CWin32InputBox::InputBox(_T("Set vanishing point"), msg, buf, 128, CWin32InputBox::NORMAL, 0, m_hWnd) == IDOK)
				{
					CString str = buf;
					str.Trim();
					str.Replace( _T(","), _T(" ") );
					if (get_char_count(str, ' ') != 1)
					{
						AfxMessageBox(_T("소실점 x y의 좌표를 123 456과 같이 입력해야 합니다."), MB_ICONEXCLAMATION);
						return;
					}
					m_ptVanishingPoint.x = _ttoi( str.Left( str.Find(_T(" ")) ) );
					m_ptVanishingPoint.y = _ttoi( str.Mid( str.Find(_T(" ")) ) );
				}
		break;
		case id_menu_measure_size :
				m_bMeasureSize = !m_bMeasureSize;
				break;
		case id_menu_copy_to_clipboard :
				CopyToClipboard();
				break;
		case id_menu_paste_from_clipboard :
				PasteFromClipboard();
				break;
		case id_menu_save_as_original_size :
				SaveAsOriginalSize();
				break;
		case id_menu_save_as_displayed_size :
				SaveAsDisplayedSize();
				break;
	}
}

//sfile 파일이 존재하는 폴더내의 동일 타입 그림파일 시퀀스를 열어준다.
void CVideoWnd::OpenSequenceImages( CString sfile )
{
	AfxGetApp()->WriteProfileString( _T("setting\\video"), _T("recent sequence image"), sfile );
	BuildImageArray( sfile, get_part(sfile, fn_ext) );
}

void CVideoWnd::OnMenuROIReset()
{
	m_bROI_Set = false;
	m_bROI_Move = false;

	//ROI가 설정된 상태가 아니라면 그냥 리턴
	if ( m_rImageROI == CRect( 0, 0, m_szImageSize.cx, m_szImageSize.cy ) )
		return;

	//ImageROI 해제
	m_rScreenROI.SetRectEmpty();
	m_rImageROI = CRect( 0, 0, m_szImageSize.cx, m_szImageSize.cy );

	//pause일때 ROI가 해제되면 원크기 영상으로 직접 갱신시켜줘야 한다.
	if ( m_nPlayState == PAUSE || m_nFileType == FILE_TYPE_IMAGE )
	{
		if ( m_matOrigin.data == NULL )
		{
			if ( m_nFileType == FILE_TYPE_VIDEO_BIN )
				ReadFrameFromBinFile();
			else
				m_capture.retrieve( m_matOrigin );
		}
		
		resize( m_matOrigin, m_mat, cv::Size( m_szImageSize.cx, m_szImageSize.cy ), 0.0, 0.0, m_nInterpolation );
		m_mat( cv::Rect(m_rImageROI.left, m_rImageROI.top, m_rImageROI.Width(), m_rImageROI.Height()) ).copyTo( m_mat );
		updateMainMat( &m_mat );
	}

	if ( m_hParentWnd && m_bSendPlayWndInfo )
	{
		::SendMessage( m_hParentWnd, message_video_wnd_roi_changed, m_nVideoWndID, 0 );
		SendTrackChanged();
	}
}

void CVideoWnd::CopyToClipboard()
{
	Play(PAUSE);

	//너비가 4의 배수가 아닌 mat을 
	//ConvertMatToBMP()와 PasteBMPToClipboard() 함수를 사용하여 클립보드로 복사하면
	//일그러진 이미지가 클립보드로 복사된다.
	//일단 CImage로 변경하여 이 문제를 해결했다.
	CImage img;
	matToCImage(m_mat, img);

	CDC memDC;
	memDC.CreateCompatibleDC(NULL);

	CBitmap bitmap;
	bitmap.CreateCompatibleBitmap(GetDC(), img.GetWidth(), img.GetHeight());
	memDC.SelectObject(&bitmap);
	img.BitBlt(memDC.GetSafeHdc(), 0, 0, img.GetWidth(), img.GetHeight(), 0, 0, SRCCOPY);

	//PasteBMPToClipboard( bitmap.GetSafeHandle() );
	if (OpenClipboard())
	{
		EmptyClipboard();
		SetClipboardData(CF_DIB, bitmap.GetSafeHandle());
		CloseClipboard();
	}

	memDC.DeleteDC();
	bitmap.Detach();
}

void CVideoWnd::CopyVideonameAndFrameToClipboard()
{
	CString str;

	str.Format( _T("%s %d\n"), m_sVideoFileName, m_nCurrentFrame );

	HGLOBAL glob = GlobalAlloc( GMEM_FIXED, sizeof(TCHAR) * str.GetLength() );
	memcpy( glob, str, sizeof(TCHAR) * str.GetLength() );
	OpenClipboard();
	EmptyClipboard();

#ifdef _UNICODE
	SetClipboardData(CF_UNICODETEXT,glob);
#else
	SetClipboardData(CF_TEXT,glob);
#endif

	CloseClipboard();
}

void CVideoWnd::PasteFromClipboard()
{
	HBITMAP bitmap;
	if ( OpenClipboard() == false )
		return;

	bitmap = (HBITMAP)GetClipboardData(CF_BITMAP);
	if ( !bitmap )
	{
		CloseClipboard();
		return;
	}

	int n = sizeof( bitmap );
	CloseClipboard();

	//미디어가 재생중일때 붙여넣기를 하는 경우라면
	//재생을 일시정지 시켜줘야 한다.
	Play( PAUSE );

	m_mat.release();
	m_matOrigin.release();
	m_matOrigin2.release();

    HDC hdc = ::GetDC(m_hWnd);
    BITMAPINFO bmi;
    BITMAPINFOHEADER* bmih = &(bmi.bmiHeader);

    ZeroMemory(bmih, sizeof(BITMAPINFOHEADER));
    bmih->biSize = sizeof(BITMAPINFOHEADER);

	//클립보드의 비트맵을 가져오면 무조건 32비트로 읽혀진다.
	//붙여넣기는 일단 24비트로 붙여넣는다.
	//또한 너비가 4의 배수가 아닌 경우는 밀리는 현상이 발생하므로
	//m_mat을 4의 배수로 만들어주고 나중에 늘린 너비는 잘라준다.
    if ( GetDIBits(hdc, bitmap, 0, 0, NULL, &bmi, DIB_RGB_COLORS) )
	{
		int ch = bmih->biBitCount / 8;
		int type = CV_MAKETYPE(CV_8U,ch);
		int width = MAKE4WIDTH(bmih->biWidth);
        int height = bmih->biHeight;
		int padding = width - bmih->biWidth;

        m_mat.create( height, width, CV_8UC3 );
		bmih->biWidth = width;
		bmih->biHeight = -height;
        bmih->biBitCount = 24;
        bmih->biCompression = BI_RGB;
        GetDIBits(hdc, bitmap, 0, height, m_mat.data, &bmi,DIB_RGB_COLORS);
		if ( padding )
			m_mat(cv::Rect( 0, 0, bmih->biWidth - padding, height )).copyTo( m_mat );

		m_bImageFromClipboard = true;
		m_mat.copyTo(m_matOrigin);

		m_szImageSize = m_szImageSizeOrigin = CSize(m_mat.cols, m_mat.rows);
		m_rImageROI = CRect(0, 0, m_mat.cols, m_mat.rows);

		if (m_nFileType == FILE_TYPE_UNKNOWN)
			m_nFileType = FILE_TYPE_IMAGE;
    }

	DeleteObject( bitmap );
    ::ReleaseDC(NULL, hdc);

	updateMainMat();

	//Invalidate();
}

void CVideoWnd::SaveAsDisplayedSize()
{
	Play(PAUSE);

	int		nIndex = m_nSaveIndex;
	bool	bShiftKeyPressed = IsShiftPressed();
	CString sMediaFilename;
	CString sImageFile;

	//roi와 index의 관계가?
	if ( IsVideoFileOpened() && !m_bImageFromClipboard)//&& !IsROIExist() )
		nIndex = m_nCurrentFrame;

	if (IsVideoFileOpened())
		sMediaFilename = get_part(m_sVideoFileName, fn_name);
	else// if (m_bImageFromClipboard)
		sMediaFilename = _T("snapshot");

	CString sRecentSavedFolder = AfxGetApp()->GetProfileString( _T("setting\\video"), _T("recent saved folder"), GetExeDirectory() );
	CString sRecentSavedFormat = AfxGetApp()->GetProfileString( _T("setting\\video"), _T("recent saved format"), _T("*.jpg") );
	sImageFile.Format(_T("%s\\%s_%08d.jpg"), sRecentSavedFolder, sMediaFilename, nIndex);
	CFileDialog fileDlg( false, sRecentSavedFormat, sImageFile, OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT, _T("JPG file|*.jpg|BMP file|*.bmp|All files|*.*||") );

	if ( fileDlg.DoModal() == IDCANCEL )
		return;

	if ( !IsVideoFileOpened() || m_bImageFromClipboard)//|| IsROIExist() )
		m_nSaveIndex++;

	//화면 캡쳐 방식으로 영상을 저장하므로 파일 대화상자가 사라질때까지 약간의 딜레이가 필요하다.
	Wait( 500 );

	CClientDC dc(this);
	//save(CaptureWindowToBitmap(m_hWnd, m_rDisplayedImageRect), dc.GetSafeHdc(), (LPTSTR)(LPCTSTR)fileDlg.GetPathName() );
	MessageBeep( 0 );

	if ( bShiftKeyPressed || IsShiftPressed() )
		ShellExecute( m_hWnd, _T("open"), fileDlg.GetPathName(), NULL, NULL, SW_SHOWNORMAL );

	sRecentSavedFormat.Format( _T("*.%s"), get_part(fileDlg.GetPathName(), fn_ext) );
	AfxGetApp()->WriteProfileString( _T("setting\\video"), _T("recent saved folder"), get_part(fileDlg.GetPathName(), fn_folder) );
	AfxGetApp()->WriteProfileString( _T("setting\\video"), _T("recent saved format"), sRecentSavedFormat );
}

void CVideoWnd::SaveAsOriginalSize()
{
	Play(PAUSE);

	int		nIndex = m_nSaveIndex;
	bool	bShiftKeyPressed = IsShiftPressed();
	CString sMediaFilename;
	CString sImageFile;

	if ( IsVideoFileOpened() && !m_bImageFromClipboard)//&& !IsROIExist() )
		nIndex = m_nCurrentFrame;

	if (IsVideoFileOpened())
		sMediaFilename = get_part(m_sVideoFileName, fn_name);
	else// if (m_bImageFromClipboard)
		sMediaFilename = _T("snapshot");

	CString sRecentSavedFolder = AfxGetApp()->GetProfileString( _T("setting\\video"), _T("recent saved folder"), GetExeDirectory() );
	CString sRecentSavedFormat = AfxGetApp()->GetProfileString( _T("setting\\video"), _T("recent saved format"), _T("*.jpg") );
	sImageFile.Format( _T("%s\\%s_%08d.jpg"), sRecentSavedFolder, sMediaFilename, nIndex );
	CFileDialog fileDlg( false, sRecentSavedFormat, sImageFile, OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT, _T("JPG file|*.jpg|BMP file|*.bmp|All files|*.*||") );

	if ( fileDlg.DoModal() == IDCANCEL )
		return;

	if ( !IsVideoFileOpened() || m_bImageFromClipboard)//|| IsROIExist() )
		m_nSaveIndex++;


	imwrite( CString2string(fileDlg.GetPathName()), m_mat );

	if ( bShiftKeyPressed || IsShiftPressed() )
		ShellExecute( m_hWnd, _T("open"), fileDlg.GetPathName(), NULL, NULL, SW_SHOWNORMAL );

	sRecentSavedFormat.Format( _T("*.%s"), get_part(fileDlg.GetPathName(), fn_ext) );
	AfxGetApp()->WriteProfileString( _T("setting\\video"), _T("recent saved folder"), get_part(fileDlg.GetPathName(), fn_folder) );
	AfxGetApp()->WriteProfileString( _T("setting\\video"), _T("recent saved format"), sRecentSavedFormat );
}


void CVideoWnd::OnSetFocus(CWnd* pOldWnd)
{
	CWnd::OnSetFocus(pOldWnd);

	// TODO: Add your message handler code here
	Invalidate();
}


void CVideoWnd::OnKillFocus(CWnd* pNewWnd)
{
	CWnd::OnKillFocus(pNewWnd);

	// TODO: Add your message handler code here
	Invalidate();
}


void CVideoWnd::OnActivate(UINT nState, CWnd* pWndOther, BOOL bMinimized)
{
	CWnd::OnActivate(nState, pWndOther, bMinimized);

	// TODO: Add your message handler code here
	CString str;
	GetWindowText( str );
	TRACE(_T("%s OnActivate\n"), str );
}


void CVideoWnd::OnActivateApp(BOOL bActive, DWORD dwThreadID)
{
	CWnd::OnActivateApp(bActive, dwThreadID);

	// TODO: Add your message handler code here
	CString str;
	GetWindowText( str );
	TRACE(_T("%s OnActivateApp\n"), str );
}

void CVideoWnd::SetShowInfoText( int idx, bool bShow )
{
	if ( idx < -1 || idx > 3 )
		return;

	if ( idx == -1 )
	{
		for ( int i = 0; i < 4; i++ )
			m_bShowInfoText[i] = bShow;
	}
	else
	{
		m_bShowInfoText[idx] = bShow;
	}
}

void CVideoWnd::DisplayInfoText( int idx )
{
	//invalidate을 빈번히 하면 매우 랙이 발생한다.
	//이미지 파일이거나 pause 상태일때만 화면을 갱신시켜야 한다.
	if ( GetPlayState() != PLAY )
		InvalidateRect( m_rInfoText[idx], false );
}


BOOL CVideoWnd::PreCreateWindow(CREATESTRUCT& cs)
{
	// TODO: Add your specialized code here and/or call the base class
	//HWND hWnd = GetParent()->GetSafeHwnd();

	return CWnd::PreCreateWindow(cs);
}

CRect CVideoWnd::GetDisplayedImageRect()
{
	CRect	rc;

	if ( m_mat.empty() )
		return 0;

	GetClientRect( rc );
	m_rDisplayedImageRect = GetRatioRect( rc, m_mat.cols, m_mat.rows );

	return m_rDisplayedImageRect;
}


void CVideoWnd::OnWindowPosChanged(WINDOWPOS* lpwndpos)
{
	CWnd::OnWindowPosChanged(lpwndpos);

	// TODO: Add your message handler code here
	//TRACE( "cx = %d, cy = %d\n", lpwndpos->cx, lpwndpos->cy );
	//vp구하는 중에 비디오 창의 크기가 변하면 vp 구하는 기능을 자동 off시킨다.
	if (m_bGetVanishingPoint)
	{
		m_bGetVanishingPoint = false;
		m_nVanishingPointCount = 0;
		Invalidate();
	}
}

void CVideoWnd::BuildImageArray(CString sfile, CString sExts)
{
	m_bUseImageArray = true;
	m_dqImageFiles.clear();

	CString sfolder;

	if (IsFolder(sfile))
	{
		sfolder = sfile;
		sfile = _T("");
	}
	else
	{
		sfolder = get_part(sfile, fn_folder);
	}

	FindAllFiles(sfolder, &m_dqImageFiles, _T("*"), sExts);
	m_nTotalFrame = m_dqImageFiles.size();
	m_nCurrentFrame = find_index(m_dqImageFiles, sfile);

	m_nFileType = FILE_TYPE_IMAGE;
	GotoFrame(m_nCurrentFrame);
}

void CVideoWnd::OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags)
{
	//하나의 CVideoWnd를 가진 앱에서는 핫키를 어디서 처리해도 관계없으나
	//2개 이상일 경우는 핫키로 원하는 제어를 여기서 할 순 없다.
	//mainDlg에서 핫키를 처리하는 것이 맞다. (ex.play pause...)
	//핫키가 눌려지면 mainDlg의 PreTranslateMessage() 함수에서 먼저 처리하므로
	//그 함수에서 처리하거나 child로 넘길 경우는 return false;해주면 된다.
	//TRACE(_T("CVideoWnd::OnKeyDown = %d\n"), nChar);

	CWnd::OnKeyDown(nChar, nRepCnt, nFlags);
}

//send TRACK_CHANGED message and display frame info
void CVideoWnd::SendTrackChanged()
{
	::SendMessage( m_hParentWnd, message_video_wnd_track_changed, m_nVideoWndID, m_nCurrentFrame );

	CString str;

	if ( m_nFileType == FILE_TYPE_IMAGE && m_bUseImageArray )
		str.Format( _T("%d / %d"), m_nCurrentFrame + 1, m_nTotalFrame );
	else if ( m_nFileType == FILE_TYPE_VIDEO || m_nFileType == FILE_TYPE_VIDEO_BIN )
	{
		if ( m_sVideoFileName == "cam" )
			str = "cam";
		else
			str.Format( _T("%d / %d"), m_nCurrentFrame, m_nTotalFrame );
	}

	if ( str != "" )
		SetInfoText( 0, str );
}

void CVideoWnd::build_file_list()
{
	if ( m_sVideoFileName == "" )
		return;

	m_dqFileList.clear();
	FindAllFiles(get_part(m_sVideoFileName, fn_folder), &m_dqFileList, _T("*"), get_part(m_sVideoFileName, fn_ext) );
	std::sort( m_dqFileList.begin(), m_dqFileList.end() );
}

void CVideoWnd::open_next_video( bool next )
{
	if ( m_sVideoFileName == "" )
		return;

	if ( m_dqFileList.size() == 0 )
		build_file_list();

	int idx = find_index(m_dqFileList, m_sVideoFileName);// , true, true );

	if ( next )
		idx++;
	else
		idx--;

	if ( idx >= 0 && idx < m_dqFileList.size() )
	{
		AfxGetApp()->WriteProfileInt( _T("setting\\video"), _T("recent frame"), 0 );
		OpenVideoFile( m_dqFileList[idx] );
	}
}

