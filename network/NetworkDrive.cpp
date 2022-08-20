#include "StdAfx.h"
#include "NetworkDrive.h"

#include <Winnetwk.h>			//for WNetAddConnection2
#pragma comment(lib, "Mpr.lib")


CNetworkDrive::CNetworkDrive(void)
{
}


CNetworkDrive::~CNetworkDrive(void)
{
}

//네트워크 드라이브와의 연결 상태 체크 쓰레드
static UINT ThreadChecking( LPVOID lpVoid )
{
	CNetworkDrive *lpMain = (CNetworkDrive*)lpVoid;

	while ( true )
	{
		//연결되었다는 상태값을 가진 경우에만 그 연결이 실제로 유지되고 있는지를 검사한다.
		//if ( lpMain->IsConnected() )
		{
			if ( PathFileExists( lpMain->m_sLocal ) )
			{
				//TRACE( "%s network drive is alive.\n", lpMain->m_sLocal );
				Sleep( 2000 );
			}
			else
			{
				//연결이 끊김을 알리고
				TRACE( "%s network drive is dead.\n", lpMain->m_sLocal );
				lpMain->m_nStatus = CNetworkDrive::DISCONNECTED;
				(*(lpMain->m_pCallback_Function))( lpMain->m_nStatus, 0 );

				//재접속을 시도한다.
				if ( lpMain->m_nRetryInteval > 0 )
				{
					Sleep( lpMain->m_nRetryInteval );

					TRACE( "retry connect to %s...\n", lpMain->m_sRemote );
					lpMain->m_nStatus = CNetworkDrive::CONNECTING;
					(*(lpMain->m_pCallback_Function))( lpMain->m_nStatus, 0 );
					lpMain->Connect();
				}

				break;
			}
		}
	}
/*
	while ( true )
	{
		bytesReceived = recv(lpMain->m_socket, recv_buf, MAX_BUF_LEN, 0);

		if(bytesReceived >= MAX_BUF_LEN)
		{
			//receive over size
			closesocket(lpMain->m_socket);			
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
					(*(lpMain->m_psocket_func))(FD_READ, msg_buf);
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

			shutdown(lpMain->m_socket, 2);
			closesocket(lpMain->m_socket);
			lpMain->m_socket = INVALID_SOCKET;
			lpMain->m_con_state = 0;

			(*(lpMain->m_psocket_func))(FD_CLOSE, NULL);

			//auto reconnect
			if(lpMain->m_auto_recon)
			{
				lpMain->Connect(lpMain->m_ip, lpMain->m_port, lpMain->m_auto_recon);
			}
			break;
		}
	}
	TRACE("Recv thread terminated.\n");
*/
	return 0;
}

static UINT ThreadConnecting( LPVOID lpVoid )
{
	CNetworkDrive	*lpMain = (CNetworkDrive*)lpVoid;
	CWinThread		*myp = lpMain->m_pThreadConnecting;

	NETRESOURCE	NetRes;
	memset( &NetRes, 0, sizeof(NETRESOURCE) );

	NetRes.dwType		= RESOURCETYPE_DISK;   
	NetRes.lpLocalName  = lpMain->m_sLocal;
	NetRes.lpProvider   = NULL;   
	NetRes.lpRemoteName = lpMain->m_sRemote;

	DWORD dwResult;

	while ( lpMain->m_nStatus == CNetworkDrive::CONNECTING )//&& myp == lpMain->m_connect_thread)
	{
		if ( lpMain->m_csThreadConnecting.m_hObject != INVALID_HANDLE_VALUE )
			lpMain->m_csThreadConnecting.Lock();

		//이미 연결되어 있거나 다른 경로가 해당 로컬명으로 연결된 경우를 위해 기존 연결을 끊고 새로 연결해준다.
		dwResult = WNetCancelConnection2( NetRes.lpLocalName, CONNECT_UPDATE_PROFILE, TRUE );

		if ( dwResult == NO_ERROR || dwResult == ERROR_NOT_CONNECTED )
		{
			//메인에게 연결중임을 알리고...
			lpMain->m_pCallback_Function( CNetworkDrive::CONNECTING, dwResult );

			//연결 시도한다.
			dwResult = WNetAddConnection2( &NetRes, lpMain->m_sPassword, lpMain->m_sID, CONNECT_UPDATE_PROFILE);

			//#define ERROR_ALREADY_ASSIGNED			85L
			//#define ERROR_DEVICE_ALREADY_REMEMBERED	1202L
			//#define ERROR_SESSION_CREDENTIAL_CONFLICT	1219L

			if ( dwResult == NO_ERROR )
			{
				lpMain->m_csThreadConnecting.Unlock();
			}
			else
			{
				//TRACE("Connection failed.\n");
				lpMain->m_pCallback_Function( CNetworkDrive::FAILTOCONNECT, dwResult );
				lpMain->m_csThreadConnecting.Unlock();

				if ( lpMain->m_nRetryInteval > 0 )
				{
					Sleep( lpMain->m_nRetryInteval );
					continue;
				}
				else
				{
					break;
				}
			}
		}

		lpMain->m_nStatus = CNetworkDrive::CONNECTED;
		TRACE("Connection success.\n");

		lpMain->m_pCallback_Function( CNetworkDrive::CONNECTED, dwResult );

		//Start Check thread.
		lpMain->m_pThreadChecking = ::AfxBeginThread( ThreadChecking, lpMain, THREAD_PRIORITY_NORMAL, 0, 0, NULL);

		if(lpMain->m_pThreadChecking == NULL)
		{ 
			TRACE("Checking thread creation failed.\n");
			return -4;
		}
		else
		{
			TRACE("Checking thread creation success.\n");
		}

		lpMain->m_csThreadConnecting.Unlock();

		break;
	}

	TRACE("Connecting thread terminated.\n");

	return 0;
}

void CNetworkDrive::Init( void (*p_func)(int, int), char *sRemote, char *sLocal, char *sID, char *sPassword, int nRetryInterval /*= 2000*/ )
{
	m_pCallback_Function = p_func;

	//로컬 드라이브명은 콜론으로 끝나야 한다.
	if ( strlen( sLocal ) == 1 )
		strcat( sLocal, ":" );

	strcpy( m_sRemote, sRemote );
	strcpy( m_sLocal, sLocal );
	strcpy( m_sID, sID );
	strcpy( m_sPassword, sPassword );
	m_nRetryInteval = nRetryInterval;
}

int CNetworkDrive::Connect()
{
/*
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
*/

	m_nStatus = CONNECTING;

	m_pThreadConnecting = ::AfxBeginThread( ThreadConnecting, this, THREAD_PRIORITY_NORMAL, 0, 0, NULL);	
	
	if ( m_pThreadConnecting == NULL )
	{
		m_nStatus = DISCONNECTED;
		TRACE("Connecting thread creation failed.\n");
		return -2;
	}
	else
	{
		TRACE("Connecting thread creation success.\n");
	}

	return 0;
}

int CNetworkDrive::Disconnect()
{
	DWORD dwResult = WNetCancelConnection2( m_sLocal, CONNECT_UPDATE_PROFILE, TRUE );

	if ( dwResult == NO_ERROR || dwResult == ERROR_NOT_CONNECTED )
	{
		m_nStatus = DISCONNECTED;
		return NO_ERROR;
	}

	return dwResult;
}
