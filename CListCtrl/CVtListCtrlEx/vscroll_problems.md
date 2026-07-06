# CVtListCtrlEx 세로 스크롤 문제 — 조사 기록 및 인수인계

> **작성**: 2026-07-06 (nFTDServer/파일전송 앱 세션 중). 파일전송 전용 앱에서 계속 파기 어려워 문서로 이관.
> **대상 파일**: `D:\1.Projects_C++\Common\CListCtrl\CVtListCtrlEx\VtListCtrlEx.cpp` / `.h`
> **상태**: 마지막 항목 안 보임 = **해결(팬텀 행)**. 하단 여백 = **미해결(설계 트레이드오프)**.
> **주의**: 이 컨트롤은 세로 스크롤 관련으로 오랜 기간 수많은 시행착오를 겪은 fragile 영역이다. 수정 시 반드시 회귀(regression)를 미리 예측하고, 진단 로그로 확인하며 단계적으로 진행할 것.

---

## 0. TL;DR

- **증상**: 세로 스크롤바 썸을 맨 아래로 내려도 **마지막 항목이 안 보이고**, 썸이 바닥에 안 붙고 **살짝 되돌아 튄다**. 마우스 휠로도 끝까지 못 감. 특정 창 높이에서 재현.
- **근본 원인 (로그로 확정)**: 이 컨트롤은 세로 스크롤을 **native 리스트뷰의 스크롤 범위에 위임**하는데, native의 스크롤 page 계산이 **커스텀 헤더(높이=행높이) 1행을 범위에서 빼지 않아**, native의 최대 top-index가 우리가 필요한 값보다 **정확히 1 작다**. 그래서 `SB_BOTTOM`·픽셀 `Scroll` **모두 그 지점에서 clamp**되어 마지막 항목이 영영 안 올라온다.
- **해결한 것**: native item count 를 실제보다 **pad(=1) 만큼 늘리는 "팬텀 행"** 으로 native 스크롤 범위를 정확히 확장. 마지막 항목 정상 표시 + 썸 튐 해소 확인.
- **남은 문제**: `m_bottom_reserve`(NC 예약 빈 띠, 0~25px)로 인한 **하단 여백**. 팬텀 행 도입 후 이 예약은 사실상 미관 낭비가 됐으나, 그냥 없애면 상단정렬 특성상 여백이 팬텀 자리로 옮겨갈 뿐. 탐색기처럼 없애려면 **bottom-align 도입**(정렬 코어 수정)이 필요.

---

## 1. 이 컨트롤의 세로 스크롤 아키텍처 (전제)

수정 전에 반드시 이해해야 하는 구조:

1. **owner-data + owner-draw**: `LVS_REPORT | LVS_OWNERDATA | LVS_OWNERDRAWFIXED`. 데이터는 `m_list_db`(std::deque)에 있고, 화면 렌더는 native가 `WM_DRAWITEM`→`DrawItem()`으로, 텍스트/이미지/상태는 `LVN_GETDISPINFO`→`OnLvnGetdispinfo()`로 우리에게 요청한다.
2. **커스텀 OnPaint**: `OnPaint()`는 memory DC 에 배경을 칠한 뒤 **`DefWindowProc(WM_PAINT)`로 native가 행을 그리게 위임**한다. 즉 *어떤 행을 어느 y에 그릴지는 native 리스트뷰의 내부 스크롤 장부(scroll info / top-index)가 결정*한다. 우리가 픽셀을 직접 찍지 않는다.
3. **오버레이 스크롤바(`CSCScrollbar`)**: 세로/가로 스크롤바는 native 스크롤바가 아니라 별도 `CSCScrollbar` 이며 **parent 가 이 리스트가 아니라 메인 다이얼로그**다. `OnNcCalcSize`가 우측/하단에 NC 띠를 예약하고 그 위에 dialog-child 로 배치된다. `sync_scrollbar()`가 매번 위치·범위·thumb 위치를 동기화한다.
4. **스타일 처리(`OnStyleChanging`)**: `WS_HSCROLL`은 strip(native 가로바 억제), **`WS_VSCROLL`은 유지**(native `SB_BOTTOM` bottom-align 을 쓰기 위해 — 이 전제가 사실 문제의 핵심, §4 참조).
5. **행 높이**: `MeasureItem()`이 `itemHeight = m_line_height`. `set_line_height()`가 imagelist 스왑 + 1px resize dance 로 재측정을 강제. shell 리스트는 `set_as_shell_listctrl()`에서 `set_header_height(26)`, `set_line_height(26)` (2026-07-05 변경, 이전 24/21).
6. **remainder 예약(`m_bottom_reserve`)**: `OnNcCalcSize`가 세로바가 보일 때 `rem = item_area % m_line_height` 를 client 하단에서 NC로 잘라내 **item 영역을 행 높이의 정확한 배수**로 만든다. 도입: 2026-06-16 `5f90861` "세로스크롤 끝 항목 잘림·썸 튐 수정". 이 예약이 §6 하단 여백의 정체다.

