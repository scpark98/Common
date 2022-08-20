#pragma once

//scpark 2014 03 17
//RexSocket 클래스를 변형하여 네트워크 드라이브 연결 및 해제를 위한 클래스로 작성.

class CNetworkDrive
{
public:
	CNetworkDrive(void);
	~CNetworkDrive(void);

	enum CONNECTION_STATUS { DISCONNECTED, CONNECTING, CONNECTED, FAILTOCONNECT };

	//초기화 함수, 인자 : callback_func(수신시나 연결 종료시 콜백 함수), 대상폴더, 로컬드라이브명, 접속계정
	void		Init( void (*p_func)( int, int ), char *sRemote, char *sLocal, char *sID, char *sPassword, int nRetryInterval = 2000 );
	int			Connect(); //nRetryInterval(재접속 시도 간격,ms). 0이면 재접속 시도 안함.
	int			Disconnect(); //네트워크 드라이브 연결 끊기
	bool		IsConnected() { return ( m_nStatus == CONNECTED ); }

	char		m_sRemote[MAX_PATH];
	char		m_sLocal[3];
	char		m_sID[32];
	char		m_sPassword[128];

	int			m_nRetryInteval; //ms
	int			m_nStatus; //0:접속해제 1: 접속시도중 2: 접속중

	void		(*m_pCallback_Function)( int, int );
	CWinThread	*m_pThreadConnecting, *m_pThreadChecking;
	CCriticalSection m_csThreadConnecting;//, m_cssend_cs;
};

