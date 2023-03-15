#include "stdafx.h"

#include "CrashHandler.h"
#pragma warning(disable:4312)

#pragma comment(lib, "dbghelp.lib")
#pragma comment(lib, "Version.lib")

#define	_CD_LOG_SILENTLY

#ifndef _CD_LOG_SILENTLY
#define	_CD_MESSAGE_TEXT  _T("치명적 에러가 발생하여 강제 종료합니다.\n에러 발생 상황을 로그파일로 출력합니까?")
#define	_CD_MESSAGE_TITLE _T("치명적 에러가 발생하였습니다!")

#define	_CD_MESSAGE_LOGOUT_TEXT  _T("에러 발생 상황을 이하의 로그파일\n '%s'\n에 기록하였습니다.\n\n강제 종료합니다.")
#define	_CD_MESSAGE_LOGOUT_TITLE _T("에러 파일 작성 완료")
#endif

#define	_CD_SYMBOL_PATH	  "%SYSTEMROOT%;.\\Symbols\\;..\\Symbols\\;..\\..\\Symbols\\;..\\..\\..\\Symbols\\;.\\"

TCHAR	CCrashHandler::m_szProgramFileName[MAX_PATH]					= { 0 };
TCHAR	CCrashHandler::m_szProgramFolder[MAX_PATH]					= { 0 };
TCHAR	CCrashHandler::m_szLogFileName[MAX_PATH]						= { 0 };
TCHAR	CCrashHandler::m_szPrivateBuild[64]							= { 0 };
TCHAR	CCrashHandler::m_szComputerName[MAX_COMPUTERNAME_LENGTH + 1]	= { 0 };
CHAR	CCrashHandler::m_szSymbolPath[MAX_PATH]						= { 0 };
BOOL	CCrashHandler::m_bAppendMode									= TRUE;
HANDLE	CCrashHandler::m_hReportFile									= INVALID_HANDLE_VALUE;

LPTOP_LEVEL_EXCEPTION_FILTER CCrashHandler::m_previousFilter			= NULL;

BOOL	CCrashHandler::m_bLoggingNow									= FALSE;

//#ifndef _DEBUG
CCrashHandler	g_CrashHandler;
//#endif

CCrashHandler::CCrashHandler()
{
	m_previousFilter = ::SetUnhandledExceptionFilter(MyCrashHandler);

	::GetModuleFileName(NULL, m_szProgramFileName, MAX_PATH);

	TCHAR szDrive[_MAX_DRIVE];
	TCHAR szDir[_MAX_DIR];

	_tsplitpath(m_szProgramFileName, szDrive, szDir, NULL, NULL);

	_tcscpy(m_szProgramFolder, szDrive);
	_tcscat(m_szProgramFolder, szDir);

	DWORD dwMaxNameLen = sizeof(m_szComputerName) / sizeof(TCHAR);
	::GetComputerName(m_szComputerName, &dwMaxNameLen);

	GetPrivateBuildString(m_szProgramFileName,
						  m_szPrivateBuild,
						  sizeof(m_szPrivateBuild) / sizeof(TCHAR));

	ExpandEnvironmentStringsA(_CD_SYMBOL_PATH, m_szSymbolPath, sizeof(m_szSymbolPath));

	SetDefaultLogFileName();
}

CCrashHandler::~CCrashHandler()
{
	::SetUnhandledExceptionFilter(m_previousFilter);
}

void CCrashHandler::SetLogFileName(LPCTSTR lpszLogFileName, BOOL bFileNew)
{
	memset(m_szLogFileName, 0, MAX_PATH);
	_tcsncpy(m_szLogFileName, lpszLogFileName, MAX_PATH-1);

	m_bAppendMode = bFileNew ? FALSE : TRUE;
}

void CCrashHandler::SetSymbolPath(LPCTSTR lpszSymbolPath)
{
	_snprintf(m_szSymbolPath, sizeof(m_szSymbolPath), "%S", lpszSymbolPath);
}

