// GraphCtrl.cpp : 구현 파일입니다.
//

//#include "stdafx.h"
#include "GraphCtrl.h"

#define TIMER_START_DEMO	0

// CGraphCtrl

IMPLEMENT_DYNAMIC(CGraphCtrl, CWnd)

	
BEGIN_MESSAGE_MAP(CGraphCtrl, CWnd)
	ON_WM_CREATE()
	ON_WM_PAINT()
	ON_WM_ERASEBKGND()
	ON_WM_MOUSEMOVE()
	ON_WM_NCMOUSEMOVE()
	ON_WM_LBUTTONDOWN()
	ON_WM_LBUTTONUP()
	ON_WM_LBUTTONDBLCLK()
	ON_WM_RBUTTONDOWN()
	ON_WM_RBUTTONUP()
	ON_WM_RBUTTONDBLCLK()
	ON_WM_SIZE()
	ON_WM_TIMER()
	ON_COMMAND_RANGE(ID_MENU_GRAPH_STYLE_LINE, ID_MENU_RESET, OnPopupMenu)
	ON_WM_DROPFILES()
	//동적 생성된 컨트롤에서는 툴팁이 동작하지 않는다. 아래 함수는 이를 가능하게는 해주나 부드럽게 연속 툴팁이 표시되지는 않는다.
	//ON_NOTIFY_EX(TTN_NEEDTEXTA, 0, OnTTNNeedText)	
	//ON_NOTIFY_EX(TTN_NEEDTEXTW, 0, OnTTNNeedText)
END_MESSAGE_MAP()

CGraphCtrl::CGraphCtrl()
{
	if( !RegisterWindowClass() )
		return;

	this;

	int	i;

	m_nGraphStyle		= GRAPH_STYLE_LINE;
	m_nGraphID			= 0;
	m_bPause			= false;

	m_bDrawBorder		= false;

	m_sGraphTitle		= _T("");
	m_bGraphTitleBold	= true;
	m_nGraphTitleSize	= 16;
	m_crGraphTitle		= GRAPH_TITLE_COLOR;

	for ( i = 0; i < 2; i++ )
	{
		m_sInfoText[i] = "";
		m_bInfoText[i] = false;
	}

	m_rMargin			= CRect( 10, 10, 10, 10 );

	m_nAxisTitleFontSize= 18;
	m_nAxisFontSize		= 12;

	m_sXAxisTitle		= _T("");
	m_crXAxisTitle		= GRAPH_AXIS_TITLE_COLOR;
	m_nXAxisLevel		= 10;
	m_nXAxisPrecision	= 0;
	m_sXAxisPrecision.Format( _T("%%0.%df"), m_nXAxisPrecision );
	m_nFixedLeftMargin	= -1;
	m_nFixedRightMargin	= -1;

	m_nMarkX			= 0;
	m_dZoneX1			= -1;
	m_dZoneX2			= -1;

	m_sYAxisTitle		= _T("");
	m_crYAxisTitle		= GRAPH_AXIS_TITLE_COLOR;
	m_nDataNumOnScreen	= 0;
	m_dYMinValue		= 0.0;
	m_dYMaxValue		= 100.0;
	m_dYMinimumValue	= std::numeric_limits<double>::infinity();
	m_dYMaximumValue	= -(std::numeric_limits<double>::infinity());
	m_dYValueMargin		= 0.1;
	m_nYAxisLevel		= 10;
	m_nYAxisPrecision	= 0;
	m_sYAxisPrecision.Format( _T("%%0.%df"), m_nYAxisPrecision );
	m_bShowLegends		= false;

	m_bShowDataLabel	= false;

	m_bDrawGraphBack	= true;
	m_bDrawChartBack	= true;
	m_bShowXAxisLabel	= true;	//false when flow style.
	m_bShowYAxisLabel	= true;
	m_bDrawGridX		= true;
	m_bDrawGridY		= true;

	m_crBackGraph		= RGB(  48,  48,  48 );
	m_crBackChart		= RGB(  64,  64,  64 );
	m_crText			= RGB( 191, 191, 191 );
	m_crAxis			= RGB(  89,  89,  89 );
	m_crGrid			= RGB(  89,  89,  89 );

	m_bShowMarkX		= true;
	m_crMarkX			= RGB( 0, 255, 0 );

	m_bUsePopupMenu		= true;
	m_bShowMousePosLine	= false;
	m_bMouseIsInChart	= false;

	//툴팁 설정
	m_pToolTip			= NULL;
	m_bShowToolTip		= false;

	m_bLButtonDown		= false;

	m_bStartDemo		= false;

	m_pLineData = (CGraphLineData**) new CGraphLineData*[GRAPH_LINE_MAX];

	for ( i = 0; i < GRAPH_LINE_MAX; i++ )
		m_pLineData[i] = NULL;

	m_vtData.resize( GRAPH_LINE_MAX + 1 );
	m_vtData.reserve( GRAPH_LINE_MAX + 1 );
}

CGraphCtrl::~CGraphCtrl()
{
	ReleaseAll();
	delete [] m_pLineData;

	if ( m_pToolTip )
	{
		m_pToolTip->DestroyWindow();
		delete m_pToolTip;
	}
}

void CGraphCtrl::ReleaseAll()
{
	m_XAxis.RemoveData();
	m_nMarkX = 0;

	for ( int i = 0; i < GRAPH_LINE_MAX; i++ )
		ReleaseLineData( i );
}

void CGraphCtrl::ReleaseLineData( int nIndex )
{
	//우선 내부 데이터를 저장한 메모리를 해제하고
	if ( m_pLineData[nIndex] == NULL )
		return;

	if ( m_pLineData[nIndex]->pData && m_pLineData[nIndex]->nData )
	{
		delete [] m_pLineData[nIndex]->pData;
		m_pLineData[nIndex]->pData = NULL;
		m_pLineData[nIndex]->nData = 0;
	}

	//라인 데이터도 지워준다.
	//주의!
	//CGraphLineData* pLine = m_pLineData[nIndex]와 같이 pLine으로 받아서
	//pLine을 지우고 초기화해도 m_pLineData[nIndex]값이 초기화 되지 않는다.
	//pLine을 사용하지 않고 직접 멤버 변수를 사용한다.
	if ( m_pLineData[nIndex] )
	{
		delete m_pLineData[nIndex];
		m_pLineData[nIndex] = NULL;
	}
}

void CGraphCtrl::SetGraphStyle( int nStyle )
{
	if ( m_nGraphStyle == GRAPH_STYLE_FLOW )
		return;

	m_nGraphStyle = nStyle;
	Invalidate();
}

void CGraphCtrl::DeleteAllLineData()
{
	for ( int i = 0; i < GRAPH_LINE_MAX; i++ )
		DeleteLineData( i );

	Invalidate();
}

CGraphLineData* CGraphCtrl::DeleteLineData( int nIndex )
{
	if ( nIndex < 0 || nIndex >= GRAPH_LINE_MAX )
		return NULL;

	CGraphLineData*	pLine = (CGraphLineData*)m_pLineData[nIndex];

	if ( pLine == NULL )
		return NULL;

	if ( pLine->pData )
	{
		delete [] pLine->pData;
		pLine->pData = NULL;
		pLine->nData = 0;
	}

	if ( m_nGraphStyle == GRAPH_STYLE_FLOW &&
		 GetValidLineCount() == 0 )
		m_nDataNumOnScreen = 0;

	GetYMinMaxValue();

	return pLine;
}

BOOL CGraphCtrl::OnTTNNeedText(UINT id, NMHDR* pTTTStruct,  LRESULT* pResult)
{
    TOOLTIPTEXT* pTTT = (TOOLTIPTEXT*)pTTTStruct;
	//strncpy_s(pTTT->lpszText, 80, m_sToolTip, _TRUNCATE);

    return TRUE;
}

// CGraphCtrl 메시지 처리기입니다.
int CGraphCtrl::OnCreate (LPCREATESTRUCT lpCreateStruct) 
{
	//AfxMessageBox( "create" );
	return 0;
}


BOOL CGraphCtrl::RegisterWindowClass()
{
	WNDCLASS wndcls;
	HINSTANCE hInst = AfxGetInstanceHandle();

	if (!(::GetClassInfo(hInst, _T("CGraphCtrl"), &wndcls)))
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
		wndcls.lpszClassName    = _T("CGraphCtrl");

		if (!AfxRegisterClass(&wndcls))
		{
			AfxThrowResourceException();
			return FALSE;
		}
	}

	return TRUE;
}

//x축에 표시할 데이터 수를 세팅한다. 0이하이면 그래프 영역의 width가 xLevel이 된다.
void CGraphCtrl::SetXAxisLevel( int nLevel )
{
	if ( nLevel <= 0 )
	{
		CRect	rc;
		GetClientRect( rc );
		nLevel = rc.Width();
	}

	if ( m_XAxis.nData > 0 && nLevel >= m_XAxis.nData )
		m_nXAxisLevel = m_XAxis.nData - 1;
	else
		m_nXAxisLevel = nLevel;

	Invalidate();
}

void CGraphCtrl::SetXAxis( int* pData, int nData )
{
	ASSERT( m_nGraphStyle != GRAPH_STYLE_FLOW );

	if ( nData <= 0 )
		return;

	double* pDoubleData = new double[nData];

	//pData가 존재하면 그 데이터를 사용하고 NULL이라면
	//기본 x축값인 0 ~ nData-1 값을 이용한다.
	for ( int i = 0; i < nData; i++ )
	{
		if ( pData )
			pDoubleData[i] = (double)( pData[i] );
		else
			pDoubleData[i] = (double)i;
	}

	SetXAxis( pDoubleData, nData );
	delete [] pDoubleData;
}

void CGraphCtrl::SetXAxis( double* pData, int nData )
{
	ASSERT( m_nGraphStyle != GRAPH_STYLE_FLOW );

	if ( nData <= 0 )
		return;

	if ( m_XAxis.pData != NULL )
		m_XAxis.RemoveData();

	m_XAxis.pData = new double[nData];

	//pData가 존재하면 그 데이터를 사용하고 NULL이라면
	//기본 x축값인 0 ~ nData-1 값을 이용한다.
	if ( pData )
		memcpy( m_XAxis.pData, pData, sizeof(double) * nData );
	else
	{
		for ( int i = 0; i < nData; i++ )
			m_XAxis.pData[i] = i;
	}

	m_XAxis.nData = nData;
	m_nMarkX = 0;

	if ( m_nXAxisLevel >= m_XAxis.nData )
		m_nXAxisLevel = m_XAxis.nData - 1;

	Invalidate();
}

void CGraphCtrl::AddMarkX( double x )
{
	//x축 데이터가 없다면 리턴.
	if ( m_XAxis.pData == NULL || m_XAxis.nData <= 0 )
		return;

	//마커의 위치가 x축 변위를 벗어난 경우 리턴.
	if ( (x < m_XAxis.pData[0]) || (x > m_XAxis.pData[m_XAxis.nData-1]) )
		return;

	//마커의 인덱스가 유효범위가 아니면 리턴.
	if ( m_nMarkX >= GRAPH_MARKX_MAX )
		return;

	m_dMarkX[m_nMarkX] = x;

	CPoint pt = GetScreenPosFromX( m_dMarkX[m_nMarkX] );
	CRect r( pt.x - 1, m_rChart.top, pt.x + 1, m_rChart.bottom );
	InvalidateRect( r, false );
	m_nMarkX++;
}

void CGraphCtrl::UpdateMarkX( int idx, double x )
{
	//x축 데이터가 없다면 리턴.
	if ( m_XAxis.pData == NULL || m_XAxis.nData <= 0 )
		return;

	//마커의 위치가 x축 변위를 벗어난 경우 리턴.
	if ( (x < m_XAxis.pData[0]) || (x > m_XAxis.pData[m_XAxis.nData-1]) )
		return;

	//마커의 인덱스가 유효범위가 아니면 리턴.
	if ( idx < 0 || idx >= GRAPH_MARKX_MAX )
		return;

	//마크의 개수가 0이었다면 개수를 증가시켜주고
	//1개 이상이었다면 이전 마크의 위치를 지워주고
	//새로운 위치에 마크를 그려준다.
	CRect r;
	CPoint pt;

	if ( m_nMarkX == 0 )
	{
		m_nMarkX++;
	}
	else
	{
		//erase old markx
		pt = GetScreenPosFromX( m_dMarkX[idx] );
		r = CRect( pt.x - 1, m_rChart.top, pt.x + 1, m_rChart.bottom );
		InvalidateRect( r, false );
	}

	//draw new markx
	m_dMarkX[idx] = x;
	pt = GetScreenPosFromX( m_dMarkX[idx] );
	r = CRect( pt.x - 1, m_rChart.top, pt.x + 1, m_rChart.bottom );
	InvalidateRect( r, false );
}

void CGraphCtrl::SetLineData( int nIndex, int* pData, int nData, bool bRedraw )
{
	ASSERT( m_nGraphStyle != GRAPH_STYLE_FLOW );

	double* pDoubleData = new double[nData];

	for ( int i = 0; i < nData; i++ )
		pDoubleData[i] = (double)( pData[i] );

	SetLineData( nIndex, pDoubleData, nData, bRedraw );
	delete [] pDoubleData;
}

void CGraphCtrl::SetLineData( int nIndex, double* pData, int nData, bool bRedraw )
{
	ASSERT( m_nGraphStyle != GRAPH_STYLE_FLOW );

	if ( nIndex < 0 || nIndex >= GRAPH_LINE_MAX )
		return;

	//라인에 이미 부여된 속성을 유지하기 위해 라인 자체를 release하지 않고
	//데이터 주소와 개수만 리셋한다.
	//새로운 데이터를 저장하도록 수정.
	DeleteLineData( nIndex );

	if ( m_pLineData[nIndex] == NULL )
		m_pLineData[nIndex] = new CGraphLineData();

	m_pLineData[nIndex]->pData = new double[nData];
	m_pLineData[nIndex]->nData = nData;
	
	memcpy( m_pLineData[nIndex]->pData, pData, sizeof(double) * nData );

	//라인데이터가 추가/삭제될때마다 매번 계산해 줘야준다.
	GetYMinMaxValue();

	if ( bRedraw )
		Invalidate();
}

