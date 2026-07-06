# CSCListCtrl — 기존 CVtListCtrlEx 리뉴얼 (픽셀 페인트 / smooth scroll)

> 작성 2026-07-06. 집↔회사 이어서 작업하기 위한 인수인계 문서.
> 대상: `D:\1.Projects_C++\Common\CListCtrl\CSCListCtrl\SCListCtrl.{cpp,h}`
> 테스트: `D:\1.Projects_C++\1.test\Test_SCColorTheme` (오른쪽 리스트 `m_list` 가 이 클래스, `set_smooth_scroll(true)` 로 실험 중)

---

## 0. 왜 만들었나 (배경)

### 해결하려는 문제
`CVtListCtrlEx`(및 형제 `CSCTreeCtrl`/`CSCListBox`)는 **하단 여백** 문제가 있다. 세로 스크롤을 native 리스트뷰에 위임하는데, **native report-view 의 세로 스크롤은 whole-item 단위로 양자화**되어(측정 확정: `Scroll(0,-23)` 요청 → native 가 -26 으로 스냅) **부분 행을 표현할 수 없다.** 그래서 `client높이−헤더` 가 행높이의 배수가 아니면 남는 0~25px 가 반드시 어딘가(프레임 reserve 띠 / client 내부)에 빈칸으로 남는다. 창 높이에 따라 여백이 생겼다 사라졌다 한다.

측정으로 확정된 사실(2026-07-06):
- 세 컨트롤(list/tree/listbox) **전부** top-align + item 양자화 → 바닥 remainder 여백 공통. 트리도 `last.bottom(504) < clientH(521)` 로 17px 여백 확인. **헤더 유무는 원인이 아니다**(트리는 헤더 없는데도 여백).
- 셋이 "해결"한 건 여백이 아니라 **마지막 항목 도달**(native self-clamp 스크롤 + list 는 팬텀). 리스트박스는 remainder 를 `cr_back` 으로 칠해 "빈 프레임"이 아니라 "리스트 빈 공간"으로 보이게 눈속임할 뿐.
- **여백을 진짜 없애는 유일한 길 = 픽셀 페인트**(native 위임 버리고 항목을 픽셀 위치에 직접 그림 → 부분행/여백0). 이게 full-custom 리스트가 처음부터 하던 방식.

### 왜 포크했나
`CVtListCtrlEx` 는 여러 프로젝트가 사용 중이라 큰 실험을 원본에 하면 위험. 그래서 **원본 복사 → 리네이밍 → 이 테스트 프로젝트에서만 include** 하여 격리 실험. 이름 충돌 회피 목적(`CVtListCtrlEx` 는 외부 유래 이름)도 겸해 `CSCListCtrl` 로.

---

## 1. 포크 구성 (네이밍·격리)

