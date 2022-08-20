
/******************************************************************************

Flash.OCX 를 이용한 플래시 플레이 클래스

    2007-08-10 오전 10:26:23 - 초기버젼

Visual C++ .NET
byunhy@hitel.net

변해룡
******************************************************************************/

#ifndef __SDFLASHCTL1_H__
#define __SDFLASHCTL1_H__

#if defined(SDSOCKLIB_EXPORTS)
#define SDSOCKLIB_API __declspec(dllexport)
#elif defined(SDSOCKLIB_IMPORTS)
#define SDSOCKLIB_API __declspec(dllimport)
#else 
#define SDSOCKLIB_API 
#endif

// Flash Player 관련 2007-08-09 
//#include <exdisp.h>   [';'이(가) 'IWebBrowser' 식별자 앞에 없습니다.] 이런 오류가 뜨면 이 인클루드를 막아야 한다.
#include <mshtmlc.h>

#include <atlbase.h>



//#import  "PROGID:ShockwaveFlash.ShockwaveFlash.10" no_namespace raw_interfaces_only    // .NET 버젼에서
#import "C:\\windows\\system32\\Macromed\\Flash\\Flash11e.ocx"                              // VC 6.0 이상 일때
using ShockwaveFlashObjects::IShockwaveFlash;                                     // VC 6.0 이상 일때

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
// Purpose  : 플래시 OCX를 이용한 플래시 플레이
///////////////////////////////////////////////////////////////////////////////////////
class SDSOCKLIB_API SDFlashCtl1
{
    protected:
        HMODULE                 m_hAtlDLL;                                  // atl dll의 핸들
        HMODULE                 m_hUserDLL;                                 // USER32.DLL 핸들

        LPFNATLAXWININIT        m_lpAtlAxWinInit;
        LPFNATLAXATTACHCONTROL  m_lpAtlAxAttachControl;
        HWND                    m_FlashWnd;

        static LPFNUPDATELAYEREDWINDOW m_pfnUpdateLayeredWindow;

        static IShockwaveFlash* m_pFlash;
        static IViewObjectEx  * m_pFlashView;

    public :
        SDFlashCtl1();
        ~SDFlashCtl1();

        void    Release();                                                  // 플래시와 뷰 오브젝트 삭제
                                                                            // 플래시 윈도우 생성 및 플래시 오브젝트 초기화
        int     Create(HWND hParentWnd, char bPopup,RECT rcPos,char bTrans=FALSE,DWORD dwExStyle=WS_EX_TOPMOST);

        HRESULT Load(char szFlashFile[]);                                   // 플래시 파일을 읽어온다.

        static LRESULT  CALLBACK NewFlashWndProc(HWND,UINT,WPARAM,LPARAM);  // 새로운 플래시 플레이 윈도우 프로시져
        static WNDPROC  lpOrgFlashWndProc;                                  // 플래시를 컨트롤할 원래 윈도우 프로시져

		int		GetCurrentFrame();
		CString m_Windowname;
		static	int		m_frame_count;

		HWND	GetHwnd() { return m_FlashWnd; }

};

#endif