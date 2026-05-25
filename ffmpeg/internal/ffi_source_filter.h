#pragma once

/*
* ffi::CFFiSource — custom DirectShow source filter (LAV Splitter + LAV Video/Audio Decoder 통합 대체).
*
*  - Phase 3 본 작업. internal FFmpeg (CDecoder) 으로 demux+decode 한 결과를 DirectShow 의 IMediaSample 로 변환해
*    후단 (MPC-VR + DSound + SC Audio chain + DirectVobSub) 로 push.
*  - LAV Splitter 의 SetPositions 동기 블로킹 (25-604ms UI freeze) 회피가 본 목적.
*    이 filter 가 IMediaSeeking::SetPositions 받으면 CDecoder::seek() 에 비동기 위임 → UI 즉시 반환.
*
*  Phase 3a: video pin only. NV12 출력.
*  Phase 3b: audio pin 추가.
*  Phase 3c: dshow.cpp 의 load_media 에 LAV ↔ Internal 토글 분기.
*/

#include "../../directx/dshow_baseclasses/streams.h"
#include "ffi_decoder.h"

#include <memory>

extern "C" {
#include <libavfilter/avfilter.h>
#include <libavfilter/buffersrc.h>
#include <libavfilter/buffersink.h>
}

//{C0D9F19A-1AAA-4B5E-8C3F-7D3B6E4F8A2D}
//자체 filter CLSID. DirectShow 카탈로그에 등록 안 함 (exe 빌드, in-process 직접 instantiate).
static const GUID CLSID_FFiSource =
{ 0xC0D9F19A, 0x1AAA, 0x4B5E, { 0x8C, 0x3F, 0x7D, 0x3B, 0x6E, 0x4F, 0x8A, 0x2D } };

namespace ffi
{
    class CFFiSource;

    class CFFiVideoStream;

    //CSourceSeeking 의 aggregated 구현 — pin 에 composition 으로 부착.
    //ChangeStart/Stop/Rate 가 trigger 되면 back-pointer 로 pin 에 알림.
    class CFFiSeeking : public CSourceSeeking
    {
    public:
        CFFiSeeking(LPUNKNOWN pUnkOuter, HRESULT* phr, CCritSec* pLock, CFFiVideoStream* pPin);

        //CSourceSeeking pure virtual override.
        HRESULT ChangeStart() override;
        HRESULT ChangeStop() override;
        HRESULT ChangeRate() override;

        //GetCurrentPosition override — pin 의 last_rtStart 반환.
        STDMETHODIMP GetCurrentPosition(LONGLONG* pCurrent) override;

    private:
        CFFiVideoStream*    m_pPin;
    };

    class CFFiAudioStream;

    //CSourceStream + IMediaSeeking 노출 (composition).
    //NV12 sample 을 IMediaSample 에 채워 downstream 에 push. graph 의 SetPositions 는 CDecoder::seek 로 async 위임.
    class CFFiVideoStream : public CSourceStream
    {
    public:
        CFFiVideoStream(HRESULT* phr, CFFiSource* pParent, LPCWSTR pPinName);
        ~CFFiVideoStream();

        //IMediaSeeking 노출 — m_pSeeking 으로 라우팅. 그 외는 CSourceStream (CBaseOutputPin → CBasePin).
        STDMETHODIMP NonDelegatingQueryInterface(REFIID riid, void** ppv) override;

        //CSourceStream overrides
        HRESULT FillBuffer(IMediaSample* pSample) override;
        HRESULT GetMediaType(CMediaType* pMediaType) override;
        HRESULT CheckMediaType(const CMediaType* pmt) override;
        HRESULT DecideBufferSize(IMemAllocator* pAlloc, ALLOCATOR_PROPERTIES* pProperties) override;
        HRESULT OnThreadCreate() override;
        HRESULT OnThreadDestroy() override;
        HRESULT OnThreadStartPlay() override;   //MS 표준 정공법 — thread restart 직후 NewSegment + discontinuity.
        HRESULT DoBufferProcessingLoop() override;   //(Z) 측정 — GetDeliveryBuffer / Deliver wait 분리.

        //CFFiSeeking 의 ChangeStart 가 호출 — CDecoder::seek 비동기 위임.
        void                on_change_start(REFERENCE_TIME rtStart);

