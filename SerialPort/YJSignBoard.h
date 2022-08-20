#pragma once

//scpark.
//CommThread 시리얼 제어 클래스를 이용하여 양재마이크로 전광판을 제어하는 클래스.
//기본 연결, 해제, 전광판과의 연결 상태 체크, 재연결 기능 구현 목적.
#include "CommThread.h"

#define WM_MESSAGE_SIGNBOARD	WM_USER + 4158

class CYJSignBoard
{
public:
	CYJSignBoard(void);
	~CYJSignBoard(void);

	enum CONNECTION_STATUS { DISCONNECTED, CONNECTING, CONNECTED, FAILTOCONNECT, READ_COMM };

	CCommThread			m_Serial;
	bool				m_bIsTMode;
	bool				m_bInRepeatMessage;
	int					m_nFailCount;

	void	Init( HWND hParentWnd, CString sPort, DWORD dwBaud = 9600, BYTE btDataBit = 8, BYTE btStopBit = ONESTOPBIT, BYTE btParity = NOPARITY )
	{
		m_hParentWnd= hParentWnd;
		m_sPort		= sPort;
		m_dwBaud	= dwBaud;
		m_btDataBit	= btDataBit;
		m_btStopBit	= btStopBit;
		m_btParity	= btParity;
	}

	bool		Connect( bool bRetryConnect = true );
	bool		Disconnect(); //연결 끊기
	bool		IsConnected() { return ( m_nStatus == CONNECTED ); }
	int			GetStatus() { return m_nStatus; }

	void		DisplaySignBoardMessage( int nRow, CString sMsg, int nColor, int px = -1 );	//px=-1(중앙정렬), px>=0(x좌표)
	int			MakePacketAndSend( BYTE* bt, int n, int nDelay = 1 );
	int			MakePacketAndSend( CString sPacktet, int nDelay = 1 );

	static void CallbackFromSerialPort( DWORD dwData, char* data );
	LRESULT		OnReceiveDataFromController(WPARAM wParam, LPARAM lParam);

	CWinThread	*m_pThreadConnecting, *m_pThreadChecking;
	CCriticalSection m_csThreadConnecting, m_csLastData;


	HWND	m_hParentWnd;
	CString m_sPort;
	DWORD	m_dwBaud;
	BYTE	m_btDataBit;
	BYTE	m_btStopBit;
	BYTE	m_btParity;
	int		m_nStatus;	//0:접속해제 1: 접속시도중 2: 접속중
};

