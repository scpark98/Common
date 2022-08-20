/*
		Big thanks to  Ken Sutherland, he answered a post in
		codeguru and this class was made using his answer.
		Qu: How to Get file form ftp server
http://www.codeguru.com/bbs/wt/showpost.pl?Board=vc&Number=48384&page=&view=&sb=
*/

/*
		Copyright	Robert Lascelle
					RLProgrammation
					vlad3@sympatico.ca

You can use this source for anything, if any change, please show
me the change and explain them.

			It's my first i/o class over internet.
*/

// FtpGet.cpp: implementation of the CFtpGet class.

#include "stdafx.h"
// from my program
// #include "http.h"
//
#include "FtpGet.h"
//#include "resource.h"
#include "Functions.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif


//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CFtpGet::CFtpGet()
{
	// get the name of the app
	m_sAppName.LoadString(AFX_IDS_APP_TITLE);


	// create an internet session
	m_hParentWnd		= NULL;
	m_pFtpConnection	= NULL;
	m_bDeleteAfterGet	= FALSE;

	m_bAbortDownloading	= FALSE;
	m_bAbortUploading	= FALSE;

	m_nBufferSize		= FTP_BUFFER_SIZE;
}

CFtpGet::~CFtpGet()
{
	CloseConnection();
}

// function, in logical order
BOOL CFtpGet::Connect( HWND hParentWnd, CString sServerIP, CString sID, CString sPass, int nPort /*= 21*/, BOOL bPassive /*= FALSE*/ )
{
	if ( m_pFtpConnection != NULL )
	{
		m_pFtpConnection->Close();
		delete m_pFtpConnection;
		m_pFtpConnection = NULL;
	}

	if ( sServerIP == "" || sID == "" || sPass == "" )
		return FALSE;

	m_hParentWnd	= hParentWnd;
	m_sUser			= sID;
	m_sPass			= sPass;
	m_nPort			= nPort;
	m_bPassive		= bPassive;

	// put the server name in the CFtpGet class
	m_sServerIP = sServerIP;

	try
	{
		// try to connect to a ftp server
		m_pFtpConnection = m_InternetSession.GetFtpConnection( m_sServerIP, m_sUser, m_sPass, m_nPort, m_bPassive );
	}
	catch (CInternetException* pEx) 
	{ 
		// if failed, just show the error

		// Oops! We failed to connect!
		TCHAR szErr[1024];
		pEx->GetErrorMessage(szErr, 1024);
		TRACE(szErr);
		//		AfxMessageBox(szErr);
		pEx->Delete();
		return FALSE;// return 1 but previous error box have been showed
	}


	return 1;

}

BOOL CFtpGet::CloseConnection()
{
	// close the connection to server, you can reconnect latter
	if ( m_pFtpConnection == NULL )
		return TRUE;

	try
	{
		m_pFtpConnection->Close();

		if ( m_pFtpConnection != NULL )
			delete m_pFtpConnection;

		m_pFtpConnection = NULL;
	}
	catch(...)
	{
		return FALSE;
	}

	return TRUE;
}

