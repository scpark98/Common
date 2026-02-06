#pragma once

//https://docs.google.com/presentation/d/1DAoQ3uADiSL5pFGJF25tN0mNlVJnYzkBemAUIWsxO0A/edit?slide=id.p#slide=id.p

#include <atomic>
#include <condition_variable>
#include <functional>
#include <mutex>
#include <thread>

class CSCThread
{
public:
	using thread_function = std::function<void(CSCThread&)>;

	enum class State
	{
		Stopped,
		Running,
		Paused,
		Stopping
	};

	CSCThread() = default;
	~CSCThread() { stop(); }

	CSCThread(const CSCThread&) = delete;
	CSCThread& operator=(const CSCThread&) = delete;

	void start(thread_function work)
	{
		//이미 실행중이라면 스킵
		if (is_running())
			return;

		//일시정지 상태라면 재개
		if (is_paused())
		{
			resume();
			return;
		}

		stop();

		m_work = std::move(work);
		m_stop_requested.store(false, std::memory_order_release);
		m_paused.store(false, std::memory_order_release);
		m_state.store(State::Running, std::memory_order_release);

		m_thread = std::thread([this]
			{
				try
				{
					if (m_work) m_work(*this);
				}
				catch (...)
				{
					// swallow exceptions to avoid std::terminate; thread will be marked stopped below
				}

				m_paused.store(false, std::memory_order_release);
				m_state.store(State::Stopped, std::memory_order_release);
			});
	}

	void pause()
	{
		m_paused.store(true, std::memory_order_release);
		if (!m_stop_requested.load(std::memory_order_acquire))
			m_state.store(State::Paused, std::memory_order_release);
	}

	void resume()
	{
		m_paused.store(false, std::memory_order_release);
		if (!m_stop_requested.load(std::memory_order_acquire))
			m_state.store(State::Running, std::memory_order_release);
		m_cv.notify_one();
	}

	void stop()
	{
		m_stop_requested.store(true, std::memory_order_release);
		m_paused.store(false, std::memory_order_release);
		m_state.store(State::Stopping, std::memory_order_release);
		m_cv.notify_one();

		if (m_thread.joinable())
			m_thread.join();

		m_state.store(State::Stopped, std::memory_order_release);
	}

	bool is_running() const
	{
		return m_state.load(std::memory_order_acquire) == State::Running;
	}

	bool is_stopped() const
	{
		return m_state.load(std::memory_order_acquire) == State::Stopped;
	}

	bool is_paused() const
	{
		return m_state.load(std::memory_order_acquire) == State::Paused;
	}

	State state() const
	{
		return m_state.load(std::memory_order_acquire);
	}

	bool stop_requested() const
	{
		return m_stop_requested.load(std::memory_order_acquire);
	}

	// 워커 루프에서 주기적으로 호출: Pause면 sleep 상태로 들어감
	void wait_if_paused()
	{
		if (!m_paused.load(std::memory_order_acquire))
			return;

		std::unique_lock<std::mutex> lock(m_mtx);
		m_cv.wait(lock, [this]
			{
				return m_stop_requested.load(std::memory_order_acquire) ||
					!m_paused.load(std::memory_order_acquire);
			});
	}

private:
	std::thread m_thread;
	thread_function m_work;

	std::atomic<bool> m_stop_requested{ false };
	std::atomic<bool> m_paused{ false };
	std::atomic<State> m_state{ State::Stopped };

	std::mutex m_mtx;
	std::condition_variable m_cv;
};
