//Include 순서 critical (Common/ffmpeg_internal/ffi_source_filter.cpp 와 동일 이유):
//  winsock2.h → afxwin.h → BaseClasses (streams.h via ffi_source_filter.h).
//  MFC + BaseClasses 의 __POSITION 충돌 회피 + MFC Winsock2 detection.
#include <winsock2.h>
#include <afxwin.h>

#include "ffmpeg_internal.h"
#include "ffi_decoder.h"
#include "ffi_source_filter.h"

#include "../Functions.h"
#include "../log/SCLog/SCLog.h"   //logWrite

#include <afxstr.h>
#include <atomic>
#include <chrono>
#include <thread>
#include <vector>

namespace ffi
{
    //FFmpeg log callback — libav* 내부 로그를 우리 logWrite 로 라우팅.
    //AV_LOG_WARNING 이상만 캡처 (TRACE / DEBUG 너무 verbose).
    static void av_log_cb(void* avcl, int level, const char* fmt, va_list vl)
    {
        if (level > AV_LOG_WARNING)
            return;

        char line[1024];
        vsnprintf(line, sizeof(line), fmt, vl);
        line[sizeof(line) - 1] = '\0';

        //trailing newline 제거 — logWrite 가 자체로 줄바꿈.
        size_t len = strlen(line);
        while (len > 0 && (line[len-1] == '\n' || line[len-1] == '\r'))
            line[--len] = '\0';

        CStringW wline(line);
        logWrite(_T("[av/%d] %s"), level, (LPCTSTR)CString(wline));
    }

    void init_once()
    {
        static std::atomic<bool> done(false);
        bool expected = false;
        if (!done.compare_exchange_strong(expected, true))
            return;

        //modern FFmpeg (4.0+) 는 av_register_all / avformat_network_init 가 자동 / no-op.
        //avformat_network_init 만 명시 호출 — 일부 빌드에서 lazy init.
        avformat_network_init();

        av_log_set_callback(av_log_cb);
        av_log_set_level(AV_LOG_WARNING);

        logWrite(_T("[ffi] init_once — avformat=%u avcodec=%u avutil=%u swresample=%u swscale=%u"),
            avformat_version(),
            avcodec_version(),
            avutil_version(),
            swresample_version(),
            swscale_version());
    }

    int dump_streams(const wchar_t* utf16_path)
    {
        if (!utf16_path)
            return AVERROR(EINVAL);

        //UTF-16 → UTF-8 변환 — FFmpeg 의 file API 는 UTF-8 경로를 받아 내부에서 Windows _wfopen 등으로 처리.
        int u8_len = ::WideCharToMultiByte(CP_UTF8, 0, utf16_path, -1, NULL, 0, NULL, NULL);
        if (u8_len <= 0)
            return AVERROR(EINVAL);
        std::vector<char> u8(u8_len);
        ::WideCharToMultiByte(CP_UTF8, 0, utf16_path, -1, u8.data(), u8_len, NULL, NULL);

        AVFormatContext* fmt = NULL;
        int hr = avformat_open_input(&fmt, u8.data(), NULL, NULL);
        if (hr < 0)
        {
            char buf[256];
            logWrite(_T("[ffi/dump] avformat_open_input fail hr=%d (%hs)"), hr, ffi::err_str(hr, buf, sizeof(buf)));
            return hr;
        }

        hr = avformat_find_stream_info(fmt, NULL);
        if (hr < 0)
        {
            char buf[256];
            logWrite(_T("[ffi/dump] find_stream_info fail hr=%d (%hs)"), hr, ffi::err_str(hr, buf, sizeof(buf)));
            avformat_close_input(&fmt);
            return hr;
        }

        logWrite(_T("[ffi/dump] format=%hs duration=%lldms bit_rate=%lld streams=%u"),
            fmt->iformat ? fmt->iformat->name : "?",
            (long long)(fmt->duration / 1000),
            (long long)fmt->bit_rate,
            (unsigned)fmt->nb_streams);

        for (unsigned i = 0; i < fmt->nb_streams; ++i)
        {
            AVStream* st = fmt->streams[i];
            AVCodecParameters* par = st->codecpar;
            const AVCodec* codec = avcodec_find_decoder(par->codec_id);

            if (par->codec_type == AVMEDIA_TYPE_VIDEO)
            {
                logWrite(_T("[ffi/dump] stream[%u] video codec=%hs %dx%d fps=%.2f pix_fmt=%d"),
                    i,
                    codec ? codec->name : "?",
                    par->width, par->height,
                    st->avg_frame_rate.den ? (double)st->avg_frame_rate.num / st->avg_frame_rate.den : 0.0,
                    par->format);
            }
            else if (par->codec_type == AVMEDIA_TYPE_AUDIO)
            {
                logWrite(_T("[ffi/dump] stream[%u] audio codec=%hs %dHz ch=%d sample_fmt=%d"),
                    i,
                    codec ? codec->name : "?",
                    par->sample_rate, par->ch_layout.nb_channels,
                    par->format);
            }
            else if (par->codec_type == AVMEDIA_TYPE_SUBTITLE)
            {
                logWrite(_T("[ffi/dump] stream[%u] subtitle codec=%hs"),
                    i, codec ? codec->name : "?");
            }
            else
            {
                logWrite(_T("[ffi/dump] stream[%u] type=%d codec=%hs"),
                    i, (int)par->codec_type, codec ? codec->name : "?");
            }
        }

        avformat_close_input(&fmt);
        return 0;
    }

