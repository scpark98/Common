#pragma once

/*
* m_ftp.connect(...);
* //�ϳ��� ����� n���� �ۼ����� �����Ѱ�?
* //n���� �ۼ����� ���� �������� thread�� �����ؾ� �Ѵ�.
* m_ftp.put(local, remote);
* m_ftp.get(remote, local);
*/

#ifndef __SCFTP_CLIENT__
#define __SCFTP_CLIENT__

#include <afxcmn.h>
#include <deque>
#include <atomic>
#include <chrono>
#include <future>
#include <map>

#include <wininet.h>

#pragma comment(lib, "wininet.lib")

#pragma pack(1)

#define ICMP_ECHOREPLY	0
#define ICMP_ECHOREQ	8

//static const UINT Message_CSCFtp = ::RegisterWindowMessage(_T("MessageString_CSCFtp1111111111"));
#define Message_CSCFtp WM_USER + 0x352

class CSCFtpMessage
{
public:
	CSCFtpMessage(int _msg, int _index = -1, int _percentage = -1)
	{
		msg = _msg;
		index = _index;
		percentage = _percentage;
	}

	int		msg = -1;
	int		index = -1;			//listctrl�� index�� �ƴ� SetItemData()�� �Ҵ�� data��
	int		percentage = 0;
};

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

class CSCFtp;

class CSCFtpParams
{
public:
	CSCFtpParams() {};
	CSCFtpParams(int _job_index, CString _local_path, CString _remote_path)
	{
		job_index = _job_index;
		local_path = _local_path;
		remote_path = _remote_path;
	}

	//input
	int			job_index;				//�� job�� ������ �� �ִ� unique Ű���̾�� �Ѵ�.
	CString		addr = _T("");			//domain name or ip address
	int			port = 0;
	CString		user;
	CString		password;
	int			retry_total = -1;		//-1 : infinite
	int			timeout = 5000;			//ms
	CString		local_path;				//local path
	CString		remote_path;			//remote path


	//output
	int			retry_count = -1;
	int			delay = 0;
};

class CSCFtp
{
public:
	CSCFtp();
	~CSCFtp();

	enum SCFTP_STATUS
	{
		FTP_STATUS_UPLOAD_STARTED = 0,
		FTP_STATUS_UPLOAD_FILEOPEN_FAILED,
		FTP_STATUS_UPLOAD_SKIPPED,
		FTP_STATUS_UPLOAD_SENDING,
		FTP_STATUS_UPLOAD_COMPLETED,

	};

	enum PING_THREAD_STATUS
	{
		FTP_THREAD_NOT_RUNNING = -1,	//thread�� ���� ���۵��� ���� ����
		FTP_THREAD_IS_STOPPING,		//��� ������� �������� ����
		FTP_THREAD_IS_RUNNING,			//���������� �������� ����
	};

	bool	connect(CWnd* parent, CString ip, int port, CString user = _T(""), CString password = _T(""));
	void	add_job(int job_index, CString local_file, CString remote_file);
	void	thread_ftp(CSCFtpParams* params);

	//Ư�� job�� �� �����带 ����. ""�̸� ��� �� ����.
	void	stop(int job_index = -1);
	bool	is_thread_running(int job_index);
	bool	is_all_threads_stopped();

protected:
	CWnd*		m_parent = NULL;

	HINTERNET	m_internet = NULL;
	HINTERNET	m_connection = NULL;	//FTP ���� �ڵ�

	//id�� thread ���� ���� ���� ��. ������ų �� �ʿ�. ���� ��� �����Ǿ����� Ȯ�νÿ��� �ʿ�.
	//1:running, 0:stop thread, -1:stopped
	std::map<int, int> m_map;
};

#endif
