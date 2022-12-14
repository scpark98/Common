#pragma once

#include <afxwin.h>
#include <afxmt.h>
#include <afxpriv.h>
#include <urlmon.h>

/*
- url의 파일을 thread를 이용하여 local_file로 다운로드하는 클래스.

in .h file...
	#include "url_download.h"
	...
	//멤버변수로 선언하지 않고 로컬 변수로 선언하면 thread에서 에러 발생함.
	CURLDownload m_downloader;
	...
	//message handler functions
	afx_msg LRESULT OnEndDownload(WPARAM, LPARAM);
	afx_msg LRESULT OnDisplayStatus(WPARAM, LPARAM lParam);

in .cpp file...
BEGIN_MESSAGE_MAP에 두 이벤트 처리 함수 등록.
	ON_MESSAGE(WM_USER_URL_DOWNLOAD_COMPLETED, &CHanabank_PlayerDlg::OnURLDownloadCompleted)
	ON_MESSAGE(WM_USER_URL_DOWNLOAD_STATUS, &CHanabank_PlayerDlg::OnURLDownloadStatus)
	...
	OnDisplayStatus()함수에서 DOWNLOADSTATUS 구조체 값을 이용하여 다운로드 상태값 표시.
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

	void use_cache(bool use) { m_use_cache = use; }
	void download(HWND hWnd, CString url, CString local_file);
	void thread_download();

	CString get_url() { return m_url; }
	CString get_local_file() { return m_local_file; }

protected:
	HWND m_hParent = NULL;
	bool m_use_cache = true;	//인터넷 캐시 폴더에 받을지, 다운받는 파일을 지정할 지
	CString m_url;				//다운받을 url 주소
	CString m_local_file;		//다운받은 로컬 파일 fullpath. m_use_cache가 false이면 fullpath가 반드시 주어져야 한다.
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
