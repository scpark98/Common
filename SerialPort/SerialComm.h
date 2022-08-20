// SerialComm.h
// 
// RS-232c Serial Communication Class
// 

#ifndef __SERIALCOMM_H__
#define __SERIALCOMM_H__


// Defines
//

#define UM_SERIALRXDED		WM_USER+100		// The receive buffer is set with new data
#define UM_SERIALTXDED		WM_USER+101		// All the data in transmit buffer have been sent.

#define MAXBLOCK        4096
#define MAXPORTS        4

// Flow control flags
#define FC_DTRDSR       0x01
#define FC_RTSCTS       0x02
#define FC_XONXOFF      0x04

// ASCII code definitions
#define ASCII_BEL       0x07
#define ASCII_BS        0x08
#define ASCII_LF        0x0A
#define ASCII_CR        0x0D
#define ASCII_XON       0x11
#define ASCII_XOFF      0x13

//
// End Of Defines


class CSerialComm
{
public:
	CSerialComm (); 
	virtual ~CSerialComm ();	
   
public:
	BOOL	CloseConnection ();
   	
	// Port operation
	BOOL	StartComm ( HWND hwnd, BYTE btPort );
	BOOL	StartComm ( HWND hwnd, BYTE btPort, DWORD dwBaudRate );
	void	StopComm();
	BOOL	IsConnected ();
	
	BOOL	WriteCommBlock ( char *, DWORD );
	BOOL	WriteComm( char * lpBuf, DWORD dwToWrite );

private:
	BOOL	OpenComPort ();
	BOOL	SetupConnection ();
	int		ReadCommBlock ( LPBYTE, int );
	BOOL	CreateCommInfo ();
	BOOL	DestroyComm ();

	static ULONG WINAPI Thread_WatchCommPort ( LPVOID PresentDevice );


// Implementation
public:
	BOOL	SendOneByteNow ( char the_byte );
	BOOL	ClearBuffer ();
	void	TXDEmptyNotify ();


// Attributes
public:
	// COM port setup	
	void	SetComPort ( BYTE	btPort,		// port : port number
						 DWORD	rate,		// rate : baud rate
						 BYTE	bytesize,	// bytesize : number of bits in a byte	
						 BYTE	stop,		// stop : the size of stopbits in number of bits
						 BYTE	parity );	// parity : parity selection
										
	//Port Setup
	void	SetXonOff ( BOOL chk );	
	void	SetDtrRts ( BYTE chk );
	void	SetPause ( BOOL bPause );

public:
	BYTE	m_btPort;
	BOOL	m_bXOnOff;
	BYTE	m_btSize, 
			m_btFlowCtrl, 
			m_btParity, 
			m_btStopBits;
	DWORD	m_dwBaudRate;
	
	OVERLAPPED	m_osWrite, 
				m_osRead;

	
	HANDLE	m_hComm;		// COM port device handle
	BOOL	m_bConnected,
			m_bPause;
   
	HWND	m_hwndNotify;	// Comm Port Data / Status 수신 윈도우

	
	/*
	// DCB, COMMTIMEOUTS

	DWORD	m_dwBaudRate; 

	BOOL	m_bBinary;
	BOOL	m_bParity;
	BOOL	m_bOutxCtsFlow;
	BOOL	m_bOutxDsrFlow;
	BOOL	m_bDtrControl;
	BOOL	m_bDsrSensitivity;
	BOOL	m_bTXContinueOnXoff;
	BOOL	m_bOutX;
	BOOL	m_bInX;
	BOOL	m_bErrorChar;
	BOOL	m_bNull; 
	BOOL	m_bRtsControl; 
	BOOL	m_bAbortOnError;

	WORD	m_wXonLim; 
	WORD	m_wXoffLim;  
	BYTE	m_byByteSize; 
	BYTE	m_byParity; 
	BYTE	m_byStopBits;
	char	m_cXonChar; 
	char	m_cXoffChar; 
	char	m_cErrorChar;
	char	m_cEofChar; 
	char	m_cEvtChar; 

	// COMMTIMEOUTS

	DWORD	m_dwReadIntervalTimeout; 
	DWORD	m_dwReadTotalTimeoutMultiplier; 
	DWORD	m_dwReadTotalTimeoutConstant; 
	DWORD	m_dwWriteTotalTimeoutMultiplier; 
	DWORD	m_dwWriteTotalTimeoutConstant;
	*/
};

inline BOOL CSerialComm::IsConnected ()
{
	return ( NULL != m_hComm ) ? TRUE : FALSE;
}

inline void CSerialComm::SetXonOff ( BOOL chk )
{
	m_bXOnOff = chk;
}

inline void CSerialComm::SetDtrRts ( BYTE chk )
{
	m_btFlowCtrl = chk;
}

inline void CSerialComm::SetPause ( BOOL bPause )
{
	m_bPause = bPause;
}


#endif // __SERIALCOMM_H__