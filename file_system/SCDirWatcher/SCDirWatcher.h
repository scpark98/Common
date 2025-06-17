#pragma once

/*
- CReadDirectoryChanges 코드에 대한 wrapper class로 제작.

- CSCDirWatcher 인스턴스 선언 후 init()에 의해 모니터링 쓰레드가 구동되며 add()를 통해 모니터링 폴더를 추가할 수 있다.

- 모니터링 중인 폴더의 변경 사항은 CSCDirWatcherMessage 메시지로 전달된다.
  parent에서 ON_REGISTERED_MESSAGE(Message_CSCDirWatcher, &CASeeDlg::on_message_CSCDirWatcher) 로 수신하고 다음과 같이 처리한다.
		CSCDirWatcherMessage* msg = reinterpret_cast<CSCDirWatcherMessage*>(wParam);
		TRACE(_T("action: %d, path0: %s, path1: %s\n"), msg->action, msg->path0, msg->path1);

*/

#include <afxwin.h>
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

	void		init(CWnd* parent);
	void		add(CString folder, bool watch_sub_dir = false);

	//모니터링 중인 특정 폴더만 중지시킨다. 구현중...
	void		stop(CString folder);

	//모든 모니터링을 중지한다.
	void		stop_all();

protected:
	CWnd*		m_parent = nullptr;
	CReadDirectoryChanges m_watcher;

	CString		m_old_path; //이전 경로를 저장하여 변경된 경로를 알 수 있도록 한다.

	DWORD dwNotificationFlags = FILE_NOTIFY_CHANGE_FILE_NAME | FILE_NOTIFY_CHANGE_DIR_NAME | FILE_NOTIFY_CHANGE_ATTRIBUTES | 
								FILE_NOTIFY_CHANGE_SIZE | FILE_NOTIFY_CHANGE_LAST_WRITE | FILE_NOTIFY_CHANGE_LAST_ACCESS |
								FILE_NOTIFY_CHANGE_CREATION | FILE_NOTIFY_CHANGE_SECURITY;

	bool	m_is_thread_running = false;
	bool	m_is_thread_stopped = false;
	void	thread_directory_change_watcher();
};
