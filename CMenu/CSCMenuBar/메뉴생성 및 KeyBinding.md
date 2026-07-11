# CSCMenu + CSCKeyBindings — 메뉴 생성 및 단축키 자동 처리

> 대상: `Common\CMenu\CSCMenuBar\` (CSCMenu) + `Common\system\SCKeyBindings\` (CSCKeyBindings) 를 처음 도입하는 프로젝트의 개발자.
> 참조 구현: `D:\1.projects_c++\MiniClock2` — 단일 컨텍스트 메뉴 + 자동 단축키 최소 케이스.

---

## <span style="color:#0066cc">0. 왜 CMenu 대신 CSCMenu 인가</span>

MFC 의 `CMenu` + `TrackPopupMenu` 는 **OS 가 그려주는 시스템 룩** 을 그대로 쓴다. 어느 앱이든 똑같은 회색 메뉴. 반면 `CSCMenu` 는 **자체 페인트** 한다 — 그래서 앱의 컬러 테마와 통일된 외관, 아이콘, 서브버튼, 썸네일, 스크롤, 커스텀 폰트가 가능.

### 한눈에 보는 비교

| 항목 | `CMenu` + TrackPopupMenu | <span style="color:#2e7d32">`CSCMenu`</span> |
|---|---|---|
| **외관** | OS 시스템 룩 (회색 고정) | <span style="color:#2e7d32">**컬러 테마 지원**</span> (dark / dark_gray / light / white / 커스텀) — 앱 전체 톤과 정합 |
| **폰트** | 시스템 폰트 (변경 불가) | <span style="color:#2e7d32">**임의 폰트·크기·굵기**</span> (`set_font_name` / `set_font_size` / `set_font_bold`) |
| **라인 높이** | OS 고정 | <span style="color:#2e7d32">**임의 지정**</span> (`set_line_height(28)` 등) — 폰트 크기에 맞춰 여백 확보 |
| **항목 아이콘** | 32bpp 비트맵 (`SetMenuItemBitmaps`) — 알파 다루기 까다로움 | <span style="color:#2e7d32">**투명 PNG 리소스 직접**</span> (`CSCMenuItem` 생성자에 icon_id) |
| **서브버튼** | 없음 | <span style="color:#2e7d32">**항목 오른쪽에 radio / check 다수 배치**</span> — 예: "화면 비율" 항목에서 4:3 / 16:9 / 자동 아이콘 나열 |
| **썸네일 항목** | 없음 | <span style="color:#2e7d32">**미니 이미지 + 2 줄 텍스트**</span> — 북마크·최근파일 미리보기 (`add_thumbnail_item`) |
| **긴 메뉴** | 화면 밖은 잘림 | <span style="color:#2e7d32">**자동 스크롤 arrow**</span> (`scroll_arrow_h`) — 모니터 높이 초과 시 위/아래 화살표 |
| **submenu** | 표준 nested | <span style="color:#2e7d32">**동적 attach**</span> — `.rc` 의 POPUP 자리에 코드에서 만든 CSCMenu 인스턴스를 caption/ID 로 매핑 (동적 리스트 메뉴 편리) |
| **외부 클릭 dismiss** | OS 처리 | <span style="color:#2e7d32">**mouse hook 로 자체 처리**</span> — 토글 트리거 재클릭 시 즉시 재오픈 방지 (per-instance debounce) |
| **키보드 nav** | OS 처리 | <span style="color:#2e7d32">**Up/Down/Left/Right/Enter/Esc + access key 자체 구현**</span> (`&X` 마커 인식) |
| **키 dispatch** | 개발자가 `ACCELERATORS` 리소스 수동 편집 | <span style="color:#2e7d32">**캡션의 `\t단축키` 자동 파싱**</span> (`CSCKeyBindings::seed_from_scmenu`) → `HACCEL` 자동 등록 |
| **remap UI** | 없음 (수작업) | <span style="color:#2e7d32">**사용자 커스터마이즈 지원**</span> (`set_binding` + `save_overrides` — 레지스트리에 사용자 override 저장) |
| **XP 호환** | O | O — <span style="color:#2e7d32">GDI+ 만 사용, Vista+ API 없음</span> |

### <span style="color:#0066cc">CSCMenu 가 진짜로 좋은 5 가지 상황</span>

1. **다크 테마 앱** — 다른 부분은 다 다크인데 메뉴만 밝은 회색으로 튀는 문제. `set_color_theme(color_theme_dark_gray)` 한 줄로 통일.
2. **아이콘 있는 메뉴** — 재생 컨트롤·에디터 툴 같이 각 항목에 이미지가 필요할 때. PNG 리소스 하나로 hover/selected 자동 처리.
3. **한 항목에 여러 옵션 (radio)** — "화면 비율: 4:3 / 16:9 / 자동" 을 한 줄에. 서브버튼으로 배치.
4. **캡션에 표시된 단축키가 실제로 동작해야 할 때** — 캡션에 `\tCtrl+T` 만 적으면 끝. `.rc` ACCELERATORS 수동 편집 불필요, 캡션·accelerator 불일치 사고 방지.
5. **북마크·최근파일 목록 메뉴** — 썸네일 이미지 + 2 줄 텍스트 (제목/부제) 필요할 때.

### <span style="color:#c62828">언제 CSCMenu 가 과할까</span>

- **일회성 debug 팝업** — `TrackPopupMenu` 4 줄로 되는 걸 굳이. 시스템 룩이라도 무관.
- **표준 문서앱** — Notepad / calculator 처럼 OS 룩이 오히려 자연스러운 경우.
- **초 저사양 요구** — CSCMenu 는 GDI+ 페인트 (렌더링 비용 조금 더). 다만 popup 은 순간이라 실측상 무시할 수준.

### 요약

> **컬러 테마 통일** · **아이콘·썸네일·서브버튼** · **캡션의 `\t단축키` 자동 dispatch** — 이 세 가지 중 하나라도 필요하면 CSCMenu. 다 필요 없으면 CMenu 로도 충분.

이 문서는 세 번째 (**자동 단축키**) 를 중심으로, `.rc` → CSCMenu 로드 → CSCKeyBindings 시드 → HACCEL 발화 의 한 세트 흐름을 설명한다.

---

## 1. 두 클래스의 역할

| 클래스 | 역할 | 관계 |
|---|---|---|
| `CSCMenu` (`Common\CMenu\CSCMenuBar\SCMenu.h/cpp`) | 리소스 메뉴를 자체 페인트로 팝업. 컬러 테마, 아이콘, 서브버튼(radio/check), 썸네일, submenu, 스크롤 등 지원. `TrackPopupMenu` 대체. | 캡션의 `\t` **뒤 표기(`m_hot_key`)** 를 보관만 함 — 실제 키 dispatch 는 안 함. |
| `CSCKeyBindings` (`Common\system\SCKeyBindings\SCKeyBindings.h/cpp`) | 캡션의 `\t단축키` 를 파싱해 표준 Win32 `HACCEL` 로 등록. `TranslateAccelerator` 로 `WM_COMMAND` 발화 → 기존 `ON_COMMAND` 핸들러가 그대로 실행. | 메뉴 클릭과 단축키가 **같은 `WM_COMMAND` 경로**로 수렴 — 로직 중복 0.

**핵심**: `CSCMenu` 는 화면 그리기, `CSCKeyBindings` 는 키 발화. 서로 독립적이지만 `.rc` 리소스라는 single source of truth 를 공유.

---

## 2. 프로젝트에 추가할 소스 파일

`.vcxproj` / `.vcxproj.filters` 에서 다음을 **cpp·h 세트로 함께** (§Common `claude.md` §2.2) 필터 이름 `Common\CSCMenu`, `Common` 등에 배치:

```
Common\CMenu\CSCMenuBar\SCMenu.cpp/h        (CSCMenu 본체)
Common\CMenu\CSCMenuBar\SCMenuBar.cpp/h     (CSCMenuBar — 상단 menu bar 필요 시)
Common\CMenu\CSCMenuBar\SCMenuButton.cpp/h  (CSCMenu 내부 사용)
Common\system\SCKeyBindings\SCKeyBindings.cpp/h
```

의존:
- `CGdiButton` (`Common\CButton\GdiButton\`)
- `CSCGdiplusBitmap` (`Common\SCGdiplusBitmap.h`)
- `CSCColorTheme` (`Common\colors.h/cpp`)
- `Functions.h/cpp`

이 프로젝트가 다른 SC* 컨트롤 (`CSCEdit`, `CSCListCtrl` 등) 을 이미 쓰고 있다면 위 의존은 대부분 이미 들어있음.

PCH 규약: 4 config 모두 `<PrecompiledHeader>NotUsing</PrecompiledHeader>`.

---

## 3. `.rc` 메뉴 캡션 규약

메뉴 항목 캡션에 **`\t단축키표기`** 를 붙이면 그게 자동 accelerator 로 등록된다.

```rc
IDR_MENU_CONTEXT MENU
BEGIN
    POPUP "Context Menu"
    BEGIN
        MENUITEM "설정 시각 목록 보기\tF5",             ID_MENU_VIEW_TIME_LIST
        MENUITEM "항상 맨 위에(&T)\tCtrl+T",            ID_MENU_ALWAYS_ON_TOP
        POPUP "기타 기능"
        BEGIN
            MENUITEM "1번 위치로 이동\t1",              ID_MENU_MOVE_TO_POS1
            MENUITEM "1번 위치로 저장\tF1",             ID_MENU_SAVE_POS1
            MENUITEM "2번 위치로 이동\t2",              ID_MENU_MOVE_TO_POS2
            ...
        END
        MENUITEM "프로그램 닫기(&X)\tAlt+F4",           ID_MENU_CLOSE
    END
