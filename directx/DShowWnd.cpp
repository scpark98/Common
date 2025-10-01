// VideoWnd.cpp : implementation file
//

#include "stdafx.h"
#include "DShowWnd.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#include <math.h>
#include <atlbase.h>	// for USES_CONVERSION
#include "nserror.h"	// for S_E_LICENSE_REQUIRED, NS_E_PROTECTED_CONTENT
//#include "wmprop.h"		// for WMFormat property
//#include "Qedit.h"		// for ISampleGrabberCB
#include "ddraw.h"
//#include "LegendsVideo_i.h"
//#include "OverlayWarningDlg.h"

#include <initguid.h>
#include <qnetwork.h>

#pragma comment( lib, "ddraw.lib" )
#pragma comment( lib, "strmiids.lib" )
//#pragma comment( lib, "wmstub.lib" )	// for Windows Media File Fomat
#pragma comment( lib, "quartz.lib" )	// for AMGetErrorText

#pragma warning(disable: 4996)	//disable _CRT_SECURE_NO_WARNINGS warning


#define WM_GRAPHNOTIFY			WM_APP + 1

typedef HRESULT(WINAPI * DIRECTDRAWCREATE)( GUID*, LPDIRECTDRAW*, IUnknown* );
typedef HRESULT(WINAPI * DIRECTDRAWCREATEEX)( GUID*, VOID**, REFIID, IUnknown* );

// Macros
#define DibNumColors(lpbi)      ((lpbi)->biClrUsed == 0 && (lpbi)->biBitCount <= 8 \
                                    ? (int)(1 << (int)(lpbi)->biBitCount)          \
                                    : (int)(lpbi)->biClrUsed)
#define DibSize(lpbi)           ((lpbi)->biSize + (lpbi)->biSizeImage + (int)(lpbi)->biClrUsed * sizeof(RGBQUAD))
#define DibPaletteSize(lpbi)    (DibNumColors(lpbi) * sizeof(RGBQUAD))
#define BFT_BITMAP 0x4d42		// 'BM'


// 프로그램에서 사용한 리소스를 안전하게 해제한다. 
#define SAFE_RELEASE(x) { if (x) x->Release(); x = NULL; }

#define JIF(x) (x)
//#define JIF(x) if (FAILED(hr=(x))) \
//{Msg(TEXT("FAILED(hr=0x%x) in ") TEXT(#x) TEXT("\n"), hr); return -999;}

#define LIF(x) (x)
//#define LIF(x) if (FAILED(hr=(x))) \
//{Msg(TEXT("FAILED(hr=0x%x) in ") TEXT(#x) TEXT("\n"), hr);}

#if 0
void Msg(TCHAR *szFormat, ...)
{
	TCHAR szBuffer[512];  // Large buffer for very long filenames (like with HTTP)

	va_list pArgs;
	va_start(pArgs, szFormat);
	vsprintf(szBuffer, szFormat, pArgs);
	va_end(pArgs);
#ifdef _DEBUG	
//	AfxMessageBox( szBuffer );
#endif
}
#endif

/////////////////////////////////////////////////////////////////////////////
// CDShowWnd

CDShowWnd::CDShowWnd()
{
	if( !RegisterWindowClass() )
		return;

	CoInitialize(NULL);

	FindAudioRenderer();

	m_hParentWnd		= NULL;

	m_pGraphBuilder		= NULL;
	m_pMediaControl		= NULL;
	m_pEvent			= NULL;
	m_pMediaSeeking		= NULL;
	m_pVideoWindow		= NULL;
	m_pBasicVideo		= NULL;
	m_pBasicAudio		= NULL;
	m_pMixerPinConfig2	= NULL;
	m_pVideoFrameStep	= NULL;
	m_pOverlayMixer		= NULL;
	m_pOverlay			= NULL;

	m_dwGraphRegister	= 0;

	m_nVideoStream		= 0;
	m_nAudioStream		= 0;
	m_sMediaFile		= "";
	m_dDuration			= 0.0;

	m_crBack			= RGB( 32, 32, 32 );

	m_sStatusMessage	= "No media file loaded.";
	m_crStatusMessage	= RGB( 255, 255, 255 );
}

CDShowWnd::~CDShowWnd()
{
	Close();
}

BOOL CDShowWnd::RegisterWindowClass()
{
	WNDCLASS wndcls;
	HINSTANCE hInst = AfxGetInstanceHandle();

	if (!(::GetClassInfo(hInst, _T("CDShowWnd"), &wndcls)))
	{
		// otherwise we need to register a new class
		CBrush	brush;
		brush.CreateSolidBrush( ::GetSysColor( COLOR_3DFACE ) );

		wndcls.style            = CS_DBLCLKS | CS_HREDRAW | CS_VREDRAW ;
		wndcls.lpfnWndProc      = ::DefWindowProc;
		wndcls.cbClsExtra       = wndcls.cbWndExtra = 0;
		wndcls.hInstance        = hInst;
		wndcls.hIcon            = NULL;
		wndcls.hCursor          = AfxGetApp()->LoadStandardCursor(IDC_ARROW);
		wndcls.hbrBackground    = (HBRUSH) brush.GetSafeHandle();
		wndcls.lpszMenuName     = NULL;
		wndcls.lpszClassName    = _T("CDShowWnd");

		if (!AfxRegisterClass(&wndcls))
		{
			AfxThrowResourceException();
			return FALSE;
		}
	}

	return TRUE;
}

BEGIN_MESSAGE_MAP(CDShowWnd, CWnd)
	//{{AFX_MSG_MAP(CDShowWnd)
	ON_WM_DESTROY()
	ON_WM_TIMER()
	ON_WM_PAINT()
	ON_WM_ERASEBKGND()
	ON_WM_SETCURSOR()
	//}}AFX_MSG_MAP
	ON_MESSAGE(WM_GRAPHNOTIFY,OnGraphNotify)
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// CDShowWnd message handlers

void CDShowWnd::DisplayStatusMessage( CString sMsg, COLORREF crText )
{
	m_sStatusMessage = sMsg;
	m_crStatusMessage = crText;
	Invalidate();
}

_COM_SMARTPTR_TYPEDEF(IBaseFilter, __uuidof(IBaseFilter));

typedef HRESULT (STDAPICALLTYPE* FN_DLLGETCLASSOBJECT)(REFCLSID clsid, REFIID iid, void** ppv);

HRESULT CDShowWnd::CreateObjectFromPath(TCHAR* pPath, REFCLSID clsid, IUnknown** ppUnk)
{
	// load the target DLL directly
	HMODULE lib = LoadLibrary(pPath);
	if (!lib)
	{
		return HRESULT_FROM_WIN32(GetLastError());
	}

	// the entry point is an exported function
	FN_DLLGETCLASSOBJECT fn = (FN_DLLGETCLASSOBJECT)GetProcAddress(lib, "DllGetClassObject");
	if (fn == NULL)
	{
		return HRESULT_FROM_WIN32(GetLastError());
	}

	// create a class factory
	IUnknownPtr pUnk;
	HRESULT hr = fn(clsid,  IID_IUnknown,  (void**)(IUnknown**)&pUnk);
	if (SUCCEEDED(hr))
	{
		IClassFactoryPtr pCF = pUnk;
		if (pCF == NULL)
		{
			hr = E_NOINTERFACE;
		}
		else
		{
			// ask the class factory to create the object
			hr = pCF->CreateInstance(NULL, IID_IUnknown, (void**)ppUnk);
		}
	}

	return hr;
}

