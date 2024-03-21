#include "SCLog.h"
#include <afxmt.h>

CCriticalSection theCSLog;

SCLog* pLog = NULL;

SCLog::SCLog()
{
	pLog = this;

	m_showLogLevel = SCLOG_LEVEL_RELEASE;

	m_fp = NULL;
}

SCLog::~SCLog()
{
	try
	{
		release();
	}
	catch (...)
	{

	}
}

bool SCLog::init(CString logFolder, CString filetitle, int showLogLevel)
{
	try
	{
		if (!release())
		{
			return false;
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
		m_filename.Format(_T("%s_%d%02d%02d.log"), m_filetitle, t.GetYear(), t.GetMonth(), t.GetDay());
		m_fullpath.Format(_T("%s\\%s"), m_folder, m_filename);

		//CreateDirectory(m_folder, NULL);
		recursive_make_full_directory(m_folder);

		_tfopen_s(&m_fp, m_fullpath, _T("a")CHARSET);

		if (m_fp == NULL)
		{
			//AfxMessageBox(m_fullpath + _T("\nfopen failed. m_fp is null"));
			return false;
		}

		return true;
	}
	catch (...)
	{
		return false;
	}
}


bool SCLog::release()
{
	try
	{
		if (m_fp != NULL)
		{
			fclose(m_fp);
			m_fp = NULL;
		}
		return true;
	}
	catch (...)
	{
		return false;
	}
}

CString SCLog::write(int logLevel, TCHAR* func, int line, LPCTSTR format, ...)
{
	CString result = CString();

	try
	{
		theCSLog.Lock();

		if (m_fp == NULL)
		{
			if (!init(m_folder))
			{
				theCSLog.Unlock();
				return result;
			}
		}

		//if (logLevel < m_showLogLevel)
		//{
		//	theCSLog.Unlock();
		//	return result;
		//}

		va_list args;
		va_start(args, format);

		CString log_text;
		CString log_level;

		log_text.FormatV(format, args);

		//만약 로그 텍스트의 맨 앞에 \n이 붙어있으면 이전 로그 라인과 라인을 구분하기 위함인데
		//그냥 기록하면 시간부터 라인번호까지 출력한 후 라인이 변경된다.
		//log_text의 맨 앞에 \n이 있다면 먼저 처리해준다.
		int i;
		int linefeed_count = 0;
		for (i = 0; i < log_text.GetLength(); i++)
		{
			if (log_text[i] == '\n')
				linefeed_count++;
			else
				break;
		}

		if (linefeed_count > 0)
			log_text = log_text.Mid(linefeed_count);

		switch (logLevel)
		{
		case SCLOG_LEVEL_INFO :
			log_level = _T("[info]");
			break;
		case SCLOG_LEVEL_WARN:
			log_level = _T("[warn]");
			break;
		case SCLOG_LEVEL_ERROR:
			log_level = _T("[error]");
			break;
		case SCLOG_LEVEL_CRITICAL:
			log_level = _T("[critical]");
			break;
		case SCLOG_LEVEL_SQL:
			log_level = _T("[sql]");
			break;
		case SCLOG_LEVEL_DEBUG:
			log_level = _T("[debug]");
			break;
		case SCLOG_LEVEL_RELEASE:		//release도 no level로 처리한다.
			//log_text = _T("info");
			break;
		}

		SYSTEMTIME t = { 0 };
		GetLocalTime(&t);

		CString func_name(func);
		CString line_str;

		line_str.Format(_T("%d"), line);

		//함수명과 라인번호를 감춰야 할 경우
		//그 값들은 감추되 차후 파싱을 위해 대괄호는 출력한다.
		if (!m_show_function_name)
			func_name.Empty();

		if (!m_show_line_number)
			line_str.Empty();

		if (m_fp)
		{
			if (linefeed_count == 0 && log_text.IsEmpty())
			{
				_ftprintf(m_fp, _T("\n"));
			}
			else
			{
				for (i = 0; i < linefeed_count; i++)
					_ftprintf(m_fp, _T("\n"));

				result.Format(_T("[%d/%02d/%02d %02d:%02d:%02d.%03d][%s][%s]%s %s"),
					t.wYear, t.wMonth, t.wDay, t.wHour, t.wMinute, t.wSecond, t.wMilliseconds,
					func_name, line_str, log_level, log_text);
				_ftprintf(m_fp, _T("%s\n"), result);
				TRACE(_T("%s\n"), result);
			}
			fflush(m_fp);
		}

		release();
		theCSLog.Unlock();
	}
	catch (...)
	{
		release();
		theCSLog.Unlock();
	}

	return result;
}

//usage : sFolder include folder names only except file name.
//c:\test\00\1.bmp	(x)	=> 1.bmp folder will be created.(not intended)
//c:\test\00		(o)
bool SCLog::recursive_make_full_directory(LPCTSTR sFolder)
{
	if (PathFileExists(sFolder) && ::PathIsDirectory(sFolder))
		return true;

	TCHAR parent[MAX_PATH] = _T("");

	_tcscpy_s(parent, sFolder);
	::PathRemoveFileSpec(parent);

	if (recursive_make_full_directory(parent))
		return (::CreateDirectory(sFolder, NULL) != false);

	return false;
}
