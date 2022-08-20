// RexPlateCtrl.cpp : 구현 파일입니다.
//

#include "stdafx.h"
#include "RexPlateCtrl.h"

#include "../Common/Functions.h"

#define CREXPLATECONTROL_CLASSNAME _T("CRexPlateCtrl")

#define ID_MENU_TOGGLE_VEHICLE_TYPE_NAME	9700

// CRexPlateCtrl

IMPLEMENT_DYNAMIC(CRexPlateCtrl, CWnd)

CRexPlateCtrl::CRexPlateCtrl()
{
	if( !RegisterWindowClass() )
		return;

	m_crBack		= RGB( 32, 32, 32 );
	m_bTransparent	= false;
	m_sFaceName		= "RexPlate";

	m_crPlateText	= WHITE_TEXT;
	m_bUseStandardPlateTextColor = true;
	m_bUseStandardPlateBackColor = true;

	m_nSunken		= 2;

	memset( &m_PlateInfo, 0, sizeof( PLATE_INFO ) );

	LOGFONT		lf;
	
	memset( &lf, 0, sizeof(LOGFONT) );


	//번호판 첫번째 라인 폰트
	lf.lfHeight=16;
	lf.lfEscapement=0;
	lf.lfOrientation=0;
	lf.lfWeight=FW_BOLD;
	lf.lfItalic=0;
	lf.lfUnderline=0;
	lf.lfStrikeOut=0;
	lf.lfCharSet=DEFAULT_CHARSET;
	lf.lfOutPrecision=OUT_DEFAULT_PRECIS;
	lf.lfClipPrecision=CLIP_DEFAULT_PRECIS;
	lf.lfQuality=ANTIALIASED_QUALITY;
	lf.lfPitchAndFamily=VARIABLE_PITCH | FF_ROMAN;
	_tcscpy( lf.lfFaceName, TEXT( m_sFaceName ) );
	m_Font0.CreateFontIndirect( &lf );


	//번호판 두번째 라인 폰트
	lf.lfHeight=30;
	lf.lfWidth=0;
	lf.lfWeight=FW_NORMAL;
	m_Font1.CreateFontIndirect( &lf );


	//부가정보 출력 관련
	m_bShowInfo = true;
	lf.lfHeight = 10;
	_tcscpy( lf.lfFaceName, TEXT( "굴림" ) );
	m_Font_Info.CreateFontIndirect( &lf );

	m_uInfoFormat = DT_LEFT | DT_SINGLELINE | DT_BOTTOM;
}

CRexPlateCtrl::~CRexPlateCtrl()
{
	m_Font0.DeleteObject();
	m_Font1.DeleteObject();
	m_Font_Info.DeleteObject();
}


BEGIN_MESSAGE_MAP(CRexPlateCtrl, CWnd)
	ON_WM_PAINT()
	ON_WM_ERASEBKGND()
	ON_WM_LBUTTONDBLCLK()
	ON_WM_RBUTTONDBLCLK()
END_MESSAGE_MAP()

BOOL CRexPlateCtrl::RegisterWindowClass()
{
	WNDCLASS wndcls;
	HINSTANCE hInst = AfxGetInstanceHandle();

	if (!(::GetClassInfo(hInst, CREXPLATECONTROL_CLASSNAME, &wndcls)))
	{
		// otherwise we need to register a new class
		CBrush	brush;
		brush.CreateSolidBrush( m_PlateInfo.crBack );

		wndcls.style            = CS_DBLCLKS | CS_HREDRAW | CS_VREDRAW ;
		wndcls.lpfnWndProc      = ::DefWindowProc;
		wndcls.cbClsExtra       = wndcls.cbWndExtra = 0;
		wndcls.hInstance        = hInst;
		wndcls.hIcon            = NULL;
		wndcls.hCursor          = AfxGetApp()->LoadStandardCursor(IDC_ARROW);
		wndcls.hbrBackground    = (HBRUSH) brush.GetSafeHandle();
		wndcls.lpszMenuName     = NULL;
		wndcls.lpszClassName    = CREXPLATECONTROL_CLASSNAME;

		if (!AfxRegisterClass(&wndcls))
		{
			AfxThrowResourceException();
			return FALSE;
		}
	}

	return TRUE;
}


