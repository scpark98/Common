# CSCColorTheme — 새 color theme 추가/제거 가이드

`Common/colors.h` / `Common/colors.cpp` 의 `CSCColorTheme` 에 color theme 을 추가·제거할 때
반드시 손대야 하는 위치와 순서를 정리한다. 세 곳(enum·이름 테이블·switch case)이
`static_assert` 와 `ASSERT(id==i)` 로 묶여 있어, 하나라도 빠뜨리면 컴파일 또는 디버그
실행 시 즉시 잡힌다.

---

## 0. 두 가지 정의 방식

새 테마의 색을 정하는 방법은 두 가지다.

### (A) seed 3색 생성기 — 권장

배경/본문/강조 **3색만** 주면 나머지 40여 필드가 자동 파생된다.

```cpp
set_theme_from_seed(bg, text, accent);
//  bg     : 배경. light/dark 는 이 색의 luma 로 자동 판정 → 파생 방향(밝게/어둡게)이 뒤집힌다.
//  text   : 본문 글자. dim/disabled/title/header 텍스트가 여기서 파생.
//  accent : 강조색(선택 배경 · focus/selected border · primary 버튼). progress 바는 말미 자동 산출.
```

- 내부적으로 검증된 `set_theme_from_editor_palette(bg, fg, sel_bg, header_fg, header_bg)`
  를 재사용한다(header 2색은 bg/text 에서 자동). 그 위에 primary 버튼만 accent solid 로 얹는다.
- Notepad++ 등에서 추출한 **5색 팔레트**가 있으면 `set_theme_from_editor_palette` 를 직접 써도 된다.

### (B) 수동 — 필드 개별 지정

특수한 테마(signature 색이 촘촘히 정해진 브랜드 등)는 각 `cr_*` 필드를 직접 대입한다.
`dark`, `claude`, `linkmemine` 등이 이 방식. 손이 많이 가고 "복사 후 일부만 수정 → 나머지
stale" 사고가 나기 쉬우므로, 특별한 이유가 없으면 (A) 를 쓴다.

> 대부분의 브랜드 테마는 실제로 **accent 1색**만 다르다. bg=흰/검, text=검/흰 인 경우가 많다.

---

## 1. 추가 — 손대는 3곳(+1)

새 테마 이름을 `color_theme_myname` 이라 하자.

### 1-1. enum 추가 — `colors.h`

`enum SC_COLOR_THEMES` 에 값을 추가한다.

```cpp
enum SC_COLOR_THEMES
{
    ...
    color_theme_myname,     // 주석: bg/text/accent 또는 용도
    ...
    color_theme_custom,             // ← custom/popup_folder_list 는 항상 끝쪽 유지
    color_theme_popup_folder_list,
};
```

- **위치**: 콤보에서 비교하고 싶은 기존 테마 바로 뒤에 넣으면 up/down 으로 나란히 볼 수 있다.
  (예: `seed_dark` 는 `dark_gray` 뒤, `seed_claude`·`figma` 는 `claude` 뒤에 배치.)
- **인덱스 밀림 주의**: 중간에 삽입하면 뒤 테마의 enum 값이 +1 밀린다. **영속화는 이름 기반**
  (`get_theme_name`/`get_theme_index`)이라 저장값은 안 깨지지만, `custom`/`popup_folder_list`
  는 내부용이라 밀려도 무방하다. → 그래서 이 둘은 항상 enum 맨 끝에 둔다.

### 1-2. 이름 테이블 추가 — `colors.cpp` (`g_sc_theme_table`)

```cpp
{ CSCColorTheme::color_theme_myname,  _T("myname") },
```

- **테이블 순서 = enum 값 순서여야 한다.** `get_color_theme_list` 가 이 테이블 순서대로
  콤보를 채우고, 콤보 인덱스를 그대로 `set_color_theme(index)` 에 넘기기 때문.
  → enum 에서 넣은 위치와 **같은 자리**에 테이블 항목도 넣는다.
- `static_assert(_countof(g_sc_theme_table) == color_theme_XXX + 1, ...)` 이 개수를,
  `ASSERT(g_sc_theme_table[i].id == i)` (Debug) 이 순서를 검증한다.
  마지막 enum 값이 바뀌면 `static_assert` 의 `color_theme_XXX` 도 그 마지막 값으로 맞춘다.
  (seed 처럼 **중간 삽입**이면 테이블 마지막 항목은 그대로이므로 static_assert 는 안 바꿔도 된다.)

### 1-3. switch case 추가 — `colors.cpp` (`set_color_theme`)

```cpp
case color_theme_myname:
    set_theme_from_seed(gRGB(0xFF,0xFF,0xFF), gRGB(0x1E,0x1E,0x1E), gRGB(0x0D,0x99,0xFF));
    break;
```

