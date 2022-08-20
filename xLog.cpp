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
		//AfxMessageBox( "log ������ ������ �� �����ϴ�." );
		m_cs.Unlock();
		return;
	}

	//�αװ� ""�̸� �α׸� ����� ���Ժ��ٴ� �����ǵ��� �ǹ̰� �ִ�.
	if ( str == "" )
	{
		fprintf( fp, "\n" );
		fclose( fp );
		m_cs.Unlock();
		return;
	}

	//���� ù ���ڰ� carriage return�̸� ������ ����ְ� �� ���ڴ� �����ش�.
	//�ȱ׷��� ��¥�� ������ �����ٿ� �����Ͱ� ��ϵȴ�.
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