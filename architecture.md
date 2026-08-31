# Common 라이브러리 모듈 지도

모든 프로젝트가 재사용하는 Common 모듈의 **빠른 참조 맵**. 규칙이 아닌 *인벤토리*이므로 `claude.md` 와 분리한다. 모듈 세부를 파악할 때 먼저 이 파일을 보고, 필요한 헤더만 Read 로 확인한다.

경로는 전부 `D:\1.projects_c++\Common\` 기준 상대.

---

## 1. 이미지 렌더링 코어

### `CDialog/SCD2ImageDlg/SCD2ImageDlg.h` — `CSCD2ImageDlg` (CDialog)
이미지 뷰어 본체. Direct2D 기반. ASee 의 `m_imgDlg`.

- **멤버**: `m_zoom` (double), `m_fit2ctrl` (bool), `m_offset` (CPoint), `m_simple_mode` (bool, 기본 true), `m_image_roi`/`m_screen_roi` (RectF), `m_thumb` (CSCThumbCtrl), `m_slider_gif` (CSCSliderCtrl), `m_images` (deque<CSCD2Image>).
- **API**: `create(parent, x, y, cx, cy)`, `load(file, load_thumbs, auto_play)`, `zoom(int mode | double ratio)`, `fit2ctrl(bool)`, `display_image(idx)`, `play/pause/stop()`.
- **레지스트리** `setting\CSCD2ImageDlg`: `zoom ratio`, `fit to ctrl`, `show info`, `show pixel`, `show pixel_pos`, `show cursor guide line`, `show roi info`, `image roi`, `interpolation mode`, `recent file`.
- **주의**:
  - `set_simple_mode()` 는 `create()` 호출 **전**에만 적용.
  - `m_images` 는 cur(0) 기준 ±5 프레임 버퍼링.
  - GIF: `CSCD2Image::thread_animation()` 이 parent 에 메시지 전송 → parent 가 `Invalidate()`.
  - 프로그램 시작 시 `fit2ctrl` 호출로 저장값을 덮어쓰는 버그 이력 (2026-04-21 수정).

### `directx/CSCD2Context/SCD2Context.h` — `CSCD2Context`
Direct2D 팩토리/디바이스/SwapChain 관리 엔진.

- **멤버**: `m_d2factory` (ID2D1Factory1), `m_d2device`/`m_d2context`, `m_WICFactory` (IWICImagingFactory2), `m_swapchain`, `m_br_zigzag` (투명 PNG 배경 패턴).
- **API**: `init(hWnd, cx, cy)`, `init(hWnd, pShared, ...)` (공유), `get_d2dc()`, `get_WICFactory()`, `on_size_changed(cx, cy)`.
- **주의**: COM 포인터는 `GetAddressOf()` 사용. MFC 컨트롤과 혼용 시 Clip Children 필수.

### `directx/CSCD2Image/SCD2Image.h` — `CSCD2Image`
Direct2D 이미지 데이터/애니메이션.

- **멤버**: `m_img` (deque<ID2D1Bitmap1>), `m_frame_index`, `m_frame_delay` (deque<int> ms), `m_exif_info` (CSCEXIFInfo), `m_ani_thread` (CSCThread), `m_parent`.
- **API**: `load(WIC, d2dc, path, auto_play)`, `load(WIC, d2dc, data, w, h, ch)`, `draw(d2dc, ...)`, `play/pause/stop()`, `goto_frame(idx, pause)`, `get_pixel(x,y)`, `copy_to_clipboard()`, `paste_from_clipboard()`, `save(path, quality)`.
- **주의**: `total_frame > 1` 이면 load 후 자동 play. `set_parent()` 안 하면 GIF 메시지 미수신. PBGRA↔RGBA 변환 유의.

### `SCGdiplusBitmap.h` — GDI+ 경로 전용
Direct2D 아닌 GDI+ 기반 이미지. 독립 모듈.

- **멤버**: `m_pBitmap` (Gdiplus::Bitmap*), `m_frame_count`/`m_frame_index`, `m_data` (uint8_t*), `m_bright`/`m_contrast` (100%=중립), `width/height/channel/stride`.
- **API**: `load(file | type,id)`, `draw(Graphics&, rect, mode)`, `resize`, `rotate(deg, auto_resize)`, `mirror/flip`, `blur(sigma)`, `adjust_bright/contrast(%)`, `set_animation(hwnd, ...)`, `play_gif/pause/stop`, `save(path, quality)`, `copy_to_clipboard`, `paste_from_clipboard`.
- **주의**:
  - **`CGdiplusDummyForInitialization` 전역이 GDI+ 자동 초기화** — `GdiplusStartup/Shutdown` 직접 호출 금지.
  - `m_pOrigin` 이 원본 유지 — 반복 조정의 누적 오차 방지.
  - `set_gif_play_itself(false)` 로 설정하면 로드만 하고 재생은 caller (CSCD2ImageDlg 용).

### `ThumbCtrl/SCThumbCtrl.h` — `CSCThumbCtrl`
타일 썸네일 브라우저.

- **멤버**: `m_thumb` (deque<CThumbImage>), `m_files`, `m_selected` (deque<int>), `m_sz_thumb` (100x120 기본), `m_thread` (CSCThreadGroup), `m_scroll_pos`.
- **API**: `create(parent, l, t, r, b)`, `set_path(folder)` (자동 스캔), `add_files`, `insert(idx, path, title, keyThumb)`, `remove(idx)`, `remove_selected`, `select_item(idx, sel, make_visible)`, `find_text`, `sort_by_title/score`, `get_selected_item(s)`.
- **의존**: `CSCGdiplusBitmap` (썸네일), `CSCThreadGroup` + `CSCThread` (병렬 로드, 안전 종료), `CSCEdit` (타이틀 인라인 편집), `CSCColorTheme`.
- **주의**: 종료 시 `stop_loading()` 호출. 로딩 중 `remove()` 호출 충돌 가능.

---

## 2. 컬러 시스템 (`colors.h`, `colors.cpp`)

### 매크로
- `gRGB(r,g,b)` → `Gdiplus::Color(255, r, g, b)` — **표준 RGB 대신 이걸 기본 사용** (§2.1).
- `gGRAY(x)`, `GRAY(x)` (COLORREF), `GRAY_FACE`.
- `RGB2Y/U/V`, `YUV2R/G/B`, `rgb_bgr(c)`, `color_complementary(c)`.

### `CSCColorTheme`
컨트롤마다 `m_theme = CSCColorTheme(this)` 로 보유. 개별 `m_cr_*` 멤버 대신 이걸 쓴다.

**필드 (자주 쓰는 것)**:
- 텍스트: `cr_text`, `cr_text_dim`, `cr_text_hover`, `cr_text_selected`, `cr_text_selected_inactive`, `cr_text_dropHilited`.
- 배경: `cr_back`, `cr_back_hover`, `cr_back_selected`, `cr_back_selected_inactive`, `cr_back_alternate`, `cr_back_dropHilited`, `cr_parent_back`.
- 경계: `cr_border_active`, `cr_border_inactive`, `cr_selected_border(_inactive)`.
- 편집: `cr_edit_text`, `cr_edit_back`.
- 타이틀/시스템버튼: `cr_title_text`, `cr_title_back_active`, `cr_sys_buttons_hover_back`, `cr_sys_buttons_down_back`.
- 헤더/프로그레스: `cr_header_text/back`, `cr_progress_active`, `cr_percentage_bar[]`.
- 상태: `cr_success`, `cr_info`, `cr_warning`, `cr_error`.

**테마 enum `SC_COLOR_THEMES`**:
`color_theme_default` (시스템), `color_theme_white`, `color_theme_gray`, `color_theme_dark_gray`, `color_theme_dark` (RGB 37,37,38), `color_theme_linkmemine`, `color_theme_linkmemine_se`, `color_theme_anysupport`, `color_theme_helpu`, `color_theme_pcanypro`, `color_theme_custom`, `color_theme_popup_folder_list`.

**메서드**: `set_color_theme(int)`, `get_color_theme()`, `static get_color_theme_list(deque<CString>&)` (popup_folder_list 제외).

**변환 유틸**:
- `RGB2gpColor(cr, alpha=255)` — COLORREF → Gdiplus::Color.
- `get_color(COLORREF)` 오버로드, `get_color(cr1, cr2, ratio)` 보간.
- `get_sys_color(index)` → Gdiplus::Color.
- HSV/HSL 변환: `hsv2rgb`, `gcolor_to_hsv`, `hsl_to_gcolor`, `gcolor_to_hsl`.
- 유틸: `get_gray_value/color`, `get_complementary_color`, `get_distinct_bw_color`, `get_random_gcolor`, `get_color_hexa_str`.
- `Colors enum` 148개 웹컬러, `CSCColorList`(name↔rgb 대소문자 무시), `g_cr` unordered_map.

---

## 3. 커스텀 컨트롤

공통: 모두 `m_theme = CSCColorTheme(this)`. setter 는 snake_case (`set_text_color`, `set_back_color`, `set_border_color`, `set_font_size`). ON_WM_* 는 MFC 관례 유지, ON_MESSAGE 커스텀은 snake_case (`on_ime_composition` 등).

| 헤더 | 기반 | 특징 |
|---|---|---|
| `CStatic/SCStatic/SCStatic.h` | CStatic | 그라디언트, 라운드, PNG/GIF 아이콘, 인라인 편집(`set_use_edit`). 커스텀 메시지 `Message_CSCStatic`. |
| `CEdit/SCEdit/SCEdit.h` | CEdit | **`CSCColorTheme` 사용 대표 예시**. border active/inactive, dim text (placeholder), action button (search icon — parent 에 `message_scedit_action_button_down/up`), readonly 전용 배경색. |
| `CEdit/CSCStaticEdit/SCStaticEdit.h` | **CStatic** (Edit 아님) | Static 으로 배치되지만 Edit 처럼 동작. OnPaint 완전 커스텀. IME: `on_ime_start_composition_message`, `on_ime_end_composition`, `on_ime_composition` (ON_MESSAGE, snake_case). password masking, undo/redo, 드래그 선택 자동 스크롤. |
| `CSliderCtrl/SCSliderCtrl/SCSliderCtrl.h` | CSliderCtrl | 9+ 스타일 (normal/thumb/progress/track/step), 북마크, N-stop 그라디언트. 커스텀 메시지 `Message_CSCSliderCtrl` + `CSCSliderCtrlMsg{msg, pos}` 구조체. msg enum: `msg_thumb_grab/move/release/track_bottom_slide`. 이벤트 전송 스타일 timer/post/callback 선택. |
| `CButton/SCSystemButtons/SCSystemButtons.h` | CButton | 타이틀바 minimize/maximize/close/pin/help/custom 복합. `create(parent, top, right, w, h, SC_PIN, SC_MINIMIZE, ...)` 가변 템플릿. parent 에 SC_MINIMIZE/MAXIMIZE/CLOSE(표준) + SC_PIN/SC_HELP(커스텀, WM_USER+) 전송. `CSCSystemButtonsMessage{cmd}`. |
| `CScrollbar/SCScrollbar/SCScrollbar.h` | CWnd | 자체 그리기 스크롤바 (OS native 스크롤바 색 못 바꾸는 한계 우회, XP 호환). `create(parent, vertical/horizontal, x,y,cx,cy)`. 모델 push: `set_range/page/pos/line`, 보조 `scroll_by_lines/pages`. host 가 `Message_CSCScrollbar` 수신 — `CSCScrollbarMsg{msg, pos}` 의 msg = `msg_scrollbar_pos_changed/drag_start/drag_end`. thumb hover/pressed 색은 `cr_back ↔ cr_text` ratio 자동 derive — theme 무관. `set_show_arrows(false)` 기본 (modern minimal). |
| `CComboBox/SCComboBox/SCComboBox.h` | CComboBox | 테마 적용 콤보. 드롭다운 리스트까지 자체 그리기. |
| `CStatic/PathCtrl/PathCtrl.h` | CStatic | 탐색기 주소표시줄 스타일 경로 표시·이동. 세그먼트 클릭으로 상위 폴더 이동. Border 는 리소스 속성으로 켠다 — `OnPaint` 입구의 early return 때문에 path 미설정 상태에서도 그려지는지 확인할 것. |
| `CStatic/CSCStepCtrl/SCStepCtrl.h` | CStatic | 단계 진행 표시(step indicator). `set_step_count`, `set_pos`, `step()`, `set_thumb_style(index, style)`, `set_text(index, text, cr)`. 가로/세로는 `set_style(is_horz, thumb_style)`. |
| `CIPAddressCtrl/SCIPAddressCtrl/SCIPAddressCtrl.h` | CIPAddressCtrl | 표준 컨트롤이 안 주는 **Return 키 / KillFocus 이벤트**를 parent 에 전달. `Message_CSCIPAddressCtrl`. |
| `CToolTipCtrl/CSCToolTipCtrl/SCToolTipCtrl.h` | CToolTipCtrl | 태그 서식(`CSCParagraph`) 툴팁 — `CSCStatic` 과 서식 결과가 항상 일치한다. `set_padding/max_width/round/line_spacing/fade`. `Create` override 가 `TTS_NOPREFIX` 를 항상 붙인다(없으면 문자열의 `&` 가 사라진다). **사용자 데이터를 넣을 땐 `static escape_tags()` 필수** — 데이터의 `<` 가 태그로 파싱된다. `relay_message(MSG*)` 는 top-level dlg 의 PreTranslateMessage 에서 호출(그래야 disabled 컨트롤에서도 뜬다). |

### 3.1 목록·트리 컨트롤

셋 다 **native 레이아웃을 쓰지 않고** 직접 그리며, `CSCScrollbar` 오버레이 + NC 테두리 인프라
(`setup_scrollbar` / `sync_scrollbar` / `OnNcCalcSize` / `OnNcPaint`)를 **각자 따로** 갖고 있다.
합계 약 1,000줄이 거의 같은 코드다 — 이 영역의 버그는 세 곳을 고쳐야 한다(공통화 미착수).

| 헤더 | 기반 | 특징 |
|---|---|---|
| `CListCtrl/CSCListCtrl/SCListCtrl.h` | CListCtrl | `CVtListCtrlEx` 의 후계. **virtual list 단일 모드** — 데이터는 `m_list_db`(deque)가 갖고 `draw_row` 가 직접 그린다. 리소스에서 **Owner Data + Owner Draw Fixed 필수**(동적 변경 불가). `InsertItem`/`SetItemText` 대신 `insert_item`/`set_text` 계열을 쓸 것. 항목 Y 는 native 스크롤이 아니라 `header + i*row_height - m_scroll_y`(픽셀) 이며 이 산식이 20여 곳에 흩어져 있다. 셀별 색·폰트, 정렬, 인라인 편집, 마퀴 선택, 드래그&드롭, shell 모드(`set_as_shell_listctrl`), progress 셀, 잘린 셀 툴팁(`set_use_ellipsis_tooltip`). 동반 파일: `SCHeaderCtrl.h`(`CSCHeaderCtrl`), `list_data.h`. |
| `CTreeCtrl/SCTreeCtrl/SCTreeCtrl.h` | CTreeCtrl | `OnNMCustomDraw` 로 전면 자체 그리기. chevron 은 **GDI+ 벡터로 직접** 그린다(XP 에 아이콘 폰트가 없다). 폴더 트리(`CSCTreeCtrlFolder`), 인라인 편집, 드래그&드롭 + 자동 확장/스크롤, node count 배지, 잘린 라벨 툴팁. |
| `CListBox/SCListBox/SCListBox.h` | CListBox | `DrawItem` 자체 그리기. 거터(줄 번호), 가로 오버레이 스크롤. 큰 글씨는 GDI+ 로 그려 CJK 내장 비트맵 폰트의 톱니를 피한다(`m_text_smooth`). |
| `CListCtrl/CVtListCtrlEx/VtListCtrlEx.h` | CListCtrl | **구 버전.** `CSCListCtrl` 의 원본이며 아직 쓰는 프로젝트가 있어 남아 있다. 신규 코드는 `CSCListCtrl` 을 쓴다. |

### 3.2 메뉴·툴바·타이틀바

| 헤더 | 기반 | 특징 |
|---|---|---|
| `CMenu/CSCMenuBar/SCMenu.h` | — | `CSCMenu` / `CSCMenuItem`. 자체 그리기 팝업 메뉴. 리소스 메뉴(`load_from_menu`)에서 nested 까지 자동 구성. 아이콘·서브 버튼·체크. 트리거를 다시 눌러 닫는 토글은 per-instance `m_dismiss_tick` 디바운스로 **모듈 안에서** 처리한다(사용처마다 반복하지 않도록). |
| `CMenu/CSCMenuBar/SCMenuBar.h` | CDialogEx | 메뉴 버튼 줄. `init(parent, resource_menu_id, ...)`, `set_icon_and_buttons(...)` 가변 템플릿. `set_color_theme` 은 int / `const CSCColorTheme&` 두 가지. |
| `CMenu/CSCMenuBar/SCMenuButton.h` | CGdiButton | 메뉴바의 버튼 하나 + 거기 달린 `CSCMenu`. |
| `CToolBar/SCToolBar/SCToolBar.h` | CDialogEx | `init(parent, x, y, w, h)` → `add(caption, resource_id, button_type)` → `create()`. |
| `ui/CSCTitleBar/SCTitleBar.h` | — | 창이 아니라 **그리기 헬퍼**. `draw(CDC*/Graphics*, rc)` 로 호스트가 자기 영역에 타이틀바를 그린다. |
| `CButton/GdiButton/GdiButton.h` | CButton | 이미지·라운드·hover/down 파생색 버튼. `Message_CGdiButton` 로 parent 에 통지. SC 계열 버튼의 실질 베이스. |

### 3.3 그 밖의 컨트롤·다이얼로그

| 헤더 | 기반 | 특징 |
|---|---|---|
| `messagebox/CSCMessageBox/SCMessageBox.h` | CDialogEx | 테마 적용 메시지 박스. 본문에 `CSCParagraph` 태그 사용 가능. modeless 로도 띄운다. |
| `CDialog/SCPropertyCtrl/SCPropertyCtrl.h` | CDialogEx | 속성 편집 패널. `begin()` → `section()` / `begin_row()` / `add(&변수, label, ...)` / `add_info()` → `end()` 로 **변수에 직접 바인딩**해 자식 컨트롤을 생성. 외부에서 값이 바뀌면 `refresh()`. |
| `CDialog/CSCHeatmapCtrl/SCHeatmapCtrl.h` | CDialogEx | 히트맵 격자(`CSCHeatmapCell`). |
| `ControlSplitter.h` | CButton | 컨트롤 사이를 드래그로 나누는 스플리터. `set_type(CS_VERT/CS_HORZ)`, `AddToTopOrLeftCtrls` / `AddToBottomOrRightCtrls` / `AddToBoth` 에 `SPF_*` 로 어느 변이 따라올지 지정. `get/set_split_offset` 으로 위치 저장·복원. 이동은 `DeferWindowPos` + `SWP_NOCOPYBITS` + `RDW_UPDATENOW` **한 묶음** — 셋 중 하나만 빠져도 빠른 드래그에서 잔상이 남는다. `set_live_resize_hook` 은 선택(리스트/트리를 드래그 동안 재우려면 app 이 연결). |

### 정책 — CSCScrollbar overlay 컨트롤의 단일 결정자 원칙

`CVtListCtrlEx`, `CSCTreeCtrl`, `CSCListBox`, `SCThumbCtrl` 등 CSCScrollbar 오버레이를 사용하는 컨트롤은 **CSCScrollbar 가 스크롤 표시의 단독 결정자**다. 다음을 지킨다:

- **`LVS_EX_FLATSB` 사용 금지.** FlatSB 모듈이 native scrollbar 비트를 시각화하면 overlay 와 충돌해 "컬럼 폭 합이 줄어도 가로 scrollbar 가 남아 있고, 드래그하면 사라지는" 종류의 sticky scrollbar 현상이 생긴다. 컨트롤 자체 `SetExtendedStyle` 호출에서도 제거, 사용자(다이얼로그) 측 호출에서도 제거.
- **scroll 비트 자동 set 차단.** listview 등은 컬럼/내용 합이 client 를 넘는 순간 자체적으로 `WS_HSCROLL`/`WS_VSCROLL` 을 set 하고 줄어도 떼지 않는다. `sync_scrollbar()` 류의 단일 sync 함수 진입부에서 `ModifyStyle(WS_VSCROLL | WS_HSCROLL, 0)` 으로 매번 비트를 제거한다.
- **컬럼/내용 변경의 single entry point.** 컬럼 폭/항목 수가 바뀌는 모든 경로 — 초기화, registry 복원, 자동 맞춤, 고정폭 컬럼, **사용자 헤더 manual drag/double-click**(`HDN_ENDTRACK` / `HDN_DIVIDERDBLCLICK` reflected) — 가 마지막에 `sync_scrollbar()` 를 호출해야 한다. `SetColumnWidth` 직접 호출은 각 컨트롤 내부 단일 함수(`set_column_width`)에만 두고 나머지는 그 함수를 경유.

KoinoTools 2026-06-02 적용 완료 (`VtListCtrlEx.cpp` / `KoinoToolsDlg.cpp`).

---

## 4. 다이얼로그·컬러피커

### `CDialog/SCShapeDlg/SCShapeDlg.h` — `CSCShapeDlg` (CDialogEx)
비정형 모양 다이얼로그. IDD 불필요. 알파 채널 기반 Region + WS_EX_LAYERED.
- API: `create(parent, l, t, r, b)`, `set_image(parent, CSCGdiplusBitmap*, deep_copy)`, `load(parent, file)`, `set_text(...)` (GDI+ 고급 텍스트), `set_alpha(0..255)`, `fade_in(in_ms, hide_ms, fadeout)`.
- 의존: `CSCGdiplusBitmap`, `CSCThread`, `CSCParagraph`.
- CSCColorTheme 미사용.

### `CDialog/SCThemeDlg/SCThemeDlg.h` — `CSCThemeDlg` (CDialogEx)
타이틀바 없는 모던 다이얼로그 베이스. `m_theme`, 자체 시스템 버튼.
- API: `create(...)`, `set_system_buttons(parent, SC_PIN, SC_MINIMIZE, ...)`, `set_color_theme(theme, invalidate)`, `set_back_image(path, draw_mode)`, `set_titlebar_height(h)`.
- **테마 전파 한계**: OnInitDialog 에서 `m_sys_buttons.set_color_theme(...)` 만 전파. 나머지 자식 컨트롤은 개발자가 직접 전파해야 함 (CtlColor 로 Windows 기본 컨트롤만 자동 커버).

### `CDialog/CSCColorPicker/SCColorPicker.h` — `SCColorPicker` (CDialog)
ARGB/Hex/HSL/팔레트/최근색상 통합 피커. Modal/Modeless.
- API: `create(parent, title, as_modal)`, `DoModal(parent, cr_selected, title)`, `get_selected_color()`, `set_use_shared_color(bool)` (HKLM vs HKCU).
- 내부 구성: CSCSliderCtrl × 다수, CSCStaticEdit, 팔레트.
- Modeless 완료 시 parent 에 `Message_CSCColorPicker` 전송.
- 내부 핸들러: `on_message_CSCSliderCtrl`, `on_message_CSCStaticEdit`.

### `CDialog/CSCColorPicker/SCDropperDlg.h` — `SCDropperDlg` (CDialog)
화면 픽셀 스포이드.
- API: `create(parent)`, `get_picked_color()`, `is_picked()`.
- 설계: 화면 1회 캡처 후 OnTimer 로 커서 주변 원형 확대. 좌클=선택, 우클=취소, 휠클=토글, Ctrl+Wheel=확대범위 조정.
- CSCShapeDlg 상속 아님 (코드 80% 불필요 이유).

---

## 5. 파일시스템·쓰레드

### `file_system/SCDirWatcher/SCDirWatcher.h` — `CSCDirWatcher`
디렉터리 변경 감시. ReadDirectoryChangesW 래핑.
- API: `init(parent)`, `add(folder, watch_sub_dir)`, `stop(folder="")`, `is_watching(folder)`.
- parent 등록: `ON_REGISTERED_MESSAGE(Message_CSCDirWatcher, &...)`. wParam = `CSCDirWatcherMessage*{action, path0, path1}`. action = `FILE_ACTION_ADDED/REMOVED/MODIFIED/RENAMED_OLD_NAME/NEW_NAME`.
- **주의**: `FILE_ACTION_MODIFIED` 는 Windows 가 2+ 회 발생시킴 → **타이머로 디바운싱 필수**.
- 동반 헤더: `ReadDirectoryChanges.h` (API 래핑), `ReadDirectoryChangesPrivate.h`, `ThreadSafeQueue.h` (std::list + CComCriticalSection + Windows 세마포).

### `thread/CSCThread/SCThread.h` — `CSCThread`
std::thread 래핑 (CreateThread/AfxBeginThread 아님).
- API: `start(std::function<void(CSCThread&)>)`, `stop()` (join 포함), `request_stop()`, `pause/resume`, `join()`.
- 상태 enum: `Stopped/Running/Paused/Stopping` (atomic). 조회: `is_running/stopped/paused/state`, `stop_requested()`.
- 워커 내부: `th.stop_requested()` 주기 체크, `th.wait_if_paused()`, `th.sleep_for(dur)` (중단 가능).
- 부가: `set_on_cancel(cb)`, UI 콜백 `invoke_ui(...)` (PostMessage(WM_APP_UI_INVOKE)).

### `thread/CSCThreadGroup/SCThreadGroup.h` — `CSCThreadGroup`
CSCThread 위에 N 데이터 자동 분할 병렬 실행 + 전체 완료 콜백을 얹은 헬퍼. 헤더 전용.
- API: `start(count, work, on_complete=null, thread_count=0)`, `stop()` (request_stop + join), `request_stop()`, `join()`, `pause/resume()`, `is_running()`, `is_completed()`, `worker_count()`, `completed_count()`.
- `work(worker_idx, start, end, CSCThread& th)` 시그니처. 워커 내부에서 `th.stop_requested()`, `th.sleep_for()` 사용 가능.
- `on_complete` 는 마지막 끝나는 워커 스레드에서 호출 — UI 갱신은 `invoke_ui` / `PostMessage` 로 마샬링 필요.
- 자동 thread 수 휴리스틱은 CThreadManager 와 동일 (≤10→5, ≤100→10, else count/20, max 40). 명시 지정도 가능.
- ThreadManager 대비 장점: detach 안 함 (객체 소멸 시 안전 join), 람다/std::function 지원 (전역 pThisWnd 불필요), 인터럽트 가능 sleep, 예외 처리.

### `thread/ThreadManager.h` — `CThreadManager` (legacy)
데이터 분할 병렬 실행 + 전체 완료 콜백. **신규 코드는 `CSCThreadGroup` 사용 권장.**
- API: `job(count, thread_func, end_func, detach)`, `is_all_thread_completed()`, `get_thread_completed_count()`, `set_thread_completed(idx)`.
- `thread_func(idx, start, end)` 시그니처로 자동 범위 분배.
- 약점: detach 후 dangling 위험, 종료 신호 메커니즘 없음, 콜백이 free function 포인터라 멤버 함수 직접 사용 불가 (file-static 전역 우회 필요), busy poll 완료 감지.

---

## 6. 유틸 헬퍼

### `Functions.h/.cpp` — 범용 함수 라이브러리 (3100+ 줄)
카테고리 (대표 함수만):
- **파일/경로**: `get_part`, `get_file_size`, `normalize_path`, `concat_path`, `FindAllFiles`, `change_extension`, `get_filetype_from_filename`, `delete_file`. **파일 경로 비교는 `=` 금지, `IsFileFolderPathIsEqual()` 사용**.
- **문자열**: `get_token_str`, `get_exact_token_str`, `GetToken`, `find_dqstring`, `FindStringFromArray`, `extract_sub_str`, `get_tag_str`.
- **검증**: `IsNumericString`, `IsInteger`, `IsAlphaNumeric`, `is_hangul`, `is_valid_string`.
- **인코딩**: `base64_encode/decode`, `read/save/file_open` (자동 인코딩 판별), `get_text_encoding`, `read_lines` (read + \r\n→\n + 라인 split), `load_string` (리소스). `get_text_encoding` 은 BOM 검사 + UTF-16LE no-BOM strict (filesize 짝수, buf[0]!=0, buf[1]==0, IsTextUnicode 통계 4중) + UTF-8 valid sequence 순서로 판별.
- **수치 매크로**: `DISTANCE`, `CLIP`, `ROUND`, `SQR`, `RADIAN`, `DEGREE`, `MAKE_MULTIPLY_UP/DOWN`.
- **GDI/영상**: `resize11`, `rotate90`, `mirror`, `gaussian_blur`, `scv_image_threshold`, `scv_absdiff`, `scv_subImage`.
- **네트워크**: `is_server_reachable`, `request_url` (`CRequestUrlParams`), `parse_url`, `DownloadFile`, `ReadURLFile`, `HttpUploadFile`.
- **EXIF**: `CSCEXIFInfo` (카메라/렌즈/GPS/촬영시각).
- **디버깅**: `Trace`, `Traceln`, `trace` 매크로 (함수명/라인 자동), `printf_string`, `trace_output`.

### `LayeredWindowHelperST.h` — `CLayeredWindowHelperST`
WS_EX_LAYERED 투명도. `AddLayeredStyle`, `SetTransparent(0..255)`. **child window 불가, popup only**.

### `CWnd/WndShadow/WndShadow.h` — `CWndShadow`
윈도우 그림자. `Initialize()` (static), `Create(parent)`, `SetSize/Sharpness/Darkness/Position/Color`. **Vista Aero 활성화 시 DWM 합성 때문에 자동 비활성** (`SS_DISABLEDBYAERO`).

### `MemoryDC.h` — `CMemoryDC : CDC`
더블 버퍼링. 생성자에서 메모리 비트맵 할당, 소멸자에서 BitBlt. `CMemoryDC(pDC, &rect, bBg=false)`. 사용 시 `OnEraseBkgnd → return FALSE` + Dialog 속성 "Clip Children" = true.

### `AutoFont.h` — `CAutoFont : CFont`
CFont 래핑. `SetHeight/Bold/Italic/Underline/StrikeOut/FaceName/Weight/Charset/FontColor`. 직렬화: `ContractFont()`/`ExtractFont()` — 레지스트리 저장 가능. CFontDialog 래핑: `GetFontFromDialog()`.

### `messagebox/Win32InputBox/Win32InputBox.h` — `CWin32InputBox`
커스텀 InputBox. 스타일 enum: `NORMAL/MULTILINE/NUMERIC/PASSWORD/MESSAGE`. static `InputBox(title, prompt, buf, size, style, timeout_ms, parent)`. `WIN32INPUTBOX_PARAM` 구조체로 고급 옵션.

### `data_structure/SCParagraph/SCParagraph.h` — `CSCParagraph`
태그 기반 리치 텍스트. 글자별 폰트/색/스타일. `CSCStatic` · `CSCToolTipCtrl` · `CSCMessageBox` · `CSCShapeDlg` 가 공유한다.
- `CSCTextProperty` = font name/size/style + cr_text/back/stroke/shadow + 그림자·발광 파라미터.
- 파이프라인: `build_paragraph_str()` → `calc_text_rect()` → `draw_text(CDC or Graphics)`. 전부 static.
- **태그 전체 목록과 정확한 인자 의미는 헤더 주석이 유일한 출처다.** 여기서는 갈래만 적는다.
  - 폰트·스타일: `<f=> <sz=> <b> <i> <u> <s> <sup> <sub> <sp=>`
  - 색: `<cr=>`(=`<ct=>`), `<crb=>`(=`<cb=>`), `<grad=도착색>` (인자는 **도착색 하나** — 출발색은 그 구간의 `cr_text`), `<box=색,반지름,여백>` (CSS 의 background + padding + border-radius. `<crb>` 와 달리 여백이 붙어 한 글자 run 은 세로로 긴 알약이 된다)
  - 외곽선·그림자·발광: `<st=> <cs=> <ts=...> <sd=> <csh=> <sb=> <glow=색,sigma>` (`<glow>` 는 `<ts=0,0,σ,색,σ>` 와 같다 — 작은 글자에 큰 σ 를 주면 글자끼리 뭉친다)
  - 줄·정렬: `<br> <ls=> <vsp=> <al=> <la=> <indent=> <hang=> <tab=> <nowrap>` (줄 단위 태그는 그 줄이 확정되면 리셋된다)
  - 그 밖: `<ruby=> <cru=> <img=> <style=> <id=>`
- **태그를 글자 그대로 보여주려면 HTML 엔티티** — `&lt; &gt; &amp;`. 특수한 글자는 이 셋뿐이다.
  사용자 데이터를 넣을 땐 `CSCToolTipCtrl::escape_tags()` 를 쓴다.

### `log/SCLog/SCLog.h` — `CSCLog`
파일 로그. 출력은 `<exe_dir>\Log\<exe_title>_YYYYMMDD_HHMMSS.log` — **실행할 때마다 새 파일**이라 덮어쓰기가 없다.
`set(folder, title, level)`, `write(...)`, `show_function_name/line_number`, `get_log_full_path()`.
로그 파일은 배포·커밋 대상이 아니므로(.gitignore) 실제 경로·파일명이 문자열로 들어가도 된다.

### `system/SCKeyBindings/SCKeyBindings.h` — `CSCKeyBindings`
단축키를 리소스가 아니라 **런타임 테이블**로 다룬다. `seed_from_resource(accel_id)` / `seed_from_menu(HMENU)` /
`seed_from_scmenu(CSCMenu*)` 로 기존 정의를 흡수한 뒤 `register_action`, `set_binding`, `reset_to_default`.
사용자 변경분은 레지스트리에 `load_overrides` / `save_overrides`. 표시·파싱은 `key_to_string` / `string_to_key`.

### `data_types/` — 값 타입
- `CSCTime/SCTime.h` — `CSCTime`. `CTime` 은 ms 가 없고 `SYSTEMTIME` 은 연산자가 없으며 둘 다 **음수를 못 담는다**.
  시각과 시간차를 한 타입으로 다루기 위해 만든 것. 연산자 오버로딩 제공.
- `CSCGroupRect/SCGroupRect.h` — `CSCRect`(라벨 붙은 `Gdiplus::RectF`) + `CSCGroupRect`(그 묶음).
- `CSCUIElement/SCUIElement.h` — `CSCUIElement`. 라벨 있는 사각 UI 요소의 공통 표현.
- `data_structure/CSCBookmark/SCBookmark.h` — `CSCBookmark`. 재생 위치 + 썸네일 한 단위.
  썸네일은 `unique_ptr` — `CSCGdiplusBitmap` 에 안전한 copy/move 가 없어 vector 에 값으로 담으면 shallow copy 위험.

### `win_compat/dwm.h` — `win_compat::dwm`
Vista+ 전용 DWM API 래퍼. XP/Vista 에서 자동 no-op 이라 **호출측에 OS 분기를 두지 않아도 된다.**
`dwmapi`/`uxtheme` 를 직접 호출하지 말고 여기를 경유한다(SC 컨트롤은 XP 까지 지원한다).

### `directx/CSCVideoWndD2/SCVideoWndD2.h` — `CSCVideoWndD2`
Direct2D 비디오 재생 창. 구 `CVideoWnd` 대비 — 디코딩은 `ffi::CDecoder`(FFmpeg + D3D11VA/DXVA2 하드웨어),
렌더링은 D2D `DrawBitmap`(GPU 샘플러 — 창을 키워도 비용이 거의 안 는다. GDI `StretchDIBits` 는 목적지
픽셀 수에 비례해 CPU 를 쓴다), 프레임 페이싱은 `WM_TIMER` 가 아니라 고해상도 waitable timer pacer 스레드
(`WM_TIMER` 는 해상도 15.6ms + 큐가 빌 때만 합성되는 최저 우선순위 메시지라 안정적 페이싱이 불가능하다).

---

## 7. 의존 그래프 요약

```
CASeeDlg (프로젝트)
 ├─ CSCD2ImageDlg ──┬─ CSCD2Context ──(D2D, WIC)
 │                  ├─ CSCD2Image × deque ─── CSCThread (animation)
 │                  ├─ CSCThumbCtrl ──┬─ CSCGdiplusBitmap
 │                  │                 ├─ CSCThreadGroup → CSCThread × N
 │                  │                 ├─ CSCEdit (inline title edit)
 │                  │                 └─ CSCColorTheme
 │                  └─ CSCSliderCtrl (GIF frame)
 ├─ CSCShapeDlg × 2 (message, shapeDlg) ── CSCGdiplusBitmap, CSCThread, CSCParagraph
 ├─ CSCDirWatcher ── ReadDirectoryChanges ── ThreadSafeQueue
 └─ TitleDlg (프로젝트) ── CSCSystemButtons, LayeredWindowHelperST, MemoryDC

