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
	CSCPingParams(CString _addr, int _retry_total, int _timeout = 5000, CString _job_name = _T(""), int _status = -2, int _retry_count = -1, int _delay = 0)
	{
		addr = _addr;
		retry_total = _retry_total;
		timeout = _timeout;
		job_name = (_job_name.IsEmpty() ? addr : _job_name);
	}

	//input

	//id�� �� ping job�� �����Ϸ� ������ �� ���̵�� �׻� unique�ؾ��ϰ� ������� �ʾƾ��ϹǷ� ������ ���� �� �ִ�.
	//���� ��� listctrl�� index�� id�� ��µ� sort�� �Ѵٸ� id�� �ǹ� ��������.
	//���� addr�� key�� �Ѵ�.
	//int			id = -1;

	CString		addr = _T("");			//domain name or ip address
	int			retry_total = -1;		//-1 : infinite
	int			timeout = 5000;			//ms
	//job�� �����ϴ� Ű�μ� ""�̸� addr���� �����ؼ� ����Ѵ�.
	//�� �� ������ ping job�� �����ٸ� �����ص� ������
	//addr�� �ߺ��� �� �ִ� ���� �ݵ�� ����ؾ� ������� ������ �� �ִ�.
	CString		job_name;


	//out
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
		PING_THREAD_NOT_RUNNING = -1,	//thread�� ���� ���۵��� ���� ����
		PING_THREAD_IS_STOPPING,		//��� ������� �������� ����
		PING_THREAD_IS_RUNNING,			//���������� �������� ����
	};

	bool	init(HWND hParent);
	void	add(CString addr, int retry = -1, int timeout = 5000, CString _job_name = _T(""));
	void	thread_ping(CSCPingParams* params);

	//�ش� �ּ��� �� �����带 ����. ""�̸� ��� �� ����.
	void	stop(CString job_name = _T(""));
	bool	is_thread_running(CString job_name);
	bool	is_all_threads_stopped();

protected:
	HWND m_parent = NULL;

	//id�� thread ���� ���� ���� ��. ������ų �� �ʿ�. ���� ��� �����Ǿ����� Ȯ�νÿ��� �ʿ�.
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
