#include "ffi_decoder.h"

#include "../Functions.h"
#include "../log/SCLog/SCLog.h"

#include <vector>

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

        //multi-threaded decode — FFmpeg 의 thread_type 자동 (frame + slice). PotPlayer 의 "Thread Frame" 과 동일.
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

        return true;
    }

    void CDecoder::close()
    {
        stop();

        if (m_video_ctx)
            avcodec_free_context(&m_video_ctx);

        if (m_fmt)
            avformat_close_input(&m_fmt);

        m_video_stream_idx = -1;
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
        logWrite(_T("[ffi/dec] worker stopped"));
    }

    void CDecoder::seek(double pos_ms)
    {
        {
            std::unique_lock<std::mutex> lk(m_mtx_seek);
            m_pending_seek_ms = pos_ms;
            m_seek_processed = false;   //worker 가 처리하면 true 로.
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
        std::unique_lock<std::mutex> lk(m_mtx_queue);
        if (m_video_queue.empty())
            return nullptr;
        AVFrame* f = m_video_queue.front();
        m_video_queue.pop_front();
        m_cv_queue.notify_one();   //queue 비어졌으니 worker 가 wait 중이면 깨움.
        return f;
    }

    size_t CDecoder::video_queue_size()
    {
        std::unique_lock<std::mutex> lk(m_mtx_queue);
        return m_video_queue.size();
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

                    //queue flush — 이전 위치 frame 들 모두 버림.
                    {
                        std::unique_lock<std::mutex> lkq(m_mtx_queue);
                        while (!m_video_queue.empty())
                        {
                            AVFrame* f = m_video_queue.front();
                            m_video_queue.pop_front();
                            av_frame_free(&f);
                        }
                    }

                    int64_t target = (int64_t)(seek_pos * AV_TIME_BASE / 1000.0);
                    int hr_seek = av_seek_frame(m_fmt, -1, target, AVSEEK_FLAG_BACKWARD);
                    avcodec_flush_buffers(m_video_ctx);
                    logWrite(_T("[ffi/dec] seek to %.0fms hr=%d"), seek_pos, hr_seek);

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
            {
                std::unique_lock<std::mutex> lk(m_mtx_queue);
                m_cv_queue.wait(lk, [this]() {
                    return m_quit.load() ||
                           (int)m_video_queue.size() < m_max_queue ||
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

            if (pkt->stream_index != m_video_stream_idx)
            {
                av_packet_unref(pkt);
                continue;
            }

            //=== 4) decode ===
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

                //queue push
                {
                    std::unique_lock<std::mutex> lk(m_mtx_queue);
                    m_video_queue.push_back(frame);
                }
                m_cv_queue.notify_one();
            }
        }

        av_packet_free(&pkt);
    }
}
