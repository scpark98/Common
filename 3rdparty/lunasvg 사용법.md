# lunasvg(SVG) + plutovg 사용법

SVG 래스터화 및 SMIL 애니메이션 재생/내보내기를 위한 벤더 라이브러리.

- **lunasvg v3.5.0** (`lunasvg/`) — SVG 파서/렌더러.
- **plutovg** (`lunasvg/plutovg/`) — lunasvg의 2D 래스터 백엔드.
- **pugixml** (`../xml/pugixml/`) — SMIL 애니메이션 프레임 베이킹용 DOM 파서(`PUGIXML_WCHAR_MODE` 전제).
- 모두 **소스로 정적 컴파일**한다(별도 `.lib` 없음 → CRT 자동 일치, x64/Win32 무관).
- 래퍼: `Common/SCSvg.h/.cpp` — 파싱/래스터화(`sc_svg`), 애니메이션 프레임 베이킹, GIF/WebP 내보내기.

> lunasvg 전용 `.props`는 아직 없다(apng.props 같은 걸 원하면 만들 수 있음). 아래는 수동 설정.

---

## 1. 설정 (대상 구성에)

### 전처리기 정의 (4개)
```
SC_USE_SVG;LUNASVG_BUILD_STATIC;PLUTOVG_BUILD_STATIC;LUNASVG_DISABLE_LOAD_SYSTEM_FONTS
```
- `SC_USE_SVG` — Common의 SVG 코드(멤버·분기) 활성화 게이트. 없으면 SVG 의존 0으로 빌드.
- `LUNASVG_BUILD_STATIC`, `PLUTOVG_BUILD_STATIC` — 정적 링크.
- `LUNASVG_DISABLE_LOAD_SYSTEM_FONTS` — 시스템 폰트 전체 로드 끔(대량 릭 방지). 텍스트 SVG는 참조 폰트만 온디맨드 등록(SCSvg가 DirectWrite로 처리).

### 추가 포함 디렉터리 (3개)
```
$(CommonLibDir)\3rdparty\lunasvg\include;$(CommonLibDir)\3rdparty\lunasvg\plutovg\include;$(CommonLibDir)\xml\pugixml\src
```
(CommonLib.props 안 쓰면 절대경로 `D:\1.Projects_C++\Common\...` 로.)

### 소스 파일 추가 (모두 **미리 컴파일된 헤더 = 사용 안 함**)
- `Common\SCSvg.cpp`
- lunasvg 10개 (`lunasvg\source\`):
  `graphics.cpp lunasvg.cpp svgelement.cpp svggeometryelement.cpp svglayoutstate.cpp svgpaintelement.cpp svgparser.cpp svgproperty.cpp svgrenderstate.cpp svgtextelement.cpp`
- plutovg 11개 (`lunasvg\plutovg\source\`):
  `plutovg-blend.c plutovg-canvas.c plutovg-font.c plutovg-ft-math.c plutovg-ft-raster.c plutovg-ft-stroker.c plutovg-matrix.c plutovg-paint.c plutovg-path.c plutovg-rasterize.c plutovg-surface.c`
- pugixml 1개 (`xml\pugixml\src\`): `pugixml.cpp`

> pugixml은 `pugiconfig.hpp`의 **`PUGIXML_WCHAR_MODE`**(char_t=wchar_t) 전제. SCSvg가 경계에서 UTF-8↔UTF-16 변환한다.

---

## 2. 코드에서 쓰기

### 2-1. 파일 열기 (CSCD2Image, ASee 뷰어 경로)
`CSCD2Image::load(path)`가 `.svg`를 자동 처리(`load_svg`):
- 정적 SVG → 논리 크기로 래스터화, 확대 시 재래스터(선명 유지).
- 애니메이션 SVG(SMIL `<animate>`/`<animateTransform>`/`<set>`) → 프레임으로 베이킹해 GIF처럼 재생.

### 2-2. 썸네일 / 고정 크기 래스터 (CSCGdiplusBitmap)
```cpp
CSCGdiplusBitmap img;
img.load_svg(sfile, 256);   // 긴 변 256px, 자연 비율 유지 → Gdiplus::Bitmap
```

### 2-3. 직접 래스터화 (sc_svg)
```cpp
#include "SCSvg.h"
sc_svg svg;
if (svg.load(path)) {
    std::vector<uint8_t> bgra;                 // straight BGRA, w*h*4
    svg.render(width, height, bgra);
    // 애니면 svg.is_animated(), svg.build_frames(w,h,frames,delays)
}
```

### 2-4. 애니메이션 SVG → GIF/WebP 내보내기 (SCSvg.h)
```cpp
export_svg_to_animated_gif (svg_path, out_gif,  w, h, fps, progress);   // WIC, 256색+1비트 투명
export_svg_to_animated_webp(svg_path, out_webp, w, h, fps, progress);   // libwebp, 풀컬러+알파(무손실)
```
(WebP 내보내기는 libwebp 필요 — `Common/directx/webp` 참조.)

---

## 3. 참고 / 한계

- **애니메이션은 SMIL을 자체 해석해 프레임 베이킹**한다(lunasvg는 애니 재생 못 함). 지원: `<animate>/<animateTransform>/<animateColor>/<set>`, from/to/by·values;keyTimes, repeatCount, fill freeze/remove, dispose. 미지원: CSS `@keyframes`, JS/상호작용 트리거, `<animateMotion>`, keySplines 이징(선형 근사) 등 → 첫 프레임 정적 폴백.
- **텍스트 폰트**: 시스템 폰트 전체 로드는 껐고, SVG가 참조하는 `font-family`만 DirectWrite로 해석해 온디맨드 등록. 한글 텍스트는 `font-family="Malgun Gothic"`처럼 **명시**돼야 표시(미지정은 Arial 폴백 → 한글 글리프 없음).
- 벤더 원본 클론 위치가 아닌 **Common repo 안으로 복사**해 두어 양 머신에서 경로 무관하게 빌드된다.
- **솔루션 탐색기 정리**: 이 소스들은 각 프로젝트 vcxproj에 직접 넣으면 `.vcxproj.filters`로 폴더(예: `Common\lunasvg`) 정리가 유지된다(현재 ASee가 그렇게 정리돼 있음). props로 넣으면 정리가 유지되지 않는다(VS 한계).
