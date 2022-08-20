///////////////////////////////////////////////////////////////////////////////
// NMEAParser.cpp: 
// Desctiption:	Implementation of the CNMEAParser class.
///////////////////////////////////////////////////////////////////////////////
// Copyright (C) 1998-2002 VGPS
// All rights reserved.
//
// VGPS licenses this source code for use within your application in
// object form. This source code is not to be distributed in any way without
// prior written permission from VGPS.
//
// Visual Source Safe: $Revision: 8 $
///////////////////////////////////////////////////////////////////////////////
#include "stdafx.h"
#include "NMEAParser.h"

#pragma warning(disable:4996)	//discard => "error C4996: 'strcpy': This function or variable may be unsafe. Consider using strcpy_s instead. To disable deprecation, use _CRT_SECURE_NO_WARNINGS."

#define MAXFIELD	25		// maximum field length

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////
CNMEAParser::CNMEAParser()
{
	m_nState = NP_STATE_SOM;
	m_dwCommandCount = 0;
	Reset();
}

CNMEAParser::~CNMEAParser()
{
	ReleaseData();
}

///////////////////////////////////////////////////////////////////////////////
// ParseBuffer:	Parse the supplied buffer for NMEA sentence information. Since
//				the parser is a state machine, partial NMEA sentence data may
//				be supplied where the next time this method is called, the
//				rest of the partial NMEA sentence will complete	the sentence.
//
//				NOTE:
//
// Returned:	TRUE all the time....
///////////////////////////////////////////////////////////////////////////////
BOOL CNMEAParser::ParseBuffer(BYTE *pBuff, DWORD dwLen)
{
	for(DWORD i = 0; i < dwLen; i++)
	{
		ProcessNMEA(pBuff[i]);
	}
	return TRUE;
}

///////////////////////////////////////////////////////////////////////////////
// ProcessNMEA: This method is the main state machine which processes individual
//				bytes from the buffer and parses a NMEA sentence. A typical
//				sentence is constructed as:
//
//					$CMD,DDDD,DDDD,....DD*CS<CR><LF>
//
//				Where:
//						'$'			HEX 24 Start of sentence
//						'CMD'		Address/NMEA command
//						',DDDD'		Zero or more data fields
//						'*CS'		Checksum field
//						<CR><LF>	Hex 0d 0A End of sentence
//
//				When a valid sentence is received, this function sends the
//				NMEA command and data to the ProcessCommand method for
//				individual field parsing.
//
//				NOTE:
//						
///////////////////////////////////////////////////////////////////////////////
void CNMEAParser::ProcessNMEA(BYTE btData)
{
	switch(m_nState)
	{
		///////////////////////////////////////////////////////////////////////
		// Search for start of message '$'
		case NP_STATE_SOM :
			if(btData == '$')
			{
				m_btChecksum = 0;			// reset checksum
				m_wIndex = 0;				// reset index
				m_nState = NP_STATE_CMD;
			}
		break;

		///////////////////////////////////////////////////////////////////////
		// Retrieve command (NMEA Address)
		case NP_STATE_CMD :
			if(btData != ',' && btData != '*')
			{
				m_pCommand[m_wIndex++] = btData;
				m_btChecksum ^= btData;

				// Check for command overflow
				if(m_wIndex >= NP_MAX_CMD_LEN)
				{
					m_nState = NP_STATE_SOM;
				}
			}
			else
			{
				m_pCommand[m_wIndex] = '\0';	// terminate command
				m_btChecksum ^= btData;
				m_wIndex = 0;
				m_nState = NP_STATE_DATA;		// goto get data state
			}
		break;

		///////////////////////////////////////////////////////////////////////
		// Store data and check for end of sentence or checksum flag
		case NP_STATE_DATA :
			if(btData == '*') // checksum flag?
			{
				m_pData[m_wIndex] = '\0';
				m_nState = NP_STATE_CHECKSUM_1;
			}
			else // no checksum flag, store data
			{
				//
				// Check for end of sentence with no checksum
				//
				if(btData == '\r')
				{
					m_pData[m_wIndex] = '\0';
					ProcessCommand(m_pCommand, m_pData);
					m_nState = NP_STATE_SOM;
					return;
				}

				//
				// Store data and calculate checksum
				//
				m_btChecksum ^= btData;
				m_pData[m_wIndex] = btData;
				if(++m_wIndex >= NP_MAX_DATA_LEN) // Check for buffer overflow
				{
					m_nState = NP_STATE_SOM;
				}
			}
		break;

		///////////////////////////////////////////////////////////////////////
		case NP_STATE_CHECKSUM_1 :
			if( (btData - '0') <= 9)
			{
				m_btReceivedChecksum = (btData - '0') << 4;
			}
			else
			{
				m_btReceivedChecksum = (btData - 'A' + 10) << 4;
			}

			m_nState = NP_STATE_CHECKSUM_2;

		break;

		///////////////////////////////////////////////////////////////////////
		case NP_STATE_CHECKSUM_2 :
			if( (btData - '0') <= 9)
			{
				m_btReceivedChecksum |= (btData - '0');
			}
			else
			{
				m_btReceivedChecksum |= (btData - 'A' + 10);
			}

			//if(m_btChecksum == m_btReceivedChecksum)
			{
				ProcessCommand(m_pCommand, m_pData);
			}

			m_nState = NP_STATE_SOM;
		break;

		///////////////////////////////////////////////////////////////////////
		default : m_nState = NP_STATE_SOM;
	}
}

