#include "ThreadPool.h"
#if 0
void ThreadPool::Start()
{
    const uint32_t num_threads = std::thread::hardware_concurrency(); // Max # of threads the system supports
    for (uint32_t ii = 0; ii < num_threads; ++ii) {
        threads.emplace_back(std::thread(&ThreadPool::ThreadLoop, this));
    }
}

void ThreadPool::ThreadLoop() {
    while (true) {
        std::function<void()> job;
        {
            std::unique_lock<std::mutex> lock(queue_mutex);
            mutex_condition.wait(lock, [this] {
                return !jobs.empty() || should_terminate;
                });
            if (should_terminate) {
                return;
            }
            job = jobs.front();
            jobs.pop();
        }
        job();
    }
}

void ThreadPool::QueueJob(const std::function<void()>& job) {
    {
        std::unique_lock<std::mutex> lock(queue_mutex);
        jobs.push(job);
    }
    mutex_condition.notify_one();
}

bool ThreadPool::busy() {
    bool poolbusy;
    {
        std::unique_lock<std::mutex> lock(queue_mutex);
        poolbusy = !jobs.empty();
    }
    return poolbusy;
}

void ThreadPool::Stop() {
    {
        std::unique_lock<std::mutex> lock(queue_mutex);
        should_terminate = true;
    }
    mutex_condition.notify_all();
    for (std::thread& active_thread : threads) {
        active_thread.join();
    }
    threads.clear();
}
#endif

#if 0
// the constructor just launches some amount of workers
inline ThreadPool::ThreadPool()
    : stop(false)
{
    for (size_t i = 0; i < 5; ++i)
        workers.emplace_back(
            [this]
            {
                for (;;)
                {
                    std::function<void()> task;

                    {
                        std::unique_lock<std::mutex> lock(this->queue_mutex);
                        this->condition.wait(lock,
                            [this] { return this->stop || !this->tasks.empty(); });
                        if (this->stop && this->tasks.empty())
                            return;
                        task = std::move(this->tasks.front());
                        this->tasks.pop();
                    }

                    task();
                }
            }
            );
}

// add new work item to the pool
template<class F, class... Args>
auto ThreadPool::Enqueue(F&& f, Args&&... args)
-> std::future<typename std::result_of<F(Args...)>::type>
{
    using return_type = typename std::result_of<F(Args...)>::type;

    auto task = std::make_shared< std::packaged_task<return_type()> >(
        std::bind(std::forward<F>(f), std::forward<Args>(args)...)
        );

    std::future<return_type> res = task->get_future();
    {
        std::unique_lock<std::mutex> lock(queue_mutex);

        // don't allow enqueueing after stopping the pool
        if (stop)
            throw std::runtime_error("enqueue on stopped ThreadPool");

        tasks.emplace([task]() { (*task)(); });
    }
    condition.notify_one();
    return res;
}

void ThreadPool::EnqueueJob(std::function<void()> job)
{
    if (stop) {
        throw std::runtime_error("ThreadPool 사용 중지됨");
    }
    {
        std::lock_guard<std::mutex> lock(queue_mutex);
        tasks.push(std::move(job));
    }
    condition.notify_one();
}

// the destructor joins all threads
inline ThreadPool::~ThreadPool()
{
    {
        std::unique_lock<std::mutex> lock(queue_mutex);
        stop = true;
    }
    condition.notify_all();
    for (std::thread& worker : workers)
        worker.join();
}
#endif

#if 0
ThreadPool::ThreadPool()
    :
    num_threads(5),
    stop_all(false)
{
    worker_threads.reserve(num_threads);
    for (size_t i = 0; i < num_threads; ++i)
    {
        // 워커 쓰레드 추가
        worker_threads.emplace_back([this]() { this->WorkerThread(); });
    }
}

ThreadPool::~ThreadPool()
{
    stop_all = true;
    // 자고 있는 모든 쓰레드를 깨운다.
    cv.notify_all();

    // 합류 불가능하게 만든다.
    for (auto& t : worker_threads)
    {
        t.join();
    }
}