int CDShowWnd::LoadMediaFile( CString sFile, bool bAutoRender, int nVideoCh )
{
	if (CheckMediaIsOpened())
		ReleaseAll();

	HRESULT		hr;
	WCHAR		wFile[MAX_PATH];
	CString		wColorSpace = /*L"YUV Transform";/*/_T("Color Space Converter");
	CString		sFilename;

	if ( sFile.Find( '\\' ) > 0 )
		sFilename = sFile.Right( sFile.GetLength() - sFile.ReverseFind('\\') - 1 );
	else if ( sFile.Find( '/' ) > 0 )
		sFilename = sFile.Right( sFile.GetLength() - sFile.ReverseFind('/') - 1 );
	else
		sFilename = sFile;

	DisplayStatusMessage(_T("Loading media file..."), MESSAGE_COLOR_INFO );

	if ( PathFileExists( sFile ) == false )
	{
		DisplayStatusMessage(_T("File does not exist."), MESSAGE_COLOR_ERROR );
		return -1;
	}

	CoInitialize( NULL );
	MultiByteToWideChar( CP_ACP, 0, sFile, -1, wFile, MAX_PATH );
	
	
	IBaseFilter	*pBaseFilter = NULL;
	IBaseFilter *pTempFilter = NULL;
	IFileSourceFilter	*pFileSource = NULL;
	hr = FindFilter( "LAV Splitter Source", CLSID_LegacyAmFilterCategory, &pBaseFilter );
	if ( !bAutoRender && (FAILED(hr) || pBaseFilter == NULL) )
	{
		DisplayStatusMessage( "LAV Codec을 설치한 후 다시 실행해 주십시오.", MESSAGE_COLOR_ERROR );
		return 0;
	}
	
	// 필터그래프 매니저를 생성하고 인터페이스를 얻는다.
	hr = CoCreateInstance( CLSID_FilterGraph, NULL, CLSCTX_INPROC_SERVER, IID_IGraphBuilder, (void **)&m_pGraphBuilder );
    if( FAILED(hr) )
	{
		DisplayStatusMessage(_T("CLSID_FilterGraph 생성 실패."));
		return 99;
	}

    // Overlay Mixer
	// 이 필터를 추가하면 뭔가 문제가 있다. 우선 사용을 자제한다.
	//hr = CoCreateInstance( CLSID_OverlayMixer, NULL, CLSCTX_INPROC, IID_IBaseFilter, (void **)&m_pOverlayMixer );    
	// Add it to the filter graph.   
	//if( SUCCEEDED(hr) )   
		//m_pGraphBuilder->AddFilter(m_pOverlayMixer, L"Overlay Mixer");   

	m_pGraphBuilder->QueryInterface( IID_IMediaControl, (void **)&m_pMediaControl );
	m_pGraphBuilder->QueryInterface( IID_IMediaEventEx, (void**)&m_pEvent );
	m_pGraphBuilder->QueryInterface( IID_IVideoWindow, (void **)&m_pVideoWindow );
	m_pGraphBuilder->QueryInterface( IID_IBasicVideo, (void **)&m_pBasicVideo );
	m_pGraphBuilder->QueryInterface( IID_IBasicAudio, (void **)&m_pBasicAudio );
	m_pEvent->SetNotifyWindow( (OAHWND)GetSafeHwnd(), WM_GRAPHNOTIFY, 0 );
	m_pGraphBuilder->QueryInterface( IID_IMediaSeeking, (void **)&m_pMediaSeeking );
	
	if ( bAutoRender )
	{
		hr = m_pGraphBuilder->RenderFile(wFile, NULL);
	}
#if 1
	else
	{
		
		hr = FindFilter( _T("LAV Splitter Source"), IID_IFileSourceFilter, &pBaseFilter );
		hr = pBaseFilter->QueryInterface( IID_IFileSourceFilter, (void **) &pFileSource );
		// Attempt to load this file
		hr = m_pGraphBuilder->AddFilter( pBaseFilter, L"LAV Splitter Source" );
		hr = pFileSource->Load(wFile, NULL);

		//stream 분석
		IAMStreamSelect *pStreamSelect = NULL;
		pFileSource->QueryInterface( IID_IAMStreamSelect, (void**)&pStreamSelect);

		DWORD			dwCount = 0;
		AM_MEDIA_TYPE	*pmt;
		DWORD			dwFlags;
		LCID			lcid;
		DWORD			dwGroup;
		WCHAR			*pzsName;

		m_nVideoStream = m_nAudioStream = 0;

		pStreamSelect->Count( &dwCount );

		for ( int i = 0; i < dwCount; i++ )
		{
			hr = pStreamSelect->Info( i, &pmt, &dwFlags, &lcid, &dwGroup, &pzsName, NULL, NULL );
			if ( pmt->majortype == MEDIATYPE_Video )
				m_nVideoStream++;
			else if ( pmt->majortype == MEDIATYPE_Audio )
				m_nAudioStream++;
		}

		if ( m_nVideoStream <= 1 && nVideoCh > 0 )
		{
			pStreamSelect->Release();
			DisplayStatusMessage( _T("This video has only 1 channel video stream."), MESSAGE_COLOR_INFO );
			return 777;
		}

		if ( m_nVideoStream > 1 && nVideoCh > 0 )
			pStreamSelect->Enable( nVideoCh, AMSTREAMSELECTENABLE_ENABLE );
		//pStreamSelect->Dis( !nVideoCh, AMSTREAMSELECTENABLE_ENABLE );
		pStreamSelect->Release();
		//stream 분석 완료

		IBaseFilter *pLAVAudioDecoder = NULL;
		hr = FindFilter( "LAV Audio Decoder", CLSID_LegacyAmFilterCategory, &pLAVAudioDecoder );
		if ( hr == S_OK )
		{
			hr = m_pGraphBuilder->AddFilter( pLAVAudioDecoder, L"LAV Audio Decoder" );
			hr = ConnectFilters(m_pGraphBuilder, pBaseFilter, pLAVAudioDecoder );

			//필터 목록에서 "Default DirectSound Device"를 찾아서 연결하려 했으나
			//필터목록에는 막상 저 이름으로는 존재하지 않는다. 일단 RenderOutputPins 함수로 자동 렌더시켜준다.
			hr = RenderOutputPins( m_pGraphBuilder, pLAVAudioDecoder );
		}

		//IBaseFilter *pDefaultDirectSoundDevice = NULL;
		//hr = FindFilter( "Default DirectSound Device", CLSID_DSoundRender, &pDefaultDirectSoundDevice );
		//if ( hr == S_OK )
		//{
		//	hr = m_pGraphBuilder->AddFilter( pDefaultDirectSoundDevice, L"Default DirectSound Device" );
		//	hr = ConnectFilters(m_pGraphBuilder, pLAVAudioDecoder, pDefaultDirectSoundDevice );
		//}

		IBaseFilter* pMSDTV = NULL;
		IBaseFilter* pVideoDecoder = NULL;
		IBaseFilter* pRenderer = NULL;

		hr = FindFilter( "Microsoft DTV-DVD Video Decoder", CLSID_LegacyAmFilterCategory, &pMSDTV );
		hr = m_pGraphBuilder->AddFilter( pMSDTV, L"Microsoft DTV-DVD Video Decoder" );
		hr = ConnectFilters(m_pGraphBuilder, pBaseFilter, pMSDTV );
		//hr = m_pGraphBuilder->RenderStream( NULL, &MEDIATYPE_Video, pBaseFilter, NULL, pMSDTV );

		//TRACE( "======================================\n" );
		//CoCreateInstance(CLSID_ffdshow_Video_Decoder, NULL, CLSCTX_INPROC_SERVER, IID_IBaseFilter, (void**)&pVideoDecoder );
		////hr = FindFilter( "ffdshow Video Decoder", CLSID_LegacyAmFilterCategory, &pVideoDecoder );
		//hr = m_pGraphBuilder->AddFilter( pVideoDecoder, L"ffdshow Video Decoder" );
		//hr = ConnectFilters(m_pGraphBuilder, pMSDTV, pVideoDecoder );
		//TRACE( "======================================\n" );

		hr = FindFilter( "Video Renderer", CLSID_LegacyAmFilterCategory, &pRenderer );
		CoCreateInstance(CLSID_VideoRendererDefault, NULL, CLSCTX_INPROC_SERVER, IID_IBaseFilter, (void**)&pRenderer );

		//IUnknownPtr pUnk;
		//IBaseFilterPtr pFilter;
		//HRESULT hr = CreateObjectFromPath(TEXT("C:\\Windows\\SysWOW64\\evr.dll"), CLSID_EnhancedVideoRenderer, &pUnk );
		//if (SUCCEEDED(hr))
		//{
		//	pFilter = pUnk;
		//	hr = m_pGraphBuilder->AddFilter( pFilter, L"Enhanced Video Renderer" );
		//}
		
		hr = m_pGraphBuilder->AddFilter( pRenderer, L"Video Renderer" );
		hr = ConnectFilters(m_pGraphBuilder, pMSDTV, pRenderer );

		//hr = ConnectFilters(m_pGraphBuilder, pBaseFilter, pVideoDecoder );
		//hr = ConnectFilters(m_pGraphBuilder, pBaseFilter, pMSDTV );
		//hr = m_pGraphBuilder->RenderFile( wFile, NULL );
		
  		//hr = RenderOutputPins( m_pGraphBuilder, pBaseFilter );
	}
#endif

	if ( FAILED(hr) )
	{
		if ( bAutoRender )
			DisplayStatusMessage(_T("재생할 수 있는 코덱이 설치되어 있지 않습니다."), MESSAGE_COLOR_ERROR );
		else
			DisplayStatusMessage(_T("필터 그래프 구성이 올바르지 않습니다."), MESSAGE_COLOR_ERROR );
		ReleaseAll();
		return 0;
	}

//#ifdef _DEBUG
	hr = AddGraphToRot( m_pGraphBuilder, sFilename, &m_dwGraphRegister );
//#endif

	//SetColorKey();
	
	m_hParentWnd = GetParent()->GetSafeHwnd();
	m_pVideoWindow->put_Owner((OAHWND)m_hParentWnd);
	//m_pVideoWindow->put_WindowStyle( WS_CHILD );
	m_pVideoWindow->put_WindowStyle(WS_CHILD | WS_CLIPSIBLINGS | WS_CLIPCHILDREN);
	m_pVideoWindow->put_MessageDrain((OAHWND)m_hParentWnd);

	m_szVideo = GetVideoSize();

	GetFrameStepInterface();

	m_dDuration	= GetMediaDuration();
	m_sMediaFile = sFile;
	DisplayStatusMessage(_T(""));

	//NOT_IMPLEMENTED?
	//LONGLONG	pllTotal = 0;
	//LONGLONG	pllCurrent = 0;
	//pFileSource->QueryInterface(IID_IAMOpenProgress, (void**)&m_pOpenProgress);
	//hr = m_pOpenProgress->QueryProgress( &pllTotal, &pllCurrent );
	//if ( FAILED(hr) )
	//	ShowHResultErrorMessage( hr );

	return 1;
}

void CDShowWnd::SetRatioMode( int nMode )
{
	// 전체화면인 경우와 일반/최대화면인 경우는 다르다.
	if ( !m_pMixerPinConfig2 )
		return;

	if ( nMode < AM_ARMODE_STRETCHED || nMode > AM_ARMODE_STRETCHED_AS_PRIMARY )
		return;

	m_pMixerPinConfig2->SetAspectRatioMode( (AM_ASPECT_RATIO_MODE)nMode );
}

