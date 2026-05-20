// BaseClasses 의 dllentry.cpp / dllsetup.cpp 는 DLL filter 등록용이라 exe 빌드에서 제외.
// 그 두 파일이 정의하던 globals (g_hInst) 를 exe 빌드용으로 여기서 직접 제공.
//
// g_hInst = exe 모듈의 HINSTANCE. cprop.cpp / videoctl.cpp / winutil.cpp 가 dialog 띄울 때 사용.
//          exe 의 main module 핸들로 충분 (DllMain 이 set 해주는 값과 동등).

#include <windows.h>

HINSTANCE g_hInst = NULL;

namespace
{
    struct GHInstInit
    {
        GHInstInit() { g_hInst = ::GetModuleHandleW(NULL); }
    };
    static GHInstInit s_init;
}
