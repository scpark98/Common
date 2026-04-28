# CSCStaticEdit

CStatic 을 상속받아 CEdit 처럼 동작하도록 완전히 직접 그린 싱글라인 edit 컨트롤. round rect border, 커스텀 색상, placeholder, IME, clipboard, undo/redo, Shift+Up/Down 및 Shift+MouseWheel 로 수치 증감, I-beam 커서 자동 전환 등 지원.

## 개발 목적

- 기존 `CEdit` 는 시스템 edit control 에 의존해 `OnPaint`/`DrawItem` 만으로는 테두리 모양·배경·선택 하이라이트 등을 일관되게 커스터마이즈하기 어렵다.
- 특히 round rect, 그라디언트, 외부 border 색 상태 전환 같은 "완전 직접 그리기" 가 필요하면 `CStatic` 기반으로 빔 캐럿·선택·IME·클립보드를 모두 다시 구현해야 한다. 이걸 하나의 컨트롤로 정리한 것이 `CSCStaticEdit`.

## 프로젝트에 추가

`Common\CEdit\CSCStaticEdit\` 폴더의 두 파일을 프로젝트에 포함:

- `SCStaticEdit.h` / `SCStaticEdit.cpp`

의존성: `Common\colors.h` (CSCColorTheme), `Common\Functions.h`

## CEdit 과의 주요 차이 (필수 숙지)

| 항목 | CEdit | CSCStaticEdit |
|------|-------|---------------|
| 기반 클래스 | `CEdit` | `CStatic` (CEdit 아님) |
| 리소스 에디터 | Edit Control 배치 | **Static Text** 배치 |
| 노티피케이션 | `EN_CHANGE`, `EN_SETFOCUS`, `EN_KILLFOCUS`, `EN_UPDATE` 등 | 표준 EN_* 이벤트 **발생하지 않음**. `ON_REGISTERED_MESSAGE(Message_CSCStaticEdit, ...)` 로 수신 |
| 텍스트 제한 | `LimitText(n)` | `set_max_length(n)` |
| 정렬 | 스타일 `ES_LEFT/CENTER/RIGHT` | `set_text_align(DT_LEFT/CENTER/RIGHT)` + `set_line_align(DT_TOP/VCENTER/BOTTOM)` |
| 멀티라인 | `ES_MULTILINE` 가능 | **싱글라인 전용** (미지원) |
| border 스타일 | `WS_BORDER` | 직접 그림. `set_draw_border`, `set_border_width`, `set_border_color(_active/_inactive)` |
| round 모서리 | 없음 | `set_round(radius)` |
| `GetWindowText` | 시스템 edit buffer | **override — 내부 `m_text` 반환**. `CStatic::GetWindowText(str)` 로 호출하면 HWND 의 원래 캡션이 나오지만 내부 상태와 다를 수 있음 |

### EN_CHANGE → message_scstaticedit_text_changed 전환 예시

**CEdit 일 때 (기존)**
```cpp
// Dlg.h
afx_msg void OnEnChangeMyEdit();

// Dlg.cpp
BEGIN_MESSAGE_MAP(CMyDlg, CDialogEx)
    ON_EN_CHANGE(IDC_EDIT_MY, &CMyDlg::OnEnChangeMyEdit)
END_MESSAGE_MAP()

void CMyDlg::OnEnChangeMyEdit()
{
    CString s;
    m_edit.GetWindowText(s);
    // ...
}
```

**CSCStaticEdit 로 교체**
```cpp
// Dlg.h
LRESULT on_message_CSCStaticEdit(WPARAM wParam, LPARAM lParam);

// Dlg.cpp
BEGIN_MESSAGE_MAP(CMyDlg, CDialogEx)
    ON_REGISTERED_MESSAGE(Message_CSCStaticEdit, &CMyDlg::on_message_CSCStaticEdit)
END_MESSAGE_MAP()

