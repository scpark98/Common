# CSCColorPicker

MFC용 ARGB 컬러 피커 다이얼로그. 팔레트/recent 선택, 슬라이더·Hex·색상 이름 편집, 스포이드(확대), HKLM 공유 모드, Modal/Modeless 두 가지 사용 방식을 지원한다.

## 개발 목적

- 표준 `CColorDialog`·`CMFCColorButton` 은 여전히 `COLORREF` 기반이라 alpha 채널을 다룰 수 없다.
- 사용자 정의색 추가/삭제 UI 가 빈약하고 import/export 도 없다.
- `CMFCColorButton` 의 스포이드는 모든 창에서 픽셀이 잡히지 않으며 확대 기능이 없다.
- 이 컨트롤은 **ARGB 를 기본 타입**으로 가지며, `ARGB / ABGR / RGBA / BGRA` Hex 포맷 표시, 색상 이름 검색·표시, 휠 확대/축소 스포이드, `.reg` 파일 import/export, Modal·Modeless 양쪽 사용을 모두 지원한다.

## 프로젝트에 추가

`Common\CDialog\CSCColorPicker\` 폴더의 4개 파일을 프로젝트에 포함:

- `SCColorPicker.h` / `SCColorPicker.cpp`
- `SCDropperDlg.h` / `SCDropperDlg.cpp`

의존성:
- `Common\Functions.h` / `Functions.cpp` (레지스트리 헬퍼 `get_registry_int/str`, `set_registry_int/str`, 기타 유틸)
- `Common\colors.h` / `colors.cpp` (ARGB 매크로·테마)
- `Common\CEdit\CSCStaticEdit\` (내부 편집 컨트롤 — 이전에는 `CEdit/SCEdit` 였으나 CSCStaticEdit 으로 교체됨)
- `Common\CSliderCtrl\SCSliderCtrl\` (Alpha/Hue/Light 슬라이더)

프로젝트 IncludePath 에 `D:\1.Projects_C++;` 포함이 전제 — `#include "Common/..."` 형식 경로가 동작해야 한다.

## Modal 사용법

지역 변수로 선언 후 `DoModal()` 호출.

```cpp
#include "Common/CDialog/CSCColorPicker/SCColorPicker.h"

void CMyDlg::OnBnClickedButtonPickColor()
{
    CSCColorPicker picker;

    // 두 번째·세 번째 인자는 옵션
    //   cr_selected : 초기 색상. Gdiplus::Color::Transparent 전달 시 recent[0] 기본 선택
    //   title       : 타이틀바 텍스트 (기본 "Color Picker")
    if (picker.DoModal(this, m_cr_back, _T("배경 색상")) == IDCANCEL)
        return;

    m_cr_back = picker.get_selected_color();
    // 선택된 색을 사용...
}
```

`DoModal` 시그니처:
```cpp
INT_PTR DoModal(CWnd* parent,
                Gdiplus::Color cr_selected = Gdiplus::Color::Transparent,
                CString title = _T(""));
```

- 초기 색상이 팔레트/recent 에 있으면 해당 위치에 선택 링 표시.
- 없으면 `HitArea::External` sentinel 로 표시 (선택 링은 그려지지 않지만 slider/edit/preview 는 전달받은 색으로 초기화).
- `Transparent` 전달 시 recent[0] 을 기본 선택으로 사용.

## Modeless 사용법

부모 다이얼로그의 멤버로 선언 후 `create()` 호출. 색상 변경 이벤트는 등록 메시지로 수신.

**헤더 (CMyDlg.h)**
```cpp
#include "Common/CDialog/CSCColorPicker/SCColorPicker.h"

class CMyDlg : public CDialogEx
{
    CSCColorPicker m_color_picker;
    LRESULT on_message_CSCColorPicker(WPARAM wParam, LPARAM lParam);
    // ...
};
```

**구현 (CMyDlg.cpp)**
```cpp
BEGIN_MESSAGE_MAP(CMyDlg, CDialogEx)
    ON_REGISTERED_MESSAGE(Message_CSCColorPicker, &CMyDlg::on_message_CSCColorPicker)
END_MESSAGE_MAP()

BOOL CMyDlg::OnInitDialog()
{
    CDialogEx::OnInitDialog();
    // 세 번째 인자 false → modeless
    m_color_picker.create(this, _T("Color Picker"), false);
    return TRUE;
}

LRESULT CMyDlg::on_message_CSCColorPicker(WPARAM wParam, LPARAM /*lParam*/)
{
    auto msg = reinterpret_cast<CSCColorPickerMessage*>(wParam);
    m_cr_back = msg->cr_selected;  // 실시간으로 바뀌는 선택 색상
    Invalidate();
    return 0;
}

// 필요 시 표시/숨김 토글
void CMyDlg::OnBnClickedButtonToggle()
{
    m_color_picker.ShowWindow(
        m_color_picker.IsWindowVisible() ? SW_HIDE : SW_SHOW);
}
```

`CSCColorPickerMessage` 멤버:
- `CSCColorPicker* pThis` — 이벤트 발생 인스턴스
- `Gdiplus::Color cr_selected` — 현재 선택 색상

## 공유 컬러 (HKLM) 모드

기본값은 앱별 경로(`HKCU\Software\<CompanyName>\<AppName>\setting\color picker`)에 recent colors 를 저장. 여러 앱이 동일한 recent 목록을 공유하려면 공유 모드를 켠다.

```cpp
picker.set_use_shared_color(true);   // HKLM\Software\CSCColorPicker 사용
picker.DoModal(this);
```

또는 런타임에 우클릭 컨텍스트 메뉴 → **공유 컬러 사용** 체크 토글.

