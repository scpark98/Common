# CSCColorPicker

MFC용 고급 컬러 피커 대화 상자. ARGB 기반 색상 선택, 최근 색상 관리, 다양한 Hex 표시 형식, 색상 이름 검색, 확대 스포이드를 지원합니다.

## 개발 목적

- 기존 `CColorDialog`, `CMFCColorButton`은 `COLORREF` 기반이라 alpha 채널을 다루지 못함.
- 사용자 정의색 추가/제거 UI가 불편하고, `CMFCColorButton`의 스포이드는 모든 창에서 검출되지 않으며 확대 기능이 없음.
- 이 컨트롤은 ARGB를 기본으로 하고 ARGB / ABGR / RGBA / BGRA Hex 포맷, 색상 이름 표시, 휠 확대/축소, 사용자 정의색 import/export, Modal + Modeless 2가지 사용 방식을 모두 지원함.

## 프로젝트에 추가

`Common\CDialog\CSCColorPicker\` 폴더의 4개 파일을 프로젝트에 포함:

- `SCColorPicker.h` / `SCColorPicker.cpp`
- `SCDropperDlg.h` / `SCDropperDlg.cpp`

의존성: `Common\Functions.h` (공유 레지스트리 접근용), `Common\colors.h`, `Common\CEdit\SCEdit\`, `Common\CSliderCtrl\SCSliderCtrl\`

## Modal 사용법

사용 지점에서 지역 변수로 선언 후 `DoModal()` 호출.

```cpp
#include "Common/CDialog/CSCColorPicker/SCColorPicker.h"

void CMyDlg::OnBnClickedButtonPickColor()
{
    CSCColorPicker picker;

    // 두 번째·세 번째 인자는 옵션
    //   cr_selected : 초기 선택 색상 (Transparent → 마지막 선택 색상으로 시작)
    //   title       : 타이틀바 텍스트 (생략 시 "Color Picker")
    if (picker.DoModal(this, Gdiplus::Color(255, 128, 64, 32), _T("배경 색상")) == IDCANCEL)
        return;

    Gdiplus::Color cr = picker.get_selected_color();
    // 선택된 색을 사용...
}
```

`DoModal` 시그니처:
```cpp
INT_PTR DoModal(CWnd* parent,
                Gdiplus::Color cr_selected = Gdiplus::Color::Transparent,
                CString title = _T(""));
```

초기 색상이 팔레트/recent에 존재하지 않아도 preview·슬라이더·edit에 정상 반영되며, 어느 셀에도 선택 링이 그려지지 않음.

## Modeless 사용법

부모 대화 상자의 멤버로 선언해 오래 유지하고, 색상 변경 이벤트는 등록된 메시지로 수신.

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

LRESULT CMyDlg::on_message_CSCColorPicker(WPARAM wParam, LPARAM lParam)
{
    auto msg = (CSCColorPickerMessage*)wParam;
    m_cr_back = msg->cr_selected;   // 실시간으로 바뀌는 선택 색상
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
- `CSCColorPicker* pThis` — 이벤트를 발생시킨 인스턴스
- `Gdiplus::Color cr_selected` — 현재 선택 색상

## 공유 컬러 (HKLM) 사용

기본값은 앱별 경로(`HKCU\Software\<CompanyName>\<AppName>\setting\color picker`)에 recent colors를 저장. 여러 앱이 동일한 recent 목록을 공유하려면 공유 모드를 켭니다.

```cpp
picker.set_use_shared_color(true);   // HKLM\Software\CSCColorPicker 사용
picker.DoModal(this);
```

또는 런타임에 우클릭 컨텍스트 메뉴 → **공유 컬러 사용** 체크 토글로도 전환 가능.

**주의 사항**
- HKLM 쓰기는 **관리자 권한** 필요. 일반 권한이면 저장이 silently 실패함. 관리자 권한 실행이 어려우면 앱 매니페스트에 `requireAdministrator` 지정 고려.
- 32/64비트 앱이 혼재하면 WOW64 리디렉션으로 서로 다른 하이브를 보게 됨 (32-bit → `Software\WOW6432Node\CSCColorPicker`). 동일한 비트너스끼리만 공유됨.

## 컨텍스트 메뉴 (우클릭)

팔레트/recent 영역에서 우클릭하면 나타나는 메뉴 항목:

| 항목 | 동작 |
|------|------|
| Hex 복사 / 컴포넌트 복사 / Web Color 복사 | 현재 선택 색상을 클립보드에 복사 |
| 최근 색상 삭제 / 모두 삭제 | 개별 또는 전체 recent 제거 |
| 보색 보기 | 현재 색의 보색을 적용 |
| 툴팁 표시 | 호버 툴팁 on/off |
| ARGB / ABGR / RGBA / BGRA | Hex 표시 포맷 선택 (라디오) |
| **공유 컬러 사용** | HKLM ↔ HKCU 전환 (체크박스) |
| 최근 색상 내보내기 / 가져오기 | .reg 파일로 export/import |

## 조작법

- **더블클릭** 또는 **Enter**: 색상 선택 확정
- **Esc**: 취소
- **휠**: 스포이드 영역 확대/축소
- **Ctrl+휠**: 스포이드 픽셀 크기 확대/축소
- **`+` 버튼**: 현재 선택 색상을 recent에 추가
- **스포이드 버튼**: 화면 임의 지점 색상 추출 (확대 가능)
- **recent 영역 드래그**: 8칸 초과분 스크롤
- **Hex 레이블 더블클릭**: Hex 포맷 순환 (ARGB → ABGR → RGBA → BGRA → ARGB)
- **색상 이름 edit에 이름 입력 후 Enter**: 해당 색으로 설정

## 공개 API

```cpp
class CSCColorPicker : public CDialog
{
public:
    // Modeless로 사용 시 먼저 호출
    bool create(CWnd* parent,
                CString title = _T(""),
                bool as_modal = true);

    // Modal 방식 호출
    virtual INT_PTR DoModal(CWnd* parent,
                            Gdiplus::Color cr_selected = Gdiplus::Color::Transparent,
                            CString title = _T(""));

    // 현재 선택된 색상
    Gdiplus::Color get_selected_color() const;

    // 공유/앱별 저장 위치 전환. true → HKLM, false → HKCU (앱별)
    void set_use_shared_color(bool use_shared = true);
};
```

## 레지스트리 저장 위치

| 모드 | 경로 |
|------|------|
| 앱별 (기본) | `HKCU\Software\<CompanyName>\<AppName>\setting\color picker` |
| 공유 | `HKLM\Software\CSCColorPicker\setting\color picker` |

저장 항목:
- `count` (DWORD) — recent 개수
- `color_0` … `color_N` (문자열) — 8자리 ARGB Hex
- `hex_format` (DWORD) — 0:ARGB, 1:ABGR, 2:RGBA, 3:BGRA
- `show_tooltip` (DWORD)
- `recent colors exported folder` (문자열) — export 마지막 폴더
