
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


#define GRAPH_LINE_MAX					10		//�ϳ��� ��Ʈ�ѿ� �ִ� 10���� Y ���ε����͸� ���.
#define GRAPH_DATA_MAX					1024	//�� ���ο� ���� �� �ִ� ������ �ִ� ����. ��� �׷��� ��Ÿ���� GRAPH_STYLE_FLOW�϶��� �ش�ȴ�.
#define GRAPH_MARKX_MAX					100		//x�࿡ ���� ǥ���� x������ ��ġ �ִ� ����

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

	//x�� �������� ����
	int		nData;				
	//x�� ������
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
	int			nData;			//������ ����
	TCHAR		sTitle[256];	//Y�࿡ ǥ���� ���̸�
	int			nLineWidth;
	int			nLineStyle;
	int			nBarWidth;
	int			nBarBrushStyle;
	COLORREF	crLine;

	//������ �� ǥ��
	bool		bShowDataLabel;
	int			nDataLabelSize;
	COLORREF	crDataLabel;

	//������ ��Ŀ ǥ�� ����
	int			nMarkerShape;	//������ ����Ʈ ���
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

	//popup menu ����
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

	//import data�� thread�� ���۽�Ű�� ���� �����.
	std::vector<std::vector <CString>> m_vtData;
	int				m_nImportedMarkX;
	double			m_dImportedMarkX[GRAPH_MARKX_MAX];
	void			ImportTextData( CString sData );	//parsing data from clipboard or file
	static UINT		ThreadImportData( LPVOID pParam );	//thread function
	void			ImportDataFunction();				//called by thread, and add graph data.


	int				m_nGraphStyle;	//default GRAPH_STYLE_LINE
	void			SetGraphStyle( int nStyle );
	bool			m_bPause;		//flow graph �׸��� �Ͻ�����

	void			ReleaseAll();	//x��, y��, ������ ��� ����

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
	
	CString			m_sInfoText[2];		//Ÿ��Ʋ ������ ���� �Ǵ� ������ ���� �ΰ� ������ ����� �� ���
	bool			m_bInfoText[2];	//default = false;
	void			SetShowInfoText( int idx, bool bShow ) { m_bInfoText[idx] = bShow; }
	void			SetInfoText( int idx, CString sInfo ) { m_sInfoText[idx] = sInfo; Invalidate(); }

	CRect			m_rChart;
	CRect			m_rMargin;		//client(graph) ������ ��Ʈ �������� �׵θ� �����̸� x, y���� �̸� �Ǵ� �ప ǥ�� ���ο� ������ ���� �ʴ´�.
	void			SetChartMargin( CRect rMargin ) { m_rMargin = rMargin; }
	void			SetChartMargin( int l, int t, int r, int b ) { m_rMargin = CRect(l,t,r,b); }
	void			SetChartMargin( int n ) { m_rMargin = CRect(n,n,n,n); }
	int				m_nFixedLeftMargin;		//y�� ���̺��� �ڸ����� ���� �������� ũ��� ���õ����� ���� ���� ũ��� ǥ���ؾ� �� �ʿ䰡 �ִ�. default = -1(���� ũ�Ⱑ �����).
	void			SetFixedLeftMargin( int margin ) { m_nFixedLeftMargin = margin; }
	int				m_nFixedRightMargin;	//x�� �ִ밪�� �ڸ����� ���� �������� ũ��� ���õ����� ���� ���� ũ��� ǥ���ؾ� �� �ʿ䰡 �ִ�. default = -1(���� ũ�Ⱑ �����).
	void			SetFixedRightMargin( int margin ) { m_nFixedRightMargin = margin; }

	void			SetBackGraphColor( COLORREF crBack ) { m_crBackGraph = crBack; Invalidate(); }
	void			SetBackChartColor( COLORREF crBack ) { m_crBackChart = crBack; Invalidate(); }
	void			SetChartTextColor( COLORREF crText ) { m_crText = crText; Invalidate(); }

	bool			m_bShowCenter;

	//x, y �� �࿡ ���� ����
	void			SetAxisColor( COLORREF crAxis ) { m_crAxis = crAxis; Invalidate(); }
	void			SetAxisTitleFontSize( int nFontSize ) { m_nAxisTitleFontSize = nFontSize; Invalidate(); }
	void			SetAxisFontSize( int nFontSize ) { m_nAxisFontSize = nFontSize; Invalidate(); }

