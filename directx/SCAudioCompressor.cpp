#include "SCAudioCompressor.h"
#include "../log/SCLog/SCLog.h"
#include <math.h>
#include <mmreg.h>

CSCAudioCompressor::CSCAudioCompressor()
	: CSCAudioTransformFilter(L"SC Audio Compressor"),
	  m_threshold_db(-20.0f), m_threshold_lin(0.1f),
	  m_ratio(4.0f),
	  m_attack_ms(30.0f), m_release_ms(200.0f),
	  m_makeup_db(0.0f), m_makeup_lin(1.0f),
	  m_enabled(true),
	  m_envelope_linked(0.0f), m_smoothed_gain(1.0f),
	  m_last_sample_rate(0), m_attack_coef(0.0f), m_release_coef(0.0f), m_gain_smooth_coef(0.0f)
{
	recalc_threshold();
	recalc_makeup();
}

CSCAudioCompressor::~CSCAudioCompressor()
{
}

void CSCAudioCompressor::recalc_threshold()
{
	float db = m_threshold_db.load();
	m_threshold_lin.store((float)pow(10.0, db / 20.0));
}

void CSCAudioCompressor::recalc_makeup()
{
	float db = m_makeup_db.load();
	m_makeup_lin.store((float)pow(10.0, db / 20.0));
}

void CSCAudioCompressor::set_threshold_db(float db) { m_threshold_db.store(db); recalc_threshold(); }
void CSCAudioCompressor::set_ratio(float ratio)     { if (ratio < 1.0f) ratio = 1.0f; m_ratio.store(ratio); }
void CSCAudioCompressor::set_attack_ms(float ms)    { if (ms < 0.1f) ms = 0.1f; m_attack_ms.store(ms); m_last_sample_rate = 0; }
void CSCAudioCompressor::set_release_ms(float ms)   { if (ms < 1.0f) ms = 1.0f; m_release_ms.store(ms); m_last_sample_rate = 0; }
void CSCAudioCompressor::set_makeup_db(float db)    { m_makeup_db.store(db); recalc_makeup(); }

void CSCAudioCompressor::update_coefs(DWORD sample_rate)
{
	if (sample_rate == 0) sample_rate = 48000;
	float attack = m_attack_ms.load();
	float release = m_release_ms.load();
	m_attack_coef  = (float)exp(-1.0 / ((double)attack  * sample_rate / 1000.0));
	m_release_coef = (float)exp(-1.0 / ((double)release * sample_rate / 1000.0));
	//Gain LPF — 30ms time constant. zipper / modulation noise 방지용 부드러운 gain 변화.
	m_gain_smooth_coef = (float)exp(-1.0 / (30.0 * sample_rate / 1000.0));
}

