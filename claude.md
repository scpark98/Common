# Common 규칙 (모든 C++ 프로젝트 공용)

이 문서는 `D:\1.Projects_C++\Common` repo 에 속한다. 모든 프로젝트의 `claude.md` 가 `@../Common/claude.md` (또는 하위 경로에서는 `@../../Common/claude.md`) 로 import 하여 공통 규칙으로 적용한다.

Common repo 자체도 github.com/scpark98/Common 으로 집·회사 간 git 동기화된다.

---

## 1. 파일 네이밍 — GNU 스타일

새로 생성하는 파일명은 **소문자 + 언더스코어** 를 기본으로 한다.

- O: `claude.md`, `settings.local.json`, `build_notes.md`, `zoom_fix.patch`
- X: `CLAUDE.md`, `BuildNotes.md`, `zoomFix.patch`

예외: 툴·언어 생태계 관례로 고정된 파일명(`CMakeLists.txt`, `Dockerfile` 등)과 repo 에 이미 존재하는 기존 파일명은 그대로 따른다.

## 2. Common 라이브러리 공유 의존

이 사용자의 대부분 C++(MFC) 프로젝트는 `D:\1.Projects_C++\Common` 의 공통 컨트롤·다이얼로그·함수집합을 광범위하게 재사용한다. 레지스트리 저장/복원, 커스텀 컨트롤 동작 등 상당수 기능이 Common 라이브러리 내부에 구현되어 있다.

**현재 프로젝트 폴더 내 grep 결과만으로 "기능 없음" 이라고 단정하지 말 것.**

적용 방법:
1. 멤버 호출(`m_xxx.foo()`) 을 보면 먼저 `m_xxx` 의 **타입**을 헤더에서 확인 (grep 말고 Read)
2. 타입이 현재 프로젝트 내 정의가 아니면 `.vcxproj` 의 `ClInclude` 나 헤더의 `#include "Common/..."` 를 따라가 실제 파일 위치 확인
3. 레지스트리 키 이름에 Common 클래스명(예: `setting\CSCD2ImageDlg`) 이 보이면 그 클래스 내부를 우선 의심
4. 필요하면 `D:\1.Projects_C++\Common\...` 의 소스를 직접 읽을 것 — 현재 프로젝트의 일부로 취급

### 2.0 헬퍼 함수를 *새로* 만들기 전 — Common 먼저 (강제)

새로운 helper 함수·utility 를 작성하기 직전에 **반드시 `Common/Functions.h` + `Common/Functions.cpp` 를 먼저 검색**한다. 같은 일 하는 함수가 이미 있을 확률이 매우 높다.

**Why:** 이 사용자의 Common 라이브러리는 수년간 누적된 범용 헬퍼들을 보유하고 있다. 모니터 enumeration, 가상 데스크톱 영역, 윈도우 위치 복원, 경로 파싱(`get_part`), 색상 매크로(`gRGB`), 그라디언트 / 라운드 사각형 그리기, 폴더 입출력, 문자열 변환 등 — 거의 모든 "상식적 유틸" 이 이미 작성되어 있다. 자체 구현하면 (a) Common 의 모듈성·일관성을 깨뜨리고, (b) 두 머신·여러 프로젝트가 공유하는 자산을 활용하지 못하며, (c) 사용자가 같은 피드백을 반복하게 만든다. **이 피드백은 여러 차례 반복됐다 — 같은 실수를 또 하지 말 것.**

**구체 사례 (2026-04-27, SCDeskTools 모니터 캡처):**
- `enum_monitor_rects()` 라는 자체 helper 작성 → 사용자 지적 → Common 의 `enum_display_monitors()` + `g_monitors` + `get_monitor_rect(-1)` 로 전부 대체 가능했음.
- `GetSystemMetrics(SM_*VIRTUALSCREEN)` 4번 조합도 `get_monitor_rect(-1)` 한 번으로 충분.

**적용 (helper 작성 전 의무 절차):**

1. **Functions.h 카테고리 grep** — 함수 의도와 관련된 키워드로 검색. 예: 모니터 → `monitor`, 색상 → `color`/`gRGB`/`Gdiplus`, 경로 → `path`/`get_part`/`fn_`, 윈도우 → `window`/`hwnd`/`Restore`/`Save`, 그리기 → `draw`/`gradient`/`round`.
2. **Common/architecture.md 인덱스 확인** (§3A 참조) — 어느 카테고리/모듈에 있을지 후보 좁힘.
3. **존재 확인되면 사용** — 그게 80~90% 의 케이스다. 시그니처가 약간 부족하면 Common 함수에 인자 추가하는 쪽이 자체 구현보다 낫다.
4. **검색 후에도 못 찾으면 사용자에게 질문** — *"이 helper 가 Common 에 있는지 못 찾겠는데, 자체 구현해도 될까요? 아니면 어느 모듈을 보면 좋을까요?"* 형태로. 임의 자체 구현 금지.

**금지 패턴:**
- ❌ 프로젝트 내 grep 으로 안 보인다고 즉시 새 helper 작성.
- ❌ Win32 API (`EnumDisplayMonitors`, `GetSystemMetrics`, `SHGetPathFromIDList` 등) 를 직접 호출하는 짧은 wrapper 가 필요해 보일 때 — 이런 게 Common 의 핵심 기능이다. 거의 100% 이미 있음.
- ❌ "간단하니까 그냥 한 번 만들자" — 단순함이 정당화 사유가 되지 않는다. 일관성이 더 중요.

### 2.1 Common 의존 세부 규약

