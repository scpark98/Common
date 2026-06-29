#pragma once

/*
* ffi::CFFiSource — custom DirectShow source filter (LAV Splitter + LAV Video/Audio Decoder 통합 대체).
*
*  - Phase 3 본 작업. internal FFmpeg (CDecoder) 으로 demux+decode 한 결과를 DirectShow 의 IMediaSample 로 변환해
*	 후단 (MPC-VR + DSound + SC Audio chain + DirectVobSub) 로 push.
*  - LAV Splitter 의 SetPositions 동기 블로킹 (25-604ms UI freeze) 회피가 본 목적.
*	 이 filter 가 IMediaSeeking::SetPositions 받으면 CDecoder::seek() 에 비동기 위임 → UI 즉시 반환.
*
*  Phase 3a: video pin only. NV12 출력.
*  Phase 3b: audio pin 추가.
*  Phase 3c: dshow.cpp 의 load_media 에 LAV ↔ Internal 토글 분기.
*/

#include "../../directx/dshow_baseclasses/streams.h"
#include "ffi_decoder.h"

#include <memory>
#include <vector>
#include <atomic>

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
		CFFiVideoStream*	m_pPin;
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
		HRESULT OnThreadStartPlay() override;	//MS 표준 정공법 — thread restart 직후 NewSegment + discontinuity.
		HRESULT DoBufferProcessingLoop() override;	 //(Z) 측정 — GetDeliveryBuffer / Deliver wait 분리.

		//CFFiSeeking 의 ChangeStart 가 호출 — CDecoder::seek 비동기 위임.
		void				on_change_start(REFERENCE_TIME rtStart);

		REFERENCE_TIME		last_rtStart() const { return m_last_rtStart; }
		int64_t				last_emitted_pts_ms() const { return m_last_emitted_pts_ms.load(); }
		//frame step settle 신호 — *실제* frame emit 마다 ++. seek 의 pre-set(on_change_start)은 미증가.
		//CDShow::step_frame 의 closed-loop backward 가 seek 후 이 값 변화로 새 표시 frame 착지를 확인.
		int64_t				emit_seq() const { return m_emit_seq.load(); }
		CFFiSource*			source() { return m_pSource; }

	private:
		CFFiSource*		m_pSource;
		LONGLONG		m_sample_count = 0;	  //sample 순번. timestamping 에 사용.
		LONGLONG		m_last_rtStart = 0;	  //직전 emit 한 frame 의 rtStart. GetCurrentPosition 의 응답값.
	public:
		REFERENCE_TIME	m_segment_start = 0;  //seek target rt. video/audio sync 의 공유 baseline.

		//(A) FillBuffer entry 측정 — seek 후 첫 호출 latency / 비정상 gap.
		ULONGLONG		m_seek_t0_ms = 0;
		ULONGLONG		m_fb_last_entry_ms = 0;
		int				m_fb_count_since_seek = 0;

		//[latency budget] seek 후 pre-target skip 누적이 budget 초과해 첫 frame emit (keyframe-first fallback) 한 적 있음.
		//true 인 동안 frame->pts < target_pts 조건의 skip 자체 비활성 — 모든 frame 정상 emit (forward 재생).
		//on_change_start 가 매 seek 시 false 로 reset. (옵션 OFF 정확 모드 전용.)
		bool			m_budget_exceeded_this_seek = false;

		//이 seek 의 keyframe 모드 스냅샷 — on_change_start(UI thread, put_CurrentPosition 동기 구간)에서 1회 캡처.
		//FillBuffer emit(pin thread)이 live source flag 대신 이 값을 읽어야 frame step 의 "off→seek→on" 우회가
		//emit 시점의 복원된 flag 에 영향받지 않음 (decoder 측 m_pending_kf_mode 와 동일 취지).
		bool			m_kf_mode_this_seek = true;

		//이 seek 이 frame step(F/D) 인지 — on_change_start 에서 캡처. true 면 FillBuffer 의 latency budget 비활성
		//(정확히 target 프레임까지 skip). budget 이 frame step 에서 target 직전 중간 프레임을 emit 해 화면이 튀던 문제 차단.
		bool			m_frame_step_this_seek = false;

		//flush 후 첫 sample 에 SetDiscontinuity(TRUE) 알림 플래그.
		bool			m_need_discontinuity = false;

		//MS 표준 정공법 — on_change_start 가 저장 → OnThreadStartPlay 가 NewSegment 에 사용.
		REFERENCE_TIME	m_pending_segment_stop = 0;

		//get_track_pos 의 source — 마지막 emit 한 video frame 의 원본 PTS (ms). 화면에 보이는 frame 시점.
		//audio PTS 기반은 seek 시 video keyframe 후진과 mismatch → 컨트롤바 / 자막 timing 이 화면보다 앞섬.
		//video frame 의 원본 PTS = 실제 화면 frame 시점 = .smi 자막 timing reference.
		std::atomic<int64_t> m_last_emitted_pts_ms{ -1 };

		//실제 표시 frame 이 갱신될 때마다 ++ (FillBuffer 의 real emit). seek 의 pre-set 은 미증가 →
		//frame step closed-loop 가 "pre-set target" 과 "실제 착지 frame" 을 구분해 settle 을 판정.
		std::atomic<int64_t> m_emit_seq{ 0 };
	private:

		CCritSec		m_cs_seeking;
		CFFiSeeking*	m_pSeeking = nullptr;	//aggregated seeking object — pin 의 ref count 와 공유.
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
		HRESULT DoBufferProcessingLoop() override;	//(측정) 로직은 stock 과 동일 + park 단계 기록.

		//video pin 의 on_change_start 에서 호출 — flush + new_segment.
		void	on_seek_flush(REFERENCE_TIME rtStart);

		//get_track_pos 의 source — 마지막 emit 한 atempo input frame 의 원본 PTS (ms). rate 무관.
		int64_t last_input_pts_ms() const { return m_last_input_pts_ms.load(); }

		//atempo audio filter graph — rate != 1.0 시 PCM time-stretch (pitch 유지).
		//swr_convert 의 S16 stereo output 을 source 로 받아 atempo filter 통과 → sink 에서 sample count 1/rate 줄어든 PCM 출력.
		//graph 의 audio renderer (DSound) 가 줄어든 sample 양만큼 빨리 처리 → graph clock 가속 → video 도 따라 가속.
		//rate runtime 변경은 avfilter_graph_send_command 로 atempo tempo 갱신 (graph 재생성 불필요).
		bool				init_audio_filter(double rate);
		void				release_audio_filter();
		bool				update_audio_filter_rate(double rate);

	private:
		CFFiSource*		m_pSource;
		LONGLONG		m_sample_count = 0;
		SwrContext*		m_swr = nullptr;
		int				m_out_sample_rate = 0;
		int				m_out_channels = 0;
		AVChannelLayout m_out_chlayout{};
		bool			m_need_discontinuity = false;
		REFERENCE_TIME	m_pending_segment_stop = 0;

		AVFilterGraph*	 m_filter_graph = nullptr;
		AVFilterContext* m_filter_src	= nullptr;	 //abuffer
		AVFilterContext* m_filter_atempo = nullptr;
		AVFilterContext* m_filter_sink	= nullptr;	 //abuffersink
		double			 m_filter_rate	= 1.0;		 //현재 atempo 에 적용된 rate. source 의 playback_rate 와 비교해 달라지면 send_command.

		//filter graph 직렬화 — rate 변경 시 set_playback_rate→flush(seek)→on_seek_flush→init (UI thread) 와
		//FillBuffer→update_audio_filter_rate→init (audio worker thread) 가 동시에 graph 를 release/재빌드/사용 →
		//use-after-free 크래시. init/release/사용을 이 lock 으로 직렬화. CRITICAL_SECTION 이라 동일 스레드 재진입 OK.
		CCritSec		 m_filter_cs;

		//atempo 가 한 FillBuffer 호출 안에서 여러 frame 을 emit. pData 가 한 frame 분량밖에 못 받으면 *나머지 drop* → audio sample 손실 → audio 가 stretch 안 된 듯 들림 + 띡 노이즈.
		//드롭 대신 byte 로 보관 → 다음 FillBuffer pData 첫머리에 우선 copy. 사이즈 상한 (1MB) 으로 unbounded growth 방어.
		std::vector<uint8_t> m_atempo_overflow;

		//audio sync offset — audio first emit 의 미디어 시점이 video first emit 보다 *delta* 만큼 후 인 경우
		//audio sample.rtStart 에 delta 만큼 더해서 audio 표시 시점을 delta 늦춤. 같은 graph_clock 시점에 video / audio
		//의 *미디어 시점이 정렬* → sync 정확.
		REFERENCE_TIME	m_audio_offset_rt = 0;
		bool			m_audio_offset_set = false;

		//NOPTS audio 의 audio_sync delay<0(빠르게) 적용 — frame pts 가 없어 anchor pts 비교(시점 skip)를 못 하므로
		//segment 시작부터 |delay| 만큼의 frame 을 sample-count 로 누적 skip 한다. on_seek_flush 에서 0 reset —
		//audio_sync 변경마다 flush(re-seek) 되므로 매번 재계산. delay>=0(느리게)은 skip 없이 rtStart +delay 로 처리.
		int64_t			m_audio_sync_skipped_rt = 0;

		//실제 *미디어 시점* — audio decoder 가 마지막으로 emit 한 frame 의 원본 PTS (ms).
		//get_track_pos 의 source 로 사용. graph clock 은 wall clock 진행이라 rate 변경 시 미디어 진행과 어긋남.
		//이 값은 atempo input frame 의 pts 라 *원본 미디어 시점 그대로* — rate 무관 정확.
		std::atomic<int64_t> m_last_input_pts_ms{ -1 };

		//(측정) streaming thread 가 현재 어느 단계에 park 됐는지 — 0=기타/wait, 1=GetDeliveryBuffer, 2=FillBuffer, 3=Deliver.
		//on_seek_flush 가 Stop 직전 읽어 잔존 ~5.9ms 가 GetDeliveryBuffer vs Deliver 중 어디인지 확정.
		std::atomic<int> m_audio_loop_state{ 0 };

		//(측정) A/V drift — audio rtStart 가 sample_count 누적(=출력 샘플수/샘플레이트)인데 video 는 실제 PTS.
		//실제 audio PTS 진행과 sample_count 진행이 어긋나면 audio 가 점점 빠르거나 느려진다. offset-set 시점의
		//실제 audio PTS(rt)를 anchor 로 저장해, 이후 (실제PTS진행 - sample_count진행) diff 를 주기 로깅한다.
		int64_t			m_avsync_anchor_apts_rt = LLONG_MIN;

		//A/V drift 보정 — 일부 컨테이너는 audio 의 "pts 1초당 실제 샘플수"가 reported sample_rate 와 다르다
		//(주기적 짧은 pts). 재생 시작 첫 ~0.7s 동안 실제 rate(samples/pts-second)를 한 번 측정해 저장하고,
		//이후 고정 비율로 swr 리샘플해 audio 가 pts(=video=real-time)를 따르게 한다. 0=미측정. 미디어 1회만 측정.
		double			m_audio_truerate = 0.0;

		//[seekgap diag] seek 후 첫 N fill 의 실제 delivery 타이밍·진폭 측정 — 오디오 끊김 원인이
		//source 측(무음/지연 delivery)인지 downstream(DSound 재프라임)인지 격리용. on_seek_flush 가 t0/카운트 set.
		std::atomic<long long> m_seekgap_qpc{ 0 };
		std::atomic<int>		m_seekgap_remaining{ 0 };
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
		void		   set_audio_sync_delay_ms(int ms) { m_audio_sync_delay_rt.store((int64_t)ms * 10000LL); }
		int64_t		   audio_sync_delay_rt() const { return m_audio_sync_delay_rt.load(); }

		//CFFiSeeking::ChangeRate 가 m_dRate 를 여기로 propagate. video/audio FillBuffer 가 sample 의 rtStart/rtStop 을
		//1/rate 로 scale → renderer 가 graph clock 같은 시간에 rate 배의 미디어 시간 sample 표시 → 빠르게/느리게 재생.
		//audio 는 sample data 양은 그대로 + duration 만 scale → renderer 가 시간 맞춰 빠르게 재생 (chipmunk 효과, LAV path 동등).
		void		   set_playback_rate(double r) { m_playback_rate.store(r); }
		double		   playback_rate() const { return m_playback_rate.load(); }

		//[seek mode] 환경설정→재생→"키 프레임 단위로 이동(정확하지 않음)" 옵션.
		//true: decoder 가 forward keyframe(pts>=target) 까지 demux-skip → 즉시 emit. 빠름, 5초 step 이 GOP 보다 클 수도.
		//false: pre-target skip 으로 정확 target 위치 (현재 동작 + budget exceeded fallback).
		//source filter (emit 분기) 와 decoder (seek+demux-skip) 양쪽이 같은 flag 필요 → 둘 다 전파.
		void		   set_seek_keyframe_mode(bool seek_keyframe) { m_seek_keyframe_mode.store(seek_keyframe); m_decoder.set_seek_keyframe_mode(seek_keyframe); }
		bool		   seek_keyframe_mode() const { return m_seek_keyframe_mode.load(); }

		//[frame step] step_frame 이 1프레임 seek 직전 set(true), 직후 restore. on_change_start 가 per-seek 캡처해
		//FillBuffer 의 latency budget 을 끔 → sparse keyframe 미디어에서도 정확히 target 프레임까지 디코드(화면 튐 방지).
		void		   set_frame_step_mode(bool b) { m_frame_step_mode.store(b); }
		bool		   frame_step_mode() const { return m_frame_step_mode.load(); }

		//실제 *미디어 시점* (ms) — audio pin 이 마지막 emit 한 atempo input frame 의 원본 PTS.
		//get_track_pos 가 wall clock 기반 graph position 대신 이 값을 반환해 rate 무관 정확 시점.
		//audio pin 없으면 -1.
		int64_t		   audio_current_pts_ms() const;

		//A/V stretch — audio 의 "pts 1초당 실제 샘플수 / reported sample_rate" 비율(≈1.0=정상). 1 보다 크면 이 파일은
		//오디오 샘플이 pts 보다 많아(주기적 짧은 pts) 모든 샘플 자연재생 시 ~ratio 배 느려진다(PotPlayer 와 동일).
		//그 자연 재생에 video 를 맞추려고 video FillBuffer 가 rtStart 을 이 비율로 stretch 한다. audio 가 첫 측정 후 set.
		void		   set_av_stretch(double r) { m_av_stretch.store(r); }
		double		   av_stretch() const { return m_av_stretch.load(); }

	private:
		CDecoder		 m_decoder;
		CFFiVideoStream* m_pVideoStream = nullptr;
		CFFiAudioStream* m_pAudioStream = nullptr;	 //has_audio 일 때만 생성.
		std::atomic<int64_t> m_audio_sync_delay_rt{0};
		std::atomic<double>	 m_playback_rate{1.0};
		std::atomic<bool>	 m_frame_step_mode{false};	//frame step seek 동안만 true (budget 비활성).
		std::atomic<bool>	 m_seek_keyframe_mode{true};	//기본 true — 응답성 우선. UI 옵션 추가 시 reg 에서 load.
		std::atomic<double>	 m_av_stretch{0.0};			//0=미측정(=stretch 없음). >1 이면 video rtStart 을 그만큼 늘림.
	};
}
