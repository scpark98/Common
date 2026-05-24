#include "SCAudioTimeStretch.h"

#include "../../log/SCLog/SCLog.h"

extern "C" {
#include <libavutil/opt.h>
#include <libavutil/samplefmt.h>
#include <libavutil/channel_layout.h>
}

CSCAudioTimeStretch::CSCAudioTimeStretch()
	: CSCAudioTransformFilter(L"SC Audio Time-Stretch")
{
}

CSCAudioTimeStretch::~CSCAudioTimeStretch()
{
	release_filter_graph();
}

void CSCAudioTimeStretch::set_rate(double rate)
{
	if (rate < 0.5) rate = 0.5;
	if (rate > 2.0) rate = 2.0;
	m_pending_rate.store(rate);
	m_pending_rate_changed.store(true);
	logWrite(_T("[time_stretch] set_rate pending=%.3f (anchor reset 트리거)"), rate);
}

bool CSCAudioTimeStretch::init_filter_graph(int sample_rate, int channels, int bits_per_sample, bool is_float)
{
	release_filter_graph();

	if (sample_rate <= 0 || channels <= 0)
		return false;

	m_filter_graph = avfilter_graph_alloc();
	if (!m_filter_graph)
		return false;

	const AVFilter* abuffer     = avfilter_get_by_name("abuffer");
	const AVFilter* atempo      = avfilter_get_by_name("atempo");
	const AVFilter* abuffersink = avfilter_get_by_name("abuffersink");
	if (!abuffer || !atempo || !abuffersink)
	{
		logWrite(_T("[time_stretch] filter lookup fail"));
		release_filter_graph();
		return false;
	}

	AVChannelLayout layout = {};
	av_channel_layout_default(&layout, channels);
	char layout_desc[64] = {0};
	av_channel_layout_describe(&layout, layout_desc, sizeof(layout_desc));

	const AVSampleFormat sfmt = is_float ? AV_SAMPLE_FMT_FLT : AV_SAMPLE_FMT_S16;
	const char* sfmt_name = is_float ? "flt" : "s16";

	char src_args[256] = {0};
	_snprintf_s(src_args, _TRUNCATE,
		"sample_rate=%d:sample_fmt=%s:channel_layout=%s:time_base=1/%d",
		sample_rate, sfmt_name, layout_desc, sample_rate);

	av_channel_layout_uninit(&layout);

	int hr = avfilter_graph_create_filter(&m_filter_src, abuffer, "in", src_args, NULL, m_filter_graph);
	if (hr < 0) { logWrite(_T("[time_stretch] abuffer create fail hr=%d args=%S"), hr, src_args); release_filter_graph(); return false; }

	double initial_rate = m_pending_rate.load();
	if (initial_rate < 0.5) initial_rate = 0.5;
	if (initial_rate > 2.0) initial_rate = 2.0;

	char tempo_args[32] = {0};
	_snprintf_s(tempo_args, _TRUNCATE, "tempo=%.3f", initial_rate);
	hr = avfilter_graph_create_filter(&m_filter_atempo, atempo, "atempo", tempo_args, NULL, m_filter_graph);
	if (hr < 0) { logWrite(_T("[time_stretch] atempo create fail hr=%d"), hr); release_filter_graph(); return false; }

	hr = avfilter_graph_create_filter(&m_filter_sink, abuffersink, "out", NULL, NULL, m_filter_graph);
	if (hr < 0) { logWrite(_T("[time_stretch] abuffersink create fail hr=%d"), hr); release_filter_graph(); return false; }

	//sink 의 출력 sample format 고정 — input 과 동일.
	const AVSampleFormat out_fmts[] = { sfmt, AV_SAMPLE_FMT_NONE };
#pragma warning(push)
#pragma warning(disable: 4996)
	av_opt_set_int_list(m_filter_sink, "sample_fmts", out_fmts, AV_SAMPLE_FMT_NONE, AV_OPT_SEARCH_CHILDREN);
#pragma warning(pop)

	hr = avfilter_link(m_filter_src, 0, m_filter_atempo, 0);
	if (hr < 0) { logWrite(_T("[time_stretch] link src->atempo fail hr=%d"), hr); release_filter_graph(); return false; }
	hr = avfilter_link(m_filter_atempo, 0, m_filter_sink, 0);
	if (hr < 0) { logWrite(_T("[time_stretch] link atempo->sink fail hr=%d"), hr); release_filter_graph(); return false; }

	hr = avfilter_graph_config(m_filter_graph, NULL);
	if (hr < 0) { logWrite(_T("[time_stretch] graph_config fail hr=%d"), hr); release_filter_graph(); return false; }

	m_filter_sr  = sample_rate;
	m_filter_ch  = channels;
	m_filter_bps = bits_per_sample;
	m_filter_flt = is_float;
	m_filter_rate = initial_rate;
	m_in_pts_samples = 0;
	m_anchor_set = false;
	m_emitted_total = 0;

	logWrite(_T("[time_stretch] graph init OK sr=%d ch=%d bps=%d flt=%d rate=%.3f"),
		sample_rate, channels, bits_per_sample, (int)is_float, initial_rate);
	return true;
}