- **Include 경로 전제**: 프로젝트 IncludePath 에 `D:\1.Projects_C++;` 가 들어 있어 `#include "Common/colors.h"`, `#include "Common/Functions.h"`, `#include "Common/SCGdiplusBitmap.h"` 등이 직접 동작한다. 새 프로젝트 셋업시 이 경로 누락 여부 먼저 확인.
- **색상 타입 기본 = `Gdiplus::Color`**. 특별한 이유 없으면 `COLORREF` 대신 `Gdiplus::Color` 사용. `RGB(r,g,b)` 대신 `gRGB(r,g,b)` (colors.h 매크로). GDI API 에 넘길 때만 `RGB(c.GetR(), c.GetG(), c.GetB())` 로 변환.
- **표준 색은 named constant 직접 사용 (강제)** — `Gdiplus::Color::{White, Black, Red, Green, Blue, Yellow, Transparent, ...}` 가 이미 GDI+ 에 정의되어 있다. 호출처에서 **그 이름을 그대로 인자로 넘긴다**.
  - O: `draw_rect(d2dc, rc, Gdiplus::Color::Transparent, cr_mask, 0, 0);`
  - X: `const Gdiplus::Color cr_transp = Gdiplus::Color::Transparent;` 후 `cr_transp` 사용
  - X: `const Gdiplus::Color cr_white = Gdiplus::Color(255,255,255,255);` 후 `cr_white` 사용
  - **Why:** 이미 named 인 식별자를 다시 임시 변수로 묶는 건 줄 수만 늘리는 노이즈. 의미 추가 없음. *반복되는 색 = 자동으로 const 추출* 같은 습관을 적용하지 말 것.
  - **반복 위반 사례 (2026-04-27, SCDeskTools 한 세션 안에서 2회)**:
    1. `SCCapturedNoteDlg.cpp` 닫기 버튼 X 그리기 — `cr_white` 임시 변수 추출 → 사용자 지적
    2. `SCRegionCaptureDlg.cpp` paint — `cr_transp` 임시 변수 추출 → 사용자 재지적, *"규칙을 만들어주면 뭐하나? 지키지도 않는데?"*
  - **체크 절차 (강제)**: `Gdiplus::Color(...)` 또는 `D2D1::ColorF(...)` 로 색을 만들기 전에 `Gdiplus::Color::<Name>` 으로 이미 있는지 확인. 있으면 그것을 직접 인자로. 없을 때만 `Gdiplus::Color(a, r, g, b)` 인스턴스 생성.
- **컬러 프리셋 = `CSCColorTheme`**. 커스텀 컨트롤은 개별 `m_cr_*` 멤버 대신 `CSCColorTheme m_theme = CSCColorTheme(this);` 로 보관. 필드는 `m_theme.cr_text`, `cr_back`, `cr_border_active/inactive`, `cr_text_selected/dim`, `cr_back_selected`, `cr_parent_back` 등. 테마 교체: `m_theme.set_color_theme(CSCColorTheme::color_theme_dark)`. 개별 setter 는 필드에 직접 쓰기 후 `Invalidate()`. disabled/readonly 같이 theme 에 직접 필드 없는 상태는 `cr_text_dim`, `cr_parent_back` 으로 매핑.
- **GDI+ 수동 초기화 금지**. `Common/SCGdiplusBitmap.h` 의 `CGdiplusDummyForInitialization` 전역 인스턴스가 자동 처리. `GdiplusStartup`/`GdiplusShutdown` 호출하지 말 것. 대신 빌드에 `SCGdiplusBitmap.cpp` 가 포함되어 있는지 확인.
- **참조 구현**: `D:\1.Projects_C++\Common\CEdit\SCEdit\SCEdit.h` — `CSCColorTheme` 사용 대표 예시.

## 2A. 식별자 네이밍 — snake_case + 타입별 접두사

전역 `CLAUDE.md` 의 snake_case 기본 규칙 위에, MFC 코드 한정으로 다음 규칙을 추가한다.

1. **복합어를 불필요하게 분리하지 않음**: 사용자가 한 단어로 인식하는 조합은 그대로 사용.
   - O: `antialias`, `readonly`, `valign`
   - X: `anti_alias`, `read_only`, `v_align` / `vertical_align`

2. **색상 멤버 변수는 `m_cr_` 접두사** (COLORREF / `Gdiplus::Color` 공통):
   - `m_cr_text`, `m_cr_back`, `m_cr_border_active/inactive`, `m_cr_text_disabled`, `m_cr_back_disabled/readonly`, `m_cr_sel_back`, `m_cr_sel_text`, `m_cr_dim`
   - 함수명은 `_color` 형태로: `set_text_color()`, `set_back_color()`, `set_border_color()` (함수에는 `cr_` 안 씀)
   - 파라미터는 `cr_text`, `cr_back`, `cr_border` 형태

3. **타입별 접두사 (멤버 변수)**:
   - `m_sz_` — `CSize` (`m_sz_action_button`)
   - `m_rect_` — `CRect` (`m_rect_NCbottom`)
   - `m_br_` — `CBrush` (`m_br_back`)
   - `m_lf` — `LOGFONT` (그대로 `m_lf`, NOT `m_logfont`)
   - `m_font` — `CFont`
   - `m_button_` — `CButton` 단일 인스턴스 (`m_button_exit`). 여러 개의 컬렉션이면 복수형 `m_buttons_` (`m_buttons_favorite`).
   - 단순 타입(`int`, `bool`, `CString` 등)은 접두사 없이 snake_case 만. Hungarian `str/n/b` 접두사 일반적으로 제거.

   **약어 — 풀 단어 원칙 + 화이트리스트**: 기본은 풀 단어. `btn` → `button`, `msg` → `message`, `idx` → `index`, `ctx` → `context`, `tmp` → `temp`/의미 있는 단어, `cnt` → `count`. **Why:** 사용자 명시 — *"btn과 같이 줄이는 것은 내 성향이 아니다"* (2026-04-27, SCDeskTools `m_btn_dev_exit` → `m_button_exit`).

   **허용되는 약어 (사용자 화이트리스트, 2026-04-27 정정)**: `cr` (color), `str` (string), `id`, `sz` (size), `br` (brush), `lf` (logfont). 이들은 멤버 접두사·지역변수 어디서든 약어로 써도 된다 (`m_cr_text`, `m_str_name`, `m_id`, `m_sz_action`, `m_br_back`, `m_lf`). 그 외는 풀 단어 사용.

   MFC/Win32 의 `CRect`/`CSize`/`HWND` 같은 클래스·핸들명 자체는 외부 컨벤션이라 그대로 사용.

4. **Boolean 멤버**: `m_is_*` 또는 그냥 서술어 (`m_readonly`, `m_composing`, `m_dynamic`, `m_draw_border`). 맥락에 따라.

5. **MFC 관례 유지 (예외)**:
   - 기반 클래스 override (`Create`, `SetFont`, `SetWindowText`, `GetWindowText`, `DoDataExchange`, `PreSubclassWindow`, `PreTranslateMessage`) — MFC convention 유지
   - **`ON_WM_*` 매크로로 wiring 되는 표준 메시지 핸들러** (`OnPaint`, `OnChar`, `OnKeyDown`, `OnLButtonDown`, `OnSetFocus`, `OnSize` 등) — MFC convention 유지 (매크로가 이름을 강제)
   - 메시지 맵 매크로 (`DECLARE_MESSAGE_MAP`, `BEGIN_MESSAGE_MAP`) 유지
   - MFC override 함수의 **파라미터** 에는 MFC Hungarian 남겨도 됨 (`BOOL bRedraw`, `CFont* pFont` 등) — 혼용 허용

