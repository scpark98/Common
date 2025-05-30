//#include "StdAfx.h"
#include "FtpThread.h"

CFtpThread::CFtpThread()
{
	m_hInternet		= NULL;
	m_hConnect		= NULL;
	m_hParentWnd	= NULL;
}

CFtpThread::~CFtpThread()
{
	CloseConnection();
}

BOOL CFtpThread::Connect( CString sServerIP, CString sID, CString sPass, int nPort /*= 21*/, BOOL bPassive /*= FALSE*/ )
{
	m_hInternet = InternetOpen(NULL, INTERNET_OPEN_TYPE_PRECONFIG, NULL, NULL, NULL);
	
	if (NULL == m_hInternet)
	{
		//AfxMessageBox("InternetOpen Error");
		return FALSE;
	}

	m_hConnect = InternetConnect(m_hInternet, sServerIP, nPort, sID, sPass, INTERNET_SERVICE_FTP, INTERNET_FLAG_PASSIVE | INTERNET_FLAG_ASYNC, 0);

	if (NULL == m_hConnect)
	{
		//AfxMessageBox("InternetConnect Error");
		return FALSE;
	}

	return TRUE;
}

void CFtpThread::CloseConnection()
{
	if ( m_hConnect )
	{
		InternetCloseHandle( m_hConnect );
		m_hConnect = NULL;
	}

	if ( m_hInternet )
	{
		InternetCloseHandle( m_hInternet );
		m_hInternet = NULL;
	}
}

BOOL CFtpThread::GetFile( CString sRemote, CString sLocal )
{
	if ( !FtpGetFile( m_hConnect, sRemote, sLocal, FALSE , NULL, INTERNET_FLAG_TRANSFER_BINARY | INTERNET_FLAG_RELOAD, 0) )
	{
		//AfxMessageBox("FTP GetFile Error");
		return FALSE;
	}

	return TRUE;
}

BOOL CFtpThread::PutFile( CString sLocal, CString sRemote )
{
	if ( !FtpPutFile( m_hConnect, sLocal, sRemote, INTERNET_FLAG_TRANSFER_BINARY | INTERNET_FLAG_RELOAD, 0) )
	{
		//AfxMessageBox("FTP PutFile Error");
		return FALSE;
	}

	return TRUE;
}
