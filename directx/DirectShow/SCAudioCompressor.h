#pragma once

//CSCAudioCompressor — Single-band feed-forward dynamic range compressor.
//
//설계
//  Peak follower (attack/release) → over-threshold 시 ratio 압축 → makeup gain → soft clip.
//  Float / 16-bit / 24-bit / 32-bit PCM 지원. Multi-channel 처리 (per-channel envelope).
//
//PotPlayer 처럼 "작은 슬라이더 %로 큰 인지 음량" 효과를 내기 위한 핵심 DSP.
//단순 gain ×N 은 saturation 한계로 amplitude 가 막히는데, compressor 는
//큰 신호 압축 + makeup 으로 평균 loudness 를 키워 인지 음량 향상.

#include "SCAudioTransformFilter.h"

class CSCAudioCompressor : public CSCAudioTransformFilter
{
public:
	CSCAudioCompressor();
	virtual ~CSCAudioCompressor();

	//threshold (dB) 위 신호 압축. default -20 dB
	void	set_threshold_db(float db);
	float	get_threshold_db() { return m_threshold_db.load(); }

	//ratio (N:1). default 4.0
	void	set_ratio(float ratio);
	float	get_ratio() { return m_ratio.load(); }

	//attack / release time (ms). default 5 / 50
	void	set_attack_ms(float ms);
	void	set_release_ms(float ms);

	//makeup gain (dB). 압축 후 평균 amplitude 회복용. default 0 (외부에서 동적 조정)
	void	set_makeup_db(float db);
	float	get_makeup_db() { return m_makeup_db.load(); }

	//enabled. false 면 sample passthrough.
	void	set_enabled(bool e) { m_enabled.store(e); }
	bool	get_enabled() { return m_enabled.load(); }

	virtual void process_sample(BYTE* buf, long len, const WAVEFORMATEX* wfx) override;

protected:
	std::atomic<float>	m_threshold_db;
	std::atomic<float>	m_threshold_lin;	//cached linear (10^(db/20))
	std::atomic<float>	m_ratio;
	std::atomic<float>	m_attack_ms;
	std::atomic<float>	m_release_ms;
	std::atomic<float>	m_makeup_db;
	std::atomic<float>	m_makeup_lin;
	std::atomic<bool>	m_enabled;

	//state — process_sample 내부에서만 (thread 1개에서만 호출)
	float	m_envelope_linked;	//channel-linked peak envelope (모든 채널 max)
	float	m_smoothed_gain;	//LPF 적용된 compression gain — zipper noise 방지
	DWORD	m_last_sample_rate;
	float	m_attack_coef;
	float	m_release_coef;
	float	m_gain_smooth_coef;	//gain LPF 계수

	void	update_coefs(DWORD sample_rate);
	void	recalc_threshold();
	void	recalc_makeup();
};
