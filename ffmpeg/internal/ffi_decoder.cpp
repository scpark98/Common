#include "ffi_decoder.h"

#include "../../Functions.h"
#include "../../log/SCLog/SCLog.h"

#include <windows.h>

extern "C" {
#include <libavutil/hwcontext.h>
#include <libavutil/pixdesc.h>
}

#include <vector>

namespace
{
    //CDecoder::open 에서 ctx->opaque 에 target hw_pix_fmt 저장. callback 이 FFmpeg internal thread 에서 호출돼도
    //ctx 매개변수 통해 안전히 접근 (thread_local 은 다른 thread 에서 안 보임).
    enum AVPixelFormat get_hw_format(AVCodecContext* ctx, const enum AVPixelFormat* pix_fmts)
    {
        AVPixelFormat target = (AVPixelFormat)(intptr_t)ctx->opaque;
        for (const enum AVPixelFormat* p = pix_fmts; *p != AV_PIX_FMT_NONE; ++p)
        {
            if (*p == target)
                return *p;
        }
        //fallback — 첫 format (보통 SW format). decoder open 자체는 성공.
        return pix_fmts[0];
    }
}

namespace ffi
{
    CDecoder::CDecoder() {}

    CDecoder::~CDecoder()
    {
        close();
    }

    int CDecoder::avio_read_cb(void* opaque, uint8_t* buf, int buf_size)
    {
        CDecoder* self = (CDecoder*)opaque;
        DWORD read = 0;
        if (!::ReadFile(self->m_file_handle, buf, (DWORD)buf_size, &read, NULL))
            return AVERROR(EIO);
        if (read == 0)
            return AVERROR_EOF;
        return (int)read;
    }

    int64_t CDecoder::avio_seek_cb(void* opaque, int64_t offset, int whence)
    {
        CDecoder* self = (CDecoder*)opaque;
        if (whence == AVSEEK_SIZE)
        {
            LARGE_INTEGER sz;
            if (!::GetFileSizeEx(self->m_file_handle, &sz))
                return AVERROR(EIO);
            return sz.QuadPart;
        }
        DWORD method;
        switch (whence & ~AVSEEK_FORCE)
        {
            case SEEK_CUR: method = FILE_CURRENT; break;
            case SEEK_END: method = FILE_END;     break;
            default:       method = FILE_BEGIN;   break;
        }
        LARGE_INTEGER pos;
        pos.QuadPart = offset;
        LARGE_INTEGER new_pos;
        if (!::SetFilePointerEx(self->m_file_handle, pos, &new_pos, method))
            return AVERROR(EIO);
        return new_pos.QuadPart;
    }

