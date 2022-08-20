// CSerialComm.cpp
// RS-232c Serial Communication Class
// 

#include "stdafx.h"
#include "SerialComm.h"


CSerialComm::CSerialComm ()
{
	m_hComm = NULL;
	m_btFlowCtrl = FC_XONXOFF;
	m_bConnected = FALSE;

	m_btPort		= 1;
	m_dwBaudRate	= 9600;
	m_btSize		= 8;
	m_btStopBits	= ONESTOPBIT;

	memset ( &m_osWrite, NULL, sizeof OVERLAPPED );
	memset ( &m_osRead, NULL, sizeof OVERLAPPED );
}

CSerialComm::~CSerialComm ()
{
    DestroyComm ();
}

void CSerialComm::SetComPort ( BYTE btPort, DWORD rate, BYTE bytesize, BYTE stop, BYTE parity )
{
	m_btPort = btPort;
	m_dwBaudRate = rate;
	m_btSize = bytesize;
	m_btStopBits = stop;
	m_btParity = parity;
}

// After calling SetComPort(), SetXonOff(), and SetDtrRts()
BOOL CSerialComm::CreateCommInfo ()
{
	m_osWrite.Offset = 0;
	m_osWrite.OffsetHigh = 0;
	m_osRead.Offset = 0;
	m_osRead.OffsetHigh = 0;

	if ( NULL == ( m_osRead.hEvent = CreateEvent ( NULL, TRUE, FALSE, NULL) ) )
		return FALSE ;

	if ( NULL == ( m_osWrite.hEvent = CreateEvent ( NULL, TRUE, FALSE, NULL ) ) ) 
	{
		CloseHandle ( m_osRead.hEvent );
		m_osRead.hEvent = NULL;
		return FALSE;
	}

	return TRUE;
} 

BOOL CSerialComm::OpenComPort ()
{
	BOOL	bRetVal ;
	TCHAR	tzPort[32];
	COMMTIMEOUTS		CommTimeOuts ;

	memset ( tzPort, 0, 32 * sizeof TCHAR );
#ifdef _WIN32_WCE
	_stprintf ( tzPort, _T("COM%d:"), m_btPort );
#else
	_stprintf ( tzPort, _T("COM%d"), m_btPort );
#endif 

	m_hComm = CreateFile ( tzPort, GENERIC_READ | GENERIC_WRITE,
						   0,						// exclusive access
						   NULL,					// no security attrs
						   OPEN_EXISTING,
						#ifdef _WIN32_WCE
						   0,
						#else
						   FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OVERLAPPED,	// overlapped I/O
						#endif 
						   NULL );
	if ( (HANDLE)-1 == m_hComm )
	{
		AfxMessageBox ( _T("Can't Open COM Port") );
		m_hComm = NULL;
		return FALSE;
	}

	SetCommMask ( m_hComm, EV_RXCHAR | EV_TXEMPTY );
	SetupComm ( m_hComm, 4096, 4096) ;
	PurgeComm ( m_hComm, PURGE_TXABORT | PURGE_RXABORT | PURGE_TXCLEAR | PURGE_RXCLEAR );

	CommTimeOuts.ReadIntervalTimeout = 0xFFFFFFFF;
	CommTimeOuts.ReadTotalTimeoutMultiplier = 1;
	CommTimeOuts.ReadTotalTimeoutConstant = 1000;
	CommTimeOuts.WriteTotalTimeoutMultiplier = 1;
	CommTimeOuts.WriteTotalTimeoutConstant = 1000;
	SetCommTimeouts ( m_hComm, &CommTimeOuts );

	bRetVal = SetupConnection () ;

	if ( bRetVal ) 
	{
		m_bConnected = TRUE;
		CreateThread ( NULL, 0, Thread_WatchCommPort, this, 0, NULL );
	}
	else 
	{
		m_bConnected = FALSE ;
		CloseHandle ( m_hComm ) ;
		m_hComm = NULL;
	}

	return bRetVal;
} 