//라인데이터의 특정 값을 원하는 값으로 변경한다.
//라인이 추가되기 전이라면 라인을 추가한 후 데이터를 변경해준다.
void CGraphCtrl::SetLineData( int nIndex, int nX, double dYData, bool bRedraw )
{
	ASSERT( m_nGraphStyle != GRAPH_STYLE_FLOW );

	if ( nIndex < 0 || nIndex >= GRAPH_LINE_MAX )
		return;

	if ( nX < 0 || nX >= m_XAxis.nData )
		return;

	CGraphLineData*	pLineData = GetLineDataPtr( nIndex );

	if ( pLineData->pData == NULL || pLineData->nData <= 0 )
	{
		pLineData->nData = m_XAxis.nData;
		pLineData->pData = new double[m_XAxis.nData];
		for ( int i = 0; i < pLineData->nData; i++ )
			pLineData->pData[i] = -(std::numeric_limits<double>::infinity());
	}

	m_pLineData[nIndex]->pData[nX] = dYData;

	GetYMinMaxValue();

	if ( bRedraw )
		Invalidate();
}

/*
//기존 라인 데이터를 새로운 데이터로 교체한다.
//기존 라인 데이터가 없다면 기본 타입으로 생성해서 사용한다.
void CGraphCtrl::ModifyLineData( int nIndex, int* pNewData, int nData )
{
	ASSERT( m_nGraphStyle != GRAPH_STYLE_FLOW );

	double* pDoubleData = new double[nData];

	for ( int i = 0; i < nData; i++ )
		pDoubleData[i] = (double)( pNewData[i] );

	ModifyLineData( nIndex, pDoubleData, nData );
	delete [] pDoubleData;
}

void CGraphCtrl::ModifyLineData( int nIndex, double* pNewData, int nData )
{
	ASSERT( m_nGraphStyle != GRAPH_STYLE_FLOW );
		
	//기존에 존재하는 라인의 데이터 필드를 삭제한다.
	if ( nIndex < 0 || nIndex >= GRAPH_LINE_MAX )
	{
		AfxMessageBox( "Out of Line index range on CGraphCtrl::ModifyLineData()" );
		return;
	}

	if ( m_pLineData[nIndex] == NULL )
	{
		SetLineData( nIndex, pNewData, nData );
		return;
	}

	if ( m_pLineData[nIndex]->pData )
		delete [] m_pLineData[nIndex]->pData;

	m_pLineData[nIndex]->pData = new double[nData];

	m_pLineData[nIndex]->nData = nData;
	
	for ( int i = 0; i < nData; i++ )
		m_pLineData[nIndex]->pData[i] = pNewData[i];
		//memcpy( m_pLineData[nIndex]->pData, pNewData, sizeof(double) * nData );

	//라인데이터가 추가/삭제될때마다 매번 계산해 줘야준다.
	GetYMinMaxValue();
}
*/

void CGraphCtrl::GetYMinMaxValue()
{
	int		i, j;
	int		start = 0, end;		//search start and end index
	double	d;

	//y변위 최대값을 지정하지 않은 경우 기본 최대값이 100이지만
	//데이터가 하나라도 들어오면 그 데이터가 y변위의 최대값 기준으로 사용된다.
	if ( m_dYMaximumValue == -(std::numeric_limits<double>::infinity()) )
		m_dYMaxValue = m_dYMaximumValue;

	for ( i = 0; i < GRAPH_LINE_MAX; i++ )
	{
		if ( m_pLineData[i] && m_pLineData[i]->pData )
		{
			end = m_pLineData[i]->nData;

			if ( (m_nGraphStyle == GRAPH_STYLE_FLOW) && (m_nDataNumOnScreen > 0) )
				start = m_pLineData[i]->nData - m_nDataNumOnScreen;

			for ( j = start; j < end; j++ )
			{
				d = m_pLineData[i]->pData[j];

				if ( d != std::numeric_limits<double>::infinity() &&
					 d != -(std::numeric_limits<double>::infinity()) &&
					 d < m_dYMinValue )
					m_dYMinValue = d;

				if ( d != std::numeric_limits<double>::infinity() &&
					 d != -(std::numeric_limits<double>::infinity()) &&
					 d > m_dYMaxValue )
					m_dYMaxValue = d;
			}
		}
	}

	//y변위 최소값을 지정한 경우
	if ( m_dYMinimumValue != (std::numeric_limits<double>::infinity()) )
		m_dYMinValue = m_dYMinimumValue;

	//y변위 최대값을 지정한 경우
	if ( m_dYMaximumValue != -(std::numeric_limits<double>::infinity()) )
		m_dYMaxValue = m_dYMaximumValue;

	if ( m_dYMinValue == std::numeric_limits<double>::infinity() &&
		 m_dYMaxValue == -(std::numeric_limits<double>::infinity()) )
	{
		m_dYMinValue = 0.0;
		m_dYMaxValue = 100.0;
	}

	if ( m_dYMaxValue == m_dYMinValue )
	{
		if ( m_dYMaxValue == 0.0 )
			m_dYMaxValue = 100.0;

		m_dYMinValue = 0.0;

		if ( m_dYMaxValue < m_dYMinValue )
		{
			double temp = m_dYMaxValue;
			m_dYMaxValue = m_dYMinValue;
			m_dYMinValue = temp;
		}
	}
}

void CGraphCtrl::SetYValueMargin( double dMargin, bool bInvalidate )
{
	Clamp( dMargin, 0.0, 0.5 );

	m_dYValueMargin = dMargin;
	GetYMinMaxValue();

	if ( bInvalidate )
		Invalidate();
}

void CGraphCtrl::SetYMinimumValue( double dMinimumValue )
{
	m_dYMinValue = m_dYMinimumValue = dMinimumValue;
	int nPrecision = getPrecision( m_dYMinValue );

	if ( nPrecision > m_nYAxisPrecision )
	{
		m_nYAxisPrecision = nPrecision;
		m_sYAxisPrecision.Format( _T("%%0.%df"), m_nYAxisPrecision );
	}

	Invalidate();
}

void CGraphCtrl::SetYMaximumValue( double dMaximumValue )
{ 
	m_dYMaxValue = m_dYMaximumValue = dMaximumValue;

	int nPrecision = getPrecision( m_dYMaxValue );

	if ( nPrecision > m_nYAxisPrecision )
	{
		m_nYAxisPrecision = nPrecision;
		m_sYAxisPrecision.Format( _T("%%0.%df"), m_nYAxisPrecision );
	}

	Invalidate();
}

void CGraphCtrl::SetYRange( double dMin, double dMax )
{
	SetYMinimumValue( dMin );
	SetYMaximumValue( dMax );
}

void CGraphCtrl::ResetYRange()
{
	m_dYMinValue = m_dYMinimumValue = std::numeric_limits<double>::infinity();
	m_dYMaxValue = m_dYMaximumValue = -std::numeric_limits<double>::infinity();
	Invalidate();
}

//기존 라인 데이터에 새로운 값을 추가한다.
void CGraphCtrl::AddLineData( int nIndex, double dValue )
{
	ASSERT( m_nGraphStyle == GRAPH_STYLE_FLOW );

	if ( m_bPause )
		return;

	//기존에 존재하는 라인 인덱스가 아니면 새로 생성해준다.
	if ( nIndex < 0 || nIndex >= GRAPH_LINE_MAX )
	{
		AfxMessageBox(_T("Out of Line index range on CGraphCtrl::AddLineData()") );
		return;
	}

	if ( m_pLineData[nIndex] == NULL )
	{
		m_pLineData[nIndex] = new CGraphLineData();
		m_pLineData[nIndex]->nData = 0;
	}

	if ( m_pLineData[nIndex]->pData == NULL )
		m_pLineData[nIndex]->pData = new double[GRAPH_DATA_MAX];

	//데이터 개수가 GRAPH_DATA_MAX에 도달하면 최근 1/4개를 앞으로 당겨준다.
	if ( m_pLineData[nIndex]->nData >= GRAPH_DATA_MAX - 1 )
	{
		int dQuarter = GRAPH_DATA_MAX / 4.0;
		memcpy( (void*)&(m_pLineData[nIndex]->pData[0]), (void*)&(m_pLineData[nIndex]->pData[GRAPH_DATA_MAX-1-dQuarter]), sizeof(double) * dQuarter );
		m_pLineData[nIndex]->nData = dQuarter;
	}

	m_pLineData[nIndex]->pData[m_pLineData[nIndex]->nData++] = dValue;

	//라인데이터가 추가/삭제될때마다 매번 계산해 줘야준다.
	GetYMinMaxValue();

	Invalidate();
}


