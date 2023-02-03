#pragma once

/*
webView2 ������Ʈ�� ����ϱ� ���� �ܰ谡 �����Ͽ�
CWnd�� ��ӹ��� Custom Control�� webView2�� ǥ�õǵ��� CWebView2Ctrl ����.

[��� ���]
- �� ��Ʈ���� ����ϰ��� �ϴ� ������Ʈ���� NuGet ��Ű�� �����ڸ� ����
  ���� 2���� ��Ű���� ��ġ.
  Microsoft.Web.WebView2 (VS2015���� �ֽ� ���� �߰��� ������ �߻��Ͽ� 1.0.622.22�� ��ġ, VS2022������ �ֽ� ���� ��������)
  ���� �ٽ� 1.0.1293.44�� ���׷��̵�
  Microsoft.Windows.ImplementationLibrary (�ֽ� ���� ��ġ�ص� ���� ����)

- ������Ʈ�� ���� 5���� ���� �߰�.(���簡 �ƴ�)
  WebView2Ctrl.cpp
  WebView2Ctrl.h
  ViewComponent.cpp
  ViewComponent.h
  ComponentBase.h

  ���� ���ϵ� �� 2���� cpp�� �Ӽ�->C/C++->�̸� �����ϵ� ��� : "�̸� �����ϵ� ��� ��� �� ��"���� ����

- mainDlg�� Custom Control�� �߰��ϰ� Ŭ���� �̸��� CWebView2Ctrl�� �Է�.
- �� ��Ʈ�ѿ� CWebView2CtrlŸ���� ���� ����(m_web)�� ����.

- #include "../../Common/webView2Ctrl/WebView2Ctrl.h" �ڵ� �߰����� �ʾҴٸ� ���� �Է�.
  (����! �� WebView2Ctrl.h�� �ٸ� include���� �ڿ� ������ ���
   Microsoft.Windows.ImplementationLibrary�� wil/resource.h���� ������ ������ �߻��Ѵ�.)

- ��� : m_web.navigate(url);
- navigate�̿��� �ٸ� �Լ��� ����ϰ��� �Ѵٸ� m_web.GetWebView()->Navigate(...)ó�� �����Ͽ� ȣ��.
- mainDlg�� ũ�Ⱑ �ٲ�� m_web.MoveWindow(...)�� ũ�� ����

* WebView2Loader.dll�� ��� ����ǰ� �Ϸ���(Static build�� ����)
  $(ProjectDir)\packages\Microsoft.Web.WebView2.1.0.xxxx.xx\build ��������
  Common.targets ������ �ְ� �� ���Ͽ��� WebView2LoaderPreference �±׸� ã�� "Static"���� �����Ѵ�.
  (������ ���� �ش� �±��� ��ġ�� ������ �ణ�� �ٸ� �� ����)


[�ϳ����� �޸�]
- C://Program Files (x86)/UCTogether �������� ���䰡 ���Ե� ���� �����ų����
  �ݵ�� InitializeWebView()�� CreateCoreWebView2EnvironmentWithOptions()ȣ�� ��	m_userDataFolder�� ��߸� ����ȴ�.
  �׷��� ������ ���� �ƿ� �������� �ȵ�.

[NH���ڽ�Ź �޸�]
- n���� cam�߿� Ư�� ķ�� �����Ͽ� ���信 ǥ���ϴ� ����� ������ ��(cam_capture.html)
  �ش� html�� ���Ϸ� ���� ī�޶� ���� �ڵ尡 �������� �ʴ� ��찡 �ִ�.
  ATEC���� localhost�� �������� ������Ű�� http://localhost/..../cam_capture.html�� �ؾ� ��ġ ���� ����.
  (���� ����2������ ī�޶�, ����ũ � ���� ���� ������ �׻� ������� ������ ���¿��� �Ѵ�.)
- html�󿡼� wav ������ ����Ҷ��� �̿� ������ ������ �߻��Ͽ� ���� ���������� loading�ϵ��� �Ͽ� �ذ�.
- �ѱ� ���ϸ��� gif�� html ���� ǥ�õ��� �ʴ� ���� �߻� -> ATEC���� �������� encoding�� �����Ͽ� �ذ�.
- 1.0.1293.44�� ��������� ��������â�� ���� �� dlg�� �״� ������ �߻��Ͽ�
  ��⿡ ��ġ�� ���䷱Ÿ�� ������ 1.0.1185.36���� ���� 1150.38�� ����.
  
  //2023013118
  add_PermissionRequested()������ ī�޶� ������ �ȳ����� ������ Ȯ���ϱ� ����
  �� API�� �����Ǵ� 1185.39�� �ٽ� �÷��� �׽�Ʈ ����. ���� ī�޶� �ȳ���.
  �Ѻ������� ī�޶� �� ��ġ ��������� �����Ǿ� ���� �� �ٽ� ���� ����.

[��������]
*20230122
	- �� ��Ʈ���� �����ϴ� dlg �Ǵ� �� ��Ʈ���� hide���·� ���۵Ǹ�
	  html�� ����� �ε����� �ʴ� ������ ����.
	- navigate_completed, download_completed���� �̺�Ʈ �߻��ÿ� �ش� �޽����� main���� ���� ó���ϵ��� ����.
*/

