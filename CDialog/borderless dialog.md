# Borderless Customized Dialog 적용 매뉴얼

WS_CAPTION 없는 *완전 customized* MFC 다이얼로그 (caption / 시스템 system buttons / NC frame 을 사용자가 직접 그리되, OS resize / Aero Snap / Win11 round corner / shadow 는 유지) 만들기.

본 문서는 `Endorphin2Dlg` / `CBookmarkEditDlg` 에 적용한 패턴을 다른 프로젝트에 그대로 옮기기 위한 절차서다. MFC 를 아는 사람이라면 따라가서 1-2 시간 내 적용 가능하다.

---

## 1. 전제 조건

- **VS 2022 / 2026**, MSVC, MFC.
- 다이얼로그가 `CDialogEx` 파생.
- `D:\1.Projects_C++\Common` 의 헤더에 include path 가 잡혀 있다 (`D:\1.Projects_C++;`).
- `Common/win_compat/dwm.h` 가 존재. (없으면 Common repo `9ef5315` 이상으로 pull.)
- 다이얼로그가 `CSCColorTheme m_theme` 멤버를 갖는다. (없으면 추가. 메인 dlg 의 theme 색을 받아 child 에 전파하는 패턴.)

---

## 2. 무엇을 얻고 무엇을 잃는가

**얻는 것**
- caption bar 없음. 사용자가 원하는 위치/스타일로 자체 title 영역을 client 안에 그릴 수 있다.
- Win11 round corner + DWM shadow.
- OS 의 Aero Snap (드래그로 좌/우 절반 최대화, Win + 화살표 등) + 시스템 resize cursor.
- deactivate 시 흰 frame flash 없음. NC 영역 1px dark.

**잃는 것 / 비용**
- OnNcCalcSize / OnNcPaint / OnNcActivate / WindowProc 의 NC 트랩 6 군데 추가 코드.
- 자식 컨트롤이 dlg 가장자리에 닿으면 그 자식에도 WM_NCHITTEST 트랩 (HTTRANSPARENT) 필요. 이건 자식 *클래스마다 한 번씩* 해야 함.
- 자체 title 영역 (만약 필요하면) 의 paint / drag / 닫기 버튼 등 직접 구현.

---

## 3. 적용 순서

### Step 1 — Include 추가

대상 `.cpp` 상단:

```cpp
#include "Common/win_compat/dwm.h"
```

`win_compat::dwm` 는 OS 버전·SDK 버전 무관하게 안전히 동작 (XP/Vista 에선 no-op). 다른 곳에 dwmapi.lib 직접 link 할 필요 없음.

### Step 2 — 헤더에 핸들러 선언

`.h` 의 class 정의 public/protected 부분:

```cpp
//Borderless customized dlg — borderless dialog.md 참조.
afx_msg void OnNcCalcSize(BOOL bCalcValidRects, NCCALCSIZE_PARAMS* lpncsp);
afx_msg BOOL OnNcActivate(BOOL bActive);
afx_msg void OnNcPaint();
virtual LRESULT WindowProc(UINT message, WPARAM wParam, LPARAM lParam) override;
```

### Step 3 — 메시지 맵 등록

`.cpp` 의 `BEGIN_MESSAGE_MAP` 블록 안:

```cpp
ON_WM_NCCALCSIZE()
ON_WM_NCACTIVATE()
ON_WM_NCPAINT()
```

`WM_NCHITTEST` 는 ON_WM_NCHITTEST 매크로 안 쓰고 `WindowProc` 에서 직접 분기한다 (return value 가 `UINT` 인 표준 핸들러보다 `LRESULT` 직접 반환이 유연).

### Step 4 — OnInitDialog 에서 윈도우 스타일 변경

`CDialogEx::OnInitDialog()` 직후 (가급적 다른 자식 컨트롤 create 이전):

```cpp
//Borderless customized dlg 변환.
//caption / 시스템 border / dlg-frame 제거 + resize 용 WS_THICKFRAME 부여. SWP_FRAMECHANGED 로 NC 재계산.
{
    LONG_PTR style = ::GetWindowLongPtr(m_hWnd, GWL_STYLE);
    style &= ~(WS_CAPTION | WS_THICKFRAME | WS_BORDER | WS_DLGFRAME);
    style |= (WS_THICKFRAME | WS_SYSMENU | WS_MINIMIZEBOX | WS_MAXIMIZEBOX);
    ::SetWindowLongPtr(m_hWnd, GWL_STYLE, style);
    ::SetWindowPos(m_hWnd, nullptr, 0, 0, 0, 0,
        SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE | SWP_FRAMECHANGED);
}
```