void CSCAudioTimeStretch::release_filter_graph()
{
	if (m_filter_graph)
	{
		avfilter_graph_free(&m_filter_graph);
		m_filter_graph = nullptr;
	}
	m_filter_src = nullptr;
	m_filter_atempo = nullptr;
	m_filter_sink = nullptr;
	m_filter_sr = 0;
	m_filter_ch = 0;
	m_filter_bps = 0;
	m_filter_flt = false;
	m_filter_rate = 1.0;
	m_in_pts_samples = 0;
	m_anchor_set = false;
	m_emitted_total = 0;
}

bool CSCAudioTimeStretch::send_filter_rate(double rate)
{
	if (!m_filter_graph || !m_filter_atempo)
		return false;
	if (rate < 0.5) rate = 0.5;
	if (rate > 2.0) rate = 2.0;
	if (rate == m_filter_rate)
		return true;

	char val[32] = {0};
	_snprintf_s(val, _TRUNCATE, "%.3f", rate);
	char resp[128] = {0};
	int hr = avfilter_graph_send_command(m_filter_graph, "atempo", "tempo", val, resp, sizeof(resp), 0);
	if (hr < 0)
	{
		logWrite(_T("[time_stretch] send_command tempo=%S fail hr=%d resp=%S"), val, hr, resp);
		return false;
	}
	logWrite(_T("[time_stretch] tempo %.3f -> %.3f"), m_filter_rate, rate);
	m_filter_rate = rate;
	return true;
}

void CSCAudioTimeStretch::process_bypass(IMediaSample* pIn)
{
	if (!pIn) return;
	LONGLONG delay = m_delay_100ns.load();
	if (delay != 0)
	{
		REFERENCE_TIME tStart = 0, tStop = 0;
		if (SUCCEEDED(pIn->GetTime(&tStart, &tStop)))
		{
			tStart += delay;
			tStop  += delay;
			pIn->SetTime(&tStart, &tStop);
		}
	}
	if (m_pOutputPin)
		m_pOutputPin->deliver_sample(pIn);
}

