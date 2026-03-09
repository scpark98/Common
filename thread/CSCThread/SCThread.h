#pragma once

/*
- https://docs.google.com/presentation/d/1DAoQ3uADiSL5pFGJF25tN0mNlVJnYzkBemAUIWsxO0A/edit?slide=id.p#slide=id.p
- std::thread 사용시 start, pause/resume, stop 등의 제어 및 현재 쓰레드 상태를 정확히 얻어오기 위해 개발
- Common/thread/CSCThread의 cpp, h 2개 파일을 프로젝트에 추가
- 인스턴스 선언 : CSCThread	m_thread;
- m_thread.start([this](CSCThread& th) { thread_function(th); });
- start(), pause(), resume(), stop(), is_running(), is_stopped(), is_paused() 등의 멤버함수로 제어 또는 상태 확인

- .h에 함수 선언.
	public:
		void		thread_function(int index, CSCThread& th);
		static		constexpr UINT WM_APP_UI_INVOKE = WM_APP + 2;
		afx_msg		LRESULT on_ui_invoke(WPARAM wParam, LPARAM lParam);
	private:
		void		invoke_ui(std::function<void()> func);

- thread_fnction()은 다음과 같다.
	void CTestCSCThreadDlg::thread_function(int index, CSCThread& th)
	{
		while (!th.stop_requested())
		{
			if (th.stop_requested())
				break;

			th.wait_if_paused();

			//invoke_ui()로 묶어주면 UI 관련 코드들을 안전하게 호출하여 사용할 수 있다.
			//'=' 기호를 사용하면 외부 변수 그대로 사용가능하다.(안전 - 값이 복사되어 전달됨)
			invoke_ui([=]()
				{
					m_list.set_text(index, col_progress, i2S(progress));
				});

			//이 샘플 프로젝트처럼 수백개의 thread를 생성하여 무한루프로 돌릴 경우 딜레이를 100이 아닌 10으로 줄 경우
			//message queue에 너무 많은 메시지가 쌓여서 UI가 느리게 반응하는 현상이 발생할 수 있다. (WM_APP_UI_INVOKE 메시지가 너무 많이 쌓이는 것)
			std::this_thread::sleep_for(std::chrono::milliseconds(100));
		}

		TRACE(_T("thread %d is terminated.\n"), index);
	}

	위와 같이 invoke_ui() 람다로 묶어주면 UI 관련 코드들을 메시지 방식으로 큐에 넣고 안전하게 호출하여 사용할 수 있다.


	아래 두 함수는 thread_function()과 함께 프로젝트에 추가시켜야 한다.
	void CTestCSCThreadDlg::invoke_ui(std::function<void()> func)
	{
		const HWND hWnd = GetSafeHwnd();
		if (!::IsWindow(hWnd))
			return;

		// 함수 객체를 힙에 올려서 lParam으로 전달
		auto* pfunc = new std::function<void()>(std::move(func));
		if (!::PostMessage(hWnd, WM_APP_UI_INVOKE, 0, reinterpret_cast<LPARAM>(pfunc)))
		{
			delete pfunc;
		}
	}

	LRESULT CTestCSCThreadDlg::on_ui_invoke(WPARAM wParam, LPARAM lParam)
	{
		std::unique_ptr<std::function<void()>> pfunc(
			reinterpret_cast<std::function<void()>*>(lParam));
		(*pfunc)();
		return 0;
	}

*/

#include <atomic>
#include <condition_variable>
#include <functional>
#include <mutex>
#include <thread>

class CSCThread
{
public:
	using thread_function = std::function<void(CSCThread&)>;

	enum class CSCThreadState
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
		m_state.store(CSCThreadState::Running, std::memory_order_release);

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
				m_state.store(CSCThreadState::Stopped, std::memory_order_release);
			});
	}

	void pause()
	{
		m_paused.store(true, std::memory_order_release);
		if (!m_stop_requested.load(std::memory_order_acquire))
			m_state.store(CSCThreadState::Paused, std::memory_order_release);
	}

	void resume()
	{
		m_paused.store(false, std::memory_order_release);
		if (!m_stop_requested.load(std::memory_order_acquire))
			m_state.store(CSCThreadState::Running, std::memory_order_release);
		m_cv.notify_one();
	}

	void stop()
	{
		m_stop_requested.store(true, std::memory_order_release);
		m_paused.store(false, std::memory_order_release);
		m_state.store(CSCThreadState::Stopping, std::memory_order_release);
		m_cv.notify_one();

		if (m_thread.joinable())
			m_thread.join();

		m_state.store(CSCThreadState::Stopped, std::memory_order_release);
	}

	bool is_running() const
	{
		return m_state.load(std::memory_order_acquire) == CSCThreadState::Running;
	}

	bool is_stopped() const
	{
		return m_state.load(std::memory_order_acquire) == CSCThreadState::Stopped;
	}

	bool is_paused() const
	{
		return m_state.load(std::memory_order_acquire) == CSCThreadState::Paused;
	}

	CSCThreadState state() const
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
	std::atomic<CSCThreadState> m_state{ CSCThreadState::Stopped };

	std::mutex m_mtx;
	std::condition_variable m_cv;
};
