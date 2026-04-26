#pragma once

/*
- N개의 데이터를 [start, end) 인덱스 범위로 자동 분할해 여러 CSCThread 워커가 병렬 처리하도록 묶어주는 헬퍼.
- 각 워커는 CSCThread 인스턴스이므로 stop/pause/resume, 인터럽트 가능한 sleep_for, 안전한 join 모두 지원.
- ThreadManager 의 자동 분할 편의성 + CSCThread 의 안전한 라이프사이클 관리를 결합.
- 인스턴스 생성 : CSCThreadGroup m_thread;
- 시작 :
        m_thread.start(files.size(),
                [this](int worker_idx, int start, int end, CSCThread& th)
                {
                        for (int i = start; i < end; i++)
                        {
                                if (th.stop_requested())
                                        break;
                                process(i);
                        }
                },
                [this]()
                {
                        // 모든 워커 완료 시 호출됨. 마지막으로 끝난 워커 스레드에서 호출되므로
                        // UI 갱신은 invoke_ui() 또는 PostMessage 를 통해 메인 스레드로 마샬링할 것.
                        on_all_completed();
                });
- 중지 : m_thread.stop()        // request_stop + 모든 워커 join 까지 대기
- 비차단 중지 : m_thread.request_stop()
- 자동 스레드 수 결정 휴리스틱 (CThreadManager 호환):
        count <= 10  → 5 thread
        count <= 100 → 10 thread
        else         → count / 20 (max 40)
- 명시 지정 : start(count, work, on_complete, thread_count) 의 마지막 인자로 직접 지정 가능.
*/

#include "../CSCThread/SCThread.h"

#include <atomic>
#include <functional>
#include <memory>
#include <mutex>
#include <utility>
#include <vector>

class CSCThreadGroup
{
public:
        // worker_idx : 0..thread_count-1
        // [start, end) : 이 워커가 담당할 인덱스 범위
        // th : 워커의 CSCThread 인스턴스 (stop_requested(), wait_if_paused(), sleep_for() 등 호출)
        using work_function = std::function<void(int worker_idx, int start, int end, CSCThread& th)>;
        using completion_callback = std::function<void()>;

        CSCThreadGroup() = default;
        ~CSCThreadGroup() { stop(); }

        CSCThreadGroup(const CSCThreadGroup&) = delete;
        CSCThreadGroup& operator=(const CSCThreadGroup&) = delete;

        // count       : 처리할 총 항목 수
        // work        : 각 워커가 한 번 호출되는 함수 (할당된 [start, end) 범위 처리)
        // on_complete : 모든 워커 완료 후 호출 (마지막으로 끝난 워커 스레드에서 호출됨, UI 마샬링 필요)
        // thread_count: 0 = 자동 결정, 양수 = 명시 지정 (count 보다 크면 count 로 클램프)
        void start(int count,
                           work_function work,
                           completion_callback on_complete = nullptr,
                           int thread_count = 0)
        {
                stop();

                if (count <= 0)
                {
                        if (on_complete)
                                on_complete();
                        return;
                }

                int n = (thread_count > 0) ? thread_count : auto_thread_count(count);
                if (n > count)
                        n = count;
                if (n < 1)
                        n = 1;

                int each = count / n;
                int remain = count - each * n;

                std::vector<std::pair<int, int>> ranges;
                ranges.reserve(n);

                int s = 0;
                for (int i = 0; i < n; ++i)
                {
                        int len = each + (remain > 0 ? 1 : 0);
                        if (remain > 0)
                                --remain;
                        ranges.emplace_back(s, s + len);
                        s += len;
                }

                m_total = n;
                m_completed.store(0, std::memory_order_release);
                {
                        std::lock_guard<std::mutex> lk(m_cb_mtx);
                        m_on_complete = std::move(on_complete);
                }
                m_work = std::move(work);

                m_threads.clear();
                m_threads.reserve(n);
                for (int i = 0; i < n; ++i)
                        m_threads.emplace_back(std::make_unique<CSCThread>());

                for (int i = 0; i < n; ++i)
                {
                        int worker_idx = i;
                        int start_idx = ranges[i].first;
                        int end_idx = ranges[i].second;

                        m_threads[i]->start([this, worker_idx, start_idx, end_idx](CSCThread& th)
                                {
                                        if (m_work)
                                                m_work(worker_idx, start_idx, end_idx, th);

                                        int done = m_completed.fetch_add(1, std::memory_order_acq_rel) + 1;
                                        if (done == m_total)
                                        {
                                                completion_callback cb;
                                                {
                                                        std::lock_guard<std::mutex> lk(m_cb_mtx);
                                                        cb = std::move(m_on_complete);
                                                        m_on_complete = nullptr;
                                                }
                                                if (cb)
                                                        cb();
                                        }
                                });
                }
        }

        // 모든 워커에 stop 신호 보내고 join 까지 대기
        void stop()
        {
                request_stop();
                join();
        }

        // 모든 워커에 stop 신호만 (비차단)
        void request_stop()
        {
                for (auto& th : m_threads)
                        if (th)
                                th->request_stop();
        }

        // 모든 워커 join (이미 끝난 워커는 즉시 반환)
        void join()
        {
                for (auto& th : m_threads)
                        if (th)
                                th->join();
        }

        // 모든 워커 일시정지
        void pause()
        {
                for (auto& th : m_threads)
                        if (th)
                                th->pause();
        }

        // 모든 워커 재개
        void resume()
        {
                for (auto& th : m_threads)
                        if (th)
                                th->resume();
        }

        bool is_running() const
        {
                for (const auto& th : m_threads)
                        if (th && th->is_running())
                                return true;
                return false;
        }

        bool is_completed() const
        {
                return m_total > 0 && m_completed.load(std::memory_order_acquire) == m_total;
        }

        int worker_count() const { return m_total; }
        int completed_count() const { return m_completed.load(std::memory_order_acquire); }

private:
        // CThreadManager 호환 휴리스틱
        static int auto_thread_count(int count)
        {
                constexpr int MAX_THREAD = 40;
                int n;
                if (count <= 10)
                        n = 5;
                else if (count <= 100)
                        n = 10;
                else
                        n = count / 20;

                if (n > MAX_THREAD)
                        n = MAX_THREAD;
                if (n < 1)
                        n = 1;
                return n;
        }

        std::vector<std::unique_ptr<CSCThread>> m_threads;
        int m_total = 0;
        std::atomic<int> m_completed{ 0 };

        work_function m_work;

        std::mutex m_cb_mtx;
        completion_callback m_on_complete;
};
