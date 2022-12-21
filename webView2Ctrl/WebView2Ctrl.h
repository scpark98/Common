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
  (이 WebView2Ctrl.h를 다른 include보다 뒤에 선언할 경우
   Microsoft.Windows.ImplementationLibrary의 wil/resource.h에서 컴파일 오류가 발생한다.)

- 사용 : m_web.navigate(url);
- navigate이외의 다른 함수를 사용하고자 한다면 m_web.GetWebView()->Navigate(...)처럼 접근하여 호출.
- mainDlg의 크기가 바뀌면 m_web.MoveWindow(...)로 크기 조정

[하나은행 메모]
- C://Program Files (x86)/UCTogether 폴더에서 웹뷰가 포함된 앱을 실행시킬때는
  반드시 InitializeWebView()의 CreateCoreWebView2EnvironmentWithOptions()호출 시	m_userDataFolder를 줘야만 실행된다.
  그렇지 않으면 앱이 아예 실행조차 안됨.

[NH투자신탁 메모]
- n개의 cam중에 특정 캠을 선택하여 웹뷰에 표시하는 기능을 수행할 때(cam_capture.html)
  해당 html을 파일로 열면 카메라 선택 코드가 동작하지 않는다.
  ATEC에서 localhost로 웹서버를 구동시키고 http://localhost/cam_capture.html로 해야 장치 선택 가능.
  (물론 웹뷰2에서도 카메라, 마이크 등에 대한 접근 권한을 항상 허용으로 설정한 상태여야 한다.)
- html상에서 wav 파일을 재생할때도 이와 동일한 문제가 발생했다.
- 한글 파일명의 gif가 html 에서 표시되지 않는 문제 발생 -> ATEC에서 웹서버의 encoding을 변경하여 해결.
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

enum WEBVIEW2_MESSAGE
{
	webview2_message_navigation_start = WM_APP + 123,
	webview2_message_navigation_completed,
	webview2_message_document_title_changed,
	webview2_message_
};

class CWebView2Ctrl : public CWnd
{
	DECLARE_DYNAMIC(CWebView2Ctrl)

protected:
	BOOL RegisterWindowClass();

public:
	enum TIMER_ID
	{
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
	void	navigate(CString url = _T("about:blank"));
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
	void hide_download_dialog();
	void allow_external_drop(bool allow = true) { m_allow_external_drop = allow; };

	//WebView2
	HWND m_webHwnd = NULL;
	void InitializeWebView();
	void CloseWebView(bool cleanupUserDataFolder = false);
	HRESULT OnCreateEnvironmentCompleted(HRESULT result, ICoreWebView2Environment* environment);
	HRESULT OnCreateCoreWebView2ControllerCompleted(HRESULT result, ICoreWebView2Controller* controller);
	HRESULT DCompositionCreateDevice2(IUnknown* renderingDevice, REFIID riid, void** ppv);
	HRESULT WebMessageReceived(ICoreWebView2* sender, ICoreWebView2WebMessageReceivedEventArgs* args);
	HRESULT ExecuteScriptResponse(HRESULT errorCode, LPCWSTR result);

	void	resize();
	RECT	get_rect();


	EventRegistrationToken m_permissionRequestedToken = {};
	EventRegistrationToken m_navigationCompletedToken = {};
	EventRegistrationToken m_navigationStartingToken = {};
	EventRegistrationToken m_documentTitleChangedToken = {};
	EventRegistrationToken m_downloadStartingToken = {};
	void RegisterEventHandlers();

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
	wil::com_ptr<ICoreWebView2> m_webView;
	wil::com_ptr<IDCompositionDevice> m_dcompDevice;
	//wil::com_ptr<ICoreWebView2Profile> m_profile;

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
};