**왜 `WS_THICKFRAME` 한 번 빼고 다시 넣나**: 리소스 에디터에서 켜져 있을 수도 / 없을 수도 있어 *항상 동일 상태* 만들기 위함. WS_CAPTION/WS_BORDER/WS_DLGFRAME 셋은 어느 조합이든 제거. 그 후 WS_THICKFRAME 만 다시 추가 → resize 가능 + caption 없음.

**SWP_FRAMECHANGED 가 핵심**: NC 영역 재계산을 OS 에 알린다. 이게 빠지면 style 변경이 화면에 반영 안 됨.

resize 가 필요 *없는* 다이얼로그면 WS_THICKFRAME 도 빼고 진행 가능 — 그 경우 OnNcCalcSize / WM_NCHITTEST 트랩도 생략 가능. (PlaylistDlg 처럼 수동 resize 처리하는 변종 패턴.)

### Step 5 — set_color_theme 안에서 DWM API 호출

```cpp
void CYourDlg::set_color_theme(const CSCColorTheme& theme)
{
    m_theme.copy_colors_from(theme);

    //... 자식 컨트롤들에 theme 전파 ...

    //Win11 DWM 합성 frame 을 dark 로 + round corner. theme 바뀔 때마다 동기화.
    if (m_hWnd)
    {
        win_compat::dwm::use_immersive_dark_mode(m_hWnd, true);
        win_compat::dwm::set_window_corner_round(m_hWnd);
        win_compat::dwm::set_border_color(m_hWnd, m_theme.cr_back.ToCOLORREF());
    }

    if (GetSafeHwnd())
        Invalidate();
}
```

**3 개 호출 모두 필요**:
- `use_immersive_dark_mode` — Win11 의 dark/light 비트. DWM 가 inactive frame 을 합성할 때 이 비트를 봄. dark 안 켜면 DWMWA_BORDER_COLOR 설정해도 light default 가 우선.
- `set_window_corner_round` — Win11 의 둥근 모서리 (`DWMWA_WINDOW_CORNER_PREFERENCE = DWMWCP_ROUND`).
- `set_border_color` — Win11 의 outer border 색 (`DWMWA_BORDER_COLOR`). theme.cr_back 으로 두면 NC 영역의 fill 과 같은 색이라 시각 seam 사라짐.

theme 변경 시점에 *한 번씩 다시* 호출되도록 set_color_theme 안에 두는 게 자연스럽다. OnInitDialog 안에서만 호출하면 theme 변경 시 동기화 안 됨.

### Step 6 — OnNcCalcSize: NC 영역 1px 로 축소

Win11 DWM 은 WS_THICKFRAME 윈도우에 default 로 7px 의 NC margin 을 둔다. 시각적으로 두꺼운 띠가 보이므로 6px 흡수해서 1px 만 남긴다.

```cpp
void CYourDlg::OnNcCalcSize(BOOL bCalcValidRects, NCCALCSIZE_PARAMS* lpncsp)
{
    CDialogEx::OnNcCalcSize(bCalcValidRects, lpncsp);

    //Win11 DWM 의 7px NC margin 을 1px 로 축소.
    //resize 는 WM_NCHITTEST trap 이 8px edge band 로 별도 처리.
    const int kNcShrink = 6;
    if (lpncsp && bCalcValidRects)
    {
        lpncsp->rgrc[0].left   -= kNcShrink;
        lpncsp->rgrc[0].top    -= kNcShrink;
        lpncsp->rgrc[0].right  += kNcShrink;
        lpncsp->rgrc[0].bottom += kNcShrink;
    }
}
```

**호출 순서 중요**: base 먼저 → 그 후 우리가 덮어쓰기. base 호출 *전* 에 rgrc 손대면 base 의 자체 NC 계산이 우리 보정을 덮어쓴다. (이거 거꾸로 했다가 한 세션 날렸음.)

NC=0 까지 가지 *말 것*. Win11 의 DWM 이 NC=0 + WS_THICKFRAME 윈도우에 default caption 을 자동 합성해버린다.

### Step 7 — OnNcPaint: NC 영역 dark fill

DefWindowProc(WM_NCPAINT) 가 그릴 default light frame brush 를 우리 dark 색으로 미리 덮어 가린다.

