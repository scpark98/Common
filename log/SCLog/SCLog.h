#pragma once

/*
* ���� Koino���� ����ϴ� UtilLog�� �α׸� ����� �Լ���� ���� ������ �߰�.
* (#define logWrite...��ũ�θ� ����ϹǷ� ��Ƽ�����尡 ���������� ���� Ȯ�� ����)
* 
* !����!
* �Ϲ����� �ۿ����� ���� �� ����������
* LMMAgent.exe�� ���� SYSTEM session 0������ ����Ǵ� ���α׷�������
* �� ��ũ�� �������� ���� ��Ȯ�� �� �� ������ crash�� �߻��ϹǷ� ��� ����.
* DbgOutA()�� �̿��� ��!

* ��Ƽ�����忡�� �α� ��Ͽ� ������ �߻��ϴ��� ������ �׽�Ʈ �Ϸ�.
  (�׽�Ʈ ���. 0 ~ 999���� 10ms �������� �α׸� ����ϴ� 50�� �����带 �����Ͽ� ��� �� Ȯ����)

* app.h���� (������Ʈ ������ ���� App class�� �ƴ� stdafx.h �Ǵ� pch.h � ������ ���� ����)
	#include "../Common/log/SCLog/SCLog.h"
	...
	���������� �Ʒ� ���� ����.
	extern CSCLog gLog;

  app.cpp���� ���� �������� ����.
    CSCLog gLog;

  

[set Log folder path]
	//set()�Լ��� ���� �α����� ��������, �α����� Ÿ��Ʋ, �α� ������ ������ �� �ִ�.
	//���� �α������� ��θ� Ư���ϰ��� �ϸ� �Ʒ��� ���� set()�ϸ� �ش� �����Ʒ� Log ������ �α����� ����.
	//�α����ϸ� ������ projectName_yyyymmdd.log
	gLog.set(_T("../../custom log folder/folder1/folder2"), _T("TestLogFileName"), SCLOG_LEVEL_DEBUG);
	�⺻ �α� ������ SCLOG_LEVEL_RELEASE�̹Ƿ� �� ��� logWriteD()�� ȣ���� ���� �α� ����� ��ŵ�ȴ�.

[show or not the function name or line number]
	//� ������Ʈ�� ��쿡�� �Լ����� ����Ǽ��� �ȵǴ� ��쵵 �ִ�. �Լ���� ���ι�ȣ ǥ�� ������ �ɼ�ó��.
	show_function_name(false);
	show_line_number(false);

	//��θ� ������ �������� �ʰ� �Ʒ��� ���� �ٷ� ����ϸ� exe ���� �Ʒ��� Log ������ �α� ���� �ڵ� ������.
	gLog.write(_T("log test = %d, %s, %s"), 123, _T("abc"), _T("�ѱ�  �׽�Ʈ"));

[write blank line log]
	//��Ȥ Ư���� �α� ������ ��� linefeed �Ǵ� �Լ����� ��ϵ��� �������� "" �Ǵ� " "�� �ѱ�.
	//gLog.write(_T(""));	//�� �� �� ���� �߰�
	//gLog.write(_T(" "));	//�ð�, �Լ����������� ��µǴ� ���� �߰�
*/

#include <afxwin.h>
#include <mutex>

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

class CSCLog
{
public:
	CSCLog();
	virtual ~CSCLog();

	//���� �� �α����� Ÿ��Ʋ�� ���� ������ �������� ������ "Log" ������ �������ϸ�[YYYYMMDD].log ������ �ڵ� ������.
	//���� ���� �⺻ ��ġ�� ���ϸ��� ����� ���� set()�� ȣ������ �ʾƵ� �ڵ� �����Ǹ�
	//Ư�� ��ġ�� �α������� �����ϰ��� �ϴ� ���� ������ �������ָ� ��.
	//��)Ư�� ���� ���� : "d:\\test\\Log"
	//��)��� ��� ���� : "..\\..\\Log" (���������� �ִ� ������ ���� ������ Log��� ������ �����Ͽ� �α����� ����)
	//gLog.set(_T("../../custom log folder"));�� ���� ȣ��
	//file_title�� ���� ������ "�������ϸ�_yyyymmdd.log" ���Ϸ� ����ǰ�
	//�α����ϸ��� �����ϰ� �ʹٸ� file_title�� ���� �ָ� "file_title_yyyymmdd.log"�� ����ȴ�.
	bool		set(CString log_folder = _T(""), CString file_title = _T(""), int show_log_level = SCLOG_LEVEL_RELEASE);
	void		set_log_level(int log_level = SCLOG_LEVEL_RELEASE);

	//���α׷� ���� �� �Ϲ����� ������ �α׿� ����ϸ鼭 �����Ѵ�.
	//���� �ܶ� ǥ��, exe path, file version, built time�� ����Ѵ�.
	void		write_start_log();

	//���α׷� ���� �� �Ϲ����� ������ �α׿� ����ϸ鼭 �����Ѵ�.
	void		write_end_log();

	//�Լ��� ǥ�� ����. default show
	void		show_function_name(bool show) { m_show_function_name = show; }

	//���ι�ȣ ǥ�� ����. default show
	void		show_line_number(bool show) { m_show_line_number = show; }

	//logWrite ��ũ�θ� ���ؼ� �Լ���, ���μ� ���� ���� ������ֹǷ� ��������
	//LMMAgent �Ǵ� ��Ƽ������� �����ϴ� ������Ʈ������ ���� �������� �����ϴ� ���Ͽ�
	//�ܼ� �α׸� ����� write �Լ��� �߰���.
	CString		write(LPCTSTR format, ...);
	CString		write(int log_level, TCHAR* func, int line, LPCTSTR format, ...);

	bool		release();
	CString		get_log_full_path() { return m_log_fullpath; }
	bool		recursive_make_full_directory(LPCTSTR sFolder);


	//���� �α������� �����ش�.
	void		open_log_file();

protected:
	std::mutex	m_mutex;
	CTime		m_tlog = CTime::GetCurrentTime();
	CString		m_log_fullpath;
	CString		m_log_file_title;
	CString		m_log_folder;
	int			m_log_level = SCLOG_LEVEL_RELEASE;
	bool		m_show_function_name = true;
	bool		m_show_line_number = true;
	FILE*		m_fp;

};

extern CSCLog* pLog;