void CCrashHandler::SetDefaultLogFileName()
{
	TCHAR szTempFolder[_MAX_PATH - 64];
	LPTSTR	lpszFound = NULL;
	memset(szTempFolder, 0, _MAX_PATH - 64);

	if (0 != ::GetModuleFileName(NULL, szTempFolder, _MAX_PATH - 65))
	{
		lpszFound = _tcsrchr(szTempFolder, TEXT('\\'));
		if (NULL != lpszFound)
		{
			*lpszFound = TEXT('\0');
			_tcscat(szTempFolder, _T("\\"));
		}
	}

	if (lpszFound == NULL)
	{
		if (::GetTempPath(_MAX_PATH - 65, szTempFolder) == 0)
		{
			_tcsncpy(szTempFolder, m_szProgramFolder, _MAX_PATH - 65);
		}
		else
		{
			TCHAR szLongPath[_MAX_PATH];

			if (::GetLongPathName(szTempFolder, szLongPath, _MAX_PATH - 65) == 0)
				_tcsncpy(szTempFolder, m_szProgramFolder, _MAX_PATH - 65);
			else
				_tcsncpy(szTempFolder, szLongPath, _MAX_PATH - 65);
		}
	}

	memset(m_szLogFileName, 0, MAX_PATH);
	_tcsncpy(m_szLogFileName, szTempFolder, _MAX_PATH - 64);

	SYSTEMTIME stm;
	::GetLocalTime(&stm);

	TCHAR szFileName[64];
	wsprintf(szFileName, _T("CrashHandler%d%02d.log"), stm.wYear, stm.wMonth);

	_tcscat(m_szLogFileName, szFileName);
}

LONG WINAPI CCrashHandler::MyCrashHandler(PEXCEPTION_POINTERS pExceptionInfo)
{
	if(m_bLoggingNow)
	{
		if(m_previousFilter != NULL)
			return m_previousFilter(pExceptionInfo);
		else
			return EXCEPTION_CONTINUE_SEARCH;
	}

	m_bLoggingNow = TRUE;

	if(pExceptionInfo != NULL)
	{
		if(_tcsclen(m_szLogFileName) > 0)
		{
			int nAnswer = IDYES;

		#ifndef _CD_LOG_SILENTLY
			nAnswer = ::MessageBox(	NULL,
									_CD_MESSAGE_TEXT,
									_CD_MESSAGE_TITLE,
									MB_ICONERROR|MB_SYSTEMMODAL|MB_YESNO );
		#endif

			if(nAnswer == IDYES)
			{
				::SetCurrentDirectory(m_szProgramFolder);

				DWORD dwCreationDisposition = m_bAppendMode ? OPEN_ALWAYS : CREATE_ALWAYS;

				m_hReportFile = ::CreateFile(m_szLogFileName,
											 GENERIC_WRITE,
											 0,
											 0,
											 dwCreationDisposition,
											 FILE_FLAG_WRITE_THROUGH,
											 0						 );

				if(m_hReportFile != INVALID_HANDLE_VALUE)
				{
					if(m_bAppendMode)
						::SetFilePointer(m_hReportFile, 0, 0, FILE_END);

					GenerateExceptionReport(pExceptionInfo);

					::CloseHandle(m_hReportFile);
					m_hReportFile = INVALID_HANDLE_VALUE;

				#ifndef _CD_LOG_SILENTLY
					TCHAR szMessage[1024];
					wsprintf(szMessage, _CD_MESSAGE_LOGOUT_TEXT, m_szLogFileName);

					::MessageBox(NULL,
								 szMessage,
								 _CD_MESSAGE_LOGOUT_TITLE,
								 MB_ICONINFORMATION|MB_SYSTEMMODAL|MB_OK);
				#endif
				}
				else
				{
					ReportSystemError();
				}
			}
		}
	}

	m_bLoggingNow = FALSE;

	if(m_previousFilter != NULL)
		return m_previousFilter(pExceptionInfo);
	else
		return EXCEPTION_CONTINUE_SEARCH;
}

void CCrashHandler::ReportSystemError()
{
#ifndef _CD_LOG_SILENTLY
	DWORD dwLastError = ::GetLastError();

	LPVOID lpMsgBuf = NULL;
	DWORD  dwFlags  = FORMAT_MESSAGE_ALLOCATE_BUFFER |
					  FORMAT_MESSAGE_FROM_SYSTEM	 |
					  FORMAT_MESSAGE_IGNORE_INSERTS		;

	if(::FormatMessage(dwFlags,
					NULL,
					dwLastError,
					MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
					(LPTSTR) &lpMsgBuf,
					0,
					NULL										))
	{
		::MessageBox(NULL, (LPCTSTR)lpMsgBuf, _T("Cannot open logfile!"), MB_OK|MB_ICONERROR);
	}

	if(lpMsgBuf != NULL)
		::LocalFree(lpMsgBuf);
#endif
}

