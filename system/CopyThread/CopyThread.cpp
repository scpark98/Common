//#include "stdafx.h"
#include "CopyThread.h"
#include <thread>
#include <chrono>

#include "../../Functions.h"

CCopyThread* g_thread;

CCopyInfo::CCopyInfo(CString src, CString dst, bool is_copy_mode)
{
	m_src = src;
	m_dst = dst;
	m_is_copy_mode = is_copy_mode;

	CFileStatus		status;

	if (CFile::GetStatus(m_src, status))
		m_filesize = status.m_size;
	else
		m_filesize = 0;
}

CCopyThread::CCopyThread()
{
	g_thread = this;
	m_pCallback_func = NULL;
	m_is_copying = false;
	m_cancel = false;
	m_total_filesize = 0;
	m_total_transferred = 0;
	m_check_queue_thread = true;
	std::thread th = std::thread(&CCopyThread::check_queue_thread, this);
	th.detach();
}

CCopyThread::~CCopyThread()
{
	cancel();
	m_check_queue_thread = false;
	std::this_thread::sleep_for(std::chrono::milliseconds(500));
}

void CCopyThread::check_queue_thread()
{
	while (m_check_queue_thread)
	{
		if (!m_is_copying && m_copy_queue.size() > 0)
		{
			m_mutex.Lock();
			CCopyInfo info = m_copy_queue.front();
			m_copy_queue.pop_front();
			m_mutex.Unlock();

			m_is_copying = true;
			std::thread th = std::thread(&CCopyThread::copy_thread_function, this, info);
			//std::this_thread::sleep_for(std::chrono::milliseconds(100));

			th.detach();
		}
		else
		{
			m_mutex.Lock();
			
			if (m_is_copying)
			{
				//TRACE(_T("another copy job is processing. waiting...\n"));
			}
			else if (m_copy_queue.size() == 0)
			{
				m_is_copying = false;
				m_cancel = false;
				m_total_filesize = 0;
				m_total_transferred = 0;

				//TRACE(_T("copy queue is empty...\n"));
			}

			m_mutex.Unlock();

			std::this_thread::sleep_for(std::chrono::seconds(1));
		}
	}
}

int CCopyThread::do_copy(CString src, CString dst, bool is_copy_mode)
{
	//���� ������ ���� ���
	if (PathFileExists(src) == false)
		return ERROR_FILE_NOT_FOUND;

	//���� ������ �̵��̰� ���� �����̶�� �׳� api�� ȣ���ϸ� �ȴ�.
	//dst ������ ���� ��� PathIsDirectory(dst)�� false�� �����ϹǷ� �Ʒ� ���ǽĿ����� ���ܽ��Ѿ� �Ѵ�.
	//�׳� src�� ���丮�̸� ���� �����̴�.
	if (!is_copy_mode && PathIsDirectory(src) && (src.Left(1).MakeLower() == dst.Left(1).MakeLower()))
	{
		//dst������ �̹� �����ϸ� ������ �߻��Ѵ�. 
		//MOVEFILE_REPLACE_EXISTING �ɼ��� �൵ ����������.
		bool res = MoveFileEx(src, dst, MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH);
		return (res ? ERROR_SUCCESS : GetLastError());
	}
	//�ٸ� ���������� ���� �̵��̶�� ���ϵ��� ������ ���� �� �����ؾ� �Ѵ�.
	else if (PathIsDirectory(src))
	{
		std::deque<CString> dqFiles;
		FindAllFiles(src, &dqFiles, _T("*"), _T("*"), true);

		//������ ���ٸ� ���� ���� ����, ��� ���� ������ �ص� �ȴ�.
		if (dqFiles.size() == 0)
		{
			delete_file(src, true);
			dst = dst + _T("\\") + GetFileNameFromFullPath(src);
			make_full_directory(dst);
			return ERROR_SUCCESS;
		}
		else
		{
			//���ϵ��� �̵��� ��� ��� ������ ���� ���� ���� ��������� �Ѵ�.
			if (PathFileExists(dst) == false)
				make_full_directory(dst);

			m_is_copying = true;
			m_mutex.Lock();
			for (int i = 0; i < dqFiles.size(); i++)
			{
				CCopyInfo info(dqFiles[i], dst + _T("\\") + GetFileNameFromFullPath(dqFiles[i]), is_copy_mode);
				m_copy_queue.push_back(info);
				m_total_filesize += info.m_filesize;
			}
			m_mutex.Unlock();
			m_is_copying = false;

			return -1;
		}
	}

	//���ϵ��� �̵��� ��� ��� ������ ���� ���� ���� ��������� �Ѵ�.
	CString dstFolder = GetFolderNameFromFullPath(dst);
	if (PathFileExists(dstFolder) == false)
		make_full_directory(dstFolder);

	m_mutex.Lock();
	CCopyInfo info(src, dst, is_copy_mode);
	m_copy_queue.push_back(info);
	m_total_filesize += info.m_filesize;
	m_mutex.Unlock();

	return ERROR_SUCCESS;
}

uint64_t CCopyThread::get_total_file_size(bool recalc)
{
	m_mutex.Lock();
	if (recalc)
	{
		m_total_filesize = 0;
		for (int i = 0; i < m_copy_queue.size(); i++)
			m_total_filesize += m_copy_queue[i].m_filesize;
	}
	m_mutex.Unlock();

	return m_total_filesize;
}

void CCopyThread::CopyProgressRoutine(CCopyInfo info, uint64_t TotalFileSize, uint64_t TotalBytesTransferred, DWORD status)
{
	if (m_pCallback_func)
		m_pCallback_func(info, TotalFileSize, TotalBytesTransferred, status);
}

