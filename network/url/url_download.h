#pragma once

#include <afxwin.h>
#include <afxmt.h>
#include <afxpriv.h>
#include <urlmon.h>

/*
- url�� ������ thread�� �̿��Ͽ� local_file�� �ٿ�ε��ϴ� Ŭ����.

in .h file...
	#include "url_download.h"
	...
	//��������� �������� �ʰ� ���� ������ �����ϸ� thread���� ���� �߻���.
	CURLDownload m_downloader;
	...
	//message handler functions
	afx_msg LRESULT OnEndDownload(WPARAM, LPARAM);
	afx_msg LRESULT OnDisplayStatus(WPARAM, LPARAM lParam);

in .cpp file...
BEGIN_MESSAGE_MAP�� �� �̺�Ʈ ó�� �Լ� ���.
	ON_MESSAGE(WM_USER_URL_DOWNLOAD_COMPLETED, &CHanabank_PlayerDlg::OnURLDownloadCompleted)
	ON_MESSAGE(WM_USER_URL_DOWNLOAD_STATUS, &CHanabank_PlayerDlg::OnURLDownloadStatus)
	...
	OnDisplayStatus()�Լ����� DOWNLOADSTATUS ����ü ���� �̿��Ͽ� �ٿ�ε� ���°� ǥ��.
*/

#define WM_USER_URL_DOWNLOAD_COMPLETED	(WM_USER + 1)
#define WM_USER_URL_DOWNLOAD_STATUS		(WM_USER + 2)

enum
{
	UF_BINDSTATUS_FIRST = BINDSTATUS_FINDINGRESOURCE,
	UF_BINDSTATUS_LAST = BINDSTATUS_ACCEPTRANGES
};


class CURLDownload
{
public:
	CURLDownload();

	struct DOWNLOADSTATUS
	{
		ULONG ulProgress;
		ULONG ulProgressMax;
		ULONG ulStatusCode;
		LPCWSTR szStatusText;
	};

	void download(HWND hWnd, CString url, CString local_file);
	void thread_download();

protected:
	HWND m_hParent = NULL;
	CString m_url;
	CString m_local_file;
	CEvent m_eventStop;
};

class CBSCallbackImpl : public IBindStatusCallback
{
public:
	CBSCallbackImpl(HWND hWnd, HANDLE hEventStop);

	// IUnknown methods
	STDMETHOD(QueryInterface)(REFIID riid, void **ppvObject);
	STDMETHOD_(ULONG, AddRef)();
	STDMETHOD_(ULONG, Release)();

	// IBindStatusCallback methods
	STDMETHOD(OnStartBinding)(DWORD, IBinding *);
	STDMETHOD(GetPriority)(LONG *);
	STDMETHOD(OnLowResource)(DWORD);
	STDMETHOD(OnProgress)(ULONG ulProgress,
		ULONG ulProgressMax,
		ULONG ulStatusCode,
		LPCWSTR szStatusText);
	STDMETHOD(OnStopBinding)(HRESULT, LPCWSTR);
	STDMETHOD(GetBindInfo)(DWORD *, BINDINFO *);
	STDMETHOD(OnDataAvailable)(DWORD, DWORD, FORMATETC *, STGMEDIUM *);
	STDMETHOD(OnObjectAvailable)(REFIID, IUnknown *);

protected:
	ULONG m_ulObjRefCount;

private:
	HWND m_hWnd;
	HANDLE m_hEventStop;
};
