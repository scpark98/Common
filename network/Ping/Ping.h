//
// Ping.h
//

//CPing은 실제 Ping을 위한 네트워크 관련 코드이므로
//CPingThread의 인스턴스를 이용하면 된다.

#pragma pack(1)

#define ICMP_ECHOREPLY	0
#define ICMP_ECHOREQ	8

#define MESSAGE_PING_STATUS	WM_USER + 0x0100

class CPing
{
public:
	HWND	m_hWnd;
	void	Ping(int nRetries,LPCSTR pstrHost,HWND hWnd);
	void	StopPing() { m_bPinging = false; }
	int		WaitForEchoReply(SOCKET s);
	// ICMP Echo Request/Reply functions
	int		SendEchoRequest(SOCKET, LPSOCKADDR_IN);
	DWORD	RecvEchoReply(SOCKET, LPSOCKADDR_IN, u_char *);
	u_short in_cksum(u_short *addr, int len);

	enum PING_STATUS
	{
		PING_STATUS_WSA_ERROR = -1,
		PING_STATUS_HOST_NOT_FOUND,
		PING_STATUS_START_SIGNAL,
		PING_STATUS_REQUEST_TIMED_OUT,
		PING_STATUS_ALIVE,
		PING_STATUS_END_SIGNAL,
	};
protected:
	void	WSAError(LPCSTR pstrFrom);
	bool	m_bPinging;
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
}IPHDR, *PIPHDR;


// ICMP Header - RFC 792
typedef struct tagICMPHDR
{
	u_char	Type;			// Type
	u_char	Code;			// Code
	u_short	Checksum;		// Checksum
	u_short	ID;				// Identification
	u_short	Seq;			// Sequence
	char	Data;			// Data
}ICMPHDR, *PICMPHDR;


#define REQ_DATASIZE 32		// Echo Request Data size

// ICMP Echo Request
typedef struct tagECHOREQUEST
{
	ICMPHDR icmpHdr;
	DWORD	dwTime;
	char	cData[REQ_DATASIZE];
}ECHOREQUEST, *PECHOREQUEST;


// ICMP Echo Reply
typedef struct tagECHOREPLY
{
	IPHDR	ipHdr;
	ECHOREQUEST	echoRequest;
	char    cFiller[256];
}ECHOREPLY, *PECHOREPLY;


#pragma pack()

