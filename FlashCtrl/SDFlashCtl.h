
/******************************************************************************

Flash.OCX �� �̿��� �÷��� �÷��� Ŭ����

    2007-08-10 ���� 10:26:23 - �ʱ����

Visual C++ .NET
byunhy@hitel.net

���ط�
******************************************************************************/

#ifndef __SDFLASHCTL_H__
#define __SDFLASHCTL_H__

#if defined(SDSOCKLIB_EXPORTS)
#define SDSOCKLIB_API __declspec(dllexport)
#elif defined(SDSOCKLIB_IMPORTS)
#define SDSOCKLIB_API __declspec(dllimport)
#else 
#define SDSOCKLIB_API 
#endif

// Flash Player ���� 2007-08-09 
//#include <exdisp.h>   [';'��(��) 'IWebBrowser' �ĺ��� �տ� �����ϴ�.] �̷� ������ �߸� �� ��Ŭ��带 ���ƾ� �Ѵ�.
#include <mshtmlc.h>

#include <atlbase.h>

const CLSID CLSID_ShockwaveFlash = { 0xD27CDB6E, 0xAE6D, 0x11CF, { 0x96, 0xB8, 0x44, 0x45, 0x53, 0x54, 0x00, 0x00 } };

//#import  "PROGID:ShockwaveFlash.ShockwaveFlash" no_namespace raw_interfaces_only    // .NET ��������
#import "C:\\windows\\system32\\Macromed\\Flash\\Flash11e.ocx"                              // VC 6.0 �̻� �϶�
using ShockwaveFlashObjects::IShockwaveFlash;                                     // VC 6.0 �̻� �϶�

typedef HRESULT (WINAPI *LPFNATLAXWININIT) ();
typedef HRESULT (WINAPI *LPFNATLAXATTACHCONTROL)(IUnknown* pControl,HWND hWnd,IUnknown** ppUnkContainer );
typedef BOOL    (WINAPI *LPFNUPDATELAYEREDWINDOW)(HWND hwnd,HDC hdcDst,POINT *pptDst,SIZE *psize,HDC hdcSrc,POINT *pptSrc,COLORREF crKey,BLENDFUNCTION *pblend,DWORD dwFlags);

#ifndef WS_EX_LAYERED
#define WS_EX_LAYERED           0x80000 
#endif  // #ifndef WS_EX_LAYERED

#ifndef ULW_ALPHA
#define ULW_ALPHA               0x00000002
#endif  // #ifndef ULW_ALPHA

///////////////////////////////////////////////////////////////////////////////////////
// Class    : SDFlashCtl
// Created  : 2007-08-10
// Author   : SEXYMAN ( byunhy@hitel.net )
// Filename : SDFlashCtl.h
// Purpose  : �÷��� OCX�� �̿��� �÷��� �÷���
///////////////////////////////////////////////////////////////////////////////////////
class SDSOCKLIB_API SDFlashCtl
{
    protected:
        HMODULE                 m_hAtlDLL;                                  // atl dll�� �ڵ�
        HMODULE                 m_hUserDLL;                                 // USER32.DLL �ڵ�

        LPFNATLAXWININIT        m_lpAtlAxWinInit;
        LPFNATLAXATTACHCONTROL  m_lpAtlAxAttachControl;
        HWND                    m_FlashWnd;

        static LPFNUPDATELAYEREDWINDOW m_pfnUpdateLayeredWindow;


    public :
        SDFlashCtl();
        ~SDFlashCtl();
		static IShockwaveFlash* m_pFlash;
		static IViewObjectEx  * m_pFlashView;

        void    Release();                                                  // �÷��ÿ� �� ������Ʈ ����
                                                                            // �÷��� ������ ���� �� �÷��� ������Ʈ �ʱ�ȭ
        int     Create(HWND hParentWnd, char bPopup,RECT rcPos,char bTrans=FALSE,DWORD dwExStyle=WS_EX_TOPMOST);

        HRESULT Load(char szFlashFile[]);                                   // �÷��� ������ �о�´�.

        static LRESULT  CALLBACK NewFlashWndProc(HWND,UINT,WPARAM,LPARAM);  // ���ο� �÷��� �÷��� ������ ���ν���
        static WNDPROC  lpOrgFlashWndProc;                                  // �÷��ø� ��Ʈ���� ���� ������ ���ν���

		int GetCurrentFrame();
		CString m_Windowname;

		HWND	GetHwnd() { return m_FlashWnd; }

};

#endif