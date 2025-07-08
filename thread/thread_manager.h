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

	//ó���� �� Ƚ��, thread�� ó���� �Լ�, thread�� ������ ȣ��Ǵ� �Լ� ������ �����ϸ� �˾Ƽ� multi thread�� �����Ѵ�.
	//detach : �� thread�� ���ÿ� ó���ص� data racing�� �Ͼ�� �ʴ´ٸ� true, ���������� ó���ؾ� �Ѵٸ� false�� �ش�.
	void	job(int count, ThreadJobFunc thread_func, ThreadEndFunc end_func, bool detach = true);

	//��� thread�� �Ϸ�Ǿ����� Ȯ��.
	//job() ȣ�� �� thread_func���� set_thread_completed(n)���� ��������� �Ѵ�.
	bool	is_all_thread_completed();

	//�� thread ����
	int		get_workers_size() { return m_workers.size(); }

	//�� thread �� �Ϸ�� thread count
	int		get_thread_completed_count();

	//�ش� thread�� ó���� �Ϸ�Ǿ����� ����.
	void	set_thread_completed(int index);

protected:
	ThreadEndFunc	m_end_function;

	std::vector<std::thread> m_workers;
	std::vector<int> m_thread_completed;

	void	check_all_thread_completed();
};