LRESULT CMyDlg::on_message_CSCStaticEdit(WPARAM wParam, LPARAM /*lParam*/)
{
    auto msg = reinterpret_cast<CSCStaticEditMessage*>(wParam);
    if (!msg || !msg->pThis) return 0;

    if (msg->pThis == &m_edit)
    {
        switch (msg->message)
        {
        case CSCStaticEdit::message_scstaticedit_text_changed:
            // 구 OnEnChangeMyEdit 에 해당하는 로직
            break;
        case CSCStaticEdit::message_scstaticedit_enter:
            // Enter 키 (dynamic 생성 시)
            break;
        case CSCStaticEdit::message_scstaticedit_escape:
            // Escape 키 (dynamic 생성 시)
            break;
        case CSCStaticEdit::message_scstaticedit_setfocus:
        case CSCStaticEdit::message_scstaticedit_killfocus:
            break;
        }
    }
    return 0;
}
```

한 다이얼로그에 CSCStaticEdit 가 여러 개 있으면 `msg->pThis` 로 분기해서 어느 인스턴스에서 온 메시지인지 구분한다.

`Message_CSCStaticEdit` 는 `SCStaticEdit.h` 에 `static const UINT` 로 `RegisterWindowMessage` 된 값이라 include 만 하면 바로 사용 가능.

## 리소스 에디터로 배치하기

1. **Static Text** 컨트롤을 배치 (Edit Control 이 아님).
2. 적절한 ID 부여 (`IDC_STATIC_EDIT_XXX`).
3. 기본 `SS_NOTIFY` 스타일은 `PreSubclassWindow` 에서 자동 부여되므로 설정 안 해도 됨.
4. `ClassWizard` 에서 `DDX_Control` 연결.

```cpp
// Dlg.h
#include "Common/CEdit/CSCStaticEdit/SCStaticEdit.h"

class CMyDlg : public CDialogEx
{
    CSCStaticEdit m_edit;
    // ...
};

// Dlg.cpp
void CMyDlg::DoDataExchange(CDataExchange* pDX)
{
    CDialogEx::DoDataExchange(pDX);
    DDX_Control(pDX, IDC_STATIC_EDIT_MY, m_edit);
}
```

리소스 편집기에서 Static Text 의 초기 캡션에 문자열을 써두면 `PreSubclassWindow` 에서 자동으로 `m_text` 에 복사되어 내부 상태가 초기화된다.

## 동적 생성

```cpp
CSCStaticEdit* p = new CSCStaticEdit;
p->Create(WS_CHILD | WS_VISIBLE | WS_TABSTOP,
          CRect(x, y, x + w, y + h), parent, nID);
p->set_font_name(_T("Consolas"));
p->set_font_size(10);
p->set_round(8);
p->set_parent_back_color(Gdiplus::Color::White);
```

동적 생성 시 `m_dynamic = true` 가 되어 `Enter`/`Escape` 키가 내부적으로 흡수되고 `message_scstaticedit_enter` / `_escape` 메시지를 부모에 전달한다 (resource 에 배치된 non-dynamic 컨트롤은 기본 다이얼로그 OK/Cancel 로 동작하도록 키 처리를 양보).

## 주요 API

### 텍스트

```cpp
void    SetWindowText(LPCTSTR str);
void    GetWindowText(CString& out) const;   // m_text 반환 (override)
CString get_text() const;
void    set_text(const CString& text);
void    set_text(int n);
void    set_text(double d);
int     get_text_length() const;
```

### 색상

```cpp
void set_text_color(Gdiplus::Color cr);
void set_back_color(Gdiplus::Color cr);
void set_selection_color(Gdiplus::Color cr_back, Gdiplus::Color cr_text);
// round > 0 일 때 모서리 밖 채움색. 이 환경의 부모 배경이 커스텀일 때 명시.
void set_parent_back_color(Gdiplus::Color cr_parent_back);
void set_color_theme(int color_theme, bool invalidate = false);
```

### 테두리 / round

```cpp
void set_draw_border(bool draw);
void set_border_width(int width);
void set_border_color(Gdiplus::Color cr);                                  // active/inactive 동일
void set_border_color(Gdiplus::Color cr_inactive, Gdiplus::Color cr_active);
void set_border_color_inactive(Gdiplus::Color cr);
void set_border_color_active(Gdiplus::Color cr);
void set_round(int radius = -1);                                           // -1 = 자동 (height/2)
```

### 폰트

```cpp
void SetFont(CFont* p_font, BOOL b_redraw = TRUE);
void set_font_name(LPCTSTR name);
void set_font_size(int size);           // point 단위
void set_font_bold(bool bold = true);
void set_font_antialias(bool antialias = true);
```

### 기능 옵션

```cpp
void  set_readonly(bool readonly = true);
bool  is_readonly() const;
void  set_password_mode(bool password = true, TCHAR mask_char = _T('*'));
void  set_max_length(int max);            // 0 = 무제한
void  set_dim_text(const CString& dim);   // placeholder
void  set_padding(int padding);
void  set_line_align(DWORD align = DT_VCENTER);  // DT_TOP / DT_VCENTER / DT_BOTTOM
void  set_text_align(DWORD align = DT_LEFT);     // DT_LEFT / DT_CENTER / DT_RIGHT

