#include "StdAfx.h"
#include "RexSocket.h"

/*
SocketThreadCom : 소켓 recv 쓰레드
용도 : 쓰레드로 돌면서 수신과 연결상태를 체크. 수신이나 연결상태가 변하면
RexSocket::m_psocket_func 의 함수 포인터로 콜
*/
static UINT SocketThreadCom(LPVOID dParameter)
{
	RexSocket *plpr_socket = (RexSocket*)dParameter;

	const int MAX_BUF_LEN = 2048;
	char recv_buf[MAX_BUF_LEN], msg_buf[MAX_BUF_LEN], cur_c;
	int bytesReceived, cur_len = 0;
	
	while(true)
	{
		bytesReceived = recv(plpr_socket->m_socket, recv_buf, MAX_BUF_LEN, 0);

		if(bytesReceived >= MAX_BUF_LEN)
		{
			//receive over size
			closesocket(plpr_socket->m_socket);			
		}
		else if(bytesReceived > 0)
		{
			for(int i=0; i<bytesReceived; i++)
			{
				cur_c = recv_buf[i];
				switch(cur_c)
				{
				case STX://Start
					cur_len = 0;
					break;
				case ETX://End
					msg_buf[cur_len] = 0;
					cur_len = 0;
					(*(plpr_socket->m_psocket_func))(FD_READ, msg_buf);
					break;
				default://Middle
					msg_buf[cur_len++] = cur_c;		
					break;
				}
			}
		}
		else
		{
			TRACE("Disconnection : %d\n", bytesReceived);

			shutdown(plpr_socket->m_socket, 2);
			closesocket(plpr_socket->m_socket);
			plpr_socket->m_socket = INVALID_SOCKET;
			plpr_socket->m_con_state = 0;

			(*(plpr_socket->m_psocket_func))(FD_CLOSE, NULL);

			//auto reconnect
			if(plpr_socket->m_auto_recon)
			{
				plpr_socket->Connect(plpr_socket->m_ip, plpr_socket->m_port, plpr_socket->m_auto_recon);
			}
			break;
		}
	}
	TRACE("Recv thread terminated.\n");
	return 0;
}

static UINT ConnectingThreadCom(LPVOID dParameter)
{
	RexSocket *plpr_socket = (RexSocket*)dParameter;
	CWinThread *myp = plpr_socket->m_connect_thread;

	sockaddr_in addr;
	memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = inet_addr(plpr_socket->m_ip);
	addr.sin_port = htons(plpr_socket->m_port);

	while(plpr_socket->m_con_state == 1 && myp == plpr_socket->m_connect_thread)
	{
		if ( plpr_socket->m_conth_crsection.m_hObject != INVALID_HANDLE_VALUE )
			plpr_socket->m_conth_crsection.Lock();

		if(connect(plpr_socket->m_socket, (sockaddr *)&addr, sizeof(addr)) == SOCKET_ERROR)
		{
			//TRACE("Connection failed.\n");
			plpr_socket->m_psocket_func(FD_CONNECT, (char*)RexSocket::FAILTOCONNECT);
			plpr_socket->m_conth_crsection.Unlock();
			if(plpr_socket->m_auto_recon)
			{
				Sleep(plpr_socket->m_recon_ms);
				continue;
			}
			else
				break;
		}
		else
		{
			plpr_socket->m_conth_crsection.Unlock();
		}

		plpr_socket->m_con_state = RexSocket::CONNECTED;
		TRACE("Connection success.\n");

		plpr_socket->m_psocket_func(FD_CONNECT, (char*)RexSocket::CONNECTED);

		//Start recv thread.
		plpr_socket->m_socket_thread = ::AfxBeginThread(SocketThreadCom, plpr_socket, THREAD_PRIORITY_NORMAL, 0, 0, NULL);
	
		if(plpr_socket->m_socket_thread == NULL)
		{ 
			TRACE("Recv thread creation failed.\n"); return -4;
		}
		else
		{
			TRACE("Recv thread creation success.\n");
		}
	
		plpr_socket->m_conth_crsection.Unlock();
	
		break;
	}

	TRACE("Connect thread terminated.\n");

	return 0;
}

