#pragma once

//scpark 2014 03 17
//RexSocket Ŭ������ �����Ͽ� ��Ʈ��ũ ����̺� ���� �� ������ ���� Ŭ������ �ۼ�.

class CNetworkDrive
{
public:
	CNetworkDrive(void);
	~CNetworkDrive(void);

	enum CONNECTION_STATUS { DISCONNECTED, CONNECTING, CONNECTED, FAILTOCONNECT };

	//�ʱ�ȭ �Լ�, ���� : callback_func(���Žó� ���� ����� �ݹ� �Լ�), �������, ���õ���̺��, ���Ӱ���
	void		Init( void (*p_func)( int, int ), char *sRemote, char *sLocal, char *sID, char *sPassword, int nRetryInterval = 2000 );
	int			Connect(); //nRetryInterval(������ �õ� ����,ms). 0�̸� ������ �õ� ����.
	int			Disconnect(); //��Ʈ��ũ ����̺� ���� ����
	bool		IsConnected() { return ( m_nStatus == CONNECTED ); }

	char		m_sRemote[MAX_PATH];
	char		m_sLocal[3];
	char		m_sID[32];
	char		m_sPassword[128];

	int			m_nRetryInteval; //ms
	int			m_nStatus; //0:�������� 1: ���ӽõ��� 2: ������

	void		(*m_pCallback_Function)( int, int );
	CWinThread	*m_pThreadConnecting, *m_pThreadChecking;
	CCriticalSection m_csThreadConnecting;//, m_cssend_cs;
};

