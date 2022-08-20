// FtpClient.h: interface for the CFtpClient class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_FTPCLIENT_H__F196D430_806C_4A00_B5BE_04AC559B59A2__INCLUDED_)
#define AFX_FTPCLIENT_H__F196D430_806C_4A00_B5BE_04AC559B59A2__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include <afxinet.h>	//ftp 를 위해 추가해야됨

class CFTPFileFind  
{
public:

	void GetErrorMessage();
	
	//{{AFX_FindFile 함수
	BOOL MatchsMask(DWORD dwMask);
	BOOL IsTemporary();
	BOOL IsSystem();
	BOOL IsReadOnly();
	BOOL IsNormal();
	BOOL IsHidden();
	BOOL IsDots();
	BOOL IsDirectory();
	BOOL IsCompressed();
	BOOL IsArchived();
	DWORD GetLength();
	BOOL GetLastWriteTime(CTime& Time);
	BOOL GetLastAccessTime(CTime& Time);
	CString GetFileTitle();
	CString GetFilePath();
	CString GetFileName();
	BOOL GetCreationTime(CTime& Time);
	CString GetFileURL();
	BOOL FindNextFile();
	BOOL FindFile(LPCTSTR RemoteFile = NULL, DWORD dwFlags = INTERNET_FLAG_RELOAD);
	//}}AFX_FindFile

	CFTPFileFind(CFtpConnection* pConnection);
	virtual ~CFTPFileFind();

	CFtpFileFind		*m_pRemoteFindFile;
	CString				m_strAddress;
};

class CFtpClient  
{
public:
	BOOL DownLoadDir(LPCTSTR rDir, LPCTSTR lDir);
	BOOL UpLoadDir(LPCTSTR lDir, LPCTSTR rDir);
	void GetErrorMessage();

	BOOL GetURL(CString& RemoteURL);
	BOOL LPWD();
	BOOL RPWD();
	BOOL LRen(CString LocalOldFile, CString LocalNewFile);
	BOOL RRen(CString RemoteOldFile, CString RemoteNewFile);
	BOOL LDel(CString LocalFile);
	BOOL RDel(CString RemoteFile);
	BOOL LRD(CString LocalDir);
	BOOL RRD(CString RemoteDir);
	BOOL LMD(CString LocalDir);
	BOOL RMD(CString RemoteDir);
	BOOL LCD(CString LocalDir);
	BOOL RCD(CString RemoteDir);
	CString LCD();
	CString RCD();

	BOOL DownLoad(CString strFile);
	BOOL DownLoad(CString RemoteFile, CString LocalFile);
	BOOL UpLoad(CString strFile);
	BOOL UpLoad(CString LocalFile, CString RemoteFile);

	BOOL Close();
	BOOL Open(CString strAddress, CString strName, CString strPassword, INTERNET_PORT nPort = INTERNET_DEFAULT_FTP_PORT);
	BOOL Open(CString strAddress, INTERNET_PORT nPort = INTERNET_DEFAULT_FTP_PORT);
	CFtpClient();
	virtual ~CFtpClient();

	//{{AFX_DATA(FTP)
	CInternetSession	m_Session;
	CFtpConnection		*m_pConnection;

	CString		m_strLocalDirText;
	CString		m_strRemoteDirText;
	CString		m_strAddress;
	//}}AFX_DATA
};

#endif // !defined(AFX_FTPCLIENT_H__F196D430_806C_4A00_B5BE_04AC559B59A2__INCLUDED_)