void CCrashHandler::GenerateExceptionReport(PEXCEPTION_POINTERS pExceptionInfo)
{
	PEXCEPTION_RECORD pExceptionRecord = pExceptionInfo->ExceptionRecord;

	PrintBanner();

	if(pExceptionRecord != NULL)
	{
		PrintExceptionCodeAndModule(pExceptionRecord);

		PCONTEXT pContextRecord = pExceptionInfo->ContextRecord;

		if(pContextRecord != NULL)
		{
			PrintCallStack(pExceptionRecord, pContextRecord);

			//PrintRegisterDump(pContextRecord);
		}
	}
	PrintLog(_T("\r\n\r\n"));
}

void CCrashHandler::PrintBanner()
{
	SYSTEMTIME tm;
	::GetLocalTime(&tm);

	MEMORYSTATUSEX	mst;
	::ZeroMemory( &mst, sizeof( mst ) );
	mst.dwLength = sizeof( mst );
	::GlobalMemoryStatusEx( &mst );

	PrintLog(_T("=============================================================================\r\n"));
	PrintLog(_T("  Application Crash Report --- %d/%02d/%02d %02d:%02d:%02d.%03d at %s\r\n"),
				tm.wYear, tm.wMonth, tm.wDay, tm.wHour, tm.wMinute, tm.wSecond, tm.wMilliseconds, m_szComputerName);
	PrintLog(_T("  Build Version: %s\r\n"), m_szPrivateBuild);
	PrintLog(_T("  Available Virtual Memory: %I64dKB\r\n"), mst.ullAvailVirtual / 1024LL );
	PrintLog(_T("=============================================================================\r\n"));
}

BOOL CCrashHandler::GetPrivateBuildString(LPCTSTR lpszModuleFileName,
										 LPTSTR	 lpszPrivateBuild,
										 DWORD	 dwLen				)
{
	if(dwLen < 1)
		return FALSE;

	*lpszPrivateBuild = 0;

	DWORD dwUseless;
	DWORD dwInfoSize = ::GetFileVersionInfoSize((LPTSTR)lpszModuleFileName,
												&dwUseless					);
	if(dwInfoSize == 0)
		return FALSE;

	LPVOID pVersionInfo = malloc(dwInfoSize);
	if(pVersionInfo == NULL)	return FALSE;

	if(::GetFileVersionInfo((LPTSTR)lpszModuleFileName,
							0,
							dwInfoSize,
							pVersionInfo				))
	{
		LPWORD	pVersion = NULL;
		UINT	unVersionLen;
		TCHAR	szVerStrName[256];

		if(::VerQueryValue(	pVersionInfo,
							_T("\\VarFileInfo\\Translation"),
							(LPVOID*)&pVersion,
							&unVersionLen					))
		{
			wsprintf(szVerStrName, _T("\\StringFileInfo\\%04x%04x\\%s"),
					 pVersion[0],
					 pVersion[1],
					 _T("PrivateBuild")									);

			LPTSTR	lpszBuffer = NULL;
		    DWORD	dwBufSize;

			if(::VerQueryValue(	pVersionInfo,
								szVerStrName,
								(LPVOID*)&lpszBuffer,
								(UINT*)&dwBufSize	))
			{
				_tcsncpy(lpszPrivateBuild, lpszBuffer, dwLen - 1);
				lpszPrivateBuild[dwLen - 1] = 0;
			}
		}
	}

	free(pVersionInfo);

	return TRUE;
}

void CCrashHandler::PrintExceptionCodeAndModule(PEXCEPTION_RECORD pExceptionRecord)
{
	PrintLog(_T("Exception code: %08X %s\r\n"),
					pExceptionRecord->ExceptionCode,
					GetExceptionString(pExceptionRecord->ExceptionCode));

	TCHAR szFaultingModule[MAX_PATH] = _T("");
	DWORD dwSection			 = 0;
	DWORD dwOffset			 = 0;

	GetLogicalAddress(pExceptionRecord->ExceptionAddress,
					  szFaultingModule,
					  sizeof(szFaultingModule) /sizeof(TCHAR),
					  dwSection,
					  dwOffset							 );

	PrintLog(_T("Fault address : %08X %02X:%08X\r\n"),
							pExceptionRecord->ExceptionAddress,
							dwSection,
							dwOffset							);

	if (pExceptionRecord->ExceptionCode == EXCEPTION_ACCESS_VIOLATION) {

		PrintLog(_T("Access address: %08X(%s)\r\n"),
			pExceptionRecord->ExceptionInformation[1],
			pExceptionRecord->ExceptionInformation[0] ? _T("Write") : _T("Read"));
	}

	PrintLog(_T("Module        : %s\r\n"), szFaultingModule);

	DWORD dwThreadID = ::GetCurrentThreadId();
	PrintLog(_T("Current thread: %08X(%d)\r\n"), dwThreadID, dwThreadID);
}

