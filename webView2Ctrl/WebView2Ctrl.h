#pragma once

/*
webView2 컴포넌트를 사용하기 위한 단계가 복잡하여
CWnd를 상속받은 Custom Control에 webView2가 표시되도록 제작.

[사용 방법]
- 이 컨트롤을 사용하고자 하는 프로젝트에서 NuGet 패키지 관리자를 열고
  다음 2개의 패키지를 설치.
  WebView2 (VS2015에서 최신 버전 추가시 에러가 발생하여 1.0.622.22로 설치, VS2022에서는 최신 버전 문제없음)
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
- 사용 : m_web.navigate(url);
- navigate이외의 다른 함수를 사용하고자 한다면 m_web.GetWebView()->Navigate(...)처럼 접근하여 호출.
- mainDlg의 크기가 바뀌면 m_web.MoveWindow(...)로 크기 조정
*/

#include <afxwin.h>
#include <wrl.h>
#include "WebView2EnvironmentOptions.h"
#include "WebView2.h"

#include "ComponentBase.h"
#include <dcomp.h>
#include <functional>
#include <memory>
#include <ole2.h>
#include <string>
#include <vector>
#include <winnt.h>

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

protected:
	BOOL RegisterWindowClass();

public:
	enum TIMER_ID
	{
	};

	CWebView2Ctrl();

	//가장 많이 사용하는 함수라서 추가했으나
	//main의 m_web 인스턴스에서 CWebView2Ctrl의 m_webView의 다른 기능을 이용하고자 한다면
	//m_web.GetWebView()->Navigate(...)과 같이 접근하여 이용하면 된다.
	void navigate(CString url);

	//WebView2
	HWND m_webHwnd = NULL;
	void InitializeWebView();
	void CloseWebView(bool cleanupUserDataFolder = false);
	HRESULT OnCreateEnvironmentCompleted(HRESULT result, ICoreWebView2Environment* environment);
	HRESULT OnCreateCoreWebView2ControllerCompleted(HRESULT result, ICoreWebView2Controller* controller);
	void RunAsync(std::function<void(void)> callback);
	void ResizeControls();
	HRESULT DCompositionCreateDevice2(IUnknown* renderingDevice, REFIID riid, void** ppv);

	ICoreWebView2Controller* GetWebViewController()
	{
		return m_controller.Get();
	}
	ICoreWebView2* GetWebView()
	{
		return m_webView.Get();
	}
	ICoreWebView2Environment* GetWebViewEnvironment()
	{
		return m_webViewEnvironment.Get();
	}
	HWND GetMainWindow()
	{
		return this->GetSafeHwnd();
	}
	//WebView2

protected:
	CString m_url_reserved = _T("");

	//WebView2
	DWORD m_creationModeId = 0;
	Microsoft::WRL::ComPtr<ICoreWebView2Environment> m_webViewEnvironment;
	Microsoft::WRL::ComPtr<ICoreWebView2Controller> m_controller;
	Microsoft::WRL::ComPtr<ICoreWebView2> m_webView;
	Microsoft::WRL::ComPtr<IDCompositionDevice> m_dcompDevice;
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
};
