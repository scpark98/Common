#include "WatchFileSystem.h"

#include <thread>

CWatchFileSystem::CWatchFileSystem()
{

}

CWatchFileSystem::~CWatchFileSystem()
{
	stop();

	while (!is_all_threads_stopped())
	{
		TRACE(_T("wait until all thread stop...\n"));
		Sleep(100);
	}
}

void CWatchFileSystem::init(HWND hParent)
{
	m_parent = hParent;
}

void CWatchFileSystem::add(CString target_path)
{
	auto it = m_map.find(target_path);
	if (it != m_map.end())
		m_map[target_path] = WATCH_THREAD_IS_RUNNING;
	else
		m_map.insert(std::pair<CString, int>(target_path, WATCH_THREAD_IS_RUNNING));

	thread_watch(target_path);
	//std::thread th(&CWatchFileSystem::thread_watch, this, target_path);
	//th.detach();
}

void CWatchFileSystem::thread_watch(CString target_path)
{
	HANDLE hDir = CreateFile(
		target_path,
		FILE_LIST_DIRECTORY,
		FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
		NULL,
		OPEN_EXISTING,
		FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OVERLAPPED,
		NULL);

	if (hDir == INVALID_HANDLE_VALUE)
	{
		TRACE(_T("fail to CreateFile(%s). error = %d"), target_path, GetLastError());
		return;
	}

	alignas(DWORD) uint8_t change_buf[1024] = { 0, };
	DWORD bytesReturned;
	FILE_NOTIFY_INFORMATION* fni;

	OVERLAPPED overlapped;
	overlapped.hEvent = CreateEvent(NULL, FALSE, 0, NULL);

	BOOL success = ::ReadDirectoryChangesW(
		hDir, change_buf, 1024, TRUE,
		FILE_NOTIFY_CHANGE_FILE_NAME |
		FILE_NOTIFY_CHANGE_DIR_NAME |
		FILE_NOTIFY_CHANGE_LAST_WRITE,
		NULL, &overlapped, NULL);

	while (m_map[target_path] == WATCH_THREAD_IS_RUNNING)
	{
		//TRACE(_T("watching %s\n"), target_path);

		DWORD result = WaitForSingleObject(overlapped.hEvent, 0);

		if (result == WAIT_OBJECT_0)
		{
			DWORD bytes_transferred;
			GetOverlappedResult(hDir, &overlapped, &bytes_transferred, FALSE);

			FILE_NOTIFY_INFORMATION* event = (FILE_NOTIFY_INFORMATION*)change_buf;

			for (;;)
			{
				DWORD name_len = event->FileNameLength / sizeof(wchar_t);

				switch (event->Action)
				{
					case FILE_ACTION_ADDED:
					{
						TRACE(L"       Added: %.*s\n", name_len, event->FileName);
					}
					break;
					case FILE_ACTION_REMOVED:
					{
						TRACE(L"     Removed: %.*s\n", name_len, event->FileName);
					}
					break;
					case FILE_ACTION_MODIFIED:
					{
						TRACE(L"    Modified: %.*s\n", name_len, event->FileName);
					}
					break;
					case FILE_ACTION_RENAMED_OLD_NAME:
					{
						TRACE(L"Renamed from: %.*s\n", name_len, event->FileName);
					}
					break;
					case FILE_ACTION_RENAMED_NEW_NAME:
					{
						TRACE(L"          to: %.*s\n", name_len, event->FileName);
					}
					break;
					default:
					{
						TRACE("Unknown action!\n");
					}
					break;
				}

				// Are there more events to handle?
				if (event->NextEntryOffset)
				{
					*((uint8_t**)&event) += event->NextEntryOffset;
				}
				else
				{
					break;
				}
			}
			/*
			do
			{
				switch (fni->Action)
				{
					case FILE_ACTION_ADDED:
						TRACE(_T("FILE_ACTION_ADDED\n"));
						break;
					case FILE_ACTION_REMOVED:
						TRACE(_T("FILE_ACTION_REMOVED\n"));
						break;
					case FILE_ACTION_MODIFIED:
						TRACE(_T("FILE_ACTION_MODIFIED\n"));
						::SendMessage(m_parent, Message_CWatchFileSystem, (WPARAM)&CWatchFileSystemMessage(message, fni->Action, target_path, fni->FileName), (LPARAM)0);
						break;
					case FILE_ACTION_RENAMED_OLD_NAME:
						TRACE(_T("FILE_ACTION_RENAMED_OLD_NAME. %s\n"), fni->FileName);
						break;
					case FILE_ACTION_RENAMED_NEW_NAME:
						TRACE(_T("FILE_ACTION_RENAMED_NEW_NAME. %s\n"), fni->FileName);
						break;
					default:
						break;
				}

				fni = fni->NextEntryOffset ? reinterpret_cast<FILE_NOTIFY_INFORMATION*> (reinterpret_cast<BYTE*>(fni) + fni->NextEntryOffset) : nullptr;
			} while (fni);
			*/
		}

		// Queue the next event
		BOOL success = ReadDirectoryChangesW(
			hDir, change_buf, 1024, TRUE,
			FILE_NOTIFY_CHANGE_FILE_NAME |
			FILE_NOTIFY_CHANGE_DIR_NAME |
			FILE_NOTIFY_CHANGE_LAST_WRITE,
			NULL, &overlapped, NULL);
		//std::this_thread::sleep_for(std::chrono::milliseconds(1000));
	}

	CloseHandle(hDir);

	m_map[target_path] = WATCH_THREAD_NOT_RUNNING;
	TRACE(_T("watching %s stopped.\n"), target_path);
}

//특정 쓰레드를 중지. ""이면 모든 모니터링 중지.
void CWatchFileSystem::stop(CString target_path)
{
	std::map<CString, int>::iterator it;

	if (!target_path.IsEmpty())
	{
		std::map<CString, int>::iterator it = m_map.find(target_path);

		if (it != m_map.end() && it->second == WATCH_THREAD_IS_RUNNING)
		{
			TRACE(_T("%s job will be stop\n"), target_path);
			it->second = WATCH_THREAD_IS_STOPPING;
			return;
		}
	}
	else
	{
		//모두 중지할 경우
		for (it = m_map.begin(); it != m_map.end(); it++)
		{
			if (it->second == WATCH_THREAD_IS_RUNNING)
			{
				it->second = WATCH_THREAD_IS_STOPPING;
				Sleep(1000);
			}
		}

		m_map.clear();
	}
}

bool CWatchFileSystem::is_thread_running(CString job_name)
{
	std::map<CString, int>::iterator it = m_map.find(job_name);
	return (it != m_map.end() && it->second == WATCH_THREAD_IS_RUNNING);
}

bool CWatchFileSystem::is_all_threads_stopped()
{
	for (auto it = m_map.begin(); it != m_map.end(); it++)
	{
		if (it->second == WATCH_THREAD_IS_RUNNING)
			return false;
	}

	return true;
}
