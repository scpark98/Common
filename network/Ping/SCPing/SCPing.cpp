#include "SCPing.h"

#include <thread>
#include <WinSock2.h>
#include <IcmpAPI.h>
#include <WS2tcpip.h>

#pragma comment(lib, "Iphlpapi.lib")
#pragma comment(lib, "Ws2_32.lib")

#pragma warning(disable:4996)

CSCPing::CSCPing()
{
	WSAData wsaData;

	if (WSAStartup(MAKEWORD(1, 1), &wsaData) != 0)
	{
		AfxMessageBox(_T("WSAStartup failed. Could not initialize socket."));
		return;
	}
}

CSCPing::~CSCPing()
{
	WSACleanup();
}

bool CSCPing::init(HWND hParent)
{
	m_parent = hParent;
	return true;
}

void CSCPing::add(CString job_name, CString addr, int retry, int timeout)
{
	CSCPingParams *params = new CSCPingParams(job_name, addr, retry, timeout);

	std::map<CString, int>::iterator it = m_map.find(job_name);

	//이미 등록된 장비라면
	if (it != m_map.end())
		m_map[params->job_name] = PING_THREAD_IS_RUNNING;
	else
		m_map.insert(std::pair<CString, bool>(params->job_name, PING_THREAD_IS_RUNNING));

	std::thread t(&CSCPing::thread_ping, this, params);
	t.detach();
}

//특정 job의 핑 쓰레드를 중지. ""이면 모든 핑 중지.
void CSCPing::stop(CString job_name)
{
	std::map<CString, int>::iterator it;

	if (!job_name.IsEmpty())
	{
		std::map<CString, int>::iterator it = m_map.find(job_name);

		if (it != m_map.end() && it->second == PING_THREAD_IS_RUNNING)
		{
			TRACE(_T("%s job will be stop\n"), job_name);
			it->second = PING_THREAD_IS_STOPPING;
			return;
		}
	}
	else
	{
		//모두 중지할 경우
		for (it = m_map.begin(); it != m_map.end(); it++)
		{
			if (it->second == PING_THREAD_IS_RUNNING)
				it->second = PING_THREAD_IS_STOPPING;
		}
	}

	//while (!is_all_threads_stopped())
	//{
	//	TRACE(_T("wait all threads stopped...\n"));
	//	//Sleep(1000);
	//}
}

bool CSCPing::is_thread_running(CString job_name)
{
	std::map<CString, int>::iterator it = m_map.find(job_name);
	return (it != m_map.end() && it->second == PING_THREAD_IS_RUNNING);
}

bool CSCPing::is_all_threads_stopped()
{
	for (auto it = m_map.begin(); it != m_map.end(); it++)
	{
		if (it->second != PING_THREAD_NOT_RUNNING)
			return false;
	}

	return true;
}

CString CSCPing::get_ip_error_string(DWORD error_code)
{
	//auto e = GetLastError();
	DWORD buf_size = 1024;
	TCHAR buf[1024] = { 0, };
	GetIpErrorString(error_code, buf, &buf_size);

	return CString(buf);
}

