#pragma once

/*
* ���� Koino���� ����ϴ� UtilLog�� �α׸� ����� �Լ���� ���� ������ �߰�.
* (#define logWrite...�� ����ϹǷ� ��Ƽ�����尡 ���������� ���� Ȯ�� ����)
* 
* app.h����
	#include "../../Common/log/logh"
	...
	���������� �Ʒ� ���� ����.
	extern UtilLog gLog;

  app.cpp���� ���� �������� ����.

  UtilLog gLog;

  //�Ʒ��� ���� �ٷ� ����ϸ� exe������ Ÿ��Ʋ������ �α� ���� �ڵ� ������.
	logWrite(LOG_LEVEL_RELEASE, _T("log test = %d, %s, %s"), 123, _T("abc"), _T("�ѱ�  �׽�Ʈ"));

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

	//�α����� �̸��� ���� ������ �������ϸ��� �̿�.
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