DWORD CALLBACK CopyProgressRoutine(
	LARGE_INTEGER TotalFileSize, LARGE_INTEGER TotalBytesTransferred,
	LARGE_INTEGER StreamSize, LARGE_INTEGER StreamBytesTransferred, DWORD dwStreamNumber,
	DWORD dwCallbackReason, HANDLE hSourceFile, HANDLE hDestinationFile, LPVOID lpData)
{
	if (g_thread->is_cancel())
	{
		return PROGRESS_CANCEL;
	}

	CCopyInfo* pInfo = (CCopyInfo*)lpData;
	g_thread->CopyProgressRoutine(*pInfo, TotalFileSize.QuadPart, TotalBytesTransferred.QuadPart, ERROR_SUCCESS);


	return PROGRESS_CONTINUE;
}

void CCopyThread::copy_thread_function(CCopyInfo info)
{
	TRACE(_T("%s start..,\n"), info.m_src);
	m_is_copying = true;
	bool success = false;

	//m_is_copy_mode�� false�̰� ���� ����̺� �� ��쿡�� MoveFileEx�� �����Ѵ�.
	//m_is_copy_mode�� false�̾ �ٸ� ����̺��� ������ CopyFileEx�� ������ ��
	//���簡 ������ ������ ����� ������� �����Ѵ�.
	if (!info.m_is_copy_mode && (info.m_src.Left(1).MakeLower() == info.m_dst.Left(1).MakeLower()))
	{
		success = MoveFileEx(info.m_src, info.m_dst, MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH);
		//�̵��� �ٷ� �����Ƿ� ���� �Ǵ� ���� ����� �����ϰ�
		if (m_pCallback_func)
		{
			if (success)
				m_pCallback_func(info, info.m_filesize, info.m_filesize, ERROR_SUCCESS);
			else
				m_pCallback_func(info, 0, 0, GetLastError());
		}
	}
	else
	{
		/*
		success = CopyFileEx(info.m_src, info.m_dst, ::CopyProgressRoutine, (LPVOID*)&info, NULL, 0);
		//������ ��쿡�� ó�� �Լ� ȣ�⿡ ������ ��츸 �޽����� �����Ѵ�.
		if (!success && m_pCallback_func)
		{
			m_pCallback_func(info, 0, 0, GetLastError());
		}
		*/
		/* 1,439,969,357 bytes copy
		* 8192 * 512  : 9s
		* 8192 * 1024 : 6s
		* 8192 * 2048 : 6s
		* 8192 * 4096 : 20s
		* 
		* 3.2G copy
		* 8192 * 512  : 62s
		* 8192 * 1024 : 47s
		* 8192 * 2048 : 43s
		* 8192 * 4096 : 41s
		* 8192 * 8192 : 39s
		* 10240 * 10240 : 37s
		* 20480 * 20480 : 37s
		*/
		size_t buffer_size = 8192 * 2048;
		char* buf = new char[buffer_size];
		char *src_file = CString2char(info.m_src);
		char *dst_file = CString2char(info.m_dst);
		size_t size = 0;
		uint64_t transferred = 0;
		//sprintf(src_file, "%S", info.m_src);
		//sprintf(dst_file, "%S", info.m_dst);
		FILE* src = fopen(src_file, "rb");
		FILE* dst = fopen(dst_file, "wb");

		//���� fopen�� ������ ���� �����غ��� �Ѵ�.
		if (src == NULL)
		{
			int res = AfxMessageBox(info.m_src + __T("�� ������ �� �� �����ϴ�.\n���� �۾��� �̾ �����մϴ�."), MB_YESNO);
			delete[] buf;

			if (res == IDNO)
				cancel();

			if (m_pCallback_func)
				m_pCallback_func(info, -1, -1, GetLastError());
			m_is_copying = false;
			return;
		}
		if (dst == NULL)
		{
			int res = AfxMessageBox(info.m_src + __T("�� ������ ������ �� �����ϴ�.\n���� �۾��� �̾ �����մϴ�."), MB_YESNO);
			delete[] buf;
			delete[] src_file;
			fclose(src);

			if (res == IDNO)
				cancel();

			if (m_pCallback_func)
				m_pCallback_func(info, -1, -1, GetLastError());
			m_is_copying = false;
			return;
		}

		while (!m_cancel && (size = fread(buf, 1, buffer_size, src)))
		{
			transferred += fwrite(buf, 1, size, dst);
			if (m_pCallback_func)
				m_pCallback_func(info, info.m_filesize, transferred, ERROR_SUCCESS);
		}

		delete[] src_file;
		delete[] dst_file;
		delete[] buf;

		fclose(src);
		fclose(dst);
	}

	TRACE(_T("%s end.\n\n"), info.m_dst);

	m_total_transferred += info.m_filesize;
	get_total_file_size(false);

	//���������� ���� ���簡 �Ϸ���� �˸�.
	if (m_pCallback_func)
		m_pCallback_func(info, 0, 0, ERROR_SUCCESS);

	m_is_copying = false;
}

int CCopyThread::get_copy_queue_size()
{
	m_mutex.Lock();
	int count = m_copy_queue.size();
	m_mutex.Unlock();

	return count;
}

uint64_t CCopyThread::get_total_transferred()
{
	return m_total_transferred;
}

void CCopyThread::cancel()
{
	m_mutex.Lock();
	m_copy_queue.clear();
	m_cancel = true;
	m_mutex.Unlock();
}
