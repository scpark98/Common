// FtpClient.cpp: implementation of the CFtpClient class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "FtpClient.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CFTPFileFind::CFTPFileFind(CFtpConnection* pConnection)
{
	m_pRemoteFindFile = new CFtpFileFind(pConnection);
	pConnection->GetCurrentDirectoryAsURL(m_strAddress);
}

CFTPFileFind::~CFTPFileFind()
{
	m_pRemoteFindFile->Close();
	delete m_pRemoteFindFile;
}

BOOL CFTPFileFind::FindFile(LPCTSTR RemoteFile, DWORD dwFlags)
{
	try
	{
		BOOL brc;
		brc = m_pRemoteFindFile->FindFile(RemoteFile, dwFlags);
		if(brc == FALSE){
			TRACE("RFindFile ERROR : %s -> %d \n", m_strAddress, ::GetLastError());
			GetErrorMessage();
			return FALSE;
		}
	}
	catch(	CInternetException* ee)
	{
		char buff[256];
		ee->GetErrorMessage(buff, 255);
		TRACE("RFindFile ERROR : %s -> %s \n", m_strAddress, buff);
		ee->Delete();
		return FALSE;
	}

	return TRUE;
}

BOOL CFTPFileFind::FindNextFile()
{
	try
	{
		BOOL brc;
		brc = m_pRemoteFindFile->FindNextFile();
		if(brc == FALSE){
			TRACE("RFindNextFile ERROR : %s -> %d \n", m_strAddress, ::GetLastError());
			GetErrorMessage();
			return FALSE;
		}
	}
	catch(	CInternetException* ee)
	{
		char buff[256];
		ee->GetErrorMessage(buff, 255);
		TRACE("RFindNextFile ERROR : %s -> %s \n", m_strAddress, buff);
		ee->Delete();
		return FALSE;
	}

	return TRUE;
}

CString CFTPFileFind::GetFileURL()
{
	try
	{
		return m_pRemoteFindFile->GetFileURL();
	}
	catch(	CInternetException* ee)
	{
		char buff[256];
		ee->GetErrorMessage(buff, 255);
		TRACE("RFindNextFile ERROR : %s -> %s \n", m_strAddress, buff);
		ee->Delete();
		return "";
	}
}

BOOL CFTPFileFind::GetCreationTime(CTime &Time)
{
	try
	{
		return m_pRemoteFindFile->GetCreationTime(Time);
	}
	catch(	CInternetException* ee)
	{
		char buff[256];
		ee->GetErrorMessage(buff, 255);
		TRACE("RGetCreationTime ERROR : %s -> %s \n", m_strAddress, buff);
		ee->Delete();
		return FALSE;
	}
}

CString CFTPFileFind::GetFileName()
{
	try
	{
		return m_pRemoteFindFile->GetFileName();
	}
	catch(	CInternetException* ee)
	{
		char buff[256];
		ee->GetErrorMessage(buff, 255);
		TRACE("RGetFimeName ERROR : %s -> %s \n", m_strAddress, buff);
		ee->Delete();
		return "";
	}
}

CString CFTPFileFind::GetFilePath()
{
	try
	{
		return m_pRemoteFindFile->GetFilePath();
	}
	catch(	CInternetException* ee)
	{
		char buff[256];
		ee->GetErrorMessage(buff, 255);
		TRACE("RGetFilePath ERROR : %s -> %s \n", m_strAddress, buff);
		ee->Delete();
		return "";
	}
}

CString CFTPFileFind::GetFileTitle()
{
	try
	{
		return m_pRemoteFindFile->GetFileTitle();
	}
	catch(	CInternetException* ee)
	{
		char buff[256];
		ee->GetErrorMessage(buff, 255);
		TRACE("RGetFileTitle ERROR : %s -> %s \n", m_strAddress, buff);
		ee->Delete();
		return "";
	}
}

BOOL CFTPFileFind::GetLastAccessTime(CTime &Time)
{
	try
	{
		return m_pRemoteFindFile->GetLastAccessTime(Time);
	}
	catch(	CInternetException* ee)
	{
		char buff[256];
		ee->GetErrorMessage(buff, 255);
		TRACE("RGetLastAccessTime ERROR : %s -> %s \n", m_strAddress, buff);
		ee->Delete();
		return FALSE;
	}
}

BOOL CFTPFileFind::GetLastWriteTime(CTime &Time)
{
	try
	{
		return m_pRemoteFindFile->GetLastWriteTime(Time);
	}
	catch(	CInternetException* ee)
	{
		char buff[256];
		ee->GetErrorMessage(buff, 255);
		TRACE("RGetLastWriteTime ERROR : %s -> %s \n", m_strAddress, buff);
		ee->Delete();
		return FALSE;
	}
}

DWORD CFTPFileFind::GetLength()
{
	try
	{
		return m_pRemoteFindFile->GetLength();
	}
	catch(	CInternetException* ee)
	{
		char buff[256];
		ee->GetErrorMessage(buff, 255);
		TRACE("RGetLength ERROR : %s -> %s \n", m_strAddress, buff);
		ee->Delete();
		return 0;
	}
}

BOOL CFTPFileFind::IsArchived()
{
	try
	{
		return m_pRemoteFindFile->IsArchived();
	}
	catch(	CInternetException* ee)
	{
		char buff[256];
		ee->GetErrorMessage(buff, 255);
		TRACE("RIsArchived ERROR : %s -> %s \n", m_strAddress, buff);
		ee->Delete();
		return FALSE;
	}
}