void CCrashHandler::PrintRegisterDump(PCONTEXT pContextRecord)
{
#ifdef _M_IX86  // Intel Only!
	PrintLog(_T("\r\n"));
	PrintLog(_T("Registers     : EAX: %08X\r\n"), pContextRecord->Eax);
	PrintLog(_T("                EBX: %08X\r\n"), pContextRecord->Ebx);
	PrintLog(_T("                ECX: %08X\r\n"), pContextRecord->Ecx);
	PrintLog(_T("                EDX: %08X\r\n"), pContextRecord->Edx);
	PrintLog(_T("                ESI: %08X\r\n"), pContextRecord->Esi);
	PrintLog(_T("                EDI: %08X\r\n"), pContextRecord->Edi);
	PrintLog(_T("                EBP: %08X\r\n"), pContextRecord->Ebp);
	PrintLog(_T("                DS : %04X\r\n"), pContextRecord->SegDs);
	PrintLog(_T("                ES : %04X\r\n"), pContextRecord->SegEs);
	PrintLog(_T("                FS : %04X\r\n"), pContextRecord->SegFs);
	PrintLog(_T("                GS : %04X\r\n"), pContextRecord->SegGs);
	PrintLog(_T("                CS:EIP: %04X:%08X\r\n"), pContextRecord->SegCs, pContextRecord->Eip);
	PrintLog(_T("                SS:ESP: %04X:%08X\r\n"), pContextRecord->SegSs, pContextRecord->Esp);
	PrintLog(_T("                Flags : %08X\r\n"), pContextRecord->EFlags);
#endif
}

void CCrashHandler::PrintCallStack(PEXCEPTION_RECORD pExceptionRecord, PCONTEXT pContextRecord)
{
	HANDLE hCurrentProcess = GetCurrentProcess();
	BOOL bSymbolLoaded = FALSE;

	if (pExceptionRecord->ExceptionCode != EXCEPTION_STACK_OVERFLOW) {
		bSymbolLoaded = InitializeSymbols(hCurrentProcess);
	}

//	if (bSymbolLoaded)
		PrintSourceLine(hCurrentProcess, pExceptionRecord);

	PrintLog(_T("\r\n"));
	PrintLog(_T("-----------------------------------------------------------------------------\r\n"));
	PrintLog(_T("Call stack    :\r\n"));
	PrintLog(_T("-----------------------------------------------------------------------------\r\n"));

	DWORD dwEip = pContextRecord->Eip;
	DWORD dwEsp = pContextRecord->Esp;
	DWORD dwEbp = pContextRecord->Ebp;
	ImageHelpStackWalk(pContextRecord);
	pContextRecord->Eip = dwEip;
	pContextRecord->Esp = dwEsp;
	pContextRecord->Ebp = dwEbp;

	SymCleanup(hCurrentProcess);

	PrintLog(_T("-----------------------------------------------------------------------------\r\n"));
}

void CCrashHandler::PrintSourceLine(HANDLE hHandle, PEXCEPTION_RECORD pExceptionRecord)
{
	DWORD dwExceptionAddress = (DWORD)pExceptionRecord->ExceptionAddress;

	BYTE symbolBuffer[sizeof(IMAGEHLP_SYMBOL) + 512];

	PIMAGEHLP_SYMBOL pSymbol = (PIMAGEHLP_SYMBOL)symbolBuffer;
	pSymbol->SizeOfStruct  = sizeof(symbolBuffer);
	pSymbol->MaxNameLength = 512;

	DWORD dwFnDisplacement = 0;

	if(SymGetSymFromAddr(hHandle,
						  dwExceptionAddress,
						  &dwFnDisplacement,
						  pSymbol			))
	{
		PrintLog(_T("Function      : %hs\r\n"), pSymbol->Name);

		DWORD dwSrcDisplacement = 0;

		IMAGEHLP_LINE srcLineInfo;

		::ZeroMemory(&srcLineInfo, sizeof(IMAGEHLP_LINE));
		srcLineInfo.SizeOfStruct = sizeof(IMAGEHLP_LINE);

		if(SymGetLineFromAddr(	hHandle,
								dwExceptionAddress,
								&dwSrcDisplacement,
								&srcLineInfo		))
		{
			PrintLog(_T("Source file   : %hs Line:%d\r\n"),
						srcLineInfo.FileName, srcLineInfo.LineNumber);
		}
	}
}

