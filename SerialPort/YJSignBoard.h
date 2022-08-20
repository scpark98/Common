#pragma once

//scpark.
//CommThread �ø��� ���� Ŭ������ �̿��Ͽ� ���縶��ũ�� �������� �����ϴ� Ŭ����.
//�⺻ ����, ����, �����ǰ��� ���� ���� üũ, �翬�� ��� ���� ����.
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
	bool		Disconnect(); //���� ����
	bool		IsConnected() { return ( m_nStatus == CONNECTED ); }
	int			GetStatus() { return m_nStatus; }

	void		DisplaySignBoardMessage( int nRow, CString sMsg, int nColor, int px = -1 );	//px=-1(�߾�����), px>=0(x��ǥ)
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
	int		m_nStatus;	//0:�������� 1: ���ӽõ��� 2: ������
};