6. **`ON_MESSAGE(WM_XXX, ...)` 로 wiring 하는 커스텀 핸들러는 snake_case** (예: `on_ime_start_composition_message`). `ON_WM_*` 와 구분 기준은 wiring 방식.

7. **사용자 정의 윈도우 메시지 ID — `Message_<Name>` (PascalCase, 대문자 시작)**:
   - `RegisterWindowMessage` 결과를 담는 `static UINT`, 또는 `WM_USER + N` 으로 정의하는 메시지 ID 식별자는 `Message_TaskbarCreated`, `Message_CSCColorPicker`, `Message_CGdiButton`, `Message_CSCSystemButtons` 처럼 **`Message_` 접두사 + 첫글자 대문자 / PascalCase**.
   - **Why**: 이런 식별자는 의미상 `#define WM_XXX` 와 같은 메시지 ID 토큰이지 일반 변수가 아니다. snake_case 변수 (`s_msg_*`) 와 시각적으로 섞이면 메시지 맵·`SendMessage` 호출처에서 토큰 성격이 흐려진다. Common 라이브러리(`Message_CGdiButton`/`Message_CSCStatic`/`Message_CSCSliderCtrl` 등)가 이미 이 컨벤션이라 일관성 위해 모든 프로젝트에서 통일.
   - 적용: `RegisterWindowMessage` 반환값을 받는 식별자, `WM_USER+N` 정의 식별자, `ON_REGISTERED_MESSAGE` / `ON_MESSAGE` 의 첫 인자로 들어가는 메시지 ID 모두.
   - 일반 `#define` 매크로·전역 상수의 SCREAMING_SNAKE_CASE 관례는 그대로 유지 (별개 컨벤션).

8. **사용자 정의 메시지 핸들러 함수 — `on_message_<SourceName>`**:
   - 위 7번의 `Message_<Name>` 메시지를 받아 처리하는 핸들러 함수는 **`on_message_` 접두사 + 메시지 발신원 이름 그대로**.
   - 발신원이 클래스이면 클래스명을 그대로 쓴다 (`C` 접두사·PascalCase 모두 보존):
     - `Message_CSCColorPicker` → `on_message_CSCColorPicker`
     - `Message_CGdiButton` → `on_message_CGdiButton`
     - `Message_CSysTrayIcon` → `on_message_CSysTrayIcon`
   - 발신원이 OS·시스템 등 클래스가 아니면 메시지 ID 의 `Message_` 뒤 부분을 그대로 사용:
     - `Message_TaskbarCreated` → `on_message_TaskbarCreated`
   - **Why**: 메시지 ID 와 핸들러 이름이 시각적으로 1:1 매핑돼 grep 으로 양방향 추적 가능. `on_message_` 접두사가 "사용자 정의 메시지 라우팅" 임을 즉시 나타내 `OnPaint`(MFC `ON_WM_*`) / `on_xxx`(§2A.6 의 일반 `ON_MESSAGE` snake_case) 와 구분.
   - 적용 범위: 본 항목은 **사용자 정의 메시지** (위 7번의 `Message_<Name>`) 에 한정. Win32 표준 시스템 메시지(`WM_HOTKEY`, `WM_CLIPBOARDUPDATE` 등) 의 `ON_MESSAGE` 핸들러는 §2A.6 (snake_case) 적용.

**일반 원칙**: snake_case 는 변수·함수의 기본. `#define` 매크로 / 메시지 ID / 의미상 "토큰" 인 식별자는 시각적 구분을 위해 대문자 시작 또는 ALL_CAPS 사용 — Win32/MFC 와 사용자 코드의 경계에서 자연스럽게 섞일 수 있도록.

## 2D. 그리기/레이아웃 코딩 스타일 (MFC + GDI+)

이 사용자가 커스텀 컨트롤 그리기 코드를 직접 작성할 때 일관되게 따르는 스타일.

1. **`_rect` vs `_area` 네이밍 구분**
   - `_rect` = 실제로 그려진 결과의 정확한 bounding box (예: `CSCStatic::get_text_rect()` — `m_rect_text` 저장값 반환, `get_compose_draw_box`)
   - `_area` = 더 넓은 영역 — 그려질 수 있는 공간이거나 클릭 판정용 (예: `CSCStaticEdit::get_text_area()` (=패딩/보더 제외한 텍스트 그릴 공간), `get_copy_button_area`)
   - 같은 컨트롤에서 "그려질 수 있는 공간" 과 "실제 그려진 bbox" 가 다르면 두 함수로 분리. 헷갈리면 area 가 기본.

2. **레이아웃 숫자는 inline 분해 식으로**
   - O: `btn.left = btn.right - 6 - height - 5 - 4;` (각 숫자가 right margin / icon size / offset 예약 / gap 한 가지 의미)
   - X: `btn.left = btn.right - BUTTON_AREA_WIDTH;` (단일 상수로 묶어버림)
   - **Why:** 숫자가 무엇을 뜻하는지 식 자체가 설명. 한 부분만 미세조정할 때 분해된 숫자만 만지면 됨.

3. **CRect 는 새 객체 생성보다 필드 수정 우선**
   - O:
     ```cpp
     CRect btn = rc;
     btn.left = btn.right - 16;
     ```
   - X: `CRect btn(rc.right - 16, rc.top, rc.right, rc.bottom);`

4. **선언 블록과 사용 블록 사이 빈 줄**
   ```cpp
   CRect rc;
   GetClientRect(rc);

   CRect btn_area = rc;
   btn_area.left = ...;
   ```

5. **클릭 피드백은 추가 배경 대신 색 swap**
   - 카피 버튼처럼 작은 인디케이터: pressed 시 별도 배경 라운드 사각형 그리지 말고 기존 stroke/fill 색을 swap.
   - 예: 평소 `(stroke=DimGray, fill=cr_back)` ↔ 눌림 `(stroke=RoyalBlue, fill=Moccasin)`.
   - **Why:** 그려야 할 픽셀 수 증가 없이도 시각 변화 명확. 추가 레이어 없으면 잔상/플리커도 적음.

6. **그리기 헬퍼 우선** — `get_round_rect_path + FillPath/DrawPath` 직접 조합 대신 `Functions.h` 의 `draw_round_rect()` 사용. fill only 면 `width=0`, stroke only 면 `cr_fill = Color(0,0,0,0)` 또는 `Color::Transparent`.