void CGraphCtrl::OnPaint()
{
	CPaintDC dc1(this); // device context for painting
	// TODO: 여기에 메시지 처리기 코드를 추가합니다.
	// 그리기 메시지에 대해서는 CWnd::OnPaint()을(를) 호출하지 마십시오.

	int		i, j;
	CRect	rc, r;

	GetClientRect( rc );
	CMemoryDC	dc(&dc1, &rc);// , true );

	CPen	penAxis( PS_SOLID, 1, m_crAxis );				//축 펜
	CPen	penGrid( PS_SOLID, 1, m_crGrid );				//그리드 펜
	CPen*	pOldPen = (CPen*)dc.SelectStockObject( NULL_PEN );

	CBrush	brBackGraph( m_crBackGraph );
	CBrush	brBackChart( m_crBackChart );
	CBrush*	pOldBrush = NULL;

	//y축 표시값 문자열 길이에 따라서도 왼쪽 여백이 달라진다.
	CString	str;

	CAutoFont	Font( _T("Arial") );
	int nFontHeight = m_nAxisFontSize;
	Font.SetHeight( nFontHeight );
	Font.SetBold( false );
	CFont *pOldFont = (CFont*)dc.SelectObject( &Font );

	//최대값 데이터를 출력하기 위한 텍스트 너비 계산
	int nTextWidth[2][2];	//x축, y축 각각 최소, 최대값을 출력하는 텍스트 너비를 구한다.
	double dYMinValue = m_dYMinValue;
	double dYMaxValue = m_dYMaxValue;

	str.Format( m_sXAxisPrecision, m_XAxis.GetMin() );
	dc.DrawText( str, &r, DT_CALCRECT );
	nTextWidth[0][0] = r.Width() - 5;
	str.Format( m_sXAxisPrecision, m_XAxis.GetMax() );
	dc.DrawText( str, &r, DT_CALCRECT );
	nTextWidth[0][1] = r.Width() - 5;

	//y축이 정수 데이터이고 동적 범위 표현 방식일 경우
	//miny, maxy는 항상 차트 영역의 top과 bottom에 표시되므로 가독성이 떨어진다.
	//위, 아래에 n% 만큼의 여백 범위를 추가시켜준다.
	if ( m_nYAxisPrecision == 0 )
	{
		if ( m_dYMinimumValue == (std::numeric_limits<double>::infinity()) )
			dYMinValue -= (m_dYMaxValue - m_dYMinValue) * m_dYValueMargin;
		if ( m_dYMaximumValue == -(std::numeric_limits<double>::infinity()) )
			dYMaxValue += (m_dYMaxValue - m_dYMinValue) * m_dYValueMargin;
	}

	str.Format( m_sYAxisPrecision, dYMinValue );
	dc.DrawText( str, &r, DT_CALCRECT );
	nTextWidth[1][0] = r.Width();
	str.Format( m_sYAxisPrecision, dYMaxValue );
	dc.DrawText( str, &r, DT_CALCRECT );
	nTextWidth[1][1] = r.Width();

	if ( m_nFixedLeftMargin < 0 )
	{
		m_rChart.left	= rc.left + m_rMargin.left;
		m_rChart.left	= m_rChart.left + (m_sYAxisTitle.GetLength() ? 15 : 0);
		m_rChart.left	= m_rChart.left + (m_bShowYAxisLabel ? MAX( nTextWidth[1][0], nTextWidth[1][1] ) + 4 : 0 );
	}
	else
	{
		m_rChart.left = rc.left + m_nFixedLeftMargin;
	}

	if ( m_nFixedRightMargin < 0 )
	{
		m_rChart.right	= rc.right - m_rMargin.right;

		if ( m_XAxis.nData > 0 )
			m_rChart.right	= m_rChart.right - MAX( nTextWidth[0][0], nTextWidth[0][1] );
	}
	else
	{
		m_rChart.right = rc.right - m_nFixedRightMargin;
	}

	//그래프 타이틀이 있거나 InfoText를 표시한다면 상단에서 20만큼 공간을 비워주고.
	m_rChart.top	= rc.top + m_rMargin.top + ((m_sGraphTitle.GetLength() || m_bInfoText[0] || m_bInfoText[1]) ? 20 : 0);
	m_rChart.bottom	= rc.bottom - (m_sXAxisTitle.GetLength() ? 15 : 0);
	m_rChart.bottom	= m_rChart.bottom - (m_bShowXAxisLabel ? 10 : 0);
	m_rChart.bottom	= m_rChart.bottom - m_rMargin.bottom;

	//라인 범례 표시 여부에 따른 차트 영역 크기 및 범례표시 영역 크기 계산
	//기본적으로 차트 하단에 표시한다.
	CRect	rLegend[GRAPH_LINE_MAX];
	CRect	rLegends;
	CString sLegends = _T("");
	int		nTotalLegendsWidth = 0;
	int		legend_line_length = 25;		//범례에 표시할 라인 길이
	int		legend_gap = 25;				//범례 항목 간격
	int		nLegendHeight = 0;

	if ( m_bShowLegends )
	{
		for ( i = 0; i < GRAPH_LINE_MAX; i++ )
		{
			CGraphLineData* pLine = m_pLineData[i];

			if ( pLine == NULL || _tcslen(pLine->sTitle) == 0 )
				continue;

			if (_tcslen(pLine->sTitle) > 0 )
			{
				dc.DrawText( pLine->sTitle, rLegend[i], DT_CALCRECT );
			}
			else
			{
				str.Format( _T("Graph %d"), i );
				dc.DrawText( str, rLegend[i], DT_CALCRECT );
			}

			nTotalLegendsWidth += (legend_line_length + rLegend[i].Width() + legend_gap);
			nLegendHeight = rLegend[i].Height();
		}
		nTotalLegendsWidth -= legend_gap;

		if ( nLegendHeight > 0 )
			m_rChart.bottom = m_rChart.bottom - nLegendHeight - 10;
	}


	if ( m_bDrawGraphBack )
	{
		pOldBrush = (CBrush*)dc.SelectObject( &brBackGraph );
		dc.Rectangle( rc );
		dc.SelectObject( pOldBrush );
	}
	 
	if ( m_bDrawChartBack )
	{
		pOldBrush = (CBrush*)dc.SelectObject( &brBackChart );
		dc.Rectangle( m_rChart );
		dc.SelectObject( pOldBrush );
	}

	if ( pOldBrush )
		dc.SelectObject( pOldBrush );

	brBackGraph.DeleteObject();
	brBackChart.DeleteObject();

	if ( m_bDrawBorder )
	{
		DrawSunkenRect( &dc, rc, true, GRAY(96), GRAY(140) );
	}

//y axis label display area test code
	//r.OffsetRect( m_rChart.left - r.right, 0 );
	//DrawRectangle( &dc, r, RGB(255,0,0), NULL_BRUSH );

	//차트 오른쪽 끝 라인을 넘어가지 않도록 1픽셀 왼쪽을 실제 차트의 오른쪽 끝이라 보정한다.
	m_rChart.right -= 1;

	//차트 영역이 너무 작다면 아예 그리지 않도록 한다.
	if ( m_rChart.Width() < 10 || m_rChart.Height() < 10 )
		return;

	dc.SetBkMode( TRANSPARENT );

	//그래프의 타이틀을 표시한다.
	m_rTitleArea = m_rChart;
	m_rTitleArea.bottom = m_rChart.top;
	m_rTitleArea.top = 0;

	dc.SetTextColor( m_crGraphTitle );

	dc.SelectObject( pOldFont );
	Font.SetHeight( m_nGraphTitleSize );
	Font.SetBold( m_bGraphTitleBold );
	pOldFont = (CFont*)dc.SelectObject( &Font );

	dc.DrawText( m_sGraphTitle, m_rTitleArea, DT_CENTER | DT_SINGLELINE | DT_VCENTER );

	//범례 표시
	if ( m_bShowLegends )
	{
		CRect r = m_rChart;
		r.left = m_rChart.left + (m_rChart.Width() - nTotalLegendsWidth) / 2.0;
		r.top = r.bottom;
		r.bottom = rc.bottom;

		for ( i = 0; i < GRAPH_LINE_MAX; i++ )
		{
			CGraphLineData* pLine = m_pLineData[i];

			if (pLine == NULL || _tcslen(pLine->sTitle) == 0)
				continue;

			DrawLine( &dc, r.left, r.CenterPoint().y, r.left + legend_line_length - 5, r.CenterPoint().y, pLine->crLine, pLine->nLineWidth, pLine->nLineStyle );
			r.left += legend_line_length;
			dc.SetTextColor( pLine->crLine );
			if (_tcslen(pLine->sTitle) > 0)
			{
				dc.DrawText(pLine->sTitle, r, DT_LEFT | DT_SINGLELINE | DT_VCENTER);
			}
			else
			{
				str.Format( _T("Graph %d"), i );
				dc.DrawText( str, r, DT_LEFT | DT_SINGLELINE | DT_VCENTER );
			}
			r.left += (rLegend[i].Width() + legend_gap);
		}

		//dc.SetTextColor( WHITE );
		//dc.DrawText( sLegends, r, DT_CENTER | DT_SINGLELINE | DT_VCENTER );
	}


	//infotext를 표시한다.
	dc.SetTextColor( m_crText );

	dc.SelectObject( pOldFont );
	Font.SetHeight( m_nAxisFontSize + 2 );
	Font.SetBold( false );
	pOldFont = (CFont*)dc.SelectObject( &Font );

	if ( m_bInfoText[0] )
		dc.DrawText( m_sInfoText[0], m_rTitleArea, DT_LEFT | DT_SINGLELINE | DT_VCENTER );
	if ( m_bInfoText[1] )
		dc.DrawText( m_sInfoText[1], m_rTitleArea, DT_RIGHT | DT_SINGLELINE | DT_VCENTER );

	dc.SelectObject( pOldFont );
	Font.SetHeight( m_nAxisFontSize );
	pOldFont = (CFont*)dc.SelectObject( &Font );

	//x축 표시 시작
	dc.SelectObject( pOldPen );
	pOldPen = (CPen*)dc.SelectObject( &penGrid );

	//x축 증분. 데이터를 몇 픽셀마다 표시할지...
	//flow style은 5.0으로 고정되고 chart style은 
	double	dUnitX;	//x축에 표시할 x데이터들 사이의 간격(n과 n+1사이의 픽셀 간격) = width / nData
	double	dIdxInc;//x값 index 증분 = (nData - 1) / n등분

	if ( m_nGraphStyle == GRAPH_STYLE_FLOW )
	{
		dUnitX	= (double)m_rChart.Width() / (double)m_nXAxisLevel;
		//flow graph style에서 dIdxInc는 1.0으로 설정해야 맞고 단지 그래프가 그려질때는 dUnitX에 0.x값을 곱해서
		//그래프가 그려지는 간격(속도)를 조정할 수 있다.
		dIdxInc = 1.0;
	}
	else
	{
		dUnitX	= (double)m_rChart.Width() / (double)(m_XAxis.nData - 1);
		//if ( dUnitX < 1.0 ) dUnitX = 1.0;
		dIdxInc	= (double)(m_XAxis.nData - 1) / (double)m_nXAxisLevel;
	}

	//Zone이 있으면 Zone을 먼저 그려준다.
	if ( ( m_dZoneX1 >= 0 ) && ( m_dZoneX1 < m_dZoneX2 ) )
	{
		CRect	rZone( m_rChart.left + m_dZoneX1 * dUnitX, m_rChart.top + 1, m_rChart.left + m_dZoneX2 * dUnitX, m_rChart.bottom - 1 );
		CPen	penZone( PS_SOLID, 1, RGB(128,96,96) );//GetColor( m_crBackChart, 16 ) );
		CBrush	brZone( RGB(128,96,96) );//GetColor( m_crBackChart, 16 ) );

		pOldPen = (CPen*)dc.SelectObject( &penZone );
		pOldBrush = (CBrush*)dc.SelectObject( &brZone );

		dc.Rectangle( rZone );

		dc.SelectObject( pOldPen );
		dc.SelectObject( pOldBrush );

		penZone.DeleteObject();
		brZone.DeleteObject();
	}

	dc.SetTextAlign( TA_CENTER | TA_TOP );

	//x축 표시
	for ( i = 0; i <= m_nXAxisLevel; i ++ )
	{
		//주눈금과 보조눈금을 그린다.
		if ( (m_nGraphStyle != GRAPH_STYLE_FLOW) )
		{
			//dc.MoveTo( m_rChart.left + i * dUnitX * dIdxInc, m_rChart.bottom + 0 );
			//dc.LineTo( m_rChart.left + i * dUnitX * dIdxInc, m_rChart.bottom + 5 );
		}

		if ( m_bDrawGridX )
		{
			dc.MoveTo( m_rChart.left + i * dUnitX * dIdxInc, m_rChart.top + 1 );
			dc.LineTo( m_rChart.left + i * dUnitX * dIdxInc, m_rChart.bottom - 1 );
		}
		
		//x축 레이블을 표시한다.
		if ( m_bShowXAxisLabel && m_XAxis.pData != NULL )
		{
			str.Format( m_sXAxisPrecision, ( i < m_nXAxisLevel ) ? m_XAxis.pData[(int)((double)i*dIdxInc)] : m_XAxis.pData[m_XAxis.nData-1]  );

			//x축 맨 마지막 레이블 길이가 3보다 크고 센터정렬해서 TextOut하면 차트여백에 따라 레이블이 잘릴 경우가 있다.
			//마지막은
			//if ( i == m_nXAxisLevel && str.GetLength() > 3 )
			//	dc.SetTextAlign( TA_RIGHT | TA_TOP );
			dc.TextOut( m_rChart.left + i * dUnitX * dIdxInc + 1, m_rChart.bottom + 3, str );
		}
	}

	//y축 표시
	dc.SetTextAlign( TA_RIGHT | TA_TOP );

	double dUnitY;	//y축의 1.0값에 해당하는 픽셀 수 = height / (max-min)
	double dDivY;	//(max-min) / level
	int nYAxisLevel = m_nYAxisLevel;

	dUnitY	= (double)m_rChart.Height() / (dYMaxValue - dYMinValue);

	//y축이 정수단위이고 y등분의 수가 maxy - miny보다 크면 y좌표들은
	//동일한 정수값이 중복표현되어 나타난다. nYAxisLevel은 maxy - miny를 넘지 않아야 한다.
	if ( m_nYAxisPrecision == 0 && (nYAxisLevel > dYMaxValue - dYMinValue ) )
		nYAxisLevel = dYMaxValue - dYMinValue;

	dDivY	= (dYMaxValue - dYMinValue) / (double)nYAxisLevel;

/*
	//y 최대값과 차트 height를 고려하여 y의 적절한 등분을 찾는다.
	while ( true )
	{
		nDivide = (dYMaxValue - dYMinValue) / dYUnit;
	
		if ( ((double)m_rChart.Height() / (double)nDivide) < m_nAxisFontSize )
			dYUnit++;
		else
			break;
	}
*/
	//TRACE( "dUnit = %f\nnDivide=%d\ndYMaxValue = %f\n", dYUnit, nDivide, dYMaxValue );

	//y축의 주눈금 크기대로 레이블과 그리드 라인을 그려준다.
	for ( i = 0; i <= nYAxisLevel; i++ )
	{
		if ( m_rChart.bottom - i * dUnitY * dDivY < m_rChart.top - 5 )
			break;

		if ( m_bDrawGridY )
		{
			dc.MoveTo( m_rChart.left, m_rChart.bottom - i * dUnitY * dDivY );
			dc.LineTo( m_rChart.right, m_rChart.bottom - i * dUnitY * dDivY );
		}

		//y축 중간 눈금값 표시
		if ( m_bShowYAxisLabel )
		{
			str.Format( m_sYAxisPrecision, dYMinValue + i * dDivY );
			dc.TextOut( m_rChart.left - 3, m_rChart.bottom - i * dUnitY * dDivY - m_nAxisFontSize / 2.0, str );
		}
	}

	dc.SelectObject( pOldPen );
	penGrid.DeleteObject();

	//차트의 x, y축을 grid를 그린 다음인 여기에서 그려줘야 grid에 의해 가려지지 않는다.
	//단, (0,0)이 축의 위치이나 x 또는 y 변위내에 0값이 없다면 (0,0)의 자리에 그려준다.
	pOldPen = (CPen*)dc.SelectObject( &penAxis );
	double	xmin = 0, xmax = m_rChart.Width();
	double	ymin = 0, ymax = dYMaxValue;
	double	cx = 0, cy = 0;
	
	if ( m_nGraphStyle != GRAPH_STYLE_FLOW )
	{
		//cx = (0.0 - m_XAxis.pData[0]) * (double)m_rChart.Width() / (m_XAxis.pData[m_XAxis.nData-1] - m_XAxis.pData[0]);
		//cy = (0.0 - m_Axis.pData[0]) * (double)m_rChart.Width() / (m_XAxis.pData[m_XAxis.nData-1] - m_XAxis.pData[0]);
	}

	dc.MoveTo( m_rChart.left, m_rChart.bottom - cy);
	dc.LineTo( m_rChart.right + 1, m_rChart.bottom - cy);
	dc.MoveTo( m_rChart.left + cx, m_rChart.bottom );
	dc.LineTo( m_rChart.left + cx, m_rChart.top - 1 );

	dc.SelectObject( pOldPen );
	penAxis.DeleteObject();


	//x축 이름 표시
	dc.SetTextColor( m_crXAxisTitle );
	dc.SelectObject( pOldFont );
	Font.SetHeight( m_nAxisTitleFontSize );
	Font.SetBold( false );
	pOldFont = (CFont*)dc.SelectObject( &Font );

	r			= m_rChart;
	r.bottom	= rc.bottom;
	r.top		= m_rChart.bottom;
	//dc.DrawText( /*m_sXAsisTitle*/"0123456789 0123456789 0123456789", r, DT_CENTER | DT_SINGLELINE | DT_VCENTER );
	//DrawRectangle( &dc, r, RGB(255,0,255), NULL_BRUSH );
	dc.SetTextAlign( TA_CENTER | TA_BOTTOM );
	dc.TextOut( m_rChart.left + m_rChart.Width() / 2.0, rc.bottom - 4, m_sXAxisTitle );

	//y축 이름 표시
	dc.SetTextColor( m_crYAxisTitle );
	dc.SetTextAlign( TA_CENTER | TA_BOTTOM );
	dc.SelectObject( pOldFont );
	Font.SetEscapement( 900 );
	pOldFont = (CFont*)dc.SelectObject( &Font );
	dc.TextOut( rc.left + 22, m_rChart.top + m_rChart.Height() / 2.0, m_sYAxisTitle );

	//범례 및 데이터 라인 그리기
	dc.SetTextAlign( TA_LEFT | TA_TOP );
	dc.SelectObject( pOldFont );
	Font.SetHeight( 14 );
	Font.SetEscapement( 0 );
	pOldFont = (CFont*)dc.SelectObject( &Font );

	CRgn	rgn_line, rgn_other;
	CRect	rChart = m_rChart;
	LOGBRUSH lb;

	rChart.bottom += 1;
	rgn_line.CreateRectRgnIndirect( &rChart );
	rgn_other.CreateRectRgnIndirect( &rc );
	dc.SelectClipRgn( &rgn_other );

	lb.lbStyle = BS_SOLID;

	dc.SetTextAlign( TA_CENTER | TA_BOTTOM );

	//라인 데이터들을 그래프로 디스플레이한다.
	for ( i = 0; i < GRAPH_LINE_MAX; i++ )
	{
		CGraphLineData* pLine = m_pLineData[i];

		if ( (pLine == NULL) ||
			 ((m_nGraphStyle != GRAPH_STYLE_FLOW) && (pLine->pData == NULL )) ||
			 (pLine->nData == 0 ) )
			continue;

		lb.lbColor = pLine->crLine;

		dc.SetTextColor( pLine->crDataLabel );
		Font.SetHeight( pLine->nDataLabelSize );
		pOldFont = (CFont*)dc.SelectObject( &Font );

		CPen	penLine( PS_GEOMETRIC | pLine->nLineStyle, pLine->nLineWidth, &lb );
		CPen	penMarker( PS_SOLID, 1, pLine->crMarker );
		CBrush	brMarker( pLine->crMarker );

		pOldPen = (CPen*)dc.SelectObject( &penLine );

		//if ( m_nGraphStyle == GRAPH_STYLE_LINE )
			pOldBrush = (CBrush*)dc.SelectObject( &brMarker );
		//else
			//pOldBrush = (CBrush*)dc.SelectStockObject( NULL_BRUSH );

		//TRACE( "data count = %d\n", pLine->arData.GetCount() );

		//그려지는 데이터는 x축 개수 만큼만 그려진다.
		int		nDataCount;
		int		nBarWidth = pLine->nBarWidth / 2;
		int		nLabelBottomMargin = (m_nGraphStyle == GRAPH_STYLE_LINE ? -6 : -4);
		double	x1, x2, y1, y2;
		double	dy0, dy1;		//그래프에 그려질 y좌표
		double	value0, value1;	//실제 y값
		CRect	rMarker;

		if ( m_nGraphStyle == GRAPH_STYLE_FLOW )
			nDataCount = pLine->nData;
		else
			nDataCount = m_XAxis.nData;

		//끝 데이터부터 역으로 차트의 오른쪽에서 왼쪽으로 그려준다. (작업관리자 그래프처럼)
		if ( m_nGraphStyle == GRAPH_STYLE_FLOW )
		{
			m_nDataNumOnScreen = nDataCount;

			for ( j = nDataCount - 1; j > 0 ; j-- )
			{
				value0 = pLine->pData[j-0];
				value1 = pLine->pData[j-1];

				if ( (value0 == DBL_MAX) || (value1 == DBL_MAX) )
					continue;

				dy0 = value0 - dYMinValue;
				dy1 = value1 - dYMinValue;

				x1 = m_rChart.right - (double)(nDataCount - 1 - j - 0) * dUnitX * 0.1;
				y1 = m_rChart.bottom - 1 - dy0 * dUnitY;
				x2 = m_rChart.right - (double)(nDataCount - 1 - j + 1) * dUnitX * 0.1;
				y2 = m_rChart.bottom - 1 - dy1 * dUnitY;

				if ( value0 == (std::numeric_limits<double>::infinity()) || value0 == -(std::numeric_limits<double>::infinity()) ||
					 value1 == (std::numeric_limits<double>::infinity()) || value1 == -(std::numeric_limits<double>::infinity()) )
				{
					if ( value0 == (std::numeric_limits<double>::infinity()) || value0 == -(std::numeric_limits<double>::infinity()) )
					{
						rMarker = CRect( x2 - pLine->nMarkerSize,
										 y2 - pLine->nMarkerSize,
										 x2 + pLine->nMarkerSize,
										 y2 + pLine->nMarkerSize );
						DrawMarker( &dc, rMarker, &penMarker, pLine->nMarkerShape, pLine->nMarkerWidth, pLine->crMarker, m_crBackChart );

						if ( pLine->bShowDataLabel )
						{
							str.Format( m_sYAxisPrecision, pLine->pData[j-1] );
							dc.TextOut( x2, y2 + nLabelBottomMargin, str );
						}
					}
					else if ( value1 == (std::numeric_limits<double>::infinity()) || value0 == -(std::numeric_limits<double>::infinity()) )
					{
						rMarker = CRect( x1 - pLine->nMarkerSize,
										 y1 - pLine->nMarkerSize,
										 x1 + pLine->nMarkerSize,
										 y1 + pLine->nMarkerSize );

						DrawMarker( &dc, rMarker, &penMarker, pLine->nMarkerShape, pLine->nMarkerWidth, pLine->crMarker, m_crBackChart );

						if ( pLine->bShowDataLabel )
						{
							str.Format( m_sYAxisPrecision, pLine->pData[j-0] );
							dc.TextOut( x1, y1 + nLabelBottomMargin, str );
						}
					}

					continue;
				}

				//차트의 left에 도달하면 더 이상 그려줄 필요가 없다.
				if ( x2 < m_rChart.left )
				{

					m_nDataNumOnScreen = nDataCount - j;
					break;
				}

				dc.SelectClipRgn( &rgn_line );
				dc.MoveTo( x1, y1 );
				dc.LineTo( x2, y2 );
				dc.SelectClipRgn( &rgn_other );

				//해당 Y값 데이터에 마커(표식)를 그려준다.
				//if ( pLine->nMarkerShape > GRAPH_MARKER_NONE )
				{
					rMarker = CRect( x1 - pLine->nMarkerSize,
									 y1 - pLine->nMarkerSize,
									 x1 + pLine->nMarkerSize,
									 y1 + pLine->nMarkerSize );

					DrawMarker( &dc, rMarker, &penMarker, pLine->nMarkerShape, pLine->nMarkerWidth, pLine->crMarker, m_crBackChart );

					if ( pLine->bShowDataLabel )
					{
						str.Format( m_sYAxisPrecision, pLine->pData[j-0] );
						dc.TextOut( x1, y1 + nLabelBottomMargin, str );
					}

					//(n-2)와 (n-1)을 라인으로 그려주고 끝나므로 맨 마지막 (n-1)번째 항목에는 마크 표시가 안된다.
					//따라서 반복문 맨 마지막에는 (n-1)위치에도 마커를 그려주고 끝나야 한다.
					if ( j == 1 )
					{
						rMarker = CRect( x2 - pLine->nMarkerSize,
										 y2 - pLine->nMarkerSize,
										 x2 + pLine->nMarkerSize,
										 y2 + pLine->nMarkerSize );

						DrawMarker( &dc, rMarker, &penMarker, pLine->nMarkerShape, pLine->nMarkerWidth, pLine->crMarker, m_crBackChart );

					if ( pLine->bShowDataLabel )
					{
						str.Format( m_sYAxisPrecision, pLine->pData[j-1] );
						dc.TextOut( x2, y2 + nLabelBottomMargin, str );
					}
					}
				}
			}
		}
		//라인, 바 그래프인 경우
		else
		{
			//x축의 간격이 1픽셀보다 적을 경우 모든 x마다 그려줄 필요는 없다.
			//하지만 정작 속도차이는 거의 없고
			//차트의 width에 따라 라인이 제대로 표시되지 않는 부작용이 발생한다. 1로 고정하자.
			int step = 1;
			//if ( nDataCount > m_rChart.Width() )
				//step = (double)nDataCount / m_rChart.Width();

			//TRACE( "nDataCount = %6d, dUnitX = %6.3f, step = %3d\n", nDataCount, dUnitX, step );
			for ( j = 0; j < nDataCount - step; j += step )
			{
				value0 = pLine->pData[j+0];
				value1 = pLine->pData[j+step];
				dy0 = value0 - dYMinValue;
				dy1 = value1 - dYMinValue;

				//for test for break point
				//if ( j == 0 )
					//j = j;
				x1 = m_rChart.left + (double)(j + 0) * dUnitX;
				y1 = m_rChart.bottom - dy0 * dUnitY;
				x2 = m_rChart.left + (double)(j + step) * dUnitX;
				y2 = m_rChart.bottom - dy1 * dUnitY;

				//라인데이터값 중 무한대값이 존재하는 경우는 그리지 않으나 그 전 또는 그 후의 마커는 표시해줘야 한다.
				if ( value0 == (std::numeric_limits<double>::infinity()) || value0 == -(std::numeric_limits<double>::infinity()) ||
					 value1 == (std::numeric_limits<double>::infinity()) || value1 == -(std::numeric_limits<double>::infinity()) ||
					 (!m_bDrawOutOfRange && ((value0 < m_dYMinValue) || (value0 > m_dYMaxValue))) ||
					 (!m_bDrawOutOfRange && ((value1 < m_dYMinValue) || (value1 > m_dYMaxValue))) )
				{
					if ( value0 == (std::numeric_limits<double>::infinity()) || value0 == -(std::numeric_limits<double>::infinity()) ||
						 (!m_bDrawOutOfRange && ((value0 < m_dYMinValue) || (value0 > m_dYMaxValue))) )
					{
						if ( !m_bDrawOutOfRange && ((value0 < m_dYMinValue) || (value0 > m_dYMaxValue)) )
							continue;
						if ( m_nGraphStyle == GRAPH_STYLE_LINE )
						{
							rMarker = CRect( x2 - pLine->nMarkerSize,
											 y2 - pLine->nMarkerSize,
											 x2 + pLine->nMarkerSize+1,
											 y2 + pLine->nMarkerSize+1 );
							DrawMarker( &dc, rMarker, &penMarker, pLine->nMarkerShape, pLine->nMarkerWidth, pLine->crMarker, m_crBackChart );
						}

						if ( pLine->bShowDataLabel )
						{
							str.Format( m_sYAxisPrecision, pLine->pData[j+step] );
							dc.TextOut( x2+1, y2 + nLabelBottomMargin, str );
						}
					}
					else if ( value1 == (std::numeric_limits<double>::infinity()) || value1 == -(std::numeric_limits<double>::infinity()) ||
						(!m_bDrawOutOfRange && ((value1 < m_dYMinValue) || (value1 > m_dYMaxValue))) )
					{
						if ( m_nGraphStyle == GRAPH_STYLE_LINE )
						{
							rMarker = CRect( x1 - pLine->nMarkerSize,
											 y1 - pLine->nMarkerSize,
											 x1 + pLine->nMarkerSize+1,
											 y1 + pLine->nMarkerSize+1 );
							DrawMarker( &dc, rMarker, &penMarker, pLine->nMarkerShape, pLine->nMarkerWidth, pLine->crMarker, m_crBackChart );
						}
						else if ( m_nGraphStyle == GRAPH_STYLE_BAR )
						{
							//dc.FillSolidRect( x1-2, y1, 5, m_rChart.bottom - y1, pLine->crLine );
							dc.Rectangle( x1 - nBarWidth, y1, x1 + nBarWidth + 1, m_rChart.bottom + 1 );
						}

						//if ( !m_bDrawOutOfRange && ((value1 < m_dYMinValue) || (value1 > m_dYMaxValue)) )
							//continue;

						if ( pLine->bShowDataLabel )
						{
							str.Format( m_sYAxisPrecision, pLine->pData[j] );
							dc.TextOut( x1+1, y1 + nLabelBottomMargin, str );
						}
					}

					continue;
				}


				//차트의 left에 도달하면 더 이상 그려줄 필요가 없다.
				//if ( x2 > m_rChart.right )
				//	break;

				if ( m_nGraphStyle == GRAPH_STYLE_BAR )
				{
					//dc.FillSolidRect( x1-2, y1, 5, m_rChart.bottom - y1, pLine->crLine );
					dc.Rectangle( x1 - nBarWidth, y1, x1 + nBarWidth + 1, m_rChart.bottom + 1 );
				}
				else
				{
					dc.SelectClipRgn( &rgn_line );
					dc.MoveTo( x1, y1 );
					dc.LineTo( x2, y2 );
					dc.SelectClipRgn( &rgn_other );
				}

				//해당 Y값 데이터와 마커(표식)를 그려준다.
				if ( m_nGraphStyle == GRAPH_STYLE_LINE )
				{
					rMarker = CRect( x1 - pLine->nMarkerSize,
										y1 - pLine->nMarkerSize,
										x1 + pLine->nMarkerSize+1,
										y1 + pLine->nMarkerSize+1 );
					DrawMarker( &dc, rMarker, &penMarker, pLine->nMarkerShape, pLine->nMarkerWidth, pLine->crMarker, m_crBackChart );
				}

				if ( pLine->bShowDataLabel )
				{
					str.Format( m_sYAxisPrecision, pLine->pData[j] );
					dc.TextOut( x1+1, y1 + nLabelBottomMargin, str );
				}

				//(n-2)와 (n-1)을 라인으로 그려주고 끝나므로 맨 마지막 (n-1)번째 항목에는 마크 표시가 안된다.
				//따라서 반복문 맨 마지막에는 (n-1)위치에도 마커를 그려주고 끝나야 한다.
				if ( (j == nDataCount - 2) || (j >= pLine->nData - 2) )
				{
					if ( m_nGraphStyle == GRAPH_STYLE_LINE )
					{
						rMarker = CRect( x2 - pLine->nMarkerSize,
											y2 - pLine->nMarkerSize,
											x2 + pLine->nMarkerSize+1,
											y2 + pLine->nMarkerSize+1 );
						DrawMarker( &dc, rMarker, &penMarker, pLine->nMarkerShape, pLine->nMarkerWidth, pLine->crMarker, m_crBackChart );
					}

					if ( pLine->bShowDataLabel )
					{
						str.Format( m_sYAxisPrecision, pLine->pData[j+step] );
						dc.TextOut( x2+1, y2 + nLabelBottomMargin, str );
					}
				}

				//만약 x축 데이터 개수만큼 y데이터가 존재하지 않는다면 더 이상 그리지 않는다.
				if ( j >= pLine->nData - 2 )
				{
					if ( m_nGraphStyle == GRAPH_STYLE_BAR ) 
					{
						//dc.FillSolidRect( x2-2, y2, 5, m_rChart.bottom - y2, pLine->crLine );
						dc.Rectangle( x2 - nBarWidth, y2, x2 + nBarWidth + 1, m_rChart.bottom + 1 );
					}
					else
					{
						break;
					}
				}
			}
		}

		dc.SelectObject( pOldPen );
		penLine.DeleteObject();
		penMarker.DeleteObject();

		dc.SelectObject( pOldBrush );
		brMarker.DeleteObject();

		dc.SelectObject( pOldFont );
	}

	if ( m_bShowMarkX && (m_XAxis.nData > 0) )
	{
		CPen	penMarkX( PS_DOT, 1, m_crMarkX );// GetComplementaryColor( m_crMarkX, m_crBackChart ) );
		pOldPen = (CPen*)dc.SelectObject( &penMarkX );
		//dc.SetROP2( R2_COPYP );

		//특정 x값에 대한 마크 표시
		for ( i = 0; i < m_nMarkX; i++ )
		{
			if ( (m_dMarkX[i] >= m_XAxis.pData[0]) && (m_dMarkX[i] <= m_XAxis.pData[m_XAxis.nData-1]) )
			{
				CPoint pt = GetScreenPosFromX(m_dMarkX[i]);

				dc.MoveTo( pt.x, m_rChart.top );
				dc.LineTo( pt.x, m_rChart.bottom - 1 );
			}
		}

		//dc.SetROP2( R2_COPYPEN );
		dc.SelectObject( pOldPen );
		penMarkX.DeleteObject();
	}


	CPoint	pt;
	GetCursorPos( &pt );
	ScreenToClient( &pt );

	if ( m_bShowMousePosLine && m_rChart.PtInRect( pt ) )
	{
		CPen	penGray(PS_DOT, 1, color_complementary(m_crBackChart));
		pOldPen = (CPen*)dc.SelectObject( &penGray );

		dc.SetROP2( R2_XORPEN );
		dc.MoveTo( pt.x, m_rChart.top );
		dc.LineTo( pt.x, m_rChart.bottom );
		dc.MoveTo( m_rChart.left, pt.y );
		dc.LineTo( m_rChart.right, pt.y );
		dc.SetROP2( R2_COPYPEN );

		dc.SelectObject( pOldPen );
		penGray.DeleteObject();
	}

	dc.SelectObject( pOldFont );
	dc.SelectObject( pOldPen );
	dc.SelectObject( pOldBrush );
}


