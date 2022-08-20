// RexPlateCtrl.cpp : ���� �����Դϴ�.
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


	//��ȣ�� ù��° ���� ��Ʈ
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


	//��ȣ�� �ι�° ���� ��Ʈ
	lf.lfHeight=30;
	lf.lfWidth=0;
	lf.lfWeight=FW_NORMAL;
	m_Font1.CreateFontIndirect( &lf );


	//�ΰ����� ��� ����
	m_bShowInfo = true;
	lf.lfHeight = 10;
	_tcscpy( lf.lfFaceName, TEXT( "����" ) );
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


// CRexPlateCtrl �޽��� ó�����Դϴ�.




void CRexPlateCtrl::OnPaint()
{
	CPaintDC dc(this); // device context for painting
	// TODO: ���⿡ �޽��� ó���� �ڵ带 �߰��մϴ�.
	// �׸��� �޽����� ���ؼ��� CWnd::OnPaint()��(��) ȣ������ ���ʽÿ�.

	CRect	rClient;	//��Ʈ���� ��ǥ��
	CRect	rPlate;		//��ȣ���� ��ǥ��
	int		w, h;		//��ȣ���� ��ǥ��

	GetClientRect( rClient );

	//m_nPlateType�� �����̸� �̴� ��ȣ���� �ǻ�ȭ ��Ű�� �ʰ�
	//�ܼ��� �ؽ�Ʈ ������ ǥ���ϱ� ���� ������� ��ȣ�� ������ ���� �� ���� ������ ���� �߰��� �ڵ��̴�.

	if ( !m_bTransparent )
	{
		if ( m_PlateInfo.nType < PLATE_GREEN && strlen( m_PlateInfo.sPlate ) > 0 )
			dc.FillSolidRect( rClient, WHITE );
		else
			dc.FillSolidRect( rClient, m_crBack );
	}


	//��Ʈ���� �׵θ��� m_nSunken ��ġ��ŭ �� ���̵��� �׷��ش�.
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

	//��ȣ�� ������ ������ ����.
	if ( strcmp( m_PlateInfo.sPlate, "" ) == 0 )
		return;

	LOGFONT lf;
	CFont*	pOldFont;

	//�Ϲ����� �ؽ�Ʈ ��� ����(���� ������ ��� �۾�)
	//�ǻ� ��ȣ�� ���°� �ƴ� �ܼ� �ؽ�Ʈ ������� �ǻ� ��ȣ�ǰ��� �� ���� �뵵�� ���÷����ϱ� ���� �׽�Ʈ �ڵ� ���̴�.
	if ( m_PlateInfo.nType < PLATE_GREEN )
	{
		m_Font0.GetLogFont( &lf );
		m_Font0.DeleteObject();

		strcpy( lf.lfFaceName, "����" );
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

	//��ȣ�� ũ��� ������ ������� rPlate�� �ٿ��ش�.
	//�ּ� ������ ���̿� ����ϰ� �ش�.
	double dMargin = (double)rClient.Height() / 20.0;
	rClient.DeflateRect( m_nSunken + dMargin, m_nSunken + dMargin );
	rPlate = rClient;

	//��Ʈ���� ����, ���� ������ ���� ��ȣ���� ũ�Ⱑ �����ȴ�.
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

	//ǥ�� ���� ������� ��ȣ���� ������ ǥ�ػ����� ĥ���ش�.
	//ǥ�� ���� ����尡 �ƴ� ��쿡�� �νĵ� ��ȣ�� ���� �״�� ����Ѵ�.
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


	CPen	PenBack( PS_SOLID, 1, crPlateBack );					//��ȣ�� �׵θ� ���� ��
	CPen	PenWhite( PS_SOLID, 1, RGB( 232, 232, 232 ) );			//��ȣ�� �׵θ� ��� �� (���,��Ȳ)
	CPen	PenBlack( PS_SOLID, 1, RGB( 64, 64, 64 ) );				//��ȣ�� �׵θ� ���� �� (���,���
	CPen*	pOldPen;

	CBrush	BrushBack( crPlateBack );								//��ȣ�� ������
	CBrush	BrushBoltFore( GetLightenColor( crPlateBack, 30 ) );	//��ȣ�� ��Ʈ �����
	CBrush	BrushBoltBack( GetDarkenColor( crPlateBack, 100 ) );	//��ȣ�� ��Ʈ �׸��� ��
	CBrush*	pOldBrush = (CBrush*)dc.SelectObject( &BrushBack );

	//��ȣ�� �׵θ� �� ���� ����
	if ( m_PlateInfo.nType == PLATE_GREEN || m_PlateInfo.nType == PLATE_ORANGE )
		pOldPen = (CPen*)dc.SelectObject( &PenWhite );
	else
		pOldPen = (CPen*)dc.SelectObject( &PenBlack );

	//�������� ��ȣ�� ���� �׸���
	dc.RoundRect( rPlate, CPoint( 10, 10 ) );

	//�ѹ� �� ���� �׵θ� �׸���
	rPlate.DeflateRect( 2, 2 );
	dc.RoundRect( rPlate, CPoint( 10, 10 ) );

	dc.SelectObject( pOldPen );
	dc.SelectObject( pOldBrush );


	//�¿� ��Ʈ �׸���
	int		nSize;						//��Ʈ ����
	double	dSize;						//���簢 ��ȣ���� �¿� ��Ʈ ���� ����. Ŭ���� �� ��Ʈ�� �Ÿ��� ������.
	CSize	szOffset = CSize( 2, 2 );	//��Ʈ�� �׸��� �ɼ�
	double	dPosition = 3.75;			//��Ʈ�� ��� ��ġ ����(ũ�� ����, ������ �Ʒ��� ��ġ��)

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
	//�¿� ��Ʈ �׸��� ��.

	dc.SelectObject( pOldPen );
	dc.SelectObject( pOldBrush );

	PenBack.DeleteObject();
	PenWhite.DeleteObject();
	PenBlack.DeleteObject();
	BrushBack.DeleteObject();
	BrushBoltFore.DeleteObject();
	BrushBoltBack.DeleteObject();



	//�ؽ�Ʈ ǥ��
	CRect	r0, r1;		//��, �� ����, ���� ��ȣ���� ���� r1�� �Ҵ�. m_sLPR0, m_sLPR1�� ��������
	
	r0 = r1 = rPlate;

	//�� ���� ���
	if ( m_sLPR0 == "" )
	{
		//DT_VCENTER�� �ص� �������Ϳ� �Ƚ����Ƿ� �ణ �����ش�. �и� ������ �� �Ʒ��� ������.
		r1.OffsetRect( 0, (double)rPlate.Height() / 18.0 );

		m_Font1.GetLogFont( &lf );
		m_Font1.DeleteObject();

		if ( m_PlateInfo.nSize == PLATE_WIDE )
		{
			//��� ���̵� ��ȣ���� ���
			if ( m_PlateInfo.nType == PLATE_WHITE )
			{
				lf.lfHeight = (double)r1.Width() / 5.3;
				lf.lfWidth = (double)lf.lfHeight * 0.5;
			}
			//��� ���̵�
			else
			{
				lf.lfHeight = (double)r1.Width() / 5.5;
				lf.lfWidth = (double)lf.lfHeight * 0.5;
			}
		}
		else
		{
			//��� �簢�� ��ȣ���� ��� ���̵忡 ���� ���ڰ� ���η� �� �� �����ϴ�.
			if ( m_PlateInfo.nType == PLATE_WHITE )
			{
				r1.OffsetRect( 0, (double)rPlate.Height() / 12.0 );
				lf.lfHeight = (double)r1.Width() / 3.90;	//ũ�Ⱑ �����ȴ�.
				lf.lfWidth = (double)lf.lfHeight * 0.48;	//������ �����ȴ�.	
			}
			//���⿡ �ش�Ǵ� ��ȣ���� ����.
			else
			{
				lf.lfHeight = (double)r1.Width() / 4.4;
				lf.lfWidth = (double)lf.lfHeight * 50;
			}
		}

		m_Font1.CreateFontIndirectA( &lf );
		pOldFont = (CFont*)dc.SelectObject( &m_Font1 );

		//����� ���̵��� ��� �������� ���η� ǥ�õȴ�.
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
	//�� �� ��ȣ���� ���
	else
	{
		//��Ȳ
		if ( m_PlateInfo.nType == PLATE_ORANGE )
		{
			r0.bottom	= r0.top + (double)rPlate.Height() * 6.0 / 10.0;
			r1.top		= r1.bottom - (double)rPlate.Height() * 6.2 / 10.0;

			r0.OffsetRect( 0, (double)rPlate.Height() / -15.0 );
			//r1.OffsetRect( 0, (double)rPlate.Height() / 15.0 );
		}
		//���, ���
		else
		{
			r0.bottom	= r0.top + (double)rPlate.Height() * 1.2 / 3.0;
			r1.top		= r1.bottom - (double)rPlate.Height() * 2.3 / 3.0;

			//r0.OffsetRect( 0, (double)rPlate.Height() / 15.0 );
			r1.OffsetRect( 0, (double)rPlate.Height() / 15.0 );
		}

		//��� ���� ũ�� ����
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
		else	//���
		{
			lf.lfHeight = (double)r0.Height() * 0.75;
			lf.lfWidth = (double)lf.lfHeight * 0.70;
		}

		m_Font0.CreateFontIndirectA( &lf );


		//�ϴ� ���� ũ�� ����
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

		//��� ��ȣ�� �� �� �ٷ� �Ǿ� ������ ������� �з��ڵ� ���̿� ������ �ִ�.
		if ( m_sLPR0.GetLength() == 6 && m_PlateInfo.nType == PLATE_YELLOW && m_PlateInfo.nSize == PLATE_RECTANGULAR )
			m_sLPR0.Insert( 4, ' ' );
		
		dc.DrawText( m_sLPR0, r0, DT_CENTER | DT_SINGLELINE | DT_VCENTER );
		dc.SelectObject( pOldFont );

		pOldFont = (CFont*)dc.SelectObject( &m_Font1 );

		dc.DrawText( m_sLPR1, r1, DT_CENTER | DT_SINGLELINE | DT_VCENTER );
		dc.SelectObject( pOldFont );
	}


	//�ΰ� ������ ������ش�.
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
	// TODO: ���⿡ �޽��� ó���� �ڵ带 �߰� ��/�Ǵ� �⺻���� ȣ���մϴ�.
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

	//��ȣ�� ������ ���� ���
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

	//���, ���, ��Ȳ�� ������ �����̴�.
	//����� ������ �����̰� ũ��� ���̵� �Ǵ� ���簢�̴�.

	//���
	if ( m_PlateInfo.nType == PLATE_WHITE )
	{
		m_sLPR0 = "";
		m_sLPR1 = sLPR;

		if ( m_PlateInfo.nSize == PLATE_WIDE )
		{
			//�� ��ȣ�� ��Ÿ�Ͽ� ���� m_dPlateRatio�� �����ִµ�
			//�̴� ���� ���� ������ ��Ÿ����. ũ�� Ŭ���� ���η� �� ����� �ȴ�.
			//��� ���̵� ��ȣ���� ��� �� ������ 4.7272 �����ε� 5.2�� �� ������
			//������ �Ʒ��� �������� �Կ��ϹǷ� �ణ ���ΰ� �پ�� ������� ��Ÿ���� �����̴�.
			m_dPlateRatio = 4.727272;	//���� 4.7272
			//�� 4�ڸ� ���� �տ� ������ �־� ���� ������ �Ѵ�. �����ȣ���� 2ĭ�� �ִ´�. ������� 1ĭ
			m_sLPR1.Insert( m_sLPR1.GetLength() - 4, "  " );
		}
		else
		{
			m_dPlateRatio = 2.0;	//���� 2.16 ���� �ڰ����� 2.0
		}
	}
	//��Ȳ
	else if ( m_PlateInfo.nType == PLATE_ORANGE )
	{
		//��������� ��� �ν��� ���
		if ( sLPR.GetLength() >= 11 )
		{
			m_sLPR0 = sLPR.Left( 4 );
			m_sLPR0.Insert( 2, "   " );
			m_sLPR1 = sLPR.Right( sLPR.GetLength() - 4 );
		}
		//�������� �ν����� ���ؼ� 00x0000 ���·θ� �ν��� ���
		else
		{
			m_sLPR0 = " ";
			m_sLPR1 = sLPR;
		}

		m_dPlateRatio = 2.35;		//���� 2.35
		m_PlateInfo.nSize = PLATE_RECTANGULAR;
	}
	//���
	else if ( m_PlateInfo.nType == PLATE_GREEN )
	{
		//����23��1234
		if ( sLPR.GetLength() == 12 )
		{
			m_sLPR1 = sLPR.Right( 6 );
			m_sLPR0 = sLPR.Left( sLPR.GetLength() - m_sLPR1.GetLength() );
		}
		//����7��1234, ����34X1234
		else if ( sLPR.GetLength() == 11 )
		{
			if ( IsAlphaNumeric( sLPR.Mid( 5, 1 ) ) )	//����34X1234
			{
				m_sLPR1 = sLPR.Right( 5 );
				m_sLPR0 = sLPR.Left( sLPR.GetLength() - m_sLPR1.GetLength() );
			}
			else//if ( IsAlphaNumeric( sLPR.Mid( 5, 1 ) ) )	����7��1234
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

		m_dPlateRatio = 2.0;		//���� 2.0 ���� 1.97, ���� 2.0
		m_PlateInfo.nSize = PLATE_RECTANGULAR;
	}
	else if ( m_PlateInfo.nType == PLATE_YELLOW )
	{
		if ( m_PlateInfo.nSize == PLATE_WIDE )
		{
			m_sLPR0 = "";

			if ( sLPR.GetLength() == 12 )
			{
				m_sRegion = sLPR.Left( 4 );	//���, �뱸 �� ������
				m_sLPR1 = sLPR.Right( 8 );
			}
			else
			{
				m_sRegion = "";
				m_sLPR1 = sLPR;
			}

			//�� 4�ڸ� ���� �տ� ������ �־� ���� ������ �Ѵ�.
			m_sLPR1.Insert( m_sLPR1.GetLength() - 4, " " );
		}
		else
		{
			m_sLPR1 = sLPR.Right( 6 );
			m_sLPR0 = sLPR.Left( sLPR.GetLength() - 6 );
		}
		
		if ( m_PlateInfo.nSize == PLATE_WIDE )
			m_dPlateRatio = 4.7272;	//���� 4.7272
		else
			m_dPlateRatio = 2.0;	//���� 2.0 ���� 1.97, ���� 2.0
	}

	strcpy( m_PlateInfo.sVehicleTypeName, m_PlateInfo.GetVehicleTypeName() );
	strcpy( PlateInfo->sVehicleTypeName, m_PlateInfo.sVehicleTypeName );

	Invalidate( false );
}

void CRexPlateCtrl::OnLButtonDblClk(UINT nFlags, CPoint point)
{
	// TODO: ���⿡ �޽��� ó���� �ڵ带 �߰� ��/�Ǵ� �⺻���� ȣ���մϴ�.

/*
	MessageBeep(0);
	m_bUseStandardPlateTextColor = !m_bUseStandardPlateTextColor;
	Invalidate();
*/

	CWnd::OnLButtonDblClk(nFlags, point);
}

void CRexPlateCtrl::OnRButtonDblClk(UINT nFlags, CPoint point)
{
	// TODO: ���⿡ �޽��� ó���� �ڵ带 �߰� ��/�Ǵ� �⺻���� ȣ���մϴ�.

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

