//Include 순서 critical:
//  1) winsock2.h 가 afxwin.h 보다 먼저 — MFC 가 Winsock2 detection 강제.
//  2) afxwin.h 가 streams.h 보다 먼저 — BaseClasses wxlist.h 가 __AFX_H__ 매크로로 __POSITION 정의 가드 (MFC 와 충돌 회피).
#include <winsock2.h>
#include <afxwin.h>

#include "ffi_source_filter.h"

#include "../../Functions.h"
#include "../../log/SCLog/SCLog.h"

extern "C" {
#include <libswscale/swscale.h>
#include <libswresample/swresample.h>
#include <libavutil/imgutils.h>
#include <libavutil/opt.h>
#include <libavutil/channel_layout.h>
}

#include <vector>

namespace ffi
{
    //============================================================
    // CFFiVideoStream
    //============================================================

    //============================================================
    // CFFiSeeking — aggregated CSourceSeeking
    //============================================================

    CFFiSeeking::CFFiSeeking(LPUNKNOWN pUnkOuter, HRESULT* phr, CCritSec* pLock, CFFiVideoStream* pPin)
        : CSourceSeeking(NAME("CFFiSeeking"), pUnkOuter, phr, pLock)
        , m_pPin(pPin)
    {
        //duration / caps 초기 설정.
        if (pPin && pPin->source() && pPin->source()->decoder().is_opened())
        {
            m_rtDuration = (REFERENCE_TIME)(pPin->source()->decoder().duration_ms() * 10000.0);
            m_rtStop = m_rtDuration;
        }
        m_dwSeekingCaps = AM_SEEKING_CanSeekAbsolute |
                          AM_SEEKING_CanSeekForwards |
                          AM_SEEKING_CanSeekBackwards |
                          AM_SEEKING_CanGetCurrentPos |
                          AM_SEEKING_CanGetStopPos |
                          AM_SEEKING_CanGetDuration;
    }

    HRESULT CFFiSeeking::ChangeStart()
    {
        //graph 의 IMediaSeeking::SetPositions 호출 시 CSourceSeeking::SetPositions 가 m_rtStart 갱신 후 본 함수 호출.
        if (m_pPin)
            m_pPin->on_change_start(m_rtStart);
        return S_OK;
    }

    HRESULT CFFiSeeking::ChangeStop()  { return S_OK; }

    HRESULT CFFiSeeking::ChangeRate()
    {
        //CSourceSeeking::SetRate 가 m_dRateSeeking 갱신 후 본 함수 호출. source 의 atomic rate 로 전파 →
        //video/audio FillBuffer 가 sample.rtStart/rtStop scaling 에 사용.
        logWrite(_T("[ffi/seek/diag] ChangeRate called, m_dRateSeeking=%.3f pPin=%p source=%p"),
            m_dRateSeeking, m_pPin, m_pPin ? m_pPin->source() : NULL);
        if (m_pPin && m_pPin->source())
            m_pPin->source()->set_playback_rate(m_dRateSeeking);
        return S_OK;
    }

    STDMETHODIMP CFFiSeeking::GetCurrentPosition(LONGLONG* pCurrent)
    {
        if (!pCurrent)
            return E_POINTER;
        //미디어 timeline 의 현재 표시 위치 = seek 시 미디어 timeline 위치(m_segment_start) + segment-local 누적(last_rtStart).
        //last_rtStart 자체는 sample.rtStart = m_sample_count * frame_duration (segment-local 0 부터).
        //graph 의 IMediaPosition::put_CurrentPosition(GetCurrentPosition + delta) frame step path 가
        //*미디어 timeline* 으로 SetPositions → on_change_start 받음. 만약 segment-local 만 반환하면
        //put_CurrentPosition(0 + interval) → 미디어 0초로 점프하는 버그.
        if (m_pPin)
            *pCurrent = m_pPin->m_segment_start + m_pPin->last_rtStart();
        else
            *pCurrent = 0;
        return S_OK;
    }

    //============================================================
    // CFFiVideoStream
    //============================================================

    CFFiVideoStream::CFFiVideoStream(HRESULT* phr, CFFiSource* pParent, LPCWSTR pPinName)
        : CSourceStream(NAME("CFFiVideoStream"), phr, pParent, pPinName)
        , m_pSource(pParent)
    {
        //aggregated seeking 객체 — pin 의 IUnknown (GetOwner) 을 outer 로 넘김 → AddRef/Release 가 pin 으로 위임.
        m_pSeeking = new CFFiSeeking(GetOwner(), phr, &m_cs_seeking, this);
    }

    CFFiVideoStream::~CFFiVideoStream()
    {
        if (m_pSeeking)
        {
            delete m_pSeeking;
            m_pSeeking = nullptr;
        }
    }

    STDMETHODIMP CFFiVideoStream::NonDelegatingQueryInterface(REFIID riid, void** ppv)
    {
        if (riid == IID_IMediaSeeking && m_pSeeking)
            return m_pSeeking->NonDelegatingQueryInterface(riid, ppv);
        return CSourceStream::NonDelegatingQueryInterface(riid, ppv);
    }

    void CFFiVideoStream::on_change_start(REFERENCE_TIME rtStart)
    {
        if (!m_pSource)
            return;

        double pos_ms = (double)rtStart / 10000.0;
        REFERENCE_TIME rtStop = (REFERENCE_TIME)(m_pSource->decoder().duration_ms() * 10000.0);

        //state 갱신은 ThreadExists 무관 항상. play(Running) 첫 진입 시 OnThreadStartPlay 가 이 값을 사용.
        m_pSource->decoder().seek(pos_ms, rtStart);
        m_sample_count = 0;   //sample.rtStart = m_sample_count * frame_duration 의 출발점.
        m_last_rtStart = 0;
        m_segment_start = rtStart;
        m_pending_segment_stop = rtStop;

        //(A) measurement reset.
        m_seek_t0_ms = GetTickCount64();
        m_fb_count_since_seek = 0;
        m_fb_last_entry_ms = 0;

        //streaming 중인 경우만 MS 표준 정공법 (Stop/Flush/Pause cycle) 적용.
        //  BeginFlush → Stop → EndFlush → Pause → OnThreadStartPlay → NewSegment + Discontinuity
        //pre-streaming (graph Stopped, ThreadExists false) — play(Running) 의 thread create → OnThreadStartPlay 가 동일 NewSegment 처리.
        if (ThreadExists())
        {
            DeliverBeginFlush();
            Stop();
            DeliverEndFlush();
            //decoder.seek 가 worker 에 비동기 위임. Pause→OnThreadStartPlay 시점까지 seek 처리될 가능성 높음.
            //FillBuffer 의 pre-target skip 이 stale frame (frame.pts < seek_target_pts) drop 으로 race 보호.
            Pause();
        }

        //audio pin 도 동일한 정공법 적용 (별도 streaming thread).
        if (m_pSource->audio_stream())
            m_pSource->audio_stream()->on_seek_flush(rtStart);

        logWrite(_T("[ffi/src] on_change_start → seek %.0fms + %s (NewSegment 0..%lld)"),
            pos_ms,
            ThreadExists() ? _T("Stop/Flush/Pause cycle") : _T("pre-streaming defer"),
            (long long)rtStop);
    }

