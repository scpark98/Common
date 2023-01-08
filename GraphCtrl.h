
#pragma once

#include <afxwin.h>
#include <afxcmn.h>

#include <limits>
#include <vector>

#include "MemoryDC.h"
#include "AutoFont.h"
#include "Functions.h"

#define USE_WIN32INPUTBOX				1

#if USE_WIN32INPUTBOX
#include "messagebox/Win32InputBox/Win32InputBox.h"
#endif

//#define _CRT_SECURE_NO_WARNINGS
#pragma warning(disable: 4996)


#define GRAPH_LINE_MAX					10		//하나의 컨트롤에 최대 10개의 Y 라인데이터만 허용.
#define GRAPH_DATA_MAX					1024	//한 라인에 담을 수 있는 데이터 최대 개수. 사실 그래프 스타일이 GRAPH_STYLE_FLOW일때만 해당된다.
#define GRAPH_MARKX_MAX					100		//x축에 별도 표시할 x값들의 위치 최대 개수

#define GRAPH_TITLE_COLOR				RGB( 180, 184, 181 )
#define GRAPH_AXIS_TITLE_COLOR			RGB( 212, 210, 202 )
#define GRAPH_LINE_COLOR				RGB( 91, 155, 213 )

#define GRAPH_STYLE_LINE				0	//looks like a excel line chart. default setting.
#define GRAPH_STYLE_BAR					1	//looks like a excel bar chart. currently, does not support multiple data.
#define GRAPH_STYLE_FLOW				2	//looks like a performance graph of task manager

#define GRAPH_MARKER_NONE				0
#define GRAPH_MARKER_RECT				1
#define GRAPH_MARKER_FILLED_RECT		2
#define GRAPH_MARKER_FILLED_ELLIPSE		3

#define MESSAGE_GRAPHCTRL_MOUSE_EVENT	WM_USER + 9987


class CXAxis
{
public:
	CXAxis()
	{
		pData = NULL;
		nData = 0;
	};

	~CXAxis()
	{
		RemoveData();
	};

	//x축 데이터의 개수
	int		nData;				
	//x축 데이터
	double*	pData;

	void	RemoveData()
	{
		if ( pData )
		{
			delete [] pData;
			pData = NULL;
			nData = 0;
		}
	}

	double GetMin()
	{
		if ( pData == NULL || nData == 0 )
			return 0;
		return pData[0];
	}

	double GetMax()
	{
		if ( pData == NULL || nData == 0 )
			return 0;
		return pData[nData-1];
	}
};

class CGraphLineData
{
public:
	CGraphLineData()
	{
		pData			= NULL;
		nData			= 0;
		_tcscpy( sTitle, _T("") );
		nLineWidth		= 1;
		nLineStyle		= PS_SOLID;
		nBarWidth		= 5;
		nBarBrushStyle	= BS_SOLID;
		crLine			= GRAPH_LINE_COLOR;

		bShowDataLabel	= false;
		nDataLabelSize	= 16;
		crDataLabel		= RGB( 212, 210, 202 );	//same to m_crAxis by default

		nMarkerShape	= GRAPH_MARKER_NONE;
		nMarkerSize		= 3;
		nMarkerWidth	= 1;
		crMarker		= RGB( 255, 128, 0 );
	};

	~CGraphLineData()
	{
	};

	double		*pData;
	int			nData;			//데이터 개수
	TCHAR		sTitle[256];	//Y축에 표시할 축이름
	int			nLineWidth;
	int			nLineStyle;
	int			nBarWidth;
	int			nBarBrushStyle;
	COLORREF	crLine;

	//데이터 값 표시
	bool		bShowDataLabel;
	int			nDataLabelSize;
	COLORREF	crDataLabel;

	//데이터 마커 표식 관련
	int			nMarkerShape;	//데이터 포인트 모양
	int			nMarkerSize;	//default = 3;
	int			nMarkerWidth;	//default = 1;
	COLORREF	crMarker;

	TCHAR*		GetLineTitle() { return sTitle; }
};

// CGraphCtrl

class CGraphCtrl : public CWnd
{
	DECLARE_DYNAMIC(CGraphCtrl)

public:
	CGraphCtrl();
	virtual ~CGraphCtrl();

	UINT			GetGraphID() { return m_nGraphID; }
	void			SetGraphID( int id ) { m_nGraphID = id; }

