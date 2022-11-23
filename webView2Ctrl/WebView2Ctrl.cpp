//#include "stdafx.h"
#include "webView2Ctrl.h"
#include "ViewComponent.h"

#include "../Functions.h"

#define CHECK_FAILURE_STRINGIFY(arg)         #arg
#define CHECK_FAILURE_FILE_LINE(file, line)  ([](HRESULT hr){ CheckFailure(hr, "Failure at " CHECK_FAILURE_STRINGIFY(file) "(" CHECK_FAILURE_STRINGIFY(line) ")"); })
#define CHECK_FAILURE                        CHECK_FAILURE_FILE_LINE(__FILE__, __LINE__)
#define CHECK_FAILURE_BOOL(value)            CHECK_FAILURE((value) ? S_OK : E_UNEXPECTED)

#define CWEBVIEW2CTRL_CLASSNAME _T("CWebView2Ctrl")

IMPLEMENT_DYNAMIC(CWebView2Ctrl, CWnd)

CWebView2Ctrl::CWebView2Ctrl()
{
	if (!RegisterWindowClass())
		return;
}

BOOL CWebView2Ctrl::RegisterWindowClass()
{
	WNDCLASS wndcls;
	HINSTANCE hInst = AfxGetInstanceHandle();

	if (!(::GetClassInfo(hInst, CWEBVIEW2CTRL_CLASSNAME, &wndcls)))
	{
		// otherwise we need to register a new class
		CBrush	brush;
		brush.CreateSolidBrush(::GetSysColor(COLOR_3DFACE));

		wndcls.style = CS_DBLCLKS | CS_HREDRAW | CS_VREDRAW;
		wndcls.lpfnWndProc = ::DefWindowProc;
		wndcls.cbClsExtra = wndcls.cbWndExtra = 0;
		wndcls.hInstance = hInst;
		wndcls.hIcon = NULL;
		wndcls.hCursor = AfxGetApp()->LoadStandardCursor(IDC_ARROW);
		wndcls.hbrBackground = (HBRUSH)brush.GetSafeHandle();
		wndcls.lpszMenuName = NULL;
		wndcls.lpszClassName = CWEBVIEW2CTRL_CLASSNAME;

		if (!AfxRegisterClass(&wndcls))
		{
			AfxThrowResourceException();
			return FALSE;
		}
	}

	return TRUE;
}

void CWebView2Ctrl::InitializeWebView()
{
	CloseWebView();
	m_dcompDevice = nullptr;

	HRESULT hr2 = DCompositionCreateDevice2(nullptr, IID_PPV_ARGS(&m_dcompDevice));
	if (!SUCCEEDED(hr2))
	{
		AfxMessageBox(L"Attempting to create WebView using DComp Visual is not supported.\r\n"
			"DComp device creation failed.\r\n"
			"Current OS may not support DComp.\r\n"
			"Create with Windowless DComp Visual Failed", MB_OK);
		return;
	}

#ifdef USE_WEBVIEW2_WIN10
	m_wincompCompositor = nullptr;
#endif
	LPCWSTR subFolder = nullptr;
	std::wstring m_userDataFolder;
	m_userDataFolder = get_special_folder(CSIDL_COOKIES);
	auto options = Microsoft::WRL::Make<CoreWebView2EnvironmentOptions>();
	options->put_AllowSingleSignOnUsingOSPrimaryAccount(FALSE);

	//m_userDataFolder�� ���� �ʰ� nullptr�� �� ���
	//�� webview2 ������Ʈ�� ����ϴ� ���α׷���
	//Program Files �������� �����ϸ� ���������� ǥ�õ��� �ʰ� �ȴ�.
	HRESULT hr = CreateCoreWebView2EnvironmentWithOptions
				(subFolder, m_userDataFolder.c_str(), options.Get(),
				Microsoft::WRL::Callback<ICoreWebView2CreateCoreWebView2EnvironmentCompletedHandler>
				(this, &CWebView2Ctrl::OnCreateEnvironmentCompleted).Get());


	if (!SUCCEEDED(hr))
	{
		if (hr == HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND))
		{
			TRACE("Couldn't find Edge installation. Do you have a version installed that is compatible with this ");
		}
		else
		{
			AfxMessageBox(L"Failed to create webview environment");
		}
	}
}

void CWebView2Ctrl::CloseWebView(bool cleanupUserDataFolder)
{
	if (m_controller)
	{
		m_controller->Close();
		m_controller = nullptr;
		m_webView = nullptr;
	}
	m_webViewEnvironment = nullptr;
	if (cleanupUserDataFolder)
	{
		//Clean user data        
	}
}