    HRESULT CFFiVideoStream::OnThreadStartPlay()
    {
        //thread restart 직후 — segment[0, duration] + discontinuity. MS 표준 정공.
        //sample.rtStart 는 segment-local time (0 부터 누적). renderer 가 segment.start + sample.rtStart 로 absolute time 계산.
        m_need_discontinuity = true;
        return DeliverNewSegment(0, m_pending_segment_stop, 1.0);
    }

    HRESULT CFFiVideoStream::OnThreadCreate()
    {
        m_sample_count = 0;
        //CDecoder 의 worker thread 가 아직 안 돌고 있으면 시작.
        if (m_pSource && !m_pSource->decoder().is_running())
            m_pSource->decoder().start();
        //m_pending_segment_stop 이 미초기화 (history seek 없이 바로 play 한 케이스) 일 때 duration 으로 fallback.
        //OnThreadStartPlay 가 이 값으로 NewSegment(0, stop, 1.0) 호출 — stop=0 이면 renderer 가 sample drop.
        if (m_pSource && m_pending_segment_stop <= 0)
            m_pending_segment_stop = (REFERENCE_TIME)(m_pSource->decoder().duration_ms() * 10000.0);
        logWrite(_T("[ffi/src] video stream thread create (segment_stop=%lld)"),
            (long long)m_pending_segment_stop);
        return NOERROR;
    }

    HRESULT CFFiVideoStream::OnThreadDestroy()
    {
        logWrite(_T("[ffi/src] video stream thread destroy"));
        return NOERROR;
    }

    HRESULT CFFiVideoStream::GetMediaType(CMediaType* pMediaType)
    {
        if (!pMediaType || !m_pSource)
            return E_POINTER;

        CDecoder& dec = m_pSource->decoder();
        if (!dec.is_opened())
            return E_FAIL;

        int w = dec.video_width();
        int h = dec.video_height();
        int aligned_w = (w + 127) & ~127;   //128 byte alignment — DecideBufferSize / FillBuffer 와 일치.
        double fps = dec.frame_rate();
        if (fps <= 0.0)
            fps = 30.0;

        //NV12 media type 구성. MPC-VR / EVR / VMR9 모두 NV12 받음.
        VIDEOINFOHEADER* pvi = (VIDEOINFOHEADER*)pMediaType->AllocFormatBuffer(sizeof(VIDEOINFOHEADER));
        if (!pvi)
            return E_OUTOFMEMORY;
        ZeroMemory(pvi, sizeof(VIDEOINFOHEADER));

        pvi->bmiHeader.biSize        = sizeof(BITMAPINFOHEADER);
        pvi->bmiHeader.biWidth       = aligned_w;   //stride. rcSource 가 valid 영역.
        pvi->bmiHeader.biHeight      = h;
        pvi->bmiHeader.biPlanes      = 1;
        pvi->bmiHeader.biBitCount    = 12;          //NV12 = 12 bpp
        pvi->bmiHeader.biCompression = MAKEFOURCC('N','V','1','2');
        pvi->bmiHeader.biSizeImage   = aligned_w * h * 3 / 2;

        pvi->AvgTimePerFrame = (REFERENCE_TIME)(10000000.0 / fps);   //100ns 단위
        pvi->rcSource.right  = w;   //valid 영역만 (padded stride 가 아닌 실제 표시 영역).
        pvi->rcSource.bottom = h;
        pvi->rcTarget        = pvi->rcSource;
        pvi->dwBitRate       = aligned_w * h * 3 / 2 * 8 * (DWORD)fps;
        pvi->dwBitErrorRate  = 0;

        const GUID MEDIASUBTYPE_NV12_LOCAL = { MAKEFOURCC('N','V','1','2'),
            0x0000, 0x0010, { 0x80, 0x00, 0x00, 0xAA, 0x00, 0x38, 0x9B, 0x71 } };

        pMediaType->SetType(&MEDIATYPE_Video);
        pMediaType->SetFormatType(&FORMAT_VideoInfo);
        pMediaType->SetTemporalCompression(FALSE);
        pMediaType->SetSubtype(&MEDIASUBTYPE_NV12_LOCAL);
        pMediaType->SetSampleSize(pvi->bmiHeader.biSizeImage);

        return NOERROR;
    }

    HRESULT CFFiVideoStream::CheckMediaType(const CMediaType* pmt)
    {
        if (!pmt)
            return E_POINTER;

        if (*pmt->Type()        != MEDIATYPE_Video) return E_INVALIDARG;
        if (*pmt->FormatType()  != FORMAT_VideoInfo) return E_INVALIDARG;

        //NV12 만 허용.
        const GUID MEDIASUBTYPE_NV12_LOCAL = { MAKEFOURCC('N','V','1','2'),
            0x0000, 0x0010, { 0x80, 0x00, 0x00, 0xAA, 0x00, 0x38, 0x9B, 0x71 } };
        if (*pmt->Subtype() != MEDIASUBTYPE_NV12_LOCAL) return E_INVALIDARG;

        return S_OK;
    }

    HRESULT CFFiVideoStream::DecideBufferSize(IMemAllocator* pAlloc, ALLOCATOR_PROPERTIES* pProperties)
    {
        if (!pAlloc || !pProperties || !m_pSource)
            return E_POINTER;

        CDecoder& dec = m_pSource->decoder();
        int w = dec.video_width();
        int h = dec.video_height();

        //NV12 stride 를 128 byte align — MPCVR 의 D3D11 texture upload 가 aligned stride 가정 시 480 같은 unaligned width 에서 garbage 합성 가능.
        int aligned_w = (w + 127) & ~127;
        pProperties->cBuffers = 3;
        pProperties->cbBuffer = aligned_w * h * 3 / 2;

        ALLOCATOR_PROPERTIES Actual;
        HRESULT hr = pAlloc->SetProperties(pProperties, &Actual);
        if (FAILED(hr))
            return hr;

        if (Actual.cbBuffer < pProperties->cbBuffer)
            return E_FAIL;

        return NOERROR;
    }

