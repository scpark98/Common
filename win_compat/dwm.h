#pragma once

/*
* win_compat::dwm
*  - 옛 SDK / 옛 OS 에 없거나 불완전한 DWM API 를 안전하게 호출하기 위한 래퍼.
*  - dwmapi.dll 자체가 Vista+ 이므로 XP 에서는 LoadLibrary 단계에서 거름 → no-op.
*  - SDK 7.1A (_USING_V110_SDK71_) 에 누락된 상수는 헤더 의존 없이 로컬에서 직접 정의.
*  - 함수포인터는 프로세스 수명 동안 1회만 GetProcAddress 로 해석 후 캐싱.
*    (FreeLibrary 는 일부러 호출하지 않음 — 시스템 DLL 이고 OS 가 프로세스 종료 시 회수.)
*
*  사용 예:
*    win_compat::dwm::set_window_corner_round(m_hWnd);
*    CRect r;
*    win_compat::dwm::get_extended_frame_bounds_or_window_rect(GetSafeHwnd(), r);
*/

#include <Windows.h>

namespace win_compat
{
namespace dwm
{
    typedef HRESULT (WINAPI *pfn_DwmSetWindowAttribute)(HWND, DWORD, LPCVOID, DWORD);
    typedef HRESULT (WINAPI *pfn_DwmGetWindowAttribute)(HWND, DWORD, PVOID, DWORD);

    inline HMODULE _get_dwmapi()
    {
        static HMODULE h = ::LoadLibraryW(L"dwmapi.dll");
        return h;
    }

    inline pfn_DwmSetWindowAttribute _get_set_attr()
    {
        static pfn_DwmSetWindowAttribute pfn =
            _get_dwmapi() ? (pfn_DwmSetWindowAttribute)::GetProcAddress(_get_dwmapi(), "DwmSetWindowAttribute") : nullptr;
        return pfn;
    }

    inline pfn_DwmGetWindowAttribute _get_get_attr()
    {
        static pfn_DwmGetWindowAttribute pfn =
            _get_dwmapi() ? (pfn_DwmGetWindowAttribute)::GetProcAddress(_get_dwmapi(), "DwmGetWindowAttribute") : nullptr;
        return pfn;
    }

    //Win11 Build 22000+ : 윈도우 모서리 라운딩.
    //  - XP/Vista/7/8/10 : no-op (dwmapi 미존재 또는 attribute 미지원, E_INVALIDARG 반환되며 무시)
    //  - Win11+          : 둥근 모서리 적용
    //SDK 7.1A 에 DWMWA_WINDOW_CORNER_PREFERENCE / DWMWCP_ROUND 가 없으므로 값(33, 2)을 직접 정의.
    inline void set_window_corner_round(HWND hwnd)
    {
        const DWORD attr_corner_pref = 33;  //DWMWA_WINDOW_CORNER_PREFERENCE
        const DWORD pref_round       = 2;   //DWMWCP_ROUND

        auto pfn = _get_set_attr();
        if (!pfn) return;
        DWORD v = pref_round;
        pfn(hwnd, attr_corner_pref, &v, sizeof(v));
    }

    //DWMWA_EXTENDED_FRAME_BOUNDS (Vista+) : 보이는 frame 경계를 얻는다.
    //  - XP / dwmapi 미존재 / 함수 미존재 시 fallback 으로 GetWindowRect 사용.
    //  - 호출자가 분기 안 하도록 한 함수에 통합.
    inline void get_extended_frame_bounds_or_window_rect(HWND hwnd, RECT& out_rect)
    {
        const DWORD attr_extended_frame_bounds = 9;  //DWMWA_EXTENDED_FRAME_BOUNDS

        auto pfn = _get_get_attr();
        if (pfn && SUCCEEDED(pfn(hwnd, attr_extended_frame_bounds, &out_rect, sizeof(out_rect))))
            return;

        ::GetWindowRect(hwnd, &out_rect);
    }
}
}