7. **컬럼 정렬은 space 가 아니라 tab — 강제** (사용자 명시 방침, 2026-04-27)
   - 배열 초기화 (`{ A, B, C }` 의 컬럼), 연속 선언 (`int  a = 1;` 의 `=` 정렬), 멤버 정렬 (`bmi.biSize        = ...`), 코드 뒤 주석 정렬 (`code    // comment`) 등에서 **공백 2개 이상으로 정렬하는 패턴 금지**. 모두 tab 으로.
   - **Why:** tab 폭은 사용자 환경 설정에 따라 적응되지만 공백 정렬은 글자수가 다른 한글·CJK 가 끼면 다이폴트 폰트 폭이 달라 자주 깨진다. tab 으로 통일하면 어디서 보든 같은 컬럼에 떨어진다. 본인이 직접 정렬할 때도 tab 만 사용함.
   - 예외: 단어 사이 일반 공백 1 개, 들여쓰기 (이미 leading tab 사용 중), 문자열 리터럴·주석 내부의 공백.
   - 새 코드 작성 시: 컬럼을 맞추고 싶으면 처음부터 tab 으로. 기존에 공백 정렬된 코드를 발견하면 tab 으로 일괄 치환 (자동 변환 스크립트 가능 — 코드 영역의 2+ 공백 → 단일 tab, 단 문자열·char 리터럴·`//` 주석 내부는 보존).

## 2C. 한 줄에 한 문장 (강제)

여러 문장을 같은 줄에 이어 쓰지 말 것. 한 문장(statement) = 한 줄.

- X: `CRect rc;       GetClientRect(rc);`
- X: `if (!m_use_copy_button) return CRect();`
- X: `int a = 1; int b = 2;`
- O:
  ```cpp
  CRect rc;
  GetClientRect(rc);
  ```
- O:
  ```cpp
  if (!m_use_copy_button)
      return CRect();
  ```

**Why:** diff 가독성·debugger step·grep 결과 정확도. 한 줄에 두 문장이 있으면 한 문장만 수정해도 diff 에 두 문장이 묶여 노이즈가 되고, breakpoint/step 도 한 줄 단위라 의도한 위치에서 멈추기 어렵다.

**예외**:
- 멤버 이니셜라이저 리스트 한 줄 (`Foo() : a(1), b(2) {}`) 처럼 문법상 한 문장.
- `for (int i = 0; i < n; ++i)` 의 init/cond/incr 은 같은 문장의 절(clause).
- 매크로 본문은 적용 대상 아님.

### 2C.0 사용자 명시 요구사항에 "방어 로직" 추가 금지 (강제)

사용자가 파라미터·동작을 **명시적으로 정의**하면 그 정의 그대로 구현한다. "안전을 위해", "범위 초과 방지를 위해" 같은 *추가 제약* 을 임의로 끼워넣지 말 것.

**X — 절대 금지:**
- 사용자: "size=20 이면 무조건 20px 로 그려라" → 구현: `min(size, area_height)` 로 cap (사용자가 안 시킨 cap 추가)
- 사용자: "이 함수는 음수 받아서 절대값 처리해라" → 구현: 음수면 0 으로 clamp (시키지 않은 clamp)
- 사용자: "max 길이 10000 까지 받아라" → 구현: 1000 으로 내부 cap (시키지 않은 보수화)

**Why this rule:** 2026-04-28, `CSCStaticEdit::set_prefix_image(id, size)` 구현 시 사용자가 *"size 가 그려질 height, edit height 와 무관하게 무조건 그 값"* 으로 명시했음에도 `min(size, area_height)` cap 을 추가. 결과적으로 ROUND/READONLY 두 edit 가 같은 size=20 을 줘도 area height 차이 때문에 18 vs 10 으로 다르게 그려져 사용자가 분노 — *"이봐. 장난하나? 이제와서 여백이 틀리다느니, 두 edit 의 높이가 틀리다느니 이딴 얘기를 해?"*. 명시 요구사항을 어긴 채로 원인을 PNG/edit 차이로 돌리려 한 시도가 분노 강도를 더했음.

**적용:**
1. 사용자가 동작을 명시한 부분은 *그 표현 그대로* 구현. 임의로 `min/max/clamp/null-check/early-return` 추가 금지.
2. 명시된 동작이 위험해 보이면 (예: 무한 루프 가능성, 메모리 폭발) 구현 *전에* 사용자에게 질문. 임의로 절충 금지.
3. "이렇게 하면 X 케이스에서 문제 될 수 있다" 같은 우려는 **구현 후 보고**가 아니라 **구현 전 질문** 으로 해결. 구현에 끼워넣지 않음.
4. 전역 §1 ("불필요한 구문 금지") 의 강화 버전 — 거기서는 "필요 없는 구문 금지" 였다면 여기서는 "**사용자가 안 시킨** 구문 금지".

### 2C.1 switch case 본문 — 한 줄 압축 금지 (강제)

`case` label 과 본문·`break` 를 같은 줄에 몰아 쓰지 말 것. label / 본문 statement / `break` 는 각각 별개의 줄에.

- X:
  ```cpp
  switch (m_action_type)
  {
      case action_copy:             draw_action_icon_copy           (g, rc_btn); break;
      case action_clear:            draw_action_icon_clear          (g, rc_btn); break;
      case action_find:             draw_action_icon_find           (g, rc_btn); break;
      case action_password_toggle:  draw_action_icon_password_toggle(g, rc_btn); break;
      default: break;
  }
  ```
- O:
  ```cpp
  switch (m_action_type)
  {
      case action_copy:
          draw_action_icon_copy(g, rc_btn);
          break;
      case action_clear:
          draw_action_icon_clear(g, rc_btn);
          break;
      case action_find:
          draw_action_icon_find(g, rc_btn);
          break;
      case action_password_toggle:
          draw_action_icon_password_toggle(g, rc_btn);
          break;
      default:
          break;
  }
  ```

**Why:** §2C 의 동일한 근거 (diff 가독성·breakpoint 단위·grep 정확도) 가 case 본문에도 그대로 적용된다. 함수명을 컬럼 정렬해 dispatch 표처럼 보이게 만드는 스타일도 결국 한 줄에 statement 2 개 + label 이라 §2C 위반. 사용자 명시 — *"앞으로는 이와같이 정석적으로 작성하라"* (2026-04-28, `CSCStaticEdit::draw_action_button` 의 dispatch switch).

## 2F. 주석 — "무엇" 설명 금지, "왜" 만 작성 (강제)

