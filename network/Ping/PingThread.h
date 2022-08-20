// PingThread.h: interface for the CPingThread class.
//
//////////////////////////////////////////////////////////////////////
/* Usage :
- in .h
#include "../../Common/network/Ping/PingThread.h"
...
	CPingThread	m_Ping;
	LRESULT	OnMessagePing( WPARAM wParam, LPARAM lParam );

- in .cpp
	m_Ping.StartPing( -1, m_sEdit_Address, m_hWnd );
	...
	m_Ping.StopPing();

//ping message receive handler
LRESULT	CTest_PingDlg::OnMessagePing( WPARAM wParam, LPARAM lParam )
{
	if ( wParam < 1 || wParam > 3 )
		return 1;

	char *str = (char*)lParam;

	m_Rich_Log.AppendToLog( str );

	return 0;
}
*/

#if !defined(AFX_PINGTHREAD_H__88E7C296_2B7A_4A67_AA7B_C89EA8D3BAAC__INCLUDED_)
#define AFX_PINGTHREAD_H__88E7C296_2B7A_4A67_AA7B_C89EA8D3BAAC__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "Ping.h"
class CPingThread  
{
public:
	CPingThread();
	virtual ~CPingThread();

	UINT	m_dwID;
	HANDLE	m_hThread;
	HANDLE	m_hKillEvent;
	HANDLE	m_hSignalEvent;

	static UINT __stdcall  ThreadProc(void* pThis);

	bool	IsSocketInitialized() { return m_bSocketInit; }
	//if nRetries is under 0, infinite ping test.
	void	StartPing(int nRetries,CString strHost,HWND hWnd);
	void	StopPing();

	CPing	m_ping;
	bool	IsPinging() { return m_bPinging; }
	CString m_strHost;
	HWND	m_hWnd;
	int		m_nRetries;
protected:
	WSAData wsaData;
	bool	m_bSocketInit;
	bool	m_bPinging;
};

#endif // !defined(AFX_PINGTHREAD_H__88E7C296_2B7A_4A67_AA7B_C89EA8D3BAAC__INCLUDED_)
