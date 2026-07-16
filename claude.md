# Common 규칙 (모든 C++ 프로젝트 공용)

이 문서는 `D:\1.Projects_C++\Common` repo 에 속한다. 모든 프로젝트의 `claude.md` 가 `@../Common/claude.md` (또는 하위 경로에서는 `@../../Common/claude.md`) 로 import 하여 공통 규칙으로 적용한다.

Common repo 자체도 github.com/scpark98/Common 으로 집·회사 간 git 동기화된다.

---

## 1. 파일 네이밍 — GNU 스타일

새로 생성하는 파일명은 **소문자 + 언더스코어** 를 기본으로 한다.

- O: `claude.md`, `settings.local.json`, `build_notes.md`, `zoom_fix.patch`
- X: `CLAUDE.md`, `BuildNotes.md`, `zoomFix.patch`

예외: 툴·언어 생태계 관례로 고정된 파일명(`CMakeLists.txt`, `Dockerfile` 등)과 repo 에 이미 존재하는 기존 파일명은 그대로 따른다.

## 1A. 미디어 파일명·개인 경로 — 외부 push 가는 기록에 남기지 말 것 (강제, 최우선)

소스 코드 / commit message / commit diff / README / 주석 / TODO — **외부 push / repo 에 들어가는 기록** 에 사용자의 *실제 미디어 파일명* 이나 *개인 경로* 를 기록하지 말 것.

**Log 파일은 예외 (2026-05-29 사용자 정정):** 개발 단계 전용. 배포 안 됨. `.gitignore` 로 repo 에서도 제외. `logWrite`/`gLog` literal 에 실제 path 박혀도 OK. 사용자 명시 — *"로그파일에 미디어명 노출도 관계없다. 어짜피 최종적으로 로그파일들은 배포될일 없고 말그대로 개발단계에서만 생성될것이다. 내가 미디어명 노출을 금지한것은 commit message에서다."*

**X — 절대 금지 (외부 push 가능 기록):**
- `open_media(_T("E:\\<폴더>\\<실제 작품 제목>.mkv"), ...)` 같은 hardcoded 테스트 path (소스 코드).
- commit message subject / body 에 `"<실제 미디어 제목> 한일자막 통합"` 같이 작품명 포함.
- README / claude.md / TODO 의 작업 항목에 검증용 미디어를 *실제 제목* 으로 기록.
- 주석 안 (`//test with E:\...`) 도 금지.

**O — 허용:**
- 진단 로그 (`logWrite`, `gLog`) literal 에 실제 path / 작품명 OK — log 파일은 배포 / push 안 감.
- 테스트 미디어 path 는 *registry 옵션* / *환경변수* / *런타임 인자* 로 받기 (소스 hardcoded 금지). 코드엔 placeholder 또는 변수 reference 만.
- commit / README / TODO 작업 항목은 *동작·기능 카테고리* 만 (예: "다국어 자막 멀티 클래스 머지" — *어느 작품으로 검증* 했는지는 안 씀).
- `sample.mp4`, `test.mkv` 같은 일반 placeholder 는 source 에 OK.

**Why (2026-05-23 Endorphin2 사고):** repo 가 public 인 동안 5 개 commit (`ac72082` 부터 `d3196e9` 의 삭제 직전까지) 에 사용자의 실제 작품명 + 개인 경로 (`E:\Japanese\<작품명>\...`, `C:\Users\<username>\내 드라이브\...`, `E:\temp\<일자>\<작품ID> <제목>.mp4`) 가 박혀 push 됐다. `git filter-repo` 로 전체 history redact + force push + repo private 전환 + GitHub Support cache eviction 절차가 필요했음. 다행히 fork 0 이라 외부 mirror 화는 면했지만, fork 가 1 건이라도 있었다면 영구 노출. **commit 한 번 push = 외부 clone·검색엔진 캐시·archive (Software Heritage 등) 에 영구 잔존 가능성 — force push 로도 완전 제거 불가.**

**의심되면 멈추고 묻기 (강제):** 새 테스트 코드 / commit message / README / 주석 작성 직전 자문 — *이게 외부로 push 되면 사용자의 시청 취향·개인 디렉토리를 식별 가능하게 하나?* 조금이라도 그럴 듯하면 **작성 전** 사용자에게 확인. 임의로 *"이 정도면 괜찮겠지"* 판단 절대 금지.

**적용 범위 — "외부 push 가는 기록" 의 정의:** 소스 코드 (`.cpp/.h/.py/.md/.rc/.txt/...`), commit subject / body, commit diff, README, claude.md, 주석, TODO / FIXME 메모, PR description, issue comment. **제외**: log 파일 (`Log/*.log` 류, 개발용 전용).

**적용 범위 — "미디어 파일명·개인 경로" 의 정의:**
- 사용자의 *실제 시청용 미디어* 파일명 — 영화·드라마·예능·성인물·음악·일본물 등 *작품 식별 가능한 모든 것*.
- 개인 폴더 경로 — `C:\Users\<username>`, `E:\Japanese`, `H:\9.영화`, `\내 드라이브\` 등 username·취향 카테고리·드라이브 레이아웃 노출 경로.
- exception: `sample.mp4` / `test.mkv` / `<exe_dir>` 같은 일반 placeholder.

## 1B. Common C++ 클래스 — 폴더명 = 클래스명, 파일명 = 클래스명에서 선두 `C` 제거 (신규 추가 시 강제)

Common 에 **새 C++ 컨트롤·다이얼로그 클래스 파일을 추가**할 때 폴더·파일 네이밍 규칙:

- **폴더명 = 클래스명 그대로** (선두 `C` 포함). 예: 클래스 `CSCListCtrl` → 폴더 `CSCListCtrl/`.
- **파일명 = 클래스명에서 선두 `C` 만 제거** (PascalCase 유지). 예: 클래스 `CSCListCtrl` → 파일 `SCListCtrl.cpp` / `SCListCtrl.h`.
- 클래스명 자체는 MFC 관례대로 `C` 접두사 유지 (`class CSCListCtrl`).

정리: `Common/<범주>/<클래스명>/<클래스명에서_C뺀_파일명>.{cpp,h}`
- 예: `Common/CEdit/CSCStaticEdit/SCStaticEdit.h` (클래스 `CSCStaticEdit`, 폴더 `CSCStaticEdit`, 파일 `SCStaticEdit.h`) — 기준 사례.
- 예: `Common/CListCtrl/CSCListCtrl/SCListCtrl.cpp` (클래스 `CSCListCtrl`).

**Why (사용자 명시 2026-07-06):** 클래스명과 실제 파일명이 다른 경우가 간혹 있어, *파일명보다 클래스명이 더 정확한 식별자*다. 그래서 폴더는 클래스명으로 고정한다. 파일명에서 `C` 를 빼는 건 기존 SC* 관례(`CSCTreeCtrl`→`SCTreeCtrl.cpp`, `CSCListBox`→`SCListBox.cpp`, `CVtListCtrlEx`→`VtListCtrlEx.cpp`)를 유지.

**적용 범위·이행:** 기존 폴더 중 일부는 아직 이 규칙과 다르게(폴더명이 파일명 기준, 예: `CButton/GdiButton/GdiButton.h` — 폴더가 클래스 `CGdiButton` 아님) 되어 있고 **점진적으로 이관 중**이다. 따라서 이 규칙은 **신규 추가 클래스에 강제**하고, 기존 폴더는 발견 시 이관을 제안하되 임의 대량 rename 은 하지 않는다.

**How to apply:** Claude 가 Common 에 새 클래스 파일을 만들거나 기존 클래스를 복사·포크할 때 — ① 폴더는 클래스명(`C` 포함) ② 파일은 `C` 제거 ③ `.vcxproj`/`.filters` 경로·필터(§2.2)도 이 형태로. (VtListCtrlEx→CSCListCtrl 포크 시 이 규칙으로 `CListCtrl/CSCListCtrl/SCListCtrl.{cpp,h}` 배치.)

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
- **컬러 프리셋 = `CSCColorTheme`**. 커스텀 컨트롤은 개별 `m_cr_*` 멤버 대신 `CSCColorTheme m_theme = CSCColorTheme(this);` 로 보관. 필드는 `m_theme.cr_text`, `cr_back`, `cr_border_active/inactive`, `cr_text_selected/dim`, `cr_back_selected`, `cr_parent_back`, `cr_edit_text/back`, `cr_button_text/back` 등. 테마 교체: `m_theme.set_color_theme(CSCColorTheme::color_theme_dark)`. 개별 setter 는 필드에 직접 쓰기 후 `Invalidate()`. disabled/readonly 같이 theme 에 직접 필드 없는 상태는 `cr_text_dim`, `cr_parent_back` 으로 매핑.
- **응용단 (dlg::init_controls) — `set_color_theme(m_theme)` 한 줄만 호출, 개별 색상 setter 금지 (강제)**. 다이얼로그/뷰에서 자식 컨트롤을 초기화할 때 `control.set_color_theme(m_theme)` 외에 `control.set_back_color(...)`, `control.set_text_color(...)`, `control.set_check_color(...)` 등 개별 색 setter 를 *덧붙이지 말 것*. 테마가 적용된 컨트롤 위에 응용단 override 를 덮으면 (a) theme 교체 시 override 만 남아 색 조합이 깨지고 (b) 같은 theme 을 쓰는 다른 dlg/test 와 외관이 어긋난다. **참조**: `D:\1.Projects_C++\1.test\Test_SCColorTheme\Test_SCColorThemeDlg.cpp` — 모든 자식 컨트롤이 `set_color_theme(m_theme)` 단일 호출만 받고, 콤보로 테마를 바꾸면 추가 setter 없이 전 컨트롤이 정합되게 바뀐다 (검증된 기준 동작).
  - **schema 가 부족하면 응용단에서 우회 말고 `CSCColorTheme` 에 필드를 추가하라**. 예: agent UI 의 primary 버튼이 다이얼로그 bg 와 다른 색이어야 한다면 → dlg 코드에 `m_button_login.set_back_color(blue)` 박지 말고 `cr_button_back` 슬롯을 schema 에 신설해서 `CGdiButton::set_color_theme` 가 읽게 한다. (실제 2026-05-21 LMMLoginManager 작업에서 `cr_button_back`/`cr_button_text` 가 이 경로로 추가됐다. edit 본문도 같은 이유로 `cr_edit_back`/`cr_edit_text` 가 `CSCStaticEdit::set_color_theme` 에서 사용되도록 연결됨.)
  - **Why:** VSCode / Visual Studio / Notepad++ 등 외부 컬러테마를 향후 `CSCColorTheme` 에 import 확장할 계획 — schema 가 design intent 의 단일 source of truth 여야 한다. 응용단 override 가 흩어져 있으면 외부 테마 적용 시 부분만 바뀌어 비일관 발생. 사용자 명시 (2026-05-21).
- **GDI+ 수동 초기화 금지**. `Common/SCGdiplusBitmap.h` 의 `CGdiplusDummyForInitialization` 전역 인스턴스가 자동 처리. `GdiplusStartup`/`GdiplusShutdown` 호출하지 말 것. 대신 빌드에 `SCGdiplusBitmap.cpp` 가 포함되어 있는지 확인.
- **기본 폰트 = `Segoe UI` (강제)**. 새 컨트롤·다이얼로그·D2D/GDI+ 텍스트 그리기에서 폰트 face 를 지정하지 않으면 default 가 Segoe UI 가 되도록 한다. 시스템 폰트(굴림/맑은 고딕/Tahoma 등) 을 그대로 두지 말 것. **Why**: 사용자 명시 — *"앞으로도 모든 기본 폰트는 이 폰트를 기본으로 하자"* (2026-05-06, SCMenu). Endorphin / SCDeskTools 등 기존 컨트롤도 D2D 텍스트 그리기에 이미 `_T("Segoe UI")` 를 사용 중이라 일관성 확보.
  - 새 `LOGFONT` 초기화 시: `GetObject(SYSTEM_FONT)` 또는 부모 폰트로 height/weight 받은 후 `_tcscpy_s(lf.lfFaceName, ..., _T("Segoe UI")); lf.lfCharSet = DEFAULT_CHARSET;` 강제.
  - GDI+ `Gdiplus::Font` / DirectWrite `IDWriteTextFormat` 생성 시 face 인자에 `_T("Segoe UI")` 직접.
  - 기존 코드에서 별도 폰트가 *의도적으로* 지정된 부분 (예: 모노스페이스가 필요한 코드 디스플레이) 은 그대로 둠 — 단 default 가 시스템 폰트로 떨어지는 곳을 찾으면 Segoe UI 로 교체.
- **참조 구현**: `D:\1.Projects_C++\Common\CEdit\SCEdit\SCEdit.h` — `CSCColorTheme` 사용 대표 예시.

### 2.2 Common 모듈을 프로젝트에 추가할 때 — "Common" 필터 + cpp·h 둘 다 (강제)

Common 폴더의 공통 모듈(예: `CSCMenu`, `CSCStaticEdit` 등)을 프로젝트가 필요로 해서 `.vcxproj` 에 추가할 때:

1. **cpp 와 h 를 *둘 다* 추가한다.** `.cpp` 만 빌드에 넣으면 링크는 되지만, 솔루션 탐색기에서 그 모듈의 헤더를 볼 수 없어 탐색·편집이 불편하다. `SCMenu.cpp` 가 필요하면 `SCMenu.h` 도 함께 추가 (`.cpp` → `<ClCompile>`, `.h` → `<ClInclude>`).
   - `.cpp` 는 Common 관례대로 PCH 미사용: `<PrecompiledHeader ...>NotUsing</PrecompiledHeader>` 4 config 모두.
2. **솔루션 탐색기의 "Common" 필터 카테고리에 넣는다.** 즉 `.vcxproj.filters` 에서 그 파일들의 `<Filter>` 를 `Common`(없으면 신설)으로 지정한다. **"소스 파일" / "헤더 파일"(Source Files / Header Files) 필터에 넣지 말 것** — 이 두 필터는 *현재 프로젝트에서 직접 만든 파일* 만 표시되어야 한다.

`.vcxproj.filters` 예시 (Common 모듈 추가 시):
```xml
<!-- 필터 카테고리 선언 (한 번만) -->
<ItemGroup>
  <Filter Include="Common"><UniqueIdentifier>{...}</UniqueIdentifier></Filter>