END
```

파서가 인식하는 표기 형식 (모디파이어 순서 무관·대소문자 무관):

| 카테고리 | 예 |
|---|---|
| 모디파이어 | `Ctrl+`, `Control+`, `Alt+`, `Shift+` |
| 문자·숫자 | `A`~`Z`, `0`~`9` (단독 or 조합) |
| 함수키 | `F1` ~ `F24` |
| 네비/편집 | `Left`, `Right`, `Up`, `Down`, `Home`, `End`, `PgUp`/`PageUp`/`Prior`, `PgDn`/`PageDown`/`Next`, `Insert`, `Delete`, `Space`, `Enter`, `Tab`, `Esc`/`Escape`, `Backspace` |
| 기호 | `Plus`, `Minus`, `Comma`, `Period` |
| 조합 예 | `Ctrl+Alt+E`, `Shift+Left`, `Ctrl+F1` |

**미인식 표기 (예: `"Enter or DBClick"` 같이 자연어 hint)** 는 파서가 실패로 처리해 accelerator 등록만 skip — 메뉴 표시는 그대로 되지만 자동 단축키는 안 붙는다. 필요하면 그 항목만 별도로 `PreTranslateMessage` 에서 처리하거나 `register_action` 으로 코드에서 등록.

---

## 4. 헤더 (`YourDlg.h`) 세팅

```cpp
#include "Common/CMenu/CSCMenuBar/SCMenu.h"
#include "Common/system/SCKeyBindings/SCKeyBindings.h"

