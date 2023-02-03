#pragma once

/*
webView2 컴포넌트를 사용하기 위한 단계가 복잡하여
CWnd를 상속받은 Custom Control에 webView2가 표시되도록 CWebView2Ctrl 제작.

[사용 방법]
- 이 컨트롤을 사용하고자 하는 프로젝트에서 NuGet 패키지 관리자를 열고
  다음 2개의 패키지를 설치.
  Microsoft.Web.WebView2 (VS2015에서 최신 버전 추가시 에러가 발생하여 1.0.622.22로 설치, VS2022에서는 최신 버전 문제없음)
  추후 다시 1.0.1293.44로 업그레이드
  Microsoft.Windows.ImplementationLibrary (최신 버전 설치해도 문제 없음)

- 프로젝트에 다음 5개의 파일 추가.(복사가 아님)
  WebView2Ctrl.cpp
  WebView2Ctrl.h
  ViewComponent.cpp
  ViewComponent.h
  ComponentBase.h

  위의 파일들 중 2개의 cpp는 속성->C/C++->미리 컴파일된 헤더 : "미리 컴파일된 헤더 사용 안 함"으로 설정

- mainDlg에 Custom Control을 추가하고 클래스 이름은 CWebView2Ctrl로 입력.
- 위 컨트롤에 CWebView2Ctrl타입의 제어 변수(m_web)를 선언.

- #include "../../Common/webView2Ctrl/WebView2Ctrl.h" 자동 추가되지 않았다면 수동 입력.
  (주의! 이 WebView2Ctrl.h를 다른 include보다 뒤에 선언할 경우
   Microsoft.Windows.ImplementationLibrary의 wil/resource.h에서 컴파일 오류가 발생한다.)

- 사용 : m_web.navigate(url);
- navigate이외의 다른 함수를 사용하고자 한다면 m_web.GetWebView()->Navigate(...)처럼 접근하여 호출.
- mainDlg의 크기가 바뀌면 m_web.MoveWindow(...)로 크기 조정

* WebView2Loader.dll이 없어도 실행되게 하려면(Static build로 변경)
  $(ProjectDir)\packages\Microsoft.Web.WebView2.1.0.xxxx.xx\build 폴더내에
  Common.targets 파일이 있고 그 파일에서 WebView2LoaderPreference 태그를 찾아 "Static"으로 변경한다.
  (버전에 따라 해당 태그의 위치와 구분은 약간씩 다를 수 있음)


[하나은행 메모]
- C://Program Files (x86)/UCTogether 폴더에서 웹뷰가 포함된 앱을 실행시킬때는
  반드시 InitializeWebView()의 CreateCoreWebView2EnvironmentWithOptions()호출 시	m_userDataFolder를 줘야만 실행된다.
  그렇지 않으면 앱이 아예 실행조차 안됨.

[NH투자신탁 메모]
- n개의 cam중에 특정 캠을 선택하여 웹뷰에 표시하는 기능을 수행할 때(cam_capture.html)
  해당 html을 파일로 열면 카메라 선택 코드가 동작하지 않는 경우가 있다.
  ATEC에서 localhost로 웹서버를 구동시키고 http://localhost/..../cam_capture.html로 해야 장치 선택 가능.
  (물론 웹뷰2에서도 카메라, 마이크 등에 대한 접근 권한을 항상 허용으로 설정한 상태여야 한다.)
- html상에서 wav 파일을 재생할때도 이와 동일한 문제가 발생하여 가상 웹서버에서 loading하도록 하여 해결.
- 한글 파일명의 gif가 html 에서 표시되지 않는 문제 발생 -> ATEC에서 웹서버의 encoding을 변경하여 해결.
- 1.0.1293.44를 사용했으나 문서공유창을 닫을 때 dlg가 죽는 현상이 발생하여
  기기에 설치된 웹뷰런타임 버전인 1.0.1185.36보다 낮은 1150.38로 낮춤.
  
  //2023013118
  add_PermissionRequested()때문에 카메라 영상이 안나오는 것인지 확인하기 위해
  이 API가 지원되는 1185.39로 다시 올려서 테스트 시작. 역시 카메라 안나옴.
  한비전에서 카메라 등 장치 권한허용이 누락되어 수정 후 다시 영상 나옴.

[수정사항]
*20230122
	- 이 컨트롤을 포함하는 dlg 또는 이 컨트롤이 hide상태로 시작되면
	  html이 제대로 로딩되지 않는 문제점 수정.
	- navigate_completed, download_completed등의 이벤트 발생시에 해당 메시지를 main으로 보내 처리하도록 수정.
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

	//카메라와 마이크에 대한 허용을 묻는 팝업에 대한 설정 변경
	//mode = 0(default), 1(allow), 2(deny)
	void set_permission_request_mode(int mode);
	//아직 webview2가 완전히 생성되지 않은 상태에서 호출하면 ""이 리턴됨에 주의.
	CString get_default_download_path() { return CString(m_default_download_path); }

	HRESULT execute_jscript(CString jscript);
	HRESULT post_web_message_as_json(CString json);

	CWebView2Ctrl();
	~CWebView2Ctrl();

	//webView_version : get webView version = true, get Edge version = false
	//CString get_webview2_runtime_version(bool webView_version = true);

	//environment를 통해 간단히 얻어올 수 있다.
	CString get_webview2_runtime_version() { return m_webview2_runtime_version; }
	//현재 프로젝트에 설치된 NuGet package의 WebView2 버전을 구한다.
	CString get_webview2_nuget_version() { return m_webview2_nuget_version; }

	//가장 많이 사용하는 함수라서 추가했으나
	//main의 m_web 인스턴스에서 CWebView2Ctrl의 m_webView의 다른 기능을 이용하고자 한다면
	//m_web.GetWebView()->Navigate(...)과 같이 접근하여 이용하면 된다.
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
	//일반적으로는 리소스 뷰의 dlg editor에서 custom control로 정적 생성해서 사용하게 되는데
	//부득이하게 동적 생성이 필요한 경우는 m_create_static을 false로 하여 생성하면 된다.
	//이 값을 false로 하지 않으면 PreCreateWindow와 CWebView2Ctrl::Create~함수에서
	//InitializeWebView()가 중복호출되는데 Create~함수에서 이를 제거하여 한번만 호출되게 해도 안되어
	//우선 이 변수로 동적, 정적 생성을 구분하여 사용한다.
	bool	m_create_static = true;
	HWND m_webHwnd = NULL;
	void InitializeWebView();
	void CloseWebView(bool cleanupUserDataFolder = false);
	HRESULT OnCreateEnvironmentCompleted(HRESULT result, ICoreWebView2Environment* environment);
	HRESULT OnCreateCoreWebView2ControllerCompleted(HRESULT result, ICoreWebView2Controller* controller);
	HRESULT DCompositionCreateDevice2(IUnknown* renderingDevice, REFIID riid, void** ppv);
	HRESULT WebMessageReceived(ICoreWebView2* sender, ICoreWebView2WebMessageReceivedEventArgs* args);
	HRESULT ExecuteScriptResponse(HRESULT errorCode, LPCWSTR result);

	//cx 또는 cy가 0이면 parent의 크기에 맞춤
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

	//html에서 카메라 사용시 접근 권한을 매번 물어보는 문제때문에
	//이를 항상 허용으로 해주기 위해 추가한 코드
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
	//nuget 버전이 runtime버전보다 높으면 일부 API는 사용 불가하고 앱이 실행되지 않을 수 있다.	
	CString m_webview2_nuget_version = _T("");

	//카메라와 마이크에 대한 허용을 묻는 팝업에 대한 설정 변경
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