    //(Z) DoBufferProcessingLoop override — default impl 그대로 + GetDeliveryBuffer / Deliver wait 측정.
    //정확한 block 위치 추적: flush 후 첫 iteration 에서 어느 단계가 stuck 인지.
    HRESULT CFFiVideoStream::DoBufferProcessingLoop()
    {
        Command com;
        OnThreadStartPlay();
        do
        {
            while (!CheckRequest(&com))
            {
                IMediaSample* pSample;

                ULONGLONG t0 = GetTickCount64();
                HRESULT hr = GetDeliveryBuffer(&pSample, NULL, NULL, 0);
                ULONGLONG t_gdb_ms = GetTickCount64() - t0;
                if (t_gdb_ms > 100)
                    logWrite(_T("[ffi/src/video/diag] GetDeliveryBuffer wait %llums"), t_gdb_ms);

                if (FAILED(hr))
                {
                    Sleep(1);
                    continue;
                }

                hr = FillBuffer(pSample);

                if (hr == S_OK)
                {
                    ULONGLONG t1 = GetTickCount64();
                    hr = Deliver(pSample);
                    ULONGLONG t_dlv_ms = GetTickCount64() - t1;
                    if (t_dlv_ms > 100)
                        logWrite(_T("[ffi/src/video/diag] Deliver wait %llums"), t_dlv_ms);

                    pSample->Release();
                    if (hr != S_OK)
                        return S_OK;
                }
                else if (hr == S_FALSE)
                {
                    pSample->Release();
                    DeliverEndOfStream();
                    return S_OK;
                }
                else
                {
                    pSample->Release();
                    DeliverEndOfStream();
                    m_pFilter->NotifyEvent(EC_ERRORABORT, hr, 0);
                    return hr;
                }
            }

            if (com == CMD_RUN || com == CMD_PAUSE)
                Reply(NOERROR);
            else if (com != CMD_STOP)
                Reply((DWORD)E_UNEXPECTED);
        } while (com != CMD_STOP);

        return S_FALSE;
    }

