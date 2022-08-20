#if !defined(AFX_COMM_THREAD_INCLUDED_)
#define AFX_COMM_THREAD_INCLUDED_
#endif

#pragma once
/*
#define DLE		0x10
#define STX		0x02
#define ETX		0x03
*/
#define WM_COMM_READ		WM_USER + 7000
#define BUFF_SIZE			1024


//	통신 클래스	CCommThread 

// 포트에서 읽기 :
//   포트를 연 후에 포트에 자료가 도착하면 WM_COMM_READ 메시지가 메인 
//   윈도우에 전달된다. ON_MESSAGE 매크로를 이용, 함수를 연결하고
//   m_ReadData String에 저장된 데이터를 이용 읽기
// 포트에 쓰기 : WriteComm(buff, 30)과 같이 버퍼와 그 크기를 건네면 된다.
class CQueue
{
public:
	BYTE buff[BUFF_SIZE];//큐 버퍼 사이즈는 4192로 만듬 
	int m_iHead, m_iTail;//큐 버퍼에 데이터를 넣고 빼 올때 사용할 변수 
	CQueue();
	void Clear();//버퍼를 초기화 시킴
	int GetSize();//현재 버퍼에 들어있는 데이터의 size를 리턴
	BOOL PutByte(BYTE b);//큐버퍼에 1바이트를 넣음
	BOOL GetByte(BYTE *pb);//큐버퍼에서 1바이트를 빼 옴
};

class	CCommThread
{
public:
	
	CCommThread();
	~CCommThread();

	//--------- 환경 변수 -----------------------------------------//
	HWND		m_hParentWnd;
	//CWnd*		m_pParentWnd;
	int			m_nCommID;
	//bool		m_bMessageCall;			//bMessageCall이 true이면 포트 데이터 전송을 메시지 방식으로 전달하고 false이면 함수콜 방식으로 메인에 전달한다.
	void		(*m_pCallback_func)(DWORD, char*);

	BOOL        check;
	HANDLE		m_hComm;				// 통신 포트 파일 핸들
	CString		m_sPortName;			// 포트 이름 (COM1 ..)
	BOOL		m_bConnected;			// 포트가 열렸는지 유무를 나타냄.
	OVERLAPPED	m_osRead, m_osWrite;	// 포트 파일 Overlapped structure
	HANDLE		m_hThreadWatchComm;		// Watch함수 Thread 핸들.
	WORD		m_wPortID;				// WM_COMM_READ와 함께 보내는 인수.
	CQueue      m_QueueRead;			//큐버퍼

	DWORD		m_tReadData;


	//--------- 외부 사용 함수 ------------------------------------//
	void		SetParentHwnd( HWND hWnd );	//메시지 패싱으로 이벤트를 전달할 경우 사용.
	void		SetCallbackFunction( void (*p_func)(DWORD, char*) ) { m_pCallback_func = p_func; }	//콜백함수 콜 방식으로 이벤트를 전달할 경우 사용.

	void		SetInstanceID( int nCommID );		// 이 클래스를 2개 이상의 인스턴스가 사용할 경우 수신시 구분자로 사용
	BOOL		OpenPort( CString strPortName, DWORD dwBaud, BYTE byData, BYTE byStop, BYTE byParity );//포트 열기 
	void		ClosePort();				//포트 닫기
	DWORD		WriteComm(BYTE *pBuff, DWORD nToWrite);//포트에 데이터 쓰기
	void		ClearBuffer() { m_QueueRead.Clear(); }
	BOOL		IsConnected() { return m_bConnected; }
	CString		GetPortName() { return m_sPortName.Mid(4); }

	//--------- 내부 사용 함수 ------------------------------------//
	DWORD		ReadComm(BYTE *pBuff, DWORD nToRead);//포트에서 데이터 읽어오기

protected:

};

// Thread로 사용할 함수 
DWORD	ThreadWatchComm(CCommThread* pComm);

