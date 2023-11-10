#include "thread_manager.h"

#define MAX_THREAD 40

//처리할 총 횟수, thread로 처리할 함수, thread가 끝나면 호출되는 함수
void CThreadManager::job(int count, ThreadJobFunc thread_func, ThreadEndFunc end_func)
{
	int i;
	int thread_num;
	int start[MAX_THREAD];
	int end[MAX_THREAD];

	thread_ended.clear();

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

	TRACE(_T("%d threads start...\n"), thread_num);

	std::vector<std::thread> workers;

	for (i = 0; i < thread_num; i++)
	{
		workers.push_back(std::thread(thread_func, i, start[i], end[i]));
	}

	for (i = 0; i < thread_num; i++)
	{
		//workers[i].detach();
		workers[i].join();
	}

	end_func();
}
