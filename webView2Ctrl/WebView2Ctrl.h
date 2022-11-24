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
- ��� : m_web.navigate(url);
- navigate�̿��� �ٸ� �Լ��� ����ϰ��� �Ѵٸ� m_web.GetWebView()->Navigate(...)ó�� �����Ͽ� ȣ��.
- mainDlg�� ũ�Ⱑ �ٲ�� m_web.MoveWindow(...)�� ũ�� ����
*/

#include <afxwin.h>
#include <wrl.h>
#include <wrl/event.h>
#include <wil/com.h>

#include "WebView2EnvironmentOptions.h"
#include "WebView2.h"

#include "ComponentBase.h"
#include <dcomp.h>
#include <EventToken.h>
#include <functional>
#include <memory>
#include <ole2.h>
#include <string>
#include <vector>
#include <map>
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

enum WEBVIEW2_MESSAGE
{
	webview2_message_navigation_start = WM_APP + 123,
	webview2_message_navigation_completed,
	webview2_message_document_title_changed,
	webview2_message_
};

class CWebView2Ctrl : public CWnd//, public ICoreWebView2PermissionRequestedEventHandler
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

	//ī�޶�� ����ũ�� ���� ����� ���� �˾��� ���� ���� ����
	//mode = 0(default), 1(allow), 2(deny)
	void set_permission_request_mode(int mode);

	CWebView2Ctrl();

	//���� ���� ����ϴ� �Լ��� �߰�������
	//main�� m_web �ν��Ͻ����� CWebView2Ctrl�� m_webView�� �ٸ� ����� �̿��ϰ��� �Ѵٸ�
	//m_web.GetWebView()->Navigate(...)�� ���� �����Ͽ� �̿��ϸ� �ȴ�.
	void navigate(CString url);
	void hide_download_dialog();

	//WebView2
	HWND m_webHwnd = NULL;
	void InitializeWebView();
	void CloseWebView(bool cleanupUserDataFolder = false);
	HRESULT OnCreateEnvironmentCompleted(HRESULT result, ICoreWebView2Environment* environment);
	HRESULT OnCreateCoreWebView2ControllerCompleted(HRESULT result, ICoreWebView2Controller* controller);
	void ResizeControls();
	HRESULT DCompositionCreateDevice2(IUnknown* renderingDevice, REFIID riid, void** ppv);

	EventRegistrationToken m_permissionRequestedToken = {};
	EventRegistrationToken m_navigationCompletedToken = {};
	EventRegistrationToken m_navigationStartingToken = {};
	EventRegistrationToken m_documentTitleChangedToken = {};
	void RegisterEventHandlers();

	ICoreWebView2Controller* GetWebViewController()
	{
		return m_controller.Get();
	}
	ICoreWebView2_15* GetWebView()
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
	COREWEBVIEW2_PERMISSION_STATE m_permission_request_mode = COREWEBVIEW2_PERMISSION_STATE_DEFAULT;

	//WebView2
	DWORD m_creationModeId = 0;
	Microsoft::WRL::ComPtr<ICoreWebView2Environment> m_webViewEnvironment;
	Microsoft::WRL::ComPtr<ICoreWebView2Controller> m_controller;
	Microsoft::WRL::ComPtr<ICoreWebView2_15> m_webView;
	Microsoft::WRL::ComPtr<IDCompositionDevice> m_dcompDevice;
	Microsoft::WRL::ComPtr<ICoreWebView2DownloadStartingEventArgs> m_downloadStartingEvent;
	Microsoft::WRL::ComPtr<ICoreWebView2DocumentTitleChangedEventHandler> m_titleChangedEvent;
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
};