**플래그 자체는 앱별 저장**
- `use_shared_color` DWORD 값은 **항상 HKCU 앱별** 경로에 저장/복원된다 — 즉 "이 앱이 공유 모드를 쓸지" 는 각 앱마다 독립적.
- `create()` 시 이 값을 복원하므로 사용자가 컨텍스트 메뉴로 토글한 설정이 다음 실행 때 그대로 적용된다.
- 토글 순간 새 경로에서 recent 를 재로딩하고, 현재 선택색(`m_sel_color`)이 새 recent/palette 안에 있으면 선택 링을 그 위치로 갱신.

**주의 사항**
- HKLM 쓰기/읽기는 **관리자 권한 프로세스**에서만 성공. 일반 권한이면 silently 실패하므로, 컨텍스트 메뉴의 **공유 컬러 사용** 항목은 `HKLM\Software` 에 KEY_WRITE 열기가 실패하면 자동으로 **grayed** 처리된다.
- 32/64비트 앱이 혼재하면 WOW64 리디렉션으로 서로 다른 하이브를 보게 된다 (32-bit → `Software\WOW6432Node\CSCColorPicker`). 동일 비트너스끼리만 실제로 공유된다.

## 컨텍스트 메뉴 (우클릭)

팔레트/recent 영역에서 우클릭 시 표시:

| 항목 | 동작 |
|------|------|
| Hex 복사 / 컴포넌트 복사 / Web Color 복사 | 현재 색상을 클립보드에 복사 |
| 최근 색상 삭제 / 모두 삭제 | 개별 또는 전체 recent 제거 |
| 보색 보기 | 현재 색의 보색을 적용 |
| 툴팁 표시 | 호버 툴팁 on/off |
| ARGB / ABGR / RGBA / BGRA | Hex 표시 포맷 선택 (라디오) |
| **공유 컬러 사용** | HKLM ↔ HKCU 전환 (체크박스). 관리자 권한 없으면 grayed |
| 최근 색상 내보내기 / 가져오기 | `.reg` 파일로 export/import |

## 조작법

- **더블클릭** 또는 **Enter** — 색상 선택 확정
- **Esc** — 취소
- **`+` 버튼** — 현재 선택 색상을 recent 에 즉시 추가 후 레지스트리에 저장
- **스포이드 버튼** — 화면 임의 지점 색상 추출 (별도 확대 창 지원)
- **휠** — 스포이드 영역 확대/축소
- **Ctrl+휠** — 스포이드 픽셀 크기 확대/축소
- **recent 영역 드래그** — 8칸 초과분 스크롤
- **Hex 레이블 더블클릭** — Hex 포맷 순환 (ARGB → ABGR → RGBA → BGRA → ARGB)
- **색상 이름 edit** — 이름 입력 + Enter 로 해당 색으로 설정
- **ARGB / HSL edit** — 정수 필드에서 위/아래 방향키로 값 증감 (`CSCStaticEdit::set_use_updown_key` 활성)

## 내부 편집 컨트롤 (CSCStaticEdit)

모든 Hex/ARGB/HSL/Name edit 은 `CSCStaticEdit` (CStatic 기반 커스텀 에디트) 로 **동적 생성**된다. 리소스 에디터에 대응 항목은 없고 전부 코드에서 `Create` 한다. 주요 설정:
- `set_round(radius)` — 모서리 둥글게
- `set_parent_back_color(White)` — round 모서리 밖을 부모 배경색과 맞춤
- `set_use_updown_key()` — ARGB/HSL 숫자 필드에서 위/아래 키 증감 활성
- 값 변경 이벤트는 `message_scstaticedit_text_changed` — 부모(picker) 가 `ON_REGISTERED_MESSAGE(Message_CSCStaticEdit, ...)` 로 수신해 실시간 필터링
- Enter/Escape 는 `message_scstaticedit_enter/escape` 로 통지되어 picker 가 OK/Cancel 로 매핑

## 공개 API

```cpp
class CSCColorPicker : public CDialog
{
public:
    // Modeless 사용 시 먼저 호출
    bool create(CWnd* parent,
                CString title = _T(""),
                bool as_modal = true);

    // Modal 호출
    virtual INT_PTR DoModal(CWnd* parent,
                            Gdiplus::Color cr_selected = Gdiplus::Color::Transparent,
                            CString title = _T(""));

    // 현재 선택된 색상
    Gdiplus::Color get_selected_color() const;

    // 공유/앱별 저장 위치 전환. true → HKLM, false → HKCU 앱별.
    // 값은 HKCU 앱별에 저장되어 다음 실행 시 자동 복원.
    void set_use_shared_color(bool use_shared = true);
};
```

## 레지스트리 저장 위치

| 항목 | 경로 |
|------|------|
| `use_shared_color` 플래그 (0/1) | `HKCU\Software\<CompanyName>\<AppName>\setting\color picker` |
| recent colors (앱별) | `HKCU\Software\<CompanyName>\<AppName>\setting\color picker` |
| recent colors (공유) | `HKLM\Software\CSCColorPicker\setting\color picker` |
| 창 위치 | `HKCU\Software\<CompanyName>\<AppName>\color picker` |

저장 항목:
- `count` (DWORD) — recent 개수
- `color_0` … `color_N` (REG_SZ) — 8자리 ARGB Hex 문자열
- `hex_format` (DWORD) — 0:ARGB, 1:ABGR, 2:RGBA, 3:BGRA
- `show_tooltip` (DWORD)
- `use_shared_color` (DWORD, HKCU 앱별)
- `recent colors exported folder` (REG_SZ) — export 마지막 폴더

## 참조 / 테스트 프로젝트

- `D:\1.Projects_C++\1.test\Test_CSCColorPicker` — 이 컨트롤의 Modal/Modeless 데모 및 공유 컬러 모드 검증용 MFC 다이얼로그 프로젝트.
