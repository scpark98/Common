#include "SCFtp.h"

#include <thread>
#include "../../../Functions.h"

#pragma warning(disable:4996)

CSCFtp::CSCFtp()
{
}

CSCFtp::~CSCFtp()
{
	stop();

	if (m_internet)
		::InternetCloseHandle(m_internet);

	if (m_connection)
		::InternetCloseHandle(m_connection);
}

bool CSCFtp::connect(CWnd* parent, CString ip, int port, CString user, CString password)
{
	m_parent = parent;

	m_internet = ::InternetOpen(_T("CSCFtp Internet Open"), INTERNET_OPEN_TYPE_DIRECT, NULL, 0, 0);

	if (!m_internet)
	{
		TRACE(_T("InternetOpen failed: %d\n"), ::GetLastError());
		return false;
	}

	//m_connection = ::InternetConnect(m_internet, ip, port, user, password, INTERNET_SERVICE_FTP, INTERNET_FLAG_PASSIVE, 0);
	m_connection = ::InternetConnect(m_internet, ip, port, user, password, INTERNET_SERVICE_FTP, 0, 0);

	if (!m_connection)
	{
		TRACE(_T("InternetConnect failed: %d\n"), ::GetLastError());
		::InternetCloseHandle(m_internet);
		m_internet = NULL;
		return false;
	}

	return true;
}

void CSCFtp::add_job(int job_index, CString local_file, CString remote_file)
{
	CSCFtpParams *params = new CSCFtpParams(job_index, local_file, remote_file);

	std::map<int, int>::iterator it = m_map.find(job_index);

	//이미 등록된 장비라면
	if (it != m_map.end())
		m_map[params->job_index] = FTP_THREAD_IS_RUNNING;
	else
		m_map.insert(std::pair<int, int>(params->job_index, FTP_THREAD_IS_RUNNING));

	std::thread t(&CSCFtp::thread_ftp, this, params);
	t.detach();
}

//특정 job의 핑 쓰레드를 중지. ""이면 모든 핑 중지.
void CSCFtp::stop(int job_index)
{
	std::map<int, int>::iterator it;

	if (job_index >= 0)
	{
		std::map<int, int>::iterator it = m_map.find(job_index);

		if (it != m_map.end() && it->second == FTP_THREAD_IS_RUNNING)
		{
			TRACE(_T("%d job will be stop\n"), job_index);
			it->second = FTP_THREAD_IS_STOPPING;
			return;
		}
	}
	else
	{
		//모두 중지할 경우
		for (it = m_map.begin(); it != m_map.end(); it++)
		{
			if (it->second == FTP_THREAD_IS_RUNNING)
				it->second = FTP_THREAD_IS_STOPPING;
		}
	}

	//while (!is_all_threads_stopped())
	//{
	//	TRACE(_T("wait all threads stopped...\n"));
	//	//Sleep(1000);
	//}
}

bool CSCFtp::is_thread_running(int job_index)
{
	std::map<int, int>::iterator it = m_map.find(job_index);
	return (it != m_map.end() && it->second == FTP_THREAD_IS_RUNNING);
}

bool CSCFtp::is_all_threads_stopped()
{
	for (auto it = m_map.begin(); it != m_map.end(); it++)
	{
		if (it->second != FTP_THREAD_NOT_RUNNING)
			return false;
	}

	return true;
}

#define MAX_BUFFER_SIZE 65536 

