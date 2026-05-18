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
        void seek(double pos_ms);

        //worker 가 seek 를 *처리 완료* 할 때까지 동기 대기 (queue drain + av_seek_frame + codec flush 끝남).
        //그 후 pop_video_frame 으로 받는 frame 은 모두 post-seek. timeout_ms 안에 처리되면 true.
        //정확한 seek latency 측정 / "seek 후 첫 frame" 확보용.
        bool wait_seek_done(int timeout_ms);

        //queue 에서 frame 1개 pop. 비면 nullptr. caller 가 av_frame_free 책임.
        AVFrame* pop_video_frame();

        //info
        int     video_width()  const;
        int     video_height() const;
        double  duration_ms()  const;
        double  frame_rate()   const;        //avg_frame_rate (fps). 0 이면 unknown.
        AVRational video_time_base() const;  //stream 의 time_base. pts→ms 변환에 사용.
        int     video_pixel_format() const;  //AVPixelFormat. AVFrame 의 format 과 동일 또는 codec 의 hw_pix_fmt.
        bool    is_opened()    const { return m_fmt != nullptr; }
        bool    is_running()   const { return m_thread.joinable(); }

        //queue 의 현재 size — 디버깅용.
        size_t  video_queue_size();

        //queue 최대 깊이 (default 5, PotPlayer 와 동일).
        void    set_max_queue(int n) { m_max_queue = n; }

    private:
        void    worker_loop();

        AVFormatContext*    m_fmt = nullptr;
        AVCodecContext*     m_video_ctx = nullptr;
        int                 m_video_stream_idx = -1;

        std::thread             m_thread;
        std::atomic<bool>       m_quit{false};

        //seek 요청 — worker 가 다음 iteration 에서 pickup.
        std::mutex              m_mtx_seek;
        std::condition_variable m_cv_seek_done;
        double                  m_pending_seek_ms = -1.0;
        bool                    m_seek_processed = true;   //worker 가 seek 끝낸 직후 true. seek() 호출 시 false.

        //frame queue
        std::mutex              m_mtx_queue;
        std::condition_variable m_cv_queue;     //worker 가 queue 비울 때 ↔ UI 가 pop 할 때 양방향 wake.
        std::deque<AVFrame*>    m_video_queue;
        int                     m_max_queue = 5;
    };
}