    HRESULT CFFiVideoStream::FillBuffer(IMediaSample* pSample)
    {
        if (!pSample || !m_pSource)
            return E_POINTER;

        //(A) entry timing — seek 후 첫 호출 latency / 비정상 gap (>100ms) 측정.
        ULONGLONG now_ms = GetTickCount64();
        if (m_fb_count_since_seek == 0)
        {
            logWrite(_T("[ffi/src/video/diag] FillBuffer first entry +%llums after seek"),
                m_seek_t0_ms ? (now_ms - m_seek_t0_ms) : 0ULL);
        }
        else if (m_fb_last_entry_ms != 0)
        {
            ULONGLONG gap_ms = now_ms - m_fb_last_entry_ms;
            if (gap_ms > 100)
                logWrite(_T("[ffi/src/video/diag] FillBuffer entry gap %llums (call #%d since seek)"),
                    gap_ms, m_fb_count_since_seek);
        }
        m_fb_last_entry_ms = now_ms;
        ++m_fb_count_since_seek;

        CDecoder& dec = m_pSource->decoder();

        //AVFrame pop — decoder 가 running 인 동안 wait. 빠른 drag 시 timeout S_FALSE 반환 시 stream 종료 → 영구 freeze 회피.
        //seg_rt 재read — 매 pop iteration 마다 m_segment_rt 다시 읽기.
        //
        //pre-target skip 복원 — frame.pts < seek_target_pts 인 frame 은 skip (가장 가까운 target *직후* frame 부터 emit).
        //이유: av_seek_frame(BACKWARD) 가 *target 이전 keyframe* 으로 점프 → keyframe→target 사이 frame 들 decode.
        //skip 없이 keyframe 부터 emit 하면 sparse-keyframe 미디어 (mkv 29초 GOP 등) 에서 *작은 seek* (5초 step / frame step)
        //이 *같은 keyframe* 도달 → 같은 frame 표시 → 사용자 perception "현재 위치 회귀".
        //skip 으로 *target 위치 frame 부터 emit* → 정확한 위치 이동.
        //freeze 우려 (이전 fix 시 NewSegment + sample timing mismatch 로 wait 발생) 는 NewSegment(0,...) + sample.rtStart=0
        //부터 누적 fix 가 이미 적용 → skip 해도 첫 emit sample.rtStart=0 즉시 표시.
        AVRational video_tb = dec.video_time_base();
        REFERENCE_TIME seg_rt = 0;
        int64_t seek_target_pts = 0;

        AVFrame* frame = NULL;
        int skipped = 0;
        int wait_v_ms = 0;
        while (true)
        {
            frame = dec.pop_video_frame();
            if (frame)
            {
                seg_rt = dec.segment_rt();   //generation 과 동기.
                seek_target_pts = (video_tb.num > 0 && video_tb.den > 0) ?
                    av_rescale_q(seg_rt, AVRational{1, 10000000}, video_tb) : 0;

                //pre-target skip — target 도달 frame 부터 emit. 단 *forward keyframe fallback* (MPEG-TS 등) case 에서
                //모든 frame 이 target 보다 크면 skip 안 됨 (첫 frame emit).
                if (frame->pts != AV_NOPTS_VALUE && frame->pts < seek_target_pts)
                {
                    ++skipped;
                    av_frame_free(&frame);
                    continue;
                }
                if (skipped > 0)
                    logWrite(_T("[ffi/src/video/diag] pre-target skip=%d frame_pts=%lld target_pts=%lld"),
                        skipped, (long long)frame->pts, (long long)seek_target_pts);
                break;
            }
            if (!dec.is_running())
                return S_FALSE;
            //EOS — decoder 가 av_read_frame EOF + queue 비면 stream 끝. S_FALSE → DeliverEndOfStream → EC_COMPLETE.
            if (dec.is_eof() && dec.video_queue_size() == 0)
            {
                logWrite(_T("[ffi/src/video/diag] EOS reached — DeliverEndOfStream"));
                return S_FALSE;
            }
            //CMD_STOP / CMD_PAUSE 등 pending command peek — close 시 wait loop 깨기 (deadlock 회피).
            //CheckRequest 는 peek (consume X), DoBufferProcessingLoop 가 정상 GetRequest+Reply 처리.
            Command pending;
            if (CheckRequest(&pending))
                return S_FALSE;
            Sleep(2);
            wait_v_ms += 2;
            if (wait_v_ms == 500 || wait_v_ms == 2000 || wait_v_ms == 5000)
                logWrite(_T("[ffi/src/video/diag] FillBuffer waiting %dms queue empty"), wait_v_ms);
        }
        if (wait_v_ms >= 100)
            logWrite(_T("[ffi/src/video/diag] wait=%dms frame_pts=%lld seg_rt=%lld"),
                wait_v_ms, (long long)frame->pts, (long long)seg_rt);

        BYTE* pData = NULL;
        HRESULT hr = pSample->GetPointer(&pData);
        if (FAILED(hr) || !pData)
        {
            av_frame_free(&frame);
            return hr;
        }

        int w = dec.video_width();
        int h = dec.video_height();
        int aligned_w = (w + 127) & ~127;   //DecideBufferSize 와 같은 alignment.
        long buffer_size = pSample->GetSize();
        long required = aligned_w * h * 3 / 2;
        if (buffer_size < required)
        {
            av_frame_free(&frame);
            return E_FAIL;
        }

        //fast path — frame 이 이미 NV12 (HW transferred 등) 면 swscale 우회, 직접 copy.
        //매 frame swscale alloc/free 가 thread-unsafe 또는 무거워 crash/freeze 원인.
        if ((AVPixelFormat)frame->format == AV_PIX_FMT_NV12)
        {
            //dst stride = aligned_w. src 의 valid 영역은 처음 w byte, 나머지는 padding (0 또는 stale 무관).
            uint8_t* dst_y  = pData;
            uint8_t* dst_uv = pData + aligned_w * h;
            for (int y = 0; y < h; ++y)
                memcpy(dst_y + y * aligned_w, frame->data[0] + y * frame->linesize[0], w);
            for (int y = 0; y < h / 2; ++y)
                memcpy(dst_uv + y * aligned_w, frame->data[1] + y * frame->linesize[1], w);
        }
        else
        {
            //AVFrame → NV12 변환 (YUV420P 등). swscale 로 통합 처리.
            SwsContext* sws = sws_getContext(
                frame->width, frame->height, (AVPixelFormat)frame->format,
                w, h, AV_PIX_FMT_NV12,
                SWS_BILINEAR, NULL, NULL, NULL);

            if (!sws)
            {
                av_frame_free(&frame);
                return E_FAIL;
            }

            //dst_planes/dst_strides 는 size 4 배열 — swscale 이 4 plane 가정. size 2 면 [2], [3] 가 stack
            //uninitialized (0xCCCC...) 인데 swscale 이 NULL check 없이 access → crash. NV12 는 2 plane 만 쓰지만
            //배열 자체는 4 개 필요.
            uint8_t* dst_planes[4] = { pData, pData + aligned_w * h, NULL, NULL };
            int      dst_strides[4] = { aligned_w, aligned_w, 0, 0 };

            sws_scale(sws, frame->data, frame->linesize, 0, frame->height,
                      dst_planes, dst_strides);
            sws_freeContext(sws);
        }

        pSample->SetActualDataLength(required);
        //flush 후 첫 sample 은 SyncPoint=TRUE 강제 — keyframe walk 후 첫 frame 이 정확한 IDR 이 아닐 수 있으나
        //MPCVR 는 SyncPoint 첫 sample 만 신뢰해 internal queue baseline 설정. 누락 시 stuck 가능.
        BOOL is_sync = (frame->flags & AV_FRAME_FLAG_KEY) || m_need_discontinuity;
        pSample->SetSyncPoint(is_sync);

        //Sample timing — frame.pts 직접 사용 (video first emit 기준 segment-local).
        //rtStart = frame_pts_rt - video_first_emit_pts_rt. video 의 첫 frame.rtStart = 0, 이후 frame 의 *실제 pts 차이* 반영.
        //이렇게 하면 VFR (variable frame rate) / 23.976/24/29.97 등 *실제 frame rate* 자동 흡수. m_sample_count * 30fps
        //누적 시 mkv 의 실제 fps 와 mismatch 발생 — video 가 빠르게/느리게 진행 → audio 와 어긋남.
        //frame.pts 직접 사용 + video first 기준이라 sample.rtStart 가 *segment-local 0 부터 작은 양수* → freeze 회피.
        REFERENCE_TIME rtStart;
        if (frame->pts != AV_NOPTS_VALUE)
        {
            AVRational tb = dec.video_time_base();
            REFERENCE_TIME frame_pts_rt = av_rescale_q(frame->pts, tb, AVRational{1, 10000000});
            int64_t video_first = dec.video_first_emit_pts_rt();
            if (video_first == LLONG_MIN)
            {
                //이 emit 가 *FillBuffer 의 첫 emit* — pre-target skip 통과한 frame. audio 가 reference 로 사용할 anchor.
                video_first = frame_pts_rt;
                dec.set_video_first_emit_pts_rt(video_first);
            }
            rtStart = frame_pts_rt - video_first;
        }
        else
        {
            double fps = dec.frame_rate();
            if (fps <= 0.0) fps = 30.0;
            rtStart = (REFERENCE_TIME)(m_sample_count * 10000000.0 / fps);
        }
        double fps_for_stop = dec.frame_rate();
        if (fps_for_stop <= 0.0) fps_for_stop = 30.0;
        REFERENCE_TIME rtStop = rtStart + (REFERENCE_TIME)(10000000.0 / fps_for_stop);

        //rate scaling — graph IMediaSeeking::SetRate → ChangeRate 콜백을 통해 source 의 m_playback_rate 갱신.
        const double rate = m_pSource->playback_rate();
        if (rate > 0.0 && rate != 1.0)
        {
            rtStart = (REFERENCE_TIME)((double)rtStart / rate);
            rtStop  = (REFERENCE_TIME)((double)rtStop  / rate);
        }

        pSample->SetTime(&rtStart, &rtStop);
        m_last_rtStart = rtStart;   //GetCurrentPosition 응답값.

        //flush 후 첫 sample 에 Discontinuity flag — renderer 의 internal state reset 알림.
        //누락 시 MPCVR 가 이전 segment continuation 으로 처리 → Receive 가 stuck.
        if (m_need_discontinuity)
        {
            pSample->SetDiscontinuity(TRUE);
            m_need_discontinuity = false;
        }

        ++m_sample_count;

        //매 frame 의 rt 추적 — seek 후 first frame + 매 60 frame.
        if (m_sample_count <= 5 || m_sample_count % 60 == 0)
        {
            logWrite(_T("[ffi/src] frame[%lld] pts=%lld rtStart=%lld(ms=%lld) seg=%lld key=%d"),
                m_sample_count, (long long)frame->pts,
                (long long)rtStart, (long long)(rtStart / 10000),
                (long long)m_segment_start,
                (int)((frame->flags & AV_FRAME_FLAG_KEY) ? 1 : 0));
        }

        av_frame_free(&frame);
        return NOERROR;
    }