void CSCFtp::thread_ftp(CSCFtpParams* params)
{
	int i;
	bool res = false;
	CString str;

	m_map[params->job_index] = FTP_THREAD_IS_RUNNING;

	//params->remote_path의 폴더가 FTP 서버 Root 경로가 아닐 경우 해당 폴더로 변경해줘야 한다.
	//물론 폴더가 존재하지 않으면 폴더를 모두 만들어줘야 한다.
	DWORD remote_cur_path_len = INTERNET_MAX_PATH_LENGTH;
	TCHAR remote_cur_path[INTERNET_MAX_PATH_LENGTH];
	res = FtpGetCurrentDirectory(m_connection, remote_cur_path, &remote_cur_path_len);
	CString parent_path = get_parent_dir(params->remote_path, '/');

	if (parent_path.CompareNoCase(remote_cur_path) != 0)
	{
		//CString fullpath;
		std::deque<CString> dq;
		get_token_string(parent_path, dq, _T("/"), false);

		res = FtpSetCurrentDirectory(m_connection, _T("/"));

		for (i = 0; i < dq.size(); i++)
		{
			if (FtpSetCurrentDirectory(m_connection, dq[i]) == false)
			{
				res = FtpCreateDirectory(m_connection, dq[i]);
				res = FtpSetCurrentDirectory(m_connection, dq[i]);
			}
		}

		params->remote_path = get_part(params->remote_path, fn_name);
	}

	ULONGLONG local_file_size = get_file_size(params->local_path);

	FILE* fp = NULL;
	_tfopen_s(&fp, params->local_path, _T("rb"));
	if (!fp)
	{
		m_parent->SendMessage(Message_CSCFtp, (WPARAM)&CSCFtpMessage(FTP_STATUS_UPLOAD_FILEOPEN_FAILED, params->job_index), 0);
		TRACE(_T("파일 열기 실패"));
		return;
	}

	HINTERNET hRemoteFile = ::FtpOpenFile(m_connection, params->remote_path, GENERIC_WRITE, FTP_TRANSFER_TYPE_BINARY | INTERNET_FLAG_DONT_CACHE, 0);
	UINT64 nStartFilePointer = 0;

	if (!hRemoteFile)
	{
		m_parent->SendMessage(Message_CSCFtp, (WPARAM)&CSCFtpMessage(FTP_STATUS_UPLOAD_SKIPPED, params->job_index), 0);
		TRACE(_T("%s 파일이 이미 존재하므로 스킵.\n"), params->remote_path);
		m_map[params->job_index] = FTP_THREAD_NOT_RUNNING;
		fclose(fp);
		return;
	}

	BYTE Buff[MAX_BUFFER_SIZE] = { 0, };
	DWORD dwWrite = 0;
	UINT64 total_sent = nStartFilePointer;

	while (!feof(fp))//로컬파일을 다 읽을때까지 반복
	{
		//_tcscpy_s(st_TransferInfo_In.szFilePath, strLocalFilePath);//전송중인 파일명(경로포함)
		size_t nReadSize = fread(Buff, 1, MAX_BUFFER_SIZE, fp);

		if (nReadSize == 0)
			break;

		UINT dwSend = 0;
		do
		{
			//FTP서버에 업로드
			if (InternetWriteFile(hRemoteFile, Buff + dwSend, nReadSize - dwSend, &dwWrite) == FALSE)
			{
				InternetCloseHandle(hRemoteFile);
				fclose(fp);
				return;
			}
			total_sent += dwWrite;//총 전송된 크기
			dwSend += dwWrite;//현재 파일의 전송된 크기
			*Buff += dwSend;//MAX_BUFFER_SIZE

			double percentage = ((double)total_sent / (double)local_file_size) * 100.0;
			::SendMessage(m_parent->GetSafeHwnd(), Message_CSCFtp, (WPARAM)&CSCFtpMessage(FTP_STATUS_UPLOAD_SENDING, params->job_index, (int)percentage), 0);
			TRACE(_T("job : %d, file = %s, total_sent : %ul, percentage = %d\n"), params->job_index, params->remote_path, total_sent, (int)percentage);
			//st_TransferInfo_In.dwCurrentTime = ::GetTickCount();
			//st_TransferInfo_In.nTransferSize = nTotSend;

			//if (hCallWnd)
			//{
			//	GetTransferInfo(st_TransferInfo_In, st_TransferInfo_Out);
			//	::SendMessage(hCallWnd, WM_SHOW_UPLOAD_STATE, 0, (LPARAM)&st_TransferInfo_Out);
			//	PUMP_MESSAGES();
			//}

			//if (WaitForSingleObject(hCloseEvent, 0) == WAIT_OBJECT_0)//예외적 종료
			//{
			//	InternetCloseHandle(hFile);
			//	fclose(fp);
			//	return FALSE;
			//}
		} while (dwSend < nReadSize);
	}

	InternetCloseHandle(hRemoteFile);
	fclose(fp);

	TRACE(_T("ftp thread #%d stopped.\n"), params->job_index);
	m_parent->SendMessage(Message_CSCFtp, (WPARAM)&CSCFtpMessage(FTP_STATUS_UPLOAD_COMPLETED, params->job_index), 100);

	delete params;
}
