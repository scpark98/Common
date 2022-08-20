//
// THIS CODE IS BASED ON THE CODE FROM 
// THE BOOK WINSOCK 2.0 BY LEWIS NAPPER...
// 
//

#include "stdafx.h"
#include "ping.h"

void CPing::Ping(int nRetries,LPCSTR pstrHost,HWND hWnd)
{
	SOCKET		rawSocket;
	LPHOSTENT	lpHost;
	int			nRet;
	int			nLoop = nRetries;
	struct		 sockaddr_in saDest;
	struct		sockaddr_in saSrc;
	u_char		cTTL;
	CString		str;
	
	m_bPinging = true;
	if ( nLoop < 0 )
		nLoop = 0;

	m_hWnd = hWnd;


	ASSERT(IsWindow(hWnd));
	// Create a Raw socket
	rawSocket = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
	if (rawSocket == SOCKET_ERROR) 
	{
		WSAError("socket()");
		return;
	}

	// Lookup host
	lpHost = gethostbyname(pstrHost);
	if (lpHost == NULL)
	{
		//str.Format(_T("Host not found: %s"), pstrHost);
		::SendMessage(m_hWnd, MESSAGE_PING_STATUS, PING_STATUS_HOST_NOT_FOUND, (LPARAM)0);
		return;
	}

	// Setup destination socket address
	saDest.sin_addr.s_addr = *((u_long FAR *) (lpHost->h_addr));
	saDest.sin_family = AF_INET;
	saDest.sin_port = 0;

	// Tell the user what we're doing
	str.Format(_T("\nPinging %s [%s] with %d bytes of data:"),
		pstrHost,
		inet_ntoa(saDest.sin_addr),
		REQ_DATASIZE);
	::SendMessage(m_hWnd, MESSAGE_PING_STATUS, PING_STATUS_START_SIGNAL, (LPARAM)REQ_DATASIZE);

	// Ping multiple times
	while ( true )
	{
		// Send ICMP echo request
		SendEchoRequest(rawSocket, &saDest);

		nRet = WaitForEchoReply(rawSocket);
		if (nRet == SOCKET_ERROR)
		{
			WSAError("select()");
			break;
		}
		if (!nRet)
		{
			str.Format(_T("Request Timed Out"));
			::SendMessage(m_hWnd, MESSAGE_PING_STATUS, PING_STATUS_REQUEST_TIMED_OUT, (LPARAM)0);
		}
		else
		{
			// Receive reply
			DWORD dw = RecvEchoReply(rawSocket, &saSrc, &cTTL);

			// Calculate elapsed time
			str.Format(_T("Reply[%d] from: %s: bytes=%d time=%ldms TTL=%d"), 
				(nRetries > 0 ? nLoop-- : nLoop++),
				inet_ntoa(saSrc.sin_addr), 
				REQ_DATASIZE,
				GetTickCount() - dw,
				cTTL);
			TRACE(_T("%s\n"), str);
			::SendMessage(m_hWnd, MESSAGE_PING_STATUS, PING_STATUS_ALIVE, (LPARAM)(GetTickCount() - dw));

			if ( !m_bPinging )
				break;

			if ( nRetries > 0 && nLoop <= 0 )
				break;

			Sleep(1000);
		}
	}

	::SendMessage( m_hWnd, MESSAGE_PING_STATUS, PING_STATUS_END_SIGNAL, 0 );

	nRet = closesocket(rawSocket);
	if (nRet == SOCKET_ERROR)
		WSAError("closesocket()");

}


int CPing::SendEchoRequest(SOCKET s,LPSOCKADDR_IN lpstToAddr) 
{
	static ECHOREQUEST echoReq;
	static int nId = 1;
	static int nSeq = 1;
	int nRet;

	// Fill in echo request
	echoReq.icmpHdr.Type		= ICMP_ECHOREQ;
	echoReq.icmpHdr.Code		= 0;
	echoReq.icmpHdr.Checksum	= 0;
	echoReq.icmpHdr.ID			= nId++;
	echoReq.icmpHdr.Seq			= nSeq++;

	// Fill in some data to send
	for (nRet = 0; nRet < REQ_DATASIZE; nRet++)
		echoReq.cData[nRet] = ' '+nRet;

	// Save tick count when sent
	echoReq.dwTime				= GetTickCount();

	// Put data in packet and compute checksum
	echoReq.icmpHdr.Checksum = in_cksum((u_short *)&echoReq, sizeof(ECHOREQUEST));

	// Send the echo request  								  
	nRet = sendto(s,			/* socket */
		(LPSTR)&echoReq,		/* buffer */
		sizeof(ECHOREQUEST),
		0,						/* flags */
		(LPSOCKADDR)lpstToAddr, /* destination */
		sizeof(SOCKADDR_IN));   /* address length */

	if (nRet == SOCKET_ERROR) 
		WSAError("sendto()");
	return (nRet);
}


DWORD CPing::RecvEchoReply(SOCKET s, LPSOCKADDR_IN lpsaFrom, u_char *pTTL) 
{
	ECHOREPLY echoReply;
	int nRet;
	int nAddrLen = sizeof(struct sockaddr_in);

	// Receive the echo reply	
	nRet = recvfrom(s,			// socket
		(LPSTR)&echoReply,		// buffer
		sizeof(ECHOREPLY),		// size of buffer
		0,						// flags
		(LPSOCKADDR)lpsaFrom,	// From address
		&nAddrLen);				// pointer to address len

	// Check return value
	if (nRet == SOCKET_ERROR) 
		WSAError("recvfrom()");

	// return time sent and IP TTL
	*pTTL = echoReply.ipHdr.TTL;

	return(echoReply.echoRequest.dwTime);   		
}


int CPing::WaitForEchoReply(SOCKET s)
{
	struct timeval Timeout;
	fd_set readfds;

	readfds.fd_count = 1;
	readfds.fd_array[0] = s;
	Timeout.tv_sec = 1;
	Timeout.tv_usec = 0;

	return(select(1, &readfds, NULL, NULL, &Timeout));
}

void CPing::WSAError(LPCSTR lpMsg)
{
	//CString str;
	//str.Format(_T("%s - WSAError: %ld"),lpMsg,WSAGetLastError());	
	::PostMessage( m_hWnd, MESSAGE_PING_STATUS, -1, (LPARAM)WSAGetLastError());
}




//
// Mike Muuss' in_cksum() function
// and his comments from the original
// ping program
//
// * Author -
// *	Mike Muuss
// *	U. S. Army Ballistic Research Laboratory
// *	December, 1983

/*
*			I N _ C K S U M
*
* Checksum routine for Internet Protocol family headers (C Version)
*
*/
u_short CPing::in_cksum(u_short *addr, int len)
{
	register int nleft = len;
	register u_short *w = addr;
	register u_short answer;
	register int sum = 0;

	/*
	*  Our algorithm is simple, using a 32 bit accumulator (sum),
	*  we add sequential 16 bit words to it, and at the end, fold
	*  back all the carry bits from the top 16 bits into the lower
	*  16 bits.
	*/
	while( nleft > 1 )  {
		sum += *w++;
		nleft -= 2;
	}

	/* mop up an odd byte, if necessary */
	if( nleft == 1 ) {
		u_short	u = 0;

		*(u_char *)(&u) = *(u_char *)w ;
		sum += u;
	}

	/*
	* add back carry outs from top 16 bits to low 16 bits
	*/
	sum = (sum >> 16) + (sum & 0xffff);	/* add hi 16 to low 16 */
	sum += (sum >> 16);			/* add carry */
	answer = ~sum;				/* truncate to 16 bits */
	return (answer);
}