	//popup menu 관련
	bool			m_bUsePopupMenu;
	void			UsePopupMenu( bool bUse ) { m_bUsePopupMenu = bUse; }
	enum POPUP_MENU_ITEM
	{
		ID_MENU_GRAPH_STYLE_LINE = 1,
		ID_MENU_GRAPH_STYLE_BAR,

		//ID_MENU_CHANGE_COLOR,
		ID_MENU_LINE_00,
		ID_MENU_LINE_01,
		ID_MENU_LINE_02,
		ID_MENU_LINE_03,
		ID_MENU_LINE_04,
		ID_MENU_LINE_05,
		ID_MENU_LINE_06,
		ID_MENU_LINE_07,
		ID_MENU_LINE_08,
		ID_MENU_LINE_09,

		ID_MENU_PAUSE,

		ID_MENU_SET_X_LEVEL,
		ID_MENU_SET_Y_LEVEL,

		ID_MENU_SET_Y_MIN,
		ID_MENU_SET_Y_MAX,

		ID_MENU_SHOW_X_AXIS,
		ID_MENU_SHOW_Y_AXIS,
		ID_MENU_SHOW_DATA_LABEL,

		ID_MENU_SHOW_MARKX,
		ID_MENU_SHOW_MOUSELINE,
		ID_MENU_SHOW_TOOLTIP,

		ID_MENU_COPY_DATA_TO_CLIPBOARD,
		ID_MENU_COPY_IMAGE_TO_CLIPBOARD,
		ID_MENU_PASTE_FROM_CLIPBOARD,

		ID_MENU_SAVE_TO_FILE,
		ID_MENU_SAVE_TO_IMAGE,

		ID_MENU_START_DEMO,

		ID_MENU_RESET,
	};
	void			OnPopupMenu( UINT nID );
	void			ShowMousePosLine( bool bShow = true ) { m_bShowMousePosLine = bShow; }
	void			ShowToolTip( bool bShow = true ) { m_bShowToolTip = bShow; }

	void			OnMenuSaveToFile();
	void			OnMenuSaveToImage();
	void			OnMenuCopyDataToClipboard();
	void			OnMenuCopyImageToClipboard();
	void			OnMenuPasteFromClipboard();
	void			SaveGraphToImage( CString sfile );

	//import data를 thread로 동작시키기 위한 멤버들.
	std::vector<std::vector <CString>> m_vtData;
	int				m_nImportedMarkX;
	double			m_dImportedMarkX[GRAPH_MARKX_MAX];
	void			ImportTextData( CString sData );	//parsing data from clipboard or file
	static UINT		ThreadImportData( LPVOID pParam );	//thread function
	void			ImportDataFunction();				//called by thread, and add graph data.


	int				m_nGraphStyle;	//default GRAPH_STYLE_LINE
	void			SetGraphStyle( int nStyle );
	bool			m_bPause;		//flow graph 그리기 일시정지

	void			ReleaseAll();	//x축, y축, 데이터 모두 해제

	bool			m_bDrawBorder;	//default = false;
	void			SetDrawBorder( bool border = true ) { m_bDrawBorder = border; }

	CString			m_sGraphTitle;
	bool			m_bGraphTitleBold;
	int				m_nGraphTitleSize;
	COLORREF		m_crGraphTitle;
	CRect			m_rTitleArea;
	CString			GetGraphTitle() { return m_sGraphTitle; }
	void			SetGraphTitle( CString sTitle ) { m_sGraphTitle = sTitle; Invalidate(); }
	void			SetGraphTitleBold( bool bold ) { m_bGraphTitleBold = bold; }
	void			SetGraphTitleSize( int size ) { m_nGraphTitleSize = size; }
	void			SetGraphTitleColor( COLORREF crTitle ) { m_crGraphTitle = crTitle; Invalidate(); }
	
	CString			m_sInfoText[2];		//타이틀 영역의 왼쪽 또는 오른쪽 끝에 부가 정보를 출력할 때 사용
	bool			m_bInfoText[2];	//default = false;
	void			SetShowInfoText( int idx, bool bShow ) { m_bInfoText[idx] = bShow; }
	void			SetInfoText( int idx, CString sInfo ) { m_sInfoText[idx] = sInfo; Invalidate(); }