///////////////////////////////////////////////////////////////////////////////
// Process NMEA sentence - Use the NMEA address (*pCommand) and call the
// appropriate sentense data prossor.
///////////////////////////////////////////////////////////////////////////////
BOOL CNMEAParser::ProcessCommand(BYTE *pCommand, BYTE *pData)
{
	//
	// GPGGA
	//
	if( strcmp((char *)pCommand, "GPGGA") == NULL )
	{
		ProcessGPGGA(pData);
	}

	//
	// GPGSA
	//
	else if( strcmp((char *)pCommand, "GPGSA") == NULL )
	{
		ProcessGPGSA(pData);
	}

	//
	// GPGSV
	//
	else if( strcmp((char *)pCommand, "GPGSV") == NULL )
	{
		ProcessGPGSV(pData);
	}

	//
	// GPRMB
	//
	else if( strcmp((char *)pCommand, "GPRMB") == NULL )
	{
		ProcessGPRMB(pData);
	}

	//
	// GPRMC
	//
	else if( strcmp((char *)pCommand, "GPRMC") == NULL )
	{
		ProcessGPRMC(pData);
	}

	//
	// GPZDA
	//
	else if( strcmp((char *)pCommand, "GPZDA") == NULL )
	{
		ProcessGPZDA(pData);
	}

	m_dwCommandCount++;
	return TRUE;
}

///////////////////////////////////////////////////////////////////////////////
// Name:		GetField
//
// Description:	This function will get the specified field in a NMEA string.
//
// Entry:		BYTE *pData -		Pointer to NMEA string
//				BYTE *pField -		pointer to returned field
//				int nfieldNum -		Field offset to get
//				int nMaxFieldLen -	Maximum of bytes pFiled can handle
///////////////////////////////////////////////////////////////////////////////
BOOL CNMEAParser::GetField(BYTE *pData, BYTE *pField, int nFieldNum, int nMaxFieldLen)
{
	//
	// Validate params
	//
	if(pData == NULL || pField == NULL || nMaxFieldLen <= 0)
	{
		return FALSE;
	}

	//
	// Go to the beginning of the selected field
	//
	int i = 0;
	int nField = 0;
	while(nField != nFieldNum && pData[i])
	{
		if(pData[i] == ',')
		{
			nField++;
		}

		i++;

		if(pData[i] == NULL)
		{
			pField[0] = '\0';
			return FALSE;
		}
	}

	if(pData[i] == ',' || pData[i] == '*')
	{
		pField[0] = '\0';
		return FALSE;
	}

	//
	// copy field from pData to Field
	//
	int i2 = 0;
	while(pData[i] != ',' && pData[i] != '*' && pData[i])
	{
		pField[i2] = pData[i];
		i2++; i++;

		//
		// check if field is too big to fit on passed parameter. If it is,
		// crop returned field to its max length.
		//
		if(i2 >= nMaxFieldLen)
		{
			i2 = nMaxFieldLen-1;
			break;
		}
	}
	pField[i2] = '\0';

	return TRUE;
}