BOOL CSerialComm::SetupConnection ()
{
	DCB		dcb;

	dcb.DCBlength = sizeof DCB;

	GetCommState ( m_hComm, &dcb );

	dcb.BaudRate = m_dwBaudRate;		// 전송속도
	dcb.ByteSize = m_btSize ;			// 데이타비트
	dcb.Parity = m_btParity;			// 패리티 체크
	dcb.StopBits = m_btStopBits;		// 스톱비트

	dcb.fOutxDsrFlow =0 ;						// Dsr Flow
	dcb.fDtrControl = DTR_CONTROL_ENABLE ;		// Dtr Control
	dcb.fOutxCtsFlow = 0 ;						// Cts Flow
	dcb.fRtsControl = RTS_CONTROL_ENABLE ;		// Ctr Control
	dcb.XonChar = ASCII_XON ;
	dcb.XoffChar = ASCII_XOFF ;
	dcb.XonLim = 100 ;
	dcb.XoffLim = 100 ;
	dcb.fNull =FALSE;
	dcb.fErrorChar = FALSE;
	dcb.fInX = FALSE ;		// XON/XOFF 관한것
	dcb.fOutX = FALSE ;
	dcb.fParity = FALSE;
	dcb.fBinary = TRUE ;

	return SetCommState ( m_hComm, &dcb );
} 

//컴포트로 부터 데이타를 읽는다.
int CSerialComm::ReadCommBlock ( LPBYTE lpszBlock, int nMaxLength )
{
	BOOL       bReadStat;
	COMSTAT    ComStat;
	DWORD      dwErrorFlags;
	DWORD      dwLength;

	// only try to read number of bytes in queue 
	ClearCommError ( m_hComm, &dwErrorFlags, &ComStat );
	dwLength = min((DWORD) nMaxLength, ComStat.cbInQue);
	if ( 0 < dwLength ) 
	{
		bReadStat = ReadFile(m_hComm, lpszBlock, dwLength, &dwLength, &m_osRead );
		if ( !bReadStat ) 
		{
			// ReadFile error handling should be here.
		}
	}
   
	return dwLength;
} 

BOOL CSerialComm::DestroyComm ()
{
	if ( m_bConnected ) CloseConnection ();
	if ( m_osRead.hEvent ) 
	{
		CloseHandle ( m_osRead.hEvent );
		m_osRead.hEvent = NULL;
	}
	if ( m_osWrite.hEvent ) 
	{
		
		CloseHandle ( m_osWrite.hEvent ) ;
		m_osWrite.hEvent = NULL;
	}

	return TRUE;
} 

BOOL CSerialComm::CloseConnection ()
{
	m_bConnected = FALSE ;

	// disable event notification and wait for thread to halt
	SetCommMask ( m_hComm, 0 ) ;

	EscapeCommFunction ( m_hComm, CLRDTR ) ;

	PurgeComm ( m_hComm, PURGE_TXABORT | PURGE_RXABORT | PURGE_TXCLEAR | PURGE_RXCLEAR );

	CloseHandle ( m_hComm ) ;
	m_hComm = NULL;

	return TRUE;
} 

BOOL CSerialComm::WriteCommBlock ( LPSTR lpByte , DWORD dwBytesToWrite )
{
	TRACE ( _T("\tCSerialComm::WriteCommBlock ()\n") );

	ASSERT ( NULL != lpByte );
	ASSERT ( 0 < dwBytesToWrite );
	if ( ( NULL == lpByte ) || ( 0 == dwBytesToWrite ) )
		return FALSE;

	if ( NULL == m_hComm )
		return FALSE;
		
	BOOL	bWriteStat = TRUE;
	DWORD	dwBytesWritten = 0;

	bWriteStat = WriteFile ( m_hComm, lpByte, dwBytesToWrite, &dwBytesWritten, &m_osWrite );
	if ( !bWriteStat ) 
	{
		// WriteFile error handling should be here.
		return FALSE;
	}
	
	return bWriteStat;
} 

BOOL CSerialComm::WriteComm(char * lpBuf, DWORD dwToWrite)
{
   OVERLAPPED osWrite = {0};
   DWORD dwWritten;
   BOOL fRes;

   // Issue write.
   if (!WriteFile(m_hComm, lpBuf, dwToWrite, &dwWritten, NULL)) 
   {
      if (GetLastError() != ERROR_IO_PENDING) 
      {  // WriteFile failed, but it isn't delayed. Report error and abort.
      
          fRes = FALSE;
        }
      else
      {
         // Write is pending.

         if (!GetOverlappedResult(m_hComm, &osWrite, &dwWritten, TRUE))
         {     
             fRes = FALSE;
		 }
         else
         {  // Write operation completed successfully.

             fRes = TRUE;
		 }
      }
   }
   else
      // WriteFile completed immediately.
      fRes = TRUE;
  
   return fRes;
}

