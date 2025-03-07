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
	int			action;			//이벤트 액션 종류 (FILE_ACTION_ADDED,...)
	CString		target_path;	//모니터링중인 폴더 fullpath
	CString		filename;		//이벤트가 발생한 실제 파일 fullpath
};

class CWatchFileSystem
{
public:
	CWatchFileSystem();
	~CWatchFileSystem();

	void		init(HWND hParent);
	void		add(CString target_path);

	//특정 쓰레드를 중지. ""이면 모든 모니터링 중지.
	void		stop(CString target_path = _T(""));
	bool		is_thread_running(CString job_name);
	bool		is_all_threads_stopped();

protected:
	enum WATCH_THREAD_STATUS
	{
		WATCH_THREAD_NOT_RUNNING = -1,	//thread가 아직 시작되지 않은 상태
		WATCH_THREAD_IS_STOPPING,		//취소 명령으로 중지중인 상태
		WATCH_THREAD_IS_RUNNING,		//정상적으로 실행중인 상태
	};

	enum CWatchFileSystemMessages
	{
		message = 0,
	};

	//id와 thread 동작 유무 저장 맵. 중지시킬 때 필요. 또한 모두 중지되었는지 확인시에도 필요.
	//1:running, 0:stop thread, -1:stopped
	std::map<CString, int> m_map;

	HWND		m_parent = NULL;
	void		thread_watch(CString target_path);
};