BOOL CGraphCtrl::OnEraseBkgnd(CDC* pDC)
{
	// TODO: 여기에 메시지 처리기 코드를 추가 및/또는 기본값을 호출합니다.
	return false;
}

//x값에 해당하는 화면상의 픽셀 좌표를 얻는다.
CPoint CGraphCtrl::GetScreenPosFromX( double x, int nLineIndex )
{
	int		i;
	CPoint	pt;
	int		nXIndex;

	if ( m_nXAxisPrecision == 0 )
	{
		nXIndex = (int)x;
	}
	else
	{
		for ( i = 1; i < m_XAxis.nData; i++ )
		{
			if ( x <= m_XAxis.pData[i] )
			{
				nXIndex = i - 1;
				break;
			}
		}
	}

	double dUnitY;
	double dYMinValue = m_dYMinValue;
	double dYMaxValue = m_dYMaxValue;

	//y축이 정수 데이터이고 동적 범위 표현 방식일 경우
	//miny, maxy는 항상 차트 영역의 top과 bottom에 표시되므로 가독성이 떨어진다.
	//위, 아래에 n% 만큼의 여백 범위를 추가시켜준다.
	if ( m_nYAxisPrecision == 0 )
	{
		if ( m_dYMinimumValue == (std::numeric_limits<double>::infinity()) )
			dYMinValue -= (m_dYMaxValue - m_dYMinValue) * m_dYValueMargin;
		if ( m_dYMaximumValue == -(std::numeric_limits<double>::infinity()) )
			dYMaxValue += (m_dYMaxValue - m_dYMinValue) * m_dYValueMargin;
	}

	dUnitY = (double)m_rChart.Height() / (dYMaxValue - dYMinValue);
	pt.x = m_rChart.left + (x - m_XAxis.pData[0]) * (double)m_rChart.Width() / (m_XAxis.pData[m_XAxis.nData-1] - m_XAxis.pData[0]);
	if (nLineIndex < 0)
		pt.y = m_rChart.CenterPoint().y;
	else
		pt.y = m_rChart.bottom - 1 - (m_pLineData[nLineIndex]->pData[nXIndex] - dYMinValue) * dUnitY;
	return pt;
}

