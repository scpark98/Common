#pragma once

/*
* 기존 Koino에서 사용하던 SCLog에 로그를 출력한 함수명과 라인 정보를 추가.
* (#define logWrite...를 사용하므로 멀티쓰레드가 가능할지는 아직 확인 못함)
* 
* app.h에서 (프로젝트 구성에 따라 App class가 아닌 stdafx.h 또는 pch.h 등에 선언할 수도 있음)
	#include "../../Common/log/SCLog.h"
	...
	전역변수로 아래 변수 선언.
	extern SCLog gLog;

  app.cpp에서 실제 전역변수 선언.

  SCLog gLog;

	//만약 로그파일의 경로를 특정하고자 하면 아래와 같이 Init()하면 해당 폴더아래 Log 폴더에 로그파일 생성.
	//로그파일명 형식은 projectName_yyyymmdd.log
	gLog.Init(_T("../../custom log folder/folder1/folder2"));

	//어떤 프로젝트의 경우에는 함수명이 노출되서는 안되는 경우도 있다. 함수명과 라인번호 표시 유무를 옵션처리.
	show_function_name(false);
	show_line_number(false);

	//경로를 별도로 지정하지 않고 아래와 같이 바로 사용하면 exe 파일 아래의 Log 폴더에 로그 파일 자동 생성됨.
	logWrite(_T("log test = %d, %s, %s"), 123, _T("abc"), _T("한글  테스트"));

	//SCLOG_LEVEL_WARN 등과 같이 로그레벨을 지정하여 특정 로그들만 기록되게 할 수 있는데 아직 미구현.

	//도중에 로그파일의 위치를 변경해도 적용되는지 확인 필요함.
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

	//폴더 및 로그파일 타이틀을 주지 않으면 실행파일 하위의 "Log" 폴더에 실행파일명[YYYYMMDD].log 파일이 자동 생성됨.
	//위와 같이 기본 위치와 파일명을 사용할 경우는 Init()을 호출하지 않아도 자동 생성되며
	//특정 위치에 로그파일을 저장하고자 하는 경우는 폴더를 지정해주면 됨.
	//예)특정 폴더 지정 : "d:\\test\\Log"
	//예)상대 경로 지정 : "..\\..\\Log" (실행파일이 있는 상위의 상위 폴더에 Log라는 폴더를 생성하여 로그파일 저장)
	//gLog.Init(_T("../../custom log folder"));와 같이 호출
	bool		init(CString logFolder = _T(""), CString filetitle = _T(""), int showLogLevel = SCLOG_LEVEL_RELEASE);

	//함수명 표시 유무. default show
	void		show_function_name(bool show) { m_show_function_name = show; }

	//라인번호 표시 유무. default show
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