	CRect			m_rChart;
	CRect			m_rMargin;		//client(graph) 영역과 차트 영역간의 테두리 여백이며 x, y축의 이름 또는 축값 표시 여부에 영향을 받지 않는다.
	void			SetChartMargin( CRect rMargin ) { m_rMargin = rMargin; }
	void			SetChartMargin( int l, int t, int r, int b ) { m_rMargin = CRect(l,t,r,b); }
	void			SetChartMargin( int n ) { m_rMargin = CRect(n,n,n,n); }
	int				m_nFixedLeftMargin;		//y축 레이블의 자릿수에 따라 가변적인 크기로 세팅되지만 때론 고정 크기로 표시해야 할 필요가 있다. default = -1(가변 크기가 적용됨).
	void			SetFixedLeftMargin( int margin ) { m_nFixedLeftMargin = margin; }
	int				m_nFixedRightMargin;	//x축 최대값의 자릿수에 따라 가변적인 크기로 세팅되지만 때론 고정 크기로 표시해야 할 필요가 있다. default = -1(가변 크기가 적용됨).
	void			SetFixedRightMargin( int margin ) { m_nFixedRightMargin = margin; }

	void			SetBackGraphColor( COLORREF crBack ) { m_crBackGraph = crBack; Invalidate(); }
	void			SetBackChartColor( COLORREF crBack ) { m_crBackChart = crBack; Invalidate(); }
	void			SetChartTextColor( COLORREF crText ) { m_crText = crText; Invalidate(); }

	bool			m_bShowCenter;

	//x, y 두 축에 대한 설정
	void			SetAxisColor( COLORREF crAxis ) { m_crAxis = crAxis; Invalidate(); }
	void			SetAxisTitleFontSize( int nFontSize ) { m_nAxisTitleFontSize = nFontSize; Invalidate(); }
	void			SetAxisFontSize( int nFontSize ) { m_nAxisFontSize = nFontSize; Invalidate(); }

//x축 관련
	CXAxis			m_XAxis;
	int				m_nXAxisLevel;			//x축 등분 개수(n level = n 등분)
	bool			m_bShowXAxisLabel;		//x축 레이블 표시 여부. default = true;
	bool			m_bDrawGridX;
	int				m_nXAxisPrecision;		//소수점 자릿수 크기
	CString			m_sXAxisPrecision;		//소수점 자릿수 크기
	CString			m_sXAxisTitle;
	COLORREF		m_crXAxisTitle;

	void			SetXAxis( int* pData, int nData );
	void			SetXAxis( double* pData, int nData = 0 );	//pData가 null이면 0 ~ (level-1)까지 기본형으로 x축이 표시된다.
	void			SetXAxisLevel( int nLevel = 10 );
	int				GetXAxisLevel() { return m_nXAxisLevel; }
	int				GetXAxisPrecision() { return m_nXAxisPrecision; }
	void			SetXAxisPrecision( int nDigit );
	double			GetXMin() { return m_XAxis.GetMin(); }
	double			GetXMax() { return m_XAxis.GetMax(); }

	//특정 x값 위치에 표시를 하고자 할 경우.
	bool			m_bShowMarkX;			//x축 특정값 위치에 표시를 해준다.
	int				m_nMarkX;				//마크 개수
	double			m_dMarkX[GRAPH_MARKX_MAX];
	COLORREF		m_crMarkX;
	void			AddMarkX( double x );
	void			UpdateMarkX( int idx, double x );
	void			DeleteAllMarkX() { m_nMarkX = 0; Invalidate(); }
	void			SetMarkXColor( COLORREF cr ) { m_crMarkX = cr; Invalidate(); }

