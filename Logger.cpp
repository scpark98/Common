
#include "stdafx.h"
#include "logger.h"
#include <winbase.h>

#undef PATH_SLASH
#ifdef _WIN32 // windows, mobile
#   define PATH_SLASH           _T('\\')
#   define enter_cs(k)          EnterCriticalSection(k)
#   define leave_cs(k)          LeaveCriticalSection(k)
#   define init_cs(k)           InitializeCriticalSectionAndSpinCount(k,2000)
#   define delete_cs(k)         DeleteCriticalSection(k)
#else // __APPLE__, __linux__
#   include <sys/time.h>
#   define PATH_SLASH           _T('/')
#   define enter_cs(k)          pthread_mutex_lock(k)
#   define leave_cs(k)          pthread_mutex_unlock(k)
#   define init_cs(k)           pthread_mutex_init(k,0)
#   define delete_cs(k)         pthread_mutex_destroy(k)
#   define OutputDebugString    printf
#endif

#if !defined(THREAD_SAFE_LOGGING)
#   undef  enter_cs
#   undef  leave_cs
#   undef  init_cs
#   undef  delete_cs
#   define enter_cs(k)
#   define leave_cs(k)
#   define init_cs(k)
#   define delete_cs(k)
#endif


static FILE* resizeOpen(LPCTSTR lpszFile, int nMaxSize);

LogChannel::LogChannel()
{
    init_cs(&m_key);
    m_fpLog = NULL;
}

LogChannel::~LogChannel()
{
    enter_cs(&m_key);
    if (m_fpLog) { fclose(m_fpLog); m_fpLog = NULL;}
    leave_cs(&m_key);

    delete_cs(&m_key);
}


bool LogChannel::setOutput(LPCTSTR lpszLogPath, int nMaxLogSize, BOOL bDbgView)
{
    m_bDbgView = bDbgView;
    if (m_fpLog) fclose(m_fpLog); m_fpLog = NULL;

    if (!lpszLogPath) return TRUE;

    m_fpLog = resizeOpen(lpszLogPath, nMaxLogSize);
    return m_fpLog != NULL;
}

void LogChannel::log(LPCTSTR fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    TCHAR buffer[MAX_MESSAGE];
    _vstprintf(buffer, fmt, ap);
    va_end(ap);

    enter_cs(&m_key);

    if (m_bDbgView)
        OutputDebugString(buffer);

    if (m_fpLog) {
        _fputts(buffer, m_fpLog);
        fflush(m_fpLog);
    }
    leave_cs(&m_key);
}

void LogChannel::log(
                     LPCTSTR szLevel, LPCTSTR lpszLabel,
                     LPCTSTR lpszFile, int line,
                     LPCTSTR fmt, va_list& ap)
{
    TCHAR buffer[MAX_MESSAGE];

#ifdef _WIN32
    SYSTEMTIME* tmNow = (SYSTEMTIME*)(buffer+MAX_MESSAGE-sizeof(SYSTEMTIME)); // reuse currently not used.
    GetLocalTime(tmNow);
    if (lpszFile) {
        _stprintf(buffer, _T("%s %02d:%02d:%02d:%03d,%02d/%02d (%s:%s:%d) - "),
            szLevel,
            tmNow->wHour,  tmNow->wMinute, tmNow->wSecond, tmNow->wMilliseconds,
            tmNow->wMonth, tmNow->wDay,
            lpszLabel, _tcsrchr(lpszFile, PATH_SLASH)+1, line);
        //lpszFile = NULL;
    }
    else {
        _stprintf(buffer, _T("%s %02d:%02d:%02d:%03d,%02d/%02d (%s) - "),
            szLevel,
            tmNow->wHour,  tmNow->wMinute, tmNow->wSecond, tmNow->wMilliseconds,
            tmNow->wMonth, tmNow->wDay,
            lpszLabel);
    }
#else
    struct timeval tv;
    gettimeofday(&tv, 0);
    struct tm* tmNow = localtime( &tv.tv_sec );
    if (lpszFile) {
        _stprintf(buffer, _T("%s %02d:%02d:%02d:%03d,%02d/%02d (%s:%s:%d) - "),
            szLevel,
            tmNow->tm_hour,  tmNow->tm_min, tmNow->tm_sec, tv.tv_usec/1000,
            tmNow->tm_mon+1, tmNow->tm_mday,
            lpszLabel, _tcsrchr(lpszFile, PATH_SLASH)+1, line);
        //lpszFile = NULL;
    }
    else {
        _stprintf(buffer, _T("%s %02d:%02d:%02d:%03d,%02d/%02d (%s) - "),
            szLevel,
            tmNow->tm_hour,  tmNow->tm_min, tmNow->tm_sec, tv.tv_usec/1000,
            tmNow->tm_mon+1, tmNow->tm_mday,
            lpszLabel);
    }
#endif
    _vstprintf(buffer+25+_tcslen(buffer+25), fmt, ap); // +25 ; improve _tcslen performance.
    enter_cs(&m_key);

    if (m_bDbgView)
        OutputDebugString(buffer);

    if (m_fpLog) {
        _fputts(buffer, m_fpLog);
        fflush(m_fpLog);
    }
    leave_cs(&m_key);
}

//////////////////////////////////////////////////////////////////////////

FILE* resizeOpen(LPCTSTR lpszFile, int nMaxSize)
{
    FILE* fp = _tfopen(lpszFile, _T("a+"));
    while(fp)
    {
        if (0 != fseek(fp, 0, SEEK_END)) break;
        int fileSize = ftell(fp);

        if (fileSize > nMaxSize*2)
        {
            if (0 != fseek(fp, -nMaxSize, SEEK_END)) break;
            char* buf = (char*)malloc(nMaxSize);
            if (buf)
            {
                fileSize = fread(buf, 1, nMaxSize, fp);
                fclose(fp);

                fp = _tfopen(lpszFile, _T("w"));
                if (fp) {
                    _fputts(_T("------------cut here-----------\r\n"), fp);
                    fwrite(buf, 1, fileSize, fp);
                    fflush(fp);
                }
                free(buf);
            }
        }
        break;
    }

    if (fp) fseek(fp, 0, SEEK_END);
    return fp;
}
