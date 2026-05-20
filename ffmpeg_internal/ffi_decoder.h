#pragma once

/*
* ffi::CDecoder — async video decode engine.
*
*  Phase 2 산출물. open → start → seek/pop_frame → stop → close 의 lifecycle.
*  worker thread 가 demux + decode 를 background 에서 수행하고 frame queue (default 5) 에 push.
*  UI thread 는 pop_video_frame() 으로 디코드된 frame 을 받음 (queue 비면 nullptr).
*  seek 는 worker thread 에 비동기 위임 — UI thread 는 SetPositions 같은 동기 blocking 없음.
*
*  Phase 3 부터 audio + renderer 통합 예정.
*/

#include "ffmpeg_internal.h"

#include <atomic>
#include <condition_variable>
#include <deque>
#include <mutex>
#include <thread>

namespace ffi
{
    class CDecoder
    {
    public:
        CDecoder();
        ~CDecoder();

        //file open + 비디오 stream 의 decoder context 준비. worker thread 는 아직 시작 안 됨.
        bool open(const wchar_t* utf16_path);

        //worker thread 종료 + format/codec context 해제.
        void close();

        //worker thread 시작. decoding 시작.
        void start();

        //worker thread 종료 (queue 의 frame 들 release).
        void stop();

        //seek 요청 — worker thread 에 비동기 위임. UI thread 는 즉시 반환.
        //worker 가 queue flush + av_seek_frame + codec flush 후 새 위치부터 디코드 재개.
        //segment_rt = graph 가 알린 seek target rt. worker 가 av_seek_frame 후 generation ++ 와 동시에 m_segment_rt 갱신.
        void seek(double pos_ms, int64_t segment_rt = 0);

        //worker 가 seek 를 *처리 완료* 할 때까지 동기 대기 (queue drain + av_seek_frame + codec flush 끝남).
        //그 후 pop_video_frame 으로 받는 frame 은 모두 post-seek. timeout_ms 안에 처리되면 true.
        //정확한 seek latency 측정 / "seek 후 첫 frame" 확보용.
        bool wait_seek_done(int timeout_ms);

        //queue 에서 frame 1개 pop. 비면 nullptr. caller 가 av_frame_free 책임.
        AVFrame* pop_video_frame();
        AVFrame* pop_audio_frame();

        //info
        int     video_width()  const;
        int     video_height() const;
        double  duration_ms()  const;
        double  frame_rate()   const;        //avg_frame_rate (fps). 0 이면 unknown.
        AVRational video_time_base() const;  //stream 의 time_base. pts→ms 변환에 사용.
        int     video_pixel_format() const;  //AVPixelFormat. AVFrame 의 format 과 동일 또는 codec 의 hw_pix_fmt.

        //audio info — Phase 4. has_audio() false 면 audio stream 없음 / 디코더 fail.
        bool    has_audio() const { return m_audio_stream_idx >= 0 && m_audio_ctx != nullptr; }
        int     audio_sample_rate() const;       //Hz
        int     audio_channels()    const;
        int     audio_sample_format() const;     //AVSampleFormat enum 값
        AVRational audio_time_base() const;

        bool    is_opened()    const { return m_fmt != nullptr; }
        bool    is_running()   const { return m_thread.joinable(); }
        bool    has_hw_accel() const { return m_hw_pix_fmt != AV_PIX_FMT_NONE; }
        bool    is_eof()       const { return m_eof.load(); }   //av_read_frame AVERROR_EOF 도달 + queue 비면 stream 끝.

        //queue 의 현재 size — 디버깅용.
        size_t  video_queue_size();
        size_t  audio_queue_size();

        //queue 최대 깊이 (default video 5 / audio 50, PotPlayer 와 비슷).
        void    set_max_video_queue(int n) { m_max_queue = n; }
        void    set_max_audio_queue(int n) { m_max_audio_queue = n; }

        //CFFi*Stream 의 FillBuffer 가 매 frame pop 후 호출. atomic load — race-free segment baseline.
        int64_t segment_rt() const { return m_segment_rt.load(); }

    private:
        void    worker_loop();

        AVFormatContext*    m_fmt = nullptr;
        AVCodecContext*     m_video_ctx = nullptr;
        int                 m_video_stream_idx = -1;

        //HW 가속 — D3D11VA / DXVA2. SW keyframe walk decode 의 5-10초 freeze 회피.
        AVBufferRef*        m_hw_device_ctx = nullptr;
        AVPixelFormat       m_hw_pix_fmt = AV_PIX_FMT_NONE;   //HW frame format (codec 의 get_format 반환).

        //audio decode
        AVCodecContext*     m_audio_ctx = nullptr;
        int                 m_audio_stream_idx = -1;

        std::thread             m_thread;
        std::atomic<bool>       m_quit{false};

        //seek 요청 — worker 가 다음 iteration 에서 pickup.
        std::mutex              m_mtx_seek;
        std::condition_variable m_cv_seek_done;
        double                  m_pending_seek_ms = -1.0;
        bool                    m_seek_processed = true;   //worker 가 seek 끝낸 직후 true. seek() 호출 시 false.

        //Seek generation — frame 마다 tag 부여. seek() 호출 시 ++. pop 시 current generation 만 accept,
        //이전 generation 의 stale frame 은 skip + free. caller flush 와 worker push 의 모든 race 차단.
        std::atomic<int>        m_seek_generation{0};

        //EOF — av_read_frame AVERROR_EOF 도달 시 set, seek 시 clear. FillBuffer 가 EOS 인지 vs 일시 starve 구분.
        std::atomic<bool>       m_eof{false};

        //Segment baseline rt — worker 의 av_seek_frame 처리 후 generation ++ 와 동시에 갱신.
        //FillBuffer 의 rt 계산이 generation tag 와 같은 시점의 segment 사용 → race 없음.
        //CFFiVideoStream / CFFiAudioStream 이 직접 m_segment_rt.load() 접근 가능하도록 public-like accessor 별도 제공.
        std::atomic<int64_t>   m_segment_rt{0};
        int64_t                m_pending_segment_rt = 0;   //seek() 호출 시 저장. worker 가 av_seek_frame 후 m_segment_rt.store(this).

        //seek → first frame 시간 측정용.
        bool                   m_first_frame_after_seek = true;
        int64_t                m_seek_done_qpc = 0;

        //frame queue
        std::mutex              m_mtx_queue;
        std::condition_variable m_cv_queue;     //worker 가 queue 비울 때 ↔ UI 가 pop 할 때 양방향 wake.
        std::deque<AVFrame*>    m_video_queue;
        std::deque<AVFrame*>    m_audio_queue;
        int                     m_max_queue = 30;       //seek 후 큐 빨리 채워지도록 5→30. FillBuffer wait 시간 단축.
        int                     m_max_audio_queue = 100;
    };
}