void CDShowWnd::SetColorKey( IBaseFilter* pOvMix )
{
	if( pOvMix )
		m_pOverlayMixer = pOvMix;

	IPin* pPin = NULL;
	
	bKeyColorIsOk		= FALSE;
	BOOL       bFound	= FALSE;
    IEnumPins  *pEnum;

    m_pOverlayMixer->EnumPins(&pEnum);
    while(pEnum->Next(1, &pPin, 0) == S_OK)
    {
        PIN_DIRECTION PinDirThis;
        pPin->QueryDirection(&PinDirThis);
        if (bFound = (PINDIR_INPUT == PinDirThis))
            break;
        pPin->Release();
    }
	
	HRESULT hr = pPin->QueryInterface( IID_IOverlay, (void**)&m_pOverlay );
	hr = pPin->QueryInterface( IID_IMixerPinConfig2, (void**)&m_pMixerPinConfig2 );   

	if( SUCCEEDED(hr) )
	{
		COLORKEY clrkey;

		hr = m_pOverlay->SetColorKey( &clrkey );
		if( hr == VFW_E_NO_COLOR_KEY_SET )   
			TRACE( "VFW_E_NO_COLOR_KEY_SET\r\n" );

		m_dwColorKey = clrkey.HighColorValue;// = 0x100010
		/*
		if ( NOERROR == hr )
		{
			bKeyColorIsOk = TRUE;
			m_pOV->GetColorKey( &ckey );
			KeyColor = ckey.LowColorValue;
		}
		else
		{
			ckey.LowColorValue	= RGB(6, 6, 6);
			ckey.HighColorValue = RGB(31, 31, 31);
			
			hr = m_pOV->SetColorKey( &ckey );

			if ( SUCCEEDED(hr) )
			{
				bKeyColorIsOk = TRUE;
				m_pOV->GetColorKey( &ckey );
				KeyColor = ckey.LowColorValue;
			}
			else
			{
				if( hr == VFW_E_NO_COLOR_KEY_SET )
				{

				}
				else
				{
				}
			}
		}
		*/
		m_pOverlay->SetPalette( 0, NULL );
		hr = m_pOverlay->SetColorKey( &clrkey );

		TRACE( "HRESULT: %dl\r\n", hr );

		switch( hr )
		{
			case VFW_E_PALETTE_SET:
				TRACE( "VFW_E_PALETTE_SET\r\n" );
				break;
			case VFW_E_COLOR_KEY_SET:
				TRACE( "VFW_E_COLOR_KEY_SET\r\n" );
				break;
			default:
				TRACE( "Unknown Error!\r\n" );
				break;
		}

		//hr = pPin->QueryInterface( IID_IMixerPinConfig2, (void**)&m_pMixerPinConfig2 );
		DDCOLORCONTROL pColorControl;
		ZeroMemory(&pColorControl, sizeof(pColorControl));
		pColorControl.dwSize = sizeof( DDCOLORCONTROL );

		pColorControl.dwFlags = DDCOLOR_BRIGHTNESS |
								DDCOLOR_CONTRAST |
								DDCOLOR_SATURATION;
			
		// 현재 오버레이카드에 세팅된 값을 얻어온다.
		if ( m_pMixerPinConfig2 )
			m_pMixerPinConfig2->GetOverlaySurfaceColorControls( &pColorControl );
			
		nBrightDefault		= pColorControl.lBrightness;	//	= 750;
		nContrastDefault	= pColorControl.lContrast;		//	= 10000;
		nSaturationDefault	= pColorControl.lSaturation;	//	= 10000;

		if ( nBrightDefault < 0 )
			nBrightDefault = 0;
		if ( nBrightDefault > 20000 )
			nBrightDefault = 20000;
		if ( nContrastDefault < 0 )
			nContrastDefault = 0;
		if ( nContrastDefault > 10000 )
			nContrastDefault = 10000;
		if ( nSaturationDefault < 0 )
			nSaturationDefault = 0;
		if ( nSaturationDefault > 10000 )
			nSaturationDefault = 10000;

			// 얻어온 값을 트랙조절패널 세팅에 넘겨주고
//			if ( g_pLegendsVideo == NULL )
				//((CEndorphinDlg*)GetParent())->SetScreenControl( nBrightDefault, nContrastDefault, nSaturationDefault );
		}

		// 자신들은 원래 기본값을 가지고 시작한다.
		nBrightDefault		= 750;
		nContrastDefault	= 10000;
		nSaturationDefault	= 10000;

	pPin->Release();
    pEnum->Release();

	if( pOvMix )
		m_pOverlayMixer = NULL;
}

BOOL CDShowWnd::GetFrameStepInterface()
{
    HRESULT hr;

    // Get the frame step interface, if supported
    hr = m_pGraphBuilder->QueryInterface(__uuidof(IVideoFrameStep), (PVOID *)&m_pVideoFrameStep);

    if (FAILED(hr))
        return FALSE;

    // Check if this decoder can step
    hr = m_pVideoFrameStep->CanStep(0L, NULL);

    if (hr == S_OK)
    {
        return TRUE;
    }
    else
    {
        m_pVideoFrameStep->Release();
        return FALSE;
    }
}

HRESULT CDShowWnd::AddGraphToRot(IUnknown *pUnkGraph, CString sTitle, DWORD *pdwRegister)
{
    IMoniker * pMoniker;
    IRunningObjectTable *pROT;
    if (FAILED(GetRunningObjectTable(0, &pROT))) 
    {
        return E_FAIL;
    }

    WCHAR wsz[128];
    wsprintfW(wsz, L"%S FilterGraph %08x pid %08x", sTitle, (DWORD_PTR)pUnkGraph, 
              GetCurrentProcessId());

    HRESULT hr = CreateItemMoniker(L"!", wsz, &pMoniker);
    if (SUCCEEDED(hr)) 
    {
        hr = pROT->Register(0, pUnkGraph, pMoniker, pdwRegister);
        pMoniker->Release();
    }

    pROT->Release();
    return hr;
}

void CDShowWnd::RemoveGraphFromRot(DWORD pdwRegister)
{
   IRunningObjectTable *pROT;

    if (SUCCEEDED(GetRunningObjectTable(0, &pROT))) 
    {
        pROT->Revoke(pdwRegister);
        pROT->Release();
    }
}

// 사용한 리소스를 모두 해제한다.
void CDShowWnd::ReleaseAll()
{
	HCURSOR hCursor = AfxGetApp()->LoadStandardCursor( IDC_WAIT );
	::SetCursor( hCursor );

    HRESULT hr;
	
	if ( m_pMediaControl )
        hr = m_pMediaControl->Stop();

	// 이벤트 콜백(callback)을 중지한다.
    if (m_pEvent)
        hr = m_pEvent->SetNotifyWindow((OAHWND)NULL, 0, 0);
	
    if ( m_pVideoWindow )
    {
        hr = m_pVideoWindow->put_Visible(OAFALSE);
        hr = m_pVideoWindow->put_Owner(NULL);
    }	

#ifdef _DEBUG//UNREGISTER_FILTERGRAPH
    if( m_dwGraphRegister )
    {
        RemoveGraphFromRot( m_dwGraphRegister );
        m_dwGraphRegister = 0;
    }
#endif

    // Release and zero DirectShow interfaces
	SAFE_RELEASE( m_pBasicVideo );
	SAFE_RELEASE( m_pBasicAudio );
	SAFE_RELEASE( m_pMediaSeeking );
	SAFE_RELEASE( m_pVideoWindow );
    SAFE_RELEASE( m_pEvent );
	SAFE_RELEASE( m_pMediaControl );
	SAFE_RELEASE( m_pMixerPinConfig2 );
	SAFE_RELEASE( m_pVideoFrameStep );
	SAFE_RELEASE( m_pOverlayMixer );
	SAFE_RELEASE( m_pOverlay );
	SAFE_RELEASE( m_pGraphBuilder );

	hCursor = AfxGetApp()->LoadStandardCursor( IDC_ARROW );
	::SetCursor( hCursor );

	CoUninitialize();
}

void CDShowWnd::OnDestroy() 
{
	CWnd::OnDestroy();
	
	// TODO: Add your message handler code here
	ReleaseAll();
	CoUninitialize();
}

CRect CDShowWnd::GetWindowPosition()
{
	if ( !m_pVideoWindow )
		return CRect( 0, 0, 0, 0 );
	else
	{
		CRect	Rect;
		m_pVideoWindow->GetWindowPosition( &Rect.left, &Rect.top, &Rect.right, &Rect.bottom );
		Rect.right	= Rect.left + Rect.right;
		Rect.bottom	= Rect.top + Rect.bottom;
		ScreenToClient( Rect );
		return Rect;
	}
}

void CDShowWnd::SetWindowPosition()
{
	if ( !m_pVideoWindow )
		return;

	CRect	Rect;
	GetParent()->GetClientRect( Rect );
//	MoveWindow( Rect );
	CSize	Size	= GetVideoSize();

	// 화면 비율의 보정은 전체화면일 경우만 해준다.
	if ( false )//((CEndorphinDlg*)GetParent())->bMaximized )
	{
		double	dRatio;
		int		nVideo;
		int		nSide1;
		int		nSide2;

		if ( Size.cx > Size.cy )
		{
			dRatio		= (double)Size.cx / (double)Size.cy;
			nVideo		= (int)( (double)(Rect.Width()) / dRatio );
			nSide1		= (int)((double)(Rect.Height() - nVideo) / 2.0);
			nSide2		= Rect.Height() - nSide1 - nVideo;
			Rect.top	= Rect.top + nSide1;
			Rect.bottom	= Rect.bottom - nSide2;
		}
		else
		{
			dRatio		= (double)Size.cy / (double)Size.cx;
			nVideo		= (int)( (double)(Rect.Height()) / dRatio );
			nSide1		= (int)((double)(Rect.Width() - nVideo) / 2.0);
			nSide2		= Rect.Width() - nSide1 - nVideo;
			Rect.left	= Rect.left + nSide1;
			Rect.right	= Rect.right - nSide2;
		}
	}

	double dWidth	= (double)Rect.Width();
	double dHeight	= (double)Rect.Height();

	//Rect.left	= Rect.left		+ (int)(dWidth * m_dLeft);
	//Rect.top	= Rect.top		+ (int)(dHeight * m_dTop);
	//Rect.right	= Rect.right	+ (int)(dWidth * m_dRight);
	//Rect.bottom	= Rect.bottom	+ (int)(dHeight * m_dBottom);
	//TRACE( "%d, %d, %d, %d\n", Rect.left, Rect.top, Rect.right, Rect.bottom );

	
//	Rect.DeflateRect( 0, 30 );
	
	m_pVideoWindow->SetWindowForeground( OAFALSE );
	m_pVideoWindow->SetWindowPosition( Rect.left, Rect.top, Rect.Width(), Rect.Height() );
}

void CDShowWnd::SetWindowPosition( CRect r )
{
	SetWindowPosition( r.left, r.top, r.Width(), r.Height() );
}