void CSCAudioTimeStretch::process_with_atempo(IMediaSample* pIn)
{
	if (!pIn) return;

	const WAVEFORMATEX* pwfx = get_wave_format();
	if (!pwfx)
	{
		process_bypass(pIn);
		return;
	}

	//format 결정 — S16 또는 FLT 만 지원. 그 외는 bypass.
	bool is_float = false;
	if (pwfx->wFormatTag == WAVE_FORMAT_IEEE_FLOAT && pwfx->wBitsPerSample == 32)
		is_float = true;
	else if (pwfx->wFormatTag == WAVE_FORMAT_PCM && pwfx->wBitsPerSample == 16)
		is_float = false;
	else if (pwfx->wFormatTag == WAVE_FORMAT_EXTENSIBLE)
	{
		const WAVEFORMATEXTENSIBLE* wfx_ext = (const WAVEFORMATEXTENSIBLE*)pwfx;
		if (wfx_ext->SubFormat == KSDATAFORMAT_SUBTYPE_IEEE_FLOAT && pwfx->wBitsPerSample == 32)
			is_float = true;
		else if (wfx_ext->SubFormat == KSDATAFORMAT_SUBTYPE_PCM && pwfx->wBitsPerSample == 16)
			is_float = false;
		else
		{
			process_bypass(pIn);
			return;
		}
	}
	else
	{
		process_bypass(pIn);
		return;
	}

	const int channels = pwfx->nChannels;
	const int sample_rate = pwfx->nSamplesPerSec;
	const int bps = pwfx->wBitsPerSample;
	const int bytes_per_sample = (bps / 8) * channels;
	if (bytes_per_sample <= 0)
	{
		process_bypass(pIn);
		return;
	}

	//graph 미초기화 또는 format 변경 시 재초기화.
	if (!m_filter_graph ||
	    m_filter_sr != sample_rate ||
	    m_filter_ch != channels ||
	    m_filter_bps != bps ||
	    m_filter_flt != is_float)
	{
		if (!init_filter_graph(sample_rate, channels, bps, is_float))
		{
			process_bypass(pIn);
			return;
		}
	}

	//pending rate 적용 (다른 thread 가 set_rate 호출 → atomic 으로 전달됨).
	double pending = m_pending_rate.load();
	if (pending != m_filter_rate)
		send_filter_rate(pending);

	BYTE* pInBuf = NULL;
	if (FAILED(pIn->GetPointer(&pInBuf)) || !pInBuf)
		return;
	long in_bytes = pIn->GetActualDataLength();
	if (in_bytes <= 0)
		return;
	int in_samples = in_bytes / bytes_per_sample;
	if (in_samples <= 0)
		return;

	//input AVFrame 만들기.
	AVFrame* in_frame = av_frame_alloc();
	if (!in_frame)
		return;
	in_frame->format = is_float ? AV_SAMPLE_FMT_FLT : AV_SAMPLE_FMT_S16;
	in_frame->sample_rate = sample_rate;
	av_channel_layout_default(&in_frame->ch_layout, channels);
	in_frame->nb_samples = in_samples;
	in_frame->pts = m_in_pts_samples;
	m_in_pts_samples += in_samples;

	if (av_frame_get_buffer(in_frame, 0) < 0)
	{
		av_frame_free(&in_frame);
		return;
	}
	memcpy(in_frame->data[0], pInBuf, in_samples * bytes_per_sample);

	int hr_push = av_buffersrc_add_frame(m_filter_src, in_frame);
	av_frame_free(&in_frame);
	if (hr_push < 0)
	{
		logWrite(_T("[time_stretch] buffersrc_add_frame fail hr=%d"), hr_push);
		return;
	}

	//첫 input 의 timestamp 를 anchor 로 — 이후 output sample 들은 m_emitted_total 누적 기반.
	REFERENCE_TIME in_tStart = 0, in_tStop = 0;
	bool has_in_time = SUCCEEDED(pIn->GetTime(&in_tStart, &in_tStop));
	if (has_in_time && !m_anchor_set)
	{
		m_anchor_rt = in_tStart;
		m_anchor_set = true;
	}
	LONGLONG delay = m_delay_100ns.load();

	//downstream allocator 에서 output sample 받아서 PCM 채워 deliver.
	IMemAllocator* pAlloc = NULL;
	if (m_pOutputPin && m_pOutputPin->get_downstream_input())
	{
		m_pOutputPin->get_downstream_input()->GetAllocator(&pAlloc);
	}
	if (!pAlloc)
	{
		logWrite(_T("[time_stretch] no downstream allocator — bypass"));
		return;
	}

	//atempo output frame pull loop — 한 input 이 여러 output 또는 0 output (누적) 가능.
	//timestamp 의 핵심: m_emitted_total (누적 output sample 수) 은 atempo 가 양을 1/rate 로 줄였으므로
	//*input 의 1/rate 빈도로 진행*. output sample.tStart = anchor + m_emitted_total / sample_rate.
	//audio renderer 가 더 짧은 시간에 다음 sample 의 tStart 도달 → DSound 가 빨리 처리 → graph clock 가속.
	//*추가 /rate 는 적용하지 말 것* — atempo 가 이미 양을 줄여서 자연 1/rate scale 됨.
	while (true)
	{
		AVFrame* out_frame = av_frame_alloc();
		if (!out_frame)
			break;
		int hr_pull = av_buffersink_get_frame(m_filter_sink, out_frame);
		if (hr_pull < 0)
		{
			av_frame_free(&out_frame);
			break;
		}
		int out_samples = out_frame->nb_samples;
		int out_bytes = out_samples * bytes_per_sample;

		IMediaSample* pOut = NULL;
		HRESULT hr_get = pAlloc->GetBuffer(&pOut, NULL, NULL, 0);
		if (FAILED(hr_get) || !pOut)
		{
			av_frame_free(&out_frame);
			break;
		}
		BYTE* pOutBuf = NULL;
		if (SUCCEEDED(pOut->GetPointer(&pOutBuf)) && pOutBuf && out_bytes <= pOut->GetSize())
		{
			memcpy(pOutBuf, out_frame->data[0], out_bytes);
			pOut->SetActualDataLength(out_bytes);
			pOut->SetSyncPoint(TRUE);

			if (m_anchor_set)
			{
				REFERENCE_TIME outStart = m_anchor_rt +
					(REFERENCE_TIME)((double)m_emitted_total * 10000000.0 / sample_rate);
				REFERENCE_TIME outStop = outStart +
					(REFERENCE_TIME)((double)out_samples * 10000000.0 / sample_rate);
				if (delay != 0)
				{
					outStart += delay;
					outStop  += delay;
				}
				pOut->SetTime(&outStart, &outStop);
			}

			m_pOutputPin->deliver_sample(pOut);
		}
		pOut->Release();
		m_emitted_total += out_samples;
		av_frame_free(&out_frame);
	}

	pAlloc->Release();
}