**핵심 함수 위치**(grep 앵커):
- `OnPaint()` — 커스텀 페인트, `DefWindowProc(WM_PAINT)` 위임
- `OnNcCalcSize()` — 우측/하단 gw NC 예약 + `m_bottom_reserve` remainder 예약
- `sync_scrollbar()` — 세로/가로바 위치·범위·thumb 동기화, need_v/need_h 수렴, **[팬텀 행] pad 로직**
- `on_message_CSCScrollbar()` — 오버레이 썸 드래그 처리(세로: SB_BOTTOM vs 픽셀 Scroll 분기), **[BLANKROW2] 진단 로그**
- `OnVScroll()` / `OnLvnEndScroll()` — native 스크롤 후 sync + **top-snap**(top 항목을 헤더 밑에 flush)
- `OnLvnGetdispinfo()` / `DrawItem()` — **[팬텀 행] 인덱스 가드**(크래시 방지)
- `OnMouseWheel()` — 휠 스크롤(세로/shift+가로)
- `size()` — 가상 리스트면 `m_list_db.size()`(실제값), 아니면 `GetItemCount()`

---

## 2. 증상 (재현)

- remote(또는 local) 리스트에서 항목 수가 창보다 많아 세로바가 생긴 상태.
- 썸을 맨 아래로 드래그 → **마지막 항목이 client 아래로 1행 잘려 안 보임**. 썸이 트랙 끝에 안 붙고 살짝 위로 되돌아온다.
- 마우스 휠 아래로도 마지막 항목 도달 불가.
- 창을 살짝 resize 하면 그 높이에선 우연히 보이기도 함(높이에 따라 재현/비재현).
- **크래시는 없음**.

---

## 3. 진단 방법 (현재 코드에 남아있는 계측)

> 사용자 요청으로 진단 TRACE/SCLog 를 **제거하지 않고 유지**한 채 커밋함. 재조사 시 그대로 활용.

- **`OnVScroll()`**: `TRACE("CVtListCtrlEx::OnVScroll\n")` — DebugView 전용, native WM_VSCROLL 유입 확인용.
- **`on_message_CSCScrollbar()` 세로 분기 끝 `[BLANKROW2]`**: `logWrite(...)`(SCLog)로 스크롤 상태 전량 기록. 필드:
  `pos / new_top / cur_top / max_top / visible / total / gcp(GetCountPerPage) / nativeCount(GetItemCount) / branch(SB_BOTTOM|pixel) / afterTop(스크롤 후 GetTopIndex) / item0.top / last.bottom / clientH / hdr / reserve / WS_VSCROLL`.
- **SCLog 로그 위치**: `C:\Users\Public\Documents\LinkMeMine\Log\FileTransfer\<exe>_YYYYMMDD.log` (nFTDServer 기준; 다른 앱은 그 앱의 `gLog.set()` 경로). `logWrite` 매크로는 `extern CSCLog* pLog` 를 쓰고 NULL 가드가 있어, CSCLog 인스턴스가 없는 프로젝트에선 자동 no-op(안전). SCLog.h 를 include 해야 함 — 현재 `#include "../../log/SCLog/SCLog.h"` 를 진단용 임시로 추가해 둠.

### 3.1 결정적 로그 증거 (2026-07-06)

**케이스 A** (초기, 팬텀 없음): `total=141 visible=28 → 필요 max_top=113`, 그러나 `afterTop=112`(SB_BOTTOM 보내도 안 오름). `clientH=754 hdr=26 reserve=24 row=26 WS_VSCROLL=1`. → native max = 112 = 141 − **29**.