    bool CDecoder::open(const wchar_t* utf16_path)
    {
        if (m_fmt)
            close();
        if (!utf16_path)
            return false;

        //Win32 CreateFile 로 직접 handle 열기 — FILE_SHARE_DELETE 포함해 외부 rename / move 시
        //sharing violation 회피. avformat 기본 file protocol (_wsopen 기반) 은 FILE_SHARE_DELETE 미설정.
        HANDLE h = ::CreateFileW(utf16_path, GENERIC_READ,
            FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
            NULL, OPEN_EXISTING,
            FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN, NULL);
        if (h == INVALID_HANDLE_VALUE)
        {
            logWrite(_T("[ffi/dec] CreateFile fail err=%u"), ::GetLastError());
            return false;
        }
        m_file_handle = h;

        //custom AVIOContext — avio_alloc_context 의 buffer 는 av_malloc, avformat 이 내부에서 재할당 가능.
        //close 시 m_avio->buffer 를 av_freep + avio_context_free 필요.
        const int kAvioBufSize = 32 * 1024;
        uint8_t* avio_buf = (uint8_t*)av_malloc(kAvioBufSize);
        if (!avio_buf)
        {
            ::CloseHandle(m_file_handle);
            m_file_handle = nullptr;
            return false;
        }
        m_avio = avio_alloc_context(avio_buf, kAvioBufSize, 0 /*write_flag*/,
            this, &CDecoder::avio_read_cb, NULL, &CDecoder::avio_seek_cb);
        if (!m_avio)
        {
            av_free(avio_buf);
            ::CloseHandle(m_file_handle);
            m_file_handle = nullptr;
            return false;
        }

        m_fmt = avformat_alloc_context();
        if (!m_fmt)
        {
            av_freep(&m_avio->buffer);
            avio_context_free(&m_avio);
            ::CloseHandle(m_file_handle);
            m_file_handle = nullptr;
            return false;
        }
        m_fmt->pb = m_avio;
        m_fmt->flags |= AVFMT_FLAG_CUSTOM_IO;

        //URL=NULL 이면 avformat 이 m_fmt->pb 사용. probe 도 callback 으로 처리.
        int hr = avformat_open_input(&m_fmt, NULL, NULL, NULL);
        if (hr < 0)
        {
            char buf[256];
            logWrite(_T("[ffi/dec] avformat_open_input fail hr=%d (%hs)"), hr, ffi::err_str(hr, buf, sizeof(buf)));
            //avformat_open_input 실패 시 m_fmt 는 자동 해제 (NULL set). close() 가 m_avio + handle 정리.
            close();
            return false;
        }

        hr = avformat_find_stream_info(m_fmt, NULL);
        if (hr < 0)
        {
            char buf[256];
            logWrite(_T("[ffi/dec] find_stream_info fail hr=%d (%hs)"), hr, ffi::err_str(hr, buf, sizeof(buf)));
            close();
            return false;
        }

        //video stream 선택 — av_find_best_stream 이 자동으로 가장 적합한 video stream 고름.
        m_video_stream_idx = av_find_best_stream(m_fmt, AVMEDIA_TYPE_VIDEO, -1, -1, NULL, 0);
        if (m_video_stream_idx < 0)
        {
            logWrite(_T("[ffi/dec] no video stream"));
            close();
            return false;
        }

        //decoder 준비
        AVCodecParameters* par = m_fmt->streams[m_video_stream_idx]->codecpar;
        const AVCodec* codec = avcodec_find_decoder(par->codec_id);
        if (!codec)
        {
            logWrite(_T("[ffi/dec] no decoder for codec_id=%d"), (int)par->codec_id);
            close();
            return false;
        }

        m_video_ctx = avcodec_alloc_context3(codec);
        if (!m_video_ctx)
        {
            close();
            return false;
        }

        avcodec_parameters_to_context(m_video_ctx, par);

        //HW 가속 setup — D3D11VA 우선, 그 다음 DXVA2. codec 이 지원하는 첫 HW config 선택.
        //SW keyframe walk decode 의 freeze 해소가 본 목적.
        {
            AVHWDeviceType try_types[2] = { AV_HWDEVICE_TYPE_D3D11VA, AV_HWDEVICE_TYPE_DXVA2 };
            for (int t = 0; t < 2 && m_hw_pix_fmt == AV_PIX_FMT_NONE; ++t)
            {
                AVHWDeviceType type = try_types[t];
                for (int i = 0;; ++i)
                {
                    const AVCodecHWConfig* cfg = avcodec_get_hw_config(codec, i);
                    if (!cfg) break;
                    if ((cfg->methods & AV_CODEC_HW_CONFIG_METHOD_HW_DEVICE_CTX) &&
                        cfg->device_type == type)
                    {
                        m_hw_pix_fmt = cfg->pix_fmt;
                        break;
                    }
                }
                if (m_hw_pix_fmt != AV_PIX_FMT_NONE)
                {
                    AVBufferRef* dev = NULL;
                    int dhr = av_hwdevice_ctx_create(&dev, type, NULL, NULL, 0);
                    if (dhr >= 0 && dev)
                    {
                        m_hw_device_ctx = dev;
                        m_video_ctx->hw_device_ctx = av_buffer_ref(m_hw_device_ctx);
                        m_video_ctx->opaque = (void*)(intptr_t)m_hw_pix_fmt;   //get_hw_format callback 이 ctx->opaque 통해 접근.
                        m_video_ctx->get_format = get_hw_format;
                        logWrite(_T("[ffi/dec] HW accel enabled type=%hs hw_pix_fmt=%d"),
                            av_hwdevice_get_type_name(type), (int)m_hw_pix_fmt);
                        break;
                    }
                    else
                    {
                        logWrite(_T("[ffi/dec] HW device create fail type=%hs hr=%d — try next"),
                            av_hwdevice_get_type_name(type), dhr);
                        m_hw_pix_fmt = AV_PIX_FMT_NONE;
                    }
                }
            }
            if (m_hw_pix_fmt == AV_PIX_FMT_NONE)
                logWrite(_T("[ffi/dec] no HW accel for codec=%hs — fallback to SW"), codec->name);
        }

        //multi-threaded decode — FFmpeg 의 thread_type 자동 (frame + slice). PotPlayer 의 "Thread Frame" 과 동일.
        //HW decode 일 때는 thread_count 가 무시되거나 제한적.
        m_video_ctx->thread_count = 0;   //0 = auto = available CPU cores
        m_video_ctx->thread_type  = FF_THREAD_FRAME | FF_THREAD_SLICE;

        hr = avcodec_open2(m_video_ctx, codec, NULL);
        if (hr < 0)
        {
            char buf[256];
            logWrite(_T("[ffi/dec] avcodec_open2 fail hr=%d (%hs)"), hr, ffi::err_str(hr, buf, sizeof(buf)));
            close();
            return false;
        }

        logWrite(_T("[ffi/dec] opened codec=%hs %dx%d threads=%d"),
            codec->name, m_video_ctx->width, m_video_ctx->height, m_video_ctx->thread_count);

        //모든 audio stream enumerate + display name 빌드. m_audio_stream_indices 가 menu 의 track index 와 1:1.
        m_audio_stream_indices.clear();
        m_audio_track_names.clear();
        m_audio_track_current = -1;

        for (unsigned i = 0; i < m_fmt->nb_streams; ++i)
        {
            AVCodecParameters* p = m_fmt->streams[i]->codecpar;
            if (p->codec_type != AVMEDIA_TYPE_AUDIO) continue;
            const AVCodec* c = avcodec_find_decoder(p->codec_id);
            AVDictionaryEntry* lang  = av_dict_get(m_fmt->streams[i]->metadata, "language", NULL, 0);
            AVDictionaryEntry* title = av_dict_get(m_fmt->streams[i]->metadata, "title",    NULL, 0);

            const char* codec_str = c ? c->name : "?";
            const char* lang_str  = (lang  && lang->value)  ? lang->value  : "";
            const char* title_str = (title && title->value) ? title->value : "";

            int track_idx = (int)m_audio_stream_indices.size();
            m_audio_stream_indices.push_back((int)i);

            wchar_t buf[256];
            if (title_str[0] && lang_str[0])
                swprintf_s(buf, L"트랙 %d : %hs [%hs] / %hs / %dch / %dHz",
                    track_idx + 1, title_str, lang_str, codec_str,
                    p->ch_layout.nb_channels, p->sample_rate);
            else if (title_str[0])
                swprintf_s(buf, L"트랙 %d : %hs / %hs / %dch / %dHz",
                    track_idx + 1, title_str, codec_str,
                    p->ch_layout.nb_channels, p->sample_rate);
            else if (lang_str[0])
                swprintf_s(buf, L"트랙 %d : %hs / %hs / %dch / %dHz",
                    track_idx + 1, lang_str, codec_str,
                    p->ch_layout.nb_channels, p->sample_rate);
            else
                swprintf_s(buf, L"트랙 %d : %hs / %dch / %dHz",
                    track_idx + 1, codec_str,
                    p->ch_layout.nb_channels, p->sample_rate);

            m_audio_track_names.push_back(buf);

            logWrite(_T("[ffi/dec] audio[%d→stream%u] %s"), track_idx, i, buf);
        }

        //초기 선택: m_initial_audio_track 우선 (track switch 의 close+reopen 채널), 없으면 av_find_best_stream.
        int chosen_stream_idx = -1;
        if (m_initial_audio_track >= 0 && m_initial_audio_track < (int)m_audio_stream_indices.size())
        {
            chosen_stream_idx = m_audio_stream_indices[m_initial_audio_track];
            m_audio_track_current = m_initial_audio_track;
            logWrite(_T("[ffi/dec] audio track forced: track %d → stream %d"),
                m_initial_audio_track, chosen_stream_idx);
        }
        else if (!m_audio_stream_indices.empty())
        {
            int best = av_find_best_stream(m_fmt, AVMEDIA_TYPE_AUDIO, -1, -1, NULL, 0);
            if (best >= 0)
            {
                chosen_stream_idx = best;
                for (size_t k = 0; k < m_audio_stream_indices.size(); ++k)
                {
                    if (m_audio_stream_indices[k] == best)
                    {
                        m_audio_track_current = (int)k;
                        break;
                    }
                }
                if (m_audio_track_current < 0)
                {
                    chosen_stream_idx = m_audio_stream_indices[0];
                    m_audio_track_current = 0;
                }
            }
            else
            {
                chosen_stream_idx = m_audio_stream_indices[0];
                m_audio_track_current = 0;
            }
            logWrite(_T("[ffi/dec] audio track auto: track %d → stream %d"),
                m_audio_track_current, chosen_stream_idx);
        }
        m_initial_audio_track = -1;   //consume

        m_audio_stream_idx = chosen_stream_idx;
        logWrite(_T("[ffi/dec/diag] selected audio stream idx=%d"), m_audio_stream_idx);
        if (m_audio_stream_idx >= 0)
        {
            AVCodecParameters* apar = m_fmt->streams[m_audio_stream_idx]->codecpar;
            const AVCodec* acodec = avcodec_find_decoder(apar->codec_id);
            if (acodec)
            {
                m_audio_ctx = avcodec_alloc_context3(acodec);
                if (m_audio_ctx)
                {
                    avcodec_parameters_to_context(m_audio_ctx, apar);
                    int ahr = avcodec_open2(m_audio_ctx, acodec, NULL);
                    if (ahr < 0)
                    {
                        logWrite(_T("[ffi/dec] audio codec open fail hr=%d — audio disabled"), ahr);
                        avcodec_free_context(&m_audio_ctx);
                        m_audio_stream_idx = -1;
                    }
                    else
                    {
                        logWrite(_T("[ffi/dec] opened audio codec=%hs %dHz ch=%d sample_fmt=%d"),
                            acodec->name, m_audio_ctx->sample_rate,
                            m_audio_ctx->ch_layout.nb_channels, (int)m_audio_ctx->sample_fmt);
                    }
                }
            }
            else
            {
                logWrite(_T("[ffi/dec] no audio decoder for codec_id=%d"), (int)apar->codec_id);
                m_audio_stream_idx = -1;
            }
        }

        //subtitle stream enumeration — internal path 는 디코딩/렌더링 미지원, metadata 만 빌드.
        //메뉴에서 트랙 보이게 + 사용자가 클릭하면 LAV path 로 close+reopen.
        m_subtitle_track_names.clear();
        for (unsigned i = 0; i < m_fmt->nb_streams; ++i)
        {
            AVCodecParameters* p = m_fmt->streams[i]->codecpar;
            if (p->codec_type != AVMEDIA_TYPE_SUBTITLE) continue;
            const AVCodec* c = avcodec_find_decoder(p->codec_id);
            AVDictionaryEntry* lang  = av_dict_get(m_fmt->streams[i]->metadata, "language", NULL, 0);
            AVDictionaryEntry* title = av_dict_get(m_fmt->streams[i]->metadata, "title",    NULL, 0);

            const char* codec_str = c ? c->name : "?";
            const char* lang_str  = (lang  && lang->value)  ? lang->value  : "";
            const char* title_str = (title && title->value) ? title->value : "";

            int track_idx = (int)m_subtitle_track_names.size();

            wchar_t buf[256];
            if (title_str[0] && lang_str[0])
                swprintf_s(buf, L"트랙 %d : %hs [%hs] / %hs",
                    track_idx + 1, title_str, lang_str, codec_str);
            else if (title_str[0])
                swprintf_s(buf, L"트랙 %d : %hs / %hs",
                    track_idx + 1, title_str, codec_str);
            else if (lang_str[0])
                swprintf_s(buf, L"트랙 %d : %hs / %hs",
                    track_idx + 1, lang_str, codec_str);
            else
                swprintf_s(buf, L"트랙 %d : %hs",
                    track_idx + 1, codec_str);

            m_subtitle_track_names.push_back(buf);
            logWrite(_T("[ffi/dec] subtitle[%d→stream%u] %s"), track_idx, i, buf);
        }

        return true;
    }