void CSCAudioTimeStretch::process_one(const work_item& w)
{
	if (w.kind == kind_sample && w.sample)
	{
		//rate 변경 직후 — atempo internal buffer/state 완전 초기화 + anchor reset.
		//단순 reset 명령 (avfilter_graph_send_command "reset") 은 6ch / FLT 등 multi-channel 미디어에서 internal
		//latency 가 누적되어 *간헐적* sync 어긋남 발생 (2ch stereo 는 거의 일관). graph 자체 재생성으로 완전 초기화.
		if (m_pending_rate_changed.exchange(false))
		{
			int sr  = m_filter_sr;
			int ch  = m_filter_ch;
			int bps = m_filter_bps;
			bool flt = m_filter_flt;
			release_filter_graph();
			if (sr > 0 && ch > 0)
				init_filter_graph(sr, ch, bps, flt);
			m_anchor_set = false;
			m_emitted_total = 0;
			logWrite(_T("[time_stretch] rate change → graph rebuild sr=%d ch=%d bps=%d flt=%d"),
				sr, ch, bps, (int)flt);
		}

		double rate = m_pending_rate.load();
		if (rate == 1.0 && !m_filter_graph)
		{
			//bypass — atempo 초기화 안 함. 처음부터 rate=1.0 인 경우 overhead 0.
			process_bypass(w.sample);
		}
		else if (rate == 1.0 && m_filter_graph)
		{
			//과거 rate!=1.0 였다가 1.0 복귀 — graph 의 internal buffer 가 누적된 sample 을 drain 해야.
			//일단 atempo 통과 (tempo=1.0 이면 거의 1:1 pass-through).
			process_with_atempo(w.sample);
		}
		else
		{
			process_with_atempo(w.sample);
		}
		w.sample->Release();	//enqueue 시 AddRef 된 ref 해제
	}
	else if (w.kind == kind_new_segment)
	{
		//new segment — atempo internal buffer flush + anchor / emitted 누적 reset.
		if (m_filter_graph && m_filter_src)
		{
			//flush_buffers 가 동기 호출 — 누적 buffer 비움. 다음 frame 부터 새 baseline.
			avfilter_graph_send_command(m_filter_graph, "atempo", "reset", "", NULL, 0, 0);
		}
		m_in_pts_samples = 0;
		m_anchor_set = false;
		m_emitted_total = 0;
		if (m_pOutputPin)
			m_pOutputPin->deliver_new_segment(w.ns_start, w.ns_stop, w.ns_rate);
	}
	else if (w.kind == kind_eos)
	{
		//EOS — atempo 의 internal buffer 에 남은 sample drain.
		if (m_filter_graph && m_filter_src)
		{
			av_buffersrc_add_frame(m_filter_src, NULL);	//NULL frame = EOF 신호

			IMemAllocator* pAlloc = NULL;
			if (m_pOutputPin && m_pOutputPin->get_downstream_input())
				m_pOutputPin->get_downstream_input()->GetAllocator(&pAlloc);

			if (pAlloc)
			{
				const WAVEFORMATEX* pwfx = get_wave_format();
				int bytes_per_sample = pwfx ? (pwfx->wBitsPerSample / 8) * pwfx->nChannels : 4;
				while (true)
				{
					AVFrame* out_frame = av_frame_alloc();
					if (!out_frame) break;
					int hr_pull = av_buffersink_get_frame(m_filter_sink, out_frame);
					if (hr_pull < 0)
					{
						av_frame_free(&out_frame);
						break;
					}
					int out_bytes = out_frame->nb_samples * bytes_per_sample;
					IMediaSample* pOut = NULL;
					if (SUCCEEDED(pAlloc->GetBuffer(&pOut, NULL, NULL, 0)) && pOut)
					{
						BYTE* pOutBuf = NULL;
						if (SUCCEEDED(pOut->GetPointer(&pOutBuf)) && pOutBuf && out_bytes <= pOut->GetSize())
						{
							memcpy(pOutBuf, out_frame->data[0], out_bytes);
							pOut->SetActualDataLength(out_bytes);
							pOut->SetSyncPoint(TRUE);
							m_pOutputPin->deliver_sample(pOut);
						}
						pOut->Release();
					}
					av_frame_free(&out_frame);
				}
				pAlloc->Release();
			}
		}
		if (m_pOutputPin)
			m_pOutputPin->deliver_end_of_stream();
	}
}