///////////////////////////////////////////////////////////////////////////////
// Reset: Reset all NMEA data to start-up default values.
///////////////////////////////////////////////////////////////////////////////
void CNMEAParser::Reset()
{
	int i;

	//
	// GPGGA Data
	//
	m_btGGAHour = 0;					//
	m_btGGAMinute = 0;					//
	m_btGGASecond = 0;					//
	m_dGGALatitude = 0.0;				// < 0 = South, > 0 = North 
	m_dGGALongitude = 0.0;				// < 0 = West, > 0 = East
	m_btGGAGPSQuality = 0;				// 0 = fix not available, 1 = GPS sps mode, 2 = Differential GPS, SPS mode, fix valid, 3 = GPS PPS mode, fix valid
	m_btGGANumOfSatsInUse = 0;			//
	m_dGGAHDOP = 0.0;					//
	m_dGGAAltitude = 0.0;				// Altitude: mean-sea-level (geoid) meters
	m_dwGGACount = 0;					//
	m_nGGAOldVSpeedSeconds = 0;			//
	m_dGGAOldVSpeedAlt = 0.0;			//
	m_dGGAVertSpeed = 0.0;				//

	//
	// GPGSA
	//
	m_btGSAMode = 'M';					// M = manual, A = automatic 2D/3D
	m_btGSAFixMode = 1;					// 1 = fix not available, 2 = 2D, 3 = 3D
	for(i = 0; i < NP_MAX_CHAN; i++)
	{
		m_wGSASatsInSolution[i] = 0;	// ID of sats in solution
	}
	m_dGSAPDOP = 0.0;					//
	m_dGSAHDOP = 0.0;					//
	m_dGSAVDOP = 0.0;					//
	m_dwGSACount = 0;					//

	//
	// GPGSV
	//
	m_btGSVTotalNumOfMsg = 0;			//
	m_wGSVTotalNumSatsInView = 0;		//
	for(i = 0; i < NP_MAX_CHAN; i++)
	{
		m_GSVSatInfo[i].m_wAzimuth = 0;
		m_GSVSatInfo[i].m_wElevation = 0;
		m_GSVSatInfo[i].m_wPRN = 0;
		m_GSVSatInfo[i].m_wSignalQuality = 0;
		m_GSVSatInfo[i].m_bUsedInSolution = FALSE;
	}
	m_dwGSVCount = 0;

	//
	// GPRMB
	//
	m_btRMBDataStatus = 'V';			// A = data valid, V = navigation receiver warning
	m_dRMBCrosstrackError = 0.0;		// nautical miles
	m_btRMBDirectionToSteer = '?';		// L/R
	m_lpszRMBOriginWaypoint[0] = '\0';	// Origin Waypoint ID
	m_lpszRMBDestWaypoint[0] = '\0';	// Destination waypoint ID
	m_dRMBDestLatitude = 0.0;			// destination waypoint latitude
	m_dRMBDestLongitude = 0.0;			// destination waypoint longitude
	m_dRMBRangeToDest = 0.0;			// Range to destination nautical mi
	m_dRMBBearingToDest = 0.0;			// Bearing to destination, degrees true
	m_dRMBDestClosingVelocity = 0.0;	// Destination closing velocity, knots
	m_btRMBArrivalStatus = 'V';			// A = arrival circle entered, V = not entered
	m_dwRMBCount = 0;					//

	//
	// GPRMC
	//
	m_btRMCHour = 0;					//
	m_btRMCMinute = 0;					//
	m_btRMCSecond = 0;					//
	m_btRMCDataValid = 'V';				// A = Data valid, V = navigation rx warning
	m_dRMCLatitude = 0.0;				// current latitude
	m_dRMCLongitude = 0.0;				// current longitude
	m_dRMCGroundSpeed = 0.0;			// speed over ground, knots
	m_dRMCCourse = 0.0;					// course over ground, degrees true
	m_btRMCDay = 1;						//
	m_btRMCMonth = 1;					//
	m_wRMCYear = 2000;					//
	m_dRMCMagVar = 0.0;					// magnitic variation, degrees East(+)/West(-)
	m_dwRMCCount = 0;					//

	//
	// GPZDA
	//
	m_btZDAHour = 0;					//
	m_btZDAMinute = 0;					//
	m_btZDASecond = 0;					//
	m_btZDADay = 1;						// 1 - 31
	m_btZDAMonth = 1;					// 1 - 12
	m_wZDAYear = 2000;					//
	m_btZDALocalZoneHour = 0;			// 0 to +/- 13
	m_btZDALocalZoneMinute = 0;			// 0 - 59
	m_dwZDACount = 0;					//

	//m_pDataSize = NULL;
	//m_pDataAddress = NULL;
	//m_UserData.gpsData = NULL;
	//m_UserData.rawData = NULL;
}

void CNMEAParser::ReleaseData()
{
	/*
	if ( m_pDataSize )
	{
		delete [] m_pDataSize;
		m_pDataSize = NULL;
	}

	if ( m_pDataAddress )
	{
		delete [] m_pDataAddress;
		m_pDataAddress = NULL;
	}

	if ( m_UserData.gpsData )
	{
		delete [] m_UserData.gpsData;
		m_UserData.gpsData = NULL;
	}

	if ( m_UserData.rawData )
	{
		delete [] m_UserData.rawData;
		m_UserData.rawData = NULL;
	}
	*/
}

///////////////////////////////////////////////////////////////////////////////
// Check to see if supplied satellite ID is used in the GPS solution.
// Retruned:	BOOL -	TRUE if satellate ID is used in solution
//						FALSE if not used in solution.
///////////////////////////////////////////////////////////////////////////////
BOOL CNMEAParser::IsSatUsedInSolution(WORD wSatID)
{
	if(wSatID == 0) return FALSE;
	for(int i = 0; i < 12; i++)
	{
		if(wSatID == m_wGSASatsInSolution[i])
		{
			return TRUE;
		}
	}

	return FALSE;
}


