#pragma once

/*
* ffmpeg_internal — 내장 FFmpeg (libavformat / libavcodec / libavutil / libswresample / libswscale) wrapper.
*
*  - Endorphin2 / 기타 프로젝트의 LAV 기반 DirectShow seek delay 한계 (25-604ms SetPositions 블로킹) 우회를 위한
*    PotPlayer 식 내장 FFmpeg 구조의 foundation.
*  - 라이센스: LGPL shared (Gyan release-essentials-shared 또는 BtbN lgpl-shared).
*    binary 는 D:\1.Projects_C++\Common\ffmpeg\{include,lib,bin} 에 위치.
*  - Common/ffmpeg.props 가 include path / lib link / DLL copy 자동 처리.
*
* Phase 1 — Foundation. open + dump streams + close 만. decode 는 Phase 2.
*/

#ifdef __cplusplus
extern "C" {
#endif

#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libavutil/avutil.h>
#include <libavutil/error.h>
#include <libavutil/log.h>
#include <libswresample/swresample.h>
#include <libswscale/swscale.h>

#ifdef __cplusplus
}
#endif

//ffmpeg.props 의 ItemDefinitionGroup 이 vcxproj 의 AdditionalDependencies 에 의해 덮어써지는
//MSBuild 평가 순서 문제 우회 — 컴파일 시점 #pragma comment 로 직접 강제 링크.
//Library path 는 props 의 AdditionalLibraryDirectories 에서 처리 (덮어쓰기 영향 없음).
#pragma comment(lib, "avformat.lib")
#pragma comment(lib, "avcodec.lib")
#pragma comment(lib, "avutil.lib")
#pragma comment(lib, "swresample.lib")
#pragma comment(lib, "swscale.lib")

namespace ffi
{
    //프로세스 lifetime 동안 1회 호출되는 초기화. log callback 설치 + 네트워크 모듈 init 등.
    //멀티 호출은 no-op (idempotent).
    void                init_once();

    //Phase 1 smoke test — file 열어 stream 정보만 dump 후 close. utf-16 path 안전 처리.
    //성공 시 0, 실패 시 av error code (음수). 로그에 [ffi/dump] 로 stream 들 출력.
    int                 dump_streams(const wchar_t* utf16_path);

    //Phase 2 smoke test — CDecoder open/start/decode-N-frames/seek/stop/close 한 사이클 실행.
    //frame N 개 디코드 후 미디어 중간 위치 seek + 추가 N 개 디코드. 각 frame 의 pts / decode 소요시간 로그.
    int                 decode_test(const wchar_t* utf16_path, int num_frames_to_dump = 10);

    //av_err2str 의 C++ 호환 buffer 버전. 매크로 형태가 일부 컴파일러에서 문제 일으켜 인라인 함수로 대체.
    inline const char*  err_str(int err_code, char* buf, size_t buf_size)
    {
        av_strerror(err_code, buf, buf_size);
        return buf;
    }
}
