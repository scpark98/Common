#pragma once

/*
- CReadDirectoryChanges 코드에 대한 wrapper class로 제작. (x86, x64 테스트 완료)

- CSCDirWatcher 인스턴스 선언 후 init(this);에 의해 모니터링 쓰레드가 구동되며
  add(), stop(), is_watching() 등을 통해 모니터링 할 수 있다.

- 모니터링 중인 폴더 및 파일들의 변경 사항은 CSCDirWatcherMessage 메시지로 전달된다.
  parent에서 ON_REGISTERED_MESSAGE(Message_CSCDirWatcher, &CParentDlg::on_message_CSCDirWatcher) 로 수신하고 다음과 같이 처리한다.
		CSCDirWatcherMessage* msg = (CSCDirWatcherMessage*)wParam;
		TRACE(_T("action: %d, path0: %s, path1: %s\n"), msg->action, msg->path0, msg->path1);

*/

#include <afxwin.h>
#include <deque>
#include "ReadDirectoryChanges.h"

static const UINT Message_CSCDirWatcher = ::RegisterWindowMessage(_T("MessageString_CSCDirWatcher"));

class CSCDirWatcherMessage
{
public:
	CSCDirWatcherMessage(int _action, CString _path0, CString _path1 = _T(""))
	{
		action = _action;
		path0 = _path0;
		path1 = _path1;
	}

	//FILE_ACTION_ADDED(1), FILE_ACTION_REMOVED(2), FILE_ACTION_MODIFIED(3), FILE_ACTION_RENAMED_OLD_NAME(4), FILE_ACTION_RENAMED_NEW_NAME(5)
	int		action = 0; 
	CString	path0;
	CString path1; //rename 시에만 사용됨
};

class CSCDirWatcher
{
public:
	CSCDirWatcher();
	~CSCDirWatcher();

	//parent를 지정하여 모니터링 쓰레드가 parent에 변경 내용을 전달할 수 있도록 한다.
	void		init(CWnd* parent);

	void		add(CString folder, bool watch_sub_dir = false);
	void		add(std::deque<CString> folders, bool watch_sub_dir = false);

	//모니터링 중인 폴더를 중지시킨다. folder가 ""인 경우에는 모든 모니터링을 중지시킨다.
	//thread_directory_change_watcher()는 여전히 실행되고 있으므로
	//모니터링을 원하는 폴더를 다시 추가하면 된다.
	void		stop(CString folder = _T(""));

	bool		is_watching(CString folder);

	static CString action_str(int action)
	{
		switch (action)
		{
			case FILE_ACTION_ADDED: return _T("added");
			case FILE_ACTION_REMOVED: return _T("removed");
			case FILE_ACTION_MODIFIED: return _T("modified");
			case FILE_ACTION_RENAMED_OLD_NAME: return _T("renamed old name");
			case FILE_ACTION_RENAMED_NEW_NAME: return _T("renamed");
			default: return _T("unknown action");
		}
	}

protected:
	CWnd*		m_parent = nullptr;
	CReadDirectoryChanges m_watcher;

	CString		m_old_path; //이전 경로를 저장하여 변경된 경로를 알 수 있도록 한다.

	//all flags
	//DWORD		dwNotificationFlags =	FILE_NOTIFY_CHANGE_FILE_NAME | FILE_NOTIFY_CHANGE_DIR_NAME | FILE_NOTIFY_CHANGE_ATTRIBUTES | 
	//									FILE_NOTIFY_CHANGE_SIZE | FILE_NOTIFY_CHANGE_LAST_WRITE | FILE_NOTIFY_CHANGE_LAST_ACCESS |
	//									FILE_NOTIFY_CHANGE_CREATION | FILE_NOTIFY_CHANGE_SECURITY;
	DWORD		dwNotificationFlags =	FILE_NOTIFY_CHANGE_FILE_NAME | FILE_NOTIFY_CHANGE_DIR_NAME | 
										FILE_NOTIFY_CHANGE_SIZE | FILE_NOTIFY_CHANGE_LAST_WRITE |
										FILE_NOTIFY_CHANGE_CREATION;

	bool		m_is_thread_running = false;
	bool		m_is_thread_terminated = false;
	void		thread_directory_change_watcher();
};
