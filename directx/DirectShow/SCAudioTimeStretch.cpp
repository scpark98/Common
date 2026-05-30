#include "SCAudioTimeStretch.h"

#include "../../log/SCLog/SCLog.h"
#include <vector>

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
	//caller (dialog) 가 0.1~4.0 범위 허용. atempo 단일 instance [0.5, 2.0] 한계는 chain
	//(init_filter_graph) 으로 회피. safety bound 만.
	if (rate < 0.01) rate = 0.01;
	m_pending_rate.store(rate);
	m_pending_rate_changed.store(true);
	logWrite(_T("[time_stretch] set_rate pending=%.3f (anchor reset 트리거)"), rate);
}

int64_t CSCAudioTimeStretch::processed_input_pts_ms() const
{
	//비atomic read — process_one 의 갱신과 race 가능하지만 stale 폭이 한 frame (수 ms) 라 무시.
	//anchor 미설정 (재생 시작 전 / seek 직후) 시 -1 반환 → 호출자가 graph position fallback.
	if (m_filter_sr <= 0 || !m_anchor_set)
		return -1;
	int64_t samples = m_in_pts_samples;	//seek(new_segment) 마다 0 리셋 → seek 이후 누적 input = 미디어 경과(원본 rate).
	//미디어 절대 위치 = seek 목표(base) + 경과. base 미설정(첫 재생, seek 없음)이면 출력 anchor(stream≈0)로 fallback.
	int64_t base = m_seek_base_rt.load();
	REFERENCE_TIME base_rt = (base >= 0) ? base : m_anchor_rt;
	return (base_rt / 10000) + (int64_t)((double)samples * 1000.0 / (double)m_filter_sr);
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

	//atempo chain — 단일 instance [0.5, 2.0] 한계 회피.
	//r<0.5: 0.5 push 하며 r/=0.5 반복, 마지막 [0.5,1.0] 의 r 한 번 더.
	//r>2.0: 2.0 push 하며 r/=2.0 반복, 마지막 [1.0,2.0] 의 r 한 번 더.
	double initial_rate = m_pending_rate.load();
	if (initial_rate < 0.01) initial_rate = 0.01;
	double r = initial_rate;
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
		hr = avfilter_graph_create_filter(&ctx, atempo, name, tempo_args, NULL, m_filter_graph);
		if (hr < 0)
		{
			logWrite(_T("[time_stretch] atempo[%zu]=%.3f create fail hr=%d"), k, tempos[k], hr);
			release_filter_graph();
			return false;
		}
		atempo_chain.push_back(ctx);
	}
	m_filter_atempo = atempo_chain.empty() ? nullptr : atempo_chain.front();

	hr = avfilter_graph_create_filter(&m_filter_sink, abuffersink, "out", NULL, NULL, m_filter_graph);
	if (hr < 0) { logWrite(_T("[time_stretch] abuffersink create fail hr=%d"), hr); release_filter_graph(); return false; }

	//sink 의 출력 sample format 고정 — input 과 동일.
	const AVSampleFormat out_fmts[] = { sfmt, AV_SAMPLE_FMT_NONE };
#pragma warning(push)
#pragma warning(disable: 4996)
	av_opt_set_int_list(m_filter_sink, "sample_fmts", out_fmts, AV_SAMPLE_FMT_NONE, AV_OPT_SEARCH_CHILDREN);
#pragma warning(pop)

	//link: src → atempo_chain[0] → atempo_chain[1] → ... → sink
	hr = avfilter_link(m_filter_src, 0, atempo_chain.front(), 0);
	if (hr < 0) { logWrite(_T("[time_stretch] link src->atempo[0] fail hr=%d"), hr); release_filter_graph(); return false; }
	for (size_t k = 0; k + 1 < atempo_chain.size(); ++k)
	{
		hr = avfilter_link(atempo_chain[k], 0, atempo_chain[k + 1], 0);
		if (hr < 0) { logWrite(_T("[time_stretch] link atempo[%zu]->atempo[%zu] fail hr=%d"), k, k+1, hr); release_filter_graph(); return false; }
	}
	hr = avfilter_link(atempo_chain.back(), 0, m_filter_sink, 0);
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

	CString chain_desc;
	for (size_t k = 0; k < tempos.size(); ++k)
	{
		CString seg;
		seg.Format(_T("%s%.3f"), k ? _T(",") : _T(""), tempos[k]);
		chain_desc += seg;
	}
	logWrite(_T("[time_stretch] graph init OK sr=%d ch=%d bps=%d flt=%d rate=%.3f chain=[%s]"),
		sample_rate, channels, bits_per_sample, (int)is_float, initial_rate, chain_desc.GetString());
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
	if (!m_filter_graph)
		return false;
	if (rate < 0.01) rate = 0.01;
	if (rate == m_filter_rate)
		return true;

	//chain 의 instance 개수가 rate 따라 변하므로 send_command 만으로 불가능 — graph rebuild.
	//(ffi_source_filter 의 update_audio_filter_rate 와 동일 원칙. 호출 빈도는 dialog throttle 로 제한.)
	logWrite(_T("[time_stretch] tempo %.3f -> %.3f (graph rebuild, chain)"), m_filter_rate, rate);
	m_pending_rate.store(rate);
	if (!init_filter_graph(m_filter_sr, m_filter_ch, m_filter_bps, m_filter_flt))
	{
		logWrite(_T("[time_stretch] init_filter_graph fail on rate change"));
		return false;
	}
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
		//m_anchor_rt 는 *출력 sample timestamp 의 baseline* (line: outStart = m_anchor_rt + emitted/sr).
		//graph 의 post-seek stream clock(seek 시 ≈0 기준으로 리셋)과 맞아야 DSound 가 제때 재생한다.
		//→ segment 상대값(in_tStart)을 그대로 써야 함. *미디어 절대 위치가 아님* — 절대 위치로 바꾸면 출력
		//timestamp 가 미래로 찍혀 DSound 가 보류→재생 정지(backpressure)된다. 컨트롤바용 미디어 절대 시각은
		//processed_input_pts_ms 가 m_seek_base + 누적 input 으로 별도 계산한다.
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
		//new segment — atempo graph 자체 재생성 (단순 reset 명령은 6ch / FLT 미디어에서 internal latency 잔류).
		//rate change 분기와 동일 패턴.
		int sr = m_filter_sr;
		int ch = m_filter_ch;
		int bps = m_filter_bps;
		bool flt = m_filter_flt;
		release_filter_graph();
		if (sr > 0 && ch > 0)
			init_filter_graph(sr, ch, bps, flt);
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
