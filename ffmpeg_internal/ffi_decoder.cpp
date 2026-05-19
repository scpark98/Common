#include "ffi_decoder.h"

#include "../Functions.h"
#include "../log/SCLog/SCLog.h"

extern "C" {
#include <libavutil/hwcontext.h>
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

    bool CDecoder::open(const wchar_t* utf16_path)
    {
        if (m_fmt)
            close();
        if (!utf16_path)
            return false;

        //UTF-16 → UTF-8 path
        int u8_len = ::WideCharToMultiByte(CP_UTF8, 0, utf16_path, -1, NULL, 0, NULL, NULL);
        if (u8_len <= 0) return false;
        std::vector<char> u8(u8_len);
        ::WideCharToMultiByte(CP_UTF8, 0, utf16_path, -1, u8.data(), u8_len, NULL, NULL);

        int hr = avformat_open_input(&m_fmt, u8.data(), NULL, NULL);
        if (hr < 0)
        {
            char buf[256];
            logWrite(_T("[ffi/dec] avformat_open_input fail hr=%d (%hs)"), hr, ffi::err_str(hr, buf, sizeof(buf)));
            m_fmt = NULL;
            return false;
        }

        hr = avformat_find_stream_info(m_fmt, NULL);
        if (hr < 0)
        {
            char buf[256];
            logWrite(_T("[ffi/dec] find_stream_info fail hr=%d (%hs)"), hr, ffi::err_str(hr, buf, sizeof(buf)));
            avformat_close_input(&m_fmt);
            return false;
        }

        //video stream 선택 — av_find_best_stream 이 자동으로 가장 적합한 video stream 고름.
        m_video_stream_idx = av_find_best_stream(m_fmt, AVMEDIA_TYPE_VIDEO, -1, -1, NULL, 0);
        if (m_video_stream_idx < 0)
        {
            logWrite(_T("[ffi/dec] no video stream"));
            avformat_close_input(&m_fmt);
            return false;
        }

        //decoder 준비
        AVCodecParameters* par = m_fmt->streams[m_video_stream_idx]->codecpar;
        const AVCodec* codec = avcodec_find_decoder(par->codec_id);
        if (!codec)
        {
            logWrite(_T("[ffi/dec] no decoder for codec_id=%d"), (int)par->codec_id);
            avformat_close_input(&m_fmt);
            return false;
        }

        m_video_ctx = avcodec_alloc_context3(codec);
        if (!m_video_ctx)
        {
            avformat_close_input(&m_fmt);
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
            avcodec_free_context(&m_video_ctx);
            avformat_close_input(&m_fmt);
            return false;
        }

        logWrite(_T("[ffi/dec] opened codec=%hs %dx%d threads=%d"),
            codec->name, m_video_ctx->width, m_video_ctx->height, m_video_ctx->thread_count);

        //[diag] 모든 audio stream enumerate — multi-track 미디어에서 선택 stream 확인용.
        for (unsigned i = 0; i < m_fmt->nb_streams; ++i)
        {
            AVCodecParameters* p = m_fmt->streams[i]->codecpar;
            if (p->codec_type != AVMEDIA_TYPE_AUDIO) continue;
            const AVCodec* c = avcodec_find_decoder(p->codec_id);
            AVDictionaryEntry* lang = av_dict_get(m_fmt->streams[i]->metadata, "language", NULL, 0);
            AVDictionaryEntry* title = av_dict_get(m_fmt->streams[i]->metadata, "title", NULL, 0);
            logWrite(_T("[ffi/dec/diag] audio_stream[%u] codec=%hs %dHz ch=%d disposition=0x%x lang=%hs title=%hs"),
                i, c ? c->name : "?", p->sample_rate, p->ch_layout.nb_channels,
                m_fmt->streams[i]->disposition,
                lang ? lang->value : "(none)",
                title ? title->value : "(none)");
        }

        //audio stream 도 시도 — 없으면 has_audio() false 로 진행.
        m_audio_stream_idx = av_find_best_stream(m_fmt, AVMEDIA_TYPE_AUDIO, -1, -1, NULL, 0);
        logWrite(_T("[ffi/dec/diag] av_find_best_stream(AUDIO) → idx=%d"), m_audio_stream_idx);
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

        m_video_stream_idx = -1;
        m_audio_stream_idx = -1;
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

                    int64_t target = (int64_t)(seek_pos * AV_TIME_BASE / 1000.0);

                    LARGE_INTEGER qpc_freq, qpc_t0, qpc_t1;
                    ::QueryPerformanceFrequency(&qpc_freq);
                    ::QueryPerformanceCounter(&qpc_t0);

                    //stream_index=-1 → AV_TIME_BASE 자동 변환. 명시하면 target 이 stream time_base 단위여야 함.
                    //(stream 명시한 채 AV_TIME_BASE 값 넘기면 미디어 범위 밖으로 seek → fail → 같은 자리 stay = freeze 의 진짜 원인)
                    int hr_seek = av_seek_frame(m_fmt, -1, target, AVSEEK_FLAG_BACKWARD);
                    avcodec_flush_buffers(m_video_ctx);
                    if (m_audio_ctx)
                        avcodec_flush_buffers(m_audio_ctx);

                    ::QueryPerformanceCounter(&qpc_t1);
                    long long seek_us = (qpc_t1.QuadPart - qpc_t0.QuadPart) * 1000000LL / qpc_freq.QuadPart;

                    m_segment_rt.store(m_pending_segment_rt);
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
                //EOF — 잠시 대기 후 다음 iteration (seek 가 올 수도 있음).
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
                        AVFrame* sw_frame = av_frame_alloc();
                        sw_frame->format = AV_PIX_FMT_NV12;
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
                    {
                        std::unique_lock<std::mutex> lk(m_mtx_queue);
                        m_video_queue.push_back(out_frame);
                    }
                    m_cv_queue.notify_one();

                    //first frame 시간 측정 — seek 후 디코드 시작까지의 wallclock.
                    if (!m_first_frame_after_seek)
                    {
                        LARGE_INTEGER qpc_freq, qpc_now;
                        ::QueryPerformanceFrequency(&qpc_freq);
                        ::QueryPerformanceCounter(&qpc_now);
                        long long us = (qpc_now.QuadPart - m_seek_done_qpc) * 1000000LL / qpc_freq.QuadPart;
                        logWrite(_T("[ffi/dec/seek] seek→first_video_frame us=%lld pts=%lld"),
                            us, (long long)out_frame->pts);
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
}
