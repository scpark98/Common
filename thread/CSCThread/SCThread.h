#pragma once

/*
- https://docs.google.com/presentation/d/1DAoQ3uADiSL5pFGJF25tN0mNlVJnYzkBemAUIWsxO0A/edit?slide=id.p#slide=id.p
- std::thread 기반 start, pause/resume, stop 기능 지원 및 현재 스레드의 상태를 정확히 관리할 수 있는 클래스
- Common/thread/CSCThread의 cpp, h 2개 파일을 프로젝트에 추가
- 인스턴스 생성 : CSCThread	m_thread;
- m_thread.start([this](CSCThread& th) { thread_function(th); });
- start(), pause(), resume(), stop(), is_running(), is_stopped(), is_paused() 등의 멤버함수로 제어 또는 상태 확인

- .h에 함수 선언.
	public:
		void		thread_function(int index, CSCThread& th);
		static		constexpr UINT WM_APP_UI_INVOKE = WM_APP + 2;
		afx_msg		LRESULT on_ui_invoke(WPARAM wParam, LPARAM lParam);
	private:
		void		invoke_ui(std::function<void()> func);

- thread_fnction()의 구현은 아래와.
	void CTestCSCThreadDlg::thread_function(int index, CSCThread& th)
	{
		while (!th.stop_requested())
		{
			if (th.stop_requested())
				break;

			th.wait_if_paused();

			//invoke_ui()로 묶어주면 UI 관련 코드들을 안전하게 호출하여 사용할 수 있다.
			//'=' 기호를 사용하면 외부 변수 그대로 사용가능합니다.(복사 - 값이 복사되어 전달됨)
			invoke_ui([=]()
				{
					m_list.set_text(index, col_progress, i2S(progress));
				});

			//이 샘플 프로젝트처럼 수백개의 thread를 생성하여 무한루프를 돌릴 경우 딜레이를 100이 아닌 10으로 줄 경우
			//message queue에 너무 많은 메시지가 쌓여서 UI가 느리게 반응하는 현상이 발생할 수 있다. (WM_APP_UI_INVOKE 메시지가 너무 많이 쌓이는 것)
			th.sleep_for(std::chrono::milliseconds(100));
		}

		TRACE(_T("thread %d is terminated.\n"), index);
	}

	위와 같이 invoke_ui() 람다로 묶어주면 UI 관련 코드들을 메시지 큐에너지 큐에 넣고 안전하게 호출하여 사용할 수 있다.


	//아래 두 함수는 thread_function()과 함께 프로젝트에 추가해줘야 한다.
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

	//BEGIN_MESSAGE_MAP에 ON_MESSAGE(WM_APP_UI_INVOKE, &CSCShapeDlg::on_ui_invoke) 추가 필수.
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
		request_stop();

		if (m_thread.joinable())
			m_thread.join();

		m_state.store(CSCThreadState::Stopped, std::memory_order_release);
	}

	// 중지 신호만 보내고 join하지 않는다 (비차단)
	void request_stop()
	{
		m_stop_requested.store(true, std::memory_order_release);
		m_paused.store(false, std::memory_order_release);
		m_state.store(CSCThreadState::Stopping, std::memory_order_release);
		m_cv.notify_one();

		// 진행 중인 블로킹 작업 취소 (예: WinInet 핸들 닫기)
		{
			std::lock_guard<std::mutex> lock(m_cancel_mtx);
			if (m_on_cancel)
			{
				m_on_cancel();
				m_on_cancel = nullptr;
			}
		}
	}

	// 스레드 종료 대기만 수행
	void join()
	{
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

	// 워커 스레드에서 주기적으로 호출: Pause시 sleep 상태로 전환
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

	// 인터럽트 가능한 sleep. request_stop() 호출 시 즉시 깨어남.
	// 리턴값: true = 시간 만료(정상), false = 중지 요청으로 깨어남
	template<typename Rep, typename Period>
	bool sleep_for(const std::chrono::duration<Rep, Period>& duration)
	{
		std::unique_lock<std::mutex> lock(m_mtx);
		bool timed_out = !m_cv.wait_for(lock, duration, [this]
			{
				return m_stop_requested.load(std::memory_order_acquire);
			});
		return timed_out;	// true: 시간 만료, false: stop 요청됨
	}

	// 블로킹 작업 취소 콜백 등록/해제
	// 워커 스레드에서 블로킹 호출 전에 set, 호출 후에 clear
	void set_on_cancel(std::function<void()> fn)
	{
		std::lock_guard<std::mutex> lock(m_cancel_mtx);
		m_on_cancel = std::move(fn);
	}

	void clear_on_cancel()
	{
		std::lock_guard<std::mutex> lock(m_cancel_mtx);
		m_on_cancel = nullptr;
	}

private:
	std::thread m_thread;
	thread_function m_work;

	std::atomic<bool> m_stop_requested{ false };
	std::atomic<bool> m_paused{ false };
	std::atomic<CSCThreadState> m_state{ CSCThreadState::Stopped };

	std::mutex m_mtx;
	std::condition_variable m_cv;

	// 블로킹 작업 취소용
	std::mutex m_cancel_mtx;
	std::function<void()> m_on_cancel;
};