    void CDecoder::close()
    {
        stop();

        if (m_video_ctx)
            avcodec_free_context(&m_video_ctx);
        if (m_audio_ctx)
            avcodec_free_context(&m_audio_ctx);

        if (m_hw_device_ctx)
            av_buffer_unref(&m_hw_device_ctx);
        m_hw_pix_fmt = AV_PIX_FMT_NONE;

        if (m_fmt)
            avformat_close_input(&m_fmt);

        //AVFMT_FLAG_CUSTOM_IO 라 avformat_close_input 이 m_fmt->pb 를 정리하지 않음 — caller 책임.
        if (m_avio)
        {
            av_freep(&m_avio->buffer);
            avio_context_free(&m_avio);
        }
        if (m_file_handle)
        {
            ::CloseHandle(m_file_handle);
            m_file_handle = nullptr;
        }

        m_video_stream_idx = -1;
        m_audio_stream_idx = -1;
        m_audio_stream_indices.clear();
        m_audio_track_names.clear();
        m_audio_track_current = -1;
        m_subtitle_track_names.clear();
    }

    void CDecoder::start()
    {
        if (!m_fmt || m_thread.joinable())
            return;
        m_quit.store(false);
        m_thread = std::thread(&CDecoder::worker_loop, this);
        logWrite(_T("[ffi/dec] worker started"));
    }

