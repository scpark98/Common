//#include "stdafx.h"
#include "webView2Ctrl.h"
#include "ViewComponent.h"
#include <ShObjIdl_core.h>
#include <Shellapi.h>
#include <ShlObj_core.h>

#ifdef __windows__
#undef __windows__
#endif
using Microsoft::WRL::Callback;
static constexpr UINT s_runAsyncWindowMessage = WM_APP;

#include "../Functions.h"

#define CHECK_FAILURE_STRINGIFY(arg)         #arg
#define CHECK_FAILURE_FILE_LINE(file, line)  ([](HRESULT hr){ CheckFailure(hr, "Failure at " CHECK_FAILURE_STRINGIFY(file) "(" CHECK_FAILURE_STRINGIFY(line) ")"); })
#define CHECK_FAILURE                        CHECK_FAILURE_FILE_LINE(__FILE__, __LINE__)
#define CHECK_FAILURE_BOOL(value)            CHECK_FAILURE((value) ? S_OK : E_UNEXPECTED)

#define CWEBVIEW2CTRL_CLASSNAME _T("CWebView2Ctrl")


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

IMPLEMENT_DYNAMIC(CWebView2Ctrl, CWnd)

CWebView2Ctrl::CWebView2Ctrl()
{
	if (!RegisterWindowClass())
		return;
}

CWebView2Ctrl::~CWebView2Ctrl()
{
	SetWindowLongPtr(m_hWnd, GWLP_USERDATA, 0);
	CloseWebView();
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
	DragAcceptFiles();

	CloseWebView();
	m_dcompDevice = nullptr;

	//HRESULT hr2 = DCompositionCreateDevice2(nullptr, IID_PPV_ARGS(&m_dcompDevice));
	//if (!SUCCEEDED(hr2))
	//{
	//	AfxMessageBox(_T("Attempting to create WebView using DComp Visual is not supported.\r\n")
	//		"DComp device creation failed.\r\n"
	//		"Current OS may not support DComp.\r\n"
	//		"Create with Windowless DComp Visual Failed", MB_OK);
	//	return;
	//}

#ifdef USE_WEBVIEW2_WIN10
	m_wincompCompositor = nullptr;
#endif
	USES_CONVERSION;
	LPCWSTR subFolder = nullptr;
	std::wstring m_userDataFolder;
	m_userDataFolder = CStringW(get_special_folder(CSIDL_COOKIES));
	auto options = Microsoft::WRL::Make<CoreWebView2EnvironmentOptions>();
	options->put_AllowSingleSignOnUsingOSPrimaryAccount(FALSE);

	//LPWSTR *args = nullptr;
	//options->get_AdditionalBrowserArguments(args);

	//m_userDataFolder를 주지 않고 nullptr로 할 경우
	//이 webview2 컴포넌트를 사용하는 프로그램을
	//Program Files 폴더에서 실행하면 웹페이지가 표시되지 않게 된다.
	HRESULT hr = CreateCoreWebView2EnvironmentWithOptions
				(subFolder, m_userDataFolder.c_str(), options.Get(),
				Microsoft::WRL::Callback<ICoreWebView2CreateCoreWebView2EnvironmentCompletedHandler>
				(this, &CWebView2Ctrl::OnCreateEnvironmentCompleted).Get());


	if (!SUCCEEDED(hr))
	{
		if (hr == HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND))
		{
			AfxMessageBox(_T("Couldn't find Edge installation. Microsoft Edge WebView2 Runtime is required."));
		}
		else
		{
			AfxMessageBox(_T("Failed to create webview environment"));
		}
	}
}

