//#include "stdafx.h"
#include "UtilLog.h"
#include <afxmt.h>

CCriticalSection theCSLog;

UtilLog* pLog = NULL;

UtilLog::UtilLog()
{
	pLog = this;

	m_showLogLevel = LOG_LEVEL_RELEASE;

	m_fp = NULL;
}

UtilLog::~UtilLog()
{
	try
	{
		Release();
	}
	catch (...)
	{

	}
}

BOOL UtilLog::Init(CString logFolder, CString filetitle, int showLogLevel)
{
	try
	{
		if (!Release())
		{
			return FALSE;
		}

		TCHAR	tExeFullPath[1024];
		CString sExeFolder;

		GetModuleFileName(AfxGetInstanceHandle(), tExeFullPath, MAX_PATH);
		sExeFolder = tExeFullPath;
		sExeFolder = sExeFolder.Left(sExeFolder.ReverseFind('\\'));

		m_filetitle = PathFindFileName(tExeFullPath);

		if (m_filetitle.Find(_T(".")) > 0)
			m_filetitle = m_filetitle.Left(m_filetitle.ReverseFind('.'));

		m_folder = sExeFolder + _T("\\Log");


		m_showLogLevel = showLogLevel;

		if (!logFolder.IsEmpty())
		{
			logFolder.Replace(_T("/"), _T("\\"));

			//절대경로로 설정한 경우
			if (logFolder.Find(_T(":\\")) > 0)
				m_folder = logFolder;
			//상대경로로 설정한 경우
			else
				m_folder.Format(_T("%s\\%s"), sExeFolder, logFolder);

			DWORD  retval = 0;
			TCHAR buffer[MAX_PATH] = _T("");

			retval = GetFullPathName(m_folder, MAX_PATH, buffer, NULL);

			if (retval == 0)
			{
				TRACE(_T("GetFullPathName() error"));
			}
			else
			{
				m_folder = buffer;
			}
		}

		if (!filetitle.IsEmpty())
			m_filetitle = filetitle;

		CTime t = CTime::GetCurrentTime();
		m_filename.Format(_T("%s[%d%02d%02d].log"), m_filetitle, t.GetYear(), t.GetMonth(), t.GetDay());
		m_fullpath.Format(_T("%s\\%s"), m_folder, m_filename);

		CreateDirectory(m_folder, NULL);

		_tfopen_s(&m_fp, m_fullpath, _T("a")CHARSET);

		if (m_fp == NULL)
		{
			AfxMessageBox(m_fullpath + _T("\nfopen failed. m_fp is null"));
			return FALSE;
		}

		return TRUE;
	}
	catch (...)
	{
		return FALSE;
	}
}


BOOL UtilLog::Release()
{
	try
	{
		if (m_fp != NULL)
		{
			fclose(m_fp);
			m_fp = NULL;
		}
		return TRUE;
	}
	catch (...)
	{
		return FALSE;
	}
}

CString UtilLog::Write(int logLevel, TCHAR* func, int line, LPCTSTR format, ...)
{
	CString result = CString();

	try
	{
		theCSLog.Lock();

		if (m_fp == NULL)
		{
			if (!Init(m_folder))
			{
				theCSLog.Unlock();
				return result;
			}
		}

		if (logLevel < m_showLogLevel)
		{
			theCSLog.Unlock();
			return result;
		}

		va_list args;
		va_start(args, format);

		CString str;
		str.FormatV(format, args);

		SYSTEMTIME t = { 0 };
		GetLocalTime(&t);

		CString funcName(func);

		if (m_fp)
		{
			if (str.IsEmpty())
			{
				_ftprintf(m_fp, _T("\n"));
			}
			else
			{
				result.Format(_T("[%d/%02d/%02d %02d:%02d:%02d(%03d)][%s(%d)] %s"),
					t.wYear, t.wMonth, t.wDay, t.wHour, t.wMinute, t.wSecond, t.wMilliseconds,
					funcName, line, str);
				_ftprintf(m_fp, _T("%s\n"), result);
			}
			fflush(m_fp);
		}

		Release();
		theCSLog.Unlock();
	}
	catch (...)
	{
		Release();
		theCSLog.Unlock();
	}

	return result;
}
