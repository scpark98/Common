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
#include <libavutil/imgutils.h>
}

#include <vector>

namespace ffi
{
    //============================================================
    // CFFiVideoStream
    //============================================================

    CFFiVideoStream::CFFiVideoStream(HRESULT* phr, CFFiSource* pParent, LPCWSTR pPinName)
        : CSourceStream(NAME("CFFiVideoStream"), phr, pParent, pPinName)
        , m_pSource(pParent)
    {
    }

    CFFiVideoStream::~CFFiVideoStream()
    {
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

        //AVFrame pop — 없으면 worker 가 frame 만들 때까지 짧게 대기.
        AVFrame* frame = NULL;
        for (int wait_ms = 0; wait_ms < 500; wait_ms += 5)
        {
            frame = dec.pop_video_frame();
            if (frame) break;
            Sleep(5);
        }
        if (!frame)
        {
            logWrite(_T("[ffi/src] FillBuffer timeout — no frame for 500ms"));
            return S_FALSE;   //EOF or starvation
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

        //Sample timing — frame pts (stream time_base) → REFERENCE_TIME (100ns).
        REFERENCE_TIME rtStart = 0, rtStop = 0;
        if (frame->pts != AV_NOPTS_VALUE)
        {
            AVRational tb = dec.video_time_base();
            //pts × tb × 10^7 → 100ns. av_rescale_q 로 overflow 안전.
            rtStart = av_rescale_q(frame->pts, tb, AVRational{1, 10000000});
        }
        else
        {
            double fps = dec.frame_rate();
            if (fps <= 0.0) fps = 30.0;
            rtStart = (REFERENCE_TIME)(m_sample_count * 10000000.0 / fps);
        }
        rtStop = rtStart + (REFERENCE_TIME)(10000000.0 / (dec.frame_rate() > 0 ? dec.frame_rate() : 30.0));
        pSample->SetTime(&rtStart, &rtStop);

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

        logWrite(_T("[ffi/src] open_file OK %dx%d fps=%.2f duration=%.0fms"),
            m_decoder.video_width(), m_decoder.video_height(),
            m_decoder.frame_rate(), m_decoder.duration_ms());

        return S_OK;
    }
}
