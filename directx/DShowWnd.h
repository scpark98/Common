#if !defined(AFX_VIDEOWND_H__A822AD9E_36EB_4ABD_A260_68003DD1E52E__INCLUDED_)
#define AFX_VIDEOWND_H__A822AD9E_36EB_4ABD_A260_68003DD1E52E__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// VideoWnd.h : header file
//

#include <DShow.h>
#include <DSound.h>
#include <Mpconfig.h>
#include <wmsdkidl.h>
#include <comdef.h>
//#include "keyprovider.h"
//#include "Constants.h"

#define VOLUME_MAX	100

#define MESSAGE_COLOR_NORMAL	RGB( 192, 192, 192 )
#define MESSAGE_COLOR_INFO		RGB( 255, 255, 128 )
#define MESSAGE_COLOR_WARNING	RGB( 255, 128,   0 )
#define MESSAGE_COLOR_ERROR		RGB( 212,  64,   0 )

#define DEFINE_GUID1(name, l, w1, w2, b1, b2, b3, b4, b5, b6, b7, b8) \
    const GUID name \
	= { l, w1, w2, { b1, b2,  b3,  b4,  b5,  b6,  b7,  b8 } }
/*
// {7563207A-E773-4442-899D-9B4A308FADD4}
DEFINE_GUID(CLSID_LegendsVideo, 
0x7563207A, 0xE773, 0x4442, 0x89, 0x9D, 0x9B, 0x4A, 0x30, 0x8F, 0xAD, 0xD4);

//  {6B6D0800-9ADA-11d0-A520-00A0D10129C0}
DEFINE_GUID(CLSID_NetShowSource, 
0x6b6d0800, 0x9ada, 0x11d0, 0xa5, 0x20, 0x0, 0xa0, 0xd1, 0x1, 0x29, 0xc0);

//DEFINE_GUID(IID_IAMNetworkStatus,0xFA2AA8F3L,0x8B62,0x11D0,0xA5,0x20,0x00,0x00,0x00,0x00,0x00,0x00);
*/
//{B98D13E7-55DB-4385-A33D-09FD1BA26338}
DEFINE_GUID(CLSID_LAV_SPLITTER_SOURCE,
0xB98D13E7, 0x55DB, 0x4385, 0xA3, 0x3D, 0x09, 0xFD, 0x1B, 0xA2, 0x63, 0x38);

//{04FE9017-F873-410E-871E-AB91661A4EF7}
DEFINE_GUID1(CLSID_ffdshow_Video_Decoder,
0x04FE9017, 0xF873, 0x410E, 0x87, 0x1E, 0xAB, 0x91, 0x66, 0x1A, 0x4E, 0xF7);

/////////////////////////////////////////////////////////////////////////////
// CDShowWnd window

class CDShowWnd : public CWnd
{
protected:
	BOOL					RegisterWindowClass();

// Construction
public:
	CDShowWnd();
	void					SetParentHWnd( HWND hWnd ) { m_hParentWnd = hWnd; }
// Attributes
public:
	// 인터페이스 관련
	IGraphBuilder			*m_pGraphBuilder;
	//IBaseFilter*			pSourceFilter;
	IMediaControl			*m_pMediaControl;
	IMediaEventEx			*m_pEvent;
	IMediaSeeking			*m_pMediaSeeking;
	IVideoWindow			*m_pVideoWindow;
	IBaseFilter				*m_pOverlayMixer;
	IOverlay				*m_pOverlay;
	IBasicVideo				*m_pBasicVideo;
	IBasicAudio				*m_pBasicAudio;
	IMixerPinConfig2		*m_pMixerPinConfig2;
	IVideoFrameStep			*m_pVideoFrameStep;
	//IAMOpenProgress			*m_pOpenProgress;
	//IBaseFilter				*g_pLegendsVideo;

	// DRM support interfaces
	//CKeyProvider			g_KeyProvider;
	HRESULT					CreateObjectFromPath(TCHAR* pPath, REFCLSID clsid, IUnknown** ppUnk);

	HRESULT					CreateFilter(REFCLSID clsid, IBaseFilter **ppFilter);
	HRESULT					RenderOutputPins(IGraphBuilder *pGB, IBaseFilter *pFilter);
	HRESULT					ConnectFilters(IGraphBuilder *pGB, IBaseFilter *pUpFilter, IBaseFilter *pDownFilter, int nMAX=3);
	IPin*					GetPin(IBaseFilter *pFilter, PIN_DIRECTION PinDir);

	// AudioRenderer Filter
	int						m_nVolume;
	int						nAudioFilter;
	CString					sAudioFilter[50];
	void					FindAudioRenderer();
	HRESULT					EnumFilters(IEnumMoniker *pEnumCat);
	HRESULT					FindFilter(CString compFiterName, REFCLSID clsID, IBaseFilter **ppSrcFilter);