    //============================================================
    // CFFiAudioStream
    //============================================================

    CFFiAudioStream::CFFiAudioStream(HRESULT* phr, CFFiSource* pParent, LPCWSTR pPinName)
        : CSourceStream(NAME("CFFiAudioStream"), phr, pParent, pPinName)
        , m_pSource(pParent)
    {
        //output format: S16, **STEREO 강제 downmix**.
        //이유: DSound 의 plain WAVEFORMATEX 는 mono/stereo PCM 만 받음. 5.1ch 등은 WAVEFORMATEXTENSIBLE 필요.
        //DTS 6ch / AC3 6ch 등 multi-channel 미디어에서 DSound 가 Render(pin) 시 0x80040200 (VFW_E_INVALIDMEDIATYPE) reject → audio 무음.
        //swresample 이 downmix 자동 처리.
        if (pParent && pParent->decoder().has_audio())
        {
            m_out_sample_rate = pParent->decoder().audio_sample_rate();
            m_out_channels    = 2;   //강제 stereo downmix.

            av_channel_layout_default(&m_out_chlayout, m_out_channels);
        }
    }

    CFFiAudioStream::~CFFiAudioStream()
    {
        if (m_swr)
        {
            swr_free(&m_swr);
            m_swr = nullptr;
        }
        release_audio_filter();
        av_channel_layout_uninit(&m_out_chlayout);
    }

    bool CFFiAudioStream::init_audio_filter(double rate)
    {
        release_audio_filter();

        if (m_out_sample_rate <= 0 || m_out_channels <= 0)
            return false;

        m_filter_graph = avfilter_graph_alloc();
        if (!m_filter_graph)
            return false;

        const AVFilter* abuffer    = avfilter_get_by_name("abuffer");
        const AVFilter* atempo     = avfilter_get_by_name("atempo");
        const AVFilter* abuffersink = avfilter_get_by_name("abuffersink");
        if (!abuffer || !atempo || !abuffersink)
        {
            logWrite(_T("[ffi/src/audio/filter] filter lookup fail abuffer=%p atempo=%p abuffersink=%p"),
                abuffer, atempo, abuffersink);
            release_audio_filter();
            return false;
        }

        //abuffer source — swr 의 output format (S16 interleaved, stereo, m_out_sample_rate Hz).
        char layout_desc[64] = {0};
        av_channel_layout_describe(&m_out_chlayout, layout_desc, sizeof(layout_desc));
        char src_args[256] = {0};
        _snprintf_s(src_args, _TRUNCATE,
            "sample_rate=%d:sample_fmt=s16:channel_layout=%s:time_base=1/%d",
            m_out_sample_rate, layout_desc, m_out_sample_rate);

        int hr = avfilter_graph_create_filter(&m_filter_src, abuffer, "in",
            src_args, NULL, m_filter_graph);
        if (hr < 0) { logWrite(_T("[ffi/src/audio/filter] abuffer create fail hr=%d"), hr); release_audio_filter(); return false; }

        //atempo — clamp rate to [0.5, 2.0] (atempo 단일 valid range). 그 밖은 1.0 으로 fallback (외부에서 chain 처리 가능하지만 일단 단일).
        double clamped = rate;
        if (clamped < 0.5) clamped = 0.5;
        if (clamped > 2.0) clamped = 2.0;
        char tempo_args[32] = {0};
        _snprintf_s(tempo_args, _TRUNCATE, "tempo=%.3f", clamped);
        hr = avfilter_graph_create_filter(&m_filter_atempo, atempo, "atempo",
            tempo_args, NULL, m_filter_graph);
        if (hr < 0) { logWrite(_T("[ffi/src/audio/filter] atempo create fail hr=%d"), hr); release_audio_filter(); return false; }

        hr = avfilter_graph_create_filter(&m_filter_sink, abuffersink, "out",
            NULL, NULL, m_filter_graph);
        if (hr < 0) { logWrite(_T("[ffi/src/audio/filter] abuffersink create fail hr=%d"), hr); release_audio_filter(); return false; }

        //sink 의 출력 sample format 강제 S16 — swr 와 일치.
        //av_opt_set_int_list 가 내부에서 deprecated av_int_list_length_for_size 호출 — C4996 회피.
        const AVSampleFormat out_fmts[] = { AV_SAMPLE_FMT_S16, AV_SAMPLE_FMT_NONE };
#pragma warning(push)
#pragma warning(disable: 4996)
        av_opt_set_int_list(m_filter_sink, "sample_fmts", out_fmts, AV_SAMPLE_FMT_NONE, AV_OPT_SEARCH_CHILDREN);
#pragma warning(pop)

        //link: src → atempo → sink
        hr = avfilter_link(m_filter_src, 0, m_filter_atempo, 0);
        if (hr < 0) { logWrite(_T("[ffi/src/audio/filter] link src->atempo fail hr=%d"), hr); release_audio_filter(); return false; }
        hr = avfilter_link(m_filter_atempo, 0, m_filter_sink, 0);
        if (hr < 0) { logWrite(_T("[ffi/src/audio/filter] link atempo->sink fail hr=%d"), hr); release_audio_filter(); return false; }

        hr = avfilter_graph_config(m_filter_graph, NULL);
        if (hr < 0) { logWrite(_T("[ffi/src/audio/filter] graph_config fail hr=%d"), hr); release_audio_filter(); return false; }

        m_filter_rate = clamped;
        logWrite(_T("[ffi/src/audio/filter] init OK rate=%.3f sr=%d ch=%d layout=%S"),
            m_filter_rate, m_out_sample_rate, m_out_channels, layout_desc);
        return true;
    }

