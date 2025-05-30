// FtpGet.h: interface for the CFtpGet class.

#if !defined(AFX_FTPGET_H__9A4CA560_F2F5_11D3_9FF0_0080C8F223E1__INCLUDED_)
#define AFX_FTPGET_H__9A4CA560_F2F5_11D3_9FF0_0080C8F223E1__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include <afxinet.h>

#define FTP_RECEIVED_DATA			0
#define FTP_SENT_DATA				1
#define MESSAGE_FTP_TRANSFER_DATA	WM_USER + 0x7001

#define FTP_BUFFER_SIZE				4096

class CFtpGet
{
public:// function
	CFtpGet();
	virtual ~CFtpGet();

	//전송 상태를 전달하기 위한 parent윈도우 핸들 명시
	BOOL		Connect( HWND hParentWnd, CString sServerIP, CString sID, CString sPass, int nPort = 21, BOOL bPassive = FALSE );
	BOOL		IsConnected() {	return ( m_pFtpConnection != NULL ); }
	void		AbortDownloading( BOOL bAbortDownloading );

	// only one file
	BOOL		GetFile(CString remoteFile, CString localFile);
	BOOL		PutFile(CString localFile, CString remoteFile);
	CTime		GetFileLastWriteTime( CString sremoteFile );
	DWORD		GetRemoteFileSize( CString sRemoteFile );

	// for specified files
	BOOL		GetWildcardFile( CString remoteFolder, CString localFolder, CString sWildcard );

	// a full CString array
	int			GetMultipleFile(CStringArray *remoteArray,
				CStringArray *localArray,
				int number_file);

	// close the connection
	BOOL		CloseConnection();

	BOOL		m_bDeleteAfterGet;		// 다운받은 후 서버에서 그 파일을 지울것인지...
	int			m_nFileCount;
	CString		m_sFileName[500];

	BOOL		GetFileList( CString remoteFolder, CString sWildcard = _T("*.*") );
	BOOL		GetFileList( CString remoteFolder, CStringArray& arFileList );

	BOOL		CheckFTPFileIsExist( CString sFile );
	BOOL		FindAllFiles( CString strPath, CStringArray& arFile, CDWordArray& arSize );
	void		FindDirectoryFiles( CString strPath, CStringArray& arFile, CDWordArray& arSize );

	int			nFolderCount;
	CString		sFolderName[500];
	BOOL		GetFolderList( CString sRootFolder );
	
	CString		GetRemoteDirectory();
	BOOL		SetRemoteDirectory( CString sDir );
	BOOL		CreateDirectory( CString sDir );

	void		SetBufferSize( int nBufferSize ) { m_nBufferSize = nBufferSize; }
	int			GetBufferSize() { return m_nBufferSize; }


private:// var
	CInternetSession	m_InternetSession;
	CFtpConnection*		m_pFtpConnection;
	CString				m_sServerIP;
	CString				m_sUser;
	CString				m_sPass;
	int					m_nPort;
	BOOL				m_bPassive;
	int					m_nBufferSize;
	BOOL				m_bAbortDownloading;
	BOOL				m_bAbortUploading;
	HWND				m_hParentWnd;
public:
	CString				m_sAppName;
	bool				m_bMissed[100];
};

#endif // !defined(AFX_FTPGET_H__9A4CA560_F2F5_11D3_9FF0_0080C8F223E1__INCLUDED_)