	//x축 특정값 구간에 표시를 해준다.
	void			SetZone( double x1, double x2 ) { m_dZoneX1 = x1; m_dZoneX2 = x2; Invalidate(); }
	void			SetXAxisTitle( CString sXTitle ) { m_sXAxisTitle = sXTitle; Invalidate(); }
	void			SetXAxisTitleColor( COLORREF cr ) { m_crXAxisTitle = cr; Invalidate(); }
	void			ShowXAxisLabel( bool bShowXLabel = true ) { m_bShowXAxisLabel = bShowXLabel; Invalidate(); }

//grid 관련
	void			SetGridColor( COLORREF crGrid ) { m_crGrid = crGrid; Invalidate(); }
	void			DrawGridX( bool bDraw = true ) { m_bDrawGridX = bDraw; Invalidate(); }
	void			DrawGridY( bool bDraw = true ) { m_bDrawGridY = bDraw; Invalidate(); }


//y축
	double			m_dYMinValue;			//실제 라인 데이터들 중 최소 y값
	double			m_dYMaxValue;			//실제 라인 데이터들 중 최대 y값
	int				m_nDataNumOnScreen;		//flow타입일때 화면에 표시하는 데이터의 개수. Y가 동적 변위일때 화면에 표시되는 데이터에서 min, max를 찾게 해야 한다.
	int				m_nYAxisLevel;			//y축 등분 개수(n level = n 등분)
	int				m_nYAxisPrecision;
	CString			m_sYAxisPrecision;
	double			m_dYMinimumValue;		//y축에 표시할 최소값.
	double			m_dYMaximumValue;		//y축에 표시할 최대값. 이 값이 0 이하이면 라인 데이터들 중 최대값을 사용한다.(m_dYMaxValue)
	CString			m_sYAxisTitle;
	COLORREF		m_crYAxisTitle;
	double			m_dYValueMargin;		//y축이 가변 범위일때 miny, maxy가 차트 맨 하단과 최상단에 표시되면 가독성이 떨어져서 상하 마진을 준다. 기본값=0.5%, 범위 = 0.0% ~ 0.5%
	void			ShowYAxisLabel( bool bShowYLabel = true ) { m_bShowYAxisLabel = bShowYLabel; Invalidate(); }
	void			SetYAxisTitle( CString sYTitle ) { m_sYAxisTitle = sYTitle; Invalidate(); }
	void			SetYAxisTitleColor( COLORREF cr ) { m_crYAxisTitle = cr; Invalidate(); }
	void			SetYAxisLevel( int nLevel = 10 ) { m_nYAxisLevel = nLevel; } //y축을 n등분하여 표시한다.
	void			GetYMinMaxValue();		//라인 데이터들 중 최소값, 최대값을 찾는다. 라인 데이터 추가/삭제시 매번 해줘야 함.
	void			SetYValueMargin( double dMargin, bool bInvalidate = false );
	int				GetYAxisPrecision() { return m_nYAxisPrecision; }

	//nDigit = number of floating point
	void			SetYAxisPrecision( int nDigit );

	bool			m_bShowLegends;			//차트안에 범례 표시 여부. 기본 false.
	void			ShowLegends( bool bShow = true ) { m_bShowLegends = bShow; Invalidate(); }

	//y축의 min, max가 고정된 그래프일때 범위밖의 데이터도 그릴지 안그릴지 여부. 기본 = true
	bool			m_bDrawOutOfRange;
	void			SetDrawOutOfRange( bool bDraw = true ) { m_bDrawOutOfRange = bDraw; }

	double			GetYValueNearX( double x, int nLineIndex );	//x값과 가장 가까운 위치의 y값 리턴

	//Y변위의 min, max를 fix시키는 함수로서 이 값이 설정되면 y변위는 변하지 않는다.
	//이 값을 설정하지 않으면 y변위는 추가된 데이터들 중에서 min, max를 찾아 자동으로 변위가 변하게 된다.
	void			SetYMinimumValue( double dMinimumValue );
	void			SetYMaximumValue( double dMaximumValue );
	void			SetYRange( double dMin, double dMax );
	void			ResetYRange();
	double			GetYMinimumValue() { return m_dYMinValue; }
	double			GetYMaximumValue() { return m_dYMaxValue; }

	//라인 데이터 관련
	CGraphLineData*	*m_pLineData;
	void			ReleaseLineData( int nIndex );
	bool			m_bShowYAxisLabel;		//y축 레이블 값 표시 여부. default = true;
	bool			m_bDrawGridY;
	//화면 좌표에 해당하는 x, y값을 얻는다.
	bool			GetXYFromScreenPos( int nIndex, CPoint point, double& x, double& y );
	//x값에 해당하는 화면상의 픽셀 좌표를 얻는다.
	CPoint			GetScreenPosFromX( double x, int nLineIndex = -1 );

	//chart style
	void			SetLineData( int nIndex, int* pData, int nData, bool bRedraw = true );
	void			SetLineData( int nIndex, double* pData, int nData, bool bRedraw = true );
	void			SetLineData( int nIndex, int nX, double dYData, bool bRedraw = true );		//라인데이터의 특정 값을 원하는 값으로 변경한다.