// CRexPlateCtrl 메시지 처리기입니다.




void CRexPlateCtrl::OnPaint()
{
	CPaintDC dc(this); // device context for painting
	// TODO: 여기에 메시지 처리기 코드를 추가합니다.
	// 그리기 메시지에 대해서는 CWnd::OnPaint()을(를) 호출하지 마십시오.

	CRect	rClient;	//컨트롤의 좌표값
	CRect	rPlate;		//번호판의 좌표값
	int		w, h;		//번호판의 좌표값

	GetClientRect( rClient );

	//m_nPlateType이 음수이면 이는 번호판을 실사화 시키지 않고
	//단순히 텍스트 정보만 표시하기 위한 기능으로 번호판 가독성 실험 등 별도 목적에 의해 추가된 코드이다.

	if ( !m_bTransparent )
	{
		if ( m_PlateInfo.nType < PLATE_GREEN && strlen( m_PlateInfo.sPlate ) > 0 )
			dc.FillSolidRect( rClient, WHITE );
		else
			dc.FillSolidRect( rClient, m_crBack );
	}


	//컨트롤의 테두리를 m_nSunken 수치만큼 들어가 보이도록 그려준다.
	if ( m_nSunken > 0 )
	{
		int		i;
		CPen	PenGray( PS_SOLID, 1, ::GetSysColor( COLOR_BTNSHADOW ) );
		CPen	PenWhite( PS_SOLID, 1, RGB( 255, 255, 255 ) );
		CPen*	pOldPen = (CPen*)dc.SelectObject( &PenGray );

		for ( i = 0; i < m_nSunken; i++ )
		{
			//top
			dc.MoveTo( rClient.left  + i, rClient.top    + i );
			dc.LineTo( rClient.right - i, rClient.top    + i );
			//left
			dc.MoveTo( rClient.left  + i, rClient.top    + i );
			dc.LineTo( rClient.left  + i, rClient.bottom - i );
		}

		dc.SelectObject( pOldPen );
		PenGray.DeleteObject();
		pOldPen = (CPen*)dc.SelectObject( &PenWhite );

		for ( i = 0; i < m_nSunken; i++ )
		{
			//bottom
			dc.MoveTo( rClient.left  + i, rClient.bottom - i );
			dc.LineTo( rClient.right - i, rClient.bottom - i );
			//right
			dc.MoveTo( rClient.right - i, rClient.top    + i );
			dc.LineTo( rClient.right - i, rClient.bottom - i + 1 );
		}

		dc.SelectObject( pOldPen );
		PenWhite.DeleteObject();
	}

	//번호판 정보가 없으면 리턴.
	if ( strcmp( m_PlateInfo.sPlate, "" ) == 0 )
		return;

	LOGFONT lf;
	CFont*	pOldFont;

	//일반적인 텍스트 출력 형태(검정 바탕에 흰색 글씨)
	//실사 번호판 형태가 아닌 단순 텍스트 출력으로 실사 번호판과의 비교 실험 용도로 디스플레이하기 위한 테스트 코드 블럭이다.
	if ( m_PlateInfo.nType < PLATE_GREEN )
	{
		m_Font0.GetLogFont( &lf );
		m_Font0.DeleteObject();

		strcpy( lf.lfFaceName, "굴림" );
		lf.lfHeight = 30;
		//lf.lfWeight = FW_BOLD;

		m_Font0.CreateFontIndirectA( &lf );
		
		pOldFont = (CFont*)dc.SelectObject( &m_Font0 );
		
		dc.SetTextColor( BLACK );
		dc.DrawText( m_sLPR0 + m_sLPR1, rClient, DT_CENTER | DT_SINGLELINE | DT_VCENTER );

		dc.SelectObject( pOldFont );

		return;
	}
	


	dc.SetTextColor( m_crPlateText );
	dc.SetBkMode( TRANSPARENT );

	//번호판 크기와 동일한 모양으로 rPlate를 줄여준다.
	//최소 마진은 높이에 비례하게 준다.
	double dMargin = (double)rClient.Height() / 20.0;
	rClient.DeflateRect( m_nSunken + dMargin, m_nSunken + dMargin );
	rPlate = rClient;

	//컨트롤의 가로, 세로 비율에 따라 번호판의 크기가 결정된다.
	if ( m_dPlateRatio > ( (double)rClient.Width() / (double)rClient.Height() ) )
	{
		h = (double)rPlate.Width() / m_dPlateRatio;
		rPlate.top		+= ( rClient.Height() - h ) / 2.0;
		rPlate.bottom	= rPlate.top + h;
	}
	else
	{
		w = (double)rPlate.Height() * m_dPlateRatio;
		rPlate.left		+= ( rClient.Width() - w ) / 2.0;
		rPlate.right	= rPlate.left + w;
	}



	COLORREF	crPlateBack = m_PlateInfo.crBack;

	//표준 배경색 사용모드라면 번호판의 배경색을 표준색으로 칠해준다.
	//표준 배경색 사용모드가 아닌 경우에는 인식된 번호판 배경색 그대로 사용한다.
	if ( m_bUseStandardPlateBackColor )
	{
		switch ( m_PlateInfo.nType )
		{
			case PLATE_GREEN	:	crPlateBack = GREEN_BACK;
									break;
			case PLATE_WHITE	:	crPlateBack = WHITE_BACK;
									break;
			case PLATE_YELLOW	:	crPlateBack = YELLOW_BACK;
									break;
			case PLATE_ORANGE	:	crPlateBack = ORANGE_BACK;
									break;
		}
	}


	CPen	PenBack( PS_SOLID, 1, crPlateBack );					//번호판 테두리 선의 색
	CPen	PenWhite( PS_SOLID, 1, RGB( 232, 232, 232 ) );			//번호판 테두리 흰색 선 (녹색,주황)
	CPen	PenBlack( PS_SOLID, 1, RGB( 64, 64, 64 ) );				//번호판 테두리 검정 선 (흰색,노랑
	CPen*	pOldPen;

	CBrush	BrushBack( crPlateBack );								//번호판 바탕색
	CBrush	BrushBoltFore( GetLightenColor( crPlateBack, 30 ) );	//번호판 볼트 전경색
	CBrush	BrushBoltBack( GetDarkenColor( crPlateBack, 100 ) );	//번호판 볼트 그림자 색
	CBrush*	pOldBrush = (CBrush*)dc.SelectObject( &BrushBack );

	//번호판 테두리 선 색상 결정
	if ( m_PlateInfo.nType == PLATE_GREEN || m_PlateInfo.nType == PLATE_ORANGE )
		pOldPen = (CPen*)dc.SelectObject( &PenWhite );
	else
		pOldPen = (CPen*)dc.SelectObject( &PenBlack );

	//배경색으로 번호판 영역 그리기
	dc.RoundRect( rPlate, CPoint( 10, 10 ) );

	//한번 더 안쪽 테두리 그리기
	rPlate.DeflateRect( 2, 2 );
	dc.RoundRect( rPlate, CPoint( 10, 10 ) );

	dc.SelectObject( pOldPen );
	dc.SelectObject( pOldBrush );


	//좌우 볼트 그리기
	int		nSize;						//볼트 지름
	double	dSize;						//직사각 번호판의 좌우 볼트 간격 배율. 클수록 두 볼트의 거리는 벌어짐.
	CSize	szOffset = CSize( 2, 2 );	//볼트와 그림자 옵셋
	double	dPosition = 3.75;			//볼트의 상단 위치 배율(크면 위로, 작으면 아래로 위치함)

	pOldPen = (CPen*)dc.SelectObject( &PenBack );
	pOldBrush = (CBrush*)dc.SelectObject( &BrushBoltBack );

	if ( m_PlateInfo.nType == PLATE_GREEN  || m_PlateInfo.nType == PLATE_WHITE )
		dSize = 5.0;
	else
		dSize = 7.0;

	if ( m_PlateInfo.nSize == PLATE_WIDE )
	{
		nSize = (double)rPlate.Height() / 7.0;

		dc.Ellipse( rPlate.left + nSize, rPlate.CenterPoint().y - nSize, rPlate.left + nSize * 3, rPlate.CenterPoint().y + nSize );
		dc.Ellipse( rPlate.right - nSize, rPlate.CenterPoint().y - nSize, rPlate.right - nSize * 3, rPlate.CenterPoint().y + nSize );

		dc.SelectObject( pOldBrush );
		pOldBrush = (CBrush*)dc.SelectObject( &BrushBoltFore );

		dc.Ellipse( rPlate.left + nSize - szOffset.cx, rPlate.CenterPoint().y - nSize - szOffset.cy, rPlate.left + nSize * 3 - szOffset.cx, rPlate.CenterPoint().y + nSize - szOffset.cy );
		dc.Ellipse( rPlate.right - nSize - szOffset.cx, rPlate.CenterPoint().y - nSize - szOffset.cy, rPlate.right - nSize * 3 - szOffset.cx, rPlate.CenterPoint().y + nSize - szOffset.cy );
	}
	else
	{
		nSize = (double)rPlate.Height() / 14.0;

		dc.Ellipse( rPlate.left + rPlate.Width() / dSize - nSize, rPlate.top + (double)rPlate.Height() / dPosition - nSize * 2, rPlate.left + rPlate.Width() / dSize + nSize, rPlate.top + (double)rPlate.Height() / dPosition );
		dc.Ellipse( rPlate.right - rPlate.Width() / dSize - nSize, rPlate.top + (double)rPlate.Height() / dPosition - nSize * 2, rPlate.right - rPlate.Width() / dSize + nSize, rPlate.top + (double)rPlate.Height() / dPosition );

		dc.SelectObject( pOldBrush );
		pOldBrush = (CBrush*)dc.SelectObject( &BrushBoltFore );

		dc.Ellipse( rPlate.left + rPlate.Width() / dSize - nSize - szOffset.cx, rPlate.top + (double)rPlate.Height() / dPosition - nSize * 2 - szOffset.cy, rPlate.left + rPlate.Width() / dSize + nSize - szOffset.cx, rPlate.top + (double)rPlate.Height() / dPosition - szOffset.cy );
		dc.Ellipse( rPlate.right - rPlate.Width() / dSize - nSize - szOffset.cx, rPlate.top + (double)rPlate.Height() / dPosition - nSize * 2 - szOffset.cy, rPlate.right - rPlate.Width() / dSize + nSize - szOffset.cx, rPlate.top + (double)rPlate.Height() / dPosition - szOffset.cy );
	}
	//좌우 볼트 그리기 끝.

	dc.SelectObject( pOldPen );
	dc.SelectObject( pOldBrush );

	PenBack.DeleteObject();
	PenWhite.DeleteObject();
	PenBlack.DeleteObject();
	BrushBack.DeleteObject();
	BrushBoltFore.DeleteObject();
	BrushBoltBack.DeleteObject();



	//텍스트 표시
	CRect	r0, r1;		//상, 하 라인, 한줄 번호판인 경우는 r1에 할당. m_sLPR0, m_sLPR1도 마찬가지
	
	r0 = r1 = rPlate;

	//한 줄인 경우
	if ( m_sLPR0 == "" )
	{
		//DT_VCENTER를 해도 수직센터에 안써지므로 약간 내려준다. 분모가 작으면 더 아래로 써진다.
		r1.OffsetRect( 0, (double)rPlate.Height() / 18.0 );

		m_Font1.GetLogFont( &lf );
		m_Font1.DeleteObject();

		if ( m_PlateInfo.nSize == PLATE_WIDE )
		{
			//흰색 와이드 번호판인 경우
			if ( m_PlateInfo.nType == PLATE_WHITE )
			{
				lf.lfHeight = (double)r1.Width() / 5.3;
				lf.lfWidth = (double)lf.lfHeight * 0.5;
			}
			//노랑 와이드
			else
			{
				lf.lfHeight = (double)r1.Width() / 5.5;
				lf.lfWidth = (double)lf.lfHeight * 0.5;
			}
		}
		else
		{
			//흰색 사각형 번호판은 흰색 와이드에 비해 글자가 세로로 좀 더 길쭉하다.
			if ( m_PlateInfo.nType == PLATE_WHITE )
			{
				r1.OffsetRect( 0, (double)rPlate.Height() / 12.0 );
				lf.lfHeight = (double)r1.Width() / 3.90;	//크기가 결정된다.
				lf.lfWidth = (double)lf.lfHeight * 0.48;	//장평이 결정된다.	
			}
			//여기에 해당되는 번호판은 없다.
			else
			{
				lf.lfHeight = (double)r1.Width() / 4.4;
				lf.lfWidth = (double)lf.lfHeight * 50;
			}
		}

		m_Font1.CreateFontIndirectA( &lf );
		pOldFont = (CFont*)dc.SelectObject( &m_Font1 );

		//노랑색 와이드인 경우 지역명이 세로로 표시된다.
		if ( m_PlateInfo.nType == PLATE_YELLOW && m_PlateInfo.nSize == PLATE_WIDE && m_sRegion != "" )
		{
			CRect r;

			m_Font0.GetLogFont( &lf );
			m_Font0.DeleteObject();

			lf.lfHeight = (double)r1.Height() * 0.50;
			lf.lfWidth = (double)lf.lfHeight * 0.80;

			dc.DrawText( m_sRegion.Left(2) + m_sLPR1, &r, DT_CENTER | DT_SINGLELINE | DT_VCENTER | DT_CALCRECT );
			r.left = r1.left + ( r1.Width() - r.Width() ) / 2.0 + (double)lf.lfWidth * 1.7;
			r.right = r1.right;
			r.top = r1.top;
			r.bottom = r1.bottom;
			dc.DrawText( m_sLPR1, r, DT_LEFT | DT_SINGLELINE | DT_VCENTER );

			dc.SelectObject( pOldFont );


			m_Font0.CreateFontIndirect( &lf );
			pOldFont = (CFont*)dc.SelectObject( &m_Font0 );

			dc.SetTextAlign( TA_LEFT | TA_BOTTOM );
			dc.TextOut( r.left - (double)lf.lfWidth * 1.5, r1.CenterPoint().y + (double)lf.lfHeight * 0.1, m_sRegion.Left(2) );
			dc.SetTextAlign( TA_LEFT | TA_TOP );
			dc.TextOut( r.left - (double)lf.lfWidth * 1.5, r1.CenterPoint().y - (double)lf.lfHeight * 0.1, m_sRegion.Mid(2, 2) );

			//dc.FillSolidRect( r, RGB(255,0,0) );
		}
		else
		{
			dc.DrawText( m_sLPR1, r1, DT_CENTER | DT_SINGLELINE | DT_VCENTER );
		}

		dc.SelectObject( pOldFont );
	}
	//두 줄 번호판의 경우
	else
	{
		//주황
		if ( m_PlateInfo.nType == PLATE_ORANGE )
		{
			r0.bottom	= r0.top + (double)rPlate.Height() * 6.0 / 10.0;
			r1.top		= r1.bottom - (double)rPlate.Height() * 6.2 / 10.0;

			r0.OffsetRect( 0, (double)rPlate.Height() / -15.0 );
			//r1.OffsetRect( 0, (double)rPlate.Height() / 15.0 );
		}
		//녹색, 노랑
		else
		{
			r0.bottom	= r0.top + (double)rPlate.Height() * 1.2 / 3.0;
			r1.top		= r1.bottom - (double)rPlate.Height() * 2.3 / 3.0;

			//r0.OffsetRect( 0, (double)rPlate.Height() / 15.0 );
			r1.OffsetRect( 0, (double)rPlate.Height() / 15.0 );
		}

		//상단 글자 크기 설정
		m_Font0.GetLogFont( &lf );
		m_Font0.DeleteObject();

		if ( m_PlateInfo.nType == PLATE_GREEN )
		{
			lf.lfHeight = (double)r0.Height() * 0.80;
			lf.lfWidth = (double)lf.lfHeight * 0.70;
		}
		else if ( m_PlateInfo.nType == PLATE_ORANGE )
		{
			lf.lfHeight = (double)r0.Height() * 0.70;
			lf.lfWidth = (double)lf.lfHeight * 0.80;
		}
		else	//노랑
		{
			lf.lfHeight = (double)r0.Height() * 0.75;
			lf.lfWidth = (double)lf.lfHeight * 0.70;
		}

		m_Font0.CreateFontIndirectA( &lf );


		//하단 글자 크기 설정
		m_Font1.GetLogFont( &lf );
		m_Font1.DeleteObject();

		if ( m_PlateInfo.nType == PLATE_GREEN )
		{
			if ( m_sLPR1.GetLength() == 4 )
				lf.lfHeight = (double)r1.Height() * 0.95;
			else
				lf.lfHeight = (double)r1.Height() * 0.80;

			lf.lfWidth = (double)lf.lfHeight * 0.55;
		}
		else if ( m_PlateInfo.nType == PLATE_ORANGE )
		{
			lf.lfHeight = (double)r1.Height() * 0.85;
			lf.lfWidth = (double)lf.lfHeight * 0.54;
		}
		else
		{
			lf.lfHeight = (double)r1.Height() * 0.85;
			lf.lfWidth = (double)lf.lfHeight * 0.54;
		}
		
		m_Font1.CreateFontIndirectA( &lf );

		pOldFont = (CFont*)dc.SelectObject( &m_Font0 );

		//노란 번호판 중 두 줄로 되어 있으면 지역명과 분류코드 사이에 공백이 있다.
		if ( m_sLPR0.GetLength() == 6 && m_PlateInfo.nType == PLATE_YELLOW && m_PlateInfo.nSize == PLATE_RECTANGULAR )
			m_sLPR0.Insert( 4, ' ' );
		
		dc.DrawText( m_sLPR0, r0, DT_CENTER | DT_SINGLELINE | DT_VCENTER );
		dc.SelectObject( pOldFont );

		pOldFont = (CFont*)dc.SelectObject( &m_Font1 );

		dc.DrawText( m_sLPR1, r1, DT_CENTER | DT_SINGLELINE | DT_VCENTER );
		dc.SelectObject( pOldFont );
	}


	//부가 정보를 출력해준다.
	if ( m_bShowInfo && m_sInfo != "" )
	{
		pOldFont = (CFont*)dc.SelectObject( &m_Font_Info );

		dc.SetBkMode( TRANSPARENT );

		//rClient.OffsetRect( 0, 4 );
		dc.SetTextColor( RGB(32, 32, 32) );
		dc.DrawText( m_sInfo, rClient, m_uInfoFormat );

		rClient.OffsetRect( -1, -1 );
		dc.SetTextColor( RGB(192, 192, 192) );
		dc.DrawText( m_sInfo, rClient, m_uInfoFormat );

		dc.SelectObject( pOldFont );
	}
}

