#if !defined(AFX_COMM_THREAD_INCLUDED_)
#define AFX_COMM_THREAD_INCLUDED_
#endif

#pragma once
/*
#define DLE		0x10
#define STX		0x02
#define ETX		0x03
*/
#define WM_COMM_READ		WM_USER + 7000
#define BUFF_SIZE			1024


//	��� Ŭ����	CCommThread 

// ��Ʈ���� �б� :
//   ��Ʈ�� �� �Ŀ� ��Ʈ�� �ڷᰡ �����ϸ� WM_COMM_READ �޽����� ���� 
//   �����쿡 ���޵ȴ�. ON_MESSAGE ��ũ�θ� �̿�, �Լ��� �����ϰ�
//   m_ReadData String�� ����� �����͸� �̿� �б�
// ��Ʈ�� ���� : WriteComm(buff, 30)�� ���� ���ۿ� �� ũ�⸦ �ǳ׸� �ȴ�.
class CQueue
{
public:
	BYTE buff[BUFF_SIZE];//ť ���� ������� 4192�� ���� 
	int m_iHead, m_iTail;//ť ���ۿ� �����͸� �ְ� �� �ö� ����� ���� 
	CQueue();
	void Clear();//���۸� �ʱ�ȭ ��Ŵ
	int GetSize();//���� ���ۿ� ����ִ� �������� size�� ����
	BOOL PutByte(BYTE b);//ť���ۿ� 1����Ʈ�� ����
	BOOL GetByte(BYTE *pb);//ť���ۿ��� 1����Ʈ�� �� ��
};

class	CCommThread
{
public:
	
	CCommThread();
	~CCommThread();

	//--------- ȯ�� ���� -----------------------------------------//
	HWND		m_hParentWnd;
	//CWnd*		m_pParentWnd;
	int			m_nCommID;
	//bool		m_bMessageCall;			//bMessageCall�� true�̸� ��Ʈ ������ ������ �޽��� ������� �����ϰ� false�̸� �Լ��� ������� ���ο� �����Ѵ�.
	void		(*m_pCallback_func)(DWORD, char*);

	BOOL        check;
	HANDLE		m_hComm;				// ��� ��Ʈ ���� �ڵ�
	CString		m_sPortName;			// ��Ʈ �̸� (COM1 ..)
	BOOL		m_bConnected;			// ��Ʈ�� ���ȴ��� ������ ��Ÿ��.
	OVERLAPPED	m_osRead, m_osWrite;	// ��Ʈ ���� Overlapped structure
	HANDLE		m_hThreadWatchComm;		// Watch�Լ� Thread �ڵ�.
	WORD		m_wPortID;				// WM_COMM_READ�� �Բ� ������ �μ�.
	CQueue      m_QueueRead;			//ť����

	DWORD		m_tReadData;


	//--------- �ܺ� ��� �Լ� ------------------------------------//
	void		SetParentHwnd( HWND hWnd );	//�޽��� �н����� �̺�Ʈ�� ������ ��� ���.
	void		SetCallbackFunction( void (*p_func)(DWORD, char*) ) { m_pCallback_func = p_func; }	//�ݹ��Լ� �� ������� �̺�Ʈ�� ������ ��� ���.

	void		SetInstanceID( int nCommID );		// �� Ŭ������ 2�� �̻��� �ν��Ͻ��� ����� ��� ���Ž� �����ڷ� ���
	BOOL		OpenPort( CString strPortName, DWORD dwBaud, BYTE byData, BYTE byStop, BYTE byParity );//��Ʈ ���� 
	void		ClosePort();				//��Ʈ �ݱ�
	DWORD		WriteComm(BYTE *pBuff, DWORD nToWrite);//��Ʈ�� ������ ����
	void		ClearBuffer() { m_QueueRead.Clear(); }
	BOOL		IsConnected() { return m_bConnected; }
	CString		GetPortName() { return m_sPortName.Mid(4); }

	//--------- ���� ��� �Լ� ------------------------------------//
	DWORD		ReadComm(BYTE *pBuff, DWORD nToRead);//��Ʈ���� ������ �о����

protected:

};

// Thread�� ����� �Լ� 
DWORD	ThreadWatchComm(CCommThread* pComm);

