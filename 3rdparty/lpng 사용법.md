# libpng(APNG) + zlib 사용법

애니메이션 PNG(APNG) 및 일반 PNG 디코드를 위한 벤더 라이브러리.

- **libpng 1.6.58** (`lpng1658/`) — APNG read 지원이 mainline에 병합된 버전. `png.h`가 `PNG_READ_APNG_SUPPORTED`를 무조건 정의한다.
- **zlib 1.3.2** (`zlib-1.3.2/`) — libpng의 압축 의존.
- 둘 다 **소스로 정적 컴파일**한다(별도 `.lib` 없음). 그래서 프로젝트의 CRT(`/MD`·`/MDd`·`/MT`)에 자동으로 맞고 x64/Win32 무관하다.
- 공용 디코더: `Common/SCApng.h/.cpp` 의 `sc_apng::decode()` — libpng로 프레임을 디코드하고 fcTL offset + dispose/blend를 합성해 straight BGRA 프레임 + delay를 돌려준다(GDI+/D2D 비의존).

---

## 1. 빠른 시작 (권장) — `apng.props` Import 한 줄

이 폴더의 **`apng.props`** 하나만 Import하면 아래가 전부 적용된다:
- 전처리기 `SC_USE_APNG` 정의
- 포함 디렉터리 `lpng1658`, `zlib-1.3.2` 추가
- 소스 `SCApng.cpp` + libpng 15개 + zlib 15개 컴파일(PCH 미사용, libpng/zlib는 SDL 검사 해제)

> **전제**: `CommonLib.props`가 먼저 적용돼 `$(CommonLibDir)`가 정의돼 있어야 한다. (CommonLib.props 사용법은 Common 문서 참조.)

### 방법 A — 속성 관리자(권장, UI에 보임)
1. **보기 → 다른 창 → 속성 관리자**.
2. 프로젝트의 **구성 노드들(Debug|x64 등)을 모두 다중 선택**.
3. 우클릭 → **기존 속성 시트 추가…** → `Common\3rdparty\apng.props` 선택.
   - 다중 선택하면 **전 구성에 한 번에** 들어간다(이미 있는 구성엔 중복 추가 안 됨).

### 방법 B — .vcxproj 직접(무조건 Import, 전 구성)
`CommonLib.props` Import 다음 줄에 한 줄 추가:
```xml
<Import Project="$(CommonLibDir)\3rdparty\apng.props" />
```
- 전 구성에 적용되지만 **속성 관리자 UI에는 안 보인다**(최상위 무조건 Import라서). 기능은 A와 동일.
- **주의**: A와 B를 같은 구성에 동시에 쓰면 props가 두 번 import되어 ClCompile 중복 → 빌드 에러. 한 가지만 사용.

---

## 2. 수동 설정 (props 안 쓰고 직접 넣을 때)

**대상 구성**(APNG 쓸 구성)에:

- **전처리기 정의**: `SC_USE_APNG`
- **추가 포함 디렉터리**:
  ```
  $(CommonLibDir)\3rdparty\lpng1658;$(CommonLibDir)\3rdparty\zlib-1.3.2
  ```
  (CommonLib.props 안 쓰면 절대경로 `D:\1.Projects_C++\Common\3rdparty\...` 로.)
- **소스 파일 추가**(모두 **미리 컴파일된 헤더 = 사용 안 함**):
  - `Common\SCApng.cpp`
  - libpng 15개(`lpng1658\`): `png.c pngerror.c pngget.c pngmem.c pngpread.c pngread.c pngrio.c pngrtran.c pngrutil.c pngset.c pngtrans.c pngwio.c pngwrite.c pngwtran.c pngwutil.c` — **`example.c`, `pngtest.c`는 제외**(라이브러리 아님).
  - zlib 15개(`zlib-1.3.2\`): `adler32.c compress.c crc32.c deflate.c gzclose.c gzlib.c gzread.c gzwrite.c infback.c inffast.c inflate.c inftrees.c trees.c uncompr.c zutil.c`
- **SDL 검사 끄기**: libpng가 `C4146`(unsigned 단항 마이너스)을 `/sdl`이 에러로 승격시킨다. libpng/zlib 파일에 `SDLCheck=false`를 주거나, 테스트 프로젝트면 **프로젝트 전체 SDL 검사=아니요**(C/C++ → 일반 → SDL 검사)로.

---

## 3. 코드에서 쓰기

### 3-1. 파일 열기 (CSCD2Image, ASee 뷰어 경로)
`CSCD2Image::load(path)`가 `.png`/`.apng` 확장자를 자동 처리한다:
- **acTL 청크 감지** → APNG면 libpng로 프레임 디코드·재생(GIF/webp와 동일 경로).
- acTL 없으면(정적 png) → 기존 WIC 경로(정적).
- `.apng` 확장자 인식을 위해 `Common/Functions.h`의 `FILE_EXTENSION_IMAGE`에 `apng`가 포함돼 있다(폴더 스캔·열기 대화상자용).

### 3-2. 리소스 임베드 (CSCGdiplusBitmap / CSCStatic)
- **`.rc`에 그냥 PNG로 등록**한다(별도 `.bin` 리네임 불필요). VS 리소스 편집기는 PNG를 "PNG" 타입 **원본 바이트**로 저장하므로 APNG 애니 데이터(acTL/fdAT)가 보존된다(미리보기가 1프레임인 건 표시상일 뿐).
- 호출:
  ```cpp
  // CSCStatic 배경으로
  m_static.set_back_image(_T("PNG"), IDR_MY_APNG);
  m_static.fit_to_back_image(false);   // 컨트롤 크기에 비율 맞춤

  // 또는 직접
  CSCGdiplusBitmap img;
  img.load(_T("PNG"), UINT(IDR_MY_APNG));   // APNG면 자동 애니, 정적 png면 정적
  ```
- 예전 GIF처럼 커스텀 타입으로 넣고 싶으면 `.rc`에 타입 `APNG`로 등록 후 `load(_T("APNG"), id)`도 가능.

### 3-3. 공용 디코더 직접 호출
```cpp
#include "SCApng.h"   // 프로젝트에 SCApng.cpp 포함(apng.props가 처리)
int w, h;
std::vector<std::vector<uint8_t>> frames;   // 각 프레임 = 캔버스 straight BGRA(w*h*4)
std::vector<int> delays;                     // 프레임별 표시시간(ms)
if (sc_apng::decode(data, size, w, h, frames, delays)) {
    // frames[i] 를 원하는 비트맵으로 감싸 사용(정적 png는 1프레임)
}
```

---

## 4. 참고 / 한계

- **APNG 표준 확장자는 `.png`** (비-APNG 뷰어에서 정적으로 보이게 하려는 스펙 의도). `.apng`도 지원한다.
- **솔루션 탐색기 정리 불가**: props가 추가한 소스는 "소스 파일" 밑에 평평하게 나열되며, 폴더(필터)로 옮겨도 리로드 시 풀린다(props 항목은 `.vcxproj`가 소유하지 않아 `.vcxproj.filters`가 유지 못 함 — VS 한계). 조직화가 꼭 필요하면 props 대신 소스를 각 프로젝트 vcxproj에 직접 넣어야 한다.
- 벤더 소스는 문서/테스트/빌드스크립트/arch 하위폴더 등을 제거하고 **필수 `.c/.h` + LICENSE만** 남겼다.