void CSCPing::thread_ping(CSCPingParams* params)
{
	CString str;

	// Create the ICMP context.
	HANDLE icmp_handle = IcmpCreateFile();
	if (icmp_handle == INVALID_HANDLE_VALUE)
	{
		m_map[params->job_name] = PING_THREAD_NOT_RUNNING;
		params->status = PING_STATUS_WSA_ERROR;
		::SendMessage(m_parent, Message_CSCPing, (WPARAM)params, (LPARAM)0);
		delete params;
		return;
	}

	// Parse the destination IP address.
	IN_ADDR dest_ip{};
	LPHOSTENT lpHost = gethostbyname(CT2CA(params->addr));
	if (lpHost == NULL)
	{
		m_map[params->job_name] = PING_THREAD_NOT_RUNNING;
		auto e = GetLastError();
		DWORD buf_size = 1024;
		TCHAR buf[1024] = { 0, };
		GetIpErrorString(e, buf, &buf_size);

		str = get_ip_error_string(GetLastError());
		params->status = PING_STATUS_HOST_NOT_FOUND;
		::SendMessage(m_parent, Message_CSCPing, (WPARAM)params, (LPARAM)0);
		delete params;
		return;
	}

	struct sockaddr_in sa_dst;
	sa_dst.sin_addr.s_addr = *((u_long FAR*) (lpHost->h_addr));
	sa_dst.sin_family = AF_INET;
	sa_dst.sin_port = 0;

	if (1 != InetPton(AF_INET, CString(inet_ntoa(sa_dst.sin_addr)), &dest_ip))
	{
		m_map[params->job_name] = PING_THREAD_NOT_RUNNING;
		params->status = PING_STATUS_NOT_DEFINED;
		::SendMessage(m_parent, Message_CSCPing, (WPARAM)params, (LPARAM)0);

		TRACE(_T("InetPton failed.\n"));
		throw;
	}

	params->addr_ip = CString(inet_ntoa(sa_dst.sin_addr));

	// Payload to send.
	constexpr WORD payload_size = 32;
	unsigned char payload[payload_size]{ 42 };

	params->payload_size = payload_size;


	// Reply buffer for exactly 1 echo reply, payload data, and 8 bytes for ICMP error message.
	constexpr DWORD reply_buf_size = sizeof(ICMP_ECHO_REPLY) + payload_size + 8;
	unsigned char reply_buf[reply_buf_size]{};

	params->retry_count = 0;

	//start signal
	params->status = PING_STATUS_START_SIGNAL;
	::SendMessage(m_parent, Message_CSCPing, (WPARAM)params, (LPARAM)0);

	while ((m_map[params->job_name] == PING_THREAD_IS_RUNNING) &&
		   (params->retry_total < 0 || (params->retry_count < params->retry_total)))
	{
		params->retry_count++;

		long t0 = clock();

		// Make the echo request.
		DWORD reply_count = IcmpSendEcho(icmp_handle, dest_ip.S_un.S_addr,
			payload, payload_size, NULL, reply_buf, reply_buf_size, params->timeout);

		// Return value of 0 indicates failure, try to get error info.
		if (reply_count == 0)
		{
			params->delay = clock() - t0;

			auto e = GetLastError();
			DWORD buf_size = 1000;
			WCHAR buf[1000];
			GetIpErrorString(e, buf, &buf_size);

			//IP_REQ_TIMED_OUT
			//str.Format(_T("IcmpSendEcho returned error %d (%s)"), e, buf);
			//TRACE(_T("%s\n"), str);
			if (e == IP_REQ_TIMED_OUT)
				params->status = PING_STATUS_REQUEST_TIMED_OUT;
			else
				params->status = e;
			if (m_map[params->job_name] == PING_THREAD_IS_RUNNING)
				::SendMessage(m_parent, Message_CSCPing, (WPARAM)params, (LPARAM)0);
		}
		else
		{
			const ICMP_ECHO_REPLY* r = (const ICMP_ECHO_REPLY*)reply_buf;

			params->delay = r->RoundTripTime;

			struct in_addr addr;
			addr.s_addr = r->Address;
			char* s_ip = inet_ntoa(addr);

			if (CString(s_ip) == params->addr_ip)
			{
				params->TTL = (int)r->Options.Ttl;
				params->status = PING_STATUS_ALIVE;
				if (m_map[params->job_name] == PING_THREAD_IS_RUNNING)
					::SendMessage(m_parent, Message_CSCPing, (WPARAM)params, (LPARAM)0);

				//str.Format(_T("Reply from: %s: bytes=%d time=%d ms TTL=%d"), CString(s_ip), r->DataSize, r->RoundTripTime, (int)r->Options.Ttl);
				//TRACE(_T("%s\n"), str);
			}
			else
			{
				params->status = PING_STATUS_REQUEST_TIMED_OUT;
				if (m_map[params->job_name] == PING_THREAD_IS_RUNNING)
					::SendMessage(m_parent, Message_CSCPing, (WPARAM)params, (LPARAM)0);
			}

			//std::cout << "Reply from: " << s_ip << ": bytes=" << r->DataSize << " time=" << r->RoundTripTime << "ms TTL=" << (int)r->Options.Ttl << std::endl;
		}

		if (params->delay >= 0 && params->delay < 1000)
			std::this_thread::sleep_for(std::chrono::milliseconds(1000 - params->delay));

		//std::this_thread::sleep_for(std::chrono::seconds(1));
	}

	// Close ICMP context.
	IcmpCloseHandle(icmp_handle);

	m_map[params->job_name] = PING_THREAD_NOT_RUNNING;

	TRACE(_T("ping %s stopped.\n"), params->job_name);

	delete params;
}
