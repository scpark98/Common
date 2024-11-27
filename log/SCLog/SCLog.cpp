#include "SCLog.h"
#include <afxmt.h>

#include "../../Functions.h"

//CCriticalSection theCSLog;

CSCLog* pLog = NULL;

CSCLog::CSCLog()
{
	pLog = this;

	m_showLogLevel = SCLOG_LEVEL_RELEASE;

	m_fp = NULL;
}

CSCLog::~CSCLog()
{
	try
	{
		release();
	}
	catch (...)
	{

	}
}

bool CSCLog::set(CString log_folder, CString file_title, int show_log_level)
{
	try
	{
		if (!release())
		{
			return false;
		}

		m_showLogLevel = show_log_level;

		//로그 폴더 설정
		if (log_folder.IsEmpty())
		{
			m_log_folder.Format(_T("%s\\Log"), get_exe_directory());
		}
		else
		{
			log_folder.Replace(_T("/"), _T("\\"));

			//절대경로로 설정한 경우
			if (log_folder.Find(_T(":\\")) > 0)
			{
				m_log_folder = log_folder;
			}
			//상대경로로 설정한 경우
			else
			{
				DWORD  retval = 0;
				TCHAR buffer[MAX_PATH] = _T("");

				//상대경로에 포함된 "..\\..\\"와 같은 표현을 없애고 절대경로로 변환.
				m_log_folder.Format(_T("%s\\%s"), get_exe_directory(), log_folder);
				retval = GetFullPathName(m_log_folder, MAX_PATH, buffer, NULL);
				if (retval == 0)
				{
					TRACE(_T("GetFullPathName() error"));
				}
				else
				{
					m_log_folder = CString(buffer);
				}
			}
		}

		make_full_directory(m_log_folder);


		//로그파일명 설정
		if (file_title.IsEmpty())
		{
			m_log_file_title = get_part(get_exe_filename(), fn_title);// .Format(_T("%s_%d%02d%02d.log"), get_part(get_exe_filename(), fn_title), m_tlog.GetYear(), m_tlog.GetMonth(), m_tlog.GetDay());
		}
		else
		{
			m_log_file_title = file_title;// .Format(_T("%s_%d%02d%02d.log"), file_title, m_tlog.GetYear(), m_tlog.GetMonth(), m_tlog.GetDay());
		}

		m_log_fullpath.Format(_T("%s\\%s_%d%02d%02d.log"), m_log_folder, m_log_file_title, m_tlog.GetYear(), m_tlog.GetMonth(), m_tlog.GetDay());

		_tfopen_s(&m_fp, m_log_fullpath, _T("a")CHARSET);

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


bool CSCLog::release()
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

CString	CSCLog::write(LPCTSTR format, ...)
{
	va_list args;
	va_start(args, format);

	CString log_text;

	log_text.FormatV(format, args);

	return write(SCLOG_LEVEL_NONE, _T(""), 0, log_text);
}

CString CSCLog::write(int logLevel, TCHAR* func, int line, LPCTSTR format, ...)
{
	CString result = CString();

	try
	{
		//theCSLog.Lock();
		m_mutex.lock();

		if (m_fp == NULL)
		{
			if (!set(m_log_folder, m_log_file_title))
			{
				//theCSLog.Unlock();
				m_mutex.unlock();
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

		try
		{
			log_text.FormatV(format, args);
		}
		catch (CException* e)
		{
			CString str = get_last_error_string();
			TRACE(_T("%s\n"), str);
			m_mutex.unlock();
			return _T("log_text.FormatV() exception.");
		}

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

		if (!log_level.IsEmpty())
			log_level = _T(" ") + log_level;

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
		m_mutex.unlock();
		//theCSLog.Unlock();
	}
	catch (...)
	{
		release();
		m_mutex.unlock();
		//theCSLog.Unlock();
	}

	return result;
}

//usage : sFolder include folder names only except file name.
//c:\test\00\1.bmp	(x)	=> 1.bmp folder will be created.(not intended)
//c:\test\00		(o)
bool CSCLog::recursive_make_full_directory(LPCTSTR sFolder)
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

//현재 로그파일을 열어준다.
void CSCLog::open_log_file()
{
	ShellExecute(NULL, _T("open"), m_log_fullpath, NULL, NULL, SW_SHOWNORMAL);
}
