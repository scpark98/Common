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
- 헤더/프로그레스: `cr_header_text/back`, `cr_progress`, `cr_percentage_bar[]`.
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
- **인코딩**: `base64_encode/decode`, `read/save/file_open` (자동 인코딩 판별), `get_text_encoding`, `is_utf8_encoding`, `load_string` (리소스).
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
태그 기반 리치 텍스트. 글자별 폰트/색/스타일.
- `CSCTextProperty` = font name/size/style (FontStyleRegular|Bold|Italic|Underline|Strikeout) + cr_text/back/stroke/shadow + shadow_depth/thickness.
- 태그: `<f=>`, `<sz=>`, `<b>`, `<i>`, `<u>`, `<s>`, `<cr=red>` | `<cr=#FF00AA>` | `<cr=123,45,67,255>` | `<cr=h90,30,100>` (HSI), `<ct=>`, `<crb=>`, `<cb=>`, `<br>`.
- 파이프라인: `build_paragraph_str()` → `calc_text_rect()` → `draw_text(CDC or Graphics)`. 정적 함수.

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
