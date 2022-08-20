#pragma once

class CxLog  
{
public:
	// Constructor & destructor
	CxLog();
	// Functions
	void Log(CString sText, bool bTimeInfo = true, bool bNewLine = true );
	void EnableTimestamp(BOOL bEnable = TRUE){m_bEnableTimestamp = bEnable;}
	void SetLogPath(CString strLogPath){m_strLogPath = strLogPath;}
	void SetLogTitle ( CString strLogTitle );
	void SetLogExt(CString strLogExt){m_strLogExt = strLogExt;}
	CString GetLogFileName() { return m_sLogFileName; }
	// Variables
private:
	// Functions
	CString GetTime();
	CString RemakeLogFileName();
	CCriticalSection m_cs;

	// Variables
	BOOL    m_bEnableTimestamp;
	CString m_strLogPath;
	CString m_strLogTitle;
	CString m_strLogExt;
	CString m_sLogFileName;
};
