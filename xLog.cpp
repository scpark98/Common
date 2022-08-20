#include "stdafx.h"
#include "xLog.h"

CxLog::CxLog()
{
	m_bEnableTimestamp	= true;
	m_strLogPath		= ".";
	m_strLogTitle		= "";
	m_strLogExt			= "log";
}

void CxLog::SetLogTitle( CString strLogTitle )
{
	m_strLogTitle = strLogTitle;
}

void CxLog::Log(CString str, bool bTimeInfo/* = true*/, bool bNewLine/* = true*/ )
{

	m_cs.Lock();

	FILE*	fp;

	if ( (fp = fopen( RemakeLogFileName(), "a" )) == NULL )
	{
		//AfxMessageBox( "log 파일을 생성할 수 없습니다." );
		m_cs.Unlock();
		return;
	}

	//로그가 ""이면 로그를 남기기 위함보다는 라인피드의 의미가 있다.
	if ( str == "" )
	{
		fprintf( fp, "\n" );
		fclose( fp );
		m_cs.Unlock();
		return;
	}

	//만약 첫 문자가 carriage return이면 한줄을 띠워주고 그 문자는 지워준다.
	//안그러면 날짜가 찍히고 다음줄에 데이터가 기록된다.
	if ( str.GetAt( 0 ) == '\n' )
	{
		TRACE( "\n" );
		fprintf( fp, "\n" );
		str = str.Mid( 1 );
	}

	SYSTEMTIME	tm;
	CString		sTimeString;

	GetLocalTime( &tm );
	sTimeString.Format( "%d/%02d/%02d %02d:%02d:%02d(%03d)",
		tm.wYear, tm.wMonth, tm.wDay,
		tm.wHour, tm.wMinute, tm.wSecond, tm.wMilliseconds );

	if ( bTimeInfo )
		str =  sTimeString + "  " + str;

	if ( bNewLine )
		str += "\n";
	else
		str += " ";

	fprintf( fp, str );

#ifdef DEBUG
	TRACE( "%s", str );
#endif

	fclose( fp );

	m_cs.Unlock();

/*
	CStdioFile fw;

	m_cs.Lock();
	BOOL bOpen = fw.Open(RemakeLogFileName(),CFile::modeCreate|CFile::modeNoTruncate|CFile::modeWrite|CFile::typeText);
	if(bOpen==FALSE)
	{
		m_cs.Unlock();
		return ;
	}
	fw.SeekToEnd();

	// Should we add time to beginning of string
	if (m_bEnableTimestamp)
	{
		str = GetTime() + str + "  ";
	}

	// Add \n\r to end of text
	str += "\n";

	fw.WriteString(str);
	fw.Close();
	m_cs.Unlock();
*/	
}

CString CxLog::GetTime()
{
	// Declare variables
	SYSTEMTIME time;
	CString sTime;

	// Get time
	GetLocalTime(&time);

	// Set up time
	sTime.Format("%02d:%02d:%02d.%03d", time.wHour, time.wMinute, time.wSecond, time.wMilliseconds);

	// Return value
	return sTime;
}

CString CxLog::RemakeLogFileName()
{
	SYSTEMTIME time;

	GetLocalTime(&time);

	if ( m_strLogPath.IsEmpty() )
		m_strLogPath = ".";

	m_sLogFileName.Format("%s\\%s %d%02d%02d.log", m_strLogPath, m_strLogTitle, time.wYear, time.wMonth, time.wDay );

	return m_sLogFileName;
}