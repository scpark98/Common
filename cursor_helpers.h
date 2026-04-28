#pragma once
#include <windows.h>
#include <algorithm>

/*
얇은 십자 커서를 런타임에 만들어 반환한다.
Windows 표준 IDC_CROSS 가 두꺼워 정밀한 위치 지정이 어려울 때 사용.

- size       : 커서 정사각형 한 변 (px). 홀수 권장. 기본 31.
- thickness  : 십자 선 두께 (px). 1 = 1 픽셀 라인.
- gap        : 중심 빈 영역 반경 (px). 0 = 빈틈 없는 +, >0 = 중심에 hole.
- cr_line    : 십자 선 색 (RGB 매크로 사용).
- cr_outline : 시인성용 1px 외곽 색. -1 패스 시 외곽 없음.

반환값: HCURSOR. 호출자가 DestroyCursor() 책임. 프로세스 수명 동안 캐싱하려면
get_thin_cross_cursor() 를 사용 — 첫 호출 시 1회 생성 후 같은 핸들 반환.
*/

inline HCURSOR make_cross_cursor(
    int      size       = 31,
    int      thickness  = 1,
    int      gap        = 5,
    COLORREF cr_line    = RGB(0, 0, 0),
    COLORREF cr_outline = RGB(255, 255, 255))
{
    if (size < 7)
        size = 7;
    if ((size & 1) == 0)
        ++size;
    const int center = size / 2;

    BITMAPINFO bi = {};
    bi.bmiHeader.biSize        = sizeof(BITMAPINFOHEADER);
    bi.bmiHeader.biWidth       = size;
    bi.bmiHeader.biHeight      = -size;
    bi.bmiHeader.biPlanes      = 1;
    bi.bmiHeader.biBitCount    = 32;
    bi.bmiHeader.biCompression = BI_RGB;

    void* bits = nullptr;
    HDC hdc_screen = ::GetDC(nullptr);
    HBITMAP color_bmp = ::CreateDIBSection(hdc_screen, &bi, DIB_RGB_COLORS, &bits, nullptr, 0);
    ::ReleaseDC(nullptr, hdc_screen);

    if (!color_bmp || !bits)
    {
        if (color_bmp) ::DeleteObject(color_bmp);
        return nullptr;
    }

    DWORD* px = (DWORD*)bits;

    auto pack = [](COLORREF cr) -> DWORD
    {
        return (0xFFu << 24)
             | (DWORD(GetRValue(cr)) << 16)
             | (DWORD(GetGValue(cr)) << 8)
             |  DWORD(GetBValue(cr));
    };
    auto set_px = [&](int x, int y, DWORD v)
    {
        if (x < 0 || x >= size || y < 0 || y >= size)
            return;
        px[y * size + x] = v;
    };

    const DWORD line_v    = pack(cr_line);
    const bool  has_out   = (cr_outline != (COLORREF)-1);
    const DWORD outline_v = has_out ? pack(cr_outline) : 0;

    const int t_lo = thickness / 2;
    const int t_hi = thickness - 1 - t_lo;

    auto on_line = [&](int v) { return v >= center - t_lo && v <= center + t_hi; };
    auto in_gap  = [&](int v) { return gap > 0 && std::abs(v - center) < gap; };

    if (has_out)
    {
        for (int y = 0; y < size; ++y)
            for (int x = 0; x < size; ++x)
            {
                const bool h = on_line(y) && !in_gap(x);
                const bool v = on_line(x) && !in_gap(y);
                if (!h && !v)
                    continue;
                for (int dy = -1; dy <= 1; ++dy)
                    for (int dx = -1; dx <= 1; ++dx)
                        set_px(x + dx, y + dy, outline_v);
            }
    }

    for (int y = 0; y < size; ++y)
        for (int x = 0; x < size; ++x)
        {
            const bool h = on_line(y) && !in_gap(x);
            const bool v = on_line(x) && !in_gap(y);
            if (h || v)
                set_px(x, y, line_v);
        }

    HBITMAP mask_bmp = ::CreateBitmap(size, size, 1, 1, nullptr);
    HDC hdc_mask = ::CreateCompatibleDC(nullptr);
    HBITMAP old_mask = (HBITMAP)::SelectObject(hdc_mask, mask_bmp);
    RECT rc = { 0, 0, size, size };
    ::FillRect(hdc_mask, &rc, (HBRUSH)::GetStockObject(WHITE_BRUSH));
    ::SelectObject(hdc_mask, old_mask);
    ::DeleteDC(hdc_mask);

    ICONINFO ii = {};
    ii.fIcon    = FALSE;
    ii.xHotspot = center;
    ii.yHotspot = center;
    ii.hbmMask  = mask_bmp;
    ii.hbmColor = color_bmp;

    HCURSOR hc = (HCURSOR)::CreateIconIndirect(&ii);

    ::DeleteObject(color_bmp);
    ::DeleteObject(mask_bmp);

    return hc;
}

inline HCURSOR get_thin_cross_cursor()
{
    static const HCURSOR s_hc = make_cross_cursor();
    return s_hc;
}