BOOL CFtpGet::GetFile( CString remoteFile, CString localFile )
{
	if ( !m_pFtpConnection )
		return FALSE;

	m_bAbortDownloading = FALSE;

	if ( TRUE )
	{
		//TRACE( "GetFile\n" );
		CInternetFile* pIFile = NULL;

		try
		{
			pIFile = m_pFtpConnection->OpenFile( remoteFile, GENERIC_READ, FTP_TRANSFER_TYPE_BINARY | INTERNET_FLAG_RELOAD, 0 );
		}
		catch ( CInternetException* pEx )
		{ 
			TCHAR szErr[1024];
			pEx->GetErrorMessage(szErr, 1024);
			TRACE(szErr);
			pEx->Delete();
			return 0;// return 1 but previous error box have been showed
		}

		if ( pIFile == NULL )
		{
			TRACE( "=================================can't open remoteFile : %s\n", remoteFile );
			GetFile( remoteFile, localFile );
			return TRUE;
		}


		//TRACE( "GetFile-before download\n" );

		FILE*	fp = fopen( localFile, "wb" );

		if ( !fp )
		{
			TRACE( "can't create %s\n", localFile );
			return FALSE;
		}

		int		nRead;
		int		nReadTotal = 0;
		BYTE*	buffer = new BYTE[m_nBufferSize];

		while ( TRUE )
		{
			if ( !m_pFtpConnection || m_bAbortDownloading )
			{
				//delete[] buffer;
				pIFile->Close();
				delete pIFile;
				fclose( fp );
				return FALSE;
			}

			nRead = pIFile->Read( buffer, m_nBufferSize );
			fwrite( buffer, sizeof(byte), nRead, fp );
			nReadTotal += nRead;

			if ( m_hParentWnd )
				::SendMessage( m_hParentWnd, MESSAGE_FTP_TRANSFER_DATA, FTP_RECEIVED_DATA, nReadTotal );

			if ( nRead < m_nBufferSize )
				break;
		}

		delete[] buffer;
		pIFile->Close();
		delete pIFile;
		fclose( fp );

		return TRUE;
	}
	else
	{
		// Try to get the file
		BOOL bGotFile = m_pFtpConnection->GetFile(	remoteFile,
													localFile,
													FALSE,
													FILE_ATTRIBUTE_NORMAL,	
													FTP_TRANSFER_TYPE_BINARY | INTERNET_FLAG_RELOAD | INTERNET_FLAG_NO_CACHE_WRITE );

// 		if ( bGotFile && m_bDeleteAfterGet )
// 			m_pFtpConnection->Remove( remoteFile );

		return bGotFile ? 1 : 0 ;
		// if bGotFile is 0 ( FALSE ), return 0
		// if bGotFile is 1 ( TRUE  ), return 1
	}
}

BOOL CFtpGet::PutFile( CString localFile, CString remoteFile )
{
	if ( !m_pFtpConnection )
		return FALSE;

	//심플 방식
	return m_pFtpConnection->PutFile(localFile,	remoteFile );
/*
	//패킷 방식
	CInternetFile* pIFile = NULL;
	pIFile = m_pFtpConnection->OpenFile( remoteFile, GENERIC_WRITE, FTP_TRANSFER_TYPE_BINARY );

	if ( pIFile == NULL )
	{
		TRACE( "can't create remoteFile : %s\n", remoteFile );
		return FALSE;
	}

	byte	packet[FTP_BUFFER_SIZE];
	int		nWrite;
	int		nWriteTotal = 0;

	CFile	File;
	if ( !File.Open( localFile, CFile::modeRead ) )
	{
		TRACE( "can't read %s\n", localFile );
		pIFile->Close();
		return FALSE;
	}

	while ( TRUE )
	{
		nWrite = File.Read( packet, FTP_BUFFER_SIZE );
		pIFile->Write( packet, FTP_BUFFER_SIZE );

		nWriteTotal += nWrite;

		if ( m_hParentWnd )
			::SendMessage( m_hParentWnd, MESSAGE_FTP_TRANSFER_DATA, FTP_SENT_DATA, nWriteTotal );
		else
			TRACE( "warning : m_hParentWnd is NULL\n" );

		if ( nWrite < FTP_BUFFER_SIZE )
			break;
	}

	pIFile->Close();
	delete pIFile;
	File.Close();
*/
	return TRUE;
}

int CFtpGet::GetMultipleFile(CStringArray *remoteArray,
							 CStringArray *localArray,
							 int number_file)
{
	// init some var
	BOOL goodfile;
	int x=0;
	int nb_lost_file =0;

	// while loop to transfer every file in the array
	while(x<number_file)
	{
		// try to get file
		goodfile = m_pFtpConnection->GetFile(remoteArray->GetAt(x),
			localArray->GetAt(x),
			FALSE,
			FILE_ATTRIBUTE_NORMAL,
			FTP_TRANSFER_TYPE_BINARY | INTERNET_FLAG_RELOAD | INTERNET_FLAG_NO_CACHE_WRITE);

		m_bMissed[x] = goodfile ? 0 : 1;
		// if failed, missed[x] become 1
		// if good, missed become 0
		if ( m_bMissed[x] )
			nb_lost_file++;
		// if the file was missed, increase the number of 
		// missing file.
		// increase to the next file
		x++;
	}
	//return the number of missing file, if any.
	return nb_lost_file;
}

CString CFtpGet::GetRemoteDirectory()
{
	if ( m_pFtpConnection == NULL )
		return "";

	CString		sDir = "";
	m_pFtpConnection->GetCurrentDirectory( sDir );
	return sDir;
}

