#ifndef __INC_CRASHDOCTOR_H__
#define __INC_CRASHDOCTOR_H__

/*by sdh (Koino)
- ������Ʈ�� CrashHandler.cpp, h �� ���� �߰�.
- app.cpp�� DisableSetUnhandledExceptionFilter() �Լ� �߰� �� ȣ��.
- �������� ��ο� .pdb ������ �����ؾ� �ϰ�
crash�� �߻��ϸ� CrashHandler202303.log�� ���� �α������� �ڵ� ������.
*/

#define USE_CRASH_HANDLER
#ifdef USE_CRASH_HANDLER
#include <tchar.h>
#include <Windows.h>
#pragma warning(disable:4091)
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
	static CHAR							m_szSymbolPath[MAX_PATH];
	static TCHAR						m_szPrivateBuild[64];
	static TCHAR						m_szComputerName[MAX_COMPUTERNAME_LENGTH + 1];

	static TCHAR						m_szLogFileName[MAX_PATH];
	static BOOL							m_bAppendMode;
	static HANDLE						m_hReportFile;
};

#ifndef _DEBUG
extern CCrashHandler	g_CrashHandler;
#endif

#endif	// USE_CRASH_HANDLER
#endif // __INC_CRASHDOCTOR_H__