BOOL CFTPFileFind::IsCompressed()
{
	try
	{
		return m_pRemoteFindFile->IsCompressed();
	}
	catch(	CInternetException* ee)
	{
		char buff[256];
		ee->GetErrorMessage(buff, 255);
		TRACE("RIsCompressed ERROR : %s -> %s \n", m_strAddress, buff);
		ee->Delete();
		return FALSE;
	}
}

BOOL CFTPFileFind::IsDirectory()
{
	try
	{
		return m_pRemoteFindFile->IsDirectory();
	}
	catch(	CInternetException* ee)
	{
		char buff[256];
		ee->GetErrorMessage(buff, 255);
		TRACE("RIsDirectory ERROR : %s -> %s \n", m_strAddress, buff);
		ee->Delete();
		return FALSE;
	}
}

BOOL CFTPFileFind::IsDots()
{
	try
	{
		return m_pRemoteFindFile->IsDots();
	}
	catch(	CInternetException* ee)
	{
		char buff[256];
		ee->GetErrorMessage(buff, 255);
		TRACE("RIsDots ERROR : %s -> %s \n", m_strAddress, buff);
		ee->Delete();
		return FALSE;
	}
}

BOOL CFTPFileFind::IsHidden()
{
	try
	{
		return m_pRemoteFindFile->IsHidden();
	}
	catch(	CInternetException* ee)
	{
		char buff[256];
		ee->GetErrorMessage(buff, 255);
		TRACE("RIsHidden ERROR : %s -> %s \n", m_strAddress, buff);
		ee->Delete();
		return FALSE;
	}
}

BOOL CFTPFileFind::IsNormal()
{
	try
	{
		return m_pRemoteFindFile->IsNormal();
	}
	catch(	CInternetException* ee)
	{
		char buff[256];
		ee->GetErrorMessage(buff, 255);
		TRACE("LIsNormal ERROR : %s -> %s \n", m_strAddress, buff);
		ee->Delete();
		return FALSE;
	}
}

BOOL CFTPFileFind::IsReadOnly()
{
	try
	{
		return m_pRemoteFindFile->IsReadOnly();
	}
	catch(	CInternetException* ee)
	{
		char buff[256];
		ee->GetErrorMessage(buff, 255);
		TRACE("RIsReadOnly ERROR : %s -> %s \n", m_strAddress, buff);
		ee->Delete();
		return FALSE;
	}
}

BOOL CFTPFileFind::IsSystem()
{
	try
	{
		return m_pRemoteFindFile->IsSystem();
	}
	catch(	CInternetException* ee)
	{
		char buff[256];
		ee->GetErrorMessage(buff, 255);
		TRACE("RIsSystem ERROR : %s -> %s \n", m_strAddress, buff);
		ee->Delete();
		return FALSE;
	}
}

BOOL CFTPFileFind::IsTemporary()
{
	try
	{
		return m_pRemoteFindFile->IsTemporary();
	}
	catch(	CInternetException* ee)
	{
		char buff[256];
		ee->GetErrorMessage(buff, 255);
		TRACE("RIsTemporary ERROR : %s -> %s \n", m_strAddress, buff);
		ee->Delete();
		return FALSE;
	}
}

BOOL CFTPFileFind::MatchsMask(DWORD dwMask)
{
	try
	{
		return m_pRemoteFindFile->MatchesMask(dwMask);
	}
	catch(	CInternetException* ee)
	{
		char buff[256];
		ee->GetErrorMessage(buff, 255);
		TRACE("RMatchsMask ERROR : %s -> %s \n", m_strAddress, buff);
		ee->Delete();
		return FALSE;
	}
}