기본은 **주석 없음**. 함수명·변수명·시그니처가 이미 그 코드가 무엇을 하는지 충분히 설명하므로, 그걸 다시 한국어로 풀어 쓰는 주석은 노이즈일 뿐이다.

**X — 절대 작성 금지:**
- "함수 X 가 SmoothingMode 자동 설정 + LineCap 인자 처리." (호출하는 함수가 무엇을 하는지 설명)
- "버튼을 만든다.", "이미지를 그린다.", "결과를 반환한다." (시그니처로 자명한 동작)
- "직전 폴더 읽기", "기본 파일명 생성" (코드 한 줄 위에 그대로 붙는 동어반복)
- "Common 의 X 를 사용한다." (호출문이 이미 그렇게 되어 있음)
- 현재 작업·PR 컨텍스트 (`// 사용자가 요청한 X 추가`, `// 위 §3 항목 처리`) — 코드가 merge 되는 순간 의미 상실

**O — 작성할 가치가 있는 주석:**
- **Why 가 비자명**: "왜 이 우회로를 썼나" — API 한계, 컴파일러 버그, MFC 내부 동작, 성능/메모리 트레이드오프
- **숨은 제약·주의사항**: "MOD_NOREPEAT 안 붙이면 키 누르고 있을 때 캡처가 겹쳐 뜸", "이 시점에 ShowWindow 호출 시 dialog manager 가 무시함"
- **겉보기와 다른 동작**: "레지스트리 키가 클래스명과 다름 — 과거 호환성 때문에", "SetCapture 중에는 WM_SETCURSOR 안 옴"
- **TODO/FIXME**: 알려진 한계나 후속 작업 지점

**판정 기준 (작성 직전 자문):**
> "코드만 보고도 알 수 있는 내용인가?" → 그렇다 = **금지**.
> "왜 이 코드가 이렇게 되어 있는지 6개월 뒤 내가 다시 봐도 이해될까?" → 안 될 것 같다 = **써라**.

**Why this rule:** 사용자 명시 — *"이러한 주석문은 이미 내가 다 인지하고 있는, 다른사람이 봐도 굳이 도움안되는 주석문이다. 이런 불필요한 주석문은 추가하지마라"* (2026-04-27, SCCapturedNoteDlg.cpp 의 `//Common draw_line 이 SmoothingMode 자동 설정 + LineCap 인자 처리.`). 같은 피드백이 전역 `~/.claude/CLAUDE.md` §1·§2 에도 있으나 반복돼 강화.

## 2G. 함수 호출 인자 — 기본값과 같으면 명시 금지 (강제)

함수 호출 시 **기본값(default argument) 과 같은 값을 명시적으로 넘기지 말 것**. 시그니처에 이미 같은 값이 default 로 설정되어 있다면 그 자리는 비워서 default 가 적용되게 둔다.

**X — 절대 작성 금지:**
```cpp
draw_text(d2dc, rc, text, font, size, weight,
    cr_text, cr_shadow, cr_stroke, stroke_width,
    DT_LEFT | DT_VCENTER,
    true,  //show_text   ← default 가 true 이고 인자명 주석도 동어반복
    true); //show_shadow ← default 가 true
```

**O — 디폴트 그대로 두기:**
```cpp
draw_text(d2dc, rc, text, font, size, weight,
    cr_text, cr_shadow, cr_stroke, stroke_width,
    DT_LEFT | DT_VCENTER);
```

**예외 (명시할 가치가 있는 경우):**
- default 와 *다른* 값을 넘길 때 — 당연히 명시
- 디폴트 변경 위험을 명시적으로 차단해야 하는 critical path (드뭄, 정말 드뭄)
- 자신 다음 위치의 인자에 non-default 를 넘기기 위해 어쩔 수 없이 채우는 경우 (C++ 의 위치 인자 한계 — *그래도 그 자리에는 default 와 같은 값이 가야 한다면 코멘트로 표시*)

**판정 기준 (호출 작성 직전 자문):**
> "이 자리의 default 는 무엇이고, 내가 넘기는 값과 같은가?" → 같다 = **생략**.
> "다음 자리에 non-default 를 넘기려고 어쩔 수 없이 채우나?" → 그렇다 = 어쩔 수 없음, 명시.

**Why:** default 와 동일값을 명시하는 건 (a) 정보 0, (b) 시그니처 변경 시 두 군데 동기화 부담, (c) 호출 라인 길이만 늘려 가독성 ↓. 사용자 명시 — *"파라미터 기본값이 원하는 설정값과 같을 경우는 굳이 명시하지 말자"* (2026-04-27, `SCRegionCaptureDlg::on_overlay_paint` 의 `draw_text(... true, true);`).

**전역 §1 ("불필요한 구문 금지") 의 함수 호출 버전.** §2F 의 주석 룰과 같이 묶어서 기억: *"코드에 더 들어가는 것은 의미를 추가할 때만"*.

## 2H. 람다·헬퍼 추출 — 의미 추가 없는 한 줄 wrapper 금지 (강제)

함수 1 회 호출을 그대로 감싸는 람다·헬퍼는 **만들지 말 것**. 인자가 반복된다고 해서 자동으로 람다로 뺄 이유가 되지 않는다.

**X — 절대 작성 금지:**
```cpp
auto label = [&](CRect rt, CString s, UINT align)
{
    draw_text(d2dc, rt, s,
        _T("Segoe UI"), font_size, DWRITE_FONT_WEIGHT_SEMI_BOLD,
        cr_label_text, cr_label_outline, cr_label_shadow, Gdiplus::Color::Transparent,
        1.0f, align);
};
label(rect1, text1, DT_LEFT | DT_BOTTOM);
label(rect2, text2, DT_CENTER | DT_TOP);
//... 람다는 단지 `draw_text(...)` 의 별명. 추상화 가치 0.
```

**O — 직접 호출:**
```cpp
draw_text(d2dc, rect1, text1,
    _T("Segoe UI"), font_size, DWRITE_FONT_WEIGHT_SEMI_BOLD,
    cr_label_text, cr_label_outline, cr_label_shadow, Gdiplus::Color::Transparent,
    1.0f, DT_LEFT | DT_BOTTOM);
draw_text(d2dc, rect2, text2,
    _T("Segoe UI"), font_size, DWRITE_FONT_WEIGHT_SEMI_BOLD,
    cr_label_text, cr_label_outline, cr_label_shadow, Gdiplus::Color::Transparent,
    1.0f, DT_CENTER | DT_TOP);
//인자 텍스트가 반복되어도 OK — 의미 추가가 없는데 추상화로 감싸면 간접성만 늘어남.
```