// 텍스트가 실수로 파싱되는 경우 Shift+Up / Shift+Down / Shift+MouseWheel 로 ± interval.
// 실수 파싱 실패 시 skip. 맨 Up/Down 또는 맨 MouseWheel 은 동작하지 않음 —
// 탭/폼 내비게이션 중 값이 무심코 바뀌는 footgun 방지 (CEdit 은 방향키 시 아무 동작 없음).
// interval 은 float (예: 0.001). 표시 자리수 = max(interval 자리수, 현재 텍스트 자리수).
// 예: interval=0.01, "0.09" → Shift+Up → "0.10" (trailing 0 보존).
void  set_use_updown_key(bool use = true, float interval = 1.0f);
```

### Action 버튼 (우측 아이콘)

오른쪽에 작은 아이콘 버튼을 그려 다양한 보조 액션을 수행. 별도 `CButton` 없이 `OnPaint` / 마우스 캡처로 구현.

```cpp
enum action_button_type
{
    action_none = 0,
    action_copy,            // 클립보드로 복사 (자체 처리)
    action_clear,           // 텍스트 전체 지움 (자체 처리)
    action_find,            // 돋보기. parent 가 message_scstaticedit_action_button 수신해 처리
    action_password_toggle, // 패스워드 보이기/감추기. password 모드와 연동
};

void                set_action_button(action_button_type action = action_copy);
action_button_type  get_action_button() const;
bool                has_action_button() const;
```

`action_find` 등 parent 처리가 필요한 action 은 `Message_CSCStaticEdit` 의 `message_scstaticedit_action_button` 으로 알림이 오므로 위 *EN_CHANGE → 메시지 전환 예시* 의 핸들러에 case 추가:

```cpp
case CSCStaticEdit::message_scstaticedit_action_button:
    if (m_edit.get_action_button() == CSCStaticEdit::action_find)
    {
        // 검색 다이얼로그 열기 등
    }
    break;
```

### 선택

```cpp
void    set_sel(int start, int end);
void    get_sel(int& start, int& end) const;
void    select_all();
CString get_sel_text() const;
```

## 내장 동작

- 싱글라인 텍스트 편집, 빔 캐럿, 선택 영역(드래그/Shift+화살표)
- 클립보드: `Ctrl+C` / `V` / `X` / `A`
- Backspace / Delete / Home / End / 화살표
- IME (WM_IME_STARTCOMPOSITION / COMPOSITION / ENDCOMPOSITION)
- Undo / Redo (`Ctrl+Z` / `Ctrl+Y` — 내부 2-stack)
- Placeholder (dim text) 표시: 포커스 없음 + 텍스트 비었을 때만
- 포커스 active/inactive 에 따라 border 색 전환
- 가로 스크롤 오프셋 자동 관리 (캐럿 가시화)
- Drag 선택 시 가장자리 자동 스크롤
- 컨트롤 위에서 마우스 커서 자동으로 I-beam 전환 (disabled 시 기본 커서 유지, 카피 버튼 위에서는 화살표)
- `set_use_updown_key(true)` 활성 시 Shift+Up/Down 및 Shift+MouseWheel 로 수치 증감 (고해상도 휠은 `WHEEL_DELTA` 단위로 반복 적용)

## 제약 사항

- `ES_MULTILINE` 상당 기능 미지원 — 싱글라인 전용.
- `WS_BORDER` / `WS_EX_CLIENTEDGE` 는 `PreSubclassWindow` 에서 자동으로 제거된다 (border 를 직접 그리기 때문). 리소스 에디터에서 체크해도 무시됨.
- `m_theme.cr_parent_back` 기본값이 `Gdiplus::Color()` = **Black opaque** 이라, round 모서리 밖을 부모 배경에 맞추려면 반드시 `set_parent_back_color()` 로 명시 호출. 미호출 시 `COLOR_3DFACE` 로 폴백.
- 부모가 `WS_CLIPCHILDREN` 이고 커스텀 `OnPaint` 로 비표준 단색 배경을 그리는 다이얼로그에서 `set_parent_back_color` 없이 사용하면 round 모서리 밖이 회색/검정으로 보일 수 있음.

## 참조 구현

- `Common\CDialog\CSCColorPicker\SCColorPicker.cpp` — 동적 생성 + round + set_parent_back_color + set_use_updown_key + text_changed 필터링 재사용 예
- `Test_CSCStaticEdit` (이 프로젝트) — 리소스 배치 + DDX + 각종 옵션 데모