    void CDecoder::stop()
    {
        if (!m_thread.joinable())
            return;
        m_quit.store(true);
        m_cv_queue.notify_all();
        m_thread.join();

        //queue 의 남은 frame 들 release.
        std::unique_lock<std::mutex> lk(m_mtx_queue);
        while (!m_video_queue.empty())
        {
            AVFrame* f = m_video_queue.front();
            m_video_queue.pop_front();
            av_frame_free(&f);
        }
        while (!m_audio_queue.empty())
        {
            AVFrame* f = m_audio_queue.front();
            m_audio_queue.pop_front();
            av_frame_free(&f);
        }
        //[reorder] worker 종료(join 완료)라 reorder 버퍼에 남은 frame free.
        while (!m_video_reorder.empty())
        {
            AVFrame* f = m_video_reorder.front();
            m_video_reorder.pop_front();
            av_frame_free(&f);
        }
        logWrite(_T("[ffi/dec] worker stopped"));
    }

    void CDecoder::seek(double pos_ms, int64_t segment_rt)
    {
        //Generation 증가는 worker 의 av_seek_frame 처리 후. caller 에서 ++ 하면 worker 가 이미 디코드 중인 frame 들도 stale tag →
        //pop drop → freeze. worker 가 새 위치 처리 후부터 새 generation push 가 정공.
        //m_pending_segment_rt 저장 — worker 가 av_seek_frame 후 m_segment_rt 갱신할 때 사용.
        m_pending_segment_rt = segment_rt;

        {
            std::unique_lock<std::mutex> lk(m_mtx_seek);
            m_pending_seek_ms = pos_ms;
            m_seek_processed = false;
        }

        {
            std::unique_lock<std::mutex> lk(m_mtx_queue);
            while (!m_video_queue.empty())
            {
                AVFrame* f = m_video_queue.front();
                m_video_queue.pop_front();
                av_frame_free(&f);
            }
            while (!m_audio_queue.empty())
            {
                AVFrame* f = m_audio_queue.front();
                m_audio_queue.pop_front();
                av_frame_free(&f);
            }
        }

        m_cv_queue.notify_all();   //worker 가 wait 중이면 깨움.
    }

    bool CDecoder::wait_seek_done(int timeout_ms)
    {
        std::unique_lock<std::mutex> lk(m_mtx_seek);
        return m_cv_seek_done.wait_for(lk, std::chrono::milliseconds(timeout_ms),
            [this]() { return m_seek_processed; });
    }

    AVFrame* CDecoder::pop_video_frame()
    {
        int current = m_seek_generation.load();
        std::unique_lock<std::mutex> lk(m_mtx_queue);
        while (!m_video_queue.empty())
        {
            AVFrame* f = m_video_queue.front();
            m_video_queue.pop_front();
            if ((intptr_t)f->opaque == current)
            {
                m_cv_queue.notify_one();
                return f;
            }
            //이전 generation 의 stale frame — skip + free.
            av_frame_free(&f);
        }
        m_cv_queue.notify_one();
        return nullptr;
    }

    AVFrame* CDecoder::pop_audio_frame()
    {
        int current = m_seek_generation.load();
        std::unique_lock<std::mutex> lk(m_mtx_queue);
        while (!m_audio_queue.empty())
        {
            AVFrame* f = m_audio_queue.front();
            m_audio_queue.pop_front();
            if ((intptr_t)f->opaque == current)
            {
                m_cv_queue.notify_one();
                return f;
            }
            av_frame_free(&f);
        }
        m_cv_queue.notify_one();
        return nullptr;
    }

    size_t CDecoder::video_queue_size()
    {
        std::unique_lock<std::mutex> lk(m_mtx_queue);
        return m_video_queue.size();
    }

    size_t CDecoder::audio_queue_size()
    {
        std::unique_lock<std::mutex> lk(m_mtx_queue);
        return m_audio_queue.size();
    }

    int CDecoder::video_width() const
    {
        return m_video_ctx ? m_video_ctx->width : 0;
    }

    int CDecoder::video_height() const
    {
        return m_video_ctx ? m_video_ctx->height : 0;
    }

    double CDecoder::duration_ms() const
    {
        if (!m_fmt || m_fmt->duration == AV_NOPTS_VALUE)
            return 0.0;
        return (double)m_fmt->duration / 1000.0;   //AV_TIME_BASE = 1000000 (μs), /1000 → ms.
    }

    double CDecoder::frame_rate() const
    {
        if (!m_fmt || m_video_stream_idx < 0)
            return 0.0;
        AVRational r = m_fmt->streams[m_video_stream_idx]->avg_frame_rate;
        if (r.den == 0)
            return 0.0;
        return (double)r.num / (double)r.den;
    }

    AVRational CDecoder::video_time_base() const
    {
        AVRational zero = { 0, 1 };
        if (!m_fmt || m_video_stream_idx < 0)
            return zero;
        return m_fmt->streams[m_video_stream_idx]->time_base;
    }

    int CDecoder::video_pixel_format() const
    {
        return m_video_ctx ? (int)m_video_ctx->pix_fmt : (int)AV_PIX_FMT_NONE;
    }

    int CDecoder::audio_sample_rate() const
    {
        return m_audio_ctx ? m_audio_ctx->sample_rate : 0;
    }

    int CDecoder::audio_channels() const
    {
        return m_audio_ctx ? m_audio_ctx->ch_layout.nb_channels : 0;
    }

    int CDecoder::audio_sample_format() const
    {
        return m_audio_ctx ? (int)m_audio_ctx->sample_fmt : (int)AV_SAMPLE_FMT_NONE;
    }

    AVRational CDecoder::audio_time_base() const
    {
        AVRational zero = { 0, 1 };
        if (!m_fmt || m_audio_stream_idx < 0)
            return zero;
        return m_fmt->streams[m_audio_stream_idx]->time_base;
    }

    //--- display info — UI 표시용 (PotPlayer Tab OSD 식 codec/format 표시).
    //AVCodecParameters / AVStream 의 정보 직접 query. MediaInfo lib 의존 없음.

    static std::wstring utf8_to_wstring_upper(const char* s)
    {
        std::wstring out;
        if (!s)
            return out;
        while (*s)
        {
            wchar_t c = (wchar_t)(unsigned char)(*s++);
            if (c >= L'a' && c <= L'z')
                c -= 32;
            out += c;
        }
        return out;
    }

    static std::wstring utf8_to_wstring(const char* s)
    {
        std::wstring out;
        if (!s)
            return out;
        while (*s)
            out += (wchar_t)(unsigned char)(*s++);
        return out;
    }

