#include <afxinet.h>

#define FTP_RECEIVED_DATA			0
#define FTP_SENT_DATA				1
#define MESSAGE_FTP_TRANSFER_DATA	WM_USER + 200

class CFtpThread
{
public:
	CFtpThread();
	~CFtpThread();

	//전송 상태를 전달하기 위한 parent윈도우 핸들 명시
	void		SetParentHwnd( HWND hParentWnd ) { m_hParentWnd = hParentWnd; }

	//
	BOOL		Connect( CString sServerIP, CString sID, CString sPass, int nPort = 21, BOOL bPassive = FALSE );
	BOOL		IsConnected() {	return ( m_hConnect != NULL ); }
	void		CloseConnection();

	BOOL		GetFile( CString sRemote, CString sLocal );
	BOOL		PutFile( CString sLocal, CString sRemote );

private:
	HINTERNET	m_hInternet;
	HINTERNET	m_hConnect;
	HWND		m_hParentWnd;
};
