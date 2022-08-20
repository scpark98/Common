#pragma once

#include <Afxwin.h>
#include <Afxdisp.h>
#include <afxmt.h>
#include <deque>

/*
���簡 �������̸� ť�� �ϴ� �װ�
���� ���簡 �Ϸ�Ǹ� ť���� ���� ���� �۾��� �����Ѵ�.
���� ����� ����ó���� �ǹ̰� �����Ƿ�
���������� ���縦 �����Ѵ�.

- ���縦 ���� �����Ͽ� pause, resume �߰�

*/

class CCopyInfo
{
public:
	CCopyInfo(CString src, CString dst, bool is_move);

//protected:
	bool	m_is_copy_mode;		//move? or copy?
	CString m_src;
	CString m_dst;
	uint64_t m_filesize;
};

class CCopyThread
{
public:
	CCopyThread();
	~CCopyThread();

	int do_copy(CString src, CString dst, bool is_copy_mode);
	void check_queue_thread();
	void copy_thread_function(CCopyInfo info);
	void CopyProgressRoutine(CCopyInfo info, uint64_t TotalFileSize, uint64_t TotalBytesTransferred, DWORD status);
	void SetCallbackFunction(void (*p_func)(CCopyInfo info, uint64_t, uint64_t, DWORD status)) { m_pCallback_func = p_func; }
	
	int get_copy_queue_size();
	uint64_t get_total_file_size(bool recalc);
	uint64_t get_total_transferred();
	void cancel();
	bool is_cancel() { return m_cancel; }
	bool is_copying() { return m_is_copying; }

protected:
	bool m_check_queue_thread;
	bool m_cancel;
	std::deque<CCopyInfo> m_copy_queue;
	void (*m_pCallback_func)(CCopyInfo, uint64_t total, uint64_t transferred, DWORD status);
	CMutex	m_mutex;
	bool m_is_copying;
	uint64_t m_total_filesize;
	uint64_t m_total_transferred;
};