    static std::wstring fourcc_to_wstring(uint32_t tag)
    {
        std::wstring out;
        if (tag == 0)
            return out;
        for (int i = 0; i < 4; i++)
        {
            unsigned char c = (tag >> (i * 8)) & 0xFF;
            if (c >= 0x20 && c < 0x7F)
                out += (wchar_t)c;
        }
        return out;
    }

    std::wstring CDecoder::video_codec_name() const
    {
        if (!m_fmt || m_video_stream_idx < 0)
            return L"";
        AVCodecParameters* par = m_fmt->streams[m_video_stream_idx]->codecpar;
        //실제 디코더명 그대로 (PotPlayer 와 동일 표기): h264 / hevc 등 소문자. find 실패 시 codec id 명 fallback.
        const AVCodec* dec = avcodec_find_decoder(par->codec_id);
        return utf8_to_wstring(dec ? dec->name : avcodec_get_name(par->codec_id));
    }

    std::wstring CDecoder::video_fourcc() const
    {
        if (!m_fmt || m_video_stream_idx < 0)
            return L"";
        AVCodecParameters* par = m_fmt->streams[m_video_stream_idx]->codecpar;
        return fourcc_to_wstring(par->codec_tag);
    }

    int CDecoder::video_bit_depth() const
    {
        if (!m_fmt || m_video_stream_idx < 0)
            return 0;
        AVCodecParameters* par = m_fmt->streams[m_video_stream_idx]->codecpar;
        if (par->bits_per_raw_sample > 0)
            return par->bits_per_raw_sample;
        if (par->bits_per_coded_sample > 0)
            return par->bits_per_coded_sample;
        const AVPixFmtDescriptor* d = av_pix_fmt_desc_get((AVPixelFormat)par->format);
        if (d && d->nb_components > 0)
            return d->comp[0].depth;
        return 0;
    }

    int64_t CDecoder::video_bit_rate() const
    {
        if (!m_fmt || m_video_stream_idx < 0)
            return 0;
        AVCodecParameters* par = m_fmt->streams[m_video_stream_idx]->codecpar;
        return par->bit_rate;
    }

    std::wstring CDecoder::video_aspect_ratio() const
    {
        if (!m_fmt || m_video_stream_idx < 0)
            return L"";
        AVStream* s = m_fmt->streams[m_video_stream_idx];
        AVCodecParameters* par = s->codecpar;
        AVRational sar = s->sample_aspect_ratio;
        if (sar.num == 0 || sar.den == 0)
            sar = par->sample_aspect_ratio;
        int dar_num = par->width;
        int dar_den = par->height;
        if (sar.num > 0 && sar.den > 0)
        {
            dar_num = par->width * sar.num;
            dar_den = par->height * sar.den;
        }
        if (dar_num <= 0 || dar_den <= 0)
            return L"";
        //gcd 로 simplify.
        int a = dar_num, b = dar_den;
        while (b) { int t = a % b; a = b; b = t; }
        if (a > 0)
        {
            dar_num /= a;
            dar_den /= a;
        }
        wchar_t buf[32];
        swprintf_s(buf, 32, L"%d:%d", dar_num, dar_den);
        return buf;
    }

    std::wstring CDecoder::video_pixel_format_name() const
    {
        AVPixelFormat fmt = (AVPixelFormat)video_pixel_format();
        if (fmt == AV_PIX_FMT_NONE)
            return L"";
        const char* name = av_get_pix_fmt_name(fmt);
        if (!name)
            return L"";
        //pixel format 이름은 lower 그대로 (yuv420p, nv12 등 관례).
        std::wstring out;
        while (*name)
            out += (wchar_t)(unsigned char)(*name++);
        return out;
    }

    std::wstring CDecoder::video_hw_accel_name() const
    {
        if (!m_hw_device_ctx)
            return L"";
        AVHWDeviceContext* hwctx = (AVHWDeviceContext*)m_hw_device_ctx->data;
        const char* name = av_hwdevice_get_type_name(hwctx->type);
        return utf8_to_wstring_upper(name);
    }

    std::wstring CDecoder::audio_codec_name() const
    {
        if (!m_fmt || m_audio_stream_idx < 0)
            return L"";
        AVCodecParameters* par = m_fmt->streams[m_audio_stream_idx]->codecpar;
        //실제 디코더명 그대로 (PotPlayer 와 동일): mp3float / aac 등 소문자.
        const AVCodec* dec = avcodec_find_decoder(par->codec_id);
        return utf8_to_wstring(dec ? dec->name : avcodec_get_name(par->codec_id));
    }

    std::wstring CDecoder::audio_channel_layout_name() const
    {
        if (!m_audio_ctx)
            return L"";
        char buf[64] = { 0 };
        int n = av_channel_layout_describe(&m_audio_ctx->ch_layout, buf, sizeof(buf));
        if (n <= 0)
            return L"";
        std::wstring out;
        for (int i = 0; i < n - 1 && buf[i]; i++)
            out += (wchar_t)(unsigned char)buf[i];
        return out;
    }

    int CDecoder::audio_bit_depth() const
    {
        if (!m_fmt || m_audio_stream_idx < 0)
            return 0;
        AVCodecParameters* par = m_fmt->streams[m_audio_stream_idx]->codecpar;
        return par->bits_per_coded_sample;
    }

    int64_t CDecoder::audio_bit_rate() const
    {
        if (!m_fmt || m_audio_stream_idx < 0)
            return 0;
        AVCodecParameters* par = m_fmt->streams[m_audio_stream_idx]->codecpar;
        return par->bit_rate;
    }

    const std::wstring& CDecoder::audio_track_name(int track_idx) const
    {
        static const std::wstring empty;
        if (track_idx < 0 || track_idx >= (int)m_audio_track_names.size())
            return empty;
        return m_audio_track_names[track_idx];
    }

    const std::wstring& CDecoder::subtitle_track_name(int track_idx) const
    {
        static const std::wstring empty;
        if (track_idx < 0 || track_idx >= (int)m_subtitle_track_names.size())
            return empty;
        return m_subtitle_track_names[track_idx];
    }