</ItemGroup>
<!-- 각 파일을 Common 필터에 매핑 -->
<ItemGroup>
  <ClCompile Include="..\Common\CMenu\CSCMenuBar\SCMenu.cpp"><Filter>Common</Filter></ClCompile>
  <ClInclude Include="..\Common\CMenu\CSCMenuBar\SCMenu.h"><Filter>Common</Filter></ClInclude>
</ItemGroup>
```

**Why:** 사용자 명시 (2026-06-24, SCSimDrone 에 `SCMenu.cpp` 만 추가했더니 지적). 사용자는 항상 솔루션 탐색기에 **"Common" 필터**를 만들어 공통 모듈을 그곳에 모은다 — "소스 파일"/"헤더 파일" 에는 그 프로젝트 고유 파일만 두어 *프로젝트 자체 코드와 외부 공유 모듈을 시각적으로 분리*한다. `.cpp` 만 추가하면 헤더가 솔루션 탐색기에서 누락되어 일관성이 깨진다.

**How to apply:** Claude 가 `.vcxproj` 에 Common `.cpp` 를 추가할 땐 ① 같은 모듈의 `.h` 도 `<ClInclude>` 로 추가하고, ② `.vcxproj.filters` 에서 둘 다 `Common` 필터로 매핑한다. (사용자가 수동 정리하겠다고 하면 그 말대로 두되, 자동으로 할 땐 위 규칙을 따른다.)

### 2.3 Common 모듈의 동작·부작용은 그 모듈 안에서 해결 (강제)

Common 공통모듈(컨트롤·다이얼로그·메뉴 등)의 *고유한 동작 특성이나 부작용*은, 가능하면 **그 모듈 내부에서** 해결한다. 각 사용처(응용단)에 같은 처리를 분산시키지 말 것.

**판정 기준 (수정 위치 결정 직전 자문):**
> "이 동작/버그는 *이 모듈을 쓰는 누구에게나* 발생하는가?" → 그렇다 = **모듈 내부에서** 고친다.
> "이 트리거·바인딩·정책은 *이 사용처에만* 해당하는가?" → 그렇다 = 사용처에서.

- 모듈 자체의 메커니즘에서 비롯된 문제(메뉴의 hook dismiss, 컨트롤의 paint/focus/enable 동작 등)는 모듈에서.
- 단, 모듈을 *공유하는 다른 사용처의 동작을 바꾸는* 수정은 회귀(§5C)에 주의 — 부작용 없이 모듈에서 해결할 방법을 우선 찾는다. (per-instance 상태로 국한, capability 분기 등.)

**구체 사례 (2026-06-24, CSCMenu 토글 디바운스):** combo 트리거를 다시 눌러 메뉴를 닫는 그 클릭이 통과되어 `popup_menu` 를 재호출 → 메뉴가 다시 뜨는 토글 버그. 처음엔 트리거 소유자(`CSCPropertyCtrl`)에 디바운스를 넣었으나, 사용자 지적 — *"공통모듈의 어떤 특성은 공통모듈에서 해결했으면 한다. 그렇지 않으면 다른 사용처에서도 모두 다 이런 경우를 처리해줘야하지 않나?"* → `CSCMenu::popup_menu` 가 *per-instance* `m_dismiss_tick` 로 "방금 dismiss 된 직후의 재오픈을 무시"하도록 이전. per-instance 라서 메뉴바의 *다른* 메뉴 인스턴스 전환은 영향받지 않아 회귀도 없음 — 이후 `CSCMenu` 를 트리거로 쓰는 모든 곳이 별도 처리 없이 토글된다.

**Why:** 공통모듈은 N 개 프로젝트가 공유한다. 모듈 고유 특성을 응용단에서 처리하면 (a) 새 사용처마다 같은 코드를 반복해야 하고, (b) 빠뜨리면 그 사용처에서 버그가 재현되며, (c) 모듈의 캡슐화가 깨진다. §2.0("헬퍼는 Common 먼저") 의 *동작/버그 수정 버전* — 기능뿐 아니라 *동작·부작용의 해결 위치*도 Common 우선.

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

## 2F.1 함수 문서화 위치 — 계약은 .h, 구현 사정은 .cpp (강제)

§2F 가 "주석에 *무엇* 을 쓸지" 라면, 이 항목은 "그 주석을 *어디에* 둘지" 다. **중복 금지 — 같은 설명을 .h 와 .cpp 양쪽에 쓰지 말 것.**

**기준: 독자가 누구인가.**

| 위치 | 독자 | 내용 | 판정 질문 |
|---|---|---|---|
| **선언부 (.h)** | *호출자* | 용도 한 줄 요약, 파라미터 의미·단위·유효범위, 반환값 의미(특히 `-1`/`nullptr`/개수 같은 특수값), 사전·사후조건, 부작용(admin 권한 필요·호출 전 `count` 리셋 등), 소유권(반환 포인터 free 책임), 스레드 안전성 | "헤더만 보고 이 함수를 쓸 수 있나?" |
| **정의부 (.cpp)** | *유지보수자·미래의 나* | 비자명한 *왜* — 알고리즘/우회 선택 이유(API 한계·컴파일러 버그·MFC 내부 동작·성능 트레이드오프), 매직 넘버 출처, 비자명한 계산 | "6개월 뒤 이 함수를 *고칠* 때 필요한가?" |
| **양쪽** | — | **지양** (동기화 부담만 증가) | — |

**핵심 규칙:**
- 함수의 *용도·계약* 은 **.h 한 곳에만**. .cpp 에 그 함수가 "무엇을 하는지" 다시 적지 말 것 — §2F 의 "무엇 설명 금지" 가 cpp 정의부에도 그대로 적용된다.
- .cpp 에는 *왜 이렇게 구현했는지* 만, 그것도 비자명할 때만.
- 예: `get_browser_list` 의 계약("HKLM/HKCU enum, 기본 브라우저 0번 swap, 반환=인덱스/빈목록 -1")은 .h 에. .cpp 에는 — 만약 비자명하면 — "HKLM 을 먼저 enum 해 HKCU 중복이 뒤로 밀리게 dedup" 같은 *이유* 만.

**예외 — 공개 헤더가 없는 함수:**
- `static` / 익명 네임스페이스 / .cpp 로컬 함수는 선언이 곧 정의이므로 계약 주석도 .cpp 에 둘 수밖에 없다. 이때만 .cpp 에 용도 요약 허용.

**Why this rule:** 업계 관례(STL·Windows SDK·Qt·.NET·Doxygen)가 전부 **선언부(헤더)** 를 API 문서의 single source of truth 로 삼는다 — IDE hover/IntelliSense·자동 문서생성이 모두 선언을 읽기 때문. `Common/Functions.h` 가 이미 이 방식으로 계약을 적고 있어(`add_registry_str` "호출 전 count 리셋 필요", `delete_registry_key` "키 없으면 ERROR_FILE_NOT_FOUND" 등) 정석에 부합. 사용자 질문(2026-06-01) 에 따라 규칙화하여 모든 프로젝트·양쪽 머신에 일관 적용.

## 2F.2 Claude 가 작성/수정하는 주석에는 `//YYYYMMDD by claude.` 마커 (강제)

Claude 가 **새로 작성하거나 실질적으로 다시 쓰는 주석**에는 반드시 `//YYYYMMDD by claude.` 마커를 **주석 앞에** 붙인다(YYYYMMDD = 작성일). 코드가 아니라 **주석 한정**이다. 사용자가 자기 주석을 `//scpark` / `//20260702 scpark.` 로 남기는 것과 대칭 — 그래서 `by claude.` 한 키워드로 grep 하면 Claude 작성 주석을 전부 찾을 수 있다.

- **한 줄 주석**: 앞에 붙인다 — `//20260704 by claude. SetRedraw(FALSE) 로 native 무효화 paint 를 묶는다.`
- **여러 줄 주석 블록**: 블록당 **한 번**, 첫 줄 앞에만 붙인다 — 첫 줄 `//20260704 by claude. 대상 창이 바뀌면 이전 창을 해제한다.` + 이후 줄은 일반 `//...`. 매 줄마다 붙이지 않는다.
- **inline(코드 뒤) 주석**: `#include <afxcmn.h>  //20260704 by claude. CListCtrl 용.` — 마커를 주석 시작에.
- **기존 사용자 주석을 Claude 가 실질적으로 고쳐 쓰면** 그것도 이 마커로 표시(원저자와 구분). 오탈자·띄어쓰기 등 사소한 수정은 예외.
- 커밋 메시지·PR 본문·문서(`.md`) 본문에는 붙이지 않는다 — **소스 코드 주석에만** 적용(커밋은 이미 `Co-Authored-By` 로 표시됨).