BOOL CRexPlateCtrl::OnEraseBkgnd(CDC* pDC)
{
	// TODO: 여기에 메시지 처리기 코드를 추가 및/또는 기본값을 호출합니다.
	return false;

	return CWnd::OnEraseBkgnd(pDC);
}

COLORREF CRexPlateCtrl::GetLightenColor(const COLORREF crColor, BYTE byIncreaseVal)
{
	int nRed	= GetRValue(crColor);
	int nGreen	= GetGValue(crColor);
	int nBlue	= GetBValue(crColor);

	nRed += byIncreaseVal;
	if ( nRed > 255 )
		nRed = 255;

	nGreen += byIncreaseVal;
	if ( nGreen > 255 )
		nGreen = 255;

	nBlue += byIncreaseVal;
	if ( nBlue > 255 )
		nBlue = 255;

	return RGB(nRed, nGreen, nBlue);
}

COLORREF CRexPlateCtrl::GetDarkenColor(const COLORREF crColor, BYTE byReduceVal)
{
	int nRed	= GetRValue(crColor);
	int nGreen	= GetGValue(crColor);
	int nBlue	= GetBValue(crColor);

	nRed -= byReduceVal;
	if ( nRed < 0 )
		nRed = 0;

	nGreen -= byReduceVal;
	if ( nGreen < 0 )
		nGreen = 0;

	nBlue -= byReduceVal;
	if ( nBlue < 0 )
		nBlue = 0;

	return RGB(nRed, nGreen, nBlue);
}