	BOOL					CheckOverlayAndColorKey();

	// 윈도우 관련
	COLORREF				m_crBack;
	CString					m_sStatusMessage;
	COLORREF				m_crStatusMessage;
	void					DisplayStatusMessage( CString sMsg, COLORREF crText = MESSAGE_COLOR_NORMAL );
	bool					GetFullScreenMode();
	void					SetFullScreenMode( bool bFullScreen );

	// 미디어 관련
	CString					m_sMediaFile;
	CSize					m_szVideo;
	int						m_nVideoStream;
	int						m_nAudioStream;
	double					m_dDuration;

	int						nBrightDefault;
	int						nContrastDefault;
	int						nSaturationDefault;
	enum	PanScan			{ NONE, LEFT, RIGHT, UP, DOWN, ZOOMIN, ZOOMOUT,
							  XINC, XDEC, YINC, YDEC,
							  LEFTUP, RIGHTUP, LEFTDOWN, RIGHTDOWN };
	enum	RatioMode		{ AM_ARMODE_STRETCHED,
							  AM_ARMODE_LETTER_BOX,
							  AM_ARMODE_CROP,
							  AM_ARMODE_STRETCHED_AS_PRIMARY };

	CString					GetMediaFilename() { return m_sMediaFile; }
	void					MyFreeMediaType( AM_MEDIA_TYPE& mt );
	//ms 단위로 전체 미디어의 길이를 얻는다.
	double					GetMediaDuration();
	CSize					GetVideoSize();
	void					ShowHResultErrorMessage( HRESULT hr );
	void					ChangeAudioStream();
	void					ChangeAudioStream( int nStream );
	void					SetScreenValue( int nID, int nValue );
	BOOL					IsWindowsMediaFile( CString sFile );

	// 필터 정보 관련
	CMenu					*pSubMenu;
	int						nFilter;				// 사용된 필터의 수
	BOOL					bFilter[10];			// 등록정보가 가능한 필터인지...
	CString					sFilter[10];			// 필터 이름들
	void					EnumFilters();
	void					ShowFilterPropertyPage( CString sFilterName );
	IBaseFilter*			FindFilterByNameInGraph( CString sFilter );

	// 재생 관련
	//enum	PlayState		{ State_Stopped, State_Paused, State_Running };
	BOOL					bKeyColorIsOk;
	DWORD					m_dwColorKey;
	DWORD					m_dwGraphRegister;
	void					SetColorKey( IBaseFilter* pOvMix = NULL );
	int						LoadMediaFile( CString sFile, bool bAutoRender = true, int nVideoCh = 0  );	// 성공, 그외의 수는 에러...
	void					RemoveGraphFromRot(DWORD pdwRegister);
	HRESULT					AddGraphToRot(IUnknown *pUnkGraph, CString sTitle, DWORD *pdwRegister);
	void					ReleaseAll();
	bool					CheckMediaIsOpened() { return ( m_sMediaFile != "" ); }
	BOOL					GetFrameStepInterface();
	void					StepFrame( bool bForward );
	CRect					GetWindowPosition();
	void					SetWindowPosition();
	void					SetWindowPosition( CRect r );
	void					SetWindowPosition( int left, int top, int width, int height );
	int						GetPlayState();
	void					Play();
	void					Pause();
	void					Stop();
	void					Close();
	void					SetCurrentPosition( double lNewPos );
	double					GetCurrentPosition( BOOL bMilliSec = true );

	//volume range = 0 ~ 100
	int						GetVolume() { return m_nVolume; }
	void					SetVolume( int nVolume );

	double					SetRate( int nFast );			// -1 : slower, 0 : normal, 1 : faster
	void					ShowWindow( bool bShow );
	void					SetRatioMode( int nMode );

	CString					sCapturedImageFile;
	CString					sCapturedTempFile;
	BOOL					CaptureImageByFilter( CString sImageFile );
	BOOL					CaptureImageByGrabber( CString sImageFile );

	//OSD						OSDSet;
	void					SetOSD();

	void					FlipVertical();

	LRESULT					OnGraphNotify(WPARAM wParam, LPARAM lParam);

protected:
	HWND					m_hParentWnd;
	double					m_dMediaDuration;

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CDShowWnd)
	public:
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CDShowWnd();

	// Generated message map functions
protected:

	//{{AFX_MSG(CDShowWnd)
	afx_msg void OnDestroy();
	afx_msg void OnTimer(UINT nIDEvent);
	afx_msg void OnPaint();
	afx_msg BOOL OnEraseBkgnd(CDC* pDC);
	afx_msg BOOL OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_VIDEOWND_H__A822AD9E_36EB_4ABD_A260_68003DD1E52E__INCLUDED_)
