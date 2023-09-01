#pragma once

/*
* 기존 Koino에서 사용하던 UtilLog에 로그를 출력한 함수명과 라인 정보를 추가.
* (#define logWrite...를 사용하므로 멀티쓰레드가 가능할지는 아직 확인 못함)
* 
* app.h에서
	#include "../../Common/log/UtilLog.h"
	...
	전역변수로 아래 변수 선언.
	extern UtilLog gLog;

  app.cpp에서 실제 전역변수 선언.

  UtilLog gLog;

	//만약 로그파일의 위치를 특정하고자 하면 아래와 같이 Init()하면 해당 폴더아래 Log 폴더에 로그파일 생성.
	gLog.Init(_T("../../custom log folder/folder1/folder2"));

	//위치를 지정하지 않고 아래와 같이 바로 사용하면 exe 파일 아래의 Log 폴더에 로그 파일 자동 생성됨.
	logWrite(LOG_LEVEL_RELEASE, _T("log test = %d, %s, %s"), 123, _T("abc"), _T("한글  테스트"));

	//도중에 로그파일의 위치를 변경해도 적용되는지 확인 필요함.
*/

#include <afxwin.h>

#define LOG_LEVEL_RELEASE	1
#define LOG_LEVEL_DEBUG		2

#ifdef UNICODE
#define CHARSET _T(",ccs=UTF-8")
#define __function__ __FUNCTIONW__
#else
#define CHARSET _T("")
#define __function__ __FUNCTION__
#endif

//#define logWrite(fmt, ...) logWrite(LOG_LEVEL_RELEASE, __function__, __LINE__, fmt, ##__VA_ARGS__)
#define logWrite(level, fmt, ...) pLog->Write(level, __function__, __LINE__, fmt, ##__VA_ARGS__)

class UtilLog
{
public:
	UtilLog();
	virtual ~UtilLog();

	//폴더 및 로그파일 타이틀을 주지 않으면 실행파일 하위의 "Log" 폴더에 실행파일명[YYYYMMDD].log 파일이 자동 생성됨.
	//위와 같이 기본 위치와 파일명을 사용할 경우는 Init()을 호출하지 않아도 자동 생성되며
	//특정 위치에 로그파일을 저장하고자 하는 경우는 폴더를 지정해주면 됨.
	//예)특정 폴더 지정 : "d:\\test\\Log"
	//예)상대 경로 지정 : "..\\..\\Log" (실행파일이 있는 상위의 상위 폴더에 Log라는 폴더를 생성하여 로그파일 저장)
	//gLog.Init(_T("../../custom log folder"));와 같이 호출
	BOOL Init(CString logFolder = _T(""), CString filetitle = _T(""), int showLogLevel = LOG_LEVEL_RELEASE);
	CString Write(int logLevel, TCHAR* func, int line, LPCTSTR format, ...);
	BOOL Release();
	CString get_log_full_path() { return m_fullpath; }
	bool	recursive_make_full_directory(LPCTSTR sFolder);

protected:
	CString m_filetitle;
	CString m_filename;
	CString m_folder;
	CString m_fullpath;
	int m_showLogLevel;
	FILE* m_fp;

};

extern UtilLog* pLog;