#include <afxwin.h>
#include <wrl.h>
#include <wil/com.h>

#include "WebView2EnvironmentOptions.h"
#include "WebView2.h"

#include "ComponentBase.h"
#include <dcomp.h>
#include <EventToken.h>
#include <functional>
#include <memory>
//#include <ole2.h>
//#include <string>
#include <vector>
#include <map>
//#include <winnt.h>

#ifdef __windows__
#undef __windows__
#endif

#ifdef USE_WEBVIEW2_WIN10
#include <winrt/Windows.UI.Composition.h>
#include <winrt/Windows.UI.ViewManagement.h>
#include "afxwin.h"
namespace winrtComp = winrt::Windows::UI::Composition;
#endif

#define IDM_GET_BROWSER_VERSION_AFTER_CREATION 170
#define IDM_GET_BROWSER_VERSION_BEFORE_CREATION 171
#define IDM_CREATION_MODE_TARGET_DCOMP 195

class CWebView2Ctrl : public CWnd
{
	DECLARE_DYNAMIC(CWebView2Ctrl)

	BOOL CreateHostWindow(
		LPCTSTR lpszClassName,
		LPCTSTR lpszWindowName,
		DWORD dwStyle,
		const RECT& rect,
		CWnd* pParentWnd,
		UINT nID);

	BOOL CWebView2Ctrl::Create(
		LPCTSTR lpszClassName,
		LPCTSTR lpszWindowName,
		DWORD dwStyle,
		const RECT& rect,
		CWnd* pParentWnd,
		UINT nID,
		CCreateContext*);

	BOOL CWebView2Ctrl::CreateAsync(
		DWORD dwStyle,
		const RECT& rect,
		CWnd* pParentWnd,
		UINT nID);


protected:
	BOOL RegisterWindowClass();

public:
	enum TIMER
	{
		timer_reload_due_to_process_exited = 0,
	};

	enum WEBVIEW2_MESSAGE
	{
		webview2_message_create_completed = WM_USER + 562,
		webview2_message_navigation_start,
		webview2_message_navigation_completed,
		webview2_message_document_title_changed,
		webview2_message_download_completed,
		webview2_message_web_message_received,
		webview2_message_reload_due_to_process_exited,
	};

	void on_navigation_start();
	void on_navigation_completed();
	void on_document_title_changed();

	//ī�޶�� ����ũ�� ���� ����� ���� �˾��� ���� ���� ����
	//mode = 0(default), 1(allow), 2(deny)
	void set_permission_request_mode(int mode);
	//���� webview2�� ������ �������� ���� ���¿��� ȣ���ϸ� ""�� ���ϵʿ� ����.
	CString get_default_download_path() { return CString(m_default_download_path); }