void CFTPFileFind::GetErrorMessage()
{
	char *szRcode;
	int rCode;

	rCode = ::GetLastError();
	switch(rCode) {
		 case ERROR_ACCESS_DENIED          :    szRcode = "ERROR_ACCESS_DENIED          ";    break;
		 case ERROR_ADAP_HDW_ERR           :    szRcode = "ERROR_ADAP_HDW_ERR           ";    break;
		 case ERROR_ALREADY_ASSIGNED       :    szRcode = "ERROR_ALREADY_ASSIGNED       ";    break;
		 case ERROR_ARENA_TRASHED          :    szRcode = "ERROR_ARENA_TRASHED          ";    break;
		 case ERROR_BAD_COMMAND            :    szRcode = "ERROR_BAD_COMMAND            ";    break;
		 case ERROR_BAD_DEV_TYPE           :    szRcode = "ERROR_BAD_DEV_TYPE           ";    break;
		 case ERROR_BAD_ENVIRONMENT        :    szRcode = "ERROR_BAD_ENVIRONMENT        ";    break;
		 case ERROR_BAD_FORMAT             :    szRcode = "ERROR_BAD_FORMAT             ";    break;
		 case ERROR_BAD_LENGTH             :    szRcode = "ERROR_BAD_LENGTH             ";    break;
		 case ERROR_BAD_NET_NAME           :    szRcode = "ERROR_BAD_NET_NAME           ";    break;
		 case ERROR_BAD_NET_RESP           :    szRcode = "ERROR_BAD_NET_RESP           ";    break;
		 case ERROR_BAD_NETPATH            :    szRcode = "ERROR_BAD_NETPATH            ";    break;
		 case ERROR_BAD_REM_ADAP           :    szRcode = "ERROR_BAD_REM_ADAP           ";    break;
		 case ERROR_BAD_UNIT               :    szRcode = "ERROR_BAD_UNIT               ";    break;
		 case ERROR_CANNOT_MAKE            :    szRcode = "ERROR_CANNOT_MAKE            ";    break;
		 case ERROR_CRC                    :    szRcode = "ERROR_CRC                    ";    break;
		 case ERROR_CURRENT_DIRECTORY      :    szRcode = "ERROR_CURRENT_DIRECTORY      ";    break;
		 case ERROR_DEV_NOT_EXIST          :    szRcode = "ERROR_DEV_NOT_EXIST          ";    break;
		 case ERROR_DUP_NAME               :    szRcode = "ERROR_DUP_NAME               ";    break;
		 case ERROR_FAIL_I24               :    szRcode = "ERROR_FAIL_I24               ";    break;
		 case ERROR_FILE_EXISTS            :    szRcode = "ERROR_FILE_EXISTS            ";    break;
		 case ERROR_FILE_NOT_FOUND         :    szRcode = "ERROR_FILE_NOT_FOUND         ";    break;
		 case ERROR_GEN_FAILURE            :    szRcode = "ERROR_GEN_FAILURE            ";    break;
		 case ERROR_HANDLE_DISK_FULL       :    szRcode = "ERROR_HANDLE_DISK_FULL       ";    break;
		 case ERROR_HANDLE_EOF             :    szRcode = "ERROR_HANDLE_EOF             ";    break;
		 case ERROR_INVALID_ACCESS         :    szRcode = "ERROR_INVALID_ACCESS         ";    break;
		 case ERROR_INVALID_BLOCK          :    szRcode = "ERROR_INVALID_BLOCK          ";    break;
		 case ERROR_INVALID_DATA           :    szRcode = "ERROR_INVALID_DATA           ";    break;
		 case ERROR_INVALID_DRIVE          :    szRcode = "ERROR_INVALID_DRIVE          ";    break;
		 case ERROR_INVALID_FUNCTION       :    szRcode = "ERROR_INVALID_FUNCTION       ";    break;
		 case ERROR_INVALID_HANDLE         :    szRcode = "ERROR_INVALID_HANDLE         ";    break;
		 case ERROR_INVALID_PARAMETER      :    szRcode = "ERROR_INVALID_PARAMETER      ";    break;
		 case ERROR_INVALID_PASSWORD       :    szRcode = "ERROR_INVALID_PASSWORD       ";    break;
		 case ERROR_LOCK_VIOLATION         :    szRcode = "ERROR_LOCK_VIOLATION         ";    break;
		 case ERROR_NET_WRITE_FAULT        :    szRcode = "ERROR_NET_WRITE_FAULT        ";    break;
		 case ERROR_NETNAME_DELETED        :    szRcode = "ERROR_NETNAME_DELETED        ";    break;
		 case ERROR_NETWORK_ACCESS_DENIED  :    szRcode = "ERROR_NETWORK_ACCESS_DENIED  ";    break;
		 case ERROR_NETWORK_BUSY           :    szRcode = "ERROR_NETWORK_BUSY           ";    break;
		 case ERROR_NO_MORE_FILES          :    szRcode = "ERROR_NO_MORE_FILES          ";    break;
		 case ERROR_NO_PROC_SLOTS          :    szRcode = "ERROR_NO_PROC_SLOTS          ";    break;
		 case ERROR_NO_SPOOL_SPACE         :    szRcode = "ERROR_NO_SPOOL_SPACE         ";    break;
		 case ERROR_NOT_DOS_DISK           :    szRcode = "ERROR_NOT_DOS_DISK           ";    break;
		 case ERROR_NOT_ENOUGH_MEMORY      :    szRcode = "ERROR_NOT_ENOUGH_MEMORY      ";    break;
		 case ERROR_NOT_READY              :    szRcode = "ERROR_NOT_READY              ";    break;
		 case ERROR_NOT_SAME_DEVICE        :    szRcode = "ERROR_NOT_SAME_DEVICE        ";    break;
		 case ERROR_NOT_SUPPORTED          :    szRcode = "ERROR_NOT_SUPPORTED          ";    break;
		 case ERROR_OUT_OF_PAPER           :    szRcode = "ERROR_OUT_OF_PAPER           ";    break;
		 case ERROR_OUT_OF_STRUCTURES      :    szRcode = "ERROR_OUT_OF_STRUCTURES      ";    break;
		 case ERROR_OUTOFMEMORY            :    szRcode = "ERROR_OUTOFMEMORY            ";    break;
		 case ERROR_PATH_NOT_FOUND         :    szRcode = "ERROR_PATH_NOT_FOUND         ";    break;
		 case ERROR_PRINT_CANCELLED        :    szRcode = "ERROR_PRINT_CANCELLED        ";    break;
		 case ERROR_PRINTQ_FULL            :    szRcode = "ERROR_PRINTQ_FULL            ";    break;
		 case ERROR_READ_FAULT             :    szRcode = "ERROR_READ_FAULT             ";    break;
		 case ERROR_REDIR_PAUSED           :    szRcode = "ERROR_REDIR_PAUSED           ";    break;
		 case ERROR_REM_NOT_LIST           :    szRcode = "ERROR_REM_NOT_LIST           ";    break;
		 case ERROR_REQ_NOT_ACCEP          :    szRcode = "ERROR_REQ_NOT_ACCEP          ";    break;
		 case ERROR_SECTOR_NOT_FOUND       :    szRcode = "ERROR_SECTOR_NOT_FOUND       ";    break;
		 case ERROR_SEEK                   :    szRcode = "ERROR_SEEK                   ";    break;
		 case ERROR_SHARING_BUFFER_EXCEEDED:    szRcode = "ERROR_SHARING_BUFFER_EXCEEDED";    break;
		 case ERROR_SHARING_PAUSED         :    szRcode = "ERROR_SHARING_PAUSED         ";    break;
		 case ERROR_SHARING_VIOLATION      :    szRcode = "ERROR_SHARING_VIOLATION      ";    break;
		 case ERROR_TOO_MANY_CMDS          :    szRcode = "ERROR_TOO_MANY_CMDS          ";    break;
		 case ERROR_TOO_MANY_NAMES         :    szRcode = "ERROR_TOO_MANY_NAMES         ";    break;
		 case ERROR_TOO_MANY_OPEN_FILES    :    szRcode = "ERROR_TOO_MANY_OPEN_FILES    ";    break;
		 case ERROR_TOO_MANY_SESS          :    szRcode = "ERROR_TOO_MANY_SESS          ";    break;
		 case ERROR_UNEXP_NET_ERR          :    szRcode = "ERROR_UNEXP_NET_ERR          ";    break;
		 case ERROR_WRITE_FAULT            :    szRcode = "ERROR_WRITE_FAULT            ";    break;
		 case ERROR_WRITE_PROTECT          :    szRcode = "ERROR_WRITE_PROTECT          ";    break;
		 case ERROR_WRONG_DISK             :    szRcode = "ERROR_WRONG_DISK             ";    break;
          
		 default 			               :    szRcode = "DEFAULT                      ";    break;
	}           
	TRACE("CFTPFileFind GetLastError() Errocode = %s [%d] \n", szRcode, rCode);
	return;
}

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CFtpClient::CFtpClient()
{
	m_pConnection = NULL;

	m_strLocalDirText.Empty();
	m_strRemoteDirText.Empty();
	m_strAddress.Empty();
}