        REFERENCE_TIME      last_rtStart() const { return m_last_rtStart; }
        CFFiSource*         source() { return m_pSource; }

    private:
        CFFiSource*     m_pSource;
        LONGLONG        m_sample_count = 0;   //sample 순번. timestamping 에 사용.
        LONGLONG        m_last_rtStart = 0;   //직전 emit 한 frame 의 rtStart. GetCurrentPosition 의 응답값.
    public:
        REFERENCE_TIME  m_segment_start = 0;  //seek target rt. video/audio sync 의 공유 baseline.

        //(A) FillBuffer entry 측정 — seek 후 첫 호출 latency / 비정상 gap.
        ULONGLONG       m_seek_t0_ms = 0;
        ULONGLONG       m_fb_last_entry_ms = 0;
        int             m_fb_count_since_seek = 0;

        //flush 후 첫 sample 에 SetDiscontinuity(TRUE) 알림 플래그.
        bool            m_need_discontinuity = false;

        //MS 표준 정공법 — on_change_start 가 저장 → OnThreadStartPlay 가 NewSegment 에 사용.
        REFERENCE_TIME  m_pending_segment_stop = 0;
    private:

        CCritSec        m_cs_seeking;
        CFFiSeeking*    m_pSeeking = nullptr;   //aggregated seeking object — pin 의 ref count 와 공유.
    };

    //Audio pin — PCM S16 출력. libswresample 로 decoder 출력 (FLTP 등) 을 S16 으로 변환.
    class CFFiAudioStream : public CSourceStream
    {
    public:
        CFFiAudioStream(HRESULT* phr, CFFiSource* pParent, LPCWSTR pPinName);
        ~CFFiAudioStream();

        HRESULT FillBuffer(IMediaSample* pSample) override;
        HRESULT GetMediaType(CMediaType* pMediaType) override;
        HRESULT CheckMediaType(const CMediaType* pmt) override;
        HRESULT DecideBufferSize(IMemAllocator* pAlloc, ALLOCATOR_PROPERTIES* pProperties) override;
        HRESULT OnThreadCreate() override;
        HRESULT OnThreadDestroy() override;

        //video pin 의 on_change_start 에서 호출 — flush + new_segment.
        void    on_seek_flush(REFERENCE_TIME rtStart);

        //atempo audio filter graph — rate != 1.0 시 PCM time-stretch (pitch 유지).
        //swr_convert 의 S16 stereo output 을 source 로 받아 atempo filter 통과 → sink 에서 sample count 1/rate 줄어든 PCM 출력.
        //graph 의 audio renderer (DSound) 가 줄어든 sample 양만큼 빨리 처리 → graph clock 가속 → video 도 따라 가속.
        //rate runtime 변경은 avfilter_graph_send_command 로 atempo tempo 갱신 (graph 재생성 불필요).
        bool                init_audio_filter(double rate);
        void                release_audio_filter();
        bool                update_audio_filter_rate(double rate);

    private:
        CFFiSource*     m_pSource;
        LONGLONG        m_sample_count = 0;
        SwrContext*     m_swr = nullptr;
        int             m_out_sample_rate = 0;
        int             m_out_channels = 0;
        AVChannelLayout m_out_chlayout{};
        bool            m_need_discontinuity = false;
        REFERENCE_TIME  m_pending_segment_stop = 0;

        AVFilterGraph*   m_filter_graph = nullptr;
        AVFilterContext* m_filter_src   = nullptr;   //abuffer
        AVFilterContext* m_filter_atempo = nullptr;
        AVFilterContext* m_filter_sink  = nullptr;   //abuffersink
        double           m_filter_rate  = 1.0;       //현재 atempo 에 적용된 rate. source 의 playback_rate 와 비교해 달라지면 send_command.

        //audio sync offset — audio first emit 의 미디어 시점이 video first emit 보다 *delta* 만큼 후 인 경우
        //audio sample.rtStart 에 delta 만큼 더해서 audio 표시 시점을 delta 늦춤. 같은 graph_clock 시점에 video / audio
        //의 *미디어 시점이 정렬* → sync 정확.
        REFERENCE_TIME  m_audio_offset_rt = 0;
        bool            m_audio_offset_set = false;