    void CFFiAudioStream::release_audio_filter()
    {
        if (m_filter_graph)
        {
            avfilter_graph_free(&m_filter_graph);
            m_filter_graph = nullptr;
        }
        m_filter_src = nullptr;
        m_filter_atempo = nullptr;
        m_filter_sink = nullptr;
        m_filter_rate = 1.0;
    }

    bool CFFiAudioStream::update_audio_filter_rate(double rate)
    {
        double clamped = rate;
        if (clamped < 0.5) clamped = 0.5;
        if (clamped > 2.0) clamped = 2.0;
        if (clamped == m_filter_rate)
            return true;

        //단순 send_command "tempo" 만으로는 atempo internal state 가 잔류 — 간헐 sync 어긋남.
        //graph 자체 재생성으로 internal state 완전 초기화 (LAV path 의 SCAudioTimeStretch 와 동일 패턴).
        logWrite(_T("[ffi/src/audio/filter] tempo %.3f → %.3f (graph rebuild)"), m_filter_rate, clamped);
        return init_audio_filter(clamped);
    }

    HRESULT CFFiAudioStream::OnThreadCreate()
    {
        m_sample_count = 0;
        m_audio_offset_rt = 0;
        m_audio_offset_set = false;

        //m_pending_segment_stop 미초기화 시 duration fallback. video pin 과 동일.
        if (m_pSource && m_pending_segment_stop <= 0)
            m_pending_segment_stop = (REFERENCE_TIME)(m_pSource->decoder().duration_ms() * 10000.0);

        //SwrContext 초기화 — decoder 의 sample format → S16 interleaved.
        if (!m_swr && m_pSource && m_pSource->decoder().has_audio())
        {
            CDecoder& dec = m_pSource->decoder();
            AVCodecContext* actx = nullptr;   //decoder 의 audio context 접근 — CDecoder 에 getter 없으면 sample_fmt 만 query.
            //sample_fmt 를 raw int 로 받음.
            AVSampleFormat in_fmt = (AVSampleFormat)dec.audio_sample_format();

            m_swr = swr_alloc();

            AVChannelLayout in_layout;
            av_channel_layout_default(&in_layout, dec.audio_channels());

            av_opt_set_chlayout(m_swr, "in_chlayout",     &in_layout, 0);
            av_opt_set_int     (m_swr, "in_sample_rate",  dec.audio_sample_rate(), 0);
            av_opt_set_sample_fmt(m_swr, "in_sample_fmt", in_fmt, 0);

            av_opt_set_chlayout(m_swr, "out_chlayout",    &m_out_chlayout, 0);
            av_opt_set_int     (m_swr, "out_sample_rate", m_out_sample_rate, 0);
            av_opt_set_sample_fmt(m_swr, "out_sample_fmt", AV_SAMPLE_FMT_S16, 0);

            int hr = swr_init(m_swr);
            av_channel_layout_uninit(&in_layout);
            if (hr < 0)
            {
                logWrite(_T("[ffi/src/audio] swr_init fail hr=%d"), hr);
                swr_free(&m_swr);
                m_swr = nullptr;
                return E_FAIL;
            }

            logWrite(_T("[ffi/src/audio] swr ready in_fmt=%d → S16, %dHz ch=%d"),
                (int)in_fmt, m_out_sample_rate, m_out_channels);
        }

        //atempo filter graph — source 의 현재 playback_rate 로 init. 이후 FillBuffer 가 rate 변경 감지 시 send_command.
        if (m_swr && !m_filter_graph)
        {
            double init_rate = m_pSource ? m_pSource->playback_rate() : 1.0;
            init_audio_filter(init_rate);
        }

        if (m_pSource && !m_pSource->decoder().is_running())
            m_pSource->decoder().start();

        return NOERROR;
    }

    HRESULT CFFiAudioStream::OnThreadDestroy()
    {
        if (m_swr)
        {
            swr_free(&m_swr);
            m_swr = nullptr;
        }
        release_audio_filter();
        return NOERROR;
    }

    HRESULT CFFiAudioStream::GetMediaType(CMediaType* pMediaType)
    {
        if (!pMediaType || m_out_sample_rate <= 0 || m_out_channels <= 0)
            return E_FAIL;

        WAVEFORMATEX* pwfx = (WAVEFORMATEX*)pMediaType->AllocFormatBuffer(sizeof(WAVEFORMATEX));
        if (!pwfx)
            return E_OUTOFMEMORY;
        ZeroMemory(pwfx, sizeof(WAVEFORMATEX));

        pwfx->wFormatTag      = WAVE_FORMAT_PCM;
        pwfx->nChannels       = (WORD)m_out_channels;
        pwfx->nSamplesPerSec  = (DWORD)m_out_sample_rate;
        pwfx->wBitsPerSample  = 16;
        pwfx->nBlockAlign     = pwfx->nChannels * pwfx->wBitsPerSample / 8;
        pwfx->nAvgBytesPerSec = pwfx->nSamplesPerSec * pwfx->nBlockAlign;
        pwfx->cbSize          = 0;

        pMediaType->SetType(&MEDIATYPE_Audio);
        pMediaType->SetFormatType(&FORMAT_WaveFormatEx);
        pMediaType->SetTemporalCompression(FALSE);
        pMediaType->SetSubtype(&MEDIASUBTYPE_PCM);
        pMediaType->SetSampleSize(pwfx->nBlockAlign);

        return NOERROR;
    }

    HRESULT CFFiAudioStream::CheckMediaType(const CMediaType* pmt)
    {
        if (!pmt) return E_POINTER;
        if (*pmt->Type()       != MEDIATYPE_Audio)       return E_INVALIDARG;
        if (*pmt->Subtype()    != MEDIASUBTYPE_PCM)      return E_INVALIDARG;
        if (*pmt->FormatType() != FORMAT_WaveFormatEx)   return E_INVALIDARG;
        return S_OK;
    }

    HRESULT CFFiAudioStream::DecideBufferSize(IMemAllocator* pAlloc, ALLOCATOR_PROPERTIES* pProperties)
    {
        if (!pAlloc || !pProperties)
            return E_POINTER;

        //1 frame ≈ 1024 samples (aac default). S16 stereo 44.1kHz: 1024 * 4 = 4096 bytes.
        //여유 두고 8192 bytes (1024 sample × 8byte/sample [up to 8ch]).
        pProperties->cBuffers = 8;
        pProperties->cbBuffer = 8192;

        ALLOCATOR_PROPERTIES Actual;
        HRESULT hr = pAlloc->SetProperties(pProperties, &Actual);
        if (FAILED(hr))
            return hr;
        if (Actual.cbBuffer < pProperties->cbBuffer)
            return E_FAIL;
        return NOERROR;
    }