void ThreadPool::EnqueueJob(std::function<void()> job)
{
    if (stop_all)
    {
        throw std::runtime_error("ThreadPool 사용 중지 됌");
    }
    {
        std::lock_guard<std::mutex> lock(mtx);

        // 작업 큐 보호하고 작업을 추가한다.
        jobQue.push(std::move(job));
    }
    cv.notify_one();
}

void ThreadPool::WorkerThread()
{
    while (true)
    {
        std::unique_lock<std::mutex> lock(mtx);

        // 비어있지 않거나 stop_all일 경우 다음 실행을 진행한다.
        cv.wait(lock, [this]() { return !this->jobQue.empty() || stop_all; });

        // 쓰레드 종료와 모든 작업이 끝낫을 경우 return
        if (stop_all && this->jobQue.empty())
        {
            return;
        }

        // 맨 앞의 작업을 뺀다.
        auto nextJob = std::move(jobQue.front());
        jobQue.pop();
        lock.unlock();

        // 해당 job을 수행한다.
        nextJob();
    }
}

// job을 추가하고 결과를 받을 수 있는 미래 객체를 반환한다.
template<class Func, class... Args>
std::future<typename std::result_of<Func(Args...)>::type>
EmplaceJobAndGetFuture(Func&& f, Args&&... args)
{
    // C++ 14
    using return_type = typename std::result_of<Func(Args...)>::type;
    // 완벽 전달로 함수와 인수를 전달한다.
    auto callable = std::bind(std::forward<Func>(f), std::forward<Args>(args)...);

    using job_type = std::packaged_task<return_type()>;
    auto job = std::make_shared<job_type>(std::move(callable));
    // 작업과 연관된 미래 객체를 얻어온다.
    auto result_future = job->get_future();
    {
        std::lock_guard<std::mutex> lock(mtx);

        jobQue.push([job]()
            {
                (*job)();
            });
    }
    cv.notify_one();

    return result_future;
}
#endif

#if 1
ThreadPool::ThreadPool(size_t num_threads)
    : num_threads_(num_threads), stop_all(false)
{
    thread_list.reserve(num_threads_);

    //여기서 람다 함수를 사용해서 넣는  이유는 클래스의 멤버 함수로 스레드를 생성하지 못하기 때문?
    for (size_t i = 0; i < num_threads_; ++i)
    {
        thread_list.emplace_back([this]() { this->WorkerThread(); });
    }
}

ThreadPool::~ThreadPool()
{
    stop_all = true;
    cond.notify_all();

    for (auto& t : thread_list)
    {
        t.join();
    }
}

void ThreadPool::resize(size_t num_threads)
{
    stop_all = true;
    cond.notify_all();

    for (auto& t : thread_list)
    {
        t.join();
    }


    stop_all = false;
    num_threads_ = num_threads;
    thread_list.reserve(num_threads_);

    //여기서 람다 함수를 사용해서 넣는  이유는 클래스의 멤버 함수로 스레드를 생성하지 못하기 때문?
    for (size_t i = 0; i < num_threads_; ++i)
    {
        thread_list.emplace_back([this]() { this->WorkerThread(); });
    }
}

void ThreadPool::WorkerThread()
{
    while (true)
    {
        std::unique_lock<std::mutex> lock(m_job_q_);
        cond.wait(lock, [this]()
            {
                return !this->job_list.empty() || stop_all;
            }
        );

        if (stop_all && this->job_list.empty())
        {
            return;
        }

        //맨 앞의 job 을 뺀다.
        //함수 포인터이므로 굳이 std::move를 쓸 필요는 없음.
        //std::function<void()> job = std::move(jobs_.front());
        std::function<void()> job = job_list.front();
        job_list.pop();
        lock.unlock();

        //해당 job 을 수행한다
        job();
    }
}

#endif