CFtpClient::~CFtpClient()
{
	if(m_pConnection != NULL){
		m_pConnection->Close();
		delete m_pConnection;
	}
	m_Session.Close();
}

// INTERNET_INVALID_PORT_NUMBER    0           // use the protocol-specific default
// INTERNET_DEFAULT_FTP_PORT       21          // default for FTP servers
BOOL CFtpClient::Open(CString strAddress, CString strName, CString strPassword, INTERNET_PORT nPort)
{
	char buff[256];
	//이미 연결된 것이 있으면 해제
	if(m_pConnection != NULL){
		m_pConnection->Close();
		delete m_pConnection;
		m_pConnection = NULL;
	}

	try
	{
		m_pConnection = m_Session.GetFtpConnection((LPCTSTR)strAddress, (LPCTSTR)strName, (LPCTSTR)strPassword, nPort);
		m_strAddress = strAddress;

		// 현재 Local 디렉토리
		GetCurrentDirectory(255, buff);
		m_strLocalDirText = buff;
		// 현재 Remote 디렉토리
		m_pConnection->GetCurrentDirectory(m_strRemoteDirText);
	}
	catch(	CInternetException* ee)
	{
		ee->GetErrorMessage(buff, 255);
		TRACE("(%s  ==><==  %s)", strAddress, buff);
		m_pConnection = NULL;
		ee->Delete();
		
		return FALSE;
	}
	return TRUE;
}

BOOL CFtpClient::Open(CString strAddress, INTERNET_PORT nPort)
{
	char buff[256];
	//이미 연결된 것이 있으면 해제
	if(m_pConnection != NULL){
		m_pConnection->Close();
		delete m_pConnection;
		m_pConnection = NULL;
	}

	//TRY
	try
	{
		m_pConnection = m_Session.GetFtpConnection((LPCTSTR)strAddress, NULL, NULL, nPort);
		m_strAddress = strAddress;

		// 현재 Local 디렉토리
		GetCurrentDirectory(255, buff);
		m_strLocalDirText = buff;
		// 현재 Remote 디렉토리
		m_pConnection->GetCurrentDirectory(m_strRemoteDirText);
	}
	//CATCH_ALL(ee)
	catch(	CInternetException* ee)
	{
		ee->GetErrorMessage(buff, 255);
		TRACE("Connection ERROR : %s  ==><==  %s \n", strAddress, buff);
		m_pConnection = NULL;
		ee->Delete();
		return FALSE;
	}
	//END_CATCH_ALL

	return TRUE;
}

BOOL CFtpClient::Close()
{
	if(m_pConnection == NULL) return FALSE;
	try
	{
		m_pConnection->Close();
		delete m_pConnection;
		m_pConnection = NULL;
	}
	catch(	CInternetException* ee)
	{
		char buff[256];
		ee->GetErrorMessage(buff, 255);
		TRACE("Close ERROR : %s (%s)", m_strAddress, buff);
		m_pConnection = NULL;
		ee->Delete();
		return FALSE;
	}

	m_strLocalDirText.Empty();
	m_strRemoteDirText.Empty();
	m_strAddress.Empty();

	return TRUE;
}

BOOL CFtpClient::UpLoad(CString strFile)
{
	CString strPathName, strLocalName;

	if(m_pConnection == NULL) return FALSE;

	//디렉토리명과 파일명을 붙이기 
	if(m_strRemoteDirText != "/")	// "/" 이면 안 더해야함
		strPathName = m_strRemoteDirText + "/" + strFile;
	else
		strPathName = m_strRemoteDirText + strFile;

	strLocalName = m_strLocalDirText + "\\" + strFile;
	
	return UpLoad(strLocalName, strPathName);
}

