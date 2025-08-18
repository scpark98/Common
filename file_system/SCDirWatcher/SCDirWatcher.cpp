#include "SCDirWatcher.h"

CSCDirWatcher::CSCDirWatcher()
{
}

CSCDirWatcher::~CSCDirWatcher()
{
	if (m_is_thread_running)
	{
		m_is_thread_running = false;
		while (!m_is_thread_terminated)
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
	add(std::deque<CString>{ folder }, watch_sub_dir);
}

void CSCDirWatcher::add(std::deque<CString> folder, bool watch_sub_dir)
{
    if (!m_is_thread_running)
        TRACE(_T("warning : thread_directory_change_watcher() is not running. call init() member with parent parameter.\n"));

    for (int i = 0; i < folder.size(); i++)
    {
        if (folder[i].GetLength() > 0 && folder[i].Right(1) == _T('\\'))
			folder[i].Delete(folder[i].GetLength() - 1, 1); // Remove trailing backslash

        if (m_watcher.is_watching(folder[i]))
            continue;

        //folder가 존재하지 않으면 add watching이 될 수 없다.
        //그렇다고 이 클래스에서 해당 폴더를 생성하여 watching을 하는 것도 범위를 넘을 수 있다.
        //이 클래스를 사용하는 곳에서 watching을 하고자 하는 폴더가 없다면 생성한 후 add watching을 하는 것이 맞다.
        if (PathFileExists(folder[i]))
        {
            if (PathIsDirectory(folder[i]))
                m_watcher.AddDirectory(folder[i], watch_sub_dir, dwNotificationFlags);
            else
                TRACE(_T("%s is not directory.\n"), folder[i]);
        }
        else
        {
            TRACE(_T("%s is not exist.\n"), folder[i]);
        }
    }
}

//모니터링 중인 특정 폴더만 중지시킨다. 구현중...
void CSCDirWatcher::stop(CString folder)
{
    if (folder.IsEmpty())
    {
        m_watcher.AddDirectory(_T(""), false, dwNotificationFlags);
        return;
	}

    m_watcher.stop_watching_directory(folder);
}

bool CSCDirWatcher::is_watching(CString folder)
{
    return m_watcher.is_watching(folder);
}

void CSCDirWatcher::thread_directory_change_watcher()
{
	m_is_thread_running = true;
    m_is_thread_terminated = false;

    const HANDLE hStdIn = ::GetStdHandle(STD_INPUT_HANDLE);
    const HANDLE handles[] = { hStdIn, m_watcher.GetWaitHandle() };

    std::wstring buf;

	while (m_is_thread_running)
	{
		// Watch directory logic here
		TRACE(_T("watching %d directories...\n"), m_watcher.get_watching_count());

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
            //if (TryGetKeyboardInput(hStdIn, m_is_thread_terminated, buf))
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
	m_is_thread_terminated = true;
	TRACE("thread_directory_change_watcher() terminated.\n");
}