	HRESULT execute_jscript(CString jscript);
	HRESULT post_web_message_as_json(CString json);

	CWebView2Ctrl();
	~CWebView2Ctrl();

	//webView_version : get webView version = true, get Edge version = false
	//CString get_webview2_runtime_version(bool webView_version = true);

	//environment�� ���� ������ ���� �� �ִ�.
	CString get_webview2_runtime_version() { return m_webview2_runtime_version; }
	//���� ������Ʈ�� ��ġ�� NuGet package�� WebView2 ������ ���Ѵ�.
	CString get_webview2_nuget_version() { return m_webview2_nuget_version; }

	//���� ���� ����ϴ� �Լ��� �߰�������
	//main�� m_web �ν��Ͻ����� CWebView2Ctrl�� m_webView�� �ٸ� ����� �̿��ϰ��� �Ѵٸ�
	//m_web.GetWebView()->Navigate(...)�� ���� �����Ͽ� �̿��ϸ� �ȴ�.
	void	navigate(CString url = _T("about:blank"), bool url_normalize = true);
	// The raw request header string delimited by CRLF(optional in last header).
	void	navigate_post(CString const& url, CString const& content, CString const& headers, std::function<void()> onComplete);
	void	print_document();
	void	Stop();
	void	Reload();
	void	GoBack();
	void	GoForward();
	void	DisablePopup();

	CString get_url();
	CString normalize_url(CString url);
	void	hide_download_dialog();
	void	allow_external_drop(bool allow = true) { m_allow_external_drop = allow; };

	static bool m_clear_cache_on_created;
	//static void	clear_cache_on_created(bool clear) { CWebView2Ctrl::m_clear_cache_on_created = clear; }

	//WebView2
	//�Ϲ������δ� ���ҽ� ���� dlg editor���� custom control�� ���� �����ؼ� ����ϰ� �Ǵµ�
	//�ε����ϰ� ���� ������ �ʿ��� ���� m_create_static�� false�� �Ͽ� �����ϸ� �ȴ�.
	//�� ���� false�� ���� ������ PreCreateWindow�� CWebView2Ctrl::Create~�Լ�����
	//InitializeWebView()�� �ߺ�ȣ��Ǵµ� Create~�Լ����� �̸� �����Ͽ� �ѹ��� ȣ��ǰ� �ص� �ȵǾ�
	//�켱 �� ������ ����, ���� ������ �����Ͽ� ����Ѵ�.
	bool	m_create_static = true;
	HWND m_webHwnd = NULL;
	void InitializeWebView();
	void CloseWebView(bool cleanupUserDataFolder = false);
	HRESULT OnCreateEnvironmentCompleted(HRESULT result, ICoreWebView2Environment* environment);
	HRESULT OnCreateCoreWebView2ControllerCompleted(HRESULT result, ICoreWebView2Controller* controller);
	HRESULT DCompositionCreateDevice2(IUnknown* renderingDevice, REFIID riid, void** ppv);
	HRESULT WebMessageReceived(ICoreWebView2* sender, ICoreWebView2WebMessageReceivedEventArgs* args);
	HRESULT ExecuteScriptResponse(HRESULT errorCode, LPCWSTR result);

	//cx �Ǵ� cy�� 0�̸� parent�� ũ�⿡ ����
	void	resize(int cx = 0, int cy = 0);
	RECT	get_rect();


	EventRegistrationToken m_navigationStartingToken = {};
	EventRegistrationToken m_navigationCompletedToken = {};
	EventRegistrationToken m_documentTitleChangedToken = {};
	EventRegistrationToken m_permissionRequestedToken = {};
	EventRegistrationToken m_downloadStartingToken = {};
	EventRegistrationToken m_stateChangedToken = {};
	EventRegistrationToken m_processFailedToken = {};
	void RegisterEventHandlers();
	void UpdateProgress(ICoreWebView2DownloadOperation* download);


