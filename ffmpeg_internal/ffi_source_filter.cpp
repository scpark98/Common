//Include 순서 critical:
//  1) winsock2.h 가 afxwin.h 보다 먼저 — MFC 가 Winsock2 detection 강제.
//  2) afxwin.h 가 streams.h 보다 먼저 — BaseClasses wxlist.h 가 __AFX_H__ 매크로로 __POSITION 정의 가드 (MFC 와 충돌 회피).
#include <winsock2.h>
#include <afxwin.h>

#include "ffi_source_filter.h"

#include "../Functions.h"
#include "../log/SCLog/SCLog.h"

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
    HRESULT CFFiSeeking::ChangeRate()  { return S_OK; }   //rate 변경 Phase 후속.

    STDMETHODIMP CFFiSeeking::GetCurrentPosition(LONGLONG* pCurrent)
    {
        if (!pCurrent)
            return E_POINTER;
        *pCurrent = m_pPin ? m_pPin->last_rtStart() : 0;
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

        //video pin 의 downstream flush + new segment.
        DeliverBeginFlush();
        DeliverEndFlush();

        //CDecoder.seek 는 video / audio 큐 모두 flush + 두 codec 모두 flush_buffers.
        m_pSource->decoder().seek(pos_ms);

        m_sample_count = 0;
        m_last_rtStart = 0;

        DeliverNewSegment(0, rtStop, 1.0);

        //audio pin 도 동일한 flush + new segment 전파.
        if (m_pSource->audio_stream())
            m_pSource->audio_stream()->on_seek_flush(rtStart);

        logWrite(_T("[ffi/src] on_change_start → seek %.0fms + flush + new_segment(0..%lld)"),
            pos_ms, (long long)rtStop);
    }

    HRESULT CFFiVideoStream::OnThreadCreate()
    {
        m_sample_count = 0;
        //CDecoder 의 worker thread 가 아직 안 돌고 있으면 시작.
        if (m_pSource && !m_pSource->decoder().is_running())
            m_pSource->decoder().start();
        logWrite(_T("[ffi/src] video stream thread create"));
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
        double fps = dec.frame_rate();
        if (fps <= 0.0)
            fps = 30.0;

        //NV12 media type 구성. MPC-VR / EVR / VMR9 모두 NV12 받음.
        VIDEOINFOHEADER* pvi = (VIDEOINFOHEADER*)pMediaType->AllocFormatBuffer(sizeof(VIDEOINFOHEADER));
        if (!pvi)
            return E_OUTOFMEMORY;
        ZeroMemory(pvi, sizeof(VIDEOINFOHEADER));

        pvi->bmiHeader.biSize        = sizeof(BITMAPINFOHEADER);
        pvi->bmiHeader.biWidth       = w;
        pvi->bmiHeader.biHeight      = h;          //양수 = bottom-up. NV12 는 top-down 이 표준이지만 양수도 처리됨.
        pvi->bmiHeader.biPlanes      = 1;
        pvi->bmiHeader.biBitCount    = 12;         //NV12 = 12 bpp
        pvi->bmiHeader.biCompression = MAKEFOURCC('N','V','1','2');
        pvi->bmiHeader.biSizeImage   = w * h * 3 / 2;

        pvi->AvgTimePerFrame = (REFERENCE_TIME)(10000000.0 / fps);   //100ns 단위
        pvi->rcSource.right  = w;
        pvi->rcSource.bottom = h;
        pvi->rcTarget        = pvi->rcSource;
        pvi->dwBitRate       = w * h * 3 / 2 * 8 * (DWORD)fps;
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

        pProperties->cBuffers = 3;   //triple buffer — render 가 한 frame 사용 중에도 우리는 다음 frame 채울 수 있음.
        pProperties->cbBuffer = w * h * 3 / 2;   //NV12 사이즈

        ALLOCATOR_PROPERTIES Actual;
        HRESULT hr = pAlloc->SetProperties(pProperties, &Actual);
        if (FAILED(hr))
            return hr;

        if (Actual.cbBuffer < pProperties->cbBuffer)
            return E_FAIL;

        return NOERROR;
    }

    HRESULT CFFiVideoStream::FillBuffer(IMediaSample* pSample)
    {
        if (!pSample || !m_pSource)
            return E_POINTER;

        CDecoder& dec = m_pSource->decoder();

        //AVFrame pop — decoder 가 running 인 동안 wait. 빠른 drag 시 timeout S_FALSE 반환 시 stream 종료 → 영구 freeze 회피.
        AVFrame* frame = NULL;
        while (true)
        {
            frame = dec.pop_video_frame();
            if (frame) break;
            if (!dec.is_running())
                return S_FALSE;
            Sleep(2);
        }

        BYTE* pData = NULL;
        HRESULT hr = pSample->GetPointer(&pData);
        if (FAILED(hr) || !pData)
        {
            av_frame_free(&frame);
            return hr;
        }

        int w = dec.video_width();
        int h = dec.video_height();
        long buffer_size = pSample->GetSize();
        long required = w * h * 3 / 2;
        if (buffer_size < required)
        {
            av_frame_free(&frame);
            return E_FAIL;
        }

        //AVFrame → NV12 변환. swscale 로 통합 처리 (입력이 YUV420P / NV12 / 그 외 어느 것이든 모두 NV12 로 출력).
        SwsContext* sws = sws_getContext(
            frame->width, frame->height, (AVPixelFormat)frame->format,
            w, h, AV_PIX_FMT_NV12,
            SWS_BILINEAR, NULL, NULL, NULL);

        if (!sws)
        {
            av_frame_free(&frame);
            return E_FAIL;
        }

        //NV12 destination layout: Y plane (w*h) + UV plane interleaved (w*h/2).
        uint8_t* dst_planes[2] = { pData, pData + w * h };
        int dst_strides[2]     = { w, w };

        sws_scale(sws, frame->data, frame->linesize, 0, frame->height,
                  dst_planes, dst_strides);
        sws_freeContext(sws);

        pSample->SetActualDataLength(required);
        pSample->SetSyncPoint((frame->flags & AV_FRAME_FLAG_KEY) ? TRUE : FALSE);

        //Sample timing — sample_count × 1/fps 기반 stream-relative time.
        //pts (절대 media time) 으로 SetTime 하면 graph clock (0 부터 시작) 과 mismatch → renderer 가 frame 을 멀리 미래 시점에 schedule → freeze.
        //sample_count 는 on_change_start 에서 0 으로 reset 되므로 seek 시 0 부터 다시 시작 → 즉시 표시.
        //(Phase 4 audio 통합 시 pts/clock 정렬 필요 — 그때 재검토.)
        double fps = dec.frame_rate();
        if (fps <= 0.0) fps = 30.0;
        REFERENCE_TIME rtStart = (REFERENCE_TIME)(m_sample_count * 10000000.0 / fps);
        REFERENCE_TIME rtStop  = rtStart + (REFERENCE_TIME)(10000000.0 / fps);
        pSample->SetTime(&rtStart, &rtStop);
        m_last_rtStart = rtStart;   //GetCurrentPosition 응답값.

        ++m_sample_count;

        if (m_sample_count <= 5 || m_sample_count % 60 == 0)
        {
            logWrite(_T("[ffi/src] frame[%lld] pts=%lld rtStart=%lld key=%d"),
                m_sample_count, (long long)frame->pts,
                (long long)rtStart,
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
        //output format: S16, 같은 sample_rate / channel layout. swresample 로 변환.
        if (pParent && pParent->decoder().has_audio())
        {
            m_out_sample_rate = pParent->decoder().audio_sample_rate();
            m_out_channels    = pParent->decoder().audio_channels();

            //channel layout default — mono/stereo/surround.
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
        av_channel_layout_uninit(&m_out_chlayout);
    }

    HRESULT CFFiAudioStream::OnThreadCreate()
    {
        m_sample_count = 0;

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

        //decoder 가 running 인 동안 계속 wait — 빠른 drag 시 큐 비는 시간 길어도 EOS 처리하지 않음.
        //이게 없으면 500ms timeout → S_FALSE → 스트림 종료 → 무음 영구화 (drag 후 더 이상 audio 안 나옴).
        AVFrame* frame = NULL;
        while (true)
        {
            frame = dec.pop_audio_frame();
            if (frame) break;
            if (!dec.is_running())
                return S_FALSE;
            Sleep(2);
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

        int bytes_written = out_samples * m_out_channels * 2;
        pSample->SetActualDataLength(bytes_written);
        pSample->SetSyncPoint(TRUE);

        //timing — sample_count 기반. audio sample_count 는 audio sample 수 (PCM frame 수 X — 그건 너무 빠름).
        //rtStart = (sample_count / sample_rate) × 10^7
        REFERENCE_TIME rtStart = (REFERENCE_TIME)((double)m_sample_count * 10000000.0 / m_out_sample_rate);
        m_sample_count += out_samples;
        REFERENCE_TIME rtStop  = (REFERENCE_TIME)((double)m_sample_count * 10000000.0 / m_out_sample_rate);
        pSample->SetTime(&rtStart, &rtStop);

        av_frame_free(&frame);
        return NOERROR;
    }

    void CFFiAudioStream::on_seek_flush(REFERENCE_TIME rtStart)
    {
        REFERENCE_TIME rtStop = m_pSource ?
            (REFERENCE_TIME)(m_pSource->decoder().duration_ms() * 10000.0) : 0;
        DeliverBeginFlush();
        DeliverEndFlush();
        m_sample_count = 0;
        DeliverNewSegment(0, rtStop, 1.0);
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