컬러:  모든 커스텀 컨트롤 ── CSCColorTheme ── colors.h (gRGB, get_color 등)
유틸:  대부분 ── Functions.h
```

파일 탐색기형 앱 (KoinoViewer / SCDeskTools 류) 의 전형적 조합:

```
Dlg (CSCThemeDlg 파생)
 ├─ CSCMenuBar ── CSCMenuButton × N ── CSCMenu (CSCMenuItem)
 ├─ CSCToolBar ── CGdiButton × N
 ├─ CPathCtrl                                  ─┐
 ├─ CSCTreeCtrl ─┬─ CSCScrollbar × 2 (v/h)      │ 셋 다 CSCScrollbar 오버레이 +
 │               ├─ CSCStaticEdit (인라인 편집)  │ NC 테두리 인프라를 각자 구현
 │               ├─ CSCToolTipCtrl (잘린 라벨)   │ (§3.1 — 약 1,000줄 중복)
 │               └─ CSCShapeDlg (드래그 이미지)  │
 ├─ ControlSplitter ── (트리 ↔ 리스트 배치)      │
 ├─ CSCListCtrl ─┬─ CSCHeaderCtrl               │
 │               ├─ CSCScrollbar × 2 (v/h)     ─┘
 │               ├─ CSCStaticEdit (인라인 편집)
 │               ├─ CSCToolTipCtrl (잘린 셀)
 │               ├─ CShellImageList (아이콘)
 │               └─ CSCShapeDlg (드래그 이미지)
 └─ CSCMessageBox / CSCPropertyCtrl / CSCColorPicker

서식 텍스트: CSCStatic · CSCToolTipCtrl · CSCMessageBox · CSCShapeDlg ── CSCParagraph
```
