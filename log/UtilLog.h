#pragma once

/*
* 기존 Koino에서 사용하던 UtilLog에 로그를 출력한 함수명과 라인 정보를 추가.
* (#define logWrite...를 사용하므로 멀티쓰레드가 가능할지는 아직 확인 못함)
* 
* app.h에서
	#include "../../Common/log/logh"
	...
	전역변수로 아래 변수 선언.
	extern UtilLog gLog;

  app.cpp에서 실제 전역변수 선언.

  UtilLog gLog;

  //아래와 같이 바로 사용하면 exe파일의 타이틀명으로 로그 파일 자동 생성됨.
	logWrite(LOG_LEVEL_RELEASE, _T("log test = %d, %s, %s"), 123, _T("abc"), _T("한글  테스트"));

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

#define logWrite(level, fmt, ...) pLog->Write(level, __function__, __LINE__, fmt, ##__VA_ARGS__)

class UtilLog
{
public:
	UtilLog();
	virtual ~UtilLog();

	//로그파일 이름을 주지 않으면 실행파일명을 이용.
	BOOL Init(CString filename = _T(""), int showLogLevel = LOG_LEVEL_RELEASE);
	CString Write(int logLevel, TCHAR* func, int line, LPCTSTR format, ...);
	BOOL Release();
	CString GetLogFilePath();
	CString GetLogFileFolder();

protected:
	CString m_logFileName;
	CString m_filePath;
	CString m_processTitle;
	int m_showLogLevel;
	FILE* m_fp;

};

extern UtilLog* pLog;