```cpp
void CYourDlg::OnNcPaint()
{
    //base CDialogEx::OnNcPaint 호출 안 함. 호출하면 default frame 이 우리 paint 위에 덮어씀.
    CWindowDC dc(this);

    CRect rcWindow;
    GetWindowRect(rcWindow);
    CRect rcWindowLocal(0, 0, rcWindow.Width(), rcWindow.Height());

    CRect rcClient;
    GetClientRect(rcClient);
    CPoint ptClientOrg(0, 0);
    ClientToScreen(&ptClientOrg);
    CRect rcClientInWin = rcClient;
    rcClientInWin.OffsetRect(ptClientOrg.x - rcWindow.left, ptClientOrg.y - rcWindow.top);

    dc.ExcludeClipRect(rcClientInWin);
    dc.FillSolidRect(rcWindowLocal, m_theme.cr_back.ToCOLORREF());
}
```

**핵심**:
- `CWindowDC` — window 전체 (NC 포함) 대상.
- `ExcludeClipRect(client)` — paint 가 client 영역 침범 안 하도록.
- base 호출 안 함 — base 호출 시 default 가 우리 paint 위에 덮어 그림.

### Step 8 — OnNcActivate: base 위임

```cpp
BOOL CYourDlg::OnNcActivate(BOOL bActive)
{
    //실제 NC paint 차단은 WindowProc 의 WM_NCACTIVATE → TRUE.
    return CDialogEx::OnNcActivate(bActive);
}
```

`OnNcActivate` 자체는 거의 no-op. 차단은 다음 step 의 `WindowProc` 에서.

### Step 9 — WindowProc: WM_NCACTIVATE 완전 차단 + WM_NCHITTEST resize

```cpp
LRESULT CYourDlg::WindowProc(UINT message, WPARAM wParam, LPARAM lParam)
{
    //DefWindowProc(WM_NCACTIVATE, wParam, -1) 는 Win11 에서 frame 자체를 light 색으로 그리는 경로가 남음.
    //완전 차단 위해 DefWindowProc 호출 안 하고 TRUE 반환.
    if (message == WM_NCACTIVATE)
        return TRUE;

    //NC=1px 라 default WM_NCHITTEST 의 resize 폭이 1px → 마우스 잡기 거의 불가.
    //우리가 가장자리 8px 폭에서 직접 resize 코드 반환.
    if (message == WM_NCHITTEST)
    {
        POINT pt = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
        CRect rcWin;
        GetWindowRect(rcWin);

        const int kEdge = 8;
        bool left   = pt.x >= rcWin.left   && pt.x <  rcWin.left   + kEdge;
        bool right  = pt.x <  rcWin.right  && pt.x >= rcWin.right  - kEdge;
        bool top    = pt.y >= rcWin.top    && pt.y <  rcWin.top    + kEdge;
        bool bottom = pt.y <  rcWin.bottom && pt.y >= rcWin.bottom - kEdge;

        if (top && left)     return HTTOPLEFT;
        if (top && right)    return HTTOPRIGHT;
        if (bottom && left)  return HTBOTTOMLEFT;
        if (bottom && right) return HTBOTTOMRIGHT;
        if (left)            return HTLEFT;
        if (right)           return HTRIGHT;
        if (top)             return HTTOP;
        if (bottom)          return HTBOTTOM;
        //내부는 default 가 HTCLIENT 반환하도록 위임.
    }

    return CDialogEx::WindowProc(message, wParam, lParam);
}
```

**핵심**:
- `WM_NCACTIVATE` → `return TRUE` (DefWindowProc *호출 안 함*). `lParam = -1` 트릭은 caption text 만 skip 이고 Win11 의 frame paint 는 여전히 light 색으로 발생.
- `WM_NCHITTEST` → 화면 좌표 (lParam) 기준으로 가장자리 8px 폭에서 resize 코드 직접 반환. NC 영역 두께 (1px) 와 무관하게 8px 폭 hit-test 보장.

---

## 4. 자식 컨트롤이 가장자리에 닿을 때 — HTTRANSPARENT 트랩

dlg 가장자리에 자식 컨트롤이 깔려 있으면, 그 영역에 마우스 올렸을 때 **OS 가 WM_NCHITTEST 를 dlg 가 아닌 자식한테 먼저 보낸다.** 자식이 `HTCLIENT` 반환하면 부모의 NCHITTEST trap 까지 안 가서 resize 가 안 잡힌다.