void CWebView2Ctrl::CloseWebView(bool cleanupUserDataFolder)
{
	if (m_webView)
	{
		m_webView->remove_NavigationCompleted(m_navigationCompletedToken);
		m_webView->remove_NavigationStarting(m_navigationStartingToken);
		m_webView->remove_DocumentTitleChanged(m_documentTitleChangedToken);
		m_webView->remove_PermissionRequested(m_permissionRequestedToken);

		wil::com_ptr<ICoreWebView2_4> wvWnd4 = m_webView.try_query<ICoreWebView2_4>();
		wvWnd4->remove_DownloadStarting(m_downloadStartingToken);

		m_webView = nullptr;
		m_webSettings = nullptr;
	}

	if (m_controller)
	{
		m_controller->Close();
		m_controller = nullptr;
	}

	m_webViewEnvironment = nullptr;
	m_webViewEnvironment2 = nullptr;
}

HRESULT CWebView2Ctrl::DCompositionCreateDevice2(IUnknown* renderingDevice, REFIID riid, void** ppv)
{
	HRESULT hr = E_FAIL;
	static decltype(::DCompositionCreateDevice2)* fnCreateDCompDevice2 = nullptr;
	if (fnCreateDCompDevice2 == nullptr)
	{
		HMODULE hmod = ::LoadLibraryEx(_T("dcomp.dll"), nullptr, 0);
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
	CHECK_FAILURE(environment->QueryInterface(IID_PPV_ARGS(&m_webViewEnvironment)));
	CHECK_FAILURE(environment->QueryInterface(IID_PPV_ARGS(&m_webViewEnvironment2)));

	//m_webViewEnvironment = environment;

	LPWSTR version = nullptr;
	m_webViewEnvironment->get_BrowserVersionString(&version);
	m_webview2_runtime_version = version;
	CoTaskMemFree(version);

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
		wil::com_ptr<ICoreWebView2> coreWebView2;
		m_controller->get_CoreWebView2(&coreWebView2);
		m_controller->get_ParentWindow(&m_mainWindow);
		coreWebView2.query_to(&m_webView);
		//m_webView = coreWebView2.get();

		/*
		* get_Profile을 사용하기 위해서는 NuGet에서 webView2를 1.0.1210.39 이상으로 설치해야 한다.
		wil::com_ptr<ICoreWebView2_10> wvWnd10 = m_webView.try_query<ICoreWebView2_10>();
		wvWnd10->get_Profile(&m_profile);
		*/

		//put_IsMuted는 1.0.1072.54부터 사용 가능
		//m_webView->put_IsMuted(FALSE);

		//기본 다운로드 경로를 얻어오거나 변경.
		//m_profile->get_DefaultDownloadFolderPath(&m_default_download_path);
		//m_profile->put_DefaultDownloadFolderPath();

		CHECK_FAILURE(m_webView->get_Settings(&m_webSettings));
		m_webSettings->put_IsScriptEnabled(TRUE);
		m_webSettings->put_IsWebMessageEnabled(TRUE);

		NewComponent<ViewComponent>
			(this, m_dcompDevice.get(),
#ifdef USE_WEBVIEW2_WIN10
				m_wincompCompositor,
#endif
				m_creationModeId == IDM_CREATION_MODE_TARGET_DCOMP);

		//EventRegistrationToken token;
		//m_webView->add_PermissionRequested(this, &token);
		EventRegistrationToken token;
		m_webView->add_WebMessageReceived(Callback<ICoreWebView2WebMessageReceivedEventHandler>(this, &CWebView2Ctrl::WebMessageReceived).Get(), &token);

		RegisterEventHandlers();

		UINT32 pid = 0;
		HRESULT hresult;
		//HANDLE hHandle;

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
			/*
			//for message test
			TCHAR path[FILENAME_MAX];
			GetModuleFileName(NULL, path, FILENAME_MAX);
			// Remove the file name
			CString s = path;
			CString szResult = s.Left(s.ReverseFind('\\') + 1);

			m_url_reserved = szResult + _T("\\SampleWebMessage.html");
			//for message test end.
			*/

			hresult = m_webView->Navigate(CStringW(m_url_reserved));
			m_url_reserved.Empty();
		}
		else
		{
			hresult = m_webView->Navigate(CStringW("about:blank"));
		}
		resize();
	}
	else
	{
		TRACE("Failed to create webview");
	}
	return S_OK;
}