class CYourDlg : public CDialogEx
{
    ...
protected:
    CSCMenu             m_menu_context;                  //컨텍스트 메뉴 인스턴스
    CSCKeyBindings      m_keybindings;
    HACCEL              m_hAccel = NULL;

    LRESULT             on_message_CSCMenu(WPARAM, LPARAM);
    ...
public:
    virtual BOOL        PreTranslateMessage(MSG* pMsg);
    afx_msg void        OnContextMenu(CWnd* pWnd, CPoint point);
    afx_msg void        OnDestroy();
    ...
};
```

---

## 5. 구현 (`YourDlg.cpp`) 세팅 — 순서대로

### 5.1 메시지 맵

```cpp
BEGIN_MESSAGE_MAP(CYourDlg, CDialogEx)
    ...
    ON_WM_CONTEXTMENU()
    ON_WM_DESTROY()
    ON_REGISTERED_MESSAGE(Message_CSCMenu, &CYourDlg::on_message_CSCMenu)
END_MESSAGE_MAP()
```

`Message_CSCMenu` 는 `SCMenu.h` 에서 `RegisterWindowMessage` 로 정의된 static UINT (§2A.7 컨벤션).

### 5.2 `OnInitDialog` — 메뉴 생성 + 단축키 시드

```cpp
BOOL CYourDlg::OnInitDialog()
{
    CDialogEx::OnInitDialog();
    ...

    //(1) CSCMenu 인스턴스 생성 + 컬러 테마 + 리소스 로드
    m_menu_context.create(this, 240);                              //parent + 최소 width
    m_menu_context.set_color_theme(CSCColorTheme::color_theme_dark_gray);
    m_menu_context.load(IDR_MENU_CONTEXT, 0);                      //IDR 의 GetSubMenu(0)

    //(2) 이미 load 된 m_items 를 그대로 시드 — 리소스 이중 load 불필요.
    //    m_sub_menu 를 재귀 순회하므로 attach 된 서브메뉴까지 자동 커버.
    m_keybindings.seed_from_scmenu(&m_menu_context);
    m_hAccel = m_keybindings.build_haccel();

    return TRUE;
}
```

`create` / `load` 파라미터:
- `create(parent, min_width)` — min_width 는 최소 폭. 항목 캡션이 넓으면 자동 확장.
- `load(res_id, idx0, idx1 = -1, include_popup_placeholder = false)` — `LoadMenu(res_id).GetSubMenu(idx0)` 를 로드. 서브 popup 은 auto-nest (내부에서 owned sub_menu 생성).

**시드/빌드가 왜 두 줄인가**: `seed_from_*` 는 additive (여러 source 를 순차 축적 가능), `build_haccel` 은 finalize. `register_action`·`load_overrides`·`seed_from_resource` 등을 추가 시드로 끼울 수 있게 분리되어 있다. 합치면 다중 source 조합·remap UI (`set_binding` 후 재빌드) 흐름이 깨진다.

### 5.3 `OnContextMenu` — 팝업

```cpp
void CYourDlg::OnContextMenu(CWnd* pWnd, CPoint point)
{
    //(1) dynamic state 반영 — 체크 표시, 조건부 caption 변경 등.
    m_menu_context.check_item(ID_MENU_ALWAYS_ON_TOP, is_top_most(m_hWnd));

    if (CSCMenuItem* item = m_menu_context.get_menu_item(ID_MENU_SHUTDOWN))
    {
        if (m_system_shutdown.IsEmpty())
            item->m_caption = _T("시스템 종료 시각 설정(&S)...");
        else
            item->m_caption.Format(_T("%s시에 자동 종료. 변경..."), m_system_shutdown.GetString());
    }

    //(2) 팝업. modeless — 이 호출은 즉시 리턴. close 는 on_message_CSCMenu 의 hide 분기에서.
    m_menu_context.popup_menu(point.x, point.y);
}
```

동적 상태 API:
- `m_menu_context.check_item(id, bool)` — CheckMenuItem 동등.
- `m_menu_context.enable_item(id, bool)` — EnableMenuItem 동등.
- `m_menu_context.check_radio_item(first_id, last_id, selected_id)` — CheckMenuRadioItem 동등.
- `item->m_caption = ...` — 직접 수정 (Endorphin2 패턴). 폭 재측정은 다음 popup 에서 반영.

### 5.4 `on_message_CSCMenu` — selchanged → WM_COMMAND

```cpp
LRESULT CYourDlg::on_message_CSCMenu(WPARAM wParam, LPARAM /*lParam*/)
{
    CSCMenuMessage* msg = (CSCMenuMessage*)wParam;
    if (!msg)
        return 0;

    //메뉴가 닫힘 — hover 로직·타이머 등 자체 처리가 필요하면 여기서.
    if (msg->m_message == CSCMenu::message_scmenu_hide)
    {
        //예: hover-driven UI 재개
        return 0;
    }

    if (msg->m_message != CSCMenu::message_scmenu_selchanged || !msg->m_menu_item)
        return 0;

    //항목 선택 → 기존 ON_COMMAND 핸들러로 그대로 dispatch. accelerator 경로와 완전 동일.
    SendMessage(WM_COMMAND, MAKEWPARAM(msg->m_menu_item->m_id, 0), 0);
    return 0;
}
```

메뉴 클릭 → `WM_COMMAND(id)` → `ON_COMMAND(ID_..., &Cls::OnMenuXxx)` 핸들러 실행.
accelerator 도 `TranslateAccelerator` 가 같은 `WM_COMMAND(id)` 를 발화 → **핸들러 1개로 두 경로 커버**.

### 5.5 `PreTranslateMessage` — accelerator dispatch

```cpp
BOOL CYourDlg::PreTranslateMessage(MSG* pMsg)
{
    if (m_hAccel && ::TranslateAccelerator(m_hWnd, m_hAccel, pMsg))
        return TRUE;

    return CDialogEx::PreTranslateMessage(pMsg);
}
```

이 한 줄로 캡션에 표기한 모든 단축키가 동작한다.

### 5.6 `OnDestroy` — HACCEL 정리

```cpp
void CYourDlg::OnDestroy()
{
    if (m_hAccel)
    {
        ::DestroyAcceleratorTable(m_hAccel);
        m_hAccel = NULL;
    }
    CDialogEx::OnDestroy();
}
```

---

## 6. 데이터 흐름 요약

```
.rc IDR_MENU_CONTEXT
     │
     │ m_menu_context.load(IDR_MENU_CONTEXT, 0)
     ▼
