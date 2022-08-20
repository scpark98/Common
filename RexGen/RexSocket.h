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

	int			Init(void (*p_func)(int, char*)); //초기화 함수 , 인자 : p_func - 수신시나 연결종료시 콜함수 설정, recon_ms - 접속 실패시 재시도 주기(밀리초)
	int			Connect(char *ip, int port, bool auto_recon = false, int recon_ms = 5000); //접속 함수, 인자 : 아이피, 포트, auto_recon - 자동재접속 여부
	int			Disconnect(); //접속 종료 함수
	int			Send(char *str); //송신 함수, 인자 : 원문 스트링(STX와 EXT 는 자동으로 붙여줌)

	int			GetConnectionState() { return m_con_state; }	//0:접속끊김 1: 접속시도중 2: 접속됨
	char*		GetIPAddress() { return m_ip; }
	int			GetPort() { return m_port; }
	int			GetRetryConnectionInterval() { return m_recon_ms; }
	

	SOCKET		m_socket;
	char		m_ip[128];
	int			m_port;
	int			m_recon_ms;
	bool		m_auto_recon;
	int			m_con_state; //0:접속해제 1: 접속시도중 2: 접속상태


	void (*m_psocket_func)(int, char*);
	
	CWinThread	*m_connect_thread, *m_socket_thread;
	CCriticalSection m_conth_crsection, m_send_cs;

private:
	char m_send_buf[1024];
};