**케이스 B** (팬텀 1차, pad 공식 오류): `visible=23 gcp=23 nativeCount=141 → pad(=gcp−visible)=0`(팬텀 안 붙음). `afterTop=117` (필요 118). `clientH=624 reserve=25`. → native max = 117 = 141 − **24**. **여기서 native 스크롤 page = 24 = GetCountPerPage(23) + 1** 임이 드러남.

**결론**: `GetCountPerPage()`(완전 가시행수 = `visible`)와 **native 의 스크롤 page(=floor(clientH/rowH))** 는 다르다. 후자가 헤더 1행만큼 크다.

---

## 4. 근본 원인 (확정)

native report-view 리스트의 세로 스크롤 최대 top-index 는:

```
native_max_top = nativeItemCount − native_scroll_page
```

로그로 확정된 사실:
- `native_scroll_page = floor(clientH / rowH)` — **커스텀 헤더 높이를 page 계산에서 빼지 않는다.** (헤더는 client 상단 rowH 만큼을 차지하는데 native 는 그만큼을 스크롤 범위에서 제외하지 않음.)
- 우리가 논리적으로 필요한 값: `our_max_top = total − visible`, 여기서 `visible = floor((clientH − header) / rowH)` = 실제 헤더 아래 완전 가시행수 = `GetCountPerPage()`.
- 헤더 높이 == 행 높이(26==26)이므로 `native_scroll_page = visible + 1`. 즉 **native_max_top 이 our_max_top 보다 정확히 1 작다.**

그 결과:
- 썸을 끝까지 내리면 코드가 `new_top >= max_top` 을 만족해 `SB_BOTTOM` 을 보내지만, **native 는 자기 max(=our_max−1)에서 멈춘다.**
- **`SB_BOTTOM` 뿐 아니라 픽셀 `Scroll(CSize(0,dy))` 도 native 스크롤 범위로 clamp** 되어, 어떤 방법으로도 마지막 행을 못 올린다. (시도 §5.3 참조 — 로그로 확인함.)
- 이것이 "마지막 항목 안 보임 + 썸 튐"의 단일 원인이다. (썸 튐 = 마지막 항목이 client 안에 완전히 안 들어와 `sync_scrollbar`의 thumb-pin 조건(`last.bottom <= clientH`)이 거짓 → thumb 이 max_pos 로 pin 안 되고 GetTopIndex 위치로 되돌아옴.)

**아이러니**: 코드 주석은 *"SB_BOTTOM 이 마지막 항목을 bottom-align 한다"* 고 전제했으나, 이 구성(커스텀 헤더 + owner-data + reserve)에서 **SB_BOTTOM 은 헤더를 무시해 1행 짧게 top-align 한다.** 전제 자체가 틀렸다.

---

## 5. 시도한 것들과 결과 (부작용 포함)

### 5.1 진단 A — 스크롤 후 전체 `Invalidate(FALSE)` → **원인 아님 (기각)**
가설: 빈 행이 부분 무효화(Scroll 이 좁은 띠만 invalidate) 탓. 스크롤 정착 후 전체 재그리기 강제해도 **빈 행/마지막 항목 그대로**. → 페인트/무효화 문제 아님. (부작용 없음. 제거함.)

### 5.2 행 높이 일치 확인 (line-height 가설) → **원인 아님 (기각)**
가설: 2026-07-05 `set_line_height(21→26)` 재측정이 안 먹어 native 행 높이 ≠ m_line_height. 로그: `actual_row_h=26 == m_line_height=26`. → line-height 무관.

### 5.3 SB_BOTTOM 후 잔여 픽셀 보정 `Scroll(0, last.bottom − clientH)` → **native 가 clamp (무효)**
가설: SB_BOTTOM 이 1행 짧으니 넘친 만큼 픽셀로 더 스크롤. 로그: `afterTop` 그대로(112). **native 는 픽셀 Scroll 도 자기 범위로 clamp** 하여 no-op. → 픽셀 보정 접근 폐기. **교훈: native 스크롤 범위 자체를 늘리지 않으면 어떤 스크롤도 그 범위를 못 넘는다.** (제거함.)