//마우스 좌표에 해당하는 x, y값을 얻는다.
bool CGraphCtrl::GetXYFromScreenPos( int nIndex, CPoint point, double &x, double &y )
{
	if ( !m_bLButtonDown && m_rChart.PtInRect( point ) == false )
	{
		if ( m_bMouseIsInChart )
		{
			TRACE( "leave chart area\n" );
			m_bMouseIsInChart = false;
			Invalidate();
		}

		return false;
	}

	if ( m_XAxis.pData == NULL )
		return false;

	//if ( m_XAxis.pData[m_XAxis.nData-1] - m_XAxis.pData[0] <= 0 )
		//return false;

// 	if ( point == m_ptOldPoint )
// 		return;

	m_bMouseIsInChart = true;


	int		i;
	CString str;
	CString sXvalue, sYvalue;
	CString sToolTip = _T("");
	CPoint	pt = point;

	pt.x -= m_rChart.left;
	double	dAxisX = (double)m_rChart.Width() / (m_XAxis.pData[m_XAxis.nData-1] - m_XAxis.pData[0]);
	double	dXValue	= (double)pt.x / dAxisX + m_XAxis.pData[0];
	int		nXIndex = 0;

	//x축의 변위가 차트 width에 비해 클 경우 툴팁으로 주변 데이터를 촘촘히 볼 수 없는 경우가 있다.
	//마우스가 +1픽셀만 움직여도 x값은 다음값이 아닌 큰값으로 계산된다.
	//이때 주변 데이터를 보고자하는 위치에서 마우스 좌클릭을 한 상태에서 드래그하면
	//주변값을 자세히 살펴볼 수 있다.
	if ( m_bLButtonDown )
	{
		dXValue = (double)(m_ptLButtonDown.x) / dAxisX + m_XAxis.pData[0] - (double)(m_ptLButtonDown.x - pt.x) / 2.0;
		//TRACE( "dXValue = %f\nm_ptLButtonDown.x - pt.x = %d\n", dXValue, m_ptLButtonDown.x - pt.x );
		if ( dXValue < m_XAxis.pData[0] )
			dXValue = m_XAxis.pData[0];
		else if ( dXValue > m_XAxis.pData[m_XAxis.nData-1] )
			dXValue = m_XAxis.pData[m_XAxis.nData-1];
	}

	for ( i = 1; i < m_XAxis.nData; i++ )
	{
		if ( dXValue <= m_XAxis.pData[i] )
		{
			nXIndex = i - 1;
			break;
		}
	}

	//아래 코드를 넣지 않으면 x축 정수 표시에 오차가 발생한다.
	//예를 들어 3 ~ 4 사이의 구간에 마우스 커서가 존재할때
	//2.500 ~ 3.499사이는 3으로,
	//3.500 ~ 4.499까지는 4로 자동 반올림 또는 버림이 수행된다.
	if ( m_nXAxisPrecision == 0 )
		dXValue = (int)dXValue;

	x = m_XAxis.pData[0] + nXIndex;
	y = 0;

	CGraphLineData*	pLine = m_pLineData[nIndex];
	if ( pLine == NULL || pLine->pData == NULL || pLine->pData == 0 )
		return true;

	x = m_XAxis.pData[0] + nXIndex;
	y = pLine->pData[nXIndex];

	return true;
}

//x값과 가장 가까운 위치의 y값 리턴
double CGraphCtrl::GetYValueNearX( double x, int nLineIndex )
{
	double y = 0.0;

	if ( m_XAxis.pData == NULL || m_XAxis.nData <= 0 ||
		 nLineIndex < 0 || nLineIndex >= GRAPH_LINE_MAX ||
		 m_pLineData == NULL || m_pLineData[nLineIndex] == NULL ||
		 m_pLineData[nLineIndex]->pData == NULL ||
		 m_pLineData[nLineIndex]->nData <= 0 )
		return y;

	for ( int i = 1; i < m_XAxis.nData; i++ )
	{
		if ( x <= m_XAxis.pData[i] )
		{
			return m_pLineData[nLineIndex]->pData[i - 1];
			break;
		}
	}

	return y;
}

void CGraphCtrl::OnMouseMove(UINT nFlags, CPoint point)
{
	// TODO: 여기에 메시지 처리기 코드를 추가 및/또는 기본값을 호출합니다.
/*	CMouseEvent	me( point, WM_MOUSEMOVE );
	if ( m_bInfoText[1] )
	{
		double x, y;
		CString sx, sy;

		if ( GetXYFromScreenPos( 0, point, x, y ) )
		{
			sx.Format( m_sXAxisPrecision, x );// m_XAxis.pData[nXIndex] );
			if ( y == -(std::numeric_limits<double>::infinity()) )
				sy = "";
			else
				sy.Format( m_sYAxisPrecision, y );

			m_sInfoText[1].Format( "(%s, %s)", sx, sy );
			InvalidateRect( m_rTitleArea );
		}
	}
	//::SendMessage( GetParent()->GetSafeHwnd(), MESSAGE_GRAPHCTRL_MOUSE_EVENT, m_nGraphID, (LPARAM)&me );
*/
	if ( !m_bShowToolTip && !m_bInfoText[1] )
		return;

	double	dXvalue, dYvalue;
	CString str;
	CString sXvalue, sYvalue;
	CString sToolTip;

	for ( int i = 0; i < GRAPH_LINE_MAX; i++ )
	{
		CGraphLineData*	pLine = m_pLineData[i];
		if ( pLine == NULL || pLine->pData == NULL || pLine->pData == 0 ) continue;

		if ( GetXYFromScreenPos( i, point, dXvalue, dYvalue ) == false )
			continue;

		sXvalue.Format( m_sXAxisPrecision, dXvalue );// m_XAxis.pData[nXIndex] );
		sYvalue.Format( m_sYAxisPrecision, dYvalue );

		if ( dYvalue == -(std::numeric_limits<double>::infinity()) )
			sYvalue = "";

		str.Format( _T("%s[%s] = %s"), pLine->sTitle, sXvalue, sYvalue );

		if ( sToolTip == "" )
			sToolTip = sToolTip + str;
		else
			sToolTip = sToolTip + _T("\n") + str;
	}

	//TRACE( _T("%s\n"), sToolTip );
	if ( m_bShowToolTip )
		m_pToolTip->UpdateTipText( sToolTip, this );
	if ( m_bInfoText[1] )
	{
		m_sInfoText[1] = sToolTip;
		InvalidateRect( m_rTitleArea );
	}

	/*
	// Not using CToolTipCtrl::AddTool() because
	// it redirects the messages to the parent
	TOOLINFO ti = {0};
	ti.cbSize = sizeof(TOOLINFO);
	ti.uFlags = TTF_IDISHWND;    // Indicate that uId is handle to a control
	ti.uId = (UINT_PTR)m_hWnd;   // Handle to the control
	ti.hwnd = m_hWnd;            // Handle to window
	// to receive the tooltip-messages
	ti.hinst = ::AfxGetInstanceHandle();
	ti.lpszText = LPSTR_TEXTCALLBACK;
	ti.rect.left = 100;
	ti.rect.top = 10;
	ti.rect.right = 200;
	ti.rect.bottom = 50;
	m_pToolTip->SendMessage(TTM_ADDTOOL, 0, (LPARAM) (LPTOOLINFO) &ti);
	m_pToolTip->Activate(TRUE);

	m_sToolTip = sToolTip;
	*/
	//m_pToolTip->AddToolTip( this, sToolTip, CRect(point.x-1,point.y-1,point.x+1,point.y+1), true );
	//m_pToolTip->UpdateTipText( sToolTip, this );
	//m_pToolTip->Update();

	//m_ptOldPoint = point;
	//Invalidate();

	//CWnd::OnMouseMove(nFlags, point);
}

void CGraphCtrl::OnNcMouseMove(UINT nHitTest, CPoint point)
{
	TRACE( "NC mouse move : %d\n", nHitTest );
}

void CGraphCtrl::OnLButtonDown(UINT nFlags, CPoint point)
{
	// TODO: 여기에 메시지 처리기 코드를 추가 및/또는 기본값을 호출합니다.
	if ( m_bLButtonDown == false )
	{
		m_bLButtonDown = true;
		m_ptLButtonDown = point - m_rChart.TopLeft();
		TRACE( "m_ptLButtonDown.x = %d\n", m_ptLButtonDown.x );
		SetCapture();
	}

	CWnd::OnLButtonDown(nFlags, point);
}


void CGraphCtrl::OnLButtonUp(UINT nFlags, CPoint point)
{
	// TODO: 여기에 메시지 처리기 코드를 추가 및/또는 기본값을 호출합니다.
	if ( m_bLButtonDown )
	{
		m_bLButtonDown = false;
		ReleaseCapture();
	}

	CMouseEvent	me( point, WM_LBUTTONUP );
	::SendMessage( GetParent()->GetSafeHwnd(), MESSAGE_GRAPHCTRL_MOUSE_EVENT, m_nGraphID, (LPARAM)&me );

	CWnd::OnLButtonUp(nFlags, point);
}


void CGraphCtrl::OnLButtonDblClk(UINT nFlags, CPoint point)
{
	// TODO: 여기에 메시지 처리기 코드를 추가 및/또는 기본값을 호출합니다.

	CWnd::OnLButtonDblClk(nFlags, point);
}


void CGraphCtrl::OnRButtonDown(UINT nFlags, CPoint point)
{
	// TODO: 여기에 메시지 처리기 코드를 추가 및/또는 기본값을 호출합니다.

	CWnd::OnRButtonDown(nFlags, point);
}


