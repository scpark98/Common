#pragma once

#include <afxwin.h>
#include <map>

static const UINT Message_CWatchFileSystem = ::RegisterWindowMessage(_T("MessageString_CWatchFileSystem"));

class CWatchFileSystemMessage
{
public:
	CWatchFileSystemMessage(int _message, int _action, CString _target_path, CString _filename)
	{
		message = _message;
		action = _action;
		target_path = _target_path;
		filename = _filename;
	}

	int			message;
	int			action;			//�̺�Ʈ �׼� ���� (FILE_ACTION_ADDED,...)
	CString		target_path;	//����͸����� ���� fullpath
	CString		filename;		//�̺�Ʈ�� �߻��� ���� ���� fullpath
};

class CWatchFileSystem
{
public:
	CWatchFileSystem();
	~CWatchFileSystem();

	void		init(HWND hParent);
	void		add(CString target_path);

	//Ư�� �����带 ����. ""�̸� ��� ����͸� ����.
	void		stop(CString target_path = _T(""));
	bool		is_thread_running(CString job_name);
	bool		is_all_threads_stopped();

protected:
	enum WATCH_THREAD_STATUS
	{
		WATCH_THREAD_NOT_RUNNING = -1,	//thread�� ���� ���۵��� ���� ����
		WATCH_THREAD_IS_STOPPING,		//��� ������� �������� ����
		WATCH_THREAD_IS_RUNNING,		//���������� �������� ����
	};

	enum CWatchFileSystemMessages
	{
		message = 0,
	};

	//id�� thread ���� ���� ���� ��. ������ų �� �ʿ�. ���� ��� �����Ǿ����� Ȯ�νÿ��� �ʿ�.
	//1:running, 0:stop thread, -1:stopped
	std::map<CString, int> m_map;

	HWND		m_parent = NULL;
	void		thread_watch(CString target_path);
};