BOOL CFtpGet::SetRemoteDirectory( CString sDir )
{
	if ( m_pFtpConnection == NULL )
		return FALSE;

	return m_pFtpConnection->SetCurrentDirectory( sDir );
}

BOOL CFtpGet::GetFileList( CString remoteFolder, CString sWildcard )
{
	if ( m_pFtpConnection == NULL )
		return FALSE;

 	CFtpFileFind ftpFind( m_pFtpConnection );

	if ( remoteFolder.GetAt( remoteFolder.GetLength() - 1 ) != '/' )
		remoteFolder += '/';
	
	m_nFileCount = 0;
	
	BOOL bContinue = ftpFind.FindFile( sWildcard );

	if ( !bContinue )
	{
//		DWORD dwError = GetLastError();

		// the directory is empty; just close up and return.
		ftpFind.Close();
/*		GetFtpListView()->ActivateAnimation(FALSE);
		m_bBusy = FALSE;
		
		if (dwError == ERROR_INTERNET_TIMEOUT)
		{
			if (!m_FtpSite.m_bUsePASVMode)
				GetTraceView()->AddTraceLine(3, "[%d] The request has timed out, try connecting using PASV mode.", AfxGetThread()->m_nThreadID);
			else
				GetTraceView()->AddTraceLine(3, "[%d] The request has timed out.", AfxGetThread()->m_nThreadID);
		}
		else
		{
			GetTraceView()->AddTraceLine(1, "[%d] \"%s\" is empty.", AfxGetThread()->m_nThreadID, m_strCurrentDirectory);
		}
*/		return FALSE;
	}


	CString		sFileName;
//	CString		sSupportedFormat;

//	sSupportedFormat.LoadString( IDS_SUPPORTED_FORMAT );

	// set up and insert a tvstruct for each item in the directory
	while ( bContinue )
	{
		// FindNextFile muxt be called before info can be gleaned from ftpFind
		bContinue = ftpFind.FindNextFile();

		if ( ftpFind.IsDots() || ftpFind.IsDirectory() )
			continue;

		sFileName = ftpFind.GetFileName();
		sFileName.MakeLower();

		//if ( sSupportedFormat.Find( GetFileExtensionFromFullPath(sFileName) ) >= 0 )
		{
			m_sFileName[m_nFileCount] = ftpFind.GetFileName();
			m_nFileCount++;
		}

		// is selected item a directory ?
/*		if (ftpFind.IsDirectory())
		{
			// get folder icon index 
			int nIndex = GetIconIndex(strFileName, TRUE);
			tvstruct.item.iImage = nIndex;
			
			// get selected folder icon index 
			nIndex = GetIconIndex(strFileName, TRUE, TRUE);
			tvstruct.item.iSelectedImage = nIndex;

			tvstruct.hParent = hParent;
			tvstruct.hInsertAfter = TVI_LAST;
			tvstruct.item.pszText = (LPTSTR)(LPCTSTR)strFileName;   
			tvstruct.item.mask = TVIF_IMAGE | TVIF_SELECTEDIMAGE | TVIF_TEXT;

			// insert item in treeview
			if (!bHasChilderen)
				GetFtpTreeView()->GetTreeCtrl().InsertItem(&tvstruct);

			// insert item to listview
			GetFtpListView()->AddItem(nCount++, &ftpFind);
		}
		else
		{
			// insert item to listview
			GetFtpListView()->AddItem(nCount++, &ftpFind);
		}
		DoEvents();
		if (!m_bBusy)
			bContinue = FALSE;
*/	}

	ftpFind.Close();

	
	return TRUE;
}

FILE*	fp1 = fopen( "c:\\test.txt", "w" );

void CFtpGet::FindDirectoryFiles( CString strPath, CStringArray& arFile, CDWordArray& arSize )
{
	DWORD			nSize;
	CString			sFile;
	CStringArray	arFolder;
	CFtpFileFind	finder( m_pFtpConnection );

	BOOL bWorking = finder.FindFile( strPath + "/*.*" );

	while ( bWorking )
	{
		bWorking = finder.FindNextFile();
		sFile = finder.GetFilePath();
		
		if ( finder.IsDots() )
			continue;
		else if ( finder.IsDirectory() )
		{
			// 폴더이면 폴더리스트에 넣어주고
			arFolder.Add( sFile );
		}
		else
		{
			arFile.Add( sFile );
			nSize = (DWORD)finder.GetLength();
			arSize.Add( nSize );
			//fprintf( fp1, "%s\n", finder.GetFilePath() );
		}
	}

	finder.Close();

	// 각 폴더마다 재귀호출 시작.
	for ( int i = 0; i < arFolder.GetSize(); i++ )
		FindDirectoryFiles( arFolder.GetAt(i), arFile, arSize );
}