    void CDecoder::worker_loop()
    {
        AVPacket* pkt = av_packet_alloc();

        while (!m_quit.load())
        {
            //=== 1) seek 요청 처리 ===
            {
                std::unique_lock<std::mutex> lk(m_mtx_seek);
                if (m_pending_seek_ms >= 0)
                {
                    double seek_pos = m_pending_seek_ms;
                    m_pending_seek_ms = -1.0;
                    lk.unlock();

                    //queue flush — 이전 위치 frame 들 모두 버림. video + audio 둘 다.
                    {
                        std::unique_lock<std::mutex> lkq(m_mtx_queue);
                        while (!m_video_queue.empty())
                        {
                            AVFrame* f = m_video_queue.front();
                            m_video_queue.pop_front();
                            av_frame_free(&f);
                        }
                        while (!m_audio_queue.empty())
                        {
                            AVFrame* f = m_audio_queue.front();
                            m_audio_queue.pop_front();
                            av_frame_free(&f);
                        }
                    }

                    //[reorder] worker-local reorder 버퍼의 이전 위치 frame 도 free (stale generation).
                    while (!m_video_reorder.empty())
                    {
                        AVFrame* f = m_video_reorder.front();
                        m_video_reorder.pop_front();
                        av_frame_free(&f);
                    }

                    int64_t target_us = (int64_t)(seek_pos * AV_TIME_BASE / 1000.0);

                    LARGE_INTEGER qpc_freq, qpc_t0, qpc_t1;
                    ::QueryPerformanceFrequency(&qpc_freq);
                    ::QueryPerformanceCounter(&qpc_t0);

                    //stream_index=video 로 명시 + target 을 video stream time_base 단위로 변환.
                    //stream_index=-1 (format-level) 은 *어느* stream 의 keyframe 으로 갈지 모호.
                    //sparse keyframe 미디어 (MPEG-TS 29초 GOP / .mp4-as-TS 등) 에서 av_seek_frame BACKWARD 가
                    //*prefer* 일 뿐 *force* 가 아니라 target 보다 *늦은 keyframe* 으로 forward fallback 발생.
                    //fix: target_pts - 1초 margin 으로 seek → 항상 backward keyframe 보장 + source filter 의 pre-target
                    //skip 이 *원래 target* 까지 forward decode + emit. 1초 = 30 frame ≈ 150ms HW decode (사용자 perception
                    //영향 작음). frame step backward 시도 정확히 1 frame 이전 frame emit.
                    AVRational vtb = m_fmt->streams[m_video_stream_idx]->time_base;
                    int64_t target_pts = av_rescale_q(target_us,
                        AVRational{1, AV_TIME_BASE}, vtb);
                    int64_t margin_pts = av_rescale_q(1 * AV_TIME_BASE,
                        AVRational{1, AV_TIME_BASE}, vtb);   //1초 margin
                    int64_t safe_target_pts = (target_pts > margin_pts) ? target_pts - margin_pts : 0;
                    int hr_seek = av_seek_frame(m_fmt, m_video_stream_idx, safe_target_pts, AVSEEK_FLAG_BACKWARD);
                    avcodec_flush_buffers(m_video_ctx);
                    if (m_audio_ctx)
                        avcodec_flush_buffers(m_audio_ctx);
                    m_eof.store(false);   //seek 후 새 위치부터 재생 — EOF 상태 해제.

                    ::QueryPerformanceCounter(&qpc_t1);
                    long long seek_us = (qpc_t1.QuadPart - qpc_t0.QuadPart) * 1000000LL / qpc_freq.QuadPart;

                    m_segment_rt.store(m_pending_segment_rt);
                    m_video_first_emit_pts_rt.store(LLONG_MIN);   //다음 video frame push 시 set.
                    int new_gen = m_seek_generation.fetch_add(1) + 1;
                    logWrite(_T("[ffi/dec] seek to %.0fms hr=%d gen=%d seg_rt=%lld seek_us=%lld"),
                        seek_pos, hr_seek, new_gen, (long long)m_pending_segment_rt, seek_us);

                    //first frame 시간 측정용 — worker 가 다음 frame push 시점.
                    m_first_frame_after_seek = false;
                    m_seek_done_qpc = qpc_t1.QuadPart;

                    //seek 처리 완료 — wait_seek_done() 동기 대기 중인 caller 에게 신호.
                    {
                        std::unique_lock<std::mutex> lkd(m_mtx_seek);
                        m_seek_processed = true;
                    }
                    m_cv_seek_done.notify_all();
                    continue;   //다음 iteration 부터 새 위치 디코드.
                }
            }

            //=== 2) queue 가득 차면 wait ===
            //video 또는 audio 둘 다 max 이상이면 wait. 둘 중 하나 비어 있으면 packet 읽고 그쪽 채움.
            {
                std::unique_lock<std::mutex> lk(m_mtx_queue);
                m_cv_queue.wait(lk, [this]() {
                    return m_quit.load() ||
                           ((int)m_video_queue.size() < m_max_queue) ||
                           ((int)m_audio_queue.size() < m_max_audio_queue) ||
                           [this]() {
                               std::unique_lock<std::mutex> lks(m_mtx_seek);
                               return m_pending_seek_ms >= 0;
                           }();
                });
                if (m_quit.load())
                    break;
            }

            //pending seek 다시 체크 — wait 중에 seek 가 들어왔으면 우선 처리.
            {
                std::unique_lock<std::mutex> lk(m_mtx_seek);
                if (m_pending_seek_ms >= 0)
                    continue;
            }

            //=== 3) packet 읽기 ===
            int hr = av_read_frame(m_fmt, pkt);
            if (hr < 0)
            {
                //EOF — flag set. queue 비면 source FillBuffer 가 S_FALSE 반환 → DeliverEndOfStream → renderer EC_COMPLETE 발화.
                //seek 가 오면 av_seek_frame 처리 시 clear.
                //[reorder] EOF 전이 시 1회 — reorder 버퍼에 hold 된 마지막 frame 들을 pts 순으로 배출.
                if (!m_eof.load())
                {
                    while (!m_video_reorder.empty())
                    {
                        auto it_min = m_video_reorder.begin();
                        for (auto it = m_video_reorder.begin() + 1; it != m_video_reorder.end(); ++it)
                            if ((*it)->pts < (*it_min)->pts)
                                it_min = it;
                        AVFrame* emit = *it_min;
                        m_video_reorder.erase(it_min);
                        {
                            std::unique_lock<std::mutex> lk(m_mtx_queue);
                            m_video_queue.push_back(emit);
                        }
                        m_cv_queue.notify_one();
                    }
                }
                m_eof.store(true);
                std::this_thread::sleep_for(std::chrono::milliseconds(30));
                continue;
            }

            //[diag] packet 분포 — seek 후 video/audio 비율 확인. starvation 진단.
            static thread_local int64_t s_pkt_total = 0;
            static thread_local int64_t s_pkt_video = 0;
            static thread_local int64_t s_pkt_audio = 0;
            static thread_local int64_t s_pkt_other = 0;
            ++s_pkt_total;
            if (pkt->stream_index == m_video_stream_idx)        ++s_pkt_video;
            else if (pkt->stream_index == m_audio_stream_idx)   ++s_pkt_audio;
            else                                                 ++s_pkt_other;
            if ((s_pkt_total % 200) == 0)
            {
                size_t vq, aq;
                {
                    std::unique_lock<std::mutex> lk(m_mtx_queue);
                    vq = m_video_queue.size();
                    aq = m_audio_queue.size();
                }
                logWrite(_T("[ffi/dec/diag] pkt total=%lld v=%lld a=%lld other=%lld | queue v=%zu a=%zu"),
                    (long long)s_pkt_total, (long long)s_pkt_video, (long long)s_pkt_audio, (long long)s_pkt_other,
                    vq, aq);
            }

            //=== 4) decode (video or audio) ===
            //push 직전 pending seek 체크 — caller seek() 가 큐 flush 한 후 worker 가 stale frame push 하는 race 차단.
            if (pkt->stream_index == m_video_stream_idx)
            {
                hr = avcodec_send_packet(m_video_ctx, pkt);
                av_packet_unref(pkt);
                if (hr < 0)
                    continue;

                while (true)
                {
                    AVFrame* frame = av_frame_alloc();
                    hr = avcodec_receive_frame(m_video_ctx, frame);
                    if (hr < 0)
                    {
                        av_frame_free(&frame);
                        break;
                    }

                    bool pending;
                    {
                        std::unique_lock<std::mutex> lks(m_mtx_seek);
                        pending = (m_pending_seek_ms >= 0);
                    }
                    if (pending)
                    {
                        av_frame_free(&frame);
                        break;   //다음 iteration 의 seek 처리 단계로.
                    }

                    //HW frame 이면 CPU NV12 로 download. SW frame 이면 그대로.
                    AVFrame* out_frame = frame;
                    if (m_hw_pix_fmt != AV_PIX_FMT_NONE && (AVPixelFormat)frame->format == m_hw_pix_fmt)
                    {
                        //명시적 CPU buffer 할당 — device-managed pool 의 buffer 가 아닌 own buffer.
                        //이 안 하면 transferred sw_frame->data 가 D3D11 device 관리 상태로 남아 swscale 와 device lock race → FillBuffer stuck.
                        //sw_frame->format 을 frame->hw_frames_ctx 의 sw_format 으로 query — codec/미디어 마다 다름 (NV12 / P010 / NV21 등).
                        //hard-coded NV12 가 다른 native sw_format 인 미디어 (mkv 일부 H.264 등) 에서 av_hwframe_transfer_data EINVAL fail 원인.
                        AVPixelFormat sw_fmt = AV_PIX_FMT_NV12;
                        if (frame->hw_frames_ctx)
                        {
                            AVHWFramesContext* hwfc = (AVHWFramesContext*)frame->hw_frames_ctx->data;
                            if (hwfc && hwfc->sw_format != AV_PIX_FMT_NONE)
                                sw_fmt = hwfc->sw_format;
                        }

                        AVFrame* sw_frame = av_frame_alloc();
                        sw_frame->format = sw_fmt;
                        sw_frame->width  = frame->width;
                        sw_frame->height = frame->height;
                        int buf_hr = av_frame_get_buffer(sw_frame, 32);
                        if (buf_hr < 0)
                        {
                            av_frame_free(&sw_frame);
                            av_frame_free(&frame);
                            continue;
                        }

                        LARGE_INTEGER qpc_freq, qpc_t0, qpc_t1;
                        ::QueryPerformanceFrequency(&qpc_freq);
                        ::QueryPerformanceCounter(&qpc_t0);
                        int xfer = av_hwframe_transfer_data(sw_frame, frame, 0);
                        ::QueryPerformanceCounter(&qpc_t1);
                        static thread_local int s_xfer_count = 0;
                        ++s_xfer_count;
                        if (s_xfer_count <= 5 || s_xfer_count % 50 == 0)
                        {
                            long long us = (qpc_t1.QuadPart - qpc_t0.QuadPart) * 1000000LL / qpc_freq.QuadPart;
                            logWrite(_T("[ffi/dec/hw] transfer#%d xfer=%d us=%lld"), s_xfer_count, xfer, us);
                        }
                        if (xfer >= 0)
                        {
                            //pts / flags 보존
                            sw_frame->pts = frame->pts;
                            sw_frame->pkt_dts = frame->pkt_dts;
                            sw_frame->flags = frame->flags;
                            av_frame_free(&frame);
                            out_frame = sw_frame;
                        }
                        else
                        {
                            av_frame_free(&sw_frame);
                            av_frame_free(&frame);
                            continue;   //skip this frame
                        }
                    }

                    out_frame->opaque = (void*)(intptr_t)m_seek_generation.load();   //generation tag

                    //video_first_emit_pts_rt 는 *worker push 시점이 아닌* CFFiVideoStream::FillBuffer 의 *pre-target skip
                    //후 실제 emit 한 첫 frame* 시점에 set 됨 (filter source 에서). worker 의 첫 push frame 은 keyframe
                    //일 수 있어 target 이상 frame 과 미디어 시점 차이 (= GOP 길이).

                    //[reorder] decode 순서로 나온 frame 을 pts 순으로 재정렬해 emit. has_b_frames(디코더 reorder 깊이)
                    //만큼 hold 후 최소 pts 부터 m_video_queue 로. 이미 presentation 순서면 min=front 라 no-op,
                    //D3D11VA HW 경로 등에서 decode 순서로 나오는 경우만 교정 → sample.rtStart 단조 → 렌더러 judder/drift 제거.
                    int64_t cur_pts = out_frame->pts;   //logging 용 — push 후 out_frame 이 emit/free 될 수 있어 미리 보관.
                    m_video_reorder.push_back(out_frame);
                    //[test] reorder 비활성(depth 0) — avcodec 출력(receive) 순서를 그대로 유지.
                    //pts 로 정렬하면 콘텐츠가 뒤죽박죽 되는 가설(b) 검증: 받은 순서가 올바른 presentation 순서.
                    int reorder_depth = 0;
                    static bool s_logged_bf = false;
                    if (!s_logged_bf) { logWrite(_T("[reorder] DISABLED has_b_frames=%d"), m_video_ctx->has_b_frames); s_logged_bf = true; }
                    while ((int)m_video_reorder.size() > reorder_depth)
                    {
                        auto it_min = m_video_reorder.begin();
                        for (auto it = m_video_reorder.begin() + 1; it != m_video_reorder.end(); ++it)
                            if ((*it)->pts < (*it_min)->pts)
                                it_min = it;
                        AVFrame* emit = *it_min;
                        m_video_reorder.erase(it_min);
                        {
                            std::unique_lock<std::mutex> lk(m_mtx_queue);
                            m_video_queue.push_back(emit);
                        }
                        m_cv_queue.notify_one();
                    }

                    //first frame 시간 측정 — seek 후 디코드 시작까지의 wallclock.
                    if (!m_first_frame_after_seek)
                    {
                        LARGE_INTEGER qpc_freq, qpc_now;
                        ::QueryPerformanceFrequency(&qpc_freq);
                        ::QueryPerformanceCounter(&qpc_now);
                        long long us = (qpc_now.QuadPart - m_seek_done_qpc) * 1000000LL / qpc_freq.QuadPart;
                        logWrite(_T("[ffi/dec/seek] seek→first_video_frame us=%lld pts=%lld"),
                            us, (long long)cur_pts);
                        m_first_frame_after_seek = true;
                    }
                }
            }
            else if (m_audio_ctx && pkt->stream_index == m_audio_stream_idx)
            {
                hr = avcodec_send_packet(m_audio_ctx, pkt);
                av_packet_unref(pkt);
                if (hr < 0)
                    continue;

                while (true)
                {
                    AVFrame* frame = av_frame_alloc();
                    hr = avcodec_receive_frame(m_audio_ctx, frame);
                    if (hr < 0)
                    {
                        av_frame_free(&frame);
                        break;
                    }

                    bool pending;
                    {
                        std::unique_lock<std::mutex> lks(m_mtx_seek);
                        pending = (m_pending_seek_ms >= 0);
                    }
                    if (pending)
                    {
                        av_frame_free(&frame);
                        break;
                    }

                    frame->opaque = (void*)(intptr_t)m_seek_generation.load();   //generation tag
                    {
                        std::unique_lock<std::mutex> lk(m_mtx_queue);
                        m_audio_queue.push_back(frame);
                    }
                    m_cv_queue.notify_one();
                }
            }
            else
            {
                av_packet_unref(pkt);
            }
        }

        av_packet_free(&pkt);
    }