void CDShowWnd::SetWindowPosition( int left, int top, int width, int height )
{
	if ( IsWindow( m_hWnd ) == false )
		return;

	MoveWindow( left, top, width, height );

	if ( m_pVideoWindow == NULL )
		return;

	m_pVideoWindow->SetWindowPosition( left, top, width, height );
}

int CDShowWnd::GetPlayState()
{
	if ( !m_pMediaControl )
		return 0;

	OAFilterState	pfs;
	m_pMediaControl->GetState( 500, &pfs );

	return pfs;
}

void CDShowWnd::Play()
{
	if ( !m_pMediaControl )
		return;

	m_pMediaControl->Run();
}

void CDShowWnd::Pause()
{
	if ( !m_pMediaControl )
		return;

	m_pMediaControl->Pause();
}

void CDShowWnd::Stop()
{
	if ( !m_pMediaControl )
		return;

	Pause();
	SetCurrentPosition( 0.0 );
	//m_pMediaControl->Stop();
}

void CDShowWnd::Close()
{
//	if ( !m_pMediaControl )
//		return;
	m_sMediaFile = "";
	ReleaseAll();
}

CSize CDShowWnd::GetVideoSize()
{
    if ( !m_pBasicVideo )
        return CSize( 0, 0 );

    LONG lWidth, lHeight;

	// 비디오파일인데 GetMediaInfo 함수에서 정확히 구하지 못했다면
	// IBasicVideo 인터페이스를 이용하여 구해준다.
	m_pBasicVideo->GetVideoSize( &lWidth, &lHeight );
	
	return CSize( lWidth, lHeight );
}

// 밀리초 단위의 위치로 이동한다.
void CDShowWnd::SetCurrentPosition( double lNewPos )
{
	if ( !m_pMediaSeeking )
		return;

	HRESULT hr;

	if ( lNewPos < 0.0 )
		lNewPos = 0.0;
	
	LONGLONG pos = (LONGLONG) (lNewPos * 10000.0);

	// 새로운 위치로 이동
	//SetWindowRedraw( m_hWnd, FALSE );
	m_pMediaSeeking->SetPositions(&pos, AM_SEEKING_AbsolutePositioning,
		NULL, AM_SEEKING_NoPositioning );
}

// 밀리초 단위의 현재 재생 위치를 얻는다.
double CDShowWnd::GetCurrentPosition( BOOL bMilliSec )
{
	if ( !m_pMediaSeeking )
		return 0.0;

	HRESULT hr;
	LONGLONG pos = 0;

	//hr = m_pMediaSeeking->IsFormatSupported( &TIME_FORMAT_FRAME );
	//hr = m_pMediaSeeking->IsFormatSupported( &TIME_FORMAT_BYTE );
	//hr = m_pMediaSeeking->IsFormatSupported( &TIME_FORMAT_SAMPLE );
	//hr = m_pMediaSeeking->IsFormatSupported( &TIME_FORMAT_FIELD );

	// 현재 타임 포맷을 100 나노초 단위의 미디어 타임으로 설정
	m_pMediaSeeking->SetTimeFormat(&TIME_FORMAT_MEDIA_TIME);
	
	// 현재 타임 포맷에 의한 위치를 얻는다. 
	m_pMediaSeeking->GetCurrentPosition(&pos);

	if ( bMilliSec )
		return (double)pos / 10000.0;
	else
		return (double)pos / 10000000.0;
}

void CDShowWnd::SetVolume( int nVolume )
{
	if ( !m_pBasicAudio )
		return;

	if ( nVolume < 0 )
		nVolume = 0;
	if ( nVolume > VOLUME_MAX )
		nVolume = VOLUME_MAX;

	m_nVolume = nVolume;

	double d = -1.0 * pow( 1.0964, VOLUME_MAX - nVolume );
	m_pBasicAudio->put_Volume( d );
}

void CDShowWnd::ShowWindow( bool bShow )
{
	if ( !m_pVideoWindow )
		return;

	HRESULT		hr;

	//비디오 창 뿐만 아니라 DShowWnd 자체도 함께 감춰줘야 한다.
	if ( bShow )
	{
		hr = m_pVideoWindow->put_Visible( OATRUE );
		CWnd::ShowWindow( SW_SHOW );
	}
	else
	{
		hr = m_pVideoWindow->put_Visible( OAFALSE );
		CWnd::ShowWindow( SW_HIDE );
	}

	ShowHResultErrorMessage( hr );
}

//ms 단위로 전체 미디어의 길이를 얻는다.
double CDShowWnd::GetMediaDuration()
{
	if ( !m_pMediaSeeking )
		return 0;

	HRESULT hr;
	LONGLONG duration = 0;

	// 현재 타임 포맷을 100 나노초 단위의 미디어 타임으로 설정
	//if ( m_pMediaSeeking->IsFormatSupported( &TIME_FORMAT_FRAME) == S_OK )
	//	AfxMessageBox( "m_pMediaSeeking->SetTimeFormat(&TIME_FORMAT_FRAME);" );
	//else if ( m_pMediaSeeking->IsFormatSupported( &TIME_FORMAT_SAMPLE) == S_OK )
	//	AfxMessageBox( "m_pMediaSeeking->SetTimeFormat(&TIME_FORMAT_SAMPLE);" );
	//else
		m_pMediaSeeking->SetTimeFormat(&TIME_FORMAT_MEDIA_TIME);

	// 재생중인 미디어의 길이를 얻는다.
	m_pMediaSeeking->GetDuration(&duration);

	m_dDuration = (double)duration / 10000.0;
	return m_dDuration;
}

void CDShowWnd::EnumFilters()
{
    // Reset filter number
	nFilter = 0;

	if ( m_pGraphBuilder == NULL )
		return;

    HRESULT hr;
    IEnumFilters *pEnum = NULL;
    IBaseFilter *pFilter = NULL;
    ULONG cFetched;
    
    // Get filter enumerator
    hr = m_pGraphBuilder->EnumFilters(&pEnum);
    if ( FAILED(hr) )
        return;

    // Enumerate all filters in the graph
    while ( pEnum->Next(1, &pFilter, &cFetched) == S_OK )
    {
        FILTER_INFO FilterInfo;
        TCHAR szName[512];
        
        hr = pFilter->QueryFilterInfo(&FilterInfo);
        if (FAILED(hr))
        {
            //m_ListFilters.AddString(TEXT("<ERROR>"));
//			AfxMessageBox( "Error" );
        }
        else
        {
            // Add the filter name to the filters listbox
            USES_CONVERSION;

            lstrcpy(szName, W2T(FilterInfo.achName));
            
			CString sCodecName = szName;
			if ( m_sMediaFile.Find(sCodecName) < 0 )
			{
				sFilter[nFilter] = szName;

				ISpecifyPropertyPages *pSpecify;

				// Discover if this filter contains a property page
				hr = pFilter->QueryInterface(IID_ISpecifyPropertyPages, (void **)&pSpecify);
				if (SUCCEEDED(hr)) 
				{
					pSpecify->Release();
					bFilter[nFilter] = TRUE;
				}
				else
					bFilter[nFilter] = FALSE;

				nFilter++;
				if ( nFilter == 10 )
					return;
			}

            FilterInfo.pGraph->Release();
        }       
        pFilter->Release();
    }
    pEnum->Release();
}

IBaseFilter *CDShowWnd::FindFilterByNameInGraph( CString sFilter )
{
    USES_CONVERSION;

    HRESULT hr;
    IEnumFilters *pEnum = NULL;
    IBaseFilter *pFilter = NULL;
    ULONG cFetched;
    BOOL bFound = FALSE;

    // Get filter enumerator
    hr = m_pGraphBuilder->EnumFilters(&pEnum);
    if (FAILED(hr))
        return NULL;

    // Enumerate all filters in the graph
    while((pEnum->Next(1, &pFilter, &cFetched) == S_OK) && (!bFound))
    {
        FILTER_INFO FilterInfo;
        TCHAR szName[256];
        
        hr = pFilter->QueryFilterInfo(&FilterInfo);
        if (FAILED(hr))
        {
            pFilter->Release();
            pEnum->Release();
            return NULL;
        }

        // Compare this filter's name with the one we want
        lstrcpy(szName, W2T(FilterInfo.achName));
        if (! lstrcmp(szName, sFilter))
        {
            bFound = TRUE;
        }

        FilterInfo.pGraph->Release();

        // If we found the right filter, don't release its interface.
        // The caller will use it and release it later.
        if (!bFound)
            pFilter->Release();
        else
            break;
    }
    pEnum->Release();

    return (bFound ? pFilter : NULL);
}

void CDShowWnd::ShowFilterPropertyPage( CString sFilterName ) 
{
    HRESULT hr;
    IBaseFilter *pFilter = NULL;
    TCHAR szNameToFind[128];
    ISpecifyPropertyPages *pSpecify;

	_stprintf( szNameToFind, _T("%s"), sFilterName );
    // Read the current list box name and find it in the graph
    pFilter = FindFilterByNameInGraph(szNameToFind);
    if (!pFilter)
        return;

    // Discover if this filter contains a property page
    hr = pFilter->QueryInterface(IID_ISpecifyPropertyPages, (void **)&pSpecify);
    if (SUCCEEDED(hr)) 
    {
        do 
        {
            FILTER_INFO FilterInfo;
            hr = pFilter->QueryFilterInfo(&FilterInfo);
            if (FAILED(hr))
                break;

            CAUUID caGUID;
            hr = pSpecify->GetPages(&caGUID);
            if (FAILED(hr))
                break;

            pSpecify->Release();
        
            // Display the filter's property page
            OleCreatePropertyFrame(
                m_hWnd,                 // Parent window
                0,                      // x (Reserved)
                0,                      // y (Reserved)
                FilterInfo.achName,     // Caption for the dialog box
                1,                      // Number of filters
                (IUnknown **)&pFilter,  // Pointer to the filter 
                caGUID.cElems,          // Number of property pages
                caGUID.pElems,          // Pointer to property page CLSIDs
                0,                      // Locale identifier
                0,                      // Reserved
                NULL                    // Reserved
            );
            CoTaskMemFree(caGUID.pElems);
            FilterInfo.pGraph->Release(); 

        } while(0);
    }

    pFilter->Release();
}

