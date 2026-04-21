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
