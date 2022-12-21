//#include "stdafx.h"
#include "UtilLog.h"
#include <afxmt.h>

CCriticalSection theCSLog;

UtilLog* pLog = NULL;

UtilLog::UtilLog()
{
	pLog = this;
	m_logFileName = _T("");
	m_filePath = _T("");
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

BOOL UtilLog::Init(CString filename, int showLogLevel)
{
	try
	{
		if (!Release())
		{
			return FALSE;
		}

		m_showLogLevel = showLogLevel;
		m_logFileName = filename;

		if (m_filePath == _T(""))
		{
			m_filePath = GetLogFilePath();
			if (m_filePath == _T(""))
			{
				return FALSE;
			}
		}

		if (m_logFileName.IsEmpty())
			m_logFileName = m_processTitle;

		//m_fp = _fsopen(UtilPublic::unicodeToMultibyte((LPCTSTR)m_filePath).c_str(), "a+", _SH_DENYNO);
		_tfopen_s(&m_fp, m_filePath, _T("a+")CHARSET);

		if (m_fp == NULL)
		{
			return FALSE;
		}
		return TRUE;
		/*
		errno_t err = fopen_s(&m_fp, UtilPublic::unicodeToMultibyte((LPCTSTR)GetLogFilePath()).c_str(), "a+");
		if (err != 0)
		{
		return TRUE;
		}
		return FALSE;
		*/
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
			if (!Init())
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

		CTime t = CTime::GetCurrentTime();

		CString funcName(func);

		if (m_fp)
		{
			if (str.IsEmpty())
			{
				_ftprintf(m_fp, _T("\n"));
			}
			else
			{
				result.Format(_T("[%d/%02d/%02d %02d:%02d:%02d][%s(%d)] %s"),
					t.GetYear(), t.GetMonth(), t.GetDay(), t.GetHour(), t.GetMinute(), t.GetSecond(),
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

CString UtilLog::GetLogFilePath()
{
	try
	{
		CString strFolderPath = GetLogFileFolder();
		if (strFolderPath == _T(""))
		{
			return _T("");
		}

		SYSTEMTIME current_time = { 0, };
		GetLocalTime(&current_time);

		if (m_logFileName.IsEmpty())
			m_logFileName = m_processTitle;

		CString filePath = _T("");
		filePath.Format(_T("%s%s[%.4ld%.2ld%.2ld].log"), strFolderPath, m_logFileName, current_time.wYear, current_time.wMonth, current_time.wDay);
		return filePath;
	}
	catch (...)
	{
		return _T("");
	}
}

CString UtilLog::GetLogFileFolder()
{
	try
	{
		TCHAR	sFilePath[1024];

		GetModuleFileName(AfxGetInstanceHandle(), sFilePath, MAX_PATH);

		m_processTitle = PathFindFileName(sFilePath);

		if (m_processTitle.Find(_T(".")) > 0)
			m_processTitle = m_processTitle.Left(m_processTitle.ReverseFind('.'));

		CString folder = sFilePath;
		folder.Format(_T("%s\\Log\\"), folder.Left(folder.ReverseFind('\\')));
		CreateDirectory(folder, NULL);
		return folder;
	}
	catch (...)
	{
		return _T("");
	}
}