### 5.4 팬텀 행 — native item count 를 pad 만큼 늘림 → **마지막 항목 해결 (채택)**
`SetItemCountEx(real + pad)` 로 native 스크롤 범위만 확장. 핵심 격리:
- 스크롤 **로직의 `total` 은 전부 `size()`(실제값, 가상 리스트=m_list_db.size())** 로 사용 — `sync_scrollbar`, `on_message_CSCScrollbar`, `OnMouseWheel` 3곳. 팬텀은 오직 native 스크롤 범위에만 작용, 논리엔 영향 없음.
- **pad 공식(중요 — 1차 오류 후 수정)**: 처음 `GetCountPerPage() − visible` 로 했으나 gcp==visible 이라 pad=0(무효)이었다. native 스크롤 page 는 `floor(clientH/rowH)` 이므로 **`pad = floor(clientH/rowH) − visible`** 로 수정. reserve 로 `clientH = (visible+1)*rowH` 가 되어 실질 pad=1. (헤더가 여러 행일 경우까지 일반화된 기하식.)
- **크래시 방지 가드(필수)**: 팬텀 인덱스(`>= m_list_db.size()`)를 native 가 조회/그리면 `m_list_db[iItem]` OOB 로 죽는다. 기존 텍스트 분기 가드만으론 부족(IMAGE/STATE 분기·DrawItem 무방비)했다. **`OnLvnGetdispinfo()` 시작부 + `DrawItem()` 시작부에 `iItem >= m_list_db.size() → return` 가드 추가.**

**결과**: 여러 창 높이에서 **마지막 항목 완전히 보임 + 썸 튐 해소** 확인. 크래시 없음.

**팬텀 접근의 남은 미세 엣지**(현재 방치, 크래시는 없음):
- 바닥에서 방향키↓ 로 native 선택이 팬텀 인덱스(=total)로 갈 수 있음(가드로 그리기/조회는 안전하나 논리상 "선택된 것 없는 선택" 가능). 필요 시 선택 클램프 추가.
- `GetItemCount()` 를 "실제 항목 수"로 쓰는 **다른 호출처가 생기면** 팬텀만큼 어긋난다. 현재 스크롤 3곳은 `size()`로 격리했으나, 향후 코드가 `GetItemCount()`를 논리 카운트로 쓰면 주의. (권장: 논리 카운트는 항상 `size()`/`m_list_db.size()`.)

---

## 6. 남은 문제 — 하단 여백 (미해결)

### 6.1 정체
하단 여백 = **`m_bottom_reserve` (NC 예약 빈 띠, 0 ~ rowH−1 = 0~25px)**. `OnNcCalcSize`가 item 영역을 행 배수로 만들려 client 하단을 NC로 잘라낸 것. 세로바가 있으면 **항상** 존재하며 창 높이에 따라 크기가 변한다(최악 ≈ 한 행). 마지막 항목은 client 바닥에 flush 돼 있고, 그 아래 NC 띠가 빈칸으로 보인다.

원래 목적("끝 항목 잘림 방지")은 이제 **팬텀 행이 담당**하므로, 이 예약은 사실상 미관 낭비가 됐다.

### 6.2 왜 그냥 못 없애나
`reserve`를 0으로 하면 client 가 행 배수가 아니게 되고, 이 컨트롤은 **상단정렬(top-align)**(OnLvnEndScroll 의 top-snap: 매 스크롤 후 top 항목을 헤더 밑에 flush)이라 **남는 픽셀이 이번엔 바닥의 팬텀 행 자리(그리지 않음)로 옮겨가 그대로 빈칸**이 된다. 여백 위치만 바뀔 뿐.

**탐색기가 여백이 없는 이유**: 맨 아래에서 마지막 항목을 창 바닥에 flush(**bottom-align**)시키고, 남는 부분 행을 **맨 위**에 부분 표시하기 때문. 우리 컨트롤은 항상 top-align 이라 남는 게 바닥으로 간다.

### 6.3 근본적 한계
`clientH` 가 `header + N*rowH` 의 정확한 배수가 아닌 한, **top-align 을 유지하면 바닥에 remainder 여백이 불가피**하다. 없애려면 정렬 전략을 바꿔야 한다(§7).

---

## 7. 더 시도해 볼 만한 것 (다음 후보)