LPTSTR CCrashHandler::GetExceptionString( DWORD dwCode )
{
	#define EXCEPTION( x ) case EXCEPTION_##x: return _T(#x);

	switch(dwCode)
	{
		EXCEPTION( ACCESS_VIOLATION )
		EXCEPTION( DATATYPE_MISALIGNMENT )
		EXCEPTION( BREAKPOINT )
		EXCEPTION( SINGLE_STEP )
		EXCEPTION( ARRAY_BOUNDS_EXCEEDED )
		EXCEPTION( FLT_DENORMAL_OPERAND )
		EXCEPTION( FLT_DIVIDE_BY_ZERO )
		EXCEPTION( FLT_INEXACT_RESULT )
		EXCEPTION( FLT_INVALID_OPERATION )
		EXCEPTION( FLT_OVERFLOW )
		EXCEPTION( FLT_STACK_CHECK )
		EXCEPTION( FLT_UNDERFLOW )
		EXCEPTION( INT_DIVIDE_BY_ZERO )
		EXCEPTION( INT_OVERFLOW )
		EXCEPTION( PRIV_INSTRUCTION )
		EXCEPTION( IN_PAGE_ERROR )
		EXCEPTION( ILLEGAL_INSTRUCTION )
		EXCEPTION( NONCONTINUABLE_EXCEPTION )
		EXCEPTION( STACK_OVERFLOW )
		EXCEPTION( INVALID_DISPOSITION )
		EXCEPTION( GUARD_PAGE )
		EXCEPTION( INVALID_HANDLE )
	}

	static TCHAR szBuffer[512] = { 0 };

	HMODULE hModule = ::GetModuleHandle(_T("NTDLL.DLL"));

	if(hModule != NULL)
	{
		::FormatMessage(FORMAT_MESSAGE_IGNORE_INSERTS|FORMAT_MESSAGE_FROM_HMODULE,
						hModule,
						dwCode,
						0,
						szBuffer,
						sizeof(szBuffer),
						0															);
	}
	else
	{
		_tcscpy(szBuffer, _T("Unknown Exception"));
	}

	return szBuffer;
}

BOOL CCrashHandler::GetLogicalAddress(PVOID	pLinearAddress,
									 PTSTR	pszModule,
									 DWORD	dwModuleNameLen,
									 DWORD&	dwSection,
									 DWORD&	dwOffset		)
{
	MEMORY_BASIC_INFORMATION mbi;

	if(!::VirtualQuery(pLinearAddress, &mbi, sizeof(mbi)))
		return FALSE;

	DWORD hModule = (DWORD)mbi.AllocationBase;

	if (IsBadReadPtr((CONST VOID *)hModule, sizeof(IMAGE_DOS_HEADER))) {
		_tcsncpy(pszModule, _T("Unknown"), dwModuleNameLen);
		dwSection = 0;
		dwOffset = 0;
		return FALSE;
	}

	if(!::GetModuleFileName((HMODULE)hModule, pszModule, dwModuleNameLen))
		return FALSE;

	PIMAGE_DOS_HEADER pDosHdr = (PIMAGE_DOS_HEADER)hModule;
	PIMAGE_NT_HEADERS pNtHdr  = (PIMAGE_NT_HEADERS)(hModule + pDosHdr->e_lfanew);

	PIMAGE_SECTION_HEADER pSection = IMAGE_FIRST_SECTION(pNtHdr);

	DWORD dwRVA = (DWORD)pLinearAddress - hModule;

	for(UINT i = 0; i < pNtHdr->FileHeader.NumberOfSections; i++, pSection++)
	{
		DWORD dwSectionStart = pSection->VirtualAddress;
		DWORD dwSectionEnd   = dwSectionStart +
								max(pSection->SizeOfRawData, pSection->Misc.VirtualSize);

		if((dwRVA >= dwSectionStart) && (dwRVA <= dwSectionEnd))
		{
			dwSection = i + 1;
			dwOffset  = dwRVA - dwSectionStart;
			return TRUE;
		}
	}

	dwSection = 0;
	dwOffset  = 0;
	return FALSE;
}

