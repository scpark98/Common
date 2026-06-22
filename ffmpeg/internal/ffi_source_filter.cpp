//Include 순서 critical:
//	1) winsock2.h 가 afxwin.h 보다 먼저 — MFC 가 Winsock2 detection 강제.
//	2) afxwin.h 가 streams.h 보다 먼저 — BaseClasses wxlist.h 가 __AFX_H__ 매크로로 __POSITION 정의 가드 (MFC 와 충돌 회피).
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
		//logWrite(_T("[ffi/seek/diag] ChangeRate called, m_dRateSeeking=%.3f pPin=%p source=%p"),
			//m_dRateSeeking, m_pPin, m_pPin ? m_pPin->source() : NULL);
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

		//[seek_prof] sub-step 소요시간 측정 — setpos_us(≈149ms) 가 어느 단계(특히 Stop)에서 소모되는지 확정용.
		LARGE_INTEGER pf, t_enter, t_seek, t_bf, t_stop, t_ef, t_pause, t_audio;
		::QueryPerformanceFrequency(&pf);
		::QueryPerformanceCounter(&t_enter);

		double pos_ms = (double)rtStart / 10000.0;
		REFERENCE_TIME rtStop = (REFERENCE_TIME)(m_pSource->decoder().duration_ms() * 10000.0);

		//keyframe 모드 방향 판정 + 전진 보장용 — seek 직전 재생 위치 (덮어쓰기 전 값).
		int64_t prev_emit_ms = m_last_emitted_pts_ms.load();

		//state 갱신은 ThreadExists 무관 항상. play(Running) 첫 진입 시 OnThreadStartPlay 가 이 값을 사용.
		m_pSource->decoder().seek(pos_ms, rtStart, (double)prev_emit_ms);
		::QueryPerformanceCounter(&t_seek);
		m_sample_count = 0;	  //sample.rtStart = m_sample_count * frame_duration 의 출발점.
		m_last_rtStart = 0;
		m_segment_start = rtStart;
		m_pending_segment_stop = rtStop;

		//seek 후 첫 video frame emit 전까지 get_track_pos 가 stale 반환하지 않도록 seek target 으로 우선 set.
		m_last_emitted_pts_ms.store((int64_t)pos_ms);

		//(A) measurement reset.
		m_seek_t0_ms = GetTickCount64();
		m_fb_count_since_seek = 0;
		m_fb_last_entry_ms = 0;
		m_budget_exceeded_this_seek = false;	  //[latency budget] 새 seek 시작 → flag reset.
		m_kf_mode_this_seek = m_pSource->seek_keyframe_mode();	//이 seek 의 모드 고정 (frame step toggle 우회 race 차단).
		m_frame_step_this_seek = m_pSource->frame_step_mode();	//frame step seek 이면 budget 비활성 (정확 target 까지 skip).

		//streaming 중인 경우만 MS 표준 정공법 (Stop/Flush/Pause cycle) 적용.
		//	BeginFlush → Stop → EndFlush → Pause → OnThreadStartPlay → NewSegment + Discontinuity
		//pre-streaming (graph Stopped, ThreadExists false) — play(Running) 의 thread create → OnThreadStartPlay 가 동일 NewSegment 처리.
		bool thr = ThreadExists();
		t_bf = t_stop = t_ef = t_pause = t_seek;	//non-streaming 시 sub-step delta 0.
		if (thr)
		{
			DeliverBeginFlush();
			::QueryPerformanceCounter(&t_bf);
			Stop();
			::QueryPerformanceCounter(&t_stop);
			DeliverEndFlush();
			::QueryPerformanceCounter(&t_ef);
			//decoder.seek 가 worker 에 비동기 위임. Pause→OnThreadStartPlay 시점까지 seek 처리될 가능성 높음.
			//FillBuffer 의 pre-target skip 이 stale frame (frame.pts < seek_target_pts) drop 으로 race 보호.
			Pause();
			::QueryPerformanceCounter(&t_pause);
		}

		//audio pin 도 동일한 정공법 적용 (별도 streaming thread).
		if (m_pSource->audio_stream())
			m_pSource->audio_stream()->on_seek_flush(rtStart);
		::QueryPerformanceCounter(&t_audio);

		auto us = [&](const LARGE_INTEGER& a, const LARGE_INTEGER& b) -> long long
			{ return (b.QuadPart - a.QuadPart) * 1000000LL / pf.QuadPart; };
		logWrite(_T("[ffi/src/seek_prof] decoder.seek=%lld beginflush=%lld stop=%lld endflush=%lld pause=%lld audioflush=%lld | total=%lldus thr=%d"),
			us(t_enter, t_seek), us(t_seek, t_bf), us(t_bf, t_stop), us(t_stop, t_ef),
			us(t_ef, t_pause), us(t_pause, t_audio), us(t_enter, t_audio), thr ? 1 : 0);

		//logWrite(_T("[ffi/src] on_change_start → seek %.0fms + %s (NewSegment 0..%lld)"),
			//pos_ms,
			//thr ? _T("Stop/Flush/Pause cycle") : _T("pre-streaming defer"),
			//(long long)rtStop);
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
		//logWrite(_T("[ffi/src] video stream thread create (segment_stop=%lld)"),
			//(long long)m_pending_segment_stop);
		return NOERROR;
	}

	HRESULT CFFiVideoStream::OnThreadDestroy()
	{
		//logWrite(_T("[ffi/src] video stream thread destroy"));
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
		int aligned_w = (w + 127) & ~127;	//128 byte alignment — DecideBufferSize / FillBuffer 와 일치.
		double fps = dec.frame_rate();
		if (fps <= 0.0)
			fps = 30.0;

		//NV12 media type 구성. MPC-VR / EVR / VMR9 모두 NV12 받음.
		VIDEOINFOHEADER* pvi = (VIDEOINFOHEADER*)pMediaType->AllocFormatBuffer(sizeof(VIDEOINFOHEADER));
		if (!pvi)
			return E_OUTOFMEMORY;
		ZeroMemory(pvi, sizeof(VIDEOINFOHEADER));

		pvi->bmiHeader.biSize		 = sizeof(BITMAPINFOHEADER);
		pvi->bmiHeader.biWidth		 = aligned_w;	//stride. rcSource 가 valid 영역.
		pvi->bmiHeader.biHeight		 = h;
		pvi->bmiHeader.biPlanes		 = 1;
		pvi->bmiHeader.biBitCount	 = 12;			//NV12 = 12 bpp
		pvi->bmiHeader.biCompression = MAKEFOURCC('N','V','1','2');
		pvi->bmiHeader.biSizeImage	 = aligned_w * h * 3 / 2;

		pvi->AvgTimePerFrame = (REFERENCE_TIME)(10000000.0 / fps);	 //100ns 단위
		pvi->rcSource.right	 = w;	//valid 영역만 (padded stride 가 아닌 실제 표시 영역).
		pvi->rcSource.bottom = h;
		pvi->rcTarget		 = pvi->rcSource;
		pvi->dwBitRate		 = aligned_w * h * 3 / 2 * 8 * (DWORD)fps;
		pvi->dwBitErrorRate	 = 0;

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

		if (*pmt->Type()		!= MEDIATYPE_Video) return E_INVALIDARG;
		if (*pmt->FormatType()	!= FORMAT_VideoInfo) return E_INVALIDARG;

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
					;//logWrite(_T("[ffi/src/video/diag] GetDeliveryBuffer wait %llums"), t_gdb_ms);

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
						;//logWrite(_T("[ffi/src/video/diag] Deliver wait %llums"), t_dlv_ms);

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
			//logWrite(_T("[ffi/src/video/diag] FillBuffer first entry +%llums after seek"),
				//m_seek_t0_ms ? (now_ms - m_seek_t0_ms) : 0ULL);
		}
		else if (m_fb_last_entry_ms != 0)
		{
			ULONGLONG gap_ms = now_ms - m_fb_last_entry_ms;
			if (gap_ms > 100)
				;//logWrite(_T("[ffi/src/video/diag] FillBuffer entry gap %llums (call #%d since seek)"),
					//gap_ms, m_fb_count_since_seek);
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
				seg_rt = dec.segment_rt();	 //generation 과 동기.
				seek_target_pts = (video_tb.num > 0 && video_tb.den > 0) ?
					av_rescale_q(seg_rt, AVRational{1, 10000000}, video_tb) : 0;

				//pre-target skip 분기 — 옵션 환경설정→재생→"키 프레임 단위로 이동(정확하지 않음)".
				//[옵션 ON]: decoder 가 이미 forward keyframe(pts>=target) 부터 전달 → 여기선 skip 없이 첫 frame emit.
				//[옵션 OFF]: pre-target skip 으로 정확 target 위치 + budget exceeded fallback.
				const bool keyframe_mode = m_kf_mode_this_seek;	//on_change_start 가 이 seek 시점에 고정한 스냅샷.

				if (!keyframe_mode && !m_budget_exceeded_this_seek &&
					frame->pts != AV_NOPTS_VALUE && frame->pts < seek_target_pts)
				{
					//[옵션 OFF: 정확 위치, 약간 딜레이]
					//pre-target skip — target 도달 frame 부터 emit. budget exceeded fallback 으로 응답시간 상한.
					++skipped;

					//[latency budget — PotPlayer 식 keyframe-first display]
					//seek_t0 부터 누적 wall clock 이 budget 초과 시 정확 위치 포기, 현재 frame 그대로 emit.
					//flag set 후엔 그 seek 동안 skip 재진입 X — 다음 frame 들은 정상 forward 재생.
					//단 frame step 은 정밀 1프레임이 본질 — budget 적용 시 target 직전 임의 중간 프레임을 emit 해
					//화면이 튀므로(sparse keyframe 미디어), frame step seek 에선 budget 비활성 = 항상 target 까지 skip.
					const ULONGLONG SKIP_LATENCY_BUDGET_MS = 150;
					if (!m_frame_step_this_seek && m_seek_t0_ms != 0 &&
						(GetTickCount64() - m_seek_t0_ms) >= SKIP_LATENCY_BUDGET_MS)
					{
						//logWrite(_T("[ffi/src/video/diag] budget exceeded skip=%d frame_pts=%lld target=%lld elapsed=%llums → emit + skip disable"),
							//skipped, (long long)frame->pts, (long long)seek_target_pts,
							//GetTickCount64() - m_seek_t0_ms);
						m_budget_exceeded_this_seek = true;
						break;
					}

					av_frame_free(&frame);
					continue;
				}
				if (skipped > 0)
					;//logWrite(_T("[ffi/src/video/diag] pre-target skip=%d frame_pts=%lld target_pts=%lld"),
						//skipped, (long long)frame->pts, (long long)seek_target_pts);
				break;
			}
			if (!dec.is_running())
				return S_FALSE;
			//EOS — decoder 가 av_read_frame EOF + queue 비면 stream 끝. S_FALSE → DeliverEndOfStream → EC_COMPLETE.
			if (dec.is_eof() && dec.video_queue_size() == 0)
			{
				//logWrite(_T("[ffi/src/video/diag] EOS reached — DeliverEndOfStream"));
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
				logWrite(_T("[ffi/src/video/diag] FillBuffer waiting %dms queue empty (freeze 진단)"), wait_v_ms);
		}
		if (wait_v_ms >= 100)
			;//logWrite(_T("[ffi/src/video/diag] wait=%dms frame_pts=%lld seg_rt=%lld"),
				//wait_v_ms, (long long)frame->pts, (long long)seg_rt);

		BYTE* pData = NULL;
		HRESULT hr = pSample->GetPointer(&pData);
		if (FAILED(hr) || !pData)
		{
			av_frame_free(&frame);
			return hr;
		}

		int w = dec.video_width();
		int h = dec.video_height();
		int aligned_w = (w + 127) & ~127;	//DecideBufferSize 와 같은 alignment.
		long buffer_size = pSample->GetSize();
		long required = aligned_w * h * 3 / 2;
		if (buffer_size < required)
		{
			av_frame_free(&frame);
			return E_FAIL;
		}

		//fast path — frame 이 이미 NV12 (HW transferred 등) 면 swscale 우회, 직접 copy.
		//매 frame swscale alloc/free 가 thread-unsafe 또는 무거워 crash/freeze 원인.
		//단, frame 크기가 negotiated 크기(w,h)와 같을 때만. 가변 해상도 스트림(인트로 큰 해상도 → 본편 작은 해상도)
		//에서 다른 크기 frame 은 아래 swscale 로 negotiated 크기에 맞춰 스케일해야 stride 가 안 어긋난다.
		if ((AVPixelFormat)frame->format == AV_PIX_FMT_NV12 && frame->width == w && frame->height == h)
		{
			//dst stride = aligned_w. src 의 valid 영역은 처음 w byte, 나머지는 padding (0 또는 stale 무관).
			uint8_t* dst_y	= pData;
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
			int		 dst_strides[4] = { aligned_w, aligned_w, 0, 0 };

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
		//unreliable_video_pts(인덱스 없는 미종료 파일): video pts 가 비선형/엉터리라 frame_pts_rt - video_first 가
		//garbage rtStart 가 됨 → 재생 폭주/정지. sample_count 기반(아래 else)으로 강제해 segment 0 부터 fps 로 균일 진행.
		if (frame->pts != AV_NOPTS_VALUE && !dec.unreliable_video_pts())
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

		//A/V stretch — audio 가 pts 보다 ~9% 많은 샘플을 자연재생(무리샘플)하면 ~stretch 배 느려진다.
		//video 의 segment-local rtStart 을 같은 비율로 늘려 그 느린 오디오에 시점 정렬(둘 다 ~0.91× → 싱크).
		//audio 가 측정 전(처음 ~0.7s)이거나 정상 파일이면 stretch≈0/1.0 → 무변경.
		double av_stretch = m_pSource->av_stretch();
		if (av_stretch > 1.005)	//정상 파일은 ~1.000(미측정 0 포함) → 미적용. 0.5% 초과(잉여샘플 파일)만 stretch.
		{
			rtStart = (REFERENCE_TIME)((double)rtStart * av_stretch);
			rtStop	= (REFERENCE_TIME)((double)rtStop  * av_stretch);
		}

		//rate scaling — graph IMediaSeeking::SetRate → ChangeRate 콜백을 통해 source 의 m_playback_rate 갱신.
		const double rate = m_pSource->playback_rate();
		if (rate > 0.0 && rate != 1.0)
		{
			rtStart = (REFERENCE_TIME)((double)rtStart / rate);
			rtStop	= (REFERENCE_TIME)((double)rtStop  / rate);
		}

		pSample->SetTime(&rtStart, &rtStop);
		m_last_rtStart = rtStart;	//GetCurrentPosition 응답값.

		//get_track_pos 의 source — 원본 video frame PTS (ms). seek/rate 무관, 화면 frame 시점 직접.
		if (frame->pts != AV_NOPTS_VALUE && m_pSource)
		{
			AVRational video_tb = m_pSource->decoder().video_time_base();
			if (video_tb.num > 0 && video_tb.den > 0)
			{
				int64_t pts_ms = av_rescale_q(frame->pts, video_tb, AVRational{1, 1000});
				m_last_emitted_pts_ms.store(pts_ms);
			}
		}

		//(측정) A/V drift — video 타임라인(실제 video pts 기반 rtStart). audio 의 sample_count·pts 타임라인과 비교해
		//어느 클럭이 진짜 콘텐츠 시간인지 확정. v_rtStart_ms = segment-local(seek 기준 0), v_pts_ms = 절대 미디어 pts.
		{
			static thread_local int s_vfill = 0;
			if ((++s_vfill % 120) == 0)
				logWrite(_T("[ffi/src/avsync/v] v_rtStart_ms=%lld v_pts_ms=%lld sc=%lld"),
					(long long)(m_last_rtStart / 10000), (long long)m_last_emitted_pts_ms.load(),
					(long long)m_sample_count);
		}

		//flush 후 첫 sample 에 Discontinuity flag — renderer 의 internal state reset 알림.
		//누락 시 MPCVR 가 이전 segment continuation 으로 처리 → Receive 가 stuck.
		if (m_need_discontinuity)
		{
			pSample->SetDiscontinuity(TRUE);
			m_need_discontinuity = false;
		}

		++m_sample_count;

		//[render 진단] 샘플 timing vs 그래프 stream clock. NewSegment(0,end) 이라 renderer 는 sample.rtStart 시점에 표시.
		//stream clock(StreamTime)과 rtStart 가 크게 어긋나면 renderer 가 frame 을 미래로 보고 hold / 과거로 보고 drop
		//→ 화면이 1fps/정지. 검은 미디어 디버깅용 — seek 직후 첫 5 frame + 이후 60 frame 마다.
		if (m_sample_count <= 5 || m_sample_count % 60 == 0)
		{
			CRefTime stream_t;
			REFERENCE_TIME clk = (m_pFilter && m_pFilter->StreamTime(stream_t) == S_OK) ? (REFERENCE_TIME)stream_t : -1;
			//logWrite(_T("[ffi/src/render] frame[%lld] rtStart=%lldms rtStop=%lldms seg=%lldms streamClock=%lldms (rtStart-clk=%lldms) key=%d"),
				//m_sample_count,
				//(long long)(rtStart / 10000), (long long)(rtStop / 10000),
				//(long long)(m_segment_start / 10000),
				//(long long)(clk < 0 ? -1 : clk / 10000),
				//(long long)(clk < 0 ? 0 : (rtStart - clk) / 10000),
				//(int)((frame->flags & AV_FRAME_FLAG_KEY) ? 1 : 0));
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
			m_out_channels	  = 2;	 //강제 stereo downmix.

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
		CAutoLock lock(&m_filter_cs);	//다른 스레드의 init/release/use 와 직렬화 (use-after-free 크래시 방지).
		release_audio_filter();

		if (m_out_sample_rate <= 0 || m_out_channels <= 0)
			return false;

		//rate=1.0 은 atempo 가 pass-through 일 뿐 — graph 생성 자체 skip.
		//FillBuffer (line 1003) 의 if (m_filter_graph && ...) 가드가 NULL 시 swr 출력을 그대로 deliver.
		//on_seek_flush 의 매 seek 마다 ~33ms (avfilter_graph_alloc + filter create + link + config) 소비를
		//rate=1.0 (디폴트) 에선 완전 제거. release_audio_filter() 가 이미 호출돼 overflow / pointers 모두 NULL.
		if (rate == 1.0)
		{
			m_filter_rate = 1.0;
			//logWrite(_T("[ffi/src/audio/filter] rate=1.0 → graph skip (swr passthrough)"));
			return true;
		}

		m_filter_graph = avfilter_graph_alloc();
		if (!m_filter_graph)
			return false;

		const AVFilter* abuffer	   = avfilter_get_by_name("abuffer");
		const AVFilter* atempo	   = avfilter_get_by_name("atempo");
		const AVFilter* abuffersink = avfilter_get_by_name("abuffersink");
		if (!abuffer || !atempo || !abuffersink)
		{
			//logWrite(_T("[ffi/src/audio/filter] filter lookup fail abuffer=%p atempo=%p abuffersink=%p"),
				//abuffer, atempo, abuffersink);
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
		if (hr < 0) { /*logWrite(_T("[ffi/src/audio/filter] abuffer create fail hr=%d"), hr);*/ release_audio_filter(); return false; }

		//atempo chain — 단일 instance valid range [0.5, 2.0] 를 벗어나는 rate 도
		//atempo=A,atempo=B,... 직렬 (곱이 rate) 로 처리. 0.1~4.0 까지 audio 도 pitch-preserve rate scale.
		//r<0.5: 0.5 push 하며 r/=0.5 반복, 마지막 [0.5,1.0] 의 r 한 번 더.
		//r>2.0: 2.0 push 하며 r/=2.0 반복, 마지막 [1.0,2.0] 의 r 한 번 더.
		double r = rate;
		if (r < 0.01) r = 0.01;	  //safety bound — 0 또는 negative 방어.
		std::vector<double> tempos;
		if (r < 0.5)
		{
			while (r < 0.5) { tempos.push_back(0.5); r /= 0.5; }
			tempos.push_back(r);
		}
		else if (r > 2.0)
		{
			while (r > 2.0) { tempos.push_back(2.0); r /= 2.0; }
			tempos.push_back(r);
		}
		else
		{
			tempos.push_back(r);
		}

		std::vector<AVFilterContext*> atempo_chain;
		for (size_t k = 0; k < tempos.size(); ++k)
		{
			char tempo_args[32] = {0};
			_snprintf_s(tempo_args, _TRUNCATE, "tempo=%.6f", tempos[k]);
			char name[32] = {0};
			_snprintf_s(name, _TRUNCATE, "atempo%zu", k);
			AVFilterContext* ctx = nullptr;
			hr = avfilter_graph_create_filter(&ctx, atempo, name,
				tempo_args, NULL, m_filter_graph);
			if (hr < 0)
			{
				//logWrite(_T("[ffi/src/audio/filter] atempo[%zu]=%.3f create fail hr=%d"), k, tempos[k], hr);
				release_audio_filter();
				return false;
			}
			atempo_chain.push_back(ctx);
		}
		m_filter_atempo = atempo_chain.empty() ? nullptr : atempo_chain.front();

		hr = avfilter_graph_create_filter(&m_filter_sink, abuffersink, "out",
			NULL, NULL, m_filter_graph);
		if (hr < 0) { /*logWrite(_T("[ffi/src/audio/filter] abuffersink create fail hr=%d"), hr);*/ release_audio_filter(); return false; }

		//sink 의 출력 sample format 강제 S16 — swr 와 일치.
		//av_opt_set_int_list 가 내부에서 deprecated av_int_list_length_for_size 호출 — C4996 회피.
		const AVSampleFormat out_fmts[] = { AV_SAMPLE_FMT_S16, AV_SAMPLE_FMT_NONE };
#pragma warning(push)
#pragma warning(disable: 4996)
		av_opt_set_int_list(m_filter_sink, "sample_fmts", out_fmts, AV_SAMPLE_FMT_NONE, AV_OPT_SEARCH_CHILDREN);
#pragma warning(pop)

		//link: src → atempo_chain[0] → atempo_chain[1] → ... → sink
		hr = avfilter_link(m_filter_src, 0, atempo_chain.front(), 0);
		if (hr < 0) { /*logWrite(_T("[ffi/src/audio/filter] link src->atempo[0] fail hr=%d"), hr);*/ release_audio_filter(); return false; }
		for (size_t k = 0; k + 1 < atempo_chain.size(); ++k)
		{
			hr = avfilter_link(atempo_chain[k], 0, atempo_chain[k + 1], 0);
			if (hr < 0) { /*logWrite(_T("[ffi/src/audio/filter] link atempo[%zu]->atempo[%zu] fail hr=%d"), k, k+1, hr);*/ release_audio_filter(); return false; }
		}
		hr = avfilter_link(atempo_chain.back(), 0, m_filter_sink, 0);
		if (hr < 0) { /*logWrite(_T("[ffi/src/audio/filter] link atempo->sink fail hr=%d"), hr);*/ release_audio_filter(); return false; }

		hr = avfilter_graph_config(m_filter_graph, NULL);
		if (hr < 0) { /*logWrite(_T("[ffi/src/audio/filter] graph_config fail hr=%d"), hr);*/ release_audio_filter(); return false; }

		m_filter_rate = rate;
		CString chain_desc;
		for (size_t k = 0; k < tempos.size(); ++k)
		{
			CString seg;
			seg.Format(_T("%s%.3f"), k ? _T(",") : _T(""), tempos[k]);
			chain_desc += seg;
		}
		//logWrite(_T("[ffi/src/audio/filter] init OK rate=%.3f chain=[%s] sr=%d ch=%d layout=%S"),
			//m_filter_rate, chain_desc.GetString(), m_out_sample_rate, m_out_channels, layout_desc);
		return true;
	}

	void CFFiAudioStream::release_audio_filter()
	{
		CAutoLock lock(&m_filter_cs);	//init 내부 호출(동일 스레드 재진입 OK) + 타 스레드 use 와 직렬화.
		if (m_filter_graph)
		{
			avfilter_graph_free(&m_filter_graph);
			m_filter_graph = nullptr;
		}
		m_filter_src = nullptr;
		m_filter_atempo = nullptr;
		m_filter_sink = nullptr;
		m_filter_rate = 1.0;
		m_atempo_overflow.clear();

		//rate 변경 graph rebuild 시 audio offset 도 재계산 트리거. 안 하면 직전 segment 의 offset 잔존으로
		//새 atempo sample 분포에 stale baseline 적용 → 점진적 sync 어긋남.
		m_audio_offset_rt = 0;
		m_audio_offset_set = false;
	}

	bool CFFiAudioStream::update_audio_filter_rate(double rate)
	{
		if (rate == m_filter_rate)
			return true;

		//atempo chain 으로 0.1~4.0 범위 audio rate 적용 — init_audio_filter 가 r 분해 후 chain build.
		//rebuild path 유지 — freezing 회피는 dialog 레벨 throttle 로 호출 빈도 자체 제한.
		//logWrite(_T("[ffi/src/audio/filter] tempo %.3f -> %.3f (graph rebuild, chain)"), m_filter_rate, rate);
		return init_audio_filter(rate);
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
			AVCodecContext* actx = nullptr;	  //decoder 의 audio context 접근 — CDecoder 에 getter 없으면 sample_fmt 만 query.
			//sample_fmt 를 raw int 로 받음.
			AVSampleFormat in_fmt = (AVSampleFormat)dec.audio_sample_format();

			m_swr = swr_alloc();

			AVChannelLayout in_layout;
			av_channel_layout_default(&in_layout, dec.audio_channels());

			av_opt_set_chlayout(m_swr, "in_chlayout",	  &in_layout, 0);
			av_opt_set_int	   (m_swr, "in_sample_rate",  dec.audio_sample_rate(), 0);
			av_opt_set_sample_fmt(m_swr, "in_sample_fmt", in_fmt, 0);

			av_opt_set_chlayout(m_swr, "out_chlayout",	  &m_out_chlayout, 0);
			av_opt_set_int	   (m_swr, "out_sample_rate", m_out_sample_rate, 0);
			av_opt_set_sample_fmt(m_swr, "out_sample_fmt", AV_SAMPLE_FMT_S16, 0);

			int hr = swr_init(m_swr);
			av_channel_layout_uninit(&in_layout);
			if (hr < 0)
			{
				//logWrite(_T("[ffi/src/audio] swr_init fail hr=%d"), hr);
				swr_free(&m_swr);
				m_swr = nullptr;
				return E_FAIL;
			}

			//logWrite(_T("[ffi/src/audio] swr ready in_fmt=%d → S16, %dHz ch=%d"),
				//(int)in_fmt, m_out_sample_rate, m_out_channels);
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

		pwfx->wFormatTag	  = WAVE_FORMAT_PCM;
		pwfx->nChannels		  = (WORD)m_out_channels;
		pwfx->nSamplesPerSec  = (DWORD)m_out_sample_rate;
		pwfx->wBitsPerSample  = 16;
		pwfx->nBlockAlign	  = pwfx->nChannels * pwfx->wBitsPerSample / 8;
		pwfx->nAvgBytesPerSec = pwfx->nSamplesPerSec * pwfx->nBlockAlign;
		pwfx->cbSize		  = 0;

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
		if (*pmt->Type()	   != MEDIATYPE_Audio)		 return E_INVALIDARG;
		if (*pmt->Subtype()	   != MEDIASUBTYPE_PCM)		 return E_INVALIDARG;
		if (*pmt->FormatType() != FORMAT_WaveFormatEx)	 return E_INVALIDARG;
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

	//손상 구간의 깨진 audio frame 은 swr_convert 내부에서 null 참조(0xC0000005) 로 크래시할 수 있다.
	//SEH 로 AV 를 잡아 -1 반환(호출측이 무음 처리). __try/__except 는 C++ unwinding 함수에 못 두므로 별도 함수로 분리.
	static int seh_swr_convert(SwrContext* swr, uint8_t** out, int out_count, const uint8_t** in, int in_count)
	{
		__try
		{
			return swr_convert(swr, out, out_count, in, in_count);
		}
		__except (EXCEPTION_EXECUTE_HANDLER)
		{
			return -1;
		}
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
		int total_anchor_wait = 0;	//(freeze 진단) video anchor 대기 누적 — searching 중엔 wait_anchor_ms 가 리셋돼 누적 안 보임.
		while (true)
		{
			frame = dec.pop_audio_frame();
			if (frame)
			{
				//unreliable_video_pts: video pts 가 garbage 라 video anchor 기반 audio skip 이 audio 를 과도 skip → 무음·정지.
				//byte-seek 는 audio·video 가 같은 byte 에 착지해 이미 정렬되므로 anchor 생략. 정상 파일은 do_anchor=true → 기존 동작.
				bool do_anchor = !dec.unreliable_video_pts();

				//video anchor 확인 — set 안 되어 있으면 video first emit 까지 wait.
				//searching(손상 구간 스캔) 중엔 video 가 수 초 뒤 복구되므로 200ms 고정 타임아웃이면 그 전에 garbage audio 를
				//anchor 없이 뿌려 타이밍 오염(무음/2배속). searching 동안엔 타이머를 리셋해 계속 대기 → 복구 시점부터 정렬.
				int64_t video_anchor = do_anchor ? dec.video_first_emit_pts_rt() : LLONG_MIN;
				bool searching = dec.is_searching();
				if (do_anchor && video_anchor == LLONG_MIN && (wait_anchor_ms < 200 || searching))
				{
					//video first emit 아직 안 됨 — frame 다시 큐 앞에 못 넣음. discard + wait.
					av_frame_free(&frame);
					//seek/stop 중이면 anchor-wait 를 즉시 중단 — 안 그러면 다음 seek 의 Stop() 이 이 wait(최대 200ms,
					//searching 중 무한)에 걸려 수 ms 블록(parked=2). 아래 audio_queue wait 와 동일한 abort 패턴.
					Command pending;
					if (CheckRequest(&pending))
						return S_FALSE;
					Sleep(5);
					total_anchor_wait += 5;
					if (total_anchor_wait == 500 || total_anchor_wait == 2000 || total_anchor_wait == 5000)
						logWrite(_T("[ffi/src/audio/diag] anchor wait %dms (video first emit 대기, searching=%d) (freeze 진단)"),
							total_anchor_wait, searching ? 1 : 0);
					if (searching)
						wait_anchor_ms = 0;	//스캔 중 — 타이머 리셋. 스캔 종료(첫 video 디코드) 후 200ms grace(decode→emit gap 흡수).
					else
						wait_anchor_ms += 5;
					continue;
				}
				//anchor skip — 항상 적용. audio first emit 의 미디어 시점 = video first emit 의 미디어 시점 강제.
				//video / audio 가 각자 first emit 시점 부터 segment-local 0 인 시간계라 정확한 시점 정렬 필요.
				//audio_sync delay 적용 — audio 가 참조하는 anchor 를 -delay 만큼 이동 (헤더 주석의 "anchor shift").
				//delay<0(audio 빠르게) → anchor 가 뒤로 가 audio 가 더 앞 content 부터 시작 → audio 가 video 보다 앞섬.
				//rtStart 직접 shift 는 negative→DSound 즉시표시로 무효라, anchor 이동으로 rtStart 는 0 부터 유지.
				int64_t sync_delay = m_pSource ? m_pSource->audio_sync_delay_rt() : 0;
				int64_t anchor_rt = video_anchor - sync_delay;
				if (do_anchor && video_anchor != LLONG_MIN && audio_tb.num > 0 && audio_tb.den > 0)
				{
					int64_t anchor_pts = av_rescale_q(anchor_rt,
						AVRational{1, 10000000}, audio_tb);
					if (frame->pts != AV_NOPTS_VALUE && frame->pts < anchor_pts)
					{
						++audio_skipped;
						av_frame_free(&frame);
						continue;
					}
				}
				//NOPTS audio (frame->pts 없음) — 위 anchor pts 비교가 무효라 audio_sync delay 가 적용 안 됨.
				//delay<0(빠르게): segment 앞부분을 |delay| 만큼 sample-count 로 skip → audio 가 video 보다 앞선 content 부터 시작.
				//frame 재생길이(nb_samples/sample_rate)를 누적해 |delay| 도달까지 frame drop. delay>=0(느리게)은 rtStart 에서 처리.
				if (do_anchor && frame->pts == AV_NOPTS_VALUE && sync_delay < 0
					&& frame->sample_rate > 0 && m_audio_sync_skipped_rt < -sync_delay)
				{
					m_audio_sync_skipped_rt += (int64_t)frame->nb_samples * 10000000LL / frame->sample_rate;
					++audio_skipped;
					av_frame_free(&frame);
					continue;
				}
				if (audio_skipped > 0)
					;//logWrite(_T("[ffi/src/audio/diag] anchor skip=%d frame_pts=%lld video_anchor_rt=%lld sync_delay_rt=%lld nopts_skip_rt=%lld(target=%lld)"),
						//audio_skipped, (long long)frame->pts, (long long)video_anchor, (long long)sync_delay,
						//(long long)m_audio_sync_skipped_rt, (long long)(sync_delay < 0 ? -sync_delay : 0));

				//(측정) seek 직후 첫 10 frame raw — nb_samples / frame 자체 sr / ctx sr / pts / timebase.
				//pts delta 와 nb_samples 로 "실제 미디어-pts 1초당 샘플수" 산출 → swr in_rate 교정값 확정.
				{
					static thread_local int s_raw = 0;
					if (s_raw < 10)
					{
						++s_raw;
						logWrite(_T("[ffi/src/avsync/raw] #%d nb_samples=%d frame_sr=%d ctx_sr=%d pts=%lld tb=%d/%d"),
							s_raw, frame->nb_samples, frame->sample_rate, m_out_sample_rate,
							(long long)frame->pts, audio_tb.num, audio_tb.den);
					}
				}

				//get_track_pos 의 source — 원본 미디어 시점 (audio decoder input PTS). rate 무관.
				if (frame->pts != AV_NOPTS_VALUE && audio_tb.num > 0 && audio_tb.den > 0)
				{
					int64_t pts_ms = av_rescale_q(frame->pts, audio_tb, AVRational{1, 1000});
					m_last_input_pts_ms.store(pts_ms);
				}
				//트랙이동 후 오디오 무음(잠깐 안 들림) 측정 — 이 FillBuffer 가 첫 deliverable audio frame 에 닿기까지
				//대기/skip 한 양. 정상 재생 중엔 셋 다 0 이라 로그 안 남고, seek 직후 첫 frame 에만 1 줄.
				//무음 길이 ≈ anchor_wait(video first emit 대기) + queue_wait(audio 큐 refill 대기). audio_skipped 는 over-skip 진단.
				if (total_anchor_wait > 0 || wait_ms > 0 || audio_skipped > 0)
					logWrite(_T("[ffi/src/audio/seekgap] anchor_wait=%dms queue_wait=%dms audio_skipped=%d"),
						total_anchor_wait, wait_ms, audio_skipped);
				break;
			}
			if (!dec.is_running())
				return S_FALSE;
			//EOS — video pin 과 동일. audio queue 도 비면 stream 끝.
			if (dec.is_eof() && dec.audio_queue_size() == 0)
			{
				//logWrite(_T("[ffi/src/audio/diag] EOS reached — DeliverEndOfStream"));
				return S_FALSE;
			}
			//video pin 과 동일 — pending CMD peek 로 wait loop 깨기.
			Command pending;
			if (CheckRequest(&pending))
				return S_FALSE;
			Sleep(2);
			wait_ms += 2;
			if (wait_ms == 500 || wait_ms == 2000 || wait_ms == 5000)
				logWrite(_T("[ffi/src/audio/diag] FillBuffer waiting %dms audio_queue empty (freeze 진단)"), wait_ms);
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
		int max_out_samples = buffer_size / (m_out_channels * 2);	//S16 = 2 bytes/sample
		uint8_t* out_planes[1] = { pData };

		//A/V stretch (누적 연속 보정) — 이 파일은 audio 의 "pts 1초당 실제 샘플수"가 reported sample_rate(48000)보다
		//~9% 많다(주기적 짧은 pts). 오디오는 *리샘플하지 않고* 모든 샘플을 48000 으로 재생해야 음정이 자연스럽다
		//(PotPlayer 동일) → 자연 재생이 ~stretch 배 느려진다(0.91×). video 를 그만큼 늦춰 A/V 를 맞춘다.
		//stretch = 누적(sample_count 재생시간 / apts 콘텐츠시간) = audio 가 실제로 늘어진 비율. *누적값* 이라
		//run 마다 흔들리는 단발측정과 달리 수렴·자기보정 → 긴 파일에서도 drift 없음. 정상 파일은 ≈1.0 → 미적용(회귀 0).
		if (m_avsync_anchor_apts_rt != LLONG_MIN && frame->pts != AV_NOPTS_VALUE && !dec.unreliable_video_pts()
			&& audio_tb.num > 0 && audio_tb.den > 0 && m_sample_count > 0
			&& m_pSource && m_pSource->playback_rate() == 1.0)	//배속 중엔 atempo 가 sample_count 변형 → 측정 오염. 정상속도만.
		{
			int64_t cur_pts_rt = av_rescale_q(frame->pts, audio_tb, AVRational{1, 10000000});
			int64_t span_rt	   = cur_pts_rt - m_avsync_anchor_apts_rt;
			if (span_rt >= 10000000 && m_pSource)	//1s 이상 누적 후부터 안정. 그 전엔 av_stretch=0 → video 무변경.
			{
				//누적 비율을 *항상* 갱신 → 정상 파일은 ~1.000 으로 수렴하며 계속 추종(일시 transient 에 stuck 안 됨).
				//video 측 게이트(av_stretch>1.005)가 적용 여부 결정 → 정상 파일(~1.000)은 video 영원히 무변경.
				double stretch = (double)m_sample_count * 10000000.0 / ((double)m_out_sample_rate * (double)span_rt);
				m_pSource->set_av_stretch(stretch);
				static thread_local int s_stretch_log = 0;
				if ((s_stretch_log++ % 200) == 0)
					logWrite(_T("[ffi/src/avsync] video_stretch=%.4f (cumulative, sc=%lld span_ms=%lld)"),
						stretch, (long long)m_sample_count, (long long)(span_rt / 10000));
			}
		}

		//손상 구간 깨진 frame 방어 — SEH 로 swr_convert 의 AV 를 잡고, 실패(-1) 면 무음(0 sample)으로 처리해
		//스트림을 끊지 않는다(E_FAIL = DeliverEndOfStream → 재생 중단). garbage 구간 무음 후 정상 구간 자동 복구.
		int out_samples = (frame->nb_samples > 0)
			? seh_swr_convert(m_swr, out_planes, max_out_samples, (const uint8_t**)frame->extended_data, frame->nb_samples)
			: 0;
		if (out_samples < 0)
			out_samples = 0;

		//filter graph 재빌드(rate 변경) + 사용(buffersrc/buffersink) 구간을 lock — UI 스레드의 seek-init 과
		//동시 접근 시 use-after-free 방지. 이 구간엔 early return 이 없어 manual Lock/Unlock 안전.
		m_filter_cs.Lock();

		//atempo time-stretch — rate != 1.0 일 때 PCM sample count 를 1/rate 로 줄여 graph clock 가속 + pitch 유지.
		//rate 변경 감지는 graph 존재가 아니라 rate 자체로 게이팅해야 함 — rate=1.0 은 graph 를 생성하지 않으므로
		//(init_audio_filter 의 1.0 skip), 게이트가 m_filter_graph 면 1.0→N 전환 시 NULL 이라 영영 build 못 해
		//오디오가 배속에 무반응. update_audio_filter_rate → init_audio_filter 가 rate!=1.0 일 때 graph 를 build,
		//N→1.0 복귀 시엔 release 후 NULL 유지(1.0 절약 유지). 실제 atempo *적용* 은 아래 블록의 graph!=NULL 가드가 담당.
		if (out_samples > 0)
		{
			double cur_rate = m_pSource ? m_pSource->playback_rate() : 1.0;
			if (cur_rate != m_filter_rate)
				update_audio_filter_rate(cur_rate);
		}
		if (m_filter_graph && m_filter_src && m_filter_sink && out_samples > 0)
		{
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

			int total_bytes = 0;

			//이전 FillBuffer 에서 pData 못 들어간 atempo 잔여분 먼저 비움. 100% sample 보존.
			if (!m_atempo_overflow.empty())
			{
				size_t copy = m_atempo_overflow.size();
				if (copy > (size_t)buffer_size) copy = (size_t)buffer_size;
				memcpy(pData, m_atempo_overflow.data(), copy);
				total_bytes = (int)copy;
				m_atempo_overflow.erase(m_atempo_overflow.begin(), m_atempo_overflow.begin() + copy);
			}

			//buffersink 에서 pull → pData 에 copy. 못 들어가는 부분은 overflow vector 에.
			const int frame_align = m_out_channels * 2;
			const size_t overflow_cap = 1024 * 1024;   //1MB 상한 — atempo 가 폭주해도 unbounded growth 방어.
			while (total_bytes < buffer_size && m_atempo_overflow.size() < overflow_cap)
			{
				AVFrame* out_frame = av_frame_alloc();
				if (!out_frame) break;
				int hr_pull = av_buffersink_get_frame(m_filter_sink, out_frame);
				if (hr_pull < 0)
				{
					av_frame_free(&out_frame);
					break;
				}
				int out_bytes = out_frame->nb_samples * frame_align;
				if (out_bytes <= 0)
				{
					av_frame_free(&out_frame);
					break;	 //비어있는 frame — 무한루프 방어.
				}
				int space = buffer_size - total_bytes;
				if (out_bytes <= space)
				{
					memcpy(pData + total_bytes, out_frame->data[0], out_bytes);
					total_bytes += out_bytes;
				}
				else
				{
					//partial: pData 에 들어가는 만큼 (frame 경계 맞춰) copy, 나머지 overflow 로.
					int copy_bytes = (space / frame_align) * frame_align;
					if (copy_bytes > 0)
					{
						memcpy(pData + total_bytes, out_frame->data[0], copy_bytes);
						total_bytes += copy_bytes;
					}
					int remain = out_bytes - copy_bytes;
					if (remain > 0)
						m_atempo_overflow.insert(m_atempo_overflow.end(), out_frame->data[0] + copy_bytes, out_frame->data[0] + out_bytes);
					av_frame_free(&out_frame);
					break;
				}
				av_frame_free(&out_frame);
			}
			out_samples = total_bytes / frame_align;
		}

		m_filter_cs.Unlock();

		int bytes_written = out_samples * m_out_channels * 2;
		pSample->SetActualDataLength(bytes_written);
		pSample->SetSyncPoint(TRUE);

		//[diag] FillBuffer 호출 카운트 + 100 호출마다 amplitude 출력. seek 회귀 진단.
		static thread_local int s_fill_count = 0;
		++s_fill_count;
		int seekgap_left = m_seekgap_remaining.load();
		bool log_diag = (s_fill_count == 1 || (s_fill_count % 100) == 0);
		int16_t seekgap_mx = 0;
		if (log_diag || seekgap_left > 0)
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
			seekgap_mx = mx;
			if (log_diag)
				;//logWrite(_T("[ffi/src/audio/diag] fill#%d samples=%d ch=%d nb=%d fmt=%d max_abs=%d sc=%lld"),
					//s_fill_count, out_samples, m_out_channels, frame->nb_samples, (int)frame->format,
					//(int)mx, (long long)m_sample_count);
		}

		//timing — anchor offset 을 *첫 emit 에서 한 번만* video_first 와 정렬해 잡고, rtStart 는 sample_count 로
		//등간격 누적한다(매끄러운 timestamp). per-frame frame->pts 를 직접 rtStart 로 쓰면, wmapro 처럼 packet
		//하나가 여러 frame 으로 디코드되어 그 frame 들이 *같은 pts* 를 공유하는 코덱에서 rtStart 가 "뭉침→점프"로
		//불규칙해져 reference clock 이 흔들리고 video 가 끊긴다(2026-06-12 실측: rtStart 56×8→416×8). atempo 활성
		//시에도 동일 경로 — sample_count 가 atempo 후 sample 수라 자동 1/rate scale.
		REFERENCE_TIME rtStart;
		if (m_filter_graph || (frame->pts != AV_NOPTS_VALUE && !dec.unreliable_video_pts()))
		{
			if (!m_audio_offset_set && dec.unreliable_video_pts())
			{
				//unreliable_video_pts: video_first(garbage) 기준 offset 은 음수 rtStart→폭주. offset=0 → sample_count 만으로 진행.
				m_audio_offset_rt = 0;
				m_audio_offset_set = true;
			}
			if (!m_audio_offset_set)
			{
				int64_t video_first = dec.video_first_emit_pts_rt();
				//video_first 미설정 시 set 보류 — audio thread 가 video pin 첫 emit 전에 진입하면
				//offset=0 으로 영구 박혀 sync 어긋남. 다음 frame 에서 재시도.
				if (video_first != LLONG_MIN)
				{
					int64_t audio_pts_rt = (frame->pts != AV_NOPTS_VALUE) ?
						av_rescale_q(frame->pts, audio_tb, AVRational{1, 10000000}) : 0;
					//audio_sync delay — anchor 를 -delay 이동 (skip threshold 와 동일). 첫 emit content 가
					//video_first - delay 라 m_audio_offset_rt ≈ 0 으로 수렴 → rtStart 는 sample_count(0 부터) 유지.
					int64_t sync_delay = m_pSource ? m_pSource->audio_sync_delay_rt() : 0;
					m_audio_offset_rt = audio_pts_rt - (video_first - sync_delay);
					m_audio_offset_set = true;
					m_avsync_anchor_apts_rt = audio_pts_rt;	//(측정) drift 기준 — 첫 emit 실제 audio PTS.
				}
			}
			rtStart = m_audio_offset_rt +
				(REFERENCE_TIME)((double)m_sample_count * 10000000.0 / m_out_sample_rate);
		}
		else
		{
			//unreliable_video_pts: video_first(garbage)에 의존하면 rtStart 이 음수가 돼 렌더러가 audio 를 폭주 덤프(33배속).
			//sample_count 기반(0 부터 균일)으로 video 와 동일 시간계 → A/V 1배속 정상.
			rtStart = (REFERENCE_TIME)((double)m_sample_count * 10000000.0 / m_out_sample_rate);
			//NOPTS audio_sync delay>0(느리게): schedule 을 +delay 미뤄 audio 가 video 보다 뒤. delay<0(빠르게)은
			//위 sample-count skip 으로 처리(rtStart 0 부터 유지). unreliable_video_pts(33배속 fix) 경로는 안 건드림.
			if (frame->pts == AV_NOPTS_VALUE && !dec.unreliable_video_pts())
			{
				int64_t sync_delay = m_pSource ? m_pSource->audio_sync_delay_rt() : 0;
				if (sync_delay > 0)
				{
					rtStart += sync_delay;
					if (m_seekgap_remaining.load() > 0)
						;//logWrite(_T("[ffi/src/audio/sync] NOPTS slower: rtStart += delay %lldms → rtStart_ms=%lld"),
							//(long long)(sync_delay / 10000), (long long)(rtStart / 10000));
				}
			}
		}
		m_sample_count += out_samples;
		REFERENCE_TIME rtStop = rtStart + (REFERENCE_TIME)((double)out_samples * 10000000.0 / m_out_sample_rate);

		//(측정) A/V drift — sample_count 진행 vs 실제 audio PTS 진행. 둘이 벌어지면 audio 가 video 대비 빠름/느림.
		//sc_ms = sample_count 기반(=rtStart 의 offset 제외분), apts_ms = 실제 audio PTS 가 anchor 이후 진행한 시간.
		//diff(=apts-sc) 가 0 근처면 정상, 선형 증가/감소면 그 부호·기울기가 drift 방향·속도.
		if ((s_fill_count % 200) == 0 && m_avsync_anchor_apts_rt != LLONG_MIN
			&& frame->pts != AV_NOPTS_VALUE && audio_tb.num > 0 && audio_tb.den > 0)
		{
			int64_t cur_apts_rt = av_rescale_q(frame->pts, audio_tb, AVRational{1, 10000000});
			long long sc_ms = (long long)((rtStart - m_audio_offset_rt) / 10000);
			long long apts_ms = (long long)((cur_apts_rt - m_avsync_anchor_apts_rt) / 10000);
			long long a_pts_abs_ms = (long long)av_rescale_q(frame->pts, audio_tb, AVRational{1, 1000});
			logWrite(_T("[ffi/src/avsync] sc_ms=%lld apts_ms=%lld diff(apts-sc)=%lldms a_pts_abs_ms=%lld sr=%d sc=%lld"),
				sc_ms, apts_ms, apts_ms - sc_ms, a_pts_abs_ms, m_out_sample_rate, (long long)m_sample_count);
		}

		pSample->SetTime(&rtStart, &rtStop);

		if (m_need_discontinuity)
		{
			pSample->SetDiscontinuity(TRUE);
			m_need_discontinuity = false;
		}

		//[seekgap] seek resume(t0) 이후 첫 15 fill — delivery 지연(+ms), 실제 진폭(max_abs),
		//샘플 스케줄 시각(rtStart_ms). rtStart_ms 가 양수로 크면 DSound 가 그만큼 audio 표시를 미뤄 무음 발생.
		if (seekgap_left > 0)
		{
			LARGE_INTEGER qf, qn;
			::QueryPerformanceFrequency(&qf);
			::QueryPerformanceCounter(&qn);
			long long t0 = m_seekgap_qpc.load();
			long long ms = t0 ? (qn.QuadPart - t0) * 1000LL / qf.QuadPart : -1;
			logWrite(_T("[ffi/src/audio/seekgap] +%lldms fill_after_seek=%d max_abs=%d rtStart_ms=%lld samples=%d sc=%lld"),
				ms, (16 - seekgap_left), (int)seekgap_mx, (long long)(rtStart / 10000), out_samples, (long long)m_sample_count);
			m_seekgap_remaining.store(seekgap_left - 1);
		}

		av_frame_free(&frame);
		return NOERROR;
	}

	//(측정) stock CSourceStream::DoBufferProcessingLoop 과 로직 동일 — m_audio_loop_state 만 단계별 기록.
	//on_seek_flush 가 Stop 직전 이 값을 읽어 잔존 ~5.9ms 가 GetDeliveryBuffer vs Deliver 중 어디서 park 인지 확정.
	HRESULT CFFiAudioStream::DoBufferProcessingLoop()
	{
		Command com;
		OnThreadStartPlay();
		do
		{
			while (!CheckRequest(&com))
			{
				IMediaSample* pSample;

				m_audio_loop_state.store(1);
				HRESULT hr = GetDeliveryBuffer(&pSample, NULL, NULL, 0);

				if (FAILED(hr))
				{
					m_audio_loop_state.store(0);
					Sleep(1);
					continue;
				}

				m_audio_loop_state.store(2);
				hr = FillBuffer(pSample);

				if (hr == S_OK)
				{
					m_audio_loop_state.store(3);
					hr = Deliver(pSample);
					m_audio_loop_state.store(0);

					pSample->Release();
					if (hr != S_OK)
						return S_OK;
				}
				else if (hr == S_FALSE)
				{
					m_audio_loop_state.store(0);
					pSample->Release();
					DeliverEndOfStream();
					return S_OK;
				}
				else
				{
					m_audio_loop_state.store(0);
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

	void CFFiAudioStream::on_seek_flush(REFERENCE_TIME rtStart)
	{
		REFERENCE_TIME rtStop = m_pSource ?
			(REFERENCE_TIME)(m_pSource->decoder().duration_ms() * 10000.0) : 0;
		//state 갱신은 ThreadExists 무관. play(Running) 첫 thread create 시 OnThreadStartPlay 가 이 값을 사용.
		m_pending_segment_stop = rtStop;
		//주의: sample_count / offset / skip 누적 reset 은 *여기서 하면 안 된다*. 아래 Stop() 으로 audio FillBuffer
		//thread 가 멈춘 뒤에 reset 한다. 일찍 reset 하면 Stop 대기(수~수십 ms) 동안 *아직 도는* FillBuffer 가
		//m_sample_count 를 0 부터 다시 증가시켜(leak), 재시작 첫 frame 의 rtStart 가 video segment(0)와 어긋난다.
		//Stop 대기시간이 seek 마다 달라 leak 량도 가변 → seek 마다 가변 desync (특히 NOPTS audio — rtStart 이
		//frame->pts 가 아니라 sample_count 기반이라 이 leak 이 그대로 audio 지연으로 나타남).

		//seek 위치 ms 로 m_last_input_pts_ms 우선 set — 다음 FillBuffer 가 실제 frame pts 로 덮어쓸 때까지
		//get_track_pos 가 stale (직전 미디어 시점) 반환하지 않도록.
		m_last_input_pts_ms.store(rtStart / 10000);	  //REFERENCE_TIME 100ns → ms.

		//[seek_prof] on_seek_flush 가 ~157ms — initfilter(atempo 재생성) vs Stop(오디오 FillBuffer 종료 대기) 중 어느쪽인지 측정.
		LARGE_INTEGER apf, a0, a1, a2, a3, a4, a5;
		::QueryPerformanceFrequency(&apf);
		::QueryPerformanceCounter(&a0);

		//atempo graph 재생성 — internal latency 잔류 제거 (LAV path 의 SCAudioTimeStretch NewSegment 처리와 동일).
		//현재 rate 유지 — m_filter_rate 가 직전 rate (init_audio_filter 가 그 rate 로 atempo 재init).
		if (m_filter_graph)
			init_audio_filter(m_filter_rate);
		::QueryPerformanceCounter(&a1);

		//streaming 중인 경우만 Stop/Flush/Pause cycle.
		a2 = a3 = a4 = a1;
		bool athr = ThreadExists();
		int parked = m_audio_loop_state.load();	//(측정) Stop 전 streaming thread 가 park 된 단계.
		if (athr)
		{
			DeliverBeginFlush();
			::QueryPerformanceCounter(&a2);
			Stop();
			::QueryPerformanceCounter(&a3);
		}
		//thread 가 멈춘 직후(재시작=Pause 전) — race 없는 유일한 reset 시점. athr=false 면 도는 thread 자체가 없어 안전.
		m_sample_count = 0;
		m_audio_offset_rt = 0;
		m_audio_offset_set = false;	  //다음 FillBuffer 첫 frame 시 video_first_emit_pts_rt 와 비교 후 set.
		m_avsync_anchor_apts_rt = LLONG_MIN;	//(측정) drift anchor — seek 마다 재설정.
		m_audio_sync_skipped_rt = 0;  //NOPTS audio_sync 빠르게-skip 누적 — 새 segment 시작이므로 재계산.
		//logWrite(_T("[ffi/src/audio/sync] on_seek_flush reset sample_count=0 (athr=%d, post-Stop)"), athr ? 1 : 0);
		if (athr)
		{
			DeliverEndFlush();
			Pause();
			::QueryPerformanceCounter(&a4);
		}
		a5 = a4;

		auto aus = [&](const LARGE_INTEGER& x, const LARGE_INTEGER& y) -> long long
			{ return (y.QuadPart - x.QuadPart) * 1000000LL / apf.QuadPart; };
		logWrite(_T("[ffi/src/audio/seek_prof] initfilter=%lld beginflush=%lld stop=%lld endflush+pause=%lld | total=%lldus thr=%d parked=%d(1=GDB,2=Fill,3=Deliver)"),
			aus(a0, a1), aus(a1, a2), aus(a2, a3), aus(a3, a4), aus(a0, a5), athr ? 1 : 0, parked);

		//[seekgap diag] resume 시점(t0)부터 첫 15 fill 의 delivery 타이밍 측정 시작.
		m_seekgap_qpc.store(a5.QuadPart);
		m_seekgap_remaining.store(15);
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

	int64_t CFFiSource::audio_current_pts_ms() const
	{
		//이름은 audio 지만 실제 source 는 *video pin 의 마지막 emit frame PTS* 우선.
		//	- video frame 의 원본 PTS = 화면에 보이는 frame 시점 = .smi 자막 timing reference (영상 기준 작성).
		//	- audio PTS 기반은 seek 시 audio 와 video 의 first-frame PTS 차이로 mismatch (1.6초 등) 발생.
		//	- video pin 없거나 첫 emit 전 → audio fallback. 둘 다 없으면 -1.
		//unreliable_video_pts(인덱스 없는 미종료 파일): video pts(비선형 garbage)도, audio pts(byte-seek 후 AVI PCM
		//절대 sample 위치 상실 → ~0 으로 리셋)도 못 믿는다. 위치 = seek 목표(m_segment_start) + segment-local 재생량
		//(last_rtStart). GetCurrentPosition 과 동일 식 — garbage pts 무관, byte-seek 후에도 정확.
		if (m_decoder.unreliable_video_pts())
		{
			if (m_pVideoStream)
				return (m_pVideoStream->m_segment_start + m_pVideoStream->last_rtStart()) / 10000;
			return -1;
		}

		//정상 파일: 기존대로 video frame pts 우선 (자막 timing reference), 없으면 audio.
		//단 kf_skip 로 손상 구간 건너뛰는 중이면 스캔 위치(전진)와 비교해 큰 값 — 스캔 중 트랙바가 복구 지점까지
		//전진하는 모습을 보이고(멈춰 보여 freeze 오인 방지), 재생 재개 후엔 last_emitted 가 추월해 자연 연결(뒤로 튐 없음).
		int64_t scan = m_decoder.kf_skip_scan_pos_ms();
		if (m_pVideoStream)
		{
			int64_t v = m_pVideoStream->last_emitted_pts_ms();
			int64_t best = (scan > v) ? scan : v;
			if (best >= 0)
				return best;
		}
		else if (scan >= 0)
			return scan;
		if (m_pAudioStream)
			return m_pAudioStream->last_input_pts_ms();
		return -1;
	}

	HRESULT CFFiSource::open_file(const wchar_t* utf16_path)
	{
		if (!utf16_path)
			return E_INVALIDARG;

		if (!m_decoder.open(utf16_path))
		{
			//logWrite(_T("[ffi/src] open_file decoder.open fail"));
			return E_FAIL;
		}

		//Video pin 생성. CSourceStream ctor 가 AddPin 자동.
		HRESULT hr = S_OK;
		m_pVideoStream = new CFFiVideoStream(&hr, this, L"Video");
		if (!m_pVideoStream || FAILED(hr))
		{
			//logWrite(_T("[ffi/src] video stream create fail hr=0x%08x"), hr);
			return FAILED(hr) ? hr : E_OUTOFMEMORY;
		}

		//Audio pin — decoder 에 audio stream 있으면 생성.
		if (m_decoder.has_audio())
		{
			HRESULT hr_a = S_OK;
			m_pAudioStream = new CFFiAudioStream(&hr_a, this, L"Audio");
			if (!m_pAudioStream || FAILED(hr_a))
			{
				//logWrite(_T("[ffi/src] audio stream create fail hr=0x%08x"), hr_a);
				//audio 실패해도 video 만으로 계속.
				m_pAudioStream = nullptr;
			}
		}

		//logWrite(_T("[ffi/src] open_file OK %dx%d fps=%.2f duration=%.0fms audio=%d"),
			//m_decoder.video_width(), m_decoder.video_height(),
			//m_decoder.frame_rate(), m_decoder.duration_ms(),
			//(int)m_decoder.has_audio());

		return S_OK;
	}
}
