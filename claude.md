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

### 2.1 Common 의존 세부 규약

- **Include 경로 전제**: 프로젝트 IncludePath 에 `D:\1.Projects_C++;` 가 들어 있어 `#include "Common/colors.h"`, `#include "Common/Functions.h"`, `#include "Common/SCGdiplusBitmap.h"` 등이 직접 동작한다. 새 프로젝트 셋업시 이 경로 누락 여부 먼저 확인.
- **색상 타입 기본 = `Gdiplus::Color`**. 특별한 이유 없으면 `COLORREF` 대신 `Gdiplus::Color` 사용. `RGB(r,g,b)` 대신 `gRGB(r,g,b)` (colors.h 매크로). GDI API 에 넘길 때만 `RGB(c.GetR(), c.GetG(), c.GetB())` 로 변환.
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
   - 단순 타입(`int`, `bool`, `CString` 등)은 접두사 없이 snake_case 만. Hungarian `str/n/b` 접두사 일반적으로 제거.

4. **Boolean 멤버**: `m_is_*` 또는 그냥 서술어 (`m_readonly`, `m_composing`, `m_dynamic`, `m_draw_border`). 맥락에 따라.

5. **MFC 관례 유지 (예외)**:
   - 기반 클래스 override (`Create`, `SetFont`, `SetWindowText`, `GetWindowText`, `DoDataExchange`, `PreSubclassWindow`, `PreTranslateMessage`) — MFC convention 유지
   - **`ON_WM_*` 매크로로 wiring 되는 표준 메시지 핸들러** (`OnPaint`, `OnChar`, `OnKeyDown`, `OnLButtonDown`, `OnSetFocus`, `OnSize` 등) — MFC convention 유지 (매크로가 이름을 강제)
   - 메시지 맵 매크로 (`DECLARE_MESSAGE_MAP`, `BEGIN_MESSAGE_MAP`) 유지
   - MFC override 함수의 **파라미터** 에는 MFC Hungarian 남겨도 됨 (`BOOL bRedraw`, `CFont* pFont` 등) — 혼용 허용

6. **`ON_MESSAGE(WM_XXX, ...)` 로 wiring 하는 커스텀 핸들러는 snake_case** (예: `on_ime_start_composition_message`). `ON_WM_*` 와 구분 기준은 wiring 방식.

## 2B. C/C++ 소스 파일 인코딩 — UTF-8 with BOM (강제)

모든 C/C++ 소스 (`.h`, `.cpp`, `.rc`) 는 **UTF-8 with BOM** (`EF BB BF` 선두 3바이트) 으로 저장한다.

**Why:** MSVC 는 BOM 없는 파일을 시스템 코드페이지(한국어 Windows = CP949) 로 해석. UTF-8 한글 주석/문자열이 CP949 로 잘못 해석되면 클래스 선언 중간에 파싱이 끊겨 이후 모든 멤버가 `'m_xxx': 선언되지 않은 식별자` 에러로 출력됨. SCStaticEditDemo / SCStaticEdit.h 에서 BOM 유실로 90+ 가짜 에러 발생한 실제 사례 있음 (2026-04-21).

**How to apply:**
- `Edit` 는 BOM 보존됨 (실측 확인). 매 편집마다 검사 불필요.
- `Write` 로 새 파일 생성·완전 교체 시 BOM 누락 위험 — 작성 후 BOM 부착 확인.
- BOM 부착: `printf '\xef\xbb\xbf' > f.new && cat f >> f.new && mv f.new f`
- CP949 → UTF-8(BOM) 변환: `iconv -f CP949 -t UTF-8 f > f.new && printf '\xef\xbb\xbf' > f.final && cat f.new >> f.final && mv f.final f && rm f.new`
- 의심 증상: 분명히 선언된 변수가 "선언되지 않은 식별자" 로 수십 개 이상 → BOM 먼저 의심.
- BOM 누락 / 다른 인코딩 발견 시 **묻지 말고 강제 변환** (사용자 명시 방침, 2026-04-21).
- 인코딩 판별: `file <path>` 결과가 "UTF-8 (with BOM)" 외이면 변환 대상.

## 3. 분석 요청 시 탐색 깊이 자제

"분석해달라" 요청이 오면 진입점(main dialog, 호출된 핸들러) 만 확인하고 보조 파일까지 재귀적으로 펼치지 말 것. 필요 최소한 깊이만 확인 후 답하고, 사용자가 더 깊이 가자고 하면 그때 확장.

**단 §2 와 혼동 금지**: 실제 사용 중인 Common 라이브러리 확인은 "과도한 탐색" 이 아니다. 호출된 메서드의 실제 구현이 외부에 있다면 그건 **진입점의 일부**다.

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