BOOL CFtpClient::UpLoad(CString LocalFile, CString RemoteFile)
{
	CString strPathName;

	if(m_pConnection == NULL) return FALSE;

	try
	{
		BOOL brc;
		brc = m_pConnection->PutFile(LocalFile, RemoteFile);		//파일 업로드
		if(brc == FALSE){
			TRACE("UpLoad ERROR : %s (%s)<==(%s) %d \n", m_strAddress, RemoteFile, LocalFile, ::GetLastError());
			GetErrorMessage();
			return FALSE;
		}
	}
	catch(	CInternetException* ee)
	{
		char buff[256];
		ee->GetErrorMessage(buff, 255);
		TRACE("UpLoad ERROR : %s (%s)<==(%s) %s \n", m_strAddress, RemoteFile, LocalFile, buff);
		ee->Delete();
		return FALSE;
	}

	return TRUE;
}

BOOL CFtpClient::DownLoad(CString strFile)
{
	CString strPathName, strLocalName;

	if(m_pConnection == NULL) return FALSE;

	//디렉토리명과 파일명을 붙이기 
	if(m_strRemoteDirText != "/")	// "/" 이면 안 더해야함
		strPathName = m_strRemoteDirText + "/" + strFile;
	else
		strPathName = m_strRemoteDirText + strFile;

	strLocalName = m_strLocalDirText + "\\" + strFile;
	
	return DownLoad(strPathName, strLocalName);
}

BOOL CFtpClient::DownLoad(CString RemoteFile, CString LocalFile)
{
	if(m_pConnection == NULL) return FALSE;

	try
	{
		BOOL brc;
		brc = m_pConnection->GetFile(RemoteFile, LocalFile);	//파일 다운로드
		if(brc == FALSE){
			TRACE("DownLoad ERROR : %s (%s)==>(%s) %d \n", m_strAddress, RemoteFile, LocalFile, ::GetLastError());
			GetErrorMessage();
			return FALSE;
		}
	}
	catch(	CInternetException* ee)
	{
		char buff[256];
		ee->GetErrorMessage(buff, 255);
		TRACE("DownLoad ERROR : %s (%s)==>(%s) %s \n", m_strAddress, RemoteFile, LocalFile, buff);
		ee->Delete();
		return FALSE;
	}

	return TRUE;
}

BOOL CFtpClient::RCD(CString RemoteDir)
{
	if(m_pConnection == NULL) return FALSE;
	try
	{
		BOOL brc;
		brc = m_pConnection->SetCurrentDirectory((LPCTSTR)RemoteDir);
		if(brc == FALSE){
			TRACE("RCD ERROR : %s (%s) >< %d \n", m_strAddress, RemoteDir, ::GetLastError());
			GetErrorMessage();
			return FALSE;
		}
		else
			m_strRemoteDirText = RemoteDir;
	}
	catch(	CInternetException* ee)
	{
		char buff[256];
		ee->GetErrorMessage(buff, 255);
		TRACE("RCD ERROR : %s (%s) >< %s \n", m_strAddress, RemoteDir, buff);
		ee->Delete();
		return FALSE;
	}

	return TRUE;
}

BOOL CFtpClient::LCD(CString LocalDir)
{
	if(::SetCurrentDirectory(LocalDir) == FALSE)
	{
		GetErrorMessage();
		return FALSE;
	}
	else
		m_strLocalDirText = LocalDir;
	return TRUE;
}

BOOL CFtpClient::RMD(CString RemoteDir)
{
	if(m_pConnection == NULL) return FALSE;
	try
	{
		BOOL brc;
		brc = m_pConnection->CreateDirectory((LPCTSTR)RemoteDir);
		if(brc == FALSE){
			TRACE("RMD ERROR : %s (%s) >< %d \n", m_strAddress, RemoteDir, ::GetLastError());
			GetErrorMessage();
			return FALSE;
		}
	}
	catch(	CInternetException* ee)
	{
		char buff[256];
		ee->GetErrorMessage(buff, 255);
		TRACE("RMD ERROR : %s (%s) >< %s \n", m_strAddress, RemoteDir, buff);
		ee->Delete();
		return FALSE;
	}
	return TRUE;
}

BOOL CFtpClient::LMD(CString LocalDir)
{
	return ::CreateDirectory(LocalDir, NULL);
}

BOOL CFtpClient::RRD(CString RemoteDir)
{
	if(m_pConnection == NULL) return FALSE;
	try
	{
		BOOL brc;
		brc = m_pConnection->RemoveDirectory((LPCTSTR)RemoteDir);
		if(brc == FALSE){
			TRACE("RrD ERROR : %s (%s) >< %d \n", m_strAddress, RemoteDir, ::GetLastError());
			GetErrorMessage();
			return FALSE;
		}
	}
	catch(	CInternetException* ee)
	{
		char buff[256];
		ee->GetErrorMessage(buff, 255);
		TRACE("RrD ERROR : %s (%s) >< %s \n", m_strAddress, RemoteDir, buff);
		ee->Delete();
		return FALSE;
	}
	return TRUE;
}

BOOL CFtpClient::LRD(CString LocalDir)
{
	return ::RemoveDirectory(LocalDir);
}