	//flow graph style
	void			AddLineData( int nIndex, double dValue );

	//
	void			ShowDataLabel( int nIndex, bool bShow = true );
	void			SetDataLabelSize( int nIndex, int nSize );
	void			SetDataLabelColor( int nIndex, COLORREF cr );

	//line data marker
	void			SetMarkerShape( int nIndex, int nShape );
	void			SetMarkerSize( int nIndex, int nSize );
	void			SetMarkerWidth( int nIndex, int nWidth );
	void			SetMarkerColor( int nIndex, COLORREF crMarker );
	void			DrawMarker( CDC* pDC, CRect r, CPen *penMarker, int nMarkerShape, int nWidth, COLORREF crMarker, COLORREF crFill = NULL_BRUSH );

	CGraphLineData*	DeleteLineData( int nIndex );	//라인 변수 주소값 및 그래프 속성은 유지한 채 데이터만 지운다.
	void			DeleteAllLineData();			//차트 속성 및 라인 속성은 유지한 채 데이터만 지운다.
	int				GetValidLineCount();			//데이터가 존재하는 유효 라인의 전체 개수 리턴.
	bool			IsExistLineData( int nIndex );	//데이터가 존재하는 라인인지 판별
	int				GetLineDataCount( int nIndex );	//한 라인에서의 데이터 개수 리턴.
	CGraphLineData*	GetLineDataPtr( int nIndex );	//n번째 라인 데이터의 시작 주소를 리턴한다. 아직 추가되기 전의 라인 데이터라면 추가해준다. 세팅값을 먼저 세팅할 때 쓰인다.


	COLORREF		GetLineColor( int nIndex );
	void			SetLineColor( int nIndex, COLORREF crLine );
	void			SetLineTitle( int nIndex, CString sTitle );	//라인데이터 타이틀 변경
	void			SetLineWidth( int nIndex, int nWidth );
	void			SetLineStyle( int nIndex, int nStyle );
	void			SetBarWidth( int nIndex, int nWidth );
	void			SetBarBrushStyle( int nIndex, int nStyle );

	//for demo
	bool			m_bStartDemo;
	void			GenerateDemoData();
	void			StartGraphDemo( bool bStart = true, bool bContinue = true );

protected:
	BOOL			RegisterWindowClass();


	bool			m_bDrawGraphBack;
	bool			m_bDrawChartBack;

	UINT			m_nGraphID;		//여러개의 그래프를 포함한 앱에서 이벤트 처리할 때 필요. GetDlgCtrlID()와는 다름. default=0.



	//Zone 관련
	double			m_dZoneX1;		//시작값
	double			m_dZoneX2;		//끝값


	COLORREF		m_crBackGraph;	//그래프 전체 영역 배경색
	COLORREF		m_crBackChart;	//차트 영역 배경색
	COLORREF		m_crText;		//텍스트 색상
	COLORREF		m_crAxis;		//축 표시 라인 색상
	COLORREF		m_crGrid;		//보조축 표시 라인 색상

	int				m_nAxisTitleFontSize;	//축 제목 글자 크기
	int				m_nAxisFontSize;		//축 글자 크기

	bool			m_bShowDataLabel;

	//마우스 위치 라인 표시 여부
	bool			m_bShowMousePosLine;
	bool			m_bMouseIsInChart;
	bool			m_bLButtonDown;
	CPoint			m_ptLButtonDown;
	void			OnMenuShowMouseLine();

	//툴팁 관련
	CToolTipCtrl*	m_pToolTip;
	CString			m_sToolTip;
	bool			m_bShowToolTip;
	CPoint			m_ptOldPoint;
	void			OnMenuShowToolTip();
	BOOL			OnTTNNeedText(UINT id, NMHDR* pTTTStruct,  LRESULT* pResult);


protected:
	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnPaint();
	afx_msg BOOL OnEraseBkgnd(CDC* pDC);
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
	afx_msg void OnNcMouseMove(UINT nHitTest, CPoint point);
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnLButtonDblClk(UINT nFlags, CPoint point);
	afx_msg void OnRButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnRButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnRButtonDblClk(UINT nFlags, CPoint point);
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnTimer(UINT_PTR nIDEvent);
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
	virtual void PreSubclassWindow();
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	afx_msg void OnDropFiles(HDROP hDropInfo);
};


