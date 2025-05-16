#pragma once

/*
* 기존 Koino에서 사용하던 UtilLog에 로그를 출력한 함수명과 라인 정보를 추가.
* (#define logWrite...매크로를 사용하므로 멀티쓰레드가 가능할지는 아직 확인 못함)
* 
* !주의!
* 일반적인 앱에서는 아주 잘 동작하지만
* LMMAgent.exe와 같이 SYSTEM session 0번에서 실행되는 프로그램에서는
* 위 매크로 때문인지 아직 명확히 알 수 없으나 crash가 발생하므로 사용 금지.
* DbgOutA()를 이용할 것!

* 멀티쓰레드에서 로그 기록에 누락이 발생하는지 간단히 테스트 완료.
  (테스트 방법. 0 ~ 999까지 10ms 간격으로 로그를 기록하는 50개 쓰레드를 생성하여 기록 후 확인함)

* app.h에서 (프로젝트 구성에 따라 App class가 아닌 stdafx.h 또는 pch.h 등에 선언할 수도 있음)
	#include "../Common/log/SCLog/SCLog.h"
	...
	전역변수로 아래 변수 선언.
	extern CSCLog gLog;

  app.cpp에서 실제 전역변수 선언.
    CSCLog gLog;

  

[set Log folder path]
	//set()함수를 통해 로그파일 저장폴더, 로그파일 타이틀, 로그 레벨을 설정할 수 있다.
	//만약 로그파일의 경로를 특정하고자 하면 아래와 같이 set()하면 해당 폴더아래 Log 폴더에 로그파일 생성.
	//로그파일명 형식은 projectName_yyyymmdd.log
	gLog.set(_T("../../custom log folder/folder1/folder2"), _T("TestLogFileName"), SCLOG_LEVEL_DEBUG);
	기본 로그 레벨이 SCLOG_LEVEL_RELEASE이므로 이 경우 logWriteD()를 호출할 경우는 로그 기록이 스킵된다.

[show or not the function name or line number]
	//어떤 프로젝트의 경우에는 함수명이 노출되서는 안되는 경우도 있다. 함수명과 라인번호 표시 유무를 옵션처리.
	show_function_name(false);
	show_line_number(false);

	//경로를 별도로 지정하지 않고 아래와 같이 바로 사용하면 exe 파일 아래의 Log 폴더에 로그 파일 자동 생성됨.
	gLog.write(_T("log test = %d, %s, %s"), 123, _T("abc"), _T("한글  테스트"));

[write blank line log]
	//간혹 특별한 로그 내용이 없어도 linefeed 또는 함수진입 기록등의 목적으로 "" 또는 " "을 넘김.
	//gLog.write(_T(""));	//한 줄 빈 라인 추가
	//gLog.write(_T(" "));	//시간, 함수정보까지만 출력되는 라인 추가
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

	//폴더 및 로그파일 타이틀을 주지 않으면 실행파일 하위의 "Log" 폴더에 실행파일명[YYYYMMDD].log 파일이 자동 생성됨.
	//위와 같이 기본 위치와 파일명을 사용할 경우는 set()을 호출하지 않아도 자동 생성되며
	//특정 위치에 로그파일을 저장하고자 하는 경우는 폴더를 지정해주면 됨.
	//예)특정 폴더 지정 : "d:\\test\\Log"
	//예)상대 경로 지정 : "..\\..\\Log" (실행파일이 있는 상위의 상위 폴더에 Log라는 폴더를 생성하여 로그파일 저장)
	//gLog.set(_T("../../custom log folder"));와 같이 호출
	//file_title을 주지 않으면 "실행파일명_yyyymmdd.log" 파일로 저장되고
	//로그파일명을 지정하고 싶다면 file_title에 값을 주면 "file_title_yyyymmdd.log"로 저장된다.
	bool		set(CString log_folder = _T(""), CString file_title = _T(""), int show_log_level = SCLOG_LEVEL_RELEASE);
	void		set_log_level(int log_level = SCLOG_LEVEL_RELEASE);

	//프로그램 시작 시 일반적인 정보를 로그에 기록하면서 시작한다.
	//시작 단락 표시, exe path, file version, built time을 기록한다.
	void		write_start_log();

	//프로그램 종료 시 일반적인 정보를 로그에 기록하면서 종료한다.
	void		write_end_log();

	//함수명 표시 유무. default show
	void		show_function_name(bool show) { m_show_function_name = show; }

	//라인번호 표시 유무. default show
	void		show_line_number(bool show) { m_show_line_number = show; }

	//logWrite 매크로를 통해서 함수명, 라인수 등을 같이 기록해주므로 편리하지만
	//LMMAgent 또는 멀티쓰레드로 동작하는 프로젝트에서는 뭔가 오동작을 유발하는 듯하여
	//단순 로그만 남기는 write 함수를 추가함.
	CString		write(LPCTSTR format, ...);
	CString		write(int log_level, TCHAR* func, int line, LPCTSTR format, ...);

	bool		release();
	CString		get_log_full_path() { return m_log_fullpath; }
	bool		recursive_make_full_directory(LPCTSTR sFolder);


	//현재 로그파일을 열어준다.
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