//x�� ����
	CXAxis			m_XAxis;
	int				m_nXAxisLevel;			//x�� ��� ����(n level = n ���)
	bool			m_bShowXAxisLabel;		//x�� ���̺� ǥ�� ����. default = true;
	bool			m_bDrawGridX;
	int				m_nXAxisPrecision;		//�Ҽ��� �ڸ��� ũ��
	CString			m_sXAxisPrecision;		//�Ҽ��� �ڸ��� ũ��
	CString			m_sXAxisTitle;
	COLORREF		m_crXAxisTitle;

	void			SetXAxis( int* pData, int nData );
	void			SetXAxis( double* pData, int nData = 0 );	//pData�� null�̸� 0 ~ (level-1)���� �⺻������ x���� ǥ�õȴ�.
	void			SetXAxisLevel( int nLevel = 10 );
	int				GetXAxisLevel() { return m_nXAxisLevel; }
	int				GetXAxisPrecision() { return m_nXAxisPrecision; }
	void			SetXAxisPrecision( int nDigit );
	double			GetXMin() { return m_XAxis.GetMin(); }
	double			GetXMax() { return m_XAxis.GetMax(); }

	//Ư�� x�� ��ġ�� ǥ�ø� �ϰ��� �� ���.
	bool			m_bShowMarkX;			//x�� Ư���� ��ġ�� ǥ�ø� ���ش�.
	int				m_nMarkX;				//��ũ ����
	double			m_dMarkX[GRAPH_MARKX_MAX];
	COLORREF		m_crMarkX;
	void			AddMarkX( double x );
	void			UpdateMarkX( int idx, double x );
	void			DeleteAllMarkX() { m_nMarkX = 0; Invalidate(); }
	void			SetMarkXColor( COLORREF cr ) { m_crMarkX = cr; Invalidate(); }

	//x�� Ư���� ������ ǥ�ø� ���ش�.
	void			SetZone( double x1, double x2 ) { m_dZoneX1 = x1; m_dZoneX2 = x2; Invalidate(); }
	void			SetXAxisTitle( CString sXTitle ) { m_sXAxisTitle = sXTitle; Invalidate(); }
	void			SetXAxisTitleColor( COLORREF cr ) { m_crXAxisTitle = cr; Invalidate(); }
	void			ShowXAxisLabel( bool bShowXLabel = true ) { m_bShowXAxisLabel = bShowXLabel; Invalidate(); }

//grid ����
	void			SetGridColor( COLORREF crGrid ) { m_crGrid = crGrid; Invalidate(); }
	void			DrawGridX( bool bDraw = true ) { m_bDrawGridX = bDraw; Invalidate(); }
	void			DrawGridY( bool bDraw = true ) { m_bDrawGridY = bDraw; Invalidate(); }