    //앞 max_frames 비디오 프레임만 디코드해 frame->pts 단조성(역행 비율)을 측정한다.
    //ffi_source_filter 의 video rtStart clamp(line 494~523) 와 *동일한* 산술을 복제 — 반환 ratio 는 그 [ptscheck] 비율과 일치한다.
    //비단조 pts 미디어는 높고(이 류 0.37~0.42), 정상 미디어는 0.0 → open 시 LAV 라우팅 판별용. 실패 시 false (호출자는 내장 유지).
    bool probe_video_pts_regress_ratio(const wchar_t* utf16_path, double* out_ratio, int max_frames)
    {
        if (out_ratio)
            *out_ratio = 0.0;

        CDecoder probe;
        if (!probe.open(utf16_path))
            return false;

        probe.start();

        double fps = probe.frame_rate();
        if (fps <= 0.0)
            fps = 30.0;
        //REFERENCE_TIME(DirectShow 타입) 대신 int64_t — 동일한 100ns tick 값. ffi_decoder 는 DirectShow 헤더 비의존.
        int64_t frame_dur = (int64_t)(10000000.0 / fps);
        AVRational tb = probe.video_time_base();

        int     frames       = 0;
        int     regress      = 0;
        int64_t last_rtStart = 0;
        int64_t video_first  = LLONG_MIN;

        DWORD t0 = ::GetTickCount();
        while (frames < max_frames)
        {
            AVFrame* f = probe.pop_video_frame();
            if (!f)
            {
                if (probe.is_eof())
                    break;
                if (::GetTickCount() - t0 > 3000)   //probe 가 hang 하지 않도록 상한
                    break;
                ::Sleep(3);
                continue;
            }

            int64_t rtStart;
            if (f->pts != AV_NOPTS_VALUE)
            {
                int64_t frame_pts_rt = av_rescale_q(f->pts, tb, AVRational{1, 10000000});
                if (video_first == LLONG_MIN)
                    video_first = frame_pts_rt;
                rtStart = frame_pts_rt - video_first;
            }
            else
            {
                rtStart = (int64_t)(frames * 10000000.0 / fps);
            }

            if (frames > 0 && rtStart <= last_rtStart)
            {
                rtStart = last_rtStart + frame_dur;
                regress++;
            }
            last_rtStart = rtStart;
            frames++;

            av_frame_free(&f);
        }

        probe.stop();
        probe.close();

        double ratio = (frames > 0) ? (double)regress / frames : 0.0;
        if (out_ratio)
            *out_ratio = ratio;

        logWrite(_T("[ptsprobe] frames=%d regress=%d ratio=%.1f%% — >=15%% 면 비단조 pts(LAV 라우팅)"),
            frames, regress, 100.0 * ratio);
        return frames > 0;
    }
}
