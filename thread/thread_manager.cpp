#include "thread_manager.h"

#define MAX_THREAD 40

//ó���� �� Ƚ��, thread�� ó���� �Լ�, thread�� ������ ȣ��Ǵ� �Լ�
void CThreadManager::job(int count, ThreadJobFunc thread_func, ThreadEndFunc end_func, bool detach)
{
	m_workers.clear();
	m_thread_completed.clear();

	int i;
	int thread_num;
	int start[MAX_THREAD];
	int end[MAX_THREAD];


	//count�� ���� thread_num�� �����ش�.
	if (count <= 10)
		thread_num = 5;
	else if (count <= 100)
		thread_num = 10;
	else
		thread_num = count / 20;

	if (thread_num > MAX_THREAD)
		thread_num = MAX_THREAD;

	//thread_num = 1;

	int each_num = count / thread_num;


	//�������� ������ thread_num �̸��̸� �� �����忡�� �ϳ��� �����͸� ó���ϰ�
	if (each_num == 0)
	{
		thread_num = count;

		for (i = 0; i < thread_num; i++)
		{
			start[i] = i;
			end[i] = i + 1;
		}
	}
	//thread_num���� ������ data_num / thread_num�ϰ� ���� ������ �� �����尡 �������� �� ó���Ѵ�.
	else
	{
		int remain = count - each_num * thread_num;

		for (i = 0; i < thread_num; i++)
		{
			if (i == 0)
				start[i] = 0;
			else
				start[i] = end[i - 1];

			if (remain > 0)
			{
				end[i] = start[i] + each_num + 1;
				remain--;
			}
			else
			{
				end[i] = start[i] + each_num;
			}
		}
	}

	//TRACE(_T("%d threads start...\n"), thread_num);

	m_thread_completed.resize(thread_num);

	for (i = 0; i < thread_num; i++)
	{
		m_thread_completed[i] = false;
		m_workers.push_back(std::thread(thread_func, i, start[i], end[i]));
	}

	for (i = 0; i < thread_num; i++)
	{
		//join()�� ȣ���ϸ� ��� ���������� hold���°� �ǰ�
		//detach()�� ȣ���ϸ� �ϴ� �����带 ���۽�Ű�� �Ѿ��.
		//thread_func()���� ���� ������ ���� ���� detach()�� ȣ���� ���
		//data racing�� �߻��ϰų� crash�� �߻��� �� �����Ƿ� �����ؾ� �Ѵ�.

		if (detach)
			m_workers[i].detach();
		else
			m_workers[i].join();
	}

	m_end_function = end_func;
	std::thread th(&CThreadManager::check_all_thread_completed, this);
	th.detach();
}

void CThreadManager::check_all_thread_completed()
{
	while (!is_all_thread_completed())
		std::this_thread::sleep_for(std::chrono::milliseconds(10));

	m_end_function();
}

bool CThreadManager::is_all_thread_completed()
{
	return (get_thread_completed_count() == m_workers.size());
}

int CThreadManager::get_thread_completed_count()
{
	int count = 0;

	for (auto th : m_thread_completed)
	{
		if (th)
			count++;
	}

	return count;
}

void CThreadManager::set_thread_completed(int index)
{
	if (index >= m_workers.size())
	{
		TRACE(_T("index >= m_workers.size()\n"));
		return;
	}

	if (index >= m_thread_completed.size())
	{
		TRACE(_T("index >= m_thread_completed.size()\n"));
		return;
	}

	m_thread_completed[index] = true;
}