CSCMenu.m_items[]  ← 각 CSCMenuItem 에 (m_id, m_caption, m_hot_key) 저장
     │
     │ m_keybindings.seed_from_scmenu(&m_menu_context)
     │      └ 재귀: m_items → item.m_sub_menu → ...
     │      └ 각 item.m_hot_key 를 string_to_key 파싱 → (fVirt, VK) → m_bindings 축적
     ▼
CSCKeyBindings.m_bindings[]
     │
     │ m_hAccel = m_keybindings.build_haccel()
     ▼
HACCEL  (Win32 accelerator table)

─── 런타임 ───
사용자 우클릭 → OnContextMenu → dynamic state → popup_menu()
                                                    │
                                                    ▼
                            항목 클릭 → Message_CSCMenu (selchanged)
                                                    │
                                                    ▼
                                     SendMessage(WM_COMMAND, id) ─┐
                                                                  │
사용자 키 입력 → PreTranslateMessage                              ├→ ON_COMMAND(id) 핸들러
                → TranslateAccelerator(m_hAccel, pMsg)            │
                → Windows 가 WM_COMMAND(id) 발화 ─────────────────┘
```

---

## 7. 자주 하는 실수 / 함정

1. **CMenu 재로드로 시드하지 말 것.** `seed_from_scmenu(&m_menu_context)` 로 이미 load 된 CSCMenu 의 items 를 그대로 시드. `CMenu::LoadMenu(...)` + `seed_from_menu(hMenu)` 는 옛 API (CSCMenu 없는 프로젝트용) — CSCMenu 를 이미 쓴다면 재로드는 낭비.

2. **`build_haccel` 을 여러 번 호출할 땐 이전 HACCEL destroy.** remap UI 에서 `set_binding` 후 재빌드하는 흐름이라면:
   ```cpp
   if (m_hAccel) ::DestroyAcceleratorTable(m_hAccel);
   m_hAccel = m_keybindings.build_haccel();
   ```

3. **`OnContextMenu` 에서 매번 CSCMenu create/load 하지 말 것.** create/load 는 OnInitDialog 에서 1회. popup 직전엔 dynamic state (check·caption) 만 갱신 후 `popup_menu()`.

4. **동적 caption 변경 시 `&` access key 유실.** `item->m_caption = "일반 문자열"` 로 덮으면 `&X` 마커가 사라진다. 유지하려면 새 문자열에도 포함:
   ```cpp
   item->m_caption = _T("시스템 종료 시각 설정(&S)...");   //OK
   ```

5. **`\t단축키` 파서가 실패하는 표기는 accelerator 등록 안 됨.** `"Enter or DBClick"` 같이 자연어 hint 는 skip. 필요하면 `PreTranslateMessage` 에서 직접 처리 or `register_action`.

6. **여러 항목이 같은 chord 를 물면 뒤에 시드된 게 이긴다.** `seed_from_scmenu` 내부 `add_or_update_binding` 이 dedup — 동일 chord 를 갖던 이전 cmd 는 제거. 즉 메뉴가 source of truth.

7. **컨텍스트 메뉴가 여럿이면 각각 seed_from_scmenu.** 예: 파일 리스트 우클릭 메뉴 + 배경 우클릭 메뉴가 다르면 두 CSCMenu 인스턴스를 각각 `seed_from_scmenu` 호출.

8. **CSCMenu 는 modeless.** `popup_menu()` 는 즉시 리턴. 팝업 닫힘 확인은 `on_message_CSCMenu` 의 `message_scmenu_hide` 분기에서.

---

## 8. `seed_*` 와 `build_haccel` 분리 이유

`seed_from_scmenu` 는 여러 시드 방식 중 하나. 완전한 사용처는 이렇게 조합될 수 있다:

```cpp
m_keybindings.seed_from_resource(IDR_ACCELERATOR);       //(1) .rc ACCELERATORS 기본값 (있으면)
m_keybindings.seed_from_scmenu(&m_menu_context);         //(2) 메뉴 캡션 override
m_keybindings.register_action(ID_XX, _T("xx_action"),    //(3) 코드-only 액션 (메뉴에 없는 shortcut)
    _T("XX 액션"), FVIRTKEY | FCONTROL, VK_OEM_4);