HRESULT CDShowWnd::CreateFilter(REFCLSID clsid, IBaseFilter **ppFilter)
{
    HRESULT hr;

    hr = CoCreateInstance(clsid, NULL, CLSCTX_INPROC_SERVER,
        IID_IBaseFilter,
        (void **) ppFilter);

    if(FAILED(hr))
    {
		//Msg(_T("CreateFilter: Failed to create filter!  hr=0x%x\n"), hr);
        if (ppFilter)
            *ppFilter = NULL;
        return hr;
    }

    return S_OK;
}

HRESULT CDShowWnd::RenderOutputPins(IGraphBuilder *pGB, IBaseFilter *pFilter)
{
	/*
    CComPtr <IEnumPins>     pEnum;
    CComPtr <IPin>          pPin;
    CComQIPtr<IFilterGraph2, &IID_IFilterGraph2> pFilterGraph2(pGB);
    HRESULT hr =S_OK;

    hr = pFilter->EnumPins(&pEnum);
    if (SUCCEEDED(hr))
    {
         while(pEnum->Next(1, &pPin, 0) == S_OK)
         {
               PIN_DIRECTION PinDirThis;
               hr = pPin->QueryDirection(&PinDirThis);

              if(SUCCEEDED(hr))
              {
                   if (PINDIR_OUTPUT == PinDirThis)
                   {
                            CComPtr<IPin> pConnectedPin;

                           hr = pPin->ConnectedTo(&pConnectedPin);

                          if(VFW_E_NOT_CONNECTED == hr)
                          {
                               if (!pConnectedPin)
                              {
                                  hr = pFilterGraph2->RenderEx(pPin, AM_RENDEREX_RENDERTOEXISTINGRENDERERS, NULL);
       
                              }
                          }
                   }
              }
              pPin.Release();
        }
    }
    return hr;  
	*/

	HRESULT         hr = S_OK;
	IEnumPins *     pEnumPin = NULL;
	IPin *          pConnectedPin = NULL, * pPin = NULL;
	PIN_DIRECTION   PinDirection;
	ULONG           ulFetched;
	BOOL			bAtLeastOnePinRendered = FALSE;

	// Enumerate all pins on the filter
	hr = pFilter->EnumPins(&pEnumPin);
	
	if(SUCCEEDED(hr))
	{
		// Step through every pin, looking for the output pins
		while (S_OK == (hr = pEnumPin->Next(1L, &pPin, &ulFetched)))
		{
			// Is this pin connected?  We're not interested in connected pins.
			hr = pPin->ConnectedTo(&pConnectedPin);
			if (pConnectedPin)
			{
				pConnectedPin->Release();
				pConnectedPin = NULL;
			}
			
			// If this pin is not connected, render it.
			if (VFW_E_NOT_CONNECTED == hr)
			{
				hr = pPin->QueryDirection(&PinDirection);
				if ((S_OK == hr) && (PinDirection == PINDIR_OUTPUT))
				{
					hr = pGB->Render(pPin);
				}
			}
			pPin->Release();
			
			// scpark
			// 출력핀들을 렌더할 때 하나의 핀이라도 렌더된다면 렌더시키자.
			// 어떤 미디어는 두 개의 핀을 가지지만 하나만 렌더되는 미디어도 있다.
			// 따라서 첫번째 핀 렌더가 실패했어도 두번째 핀렌더가 성공한다면
			// 렌더는 이루어진것으로 판단한다.
			if ( SUCCEEDED(hr) )
				bAtLeastOnePinRendered = TRUE;
			// If there was an error, stop enumerating
			if (FAILED(hr))
				break;
		}
	}
	
	// Release pin enumerator
	pEnumPin->Release();

	if ( bAtLeastOnePinRendered )
		return 0;
	
	return hr;

}

void CDShowWnd::FindAudioRenderer()
{
	nAudioFilter = 0;

    HRESULT hr;    
    IEnumMoniker *pEnumCat = NULL;

    // Instantiate the system device enumerator
    ICreateDevEnum	*m_pSysDevEnum = NULL;

    hr = CoCreateInstance(CLSID_SystemDeviceEnum, NULL, 
                          CLSCTX_INPROC, IID_ICreateDevEnum, 
                          (void **)&m_pSysDevEnum);
    if FAILED(hr)
    {
        CoUninitialize();
        return;
    }

    const CLSID *clsid;
    clsid = &CLSID_AudioRendererCategory;

    // If the CLSID wasn't allocated earlier, then fail
    if ( !clsid )
        return;

    //
    // WARNING!
    //
    // Some third-party filters throw an exception (int 3) during enumeration
    // on Debug builds, often due to heap corruption in RtlFreeHeap().
    // Sometimes it is done intentionally to prevent reverse engineering.
    // 
    // This is not an issue on Release builds.
    //

    // Enumerate all filters of the selected category  
    hr = m_pSysDevEnum->CreateClassEnumerator(*clsid, &pEnumCat, 0);
    ASSERT(SUCCEEDED(hr));
    if FAILED(hr)
        return;

    // Enumerate all filters using the category enumerator
    hr = EnumFilters(pEnumCat);

    SAFE_RELEASE(pEnumCat);

	if ( nAudioFilter > 0 )
	{
		CString		sIndex;
		for ( int i = 0; i < nAudioFilter; i++ )
		{
			sIndex.Format(_T("%d"), i );
			AfxGetApp()->WriteProfileString(_T("Config\\Sound"), sIndex, sAudioFilter[i] );
		}

		AfxGetApp()->WriteProfileInt(_T("Config\\Sound"), _T("nCount"), nAudioFilter );
	}
}

HRESULT CDShowWnd::EnumFilters(IEnumMoniker *pEnumCat)
{
    HRESULT hr=S_OK;
    IMoniker *pMoniker;
    ULONG cFetched;
    VARIANT varName={0};

	nAudioFilter = 0;

    // If there are no filters of a requested type, show default string
    if ( !pEnumCat )
    {
        return S_FALSE;
    }

    // Enumerate all items associated with the moniker
    while(pEnumCat->Next(1, &pMoniker, &cFetched) == S_OK)
    {
        IPropertyBag *pPropBag;
        ASSERT(pMoniker);

        // Associate moniker with a file
        hr = pMoniker->BindToStorage(0, 0, IID_IPropertyBag, 
                                    (void **)&pPropBag);
        ASSERT(SUCCEEDED(hr));
        ASSERT(pPropBag);
        if (FAILED(hr))
            continue;

        // Read filter name from property bag
        varName.vt = VT_BSTR;
        hr = pPropBag->Read(L"FriendlyName", &varName, 0);
        if (FAILED(hr))
            continue;

        // Get filter name (converting BSTR name to a CString)
        CString str(varName.bstrVal);
        SysFreeString(varName.bstrVal);

        // Read filter's CLSID from property bag.  This CLSID string will be
        // converted to a binary CLSID and passed to AddFilter(), which will
        // add the filter's name to the listbox and its CLSID to the listbox
        // item's DataPtr item.  When the user clicks on a filter name in
        // the listbox, we'll read the stored CLSID, convert it to a string,
        // and use it to find the filter's filename in the registry.
        VARIANT varFilterClsid;
        varFilterClsid.vt = VT_BSTR;

        // Read CLSID string from property bag
        hr = pPropBag->Read(L"CLSID", &varFilterClsid, 0);
        if(SUCCEEDED(hr))
        {
            CLSID clsidFilter;

            // Add filter name and CLSID to listbox
            if(CLSIDFromString(varFilterClsid.bstrVal, &clsidFilter) == S_OK)
            {
                //AddFilter(str, &clsidFilter);
				sAudioFilter[nAudioFilter++] = str;
            }

            SysFreeString(varFilterClsid.bstrVal);
        }
       
        // Cleanup interfaces
        SAFE_RELEASE(pPropBag);
        SAFE_RELEASE(pMoniker);
    }

	return S_OK;
}

HRESULT CDShowWnd::FindFilter(CString compFiterName, REFCLSID clsID, IBaseFilter **ppSrcFilter)
{
    HRESULT hr=S_OK;
    IBaseFilter * pSrc = NULL;
    IMoniker* pMoniker =NULL;
    ULONG cFetched;
    VARIANT varName={0};

    if (!ppSrcFilter)
        return E_POINTER;
   
    // Create the system device enumerator
    CComPtr <ICreateDevEnum> pDevEnum =NULL;

    hr = CoCreateInstance (CLSID_SystemDeviceEnum, NULL, CLSCTX_INPROC,
                           IID_ICreateDevEnum, (void **) &pDevEnum);
    if (FAILED(hr))
    {
        return hr;
    }

    // Create an enumerator for the video capture devices
    CComPtr <IEnumMoniker> pClassEnum = NULL;

    hr = pDevEnum->CreateClassEnumerator( clsID, &pClassEnum, 0 );

    if (FAILED(hr))
    {
        return hr;
    }

    // If there are no enumerators for the requested type, then 
    // CreateClassEnumerator will succeed, but pClassEnum will be NULL.
    if (pClassEnum == NULL)
    {
        return E_FAIL;
    }

    // Use the first video capture device on the device list.
    // Note that if the Next() call succeeds but there are no monikers,
    // it will return S_FALSE (which is not a failure).  Therefore, we
    // check that the return code is S_OK instead of using SUCCEEDED() macro.
    while (S_OK == (pClassEnum->Next (1, &pMoniker, &cFetched)))
    {
        IPropertyBag *pPropBag;
        ASSERT(pMoniker);

        // Associate moniker with a file
        hr = pMoniker->BindToStorage(0, 0, IID_IPropertyBag, 
                                    (void **)&pPropBag);
        ASSERT(SUCCEEDED(hr));
        ASSERT(pPropBag);
        if (FAILED(hr))
            continue;

        // Read filter name from property bag
        varName.vt = VT_BSTR;
        hr = pPropBag->Read(L"FriendlyName", &varName, 0);
        if (FAILED(hr))
            continue;

        // Get filter name (converting BSTR name to a CString)
        CString str(varName.bstrVal);
        SysFreeString(varName.bstrVal);

		//TRACE( "%s\n", str );

		if(str.Compare(compFiterName) == 0)
		{
			// Bind Moniker to a filter object
             hr = pMoniker->BindToObject(0,0,IID_IBaseFilter, (void**)&pSrc);
             if (FAILED(hr))
			 {
                  return hr;
			 }
             SAFE_RELEASE(pPropBag);
             *ppSrcFilter = pSrc;
			 return hr;
		}

        // Cleanup interfaces
        SAFE_RELEASE(pPropBag);
        SAFE_RELEASE(pMoniker);
    }

    // Copy the found filter pointer to the output parameter.
    // Do NOT Release() the reference, since it will still be used
    // by the calling function.


    return hr;
}

