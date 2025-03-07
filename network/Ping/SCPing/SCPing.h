#pragma once

#ifndef __SCPING__
#define __SCPING__

#include <afxcmn.h>
#include <deque>
#include <atomic>
#include <chrono>
#include <future>
#include <map>

#pragma pack(1)

#define ICMP_ECHOREPLY	0
#define ICMP_ECHOREQ	8

static const UINT Message_CSCPing = ::RegisterWindowMessage(_T("MessageString_CSCPing"));

// stopwatch. Returns time in seconds
class timer {
public:
	std::chrono::time_point<std::chrono::high_resolution_clock> lastTime;
	timer() : lastTime(std::chrono::high_resolution_clock::now()) {}
	inline double elapsed() {
		std::chrono::time_point<std::chrono::high_resolution_clock> thisTime = std::chrono::high_resolution_clock::now();
		double deltaTime = std::chrono::duration<double>(thisTime - lastTime).count();
		lastTime = thisTime;
		return deltaTime;
	}
};

class CSCPing;

class CSCPingParams
{
public:
	CSCPingParams() {};
	CSCPingParams(CString _job_name, CString _addr, int _retry_total, int _timeout = 5000)
	{
		job_name = _job_name;
		addr = _addr;
		retry_total = _retry_total;
		timeout = _timeout;
	}

	//input
	CString		job_name;				//각 job을 구분할 수 있는 unique 키값이어야 한다.
	CString		addr = _T("");			//domain name or ip address
	int			retry_total = -1;		//-1 : infinite
	int			timeout = 5000;			//ms

	//output
	CString		addr_ip = _T("");		//converted ip address
	int			status = -2;
	int			retry_count = -1;
	int			delay = 0;
	int			payload_size = 0;
	int			TTL = 0;
};

class CSCPing
{
public:
	CSCPing();
	~CSCPing();

	enum PING_STATUS
	{
		PING_STATUS_WSA_ERROR = -1,
		PING_STATUS_HOST_NOT_FOUND,
		PING_STATUS_START_SIGNAL,
		PING_STATUS_REQUEST_TIMED_OUT,
		PING_STATUS_ALIVE,
		PING_STATUS_END_SIGNAL,
		PING_STATUS_NOT_DEFINED,
		PING_STATUS_SOCKET_ERROR,
	};

	enum PING_THREAD_STATUS
	{
		PING_THREAD_NOT_RUNNING = -1,	//thread가 아직 시작되지 않은 상태
		PING_THREAD_IS_STOPPING,		//취소 명령으로 중지중인 상태
		PING_THREAD_IS_RUNNING,			//정상적으로 실행중인 상태
	};

	bool	init(HWND hParent);
	void	add(CString job_name, CString addr, int retry = -1, int timeout = 5000);
	void	thread_ping(CSCPingParams* params);

	//특정 job의 핑 쓰레드를 중지. ""이면 모든 핑 중지.
	void	stop(CString job_name = _T(""));
	bool	is_thread_running(CString job_name);
	bool	is_all_threads_stopped();

protected:
	HWND m_parent = NULL;

	//id와 thread 동작 유무 저장 맵. 중지시킬 때 필요. 또한 모두 중지되었는지 확인시에도 필요.
	//1:running, 0:stop thread, -1:stopped
	std::map<CString, int> m_map;

	int		WaitForEchoReply(SOCKET s);
	// ICMP Echo Request/Reply functions
	int		SendEchoRequest(SOCKET, LPSOCKADDR_IN);
	DWORD	RecvEchoReply(SOCKET, LPSOCKADDR_IN, u_char*);
	u_short in_cksum(u_short* addr, int len);
	void	WSAError(LPCSTR pstrFrom);

	CString get_ip_error_string(DWORD error_code);
};

// IP Header -- RFC 791
typedef struct tagIPHDR
{
	u_char  VIHL;			// Version and IHL
	u_char	TOS;			// Type Of Service
	short	TotLen;			// Total Length
	short	ID;				// Identification
	short	FlagOff;		// Flags and Fragment Offset
	u_char	TTL;			// Time To Live
	u_char	Protocol;		// Protocol
	u_short	Checksum;		// Checksum
	struct	in_addr iaSrc;	// Internet Address - Source
	struct	in_addr iaDst;	// Internet Address - Destination
}IPHDR, * PIPHDR;

// ICMP Header - RFC 792
typedef struct tagICMPHDR
{
	u_char	Type;			// Type
	u_char	Code;			// Code
	u_short	Checksum;		// Checksum
	u_short	ID;				// Identification
	u_short	Seq;			// Sequence
	char	Data;			// Data
}ICMPHDR, * PICMPHDR;


#define REQ_DATASIZE 32		// Echo Request Data size

// ICMP Echo Request
typedef struct tagECHOREQUEST
{
	ICMPHDR icmpHdr;
	DWORD	dwTime;
	char	cData[REQ_DATASIZE];
}ECHOREQUEST, * PECHOREQUEST;


// ICMP Echo Reply
typedef struct tagECHOREPLY
{
	IPHDR	ipHdr;
	ECHOREQUEST	echoRequest;
	char    cFiller[256];
}ECHOREPLY, * PECHOREPLY;

#pragma pack()

#endif