RexSocket::RexSocket(void)
{	
	m_con_state = 0;
	m_psocket_func = NULL;
	m_socket = INVALID_SOCKET;
	m_socket_thread = NULL;
	m_connect_thread = NULL;
	m_auto_recon = false;
	m_ip[0] = 0;
	m_recon_ms = 5000;
}


RexSocket::~RexSocket(void)
{
	if(m_socket != INVALID_SOCKET)
	{
		shutdown(m_socket, 2);
		closesocket(m_socket);
	}
	m_connect_thread = NULL;
	m_con_state = DISCONNECTED;
	Sleep(10);
	WSACleanup();	
}

/*
Init : 윈속 초기화 함수
인자 : p_func : 수신시나 연결종료시 콜함수, recon_ms - 접속 재시도 주기(밀리초), auto_recon - 자동 재접속 시도 여부
*/

int RexSocket::Init(void (*p_func)(int, char*))
{
	WSADATA wsadata = {0};
	if(WSAStartup(MAKEWORD(2,2), &wsadata) != 0)
	{
		TRACE("WSAStartup failed. Error No. %d\n", WSAGetLastError());
		return -1;
	}
	
	if(LOBYTE(wsadata.wVersion ) != 2 || HIBYTE( wsadata.wVersion ) != 2)
	{
		TRACE("Wrong Winsock version.\n");
		WSACleanup();
		return -2;
	}

	m_psocket_func = p_func;

	return 0;
}

/*
Connect : 서버로 접속
인자 : 아이피, 포트
참고사항 : 먼저 init 가 초기화 되있어야함
*/
int RexSocket::Connect(char *ip, int port, bool auto_recon, int recon_ms /* = 5000*/)
{
	if ( m_socket != INVALID_SOCKET && m_con_state != DISCONNECTED )
	{
		m_conth_crsection.Lock();
		Disconnect();		
		m_conth_crsection.Unlock();
	}

	m_socket = socket(AF_INET, SOCK_STREAM, 0);
	if(m_socket == INVALID_SOCKET) 
	{
		TRACE("Socket creation failed. Error No : %d\n", WSAGetLastError());
		return -1;
	}
	
	strcpy_s(m_ip, ip);
	m_port = port;
	
	m_auto_recon = auto_recon;
	m_con_state = CONNECTING;
	m_recon_ms = recon_ms;
	m_connect_thread = ::AfxBeginThread(ConnectingThreadCom, this, THREAD_PRIORITY_NORMAL, 0, 0, NULL);	
	if(m_connect_thread == NULL)
	{
		m_con_state = DISCONNECTED;
		TRACE("Connecting thread creation failed.\n");
		return -2;
	}
	else
	{
		TRACE("Connecting thread creation success.\n");
	}

	return 0;
}

/*
Disconnect : 연결 해제
*/
int RexSocket::Disconnect()
{
	m_con_state = 0;
	m_auto_recon = false;
	if(m_socket != INVALID_SOCKET)
	{
		shutdown(m_socket, 2);
		closesocket(m_socket);
	}	
	return 0;
}


/*
Send : 스트링 전송
인자 : str - 본문 스트링
참고사항 : str 앞과 뒤에 자동으로 STX 와 ETX 를 붙여줌
*/
int RexSocket::Send(char *str)
{
	if(m_socket == INVALID_SOCKET)
	{
		TRACE("Send failed. (wrong socket)\n");
		return -1;
	}

	if ( m_send_cs.m_sect.DebugInfo != NULL )
		m_send_cs.Lock();

	int len = strlen(str) + 2;

	memset(m_send_buf,0,1024);
	
	m_send_buf[0] = STX;
	memcpy(m_send_buf+1, str, len);
	m_send_buf[len-1] = ETX;
	
	int ret = SOCKET_ERROR;
	try {
		ret = send(m_socket, m_send_buf, len, 0);
	}
	catch(...)	{
		ret = SOCKET_ERROR;
	}

	if ( m_send_cs.m_sect.DebugInfo != NULL )
		m_send_cs.Unlock();

	if(ret == SOCKET_ERROR)
	{
		TRACE("Send failed.\n");
		return -2;
	}
	else
	{		
		TRACE("Send success.\n");
	}

	return ret;
}