///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
void CNMEAParser::ProcessGPGGA(BYTE *pData)
{
	BYTE pField[MAXFIELD];
	CHAR pBuff[10];

	//
	// Time
	//
	if(GetField(pData, pField, 0, MAXFIELD))
	{
		// Hour
		pBuff[0] = pField[0];
		pBuff[1] = pField[1];
		pBuff[2] = '\0';
		m_btGGAHour = atoi(pBuff);

		// minute
		pBuff[0] = pField[2];
		pBuff[1] = pField[3];
		pBuff[2] = '\0';
		m_btGGAMinute = atoi(pBuff);

		// Second
		pBuff[0] = pField[4];
		pBuff[1] = pField[5];
		pBuff[2] = '\0';
		m_btGGASecond = atoi(pBuff);
	}

	//
	// Latitude
	//
	if(GetField(pData, pField, 1, MAXFIELD))
	{
		m_dGGALatitude = atof((CHAR *)pField+2) / 60.0;
		pField[2] = '\0';
		m_dGGALatitude += atof((CHAR *)pField);

	}
	if(GetField(pData, pField, 2, MAXFIELD))
	{
		if(pField[0] == 'S')
		{
			m_dGGALatitude = -m_dGGALatitude;
		}
	}

	//
	// Longitude
	//
	if(GetField(pData, pField, 3, MAXFIELD))
	{
		m_dGGALongitude = atof((CHAR *)pField+3) / 60.0;
		pField[3] = '\0';
		m_dGGALongitude += atof((CHAR *)pField);
	}
	if(GetField(pData, pField, 4, MAXFIELD))
	{
		if(pField[0] == 'W')
		{
			m_dGGALongitude = -m_dGGALongitude;
		}
	}

	//
	// GPS quality
	//
	if(GetField(pData, pField, 5, MAXFIELD))
	{
		m_btGGAGPSQuality = pField[0] - '0';
	}

	//
	// Satellites in use
	//
	if(GetField(pData, pField, 6, MAXFIELD))
	{
		pBuff[0] = pField[0];
		pBuff[1] = pField[1];
		pBuff[2] = '\0';
		m_btGGANumOfSatsInUse = atoi(pBuff);
	}

	//
	// HDOP
	//
	if(GetField(pData, pField, 7, MAXFIELD))
	{
		m_dGGAHDOP = atof((CHAR *)pField);
	}
	
	//
	// Altitude
	//
	if(GetField(pData, pField, 8, MAXFIELD))
	{
		m_dGGAAltitude = atof((CHAR *)pField);
	}

	//
	// Durive vertical speed (bonus)
	//
	int nSeconds = (int)m_btGGAMinute * 60 + (int)m_btGGASecond;
	if(nSeconds > m_nGGAOldVSpeedSeconds)
	{
		double dDiff = (double)(m_nGGAOldVSpeedSeconds-nSeconds);
		double dVal = dDiff/60.0;
		if(dVal != 0.0)
		{
			m_dGGAVertSpeed = (m_dGGAOldVSpeedAlt - m_dGGAAltitude) / dVal;
		}
	}
	m_dGGAOldVSpeedAlt = m_dGGAAltitude;
	m_nGGAOldVSpeedSeconds = nSeconds;

	m_dwGGACount++;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
void CNMEAParser::ProcessGPGSA(BYTE *pData)
{
	BYTE pField[MAXFIELD];
	CHAR pBuff[10];

	//
	// Mode
	//
	if(GetField(pData, pField, 0, MAXFIELD))
	{
		m_btGSAMode = pField[0];
	}

	//
	// Fix Mode
	//
	if(GetField(pData, pField, 1, MAXFIELD))
	{
		m_btGSAFixMode = pField[0] - '0';
	}

	//
	// Active satellites
	//
	for(int i = 0; i < 12; i++)
	{
		if(GetField(pData, pField, 2 + i, MAXFIELD))
		{
			pBuff[0] = pField[0];
			pBuff[1] = pField[1];
			pBuff[2] = '\0';
			m_wGSASatsInSolution[i] = atoi(pBuff);
		}
		else
		{
			m_wGSASatsInSolution[i] = 0;
		}
	}

	//
	// PDOP
	//
	if(GetField(pData, pField, 14, MAXFIELD))
	{
		m_dGSAPDOP = atof((CHAR *)pField);
	}
	else
	{
		m_dGSAPDOP = 0.0;
	}

	//
	// HDOP
	//
	if(GetField(pData, pField, 15, MAXFIELD))
	{
		m_dGSAHDOP = atof((CHAR *)pField);
	}
	else
	{
		m_dGSAHDOP = 0.0;
	}

	//
	// VDOP
	//
	if(GetField(pData, pField, 16, MAXFIELD))
	{
		m_dGSAVDOP = atof((CHAR *)pField);
	}
	else
	{
		m_dGSAVDOP = 0.0;
	}

	m_dwGSACount++;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
void CNMEAParser::ProcessGPGSV(BYTE *pData)
{
	INT nTotalNumOfMsg, nMsgNum;
	BYTE pField[MAXFIELD];

	//
	// Total number of messages
	//
	if(GetField(pData, pField, 0, MAXFIELD))
	{
		nTotalNumOfMsg = atoi((CHAR *)pField);

		//
		// Make sure that the nTotalNumOfMsg is valid. This is used to
		// calculate indexes into an array. I've seen corrept NMEA strings
		// with no checksum set this to large values.
		//
		if(nTotalNumOfMsg > 9 || nTotalNumOfMsg < 0) return; 
	}
	if(nTotalNumOfMsg < 1 || nTotalNumOfMsg*4 >= NP_MAX_CHAN)
	{
		return;
	}

	//
	// message number
	//
	if(GetField(pData, pField, 1, MAXFIELD))
	{
		nMsgNum = atoi((CHAR *)pField);

		//
		// Make sure that the message number is valid. This is used to
		// calculate indexes into an array
		//
		if(nMsgNum > 9 || nMsgNum < 0) return; 
	}

	//
	// Total satellites in view
	//
	if(GetField(pData, pField, 2, MAXFIELD))
	{
		m_wGSVTotalNumSatsInView = atoi((CHAR *)pField);
	}

	//
	// Satelite data
	//
	for(int i = 0; i < 4; i++)
	{
		// Satellite ID
		if(GetField(pData, pField, 3 + 4*i, MAXFIELD))
		{
			m_GSVSatInfo[i+(nMsgNum-1)*4].m_wPRN = atoi((CHAR *)pField);
		}
		else
		{
			m_GSVSatInfo[i+(nMsgNum-1)*4].m_wPRN = 0;
		}

		// Elevarion
		if(GetField(pData, pField, 4 + 4*i, MAXFIELD))
		{
			m_GSVSatInfo[i+(nMsgNum-1)*4].m_wElevation = atoi((CHAR *)pField);
		}
		else
		{
			m_GSVSatInfo[i+(nMsgNum-1)*4].m_wElevation = 0;
		}

		// Azimuth
		if(GetField(pData, pField, 5 + 4*i, MAXFIELD))
		{
			m_GSVSatInfo[i+(nMsgNum-1)*4].m_wAzimuth = atoi((CHAR *)pField);
		}
		else
		{
			m_GSVSatInfo[i+(nMsgNum-1)*4].m_wAzimuth = 0;
		}

		// SNR
		if(GetField(pData, pField, 6 + 4*i, MAXFIELD))
		{
			m_GSVSatInfo[i+(nMsgNum-1)*4].m_wSignalQuality = atoi((CHAR *)pField);
		}
		else
		{
			m_GSVSatInfo[i+(nMsgNum-1)*4].m_wSignalQuality = 0;
		}

		//
		// Update "used in solution" (m_bUsedInSolution) flag. This is base
		// on the GSA message and is an added convenience for post processing
		//
		m_GSVSatInfo[i+(nMsgNum-1)*4].m_bUsedInSolution = IsSatUsedInSolution(m_GSVSatInfo[i+(nMsgNum-1)*4].m_wPRN);
	}

	m_dwGSVCount++;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
void CNMEAParser::ProcessGPRMB(BYTE *pData)
{
	BYTE pField[MAXFIELD];

	//
	// Data status
	//
	if(GetField(pData, pField, 0, MAXFIELD))
	{
		m_btRMBDataStatus = pField[0];
	}
	else
	{
		m_btRMBDataStatus = 'V';
	}

	//
	// Cross track error
	//
	if(GetField(pData, pField, 1, MAXFIELD))
	{
		m_dRMBCrosstrackError = atof((CHAR *)pField);
	}
	else
	{
		m_dRMBCrosstrackError = 0.0;
	}

	//
	// Direction to steer
	//
	if(GetField(pData, pField, 2, MAXFIELD))
	{
		m_btRMBDirectionToSteer = pField[0];
	}
	else
	{
		m_btRMBDirectionToSteer = '?';
	}

	//
	// Orgin waypoint ID
	//
	if(GetField(pData, pField, 3, MAXFIELD))
	{
		strcpy(m_lpszRMBOriginWaypoint, (CHAR *)pField);
	}
	else
	{
		m_lpszRMBOriginWaypoint[0] = '\0';
	}

	//
	// Destination waypoint ID
	//
	if(GetField(pData, pField, 4, MAXFIELD))
	{
		strcpy(m_lpszRMBDestWaypoint, (CHAR *)pField);
	}
	else
	{
		m_lpszRMBDestWaypoint[0] = '\0';
	}

	//
	// Destination latitude
	//
	if(GetField(pData, pField, 5, MAXFIELD))
	{
		m_dRMBDestLatitude = atof((CHAR *)pField+2) / 60.0;
		pField[2] = '\0';
		m_dRMBDestLatitude += atof((CHAR *)pField);

	}
	if(GetField(pData, pField, 6, MAXFIELD))
	{
		if(pField[0] == 'S')
		{
			m_dRMBDestLatitude = -m_dRMBDestLatitude;
		}
	}

	//
	// Destination Longitude
	//
	if(GetField(pData, pField, 7, MAXFIELD))
	{
		m_dRMBDestLongitude = atof((CHAR *)pField+3) / 60.0;
		pField[3] = '\0';
		m_dRMBDestLongitude += atof((CHAR *)pField);
	}
	if(GetField(pData, pField, 8, MAXFIELD))
	{
		if(pField[0] == 'W')
		{
			m_dRMBDestLongitude = -m_dRMBDestLongitude;
		}
	}

	//
	// Range to destination nautical mi
	//
	if(GetField(pData, pField, 9, MAXFIELD))
	{
		m_dRMBRangeToDest = atof((CHAR *)pField);
	}
	else
	{
		m_dRMBCrosstrackError = 0.0;
	}

	//
	// Bearing to destination degrees true
	//
	if(GetField(pData, pField, 10, MAXFIELD))
	{
		m_dRMBBearingToDest = atof((CHAR *)pField);
	}
	else
	{
		m_dRMBBearingToDest = 0.0;
	}

	//
	// Closing velocity
	//
	if(GetField(pData, pField, 11, MAXFIELD))
	{
		m_dRMBDestClosingVelocity = atof((CHAR *)pField);
	}
	else
	{
		m_dRMBDestClosingVelocity = 0.0;
	}

	//
	// Arrival status
	//
	if(GetField(pData, pField, 12, MAXFIELD))
	{
		m_btRMBArrivalStatus = pField[0];
	}
	else
	{
		m_dRMBDestClosingVelocity = 'V';
	}

	m_dwRMBCount++;

}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
void CNMEAParser::ProcessGPRMC(BYTE *pData)
{
	CHAR pBuff[10];
	BYTE pField[MAXFIELD];

	//
	// Time
	//
	if(GetField(pData, pField, 0, MAXFIELD))
	{
		// Hour
		pBuff[0] = pField[0];
		pBuff[1] = pField[1];
		pBuff[2] = '\0';
		m_btRMCHour = atoi(pBuff);

		// minute
		pBuff[0] = pField[2];
		pBuff[1] = pField[3];
		pBuff[2] = '\0';
		m_btRMCMinute = atoi(pBuff);

		// Second
		pBuff[0] = pField[4];
		pBuff[1] = pField[5];
		pBuff[2] = '\0';
		m_btRMCSecond = atoi(pBuff);
	}

	//
	// Data valid
	//
	if(GetField(pData, pField, 1, MAXFIELD))
	{
		m_btRMCDataValid = pField[0];
	}
	else
	{
		m_btRMCDataValid = 'V';
	}

	//
	// latitude
	//
	if(GetField(pData, pField, 2, MAXFIELD))
	{
		m_dRMCLatitude = (atof((CHAR *)pField) - ((int)(atof((CHAR *)pField)/100)*100)) / 60.0;
		m_dRMCLatitude += (int)(atof((CHAR *)pField)/100);

	}
	if(GetField(pData, pField, 3, MAXFIELD))
	{
		if(pField[0] == 'S')
		{
			m_dRMCLatitude = -m_dRMCLatitude;
		}
	}

	//
	// Longitude
	//
	if(GetField(pData, pField, 4, MAXFIELD))
	{
		m_dRMCLongitude = (atof((CHAR *)pField) - ((int)(atof((CHAR *)pField)/100)*100)) / 60.0;
		m_dRMCLongitude += (int)(atof((CHAR *)pField)/100);
	}
	if(GetField(pData, pField, 5, MAXFIELD))
	{
		if(pField[0] == 'W')
		{
			m_dRMCLongitude = -m_dRMCLongitude;
		}
	}

	//
	// Ground speed
	//
	if(GetField(pData, pField, 6, MAXFIELD))
	{
		m_dRMCGroundSpeed = atof((CHAR *)pField);
	}
	else
	{
		m_dRMCGroundSpeed = 0.0;
	}

	//
	// course over ground, degrees true
	//
	if(GetField(pData, pField, 7, MAXFIELD))
	{
		m_dRMCCourse = atof((CHAR *)pField);
	}
	else
	{
		m_dRMCCourse = 0.0;
	}

	//
	// Date
	//
	if(GetField(pData, pField, 8, MAXFIELD))
	{
		// Day
		pBuff[0] = pField[0];
		pBuff[1] = pField[1];
		pBuff[2] = '\0';
		m_btRMCDay = atoi(pBuff);

		// Month
		pBuff[0] = pField[2];
		pBuff[1] = pField[3];
		pBuff[2] = '\0';
		m_btRMCMonth = atoi(pBuff);

		// Year (Only two digits. I wonder why?)
		pBuff[0] = pField[4];
		pBuff[1] = pField[5];
		pBuff[2] = '\0';
		m_wRMCYear = atoi(pBuff);
		m_wRMCYear += 2000;				// make 4 digit date -- What assumptions should be made here?
	}

	//
	// course over ground, degrees true
	//
	if(GetField(pData, pField, 9, MAXFIELD))
	{
		m_dRMCMagVar = atof((CHAR *)pField);
	}
	else
	{
		m_dRMCMagVar = 0.0;
	}
	if(GetField(pData, pField, 10, MAXFIELD))
	{
		if(pField[0] == 'W')
		{
			m_dRMCMagVar = -m_dRMCMagVar;
		}
	}

	m_dwRMCCount++;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
void CNMEAParser::ProcessGPZDA(BYTE *pData)
{
	CHAR pBuff[10];
	BYTE pField[MAXFIELD];

	//
	// Time
	//
	if(GetField(pData, pField, 0, MAXFIELD))
	{
		// Hour
		pBuff[0] = pField[0];
		pBuff[1] = pField[1];
		pBuff[2] = '\0';
		m_btZDAHour = atoi(pBuff);

		// minute
		pBuff[0] = pField[2];
		pBuff[1] = pField[3];
		pBuff[2] = '\0';
		m_btZDAMinute = atoi(pBuff);

		// Second
		pBuff[0] = pField[4];
		pBuff[1] = pField[5];
		pBuff[2] = '\0';
		m_btZDASecond = atoi(pBuff);
	}

	//
	// Day
	//
	if(GetField(pData, pField, 1, MAXFIELD))
	{
		m_btZDADay = atoi((CHAR *)pField);
	}
	else
	{
		m_btZDADay = 1;
	}

	//
	// Month
	//
	if(GetField(pData, pField, 2, MAXFIELD))
	{
		m_btZDAMonth = atoi((CHAR *)pField);
	}
	else
	{
		m_btZDAMonth = 1;
	}

	//
	// Year
	//
	if(GetField(pData, pField, 3, MAXFIELD))
	{
		m_wZDAYear = atoi((CHAR *)pField);
	}
	else
	{
		m_wZDAYear = 1;
	}

	//
	// Local zone hour
	//
	if(GetField(pData, pField, 4, MAXFIELD))
	{
		m_btZDALocalZoneHour = atoi((CHAR *)pField);
	}
	else
	{
		m_btZDALocalZoneHour = 0;
	}

	//
	// Local zone hour
	//
	if(GetField(pData, pField, 5, MAXFIELD))
	{
		m_btZDALocalZoneMinute = atoi((CHAR *)pField);
	}
	else
	{
		m_btZDALocalZoneMinute = 0;
	}

	m_dwZDACount++;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////


bool CNMEAParser::MP4Parser( CString sfile, int nVideoChannel, int nAudioChannel, int step )
{
	int		i;
	int		idx = 0;	//전체 gps data의 개수가 아닌 실제 저장하는 데이터 개수.
	FILE	*fp;
	int		size;
	char	type[6][4];
	int		trackCount=0; //0:video, 1:audia, 2:text
	int		textTrack = nVideoChannel + nAudioChannel;

	m_sMP4Filename = sfile;
	m_bIncludeADASMP4 = false;	//자막 데이터 영역에 ADAS 검출 결과가 저장되어 있는 동영상인지.
	m_bExistTimeStamp = false;

	memset( type,0,sizeof(type));
	memset( m_pDataAddress,0,sizeof(m_pDataAddress));
	memset(&m_UserData,0,sizeof(m_UserData));

	fp = fopen( sfile,_T("rb") );

	//4~7까지 4바이트가 "ftyp"이어야 mp4 파일이다.
	fseek(fp,4,SEEK_SET);
	fread(type[0],4,1,fp);
	if ( strcmp( type[0],"ftyp" )==0 )
	{
		fseek(fp,0,SEEK_SET);
	}
	else
	{
		fclose(fp);
		return false;
	}

	while(!feof(fp))
	{
		fread(&size,sizeof(int),1,fp);
		fread(type[0],4,1,fp);
		size = Swap32BE(size);
		if ( strcmp(type[0],"moov")==0 || strcmp(type[0],"mdia")==0 ||
			 strcmp(type[0],"minf")==0 || strcmp(type[0],"stbl")==0 ||
			 strcmp(type[0],"stco")==0 || (strcmp(type[0],"trak")==0 && trackCount == textTrack))
		{
			if(strcmp(type[0],"stco")==0)
			{
				fseek(fp,4,SEEK_CUR);
				fread(&m_UserData.dataCount,sizeof(int),1,fp);
				m_UserData.dataCount = Swap32BE(m_UserData.dataCount);

				if ( m_UserData.dataCount > MAX_GPS_DATA )
				{
					m_UserData.dataCount = MAX_GPS_DATA;
				}

				for( i = 0; i < m_UserData.dataCount; i++ )
				{
					fread( &m_pDataAddress[i], sizeof(int), 1, fp );
					m_pDataAddress[i] = Swap32BE( m_pDataAddress[i] );
				}

				for( i = 0; i < m_UserData.dataCount; i += step )
				{
					fseek( fp, m_pDataAddress[i], SEEK_SET );
					fread( &m_pDataSize[i], sizeof(short), 1, fp );
					m_pDataSize[i] = Swap16BE( m_pDataSize[i] );
					fread( &m_UserData.rawData[idx], m_pDataSize[i], 1, fp );

					//620 타입 (raw 데이터를 직접 파싱해야 한다.)
					if ( strncmp( m_UserData.rawData[idx].gpsRawData.gpsGPRMC, "$GPRMC,", 7 ) == 0 )
					{
						GPSParser( idx, &m_UserData );
					}
					//모비스 타입 (timestamp, 위도, 경도, 속도, x, y, z이 이미 파싱되어 온다)
					else
					{
						m_bExistTimeStamp = ( strncmp( m_UserData.rawData[idx].gpsRawData.gpsGPRMC, "+", 1 ) != 0 );
						GPSTokenize( idx, &m_UserData );
					}
					//TRACE( "%04d : %s\n", i, m_UserData.rawData[idx].gpsRawData.gpsGPRMC );
					//TRACE( "gpgga = %s\n", m_UserData.rawData[i].gpsRawData.gpsGPGGA );
					if ( strncmp( m_UserData.rawData[idx].gpsRawData.gpsGPGGA, "$GPGGA,", 7 ) == 0 )
						m_bIncludeADASMP4 = false;
					else
						m_bIncludeADASMP4 = true;
					idx++;
				}
				break;
			}
		}
		else
		{
			if(strcmp(type[0],"trak")==0)
				trackCount++;

			fseek(fp,size-8,SEEK_CUR);
		}
	}

	m_UserData.dataCount = idx;
	
	fclose(fp);
	return true;
}

time_t CNMEAParser::_mkgmtime(const struct tm *tm)
{
	// Month-to-day offset for non-leap-years.
	static const int month_day[12] =
	{0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334};

	// Most of the calculation is easy; leap years are the main difficulty.
	int month = tm->tm_mon % 12;
	int year = tm->tm_year + tm->tm_mon / 12;
	if (month < 0) {   // Negative values % 12 are still negative.
		month += 12;
		--year;
	}

	// This is the number of Februaries since 1900.
	const int year_for_leap = (month > 1) ? year + 1 : year;

	time_t rt = tm->tm_sec                             // Seconds
		+ 60 * (tm->tm_min                          // Minute = 60 seconds
		+ 60 * (tm->tm_hour                         // Hour = 60 minutes
		+ 24 * (month_day[month] + tm->tm_mday - 1  // Day = 24 hours
		+ 365 * (year - 70)                         // Year = 365 days
		+ (year_for_leap - 69) / 4                  // Every 4 years is     leap...
		- (year_for_leap - 1) / 100                 // Except centuries...
		+ (year_for_leap + 299) / 400)));           // Except 400s.
	return rt < 0 ? -1 : rt;
}

int get_utc_offset() {

  time_t zero = 24*60*60L;
  struct tm * timeptr;
  int gmtime_hours;

  /* get the local time for Jan 2, 1900 00:00 UTC */
  timeptr = localtime( &zero );
  gmtime_hours = timeptr->tm_hour;

  /* if the local time is the "day before" the UTC, subtract 24 hours
    from the hours to get the UTC offset */
  if( timeptr->tm_mday < 2 )
    gmtime_hours -= 24;

  return gmtime_hours;

}

/*
  the utc analogue of mktime,
  (much like timegm on some systems)
*/
time_t tm_to_time_t_utc( struct tm * timeptr ) {

  /* gets the epoch time relative to the local time zone,
  and then adds the appropriate number of seconds to make it UTC */
  return mktime( timeptr ) + get_utc_offset() * 3600;

}
/*
time_t my_timegm(struct tm *tm)
{
    time_t ret;
    char *tz;

    tz = getenv("TZ");
    setenv("TZ", "", 1);
    tzset();
    ret = mktime(tm);
    if (tz)
        setenv("TZ", tz, 1);
    else
        unsetenv("TZ");
    tzset();
    return ret;
}
*/

time_t timegm( struct tm *tm ) {
  time_t t = mktime( tm );
  return t + -5*60*60;//localtime( &t )->tm_gmtoff;
}

//timestamp, 위도, 경도, 속도, x, y, z으로 들어오니 파싱해서 각 필드에 저장한다.
//"+37.391220 +126.945482 041 +0234 +0296 +4087"
void CNMEAParser::GPSTokenize(int index, USER_DATA* UserData)
{
	int n = 0;
	CString sSub[7];

	for ( int i = 0; i < 7; i++ )
	{
		AfxExtractSubString( sSub[i], UserData->rawData[index].gpsRawData.gpsGPRMC, i, ' ' );
	}

	if ( m_bExistTimeStamp )
		m_dTimeStamp[index]						= atof( sSub[n++] );

	UserData->gpsData[index].latitude			= atof( sSub[n++] );
	UserData->gpsData[index].longitude			= atof( sSub[n++] );
	UserData->gpsData[index].speed				= atoi( sSub[n++] );
	UserData->rawData[index].accRawData.AccX	= atoi( sSub[n++] );
	UserData->rawData[index].accRawData.AccY	= atoi( sSub[n++] );
	UserData->rawData[index].accRawData.AccZ	= atoi( sSub[n++] );
}

//gps original 패킷을 파싱한다.
void CNMEAParser::GPSParser(int index, USER_DATA* UserData)
{
	ParseBuffer((BYTE*)UserData->rawData[index].gpsRawData.gpsGPRMC, strlen(UserData->rawData[index].gpsRawData.gpsGPRMC));
	if(m_btRMCDataValid == 'A')
	{
		if(m_dRMCGroundSpeed*1.83 > -1 && m_dRMCGroundSpeed*1.83 < 400 )
		{
			//m_GPSSpeedArrayOld = (int)(m_dRMCGroundSpeed*1.83);
		}
		UserData->gpsData[index].speed = (int)(m_dRMCGroundSpeed*1.83);//m_GPSSpeedArrayOld;
		UserData->gpsData[index].longitude = m_dRMCLongitude;
		UserData->gpsData[index].latitude = m_dRMCLatitude;
		/*
		struct tm gpsTime;
		//__time32_t now, result;
		//_time32(&now);
		//_localtime32_s( &gpsTime, &now );
		gpsTime.tm_year	= m_wRMCYear;
		gpsTime.tm_mon	= m_btRMCMonth;
		gpsTime.tm_mday = m_btRMCDay;
		gpsTime.tm_hour = m_btRMCHour + 9;
		gpsTime.tm_min	= m_btRMCMinute;
		gpsTime.tm_sec	= m_btRMCSecond;


		UserData->rawData[index].gpsRawData.gpsTime.time = (__time32_t)_mkgmtime( &gpsTime );
		UserData->rawData[index].gpsRawData.gpsTime.millitm = 0;

		CTime	t( UserData->rawData[index].gpsRawData.gpsTime.time );
		CString str;
		CString sMark = "-";
		str.Format( "%d%s%02d%s%02d", t.GetYear(), sMark, t.GetMonth(), sMark, t.GetDay() );
		TRACE( "%s ", str );
		sMark = ":";
		str.Format( "%02d%s%02d%s%02d", t.GetHour(), sMark, t.GetMinute(), sMark, t.GetSecond() );
		TRACE( "%s\n", str );
		*/
	}
}
