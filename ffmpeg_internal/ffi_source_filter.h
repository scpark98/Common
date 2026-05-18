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

#include "../dshow_baseclasses/streams.h"
#include "ffi_decoder.h"

#include <memory>

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

        //CFFiSeeking 의 ChangeStart 가 호출 — CDecoder::seek 비동기 위임.
        void                on_change_start(REFERENCE_TIME rtStart);

        REFERENCE_TIME      last_rtStart() const { return m_last_rtStart; }
        CFFiSource*         source() { return m_pSource; }

    private:
        CFFiSource*     m_pSource;
        LONGLONG        m_sample_count = 0;   //sample 순번. timestamping 에 사용.
        LONGLONG        m_last_rtStart = 0;   //직전 emit 한 frame 의 rtStart. GetCurrentPosition 의 응답값.

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

    private:
        CFFiSource*     m_pSource;
        LONGLONG        m_sample_count = 0;
        SwrContext*     m_swr = nullptr;
        int             m_out_sample_rate = 0;
        int             m_out_channels = 0;
        AVChannelLayout m_out_chlayout{};
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

    private:
        CDecoder         m_decoder;
        CFFiVideoStream* m_pVideoStream = nullptr;
        CFFiAudioStream* m_pAudioStream = nullptr;   //has_audio 일 때만 생성.
    };
}