void CGraphCtrl::OnRButtonUp(UINT nFlags, CPoint point)
{
	// TODO: 여기에 메시지 처리기 코드를 추가 및/또는 기본값을 호출합니다.
	if ( !m_bUsePopupMenu )
		return;

	CMenu	menu, menuGraphStyle, menuLineColor;
	CPoint	pt = point;
	CString str, sTitle;

	menu.CreatePopupMenu();
	menuGraphStyle.CreatePopupMenu();
	menuLineColor.CreatePopupMenu();

	menuGraphStyle.AppendMenu( MF_STRING, ID_MENU_GRAPH_STYLE_BAR, _T("Bar graph style") );
	menuGraphStyle.AppendMenu( MF_STRING, ID_MENU_GRAPH_STYLE_LINE, _T("Line graph style") );
	menuGraphStyle.EnableMenuItem( ID_MENU_GRAPH_STYLE_BAR, (m_nGraphStyle != GRAPH_STYLE_FLOW) ? MF_ENABLED : MF_DISABLED );
	menuGraphStyle.EnableMenuItem( ID_MENU_GRAPH_STYLE_LINE, (m_nGraphStyle != GRAPH_STYLE_FLOW) ? MF_ENABLED : MF_DISABLED );
	menuGraphStyle.CheckMenuItem(  ID_MENU_GRAPH_STYLE_BAR, (m_nGraphStyle == GRAPH_STYLE_BAR) ? MF_CHECKED : MF_UNCHECKED );
	menuGraphStyle.CheckMenuItem(  ID_MENU_GRAPH_STYLE_LINE, (m_nGraphStyle == GRAPH_STYLE_LINE) ? MF_CHECKED : MF_UNCHECKED );

	if ( m_nGraphStyle == GRAPH_STYLE_FLOW )
	{
		menu.AppendMenu( MF_STRING, ID_MENU_PAUSE, _T("일시 정지(&P)") );
		menu.AppendMenu( MF_SEPARATOR );
	}	
	else
	{
		menu.AppendMenu( MF_POPUP, (UINT_PTR)menuGraphStyle.m_hMenu, _T("Graph Style") );
		menu.AppendMenu( MF_SEPARATOR );
	}


	//라인 컬러 변경 팝업메뉴 생성 및 추가
	for ( int i = 0; i < GRAPH_LINE_MAX; i++ )
	{
		if ( m_pLineData[i] == NULL )
		{
			str.Format( _T("line %d"), i );

		}
		else
		{
			if ( _tcslen( m_pLineData[i]->GetLineTitle() ) > 0 )
				str.Format( _T("%s"), m_pLineData[i]->GetLineTitle() );
			else
				str.Format( _T("line %d"), i );
		}
		menuLineColor.AppendMenu( MF_STRING, ID_MENU_LINE_00 + i, str );
		menuLineColor.EnableMenuItem( ID_MENU_LINE_00 + i, (m_pLineData[i] ? MF_ENABLED : MF_DISABLED) );
	}

	menu.AppendMenu( MF_POPUP, (UINT_PTR)menuLineColor.m_hMenu, _T("Change line color(&A)") );
	menu.AppendMenu( MF_SEPARATOR );


#if USE_WIN32INPUTBOX

	menu.AppendMenu( MF_STRING, ID_MENU_SET_X_LEVEL, _T("Set X axis divide level...") );
	menu.AppendMenu( MF_STRING, ID_MENU_SET_Y_LEVEL, _T("Set Y axis divide level...") );
	menu.AppendMenu( MF_SEPARATOR );
	menu.AppendMenu( MF_STRING, ID_MENU_SET_Y_MIN, _T("Set minimum value for Y range...") );
	menu.AppendMenu( MF_STRING, ID_MENU_SET_Y_MAX, _T("Set maximum value for Y range...") );
	menu.AppendMenu( MF_SEPARATOR );
#endif
	menu.AppendMenu( MF_STRING, ID_MENU_SHOW_X_AXIS, _T("Display X axis(&X)") );
	menu.AppendMenu( MF_STRING, ID_MENU_SHOW_Y_AXIS, _T("Display Y axis(&Y)") );
	menu.AppendMenu( MF_STRING, ID_MENU_SHOW_DATA_LABEL, _T("Show data label(&D)") );
	menu.AppendMenu( MF_STRING, ID_MENU_SHOW_MARKX, _T("Show specified X marker(&M)") );
	menu.AppendMenu( MF_STRING, ID_MENU_SHOW_MOUSELINE, _T("Show cross lines on cursor(&G)") );
	menu.AppendMenu( MF_STRING, ID_MENU_SHOW_TOOLTIP, _T("Show data tooltip(&T)") );
	menu.AppendMenu( MF_SEPARATOR );

	menu.AppendMenu( MF_STRING, ID_MENU_COPY_DATA_TO_CLIPBOARD, _T("Copy data to clipboard(&C)") );
	menu.AppendMenu( MF_STRING, ID_MENU_COPY_IMAGE_TO_CLIPBOARD, _T("Copy image to clipboard(&I)") );

	if ( m_nGraphStyle != GRAPH_STYLE_FLOW )
		menu.AppendMenu( MF_STRING, ID_MENU_PASTE_FROM_CLIPBOARD, _T("Paste from clipboard(&V)") );
	
	menu.AppendMenu( MF_SEPARATOR );

	menu.AppendMenu( MF_STRING, ID_MENU_SAVE_TO_FILE, _T("Save as txt file(&S)...") );
	menu.AppendMenu( MF_STRING, ID_MENU_SAVE_TO_IMAGE, _T("Save as bmp file(&B)...") );
	menu.AppendMenu( MF_SEPARATOR );
	menu.AppendMenu( MF_STRING, ID_MENU_START_DEMO, _T("Start Graph Demo(&O)") );
	menu.AppendMenu( MF_SEPARATOR );
	menu.AppendMenu( MF_STRING, ID_MENU_RESET, _T("Reset Graph(&R)") );


	if ( m_nGraphStyle == GRAPH_STYLE_FLOW )
		menu.CheckMenuItem( ID_MENU_PAUSE, m_bPause ? MF_CHECKED : MF_UNCHECKED );

	menu.CheckMenuItem(  ID_MENU_SHOW_X_AXIS, m_bShowXAxisLabel ? MF_CHECKED : MF_UNCHECKED );
	menu.CheckMenuItem(  ID_MENU_SHOW_Y_AXIS, m_bShowYAxisLabel ? MF_CHECKED : MF_UNCHECKED );
	menu.CheckMenuItem(  ID_MENU_SHOW_DATA_LABEL, m_bShowDataLabel ? MF_CHECKED : MF_UNCHECKED );
	menu.CheckMenuItem(  ID_MENU_SHOW_MARKX, m_bShowMarkX ? MF_CHECKED : MF_UNCHECKED );
	menu.CheckMenuItem(  ID_MENU_SHOW_MOUSELINE, m_bShowMousePosLine ? MF_CHECKED : MF_UNCHECKED );
	menu.CheckMenuItem(  ID_MENU_SHOW_TOOLTIP, m_bShowToolTip ? MF_CHECKED : MF_UNCHECKED );
	menu.EnableMenuItem( ID_MENU_SAVE_TO_FILE, GetValidLineCount() ? MF_ENABLED : MF_DISABLED );
	menu.EnableMenuItem( ID_MENU_SAVE_TO_IMAGE, GetValidLineCount() ? MF_ENABLED : MF_DISABLED );
	menu.CheckMenuItem(  ID_MENU_START_DEMO, m_bStartDemo ? MF_CHECKED : MF_UNCHECKED );

	SetMenu( &menu );

	ClientToScreen( &pt );
	menu.TrackPopupMenu( TPM_LEFTALIGN, pt.x, pt.y, this );
	menu.DestroyMenu();
	menuGraphStyle.DestroyMenu();

	CWnd::OnRButtonUp(nFlags, point);
}


void CGraphCtrl::OnRButtonDblClk(UINT nFlags, CPoint point)
{
	// TODO: 여기에 메시지 처리기 코드를 추가 및/또는 기본값을 호출합니다.

	CWnd::OnRButtonDblClk(nFlags, point);
}


void CGraphCtrl::OnSize(UINT nType, int cx, int cy)
{
	CWnd::OnSize(nType, cx, cy);

	// TODO: 여기에 메시지 처리기 코드를 추가합니다.

	//m_nXAxisLevel = cx;
}

void CGraphCtrl::GenerateDemoData()
{
	if ( m_nGraphStyle == GRAPH_STYLE_FLOW )
	{
		if ( IsWindowVisible() == false )
			return;

		if ( m_nYAxisPrecision == 0 )
			AddLineData( 0, (int)random19937(m_dYMinValue, m_dYMaxValue) );
		else
			AddLineData( 0, random19937( m_dYMinValue, m_dYMaxValue ) );
	}
	else
	{
		int i;
		//60000	: 22ms
		//6000	: 2.2ms
		int n = 6 * pow( 10, random19937( 1.0, 2.0 ) ) + 1;
		int *x = new int[n];
		int *y = new int[n];

		for ( i = 0; i < n; i++ )
		{
			x[i] = i;
			y[i] = random19937( 0.0, 1.0 );
		}

		SetXAxis( x, n );
		SetXAxisLevel( 30 );

		//SetXAxisPrecision( 3 );
		//SetYAxisPrecision( 3 );

		//ResetYRange();

		//ptimer_start(0);
		//double d = ptimer_get_time(0);
		SetLineData( 0, y, n );
		//TRACE( "%f\n", ptimer_get_time(0) - d );
		AddMarkX( random19937( (double)x[0], (double)x[n-1] ) );

		delete [] x;
		delete [] y;
	}
}

void CGraphCtrl::OnTimer(UINT_PTR nIDEvent)
{
	// TODO: 여기에 메시지 처리기 코드를 추가 및/또는 기본값을 호출합니다.
	if ( nIDEvent == TIMER_START_DEMO )
	{
		//ptimer_start(0);
		//double d = ptimer_get_time(0);
		if ( IsWindowVisible() )
			GenerateDemoData();
		//TRACE( "%f\n", ptimer_get_time(0) - d );
	}

	CWnd::OnTimer(nIDEvent);
}


BOOL CGraphCtrl::PreCreateWindow(CREATESTRUCT& cs)
{
	// TODO: 여기에 특수화된 코드를 추가 및/또는 기본 클래스를 호출합니다.
	if ( m_pToolTip == NULL )
		m_pToolTip = new CToolTipCtrl();

	if ( m_pToolTip->m_hWnd == NULL )
	{
		m_pToolTip->Create( this, TTS_ALWAYSTIP | TTS_NOPREFIX | TTS_NOANIMATE );
		m_pToolTip->SetDelayTime( TTDT_AUTOPOP, -1 );
		m_pToolTip->SetDelayTime( TTDT_INITIAL, 0 );
		m_pToolTip->SetDelayTime( TTDT_RESHOW, 0 );
		m_pToolTip->SetMaxTipWidth(400); 
		m_pToolTip->AddTool(this,_T("") );
		m_pToolTip->Activate(TRUE);
	}
	return CWnd::PreCreateWindow(cs);
}


void CGraphCtrl::PreSubclassWindow()
{
	// TODO: 여기에 특수화된 코드를 추가 및/또는 기본 클래스를 호출합니다.
	if ( m_pToolTip == NULL )
		m_pToolTip = new CToolTipCtrl();

	if ( m_pToolTip->m_hWnd == NULL )
	{
		m_pToolTip->Create( this, TTS_ALWAYSTIP | TTS_NOPREFIX | TTS_NOANIMATE );
		m_pToolTip->SetDelayTime( TTDT_AUTOPOP, -1 );
		m_pToolTip->SetDelayTime( TTDT_INITIAL, 0 );
		m_pToolTip->SetDelayTime( TTDT_RESHOW, 0 );
		m_pToolTip->SetMaxTipWidth(400); 
		m_pToolTip->AddTool(this,_T("") );
		m_pToolTip->Activate(TRUE);
	}

	DragAcceptFiles( true );

	CWnd::PreSubclassWindow();
}


BOOL CGraphCtrl::PreTranslateMessage(MSG* pMsg)
{
	// TODO: 여기에 특수화된 코드를 추가 및/또는 기본 클래스를 호출합니다.
	if ( m_bShowToolTip )
	{
		MSG msg = *pMsg; 
		msg.hwnd = (HWND)m_pToolTip->SendMessage( TTM_WINDOWFROMPOINT, 0, (LPARAM)&msg.pt ); 

		CPoint pt = pMsg->pt; 

		if ( msg.message >= WM_MOUSEFIRST && msg.message <= WM_MOUSELAST ) 
			::ScreenToClient( msg.hwnd, &pt ); 

		msg.lParam = MAKELONG( pt.x, pt.y ); 

		// relay mouse event before deleting old tool 
		m_pToolTip->Activate( TRUE );
		m_pToolTip->SendMessage( TTM_RELAYEVENT, 0, (LPARAM)&msg ); 
	}

	return CWnd::PreTranslateMessage(pMsg);
}

/*
CString CGraphCtrl::GetExeDirectory()
{
	char	FilePathName[MAX_PATH];
	CString sExeFolder;
	
	GetModuleFileName(AfxGetInstanceHandle(), FilePathName, MAX_PATH);
	
	sExeFolder = FilePathName;
	
	return sExeFolder.Left( sExeFolder.ReverseFind( '\\' ) );
}
*/

void CGraphCtrl::OnPopupMenu( UINT nID )
{
	int	i;
	TCHAR buf[100] = {0};

	switch ( nID )
	{
		case ID_MENU_GRAPH_STYLE_LINE :
				m_nGraphStyle = GRAPH_STYLE_LINE;
				Invalidate();
				break;
		case ID_MENU_GRAPH_STYLE_BAR :
				m_nGraphStyle = GRAPH_STYLE_BAR;
				Invalidate();
				break;
		case ID_MENU_PAUSE :
				m_bPause = !m_bPause;
				break;

		case ID_MENU_LINE_00	:
		case ID_MENU_LINE_01	:
		case ID_MENU_LINE_02	:
		case ID_MENU_LINE_03	:
		case ID_MENU_LINE_04	:
		case ID_MENU_LINE_05	:
		case ID_MENU_LINE_06	:
		case ID_MENU_LINE_07	:
		case ID_MENU_LINE_08	:
		case ID_MENU_LINE_09	:
				{
					int idx = nID - ID_MENU_LINE_00;
					
					if ( idx < 0 || idx >= GRAPH_LINE_MAX )
					{
						AfxMessageBox( _T("That line index is out of range.") );
						return;
					}

					if ( m_pLineData == NULL || m_pLineData[idx] == NULL )
					{
						AfxMessageBox(_T("A line at that index has not yet been created..") );
						return;
					}

					CColorDialog dlg( GetLineColor(idx), 0, this );
					if ( dlg.DoModal() == IDCANCEL )
						return;

					SetLineColor( 0, dlg.GetColor() );
				}
				break;
#if USE_WIN32INPUTBOX
		case ID_MENU_SET_X_LEVEL :
				if ( CWin32InputBox::InputBox(_T("CGraphCtrl"), _T("Input the X axis divide level."),
												buf, 100, CWin32InputBox::NORMAL, 0, m_hWnd ) == IDOK )
				{
					if ( _tcscmp( buf, _T("~") ) == 0 )
					{
						m_nXAxisLevel = 10;
					}
					else
					{
						m_nXAxisLevel = _tstof( buf );
						if ( m_nXAxisLevel <= 0 )
							m_nXAxisLevel = 10;
					}
					Invalidate();
				}
				break;
		case ID_MENU_SET_Y_LEVEL :
				if ( CWin32InputBox::InputBox(_T("CGraphCtrl"), _T("Input the Y axis divide level."),
												buf, 100, CWin32InputBox::NORMAL, 0, m_hWnd ) == IDOK )
				{
					if ( _tcscmp( buf, _T("~") ) == 0 )
					{
						m_nYAxisLevel = 5;
					}
					else
					{
						m_nYAxisLevel = _tstof( buf );
						if ( m_nYAxisLevel <= 0 )
							m_nYAxisLevel = 5;
					}
					Invalidate();
				}
				break;
		case ID_MENU_SET_Y_MIN :
				if ( CWin32InputBox::InputBox(_T("CGraphCtrl"), _T("Input the minimum value for fixed Y range.\nEnter \"~\" for dynamic range."),
												buf, 100, CWin32InputBox::NORMAL, 0, m_hWnd ) == IDOK )
				{
					if ( _tcscmp( buf, _T("~") ) == 0 )
					{
						m_dYMinValue = m_dYMinimumValue = std::numeric_limits<double>::infinity();
						GetYMinMaxValue();
					}
					else
					{
						m_dYMinValue = m_dYMinimumValue = _tstof( buf );
					}
					Invalidate();
				}
				break;
		case ID_MENU_SET_Y_MAX :
				if ( CWin32InputBox::InputBox(_T("CGraphCtrl"), _T("Input the maximum value for fixed Y range.\nEnter \"~\" for dynamic range."),
												buf, 100, CWin32InputBox::NORMAL, 0, m_hWnd ) == IDOK )
				{
					if ( _tcscmp( buf, _T("~") ) == 0 )
					{
						m_dYMaxValue = m_dYMaximumValue = -(std::numeric_limits<double>::infinity());
						GetYMinMaxValue();
					}
					else
					{
						m_dYMaxValue = m_dYMaximumValue = _tstof( buf );
					}
					Invalidate();
				}
				break;
#endif

		case ID_MENU_SHOW_X_AXIS :
				m_bShowXAxisLabel = !m_bShowXAxisLabel;
				Invalidate();
				break;
		case ID_MENU_SHOW_Y_AXIS :
				m_bShowYAxisLabel = !m_bShowYAxisLabel;
				Invalidate();
				break;
		case ID_MENU_SHOW_DATA_LABEL :
				m_bShowDataLabel = !m_bShowDataLabel;

				for ( i = 0; i < GRAPH_LINE_MAX; i++ )
				{
					if ( IsExistLineData(i) )
					{
						CGraphLineData* pLine = GetLineDataPtr(i);
						pLine->bShowDataLabel = m_bShowDataLabel;
					}
				}
				Invalidate();
				break;
		case ID_MENU_SHOW_MARKX :
				m_bShowMarkX = !m_bShowMarkX;
				Invalidate();
				break;
		case ID_MENU_SHOW_MOUSELINE :
				m_bShowMousePosLine = !m_bShowMousePosLine;
				Invalidate();
				break;
		case ID_MENU_SHOW_TOOLTIP :
				m_bShowToolTip = !m_bShowToolTip;
				break;
		case ID_MENU_COPY_DATA_TO_CLIPBOARD :
				OnMenuCopyDataToClipboard();
				break;
		case ID_MENU_COPY_IMAGE_TO_CLIPBOARD :
				OnMenuCopyImageToClipboard();
				break;
		case ID_MENU_PASTE_FROM_CLIPBOARD :
				OnMenuPasteFromClipboard();
				break;
		case ID_MENU_SAVE_TO_FILE :
				OnMenuSaveToFile();
				break;
		case ID_MENU_SAVE_TO_IMAGE :
				OnMenuSaveToImage();
				break;
		case ID_MENU_START_DEMO :
				m_bStartDemo = !m_bStartDemo;
				StartGraphDemo( m_bStartDemo, true );
				break;
		case ID_MENU_RESET :
				DeleteAllLineData();
				Invalidate();
				break;
	}
}