**Why:** 이 사용자의 모든 repo 는 git committer 가 `scpark98` 단일 ID 라, `git blame` 으로는 그 주석을 *사용자가* 썼는지 *Claude 가* 썼는지 구분할 수 없다. 실제로 2026-07-04, `CVtListCtrlEx::OnNMRClick` 위의 잘못된 3줄 주석(2026-07-02 `84661721`, Claude 작성)을 두고 사용자가 *"이 주석은 내가 작성한듯한데... 이해가 안 된다"* 며 자기 것으로 오인했다. 문구 최초 등장 커밋을 추적해서야 Claude 작성임이 판명됐다. 마커가 있었다면 즉시 구분됐을 것 — 사용자 명시 규칙화(2026-07-04): 처음엔 `(by claude)` 접미로 정했다가, `by claude.` 키워드 검색 직관성을 위해 사용자 스타일(`//YYYYMMDD scpark.`)과 대칭인 `//YYYYMMDD by claude.` 접두 마커로 통일.

**How to apply:** 코드에 주석을 추가·수정하는 매 순간 자문 — *이 주석은 내가(Claude) 쓰는가?* 그렇다면 `//YYYYMMDD by claude.` 로 시작. 날짜는 `currentDate` 기준 `YYYYMMDD`. §2F(왜만 작성)·§2F.1(계약은 .h) 와 함께 적용되며, 마커는 그 규칙들 위에 얹는 표시일 뿐 주석 내용 규칙을 바꾸지 않는다.

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

## 2I. 번호 표기 — 원문자(①②③) 금지, (1)(2)(3) 사용 (강제)

목록·단계·항목을 번호로 가리킬 때 **원문자(①②③④…, ㉠㉡, ⑴⑵ 등)를 쓰지 말 것.** 대신 `(1) (2) (3)` 형식을 쓴다.

- 적용 범위: **Claude 의 응답 본문**과 코드 **주석·문자열** 모두. (커밋 메시지·작업 문서 포함.)
- O: `(1) 원인 파악  (2) 진단  (3) 수정`
- X: `① 원인 파악  ② 진단  ③ 수정`

**Why:** 사용자 명시 — *"원문자가 너무 작아서 잘 보이지 않는다"* (2026-07-06). 원문자 글리프는 터미널·에디터 폰트에서 실제 숫자보다 작게 렌더되어 가독성이 떨어진다. `(1)` 은 어떤 폰트에서도 또렷하다.

## 2J. SCLog 진단 코드 — 디버깅 후 삭제 말고 주석 처리 (강제)

버그 진단을 위해 넣은 **SCLog(`logWrite*`) 로그·관련 include·임시 변수**는, 디버깅이 끝나고 커밋/푸시할 때 **삭제하지 말고 주석 처리만** 한다. 다음에 같은 지점을 다시 볼 때 주석만 풀면 되도록 남겨둔다.

- 적용: `logWrite(...)` 호출, 그를 위해 추가한 `#include ".../SCLog/SCLog.h"`, 로그용으로만 쓴 지역변수 계산 블록.
- 주석 처리 시 `//YYYYMMDD by claude.` 마커(§2F.2) 유지 + `[진단]` 등 식별 태그를 남겨 grep 가능하게.
- 단 **TRACE 는 이 규칙 대상 아님** — 진단은 SCLog 로 하고(사용자 선호), TRACE 잔재는 정리 가능.

**Why:** 사용자 명시 — *"디버깅이 완료되고 푸시할때는 SCLog 관련 코드를 다시 지우지말고 주석으로만 처리하라. 매번 넣었다 뺏다 하는것도 시간소요가 많다."* (2026-07-06). 진단 지점은 재발·재조사가 잦아 매번 재작성하는 비용이 크다. 주석으로 남기면 재활성화가 한 줄 토글로 끝난다.

### 2J.1 개발·테스트 중에는 로그를 절대 빼지 말 것 + 커밋 메시지에 명시 (강제)

위 2J 는 "디버깅 완료 후" 규칙이고, 이건 **그 전 단계** 규칙이다. **한 번 넣은 진단 로그는 개발·테스트가 진행되는 동안 절대 제거하지 않는다.**

- **제거 시점은 오직 둘 중 하나**: (1) 사용자가 "최종 테스트 완료" 를 명시해 일괄 정리를 지시할 때, (2) 사용자가 그 로그를 빼라고 직접 지시할 때. 그 외에는 원인을 찾은 것 같아도, 커밋할 때도, 코드가 지저분해 보여도 **그대로 둔다**.
- **로그가 남은 채로 커밋해도 된다.** 대신 커밋 메시지에 **`개발단계 로그 포함`** 을 한 줄 적는다. 로그를 빼려고 커밋을 미루거나, 커밋을 깔끔하게 하려고 로그를 빼는 것 — 둘 다 금지.
- Claude 가 스스로 "이제 필요 없겠다" 고 판단해 빼는 것 **금지**. 그 판단은 사용자만 한다.

**Why:** 2026-07-15 사용자 지적 — *"지금은 개발, 테스트 단계니 로그를 넣은후에 절대 빼지마라. 최종 테스트 완료후에 모두 빼던가, 그렇지 않으면 커밋메시지에 '개발단계 로그 포함'이라는 메시지를 추가로 기록해주면 된다. 자꾸 넣었다 뺏다하기도 하고 어떨때는 빼라고 얘기하지도 않앗는데 막 빼고 그런 동작들이 많았다."* 2026-07-06(§2J), 2026-07-13(자동스크롤), 2026-07-14(드래그) 에 이어 **네 번째 반복 지적**. 넣었다 뺐다가 시간을 잡아먹고, 뺄 때 엉뚱한 줄을 건드리는 실수까지 유발한다. 실제로 이 규칙이 지켜졌다면 07-15 의 `is_noop_move` 회귀는 남아 있던 `[expand]` 로그 한 줄로 즉시 확정됐을 일이었다(추측·재현 요청 반복 없이).

## 2B. 파일 인코딩 — VS 2026 디폴트 따르기

**프로젝트 디폴트** (VS 2026 새 MFC 프로젝트 생성 시 자동 적용되는 형식 그대로):
- `.cpp`, `.h`, `.cc`, `.cxx` → **UTF-8 with BOM** (`EF BB BF` 선두 3바이트)
- `.rc`, `.rc2` → **UTF-16 LE** (BOM `FF FE` + LE 인코딩, VS 의 리소스 편집기가 관리)

**Why VS 2026 디폴트를 따르는가:** VS 자체가 그 인코딩으로 파일을 작성·해석한다. 임의로 다른 형식 (BOM 제거, UTF-16 ↔ UTF-8 변환 등) 으로 바꾸면 VS 가 다음 열기 시 "이 파일을 다른 인코딩으로 변환할까요?" 다이얼로그를 띄움 → 사용자가 "모두 변경" 클릭하면 *모든 열린 파일* 이 mass-convert 되며 BOM 일괄 유실·MSVC 컴파일 에러 줄줄이 발생 (실제 사례 2026-04-27, SCDeskTools 다수 파일 동시 깨짐).

**Why MSVC 가 BOM 을 요구하나:** MSVC 는 BOM 없는 파일을 시스템 코드페이지(한국어 Windows = CP949) 로 해석. UTF-8 한글 주석/문자열이 CP949 로 잘못 해석되면 클래스 선언 중간에 파싱이 끊겨 이후 모든 멤버가 `'m_xxx': 선언되지 않은 식별자` 에러로 출력됨 (SCStaticEditDemo / SCStaticEdit.h 90+ 가짜 에러, 2026-04-21).

**보조 안전장치 — `.vcxproj` 에 `/source-charset:utf-8 /execution-charset:.949` 컴파일 옵션 추가 (강제, 모든 신규/기존 한글 MFC 프로젝트):** 모든 `ItemDefinitionGroup` 의 `<ClCompile>` 내부에 `<AdditionalOptions>/source-charset:utf-8 /execution-charset:.949 %(AdditionalOptions)</AdditionalOptions>` 를 추가한다.

- `/source-charset:utf-8` — MSVC 가 BOM 유무와 무관하게 `.cpp/.h` 를 UTF-8 로 해석. BOM 이 어떤 이유로 떨어져도 한글 주석/문자열이 시스템 코드페이지(CP949)로 잘못 해석되지 않음.
- `/execution-charset:.949` — narrow `char` literal ("한글") 을 binary 에 **CP949 (codepage 949)** 로 저장. wide literal (`L"..."`, `_T(...)` Unicode build) 은 UTF-16 으로 별개 저장.

**왜 `/utf-8` 만 쓰면 안 되나:** `/utf-8` = `/source-charset:utf-8` + `/execution-charset:utf-8` 의 단축. execution charset 까지 UTF-8 로 바뀌면 narrow char literal 이 binary 에 **UTF-8 24 bytes** 로 저장되어, *기존에 CP949 16 bytes 로 컴파일된 binary key·header·file magic 과의 호환성이 깨진다*. 사례: 2026-05-22 KoinoTools 에서 `/utf-8` 추가 후 `Common\SeedProvider.cpp` 의 `char key[17] = "베라시스캔에디터";` 가 CP949 16 bytes 에서 UTF-8 24 bytes 로 바뀌어 (a) `char[17]` overflow warning, (b) 기존 .ver 암호화 파일을 영원히 못 읽게 되는 critical regression. `/source-charset:utf-8 /execution-charset:.949` 로 정정.

**Why 이 조합이 정답:** 한글 MFC 프로젝트는 두 가지 요구가 동시에 있음 — (1) `.cpp/.h` 를 UTF-8 BOM 으로 안전하게 편집 (Claude·VS 가 한글 안 깸), (2) narrow char binary 인터페이스(파일 magic, 암호화 키, 레지스트리 ANSI 값)를 CP949 로 유지 (기존 데이터·다른 프로세스와 호환). 위 두 옵션이 정확히 그 분리를 만들어준다. BOM 부착이 *주 방어*, `/source-charset:utf-8` 가 *보조 방어*, `/execution-charset:.949` 가 *binary 호환 유지*.

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

Claude Read 툴은 파일 바이트를 UTF-8 로 해석한다. **BOM 없는 CP949 `.cpp/.h`** 를 Read 하면 한글 바이트가 UTF-8 로 유효하지 않아 내부적으로 U+FFFD 로 치환되고, 이후 Edit/Write 가 그 U+FFFD 를 UTF-8 바이트로 직렬화해 디스크에 저장 — **한글 영구 손실**. HEAD 커밋은 intact 이라 `git show HEAD:<f> | iconv -f CP949 -t UTF-8` 로 복원 가능하지만 세션 중 작업분은 복원 불가. `D:\1.projects_c++\Common` 은 이미 UTF-8 BOM 으로 통일돼 안전하고, `D:\1.projects_c++\1.test\Test_StaticEx` / `D:\1.projects_c++\SCDeskTools` 도 2026-04-25 ~ 2026-04-27 마이그레이션 완료. `D:\1.projects_c++\KoinoTools` 도 2026-05-22 마이그레이션 완료 (cpp/h 에 UTF-8 BOM 부착 + `.vcxproj` 에 `/source-charset:utf-8 /execution-charset:.949` 4 config 모두 추가 + HEAD 로부터 손상된 cpp/h 복원 + 프로젝트 루트에 `.editorconfig` 추가 — VS save 인코딩 강제). **그 외 프로젝트는 아직 CP949 no-BOM 이 남아있을 가능성.**