m_keybindings.load_overrides();                          //(4) 레지스트리의 사용자 커스터마이즈
m_hAccel = m_keybindings.build_haccel();                 //(5) 최종 HACCEL 생성
```

시드는 additive, build 는 finalize. 합치면 (1)~(4) 처럼 여러 source 를 순차 축적하는 흐름이 깨진다 — 매 seed 마다 HACCEL 이 새로 만들어져 leak 발생, remap UI 재빌드 흐름도 무너짐. **분리는 유연성을 위한 의도적 설계다.**

단일 source (컨텍스트 메뉴 하나) 만 쓰는 프로젝트는 (2)+(5) 두 줄로 끝. MiniClock2 참조.

---

## 9. 참조 구현 파일

**MiniClock2** — 단일 컨텍스트 메뉴 + 자동 단축키의 최소 완성 케이스:
- `MiniClock2Dlg.h` — 멤버 3개 (`m_menu_context`, `m_keybindings`, `m_hAccel`) + `on_message_CSCMenu` 선언
- `MiniClock2Dlg.cpp`:
  - `OnInitDialog` 끝: create → load → `seed_from_scmenu` → `build_haccel`
  - `OnContextMenu`: dynamic state 반영 + `popup_menu`
  - `on_message_CSCMenu`: hide / selchanged 처리
  - `PreTranslateMessage`: `TranslateAccelerator` 우선
  - `OnDestroy`: `DestroyAcceleratorTable`
- `MiniClock2.rc` — 메뉴 캡션에 `\t단축키` 표기 (`\tF5`, `\tCtrl+T`, `\t1`, `\tF1`, `\tAlt+F4`)

**Endorphin2** — 다중 CSCMenu + submenu 트리 + `.rc` ACCELERATORS + `register_action` + `load_overrides` 의 풀 케이스 (현재는 `seed_from_menu(hMenu)` 사용 — 향후 `seed_from_scmenu(&m_menu)` 이관 예정).
