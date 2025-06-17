#include "SCDirWatcher.h"

CSCDirWatcher::CSCDirWatcher()
{
}

CSCDirWatcher::~CSCDirWatcher()
{
	if (m_is_thread_running)
	{
		m_is_thread_running = false;
		while (!m_is_thread_stopped)
			std::this_thread::sleep_for(std::chrono::seconds(1)); // Simulate work
	}
}

void CSCDirWatcher::init(CWnd* parent)
{
	m_parent = parent;

    std::thread th(&CSCDirWatcher::thread_directory_change_watcher, this);
    th.detach();
}

void CSCDirWatcher::add(CString folder, bool watch_sub_dir)
{
    if (!m_is_thread_running)
        TRACE(_T("warning : thread_directory_change_watcher() is not running. call init() member with parent parameter.\n"));

    m_watcher.AddDirectory(folder, watch_sub_dir, dwNotificationFlags);
}

//모니터링 중인 특정 폴더만 중지시킨다. 구현중...
void CSCDirWatcher::stop(CString folder)
{
    m_watcher.StopWatchingDirectory(folder);
}

//모든 모니터링을 중지한다.
void CSCDirWatcher::stop_all()
{
    m_watcher.AddDirectory(_T(""), false, dwNotificationFlags);
}

void CSCDirWatcher::thread_directory_change_watcher()
{
	m_is_thread_running = true;
    m_is_thread_stopped = false;

    const HANDLE hStdIn = ::GetStdHandle(STD_INPUT_HANDLE);
    const HANDLE handles[] = { hStdIn, m_watcher.GetWaitHandle() };

    std::wstring buf;

	while (m_is_thread_running)
	{
		// Watch directory logic here
		//TRACE(_T("%ld, Watching directory...\n"), GetTickCount());

		//WaitForMultipleObjectsEx()를 사용하면 Invalid Handle 에러가 발생하여 WaitForSingleObject()로 변경함.
        //이 waiting을 중지시키기 위한 이벤트를 발생시켜야 하는데 찾지 못하여 우선 1초 간격으로 while문을 실행함.
        //그렇지 않으면 계속 wait하므로 종료시킬 방법이 없음.
        //DWORD rc = ::WaitForMultipleObjectsEx(_countof(handles), handles, false, INFINITE, true);
        DWORD rc = ::WaitForSingleObject(m_watcher.GetWaitHandle(), 1000);// INFINITE);
        switch (rc)
        {
            //원래는 0번에서 키입력을 처리하고 WAIT_OBJECT_0 + 1에서 notification을 처리하는 구조였으나
            //위에서 wait 방식을 변경해서 그런지 무조건 0번 이벤트로만 통지가 온다.
        case WAIT_OBJECT_0 + 0:
            // hStdIn was signaled. This can happen due to mouse input, focus change,
            // Shift keys, and more.  Delegate to TryGetKeyboardInput().
            // TryGetKeyboardInput sets bTerminate to true if the user hits Esc.
            //if (TryGetKeyboardInput(hStdIn, m_is_thread_stopped, buf))
            //    m_changes.AddDirectory(buf.c_str(), false, dwNotificationFlags);
            //break;
        case WAIT_OBJECT_0 + 1: // We've received a notification in the queue.
        {
            std::wstring wstrFilename;
            if (m_watcher.CheckOverflow())
            {
                TRACE(_T("Queue overflowed.\n"));
            }
            else
            {
                DWORD dwAction;
                m_watcher.Pop(dwAction, wstrFilename);
                //TRACE(_T("rc = %d, %s %s\n"), rc, ExplainAction(dwAction), wstrFilename.c_str());
                if (dwAction == FILE_ACTION_RENAMED_OLD_NAME)
                {
                    m_old_path = CString(wstrFilename.c_str());
                    continue; // Skip the old name, wait for the new name.
                }
                else
                {
                    if (dwAction != FILE_ACTION_RENAMED_NEW_NAME)
                        m_old_path.Empty();

                    ::SendMessage(m_parent->m_hWnd, Message_CSCDirWatcher, (WPARAM)&CSCDirWatcherMessage((int)dwAction, CString(wstrFilename.c_str()), m_old_path), 0);
                }
            }
        }
        break;
        case WAIT_OBJECT_0 + 0xFFFFFFFF:
			TRACE(_T("error = %d\n"), GetLastError());
            break;
        case WAIT_IO_COMPLETION:
            // Nothing to do.
            break;
        }
	}

	m_is_thread_running = false;
	m_is_thread_stopped = true;
	TRACE("thread_watch() stoppped.\n");
}