- case 는 **값 매칭**이라 위치·순서 무관. 관련 테마끼리 모아두면 읽기 편하다.
- `progress` 바 색(`cr_progress_active`/`cr_progress_active_selected`)은 **여기서 지정하지 않으면**
  `set_color_theme` 말미에서 배경 대비로 **자동 산출**된다(미지정 = Transparent). 브랜드 accent 를
  progress 에 강제로 쓰고 싶을 때만 명시.

### 1-4. (선택) 타이틀바 폰트 — `CDialog/SCThemeDlg/SCThemeDlg.cpp`

`CSCThemeDlg` 를 쓰는 다이얼로그의 타이틀바 폰트를 테마별로 다르게 하려면 `switch(theme)`
에 case 추가. 대부분은 **불필요**(안 넣으면 `default` 로 떨어져 표준 폰트). claude 계열처럼
특정 폰트 크기가 필요할 때만 기존 case 에 `case color_theme_myname:` 를 fall-through 로 얹는다.

---

## 2. 제거

추가의 역순으로 **같은 3곳**에서 지운다.

1. `colors.h` enum 에서 `color_theme_myname,` 줄 삭제.
2. `colors.cpp` 이름 테이블에서 해당 `{ ..., _T("myname") }` 줄 삭제.
3. `colors.cpp` switch 에서 해당 `case color_theme_myname:` 블록 삭제.
4. `SCThemeDlg.cpp` 에 case 가 있었으면 삭제(fall-through 였으면 그 줄만).

주의:
- 중간 제거 시 뒤 enum 값이 당겨진다. **영속화가 이름 기반이라** 안전하지만, enum 값을
  registry 등에 **정수로 저장**하는 코드가 있으면 그 앱은 깨진다(현재 앱들은 signature 테마를
  코드에 하드코딩해 써서 영향 없음).
- 제거한 테마의 색 값은 **git 이력에 영구 보존**된다. 나중에 되살리려면 그 커밋에서 값을 복원.
- 빌드하면 `static_assert`(개수)와 Debug `ASSERT(id==i)`(순서)가 3곳 동기화를 자동 검증한다.

---

## 3. 검증 체크리스트

- [ ] enum·이름테이블·case 3곳 모두 반영(또는 3곳 모두 제거).
- [ ] 이름 테이블 순서 = enum 순서(중간 삽입/삭제 시 위치 일치).
- [ ] 빌드 성공(= static_assert 통과). Debug 실행 시 `get_color_theme_list` 의 `ASSERT(id==i)` 통과.
- [ ] `.cpp/.h` 는 UTF-8 BOM 유지(한글 주석 깨짐 방지).
- [ ] GUI 로 콤보에서 새 테마 선택 → 전 컨트롤이 정합되게 표시되는지 시각 확인.
      (테마는 개별 컨트롤 색 지정 없이 `set_color_theme(m_theme)` 전파만으로 채색되는 게 정상.)

---

## 4. 실제 예시 — Figma 테마 추가(seed 방식)

getdesign.md/figma 의 BRAND & ACCENT: Black `#000000`, White `#FFFFFF`, Magenta `#FF3D8B`.

```cpp
// (1) colors.h — enum (claude 뒤에 배치)
color_theme_figma,

// (2) colors.cpp — 이름 테이블 (같은 자리)
{ CSCColorTheme::color_theme_figma,  _T("figma") },

// (3) colors.cpp — switch case
case color_theme_figma:
    set_theme_from_seed(gRGB(0xFF,0xFF,0xFF), gRGB(0x00,0x00,0x00), gRGB(0xFF,0x3D,0x8B));
    break;

// (4) SCThemeDlg.cpp — claude 계열과 동일 titlebar 처리(선택)
case CSCColorTheme::color_theme_figma:
    ... (기존 claude case 에 fall-through)
```

3색만 정하면 선택 배경·hover·border·header·title·button·progress·separator·gridlines·
inactive 선택(저채도)·disabled 가 전부 자동으로 채워진다.

---

## 5. 참고 — 자동 파생 헬퍼(colors.cpp)

새 테마를 손댈 때 알아두면 좋은, 이미 있는 파생 규칙들:

- `set_theme_from_seed(bg, text, accent)` — 3색 → 전체.
- `set_theme_from_editor_palette(bg, fg, sel_bg, header_fg, header_bg)` — 5색 팔레트 → 전체.
- `compute_text_dim(text, back)` — dim/disabled 텍스트(WCAG 대비 보정).
- `get_readable_text_color(back, preferred, min)` — 배경 위 가독 글자색(preferred 통과 시 유지, 아니면 흑/백).
- `auto_progress_color(back)` — progress 바 자동색(배경 명도 반대로 한 톤).
- `get_leveled_color(cr, level)` — `set_theme_level` 의 테마 강도 슬라이더(흰색 기준 거리 배율).
- `cr_gridlines` / `cr_back_selected_hover` — set_color_theme 말미에서 배경 파생.