BOOL CFtpGet::FindAllFiles( CString strPath, CStringArray& arFile, CDWordArray& arSize )
{
	if ( !m_pFtpConnection )
		return FALSE;

	FindDirectoryFiles( strPath, arFile, arSize );

	fclose( fp1 );

	return TRUE;
}

BOOL CFtpGet::GetFileList( CString remoteFolder, CStringArray& arFileList )
{
	if ( !m_pFtpConnection )
		return FALSE;

	CFtpFileFind	finder( m_pFtpConnection );
	CString			strSearchDir;
	
/*	if(strPath.Left(1) == "/")
		strSearchDir = strPath + _T("/*.*");
	else
		strSearchDir = _T("/") + strPath + _T("/*.*");
*/	

	arFileList.RemoveAll();

//	FindDirectoryFiles( remoteFolder + "//*.*" );

	finder.Close();
	return TRUE;
}


BOOL CFtpGet::GetFolderList( CString sRootFolder )
{
	if ( m_pFtpConnection == NULL )
		return FALSE;

 	CFtpFileFind ftpFind( m_pFtpConnection );

	if ( sRootFolder.GetAt( sRootFolder.GetLength() - 1 ) != '/' )
		sRootFolder += '/';
	
	BOOL bContinue = ftpFind.FindFile( "*.*" );

	if ( !bContinue )
	{
		// the directory is empty; just close up and return.
		ftpFind.Close();
		return FALSE;
	}

	nFolderCount = 0;

	// set up and insert a tvstruct for each item in the directory
	while ( bContinue )
	{
		// FindNextFile muxt be called before info can be gleaned from ftpFind
		bContinue = ftpFind.FindNextFile();

		if ( ftpFind.IsDirectory() )
		{
			sFolderName[nFolderCount] = ftpFind.GetFileName();
			nFolderCount++;
		}
	}

	ftpFind.Close();

	
	return TRUE;
}

BOOL CFtpGet::GetWildcardFile( CString remoteFolder, CString localFolder, CString sWildcard )
{
	if ( !m_pFtpConnection )
		return FALSE;

	for ( int i = 0; i < m_nFileCount; i++ )
	{
		BOOL bSuccess = GetFile( remoteFolder + m_sFileName[i], localFolder + m_sFileName[i] );

		if ( bSuccess && m_bDeleteAfterGet )
			m_pFtpConnection->Remove( m_sFileName[i] );
	}

	return TRUE;
}

CTime CFtpGet::GetFileLastWriteTime( CString sRemoteFile )
{
	if ( !m_pFtpConnection )
		return NULL;

    CFileFind       finder;
    CString         strtime;
	CTime			tTime = NULL;
	CFtpFileFind    ftpFinder( m_pFtpConnection );
    

    BOOL bFind = ftpFinder.FindFile();

	if ( ftpFinder.FindFile( sRemoteFile ) == FALSE )
		return -3;	// file not found

	ftpFinder.FindNextFile();

	FILETIME ftpLocalFileTime;
	ftpFinder.GetLastWriteTime( &ftpLocalFileTime );

	SYSTEMTIME ftpSysTime;
	FileTimeToSystemTime(&ftpLocalFileTime, &ftpSysTime);

	tTime = CTime( ftpSysTime.wYear,ftpSysTime.wMonth,ftpSysTime.wDay,
					ftpSysTime.wHour,ftpSysTime.wMinute,ftpSysTime.wSecond );

	ftpFinder.Close();

	return tTime;
}
/*
DWORD CFtpGet::GetRemoteFileSize( CString sRemoteFile )
{
	if ( m_pFtpConnection == NULL )
		return -1;	//not connected

	DWORD			wSize = -2;	
	CFtpFileFind	ftpFinder( m_pFtpConnection );

	if ( ftpFinder.FindFile( sRemoteFile ) == FALSE )
		return -3;	// file not found

	ftpFinder.FindNextFile();
	wSize = ftpFinder.GetLength();

	ftpFinder.Close();
	
	return wSize;
}
*/

