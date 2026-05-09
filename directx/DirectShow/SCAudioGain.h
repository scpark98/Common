#pragma once

//CSCAudioGain — PCM sample 에 linear gain 적용 (volume boost / attenuation).
//
//지원 sample format
//  - PCM 16-bit signed (WAVE_FORMAT_PCM, wBitsPerSample=16)
//  - PCM 32-bit signed (WAVE_FORMAT_PCM / WAVE_FORMAT_EXTENSIBLE, wBitsPerSample=32)
//  - PCM 32-bit float  (WAVE_FORMAT_IEEE_FLOAT, wBitsPerSample=32)
//
//사용
//  CSCAudioGain* gain = new CSCAudioGain();
//  gain->set_gain_db(6.0f);   //+6 dB boost
//  graph.AddFilter(gain, L"Gain");
//  graph.ConnectDirect(audio_decoder_out, gain->FindPin("In"), NULL);
//  graph.ConnectDirect(gain->FindPin("Out"), audio_renderer_in, NULL);

#include "SCAudioTransformFilter.h"

class CSCAudioGain : public CSCAudioTransformFilter
{
public:
	CSCAudioGain();
	virtual ~CSCAudioGain();

	//gain 값 설정 — thread-safe (atomic).
	//1.0 = no change. 2.0 = +6 dB. 0.5 = -6 dB.
	void	set_gain_linear(float gain);
	float	get_gain_linear() { return m_gain.load(); }

	//dB 단위 — +6 dB ≈ 2배 amplitude.
	void	set_gain_db(float db);
	float	get_gain_db();

	//AGC (Automatic Gain Control) — 미디어별 인코딩 레벨 차이 자동 보정.
	//slow-moving RMS 측정 (~1.5초) → target dB 까지 천천히 (~2초 시상수) gain 자동 조정.
	//사용자 m_gain (slider) 위에 곱해지므로 사용자 볼륨 조절은 그대로 유효.
	//매우 조용한 구간 (-50 dB 이하) 에선 update 동결 — silence 에서 noise 증폭 회피.
	void	set_agc_enabled(bool e) { m_agc_enabled.store(e); }
	bool	get_agc_enabled() { return m_agc_enabled.load(); }

	//AGC 목표 RMS (dB FS, 0 = full scale). default -20 dB ~ 일반 broadcast 의 program loudness 근사.
	void	set_agc_target_db(float db) { m_agc_target_db.store(db); }
	float	get_agc_target_db() { return m_agc_target_db.load(); }

	//AGC 최대 boost (dB). 너무 조용한 source 를 amplify 할 때 noise 폭주 방지. default +12 dB.
	void	set_agc_max_boost_db(float db) { m_agc_max_boost_db.store(db); }
	float	get_agc_max_boost_db() { return m_agc_max_boost_db.load(); }

	//AGC 상태 reset — 미디어 open 시 호출. 측정값 초기화 후 처음부터 다시 학습.
	void	reset_agc();

	//override.
	virtual void process_sample(BYTE* buf, long len, const WAVEFORMATEX* wfx) override;

protected:
	std::atomic<float>	m_gain;	//linear multiplier (사용자 volume slider)

	//AGC 설정
	std::atomic<bool>	m_agc_enabled;
	std::atomic<float>	m_agc_target_db;
	std::atomic<float>	m_agc_max_boost_db;

	//AGC 상태 — process_sample 내부 단일 thread 에서만 접근.
	float	m_agc_running_rms_sq;	//EMA of input rms² (normalized to [-1,1])
	float	m_agc_smoothed_gain_db;	//현재 적용 중인 AGC offset (dB)
	DWORD	m_agc_last_sample_rate;
	float	m_agc_rms_alpha;	//RMS EMA 계수 (sample rate 따라 갱신)
	float	m_agc_gain_alpha;	//gain LPF 계수
	bool	m_agc_initialized;	//RMS 첫 측정 완료 (초기 큰 값 회피용)

	void	update_agc_coefs(DWORD sample_rate);
};
