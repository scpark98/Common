// PingThread.cpp: implementation of the CPingThread class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "PingThread.h"
#include <process.h>    /* _beginthread, _endthread */

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CPingThread::CPingThread()
{
	m_dwID = 0;  
	m_hThread = NULL;
	m_bPinging = false;
	m_hKillEvent =   CreateEvent(NULL,TRUE,FALSE,NULL);
	m_hSignalEvent = CreateEvent(NULL,FALSE,FALSE,NULL);

	m_bSocketInit = true;

	if (WSAStartup(MAKEWORD(1, 1), &wsaData) != 0) 
	{
		AfxMessageBox( _T("WSAStartup failed. Could not initialize socket.") );
		m_bSocketInit = false;
        return;
    }


	m_hThread = (HANDLE) _beginthreadex(NULL,			// Security
										0,				// Stack size - use default
										ThreadProc,     // Thread fn entry point
										(void*) this,	     
										0,				// Init flag
										&m_dwID);		// Thread address
}

CPingThread::~CPingThread()
{
	WSACleanup();

	SetEvent(m_hKillEvent);
	WaitForSingleObject(m_hThread,INFINITE);
}


UINT CPingThread::ThreadProc(void* lpParam)
{
	CPingThread* pThis = reinterpret_cast<CPingThread*>(lpParam);

	while (1)
	{
		HANDLE hObjects[2];
		hObjects[0] = pThis->m_hKillEvent;
		hObjects[1] = pThis->m_hSignalEvent;

		DWORD dwWait = WaitForMultipleObjects(2,hObjects,FALSE,INFINITE);
		if (dwWait == WAIT_OBJECT_0)		
			break;

		if (dwWait == WAIT_OBJECT_0 + 1)
			pThis->m_ping.Ping(pThis->m_nRetries, (LPCSTR)(LPCTSTR)(pThis->m_strHost), pThis->m_hWnd);
	}

	return 0;
}

void CPingThread::StartPing(int nRetries,CString strHost, HWND hWnd)
{
	m_nRetries = nRetries;
	m_strHost = strHost;
	m_hWnd = hWnd;
	m_bPinging = true;
	SetEvent(m_hSignalEvent);
}

void CPingThread::StopPing()
{
	m_ping.StopPing();
	m_bPinging = false;
}