BOOL CSerialComm::StartComm ( HWND hwnd, BYTE btPort )
{
	m_hwndNotify = hwnd;
	SetComPort ( btPort, m_dwBaudRate, 8, ONESTOPBIT, 0 );

	if ( CreateCommInfo () )
	{
		m_bPause = FALSE;
		return OpenComPort ();
	}
	else
		DestroyComm ();

	return FALSE;
}

BOOL CSerialComm::StartComm ( HWND hwnd, BYTE btPort, DWORD dwBaudRate )
{
	m_hwndNotify = hwnd;
	SetComPort ( btPort, dwBaudRate, 8, ONESTOPBIT, 0 );

	if ( CreateCommInfo () )
	{
		m_bPause = FALSE;
		return OpenComPort ();
	}
	else
		DestroyComm ();

	return FALSE;
}

void CSerialComm::StopComm()
{
	DestroyComm ();
}

void CSerialComm::TXDEmptyNotify ()
{
	if ( NULL != m_hwndNotify )
		SendMessage ( m_hwndNotify, UM_SERIALTXDED, 0, 0 );
}

BOOL CSerialComm::ClearBuffer ()
{
	if ( NULL != m_hComm )
		return PurgeComm ( m_hComm, PURGE_TXABORT | PURGE_RXABORT | PURGE_TXCLEAR | PURGE_RXCLEAR );

	return FALSE;
}

BOOL CSerialComm::SendOneByteNow ( char the_byte )
{
	if ( NULL != m_hComm )
		return TransmitCommChar ( m_hComm, the_byte );
	
	return FALSE;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
// Watch Thread

// This is a thread procedure for watching COM port
ULONG WINAPI CSerialComm::Thread_WatchCommPort ( LPVOID PresentDevice ) 
{
	TRACE ( _T("\tCSerialComm::Thread_WatchCommPort ()\n") );
	
	DWORD	dwEvtMask;
	BYTE	byBuff[MAXBLOCK + 1];   
	int		nLength = 0;

	OVERLAPPED		os;
	CSerialComm*	npComm = (CSerialComm*)PresentDevice;
   
	if ( !npComm )
	{
		npComm->CloseConnection ();
		return 0;
	}

	memset ( &os, 0, sizeof OVERLAPPED );
	if ( NULL == ( os.hEvent = CreateEvent ( NULL,	TRUE, FALSE, NULL) ) ) 
	{
		npComm->CloseConnection ();
		return 0;
	}

	// Event masking, data receiving event, data transmission-completed event	
	if( !SetCommMask(npComm->m_hComm, EV_RXCHAR|EV_TXEMPTY)) 
	{
		npComm->CloseConnection ();
		return 0;
	}

	while ( npComm->m_bConnected ) 
	{
		dwEvtMask = 0 ;
		WaitCommEvent ( npComm->m_hComm, &dwEvtMask, NULL );
		
		// Data Receiving
		if ( EV_RXCHAR == ( dwEvtMask & EV_RXCHAR ) ) 
		{
			nLength = npComm->ReadCommBlock ( (LPBYTE)byBuff, MAXBLOCK );
			if ( ( TRUE == npComm->m_bPause ) || ( 0 >= nLength ) )
				continue;
			
			SendMessage ( npComm->m_hwndNotify, UM_SERIALRXDED, nLength, (LPARAM)byBuff );
		}

		// Data Sending
		if ( EV_TXEMPTY == ( dwEvtMask & EV_TXEMPTY ) ) 
		{
			// If previous data are all sent
			npComm->TXDEmptyNotify ();
		}
   }
  
   CloseHandle ( os.hEvent );
   os.hEvent = NULL;

   npComm->CloseConnection ();

   TRACE ( _T("\tCSerialComm::Thread_WatchCommPort ()... RETURN...\n") );

   return 1;
} 