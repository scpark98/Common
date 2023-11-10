#pragma once

/*
* 병렬적으로 수행하는 작업들을
* n개의 thread로 나눠서 실행시켜줄 뿐
* 실제 thread에 의해 동작되는 함수는 main에 존재한다.
* (서로 영향을 주지 않는 n번의 반복작업 등)
* 
* 사용법 :
* 
* [in .h]
* #include "../thread/thread_manager.h"
* ....
* //인스턴스 선언
* CThreadManager m_thread; 

	//각 함수의 이름은 job에 맞게 변경
	static void		thread_job_function(int idx, int start, int end);	//multi thread로 동작시킬 콜백 함수
	static void		thread_job_completed();								//모든 thread가 끝나면 호출될 콜백 함수
	void			on_thread_job_completed();							//thread_job_completed() 함수에서 호출하는 메인 클래스의 멤버 함수
	bool			m_thread_job_completed;								//모든 multi thread가 끝났음을 저장하는 변수

  [in .cpp]
	//실제 작업을 시작시키는 함수에서 아래와 같이 호출하면
	//n개의 thread가 thread_job_function을 호출하면서
	//start ~ end 인덱스의 데이터를 처리하기 시작한다.
	m_job_completed = false;
	m_thread.job(files.size(), thread_job_function, job_completed_callback);

	void CMyClass::thread_job_function(int idx, int start, int end)
	{
		for (int i = start; i < end; i++)
		{
			...
		}

		//한 쓰레드가 완료되면
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
		//모든 쓰레드 작업이 완료된 후...
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

	//처리할 총 횟수, thread로 처리할 함수, thread가 끝나면 호출되는 함수
	void job(int count, ThreadJobFunc thread_func, ThreadEndFunc end_func);
	std::vector<int> thread_ended;
};