void CCrashHandler::IntelStackWalk(PCONTEXT pContext)
{
#ifdef _M_IX86  // Intel Only!

	PrintLog( _T("Address   Frame     Logical addr  Module\r\n") );

	DWORD  pc = pContext->Eip;
	PDWORD pFrame, pPrevFrame;

	pFrame = (PDWORD)pContext->Ebp;

	do
	{
		TCHAR szModule[MAX_PATH] = _T("");
		DWORD dwSection			 = 0;
		DWORD dwOffset			 = 0;

		if(!GetLogicalAddress((PVOID)pc,
							 szModule,
							 sizeof(szModule) /sizeof(TCHAR),
							 dwSection,
							 dwOffset			))
		{
			break;
		}

		PrintLog(_T("%08X  %08X  %04X:%08X %s\r\n"),
					pc, pFrame, dwSection, dwOffset, szModule);

		pc = pFrame[1];

		pPrevFrame = pFrame;

		pFrame = (PDWORD)pFrame[0];

		if((DWORD)pFrame & 3)
			break;

		if(pFrame <= pPrevFrame)
			break;

		if(::IsBadWritePtr(pFrame, sizeof(PVOID) * 2))
			break;
	} while(1);

#endif
}

void CCrashHandler::ImageHelpStackWalk(PCONTEXT pContext)
{
	PrintLog(_T("Address \r\n"));

	STACKFRAME sf;
	memset(&sf, 0, sizeof(sf));

	sf.AddrPC.Offset	= pContext->Eip;
	sf.AddrPC.Mode		= AddrModeFlat;
	sf.AddrStack.Offset	= pContext->Esp;
	sf.AddrStack.Mode	= AddrModeFlat;
	sf.AddrFrame.Offset	= pContext->Ebp;
	sf.AddrFrame.Mode	= AddrModeFlat;

	HANDLE hCurrentProcess = ::GetCurrentProcess();
	HANDLE hCurrentThread  = ::GetCurrentThread();

	while(1)
	{
		if(!StackWalk(	IMAGE_FILE_MACHINE_I386,
						hCurrentProcess,
						hCurrentThread,
						&sf,
						pContext,
						0,
						SymFunctionTableAccess,
						SymGetModuleBase,
						0						))
		{
			break;
		}

		if(sf.AddrFrame.Offset == 0)
			break;

		PrintLog(_T("%08X  "), sf.AddrPC.Offset);

		BYTE symbolBuffer[sizeof(IMAGEHLP_SYMBOL) + 512];

		PIMAGEHLP_SYMBOL pSymbol = (PIMAGEHLP_SYMBOL)symbolBuffer;
		pSymbol->SizeOfStruct  = sizeof(symbolBuffer);
		pSymbol->MaxNameLength = 512;

		DWORD dwFnDisplacement = 0;

		if(SymGetSymFromAddr(hCurrentProcess,
							  sf.AddrPC.Offset,
							  &dwFnDisplacement,
							  pSymbol			))
		{
			TCHAR szModule[MAX_PATH] = _T("");
			DWORD dwSection = 0;
			DWORD dwOffset  = 0;

			GetLogicalAddress((PVOID)sf.AddrPC.Offset,
							  szModule,
							  sizeof(szModule) / sizeof(TCHAR),
							  dwSection,
							  dwOffset				 );

			PrintLog(_T("%hs+%X(%04X:%08X %s)\r\n"), pSymbol->Name, dwFnDisplacement, dwSection, dwOffset, szModule);
		}
		else
		{
			TCHAR szModule[MAX_PATH] = _T("");
			DWORD dwSection = 0;
			DWORD dwOffset  = 0;

			GetLogicalAddress((PVOID)sf.AddrPC.Offset,
							  szModule,
							  sizeof(szModule) / sizeof(TCHAR),
							  dwSection,
							  dwOffset				 );

			PrintLog(_T("%04X:%08X %s\r\n"), dwSection, dwOffset, szModule);
		}
	}
}

#define	PAGEEXECUTEMASK	(PAGE_EXECUTE | PAGE_EXECUTE_READ | PAGE_EXECUTE_READWRITE | PAGE_EXECUTE_WRITECOPY)