void CDShowWnd::StepFrame( bool bForward )
{
	if ( m_pVideoFrameStep == NULL )
		return;

	/*
	DWORD dwCapabilities;
	m_pMediaSeeking->GetCapabilities( &dwCapabilities );
	if ( dwCapabilities & AM_SEEKING_CanSeekAbsolute )
		TRACE( "AM_SEEKING_CanSeekAbsolute ok\n" );
	if ( dwCapabilities & AM_SEEKING_CanSeekForwards )
		TRACE( "AM_SEEKING_CanSeekForwards ok\n" );
	if ( dwCapabilities & AM_SEEKING_CanSeekBackwards )
		TRACE( "AM_SEEKING_CanSeekBackwards ok\n" );
	if ( dwCapabilities & AM_SEEKING_CanGetCurrentPos )
		TRACE( "AM_SEEKING_CanGetCurrentPos ok\n" );
	if ( dwCapabilities & AM_SEEKING_CanGetStopPos )
		TRACE( "AM_SEEKING_CanGetStopPos ok\n" );
	if ( dwCapabilities & AM_SEEKING_CanGetDuration )
		TRACE( "AM_SEEKING_CanGetDuration ok\n" );
	if ( dwCapabilities & AM_SEEKING_CanPlayBackwards )
		TRACE( "AM_SEEKING_CanPlayBackwards ok\n" );
	if ( dwCapabilities & AM_SEEKING_CanDoSegments )
		TRACE( "AM_SEEKING_CanDoSegments ok\n" );
	if ( dwCapabilities & AM_SEEKING_Source )
		TRACE( "AM_SEEKING_Source ok\n" );
	*/

	if ( bForward )
	{
		m_pVideoFrameStep->Step( 1, NULL );
	}
	else
	{
		double d = GetCurrentPosition( true );
		SetCurrentPosition( d - 33.366667 * 2.0 );
		m_pVideoFrameStep->Step( 1, NULL );
		//m_pVideoFrameStep->Step( 1, NULL );
	}
}

BOOL CDShowWnd::PreTranslateMessage(MSG* pMsg) 
{
	// TODO: Add your specialized code here and/or call the base class
	//TRACE( "%ld\n", pMsg->message );

	if (pMsg->message == WM_GRAPHNOTIFY)
	{
		if (m_pGraphBuilder == NULL)
			return TRUE;

		HRESULT hr ;

		LONG lEvent, lParam1, lParam2;
		LONG lTimeOut = 0;

//		if ( !m_pEvent )
//			return TRUE;
			CString s;
			s.Format(_T("%ld\n"), pMsg->lParam );
			//TRACE( s );

		while (SUCCEEDED(m_pEvent->GetEvent(&lEvent, (LONG_PTR*)&lParam1, (LONG_PTR*)&lParam2, lTimeOut)))
		{
			hr = m_pEvent->FreeEventParams(lEvent, lParam1, lParam2);
			switch (lEvent)
			{
			// TODO: Place code here.
			case EC_LENGTH_CHANGED:
				{
					//MessageBeep(0);
					TRACE(_T("EC_LENGTH_CHANGED\n"));
				}
				break;
			case EC_BUFFERING_DATA:
				{
					//IAMNetworkStatus* g_pAMNetworkStatus = NULL;
					//m_pGraphBuilder->QueryInterface(IID_IAMNetworkStatus, (void **)&g_pAMNetworkStatus);
					//MessageBeep(0);
					TRACE( "EC_BUFFERING_DATA\n" );
				}
				break;
#if 0
			case EC_OLE_EVENT:
				{
					TRACE( "EC_OLE_EVENT\n" );
					int nLength1 = SysStringLen((BSTR)lParam1) * 2 + 1;
					TCHAR* pszBuf1 = new TCHAR[nLength1];
					WideCharToMultiByte(CP_ACP, 0, 
						(BSTR)lParam1,
						-1,
						pszBuf1,
						nLength1,
						NULL, NULL);

					int nLength2 = SysStringLen((BSTR)lParam2) * 2 + 1;
					TCHAR* pszBuf2 = new TCHAR[nLength2];
					WideCharToMultiByte(CP_ACP, 0, 
						(BSTR)lParam2,
						-1,
						pszBuf2,
						nLength2,
						NULL, NULL);

					CString sText;
					sText.Format( "EC_OLE_EVENT : %s", pszBuf2 );
					TRACE( "%s\n", sText );
					//if ( bKeyColorIsOk )
						//((CEndorphinDlg*)GetParent())->OnGraphEvent( sText );

					delete[] pszBuf1;
					delete[] pszBuf2;
				}
				break;
#endif
			}

			//hr = m_pEvent->FreeEventParams(lEvent, lParam1, lParam2);
		}

		return TRUE;
	}
	/*
	else if (pMsg->message == WM_LBUTTONDOWN)
	{
		return false;
	}
	*/
	return CWnd::PreTranslateMessage(pMsg);
}
/*
#include "Vfw.h"
#include "Aviriff.h"

void CDShowWnd::AVIFileCheck( CString sFile )
{
	bIsAVIFileType		= FALSE;
	bIsAVIFileValid		= TRUE;
	nAVIAudioStream		= 1;
	nAVIAudioIndex		= 0;

	FILE*	fp;

	if ( (fp = fopen( sFile, "rb" )) == NULL )
		return;

	fseek( fp, 0x8, SEEK_SET );		// Ox8번지부터 "AVI"
	
	char		Tag[3];
	CString		sStr;
	fread( Tag, sizeof(char), 3, fp );

	sStr = Tag;
	sStr = sStr.Left( 3 );
	if ( sStr == "AVI" )
		bIsAVIFileType = TRUE;

	if ( bIsAVIFileType )
	{
		AVIMAINHEADER	aviHeader;
		fseek( fp, 0x18, SEEK_SET );
		fread( &aviHeader, sizeof(AVIMAINHEADER), 1, fp );

		nAVIAudioStream = aviHeader.dwStreams - 1;

		if ( nAVIAudioStream > 4 )
			nAVIAudioStream = 4;
	}

	fclose( fp );
}
*/
void CDShowWnd::MyFreeMediaType(AM_MEDIA_TYPE& mt)
{
    if (mt.cbFormat != 0)
    {
        CoTaskMemFree((PVOID) mt.pbFormat);
        mt.cbFormat = 0;
        mt.pbFormat = NULL;
    }
    if (mt.pUnk != NULL)
    {
                // pUnk 는 사용하지 않는 것이 좋기 때문에 불필요하지만, 안전을 기하기 (위해)때문에.
        mt.pUnk->Release();
        mt.pUnk = NULL;
    }
}

void CDShowWnd::ShowHResultErrorMessage( HRESULT hr )
{
    if (FAILED(hr))
    {
        TCHAR szErr[MAX_ERROR_TEXT_LEN];
        DWORD res = ::AMGetErrorText(hr, szErr, MAX_ERROR_TEXT_LEN);
        if (res == 0)
        {
            wsprintf(szErr, _T("Unknown Error: 0x%2x"), hr);
        }
		
        MessageBox( szErr, TEXT("Error!"), MB_OK | MB_ICONERROR);
    }
}
#if 0
class CFakeCallback : public ISampleGrabberCB 
{
public:
    STDMETHODIMP_(ULONG) AddRef() { return 2; }
    STDMETHODIMP_(ULONG) Release() { return 1; }

    STDMETHODIMP QueryInterface(REFIID riid, void ** ppv)
    {
        if (riid == IID_ISampleGrabberCB || riid == IID_IUnknown) 
        {
            *ppv = (void *) static_cast<ISampleGrabberCB *>(this);
            return NOERROR;
        }    
        return E_NOINTERFACE;
    }

    STDMETHODIMP SampleCB( double SampleTime, IMediaSample * pSample )
    {
        static long counter = 0;
        printf( "Sample received = %05ld  Clock = %ld\r\n", 
                 counter++, timeGetTime( ) );
        return 0;
    }

    STDMETHODIMP BufferCB( double SampleTime, BYTE * pBuffer, long lBufferSize )
    {
        // write out a BMP file
        //
		//if ( !bOneShotMode )
		//	return FALSE;

        HANDLE hf = CreateFile(
            sImagePath, GENERIC_WRITE, 0, NULL,
            CREATE_ALWAYS, NULL, NULL );

        if( hf == INVALID_HANDLE_VALUE )
		{
			::SendMessage( hWnd_EndorphinDlg, MSG_CAPTURE_COMPLETE, 0, 0 );
            return 0;
		}

        // write out the file header
        //
        BITMAPFILEHEADER bfh;
        memset( &bfh, 0, sizeof( bfh ) );
        bfh.bfType = 'MB';
        bfh.bfSize = sizeof( bfh ) + lBufferSize + sizeof( BITMAPINFOHEADER );
        bfh.bfOffBits = sizeof( BITMAPINFOHEADER ) + sizeof( BITMAPFILEHEADER );

        DWORD dwWritten = 0;
        WriteFile( hf, &bfh, sizeof( bfh ), &dwWritten, NULL );

        // and the bitmap format
        //
        BITMAPINFOHEADER bih;
        memset( &bih, 0, sizeof( bih ) );
        bih.biSize = sizeof( bih );
        bih.biWidth = lImageWidth;
        bih.biHeight = lImageHeight;
        bih.biPlanes = 1;
        bih.biBitCount = 24;

        dwWritten = 0;
        WriteFile( hf, &bih, sizeof( bih ), &dwWritten, NULL );

        // and the bits themselves
        //
        dwWritten = 0;
        WriteFile( hf, pBuffer, lBufferSize, &dwWritten, NULL );
        CloseHandle( hf );
		
		::SendMessage( hWnd_EndorphinDlg, MSG_CAPTURE_COMPLETE, 0, 0 );
		bOneShotMode = FALSE;

		return NOERROR;
    }
};