BOOL CFtpGet::CheckFTPFileIsExist( CString sRemoteFile )
{
	if ( m_pFtpConnection == NULL )
		return -1;	//not connected

	CString			sFolder;
	CString			sFileName;
	TCHAR			chPathMark = '/';
	CFtpFileFind	finder( m_pFtpConnection );
	CString			sFoundFile;
	
	if ( sRemoteFile.Find( "\\" ) > 0 )
		chPathMark = '\\';

	sFolder		= sRemoteFile.Left( sRemoteFile.ReverseFind( chPathMark ) );
	sFileName	= sRemoteFile.Right( sRemoteFile.GetLength() - sFolder.GetLength() - 1 );

	SetRemoteDirectory( sFolder );

	// start looping
	BOOL bFound = FALSE;
	BOOL bWorking = finder.FindFile( NULL, INTERNET_FLAG_RELOAD | INTERNET_FLAG_EXISTING_CONNECT );

	while ( bWorking )
	{
		bWorking = finder.FindNextFile();
		sFoundFile = finder.GetFileName();

		if ( sFoundFile == sFileName )
		{
			bFound = TRUE;
			break;
		}
	}

	SetRemoteDirectory( "/" );

	return bFound;
/*	
	while ( bWorking )
	{
		bWorking = finder.FindNextFile();
		sFoundFile = finder.GetFileName();
		//sFoundFile.MakeLower();
		
		if ( sFoundFile == sFile )
			return TRUE;
	}

	return FALSE;


	CInternetSession	sess;
	CFtpConnection*		pConnect = NULL;
	DWORD				dwRemoteFilesize;
	
	sFile.MakeLower();
	
	try
	{
		pConnect = sess.GetFtpConnection( m_sServerName, m_sUser, m_sPass, 21, FALSE);
		
		// use a file find object to enumerate files
		CFtpFileFind finder(pConnect);
		
		// start looping
		BOOL bWorking = finder.FindFile(_T("*"));
		
		while ( bWorking )
		{
			bWorking = finder.FindNextFile();
			sFoundFile = finder.GetFileName();
			sFoundFile.MakeLower();
			
			if ( sFoundFile == sFile )
				return TRUE;
		}
	}
	catch (CInternetException* pEx)
	{
		TCHAR sz[1024];
		pEx->GetErrorMessage(sz, 1024);
		printf("ERROR!  %s\n", sz);
		pEx->Delete();
		dwRemoteFilesize =0;
	}
	
	// if the connection is open, close it
	if (pConnect != NULL)
		pConnect->Close();
	
	delete pConnect;
	
	return FALSE;
*/
}

DWORD CFtpGet::GetRemoteFileSize( CString strRemoteFile )
{
	CInternetSession	sess;
	CFtpConnection*		pConnect = NULL;
	DWORD				dwRemoteFilesize;
	CString				sFoundFile;
	
	strRemoteFile.MakeLower();
	
	try
	{
		pConnect = sess.GetFtpConnection( m_sServerIP, m_sUser, m_sPass, m_nPort, FALSE);
		
		// use a file find object to enumerate files
		CFtpFileFind finder(pConnect);
		
		// start looping
		BOOL bWorking = finder.FindFile(_T("*"));
		
		while ( bWorking )
		{
			bWorking = finder.FindNextFile();
			sFoundFile = finder.GetFileName();
			sFoundFile.MakeLower();
			
			if ( sFoundFile == strRemoteFile )
				return dwRemoteFilesize = (DWORD)finder.GetLength();
			
			printf("%s\n %ld %s", (LPCTSTR) finder.GetFileURL(), finder.GetLength(), finder.GetFileName());
		}
	}
	catch (CInternetException* pEx)
	{
		TCHAR sz[1024];
		pEx->GetErrorMessage(sz, 1024);
		printf("ERROR!  %s\n", sz);
		pEx->Delete();
		dwRemoteFilesize =0;
	}
	
	// if the connection is open, close it
	if (pConnect != NULL)
		pConnect->Close();
	
	delete pConnect;
	
	return -1;
}


BOOL CFtpGet::CreateDirectory( CString sDir )
{
	if ( m_pFtpConnection == NULL )
		return FALSE;

	return m_pFtpConnection->CreateDirectory( sDir );
}

void CFtpGet::AbortDownloading( BOOL bAbortDownloading )
{
	if ( bAbortDownloading )
		m_bAbortDownloading = TRUE;
	else
		m_bAbortUploading = TRUE;
}