해결: 자식이 가장자리 8px 영역에서 `HTTRANSPARENT` 반환 → 메시지가 부모로 bubble up → 부모의 WM_NCHITTEST 가 resize 코드 반환.

### 자식 측 패턴 (예시: top strip 자식)

```cpp
//자식 헤더
virtual LRESULT WindowProc(UINT message, WPARAM wParam, LPARAM lParam) override;
```

```cpp
//자식 cpp
LRESULT CYourChildDlg::WindowProc(UINT message, WPARAM wParam, LPARAM lParam)
{
    //부모의 borderless 패턴 — 우리 가장자리가 부모 가장자리와 겹치는 영역에서 HTTRANSPARENT 반환.
    //자식이 HTCLIENT 반환하면 부모의 NCHITTEST trap 까지 안 와서 resize 불가.
    if (message == WM_NCHITTEST)
    {
        POINT pt = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
        CRect rcWin;
        GetWindowRect(rcWin);

        const int kEdge = 8;
        bool left  = pt.x >= rcWin.left   && pt.x <  rcWin.left   + kEdge;
        bool right = pt.x <  rcWin.right  && pt.x >= rcWin.right  - kEdge;
        bool top   = pt.y >= rcWin.top    && pt.y <  rcWin.top    + kEdge;
        //자식이 어느 edge 와 겹치는지에 따라 조건 조정 — 예: top strip 이면 top/left/right 만, bottom strip 이면 bottom/left/right 만.

        if (top || left || right)
            return HTTRANSPARENT;
    }

    return CDialogEx::WindowProc(message, wParam, lParam);
}
```

**Endorphin2 적용 예**:
- `CTitleDlg` (top strip): top / left / right 가장자리 → HTTRANSPARENT
- `CControlDlg` (bottom strip): bottom / left / right 가장자리 → HTTRANSPARENT

### 자식 안에 full-height 컨트롤이 또 있을 때 (slider, list 등)

부모 자식에 HTTRANSPARENT trap 을 넣어도, 그 자식 안의 *손자* 컨트롤이 full height 로 깔려 있으면 손자가 HTCLIENT 반환해서 마찬가지 막힌다.

**해결**: 손자 컨트롤의 *위치*를 가장자리에서 일정 px 떨어뜨려 빈 공간 확보. 그 빈 공간에 마우스 오면 부모-자식이 받아 처리.

**Endorphin2 적용 예** (CControlDlg 의 track slider):
```cpp
GetClientRect(rc);
rc.DeflateRect(4, 0, 4, 4);   //bottom 4px 흡수 — track slider 가 full height 로 깔리지 않도록.
// ... slider 들은 rc.bottom 기준으로 배치, 자동으로 4px 위에서 끊김
// ... buttons 도 같이 2px 위로 (시각 정렬)
r.top = rc.top + 3;   //was 5
```

이 작업 하면 시각적으로 컨트롤들이 *조금 위로* 이동한다. 디자인 OK 면 진행.

---

## 5. 선택 사항 — 자체 title 영역 (caption 대체)

caption 을 제거했으니 title 텍스트 / 닫기 버튼 등이 필요하면 client 안에 직접 만든다. CBookmarkEditDlg 가 채택한 패턴.

### 5.1 리소스 (.rc) 에 컨트롤 추가
- `IDC_STATIC_TITLE` (CSCStatic — title 텍스트 + 드래그 영역)
- `IDC_BUTTON_HIDE` (CGdiButton — 닫기 X 버튼)

### 5.2 헤더에 멤버 + DDX
```cpp
CSCStatic   m_static_title;
CGdiButton  m_button_hide;
afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
afx_msg void OnBnClickedButtonHide();
```

```cpp
//DoDataExchange
DDX_Control(pDX, IDC_STATIC_TITLE, m_static_title);
DDX_Control(pDX, IDC_BUTTON_HIDE,  m_button_hide);
```

```cpp
//Message map
ON_WM_LBUTTONDOWN()
ON_BN_CLICKED(IDC_BUTTON_HIDE, &CYourDlg::OnBnClickedButtonHide)
```

### 5.3 OnInitDialog / set_color_theme 에서 색 적용
```cpp
m_static_title.set_text_color(m_theme.cr_title_text);
m_static_title.set_back_color(m_theme.cr_title_back_inactive);
m_button_hide .set_text_color(m_theme.cr_title_text);
m_button_hide .set_back_color(m_theme.cr_title_back_inactive);
```