void CRexPlateCtrl::SetPlate( PLATE_INFO* PlateInfo )
{
	ZeroMemory( &m_PlateInfo, sizeof( PLATE_INFO ) );

	//번호판 정보가 없는 경우
	if ( PlateInfo == NULL )
	{
		Invalidate();
		return;
	}

	memcpy( &m_PlateInfo, PlateInfo, sizeof( PLATE_INFO ) );
	m_sInfo.Format( "%.2f", m_PlateInfo.fReliability );

	CString sLPR = m_PlateInfo.sPlate;
	sLPR.Replace( " ", "" );

	if ( m_bUseStandardPlateTextColor )
	{
		switch ( m_PlateInfo.nType )
		{
			case PLATE_GREEN	:	m_crPlateText = GREEN_TEXT;
									break;
			case PLATE_WHITE	:	m_crPlateText = WHITE_TEXT;
									break;
			case PLATE_YELLOW	:	m_crPlateText = YELLOW_TEXT;
									break;
			case PLATE_ORANGE	:	m_crPlateText = ORANGE_TEXT;
									break;
		}
	}

	//녹색, 노랑, 주황은 무조건 두줄이다.
	//흰색은 무조건 한줄이고 크기는 와이드 또는 직사각이다.

	//흰색
	if ( m_PlateInfo.nType == PLATE_WHITE )
	{
		m_sLPR0 = "";
		m_sLPR1 = sLPR;

		if ( m_PlateInfo.nSize == PLATE_WIDE )
		{
			//각 번호판 스타일에 따라 m_dPlateRatio를 정해주는데
			//이는 가로 세로 비율을 나타낸다. 크면 클수록 가로로 긴 모양이 된다.
			//흰색 와이드 번호판의 경우 그 비율은 4.7272 정도인데 5.2로 준 이유는
			//위에서 아래로 내려보며 촬영하므로 약간 세로가 줄어든 모양으로 나타나기 때문이다.
			m_dPlateRatio = 4.727272;	//원래 4.7272
			//끝 4자리 숫자 앞에 공백을 넣어 보기 쉽도록 한다. 흰색번호판은 2칸을 넣는다. 노란색은 1칸
			m_sLPR1.Insert( m_sLPR1.GetLength() - 4, "  " );
		}
		else
		{
			m_dPlateRatio = 2.0;	//원래 2.16 대형 자가용은 2.0
		}
	}
	//주황
	else if ( m_PlateInfo.nType == PLATE_ORANGE )
	{
		//지역명까지 모두 인식한 경우
		if ( sLPR.GetLength() >= 11 )
		{
			m_sLPR0 = sLPR.Left( 4 );
			m_sLPR0.Insert( 2, "   " );
			m_sLPR1 = sLPR.Right( sLPR.GetLength() - 4 );
		}
		//지역명을 인식하지 못해서 00x0000 형태로만 인식한 경우
		else
		{
			m_sLPR0 = " ";
			m_sLPR1 = sLPR;
		}

		m_dPlateRatio = 2.35;		//원래 2.35
		m_PlateInfo.nSize = PLATE_RECTANGULAR;
	}
	//녹색
	else if ( m_PlateInfo.nType == PLATE_GREEN )
	{
		//전북23다1234
		if ( sLPR.GetLength() == 12 )
		{
			m_sLPR1 = sLPR.Right( 6 );
			m_sLPR0 = sLPR.Left( sLPR.GetLength() - m_sLPR1.GetLength() );
		}
		//전북7차1234, 전북34X1234
		else if ( sLPR.GetLength() == 11 )
		{
			if ( IsAlphaNumeric( sLPR.Mid( 5, 1 ) ) )	//전북34X1234
			{
				m_sLPR1 = sLPR.Right( 5 );
				m_sLPR0 = sLPR.Left( sLPR.GetLength() - m_sLPR1.GetLength() );
			}
			else//if ( IsAlphaNumeric( sLPR.Mid( 5, 1 ) ) )	전북7차1234
			{
				m_sLPR1 = sLPR.Right( 6 );
				m_sLPR0 = sLPR.Left( sLPR.GetLength() - m_sLPR1.GetLength() );
			}
		}
		else if ( sLPR.GetLength() > 4 )
		{
			m_sLPR1 = sLPR.Right( 4 );
			m_sLPR0 = sLPR.Left( sLPR.GetLength() - m_sLPR1.GetLength() );
		}
		else
		{
			m_sLPR0 = " ";
			m_sLPR1 = sLPR;
		}

		m_dPlateRatio = 2.0;		//원래 2.0 소형 1.97, 대형 2.0
		m_PlateInfo.nSize = PLATE_RECTANGULAR;
	}
	else if ( m_PlateInfo.nType == PLATE_YELLOW )
	{
		if ( m_PlateInfo.nSize == PLATE_WIDE )
		{
			m_sLPR0 = "";

			if ( sLPR.GetLength() == 12 )
			{
				m_sRegion = sLPR.Left( 4 );	//경기, 대구 등 지역명
				m_sLPR1 = sLPR.Right( 8 );
			}
			else
			{
				m_sRegion = "";
				m_sLPR1 = sLPR;
			}

			//끝 4자리 숫자 앞에 공백을 넣어 보기 쉽도록 한다.
			m_sLPR1.Insert( m_sLPR1.GetLength() - 4, " " );
		}
		else
		{
			m_sLPR1 = sLPR.Right( 6 );
			m_sLPR0 = sLPR.Left( sLPR.GetLength() - 6 );
		}
		
		if ( m_PlateInfo.nSize == PLATE_WIDE )
			m_dPlateRatio = 4.7272;	//원래 4.7272
		else
			m_dPlateRatio = 2.0;	//원래 2.0 소형 1.97, 대형 2.0
	}

	strcpy( m_PlateInfo.sVehicleTypeName, m_PlateInfo.GetVehicleTypeName() );
	strcpy( PlateInfo->sVehicleTypeName, m_PlateInfo.sVehicleTypeName );

	Invalidate( false );
}

