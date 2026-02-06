#pragma once

/*
* 참조 : https://modoocode.com/285
* 실제 thread로 처리하는 함수는 main에 있고
* ThreadPool에 넣어주면 알아서 해당 함수를 multi-thread로 동작시킨다.
* main에 있는 함수가 thread로 동작하므로 UI 갱신, 메시지 전달등이 수월해진다.
*/

#include <afxwin.h>
#include <functional>
#include <future>
#include <thread>
#include <queue>
#include <vector>
#include <condition_variable>
#include <mutex>

static const UINT Message_ThreadPool = ::RegisterWindowMessage(_T("MessageString_ThreadPool"));

/*
class ThreadPool {
public:
	void Start();
	void QueueJob(const std::function<void()>& job);
	void Stop();
	bool busy();

private:
	void ThreadLoop();

	bool should_terminate = false;           // Tells threads to stop looking for jobs
	std::mutex queue_mutex;                  // Prevents data races to the job queue
	std::condition_variable mutex_condition; // Allows threads to wait on new jobs or termination
	std::vector<std::thread> threads;
	std::queue<std::function<void()>> jobs;
};
*/

#if 0
class ThreadPool {
public:
	ThreadPool();
	template<class F, class... Args>
	//auto Enqueue(F&& f, Args&&... args) -> std::future<typename std::result_of<F(Args...)>::type>;
	void EnqueueJob(std::function<void()> job);
	~ThreadPool();
private:
	// need to keep track of threads so we can join them
	std::vector< std::thread > workers;
	// the task queue
	std::queue< std::function<void()> > tasks;

	// synchronization
	std::mutex queue_mutex;
	std::condition_variable condition;
	bool stop;
};
#endif

#if 0
class ThreadPool
{
public:
	ThreadPool();
	~ThreadPool();

	// job 추가
	void EnqueueJob(std::function<void()> job);

	// C ++14
	template<class Func, class... Args>
	std::future<typename std::result_of<Func(Args...)>::type>
		EmplaceJobAndGetFuture(Func&& f, Args&&... args);


private:
	// 총 Worker 쓰레드의 개수
	size_t num_threads;
	// Worker 쓰레드를 보관하는 벡터
	std::vector<std::thread> worker_threads;
	// 할 일들을 보관하는 job 큐
	std::queue<std::function<void()>> jobQue;
	// 위의 job 큐들을 위한 조건 변수와 뮤텍스
	std::condition_variable cv;
	std::mutex mtx;

	// 모든 쓰레드 종료
	bool stop_all;

	// 작업 큐에 있는 작업 하나를 실행한다.
	void WorkerThread();
};
#endif


#if 1
class ThreadPool
{
public:
	ThreadPool(size_t max_num_threads = 10);
	~ThreadPool();

	void resize(size_t max_num_threads);

	//thread로 동작시킬 함수(job)을 추가한다.
	template <class F, class... Args>
	std::future<typename std::result_of<F(Args...)>::type> EnqueueJob(
		F&& f, Args&&... args);

	size_t get_max_pool_size() { return m_max_num_threads; }
	size_t get_job_count() { return job_list.size(); }
	void stop_all_threads() { stop_all = true; }

private:
	//Worker 쓰레드의 최대 개수 (1515를 넘지못함).
	size_t m_max_num_threads;
	// Worker 쓰레드를 보관하는 벡터.
	std::vector<std::thread> thread_list;
	// 할일들을 보관하는 job 큐.
	std::queue<std::function<void()>> job_list;
	// 위의 job 큐를 위한 cv 와 m.
	std::condition_variable cond;
	std::mutex m_job_q_;

	// 모든 쓰레드 종료
	bool stop_all = false;

	// Worker 쓰레드
	void WorkerThread();
};

template <class F, class... Args>
std::future<typename std::result_of<F(Args...)>::type> ThreadPool::EnqueueJob(
	F&& f, Args&&... args)
{
	if (stop_all)
	{
		throw std::runtime_error("ThreadPool 사용 중지됨");
	}

	using return_type = typename std::result_of<F(Args...)>::type;
	auto job = std::make_shared<std::packaged_task<return_type()>>(
		std::bind(std::forward<F>(f), std::forward<Args>(args)...));
	std::future<return_type> job_result_future = job->get_future();

	//std::lock_guard<std::mutex> lock(m_job_q_);
	m_job_q_.lock();
	job_list.push([job]() { (*job)(); });
	m_job_q_.unlock();

	cond.notify_one();

	//Sleep()을 걸어주지 않을 경우 EnqueueJob()이 짧은 시간 간격으로 호출될 경우 에러 발생.
	//Sleep(1);

	return job_result_future;
}
#endif