BOOL CFtpClient::RDel(CString RemoteFile)
{
	if(m_pConnection == NULL) return FALSE;
	try
	{
		BOOL brc;
		brc = m_pConnection->Remove((LPCTSTR)RemoteFile);
		if(brc == FALSE){
			TRACE("RDel ERROR : %s (%s) >< %d \n", m_strAddress, RemoteFile, ::GetLastError());
			GetErrorMessage();
			return FALSE;
		}
	}
	catch(	CInternetException* ee)
	{
		char buff[256];
		ee->GetErrorMessage(buff, 255);
		TRACE("RDel ERROR : %s (%s) >< %s \n", m_strAddress, RemoteFile, buff);
		ee->Delete();
		return FALSE;
	}
	return TRUE;
}

BOOL CFtpClient::LDel(CString LocalFile)
{
	return ::DeleteFile(LocalFile);
}

BOOL CFtpClient::RRen(CString RemoteOldFile, CString RemoteNewFile)
{
	if(m_pConnection == NULL) return FALSE;
	try
	{
		BOOL brc;
		brc = m_pConnection->Rename((LPCTSTR)RemoteOldFile,(LPCTSTR)RemoteNewFile);
		if(brc == FALSE){
			TRACE("RRen ERROR : %s (%s) %s -> %d \n", m_strAddress, RemoteOldFile, RemoteNewFile, ::GetLastError());
			GetErrorMessage();
			return FALSE;
		}
	}
	catch(	CInternetException* ee)
	{
		char buff[256];
		ee->GetErrorMessage(buff, 255);
		TRACE("RRen ERROR : %s (%s) %s -> %s \n", m_strAddress, RemoteOldFile, RemoteNewFile, buff);
		ee->Delete();
		return FALSE;
	}
	return TRUE;
}

BOOL CFtpClient::LRen(CString LocalOldFile, CString LocalNewFile)
{
	return ::MoveFile(LocalOldFile, LocalNewFile);
}

CString CFtpClient::RCD()
{
	RPWD();
	return m_strRemoteDirText;
}

CString CFtpClient::LCD()
{
	LPWD();
	return m_strLocalDirText;
}

BOOL CFtpClient::RPWD()
{
	if(m_pConnection == NULL) return FALSE;
	try
	{
		BOOL brc;
		brc = m_pConnection->GetCurrentDirectory(m_strRemoteDirText);
		if(brc == FALSE){
			TRACE("RPWD ERROR : %s -> %d \n", m_strAddress, ::GetLastError());
			GetErrorMessage();
			return FALSE;
		}
	}
	catch(	CInternetException* ee)
	{
		char buff[256];
		ee->GetErrorMessage(buff, 255);
		TRACE("RPWD ERROR : %s -> %s \n", m_strAddress, buff);
		ee->Delete();
		return FALSE;
	}

	return TRUE;
}

BOOL CFtpClient::LPWD()
{
	char buff[256];
	::GetCurrentDirectory(255, buff);
	m_strLocalDirText = buff;
	return TRUE;
}

BOOL CFtpClient::GetURL(CString &RemoteURL)
{
	if(m_pConnection == NULL) return FALSE;
	try
	{
		BOOL brc;
		brc = m_pConnection->GetCurrentDirectoryAsURL(RemoteURL);
		if(brc == FALSE){
			TRACE("GetURL ERROR : %s -> %d \n", m_strAddress, ::GetLastError());
			GetErrorMessage();
			return FALSE;
		}
	}
	catch(	CInternetException* ee)
	{
		char buff[256];
		ee->GetErrorMessage(buff, 255);
		TRACE("GetURL ERROR : %s -> %s \n", m_strAddress, buff);
		ee->Delete();
		return FALSE;
	}

	return TRUE;
}

