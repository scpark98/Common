#pragma once

/*
* win_compat::dpi
*  - 모니터 DPI 조회 래퍼. XP ~ Win7 에는 GetDpiForMonitor 가 없으므로 안전하게 96 으로 떨어진다.
*  - shcore.dll 을 LoadLibrary + GetProcAddress 로 늦게 묶는다. .lib 링크를 하지 않으므로
*    XP 에서도 로더가 실패하지 않는다. (win_compat::dwm 과 같은 방식.)
*  - 함수포인터는 프로세스 수명 동안 1회만 해석 후 캐싱.
*
*  [무엇에 쓰나]
*  Per-Monitor DPI 인식 앱은 논리 픽셀이 아니라 물리 픽셀을 직접 다룬다. 즉 코드에 적은
*  "버튼 높이 50" 은 175% 모니터에서 물리적으로 57% 크기로 보인다. 화면에 그리는 UI 크기는
*  전부 scale() 을 거쳐야 한다.
*
*  [중요 — 무엇을 스케일하고 무엇을 하지 않나]
*  스케일 대상 = UI 크기(여백, 버튼, 핸들 반경, 폰트, 테두리 두께).
*  스케일 금지 = 화면 픽셀 자체를 다루는 값(캡처 영역, 돋보기가 확대하는 픽셀 수,
*                줄자가 재는 픽셀 길이). 이건 물리 픽셀이 곧 의미다.
*
*  사용 예:
*    const UINT dpi = win_compat::dpi::for_window(m_hWnd);
*    const int margin = win_compat::dpi::scale(14, dpi);
*/

#include <Windows.h>

namespace win_compat
{
namespace dpi
{
	//GetDpiForMonitor 의 MONITOR_DPI_TYPE. shellscalingapi.h 없이 쓰기 위해 값을 직접 둔다.
	enum monitor_dpi_type
	{
		dpi_effective = 0,	//배율 설정이 반영된 DPI (96 * 배율). UI 크기 계산에 쓴다.
		dpi_angular = 1,
		dpi_raw = 2,		//패널의 실제 물리 DPI. 길이(cm/inch) 환산에 쓴다.
	};

	typedef HRESULT (WINAPI *pfn_GetDpiForMonitor)(HMONITOR, int, UINT*, UINT*);

	inline pfn_GetDpiForMonitor _get_dpi_for_monitor()
	{
		static pfn_GetDpiForMonitor pfn = []() -> pfn_GetDpiForMonitor
		{
			HMODULE h = ::LoadLibraryW(L"shcore.dll");
			return h ? (pfn_GetDpiForMonitor)::GetProcAddress(h, "GetDpiForMonitor") : nullptr;
		}();
		return pfn;
	}

	//지정 모니터의 DPI. 실패하면 96.
	inline UINT for_monitor(HMONITOR monitor, monitor_dpi_type type = dpi_effective)
	{
		auto pfn = _get_dpi_for_monitor();
		if (!pfn || !monitor)
			return 96;

		UINT dpi_x = 96;
		UINT dpi_y = 96;
		if (FAILED(pfn(monitor, (int)type, &dpi_x, &dpi_y)) || dpi_x == 0)
			return 96;

		return dpi_x;
	}

	//창이 올라가 있는 모니터의 DPI.
	//GetDpiForWindow 를 쓰지 않는 이유: 그쪽은 *그 창의* DPI 인식 수준을 따라가므로,
	//DPI-unaware 창을 넘겨받으면 175% 모니터 위에서도 96 을 돌려준다. 남의 창(캡처 대상)의
	//물리 크기를 재야 하는 경우가 있어 항상 모니터 기준으로 통일한다.
	inline UINT for_window(HWND hwnd, monitor_dpi_type type = dpi_effective)
	{
		return for_monitor(::MonitorFromWindow(hwnd, MONITOR_DEFAULTTONEAREST), type);
	}

	//해당 화면 좌표가 속한 모니터의 DPI. 전체화면 오버레이처럼 창 하나가 여러 모니터에
	//걸쳐 있어 for_window 가 모호한 경우, 사용자가 작업을 시작한 지점을 기준으로 삼는다.
	inline UINT for_point(POINT pt_screen, monitor_dpi_type type = dpi_effective)
	{
		return for_monitor(::MonitorFromPoint(pt_screen, MONITOR_DEFAULTTONEAREST), type);
	}

	//96 DPI 기준으로 적은 픽셀을 그 DPI 로 환산한다.
	inline int scale(int px_at_96dpi, UINT dpi)
	{
		return ::MulDiv(px_at_96dpi, (int)dpi, 96);
	}

	typedef BOOL (WINAPI *pfn_AdjustWindowRectExForDpi)(LPRECT, DWORD, BOOL, DWORD, UINT);

	inline pfn_AdjustWindowRectExForDpi _get_adjust_for_dpi()
	{
		static pfn_AdjustWindowRectExForDpi pfn = []() -> pfn_AdjustWindowRectExForDpi
		{
			HMODULE h = ::GetModuleHandleW(L"user32.dll");
			return h ? (pfn_AdjustWindowRectExForDpi)::GetProcAddress(h, "AdjustWindowRectExForDpi") : nullptr;
		}();
		return pfn;
	}

	//클라이언트 크기 → 창 크기. AdjustWindowRectEx 를 쓰면 안 되는 이유:
	//그쪽은 *시스템* DPI 로 프레임 두께를 계산한다. Per-Monitor V2 에서는 캡션·테두리가 모니터마다
	//다른 크기로 그려지므로, 주 모니터가 175% 인데 창이 100% 모니터에 있으면 프레임을 175% 로 잡아
	//클라이언트가 의도한 크기와 어긋난다. Win10 1607+ 는 DPI 를 받는 버전이 있고, 없으면 옛 API 로 떨어진다.
	inline BOOL adjust_window_rect(LPRECT rc, DWORD style, BOOL has_menu, DWORD ex_style, UINT dpi)
	{
		auto pfn = _get_adjust_for_dpi();
		if (pfn)
			return pfn(rc, style, has_menu, ex_style, dpi);

		return ::AdjustWindowRectEx(rc, style, has_menu, ex_style);
	}

	inline float scale_f(float px_at_96dpi, UINT dpi)
	{
		return px_at_96dpi * (float)dpi / 96.0f;
	}
}
}
