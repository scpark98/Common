#pragma once

/*
- CReadDirectoryChanges �ڵ忡 ���� wrapper class�� ����.

- CSCDirWatcher �ν��Ͻ� ���� �� init()�� ���� ����͸� �����尡 �����Ǹ� add()�� ���� ����͸� ������ �߰��� �� �ִ�.

- ����͸� ���� ������ ���� ������ CSCDirWatcherMessage �޽����� ���޵ȴ�.
  parent���� ON_REGISTERED_MESSAGE(Message_CSCDirWatcher, &CASeeDlg::on_message_CSCDirWatcher) �� �����ϰ� ������ ���� ó���Ѵ�.
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
	CString path1; //rename �ÿ��� ����
};

class CSCDirWatcher
{
public:
	CSCDirWatcher();
	~CSCDirWatcher();

	void		init(CWnd* parent);
	void		add(CString folder, bool watch_sub_dir = false);

	//����͸� ���� Ư�� ������ ������Ų��. ������...
	void		stop(CString folder);

	//��� ����͸��� �����Ѵ�.
	void		stop_all();

protected:
	CWnd*		m_parent = nullptr;
	CReadDirectoryChanges m_watcher;

	CString		m_old_path; //���� ��θ� �����Ͽ� ����� ��θ� �� �� �ֵ��� �Ѵ�.

	DWORD dwNotificationFlags = FILE_NOTIFY_CHANGE_FILE_NAME | FILE_NOTIFY_CHANGE_DIR_NAME | FILE_NOTIFY_CHANGE_ATTRIBUTES | 
								FILE_NOTIFY_CHANGE_SIZE | FILE_NOTIFY_CHANGE_LAST_WRITE | FILE_NOTIFY_CHANGE_LAST_ACCESS |
								FILE_NOTIFY_CHANGE_CREATION | FILE_NOTIFY_CHANGE_SECURITY;

	bool	m_is_thread_running = false;
	bool	m_is_thread_stopped = false;
	void	thread_directory_change_watcher();
};