void CFtpClient::GetErrorMessage()
{
	char *szRcode;
	int rCode;

	rCode = ::GetLastError();
	switch(rCode) {
		 case ERROR_ACCESS_DENIED          :    szRcode = "ERROR_ACCESS_DENIED          ";    break;
		 case ERROR_ADAP_HDW_ERR           :    szRcode = "ERROR_ADAP_HDW_ERR           ";    break;
		 case ERROR_ALREADY_ASSIGNED       :    szRcode = "ERROR_ALREADY_ASSIGNED       ";    break;
		 case ERROR_ARENA_TRASHED          :    szRcode = "ERROR_ARENA_TRASHED          ";    break;
		 case ERROR_BAD_COMMAND            :    szRcode = "ERROR_BAD_COMMAND            ";    break;
		 case ERROR_BAD_DEV_TYPE           :    szRcode = "ERROR_BAD_DEV_TYPE           ";    break;
		 case ERROR_BAD_ENVIRONMENT        :    szRcode = "ERROR_BAD_ENVIRONMENT        ";    break;
		 case ERROR_BAD_FORMAT             :    szRcode = "ERROR_BAD_FORMAT             ";    break;
		 case ERROR_BAD_LENGTH             :    szRcode = "ERROR_BAD_LENGTH             ";    break;
		 case ERROR_BAD_NET_NAME           :    szRcode = "ERROR_BAD_NET_NAME           ";    break;
		 case ERROR_BAD_NET_RESP           :    szRcode = "ERROR_BAD_NET_RESP           ";    break;
		 case ERROR_BAD_NETPATH            :    szRcode = "ERROR_BAD_NETPATH            ";    break;
		 case ERROR_BAD_REM_ADAP           :    szRcode = "ERROR_BAD_REM_ADAP           ";    break;
		 case ERROR_BAD_UNIT               :    szRcode = "ERROR_BAD_UNIT               ";    break;
		 case ERROR_CANNOT_MAKE            :    szRcode = "ERROR_CANNOT_MAKE            ";    break;
		 case ERROR_CRC                    :    szRcode = "ERROR_CRC                    ";    break;
		 case ERROR_CURRENT_DIRECTORY      :    szRcode = "ERROR_CURRENT_DIRECTORY      ";    break;
		 case ERROR_DEV_NOT_EXIST          :    szRcode = "ERROR_DEV_NOT_EXIST          ";    break;
		 case ERROR_DUP_NAME               :    szRcode = "ERROR_DUP_NAME               ";    break;
		 case ERROR_FAIL_I24               :    szRcode = "ERROR_FAIL_I24               ";    break;
		 case ERROR_FILE_EXISTS            :    szRcode = "ERROR_FILE_EXISTS            ";    break;
		 case ERROR_FILE_NOT_FOUND         :    szRcode = "ERROR_FILE_NOT_FOUND         ";    break;
		 case ERROR_GEN_FAILURE            :    szRcode = "ERROR_GEN_FAILURE            ";    break;
		 case ERROR_HANDLE_DISK_FULL       :    szRcode = "ERROR_HANDLE_DISK_FULL       ";    break;
		 case ERROR_HANDLE_EOF             :    szRcode = "ERROR_HANDLE_EOF             ";    break;
		 case ERROR_INVALID_ACCESS         :    szRcode = "ERROR_INVALID_ACCESS         ";    break;
		 case ERROR_INVALID_BLOCK          :    szRcode = "ERROR_INVALID_BLOCK          ";    break;
		 case ERROR_INVALID_DATA           :    szRcode = "ERROR_INVALID_DATA           ";    break;
		 case ERROR_INVALID_DRIVE          :    szRcode = "ERROR_INVALID_DRIVE          ";    break;
		 case ERROR_INVALID_FUNCTION       :    szRcode = "ERROR_INVALID_FUNCTION       ";    break;
		 case ERROR_INVALID_HANDLE         :    szRcode = "ERROR_INVALID_HANDLE         ";    break;
		 case ERROR_INVALID_PARAMETER      :    szRcode = "ERROR_INVALID_PARAMETER      ";    break;
		 case ERROR_INVALID_PASSWORD       :    szRcode = "ERROR_INVALID_PASSWORD       ";    break;
		 case ERROR_LOCK_VIOLATION         :    szRcode = "ERROR_LOCK_VIOLATION         ";    break;
		 case ERROR_NET_WRITE_FAULT        :    szRcode = "ERROR_NET_WRITE_FAULT        ";    break;
		 case ERROR_NETNAME_DELETED        :    szRcode = "ERROR_NETNAME_DELETED        ";    break;
		 case ERROR_NETWORK_ACCESS_DENIED  :    szRcode = "ERROR_NETWORK_ACCESS_DENIED  ";    break;
		 case ERROR_NETWORK_BUSY           :    szRcode = "ERROR_NETWORK_BUSY           ";    break;
		 case ERROR_NO_MORE_FILES          :    szRcode = "ERROR_NO_MORE_FILES          ";    break;
		 case ERROR_NO_PROC_SLOTS          :    szRcode = "ERROR_NO_PROC_SLOTS          ";    break;
		 case ERROR_NO_SPOOL_SPACE         :    szRcode = "ERROR_NO_SPOOL_SPACE         ";    break;
		 case ERROR_NOT_DOS_DISK           :    szRcode = "ERROR_NOT_DOS_DISK           ";    break;
		 case ERROR_NOT_ENOUGH_MEMORY      :    szRcode = "ERROR_NOT_ENOUGH_MEMORY      ";    break;
		 case ERROR_NOT_READY              :    szRcode = "ERROR_NOT_READY              ";    break;
		 case ERROR_NOT_SAME_DEVICE        :    szRcode = "ERROR_NOT_SAME_DEVICE        ";    break;
		 case ERROR_NOT_SUPPORTED          :    szRcode = "ERROR_NOT_SUPPORTED          ";    break;
		 case ERROR_OUT_OF_PAPER           :    szRcode = "ERROR_OUT_OF_PAPER           ";    break;
		 case ERROR_OUT_OF_STRUCTURES      :    szRcode = "ERROR_OUT_OF_STRUCTURES      ";    break;
		 case ERROR_OUTOFMEMORY            :    szRcode = "ERROR_OUTOFMEMORY            ";    break;
		 case ERROR_PATH_NOT_FOUND         :    szRcode = "ERROR_PATH_NOT_FOUND         ";    break;
		 case ERROR_PRINT_CANCELLED        :    szRcode = "ERROR_PRINT_CANCELLED        ";    break;
		 case ERROR_PRINTQ_FULL            :    szRcode = "ERROR_PRINTQ_FULL            ";    break;
		 case ERROR_READ_FAULT             :    szRcode = "ERROR_READ_FAULT             ";    break;
		 case ERROR_REDIR_PAUSED           :    szRcode = "ERROR_REDIR_PAUSED           ";    break;
		 case ERROR_REM_NOT_LIST           :    szRcode = "ERROR_REM_NOT_LIST           ";    break;
		 case ERROR_REQ_NOT_ACCEP          :    szRcode = "ERROR_REQ_NOT_ACCEP          ";    break;
		 case ERROR_SECTOR_NOT_FOUND       :    szRcode = "ERROR_SECTOR_NOT_FOUND       ";    break;
		 case ERROR_SEEK                   :    szRcode = "ERROR_SEEK                   ";    break;
		 case ERROR_SHARING_BUFFER_EXCEEDED:    szRcode = "ERROR_SHARING_BUFFER_EXCEEDED";    break;
		 case ERROR_SHARING_PAUSED         :    szRcode = "ERROR_SHARING_PAUSED         ";    break;
		 case ERROR_SHARING_VIOLATION      :    szRcode = "ERROR_SHARING_VIOLATION      ";    break;
		 case ERROR_TOO_MANY_CMDS          :    szRcode = "ERROR_TOO_MANY_CMDS          ";    break;
		 case ERROR_TOO_MANY_NAMES         :    szRcode = "ERROR_TOO_MANY_NAMES         ";    break;
		 case ERROR_TOO_MANY_OPEN_FILES    :    szRcode = "ERROR_TOO_MANY_OPEN_FILES    ";    break;
		 case ERROR_TOO_MANY_SESS          :    szRcode = "ERROR_TOO_MANY_SESS          ";    break;
		 case ERROR_UNEXP_NET_ERR          :    szRcode = "ERROR_UNEXP_NET_ERR          ";    break;
		 case ERROR_WRITE_FAULT            :    szRcode = "ERROR_WRITE_FAULT            ";    break;
		 case ERROR_WRITE_PROTECT          :    szRcode = "ERROR_WRITE_PROTECT          ";    break;
		 case ERROR_WRONG_DISK             :    szRcode = "ERROR_WRONG_DISK             ";    break;
          
		 default 			               :    szRcode = "DEFAULT                      ";    break;
	}           
	TRACE("CFtpClient GetLastError() Errocode = %s [%d] \n", szRcode, rCode);
	return;
}