        //A/V drift 보정 — 오디오 하드웨어 클럭이 시스템(=비디오 real-time) 보다 미세하게 빨라(예: +0.115%)
        //시간이 갈수록 오디오가 앞서가는 문제. emit content vs 시스템 실시간(QPC)을 측정해 swr_set_compensation
        //으로 오디오를 미세 resample(늘림/줄임)해 real-time 에 lock. prebuffer 영향 배제 위해 3초 후 reference latch.
        bool            m_av_started = false;
        bool            m_av_ref_set = false;
        bool            m_av_locked = false;
        double          m_av_corr = 0.0;      //측정된 drift 보정율 (예: +0.00115 = 오디오 0.115% 느리게).
        LARGE_INTEGER   m_av_qpc_freq{};
        LARGE_INTEGER   m_av_qpc_start{};
        LARGE_INTEGER   m_av_ref_qpc{};
        int64_t         m_av_ref_samples = 0;
        int64_t         m_av_ref_clock = 0;            //기준점의 graph reference clock(=DSound 재생) 시각(100ns).
        int64_t         m_av_last_comp_samples = 0;    //마지막 swr_set_compensation 적용 시점의 m_sample_count.
        double          m_av_D_ema = -1.0;             //device 재생 rate 의 EMA (spike 제거 + 평활). -1 = 미초기화.
    public:
        HRESULT OnThreadStartPlay() override;
    };

    //CSource — base filter. Pin (CFFiVideoStream) 을 보유. open_file 로 CDecoder 준비.
    class CFFiSource : public CSource
    {
    public:
        //pUnk = outer IUnknown (aggregation). 일반적으로 NULL.
        CFFiSource(LPUNKNOWN pUnk, HRESULT* phr);
        ~CFFiSource();

        //CSource override (위) 의 ctor 가 CLSID 를 요구하므로 별도 정적 메서드 불필요.
        //pUnk = NULL 로 호출자가 직접 `new CFFiSource(NULL, &hr)` 식으로 instantiate.

        //File open — graph add 전 또는 직후에 호출. CDecoder 초기화 + 비디오 stream open.
        HRESULT open_file(const wchar_t* utf16_path);

        //CDecoder 직접 접근 — VideoStream 의 FillBuffer 가 frame 꺼낼 때 사용.
        CDecoder& decoder() { return m_decoder; }

    public:
        CFFiVideoStream*  video_stream() const { return m_pVideoStream; }
        CFFiAudioStream*  audio_stream() const { return m_pAudioStream; }

        //audio sync delay — video first emit pts_rt anchor 에 적용할 offset (REFERENCE_TIME, 100ns).
        //CSCAudioGain 의 sample.rtStart shift 방식은 audio.rtStart 가 segment-local 0 부터인 internal path 에서
        //negative 결과 → DSound 즉시 표시 → delay 무효. 대신 audio anchor 자체를 ±delay 만큼 shift 하여
        //*audio first emit 의 미디어 시점* 을 video 와 분리 → 정상 sync.
        void           set_audio_sync_delay_ms(int ms) { m_audio_sync_delay_rt.store((int64_t)ms * 10000LL); }
        int64_t        audio_sync_delay_rt() const { return m_audio_sync_delay_rt.load(); }

        //CFFiSeeking::ChangeRate 가 m_dRate 를 여기로 propagate. video/audio FillBuffer 가 sample 의 rtStart/rtStop 을
        //1/rate 로 scale → renderer 가 graph clock 같은 시간에 rate 배의 미디어 시간 sample 표시 → 빠르게/느리게 재생.
        //audio 는 sample data 양은 그대로 + duration 만 scale → renderer 가 시간 맞춰 빠르게 재생 (chipmunk 효과, LAV path 동등).
        void           set_playback_rate(double r) { m_playback_rate.store(r); }
        double         playback_rate() const { return m_playback_rate.load(); }

    private:
        CDecoder         m_decoder;
        CFFiVideoStream* m_pVideoStream = nullptr;
        CFFiAudioStream* m_pAudioStream = nullptr;   //has_audio 일 때만 생성.
        std::atomic<int64_t> m_audio_sync_delay_rt{0};
        std::atomic<double>  m_playback_rate{1.0};
    };
}