void CDShowWnd::SetOSD()
{
	CComQIPtr<ILegendsVideo> pVideoFilter( g_pLegendsVideo );
	if (pVideoFilter != NULL)
		pVideoFilter->put_OSDSetting( &OSDSet );
}

void CDShowWnd::FlipVertical()
{
	CComQIPtr<ILegendsVideo> pVideoFilter( g_pLegendsVideo );
	if (pVideoFilter != NULL)
		pVideoFilter->put_FlipVertical();
}

BOOL CDShowWnd::CaptureImageByFilter( CString sImageName )
{
	if ( !m_pGraphBuilder || !MediaInfo.bIsVideo )
		return FALSE;

	int nOldState = GetState();

	CComQIPtr<ILegendsVideo> pVideoFilter( g_pLegendsVideo );
	if (pVideoFilter != NULL)
	{
		pVideoFilter->put_CaptureMode( 1 );
		char sImage[300];
		strcpy( sImage, sImageName );
		pVideoFilter->put_CaptureImage( (unsigned char*)sImage );
		StepFrame();

		int n = 1, nCount = 0;
		while ( n == 0 || n == -1 )
		{
			pVideoFilter->get_CaptureMode( &n );
			nCount++;
		}

		if ( nOldState == State_Running )
			Play();

		if ( n == -1 )
			return FALSE;
		
		return TRUE;
	}

	return FALSE;
}

BOOL CDShowWnd::CaptureImageByGrabber( CString sImageFile )
{
	// SampleGrabber를 이용한 캡처는 WMF인 경우 Seekable아니면 캡처되지 않는다.
	// 시도하려 하면서 다운된다.
	if ( (MediaInfo.bIsWMFFile && !MediaInfo.bIsSeekable) ||
		 (MediaInfo.bIsAVIFile && !MediaInfo.bIsAVIValid) )
		return FALSE;

	// 현재 타임 포맷을 100 나노초 단위의 미디어 타임으로 설정
	if ( !m_pMediaSeeking )
		return FALSE;
	
	LONGLONG	pos;
	m_pMediaSeeking->SetTimeFormat(&TIME_FORMAT_MEDIA_TIME);
	m_pMediaSeeking->GetCurrentPosition( &pos );

	sCapturedImageFile = sImageFile;
	
    CoInitialize( NULL );

    // Run the test on the filename specified on the command line
    USES_CONVERSION;

    CFakeCallback pCallback;
    HRESULT hr;

    // create a media detector
    //
    CComPtr< IMediaDet > pDet;
    hr = CoCreateInstance( CLSID_MediaDet, NULL, CLSCTX_INPROC_SERVER, 
                           IID_IMediaDet, (void**) &pDet );
    if( FAILED( hr ) ) 
    {
        printf( "Failed in CoCreateInstance!  hr=0x%x\r\n", hr );
        return FALSE;
    }

    // set filename
    //
    hr = pDet->put_Filename( T2W( MediaInfo.sMediaName ) );
    if( FAILED( hr ) ) 
    {
        printf( "couldn't load the file!  hr=0x%x\r\n", hr );
        return FALSE;// hr;
    }

    // look for a video stream
    //
    long Streams = 0;
    hr = pDet->get_OutputStreams( &Streams );
    if( FAILED( hr ) ) 
    {
        printf( "couldn't get the output streams!  hr=0x%x\r\n", hr );
        return FALSE;// hr;
    }

    BOOL bFoundVideo = FALSE;

    for( int i = 0 ; i < Streams ; i++ )
    {
        BOOL bIsVideo = FALSE;

        AM_MEDIA_TYPE Type;
		ZeroMemory( &Type, sizeof(Type) );

        // Select a media stream
        hr = pDet->put_CurrentStream( i );
        if( FAILED( hr ) ) 
        {
            printf( "couldn't put stream %d  hr=0x%x\r\n", i, hr );
            return FALSE;// hr;
        }

        // Read the media type of the selected stream
        hr = pDet->get_StreamMediaType( &Type );
        if( FAILED( hr ) ) 
        {
            printf( "couldn't get stream media type for stream %d  hr=0x%x\r\n",
                    i, hr );
            return FALSE;// hr;
        }

        // Does this stream contain video?
        if( Type.majortype == MEDIATYPE_Video )
            bIsVideo = TRUE;

        MyFreeMediaType( Type );

        if( !bIsVideo ) 
            continue;

        // Found a video stream
        bFoundVideo = TRUE;
        break;
    }

    if( !bFoundVideo )
    {
        printf( "Couldn't find a video stream\r\n" );
        return FALSE;// 0;
    }

    // this method will change the MediaDet to go into 
    // "sample grabbing mode" at time 0.
    //
    hr = pDet->EnterBitmapGrabMode( 0.0 );
    if( FAILED( hr ) ) 
    {
        printf( "Failed in EnterBitmapGrabMode!  hr=0x%x\r\n", hr );
        return FALSE;// hr;
    }

    // ask for the sample grabber filter that we know lives inside the
    // graph made by the MediaDet
    //

    CComPtr< ISampleGrabber > pGrabber;
    hr = pDet->GetSampleGrabber( &pGrabber );
    if( FAILED(hr) || !pGrabber)
    {
        printf( "couldn't find the sample grabber filter!  hr=0x%x\r\n", hr );
        return FALSE;// hr;
    }

    // set the callback (our COM object callback)
    //
    CComQIPtr< ISampleGrabberCB, &IID_ISampleGrabberCB > pCB( &pCallback );
    CComQIPtr< IBaseFilter, &IID_IBaseFilter > pFilter( pGrabber );
    hr = pGrabber->SetCallback( pCB, 1 );
    hr = pGrabber->SetOneShot( TRUE );       // don't do one-shot mode
    hr = pGrabber->SetBufferSamples( TRUE ); // don't buffer samples

	// 미디어의 크기를 세팅해준다.
    AM_MEDIA_TYPE mt;
    pGrabber->GetConnectedMediaType( &mt );
    VIDEOINFOHEADER * vih = (VIDEOINFOHEADER*) mt.pbFormat;
    lImageWidth  = vih->bmiHeader.biWidth;
    lImageHeight = vih->bmiHeader.biHeight;
    MyFreeMediaType( mt );

	char	sSystemDir[500];
	GetSystemDirectory( sSystemDir, 500 );
	sImagePath.Format( "%s\\Endorphin.tmp", sSystemDir );
	sCapturedTempFile = sImagePath;

    // find the filter graph interface from the sample grabber filter
    //
    FILTER_INFO fi;
    memset( &fi, 0, sizeof( fi ) );
    hr = pFilter->QueryFilterInfo( &fi );
    if( FAILED( hr ) ) 
    {
        printf( "Failed in QueryFilterInfo!  hr=0x%x\r\n", hr );
        return FALSE;// hr;
    }

    // Release the filter's graph reference
    if( fi.pGraph ) 
        fi.pGraph->Release( );
    IFilterGraph * pGraph = fi.pGraph;

    // The graph will have been paused by entering bitmap grab mode.
    // We'll need to seek back to 0 to get it to deliver correctly.
    //
    CComQIPtr< IMediaSeeking, &IID_IMediaSeeking > pSeeking( pGraph );
    REFERENCE_TIME Start	= pos;
    REFERENCE_TIME Duration	= Start;

    hr = pSeeking->SetPositions( &Start,    AM_SEEKING_AbsolutePositioning, 
                                 &Duration, AM_SEEKING_AbsolutePositioning );
    if( FAILED( hr ) ) 
    {
        printf( "Failed in SetPositions!  hr=0x%x\r\n", hr );
        return FALSE;// hr;
    }

    // run the graph
    //
    CComQIPtr< IMediaEvent, &IID_IMediaEvent > pEvent( pGraph );
    CComQIPtr< IMediaControl, &IID_IMediaControl > pControl( pGraph );
    hr = pControl->Run( );
    if( FAILED( hr ) ) 
    {
        printf( "Failed to run the graph!  hr=0x%x\r\n", hr );
        return FALSE;// hr;
    }

    // wait 
    //
    long EventCode = 0;
    hr = pEvent->WaitForCompletion( INFINITE, &EventCode );
    if( FAILED( hr ) ) 
    {
        printf( "Failed in WaitForCompletion!  hr=0x%x\r\n", hr );
        return FALSE;// hr;
    }

	return TRUE;
}
#endif

void CDShowWnd::OnTimer(UINT nIDEvent) 
{
	// TODO: Add your message handler code here and/or call default
	CWnd::OnTimer(nIDEvent);
}

void CDShowWnd::OnPaint() 
{
	CPaintDC dc(this); // device context for painting
	CRect	rc;

	GetClientRect( rc );
	// TODO: Add your message handler code here
	// Do not call CWnd::OnPaint() for painting messages
	if ( m_sMediaFile != "" )
		return;

	dc.FillSolidRect( rc, m_crBack );


	//dc.TextOut( 10, 30, "1317_20160319_135350_E_F.MP41317_20160319_135350_E_F.MP41317_20160319_135350_E_F.MP4" );

	if ( m_sStatusMessage != "" )
	{
		dc.SetTextColor( m_crStatusMessage );
		if ( m_sMediaFile == "" )
			dc.DrawText( m_sStatusMessage, rc, DT_CENTER | DT_VCENTER | DT_SINGLELINE );
	}
}

