#include "SCAudioGain.h"
#include "../../log/SCLog/SCLog.h"
#include <math.h>
#include <mmreg.h>

CSCAudioGain::CSCAudioGain() : CSCAudioTransformFilter(L"SC Audio Gain"), m_gain(1.0f),
	m_agc_enabled(false), m_agc_target_db(-20.0f), m_agc_max_boost_db(12.0f),
	m_agc_running_rms_sq(0.0f), m_agc_smoothed_gain_db(0.0f),
	m_agc_last_sample_rate(0), m_agc_rms_alpha(0.0f), m_agc_gain_alpha(0.0f),
	m_agc_initialized(false)
{
}

CSCAudioGain::~CSCAudioGain()
{
}

void CSCAudioGain::reset_agc()
{
	m_agc_running_rms_sq = 0.0f;
	m_agc_smoothed_gain_db = 0.0f;
	m_agc_initialized = false;
}

//RMS / gain LPF 계수 — sample rate 변경 시만 재계산.
//RMS tau = 1.5초 (단기 program loudness 근사), gain tau = 2초 (slow convergence — pumping 회피).
void CSCAudioGain::update_agc_coefs(DWORD sample_rate)
{
	if (sample_rate == m_agc_last_sample_rate)
		return;
	m_agc_last_sample_rate = sample_rate;

	//Per-sample EMA: alpha = 1 - exp(-1 / (sr * tau))
	const float rms_tau = 1.5f;
	const float gain_tau = 2.0f;
	m_agc_rms_alpha = 1.0f - (float)exp(-1.0 / ((double)sample_rate * (double)rms_tau));
	m_agc_gain_alpha = 1.0f - (float)exp(-1.0 / ((double)sample_rate * (double)gain_tau));
}

void CSCAudioGain::set_gain_linear(float gain)
{
	if (gain < 0.0f) gain = 0.0f;
	m_gain.store(gain);
}

void CSCAudioGain::set_gain_db(float db)
{
	//linear = 10 ^ (db / 20)
	float linear = (float)pow(10.0, db / 20.0);
	set_gain_linear(linear);
}

float CSCAudioGain::get_gain_db()
{
	float linear = m_gain.load();
	if (linear <= 0.0f) return -100.0f;
	return (float)(20.0 * log10(linear));
}

//Block 의 input sample 들을 [-1,1] normalize 한 RMS² 평균 계산.
//AGC 가 측정 단위로 사용. format 별로 분기.
static float compute_block_rms_sq(const BYTE* buf, long len, const WAVEFORMATEX* wfx, long& out_count)
{
	out_count = 0;
	WORD format_tag = wfx->wFormatTag;
	bool is_float = false;
	if (format_tag == WAVE_FORMAT_EXTENSIBLE && wfx->cbSize >= 22)
	{
		const WAVEFORMATEXTENSIBLE* wfex = (const WAVEFORMATEXTENSIBLE*)wfx;
		if (IsEqualGUID(wfex->SubFormat, KSDATAFORMAT_SUBTYPE_IEEE_FLOAT))
			is_float = true, format_tag = WAVE_FORMAT_IEEE_FLOAT;
		else if (IsEqualGUID(wfex->SubFormat, KSDATAFORMAT_SUBTYPE_PCM))
			format_tag = WAVE_FORMAT_PCM;
	}
	else if (format_tag == WAVE_FORMAT_IEEE_FLOAT)
		is_float = true;

	double sum_sq = 0.0;

	if (format_tag == WAVE_FORMAT_PCM && wfx->wBitsPerSample == 16)
	{
		const short* p = (const short*)buf;
		long count = len / sizeof(short);
		for (long i = 0; i < count; i++)
		{
			float v = (float)p[i] * (1.0f / 32768.0f);
			sum_sq += v * v;
		}
		out_count = count;
	}
	else if (format_tag == WAVE_FORMAT_PCM && wfx->wBitsPerSample == 32)
	{
		const INT32* p = (const INT32*)buf;
		long count = len / sizeof(INT32);
		for (long i = 0; i < count; i++)
		{
			double v = (double)p[i] * (1.0 / 2147483648.0);
			sum_sq += v * v;
		}
		out_count = count;
	}
	else if (format_tag == WAVE_FORMAT_PCM && wfx->wBitsPerSample == 24)
	{
		long count = len / 3;
		const BYTE* p = buf;
		for (long i = 0; i < count; i++, p += 3)
		{
			INT32 s = (INT32)((p[2] << 24) | (p[1] << 16) | (p[0] << 8));
			s >>= 8;
			float v = (float)s * (1.0f / 8388608.0f);
			sum_sq += v * v;
		}
		out_count = count;
	}
	else if (is_float && wfx->wBitsPerSample == 32)
	{
		const float* p = (const float*)buf;
		long count = len / sizeof(float);
		for (long i = 0; i < count; i++)
		{
			float v = p[i];
			sum_sq += v * v;
		}
		out_count = count;
	}
	else if (is_float && wfx->wBitsPerSample == 64)
	{
		const double* p = (const double*)buf;
		long count = len / sizeof(double);
		for (long i = 0; i < count; i++)
			sum_sq += p[i] * p[i];
		out_count = count;
	}

	if (out_count <= 0) return 0.0f;
	return (float)(sum_sq / out_count);
}

