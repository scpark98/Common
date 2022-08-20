// FtpProgressCtrl.cpp : implementation file
//

#include "stdafx.h"
#include "DSClient.h"
#include "FtpProgressCtrl.h"

#define TIMER_CHECK_DOWNLOAD	0

// CFtpProgressCtrl

IMPLEMENT_DYNAMIC(CFtpProgressCtrl, CProgressCtrl)

CFtpProgressCtrl::CFtpProgressCtrl()
{
	// create an internet session
	m_pInternetSession = new CInternetSession( NULL, 1, INTERNET_OPEN_TYPE_DIRECT, NULL, NULL, NULL );

	// if Not good, show message + return
	// should never failed anyway
	if ( !m_pInternetSession )
	{
		AfxMessageBox("Can't start internet session");
		return;
	}

	m_hParentWnd		= NULL;
	m_pFtpConnection	= NULL;

	m_bAbortDownloading	= FALSE;
	m_bAbortUploading	= FALSE;

	m_nBufferSize		= FTP_BUFFER_SIZE;
}

CFtpProgressCtrl::~CFtpProgressCtrl()
{
	// close the internet session
	m_pInternetSession->Close();

	// delete the session
	if ( m_pInternetSession != NULL )
		delete m_pInternetSession;
}


BEGIN_MESSAGE_MAP(CFtpProgressCtrl, CProgressCtrl)
	ON_WM_TIMER()
END_MESSAGE_MAP()


BOOL CFtpProgressCtrl::Connect( HWND hParentWnd, CString sServerIP, CString sID, CString sPass, int nPort /*= 21*/, BOOL bPassive /*= FALSE*/ )
{
	if ( sServerIP == "" || sID == "" || sPass == "" )
		return FALSE;

	m_hParentWnd	= hParentWnd;
	m_sUser			= sID;
	m_sPass			= sPass;
	m_nPort			= nPort;
	m_bPassive		= bPassive;

	// put the server name in the CFtpGet class
	m_sServerIP = sServerIP;

	try
	{
		// try to connect to a ftp server
		m_pFtpConnection = m_pInternetSession->GetFtpConnection( m_sServerIP, m_sUser, m_sPass, m_nPort, m_bPassive );
	}
	catch (CInternetException* pEx) 
	{ 
		// if failed, just show the error

		// Oops! We failed to connect!
		TCHAR szErr[1024];
		pEx->GetErrorMessage(szErr, 1024);
		TRACE(szErr);
		//		AfxMessageBox(szErr);
		pEx->Delete();
		return 0;// return 1 but previous error box have been showed
	}


	return 1;

}

BOOL CFtpProgressCtrl::CloseConnection()
{
	// close the connection to server, you can reconnect latter
	if ( m_pFtpConnection == NULL )
		return FALSE;

	try
	{
		m_pFtpConnection->Close();
	}
	catch(...)
	{
		return FALSE;
	}

	if ( m_pFtpConnection != NULL )
		delete m_pFtpConnection;

	return TRUE;
}

