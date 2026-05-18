#include "ffmpeg_internal.h"

#include "../Functions.h"
#include "../log/SCLog/SCLog.h"   //logWrite

#include <afxstr.h>
#include <atomic>
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
}