BOOL CDShowWnd::OnEraseBkgnd(CDC* pDC) 
{
	// TODO: Add your message handler code here and/or call default
	return false;
	return TRUE;//CWnd::OnEraseBkgnd(pDC);
}

void CDShowWnd::SetScreenValue( int nID, int nValue )
{
/*	if ( g_pLegendsVideo && ((CEndorphinDlg*)GetParent())->bApplyVideoFilter )
	{
		CComQIPtr<ILegendsVideo> pVideoFilter( g_pLegendsVideo );
		if (pVideoFilter != NULL)
		{
			if ( nID == 0 )
				pVideoFilter->put_Bright( (nValue - 50) * 3 );
			else if ( nID == 1 )
				pVideoFilter->put_Contrast( (nValue - 50) );
		}

		return;
	}
*/
	if ( !m_pMixerPinConfig2 )
		return;
	
	int		nBright;
	int		nContrast;
	int		nSaturation;

	DDCOLORCONTROL pColorControl;
	ZeroMemory(&pColorControl, sizeof(pColorControl));
	pColorControl.dwSize = sizeof( DDCOLORCONTROL );

	m_pMixerPinConfig2->GetOverlaySurfaceColorControls( &pColorControl );

	if ( nID == 0 )
	{
		int	nMultiply;
		( nValue >= 50 ) ? nMultiply = 185 : nMultiply = 15;
		nBright = nBrightDefault + (nValue - 50) * nMultiply;

		if ( nBright < 0 )
			nBright = 0;
		if ( nBright > 10000 )
			nBright = 10000;

		pColorControl.lBrightness	= nBright;
	}
	else if ( nID == 1 )
	{
		nContrast = nValue * 200;

		if ( nContrast < 0 )
			nContrast = 0;
		if ( nContrast > 20000 )
			nContrast = 20000;

		pColorControl.lContrast	= nContrast;
	}
	else if ( nID == 2 )
	{
		nSaturation = nValue * 200;

		if ( nSaturation < 0 )
			nSaturation = 0;
		if ( nSaturation > 20000 )
			nSaturation = 20000;

		pColorControl.lSaturation	= nSaturation;
	}
	else
		return;

	pColorControl.dwFlags = DDCOLOR_BRIGHTNESS |
							DDCOLOR_CONTRAST |
							DDCOLOR_SATURATION;

	m_pMixerPinConfig2->SetOverlaySurfaceColorControls( &pColorControl );
}

HRESULT CDShowWnd::ConnectFilters(IGraphBuilder *pGB, IBaseFilter *pUpFilter, IBaseFilter *pDownFilter, int nMAX)
{
	HRESULT hr;

	// Get the first output pin of the new source filter. Audio sources 
	// typically have only one output pin, so for most audio cases finding 
	// any output pin is sufficient.

	IEnumPins *pUpEnumPins = NULL, *pDownEnumPins = NULL;
	IPin *ppinOut = NULL, *pDownstreamPin;
	IPin *ppinIn = NULL, *pUpstreamPin;
	PIN_DIRECTION direction;
	bool bIsAnySuccess = false;

	pUpFilter->EnumPins(&pUpEnumPins);

	while (pUpEnumPins->Next(1, &ppinOut, 0) == S_OK)
	{
		ppinOut->QueryDirection(&direction);
		
		if (direction == PINDIR_OUTPUT)
		{
			ppinOut->ConnectedTo(&pDownstreamPin);
			
			if (pDownstreamPin == NULL)
			{
				// Downstream filter: pin enumeration
				pDownFilter->EnumPins(&pDownEnumPins);

				while (pDownEnumPins->Next(1, &ppinIn, 0) == S_OK)
				{
					JIF(ppinIn->QueryDirection(&direction));
					
					if (direction == PINDIR_INPUT)
					{
						ppinIn->ConnectedTo(&pUpstreamPin);
						
						if (pUpstreamPin == NULL)
						{
							//
							// We have two pins. Connect it
							//

							// 동적 생성 필터 둘이 결합할 때
							// 발생하는 무한 루프를 막는다!
							//if (nMAX-- > 0)
							{
								hr = pGB->Connect(ppinOut, ppinIn);
							}

							if (hr == S_OK)
								bIsAnySuccess |= true;
							else if (hr == VFW_S_PARTIAL_RENDER)
								bIsAnySuccess |= true;
							else if (hr == E_ABORT)
								bIsAnySuccess |= false;
							else if (hr == E_POINTER)
								bIsAnySuccess |= false;
							else if (hr == VFW_E_CANNOT_CONNECT)
								bIsAnySuccess |= false;
							else if (hr == VFW_E_NOT_IN_GRAPH)
								bIsAnySuccess |= false;
						}
						SAFE_RELEASE(pUpstreamPin);
					}
					SAFE_RELEASE(ppinIn);
				}
				SAFE_RELEASE(pDownEnumPins);
			}
			// end of downstream
			SAFE_RELEASE(pDownstreamPin);
		}
		SAFE_RELEASE(ppinOut);
	}
	SAFE_RELEASE(pUpEnumPins);
	// end of upstream

	return (bIsAnySuccess == false ? E_FAIL : S_OK);
}

// BaseFilter에서 연결되어있지 않은 핀을 리턴해준다.
IPin* CDShowWnd::GetPin(IBaseFilter *pFilter, PIN_DIRECTION PinDir)
{
	PIN_DIRECTION PinDirThis;
	IEnumPins *pEnum;
	IPin *pPin;
	IPin *checkPin;
	
	HRESULT hr = pFilter->EnumPins(&pEnum);
	
	if (FAILED(hr))
		return NULL;
	
	while(pEnum->Next(1, &pPin, 0) == S_OK){
		pPin->QueryDirection(&PinDirThis);
		if (PinDir == PinDirThis){
			if (pPin->ConnectedTo(&checkPin) == VFW_E_NOT_CONNECTED){
				return pPin;
			}
			else checkPin->Release();
		}
		pPin->Release();
	}
	pEnum->Release();
	
	return NULL;
}

BOOL CDShowWnd::CheckOverlayAndColorKey()
{
	LPDIRECTDRAW	lpDD		= NULL;
	BOOL	bSupportOverlay		= FALSE;
	BOOL	bSupportColorKey	= FALSE;

	DirectDrawCreate( NULL, &lpDD, NULL );
	IDirectDraw_SetCooperativeLevel( lpDD, m_hWnd, DDSCL_NORMAL );

	DDCAPS ddcaps;
	ddcaps.dwSize = sizeof(ddcaps);
	HRESULT hr = lpDD->GetCaps( &ddcaps, NULL );
	if ( hr != DD_OK )
		return FALSE;

	if ( ( ddcaps.dwCaps & DDCAPS_OVERLAY ) == DDCAPS_OVERLAY )
		bSupportOverlay = TRUE;
	else
		AfxMessageBox(_T("비디오카드의 오버레이 기능이 지원되지 않습니다."));

	if ( ( ddcaps.dwCaps & DDCAPS_COLORKEY ) == DDCAPS_COLORKEY )
		bSupportColorKey = TRUE;
	else
		AfxMessageBox(_T("비디오카드의 컬러키 기능이 지원되지 않습니다."));
/*
	DDSURFACEDESC ddsdOverlay;
	ZeroMemory( &ddsdOverlay, sizeof(ddsdOverlay) );
	ddsdOverlay.dwSize = sizeof(ddsdOverlay);
	
	ddsdOverlay.ddsCaps.dwCaps = DDSCAPS_OVERLAY;// | DDSCAPS_FLIP | DDSCAPS_COMPLEX | DDSCAPS_VIDEOMEMORY;
	ddsdOverlay.dwFlags = DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH;// | DDSD_BACKBUFFERCOUNT | DDSD_PIXELFORMAT;
	ddsdOverlay.dwWidth = 320;
	ddsdOverlay.dwHeight	= 240;
//	ddsdOverlay.dwBackBufferCount = 2;

	CComPtr<IDirectDrawSurface> pddsOverlay;

	hr = lpDD->CreateSurface( &ddsdOverlay, &pddsOverlay, NULL );
	if ( SUCCEEDED(hr) )
		return TRUE;
*/
	return FALSE;
}

double CDShowWnd::SetRate( int nFast )
{
	if ( !m_pMediaSeeking )
		return -1.0;

	double	dRate;
	m_pMediaSeeking->GetRate( &dRate );

	if ( nFast == 1 && dRate < 16.0 )
	{
		dRate *= 2.0;
		m_pMediaSeeking->SetRate( dRate );
	}
	else if ( nFast == -1 && dRate > 0.25 )
	{
		dRate /= 2.0;
		m_pMediaSeeking->SetRate( dRate );
	}
	else if ( nFast == 0 )
	{
		m_pMediaSeeking->SetRate( 1.0 );
	}

	m_pMediaSeeking->GetRate( &dRate );
	return dRate;
}

BOOL CDShowWnd::OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message) 
{
	// TODO: Add your message handler code here and/or call default
//	MessageBeep(0);
	//::SetCursor( AfxGetApp()->LoadStandardCursor( IDC_ARROW ) );
	//return TRUE;
	return CWnd::OnSetCursor(pWnd, nHitTest, message);
}

LRESULT CDShowWnd::OnGraphNotify( WPARAM wParam, LPARAM lParam )
{
	//TRACE( "%ld\n", GetTickCount() );
	return 1;
}

bool CDShowWnd::GetFullScreenMode()
{
	if ( m_pVideoWindow == NULL )
		return false;

	long bFullScreenMode;
	m_pVideoWindow->get_FullScreenMode( &bFullScreenMode );

	if ( bFullScreenMode == OATRUE )
		return true;

	return false;
}

void CDShowWnd::SetFullScreenMode( bool bFullScreen )
{
	if ( m_pVideoWindow == NULL )
		return;

	m_pVideoWindow->put_FullScreenMode( (bFullScreen ? OATRUE : OAFALSE) );
}

