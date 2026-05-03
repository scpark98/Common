#include "SCAudioGain.h"
#include "../log/SCLog/SCLog.h"
#include <math.h>
#include <mmreg.h>

CSCAudioGain::CSCAudioGain() : CSCAudioTransformFilter(L"SC Audio Gain"), m_gain(1.0f)
{
}

CSCAudioGain::~CSCAudioGain()
{
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

//Sample format 별 in-place gain 적용 + clamp.
//WAVEFORMATEXTENSIBLE 의 SubFormat 으로 PCM/Float 구분 (wFormatTag = WAVE_FORMAT_EXTENSIBLE 시).
void CSCAudioGain::process_sample(BYTE* buf, long len, const WAVEFORMATEX* wfx)
{
	if (!buf || len <= 0 || !wfx) return;

	float gain = m_gain.load();

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