    int decode_test(const wchar_t* utf16_path, int num_frames_to_dump)
    {
        CDecoder dec;
        if (!dec.open(utf16_path))
            return -1;

        logWrite(_T("[ffi/dec_test] open OK %dx%d duration=%.0fms — starting worker"),
            dec.video_width(), dec.video_height(), dec.duration_ms());

        dec.start();

        //frame N 개 받을 때까지 polling.
        int received = 0;
        auto t_start = std::chrono::steady_clock::now();
        while (received < num_frames_to_dump)
        {
            AVFrame* f = dec.pop_video_frame();
            if (!f)
            {
                if (std::chrono::steady_clock::now() - t_start > std::chrono::seconds(5))
                {
                    logWrite(_T("[ffi/dec_test] timeout — only %d frames"), received);
                    break;
                }
                std::this_thread::sleep_for(std::chrono::milliseconds(5));
                continue;
            }
            ++received;
            logWrite(_T("[ffi/dec_test] frame[%d] pts=%lld key=%d %dx%d queue=%zu"),
                received, (long long)f->pts, (int)(f->flags & AV_FRAME_FLAG_KEY),
                f->width, f->height, dec.video_queue_size());
            av_frame_free(&f);
        }

        //seek 테스트 — 미디어 중간 위치.
        double mid_pos_ms = dec.duration_ms() * 0.5;
        logWrite(_T("[ffi/dec_test] seek to %.0fms"), mid_pos_ms);
        auto t_seek = std::chrono::steady_clock::now();
        dec.seek(mid_pos_ms);

        //worker 가 av_seek_frame + queue drain 끝낼 때까지 동기 대기.
        //이걸 안 하면 pop_video_frame 이 pre-seek 잔여 frame 을 반환해 latency 측정이 거의 0 으로 나옴 (버그).
        bool seek_done = dec.wait_seek_done(2000);
        auto t_seek_done = std::chrono::steady_clock::now();
        long long seek_proc_us = std::chrono::duration_cast<std::chrono::microseconds>(t_seek_done - t_seek).count();
        logWrite(_T("[ffi/dec_test] wait_seek_done=%d seek_proc_us=%lld"), (int)seek_done, seek_proc_us);

        //queue drain (worker 이미 drain 했지만 race 방어).
        while (AVFrame* old_f = dec.pop_video_frame())
            av_frame_free(&old_f);

        //이제 진짜 post-seek 첫 frame 까지 시간 측정.
        AVFrame* first_after_seek = nullptr;
        auto t_drain = std::chrono::steady_clock::now();
        while (!first_after_seek)
        {
            first_after_seek = dec.pop_video_frame();
            if (!first_after_seek)
            {
                if (std::chrono::steady_clock::now() - t_drain > std::chrono::seconds(5))
                {
                    logWrite(_T("[ffi/dec_test] post-seek first frame timeout"));
                    break;
                }
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
            }
        }
        if (first_after_seek)
        {
            auto t_first = std::chrono::steady_clock::now();
            long long total_us = std::chrono::duration_cast<std::chrono::microseconds>(t_first - t_seek).count();
            long long decode_us = std::chrono::duration_cast<std::chrono::microseconds>(t_first - t_seek_done).count();
            logWrite(_T("[ffi/dec_test] first post-seek frame pts=%lld total_latency_us=%lld decode_us=%lld"),
                (long long)first_after_seek->pts, total_us, decode_us);
            av_frame_free(&first_after_seek);
        }

        dec.stop();
        dec.close();
        return 0;
    }

