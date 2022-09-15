//#include "stdafx.h"
#include "webView2Ctrl.h"
#include "ViewComponent.h"

#include "../Functions.h"


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
		Microsoft::WRL::ComPtr<ICoreWebView2> coreWebView2;
		m_controller->get_CoreWebView2(&coreWebView2);
		m_webView = coreWebView2.Get();

		NewComponent<ViewComponent>
			(this, m_dcompDevice.Get(),
#ifdef USE_WEBVIEW2_WIN10
				m_wincompCompositor,
#endif
				m_creationModeId == IDM_CREATION_MODE_TARGET_DCOMP);

		UINT32 pid = 0;
		HRESULT hresult;

		m_webView->get_BrowserProcessId(&pid);
		CString str = GetProcessNameByPID(pid);
		//HWND hwnd = GetHWNDbyPID(pid);
		//TRACE(_T("pid = %u, hwnd = 0x%08X\n"), pid, hwnd);

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
	}
}
void CWebView2Ctrl::navigate(CString url)
{
	//mainDlg의 OnInitDialog에서 navigate()을 호출하면
	//아직 m_webView가 생성되기 전이므로 url을 기억했다가
	//OnCreateCoreWebView2ControllerCompleted()에서 생성 완료되면 url을 로딩한다.
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
	ResizeControls();
}