void CSCAudioCompressor::process_sample(BYTE* buf, long len, const WAVEFORMATEX* wfx)
{
	if (!m_enabled.load() || !buf || len <= 0 || !wfx) return;

	if (wfx->nSamplesPerSec != m_last_sample_rate)
	{
		update_coefs(wfx->nSamplesPerSec);
		m_last_sample_rate = wfx->nSamplesPerSec;
	}

	float threshold = m_threshold_lin.load();
	float ratio     = m_ratio.load();
	float makeup    = m_makeup_lin.load();

	//진단 — 100 호출마다 input/output max amplitude 측정.
	static long s_call_count = 0;
	s_call_count++;
	bool do_trace = (s_call_count % 100 == 1);

	float max_in = 0.0f;
	if (do_trace)
	{
		WORD ftag = wfx->wFormatTag;
		if (ftag == WAVE_FORMAT_EXTENSIBLE && wfx->cbSize >= 22)
		{
			const WAVEFORMATEXTENSIBLE* wfex = (const WAVEFORMATEXTENSIBLE*)wfx;
			if (IsEqualGUID(wfex->SubFormat, KSDATAFORMAT_SUBTYPE_IEEE_FLOAT)) ftag = WAVE_FORMAT_IEEE_FLOAT;
		}
		if (ftag == WAVE_FORMAT_IEEE_FLOAT && wfx->wBitsPerSample == 32)
		{
			long count = len / sizeof(float);
			float* p = (float*)buf;
			for (long i = 0; i < count; i++)
			{
				float a = p[i] < 0 ? -p[i] : p[i];
				if (a > max_in) max_in = a;
			}
		}
	}
	int   channels  = wfx->nChannels;
	if (channels < 1) channels = 1;
	if (channels > 8) channels = 8;	//envelope 배열 한계

	//format 식별
	WORD format_tag = wfx->wFormatTag;
	bool is_float = false;
	if (format_tag == WAVE_FORMAT_EXTENSIBLE && wfx->cbSize >= 22)
	{
		const WAVEFORMATEXTENSIBLE* wfex = (const WAVEFORMATEXTENSIBLE*)wfx;
		if (IsEqualGUID(wfex->SubFormat, KSDATAFORMAT_SUBTYPE_IEEE_FLOAT))
		{
			is_float = true;
			format_tag = WAVE_FORMAT_IEEE_FLOAT;
		}
		else if (IsEqualGUID(wfex->SubFormat, KSDATAFORMAT_SUBTYPE_PCM))
		{
			format_tag = WAVE_FORMAT_PCM;
		}
	}
	else if (format_tag == WAVE_FORMAT_IEEE_FLOAT)
	{
		is_float = true;
	}

	int bps = wfx->wBitsPerSample;
	int bytes_per_sample = bps / 8;
	if (bytes_per_sample <= 0) return;
	long total_samples = len / bytes_per_sample;
	long frames = total_samples / channels;

	for (long f = 0; f < frames; f++)
	{
		//1. 한 frame 내 모든 채널 sample 을 float 로 read (in-place 위해 일단 버퍼에)
		float fs[8];
		for (int c = 0; c < channels; c++)
		{
			long idx = f * channels + c;
			float s = 0.0f;
			if (is_float)
			{
				if (bps == 32) s = ((float*)buf)[idx];
				else if (bps == 64) s = (float)(((double*)buf)[idx]);
			}
			else if (format_tag == WAVE_FORMAT_PCM)
			{
				if (bps == 16) s = ((short*)buf)[idx] / 32768.0f;
				else if (bps == 32) s = (float)(((INT32*)buf)[idx]) / 2147483648.0f;
				else if (bps == 24)
				{
					BYTE* p = buf + idx * 3;
					INT32 v = (INT32)((p[2] << 24) | (p[1] << 16) | (p[0] << 8));
					v >>= 8;
					s = (float)v / 8388608.0f;
				}
			}
			fs[c] = s;
		}

		//2. channel-linked envelope — 한 frame 내 채널 max 의 abs amplitude 로 envelope 갱신.
		//   모든 채널에 동일 gain 적용해 stereo image 흔들림·channel imbalance 방지.
		float frame_peak = 0.0f;
		for (int c = 0; c < channels; c++)
		{
			float a = fs[c] < 0.0f ? -fs[c] : fs[c];
			if (a > frame_peak) frame_peak = a;
		}
		float env = m_envelope_linked;
		if (frame_peak > env)
			env = m_attack_coef * env + (1.0f - m_attack_coef) * frame_peak;
		else
			env = m_release_coef * env + (1.0f - m_release_coef) * frame_peak;
		m_envelope_linked = env;

		//3. compression gain (target gain).
		float target_gain = 1.0f;
		if (env > threshold && env > 0.0f)
		{
			float compressed = threshold + (env - threshold) / ratio;
			target_gain = compressed / env;
		}

		//4. gain smoothing (LPF, 5ms time constant) — zipper noise 방지.
		m_smoothed_gain = m_gain_smooth_coef * m_smoothed_gain + (1.0f - m_gain_smooth_coef) * target_gain;
		float frame_gain = m_smoothed_gain * makeup;

		//5. write 모든 채널 동일 gain × makeup 적용 + clip
		for (int c = 0; c < channels; c++)
		{
			float s = fs[c] * frame_gain;

			//tanh 는 작은 sample 도 nonlinear distortion 을 만들어 "지글거림" 노이즈 원인.
			//zipper noise 는 이미 m_smoothed_gain LPF 로 처리됨. saturation 만 남기는 hard clamp.
			if (s >  1.0f) s =  1.0f;
			if (s < -1.0f) s = -1.0f;

			long idx = f * channels + c;
			if (is_float)
			{
				if (bps == 32) ((float*)buf)[idx] = s;
				else if (bps == 64) ((double*)buf)[idx] = (double)s;
			}
			else if (format_tag == WAVE_FORMAT_PCM)
			{
				if (bps == 16)
				{
					int v = (int)(s * 32767.0f);
					if (v >  32767) v =  32767;
					if (v < -32768) v = -32768;
					((short*)buf)[idx] = (short)v;
				}
				else if (bps == 32)
				{
					double v = (double)s * 2147483647.0;
					if (v >  2147483647.0) v =  2147483647.0;
					if (v < -2147483648.0) v = -2147483648.0;
					((INT32*)buf)[idx] = (INT32)v;
				}
				else if (bps == 24)
				{
					double v = (double)s * 8388607.0;
					if (v >  8388607.0) v =  8388607.0;
					if (v < -8388608.0) v = -8388608.0;
					INT32 res = (INT32)v;
					BYTE* p = buf + idx * 3;
					p[0] = (BYTE)(res & 0xFF);
					p[1] = (BYTE)((res >> 8) & 0xFF);
					p[2] = (BYTE)((res >> 16) & 0xFF);
				}
			}
		}
	}

	if (do_trace)
	{
		float max_out = 0.0f;
		WORD ftag2 = wfx->wFormatTag;
		if (ftag2 == WAVE_FORMAT_EXTENSIBLE && wfx->cbSize >= 22)
		{
			const WAVEFORMATEXTENSIBLE* wfex = (const WAVEFORMATEXTENSIBLE*)wfx;
			if (IsEqualGUID(wfex->SubFormat, KSDATAFORMAT_SUBTYPE_IEEE_FLOAT)) ftag2 = WAVE_FORMAT_IEEE_FLOAT;
		}
		if (ftag2 == WAVE_FORMAT_IEEE_FLOAT && wfx->wBitsPerSample == 32)
		{
			long count = len / sizeof(float);
			float* p = (float*)buf;
			for (long i = 0; i < count; i++)
			{
				float a = p[i] < 0 ? -p[i] : p[i];
				if (a > max_out) max_out = a;
			}
		}
		logWrite(_T("[AudioComp] call#%ld max_in=%.4f max_out=%.4f env[0]=%.4f makeup=%.3f thr=%.4f"),
			s_call_count, max_in, max_out, m_envelope_linked, makeup, threshold);
	}
}