**적용 — `D:\1.projects_c++\<project>` 에서 `.cpp/.h` 를 첫 Read/Edit 하기 전에 반드시 (Edit 가 아니라 Read 도 금지 — Read 자체가 손상의 첫 단계):**

**Step 0 (가장 먼저): `.editorconfig` 존재 점검.** 이 파일이 없으면 BOM 을 부착해도 VS 가 다음 save 때 시스템 코드페이지(CP949) 로 다시 떨어뜨린다 — 무한루프. 따라서:

```bash
test -f <project_root>/.editorconfig || echo "MISSING .editorconfig — must add before any Read/Edit"
```

없으면 `Common\.editorconfig` 와 동일 내용으로 즉시 추가. 그 다음 Step 1 (BOM 점검) 진행.

**중요**: `Common\.editorconfig` 가 있다고 *형제 프로젝트* (`KoinoTools` 등) 의 파일이 보호되지 않는다. `.editorconfig` 는 자기 디렉터리 + 하위에만 적용. 각 프로젝트 루트마다 독립적으로 필요.

**Step 1 — BOM 점검:**

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
- 2026-06-10 `Test_StaticExDlg.cpp` 재손상 (다른 프로젝트의 `CSCParagraphStatic`→`CSCStatic` 통합 작업 중 Claude 가 CP949 파일 Edit). `2613bd0` 에서 복원 + `set_tagged_text` 변경만 재적용 + UTF-8 BOM 마이그레이션. 이후 §2B.2 자동화 훅 도입.

## 2B.2 자동화 — SessionStart 훅으로 손상 원천 차단 (권장)

§2B.1 의 수동 audit 은 Claude 가 매번 기억해야 하고 실제로 실패한 적이 있다 (2026-04-24, 2026-06-10 동일 파일 두 번). 사람·에이전트 기억에 의존하지 말고 **SessionStart 훅으로 결정적으로 처리**한다. 훅은 클로드가 Read/Edit 하기 *전에* 실행되어:

1. 프로젝트 안의 CP949(또는 BOM 없는 UTF-8) 한글 소스를 **UTF-8 BOM 으로 변환** (한글·CRLF 보존, 하위폴더 포함). → 이후 Read 가 깨끗한 UTF-8 을 보고, Write 가 U+FFFD 를 만들 수 없음.
2. `.editorconfig` 가 없으면 생성 (VS save 시 BOM 유지).

건드리는 파일은 **non-ASCII 가 든 BOM 없는 소스뿐** — 이미 UTF-8 BOM / 순수 ASCII / UTF-16(`.rc`,`resource.h`) / 빌드 산출물(`x64`,`Debug`,`Release` 등)은 스킵하므로 diff 가 불필요하게 부풀지 않는다. 멱등 — 변환된 프로젝트는 다음 세션부터 스킵.

**한계 — 컴파일 플래그는 훅에 넣지 않는다**: `/source-charset:utf-8` 은 소스가 *전부* UTF-8 일 때만 안전하다. 부모 폴더(`D:\1.Projects_C++`)에 두면 아직 CP949 가 남은 형제 프로젝트가 컴파일 시점에 깨진다. 따라서 컴파일 플래그(`/source-charset:utf-8 /execution-charset:.949`)는 소스를 UTF-8 로 정리한 프로젝트의 `.vcxproj` 또는 그 프로젝트 루트 `Directory.Build.props` 에만 개별 적용.

**한계 — `.rc` 가 include 하는 `.h` 는 변환하면 안 된다 (2026-06-19)**: 훅은 UTF-16 인 `.rc`/`resource.h` 만 스킵할 뿐, **`.rc` 가 `#include` 하는 평범한 `.h`(대표적으로 `targetver.h`)는 한글이 들어 있으면 UTF-8 BOM 으로 변환한다.** 그런데 rc.exe 는 이 헤더도 읽으므로, 변환된 헤더 상단의 doxygen 주석 `@file`/`@brief` 의 `@` 가 `unknown character '0x40'` 로 떠 RC 전처리가 중단되고(`RC terminating after preprocessor errors`), 그 여파로 `CMFCOutlookBarTabCtrl 정의되지 않음` 같은 연쇄 에러가 따라온다. (cl.exe 는 UTF-8 BOM 을 잘 처리하지만 rc.exe 는 민감하다.) **조치**: ① 훅에 `$skipNames = @('targetver.h')` 추가 — 이름이 일치하는 RC-include 헤더는 인코딩 불문 변환 제외. ② 이미 깨진 `targetver.h` 는 표준 형태의 **pure ASCII** 로 되돌리면(한글 주석 제거) 훅이 자동 스킵(순수 ASCII)하고 rc.exe 도 영구 안전. 사례: 2026-06-19 `KoinoViewer\targetver.h`.

### 설치 방식 두 가지 — 상황에 따라 선택

이 한글 손상은 특정 머신·특정 사람 문제가 아니다. 한국어 Windows 에서 CP949 소스를 Claude 로 편집하는 **누구에게나** 발생한다 — 집/회사 머신, 그리고 **다른 개발자 PC** 모두. 두 가지 배포 방식이 있다:

| 방식 | 적용 범위 | 배포 | 적합 |
|---|---|---|---|
| **A. user-scope** (`~/.claude`) | 그 머신에서 여는 *모든* C++ 프로젝트 | git 동기화 안 됨 → 머신마다 1회 수동 설치 | (구) 내 머신들 |
| **B. project-scope** (프로젝트 repo 의 `.claude/`) | 그 *프로젝트 한정*, 그러나 clone 한 **모든 개발자 자동 적용** | repo 에 커밋 → 팀 전체 배포 | 다른 개발자와 공유하는 프로젝트 |
| **C. Common 동기화** (`Common\hooks` + `~/.claude` 등록) | 그 머신에서 여는 *모든* C++ 프로젝트 | **스크립트는 git 동기화**, settings.json 등록만 머신당 1회 | **내 집·회사 머신 (권장, 2026-06-19~)** |

둘은 배타적이지 않다. 내 머신엔 C, 팀 공유 프로젝트엔 B 를 함께 둬도 무방 (둘 다 돌아도 멱등이라 중복 변환 없음).

#### 방식 C — Common 동기화 (권장, 내 집·회사 머신) — 2026-06-19~

방식 A 는 `~/.claude` 가 git 동기화되지 않아 머신마다 스크립트를 *따로* 관리해야 한다 (한쪽에서 버그픽스해도 다른 쪽엔 반영 안 됨). 대신 **훅 스크립트를 Common repo 에 두고** settings.json 만 거기를 가리키게 하면, 스크립트 수정이 양쪽 머신에 git 으로 자동 전파된다.

