/*
인식된 차량 번호를 실제 번호판과 유사하게 표시하기 위한 컨트롤이며
Plate_Type과 Plate_Size에 의해 색상 및 모양이 결정된다.
위 두 값은 메인에서 번호판의 크기, 비율 및 색상정보를 통해 결정된다.
*/

#pragma once

// CRexPlateCtrl

#define PLATE_TYPE_NUM		5
#define VEHICLE_TYPE_NUM	6

enum Plate_Type { PLATE_UNKNOWN, PLATE_GREEN, PLATE_ORANGE, PLATE_WHITE, PLATE_YELLOW };
enum Plate_Size { PLATE_WIDE, PLATE_RECTANGULAR };
enum Vehicle_Type { VEHICLE_UNKNOWN, VEHICLE_SEDAN, VEHICLE_VAN, VEHICLE_TRUCK, VEHICLE_SPECIAL, VEHICLE_CONSTRUCTION };
					//미분류, 승용, 승합, 화물, 특수, 건설기계

//흰색 번호판
#define WHITE_TEXT		RGB(  32,  32,  32 )
#define WHITE_BACK		RGB( 243, 238, 235 )

//녹색 번호판
#define GREEN_TEXT		RGB( 208, 225, 207 )
#define GREEN_BACK		RGB( 66, 97, 81 )

//노란색 번호판
#define YELLOW_TEXT		RGB(  32,  32,  32 )
#define YELLOW_BACK		RGB( 239, 187,  86 )

//노란색 번호판
#define ORANGE_TEXT		RGB( 188, 187, 152 )
#define ORANGE_BACK		RGB( 109, 66, 47 )


struct PLATE_INFO
{
	char	sPlate[20];		//차량번호
	int		nType;			//번호판 종류(색상별로 녹색, 주황, 흰색, 노랑)
	int		nSize;			//와이드형인지 직사각형인지
	int		nVehicleType;	//자동차 관리법에 의한 정식 분류 코드(0-미분류, 1-승용, 2-승합, 3-화물, 4-특수, 5-건설기계)
	char	sVehicleTypeName[20];
	COLORREF crBack;		//번호판 배경색
	COLORREF crText;		//번호판 글자 색상
	int		nAvg;			//배경 대표색상의 평균 밝기값
	float	fReliability;	//차량 번호판의 인식 신뢰도

	int		r, g, b, H, S, L;
	COLORREF	crMajor;


	char*	GetVehicleTypeName()
	{
		switch ( nVehicleType )
		{
			case VEHICLE_SEDAN			:	return "승용차";
			case VEHICLE_VAN			:	return "승합차";
			case VEHICLE_TRUCK			:	return "화물차";
			case VEHICLE_SPECIAL		:	return "특장차";
			case VEHICLE_CONSTRUCTION	:	return "건설기계";
		}

		return "알 수 없음";
	}
};

class CRexPlateCtrl : public CWnd
{
	DECLARE_DYNAMIC(CRexPlateCtrl)

public:
	CRexPlateCtrl();
	virtual ~CRexPlateCtrl();

	void			SetBackColor( COLORREF crBack ) { m_crBack = crBack; Invalidate(); }
	void			SetPlate( PLATE_INFO* pPlateInfo );
	void			SetSunken( int nSunken ) { m_nSunken = nSunken; Invalidate(); }
	void			SetTransparent( bool bTrans ) { m_bTransparent = bTrans; Invalidate(); }

	bool			GetUseStandardPlateTextColor() { return m_bUseStandardPlateTextColor; }
	void			SetUseStandardPlateTextColor( bool bUse ) { m_bUseStandardPlateTextColor = bUse; Invalidate(); }
	bool			GetUseStandardPlateBackColor() { return m_bUseStandardPlateBackColor; }
	void			SetUseStandardPlateBackColor( bool bUse ) { m_bUseStandardPlateBackColor = bUse; Invalidate(); }

	COLORREF		GetLightenColor(const COLORREF crColor, BYTE byIncreaseVal);
	COLORREF		GetDarkenColor(const COLORREF crColor, BYTE byReduceVal);

	//부가정보 표시 설정 관련(RexWatchLPR에서는 인식 신뢰도를 표시하고 있다)
	void			ShowInfoText( bool bShow = true );
	void			SetInfoTextSize( int nSize );
	void			SetInfoTextAlign( UINT uFormat );

protected:
	BOOL			RegisterWindowClass();


	PLATE_INFO		m_PlateInfo;
	COLORREF		m_crBack;		//컨트롤의 기본 배경색
	bool			m_bTransparent;	//기본 배경색을 아예 칠하지 않으면 번호판만 그려지므로 투명 효과를 비슷하게 표현할 수 있다.

	double			m_dPlateRatio;

	CString			m_sLPR0;
	CString			m_sLPR1;
	CString			m_sRegion;		//노란 와이드의 경우 지역명이 세로로 표시된다.
	COLORREF		m_crPlateText;	//번호판 글자색
	
	bool			m_bUseStandardPlateTextColor;	//표준 번호판 글자색을 사용할지 실사 검출된 번호판 배경색을 사용할지(기본 true)
	bool			m_bUseStandardPlateBackColor;	//표준 번호판 배경색을 사용할지 실사 검출된 번호판 배경색을 사용할지(기본 true)

	CFont			m_Font0;
	CFont			m_Font1;
	CString			m_sFaceName;
	int				m_nSunken;		//0:none, 1~:thickness

	//부가정보 표시 관련
	bool			m_bShowInfo;
	CString			m_sInfo;		//부가정보
	CFont			m_Font_Info;	//부가정보를 표시하기 위한 작은 폰트
	UINT			m_uInfoFormat;	



protected:
	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnPaint();
	afx_msg BOOL OnEraseBkgnd(CDC* pDC);
	afx_msg void OnLButtonDblClk(UINT nFlags, CPoint point);
	afx_msg void OnRButtonDblClk(UINT nFlags, CPoint point);
};