### (1) 탐색기식 bottom-align (미관 최상, 중위험) — **1순위 추천**
- `m_bottom_reserve` 예약 제거 → client 전체 사용.
- 맨 아래로 스크롤 시(SB_BOTTOM 자리) **마지막 실제 항목의 bottom 을 clientH 에 flush** 시키는 픽셀 위치로 스크롤. 남는 부분 행은 **맨 위**에 부분 표시.
- 관건: 팬텀으로 native 범위가 +1 확장돼 있으므로, bottom-align 에 필요한 sub-row 픽셀 위치가 native 범위 **안**에 드는지 검증 필요(범위 밖이면 또 clamp). `[BLANKROW2]` 로그로 `afterTop`/`last.bottom` 확인하며 진행.
- 위험: `OnLvnEndScroll` 의 top-snap 이 bottom-align 을 매번 top 으로 되돌린다. **top-snap 을 "바닥 상태에선 skip"** 하도록 조건 분기 필요 — 이 부분이 정렬 코어라 회귀 위험(과거 "끝 항목 잘림·썸 튐"이 이 근처에서 나왔음).

### (2) 리스트 컨트롤 높이를 행 배수로 스냅 (저~중위험)
- 리사이즈 시 리스트 높이를 `header + N*rowH` 로 내림 스냅 → remainder=0 → 예약도 여백도 0, 부분 행도 없음.
- 단점: 잘려나간 픽셀(최대 25px)이 리스트 밖(리스트와 다음 컨트롤/상태바 사이)의 다이얼로그 배경으로 나타날 수 있음. 레이아웃 의존적이라 앱마다 결과 다름. 리스트 컨트롤이 자기 높이를 강제하면 상위 resize 로직과 충돌 가능.

### (3) 현행 유지
- 여백은 최대 ~1행, 기능은 정상(마지막 항목 보임). 미관만 감수.

### 그 밖에 확인해 볼 것
- **native 가 왜 헤더를 스크롤 page 에서 안 빼는가**의 진짜 이유(커스텀 `CHeaderCtrlEx` 높이 vs native 가 인지하는 헤더 높이 불일치? `WS_HSCROLL` strip / OnNcCalcSize NC=0 커스터마이즈의 부작용?). 이걸 바로잡으면 팬텀 없이 native 만으로 해결될 여지.
- `GetCountPerPage()` 와 native scroll page 가 갈리는 지점을 Spy++/WM_VSCROLL SCROLLINFO 로 직접 관찰(`GetScrollInfo(SB_VERT)`)해 native 내부 range/page 를 확인.
- 헤더 높이를 행 높이보다 **작게**(예: 헤더 22, 행 26) 하면 native over-count 가 사라져 팬텀 없이도 도달 가능한지(임시 실험).

---

## 8. 현재 코드에 들어가 있는 변경 요약 (이 문서 시점)

`VtListCtrlEx.cpp`:
- **[팬텀 행]** `sync_scrollbar()`: `total = size()`, `pad = floor(clientH/rowH) − visible`, `SetItemCountEx(real + pad, LVSICF_NOSCROLL)` (가상 리스트 + 스크롤바 셋업 시).
- **[팬텀 행]** `on_message_CSCScrollbar()` / `OnMouseWheel()`: `total = size()` (native pad 무관하게 실제값 사용).
- **[팬텀 행 가드]** `OnLvnGetdispinfo()` 시작부 + `DrawItem()` 시작부: `iItem >= m_list_db.size() → return`.
- **[진단]** `on_message_CSCScrollbar()` 세로 분기 끝: `[BLANKROW2]` `logWrite` (SCLog). `#include "../../log/SCLog/SCLog.h"` 임시 추가.
- **[진단]** `OnVScroll()`: 기존 `TRACE` 유지.

모든 진단/수정 주석에 `//20260706 by claude.` 마커. **진단 코드(TRACE/SCLog)는 사용자 지시로 제거하지 않고 유지 상태로 커밋.** 정식 마무리 시 §2J 규칙(Common/claude.md)에 따라 삭제가 아니라 주석 처리 예정.

---

## 9. 재현·검증 체크리스트 (다음 세션용)

1. 세로바가 생기는 폴더로 이동, 창 높이를 여러 값으로 바꿔가며 썸을 맨 아래로.
2. SCLog `[BLANKROW2]` 확인: `nativeCount == total + 1`(pad 적용됨?), `afterTop == max_top`(끝 도달?), `last.bottom <= clientH`(마지막 항목 완전 가시?).
3. 마지막 항목 육안 확인 + 썸 바닥 pin 확인 + 바닥 방향키↓ 크래시 없음 확인.
4. 하단 여백(=reserve) 크기 = `reserve` 필드. §7 (1) 시도 시 이 값이 0 으로 가고 여백이 사라지는지 확인.