void CRexPlateCtrl::OnLButtonDblClk(UINT nFlags, CPoint point)
{
	// TODO: 여기에 메시지 처리기 코드를 추가 및/또는 기본값을 호출합니다.

/*
	MessageBeep(0);
	m_bUseStandardPlateTextColor = !m_bUseStandardPlateTextColor;
	Invalidate();
*/

	CWnd::OnLButtonDblClk(nFlags, point);
}

void CRexPlateCtrl::OnRButtonDblClk(UINT nFlags, CPoint point)
{
	// TODO: 여기에 메시지 처리기 코드를 추가 및/또는 기본값을 호출합니다.

	MessageBeep(0);
	m_bUseStandardPlateBackColor = !m_bUseStandardPlateBackColor;
	Invalidate();

	CWnd::OnLButtonDblClk(nFlags, point);
}

void CRexPlateCtrl::ShowInfoText( bool bShow /*= true*/ )
{
	m_bShowInfo = bShow;
	Invalidate();
}

void CRexPlateCtrl::SetInfoTextSize( int nSize )
{
	LOGFONT	lf;

	m_Font_Info.GetLogFont( &lf );
	m_Font_Info.DeleteObject();

	lf.lfHeight = nSize;
	m_Font_Info.CreateFontIndirect( &lf );
}

void CRexPlateCtrl::SetInfoTextAlign( UINT uFormat )
{
	m_uInfoFormat = uFormat;
}

