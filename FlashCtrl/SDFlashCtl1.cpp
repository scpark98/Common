
/******************************************************************************

Flash.OCX 를 이용한 플래시 플레이 클래스

2007-08-10 오전 10:26:23 - 초기버젼

Visual C++ .NET
byunhy@hitel.net

변해룡
******************************************************************************/

#include "stdafx.h"	

#include "SDFlashCtl1.h"

const CLSID CLSID_ShockwaveFlash = { 0xD27CDB6E, 0xAE6D, 0x11CF, { 0x96, 0xB8, 0x44, 0x45, 0x53, 0x54, 0x00, 0x00 } };

WNDPROC SDFlashCtl1::lpOrgFlashWndProc       = NULL;                 // 플래시를 컨트롤할 원래 윈도우 프로시져
IShockwaveFlash* SDFlashCtl1::m_pFlash       = NULL;
IViewObjectEx  * SDFlashCtl1::m_pFlashView   = NULL;
int				 SDFlashCtl1::m_frame_count	= 0;


#define FLASHCONRELEASE WM_USER + 2010


LPFNUPDATELAYEREDWINDOW SDFlashCtl1::m_pfnUpdateLayeredWindow = NULL;

SDFlashCtl1::SDFlashCtl1()
{
	m_frame_count = 0;
    m_FlashWnd = NULL;

    // 한번만 초기화
    m_hAtlDLL = LoadLibrary("atl");
    if (m_hAtlDLL)
    {
        m_lpAtlAxWinInit = (LPFNATLAXWININIT)GetProcAddress(m_hAtlDLL,"AtlAxWinInit"); 

        HRESULT  hr = m_lpAtlAxWinInit();       // ***** atl activeX 초기화 *****

        m_lpAtlAxAttachControl = (LPFNATLAXATTACHCONTROL)GetProcAddress(m_hAtlDLL,"AtlAxAttachControl"); 
    }

    m_hUserDLL = LoadLibrary("USER32.DLL");
    if(m_hUserDLL)
        m_pfnUpdateLayeredWindow = (LPFNUPDATELAYEREDWINDOW)GetProcAddress(m_hUserDLL, "UpdateLayeredWindow");

	m_frame_count = 0;
	m_Windowname = "AtlAxWin";
  
}

SDFlashCtl1::~SDFlashCtl1()
{
    Release();


	FreeLibrary(m_hAtlDLL);
	FreeLibrary(m_hUserDLL);
}

// 플래시와 뷰 오브젝트 삭제
void SDFlashCtl1::Release()
{

    if (m_pFlash)
    {
        m_pFlash->Release();   
        m_pFlash = NULL;
    }
	else
	{
		return;
	}

    if (m_pFlashView)
    {
        m_pFlashView->Release();
        m_pFlashView = NULL;
    }

	DestroyWindow(m_FlashWnd);

	m_frame_count = 0;

}

// 플래시 컨트롤과 플래시 윈도우를 생성
// hParentWnd   : 부모윈도우 핸들
// bPopup       : 팝업 윈도우를 띄울것인가 ?
// rcPos        : 플래시 윈도우의 좌표
// dwExStyle    : 추가 윈도우 스타일
// bTrans       : 투과 모드로 플래시를 띄울것인가? (현재 지원하지 않음)
int SDFlashCtl1::Create(HWND hParentWnd , char bPopup,RECT rcPos,char bTrans,DWORD dwExStyle)
{

	if(m_pFlash != NULL)
	{
		Release();
	}
    if (m_lpAtlAxAttachControl)
    {
        HRESULT hr;

        DWORD dwStyle = 0;
        
        if (bPopup)
            dwStyle = WS_POPUP|WS_VISIBLE;
        else
            dwStyle = WS_CHILD|WS_VISIBLE;

        m_FlashWnd = ::CreateWindowEx(dwExStyle, "AtlAxWin", NULL,
            dwStyle, rcPos.left, rcPos.top, rcPos.right - rcPos.left, rcPos.bottom - rcPos.top,
            hParentWnd, NULL, NULL, NULL);

        if(!::IsWindow(m_FlashWnd))
            return E_FAIL;

        hr = ::CoCreateInstance(CLSID_ShockwaveFlash, NULL,
            CLSCTX_ALL, __uuidof(IShockwaveFlash), (void **)&m_pFlash);

        if(FAILED(hr))
            return hr;

        hr = m_pFlash->QueryInterface(__uuidof(IViewObjectEx),(void **)&m_pFlashView);
        if(FAILED(hr) || NULL == m_pFlashView)
            return E_NOINTERFACE;

        if (bTrans)
        {
            hr = m_pFlash->put_WMode(L"transparent"); 
            if(FAILED(hr))
                return hr;
        }

		m_pFlash->put_ScaleMode( 2 );

        hr = m_lpAtlAxAttachControl(m_pFlash,m_FlashWnd,0);
        if(FAILED(hr))
            return hr;

        lpOrgFlashWndProc = (WNDPROC)GetWindowLong(m_FlashWnd,GWL_WNDPROC);
        SetWindowLong(m_FlashWnd,GWL_WNDPROC,(LONG)NewFlashWndProc);

        InvalidateRect(m_FlashWnd,NULL,TRUE);

		m_frame_count = 0;
    }
    else
        return FALSE;

    return TRUE;
}