BOOL CFtpProgressCtrl::GetFile( CString remoteFile, CString localFile, BOOL bSendMessage /*= TRUE*/  )
{
	m_sRemoteFile	= remoteFile;
	m_sLocalFile	= localFile;
	m_bSendMessage	= bSendMessage;

	if ( !IsConnected() )
	{
		Connect( m_hParentWnd, m_sServerIP, m_sUser, m_sPass, m_nPort, m_bPassive );
	}

	SetTimer( TIMER_CHECK_DOWNLOAD, 1000, NULL );
	m_bAbortDownloading = FALSE;

	if ( TRUE )
	{
		//TRACE( "GetFile\n" );
		CInternetFile* pIFile = NULL;

		m_nTotalDownloaded = 0;
		SetPos( m_nTotalDownloaded );

		try
		{
			//SetTimer( TIMER_CHECK_DOWNLOAD, 5000, NULL );
			pIFile = m_pFtpConnection->OpenFile( m_sRemoteFile, GENERIC_READ, FTP_TRANSFER_TYPE_BINARY | INTERNET_FLAG_RELOAD, 0 );
		}
		catch ( CInternetException* pEx )
		{ 
			KillTimer( TIMER_CHECK_DOWNLOAD );

			TCHAR szErr[1024];
			pEx->GetErrorMessage(szErr, 1024);
			TRACE( "try catch error at OpenFile = %s\n", szErr);
			::SendMessage( m_hParentWnd, MESSAGE_FTP_TRANSFER_DATA, FTP_ERROR_REMOTE_FILE_OPEN_FAIL, 0 );
			pEx->Delete();
			return FALSE;// return 1 but previous error box have been showed
		}

		if ( pIFile == NULL )
		{
			KillTimer( TIMER_CHECK_DOWNLOAD );

			TRACE( "=================================can't open m_sRemoteFile : %s\n", m_sRemoteFile );
			::SendMessage( m_hParentWnd, MESSAGE_FTP_TRANSFER_DATA, FTP_ERROR_REMOTE_FILE_OPEN_FAIL, 0 );
			return FALSE;
		}


		//TRACE( "GetFile-before download\n" );

		FILE*	fp = fopen( m_sLocalFile, "wb" );

		if ( !fp )
		{
			KillTimer( TIMER_CHECK_DOWNLOAD );
			TRACE( "can't create local file : %s\n", m_sLocalFile );
			::SendMessage( m_hParentWnd, MESSAGE_FTP_TRANSFER_DATA, FTP_ERROR_LOCAL_FILE_CREATE_FAIL, 0 );
			return FALSE;
		}

		int		nRead;
		BYTE*	buffer = new BYTE[m_nBufferSize];

		while ( TRUE )
		{
			if ( !m_pFtpConnection || m_bAbortDownloading )
			{
				delete[] buffer;
				pIFile->Close();
				delete pIFile;
				fclose( fp );
				return FALSE;
			}

			//TRACE( "GetFile-downloading\n" );

			nRead = pIFile->Read( buffer, m_nBufferSize );
			//TRACE( "nRead = %d\n", nRead );

			if ( nRead > 0 )
				KillTimer( TIMER_CHECK_DOWNLOAD );

			fwrite( buffer, sizeof(byte), nRead, fp );
			m_nTotalDownloaded += nRead;
			SetPos( m_nTotalDownloaded );

			if ( m_hParentWnd && m_bSendMessage )
				::SendMessage( m_hParentWnd, MESSAGE_FTP_TRANSFER_DATA, FTP_RECEIVED_DATA, m_nTotalDownloaded );

			if ( nRead < m_nBufferSize )
				break;
			// 			else if ( nRead == m_nBufferSize )
			// 			{
			// 				m_nBufferSize += 4096;
			// 				TRACE( "buffer size = %d\n", m_nBufferSize );
			// 			}
		}

		delete[] buffer;
		pIFile->Close();
		delete pIFile;
		fclose( fp );

		return TRUE;
	}
	else
	{
		// Try to get the file
		BOOL bGotFile = m_pFtpConnection->GetFile(	m_sRemoteFile,
			m_sLocalFile,
			FALSE,
			FILE_ATTRIBUTE_NORMAL,	
			FTP_TRANSFER_TYPE_BINARY | INTERNET_FLAG_RELOAD | INTERNET_FLAG_NO_CACHE_WRITE );

		// 		if ( bGotFile && m_bDeleteAfterGet )
		// 			m_pFtpConnection->Remove( m_sRemoteFile );

		return bGotFile ? 1 : 0 ;
		// if bGotFile is 0 ( FALSE ), return 0
		// if bGotFile is 1 ( TRUE  ), return 1
	}
}

void CFtpProgressCtrl::OnTimer(UINT_PTR nIDEvent)
{
	// TODO: Add your message handler code here and/or call default
	if ( nIDEvent == TIMER_CHECK_DOWNLOAD )
	{
		KillTimer( TIMER_CHECK_DOWNLOAD );

		AfxMessageBox( "download timer call" );
		GetFile( m_sRemoteFile, m_sLocalFile, m_bSendMessage );

		if ( m_hParentWnd && m_bSendMessage )
			::SendMessage( m_hParentWnd, MESSAGE_FTP_TRANSFER_DATA, FTP_TRANSFER_RETRY, m_nTotalDownloaded );
	}

	CProgressCtrl::OnTimer(nIDEvent);
}