void CGraphCtrl::OnMenuSaveToFile()
{
	bool	bShiftKeyPressed = IsShiftPressed();
	CString sDataFile;
	
	sDataFile.Format( _T("%s\\graph_data_%s.txt"),
						GetExeDirectory(), GetCurrentDateTimeString( false ) );

	CFileDialog fileDlg( false, _T("*.txt"), sDataFile, OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT, _T("Text file|*.txt|All files|*.*||") );

	if ( fileDlg.DoModal() == IDCANCEL )
		return;

	int		i, j;
	FILE*	fp = fopen( (char*)(LPCTSTR)fileDlg.GetPathName(), "wt" );

	if ( fp == NULL )
	{
		AfxMessageBox( _T("Fail to create output text file."), MB_ICONSTOP );
		return;
	}

	CGraphLineData* pLine = NULL;

	//제목 라인 출력
/*
	fprintf( fp, "X_axis" );
	for ( i = 0; i < GRAPH_LINE_MAX; i++ )
	{
		pLine = m_pLineData[i];
		if ( pLine == NULL ) continue;

		if ( strlen( pLine->sTitle ) == 0 )
			fprintf( fp, "\t#%d", i );
		else
			fprintf( fp, "\t%s", pLine->sTitle );
	}
	fprintf( fp, "\n" );
*/
	//실제 데이터 출력
	CString str;

	for ( i = 0; i < m_XAxis.nData; i++ )
	{
		str.Format( m_sXAxisPrecision, m_XAxis.pData[i] );
		fprintf( fp, "%s\t", str );

		for ( j = 0; j < GRAPH_LINE_MAX; j++ )
		{
			pLine = m_pLineData[j];
			if ( pLine == NULL ) continue;

			if ( pLine->pData == NULL ) continue;

			if ( pLine->pData[i] == std::numeric_limits<double>::infinity() ||
				 pLine->pData[i] == -std::numeric_limits<double>::infinity() )
				 str.Format(_T("%.3f"), pLine->pData[i] );
			else
				str.Format( m_sYAxisPrecision, pLine->pData[i] );
			fprintf( fp, "%s\t", str );
		}

		fprintf( fp, "\n" );
	}

	//x축 mark 정보 저장
	if ( m_nMarkX > 0 )
	{
		fprintf( fp, "_mark_x_\n" );
		for ( i = 0; i < m_nMarkX; i++ )
			fprintf( fp, "%f\n", m_dMarkX[i] );
	}

	fclose( fp );

	MessageBeep( 0 );

	if ( bShiftKeyPressed || IsShiftPressed() )
		ShellExecute( m_hWnd, _T("open"), fileDlg.GetPathName(), NULL, NULL, SW_SHOWNORMAL );
}

void CGraphCtrl::OnMenuSaveToImage()
{
	bool	bShiftKeyPressed = IsShiftPressed();
	CString sImageFile;

	sImageFile.Format( _T("%s\\graph_data_%s.bmp"), GetExeDirectory(), GetCurrentDateTimeString( false ) );
	CFileDialog fileDlg( false, _T("*.bmp"), sImageFile, OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT, _T("Bmp file|*.bmp|All files|*.*||") );

	if ( fileDlg.DoModal() == IDCANCEL )
		return;

	//화면 캡쳐 방식으로 그래프를 저장하므로 파일 대화상자가 사라질때까지 약간의 딜레이가 필요하다.
	Wait( 500 );

	SaveGraphToImage( fileDlg.GetPathName() );

	if ( bShiftKeyPressed || IsShiftPressed() )
		ShellExecute( m_hWnd, _T("open"), fileDlg.GetPathName(), NULL, NULL, SW_SHOWNORMAL );
}

//이 함수는 그래프를 bmp로 자동 저장하기 위한 코드로서
//화면 캡쳐 방식으로 저장하므로 반드시 그래프가 화면상에 떠있는 상태에서 호출되어야 한다.
void CGraphCtrl::SaveGraphToImage( CString sfile )
{
	CClientDC dc(this);
	WriteBMP( CaptureWindowToBitmap( m_hWnd ), dc.GetSafeHdc(), (LPTSTR)(LPCTSTR)sfile );
	MessageBeep( 0 );
}


void CGraphCtrl::OnMenuPasteFromClipboard()
{
	TCHAR	*buffer = NULL;
	HGLOBAL	hGlobal;

	CString sWhole, str, sToken;

	if ( OpenClipboard() == false )
		return;

#ifdef _UNICODE
	hGlobal = GetClipboardData( CF_UNICODETEXT );
#else
	hGlobal = GetClipboardData( CF_TEXT );
#endif

	if ( hGlobal )
	{
		buffer = (TCHAR*)GlobalLock( hGlobal );
		GlobalUnlock( hGlobal );
	}

	CloseClipboard();

	if ( buffer )
		ImportTextData( buffer );
}

void CGraphCtrl::ImportTextData( CString sData )
{
	m_pToolTip->UpdateTipText(_T(""), this );

	sData.Replace( _T("\r\n"), _T("\n") );
	sData.Replace( _T("\t"), _T(",") );
	sData.Replace( _T(" "), _T(",") );

	if (get_char_count(sData, ',')== 0 || get_char_count(sData, '\n')== 0)
	{
		AfxMessageBox(_T("Invalid chart data format.\n\
ex)=================\n\
x[0]	y[0][0]	y[1][0]\n\
x[1]	y[0][1]	y[1][1]\n\
x[2]	y[0][2]	y[1][2]\n\
..................................................\n\
* seperators : tab, space, comma") );
		return;
	}

	int		nIndex, nDigit, nDigitX = 0, nDigitY = 0;
	int		nXAxisPrecision = 0;
	int		nYAxisPrecision = 0;
	int		max = 1 + GRAPH_LINE_MAX;
	bool	bMarkXStart = false;
	CString str, sToken;
	
	m_nImportedMarkX = 0;

	while ( sData.GetLength() )
	{
		str = sData.Left( sData.Find(_T("\n")) );

		if ( str == "" )
			str = sData;

		sData = sData.Mid( str.GetLength() );
		sData.Trim();

		str.Trim();
		nIndex = 0;

		if ( str == "_mark_x_" )
		{
			bMarkXStart = true;
			continue;
		}

		if ( bMarkXStart )
		{
			m_dImportedMarkX[m_nImportedMarkX++] = _tstof( str );
			continue;
		}

		while ( str != "" )
		{
			sToken = GetToken( str, _T(",") );
			sToken.Trim();

			if ( (sToken != "1.#IO") && (sToken != "-1.#IO") && (sToken != " ") && (sToken != "inf") && !IsNumericString( sToken ) )
			{
				AfxMessageBox( _T("Invalid chart data format.\n\
ex)=================\n\
x[0]	y[0][0]	y[1][0]\n\
x[1]	y[0][1]	y[1][1]\n\
x[2]	y[0][2]	y[1][2]\n\
..................................................\n\
* seperators : tab, space, comma") );

				return;
			}

			//1.234 => length=5, rfind('.') = 1, nDigit = 5 -1 - 1 = 3
			if ( (sToken != "1.#IO") && (sToken != "-1.#IO") && (sToken.ReverseFind( '.' ) > 0) )
				nDigit = sToken.GetLength() - sToken.ReverseFind( '.' ) - 1;
			else
				nDigit = 0;

			//x축 데이터의 소수점 자리수
			if ( nIndex == 0 )
			{
				if ( nDigit > nXAxisPrecision )
					nXAxisPrecision = nDigit;
			}
			//y축 소수점
			else
			{
				if ( sToken != "1.#IO" && sToken != "-1.#IO" )
				{
					if ( nDigit > nYAxisPrecision )
						nYAxisPrecision = nDigit;
				}
			}

			m_vtData[nIndex].push_back( sToken );
			
			//x값은 있으나 y값이 NULL인 경우
			if ( nIndex == 0 && str == "" )
				m_vtData[nIndex+1].push_back( _T("-1.#IO") );

			nIndex++;
		}
	}

	if ( m_vtData[0].size() <= 0 )
		return;

	m_nXAxisPrecision = nXAxisPrecision;
	m_nYAxisPrecision = nYAxisPrecision;

	CWinThread	*pThread = AfxBeginThread( ThreadImportData, this );
}

UINT CGraphCtrl::ThreadImportData( LPVOID pParam )
{
	CGraphCtrl*	pCtrl = (CGraphCtrl*)pParam;
	pCtrl->ImportDataFunction();
	return 0;
}

//뭔가 쓰레드가 이상하다. 멀티쓰레드로 수정해보자.
void CGraphCtrl::ImportDataFunction()
{
	int i, j;
	int	nSize = m_vtData[0].size();
	double *x = new double[nSize];
	double *y = new double[nSize];
	CString str;

	for ( i = 0; i < nSize; i++ )
		x[i] = _tstof( m_vtData[0][i] );

	ReleaseAll();

	//이미 설정된 Y의 min, max가 있다면 해제시켜주고 다시 잡는다.
	ResetYRange();

	SetXAxisPrecision( m_nXAxisPrecision );
	SetYAxisPrecision( m_nYAxisPrecision );

	SetXAxis( x, nSize );
	if ( m_nXAxisPrecision == 0 )
	{
		double dLevel = nSize - 1;

		while ( (double)m_rChart.Width() / dLevel < 30.0 )
			dLevel *= 0.9;

		SetXAxisLevel( dLevel - 1 );
	}

	for ( j = 1; j < GRAPH_LINE_MAX; j++ )
	{
		if ( m_vtData[j].size() == 0 ) continue;

		for ( i = 0; i < nSize; i++ )
		{
			if ( m_vtData[j][i] == "1.#IO" )
				y[i] = std::numeric_limits<double>::infinity();
			else if ( m_vtData[j][i] == "-1.#IO" )
				y[i] = -std::numeric_limits<double>::infinity();
			else
				y[i] = _tstof( m_vtData[j][i] );
			//str.Format( "x:%d, y[%d]:%d", i, j, i );
			//m_sInfoText[1] = str;
			//InvalidateRect( m_rTitleArea );
		}

		str.Format(_T("line%d"), j - 1 );
		SetLineTitle( j - 1, str );
		SetLineColor(j - 1, get_default_color(j-1));
		SetLineData( j - 1, y, nSize );
	}

	delete [] x;
	delete [] y;

	for ( i = 0; i < GRAPH_LINE_MAX + 1; i++ )
		m_vtData[i].clear();

	if ( m_nImportedMarkX == 0 )
		return;

	m_nMarkX = 0;
	for ( i = 0; i < m_nImportedMarkX; i++ )
	{
		if ( m_dImportedMarkX[i] >= m_XAxis.pData[0] && m_dImportedMarkX[i] <= m_XAxis.pData[m_XAxis.nData-1] )
			m_dMarkX[m_nMarkX++] = m_dImportedMarkX[i];
	}
}