### 5.4 OnSize 에서 배치
```cpp
m_static_title.MoveWindow(0, 0, rc.Width(), TOOL_TITLE_HEIGHT);
m_button_hide .MoveWindow(rc.right - TOOL_TITLE_HEIGHT - 2, 2,
                          TOOL_TITLE_HEIGHT - 2, TOOL_TITLE_HEIGHT - 4);
//본문 자식들은 그 아래부터 배치 — top 에 TOOL_TITLE_HEIGHT 만큼 padding
```

### 5.5 title bar drag 로 윈도우 이동
```cpp
void CYourDlg::OnLButtonDown(UINT nFlags, CPoint point)
{
    //title bar 영역 클릭 → 시스템 드래그-이동 위임 (caption 클릭처럼 동작).
    CRect rcTitle;
    m_static_title.GetWindowRect(rcTitle);
    ScreenToClient(rcTitle);
    if (rcTitle.PtInRect(point))
    {
        ReleaseCapture();
        ::SendMessage(m_hWnd, WM_SYSCOMMAND, SC_MOVE | HTCAPTION, 0);
        return;
    }
    CDialogEx::OnLButtonDown(nFlags, point);
}

void CYourDlg::OnBnClickedButtonHide() { ShowWindow(SW_HIDE); }
```

---

## 6. 디버깅 — logWrite 로 검증

문제 생기면 `[nc]` 태그 로그 추가해 NC 흐름 추적.

```cpp
//OnInitDialog 끝부분
{
    LONG_PTR styleNow   = ::GetWindowLongPtr(m_hWnd, GWL_STYLE);
    LONG_PTR exStyleNow = ::GetWindowLongPtr(m_hWnd, GWL_EXSTYLE);
    CRect rcWin, rcCli;
    GetWindowRect(rcWin);
    GetClientRect(rcCli);
    logWrite(_T("[nc/init] style=0x%08X (THICKFRAME=%d CAPTION=%d) win=%dx%d client=%dx%d"),
        (unsigned int)styleNow,
        (int)((styleNow & WS_THICKFRAME) != 0),
        (int)((styleNow & WS_CAPTION) != 0),
        rcWin.Width(), rcWin.Height(),
        rcCli.Width(), rcCli.Height());
}
```

`OnNcCalcSize` / `OnNcPaint` / `OnNcActivate` / `WM_NCACTIVATE` 안에도 logWrite 추가해 호출 시점 / 매개변수 확인 가능. 문제 잡힌 후 제거하거나 유지 선택.

---

## 7. 자주 빠지는 함정

| 증상 | 원인 | 해결 |
|---|---|---|
| 두꺼운 흰 frame 이 deactivate 시 그려짐 | `WM_NCACTIVATE` 에서 `DefWindowProc(..., -1)` 만 함 | `return TRUE` 로 DefWindowProc 자체를 호출 안 함 |
| 두꺼운 dark frame 이 평소에도 보임 | NC 영역이 default 7px 그대로 | OnNcCalcSize 에서 6px 흡수해 1px 로 |
| Round corner / shadow 없음 | DWM 호출 누락 | set_color_theme 안에서 corner_round + immersive_dark_mode 호출 |
| Inactive frame 이 light 색 | dark mode 비트 미설정 | `use_immersive_dark_mode(true)` 호출 |
| `DwmSetWindowAttribute(BORDER_COLOR)` 효과 없음 | dark_mode 비트 안 켜져 default 가 우선 | dark_mode 먼저 켜고 border_color 설정 |
| 가장자리에서 resize cursor 안 뜸 | NC=1px → default hit-test 도 1px | WM_NCHITTEST trap 8px edge band 직접 반환 |
| 가장자리 *일부* 만 resize 안 됨 | 자식 컨트롤이 가장자리 위에 깔림 | 자식의 WindowProc 에서 HTTRANSPARENT 반환 |
| 자식 안의 손자 (slider 등) 가 가장자리 막음 | 손자가 부모 자식의 full height 차지 | OnSize 에서 손자 위치 가장자리에서 떨어뜨림 |
| NCCALCSIZE 의 보정이 무효 | base 호출 *전* 에 rgrc 변경 | base 호출 *후* 에 변경 |
| NC=0 시도했더니 default caption 이 자동 그려짐 | Win11 DWM 이 NC=0 + WS_THICKFRAME 윈도우에 caption 자동 합성 | NC=0 까지 가지 말고 1px 유지 |
| 빌드 시 한글 식별자 깨짐 (`'m_xxx': 선언되지 않은 식별자` 90+) | 파일 인코딩 BOM 누락 / CP949 | UTF-8 BOM + CRLF 강제 |

