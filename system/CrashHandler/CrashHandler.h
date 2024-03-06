#ifndef __INC_CRASHDOCTOR_H__
#define __INC_CRASHDOCTOR_H__

/*by sdh (Koino)
- 프로젝트에 CrashHandler.cpp, h 두 파일 추가.
- app.cpp에 DisableSetUnhandledExceptionFilter() 함수 바디 추가 및 호출.
	void DisableSetUnhandledExceptionFilter()
	{
		void *addr = (void*)GetProcAddress(LoadLibrary(_T("kernel32.dll")),
			"SetUnhandledExceptionFilter");
		if (addr)
		{
			unsigned char code[16];
			int size = 0;
			code[size++] = 0x33;
			code[size++] = 0xC0;
			code[size++] = 0xC2;
			code[size++] = 0x04;
			code[size++] = 0x00;
			DWORD dwOldFlag, dwTempFlag;
			VirtualProtect(addr, size, PAGE_READWRITE, &dwOldFlag);
			WriteProcessMemory(GetCurrentProcess(), addr, code, size, NULL);
			VirtualProtect(addr, size, dwOldFlag, &dwTempFlag);
		}
	}

- 반드시 실행파일과 함께 .pdb 파일을 함께 배포해야 함!

- crash가 발생하면 CrashHandler20230302.log와 같이 로그파일이 자동 생성됨.

  by scpark
- 로그파일의 날짜형식을 yyyymmdd로 변경.
- 
*/

#pragma warning(disable:4091)
#include <afxwin.h>
#include <dbghelp.h>

class CCrashHandler
{
public:
	CCrashHandler();
	~CCrashHandler();

public:
	static void	SetLogFileName(LPCTSTR lpszLogFileName, BOOL bFileNew);
	static void	SetSymbolPath(LPCTSTR lpszSymbolPath);

private:
	static LONG WINAPI MyCrashHandler(PEXCEPTION_POINTERS pExceptionInfo);

private:
	void	SetDefaultLogFileName();

	static void ReportSystemError();

	static void GenerateExceptionReport(PEXCEPTION_POINTERS pExceptionInfo);
	static void PrintBanner();
	static void PrintExceptionCodeAndModule(PEXCEPTION_RECORD pExceptionRecord);
	static void PrintCallStack(PEXCEPTION_RECORD pExceptionRecord, PCONTEXT pContextRecord);
	static void PrintSourceLine(HANDLE hHandle, PEXCEPTION_RECORD pExceptionRecord);
	static void PrintRegisterDump(PCONTEXT pContextRecord);

	static LPTSTR	GetExceptionString(DWORD dwCode);
	static BOOL		GetLogicalAddress(PVOID	 pLinearAddress,
									  PTSTR	 pszModule,
									  DWORD	 dwModuleNameLen,
									  DWORD& dwSection,
									  DWORD& dwOffset		);
	static void		IntelStackWalk(PCONTEXT pContext);
	static void		DetailStackWalk(PCONTEXT pContext);

	static BOOL		InitializeSymbols(HANDLE hHandle);
	static void		ImageHelpStackWalk(PCONTEXT pContext);

	static int __cdecl PrintLog(const TCHAR * format, ...);

public:
	static BOOL	GetPrivateBuildString(LPCTSTR lpszModuleFileName, LPTSTR lpszPrivateBuild, DWORD dwLen);	

private:
	static LPTOP_LEVEL_EXCEPTION_FILTER	m_previousFilter;

	static BOOL							m_bLoggingNow;

	static TCHAR						m_szProgramFileName[MAX_PATH];
	static TCHAR						m_szProgramFolder[MAX_PATH];
	static TCHAR						m_szSymbolPath[MAX_PATH];
	static TCHAR						m_szPrivateBuild[64];
	static TCHAR						m_szComputerName[MAX_COMPUTERNAME_LENGTH + 1];

	static TCHAR						m_szLogFileName[MAX_PATH];
	static BOOL							m_bAppendMode;
	static HANDLE						m_hReportFile;
};

#ifndef _DEBUG
extern CCrashHandler	g_CrashHandler;
#endif

#endif // __INC_CRASHDOCTOR_H__
