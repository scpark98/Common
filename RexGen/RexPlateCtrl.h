/*
�νĵ� ���� ��ȣ�� ���� ��ȣ�ǰ� �����ϰ� ǥ���ϱ� ���� ��Ʈ���̸�
Plate_Type�� Plate_Size�� ���� ���� �� ����� �����ȴ�.
�� �� ���� ���ο��� ��ȣ���� ũ��, ���� �� ���������� ���� �����ȴ�.
*/

#pragma once

// CRexPlateCtrl

#define PLATE_TYPE_NUM		5
#define VEHICLE_TYPE_NUM	6

enum Plate_Type { PLATE_UNKNOWN, PLATE_GREEN, PLATE_ORANGE, PLATE_WHITE, PLATE_YELLOW };
enum Plate_Size { PLATE_WIDE, PLATE_RECTANGULAR };
enum Vehicle_Type { VEHICLE_UNKNOWN, VEHICLE_SEDAN, VEHICLE_VAN, VEHICLE_TRUCK, VEHICLE_SPECIAL, VEHICLE_CONSTRUCTION };
					//�̺з�, �¿�, ����, ȭ��, Ư��, �Ǽ����

//��� ��ȣ��
#define WHITE_TEXT		RGB(  32,  32,  32 )
#define WHITE_BACK		RGB( 243, 238, 235 )

//��� ��ȣ��
#define GREEN_TEXT		RGB( 208, 225, 207 )
#define GREEN_BACK		RGB( 66, 97, 81 )

//����� ��ȣ��
#define YELLOW_TEXT		RGB(  32,  32,  32 )
#define YELLOW_BACK		RGB( 239, 187,  86 )

//����� ��ȣ��
#define ORANGE_TEXT		RGB( 188, 187, 152 )
#define ORANGE_BACK		RGB( 109, 66, 47 )


struct PLATE_INFO
{
	char	sPlate[20];		//������ȣ
	int		nType;			//��ȣ�� ����(���󺰷� ���, ��Ȳ, ���, ���)
	int		nSize;			//���̵������� ���簢������
	int		nVehicleType;	//�ڵ��� �������� ���� ���� �з� �ڵ�(0-�̺з�, 1-�¿�, 2-����, 3-ȭ��, 4-Ư��, 5-�Ǽ����)
	char	sVehicleTypeName[20];
	COLORREF crBack;		//��ȣ�� ����
	COLORREF crText;		//��ȣ�� ���� ����
	int		nAvg;			//��� ��ǥ������ ��� ��Ⱚ
	float	fReliability;	//���� ��ȣ���� �ν� �ŷڵ�

	int		r, g, b, H, S, L;
	COLORREF	crMajor;


	char*	GetVehicleTypeName()
	{
		switch ( nVehicleType )
		{
			case VEHICLE_SEDAN			:	return "�¿���";
			case VEHICLE_VAN			:	return "������";
			case VEHICLE_TRUCK			:	return "ȭ����";
			case VEHICLE_SPECIAL		:	return "Ư����";
			case VEHICLE_CONSTRUCTION	:	return "�Ǽ����";
		}

		return "�� �� ����";
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

	//�ΰ����� ǥ�� ���� ����(RexWatchLPR������ �ν� �ŷڵ��� ǥ���ϰ� �ִ�)
	void			ShowInfoText( bool bShow = true );
	void			SetInfoTextSize( int nSize );
	void			SetInfoTextAlign( UINT uFormat );

protected:
	BOOL			RegisterWindowClass();


	PLATE_INFO		m_PlateInfo;
	COLORREF		m_crBack;		//��Ʈ���� �⺻ ����
	bool			m_bTransparent;	//�⺻ ������ �ƿ� ĥ���� ������ ��ȣ�Ǹ� �׷����Ƿ� ���� ȿ���� ����ϰ� ǥ���� �� �ִ�.

	double			m_dPlateRatio;

	CString			m_sLPR0;
	CString			m_sLPR1;
	CString			m_sRegion;		//��� ���̵��� ��� �������� ���η� ǥ�õȴ�.
	COLORREF		m_crPlateText;	//��ȣ�� ���ڻ�
	
	bool			m_bUseStandardPlateTextColor;	//ǥ�� ��ȣ�� ���ڻ��� ������� �ǻ� ����� ��ȣ�� ������ �������(�⺻ true)
	bool			m_bUseStandardPlateBackColor;	//ǥ�� ��ȣ�� ������ ������� �ǻ� ����� ��ȣ�� ������ �������(�⺻ true)

	CFont			m_Font0;
	CFont			m_Font1;
	CString			m_sFaceName;
	int				m_nSunken;		//0:none, 1~:thickness

	//�ΰ����� ǥ�� ����
	bool			m_bShowInfo;
	CString			m_sInfo;		//�ΰ�����
	CFont			m_Font_Info;	//�ΰ������� ǥ���ϱ� ���� ���� ��Ʈ
	UINT			m_uInfoFormat;	



protected:
	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnPaint();
	afx_msg BOOL OnEraseBkgnd(CDC* pDC);
	afx_msg void OnLButtonDblClk(UINT nFlags, CPoint point);
	afx_msg void OnRButtonDblClk(UINT nFlags, CPoint point);
};


