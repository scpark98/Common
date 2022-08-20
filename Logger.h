#pragma once
#include <stdio.h>
#include <stdarg.h>


#define THREAD_SAFE_LOGGING

/** @brief log class
    LogChannel gLogger;
    gLogger.setLevel(LogChannel::LOG_INFO);
    gLogger.setOutput(_T("c:\\sclog.log"), 3211024, TRUE);

    logger logger1(_T("logger1"), &gLogger);
    logger logger2(_T("logger2"), &gLogger);

    logger1.setLevel(logger::LOG_INFO);
    logger2.setLevel(logger::LOG_ERROR);

    gLogger.log(_T("\n========================\n"));
    logger1.scerror(_T("failed at...: E.%d\n"), GetLastError());
    logger2.scerror(_T("failed at...: E.%d\n"), GetLastError());

// output
========================
[ERRO] 18:23:22:796,05/15 (logger1:LoggerTestDlg.cpp:127) - failed at...: E.183
[ERRO] 18:23:22:796,05/15 (logger2:LoggerTestDlg.cpp:128) - failed at...: E.183

*/
class LogChannel
{
    enum { MAX_MESSAGE = 1024 };
    BOOL                m_bDbgView;
    FILE*               m_fpLog;
#if defined(THREAD_SAFE_LOGGING)
#   if  defined(_WIN32)
    CRITICAL_SECTION    m_key;
#   else
    pthread_mutex_t     m_key;
#   endif
#endif

    friend class logger;
    void log(LPCTSTR szLevel, LPCTSTR lpszLabel, LPCTSTR lpszFile, int line, LPCTSTR fmt, va_list& ap);
public:
    LogChannel();
    ~LogChannel();

/** @brief set log output channel
    @param  lpszLogPath log file path, NULL : not using file logging.
    @param  nMaxLogSize 0(no limit), n(max file size is n-Byte)
    @param  bDbgView    enable debug/console ouput logging.
    @return bool        success/fail
*/
    bool setOutput(LPCTSTR lpszLogPath, int nMaxLogSize = 0, BOOL bDbgView = FALSE);

/** @brief logging function
    @param  fmt. log format like printf
*/
    void log(LPCTSTR fmt, ...);
};


class logger
{
    LPCTSTR             m_srcFile;
    int                 m_srcLine;
    ULONG               m_logLevel;
    LPCTSTR             m_szLabel;
    LogChannel*         m_chan;
public:
    logger(LPCTSTR szLabel, LogChannel* _channel) {
        m_szLabel = szLabel;
        m_chan = _channel;
        m_logLevel = LOG_ERROR;
    }
    ~logger() {};


/** @brief set logging level
    @param  level : [LOG_ALWAYS ~ LOG_MAX)
*/
    void setLevel(ULONG level) { ASSERT(level < LOG_MAX); m_logLevel = level; }


/** @brief set source position
    @param  lpszFile    source file name
    @param  line    source file line
    @return LogChannel& self object
*/
    logger& setSrcPos(LPCTSTR lpszFile = NULL, int line = 0) {
        m_srcFile = lpszFile; m_srcLine = line; return *this;
    }


/** @brief logging function
    @param  fmt. log format like printf
*/
    void always(LPCTSTR fmt, ...) {
        va_list ap; va_start(ap, fmt);
        m_chan->log(_T("[ALWS]"), m_szLabel, m_srcFile, m_srcLine, fmt, ap);
        va_end(ap);
    }
    void fatal(LPCTSTR fmt, ...) {
        if (LOG_FATAL > m_logLevel) return ;
        va_list ap; va_start(ap, fmt);
        m_chan->log(_T("[FATL]"), m_szLabel, m_srcFile, m_srcLine, fmt, ap);
        va_end(ap);
    }
    void error(LPCTSTR fmt, ...) {
        if (LOG_ERROR > m_logLevel) return ;
        va_list ap; va_start(ap, fmt);
        m_chan->log(_T("[ERRO]"), m_szLabel, m_srcFile, m_srcLine, fmt, ap);
        va_end(ap);
    }
    void warn(LPCTSTR fmt, ...) {
        if (LOG_WARN > m_logLevel) return ;
        va_list ap; va_start(ap, fmt);
        m_chan->log(_T("[WARN]"), m_szLabel, m_srcFile, m_srcLine, fmt, ap);
        va_end(ap);
    }
    void info(LPCTSTR fmt, ...) {
        if (LOG_INFO > m_logLevel) return ;
        va_list ap; va_start(ap, fmt);
        m_chan->log(_T("[INFO]"), m_szLabel, m_srcFile, m_srcLine, fmt, ap);
        va_end(ap);
    }
    void step(LPCTSTR fmt, ...) {
        if (LOG_STEP > m_logLevel) return ;
        va_list ap; va_start(ap, fmt);
        m_chan->log(_T("[STEP]"), m_szLabel, m_srcFile, m_srcLine, fmt, ap);
        va_end(ap);
    }
    void debug(LPCTSTR fmt, ...) {
        if (LOG_DEBUG > m_logLevel) return ;
        va_list ap; va_start(ap, fmt);
        m_chan->log(_T("[DEBG]"), m_szLabel, m_srcFile, m_srcLine, fmt, ap);
        va_end(ap);
    }

    enum {
        MAX_MESSAGE = 1024,
        LOG_ALWAYS = 0,
        LOG_FATAL,
        LOG_ERROR,
        LOG_WARN,
        LOG_INFO,
        LOG_STEP,
        LOG_DEBUG,
        LOG_MAX
    };
};

#ifdef _UNICODE
#   define __toWideChar(str) L##str
#   define _toWideChar(str) __toWideChar(str)
#   define __TFUNC__  _toWideChar(__FILE__)
#else
#   define __TFUNC__  __FILE__
#endif

#define srcpos  setSrcPos(__TFUNC__, __LINE__)
#define sclog   setSrcPos(__TFUNC__, __LINE__).log
#define scfatal setSrcPos(__TFUNC__, __LINE__).fatal
#define scerror setSrcPos(__TFUNC__, __LINE__).error
#define scwarn  setSrcPos(__TFUNC__, __LINE__).warn
#define scinfo  setSrcPos(__TFUNC__, __LINE__).info
#define scstep  setSrcPos(__TFUNC__, __LINE__).step
#define scdebug setSrcPos(__TFUNC__, __LINE__).debug