HRESULT CWebView2Ctrl::DCompositionCreateDevice2(IUnknown* renderingDevice, REFIID riid, void** ppv)
{
	HRESULT hr = E_FAIL;
	static decltype(::DCompositionCreateDevice2)* fnCreateDCompDevice2 = nullptr;
	if (fnCreateDCompDevice2 == nullptr)
	{
		HMODULE hmod = ::LoadLibraryEx(L"dcomp.dll", nullptr, 0);
		if (hmod != nullptr)
		{
			fnCreateDCompDevice2 = reinterpret_cast<decltype(::DCompositionCreateDevice2)*>(
				::GetProcAddress(hmod, "DCompositionCreateDevice2"));
		}
	}
	if (fnCreateDCompDevice2 != nullptr)
	{
		hr = fnCreateDCompDevice2(renderingDevice, riid, ppv);
	}
	return hr;
}

HRESULT CWebView2Ctrl::OnCreateEnvironmentCompleted(HRESULT result, ICoreWebView2Environment* environment)
{
	m_webViewEnvironment = environment;
	m_webViewEnvironment->CreateCoreWebView2Controller
	(this->GetSafeHwnd(), Microsoft::WRL::Callback<ICoreWebView2CreateCoreWebView2ControllerCompletedHandler>
		(this, &CWebView2Ctrl::OnCreateCoreWebView2ControllerCompleted).Get());

	return S_OK;
}

HRESULT CWebView2Ctrl::OnCreateCoreWebView2ControllerCompleted(HRESULT result, ICoreWebView2Controller* controller)
{
	if (result == S_OK)
	{
		m_controller = controller;
		Microsoft::WRL::ComPtr<ICoreWebView2_15> coreWebView2;
		m_controller->get_CoreWebView2(&coreWebView2);
		m_webView = coreWebView2.Get();


		NewComponent<ViewComponent>
			(this, m_dcompDevice.Get(),
#ifdef USE_WEBVIEW2_WIN10
				m_wincompCompositor,
#endif
				m_creationModeId == IDM_CREATION_MODE_TARGET_DCOMP);

		//EventRegistrationToken token;
		//m_webView->add_PermissionRequested(this, &token);

		RegisterEventHandlers();

		UINT32 pid = 0;
		HRESULT hresult;
		HANDLE hHandle;

		m_webView->get_BrowserProcessId(&pid);
		CString str = GetProcessNameByPID(pid);
		//HWND hwnd = GetHWNDbyPID(pid);
		//TRACE(_T("pid = %u, hwnd = 0x%08X\n"), pid, hwnd);

		// Register a handler for the DocumentTitleChanged event.
	// This handler just announces the new title on the window's title bar.
		/*
		m_webView->add_DocumentTitleChanged(
			Callback<ICoreWebView2DocumentTitleChangedEventHandler>(
				[this](ICoreWebView2* sender, IUnknown* args) -> HRESULT {
					wil::unique_cotaskmem_string title;
					CHECK_FAILURE(sender->get_DocumentTitle(&title));
					m_appWindow->SetDocumentTitle(title.get());
					return S_OK;
				})
			.Get(),
					&m_documentTitleChangedToken);
		*/

		if (m_url_reserved.IsEmpty() == false)
		{
			hresult = m_webView->Navigate(m_url_reserved);
			m_url_reserved.Empty();
		}
		else
		{
			hresult = m_webView->Navigate(_T("about:blank"));
		}
		ResizeControls();
	}
	else
	{
		TRACE("Failed to create webview");
	}
	return S_OK;
}

void ShowFailure(HRESULT hr, CString const& message)
{
	CString text;
	text.Format(_T("%s (0x%08X)"), (LPCTSTR)message, hr);

	::MessageBox(nullptr, static_cast<LPCTSTR>(text), _T("Failure"), MB_OK);
}

void CheckFailure(HRESULT hr, CString const& message)
{
	if (FAILED(hr))
	{
		CString text;
		text.Format(_T("%s : 0x%08X"), (LPCTSTR)message, hr);

		// TODO: log text

		std::exit(hr);
	}
}

