#include "ThreadManager.h"

#define MAX_THREAD 40

//처리할 총 횟수, thread로 처리할 함수, thread가 끝나면 호출되는 함수
void CThreadManager::job(int count, ThreadJobFunc thread_func, ThreadEndFunc end_func, bool detach)
{
	m_workers.clear();
	m_thread_completed.clear();

	int i;
	int thread_num;
	int start[MAX_THREAD];
	int end[MAX_THREAD];


	//count에 따라 thread_num을 정해준다.
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


	//데이터의 개수가 thread_num 미만이면 한 쓰레드에서 하나의 데이터를 처리하고
	if (each_num == 0)
	{
		thread_num = count;

		for (i = 0; i < thread_num; i++)
		{
			start[i] = i;
			end[i] = i + 1;
		}
	}
	//thread_num보다 많으면 data_num / thread_num하고 남은 개수를 각 쓰레드가 나눠가진 후 처리한다.
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
		//join()을 호출하면 모두 끝날때까지 hold상태가 되고
		//detach()를 호출하면 일단 쓰레드를 시작시키고 넘어간다.
		//thread_func()에서 서로 간섭이 있을 경우는 detach()를 호출할 경우
		//data racing이 발생하거나 crash가 발생할 수 있으므로 주의해야 한다.

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
	if (index >= (int)m_workers.size())
	{
		TRACE(_T("index >= m_workers.size()\n"));
		return;
	}

	if (index >= (int)m_thread_completed.size())
	{
		TRACE(_T("index >= m_thread_completed.size()\n"));
		return;
	}

	m_thread_completed[index] = true;
}