**판정 기준 — 람다·헬퍼 만들기 전 자문:**
> "이 추출은 어떤 *의미·결정·알고리즘* 을 한 곳에 모으나?"
> - 답이 "그냥 인자가 반복돼서" 면 → **만들지 말 것**.
> - 답이 "복잡한 측정·정렬·flip 같은 *로직* 이 묶임" 이면 → 가치 있음.

**핵심 구분:**
- 알고리즘·결정·여러 단계 로직이 묶이면 추출은 가치 있음 (예: 이전의 `draw_label` 이 measure→positioning→flip 다 처리하던 시점).
- 단순 인자 forwarding wrapper 는 가치 0. 호출 한 번을 호출 한 번으로 alias 하는 것일 뿐.

**Why this rule:** 사용자 명시 — *"한줄짜리 호출하는 람다함수를 왜 만든것인지?"* (2026-04-27, `SCRegionCaptureDlg::on_overlay_paint` 의 `label` 람다). 같은 패턴이 한 세션에서 *진화* 했다 — 처음엔 의미 있던 헬퍼(`draw_label`: measure+pos+flip) 가 단순화되며 wrapper 로 전락했는데도 그대로 두었음. **추출은 한 번 만들었다고 영원하지 않다 — 본문이 단순화되면 추출도 해체해야 한다.**

**정리 트리거 (코드 단순화될 때 항상 같이 검토):**
- helper 본문이 1-2 줄로 줄어듦 → 인라인화 검토
- helper 호출처가 1-2 곳뿐 → 인라인화 검토
- helper 가 외부 함수에 인자 forwarding 만 함 → 즉시 인라인화

## 2B. 파일 인코딩 — VS 2026 디폴트 따르기

**프로젝트 디폴트** (VS 2026 새 MFC 프로젝트 생성 시 자동 적용되는 형식 그대로):
- `.cpp`, `.h`, `.cc`, `.cxx` → **UTF-8 with BOM** (`EF BB BF` 선두 3바이트)
- `.rc`, `.rc2` → **UTF-16 LE** (BOM `FF FE` + LE 인코딩, VS 의 리소스 편집기가 관리)

**Why VS 2026 디폴트를 따르는가:** VS 자체가 그 인코딩으로 파일을 작성·해석한다. 임의로 다른 형식 (BOM 제거, UTF-16 ↔ UTF-8 변환 등) 으로 바꾸면 VS 가 다음 열기 시 "이 파일을 다른 인코딩으로 변환할까요?" 다이얼로그를 띄움 → 사용자가 "모두 변경" 클릭하면 *모든 열린 파일* 이 mass-convert 되며 BOM 일괄 유실·MSVC 컴파일 에러 줄줄이 발생 (실제 사례 2026-04-27, SCDeskTools 다수 파일 동시 깨짐).

**Why MSVC 가 BOM 을 요구하나:** MSVC 는 BOM 없는 파일을 시스템 코드페이지(한국어 Windows = CP949) 로 해석. UTF-8 한글 주석/문자열이 CP949 로 잘못 해석되면 클래스 선언 중간에 파싱이 끊겨 이후 모든 멤버가 `'m_xxx': 선언되지 않은 식별자` 에러로 출력됨 (SCStaticEditDemo / SCStaticEdit.h 90+ 가짜 에러, 2026-04-21).

**How to apply:**
- ~~`Edit` 는 BOM 보존됨~~ **(틀림 — 2026-04-28 반증)**: `SCStaticEdit.cpp` (한 세션 내 다수 Edit 후 BOM 유실 + CP949 재인코딩) / `Test_CSCStaticEditDlg.cpp` (단발 Edit 후 BOM 만 유실, UTF-8 유지) 양쪽 모두 발생. 환경/파일 크기/Edit 횟수 어떤 조건에서 BOM 이 떨어지는지 단정할 수 없으므로 **모든 `.cpp/.h` Edit/Write 직후 BOM 검사 강제**.
- **BOM 검사 의무 절차 (Edit/Write 직후 매번):**
  ```bash
  file <edited_file>
  # "UTF-8 (with BOM)" 가 아니면 즉시 BOM 부착 (또는 CP949 라면 변환):
  #  - UTF-8 no-BOM: printf '\xef\xbb\xbf' > f.new && cat f >> f.new && mv f.new f
  #  - CP949: iconv -f CP949 -t UTF-8 f > f.utf8 && printf '\xef\xbb\xbf' > f.new && cat f.utf8 >> f.new && mv f.new f && rm f.utf8
  ```
- 한 응답에서 같은 파일을 여러 번 Edit 하면 마지막 Edit 후 한 번만 검사해도 충분.
- `Write` 로 새 파일 생성·완전 교체 시 BOM 누락 위험 — 작성 후 BOM 부착 확인.
- BOM 부착: `printf '\xef\xbb\xbf' > f.new && cat f >> f.new && mv f.new f`
- CP949 → UTF-8(BOM) 변환: `iconv -f CP949 -t UTF-8 f > f.new && printf '\xef\xbb\xbf' > f.final && cat f.new >> f.final && mv f.final f && rm f.new`
- 의심 증상: 분명히 선언된 변수가 "선언되지 않은 식별자" 로 수십 개 이상 → BOM 먼저 의심.
- BOM 누락 / 다른 인코딩 발견 시 **묻지 말고 VS 2026 디폴트 (UTF-8 BOM) 로 강제 변환** (사용자 명시 방침, 2026-04-21).
- 인코딩 판별: `file <path>` 결과가 "UTF-8 (with BOM)" (또는 `.rc`/`.rc2` 의 경우 "UTF-16, little-endian") 외이면 변환 대상.
- `.rc`/`.rc2` 는 **건드리지 말 것** — VS 의 리소스 편집기가 UTF-16 LE 로 자동 관리. Claude 가 직접 Edit 하지 않음 (할 필요도 거의 없음).

**VS 다이얼로그 대응 가이드:**

VS 가 띄우는 다이얼로그는 두 종류로 나눠 판단한다.

1. **"파일이 외부에서 수정됨, 다시 불러올까요? (Reload from disk)"**
   - → 보통 **Yes 안전**. 디스크 내용을 VS 화면에 반영할 뿐 파일을 *변경하지* 않는다.
   - 미저장 편집이 있다면 잃을 수 있으니 그때만 주의.
   - reload 후 IntelliSense 가 새로 파싱되면서 **잠복했던 결함이 한꺼번에 가시화**될 수 있음 (예: base 클래스에 `DECLARE_DYNAMIC` 누락 등). 인코딩 이슈가 아니라 진짜 코드 결함이니 따로 고침 (2026-04-27 사례, SCDeskTools `CSCFrozenOverlayDlg`).

