
/******************************************************************************

Flash.OCX �� �̿��� �÷��� �÷��� Ŭ����

2007-08-10 ���� 10:26:23 - �ʱ����

Visual C++ .NET
byunhy@hitel.net

���ط�
******************************************************************************/

#include "stdafx.h"	

#include "SDFlashCtl.h"

WNDPROC SDFlashCtl::lpOrgFlashWndProc       = NULL;                 // �÷��ø� ��Ʈ���� ���� ������ ���ν���
IShockwaveFlash* SDFlashCtl::m_pFlash       = NULL;
IViewObjectEx  * SDFlashCtl::m_pFlashView   = NULL;

int frame_count = 0;


#define FLASHCONRELEASE						WM_USER + 0x7002


LPFNUPDATELAYEREDWINDOW SDFlashCtl::m_pfnUpdateLayeredWindow = NULL;

SDFlashCtl::SDFlashCtl()
{
    m_FlashWnd = NULL;

    // �ѹ��� �ʱ�ȭ
    m_hAtlDLL = LoadLibrary("atl");
    if (m_hAtlDLL)
    {
        m_lpAtlAxWinInit = (LPFNATLAXWININIT)GetProcAddress(m_hAtlDLL,"AtlAxWinInit"); 

        HRESULT  hr = m_lpAtlAxWinInit();       // ***** atl activeX �ʱ�ȭ *****

        m_lpAtlAxAttachControl = (LPFNATLAXATTACHCONTROL)GetProcAddress(m_hAtlDLL,"AtlAxAttachControl"); 
    }

    m_hUserDLL = LoadLibrary("USER32.DLL");
    if(m_hUserDLL)
        m_pfnUpdateLayeredWindow = (LPFNUPDATELAYEREDWINDOW)GetProcAddress(m_hUserDLL, "UpdateLayeredWindow");

	frame_count = 0;
	m_Windowname = "AtlAxWin";
  
}

SDFlashCtl::~SDFlashCtl()
{
    Release();


	FreeLibrary(m_hAtlDLL);
	FreeLibrary(m_hUserDLL);
}

// �÷��ÿ� �� ������Ʈ ����
void SDFlashCtl::Release()
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

	frame_count = 0;

}

// �÷��� ��Ʈ�Ѱ� �÷��� �����츦 ����
// hParentWnd   : �θ������� �ڵ�
// bPopup       : �˾� �����츦 �����ΰ� ?
// rcPos        : �÷��� �������� ��ǥ
// dwExStyle    : �߰� ������ ��Ÿ��
// bTrans       : ���� ���� �÷��ø� �����ΰ�? (���� �������� ����)
int SDFlashCtl::Create(HWND hParentWnd , char bPopup,RECT rcPos,char bTrans,DWORD dwExStyle)
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

		frame_count = 0;
    }
    else
        return FALSE;

    return TRUE;
}

// �÷��� ������ �о�´�.
HRESULT SDFlashCtl::Load(char szFlashFile[])
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

int SDFlashCtl::GetCurrentFrame()
{	

	return frame_count;
}

// �÷��� ������ ���ν���
LRESULT CALLBACK SDFlashCtl::NewFlashWndProc(HWND hwnd,UINT message,WPARAM wParam,LPARAM lParam)
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

				frame_count ++;
            }

            EndPaint (hwnd, &ps);

        }
        break;
	
    }

    return CallWindowProc(lpOrgFlashWndProc, hwnd, message, wParam, lParam);
}