///////////////////////////////////////////////////////////////////////////////
// NMEAParser.h: 
// Desctiption:	interface for the CNMEAParser class.
//
// Notes:
//		NMEA Messages parsed:
//			GPGGA, GPGSA, GPGSV, GPRMB, GPRMC, GPZDA
///////////////////////////////////////////////////////////////////////////////
// Copyright (C) 1998-2002 VGPS
// All rights reserved.
//
// VGPS licenses this source code for use within your application in
// object form. This source code is not to be distributed in any way without
// prior written permission from VGPS.
//
// Visual Source Safe: $Revision: 6 $
///////////////////////////////////////////////////////////////////////////////
#ifndef _NMEAPARSER_H_
#define _NMEAPARSER_H_

#include <sys/timeb.h>	//for __timeb32 structure
#include <time.h>

//////////////////////////////////////////////////////////////////////
#define MAX_GPS_DATA	6000

#define Swap32BE(x) ((((x) & 0xff000000) >> 24) | (((x) & 0xff0000) >> 8) | (((x) & 0xff00) << 8) | (((x) & 0xff) << 24))
#define Swap16BE(x) ((((x) & 0xff00) >> 8)| (((x) & 0xff) << 8))

typedef struct
{
	int	AccX;
	int	AccY;
	int	AccZ;
	float gData;
	double reserved[10];
} ACC_RAW_DATA;

typedef struct
{
	char		gpsGPRMC[100];
	char		gpsGPGGA[100];
} GPS_RAW_DATA;

typedef struct
{
	GPS_RAW_DATA	gpsRawData;
	ACC_RAW_DATA	accRawData;
} RAW_DATA;

typedef struct
{
	int		speed;
	double	latitude;
	double	longitude;
} GPS_DATA;

typedef struct
{
	int			dataCount;
	RAW_DATA	rawData[MAX_GPS_DATA];
	GPS_DATA	gpsData[MAX_GPS_DATA];
} USER_DATA;



enum NP_STATE {
	NP_STATE_SOM =				0,		// Search for start of message
	NP_STATE_CMD,						// Get command
	NP_STATE_DATA,						// Get data
	NP_STATE_CHECKSUM_1,				// Get first checksum character
	NP_STATE_CHECKSUM_2,				// get second checksum character
};

#define NP_MAX_CMD_LEN			8		// maximum command length (NMEA address)
#define NP_MAX_DATA_LEN			256		// maximum data length
#define NP_MAX_CHAN				36		// maximum number of channels
#define NP_WAYPOINT_ID_LEN		32		// waypoint max string len

//////////////////////////////////////////////////////////////////////
class CNPSatInfo
{
public:
	WORD	m_wPRN;						//
	WORD	m_wSignalQuality;			//
	BOOL	m_bUsedInSolution;			//
	WORD	m_wAzimuth;					//
	WORD	m_wElevation;				//
};
//////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////
class CNMEAParser  
{
private:
	NP_STATE m_nState;					// Current state protocol parser is in
	BYTE m_btChecksum;					// Calculated NMEA sentence checksum
	BYTE m_btReceivedChecksum;			// Received NMEA sentence checksum (if exists)
	WORD m_wIndex;						// Index used for command and data
	BYTE m_pCommand[NP_MAX_CMD_LEN];	// NMEA command
	BYTE m_pData[NP_MAX_DATA_LEN];		// NMEA data

public:

	USER_DATA	m_UserData;
	CString		m_sMP4Filename;
	bool		m_bExistTimeStamp;
	bool		m_bIncludeADASMP4;		//일반 동영상인지 아니면 ADAS 처리 결과가 포함된 동영상 파일인지 구분
	int			m_pDataAddress[MAX_GPS_DATA];
	short		m_pDataSize[MAX_GPS_DATA];
	double		m_dTimeStamp[MAX_GPS_DATA];

	//step=1이면 모든 데이터를 배열에 담고(620기준 초당 5개. 1분 = 300개),
	//step=4이면 신호 4개 당 1개의 데이터를 배열에 저장한다.(mobis기준 초당20개 신호 -> 4개당 1개 -> 초당 5개 저장)
	bool		MP4Parser( CString sfile, int nVideoChannel = 1, int nAudioChannel = 1, int step = 1 );
	void		GPSParser(int index, USER_DATA* UserData);	//gps original 패킷을 파싱한다.
	void		GPSTokenize(int index, USER_DATA* UserData);//위도, 경도, 속도, x, y, z으로 들어오니 파싱해서 각 필드에 저장한다.

	void		ReleaseData();

	time_t		_mkgmtime(const struct tm *tm);

	DWORD m_dwCommandCount;				// number of NMEA commands received (processed or not processed)