---

## 8. 이미 적용된 dlg 들 (Endorphin2 기준)

| dlg | 패턴 | 비고 |
|---|---|---|
| `CEndorphin2Dlg` (main) | 전체 적용 | 자체 title `m_title_dlg` + `m_control_dlg` 둘 다 자식 HTTRANSPARENT 적용 |
| `CBookmarkEditDlg` | 전체 적용 | 자체 title (CSCStatic + GdiButton) 추가, title 드래그-이동 핸들러 포함 |
| `CPlaylistDlg` | 변종 (WS_THICKFRAME 제거 + 수동 마우스 resize) | corner_round + DwmExtendFrameIntoClientArea 만 추가 |

PlaylistDlg 처럼 manual mouse resize 가 이미 구현돼 있는 dlg 는 NC 트랩 전체 적용보다 corner_round + shadow 만 추가하는 *옵션 2* 가 변경 위험 적다.

---

## 9. 참고 — `Common/win_compat/dwm.h` API

```cpp
namespace win_compat::dwm
{
    //Win10 1809+ : 윈도우 chrome 의 dark 비트.
    void use_immersive_dark_mode(HWND hwnd, bool dark = true);

    //Win11 22000+ : 둥근 모서리.
    void set_window_corner_round(HWND hwnd);

    //Win11 22000+ : DWM outer border 색.
    void set_border_color(HWND hwnd, COLORREF cr);
    void disable_border_color(HWND hwnd);  //=DWMWA_COLOR_NONE

    //Vista+ : borderless 윈도우의 DWM shadow 활성용.
    //         WS_THICKFRAME 없는 윈도우에 shadow 가 필요하면 margins {0,0,0,1} 정도면 충분.
    void extend_frame_into_client_area(HWND hwnd,
                                       int left = 0, int top = 0,
                                       int right = 0, int bottom = 1);

    //Vista+ : 보이는 frame 의 정확한 경계 (drop shadow 포함 시 ClientRect 와 다름).
    void get_extended_frame_bounds_or_window_rect(HWND hwnd, RECT& out_rect);
}
```

XP/Vista 등 dwmapi 자체가 없거나 attribute 미지원 OS 에선 모든 함수가 no-op (E_INVALIDARG 무시). 안전.

---

## 10. 체크리스트 (적용 시 따라가며)

- [ ] `#include "Common/win_compat/dwm.h"` 추가
- [ ] 헤더에 OnNcCalcSize / OnNcActivate / OnNcPaint / WindowProc 선언
- [ ] 메시지 맵에 ON_WM_NCCALCSIZE / NCACTIVATE / NCPAINT
- [ ] OnInitDialog 에 style 변경 + SWP_FRAMECHANGED
- [ ] set_color_theme 에 immersive_dark_mode + corner_round + border_color
- [ ] OnNcCalcSize 본문 — base 후 6px 흡수
- [ ] OnNcPaint 본문 — CWindowDC + ExcludeClipRect(client) + FillSolidRect
- [ ] OnNcActivate 본문 — base 위임
- [ ] WindowProc 본문 — WM_NCACTIVATE → TRUE, WM_NCHITTEST → 8px edge
- [ ] 가장자리에 깔린 자식 컨트롤 식별 → 각각 WindowProc 추가 + HTTRANSPARENT
- [ ] 그 자식 안의 full-height 손자 컨트롤 식별 → OnSize 에서 위치 조정
- [ ] (선택) 자체 title 영역 — IDC_STATIC_TITLE / IDC_BUTTON_HIDE 추가, OnLButtonDown 드래그
- [ ] 빌드 후 시각 확인 — round corner / shadow / dark frame / resize cursor / 메뉴 표시 시 흰 띠 없음
- [ ] 인코딩 확인 — `file <path>` 가 "UTF-8 (with BOM) ... CRLF" 인지

---

작업 기록: Endorphin2 `b0163ae` ~ (BookmarkEditDlg 적용 시점) — 한 사람이 거의 *한 세션*에 끝낼 수 있는 분량.