//Sample format 별 in-place gain 적용 + clamp.
//WAVEFORMATEXTENSIBLE 의 SubFormat 으로 PCM/Float 구분 (wFormatTag = WAVE_FORMAT_EXTENSIBLE 시).
void CSCAudioGain::process_sample(BYTE* buf, long len, const WAVEFORMATEX* wfx)
{
	if (!buf || len <= 0 || !wfx) return;

	float user_gain = m_gain.load();
	bool agc_on = m_agc_enabled.load();

	//AGC 측정 + smoothed gain 갱신. block 단위 RMS → smoothed gain dB → 사용자 gain 위에 곱셈.
	if (agc_on)
	{
		update_agc_coefs(wfx->nSamplesPerSec);

		long sample_count = 0;
		float block_rms_sq = compute_block_rms_sq(buf, len, wfx, sample_count);

		//매우 조용한 구간 (-70 dB 이하) 은 측정/갱신 skip — silence 에서 noise 증폭 회피.
		if (sample_count > 0 && block_rms_sq > 1e-7f)
		{
			//Per-sample EMA 를 block 단위로 변환: alpha_block = 1 - (1-alpha)^N
			float rms_alpha_block = 1.0f - (float)pow((double)(1.0 - m_agc_rms_alpha), (double)sample_count);
			float gain_alpha_block = 1.0f - (float)pow((double)(1.0 - m_agc_gain_alpha), (double)sample_count);

			if (!m_agc_initialized)
			{
				m_agc_running_rms_sq = block_rms_sq;
				m_agc_initialized = true;
			}
			else
			{
				m_agc_running_rms_sq = (1.0f - rms_alpha_block) * m_agc_running_rms_sq + rms_alpha_block * block_rms_sq;
			}

			//RMS² → dB. 10*log10(rms²) = 20*log10(rms).
			float current_rms_db = 10.0f * log10f(m_agc_running_rms_sq + 1e-10f);
			float target = m_agc_target_db.load();
			float max_boost = m_agc_max_boost_db.load();

			float desired_gain_db = target - current_rms_db;
			if (desired_gain_db < -20.0f) desired_gain_db = -20.0f;
			if (desired_gain_db > max_boost) desired_gain_db = max_boost;

			m_agc_smoothed_gain_db =
				(1.0f - gain_alpha_block) * m_agc_smoothed_gain_db +
				gain_alpha_block * desired_gain_db;
		}
	}

	//최종 적용 gain — 사용자 gain × AGC offset.
	float gain = user_gain;
	if (agc_on)
	{
		float agc_lin = (float)pow(10.0, (double)m_agc_smoothed_gain_db / 20.0);
		gain = user_gain * agc_lin;
	}

	static int s_trace_count = 0;
	if (s_trace_count < 3)
	{
		logWrite(_T("[AudioGain] process_sample #%d len=%ld gain=%.3f tag=0x%04X bps=%d ch=%d"),
			s_trace_count, len, gain, wfx->wFormatTag, wfx->wBitsPerSample, wfx->nChannels);
		s_trace_count++;
	}

	if (gain == 1.0f) return;	//no-op

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

	if (format_tag == WAVE_FORMAT_PCM)
	{
		if (wfx->wBitsPerSample == 16)
		{
			short* p = (short*)buf;
			long count = len / sizeof(short);
			for (long i = 0; i < count; i++)
			{
				int s = (int)((float)p[i] * gain);
				if (s >  32767) s =  32767;
				if (s < -32768) s = -32768;
				p[i] = (short)s;
			}
		}
		else if (wfx->wBitsPerSample == 32)
		{
			INT32* p = (INT32*)buf;
			long count = len / sizeof(INT32);
			for (long i = 0; i < count; i++)
			{
				double s = (double)p[i] * gain;
				if (s >  2147483647.0) s =  2147483647.0;
				if (s < -2147483648.0) s = -2147483648.0;
				p[i] = (INT32)s;
			}
		}
		else if (wfx->wBitsPerSample == 24)
		{
			//24-bit signed — 3 byte little-endian.
			long count = len / 3;
			BYTE* p = buf;
			for (long i = 0; i < count; i++, p += 3)
			{
				INT32 s = (INT32)((p[2] << 24) | (p[1] << 16) | (p[0] << 8));	//sign-extend by shifting up
				s >>= 8;	//restore (arithmetic right shift on signed int)
				double v = (double)s * gain;
				if (v >  8388607.0) v =  8388607.0;
				if (v < -8388608.0) v = -8388608.0;
				INT32 res = (INT32)v;
				p[0] = (BYTE)(res & 0xFF);
				p[1] = (BYTE)((res >> 8) & 0xFF);
				p[2] = (BYTE)((res >> 16) & 0xFF);
			}
		}
	}
	else if (is_float)
	{
		if (wfx->wBitsPerSample == 32)
		{
			float* p = (float*)buf;
			long count = len / sizeof(float);
			for (long i = 0; i < count; i++)
			{
				float s = p[i] * gain;
				if (s >  1.0f) s =  1.0f;
				if (s < -1.0f) s = -1.0f;
				p[i] = s;
			}
		}
		else if (wfx->wBitsPerSample == 64)
		{
			double* p = (double*)buf;
			long count = len / sizeof(double);
			for (long i = 0; i < count; i++)
			{
				double s = p[i] * (double)gain;
				if (s >  1.0) s =  1.0;
				if (s < -1.0) s = -1.0;
				p[i] = s;
			}
		}
	}
	//그 외 unsupported format — 무시 (sample 변경 없이 통과).
}