2. **"한글 등 유니코드를 보존하기 위해 인코딩을 변경할까요? (Save with encoding ...)"**
   - 대상이 명시적으로 **"UTF-8 with signature/BOM"** 이면 Yes 가능 (VS 2026 디폴트와 일치).
   - 대상이 **"UTF-8 (without signature)" 이거나 모호** 하면 **No**. BOM 일괄 유실 → MSVC 가짜 에러 폭주.
   - **"모두 변경 (Apply to all)" 류는 거의 항상 No**. 한 파일에 옳더라도 다른 파일엔 틀릴 수 있어 일괄 적용 위험. 정상이던 파일 BOM 도 같이 떨어짐 (2026-04-27 SCDeskTools 다수 파일 동시 깨짐 실제 사례).
   - 개별 파일만 변경이 필요하면 VS 안에서 처리 말고 외부에서 (위의 iconv 레시피로) BOM 부착 후 reload — 이 경로는 다이얼로그 자체가 안 뜬다.
   - 사용자가 실수로 "모두 변경" 을 클릭한 흔적 (다수 파일 BOM 동시 유실) 을 발견하면, 위 audit 후 VS 2026 디폴트로 일괄 복구.

## 2B.1 세션 시작 시 인코딩 audit — Claude 의 U+FFFD 손상 방지

Claude Read 툴은 파일 바이트를 UTF-8 로 해석한다. **BOM 없는 CP949 `.cpp/.h`** 를 Read 하면 한글 바이트가 UTF-8 로 유효하지 않아 내부적으로 U+FFFD 로 치환되고, 이후 Edit/Write 가 그 U+FFFD 를 UTF-8 바이트로 직렬화해 디스크에 저장 — **한글 영구 손실**. HEAD 커밋은 intact 이라 `git show HEAD:<f> | iconv -f CP949 -t UTF-8` 로 복원 가능하지만 세션 중 작업분은 복원 불가. `D:\1.projects_c++\Common` 은 이미 UTF-8 BOM 으로 통일돼 안전하고, `D:\1.projects_c++\1.test\Test_StaticEx` / `D:\1.projects_c++\SCDeskTools` 도 2026-04-25 ~ 2026-04-27 마이그레이션 완료. **그 외 프로젝트는 아직 CP949 no-BOM 이 남아있을 가능성.**

**적용 — `D:\1.projects_c++\<project>` 에서 `.cpp/.h` 를 첫 Read/Edit 하기 전에 반드시 (Edit 가 아니라 Read 도 금지 — Read 자체가 손상의 첫 단계):**

```bash
cd <project_root>
for f in $(git ls-files '*.cpp' '*.h'); do
  file "$f" | grep -vE "UTF-8 \(with BOM\)|ASCII text"
done
```

non-empty 결과가 있으면 **편집 시작 전에** 사용자에게 통지:

> "이 프로젝트의 다음 파일이 UTF-8 BOM 이 아닙니다: [목록]. Claude 가 편집하면 한글이 U+FFFD 로 깨집니다 (Test_StaticExDlg.cpp 2026-04-24 사례). `Common` / `Test_StaticEx` 와 동일하게 UTF-8 BOM 으로 마이그레이션할까요?"

사용자 승인 시 §2B 의 "CP949 → UTF-8(BOM) 변환" 레시피로 배치 변환 후 편집 시작. **승인 전까지는 해당 파일의 Edit/Write 금지** — 한글이 있을 가능성이 있는 파일을 Read→Edit 하는 순간 손상 발생.

**왜 ASCII/UTF-8(BOM) 만 filter 에서 제외하나**: ASCII 는 non-ASCII 바이트가 없어 Read→Edit 가 손상 유발 안 함 (§2B 일관성 규칙상 BOM 부착은 권장하되 Claude 손상 위험은 없음). `UTF-16` 은 `.rc`/`resource.h` 로 VS 가 관리하므로 Claude 가 일반적으로 Edit 하지 않음 — 별도 audit 경로로 처리.

**참조 사례:**
- 2026-04-21 `SCStaticEdit.h` BOM 유실 → MSVC 가짜 에러 90+.
- 2026-04-24 `Test_StaticExDlg.cpp` 이전 Claude 세션에서 Read→Edit 으로 한글 U+FFFD 손상. HEAD 로부터 iconv 복원 + UTF-8 BOM 마이그레이션 (2026-04-25).

## 2E. `PreTranslateMessage` 반환값 — 클래스 역할에 따라 다름

MFC 다이얼로그/컨트롤의 `PreTranslateMessage(MSG* pMsg)` 반환 의미:
- `TRUE` = 메시지 소비, dispatch 중단 (자식 컨트롤 키 못 받음)
- `FALSE` = 정상 dispatch 진행 (단, **base 의 `IsDialogMessage` 우회**)
- `BaseClass::PreTranslateMessage(pMsg)` 위임 = base 가 IsDialogMessage 로 Tab/Enter/Esc 네비게이션 처리

**Common 라이브러리의 일관 규칙** (감사 결과, 2026-04-25):

1. **Top-level 다이얼로그** (앱의 메인 다이얼로그, ex. `CMiniClock2Dlg`, `CTimeListDlg`):
   - 자체 처리할 키 외엔 `return CDialogEx::PreTranslateMessage(pMsg);`
   - Tab/Enter/Esc 등 표준 dialog 네비게이션이 정상 동작해야 함.

2. **Composable child dialog** (다른 폼에 끼워 쓰는 위젯형 다이얼로그):
   - 키 처리에서 `return FALSE;` 로 base IsDialogMessage 우회.
   - 예: `CSCShapeDlg` (overlay), `CSCImageDlg`, `CSCD2ImageDlg`, `CSCHeatmapCtrl`.
   - 이유: 자체 OnOK/OnCancel 자동발화가 parent 의 닫기 흐름을 깨뜨리거나, Tab 이 parent 의 포커스 사이클을 침범하면 안 되므로 의도적으로 차단.
   - 필요시 `::SendMessage(m_parent->m_hWnd, WM_KEYDOWN, ...)` 로 명시적 전달 후 `return FALSE;`.

3. **컨트롤 파생** (`CButton`/`CEdit`/`CWnd` 등):
   - base 의 PreTranslate 가 사실상 no-op 이라 `return FALSE;` 와 base 호출이 동등.
   - 일관성 위해 base 호출 권장이나 `return FALSE;` 도 무해.

