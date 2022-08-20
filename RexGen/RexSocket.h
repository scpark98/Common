#pragma once

#include <WinSock2.h>

#pragma comment(lib, "ws2_32.lib")

#define STX 0x02
#define ETX 0x03

static UINT SocketThreadCom(LPVOID dParameter);

class RexSocket
{
public:
	RexSocket(void);
	~RexSocket(void);

	enum CONNECTION_STATUS { DISCONNECTED, CONNECTING, CONNECTED, FAILTOCONNECT, READ };

	int			Init(void (*p_func)(int, char*)); //�ʱ�ȭ �Լ� , ���� : p_func - ���Žó� ��������� ���Լ� ����, recon_ms - ���� ���н� ��õ� �ֱ�(�и���)
	int			Connect(char *ip, int port, bool auto_recon = false, int recon_ms = 5000); //���� �Լ�, ���� : ������, ��Ʈ, auto_recon - �ڵ������� ����
	int			Disconnect(); //���� ���� �Լ�
	int			Send(char *str); //�۽� �Լ�, ���� : ���� ��Ʈ��(STX�� EXT �� �ڵ����� �ٿ���)

	int			GetConnectionState() { return m_con_state; }	//0:���Ӳ��� 1: ���ӽõ��� 2: ���ӵ�
	char*		GetIPAddress() { return m_ip; }
	int			GetPort() { return m_port; }
	int			GetRetryConnectionInterval() { return m_recon_ms; }
	

	SOCKET		m_socket;
	char		m_ip[128];
	int			m_port;
	int			m_recon_ms;
	bool		m_auto_recon;
	int			m_con_state; //0:�������� 1: ���ӽõ��� 2: ���ӻ���


	void (*m_psocket_func)(int, char*);
	
	CWinThread	*m_connect_thread, *m_socket_thread;
	CCriticalSection m_conth_crsection, m_send_cs;

private:
	char m_send_buf[1024];
};