    HRESULT CFFiAudioStream::FillBuffer(IMediaSample* pSample)
    {
        if (!pSample || !m_pSource || !m_swr)
            return E_POINTER;

        CDecoder& dec = m_pSource->decoder();

        //audio frame skip — video 의 첫 emit pts_rt anchor 까지. 두 stream sync 위해.
        //MPEG-TS 등에서 video keyframe 이 target+3초 forward fallback 시 audio 도 그 시점부터 시작.
        AVRational audio_tb = dec.audio_time_base();
        int audio_skipped = 0;
        int wait_anchor_ms = 0;

        AVFrame* frame = NULL;
        int wait_ms = 0;
        while (true)
        {
            frame = dec.pop_audio_frame();
            if (frame)
            {
                //video anchor 확인 — set 안 되어 있으면 short wait (video first emit 까지).
                int64_t video_anchor = dec.video_first_emit_pts_rt();
                if (video_anchor == LLONG_MIN && wait_anchor_ms < 200)
                {
                    //video first emit 아직 안 됨 — frame 다시 큐 앞에 못 넣음. discard + wait.
                    av_frame_free(&frame);
                    Sleep(5);
                    wait_anchor_ms += 5;
                    continue;
                }
                //anchor skip — 항상 적용. audio first emit 의 미디어 시점 = video first emit 의 미디어 시점 강제.
                //video / audio 가 각자 first emit 시점 부터 segment-local 0 인 시간계라 정확한 시점 정렬 필요.
                if (video_anchor != LLONG_MIN && audio_tb.num > 0 && audio_tb.den > 0)
                {
                    int64_t anchor_pts = av_rescale_q(video_anchor,
                        AVRational{1, 10000000}, audio_tb);
                    if (frame->pts != AV_NOPTS_VALUE && frame->pts < anchor_pts)
                    {
                        ++audio_skipped;
                        av_frame_free(&frame);
                        continue;
                    }
                }
                if (audio_skipped > 0)
                    logWrite(_T("[ffi/src/audio/diag] anchor skip=%d frame_pts=%lld video_anchor_rt=%lld"),
                        audio_skipped, (long long)frame->pts, (long long)video_anchor);
                break;
            }
            if (!dec.is_running())
                return S_FALSE;
            //EOS — video pin 과 동일. audio queue 도 비면 stream 끝.
            if (dec.is_eof() && dec.audio_queue_size() == 0)
            {
                logWrite(_T("[ffi/src/audio/diag] EOS reached — DeliverEndOfStream"));
                return S_FALSE;
            }
            //video pin 과 동일 — pending CMD peek 로 wait loop 깨기.
            Command pending;
            if (CheckRequest(&pending))
                return S_FALSE;
            Sleep(2);
            wait_ms += 2;
            if (wait_ms == 500 || wait_ms == 2000 || wait_ms == 5000)
                logWrite(_T("[ffi/src/audio/diag] FillBuffer waiting %dms audio_queue empty"), wait_ms);
        }

        BYTE* pData = NULL;
        HRESULT hr = pSample->GetPointer(&pData);
        if (FAILED(hr) || !pData)
        {
            av_frame_free(&frame);
            return hr;
        }
        long buffer_size = pSample->GetSize();

        //swr_convert — frame (planar / 다양한 fmt) → S16 interleaved.
        int max_out_samples = buffer_size / (m_out_channels * 2);   //S16 = 2 bytes/sample
        uint8_t* out_planes[1] = { pData };

        int out_samples = swr_convert(m_swr,
            out_planes, max_out_samples,
            (const uint8_t**)frame->extended_data, frame->nb_samples);
        if (out_samples < 0)
        {
            av_frame_free(&frame);
            return E_FAIL;
        }

        //atempo time-stretch — rate != 1.0 일 때 PCM sample count 를 1/rate 로 줄여 graph clock 가속 + pitch 유지.
        //rate==1.0 도 동일 path (1:1 pass-through) 로 코드 일관. swr output 을 frame 으로 wrap → buffersrc 에 push → buffersink 에서 pull.
        if (m_filter_graph && out_samples > 0)
        {
            //source rate 변경 감지 → atempo tempo 갱신.
            double cur_rate = m_pSource ? m_pSource->playback_rate() : 1.0;
            if (cur_rate != m_filter_rate)
                update_audio_filter_rate(cur_rate);

            //swr output 을 AVFrame 으로 wrap. pData 가 swr 의 output buffer 이므로 별도 copy 필요.
            AVFrame* in_frame = av_frame_alloc();
            if (in_frame)
            {
                in_frame->format = AV_SAMPLE_FMT_S16;
                in_frame->sample_rate = m_out_sample_rate;
                av_channel_layout_copy(&in_frame->ch_layout, &m_out_chlayout);
                in_frame->nb_samples = out_samples;
                if (av_frame_get_buffer(in_frame, 0) >= 0)
                {
                    memcpy(in_frame->data[0], pData, out_samples * m_out_channels * 2);
                    av_buffersrc_add_frame(m_filter_src, in_frame);
                }
                av_frame_free(&in_frame);
            }

            //buffersink 에서 가능한 만큼 output frame pull → pData 에 누적 copy. buffer overflow 시 break.
            int total_bytes = 0;
            while (true)
            {
                AVFrame* out_frame = av_frame_alloc();
                if (!out_frame) break;
                int hr_pull = av_buffersink_get_frame(m_filter_sink, out_frame);
                if (hr_pull < 0)
                {
                    av_frame_free(&out_frame);
                    break;
                }
                int out_bytes = out_frame->nb_samples * m_out_channels * 2;
                if (total_bytes + out_bytes > buffer_size)
                {
                    av_frame_free(&out_frame);
                    break;
                }
                memcpy(pData + total_bytes, out_frame->data[0], out_bytes);
                total_bytes += out_bytes;
                av_frame_free(&out_frame);
            }
            out_samples = total_bytes / (m_out_channels * 2);
        }

        int bytes_written = out_samples * m_out_channels * 2;
        pSample->SetActualDataLength(bytes_written);
        pSample->SetSyncPoint(TRUE);

        //[diag] FillBuffer 호출 카운트 + 100 호출마다 amplitude 출력. seek 회귀 진단.
        static thread_local int s_fill_count = 0;
        ++s_fill_count;
        if (s_fill_count == 1 || (s_fill_count % 100) == 0)
        {
            int16_t* s16 = (int16_t*)pData;
            int n = out_samples * m_out_channels;
            int16_t mx = 0;
            for (int i = 0; i < n && i < 4096; ++i)
            {
                int16_t v = s16[i];
                if (v < 0) v = -v;
                if (v > mx) mx = v;
            }
            logWrite(_T("[ffi/src/audio/diag] fill#%d samples=%d ch=%d nb=%d fmt=%d max_abs=%d sc=%lld"),
                s_fill_count, out_samples, m_out_channels, frame->nb_samples, (int)frame->format,
                (int)mx, (long long)m_sample_count);
        }

        //timing —
        //- atempo 활성 시: sample_count 기반 (atempo 후 sample 양으로 누적되므로 자동 1/rate scale). anchor 는 첫 emit 시 video_first 와 한 번 정렬.
        //- atempo 비활성 (init 실패) 시: 기존 frame.pts 기반 (video first 기준 segment-local).
        REFERENCE_TIME rtStart;
        if (m_filter_graph)
        {
            if (!m_audio_offset_set)
            {
                int64_t video_first = dec.video_first_emit_pts_rt();
                int64_t audio_pts_rt = (frame->pts != AV_NOPTS_VALUE) ?
                    av_rescale_q(frame->pts, audio_tb, AVRational{1, 10000000}) : 0;
                m_audio_offset_rt = (video_first != LLONG_MIN) ? (audio_pts_rt - video_first) : 0;
                m_audio_offset_set = true;
            }
            rtStart = m_audio_offset_rt +
                (REFERENCE_TIME)((double)m_sample_count * 10000000.0 / m_out_sample_rate);
        }
        else if (frame->pts != AV_NOPTS_VALUE)
        {
            int64_t audio_pts_rt = av_rescale_q(frame->pts, audio_tb, AVRational{1, 10000000});
            int64_t video_first = dec.video_first_emit_pts_rt();
            if (video_first == LLONG_MIN) video_first = audio_pts_rt;
            rtStart = audio_pts_rt - video_first;
        }
        else
        {
            rtStart = (REFERENCE_TIME)((double)m_sample_count * 10000000.0 / m_out_sample_rate);
        }
        m_sample_count += out_samples;
        REFERENCE_TIME rtStop = rtStart + (REFERENCE_TIME)((double)out_samples * 10000000.0 / m_out_sample_rate);

        pSample->SetTime(&rtStart, &rtStop);

        if (m_need_discontinuity)
        {
            pSample->SetDiscontinuity(TRUE);
            m_need_discontinuity = false;
        }

        av_frame_free(&frame);
        return NOERROR;
    }