//y��
	double			m_dYMinValue;			//���� ���� �����͵� �� �ּ� y��
	double			m_dYMaxValue;			//���� ���� �����͵� �� �ִ� y��
	int				m_nDataNumOnScreen;		//flowŸ���϶� ȭ�鿡 ǥ���ϴ� �������� ����. Y�� ���� �����϶� ȭ�鿡 ǥ�õǴ� �����Ϳ��� min, max�� ã�� �ؾ� �Ѵ�.
	int				m_nYAxisLevel;			//y�� ��� ����(n level = n ���)
	int				m_nYAxisPrecision;
	CString			m_sYAxisPrecision;
	double			m_dYMinimumValue;		//y�࿡ ǥ���� �ּҰ�.
	double			m_dYMaximumValue;		//y�࿡ ǥ���� �ִ밪. �� ���� 0 �����̸� ���� �����͵� �� �ִ밪�� ����Ѵ�.(m_dYMaxValue)
	CString			m_sYAxisTitle;
	COLORREF		m_crYAxisTitle;
	double			m_dYValueMargin;		//y���� ���� �����϶� miny, maxy�� ��Ʈ �� �ϴܰ� �ֻ�ܿ� ǥ�õǸ� �������� �������� ���� ������ �ش�. �⺻��=0.5%, ���� = 0.0% ~ 0.5%
	void			ShowYAxisLabel( bool bShowYLabel = true ) { m_bShowYAxisLabel = bShowYLabel; Invalidate(); }
	void			SetYAxisTitle( CString sYTitle ) { m_sYAxisTitle = sYTitle; Invalidate(); }
	void			SetYAxisTitleColor( COLORREF cr ) { m_crYAxisTitle = cr; Invalidate(); }
	void			SetYAxisLevel( int nLevel = 10 ) { m_nYAxisLevel = nLevel; } //y���� n����Ͽ� ǥ���Ѵ�.
	void			GetYMinMaxValue();		//���� �����͵� �� �ּҰ�, �ִ밪�� ã�´�. ���� ������ �߰�/������ �Ź� ����� ��.
	void			SetYValueMargin( double dMargin, bool bInvalidate = false );
	int				GetYAxisPrecision() { return m_nYAxisPrecision; }

	//nDigit = number of floating point
	void			SetYAxisPrecision( int nDigit );

	bool			m_bShowLegends;			//��Ʈ�ȿ� ���� ǥ�� ����. �⺻ false.
	void			ShowLegends( bool bShow = true ) { m_bShowLegends = bShow; Invalidate(); }

	//y���� min, max�� ������ �׷����϶� �������� �����͵� �׸��� �ȱ׸��� ����. �⺻ = true
	bool			m_bDrawOutOfRange;
	void			SetDrawOutOfRange( bool bDraw = true ) { m_bDrawOutOfRange = bDraw; }

	double			GetYValueNearX( double x, int nLineIndex );	//x���� ���� ����� ��ġ�� y�� ����

	//Y������ min, max�� fix��Ű�� �Լ��μ� �� ���� �����Ǹ� y������ ������ �ʴ´�.
	//�� ���� �������� ������ y������ �߰��� �����͵� �߿��� min, max�� ã�� �ڵ����� ������ ���ϰ� �ȴ�.
	void			SetYMinimumValue( double dMinimumValue );
	void			SetYMaximumValue( double dMaximumValue );
	void			SetYRange( double dMin, double dMax );
	void			ResetYRange();
	double			GetYMinimumValue() { return m_dYMinValue; }
	double			GetYMaximumValue() { return m_dYMaxValue; }

	//���� ������ ����
	CGraphLineData*	*m_pLineData;
	void			ReleaseLineData( int nIndex );
	bool			m_bShowYAxisLabel;		//y�� ���̺� �� ǥ�� ����. default = true;
	bool			m_bDrawGridY;
	//ȭ�� ��ǥ�� �ش��ϴ� x, y���� ��´�.
	bool			GetXYFromScreenPos( int nIndex, CPoint point, double& x, double& y );
	//x���� �ش��ϴ� ȭ����� �ȼ� ��ǥ�� ��´�.
	CPoint			GetScreenPosFromX( double x, int nLineIndex = -1 );

	//chart style
	void			SetLineData( int nIndex, int* pData, int nData, bool bRedraw = true );
	void			SetLineData( int nIndex, double* pData, int nData, bool bRedraw = true );
	void			SetLineData( int nIndex, int nX, double dYData, bool bRedraw = true );		//���ε������� Ư�� ���� ���ϴ� ������ �����Ѵ�.

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

	CGraphLineData*	DeleteLineData( int nIndex );	//���� ���� �ּҰ� �� �׷��� �Ӽ��� ������ ä �����͸� �����.
	void			DeleteAllLineData();			//��Ʈ �Ӽ� �� ���� �Ӽ��� ������ ä �����͸� �����.
	int				GetValidLineCount();			//�����Ͱ� �����ϴ� ��ȿ ������ ��ü ���� ����.
	bool			IsExistLineData( int nIndex );	//�����Ͱ� �����ϴ� �������� �Ǻ�
	int				GetLineDataCount( int nIndex );	//�� ���ο����� ������ ���� ����.
	CGraphLineData*	GetLineDataPtr( int nIndex );	//n��° ���� �������� ���� �ּҸ� �����Ѵ�. ���� �߰��Ǳ� ���� ���� �����Ͷ�� �߰����ش�. ���ð��� ���� ������ �� ���δ�.


	COLORREF		GetLineColor( int nIndex );
	void			SetLineColor( int nIndex, COLORREF crLine );
	void			SetLineTitle( int nIndex, CString sTitle );	//���ε����� Ÿ��Ʋ ����
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

	UINT			m_nGraphID;		//�������� �׷����� ������ �ۿ��� �̺�Ʈ ó���� �� �ʿ�. GetDlgCtrlID()�ʹ� �ٸ�. default=0.



	//Zone ����
	double			m_dZoneX1;		//���۰�
	double			m_dZoneX2;		//����


	COLORREF		m_crBackGraph;	//�׷��� ��ü ���� ����
	COLORREF		m_crBackChart;	//��Ʈ ���� ����
	COLORREF		m_crText;		//�ؽ�Ʈ ����
	COLORREF		m_crAxis;		//�� ǥ�� ���� ����
	COLORREF		m_crGrid;		//������ ǥ�� ���� ����

	int				m_nAxisTitleFontSize;	//�� ���� ���� ũ��
	int				m_nAxisFontSize;		//�� ���� ũ��

	bool			m_bShowDataLabel;

	//���콺 ��ġ ���� ǥ�� ����
	bool			m_bShowMousePosLine;
	bool			m_bMouseIsInChart;
	bool			m_bLButtonDown;
	CPoint			m_ptLButtonDown;
	void			OnMenuShowMouseLine();

	//���� ����
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