HRESULT CWebView2Ctrl::WebMessageReceived(ICoreWebView2* sender, ICoreWebView2WebMessageReceivedEventArgs* args)
{
	LPWSTR pwStr;
	args->TryGetWebMessageAsString(&pwStr);
	CString receivedMessage = pwStr;
	if (!receivedMessage.IsEmpty())
	{
		AfxMessageBox("This message came from Javascript : " + receivedMessage);
	}
	return S_OK;
}

//카메라와 마이크에 대한 허용을 묻는 팝업에 대한 설정 변경
//mode = 0(default), 1(allow), 2(deny)
void CWebView2Ctrl::set_permission_request_mode(int mode)
{
	switch (mode)
	{
		case 1: m_permission_request_mode = COREWEBVIEW2_PERMISSION_STATE_ALLOW;
				break;
		case 2: m_permission_request_mode = COREWEBVIEW2_PERMISSION_STATE_DENY;
				break;
		default:
				m_permission_request_mode = COREWEBVIEW2_PERMISSION_STATE_DEFAULT;
	}
};

void CWebView2Ctrl::RegisterEventHandlers()
{
	wil::com_ptr<ICoreWebView2_4> wvWnd4 = m_webView.try_query<ICoreWebView2_4>();
	CHECK_FAILURE(wvWnd4->add_DownloadStarting(
		Microsoft::WRL::Callback<ICoreWebView2DownloadStartingEventHandler>(
			[this](ICoreWebView2*, ICoreWebView2DownloadStartingEventArgs* args) -> HRESULT
			{
				//TRUE이면 다운로드 팝업창을 표시하지 않는다.
				args->put_Handled(TRUE);

				//기본 다운로드 경로를 알 수도 있고
				ICoreWebView2DownloadOperation* downloadOperation;
				args->get_DownloadOperation(&downloadOperation);
				LPWSTR path;
				downloadOperation->get_ResultFilePath(&path);

				//다운받을 경로를 새로 지정할 수도 있다.
				//args->put_ResultFilePath(L"c:\\scpark\\test.avi");

				return S_OK;
			}).Get(), &m_downloadStartingToken));

	CHECK_FAILURE(m_webView->add_PermissionRequested(
		Microsoft::WRL::Callback<ICoreWebView2PermissionRequestedEventHandler>(
			[this](ICoreWebView2*, ICoreWebView2PermissionRequestedEventArgs* args) -> HRESULT
			{
				COREWEBVIEW2_PERMISSION_KIND kind;
				args->get_PermissionKind(&kind);
				if (kind == COREWEBVIEW2_PERMISSION_KIND_CAMERA || kind == COREWEBVIEW2_PERMISSION_KIND_MICROPHONE)
				{
					//args->put_State(COREWEBVIEW2_PERMISSION_STATE_ALLOW);
					args->put_State(m_permission_request_mode);
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
				// Disable external drop while navigating.
				/*
				auto controller4 = m_controller.try_query<ICoreWebView2Controller4>();
				if (controller4)
				{
					CHECK_FAILURE(controller4->put_AllowExternalDrop(m_allow_external_drop));
				}
				*/

				//만약 ICoreWebView2로 선언해서 생성한 경우 그 이후에 추가된 API를 사용할 경우는
				//아래와 같이 얻어온 후 API를 호출할 수 있다.
				//wil::com_ptr<ICoreWebView2_4> wvWnd4 = m_webView.try_query<ICoreWebView2_4>();

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

				on_navigation_completed();

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

				on_document_title_changed();

				return S_OK;
			})
		.Get(), &m_documentTitleChangedToken));
}

void CWebView2Ctrl::on_navigation_completed()
{
}

void CWebView2Ctrl::on_document_title_changed()
{
	wil::unique_cotaskmem_string title;
	m_webView->get_DocumentTitle(&title);
	m_document_title = title.get();
}

void CWebView2Ctrl::resize()
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

RECT CWebView2Ctrl::get_rect()
{
	RECT rc{ 0, 0, 0, 0 };

	if (m_webView)
	{
		GetWebViewController()->get_Bounds(&rc);
	}

	return rc;
}

//이 컨트롤을 사용한 앱에서 다운로드시에 표시되는 팝업창을 감추기 위해
//이 함수를 계속 호출하여 다운로드 팝업창을 닫는 방법으로 개발했으나 이는 좋은 방법이 아니다.
//정석은 다운로드 창 표시 여부를 세팅하는 인터페이스 함수를 호출하는 것이다.
void CWebView2Ctrl::hide_download_dialog()
{
	//m_webView->CloseDefaultDownloadDialog();
}

/*
//webView version = true, Edge version = false
CString CWebView2Ctrl::get_webview2_runtime_version(bool webView_version)
{
	CString path;
	CString version = _T("");
	HKEY handle = nullptr;

	LSTATUS result = ERROR_FILE_NOT_FOUND;

	if (webView_version)
	{
		result = RegOpenKeyEx(HKEY_LOCAL_MACHINE,
			R"(SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall\Microsoft EdgeWebView)",
			0,
			KEY_READ,
			&handle);

		if (result != ERROR_SUCCESS)
			result = RegOpenKeyEx(HKEY_LOCAL_MACHINE,
				R"(SOFTWARE\WOW6432Node\Microsoft\Windows\CurrentVersion\Uninstall\Microsoft EdgeWebView)",
				0,
				KEY_READ,
				&handle);
	}
	else
	{
		result = RegOpenKeyEx(HKEY_LOCAL_MACHINE,
			R"(SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall\Microsoft Edge)",
			0,
			KEY_READ,
			&handle);

		if (result != ERROR_SUCCESS)
			result = RegOpenKeyEx(HKEY_LOCAL_MACHINE,
				R"(SOFTWARE\WOW6432Node\Microsoft\Windows\CurrentVersion\Uninstall\Microsoft Edge)",
				0,
				KEY_READ,
				&handle);
	}

	if (result == ERROR_SUCCESS)
	{
		TCHAR buffer[MAX_PATH + 1]{ 0 };
		DWORD type = REG_SZ;
		DWORD size = MAX_PATH;
		result = RegQueryValueEx(handle, "InstallLocation", 0, &type, reinterpret_cast<LPBYTE>(buffer), &size);
		if (result == ERROR_SUCCESS)
			path += CString{ buffer };

		TCHAR chVersion[100]{ 0 };
		size = 100;
		result = RegQueryValueEx(handle, "Version", 0, &type, reinterpret_cast<LPBYTE>(chVersion), &size);
		if (result == ERROR_SUCCESS)
		{
			version = CString(chVersion);

			if (path.GetAt(path.GetLength() - 1) != L'\\')
				path += L"\\";
			path += CString{ chVersion };
		}
		else
			path.Empty();

		RegCloseKey(handle);
	}

	return version;
}
*/
void CWebView2Ctrl::navigate(CString url)
{
	//mainDlg의 OnInitDialog에서 navigate()을 호출하면
	//아직 m_webView가 생성되기 전이므로 url을 기억했다가
	//OnCreateCoreWebView2ControllerCompleted()에서 생성 완료되면 url을 로딩한다.
	if (!m_webView)
	{
		m_url_reserved = normalize_url(url);
		return;
	}

	url = normalize_url(url);
	m_webView->Navigate(CStringW(url));
}

CString CWebView2Ctrl::get_url()
{
	CString url;

	if (m_webView)
	{
		wil::unique_cotaskmem_string uri;
		m_webView->get_Source(&uri);

		if (wcscmp(uri.get(), L"about:blank") == 0)
		{
			uri = wil::make_cotaskmem_string(L"");
		}

		url = uri.get();
	}

	return url;
}

CString CWebView2Ctrl::normalize_url(CString url)
{
	if (url == "about:blank")
		return url;

	if (url.Find(_T("://")) < 0)
	{
		if (url.GetLength() > 1 && url[1] == ':')
			url = _T("file://") + url;
		else
			url = _T("http://") + url;
	}

	return url;
}

BEGIN_MESSAGE_MAP(CWebView2Ctrl, CWnd)
	ON_WM_SIZE()
	ON_WM_DROPFILES()
END_MESSAGE_MAP()


void CWebView2Ctrl::PreSubclassWindow()
{
	// TODO: 여기에 특수화된 코드를 추가 및/또는 기본 클래스를 호출합니다.
	HRESULT hresult = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
	//SetWindowLongPtr(this->GetSafeHwnd(), GWLP_USERDATA, (LONG_PTR)this);
	InitializeWebView();

	CWnd::PreSubclassWindow();
}


void CWebView2Ctrl::OnSize(UINT nType, int cx, int cy)
{
	CWnd::OnSize(nType, cx, cy);

	// TODO: 여기에 메시지 처리기 코드를 추가합니다.
	resize();
}

HRESULT CWebView2Ctrl::execute_jscript(CString jscript)
{
	HRESULT hr;

	if (m_webView != nullptr)
		hr = m_webView->ExecuteScript(CStringW(jscript), nullptr);

	return hr;
}

HRESULT CWebView2Ctrl::post_web_message_as_json(CString json)
{
	HRESULT hr;

	if (m_webView != nullptr)
		hr = m_webView->PostWebMessageAsJson(CStringW(json));

	return hr;
}

// The raw request header string delimited by CRLF(optional in last header).
void CWebView2Ctrl::navigate_post(CString const& url, CString const& content, CString const& headers, std::function<void()> onComplete)
{
	if (!m_webView) return;

	CString normalizedUrl{ normalize_url(url) };

	//m_callbacks[CallbackType::NavigationCompleted] = onComplete;

	wil::com_ptr<ICoreWebView2WebResourceRequest> webResourceRequest;
	wil::com_ptr<IStream> postDataStream = SHCreateMemStream(
		reinterpret_cast<const BYTE*>(static_cast<LPCTSTR>(content)),
		content.GetLength() + 1);

	CHECK_FAILURE(m_webViewEnvironment2->CreateWebResourceRequest(
		CStringW(normalizedUrl),
		L"POST",
		postDataStream.get(),
		CStringW(headers),
		&webResourceRequest));

	wil::com_ptr<ICoreWebView2_2> wvWnd2 = m_webView.try_query<ICoreWebView2_2>();
	CHECK_FAILURE(wvWnd2->NavigateWithWebResourceRequest(webResourceRequest.get()));
}

void CWebView2Ctrl::print_document()
{
	if (m_webView)
	{
		m_webView->ExecuteScript(L"window.print();", nullptr);
	}
}

void CWebView2Ctrl::Stop()
{
	if (m_webView)
	{
		m_webView->Stop();
	}
}

void CWebView2Ctrl::Reload()
{
	if (m_webView)
	{
		m_webView->Reload();
	}
}

void CWebView2Ctrl::GoBack()
{
	if (m_webView)
	{
		BOOL possible = FALSE;
		m_webView->get_CanGoBack(&possible);
		if (possible)
			m_webView->GoBack();
	}
}

void CWebView2Ctrl::GoForward()
{
	if (m_webView)
	{
		BOOL possible = FALSE;
		m_webView->get_CanGoForward(&possible);
		if (possible)
			m_webView->GoForward();
	}
}

void CWebView2Ctrl::DisablePopup()
{
	if (m_webSettings)
	{
		m_webSettings->put_AreDefaultScriptDialogsEnabled(FALSE);
	}
}


void CWebView2Ctrl::OnDropFiles(HDROP hDropInfo)
{
	// TODO: 여기에 메시지 처리기 코드를 추가 및/또는 기본값을 호출합니다.
	TCHAR sfile[MAX_PATH];
	CString file;

	DragQueryFile(hDropInfo, 0, sfile, MAX_PATH);
	file = sfile;
	navigate(file);

	CWnd::OnDropFiles(hDropInfo);
}