**감사 시 의심 우선순위**: Top-level 다이얼로그(§1) 인데 `return FALSE;` 가 default exit 인 경우만 진짜 버그. Composable child dialog(§2) 의 `return FALSE;` 는 패턴이지 버그 아님 — 클래스 사용 맥락 (m_parent 보유 여부, 모달 여부, 다른 다이얼로그에 끼워 쓰는지) 을 먼저 확인.

## 3. 분석 요청 시 탐색 깊이 자제

"분석해달라" 요청이 오면 진입점(main dialog, 호출된 핸들러) 만 확인하고 보조 파일까지 재귀적으로 펼치지 말 것. 필요 최소한 깊이만 확인 후 답하고, 사용자가 더 깊이 가자고 하면 그때 확장.

**단 §2 와 혼동 금지**: 실제 사용 중인 Common 라이브러리 확인은 "과도한 탐색" 이 아니다. 호출된 메서드의 실제 구현이 외부에 있다면 그건 **진입점의 일부**다.

## 3A. Common 모듈 인벤토리

세부 모듈 지도는 `Common/architecture.md` 에 있다. ASee 등 프로젝트에서 `m_xxx.foo()` 타입을 처음 만나 헤더로 추적하기 전에, 먼저 이 파일의 인덱스를 훑어 어떤 카테고리·용도인지 확인하면 탐색 깊이를 줄일 수 있다. 내용이 바뀌면 동기화.

## 4. 컨텍스트 저장 위치

새로 알게 된 규칙·결정사항은 다음 위치에 저장한다:

- **모든 프로젝트 공통 규칙**: 이 파일 (`Common/claude.md`) 에 추가 → Common repo 로 모든 프로젝트에 자동 반영
- **특정 프로젝트 한정**: 해당 프로젝트의 `claude.md` 에 추가 → 해당 프로젝트 repo 에 커밋
- 홈 폴더 auto-memory (`~/.claude/projects/.../memory/`) 는 git 동기화가 안 되므로 **기본 저장소가 아님**. 프로젝트·Common 쪽 파일을 우선한다.

## 5. 프로젝트 구조 전제

두 작업 장비(집·회사) 에서 프로젝트 루트 절대 경로가 동일해야 한다. 현재 기준:
- 프로젝트 루트: `D:\1.projects_c++\<ProjectName>`
- Common: `D:\1.projects_c++\Common`
- 테스트 프로젝트: `D:\1.projects_c++\1.test\<TestProjectName>`

## 6. 요청 실행 — 재확인 금지

사용자가 구체적인 변경·구현·파일 수정을 명시적으로 요청하면 **재차 확인하지 말고 바로 진행**한다.

- "X 기능 추가해달라", "Y 수정해달라", "Z 진행하자" 같은 명확한 지시는 이미 승인된 작업이다. "진행할까요?" "이렇게 하면 될까요?" 같은 확인 질문을 다시 하지 말 것.
- 실제로 멈춰야 할 때: ① 요청이 모호해서 2가지 이상 해석이 동등하게 가능할 때, ② 되돌리기 어려운 파괴적 조작(force push to main, 대량 삭제, 공유 인프라 변경 등), ③ 사용자가 언급하지 않은 범위 밖 변경을 추가로 제안할 때.
- 작업 중간에 보고용 질문 금지. 완료 후 간단히 결과만 전달.
- 세부 구현 판단(함수 분할 방식, 내부 변수명 등)은 사용자 규칙 위반이 아닌 한 알아서 결정.

**Why**: 2026-04-22, `set_text_align` 구현 중 이미 명확히 요청된 작업에 대해 중간중간 "이렇게 진행하면 될까요?" 를 반복해 사용자가 피드백 — *"분명히 변경을 요청한건인데 왜 자꾸 할지말것인가 같은건가? 알아서 진행하라. 더 이상 묻지말것."*

## 7. CFileDialog — recent_folder 패턴 필수 (사용자 철칙)

모든 파일 저장/열기 대화상자는 **반드시** 직전에 사용한 폴더를 기본 위치로 열어야 한다. 매번 처음부터 폴더를 탐색하게 만들면 안 됨.

**적용 패턴** (read → use → write-back):

```cpp
//1) 직전 폴더 읽기 (없으면 적절한 default — 보통 exe 디렉토리)
CString recent_folder = AfxGetApp()->GetProfileString(
    _T("settings"), _T("recent_folder"), get_exe_directory());

//2) 기본 파일명에 폴더 포함 → CFileDialog 가 그 폴더로 열림
CString default_name;
default_name.Format(_T("%s\\capture_%04d%02d%02d_%02d%02d%02d.png"),
    recent_folder, st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond);

CFileDialog dlg(FALSE, _T("png"), default_name,
    OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT, filter, this);
if (dlg.DoModal() != IDOK)
    return;

//3) 작업 (저장/로드) 성공 후 폴더 갱신
CString path = dlg.GetPathName();
//... save / load ...
AfxGetApp()->WriteProfileString(_T("settings"), _T("recent_folder"),
    get_part(path, fn_folder));
```

**Why**: 사용자가 특정 작업 폴더 (예: `D:\Captures\2026`) 에 저장하는 습관이 있을 때, 다음 저장 시 또 거기서 시작해야 사용성 보장. 매번 root 부터 폴더 이동 강요는 사용성 저하. 2026-04-27, SCDeskTools 노트 저장 메뉴 구현 중 사용자가 강조 — *"이는 나의 철칙이므로 부득이한 경우가 아니면 반드시 이렇게 저장해달라."*

**How to apply**:
- 새 `CFileDialog` (저장 / 열기 / 폴더 선택 등) 추가 시 — 위 3 단계 (read / use as default / write-back on success) 모두 포함. 누락 금지.
- profile 키는 일관되게 — 같은 종류 작업은 같은 키 (`settings\\recent_folder`) 공유, 작업 종류가 명확히 다르면 별도 키 (예: `settings\\recent_save_folder` vs `settings\\recent_open_folder`).
- default fallback 은 보통 `get_exe_directory()`. 다른 적절한 default 가 있으면 그걸로.
- 부득이한 예외 (보안 정책 / 시스템 파일 등) 외에는 반드시 적용.
- 프로젝트 마다 이미 사용 중인 profile 키 컨벤션이 있다면 먼저 grep 후 따를 것.
- Common 의 `get_part(path, fn_folder)` 가 path 에서 폴더 부분만 추출 (Functions.h).
