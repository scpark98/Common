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

        //folder�� �������� ������ add watching�� �� �� ����.
        //�׷��ٰ� �� Ŭ�������� �ش� ������ �����Ͽ� watching�� �ϴ� �͵� ������ ���� �� �ִ�.
        //�� Ŭ������ ����ϴ� ������ watching�� �ϰ��� �ϴ� ������ ���ٸ� ������ �� add watching�� �ϴ� ���� �´�.
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

//����͸� ���� Ư�� ������ ������Ų��. ������...
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

		//WaitForMultipleObjectsEx()�� ����ϸ� Invalid Handle ������ �߻��Ͽ� WaitForSingleObject()�� ������.
        //�� waiting�� ������Ű�� ���� �̺�Ʈ�� �߻����Ѿ� �ϴµ� ã�� ���Ͽ� �켱 1�� �������� while���� ������.
        //�׷��� ������ ��� wait�ϹǷ� �����ų ����� ����.
        //DWORD rc = ::WaitForMultipleObjectsEx(_countof(handles), handles, false, INFINITE, true);
        DWORD rc = ::WaitForSingleObject(m_watcher.GetWaitHandle(), 1000);// INFINITE);
        switch (rc)
        {
            //������ 0������ Ű�Է��� ó���ϰ� WAIT_OBJECT_0 + 1���� notification�� ó���ϴ� ����������
            //������ wait ����� �����ؼ� �׷��� ������ 0�� �̺�Ʈ�θ� ������ �´�.
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
