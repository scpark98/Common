#include "ffmpeg_internal.h"
#include "ffi_decoder.h"

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
}