	//
	// GPGGA Data
	//
	BYTE m_btGGAHour;					//
	BYTE m_btGGAMinute;					//
	BYTE m_btGGASecond;					//
	double m_dGGALatitude;				// < 0 = South, > 0 = North
	double m_dGGALongitude;				// < 0 = West, > 0 = East
	BYTE m_btGGAGPSQuality;				// 0 = fix not available, 1 = GPS sps mode, 2 = Differential GPS, SPS mode, fix valid, 3 = GPS PPS mode, fix valid
	BYTE m_btGGANumOfSatsInUse;			//
	double m_dGGAHDOP;					//
	double m_dGGAAltitude;				// Altitude: mean-sea-level (geoid) meters
	DWORD m_dwGGACount;					//
	int m_nGGAOldVSpeedSeconds;			//
	double m_dGGAOldVSpeedAlt;			//
	double m_dGGAVertSpeed;				//

	//
	// GPGSA
	//
	BYTE m_btGSAMode;					// M = manual, A = automatic 2D/3D
	BYTE m_btGSAFixMode;				// 1 = fix not available, 2 = 2D, 3 = 3D
	WORD m_wGSASatsInSolution[NP_MAX_CHAN]; // ID of sats in solution
	double m_dGSAPDOP;					//
	double m_dGSAHDOP;					//
	double m_dGSAVDOP;					//
	DWORD m_dwGSACount;					//

	//
	// GPGSV
	//
	BYTE m_btGSVTotalNumOfMsg;			//
	WORD m_wGSVTotalNumSatsInView;		//
	CNPSatInfo m_GSVSatInfo[NP_MAX_CHAN];	//
	DWORD m_dwGSVCount;					//

	//
	// GPRMB
	//
	BYTE m_btRMBDataStatus;				// A = data valid, V = navigation receiver warning
	double m_dRMBCrosstrackError;		// nautical miles
	BYTE m_btRMBDirectionToSteer;		// L/R
	CHAR m_lpszRMBOriginWaypoint[NP_WAYPOINT_ID_LEN]; // Origin Waypoint ID
	CHAR m_lpszRMBDestWaypoint[NP_WAYPOINT_ID_LEN]; // Destination waypoint ID
	double m_dRMBDestLatitude;			// destination waypoint latitude
	double m_dRMBDestLongitude;			// destination waypoint longitude
	double m_dRMBRangeToDest;			// Range to destination nautical mi
	double m_dRMBBearingToDest;			// Bearing to destination, degrees true
	double m_dRMBDestClosingVelocity;	// Destination closing velocity, knots
	BYTE m_btRMBArrivalStatus;			// A = arrival circle entered, V = not entered
	DWORD m_dwRMBCount;					//

	//
	// GPRMC
	//
	BYTE m_btRMCHour;					//
	BYTE m_btRMCMinute;					//
	BYTE m_btRMCSecond;					//
	BYTE m_btRMCDataValid;				// A = Data valid, V = navigation rx warning
	double m_dRMCLatitude;				// current latitude
	double m_dRMCLongitude;				// current longitude
	double m_dRMCGroundSpeed;			// speed over ground, knots
	double m_dRMCCourse;				// course over ground, degrees true
	BYTE m_btRMCDay;					//
	BYTE m_btRMCMonth;					//
	WORD m_wRMCYear;					//
	double m_dRMCMagVar;				// magnitic variation, degrees East(+)/West(-)
	DWORD m_dwRMCCount;					//

	//
	// GPZDA
	//
	BYTE m_btZDAHour;					//
	BYTE m_btZDAMinute;					//
	BYTE m_btZDASecond;					//
	BYTE m_btZDADay;					// 1 - 31
	BYTE m_btZDAMonth;					// 1 - 12
	WORD m_wZDAYear;					//
	BYTE m_btZDALocalZoneHour;			// 0 to +/- 13
	BYTE m_btZDALocalZoneMinute;		// 0 - 59
	DWORD m_dwZDACount;					//

public:
	void ProcessGPZDA(BYTE *pData);
	void ProcessGPRMC(BYTE *pData);
	void ProcessGPRMB(BYTE *pData);
	void ProcessGPGSV(BYTE *pData);
	void ProcessGPGSA(BYTE *pData);
	void ProcessGPGGA(BYTE *pData);
	BOOL IsSatUsedInSolution(WORD wSatID);
	void Reset();
	BOOL GetField(BYTE *pData, BYTE *pField, int nFieldNum, int nMaxFieldLen);
	BOOL ProcessCommand(BYTE *pCommand, BYTE *pData);
	void ProcessNMEA(BYTE btData);
	BOOL ParseBuffer(BYTE *pBuff, DWORD dwLen);
	CNMEAParser();
	virtual ~CNMEAParser();
};

//////////////////////////////////////////////////////////////////////
#endif // _NMEAPARSER_H_
