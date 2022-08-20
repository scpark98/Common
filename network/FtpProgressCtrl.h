#pragma once


// CFtpProgressCtrl
#include <afxinet.h>

#define FTP_RECEIVED_DATA					0
#define FTP_SENT_DATA						1
#define FTP_TRANSFER_RETRY					2
#define FTP_ERROR_CONNECT_FAIL				-1
#define FTP_ERROR_REMOTE_FILE_NOT_EXIST		-2
#define FTP_ERROR_REMOTE_FILE_OPEN_FAIL		-3
#define FTP_ERROR_LOCAL_FILE_CREATE_FAIL	-4

#define MESSAGE_FTP_TRANSFER_DATA	WM_USER + 200

#define FTP_BUFFER_SIZE				40960

class CFtpProgressCtrl : public CProgressCtrl
{
	DECLARE_DYNAMIC(CFtpProgressCtrl)

public:
	CFtpProgressCtrl();
	virtual ~CFtpProgressCtrl();

	//전송 상태를 전달하기 위한 parent윈도우 핸들 명시
	BOOL		Connect( HWND hParentWnd, CString sServerIP, CString sID, CString sPass, int nPort = 21, BOOL bPassive = FALSE );
	BOOL		IsConnected() {	return ( m_pFtpConnection != NULL ); }
	void		AbortDownloading( BOOL bAbortDownloading );

	// only one file
	BOOL		GetFile( CString remoteFile, CString localFile, BOOL bSendMessage = TRUE );
	BOOL		PutFile( CString localFile, CString remoteFile, BOOL bSendMessage = TRUE );


	// close the connection
	BOOL		CloseConnection();

protected:
	DECLARE_MESSAGE_MAP()

private:
	CInternetSession*	m_pInternetSession;
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
	BOOL				m_bSendMessage;


	CString				m_sRemoteFile;
	CString				m_sLocalFile;
	DWORD				m_nTotalDownloaded;
public:
	afx_msg void OnTimer(UINT_PTR nIDEvent);
};