// 플래시 파일을 읽어온다.
HRESULT SDFlashCtl1::Load(char szFlashFile[])
{
    if (!m_pFlash) 
        return E_FAIL;

    HRESULT hr = E_FAIL;

    hr = m_pFlash->LoadMovie(0,_bstr_t(szFlashFile));
    if(FAILED(hr))
        return hr;

    hr = m_pFlash->Rewind();
    if(FAILED(hr))
        return hr;

    hr = m_pFlash->Play();
    if(FAILED(hr))
        return hr;

    return hr;
}

int SDFlashCtl1::GetCurrentFrame()
{	

	return m_frame_count;
}

// 플래시 윈도우 프로시져
LRESULT CALLBACK SDFlashCtl1::NewFlashWndProc(HWND hwnd,UINT message,WPARAM wParam,LPARAM lParam)
{
    if (!lpOrgFlashWndProc)
        return FALSE;

    switch (message)
    {
        case WM_PAINT:
        {
            HDC 			hdc;
            PAINTSTRUCT 	ps;

            hdc=BeginPaint (hwnd, &ps);

            if (m_pFlashView)
            {
                RECT rcWindow = { 0 };
                ::GetWindowRect(hwnd, &rcWindow);

                HDC hdcMemory = ::CreateCompatibleDC(hdc);

                // to create RGBBitmap
                BITMAPINFO bmi = { 0 };
                bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
                bmi.bmiHeader.biPlanes = 1;
                bmi.bmiHeader.biBitCount = 32;
                bmi.bmiHeader.biCompression = BI_RGB;
                bmi.bmiHeader.biWidth = rcWindow.right - rcWindow.left;
                bmi.bmiHeader.biHeight = rcWindow.bottom - rcWindow.top;

                LPBYTE pbmpBits = NULL;
                HBITMAP hbmp = ::CreateDIBSection(hdc, (LPBITMAPINFO)&bmi, DIB_RGB_COLORS, (void **)&pbmpBits, NULL, 0);
                if(NULL == hbmp)
                    return FALSE;

                HBITMAP hbmpOld = (HBITMAP)::SelectObject(hdcMemory, hbmp);

                HRESULT hr = m_pFlashView->Draw( DVASPECT_CONTENT,0,NULL,NULL,NULL,hdcMemory,NULL,NULL,NULL,0);
                if(FAILED(hr))
                    return FALSE;

                BITMAP bm = { 0 };
                if(::GetObject(hbmp, sizeof(BITMAP), &bm) != sizeof(BITMAP))
                    return FALSE;

                SIZE szBmp = { bm.bmWidth, bm.bmHeight };

                POINT ptDest = { rcWindow.left, rcWindow.top }, ptSrc = { 0 };

                char m_bySourceConstantAlpha = 100;
                BLENDFUNCTION bf = { AC_SRC_OVER, 0, (BYTE)(255 * m_bySourceConstantAlpha / 100), AC_SRC_ALPHA };

                if(!m_pfnUpdateLayeredWindow(hwnd, hdc, &ptDest, &szBmp, hdcMemory, &ptSrc, 0, &bf, 0x00000002))
                    return FALSE;
                
                ::SelectObject(hdcMemory,hbmpOld);
				::DeleteObject(hbmp);
                ::DeleteDC(hdcMemory);

				m_frame_count ++;
            }

            EndPaint (hwnd, &ps);

        }
        break;
	
    }

    return CallWindowProc(lpOrgFlashWndProc, hwnd, message, wParam, lParam);
}