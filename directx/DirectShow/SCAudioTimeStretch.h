#pragma once

//CSCAudioTimeStretch — DirectShow audio transform filter. PCM 의 *시간 축* 만 1/rate 로
//stretch/compress, pitch 는 유지. ffmpeg atempo filter graph 를 내부에 보유.
//
//설계 — CSCAudioTransformFilter 상속, process_one override (variable-length output).
//  CSCAudioGain / CSCAudioCompressor 등 기존 in-place 파생과는 다른 모델 — 한 input sample 이
//  여러 output sample 또는 0 output (atempo 내부 buffer 누적 중) 으로 변환됨. base 의
//  process_one 을 override 해 output IMediaSample 을 downstream allocator 에서 새로 할당 후
//  PCM copy + deliver 반복.
//
//rate 변경 — set_rate(rate) 호출. atempo 의 valid range [0.5, 2.0] 으로 clamp.
//  graph 의 audio thread 에서 process_one 진입 시 다음 frame 부터 새 rate 적용 (lock-free,
//  std::atomic 으로 pending_rate 전달 후 process_one 안에서 실제 send_command).
//
//지원 sample format
//  - PCM 16-bit signed (WAVE_FORMAT_PCM, wBitsPerSample=16) — atempo 가 S16 직접 처리.
//  - PCM 32-bit float  (WAVE_FORMAT_IEEE_FLOAT, wBitsPerSample=32) — atempo 가 FLT 직접 처리.
//  - 그 외 format — bypass (input 그대로 deliver, time-stretch 없음).
//
//그래프 통합
//  CSCAudioGain 과 동일 패턴 — graph.AddFilter + ConnectDirect.
//  chain 위치: audio renderer 직전 (chain 마지막). pitch-preserving rate 적용이 chain 의 다른
//  filter (Gain/Compressor) 의 timing 가정에 영향 주지 않도록.

#include "SCAudioTransformFilter.h"

#include <atomic>
#include <mutex>

extern "C" {
#include <libavfilter/avfilter.h>
#include <libavfilter/buffersrc.h>
#include <libavfilter/buffersink.h>
}

class CSCAudioTimeStretch : public CSCAudioTransformFilter
{
public:
	CSCAudioTimeStretch();
	virtual ~CSCAudioTimeStretch();

	//rate 설정 — thread-safe (atomic). atempo valid range [0.5, 2.0] clamp.
	//rate==1.0 면 bypass path (atempo 통과 없이 input 그대로 deliver) — overhead 최소.
	void	set_rate(double rate);
	double	get_rate() const { return m_pending_rate.load(); }

	//get_track_pos 의 source — atempo input 누적 sample + anchor 기반 *원본 미디어 시점* (ms).
	//graph clock (wall clock 기반) 대신 이 값을 쓰면 rate 무관 정확. anchor 미설정 시 -1.
	int64_t	processed_input_pts_ms() const;

	//in-place process_sample 은 미사용. process_one 직접 override.
	virtual void	process_sample(BYTE* /*buf*/, long /*len*/, const WAVEFORMATEX* /*wfx*/) override {}

protected:
	//variable-length output — input PCM 을 atempo 통과 후 output sample 새로 할당해 deliver.
	//한 input 이 0개 또는 여러 output 으로 변환됨 (atempo internal buffer 누적). loop 로 모두 emit.
	virtual void	process_one(const work_item& w) override;

private:
	//atempo filter graph 초기화 — sample_rate/channels/format 기준. set_rate 의 첫 effective rate 로 atempo tempo 설정.
	bool	init_filter_graph(int sample_rate, int channels, int bits_per_sample, bool is_float);
	void	release_filter_graph();
	//send_command 로 atempo tempo 갱신. graph 재생성 없이 runtime 변경.
	bool	send_filter_rate(double rate);

	//input sample 의 PCM 을 atempo 에 push + output 모두 pull 해 downstream deliver.
	//timestamp shift (m_delay_100ns) 도 함께 적용.
	void	process_with_atempo(IMediaSample* pIn);
	//bypass (rate==1.0) — input sample 그대로 deliver. delay shift 만 적용.
	void	process_bypass(IMediaSample* pIn);

	AVFilterGraph*   m_filter_graph = nullptr;
	AVFilterContext* m_filter_src   = nullptr;
	AVFilterContext* m_filter_atempo = nullptr;
	AVFilterContext* m_filter_sink  = nullptr;

	int    m_filter_sr  = 0;	//현재 graph 의 sample rate.
	int    m_filter_ch  = 0;
	int    m_filter_bps = 0;
	bool   m_filter_flt = false;	//true = AV_SAMPLE_FMT_FLT, false = AV_SAMPLE_FMT_S16
	double m_filter_rate = 1.0;	//현재 atempo 의 tempo. process_one (audio thread) 만 접근.

	std::atomic<double> m_pending_rate{1.0};	//set_rate (any thread) → process_one (audio thread) 전달.
	std::atomic<bool>   m_pending_rate_changed{false};	//set_rate 호출 후 process_one 이 atempo reset + anchor reset 트리거.

	//pts 누적 — atempo input frame 의 pts 가 timeline 진행 baseline.
	int64_t m_in_pts_samples = 0;

	//output sample 의 timestamp 계산 baseline. NewSegment 마다 reset.
	//atempo 가 output sample 양 1/rate 로 줄임 → 누적 sample 양 기반 timestamp 가 자연 1/rate 빈도 진행
	//→ audio renderer 가 더 짧은 시간에 다음 sample 요청 → graph clock 가속 → video 도 따라 가속.
	REFERENCE_TIME m_anchor_rt = 0;
	bool           m_anchor_set = false;
	int64_t        m_emitted_total = 0;
};
