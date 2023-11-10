#pragma once

/*
* ���������� �����ϴ� �۾�����
* n���� thread�� ������ ��������� ��
* ���� thread�� ���� ���۵Ǵ� �Լ��� main�� �����Ѵ�.
* (���� ������ ���� �ʴ� n���� �ݺ��۾� ��)
* 
* ���� :
* 
* [in .h]
* #include "../thread/thread_manager.h"
* ....
* //�ν��Ͻ� ����
* CThreadManager m_thread; 

	//�� �Լ��� �̸��� job�� �°� ����
	static void		thread_job_function(int idx, int start, int end);	//multi thread�� ���۽�ų �ݹ� �Լ�
	static void		thread_job_completed();								//��� thread�� ������ ȣ��� �ݹ� �Լ�
	void			on_thread_job_completed();							//thread_job_completed() �Լ����� ȣ���ϴ� ���� Ŭ������ ��� �Լ�
	bool			m_thread_job_completed;								//��� multi thread�� �������� �����ϴ� ����

  [in .cpp]
	//���� �۾��� ���۽�Ű�� �Լ����� �Ʒ��� ���� ȣ���ϸ�
	//n���� thread�� thread_job_function�� ȣ���ϸ鼭
	//start ~ end �ε����� �����͸� ó���ϱ� �����Ѵ�.
	m_job_completed = false;
	m_thread.job(files.size(), thread_job_function, job_completed_callback);

	void CMyClass::thread_job_function(int idx, int start, int end)
	{
		for (int i = start; i < end; i++)
		{
			...
		}

		//�� �����尡 �Ϸ�Ǹ�
		mtx.lock();
		pWnd->m_thread.thread_ended.push_back(idx);
		mtx.unlock();
	}

	void CMyClass::thread_jobs_completed()
	{
		mainDlg->on_threads_completed();
	}

	void CMyClass::on_threads_completed()
	{
		//��� ������ �۾��� �Ϸ�� ��...
	}
*/

#include <afxwin.h>
#include <thread>
#include <vector>

typedef void (*ThreadJobFunc)(int index, int start, int end);
typedef void (*ThreadEndFunc)();

class CThreadManager
{
public:
	CThreadManager() {};

	//ó���� �� Ƚ��, thread�� ó���� �Լ�, thread�� ������ ȣ��Ǵ� �Լ�
	void job(int count, ThreadJobFunc thread_func, ThreadEndFunc end_func);
	std::vector<int> thread_ended;
};