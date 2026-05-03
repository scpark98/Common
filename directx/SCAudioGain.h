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

	//override.
	virtual void process_sample(BYTE* buf, long len, const WAVEFORMATEX* wfx) override;

protected:
	std::atomic<float>	m_gain;	//linear multiplier
};