BOOL CFtpClient::UpLoadDir(LPCTSTR lDir, LPCTSTR rDir)
{
	CFileFind finder;
	CString	cFileName, cFileDate, cLDIR, cRDIR;
	CTime time;
	unsigned long size;
	BOOL bWorking;

	cLDIR = lDir;
	if(cLDIR.Right(1) != "\\")
		cLDIR += "\\*.*";
	else
		cLDIR += "*.*";

	bWorking = finder.FindFile(cLDIR);	// 모든화일

	while(bWorking == TRUE)
	{
		bWorking = finder.FindNextFile();	// 화일하나를 찾음

		if(finder.IsDots()) continue;	// "."이나  ".." 이면 다음으로 넘어감

		// 화일이름
		cFileName = finder.GetFileName();	// 화일이름 얻기

		cLDIR = lDir;
		if(cLDIR.Right(1) != "\\")
			cLDIR += "\\";
		cLDIR += cFileName;

		cRDIR = rDir;
		if(cRDIR.Right(1) != "/")
			cRDIR += "/";
		cRDIR += cFileName;

		if(finder.IsDirectory()){
			if(RMD(cRDIR) == FALSE) return FALSE;
			UpLoadDir(cLDIR, cRDIR);
		}
		else {
			// 화일크기
			size = finder.GetLength();
			// 화일날짜
			finder.GetLastWriteTime(time);
			cFileDate = time.Format("%y%m%d %H%M");

			if(UpLoad(cLDIR, cRDIR) == FALSE) return FALSE;
		}
	}
	return TRUE;
}

BOOL CFtpClient::DownLoadDir(LPCTSTR rDir, LPCTSTR lDir)
{
	CFTPFileFind* finder = new CFTPFileFind(m_pConnection);

	CString	cFileName, cFileDate, cLDIR, cRDIR, cJDIR;
	CString cJobDirName;
	BOOL	SubDirJob = FALSE;
	CTime time;
	unsigned long size;
	BOOL bWorking;

	cJDIR = rDir;
	if(cJDIR.Right(1) != "/")
		cJDIR += "/*.*";
	else
		cJDIR += "*.*";

	bWorking = finder->FindFile(cJDIR);	// 모든화일

	while(bWorking == TRUE)
	{
		bWorking = finder->FindNextFile();	// 화일하나를 찾음

		if(finder->IsDots()) continue;	// "."이나  ".." 이면 다음으로 넘어감

		// 화일이름
		cFileName = finder->GetFileName();	// 화일이름 얻기

		if(SubDirJob == TRUE)
			if(cJobDirName != cFileName) continue;
			else{
				SubDirJob = FALSE;
				continue;
			}

		cLDIR = lDir;
		if(cLDIR.Right(1) != "\\")
			cLDIR += "\\";
		cLDIR += cFileName;

		cRDIR = rDir;
		if(cRDIR.Right(1) != "/")
			cRDIR += "/";
		cRDIR += cFileName;

		if(finder->IsDirectory()){
			LMD(cLDIR);
			cJobDirName = cFileName;
			SubDirJob = TRUE;
			delete finder;
			finder = NULL;
			DownLoadDir(cRDIR, cLDIR);
			finder = new CFTPFileFind(m_pConnection);
			bWorking = finder->FindFile(cJDIR);//, INTERNET_FLAG_EXISTING_CONNECT);	// 모든화일
			if(bWorking == FALSE) break;
		}
		else {
			// 화일크기
			size = finder->GetLength();
			// 화일날짜
			finder->GetLastWriteTime(time);
			cFileDate = time.Format("%y%m%d %H%M");

			if(DownLoad(cRDIR, cLDIR) == FALSE) {
				delete finder;
				return FALSE;
			}
		}
	}

	delete finder;
	return TRUE;
}