| 항목 | 값 |
|---|---|
| 클래스 | `CSCListCtrl` (MFC 관례, C 유지) |
| 폴더 | `Common\CListCtrl\CSCListCtrl\` (**폴더명 = 클래스명**, C 포함 — `Common/claude.md §1B` 규칙) |
| 파일 | `SCListCtrl.cpp` / `SCListCtrl.h` (**파일명 = 클래스명에서 선두 C 제거**) |
| 필터 | `Common\CSCListCtrl` |
| 리네이밍 | `CVtListCtrlEx`→`CSCListCtrl`, `Message[String]_CVtListCtrlEx`→`..._CSCListCtrl`(고유 메시지 ID), `CVtListCtrlExMessage`→`CSCListCtrlMessage` |

- **HeaderCtrlEx / list_data 는 복사 안 함 — 공유.** `SCListCtrl.h` 가 `../CVtListCtrlEx/HeaderCtrlEx.h`, `../CVtListCtrlEx/list_data.h` 를 include. (이 두 클래스는 list 타입에 독립인 헬퍼·데이터 세트. 복사하면 tree 드래그 때문에 함께 컴파일되는 원본 `CVtListCtrlEx` 의 `CHeaderCtrlEx` 와 **중복 정의 링크 충돌**. CSCListCtrl 이 CVtListCtrlEx 를 완전 대체하면 그때 세트를 폴더로 이동.)
- **원본 `VtListCtrlEx.{cpp,h}`·`SCTreeCtrl.cpp` 는 pristine(git HEAD)** — 타 프로젝트 무영향. 이 테스트는 tree 드래그(SCTreeCtrl→CVtListCtrlEx) 때문에 원본도 계속 컴파일함.
- 테스트 프로젝트: `Test_SCColorThemeDlg.{h,cpp}` 가 `CSCListCtrl m_list`, `set_smooth_scroll(true)`, `on_message_CSCListCtrl`. `.vcxproj`/`.filters` 에 CSCListCtrl + (복구된)SCShapeDlg 추가.

---

## 2. 설계 — opt-in 토글 + draw_row Y-override

**토글**: `set_smooth_scroll(bool)` (기본 off). off = 기존 native 위임 동작 100% 그대로(회귀 0). on = 픽셀 페인트 경로.
**멤버**: `bool m_smooth_scroll`, `int m_scroll_y`(픽셀 세로 오프셋, 항목영역 기준), `int m_focus_anchor`(shift-range 선택 기준).

**핵심 재사용 메커니즘 — `draw_row(CDC*, iItem, row_bounds)`**:
- 기존 `DrawItem` 본문을 `draw_row` 로 추출. 셀 X 는 `GetSubItemRect`(컬럼/가로스크롤, **세로 위치 무관**)로, **Y 만 `row_bounds` 로 덮는다.**
- native 위임 경로(`DrawItem`)는 `row_bounds = GetItemRect(iItem)`(=화면 Y) 를 넘김 → Y override 가 **no-op**(동작 불변).
- smooth 경로(`OnPaint`)는 내가 계산한 픽셀 rect 를 넘김 → 부분행/자유 위치 렌더.
- 즉 **셀 렌더링 로직(색·아이콘·체크박스·퍼센트바·텍스트·폰트)을 재작성 없이 그대로 활용.**

---

## 3. 단계별 플랜과 진행 상태

| 단계 | 내용 | 상태 |
|---|---|---|
| 0 | 골격(토글·m_scroll_y) | ✅ 완료 |
| 1 | 항목 렌더링 픽셀화 (부분행/여백0) | ✅ 완료·GUI 검증 (어느 높이에서도 마지막 항목 flush, 여백0) |
| 2 | 히트테스트(클릭 선택 정확도) | ✅ 좌클릭·우클릭·메뉴 검증 |
| 3 | 키보드 네비(방향키/PageUp·Down/Home·End/EnsureVisible) | ✅ 완료·GUI 검증 (§8-3) |
| 4 | 인라인 편집·툴팁·드래그 y 좌표 | ✅ 편집 위치·체크박스·재클릭편집·드롭대상 Y 완료 (§8-4). 툴팁은 코드 없음(해당 없음) |
| 5 | 가로 스크롤 통합 + 엣지케이스 마무리 | 🔶 가로휠 썸 동기·폭주 완화 완료. 마퀴 선택·자동스크롤 추가. 빈리스트/1행/리사이즈 최종 점검 잔여 |

XP SP3 호환 제약(불변): GDI/GDI+ 만, Vista+ API·폰트 금지.

> **진행 요약(2026-07-06 이어서)**: §5-1·§5-2 해결, 3·4단계 완료, 마퀴(rubber-band) 선택+자동스크롤 신규, 선택/체크 로직 DRY 리팩터, 가로 폭주 완화. 상세는 **§8**. (마지막 4개 항목 #1·#2·#4·#5 는 회사에서 GUI 검증 예정.)

---

## 4. 구현 상세 (완료분 — 코드 위치)

`SCListCtrl.cpp` / `.h` (라인은 대략치, 편집으로 이동 가능):

### 렌더링 (1단계)
- **`OnPaint`** overlay(`m_scrollbar_setup`) 분기 안, `m_smooth_scroll` 이면 `DefWindowProc` 대신 픽셀 루프:
  - `int first = m_scroll_y/rowH; int y = rc.top − (m_scroll_y%rowH);` → `for(i=first; i<total && y<area_bottom; i++,y+=rowH) draw_row(pDC, i, CRect(rc.left,y,rc.right,y+rowH));`
  - `pDC->IntersectClipRect(&rc)` 로 부분행이 헤더/코너 침범 방지. 헤더 child 는 `WS_CLIPCHILDREN` 로 별도 보존.
  - **폰트**: 루프 전 `pDC->SelectObject(GetFont())` — native 위임 시엔 native 가 폰트를 걸어주지만 여기선 직접 걸어야 시스템 기본폰트로 안 나옴. (셀별 styled font 는 draw_row 내부에서 별도 처리.)
- **`draw_row`**: DrawItem 에서 추출. `GetSubItemRect` 3곳(메인 셀 루프 + 선택 테두리 + 상/하단선) 뒤에 `rect.top=row_bounds.top; rect.bottom=row_bounds.bottom;` Y override.

### 스크롤 모델 (픽셀)
- **`sync_scrollbar`**: `else if (m_smooth_scroll)` 세로 모델 픽셀화 — `set_range(0, total*rowH−1); set_page(area_h); set_pos(m_scroll_y);` + `m_scroll_y` 를 `[0, content−area]` 로 **클램프**. 팬텀 pad 는 `!m_smooth_scroll` 일 때만(smooth 는 native 세로 스크롤 미사용 → pad=0).
- **`on_message_CSCScrollbar`** 세로 분기: smooth 면 `m_scroll_y = msg->pos; sync_scrollbar(); Invalidate();`
- **`OnMouseWheel`**: smooth 면 `m_scroll_y += 노치*3*rowH; sync; Invalidate;`
- **`OnNcCalcSize`**: smooth 면 remainder 예약 skip(`&& !m_smooth_scroll`) — 여백0 은 픽셀 페인트가 담당.

### 히트테스트·선택 (2단계)
- **`hit_test`**: smooth 면 후보 행 범위(`first=m_scroll_y/rowH`)와 `GetItemRect`/`GetSubItemRect` 의 Y 를 `header + i*rowH − m_scroll_y` 로 override → 픽셀 매핑.
- **`OnLButtonDown`**: smooth 면 native 위임 대신 직접 선택 — `hit_test` 로 행 구해 ctrl(토글)/shift(범위, `m_focus_anchor`)/plain(단일) `SetItemState`. (owner-data 도 SetItemState 로 선택상태·LVN_ITEMCHANGED 정상.)
- **`OnRButtonDown`**: smooth 면 직접 선택(표준 우클릭: 미선택이면 그것만, 이미 선택이면 유지) 후 return. 메뉴는 RButtonUp→WM_CONTEXTMENU→`OnContextMenu`(선택 안 건드림, parent 위임) 로 그대로 뜬다. **선택은 `OnRButtonDown` 이 담당**(OnContextMenu/OnNMRClick 아님 — 확인 완료).

---

## 5. 남은 문제 / 다음 착수점 (집에서 이어서)

> 상태 갱신(2026-07-06 이어서): **5-1 해결 / 5-2 해결 / 5-3 완료 / 5-4 검증 완료(progress 정상)**. 아래는 당시 기록 보존(원인·수정 방향 참고용). 실제 반영 내용은 §8.

### 5-1. ★버그: 오버플로 없을 때 항목 미표시 (최우선, 1줄 예상) — ✅ 해결(§8-1)
**증상**: 표시 항목 수가 리스트 높이를 **넘어야만**(세로바 생김) 항목이 보이고, 그 이하(오버플로 없음)면 리스트에 **아무것도 안 보임**. 창을 줄여 n개만 들어가게 하면 그때 표시됨.

**원인 가설(강함)**: `m_scroll_y` 클램프가 `sync_scrollbar` 의 `else if (m_smooth_scroll)`(= `need_v` true, 오버플로) 안에만 있다. 많은 항목 폴더에서 스크롤(`m_scroll_y` 큰 값) 후 항목 적은 폴더로 이동하면 `need_v` false → **클램프 미실행** → `m_scroll_y` 가 큰 값 잔류 → `OnPaint` 의 `first = m_scroll_y/rowH` 가 `total` 초과 → 루프가 한 행도 안 그림. 창 축소 → 오버플로 → 클램프 → 표시.

**수정 방향**: smooth 모드에서 `m_scroll_y` 를 **항상** `[0, max(0, total*rowH − area_h)]` 로 클램프. 최소침습은 `OnPaint` smooth 루프 직전에 클램프 한 줄 추가, 또는 폴더 변경(set_path)·데이터 리셋 시 `m_scroll_y=0`. sync_scrollbar 의 `if(!need_v)` 분기에서도 클램프(0으로)하도록 보강하는 것이 정석.

### 5-2. 가로휠 → 가로 스크롤바 미동기 (5단계) — ✅ 해결(§8-2)
가로 **드래그**·shift+세로휠은 정상. **가로휠(OnMouseHWheel)** 은 콘텐츠는 스크롤되나 **가로바 썸이 안 따라옴**. `OnMouseHWheel` 이 `m_h_scroll_pos += dx; sync_scrollbar();` 를 해서 바가 움직여야 정상인데 안 됨 → 측정 필요(추측 금지). 가로 통합(5단계)에서 처리.

### 5-3. 미착수 단계
- **3단계 키보드**: 방향키/PageUp·Down/Home·End 시 선택 이동 + `m_scroll_y` 조정으로 가시 유지, `EnsureVisible(item)` 을 `m_scroll_y` 로.
- **4단계 편집·툴팁·드래그**: 인라인 편집 박스 위치, 툴팁, 드래그 이미지 y 를 `m_scroll_y` 기준으로.
- **5단계**: 가로 통합(§5-2 포함), 빈 리스트/1행/리사이즈 엣지, 휠/썸 픽셀 정합 최종 점검.

### 5-4. 검증 안 된 렌더 경로
테스트가 shell 리스트(파일)라 **progress/percentage 컬럼**은 안 그려봄. `draw_row` 의 progress-text 는 `LPtoDP`+`SelectClipRgn` 을 쓰는데, smooth 의 MemoryDC 좌표계에서 이게 맞는지 미검증. progress 쓰는 리스트로 확인 필요.

---

## 6. 재현·검증 체크리스트 (다음 세션)
1. 테스트 실행 → 오른쪽 리스트, 항목 많은 폴더 → 휠·썸으로 바닥까지: **어느 창 높이에서도 마지막 항목 flush + 하단 여백0 + 맨 위 부분행**(1단계).
2. 여러 스크롤 위치·바닥 부분행 상태에서 좌/우클릭·Ctrl/Shift·더블클릭 폴더진입·메뉴: 클릭 행 정확(2단계).
3. **§5-1 재현**: 항목 많은 폴더에서 스크롤 후 항목 적은(오버플로 없는) 폴더로 이동 → 항목이 안 보이는지. → 클램프 수정 후 재확인.
4. 가로바: 드래그 OK, 가로휠 시 바 미동기(§5-2).

---

## 7. 관련 규칙·파일
- 네이밍 규칙: `Common/claude.md §1B`(폴더=클래스명, 파일=선두 C 제거).
- 여백/양자화 근본원인 상세: `Common/CListCtrl/CVtListCtrlEx/vscroll_problems.md`(원본 CVtListCtrlEx 조사 기록).
- 원본 대비 CSCListCtrl 은 팬텀 실측 pad fix + `[BLANKROW2]` 진단 로그 포함(원본은 pristine). smooth 완성 후 진단 로그 정리 예정.

---

## 8. 2026-07-06 이어서 작업 (상세)

집에서 3·4단계와 5단계 일부, 그리고 마퀴 선택을 진행. **#1·#2·#4·#5(§8-6)는 빌드만 통과, 회사에서 GUI 검증 예정.** 나머지(§8-1~8-5 앞부분)는 GUI 검증 완료.

### 8-1. §5-1 해결 — smooth 항목 미표시 (✅ GUI 검증)
`sync_scrollbar` 의 `if (!need_v)`/`else if (m_smooth_scroll)` 분기 **직전**에 smooth 모드 무조건 클램프 배치 — `m_scroll_y = clamp(0, total*rowH − area_h)`. 오버플로 없으면 자동 0. `else if` 안의 중복 클램프 제거. → 항목 많은 폴더 스크롤 후 적은 폴더 이동 시 리스트가 비던 문제 해결.

### 8-2. §5-2 해결 — 가로휠 가로 썸 동기 (✅ GUI 검증)
**측정 확정**: native 리스트뷰가 `WM_MOUSEHWHEEL` 을 자체 `WM_HSCROLL` 로 변환해 가로 스크롤 → 우리 `OnMouseHWheel` 우회 → `m_h_scroll_pos` 미갱신 → 썸 미동기. (`GetCurrentMessage` 로 curmsg 분포 측정: WM_HSCROLL 148 / WM_NOTIFY 144 / WM_MOUSEHWHEEL 1.)
**수정**: `OnHScroll`(native 자체 가로 스크롤에서만 진입)에서 `m_h_scroll_pos = GetScrollPos(SB_HORZ)` 미러링. 우리 자체 스크롤(`Scroll()`=LVM_SCROLL)은 OnHScroll 미진입이라 무영향.

### 8-3. 3단계 키보드 네비 (✅ GUI 검증)
`PreTranslateMessage` 에서 smooth 모드 방향키/Page/Home/End 를 `smooth_key_navigate(vk)` 로 직접 처리(처리 시 `return TRUE` 로 native 소비). 포커스 이동 + 선택(shift=범위, ctrl=포커스만, plain=단일) + `smooth_ensure_visible(item)`(m_scroll_y 픽셀 EnsureVisible, 스크롤 변경 시 Invalidate).

### 8-4. 4단계 인라인 편집·체크박스·드롭대상 Y (✅ 편집/체크 검증, 드롭 Y 는 회사 검증)
- **편집 박스 위치**: `edit_item` 에서 smooth 면 세로 `smooth_ensure_visible` + 가로 `ensure_column_visible(subItem)` 로 셀을 완전히 노출한 뒤, 행 Y 를 `header + item*rowH − m_scroll_y` 로 덮어 실제 그려진 행 위에 배치.
- **마우스 체크박스 토글 / 선택항목 재클릭 편집진입**: smooth `OnLButtonDown` 이 native 우회라 `NM_CLICK`(→`OnNMClick`)이 안 뜸 → 합성 `NMITEMACTIVATE{ptAction=클릭좌표}` 로 `OnNMClick` 직접 호출(내부 hit_test smooth-aware). **후처리 로직의 단일 출처는 OnNMClick**(중복 금지).
- **드래그&드롭 드롭 대상 Y**(#4, 회사 검증): 드래그 중 커서 아래 항목을 native `HitTest` 대신 대상 리스트의 smooth-aware `hit_test` 로 판정 → 드롭 하이라이트/드롭 위치가 픽셀 페인트와 정합. ("드래그 이미지 Y" 의 실체는 레이어드 이미지가 아니라 **드롭 대상 판정**이었음 — 이미지는 커서를 screen 좌표로 따라가 무관.)
- **툴팁**: 이 컨트롤에 툴팁 코드 없음 → 해당 없음.

### 8-5. 마퀴(rubber-band) 선택 (✅ 기본 동작 검증 / #1 자동스크롤은 회사 검증)
- 빈 공간(항목 아래 + **맨 우측 컬럼 오른쪽**) 눌러 드래그 → 사각형에 겹치는 항목 선택. 아이템 드래그(`m_bDragging`)와 독립.
- **빈 영역 판정은 `hit_test` 로 일원화**: 컬럼 총폭(`Σ GetColumnWidth − GetScrollPos(SB_HORZ)`) 오른쪽은 `item=-1` 반환(탐색기와 동일). `itemRect.right = min(rc.right, col_right)`.
- 선택 판정: 세로 겹침 **+ 컬럼 영역([left, col_right])과 가로 겹침**(우측 빈 영역만 걸친 동안은 선택 안 됨, col_right 왼쪽으로 넘어오는 순간부터 선택).
- 렌더: `OnPaint` 두 경로(smooth 루프 후 / native `DefWindowProc` 후)에 반투명 사각형. 행 좌표 `row_screen_rect`(smooth=m_scroll_y, native=GetItemRect)로 두 모드 공용.
- **#1 자동 스크롤**(회사 검증): `update_marquee_auto_scroll`(거리비례 가속, MARGIN=48/MAX_LEVEL=3) + `TIMER_ID_DRAG_AUTO_SCROLL`. tick 마다 스크롤 후 **기준항목(0)의 화면 이동량으로 마퀴 시작 앵커를 콘텐츠에 고정**(smooth/native 공용) → 커서가 멈춰 있어도 드러나는 항목까지 계속 선택.

### 8-6. 회사에서 검증할 항목 (2026-07-06 마지막 4건)
1. **#1** 마퀴 자동 스크롤(상/하단 가장자리 → 연속 스크롤+선택).
2. **#2** Ctrl+↑↓ 포커스 점선 사각형: `draw_row` 에 **선택 안 된 포커스 항목**에 점선(`DashStyleDot`) — 선택 유지한 채 포커스만 이동하는 게 보이게.
3. **#4** 드래그&드롭 드롭 대상 Y(§8-4 마지막).
4. **#5** 가로 스크롤 폭주 완화: native hwheel→다수 WM_HSCROLL 매 스텝 full `sync_scrollbar`(오버레이 재배치+동기 RedrawWindow) 과함 → `OnHScroll` 에서 `GetTickCount` **~30ms 스로틀 + 버스트 종료 후 `TIMER_ID_HSYNC_TRAILING`(40ms) 로 최종 위치 1회 반영**. 빈도만 줄이고 sync 동작은 그대로라 정확성 불변.

### 8-7. 중복 제거(DRY) 리팩터 — 사용자 강한 피드백
smooth 분기마다 native 로직을 복붙하지 말 것(사용자 지적). 원자연산으로 추출해 mode·입력원이 공유:
- **선택**: `select_single` / `select_range` / `select_range_add`(Ctrl+Shift) / `toggle_item_select` — `OnLButtonDown`·`OnRButtonDown`·`smooth_key_navigate` 공유(정책만 각자).
- **체크박스 토글**: `toggle_check`(기존 `set_check`/`get_check` 재사용) — 키보드 Space·마우스 공유. Space 는 다중 선택 전체 토글.
- **행 높이**: `row_height()` 헬퍼로 삼항식 9곳 통일.
- **원칙**: 본질적으로 다른 알고리즘(픽셀 vs 항목단위 스크롤 모델)은 분리 유지 OK — 중복은 *같은 로직의 복사*만 지칭.

### 8-8. 상태 변경 시 재그리기 (smooth 회귀 방지)
smooth 는 선택 하이라이트를 우리가 픽셀 위치로 그리므로 native 의 per-item 무효화(native 좌표)로는 잔상이 남는다. `OnLvnItemchanged`(단일)·`OnLvnOdstatechanged`(전체 -1)에서 smooth 면 `Invalidate(FALSE)` → OnPaint 로 올바르게 재그리기. (더블버퍼라 다중 SetItemState 도 WM_PAINT 1회로 합쳐져 깜빡임 없음.)

### 8-9. 진단 로그 상태
- 유지(주석 아님): `[smooth-nav]`(키 네비, 저빈도).
- 주석 처리(폭주 노이즈, §2J): `[hscroll]` / `[hsync]` / `[sync-caller]` / `[hwheel]`·`[shiftwheel]`·`[wheel-entry]`·`[hwheel-entry]`.
- `[BLANKROW2]`(팬텀 진단) 유지.
- CSCListCtrl 은 개발단계 포크(타 프로젝트 미사용)라 SCLog 포함 push 예외 허용(사용자 확인).

### 8-10. 관련 커밋 (Common, 2026-07-06)
- `a0bd2f1` smooth 항목 미표시(§8-1) + 가로휠 썸 동기(§8-2)
- `2e26634` 키보드 네비·인라인 편집·체크박스 + 선택/체크 DRY(§8-3·8-4·8-7)
- `11d5464b` 편집 진입 전 셀 가시화 + smooth 세로 스크롤 재그리기
- `b42349f3` 마퀴 선택(§8-5) + hit_test 빈영역 일원화
- `b9acdde6` 마퀴 자동스크롤(#1) + 포커스 표시(#2) + 드롭대상 Y(#4) + 가로 폭주 완화(#5)