	ICoreWebView2Controller* GetWebViewController()
	{
		return m_controller.get();
	}
	ICoreWebView2* GetWebView()
	{
		if (m_webView)
			return m_webView.get();

		return nullptr;
	}
	ICoreWebView2Environment* GetWebViewEnvironment()
	{
		return m_webViewEnvironment.get();
	}
	HWND GetMainWindow()
	{
		return this->GetSafeHwnd();
	}

	//html���� ī�޶� ���� ���� ������ �Ź� ����� ����������
	//�̸� �׻� ������� ���ֱ� ���� �߰��� �ڵ�
	/*
	HRESULT STDMETHODCALLTYPE Invoke(ICoreWebView2* sender, ICoreWebView2PermissionRequestedEventArgs* args)
	{
		COREWEBVIEW2_PERMISSION_KIND kind;
		args->get_PermissionKind(&kind);
		if (kind == COREWEBVIEW2_PERMISSION_KIND_CAMERA || kind == COREWEBVIEW2_PERMISSION_KIND_MICROPHONE)
		{
			args->put_State(COREWEBVIEW2_PERMISSION_STATE_ALLOW);
		}
		return S_OK;
	}

	ULONG STDMETHODCALLTYPE AddRef() { return 1; }
	ULONG STDMETHODCALLTYPE Release() { return 1; }
	HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, LPVOID* ppv)
	{
		return S_OK;
	}
	*/

	//WebView2

protected:
	CString m_url_reserved = _T("");
	CString m_document_title = _T("");
	CString m_webview2_runtime_version = _T("");					//107.0.1418.62
	//nuget ������ runtime�������� ������ �Ϻ� API�� ��� �Ұ��ϰ� ���� ������� ���� �� �ִ�.	
	CString m_webview2_nuget_version = _T("");

	//ī�޶�� ����ũ�� ���� ����� ���� �˾��� ���� ���� ����
	//mode = 0(default), 1(allow), 2(deny)
	COREWEBVIEW2_PERMISSION_STATE m_permission_request_mode = COREWEBVIEW2_PERMISSION_STATE_DEFAULT;

	//WebView2
	DWORD m_creationModeId = 0;
	wil::com_ptr<ICoreWebView2Environment> m_webViewEnvironment;
	wil::com_ptr<ICoreWebView2Environment2> m_webViewEnvironment2;	//for CreateWebResourceRequest()
	wil::com_ptr<ICoreWebView2Settings> m_webSettings;
	wil::com_ptr<ICoreWebView2Controller> m_controller;
	wil::com_ptr<ICoreWebView2> m_webView = nullptr;
	wil::com_ptr<IDCompositionDevice> m_dcompDevice;
	//wil::com_ptr<ICoreWebView2Profile> m_profile;		//Applies to 1.0.1245.22
	//wil::com_ptr< ICoreWebView2DownloadOperation> m_download;

	bool m_allow_external_drop = false;
	LPWSTR m_default_download_path;
	//ICoreWebView2Profile* m_profile;
	//ICoreWebView2PermissionRequestedEventHandler* m_permissionRequestedEvent;
	std::vector<std::unique_ptr<ComponentBase>> m_components;
	HWND m_mainWindow = nullptr;
	HINSTANCE g_hInstance;
	static constexpr size_t s_maxLoadString = 100;
	template <class ComponentType, class... Args> void NewComponent(Args&&... args)
	{
		m_components.emplace_back(new ComponentType(std::forward<Args>(args)...));
	}

	template <class ComponentType> ComponentType* GetComponent()
	{
		for (auto& component : m_components)
		{
			if (auto wanted = dynamic_cast<ComponentType*>(component.get()))
			{
				return wanted;
			}
		}
		return nullptr;
	}

	//WebView2

public:
	DECLARE_MESSAGE_MAP()
	virtual void PreSubclassWindow();
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnDropFiles(HDROP hDropInfo);
	afx_msg void OnTimer(UINT_PTR nIDEvent);
};