void CCrashHandler::DetailStackWalk(PCONTEXT pContext)
{
	PrintLog(_T("Address \r\n"));

	STACKFRAME sf;
	memset(&sf, 0, sizeof(sf));

	sf.AddrPC.Offset	= pContext->Eip;
	sf.AddrPC.Mode		= AddrModeFlat;
	sf.AddrStack.Offset	= pContext->Esp;
	sf.AddrStack.Mode	= AddrModeFlat;
	sf.AddrFrame.Offset	= pContext->Ebp;
	sf.AddrFrame.Mode	= AddrModeFlat;

	HANDLE hCurrentProcess = ::GetCurrentProcess();
	HANDLE hCurrentThread  = ::GetCurrentThread();

	DWORD *esp;
	
	for(esp = (DWORD *)pContext->Esp; !::IsBadWritePtr(esp, sizeof(*esp)); esp++) {

		MEMORY_BASIC_INFORMATION mbi;

		if (!::VirtualQuery((LPCVOID)*esp, &mbi, sizeof(mbi))) {
			continue;
		}

		if ((mbi.Protect & PAGEEXECUTEMASK) == 0 || mbi.Type != MEM_IMAGE || mbi.AllocationBase == 0) {
			continue;
		}

		if ((DWORD)esp > sf.AddrFrame.Offset) {
			if (StackWalk(	IMAGE_FILE_MACHINE_I386,
							hCurrentProcess,
							hCurrentThread,
							&sf,
							pContext,
							0,
							SymFunctionTableAccess,
							SymGetModuleBase,
							0						))
			{
				//PrintLog(_T("*%08X(%02X)  ESP=%08X  eip=%08X, ebp=%08X, esp=%08X  "), *esp, mbi.Protect, esp, pContext->Eip, pContext->Ebp, pContext->Esp);
				PrintLog(_T("*%08X  "), *esp);
			}
			else
			{
				break;
			}
		}
		else {
			PrintLog(_T(" %08X  "), *esp);
		}

		BYTE symbolBuffer[sizeof(IMAGEHLP_SYMBOL) + 512];

		PIMAGEHLP_SYMBOL pSymbol = (PIMAGEHLP_SYMBOL)symbolBuffer;
		pSymbol->SizeOfStruct  = sizeof(symbolBuffer);
		pSymbol->MaxNameLength = 512;

		DWORD dwFnDisplacement = 0;

		if(SymGetSymFromAddr(hCurrentProcess,
							  *esp,
							  &dwFnDisplacement,
							  pSymbol			))
		{
			PrintLog(_T("%hs+%X\r\n"), pSymbol->Name, dwFnDisplacement);
		}
		else
		{
			TCHAR szModule[MAX_PATH] = _T("");
			DWORD dwSection = 0;
			DWORD dwOffset  = 0;

			GetLogicalAddress((PVOID)*esp,
							  szModule,
							  sizeof(szModule) / sizeof(TCHAR),
							  dwSection,
							  dwOffset				 );

			PrintLog(_T("%04X:%08X %s\r\n"), dwSection, dwOffset, szModule);
		}
	}
}

int __cdecl CCrashHandler::PrintLog(const TCHAR * format, ...)
{
	if(m_hReportFile != INVALID_HANDLE_VALUE)
	{
		static TCHAR szBuff[1024];
		int retValue;
		DWORD cbWritten;
		va_list argptr;

		va_start( argptr, format );
		retValue = wvsprintf( szBuff, format, argptr );
		va_end( argptr );

	#ifdef _UNICODE
		static char szCharBuff[2048];

		int nChars = WideCharToMultiByte(
			CP_THREAD_ACP,
			0,
			szBuff,
			retValue,
			szCharBuff,
			sizeof(szCharBuff),
			NULL, NULL);

		WriteFile( m_hReportFile, szCharBuff, nChars, &cbWritten, 0 );
	#else
		WriteFile( m_hReportFile, szBuff, retValue * sizeof(TCHAR), &cbWritten, 0 );
	#endif

		return retValue;
	}
	return 0;
}


BOOL CCrashHandler::InitializeSymbols(HANDLE hHandle)
{
	SymCleanup( hHandle );

	SymInitialize(hHandle, m_szSymbolPath, TRUE);

	return TRUE;
}

//#endif // _DEBUG
