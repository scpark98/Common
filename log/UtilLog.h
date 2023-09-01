#pragma once

/*
* ���� Koino���� ����ϴ� UtilLog�� �α׸� ����� �Լ���� ���� ������ �߰�.
* (#define logWrite...�� ����ϹǷ� ��Ƽ�����尡 ���������� ���� Ȯ�� ����)
* 
* app.h����
	#include "../../Common/log/UtilLog.h"
	...
	���������� �Ʒ� ���� ����.
	extern UtilLog gLog;

  app.cpp���� ���� �������� ����.

  UtilLog gLog;

	//���� �α������� ��ġ�� Ư���ϰ��� �ϸ� �Ʒ��� ���� Init()�ϸ� �ش� �����Ʒ� Log ������ �α����� ����.
	gLog.Init(_T("../../custom log folder/folder1/folder2"));

	//��ġ�� �������� �ʰ� �Ʒ��� ���� �ٷ� ����ϸ� exe ���� �Ʒ��� Log ������ �α� ���� �ڵ� ������.
	logWrite(LOG_LEVEL_RELEASE, _T("log test = %d, %s, %s"), 123, _T("abc"), _T("�ѱ�  �׽�Ʈ"));

	//���߿� �α������� ��ġ�� �����ص� ����Ǵ��� Ȯ�� �ʿ���.
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

	//���� �� �α����� Ÿ��Ʋ�� ���� ������ �������� ������ "Log" ������ �������ϸ�[YYYYMMDD].log ������ �ڵ� ������.
	//���� ���� �⺻ ��ġ�� ���ϸ��� ����� ���� Init()�� ȣ������ �ʾƵ� �ڵ� �����Ǹ�
	//Ư�� ��ġ�� �α������� �����ϰ��� �ϴ� ���� ������ �������ָ� ��.
	//��)Ư�� ���� ���� : "d:\\test\\Log"
	//��)��� ��� ���� : "..\\..\\Log" (���������� �ִ� ������ ���� ������ Log��� ������ �����Ͽ� �α����� ����)
	//gLog.Init(_T("../../custom log folder"));�� ���� ȣ��
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