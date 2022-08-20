 #include "StdAfx.h"
#include "YJSignBoard.h"


CYJSignBoard*	g_pMain = NULL;

//�����ǰ��� ���� ���� üũ ������
static UINT ThreadChecking( LPVOID lpVoid )
{
	CYJSignBoard *lpMain = (CYJSignBoard*)lpVoid;

	while ( true )
	{
		TRACE( "%s\n", __FUNCTION__ );

		lpMain->m_bIsTMode = false;
		lpMain->MakePacketAndSend( "[GETMODE]" );
		Sleep( 500 );

		//lpMain->m_csLastData.Lock();

		if ( lpMain->m_bIsTMode )// lpMain->m_sLastData.Find( "[PM2]" ) >= 0 )
		{
			TRACE( "Signboard is alive\n" );
			lpMain->m_nFailCount = 0;

			lpMain->m_nStatus = CYJSignBoard::CONNECTED;
			::SendMessage( lpMain->m_hParentWnd, WM_MESSAGE_SIGNBOARD, lpMain->m_nStatus, 0 );
		}
		else
		{
			//lpMain->m_csLastData.Unlock();

			//�ٸ� �����Ϳ��� ������ ���� �� �����Ƿ� �� ���� ī��Ʈ�� 5 �̻��̸�
			//Connect �Լ��� ȣ���Ͽ� ��Ʈ �����Ӻ��� �ٽ� �õ��Ѵ�.
			lpMain->m_nFailCount++;
			lpMain->m_nStatus = CYJSignBoard::FAILTOCONNECT;
			::SendMessage( lpMain->m_hParentWnd, WM_MESSAGE_SIGNBOARD, lpMain->m_nStatus, lpMain->m_nFailCount );

			TRACE( "signboard is disconnected or not TMODE.(fail=%d)\n", lpMain->m_nFailCount );

			//�����ϴ�. �ѹ� TMODE�� �����ϸ� ��� ����������
			//�ý����� ����ǰ� ���߿� ���ϸ� TMODE�� �������� ���ϴ� ��찡 �߻��Ѵ�.
			//���� �������� ��Ȯ���� ������ TMODE ����� ��Ȥ ��ġ�� �� �Ͽ� �켱 3�� ���� ��������.
			//�ѹ��� ������ ��� �ȵǴ� ���� Ȯ�� �Ϸ�.
			lpMain->MakePacketAndSend( "[TMODE]" );
			Sleep( 500 );
			lpMain->MakePacketAndSend( "[TMODE]" );
			Sleep( 500 );
			lpMain->MakePacketAndSend( "[TMODE]" );
			Sleep( 500 );
			

			if ( lpMain->m_nFailCount >= 5 )
			{
				lpMain->m_nFailCount = 0;
				lpMain->Connect();
				break;
			}

		}

		//lpMain->m_csLastData.Unlock();
		Sleep( 2000 );
	}

	TRACE("ThreadChecking terminated.\n");

	return 0;
}

static UINT ThreadConnecting( LPVOID lpVoid )
{
	TRACE( "\nthread connecting.\n" );

	CYJSignBoard	*lpMain = (CYJSignBoard*)lpVoid;

	if ( lpMain->m_Serial.IsConnected() )
	{
		lpMain->Disconnect();
	}

	while ( true )
	{
		TRACE( "thread connecting while loop.\n" );

		if ( lpMain->m_csThreadConnecting.m_hObject != INVALID_HANDLE_VALUE )
			lpMain->m_csThreadConnecting.Lock();

		bool bPortOpen = lpMain->m_Serial.OpenPort( lpMain->m_sPort,
													 lpMain->m_dwBaud,
													 lpMain->m_btDataBit,
													 lpMain->m_btStopBit,
													 lpMain->m_btParity );

		lpMain->m_csThreadConnecting.Unlock();

		if ( bPortOpen )
		{
			TRACE("Serial port open success.\n");
			//�������� ��Ʈ�� �����͸����� ����Ǿ��ٰ� �� �� ����. [TMODE]�̰� [GETMODE]�� ���� [PM2] ������ �;� �����̴�.


			//���� üũ ������ ����
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

			//lpMain->m_csThreadConnecting.Unlock();
		}
		else
		{
			TRACE( "Serial port open failed. retry...\n" );
			lpMain->m_nStatus = CYJSignBoard::FAILTOCONNECT;
			::SendMessageA( lpMain->m_hParentWnd, WM_MESSAGE_SIGNBOARD, lpMain->m_nStatus, 0 );
			Sleep( 2000 );
			continue;
		}
		
		break;
	}

	TRACE("Connecting thread terminated.\n");
	return 0;
}

void CYJSignBoard::CallbackFromSerialPort( DWORD dwData, char* data )
{
	TRACE( "data received : %s\n", data );
	g_pMain->OnReceiveDataFromController( dwData, 0 );
}

CYJSignBoard::CYJSignBoard(void)
{
	m_hParentWnd= NULL;
	m_sPort		= "COM1";
	m_dwBaud	= 9600;
	m_btDataBit	= 8;
	m_btStopBit	= ONESTOPBIT;
	m_btParity	= NOPARITY;

	m_bIsTMode	= false;
	m_pThreadConnecting	= NULL;
	m_pThreadChecking	= NULL;

	g_pMain		= this;
	m_nFailCount= 0;
	m_bInRepeatMessage = false;
	m_Serial.SetCallbackFunction( CallbackFromSerialPort );
}