    void CFFiAudioStream::on_seek_flush(REFERENCE_TIME rtStart)
    {
        REFERENCE_TIME rtStop = m_pSource ?
            (REFERENCE_TIME)(m_pSource->decoder().duration_ms() * 10000.0) : 0;
        //state 갱신은 ThreadExists 무관. play(Running) 첫 thread create 시 OnThreadStartPlay 가 이 값을 사용.
        m_sample_count = 0;
        m_pending_segment_stop = rtStop;
        m_audio_offset_rt = 0;
        m_audio_offset_set = false;   //다음 FillBuffer 첫 frame 시 video_first_emit_pts_rt 와 비교 후 set.

        //atempo graph 재생성 — internal latency 잔류 제거 (LAV path 의 SCAudioTimeStretch NewSegment 처리와 동일).
        //현재 rate 유지 — m_filter_rate 가 직전 rate (init_audio_filter 가 그 rate 로 atempo 재init).
        if (m_filter_graph)
            init_audio_filter(m_filter_rate);

        //streaming 중인 경우만 Stop/Flush/Pause cycle.
        if (ThreadExists())
        {
            DeliverBeginFlush();
            Stop();
            DeliverEndFlush();
            Pause();
        }
    }

    HRESULT CFFiAudioStream::OnThreadStartPlay()
    {
        //MS 표준 — segment[0, duration] + sample.rtStart 는 segment-local time. video pin 과 동일.
        m_need_discontinuity = true;
        return DeliverNewSegment(0, m_pending_segment_stop, 1.0);
    }

    //============================================================
    // CFFiSource
    //============================================================

    CFFiSource::CFFiSource(LPUNKNOWN pUnk, HRESULT* phr)
        : CSource(NAME("CFFiSource"), pUnk, CLSID_FFiSource, phr)
    {
        if (phr && SUCCEEDED(*phr))
        {
            //Pin 은 open_file 성공 후 생성. graph add 전까지 pin 0 개라도 무방.
        }
    }

    CFFiSource::~CFFiSource()
    {
        //pin 들의 stop 은 CSource 의 base destructor 가 호출. m_decoder 는 소멸 시 close 자동.
    }

    HRESULT CFFiSource::open_file(const wchar_t* utf16_path)
    {
        if (!utf16_path)
            return E_INVALIDARG;

        if (!m_decoder.open(utf16_path))
        {
            logWrite(_T("[ffi/src] open_file decoder.open fail"));
            return E_FAIL;
        }

        //Video pin 생성. CSourceStream ctor 가 AddPin 자동.
        HRESULT hr = S_OK;
        m_pVideoStream = new CFFiVideoStream(&hr, this, L"Video");
        if (!m_pVideoStream || FAILED(hr))
        {
            logWrite(_T("[ffi/src] video stream create fail hr=0x%08x"), hr);
            return FAILED(hr) ? hr : E_OUTOFMEMORY;
        }

        //Audio pin — decoder 에 audio stream 있으면 생성.
        if (m_decoder.has_audio())
        {
            HRESULT hr_a = S_OK;
            m_pAudioStream = new CFFiAudioStream(&hr_a, this, L"Audio");
            if (!m_pAudioStream || FAILED(hr_a))
            {
                logWrite(_T("[ffi/src] audio stream create fail hr=0x%08x"), hr_a);
                //audio 실패해도 video 만으로 계속.
                m_pAudioStream = nullptr;
            }
        }

        logWrite(_T("[ffi/src] open_file OK %dx%d fps=%.2f duration=%.0fms audio=%d"),
            m_decoder.video_width(), m_decoder.video_height(),
            m_decoder.frame_rate(), m_decoder.duration_ms(),
            (int)m_decoder.has_audio());

        return S_OK;
    }
}
