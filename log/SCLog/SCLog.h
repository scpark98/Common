#pragma once

/*
* ���� Koino���� ����ϴ� SCLog�� �α׸� ����� �Լ���� ���� ������ �߰�.
* (#define logWrite...�� ����ϹǷ� ��Ƽ�����尡 ���������� ���� Ȯ�� ����)
* 
* app.h���� (������Ʈ ������ ���� App class�� �ƴ� stdafx.h �Ǵ� pch.h � ������ ���� ����)
	#include "../../Common/log/SCLog.h"
	...
	���������� �Ʒ� ���� ����.
	extern SCLog gLog;

  app.cpp���� ���� �������� ����.

  SCLog gLog;

	//���� �α������� ��θ� Ư���ϰ��� �ϸ� �Ʒ��� ���� Init()�ϸ� �ش� �����Ʒ� Log ������ �α����� ����.
	//�α����ϸ� ������ projectName_yyyymmdd.log
	gLog.Init(_T("../../custom log folder/folder1/folder2"));

	//� ������Ʈ�� ��쿡�� �Լ����� ����Ǽ��� �ȵǴ� ��쵵 �ִ�. �Լ���� ���ι�ȣ ǥ�� ������ �ɼ�ó��.
	show_function_name(false);
	show_line_number(false);

	//��θ� ������ �������� �ʰ� �Ʒ��� ���� �ٷ� ����ϸ� exe ���� �Ʒ��� Log ������ �α� ���� �ڵ� ������.
	logWrite(_T("log test = %d, %s, %s"), 123, _T("abc"), _T("�ѱ�  �׽�Ʈ"));

	//SCLOG_LEVEL_WARN ��� ���� �α׷����� �����Ͽ� Ư�� �α׵鸸 ��ϵǰ� �� �� �ִµ� ���� �̱���.

	//���߿� �α������� ��ġ�� �����ص� ����Ǵ��� Ȯ�� �ʿ���.
*/

#include <afxwin.h>

#ifdef UNICODE
#define CHARSET _T(",ccs=UTF-8")
#define __function__ __FUNCTIONW__
#else
#define CHARSET _T("")
#define __function__ __FUNCTION__
#endif

#define logWrite(fmt, ...)	pLog->write(SCLOG_LEVEL_NONE, __function__, __LINE__, fmt, ##__VA_ARGS__)
#define logWriteI(fmt, ...)	pLog->write(SCLOG_LEVEL_INFO, __function__, __LINE__, fmt, ##__VA_ARGS__)
#define logWriteW(fmt, ...)	pLog->write(SCLOG_LEVEL_WARN, __function__, __LINE__, fmt, ##__VA_ARGS__)
#define logWriteE(fmt, ...)	pLog->write(SCLOG_LEVEL_ERROR, __function__, __LINE__, fmt, ##__VA_ARGS__)
#define logWriteC(fmt, ...)	pLog->write(SCLOG_LEVEL_CRITICAL, __function__, __LINE__, fmt, ##__VA_ARGS__)
#define logWriteS(fmt, ...)	pLog->write(SCLOG_LEVEL_SQL, __function__, __LINE__, fmt, ##__VA_ARGS__)
#define logWriteD(fmt, ...)	pLog->write(SCLOG_LEVEL_DEBUG, __function__, __LINE__, fmt, ##__VA_ARGS__)

enum SCLOG_LEVEL
{
	SCLOG_LEVEL_NONE = 0,
	SCLOG_LEVEL_INFO,
	SCLOG_LEVEL_WARN,
	SCLOG_LEVEL_ERROR,
	SCLOG_LEVEL_CRITICAL,
	SCLOG_LEVEL_SQL,
	SCLOG_LEVEL_RELEASE,	//?
	SCLOG_LEVEL_DEBUG,		//?
};

class SCLog
{
public:
	SCLog();
	virtual ~SCLog();

	//���� �� �α����� Ÿ��Ʋ�� ���� ������ �������� ������ "Log" ������ �������ϸ�[YYYYMMDD].log ������ �ڵ� ������.
	//���� ���� �⺻ ��ġ�� ���ϸ��� ����� ���� Init()�� ȣ������ �ʾƵ� �ڵ� �����Ǹ�
	//Ư�� ��ġ�� �α������� �����ϰ��� �ϴ� ���� ������ �������ָ� ��.
	//��)Ư�� ���� ���� : "d:\\test\\Log"
	//��)��� ��� ���� : "..\\..\\Log" (���������� �ִ� ������ ���� ������ Log��� ������ �����Ͽ� �α����� ����)
	//gLog.Init(_T("../../custom log folder"));�� ���� ȣ��
	bool		init(CString logFolder = _T(""), CString filetitle = _T(""), int showLogLevel = SCLOG_LEVEL_RELEASE);

	//�Լ��� ǥ�� ����. default show
	void		show_function_name(bool show) { m_show_function_name = show; }

	//���ι�ȣ ǥ�� ����. default show
	void		show_line_number(bool show) { m_show_line_number = show; }

	CString		write(int logLevel, TCHAR* func, int line, LPCTSTR format, ...);

	bool		release();
	CString		get_log_full_path() { return m_fullpath; }
	bool		recursive_make_full_directory(LPCTSTR sFolder);

protected:
	CString		m_filetitle;
	CString		m_filename;
	CString		m_folder;
	CString		m_fullpath;
	int			m_showLogLevel;
	bool		m_show_function_name = true;
	bool		m_show_line_number = true;
	FILE*		m_fp;

};

extern SCLog* pLog;