- **훅 스크립트**: `D:\1.Projects_C++\Common\hooks\ensure-cpp-encoding.ps1` (git 동기화). 방식 A 스크립트 + **솔루션 폴더 인식 수정** 포함 — cwd 에 `.sln` 만 있고 `.vcxproj` 는 하위 폴더(멀티 프로젝트 솔루션)인 경우도 보호한다. (기존 방식 A 는 cwd 직속 `.vcxproj` 만 비재귀로 검사해 이런 솔루션을 통째로 누락했음. 실제 누락 사례: `filetransfer_lmm\File Transfer` — cwd 에 `.sln` 만 있고 `.vcxproj` 는 `nFTDServer\`,`nFTDClient\` 하위.)
- **settings.json 등록**: `~/.claude/settings.json` 의 SessionStart command `-File` 를 위 Common 경로로 지정. settings.json 은 git 동기화가 안 되므로 **머신마다 1회** 맞춰야 하며, 이를 위한 멱등 설치 스크립트 제공:
  ```
  powershell -NoProfile -ExecutionPolicy Bypass -File "D:\1.Projects_C++\Common\hooks\install-hook.ps1"
  ```
  (settings.json 을 JSON 재직렬화 없이 *문자열 치환만* 하므로 한글 경로 등 나머지를 바이트 보존. 이미 Common 을 가리키면 아무 작업도 안 함.)

**Claude 자동 부트스트랩 (사용자 수동 실행 불필요)**: 이 `Common/claude.md` 가 로드된 세션을 시작할 때, `~/.claude/settings.json` 이 아직 Common hook(`...\Common\hooks\ensure-cpp-encoding.ps1`)을 가리키지 않으면 Claude 가 위 `install-hook.ps1` 을 한 번 실행해 등록을 맞춘다. 등록은 *다음* 세션의 SessionStart 부터 효력이 생긴다(현 세션의 SessionStart 는 이미 지난 뒤). 단 이 지시는 **이 파일이 import 되어 로드된 프로젝트에서만** 작동하므로(프로젝트 루트 claude.md 의 `@../../../Common/claude.md` 등), Common 을 import 하지 않는 프로젝트에서는 위 설치 명령을 1회 수동 실행한다.

#### 방식 A — user-scope (머신마다 1회, `~/.claude` 는 git 동기화 안 됨)

경로의 `scpar` 는 **본인 Windows 사용자명**(`%USERPROFILE%` 로 확인)에 맞게 바꾼다.

**1) 훅 스크립트** — `%USERPROFILE%\.claude\hooks\ensure-cpp-encoding.ps1` (예: `C:\Users\scpar\.claude\hooks\ensure-cpp-encoding.ps1`) 생성:

```powershell
# SessionStart hook: make VS C++ projects safe from Korean (CP949) corruption.
$ErrorActionPreference = 'SilentlyContinue'

$proj = (Get-Location).Path
try {
    $raw = [Console]::In.ReadToEnd()
    if ($raw) { $j = $raw | ConvertFrom-Json; if ($j.cwd) { $proj = $j.cwd } }
} catch {}
if (-not (Test-Path -LiteralPath $proj)) { exit 0 }

$vcx = Get-ChildItem -LiteralPath $proj -Filter *.vcxproj -File -ErrorAction SilentlyContinue
if (-not $vcx) { exit 0 }

$actions = @()

$ecPath = Join-Path $proj '.editorconfig'
if (-not (Test-Path -LiteralPath $ecPath)) {
    $ec = @'
root = true

[*.{cpp,h,hpp,c,cc,cxx,inl,ipp}]
charset = utf-8-bom
end_of_line = crlf
indent_style = tab
tab_width = 4
insert_final_newline = true

[*.{rc,rc2}]
charset = utf-16le
end_of_line = crlf

[*.md]
charset = utf-8
end_of_line = crlf
'@
    [IO.File]::WriteAllText($ecPath, $ec, (New-Object Text.UTF8Encoding($false)))
    $actions += '.editorconfig'
}

$exts = @('.cpp', '.h', '.hpp', '.c', '.cc', '.cxx', '.inl', '.ipp')
$skipDirs = @('\x64\', '\win32\', '\debug\', '\release\', '\.vs\', '\ipch\', '\obj\')
$bom = [byte[]](0xEF, 0xBB, 0xBF)
$utf8Strict = New-Object Text.UTF8Encoding($false, $true)
$cp949 = [Text.Encoding]::GetEncoding(949)
$converted = 0

$files = Get-ChildItem -LiteralPath $proj -Recurse -File -ErrorAction SilentlyContinue |
    Where-Object { $exts -contains $_.Extension.ToLower() }

foreach ($f in $files) {
    $low = $f.FullName.ToLower()
    $skip = $false
    foreach ($d in $skipDirs) { if ($low.Contains($d)) { $skip = $true; break } }
    if ($skip) { continue }

    $bytes = [IO.File]::ReadAllBytes($f.FullName)
    if ($bytes.Length -lt 1) { continue }
    if ($bytes.Length -ge 3 -and $bytes[0] -eq 0xEF -and $bytes[1] -eq 0xBB -and $bytes[2] -eq 0xBF) { continue }
    if ($bytes.Length -ge 2 -and (($bytes[0] -eq 0xFF -and $bytes[1] -eq 0xFE) -or ($bytes[0] -eq 0xFE -and $bytes[1] -eq 0xFF))) { continue }

    $hasHigh = $false
    foreach ($b in $bytes) { if ($b -ge 0x80) { $hasHigh = $true; break } }
    if (-not $hasHigh) { continue }

    $isUtf8 = $true
    try { [void]$utf8Strict.GetString($bytes) } catch { $isUtf8 = $false }

    if ($isUtf8) {
        $out = New-Object byte[] ($bom.Length + $bytes.Length)
        [Array]::Copy($bom, 0, $out, 0, $bom.Length)
        [Array]::Copy($bytes, 0, $out, $bom.Length, $bytes.Length)
    } else {
        $text = $cp949.GetString($bytes)
        $u = ([Text.UTF8Encoding]::new($false)).GetBytes($text)
        $out = New-Object byte[] ($bom.Length + $u.Length)
        [Array]::Copy($bom, 0, $out, 0, $bom.Length)
        [Array]::Copy($u, 0, $out, $bom.Length, $u.Length)
    }
    [IO.File]::WriteAllBytes($f.FullName, $out)
    $converted++
}
if ($converted -gt 0) { $actions += "converted $converted file(s) to UTF-8 BOM" }

if ($actions.Count -gt 0) {
    $msg = "ensure-cpp-encoding [$proj]: " + ($actions -join '; ')
    (@{ systemMessage = $msg } | ConvertTo-Json -Compress)
}
exit 0
```

**2) settings.json** — `C:\Users\scpar\.claude\settings.json` 의 최상위에 `hooks` 키 병합 (기존 키 보존):

```json
"hooks": {
  "SessionStart": [
    {
      "hooks": [
        {
          "type": "command",
          "command": "powershell -NoProfile -ExecutionPolicy Bypass -File \"C:\\Users\\scpar\\.claude\\hooks\\ensure-cpp-encoding.ps1\"",
          "timeout": 15,
          "statusMessage": "Checking C++ encoding protection"
        }
      ]
    }
  ]
}
```

설치 후 `/hooks` 를 한 번 열거나 Claude Code 를 재시작하면 다음 세션부터 작동. (검증: 2026-06-10 회사 머신 설치, CP949/UTF-8무BOM/ASCII/UTF-8BOM/UTF-16/빌드폴더 6 케이스 모두 정상 처리 확인.)

#### 방식 B — project-scope (repo 에 커밋, 다른 개발자 자동 적용)

다른 개발자와 공유하는 프로젝트는 훅을 **repo 안에 커밋**하면 clone 한 모든 사람이 별도 설치 없이 보호받는다. user-scope 와 달리 경로에 사용자명이 안 들어가므로 PC 마다 손댈 게 없다.

프로젝트 루트에:

1. **`.claude/hooks/ensure-cpp-encoding.ps1`** — 방식 A 의 동일한 스크립트를 그대로 커밋.
2. **`.claude/settings.json`** — command 를 **상대 경로**로 지정 (SessionStart 훅은 프로젝트 폴더에서 실행되므로 상대 경로가 각 개발자 머신에서 그대로 해석됨):

```json
{
  "hooks": {
    "SessionStart": [
      {
        "hooks": [
          {
            "type": "command",
            "command": "powershell -NoProfile -ExecutionPolicy Bypass -File \".claude/hooks/ensure-cpp-encoding.ps1\"",
            "timeout": 15,
            "statusMessage": "Checking C++ encoding protection"
          }
        ]
      }
    ]
  }
}
```

주의:
- 각 개발자가 그 repo 를 처음 열 때 Claude Code 가 프로젝트 `.claude/settings.json` 의 훅 실행을 신뢰할지 한 번 확인할 수 있다 (정상 — 승인하면 이후 자동).
- 상대 경로 `.claude/hooks/...` 는 훅 실행 시 cwd(프로젝트 루트) 기준. 혹시 해석이 안 되는 환경이면 절대 경로 대신 스크립트 첫 줄에서 stdin 의 `cwd` 로 재구성하는 방식 A 스크립트가 이미 그 cwd 를 쓰므로, command 의 `-File` 경로만 맞으면 된다.
- 한국어 개발자가 아닌 팀원(CP949 소스를 안 만드는)에게도 무해 — non-ASCII 없는 파일은 스킵하므로 아무것도 바꾸지 않는다.

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

## 5B-0. 테스트 요청 전 반드시 빌드(링크까지) 완료 (강제)

사용자에게 *"리빌드 후 테스트해달라"* / *"재현해달라"* 를 요청하기 전에 **Claude 가 먼저 전체 빌드(컴파일 + 링크)를 성공시켜 놓아야 한다.** 산출물(exe/dll)이 실제로 생성된 것까지 확인한 뒤 요청한다.

**X — 절대 금지:**
- `-t:ClCompile` 등 **컴파일만** 돌려놓고 "빌드 후 테스트해주세요" 요청 — 링크 에러가 사용자에게 떠넘겨진다.
- 빌드를 아예 안 돌리고 "수정했으니 테스트해주세요".
- 산출물 타임스탬프를 확인하지 않고 "최신 빌드로 테스트하시면 됩니다".

**O:**
- 전체 빌드(기본 타깃) 성공 → 산출물 경로·타임스탬프 확인 → 그 다음 테스트 요청.
- 빌드가 깨지면 **고쳐서 성공시킨 뒤** 요청. 못 고치면 그 사실을 먼저 보고.

**Why:** 2026-07-16 filetransfer_lmm 에서 Claude 가 `-t:ClCompile` 로 컴파일만 확인해놓고 "컴파일 통과"라고 보고한 뒤 리빌드+재현을 요청. 사용자 지적 — *"빌드까지 성공적으로 완료한 후 부탁하는거 맞냐?"*, *"수정후엔 반드시 빌드까지 완료한 후 테스트 요청하라."* 컴파일 통과는 링크 성공을 보장하지 않는다(정적 lib 의 CRT 불일치 등). 사용자가 링크 에러를 대신 만나는 건 Claude 가 할 일을 넘긴 것.

**How to apply:** 소스 수정 → 전체 빌드 → 산출물 생성 확인 → (필요시 인코딩/BOM 검사) → 테스트 요청. 배포본 경로를 덮어쓰는 게 우려되면 그 사실을 *보고*하되, 빌드를 생략하는 근거로 삼지 말 것. §5B(검증 없이 단정 금지)와 짝 — 5B 가 "안 해본 걸 했다고 하지 마라", 이건 "내가 할 수 있는 검증은 내가 끝내고 넘겨라".

---

## 5B. 검증 없이 단정 표현 금지 (강제)

코드 수정 후 *직접 검증 (사용자가 GUI 에서 확인 / 명시 test 결과 양호) 없이* "잘 동작한다", "fix 완료", "정상 표시된다" 같은 단정 표현 금지. 백그라운드 자동 실행 + log 분석은 *frame deliver 같은 메커니즘 검증* 까지만 — *시각·청각·UX 검증* 은 사용자만 가능.

**X — 절대 금지:**
- "fix 완료. mp4 영상 정상 표시될 것" (사용자 확인 전)
- "MPCVR 정상 동작. 영상 보일 것" (백그라운드 실행 후)
- "잘 표시된다 / 정상 동작 / 회귀 없음" (가설 fix 직후, GUI 검증 없이)

**O — 정확한 표현:**
- "frame deliver / NewSegment / decoder 모두 log 상 정상. 화면 표시는 사용자 GUI 검증 필요."
- "가설 fix 적용 — 빌드 OK. 실제 영상 표시 확인 후 다음 단계."
- "code 는 의도대로 호출됨 (log 확인). 효과는 사용자 검증 후 판단."

**판정 기준 (보고 작성 직전 자문):**
> "이 결론은 *내가 직접 본 사실* 인가, *log 로 추론한 가설* 인가?"
> - 추론이면 → "가설" / "예상" / "log 상" 등 명시.
> - 사실이면 → 어떤 log 라인 / 어떤 검증 결과로 확정인지 함께.

**Why (2026-05-20, Endorphin2 mp4 화면 미표시)**: `set_video_position` 호출 fix 후 백그라운드 자동 실행 log 만 보고 "fix 완료. mp4 표시될 것" 보고 → 사용자 GUI 에서 *여전히 GRAY 배경, 전혀 변화 없음* → *"잘 표시된다<= 이딴거 앞으로 절대 쓰지마라. 전혀 개선된게 없는데 뭘 고쳤다는거냐"*. 메커니즘 (frame deliver, NewSegment 호출) log 검증과 *실제 화면 표시* 는 별개. 단정 표현이 사용자를 *fix 됐다는 잘못된 신뢰* 로 유도 → 분노 증폭.

**적용:**
- 백그라운드 실행으로 검증할 수 있는 것: log 의 함수 호출 여부, 인자 값, 데이터 흐름.
- 백그라운드 실행으로 검증 *불가* 한 것: 화면 표시, 소리 출력, UI 반응성, 시각적 품질, 사용자 인지.
- 코드 수정 후 보고: log 검증 결과 + "사용자 GUI 확인 후 fix 확정" 명시.

## 5A. 환경 불일치 — 우회 코드 금지, 환경을 맞춰라 (강제)

두 머신 (집·회사) 또는 사용자 / 배포 환경 간 차이로 빌드/런타임 오류가 발견되면 **코드에서 fallback / 우회 분기 추가 금지**. 누락된 환경 요소 (라이브러리, 코덱, 렌더러, DLL, 설정 등) 는 **환경을 맞춰서** 해결한다.

**X — 절대 금지:**
- "회사에 MPCVR 가 없으니 EVR fallback 의 windowless setup 코드를 추가하자"
- "이 머신엔 LAV 가 없으니 ffmpeg 직접 호출 path 를 만들자"
- "Win11 에선 동작하는데 Win10 에선 안 되니 OS 분기를 넣자"

**O — 정공법:**
- 누락된 라이브러리·도구·렌더러 → 다운로드 URL / 설치 절차 안내
- 두 머신 환경을 *동일하게* 유지 (이 사용자의 핵심 작업 흐름)
- 코드 fallback 은 사용자가 *명시 요청* 할 때만

**Why (2026-05-20, Endorphin2 내장 FFmpeg path)**: 회사 머신에 MPCVR 미설치 → EVR fallback 으로 영상 미표시 발견. internal FFmpeg path 에 EVR/VMR9 windowless setup 코드 추가 제안 → 사용자 거부 — *"없다고 해서 집과 다르게 우회하려 하지마라. 동일한 환경으로 개발하는게 맞지 않나?"* 사용자 의도는 두 머신 환경을 항상 일치시키는 것. 누락된 환경을 코드로 가리면 (a) "동일 환경" 원칙이 깨지고, (b) 환경 불일치가 코드 복잡성으로 영구 축적되며, (c) 추후 환경 차이가 또 발견됐을 때 같은 우회를 반복하게 됨.

**적용 (환경 차이 발견 시 의무 절차):**

1. 환경 차이 *원인* 을 먼저 식별 (예: registry CLSID grep, dependency walker, `dir <설치경로>`).
2. 사용자에게 환경 차이를 보고 + 누락 환경 설치 안내 (다운로드 URL / 설치 명령).
3. 코드 변경은 *사용자가 명시 요청* 할 때만.
4. 일반 fallback 코드가 *진짜로* 필요한 경우 (배포·튜토리얼·실험) 도 사용자 승인 후에 작업.

**전역 §1 / §2C.0 의 환경 차원 확장.** §1·§2C.0 이 "사용자가 안 시킨 *동작* 추가 금지" 라면 §5A 는 "사용자가 안 시킨 *환경 우회* 추가 금지".

## 5C. 회귀(regression) 금지 — 부작용 미리 추측, 시행착오 cascade 금지 (강제)

어떤 기능을 수정·개선할 때 **잘 동작하던 다른 기능을 깨뜨리는 것은 절대 금지**. 그리고 "고치니 다른 에러 → 그거 고치니 또 다른 에러" 식의 **시행착오 반복(fix→break→fix cascade)** 도 금지.

**Why:** 사용자 명시 (2026-06-01, Endorphin2). 트랙이동 고속화 commit 이 *배속 오디오 무반응* + *뒤로 1프레임 오작동* 두 기능을 조용히 깼다. 사용자가 우연히 발견했기에 망정이지, 못 봤다면 후일 Claude 가 전체 흐름을 다시 이해하고 고쳐야 해서 시간·난이도가 폭증한다. 사용자: *"어떤거 고치느라고 다른 에러 생기고 또 그 에러 고치니 또 다른 에러 생기고... 이런 시행착오를 반복해서는 안된다."*

**How to apply:**
1. **변경 전 영향 범위 추적 (강제)**: 함수·플래그·동작을 바꾸기 전, 그 동작에 *의존하는 모든 호출처/기능* 을 grep 으로 찾아 "이 변경이 깰 수 있는 기능" 을 나열한다. 예: seek 동작을 바꾸면 seek 에 의존하는 frame step / 스냅샷 / A-B 반복 / 자막싱크 / 배속 경로를 *먼저* 점검.
2. **예상 부작용은 *구현 전* 고지**: "이걸 바꾸면 X 기능에 Y 영향이 예상된다" 를 미리 말하면 사용자도 납득한다. 부작용을 *말없이* 만들거나 *구현 후* 발견되는 게 문제. (§2C.0 "구현 전 질문" 과 동일 맥락.)
3. **시행착오 금지 — 추측 fix 반복 대신 측정 먼저**: 증상이 코드 추론과 어긋나면(예: 로그는 깨끗한데 화면은 이상) *추가 fix 를 쌓지 말고* 진단 로그로 단일 원인을 확정한 뒤 한 번에 고친다.
4. 회귀 의심 시 작업 영역의 commit log 부터 확인 (새 기능 통합이 기존 분기를 가드로 차단했을 가능성 우선 의심).

**전역 §1 / §2C.0 / §5A 와 같은 "안 시킨 것 추가 금지" 계열의 *시간축* 확장 — 과거에 되던 것을 깨지 말 것.**

## 5D. 항상 최상의 성능 — 호환성은 폴백으로, 느린 기본값으로 도망가지 말 것 (강제)

성능에 유의미한 차이가 나는 선택지에서는 **항상 가장 빠른 경로를 기본으로** 삼는다. 구형 OS(XP 등)·구형 환경 호환이 필요하더라도, 그것 때문에 *다수 사용자가 느린 경로를 쓰게* 만들지 말 것. **빠른 API/방식 + 구형 폴백** 을 런타임 분기(capability 감지)로 둘 다 제공하는 것이 정석이다.

**X — 절대 금지:**
- "이 빠른 API 는 XP 미지원이니 그냥 (느린) XP 호환 헬퍼만 쓰자" — 다수 Win7+ 사용자를 느리게 만든다.
- 가장 느린 공통분모를 *말없이* 기본값으로 택하고 더 빠른 길을 검토조차 안 함.
- "구현이 단순하니까" 느린 쪽 선택.

**O — 정석:**
- 빠른 경로(예: `FindFirstFileEx(FindExInfoBasic + FIND_FIRST_EX_LARGE_FETCH)` — Win7+)를 시도하고, 미지원 환경에서는 `ERROR_INVALID_PARAMETER` 등으로 감지해 구형 API(`FindFirstFile`)로 폴백. capability 는 1회 판별 후 캐시.
- 디스크 접근·동기 메시지·O(n²) 등 비용이 큰 패턴은 캐시·배치·가상화·런타임 분기로 최적화.
- 더 빠른 길이 *없을 때만* 느린 길을 기본으로. 있는데 호환성 핑계로 포기하지 말 것.

**판정 기준 (구현 직전 자문):**
> "이 환경(구형 OS 등) 제약 때문에 *다른* 환경(다수)까지 느려지나?" → 그렇다 = 런타임 분기로 양쪽 최적.
> "더 빠른 API/자료구조/알고리즘이 있는데 호환성·단순함을 이유로 안 쓰고 있나?" → 그렇다 = 다시 검토.

**Why (2026-06-12):** 사용자가 listctrl 폴더 로딩 가속 중 `FindFirstFileEx` 빠른 플래그가 XP 미지원이라는 이유로 Claude 가 *그냥 느린 `FindFirstFile` 만 쓰자* 고 판단. 사용자 지적 — *"XP 사용자를 위해 그걸 포기하는건 납득이 안간다. XP 이후 OS 사용자가 훨씬 많지 않은가? XP일때는 호환헬퍼, XP이상일때는 빠른 길을 쓰는 그 정도 분기처리도 안한단 말인가? 항상 최상의 성능을 낼 수 있게 최선의 코드를 작성하라."* (이후 OS 분기 + capability 캐시로 정정.)

**전역 §1("불필요한 구문 금지")의 반대축** — *필요한 성능 코드는 호환성 핑계로 빼지 말 것*. §5A(환경 우회 금지)와 충돌하지 않는다: §5A 는 "없는 환경을 코드 우회로 가리지 말라", 본 항목은 "있는 빠른 길을 구형 호환 때문에 포기하지 말라(둘 다 제공하라)".

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

---

## OnPaint / draw 함수에 분기 추가 시 — 도달 가능성을 함께 확인

paint 함수에 새 그리기 분기를 추가할 때, 함수 *맨 아래* 에 코드를 붙이는 것만으로는 부족하다. 입구의 `if (...) return;` 같은 early return 이 *어떤 상태에서* 함수를 일찍 끊는지, 그 상태에서도 새 분기가 실행돼야 한다면 그 분기까지 같이 도달하게 만들어야 한다.

**Why:** 2026-05-21 CPathCtrl 의 `OnPaint` 끝부분에 `else if (m_draw_border) draw_rect(...)` 만 추가하고 사용자에게 "확인해 보라" 라고 보고. 함수 입구의 `if (m_path.size() == 0) return;` 때문에 path 미설정 상태에서는 새 분기가 실행되지 않아 — 사용자 입장에선 *resource editor 의 Border=true 가 여전히 효과 없는 것처럼* 보였음. 사용자 지적: *"OnPaint에서 border 그리는 조건도 충분히 클로드가 같이 봤어야 할 흐름이다"*. (같은 paint 함수에 대해 2 회 연속 부분 분석으로 사용자가 재교정한 패턴이므로 강하게 메모.)

**How to apply:**
1. paint / draw 함수에 변경 가하기 전, 함수 *전체* 를 한 번 훑어 모든 `return` / `goto` / 예외 throw / 가드 식별.
2. 추가하려는 분기가 "*어떤 상태에서 그려져야 하는가*" 를 분명히 한 뒤, 그 상태가 early return 으로 끊기는 경로에 속하지 않는지 확인.
3. 끊기는 경로에서도 그려야 한다면 ① 그 분기 *안에* 도 같은 그리기를 넣거나, ② early return 자체를 재구성해 모든 경로가 공통 paint 종료점을 거치게 한다.
4. 변경 후 "확인해 보라" 라고 보고하기 전에 *사용자가 보게 될 초기 상태에서 새 코드가 실행될지* 머릿속으로 한 번 실행해 본다. 빌드만으로는 안 잡힘.
5. 이 규칙은 paint 외에도 적용 — 입력 핸들러 (`OnLButtonDown`, `OnKeyDown`), 메시지 dispatcher 등 early return / 가드가 많은 함수 전반.

---

## Common commit 컨벤션 — cross-project 작업 분리

Common 은 여러 프로젝트 (Endorphin2 / SCDeskTools / Test_* / ASee 등) 에서 *동시 병행* 으로 수정된다. 한 push 에 여러 프로젝트 맥락의 변경이 섞이면 commit 의미가 흐려지고 blame / revert 단위가 무너진다. 다음 규칙을 모든 Common commit 에 적용한다.

### 규칙

1. **모듈명 prefix** — commit title 은 `[모듈명]` 으로 시작.
   - 예: `[Subtitle] 다중 Class 분리`, `[SCMenu] move_item_after API`, `[PathCtrl] Border 속성 처리`, `[colors] white theme inactive 색 fix`
   - 모듈명 = 디렉토리/클래스 이름 (`subtitle/`, `CMenu/CSCMenuBar/`, `CStatic/PathCtrl/`, `colors`, `Functions`, `messagebox/CSCMessageBox/` 등).
   - 한 commit = 한 모듈 원칙. 부수적으로 1-2개 파일 다른 모듈 같이 fix 됐으면 같은 prefix 안에 본문으로 명시 (`[PathCtrl] ... + 부수 colors/GdiButton fix`).

2. **트리거 프로젝트 suffix (선택)** — 그 모듈 작업을 *왜* 했는지 후일 추적 위해 commit title 또는 본문에 트리거 프로젝트 기록.
   - title: `[Subtitle] 다중 Class 분리 — Endorphin2 한영 메뉴용`
   - 본문: 첫 줄 빈 줄 후 한 줄 `(트리거: Endorphin2 자막 메뉴 작업)` 또는 본문 안에 자연스럽게.

3. **여러 모듈 섞이면 무조건 분리 commit** — push 전 git status 가 여러 모듈에 걸쳐 있으면 `git add <모듈별 파일>` 로 한 모듈씩 commit. 한 push 안에 여러 commit 은 OK, 한 commit 안에 여러 모듈은 X.

4. **"push" 명령 = Common 포함 모든 repo push 가 원칙 (강제, 2026-06-04 사용자 정정)** — 사용자가 "push" / "모두 푸시하라" 등 push 를 지시하면 그 명령의 범위는 *변경이 있는 모든 repo* 다. 현재 프로젝트 repo 뿐 아니라 **Common 도 함께 push** 한다. Common 에 *이번 세션에서 만들지 않은 타 세션 변경분* 이 있어도 포함한다 (§5 의 sync commit 으로 분리). 사용자 명시 — *"앞으로 푸시하라고 하면 Common까지 모두 푸시하는 것을 원칙으로 한다"* + *"'해당 프로젝트가 알아서 push' 의 해당 프로젝트는 이 프로젝트도 되고 다른 프로젝트도 될 수 있다"* (= 현재 세션이 Common 의 타 세션 변경분을 push 해도 됨).
   - **단, push 는 여전히 사용자의 명시 지시에서만 (불변)** — "작업 끝났으니 푸시" 같은 *Claude 의 절차적 자동 push* 는 금지. push 는 외부·되돌리기 어려운 동작이라, 사용자가 push 를 말하지 않았는데 임의 push 하지 말 것.
   - **기능 미검증이어도 push 명령 시엔 push 하되 미검증 상태를 보고한다** — 빌드 통과 = *컴파일 검증* 일 뿐 기능 검증이 아니다. 사용자가 push 를 지시하면 GUI 미검증 변경도 함께 push 하되, *어떤 변경이 아직 GUI 미검증인지* 명확히 보고한다 (§5B — 단정 표현 금지). 검증은 push 후 사용자가 수행한다. (이전 규칙은 "기능 검증 후에만 Common push" 였으나, cross-machine sync 단절 비용이 더 커 사용자가 위 원칙으로 정정.)

5. **다른 세션의 작업분도 push 명령 시 함께 push (sync commit 분리)** — 본인 (현재 세션) 이 수정하지 않은 변경이 git status 에 있어도, 사용자가 push 를 지시하면 *별도 commit* 으로 분리해 함께 push 한다. commit 본문 끝에 `(타 세션 작업분 — 이 commit 은 sync 목적, 메시지 부정확할 수 있음.)` 한 줄 명시. (이전엔 "해당 프로젝트가 알아서 push" 로 남겨뒀으나, 그 '해당 프로젝트' 에 현재 프로젝트도 포함됨 — 2026-06-04 정정.)
   - **Common 이 clean 으로 나오는 경우 = 정상** — 다른 프로젝트가 이미 그 Common 변경을 commit·push 했으면 이 프로젝트 working tree 엔 Common 변경이 안 보인다 (clean). 이때는 push 할 것이 없다. `git pull` 로 최신만 받으면 됨. clean 인데 억지로 만들 필요 없음.

6. **commit subject 는 사용자 인지 가능한 한국어 요약 (강제)** — 기술 세부 (함수명·내부 메커니즘·thread context 등) 만 적힌 subject 는 *git log 훑을 때 어떤 변경인지 직관적으로 알 수 없음*. **사용자가 요청한 요구사항의 처리 내용** 을 *간략히 한 줄* 로 표현. 기술 세부는 body 에.
   - X: `[playback_rate] get_track_pos 를 audio decoder input PTS 기반으로 — rate 무관 미디어 시점` (기술 세부만 — 사용자가 어떤 user-facing 변경인지 모름)
   - O: `[playback_rate] 배속과 관계없이 컨트롤바 시각·자막 싱크 일치` (사용자 인지 가능 — git log 한 줄 보고 즉시 어떤 fix 인지 파악)
   - X: `[Subtitle] .smi end SYNC 손실 fix + End= 속성 인식 + multi-class end 출력`
   - O: `[Subtitle] 자막 끝시간(End) 정보 저장·복원 + 다국어 통합 자막 호환`
   - subject 안에 사용자 요약 + 단축 hint 가능: `[playback_rate] 배속 변경 시 컨트롤바·자막 싱크 (get_track_pos = audio input PTS)`.
   - body 에는 기존처럼 기술적 root cause / fix detail / 파일 단위 변경 등.

### Why (사용자 인지 가능 subject)

2026-05-29, 사용자가 *"이전 커밋으로 돌아가려해도 내가 인지할 수 있는 메시지가 안 쓰여있다보니 어느 날짜의 어느 커밋이 내가 되돌리고자 하는 커밋인지 알 수가 없었다"* 보고. *git log 가 사용자의 일차 navigation tool* — 기술 세부만 적힌 subject 는 *Claude 가 회고할 땐* 충분하지만 *사용자가 revert 후보를 고르는 작업* 에 무용. *사용자 친화 요약 + 기술 body* 조합이 두 시각 모두 만족.

### Why

2026-05-21, Endorphin2 자막 작업 중 push 직전 git status 에서 Common 에 *전혀 모르는 다른 세션의 PathCtrl + theme 작업 + claude.md 규칙 추가* 가 잠재됨이 발견. 한 push 에 자막 작업과 섞으면 의미 / blame / revert 모두 흐려져, 사용자가 *"다른 프로젝트와 병행 작업 시 commit 메시지가 모두 반영되지 않는 단점이 있는데 좋은 방법이 있나?"* 라고 질문. 위 규칙으로 합의 — 모든 Common commit 에 일관 적용.

### How to apply (작업 흐름)

1. 작업 시작 시 `git status` 로 Common 현재 변경 목록 파악. 이미 다른 세션의 변경이 잠재돼 있으면 인지.
2. 본인 작업 진행 — Common 의 *어느 파일* 을 어떤 *모듈 단위* 로 만지는지 머릿속에 명확히.
3. 작업 끝나면 `git add <모듈 파일들>` 로 본인 작업분만 stage → `[모듈명]` prefix commit.
4. 남은 변경 (다른 세션 작업분) 은 diff 빠르게 확인 후 모듈별로 분리 stage + commit. 부정확할 수 있는 메시지는 `(타 세션 sync)` 명시.
5. push.

---

## Git 설명 시 commit hash 표시 형식 — `hash(yyyy-mm-dd hh:mm:ss)` (강제, 모든 프로젝트)

사용자에게 git 관련 설명을 전달할 때 commit hash 를 단독으로 쓰지 말 것. **반드시 `hash(yyyy-mm-dd hh:mm:ss)` 형식**으로 commit 시각을 함께 표시한다.

- O: `7caa152(2026-06-01 17:45:05)`, `a92f66c(2026-06-01 20:27:30)`
- X: `7caa152`, `커밋 7caa152`, `a92f66c 에서`

**Why:** 사용자 명시 (2026-06-02) — *"hash가 어떤 날짜의 hash인지 나에게는 직관적이지 않다."* hash 단독으로는 사용자가 그 커밋이 *언제* 것인지 직관적으로 알 수 없어, revert 후보 식별·작업 시간 추적이 어렵다. 시각을 붙이면 git log navigation 과 1:1 매핑된다. (`## Common commit 컨벤션` §6 "사용자 인지 가능 subject" 와 같은 맥락 — git 정보는 사용자가 *시간축으로* 인지 가능해야 한다.)

**How to apply:**
- 시각은 `git show -s --format=%ci <hash>` 또는 `git log --pretty=format:"%h %ci %s"` 의 author/commit date 사용 (`%ci` = committer date, ISO-8601 `yyyy-mm-dd hh:mm:ss +0900`). 표시는 timezone offset 생략하고 `yyyy-mm-dd hh:mm:ss` 까지만.
- 여러 commit 을 나열할 때도 각각 형식 적용.
- 설명 중 같은 commit 이 반복 등장하면 첫 등장에 형식 표기 후 이후는 hash 만 써도 무방 (가독성).
- 적용 범위: 사용자에게 *전달하는 설명·보고*. commit message 본문이나 코드 주석에는 적용 안 함 (그건 별개 컨벤션).

---

## Log 파일 위치 — SCLog 컨벤션 (모든 프로젝트 공통)

Common 의 `CSCLog` (`log/SCLog/SCLog.cpp`) 가 모든 프로젝트의 `logWrite(...)` 출력을 받아 파일에 기록한다. 위치 규칙:

- **폴더**: `<exe_dir>\Log\` (실행파일 같은 폴더의 `Log` 하위).
- **파일명**: `<exe_title>_YYYYMMDD_HHMMSS.log` — 실행 시각이 파일명에 포함되어 **매 실행마다 새 파일** 생성. 예: `Endorphin2_20260521_143012.log`.
- 같은 날 짧게 여러 번 실행했어도 시:분:초까지 들어가므로 *덮어쓰기 없음*. 최근 세션 log 를 찾을 땐 mtime 기준 가장 최근 파일.

### 적용

- 사용자가 *"로그가 어디 있나?"* / *"방금 실행 log 확인해"* 류 질문하면 **위 패턴 그대로** 답할 것. `log.txt` 같이 일반화된 이름으로 추측하지 말 것.
- 진단 코드 추가 시 `logWrite(_T("[모듈] message ..."))` 형식으로 prefix 통일 — grep 으로 모듈별 추적이 쉬워진다.
- log 파일 직접 읽으려면 가장 최근 파일을 찾아야 함 — `Get-ChildItem <exe>\Log -Filter "*.log" | Sort-Object LastWriteTime -Descending | Select-Object -First 1`.

**Why:** 2026-05-21, 자막 작업 중 사용자가 *"방금 rename 한 세션의 log 확인"* 을 요청. 클로드가 첫 응답에 `log.txt` 라고 추측해 답했고 사용자 지적 — *"log위치는 실행파일 경로의 Log라는 폴더에 실행시마다 새로 생기도록 이미 그렇게 우리 테스트하지 않았나? 왜 log.txt라는 생소한 로그파일을 언급하나? 일관성을 잃지마라"*. SCLog 컨벤션은 Common 라이브러리에 정의돼 모든 프로젝트가 동일 패턴 — 다음 세션도 이 규칙으로 일관 응답하도록 명시.

### 진단 로그는 최종 push 전까지 유지 — 넣었다 뺐다 금지 (강제)

개발/디버깅용으로 추가한 `logWrite(_T("[모듈] ..."))` 진단 로그는 **그 작업이 최종 push 되기 전까지 코드에 그대로 둔다.** 한 수정이 "검증됐다" 싶을 때마다 로그를 제거하지 말 것 — 같은 영역을 곧 다시 디버깅할 일이 흔하고, 매번 넣었다 뺐다 하면 (a) 불필요한 재빌드·diff 노이즈, (b) 다음 디버깅 때 처음부터 다시 로그를 깔아야 함.

- **추가**: 진단 시 `[모듈]` prefix 로 추가 (SCLog 컨벤션).
- **유지**: 작업이 끝나도, GUI 검증이 끝나도 push 전까지 남겨둔다. include(`Common/log/SCLog/SCLog.h`)도 함께 유지.
- **제거 시점**: 해당 작업을 **최종 push 하기 직전** 일괄 정리 (또는 사용자가 명시적으로 제거 지시할 때).
- 임시 캡처 파일·스크린샷 등 *코드 밖 산출물* 은 즉시 정리해도 무방 — 이 규칙은 *소스 내 진단 로그* 한정.

**모듈별 push 정책 — 멀티미디어만 로그 유지, 그 외는 push 전 제거 (강제, 2026-06-11 추가):**
- **멀티미디어 재생 모듈 (`directx/DirectShow`, `ffmpeg`, audio 필터, `subtitle` 등) + `log/SCLog` 인프라**: `SCLog`/`logWrite` 를 **포함한 채 push 해도 됨**. 재생 파이프라인은 런타임 진단이 상시 필요하고 로그 파일은 배포·push 안 됨(.gitignore).
- **그 외 일반 파생 컨트롤·헬퍼 모듈 (`CSliderCtrl`/`CTreeCtrl`/`CListBox`/`CVtListCtrlEx`/`CStatic`/`PathCtrl`/`Functions` 등)**: **`SCLog`/`logWrite` 를 포함한 채 절대 push 금지.** 디버깅 중엔 위 규칙대로 유지하되, **push 직전 그 모듈의 테스트 로그(+`#include ".../SCLog/SCLog.h"`)를 모두 제거**한 뒤 push.

**Why:** 사용자 명시 (2026-06-11). 슬라이더 정렬 검증 후 로그 제거 시 *"개발작업중일때마다 log를 넣었다 뺐다 할거냐? 최종 푸시하기 전까지는 로그를 유지해라."* → push 직전 정리 원칙. 이어서 *"dshow, ffmpeg, audio, subtitle 등 미디어 관련외에 일반 파생 컨트롤 및 헬퍼 모듈에서는 절대 SCLog, logWrite를 포함한 채 푸시해서는 안된다."* → 멀티미디어만 예외. (이때 tree/list/listbox 의 `[hscroll-*]` + slider `[slider]` 로그를 push 전 제거.)

---

## Windows XP 호환 — SC 컨트롤은 XP 까지 지원 (강제, 최우선)

`CSCTreeCtrl` / `CVtListCtrlEx` / `CSCListBox` / `CSCEdit` / `CSCStaticEdit` / `CSCComboBox` / `CSCStatic` 등 **Common 의 SC* 컨트롤은 Windows XP 까지 동작해야 한다.** XP 에 존재하지 않는 **API 도, 폰트도 절대 쓰지 말 것.**

**절대 금지 (XP 미존재):**
- **폰트**: `Segoe UI`(Vista+), `Segoe MDL2 Assets`(Win10+), `Segoe Fluent Icons`(Win11), `맑은 고딕/Malgun Gothic`(Vista+). → XP 표준 폰트만: `Tahoma`, `굴림(Gulim)`, `바탕(Batang)`, `돋움(Dotum)`, `MS Sans Serif`, `MS Shell Dlg`/`MS Shell Dlg 2`, `System`.
- **아이콘 글리프 폰트로 기호 그리기 금지**: 트리 expand chevron, 화살표 등을 `Segoe MDL2/Fluent` 글리프(`0xE76C` 등)로 그리지 말 것 — XP 에 그 폰트가 없다. **GDI/GDI+ 벡터(`DrawLines`/`draw_line`/path)로 직접 그린다.** (2026-05-27 CSCTreeCtrl chevron 을 폰트 글리프로 그렸다가 이 규칙으로 폴리라인으로 환원.)
- **API**: Vista+ 전용 API 직접 호출 금지. DWM(`DwmSetWindowAttribute` 등)·immersive dark mode 등은 반드시 Common 의 `win_compat::dwm::*` wrapper 경유 (XP/Vista 에서 자동 no-op). 신규 `dwmapi`/`uxtheme` API 직접 호출 금지.

**적용:**
- 새 폰트 생성 시 face 를 하드코딩하려면 위 XP 표준 목록에서 고르거나 부모/시스템 폰트를 상속. (Common §2 의 "기본 폰트 = Segoe UI" 는 *D2D/GDI+ 신규 앱* 한정 — XP 타깃 SC 컨트롤에는 적용하지 않는다. 충돌 시 XP 호환 우선.)
- 기호/아이콘은 벡터로 직접 그림. 작은 글씨 또렷함이 필요하면 `NONANTIALIASED_QUALITY` + 비트맵 내장 XP 폰트(굴림/Tahoma) 조합 (ClearType 색번짐·grayscale 뭉갬 없이 픽셀 또렷).
- 새 API 쓰기 전 "이게 XP 에 있나?" 자문. 없으면 `win_compat` wrapper 추가 또는 대체 구현.

**Why:** 2026-05-27 사용자 명시 — *"tree/listctrl/edit등 모두 XP까지 지원해야 한다. 따라서 XP 호환되지 않는 API든, 폰트는 절대 쓰지 말아야 한다. 메모해라."* 같은 세션에서 chevron 을 Segoe Fluent Icons 글리프로 그렸다가(XP 미존재) 폴리라인으로 환원한 직후 받은 지시. SC 컨트롤은 광범위한 레거시 환경에 배포되므로 XP 호환이 깨지면 해당 환경에서 컨트롤이 동작하지 않거나 폰트가 fallback 돼 외관이 무너진다.

## 한국어 표현 ambiguity — 임의 해석 금지, 질문 먼저 (강제)

사용자는 영어 native 가 아니므로 *한국어 표현의 의도* 가 *영어 native phrasing 패턴* 과 어긋날 수 있다. **임의 해석으로 진행 금지, 의심되면 질문 먼저.**

**주의 패턴:**
- **"먼저 X"** — *X 를 실행 전 단계로 끼워라* 의미가 다양 (먼저 보여줘 / 먼저 검토 / 먼저 정리 / 다음 작업 시 먼저). 의도 불명확하면 질문.
- **"~ 해달라"** — request. 적용 시점 (즉시 / 다음 / 항상) 이 불명확하면 질문.
- **"확인해라"** — *내가 자체 확인* 인지 *사용자에게 보여서 확인 받기* 인지 다름.
- **함축 (주어 / 목적어 생략)** — 한국어는 생략 빈번. *내가 할 일인지 사용자가 할 일인지* 헷갈리면 질문.

**판정 기준 (행동 전 자문):**
> "이 표현이 다른 해석 가능성이 있나? 사용자가 의도한 것을 100% 확신하나?"
> 의심 1% 라도 있으면 *질문 1 줄* 추가. 묻는 비용 << 잘못 실행하고 되돌리는 비용.

**특히 *되돌리기 어려운 작업* (push / commit / 파일 삭제 / 사용자 데이터 수정) 직전엔 의도 100% 확정 안 되면 진행 금지.** 자체 해석 후 진행하더라도 *"이렇게 해석해서 진행했습니다 — X. 다른 의도였으면 알려주세요"* 로 보고하여 사용자가 즉시 정정 가능하게.

**Why (2026-05-29):** 사용자가 *"commit 메시지는 ... 내가 인지할만한 수정내용을 *먼저 제시*해달라"* 요청. 의도는 *push 전에 commit 메시지를 사용자에게 보여달라, 검토 후 push*. 그러나 Claude 가 *"다음 commit 부터 새 룰 적용"* 으로 해석 후 그냥 push. 사용자 지적 — *"다음부터는 문장을 정확히 이해해달라. 애매한 표현이 있다면 질문부터 하라. 영어가 아니다보니 오해가 있을수 있다."*

## 긴 셸 명령·파일 경로는 화면 표시 + 클립보드 동시 (강제, Windows)

사용자에게 셸 명령(특히 PowerShell)을 제시하거나 **생성한 파일의 절대 경로를 안내할 때는, 화면에 보여주는 동시에 무조건 클립보드에도 넣는다.** 사용자가 드래그로 선택해 붙여넣지 않게 한다. (한 줄 넘는 긴 명령 / 산출물 파일 경로 모두 대상. temp·scratchpad 의 긴 경로는 드래그 복사해도 탐색기에서 바로 안 열린다.)

**Why:** 사용자 명시(2026-06-17). 화면의 명령을 드래그 선택→PowerShell 붙여넣기 하면 (a) 스마트 따옴표 변환, (b) 멀티라인 `>>` 연속 프롬프트 꼬임, (c) 터미널 선택이 줄바꿈 래핑·앞 여백·박스문자까지 포획 — 으로 에러가 쏟아져 매우 불편하다. 사용자 — *"드래그로 텍스트 선택하고 실행하면 꼭 이런가? 정말 불편하다"*, *"한줄 넘어가는 긴 명령줄은 화면에도 표시하지만 무조건 클립보드에도 같이 넣어라"*.

**How to apply:**
- 긴 명령은 한 줄로 정리해 `printf '%s' '<command>' | clip` 으로 클립보드에 넣고, 같은 내용을 화면에도 코드블록으로 표시한다.
- 클립보드는 한 번에 하나만 담기므로, 여러 명령이 필요하면 **사용자가 실행할 다음 명령 하나만** 클립보드에 두고, 지금 무엇이 클립보드에 있는지 명시한다(순차 진행).
- 짧은(한 줄 이내) 단일 명령은 화면 표시만으로 충분(클립보드는 선택).
- **파일 경로**: `Set-Clipboard -Value "<절대경로>"` 로 넣고 "클립보드에 복사됨(붙여넣기하면 바로 열림)" 을 덧붙인다. 여러 경로면 방금 만든 대표 산출물 하나를 넣는다. (코드 인용의 `file:line` 참조는 제외.)

## Confluence 문서 생성 — 라이브 문서로 (강제, 모든 프로젝트)

Confluence(`koinodoc.atlassian.net`)에 문서를 만들 때는 **무조건 라이브 문서(live doc)로 생성**한다. 일반 페이지(static page)로 만들지 말 것.

**How to apply:** `createConfluencePage` 호출 시 `contentType: "page"` + **`subtype: "live"`** 를 지정한다. 공간/부모 위치, 본문 작성 방식은 동일하다.

**Why:** 사용자 명시(2026-06-24). 수정 히스토리 문서를 일반 페이지로 처음 생성한 뒤 — *"다음부터는 무조건 페이지가 아닌 라이브문서로 생성하라."* 라이브 문서는 편집·갱신이 잦은 진행성 기록(패치 히스토리, 대응 로그 등)에 적합하며 사용자의 표준 워크플로다.

## 방식이 다른 알고리즘 개선 — 기존 것 유지 + 선택 가능하게 (강제, 모든 프로젝트)

어떤 알고리즘/처리방식을 개선하자고 했을 때, **단순 변수 튜닝·사소한 보정이 아니라 방식 자체가 다른(접근법이 바뀌는) 개선이면 기존 알고리즘을 지우지 말고 유지한 채, 둘 다 선택해서 쓸 수 있도록** 구현한다.

**How to apply:**
- 기존 함수를 통째 교체(덮어쓰기)하지 말 것. 기존 구현을 별도 함수/브랜치로 보존하고, 새 방식을 나란히 추가한 뒤 **enum/옵션 파라미터로 디스패치**한다.
- 기본값은 기존(검증된) 방식. 새 방식은 호출부에서 명시 선택. (예: `enum class drone_map_algorithm { edge, tone_fill };` + `build_drones_from_image(img, algorithm=edge)`)
- 피사체/입력 특성(인물 vs 사물, 크기, 매끈함 등)에 따라 어느 방식이 더 나은지 다르므로, 선택지를 남겨 두는 것 자체가 가치다.
- "단순 변수냐 / 방식 자체가 다르냐" 가 애매하면 유지 쪽으로 판단(보존이 기본).

**Why:** 사용자 명시(2026-06-24). 엣지 기반 이미지→드론 매핑을 톤-면채움 방식으로 "개선"하며 기존 함수를 통째 교체 → 사용자가 git 에서 복원 요청. *"방식 자체가 다른 알고리즘으로 개선한다고 하면 기존 알고리즘도 우선 유지하고 선택해서 사용할 수 있는 방향으로 구현하라. 지우고 복원시키는 작업이 그동안에도 좀 빈번했다."* 삭제→복원 왕복은 반복돼 온 비용이다.