    int filter_test(const wchar_t* utf16_path)
    {
        //CSource 는 COM (CUnknown 파생) — worker thread 에서 사용 시 COM init 필요.
        //직접 `delete` 대신 AddRef/Release 로 ref count 가 0 될 때 자동 delete.
        HRESULT hr_coinit = CoInitializeEx(NULL, COINIT_MULTITHREADED);

        HRESULT hr = S_OK;
        CFFiSource* pSource = new CFFiSource(NULL, &hr);
        if (!pSource || FAILED(hr))
        {
            logWrite(_T("[ffi/filter_test] CFFiSource ctor fail hr=0x%08x"), hr);
            if (pSource)
                delete pSource;   //ref count 0 상태라 delete 안전.
            if (SUCCEEDED(hr_coinit)) CoUninitialize();
            return -1;
        }

        pSource->AddRef();   //ref count 1 — 소유권 확보.

        hr = pSource->open_file(utf16_path);
        if (FAILED(hr))
        {
            logWrite(_T("[ffi/filter_test] open_file fail hr=0x%08x"), hr);
            pSource->Release();   //→ ref count 0 → delete this 로 안전 정리.
            if (SUCCEEDED(hr_coinit)) CoUninitialize();
            return -2;
        }

        //pin 메타데이터 dump.
        IEnumPins* pEnum = NULL;
        if (SUCCEEDED(pSource->EnumPins(&pEnum)) && pEnum)
        {
            IPin* pPin = NULL;
            int pidx = 0;
            while (pEnum->Next(1, &pPin, NULL) == S_OK)
            {
                PIN_INFO pi; memset(&pi, 0, sizeof(pi));
                pPin->QueryPinInfo(&pi);
                if (pi.pFilter) pi.pFilter->Release();

                //미디어 타입 확인
                IEnumMediaTypes* pEnumMT = NULL;
                if (SUCCEEDED(pPin->EnumMediaTypes(&pEnumMT)) && pEnumMT)
                {
                    AM_MEDIA_TYPE* pmt = NULL;
                    if (pEnumMT->Next(1, &pmt, NULL) == S_OK && pmt)
                    {
                        if (pmt->formattype == FORMAT_VideoInfo && pmt->pbFormat)
                        {
                            VIDEOINFOHEADER* pvi = (VIDEOINFOHEADER*)pmt->pbFormat;
                            char fourcc[5] = {0};
                            DWORD comp = pvi->bmiHeader.biCompression;
                            for (int i = 0; i < 4; ++i)
                                fourcc[i] = (char)((comp >> (i * 8)) & 0xFF);
                            logWrite(_T("[ffi/filter_test] pin[%d] name=%ls type=Video subtype=%hs %dx%d %d-bpp avg_fps=%.2f"),
                                pidx, pi.achName, fourcc,
                                pvi->bmiHeader.biWidth, pvi->bmiHeader.biHeight,
                                pvi->bmiHeader.biBitCount,
                                pvi->AvgTimePerFrame > 0 ? 10000000.0 / pvi->AvgTimePerFrame : 0.0);
                        }
                        else
                        {
                            logWrite(_T("[ffi/filter_test] pin[%d] name=%ls (non-video media type)"), pidx, pi.achName);
                        }
                        if (pmt->pbFormat) CoTaskMemFree(pmt->pbFormat);
                        CoTaskMemFree(pmt);
                    }
                    pEnumMT->Release();
                }

                pPin->Release();
                ++pidx;
            }
            pEnum->Release();
            logWrite(_T("[ffi/filter_test] total pin count = %d"), pidx);
        }

        //filter Release — ref count 0 → delete this → ~CSource 가 pin 까지 정리.
        pSource->Release();
        if (SUCCEEDED(hr_coinit)) CoUninitialize();
        return 0;
    }
}