void CWebView2Ctrl::RegisterEventHandlers()
{
	CHECK_FAILURE(m_webView->add_PermissionRequested(
		Microsoft::WRL::Callback<ICoreWebView2PermissionRequestedEventHandler>(
			[this](ICoreWebView2*, ICoreWebView2PermissionRequestedEventArgs* args) -> HRESULT
			{
				COREWEBVIEW2_PERMISSION_KIND kind;
				args->get_PermissionKind(&kind);
				if (kind == COREWEBVIEW2_PERMISSION_KIND_CAMERA || kind == COREWEBVIEW2_PERMISSION_KIND_MICROPHONE)
				{
					args->put_State(COREWEBVIEW2_PERMISSION_STATE_ALLOW);
				}
				return S_OK;
			}).Get(), &m_permissionRequestedToken));


	// NavigationStarting handler
	CHECK_FAILURE(m_webView->add_NavigationStarting(
		Microsoft::WRL::Callback<ICoreWebView2NavigationStartingEventHandler>(
			[this](
				ICoreWebView2*,
				ICoreWebView2NavigationStartingEventArgs* args) -> HRESULT
			{
				wil::unique_cotaskmem_string uri;
				CHECK_FAILURE(args->get_Uri(&uri));

				//m_isNavigating = true;
				m_document_title.Empty();

				return S_OK;
			}).Get(), &m_navigationStartingToken));


	// NavigationCompleted handler
	CHECK_FAILURE(m_webView->add_NavigationCompleted(
		Microsoft::WRL::Callback<ICoreWebView2NavigationCompletedEventHandler>(
			[this](
				ICoreWebView2*,
				ICoreWebView2NavigationCompletedEventArgs* args) -> HRESULT
			{
				//m_isNavigating = false;

				BOOL success;
				CHECK_FAILURE(args->get_IsSuccess(&success));

				if (!success)
				{
					COREWEBVIEW2_WEB_ERROR_STATUS webErrorStatus{};
					CHECK_FAILURE(args->get_WebErrorStatus(&webErrorStatus));
					if (webErrorStatus == COREWEBVIEW2_WEB_ERROR_STATUS_DISCONNECTED)
					{
						// Do something here if you want to handle a specific error case.
						// In most cases this isn't necessary, because the WebView will
						// display its own error page automatically.
					}
				}

				wil::unique_cotaskmem_string uri;
				m_webView->get_Source(&uri);

				if (wcscmp(uri.get(), L"about:blank") == 0)
				{
					uri = wil::make_cotaskmem_string(L"");
				}

				OnNavigationCompleted();

				return S_OK;
			})
		.Get(),
				&m_navigationCompletedToken));


	// DocumentTitleChanged handler
	CHECK_FAILURE(m_webView->add_DocumentTitleChanged(
		Microsoft::WRL::Callback<ICoreWebView2DocumentTitleChangedEventHandler>(
			[this](ICoreWebView2* sender, IUnknown* args) -> HRESULT {
				wil::unique_cotaskmem_string title;
				CHECK_FAILURE(sender->get_DocumentTitle(&title));

				OnDocumentTitleChanged();

				return S_OK;
			})
		.Get(), &m_documentTitleChangedToken));
}

void CWebView2Ctrl::OnNavigationCompleted()
{
}

void CWebView2Ctrl::OnDocumentTitleChanged()
{
	wil::unique_cotaskmem_string title;
	m_webView->get_DocumentTitle(&title);
	m_document_title = title.get();
}

void CWebView2Ctrl::ResizeControls()
{
	CRect r;
	CRect rc;

	if (!m_hWnd)
		return;

	GetClientRect(&rc);

	if (auto view = GetComponent<ViewComponent>())
	{
		view->SetBounds(CRect(rc));
		//m_webHwnd = view->get_web_HWND();
	}
}

void CWebView2Ctrl::hide_download_dialog()
{
	m_webView->CloseDefaultDownloadDialog();
}

void CWebView2Ctrl::navigate(CString url)
{
	//mainDlg�� OnInitDialog���� navigate()�� ȣ���ϸ�
	//���� m_webView�� �����Ǳ� ���̹Ƿ� url�� ����ߴٰ�
	//OnCreateCoreWebView2ControllerCompleted()���� ���� �Ϸ�Ǹ� url�� �ε��Ѵ�.
	if (!m_webView)
	{
		m_url_reserved = url;
		return;
	}

	m_webView->Navigate(url);
}
BEGIN_MESSAGE_MAP(CWebView2Ctrl, CWnd)
	ON_WM_SIZE()
END_MESSAGE_MAP()


void CWebView2Ctrl::PreSubclassWindow()
{
	// TODO: ���⿡ Ư��ȭ�� �ڵ带 �߰� ��/�Ǵ� �⺻ Ŭ������ ȣ���մϴ�.
	HRESULT hresult = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
	//SetWindowLongPtr(this->GetSafeHwnd(), GWLP_USERDATA, (LONG_PTR)this);
	InitializeWebView();

	CWnd::PreSubclassWindow();
}


void CWebView2Ctrl::OnSize(UINT nType, int cx, int cy)
{
	CWnd::OnSize(nType, cx, cy);

	// TODO: ���⿡ �޽��� ó���� �ڵ带 �߰��մϴ�.
	ResizeControls();
}