void CGraphCtrl::OnMenuCopyDataToClipboard()
{
	int		i, j;
	int		nXData;
	int		nStart = 0;
	CString str;
	CString sWhole = _T("");

	if ( m_nGraphStyle == GRAPH_STYLE_FLOW )
	{
		for ( i = 0; i < GRAPH_LINE_MAX; i++ )
		{	
			if ( m_pLineData[i] != NULL && m_pLineData[i]->pData != NULL && m_pLineData[i]->nData )
			{
				nStart = m_pLineData[i]->nData - m_nDataNumOnScreen;
				if ( nStart < 0 )
					nStart = 0;
				break;
			}
		}

		nXData = m_pLineData[i]->nData;
	}
	else
	{
		nXData = m_XAxis.nData;
	}

	for ( i = nStart; i < nXData; i++ )
	{
		if ( m_nGraphStyle == GRAPH_STYLE_FLOW )
			str.Format(_T("%d"), i - nStart );
		else
			str.Format( m_sXAxisPrecision, m_XAxis.pData[i] );
		sWhole = sWhole + str + _T("\t");

		for ( j = 0; j < GRAPH_LINE_MAX; j++ )
		{
			if ( m_pLineData[j] == NULL ) continue;

			if ( m_pLineData[j]->pData == NULL ) continue;

			if ( m_pLineData[j]->pData[i] == std::numeric_limits<double>::infinity() ||
				 m_pLineData[j]->pData[i] == -std::numeric_limits<double>::infinity() )
				 str.Format(_T("%.3f"), m_pLineData[j]->pData[i] );
			else
				str.Format( m_sYAxisPrecision, m_pLineData[j]->pData[i] );

			if ( j == 0 )
				sWhole = sWhole + str;
			else
				sWhole = sWhole + _T("\t") + str;
		}

		sWhole += _T("\n");
	}

	//x축 mark 정보 저장
	if ( m_nMarkX > 0 )
	{
		sWhole += _T("_mark_x_\n");

		for ( i = 0; i < m_nMarkX; i++ )
		{
			str.Format(_T("%f\n"), m_dMarkX[i] );
			sWhole += str;
		}
	}


	HGLOBAL glob = GlobalAlloc( GMEM_FIXED, sizeof(TCHAR) * sWhole.GetLength() );
	memcpy( glob, sWhole, sizeof(TCHAR) * sWhole.GetLength() );
	OpenClipboard();
	EmptyClipboard();
#ifdef _UNICODE
	SetClipboardData(CF_UNICODETEXT,glob);
#else
	SetClipboardData(CF_TEXT,glob);
#endif
	CloseClipboard();
	MessageBeep( 0 );
}

void CGraphCtrl::OnMenuCopyImageToClipboard()
{
	m_pToolTip->UpdateTipText(_T(""), this );

	HBITMAP hBitmap = CaptureWindowToBitmap( m_hWnd );

	OpenClipboard();
	EmptyClipboard();
	SetClipboardData( CF_BITMAP, hBitmap );
	CloseClipboard();
	MessageBeep( 0 );
}

void CGraphCtrl::SetLineTitle( int nIndex, CString sTitle )	//라인데이터 타이틀 변경
{
	CGraphLineData*	pLineData = GetLineDataPtr( nIndex );

	if ( pLineData )
	{
		_stprintf( pLineData->sTitle, _T("%s\0"), sTitle );
		Invalidate();
	}
}

void CGraphCtrl::SetLineWidth( int nIndex, int nWidth )
{
	CGraphLineData*	pLineData = GetLineDataPtr( nIndex );

	if ( pLineData )
	{
		pLineData->nLineWidth = nWidth;
		Invalidate();
	}
}

void CGraphCtrl::SetLineStyle( int nIndex, int nStyle )
{
	CGraphLineData*	pLineData = GetLineDataPtr( nIndex );

	if ( !pLineData )
		return;

	if ( nStyle < GRAPH_STYLE_LINE || nStyle > GRAPH_STYLE_FLOW )
		return;

	pLineData->nLineStyle = nStyle;
	Invalidate();
}

void CGraphCtrl::SetBarWidth( int nIndex, int nWidth )
{
	CGraphLineData*	pLineData = GetLineDataPtr( nIndex );

	if ( !pLineData )
		return;

	if ( nWidth <= 0 )
		nWidth = 1;

	pLineData->nBarWidth = nWidth;
	Invalidate();
}

void CGraphCtrl::SetBarBrushStyle( int nIndex, int nStyle )
{
	CGraphLineData*	pLineData = GetLineDataPtr( nIndex );

	if ( !pLineData )
		return;

	pLineData->nBarBrushStyle = nStyle;
	Invalidate();
}

//n번째 라인의 속성을 설정하는 함수를 호출할 때 만약 해당 라인이 아직 생성 전이면
//해당 라인을 생성한 후에 속성값을 설정해준다.
CGraphLineData*	CGraphCtrl::GetLineDataPtr( int nIndex )
{
	if ( nIndex < 0 || nIndex >= GRAPH_LINE_MAX )
		return NULL;

	if ( m_pLineData[nIndex] == NULL )
	{
		m_pLineData[nIndex] = new CGraphLineData();
		//m_pLineData[nIndex]->crLine = random( 0, 9 );
	}

	return m_pLineData[nIndex];
}

void CGraphCtrl::SetLineColor( int nIndex, COLORREF crLine )
{
	CGraphLineData*	pLineData = GetLineDataPtr( nIndex );

	if ( pLineData )
	{
		pLineData->crLine = crLine;
		Invalidate();
	}
}


COLORREF CGraphCtrl::GetLineColor( int nIndex )
{
	if ( nIndex < 0 || nIndex >= GRAPH_LINE_MAX )
		return 0;

	if ( m_pLineData[nIndex] == NULL )
		return 0;

	return m_pLineData[nIndex]->crLine;
}

void CGraphCtrl::ShowDataLabel( int nIndex, bool bShow )
{
	if ( nIndex < 0 || nIndex >= GRAPH_LINE_MAX )
		return;
	
	CGraphLineData*	pLineData = GetLineDataPtr( nIndex );

	if ( pLineData )
	{
		pLineData->bShowDataLabel = bShow;
		Invalidate();
	}
}

void CGraphCtrl::SetDataLabelSize( int nIndex, int nSize )
{
	if ( nIndex < 0 || nIndex >= GRAPH_LINE_MAX )
		return;
	
	CGraphLineData*	pLineData = GetLineDataPtr( nIndex );

	if ( pLineData )
	{
		pLineData->nDataLabelSize = nSize;
		Invalidate();
	}
}

void CGraphCtrl::SetDataLabelColor( int nIndex, COLORREF cr )
{
	if ( nIndex < 0 || nIndex >= GRAPH_LINE_MAX )
		return;
	
	CGraphLineData*	pLineData = GetLineDataPtr( nIndex );

	if ( pLineData )
	{
		pLineData->crDataLabel = cr;
		Invalidate();
	}
}


void CGraphCtrl::SetMarkerSize( int nIndex, int nSize )
{
	if ( nIndex < 0 || nIndex >= GRAPH_LINE_MAX )
		return;
	
	CGraphLineData*	pLineData = GetLineDataPtr( nIndex );

	if ( pLineData )
	{
		pLineData->nMarkerSize = nSize;
		Invalidate();
	}
}

void CGraphCtrl::SetMarkerWidth( int nIndex, int nWidth )
{
	if ( nIndex < 0 || nIndex >= GRAPH_LINE_MAX )
		return;
	
	CGraphLineData*	pLineData = GetLineDataPtr( nIndex );

	if ( pLineData )
	{
		pLineData->nMarkerWidth = nWidth;
		Invalidate();
	}
}

void CGraphCtrl::SetMarkerShape( int nIndex, int nShape )
{
	if ( nIndex < 0 || nIndex >= GRAPH_LINE_MAX )
		return;

	CGraphLineData*	pLineData = GetLineDataPtr( nIndex );

	if ( pLineData )
	{
		pLineData->nMarkerShape = nShape;
		Invalidate();
	}
}

void CGraphCtrl::SetMarkerColor( int nIndex, COLORREF crMarker )
{
	if ( nIndex < 0 || nIndex >= GRAPH_LINE_MAX )
		return;

	CGraphLineData*	pLineData = GetLineDataPtr( nIndex );

	if ( pLineData )
	{
		pLineData->crMarker = crMarker;
		Invalidate();
	}
}

int CGraphCtrl::GetValidLineCount()
{
	int		i;
	int		nCount = 0;

	for ( i = 0; i < GRAPH_LINE_MAX; i++ )
	{
		if ( IsExistLineData(i) )
			nCount++;
	}

	return nCount;
}

//데이터가 존재하는 라인인지 판별
bool CGraphCtrl::IsExistLineData( int nIndex )
{
	if ( m_pLineData[nIndex] != NULL && m_pLineData[nIndex]->pData != NULL && m_pLineData[nIndex]->nData )
		return true;
	return false;
}

int CGraphCtrl::GetLineDataCount( int nIndex )
{
	if ( nIndex < 0 || nIndex >= GRAPH_LINE_MAX )
		return -1;

	if ( m_pLineData[nIndex] == NULL )
		return -1;

	return m_pLineData[nIndex]->nData;
}

void CGraphCtrl::DrawMarker( CDC* pDC, CRect r, CPen* penMarker, int nMarkerShape, int nWidth, COLORREF crMarker, COLORREF crFill /*= NULL_BRUSH*/ )
{
	if ( nMarkerShape == GRAPH_MARKER_NONE )
		return;

	CPen	*pOldPen = (CPen*)pDC->SelectObject( penMarker );

	if ( nMarkerShape == GRAPH_MARKER_RECT )
		DrawRectangle( pDC, r, crMarker, crFill, nWidth );
	else if ( nMarkerShape == GRAPH_MARKER_FILLED_RECT )
		pDC->FillSolidRect( r, crMarker );
	else if ( nMarkerShape == GRAPH_MARKER_FILLED_ELLIPSE )
		pDC->Ellipse( r );

	pDC->SelectObject( pOldPen );
}

#include <fstream>
#include <sstream>
void CGraphCtrl::OnDropFiles(HDROP hDropInfo)
{
	// TODO: Add your message handler code here and/or call default
	//drop된 텍스트 파일을 파싱해서 그래프로 표현해준다.
	TCHAR	chStr[MAX_PATH];
	CString str, sToken;

	DragQueryFile( hDropInfo, 0, chStr, FILENAME_MAX );
	
	str = chStr;
	
	if ( str.Right(4).MakeLower() != ".txt" )
		return;

	std::ifstream t(str);
	std::stringstream buffer;
	buffer << t.rdbuf();

	ImportTextData( CString(buffer.str().c_str()) );
/*
	FILE	*fp;
	char	*buffer = NULL;
	int		length;

	if ( (fp = fopen( str, "rt" )) == NULL )
	{
		AfxMessageBox( "fail to read the file." );
		return;
	}

	fseek( fp, 0L, SEEK_END );

#if defined(_WIN32)
    length = ftell(fp) - 1;
#elif defined (_WIN64)
    length = ftell(fp) - 1;
#else
    length = ftell(fp);
#endif // defined

	buffer = new char[length + 1];	// allocate memory for a buffer of appropriate dimension
	fread( buffer, sizeof(char), length, fp );		// read the whole file into the buffer
	buffer[length] = '\0';
	fclose( fp ); 

	ImportTextData( buffer );
	delete [] buffer;
*/
	CWnd::OnDropFiles(hDropInfo);
}

void CGraphCtrl::SetXAxisPrecision( int n )
{
	m_nXAxisPrecision = n;
	m_sXAxisPrecision.Format( _T("%%0.%df"), m_nXAxisPrecision );

	Invalidate();
}

void CGraphCtrl::SetYAxisPrecision( int n )
{
	m_nYAxisPrecision = n;
	m_sYAxisPrecision.Format( _T("%%0.%df"), m_nYAxisPrecision );

	Invalidate();
}


int detect_peak( 
					const double*   data, /* the data */  
					int             data_count, /* row count of data */  
					int*            emi_peaks, /* emission peaks will be put here */  
					int*            num_emi_peaks, /* number of emission peaks found */ 
					int             max_emi_peaks, /* maximum number of emission peaks */  
					int*            absop_peaks, /* absorption peaks will be put here */  
					int*            num_absop_peaks, /* number of absorption peaks found */ 
					int             max_absop_peaks, /* maximum number of absorption peaks 
													 */  
					double          delta, /* delta used for distinguishing peaks */ 
					int             emi_first /* should we search emission peak first of absorption peak first? */ 
				) 
{ 
	int     i; 
	double  mx; 
	double  mn; 
	int     mx_pos = 0; 
	int     mn_pos = 0; 
	int     is_detecting_emi = emi_first; 




	mx = data[0]; 
	mn = data[0]; 


	*num_emi_peaks = 0; 
	*num_absop_peaks = 0; 


	for(i = 1; i < data_count; ++i) 
	{ 
		if(data[i] > mx) 
		{ 
			mx_pos = i; 
			mx = data[i]; 
		} 
		if(data[i] < mn) 
		{ 
			mn_pos = i; 
			mn = data[i]; 
		} 


		if(is_detecting_emi && 
			data[i] < mx - delta) 
		{ 
			if(*num_emi_peaks >= max_emi_peaks) /* not enough spaces */ 
				return 1; 


			emi_peaks[*num_emi_peaks] = mx_pos; 
			++ (*num_emi_peaks); 


			is_detecting_emi = 0; 


			i = mx_pos - 1; 


			mn = data[mx_pos]; 
			mn_pos = mx_pos; 
		} 
		else if((!is_detecting_emi) && 
			data[i] > mn + delta) 
		{ 
			if(*num_absop_peaks >= max_absop_peaks) 
				return 2; 


			absop_peaks[*num_absop_peaks] = mn_pos; 
			++ (*num_absop_peaks); 


			is_detecting_emi = 1; 

			i = mn_pos - 1; 


			mx = data[mn_pos]; 
			mx_pos = mn_pos; 
		} 
	} 


	return 0; 
} 

void CGraphCtrl::StartGraphDemo( bool bStart, bool bContinue )
{
#define MAX_PEAK    4
	int         emi_peaks[MAX_PEAK]; 
	int         absorp_peaks[MAX_PEAK]; 
	int         emi_count = 0; 
	int         absorp_count = 0; 
	double		delta = 50.5;
	int			emission_first = 0;

	detect_peak( m_pLineData[0]->pData, m_pLineData[0]->nData, emi_peaks, &emi_count, MAX_PEAK, absorp_peaks,&absorp_count, MAX_PEAK, delta, emission_first );

	CRect		r;
	CClientDC	dc(this);
	CPoint		pt;
	for ( int i = 0; i < emi_count; i++ )
	{
		pt = GetScreenPosFromX(emi_peaks[i], 0);
		r = CRect( pt, pt );
		r.InflateRect( 3, 3 );
		dc.FillSolidRect(r, red );
	}

	return;
	m_bStartDemo = bStart;

	if ( bStart )
	{
		if ( bContinue )
		{
			if ( m_nGraphStyle == GRAPH_STYLE_FLOW )
				SetTimer( TIMER_START_DEMO, 100, NULL );
			else
				SetTimer( TIMER_START_DEMO, 1000, NULL );
		}
		else
		{
			GenerateDemoData();
		}
	}
	else
	{
		KillTimer( TIMER_START_DEMO );
	}
}