CYJSignBoard::~CYJSignBoard(void)
{
	Disconnect();
}

bool CYJSignBoard::Connect( bool bRetryConnect /* = true */ )
{
	TRACE( "\nconnecting...\n" );

	m_nStatus = CONNECTING;
	::SendMessageA( m_hParentWnd, WM_MESSAGE_SIGNBOARD, m_nStatus, 0 );

	if ( m_hParentWnd == NULL )
	{
		return false;
	}

	if ( m_sPort == "" )
	{
		return false;
	}

	m_pThreadConnecting = ::AfxBeginThread( ThreadConnecting, this, THREAD_PRIORITY_NORMAL, 0, 0, NULL);	

	if ( m_pThreadConnecting == NULL )
	{
		m_nStatus = FAILTOCONNECT;
		::SendMessageA( m_hParentWnd, WM_MESSAGE_SIGNBOARD, m_nStatus, 0 );
		TRACE("connecting thread creation failed.\n");
		return false;
	}
	else
	{
		
	}

	TRACE("connecting thread creation success.\n");
	return true;
}

bool CYJSignBoard::Disconnect()
{
	MakePacketAndSend( "[END]" );

	if ( m_Serial.m_bConnected )
		m_Serial.ClosePort();

	return true;
}

int CYJSignBoard::MakePacketAndSend( BYTE* bt, int n, int nDelay /*= 1*/ )
{
	DWORD nWritten = m_Serial.WriteComm( bt, n );

	if ( nWritten > 0 )
		TRACE( "%d(%d) bytes write success.\n", nWritten, n );
	else
		TRACE( "%d(%d) bytes write fail.\n", nWritten, n );

/*
	if ( nDelay > 0 )
		Wait( nDelay );
*/
	return nWritten;
}

int CYJSignBoard::MakePacketAndSend( CString sPacktet, int nDelay /*= 1*/ )
{
	int		i;
	BYTE	bt[100];


	if ( sPacktet == "[TMODE]" )
		sPacktet.Insert( sPacktet.GetLength(), 13 );

	TRACE( "\nsPacket = %s(%d)\n", sPacktet, sPacktet.GetLength() );

	for ( i = 0; i < sPacktet.GetLength(); i++ )
		bt[i] = (BYTE)sPacktet.GetAt(i);

// 	if ( sPacktet == "[TMODE]" )
// 		bt[i++] = 0x0d;

	return MakePacketAndSend( bt, i, nDelay );
}

LRESULT CYJSignBoard::OnReceiveDataFromController( WPARAM wParam, LPARAM lParam )
{
	int		i;
	int		iSize = (m_Serial.m_QueueRead).GetSize();	//��Ʈ�� ���� ������ ����
	BYTE	bt;
	CString str;
	CString sData = "";

	str.Format( "%d bytes received : ", iSize );
	//m_Edit_Log.AppendToLog( str, DARKPINK, false );

	m_csLastData.Lock();
	//m_bIsTMode = false;

	for ( i = 0 ; i < iSize; i++ )				//���� ���� ��ŭ �����͸� �о�� ȭ�鿡 ������
	{
		(m_Serial.m_QueueRead).GetByte( &bt );	//ť���� ������ �Ѱ��� �о��
		str.Format( "%02x ", bt );
		sData.Insert( i, bt );
		TRACE( str );
		//m_Edit_Log.AppendToLog( str, PINK, false );
	}

 	if ( sData.Find( "[PM2]" ) >= 0 )
 		m_bIsTMode = true;


	TRACE( "%s\n", sData );
	m_csLastData.Unlock();


	::SendMessageA( m_hParentWnd, WM_MESSAGE_SIGNBOARD, READ_COMM, (LPARAM)&sData );

/*
	if ( iSize > 0 )
		m_Edit_Log.AppendToLog( sData, DARKPINK, false );

	m_Edit_Log.Append( "\n" );
*/

	return 1;
}

void CYJSignBoard::DisplaySignBoardMessage( int nRow, CString sMsg, int nColor, int px /*= -1*/ )
{
	CString str;

	//�� ó���� ���ϸ� �ݺ��޽����� ������ ǥ�õǰų� ���� ǥ�ð� ���� �ʴ´�.
	if ( m_bInRepeatMessage )
	{
		MakePacketAndSend( "[COMON]" );
		MakePacketAndSend( "[FINE]" );
		MakePacketAndSend( "[COMOFF]" );

		m_bInRepeatMessage = false;
	}

	MakePacketAndSend( "[COMON]" );

	if ( nRow == 0 )
		MakePacketAndSend( "[rtx0 rty0 rtw96 rth16]" );
	else
		MakePacketAndSend( "[rtx0 rty16 rtw96 rth16]" );

	//px=-1(�߾�����), px>=0(x��ǥ)
	str.Format( "[px%d py0 cr%d]%s[act0]", (px == -1) ? ( 16 * 6 - (sMsg.GetLength() * 8) ) / 2 : px, nColor, sMsg );
	MakePacketAndSend( str );
	MakePacketAndSend( "[COMOFF]" );
}
