#include "ffi_decoder.h"

#include "../../Functions.h"
#include "../../log/SCLog/SCLog.h"

#include <windows.h>
#include <stdlib.h>

extern "C" {
#include <libavutil/hwcontext.h>
#include <libavutil/pixdesc.h>
}

#include <vector>

namespace
{
	//CDecoder::open 에서 ctx->opaque 에 target hw_pix_fmt 저장. callback 이 FFmpeg internal thread 에서 호출돼도
	//ctx 매개변수 통해 안전히 접근 (thread_local 은 다른 thread 에서 안 보임).
	enum AVPixelFormat get_hw_format(AVCodecContext* ctx, const enum AVPixelFormat* pix_fmts)
	{
		AVPixelFormat target = (AVPixelFormat)(intptr_t)ctx->opaque;
		for (const enum AVPixelFormat* p = pix_fmts; *p != AV_PIX_FMT_NONE; ++p)
		{
			if (*p == target)
				return *p;
		}
		//fallback — 첫 format (보통 SW format). decoder open 자체는 성공.
		return pix_fmts[0];
	}

	//stream 에 명시된 비트레이트. codecpar->bit_rate 우선, 0 이면 Matroska "BPS"(+언어 suffix) 메타 태그. 둘 다 없으면 0.
	int64_t stream_known_bit_rate(AVStream* s)
	{
		if (!s)
			return 0;
		if (s->codecpar->bit_rate > 0)
			return s->codecpar->bit_rate;
		AVDictionaryEntry* e = av_dict_get(s->metadata, "BPS", nullptr, AV_DICT_IGNORE_SUFFIX);
		if (e && e->value)
		{
			int64_t b = _atoi64(e->value);
			if (b > 0)
				return b;
		}
		return 0;
	}
}

namespace ffi
{
	CDecoder::CDecoder() {}

	CDecoder::~CDecoder()
	{
		close();
	}

	int CDecoder::avio_read_cb(void* opaque, uint8_t* buf, int buf_size)
	{
		CDecoder* self = (CDecoder*)opaque;
		DWORD read = 0;
		if (!::ReadFile(self->m_file_handle, buf, (DWORD)buf_size, &read, NULL))
			return AVERROR(EIO);
		if (read == 0)
			return AVERROR_EOF;
		return (int)read;
	}

	int64_t CDecoder::avio_seek_cb(void* opaque, int64_t offset, int whence)
	{
		CDecoder* self = (CDecoder*)opaque;
		if (whence == AVSEEK_SIZE)
		{
			LARGE_INTEGER sz;
			if (!::GetFileSizeEx(self->m_file_handle, &sz))
				return AVERROR(EIO);
			return sz.QuadPart;
		}
		DWORD method;
		switch (whence & ~AVSEEK_FORCE)
		{
			case SEEK_CUR: method = FILE_CURRENT; break;
			case SEEK_END: method = FILE_END;	  break;
			default:	   method = FILE_BEGIN;	  break;
		}
		LARGE_INTEGER pos;
		pos.QuadPart = offset;
		LARGE_INTEGER new_pos;
		if (!::SetFilePointerEx(self->m_file_handle, pos, &new_pos, method))
			return AVERROR(EIO);
		return new_pos.QuadPart;
	}

	bool CDecoder::open(const wchar_t* utf16_path)
	{
		if (m_fmt)
			close();
		if (!utf16_path)
			return false;

		m_path = utf16_path;	//백그라운드 duration 스캐너가 2nd context 로 다시 열 때 사용.

		//Win32 CreateFile 로 직접 handle 열기 — FILE_SHARE_DELETE 포함해 외부 rename / move 시
		//sharing violation 회피. avformat 기본 file protocol (_wsopen 기반) 은 FILE_SHARE_DELETE 미설정.
		HANDLE h = ::CreateFileW(utf16_path, GENERIC_READ,
			FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
			NULL, OPEN_EXISTING,
			FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN, NULL);
		if (h == INVALID_HANDLE_VALUE)
		{
			//logWrite(_T("[ffi/dec] CreateFile fail err=%u"), ::GetLastError());
			return false;
		}
		m_file_handle = h;

		//custom AVIOContext — avio_alloc_context 의 buffer 는 av_malloc, avformat 이 내부에서 재할당 가능.
		//close 시 m_avio->buffer 를 av_freep + avio_context_free 필요.
		const int kAvioBufSize = 32 * 1024;
		uint8_t* avio_buf = (uint8_t*)av_malloc(kAvioBufSize);
		if (!avio_buf)
		{
			::CloseHandle(m_file_handle);
			m_file_handle = nullptr;
			return false;
		}
		m_avio = avio_alloc_context(avio_buf, kAvioBufSize, 0 /*write_flag*/,
			this, &CDecoder::avio_read_cb, NULL, &CDecoder::avio_seek_cb);
		if (!m_avio)
		{
			av_free(avio_buf);
			::CloseHandle(m_file_handle);
			m_file_handle = nullptr;
			return false;
		}

		m_fmt = avformat_alloc_context();
		if (!m_fmt)
		{
			av_freep(&m_avio->buffer);
			avio_context_free(&m_avio);
			::CloseHandle(m_file_handle);
			m_file_handle = nullptr;
			return false;
		}
		m_fmt->pb = m_avio;
		m_fmt->flags |= AVFMT_FLAG_CUSTOM_IO;
		//pts 가 없는 packet 에 대해 dts/frame duration 으로 pts 를 생성한다 (AVI 등 미종료 녹화 = pts 없음 / dts=프레임인덱스).
		//이 flag 가 없으면 첫 video packet 의 pts 가 NOPTS → video_has_pts()==false → LAV 로 fallback 하고
		//내장 path 의 현재재생시간/동기가 동작 안 한다. genpts 는 pts 가 이미 있는 정상 파일엔 영향 없다 (누락분만 생성).
		m_fmt->flags |= AVFMT_FLAG_GENPTS;

		//URL=NULL 이면 avformat 이 m_fmt->pb 사용. probe 도 callback 으로 처리.
		int hr = avformat_open_input(&m_fmt, NULL, NULL, NULL);
		if (hr < 0)
		{
			char buf[256];
			//logWrite(_T("[ffi/dec] avformat_open_input fail hr=%d (%hs)"), hr, ffi::err_str(hr, buf, sizeof(buf)));
			//avformat_open_input 실패 시 m_fmt 는 자동 해제 (NULL set). close() 가 m_avio + handle 정리.
			close();
			return false;
		}

		hr = avformat_find_stream_info(m_fmt, NULL);
		if (hr < 0)
		{
			char buf[256];
			//logWrite(_T("[ffi/dec] find_stream_info fail hr=%d (%hs)"), hr, ffi::err_str(hr, buf, sizeof(buf)));
			close();
			return false;
		}

		//video stream 선택 — av_find_best_stream 이 자동으로 가장 적합한 video stream 고름.
		m_video_stream_idx = av_find_best_stream(m_fmt, AVMEDIA_TYPE_VIDEO, -1, -1, NULL, 0);
		if (m_video_stream_idx < 0)
		{
			//logWrite(_T("[ffi/dec] no video stream"));
			close();
			return false;
		}

		//no-PTS 영상 probe — 첫 video packet 의 pts/dts 가 모두 AV_NOPTS_VALUE 면 (일부 AVI 등) 이 영상은
		//timestamp 가 없어 internal path 의 PTS 기반 A/V 동기·seek·컨트롤바가 동작 안 함. 호출측(load_media)이
		//video_has_pts()==false 를 보고 LAV 로 라우팅한다. probe 는 read-only — 몇 개 packet 만 demux 후 처음으로
		//seek-back 하므로 재생에 영향 없음. (valid-PTS 영상은 첫 video packet 에 timestamp 가 있어 영향 없음.)
		{
			AVPacket* probe = av_packet_alloc();
			if (probe)
			{
				int scanned = 0;
				while (scanned < 100 && av_read_frame(m_fmt, probe) >= 0)
				{
					if (probe->stream_index == m_video_stream_idx)
					{
						//frame->pts 는 packet 의 pts 에서 옴 (이 디코더는 best_effort_timestamp 미사용 — 971/1032행).
						//그래서 dts 는 보지 않고 pts 만 본다. AVI 처럼 packet 에 dts 만 있고 pts 가 없으면 frame->pts 가
						//NOPTS 가 되어 internal path 의 PTS 기반 동기가 깨지므로 LAV 로 보내야 함.
						m_video_has_pts = (probe->pts != AV_NOPTS_VALUE);
						av_packet_unref(probe);
						break;
					}
					av_packet_unref(probe);
					++scanned;
				}
				av_packet_free(&probe);
			}
			//probe 로 소비한 packet 위치를 처음으로 되돌림 (재생은 이후 on_change_start/seek 가 위치 설정).
			av_seek_frame(m_fmt, -1, 0, AVSEEK_FLAG_BACKWARD);
			//logWrite(_T("[ffi/dec] video_has_pts=%d"), (int)m_video_has_pts);
		}

		//decoder 준비
		AVCodecParameters* par = m_fmt->streams[m_video_stream_idx]->codecpar;
		const AVCodec* codec = avcodec_find_decoder(par->codec_id);
		if (!codec)
		{
			//logWrite(_T("[ffi/dec] no decoder for codec_id=%d"), (int)par->codec_id);
			close();
			return false;
		}

		m_video_ctx = avcodec_alloc_context3(codec);
		if (!m_video_ctx)
		{
			close();
			return false;
		}

		avcodec_parameters_to_context(m_video_ctx, par);

		//HW 가속 setup — D3D11VA 우선, 그 다음 DXVA2. codec 이 지원하는 첫 HW config 선택.
		//SW keyframe walk decode 의 freeze 해소가 본 목적.
		{
			AVHWDeviceType try_types[2] = { AV_HWDEVICE_TYPE_D3D11VA, AV_HWDEVICE_TYPE_DXVA2 };
			for (int t = 0; t < 2 && m_hw_pix_fmt == AV_PIX_FMT_NONE; ++t)
			{
				AVHWDeviceType type = try_types[t];
				for (int i = 0;; ++i)
				{
					const AVCodecHWConfig* cfg = avcodec_get_hw_config(codec, i);
					if (!cfg) break;
					if ((cfg->methods & AV_CODEC_HW_CONFIG_METHOD_HW_DEVICE_CTX) &&
						cfg->device_type == type)
					{
						m_hw_pix_fmt = cfg->pix_fmt;
						break;
					}
				}
				if (m_hw_pix_fmt != AV_PIX_FMT_NONE)
				{
					AVBufferRef* dev = NULL;
					int dhr = av_hwdevice_ctx_create(&dev, type, NULL, NULL, 0);
					if (dhr >= 0 && dev)
					{
						m_hw_device_ctx = dev;
						m_video_ctx->hw_device_ctx = av_buffer_ref(m_hw_device_ctx);
						m_video_ctx->opaque = (void*)(intptr_t)m_hw_pix_fmt;   //get_hw_format callback 이 ctx->opaque 통해 접근.
						m_video_ctx->get_format = get_hw_format;
						//logWrite(_T("[ffi/dec] HW accel enabled type=%hs hw_pix_fmt=%d"),
							//av_hwdevice_get_type_name(type), (int)m_hw_pix_fmt);
						break;
					}
					else
					{
						//logWrite(_T("[ffi/dec] HW device create fail type=%hs hr=%d — try next"),
							//av_hwdevice_get_type_name(type), dhr);
						m_hw_pix_fmt = AV_PIX_FMT_NONE;
					}
				}
			}
			if (m_hw_pix_fmt == AV_PIX_FMT_NONE)
				;//logWrite(_T("[ffi/dec] no HW accel for codec=%hs — fallback to SW"), codec->name);
		}

		//multi-threaded decode — FFmpeg 의 thread_type 자동 (frame + slice). PotPlayer 의 "Thread Frame" 과 동일.
		//HW decode 일 때는 thread_count 가 무시되거나 제한적.
		m_video_ctx->thread_count = 0;	 //0 = auto = available CPU cores
		m_video_ctx->thread_type  = FF_THREAD_FRAME | FF_THREAD_SLICE;

		hr = avcodec_open2(m_video_ctx, codec, NULL);
		if (hr < 0)
		{
			char buf[256];
			//logWrite(_T("[ffi/dec] avcodec_open2 fail hr=%d (%hs)"), hr, ffi::err_str(hr, buf, sizeof(buf)));
			close();
			return false;
		}

		//logWrite(_T("[ffi/dec] opened codec=%hs %dx%d threads=%d"),
			//codec->name, m_video_ctx->width, m_video_ctx->height, m_video_ctx->thread_count);

		//모든 audio stream enumerate + display name 빌드. m_audio_stream_indices 가 menu 의 track index 와 1:1.
		m_audio_stream_indices.clear();
		m_audio_track_names.clear();
		m_audio_track_current = -1;

		for (unsigned i = 0; i < m_fmt->nb_streams; ++i)
		{
			AVCodecParameters* p = m_fmt->streams[i]->codecpar;
			if (p->codec_type != AVMEDIA_TYPE_AUDIO) continue;
			const AVCodec* c = avcodec_find_decoder(p->codec_id);
			AVDictionaryEntry* lang	 = av_dict_get(m_fmt->streams[i]->metadata, "language", NULL, 0);
			AVDictionaryEntry* title = av_dict_get(m_fmt->streams[i]->metadata, "title",	NULL, 0);

			const char* codec_str = c ? c->name : "?";
			const char* lang_str  = (lang  && lang->value)	? lang->value  : "";
			const char* title_str = (title && title->value) ? title->value : "";

			int track_idx = (int)m_audio_stream_indices.size();
			m_audio_stream_indices.push_back((int)i);

			wchar_t buf[256];
			if (title_str[0] && lang_str[0])
				swprintf_s(buf, L"트랙 %d : %hs [%hs] / %hs / %dch / %dHz",
					track_idx + 1, title_str, lang_str, codec_str,
					p->ch_layout.nb_channels, p->sample_rate);
			else if (title_str[0])
				swprintf_s(buf, L"트랙 %d : %hs / %hs / %dch / %dHz",
					track_idx + 1, title_str, codec_str,
					p->ch_layout.nb_channels, p->sample_rate);
			else if (lang_str[0])
				swprintf_s(buf, L"트랙 %d : %hs / %hs / %dch / %dHz",
					track_idx + 1, lang_str, codec_str,
					p->ch_layout.nb_channels, p->sample_rate);
			else
				swprintf_s(buf, L"트랙 %d : %hs / %dch / %dHz",
					track_idx + 1, codec_str,
					p->ch_layout.nb_channels, p->sample_rate);

			m_audio_track_names.push_back(buf);

			//logWrite(_T("[ffi/dec] audio[%d→stream%u] %s"), track_idx, i, buf);
		}

		//초기 선택: m_initial_audio_track 우선 (track switch 의 close+reopen 채널), 없으면 av_find_best_stream.
		int chosen_stream_idx = -1;
		if (m_initial_audio_track >= 0 && m_initial_audio_track < (int)m_audio_stream_indices.size())
		{
			chosen_stream_idx = m_audio_stream_indices[m_initial_audio_track];
			m_audio_track_current = m_initial_audio_track;
			//logWrite(_T("[ffi/dec] audio track forced: track %d → stream %d"),
				//m_initial_audio_track, chosen_stream_idx);
		}
		else if (!m_audio_stream_indices.empty())
		{
			int best = av_find_best_stream(m_fmt, AVMEDIA_TYPE_AUDIO, -1, -1, NULL, 0);
			if (best >= 0)
			{
				chosen_stream_idx = best;
				for (size_t k = 0; k < m_audio_stream_indices.size(); ++k)
				{
					if (m_audio_stream_indices[k] == best)
					{
						m_audio_track_current = (int)k;
						break;
					}
				}
				if (m_audio_track_current < 0)
				{
					chosen_stream_idx = m_audio_stream_indices[0];
					m_audio_track_current = 0;
				}
			}
			else
			{
				chosen_stream_idx = m_audio_stream_indices[0];
				m_audio_track_current = 0;
			}
			//logWrite(_T("[ffi/dec] audio track auto: track %d → stream %d"),
				//m_audio_track_current, chosen_stream_idx);
		}
		m_initial_audio_track = -1;	  //consume

		m_audio_stream_idx = chosen_stream_idx;
		//logWrite(_T("[ffi/dec/diag] selected audio stream idx=%d"), m_audio_stream_idx);
		if (m_audio_stream_idx >= 0)
		{
			AVCodecParameters* apar = m_fmt->streams[m_audio_stream_idx]->codecpar;
			const AVCodec* acodec = avcodec_find_decoder(apar->codec_id);
			if (acodec)
			{
				m_audio_ctx = avcodec_alloc_context3(acodec);
				if (m_audio_ctx)
				{
					avcodec_parameters_to_context(m_audio_ctx, apar);
					int ahr = avcodec_open2(m_audio_ctx, acodec, NULL);
					if (ahr < 0)
					{
						//logWrite(_T("[ffi/dec] audio codec open fail hr=%d — audio disabled"), ahr);
						avcodec_free_context(&m_audio_ctx);
						m_audio_stream_idx = -1;
					}
					else
					{
						//logWrite(_T("[ffi/dec] opened audio codec=%hs %dHz ch=%d sample_fmt=%d"),
							//acodec->name, m_audio_ctx->sample_rate,
							//m_audio_ctx->ch_layout.nb_channels, (int)m_audio_ctx->sample_fmt);
					}
				}
			}
			else
			{
				//logWrite(_T("[ffi/dec] no audio decoder for codec_id=%d"), (int)apar->codec_id);
				m_audio_stream_idx = -1;
			}
		}

		//subtitle stream enumeration — internal path 는 디코딩/렌더링 미지원, metadata 만 빌드.
		//메뉴에서 트랙 보이게 + 사용자가 클릭하면 LAV path 로 close+reopen.
		m_subtitle_track_names.clear();
		for (unsigned i = 0; i < m_fmt->nb_streams; ++i)
		{
			AVCodecParameters* p = m_fmt->streams[i]->codecpar;
			if (p->codec_type != AVMEDIA_TYPE_SUBTITLE) continue;
			const AVCodec* c = avcodec_find_decoder(p->codec_id);
			AVDictionaryEntry* lang	 = av_dict_get(m_fmt->streams[i]->metadata, "language", NULL, 0);
			AVDictionaryEntry* title = av_dict_get(m_fmt->streams[i]->metadata, "title",	NULL, 0);

			const char* codec_str = c ? c->name : "?";
			const char* lang_str  = (lang  && lang->value)	? lang->value  : "";
			const char* title_str = (title && title->value) ? title->value : "";

			int track_idx = (int)m_subtitle_track_names.size();

			wchar_t buf[256];
			if (title_str[0] && lang_str[0])
				swprintf_s(buf, L"트랙 %d : %hs [%hs] / %hs",
					track_idx + 1, title_str, lang_str, codec_str);
			else if (title_str[0])
				swprintf_s(buf, L"트랙 %d : %hs / %hs",
					track_idx + 1, title_str, codec_str);
			else if (lang_str[0])
				swprintf_s(buf, L"트랙 %d : %hs / %hs",
					track_idx + 1, lang_str, codec_str);
			else
				swprintf_s(buf, L"트랙 %d : %hs",
					track_idx + 1, codec_str);

			m_subtitle_track_names.push_back(buf);
			//logWrite(_T("[ffi/dec] subtitle[%d→stream%u] %s"), track_idx, i, buf);
		}

		//seek 인덱스 유무 판별 — 부분 다운로드 AVI 등은 파일 끝 idx1 이 없어 인덱스 0. PTS 는 정상이므로 timing 은
		//기존 PTS 그대로, seek 만 byte 추정으로 전환(m_no_seek_index). 정상(인덱스 있음) 파일은 false → 기존 동작.
		//(libavformat 62: nb_index_entries 비공개 → avformat_index_get_entries_count 사용.)
		//AVI(SW codec) — mpeg4/divx 등은 video pts 가 byte-seek 시 garbage 라 pts-seek 가 부정확하고, 컨테이너 헤더
		//duration(m_fmt->duration)이 malformed 인 경우가 있다(예: GOM ENCODER 가 145분으로 기록했으나 실제는 33:45).
		//native video index(idx1)는 *실제 frame* 기준이라 정확하므로 — 마지막 entry timestamp = 실제 길이, entry 들의
		//(ts↔byte)로 seek 인덱스를 직접 구축 — av_read_frame 스캔(헤더가 부풀린 frame 수를 과다 카운트) 대신 사용한다.
		//그리고 unreliable_video_pts 경로(byte-seek + sample-count 타이밍)로 돌려 전 구간 정확 seek (팟플레이어식).
		//(h264-in-AVI(HW)는 real pts 가 정확하므로 제외 — 회귀 방지.)
		bool avi_native_index = false;
		if (m_video_stream_idx >= 0 &&
			m_fmt->iformat && m_fmt->iformat->name && strstr(m_fmt->iformat->name, "avi") &&
			m_hw_pix_fmt == AV_PIX_FMT_NONE)
		{
			AVStream* vst = m_fmt->streams[m_video_stream_idx];
			int idx_cnt = avformat_index_get_entries_count(vst);
			AVRational vtb = vst->time_base;
			double vfps = (vst->avg_frame_rate.den != 0) ? (double)vst->avg_frame_rate.num / (double)vst->avg_frame_rate.den : 0.0;
			double header_dur_ms = (m_fmt->duration != AV_NOPTS_VALUE) ? (double)m_fmt->duration / 1000.0 : -1.0;
			//logWrite(_T("[ffi/dec] AVI(SW) idx_cnt=%d header_dur=%.0fms"), idx_cnt, header_dur_ms);

			//native video index(idx1) 의 마지막 entry timestamp = 실제 길이.
			double idx_dur = 0.0;
			if (idx_cnt > 1)
			{
				const AVIndexEntry* last = avformat_index_get_entry(vst, idx_cnt - 1);
				idx_dur = (last && last->timestamp != AV_NOPTS_VALUE) ? (double)last->timestamp * av_q2d(vtb) * 1000.0 : 0.0;
			}

			//정상 indexed AVI 판정 — native index 가 충분(>1)하고 헤더 duration 이 index 산출 길이와 거의 일치(±2%).
			//이 경우 video pts·index 모두 신뢰 가능 → 일반 pts-seek(avformat_seek_file + probe, 팟플레이어식 정확 seek).
			//byte-seek/unreliable 경로는 *깨진* 파일(index 없음 또는 header duration malformed, 예: GOM ENCODER) 전용.
			//(정상 mpeg4/divx AVI 를 byte-seek 로 보내면 garbage pts·비결정적 착지로 트랙이동이 어긋난다.)
			double dur_diff = (header_dur_ms > idx_dur) ? (header_dur_ms - idx_dur) : (idx_dur - header_dur_ms);
			bool header_dur_bad   = (header_dur_ms <= 0.0) || (idx_dur > 0.0 && dur_diff > idx_dur * 0.02);
			bool valid_native_idx = (idx_cnt > 1) && (idx_dur > 0.0);

			if (valid_native_idx && !header_dur_bad)
			{
				//정상 파일 — unreliable/no_seek_index 설정 안 함. duration 은 헤더(정상)에서. 기존 정상-파일 동작과 동일(회귀 0).
				avi_native_index = true;
				//logWrite(_T("[ffi/dec] AVI 정상 인덱스(idx=%d dur=%.0fms header=%.0fms) — pts-seek 경로"), idx_cnt, idx_dur, header_dur_ms);
			}
			else
			{
				//깨진 파일 — byte-seek 경로. native index 가 있으면 그것으로(ts↔byte), 없으면 아래 백그라운드 스캔.
				m_unreliable_video_pts = true;
				m_no_seek_index = true;

				if (valid_native_idx)
				{
					std::vector<std::pair<int64_t, int64_t>> idx;
					int step = (vfps > 0.0) ? (int)(vfps * 5.0) : 150;	//~5s 간격 샘플.
					if (step < 1) step = 1;
					for (int i = 0; i < idx_cnt; i += step)
					{
						const AVIndexEntry* e = avformat_index_get_entry(vst, i);
						if (e && e->pos >= 0 && e->timestamp != AV_NOPTS_VALUE)
							idx.push_back(std::make_pair((int64_t)((double)e->timestamp * av_q2d(vtb) * 1000.0), e->pos));
					}
					if (!idx.empty())
					{
						m_scanned_duration_ms.store(idx_dur);	//duration_ms() 가 malformed 헤더 대신 이 값을 반환.
						m_buffered_frontier_ms.store(idx_dur);	//전 구간 가용 → 트랙 빨간선 숨김.
						{
							std::lock_guard<std::mutex> lk(m_seek_index_mtx);
							m_seek_index = idx;
							m_seek_index_snap = false;	//ts↔byte 보간 (byte-seek 착지 후 kf_skip 이 keyframe 으로).
						}
						avi_native_index = true;
						//logWrite(_T("[ffi/dec] AVI native-index(header malformed) → dur=%.0fms seek_idx=%zu byte-seek"), idx_dur, idx.size());
					}
				}
			}
		}

		//헤더에 길이가 없는 미종료 녹화, 또는 native index 가 없는 AVI — 백그라운드 전체 스캔으로 길이+seek 인덱스 산출.
		if (m_fmt->duration == AV_NOPTS_VALUE && !avi_native_index)
		{
			m_unreliable_video_pts = true;
			m_scan_quit.store(false);
			m_scanned_duration_ms.store(-1.0);
			m_scan_thread = std::thread(&CDecoder::scan_duration_worker, this);
			//logWrite(_T("[ffi/dec] duration unknown — unreliable + 스캔 시작"));
		}
		else if (m_no_seek_index && !avi_native_index)
		{
			m_scan_quit.store(false);
			m_scanned_duration_ms.store(-1.0);
			m_scan_thread = std::thread(&CDecoder::scan_duration_worker, this);
			//logWrite(_T("[ffi/dec] AVI native index 없음 — byte-seek 스캔 시작"));
		}

		//일부 파일은 컨테이너 codecpar 의 width/height 가 실제 SPS 코딩 크기와 다르다(헤더 메타 오류).
		//그 경우 m_video_ctx->width 는 첫 frame 을 디코드해야 정정된다. 그런데 GetMediaType/DecideBufferSize
		//는 graph 연결 시점(첫 디코드 전)에 호출돼 MPCVR 에 잘못된 해상도로 negotiate → FillBuffer 가 정정된
		//stride 로 채우면 렌더러 해석이 어긋나 화면이 검정/흰색으로 깨진다(컨테이너 해상도 ≠ 실제 코딩 해상도인
		//미디어). 여기서 첫 video frame 을 미리 디코드해 차원을 확정한 뒤 flush + 위치 복원.
		if (m_video_stream_idx >= 0 && m_video_ctx)
		{
			//가변 해상도 케이스: 스트림이 처음 ~1초는 큰 해상도 프레임, 이후 더 작은 해상도로 바뀌는 미디어가 있다
			//(타 플레이어도 frame 0 에서 큰 값 → <1초 후 작은 값). 단일 negotiate 인 DirectShow 는 두 해상도를
			//다 받을 수 없으므로, 여러 frame 을 디코드해 *지배적(sustained)* 해상도를 mode 로 결정한다.
			//FillBuffer 는 frame 크기가 이 negotiated 크기와 다르면 swscale 로 맞춰 스케일한다(인트로 프레임 흡수).
			//단일 해상도 미디어(99%)는 cnt_b 가 끝까지 0 → 조기 종료해 open latency 최소화.
			AVPacket* pkt = av_packet_alloc();
			AVFrame*  frm = av_frame_alloc();
			int wa = 0, ha = 0, ca = 0;	  //2-bucket — 대부분 미디어는 해상도 종류가 1~2 개.
			int wb = 0, hb = 0, cb = 0;
			int frames = 0;
			ULONGLONG t0 = GetTickCount64();
			while (pkt && frm && frames < 90 && (GetTickCount64() - t0) < 2500)
			{
				if (cb == 0 && frames >= 45)	//단일 해상도 확정 → 조기 종료.
					break;
				if (av_read_frame(m_fmt, pkt) < 0)
					break;
				if (pkt->stream_index == m_video_stream_idx)
				{
					int sr = avcodec_send_packet(m_video_ctx, pkt);
					if (sr >= 0 || sr == AVERROR(EAGAIN))
					{
						while (avcodec_receive_frame(m_video_ctx, frm) >= 0)
						{
							int fw = frm->width;
							int fh = frm->height;
							av_frame_unref(frm);
							if (fw <= 0 || fh <= 0)
								continue;
							++frames;
							if (wa == 0)			{ wa = fw; ha = fh; ca = 1; }
							else if (fw == wa && fh == ha)	++ca;
							else if (wb == 0)		{ wb = fw; hb = fh; cb = 1; }
							else if (fw == wb && fh == hb)	++cb;
						}
					}
				}
				av_packet_unref(pkt);
			}
			if (frm) av_frame_free(&frm);
			if (pkt) av_packet_free(&pkt);
			avcodec_flush_buffers(m_video_ctx);
			av_seek_frame(m_fmt, -1, 0, AVSEEK_FLAG_BACKWARD);
			if (cb > ca)		{ m_probe_w = wb; m_probe_h = hb; }
			else if (ca > 0)	{ m_probe_w = wa; m_probe_h = ha; }
			//logWrite(_T("[ffi/dec] dim probe: A=%dx%d(%d) B=%dx%d(%d) → video_width/height=%dx%d (frames=%d %llums)"),
				//wa, ha, ca, wb, hb, cb, video_width(), video_height(), frames, GetTickCount64() - t0);
		}

		return true;
	}

	//전체 파일을 *디코드 없이* demux 해 마지막 video timestamp 로 총 길이를 산출한다 (worker thread).
	//재생 중인 m_fmt 를 건드리지 않도록 독립된 2nd AVFormatContext 로 연다. m_scan_quit 로 취소 가능.
	void CDecoder::scan_duration_worker()
	{
		//wstring → utf8 (FFmpeg file protocol 은 Windows 에서 utf8 경로를 내부적으로 wide 변환해 처리).
		int u8len = ::WideCharToMultiByte(CP_UTF8, 0, m_path.c_str(), -1, NULL, 0, NULL, NULL);
		if (u8len <= 0)
			return;
		std::string u8(u8len, '\0');
		::WideCharToMultiByte(CP_UTF8, 0, m_path.c_str(), -1, &u8[0], u8len, NULL, NULL);

		AVFormatContext* fmt = nullptr;
		if (avformat_open_input(&fmt, u8.c_str(), NULL, NULL) < 0)
			return;

		//video / audio stream 둘 다 찾는다.
		int vidx = -1, aidx = -1;
		for (unsigned i = 0; i < fmt->nb_streams; ++i)
		{
			AVMediaType t = fmt->streams[i]->codecpar->codec_type;
			if (t == AVMEDIA_TYPE_VIDEO && vidx < 0) vidx = (int)i;
			else if (t == AVMEDIA_TYPE_AUDIO && aidx < 0) aidx = (int)i;
		}
		if (vidx < 0 && aidx < 0)
		{
			avformat_close_input(&fmt);
			return;
		}

		double fps = 0.0;
		if (vidx >= 0)
		{
			AVRational r = fmt->streams[vidx]->avg_frame_rate;
			if (r.den != 0)
				fps = (double)r.num / (double)r.den;
		}
		AVRational atb = (aidx >= 0) ? fmt->streams[aidx]->time_base : AVRational{ 0, 1 };
		int64_t file_size = (fmt->pb) ? avio_size(fmt->pb) : -1;

		//[2단계 순차 스캔] AVI 의 PCM audio pts 는 *순차 누적* 이라 byte-seek(tail-read) 로는 절대 위치를 못 얻는다(검증: ≈1초).
		//처음부터 디코드 없이 demux 하며 측정하되,
		//  (1단계) 앞 ~16MB 만 읽은 시점에 "그때까지 audio 시간 × (전체/현재 byte)" 로 *추정 길이* 를 즉시 store → 트랙바 거의 즉시 채움.
		//  (2단계) 끝까지 읽어 정확 길이로 교체. 견고한 두 소스 중 큰 값: (a) video 프레임수/fps, (b) audio 마지막 ts × atb.
		int64_t v_count = 0;
		int64_t a_last_end = AV_NOPTS_VALUE;	//audio tb 단위. 마지막 audio packet 의 (ts + duration).
		bool estimate_done = false;

		std::vector<std::pair<int64_t, int64_t>> index;	//(시간 ms, 파일 byte). ~5s 간격. audio 있으면 audio ms, 없으면 video 프레임 ms. 끝에 m_seek_index 로 publish.
		size_t published_size = 0;	//m_seek_index 에 점진 publish 한 index 항목 수 (스캔 중 byte-seek 가능하게).
		int64_t last_idx_ms = INT64_MIN;

		AVPacket* pkt = av_packet_alloc();
		if (pkt)
		{
			while (!m_scan_quit.load() && av_read_frame(fmt, pkt) >= 0)
			{
				if (pkt->stream_index == vidx)
				{
					++v_count;

					//audio 가 없는 파일 — seek 인덱스를 video *keyframe* 기준으로 빌드.
					//byte-seek 착지 후 디코드는 keyframe 부터만 가능하므로, 인덱스는 반드시 keyframe byte 여야 한다.
					//(미종료 녹화는 앞부분에만 keyframe 이 있고 뒷부분엔 없는 경우가 있다 → 임의 패킷 byte 로 seek 하면
					// 그 뒤로 keyframe 이 없어 EOF 까지 읽고 EOS. keyframe byte 로 snap 하면 항상 디코드 시작점에 착지.)
					//시간축은 프레임 수(v_count-1)/fps — video pts 는 garbage 라도 프레임 수는 단조 증가.
					//audio 있는 파일은 아래 audio 분기가 인덱스를 만든다(보간 경로).
					if (aidx < 0 && fps > 0.0 && (pkt->flags & AV_PKT_FLAG_KEY) && pkt->pos >= 0)
					{
						int64_t cur_ms = (int64_t)((double)(v_count - 1) * 1000.0 / fps);
						index.push_back(std::make_pair(cur_ms, pkt->pos));
					}
				}
				else if (pkt->stream_index == aidx)
				{
					int64_t ts = (pkt->pts != AV_NOPTS_VALUE) ? pkt->pts : pkt->dts;
					if (ts != AV_NOPTS_VALUE)
					{
						int64_t end = ts + (pkt->duration > 0 ? pkt->duration : 0);
						if (a_last_end == AV_NOPTS_VALUE || end > a_last_end)
							a_last_end = end;

						//seek 인덱스 — ~5s 간격으로 (audio ms, 현재 파일 byte) 기록. byte-seek 정확도 + content_end 파악.
						if (atb.den > 0)
						{
							int64_t cur_ms = (int64_t)((double)end * av_q2d(atb) * 1000.0);
							if (last_idx_ms == INT64_MIN || cur_ms - last_idx_ms >= 5000)
							{
								index.push_back(std::make_pair(cur_ms, avio_tell(fmt->pb)));
								last_idx_ms = cur_ms;
							}
						}
					}
				}
				av_packet_unref(pkt);

				//[진행상황 + 인덱스 점진 발행] 재생과 독립적으로 트랙 red(buffered frontier)가 앞서 채워지고,
				//스캔된 구간은 byte-seek(byte_for_time_ms = m_seek_index 보간)로 즉시 이동 가능해진다(팟플레이어식).
				if (atb.den > 0 && a_last_end != AV_NOPTS_VALUE)
					m_buffered_frontier_ms.store((double)a_last_end * av_q2d(atb) * 1000.0);
				else if (aidx < 0 && fps > 0.0 && v_count > 0)
					m_buffered_frontier_ms.store((double)v_count * 1000.0 / fps);

				if (index.size() != published_size)
				{
					std::lock_guard<std::mutex> lk(m_seek_index_mtx);
					m_seek_index = index;
					m_seek_index_snap = (aidx < 0);
					published_size = index.size();
				}

				//1단계 추정 — 앞부분 audio 시간 ÷ 읽은 byte 비율로 전체 길이 외삽 (bitrate 거의 균일 가정).
				if (!estimate_done && a_last_end != AV_NOPTS_VALUE && atb.den > 0 && file_size > 0)
				{
					int64_t pos = avio_tell(fmt->pb);
					if (pos > 16 * 1024 * 1024)
					{
						double covered_ms = (double)a_last_end * av_q2d(atb) * 1000.0;
						double est_ms = covered_ms * ((double)file_size / (double)pos);
						if (est_ms > 0.0)
						{
							m_scanned_duration_ms.store(est_ms);
							//logWrite(_T("[ffi/dec] duration estimate (phase1) ~%.0fms (covered=%.0fms pos=%lld/%lld)"),
								//est_ms, covered_ms, (long long)pos, (long long)file_size);
						}
						estimate_done = true;
					}
				}
			}
			av_packet_free(&pkt);
		}

		if (!m_scan_quit.load())
		{
			double dur_v_ms = (fps > 0.0 && v_count > 0) ? (double)v_count * 1000.0 / fps : 0.0;
			double dur_a_ms = (a_last_end != AV_NOPTS_VALUE && atb.den > 0) ? (double)a_last_end * av_q2d(atb) * 1000.0 : 0.0;
			double dur_ms = (dur_v_ms > dur_a_ms) ? dur_v_ms : dur_a_ms;
			if (dur_ms > 0.0)
			{
				m_scanned_duration_ms.store(dur_ms);	//2단계 — 정확값으로 교체.
				//logWrite(_T("[ffi/dec] full scan done (phase2) — duration=%.0fms (video=%.0fms[%lld frames] audio=%.0fms)"),
					//dur_ms, dur_v_ms, (long long)v_count, dur_a_ms);
			}

			//seek 인덱스 publish — 이후 byte-seek 가 시간→byte 보간으로 정확히 이동(garbage tail/비균일 bitrate overshoot 방지).
			if (!index.empty())
			{
				std::lock_guard<std::mutex> lk(m_seek_index_mtx);
				m_seek_index = std::move(index);
				m_seek_index_snap = (aidx < 0);	//video-only = keyframe 인덱스 → snap. audio = 보간.
				//logWrite(_T("[ffi/dec] seek index built — %zu entries snap=%d (last %lldms @ byte %lld)"),
					//m_seek_index.size(), (int)m_seek_index_snap, (long long)m_seek_index.back().first, (long long)m_seek_index.back().second);
			}
		}

		avformat_close_input(&fmt);
	}

	//시간(ms) → 파일 byte 위치를 seek 인덱스에서 보간. 인덱스 비었으면 -1. ms 가 끝 너머면 마지막 byte 로 clamp (content end).
	int64_t CDecoder::byte_for_time_ms(double ms)
	{
		std::lock_guard<std::mutex> lk(m_seek_index_mtx);
		if (m_seek_index.empty())
			return -1;
		if (ms <= (double)m_seek_index.front().first)
			return m_seek_index.front().second;
		if (ms >= (double)m_seek_index.back().first)
			return m_seek_index.back().second;	//content 끝 너머 — 마지막 유효 byte 로 clamp (garbage tail 진입 방지).

		size_t lo = 0, hi = m_seek_index.size() - 1;
		while (lo + 1 < hi)
		{
			size_t mid = (lo + hi) / 2;
			if ((double)m_seek_index[mid].first <= ms) lo = mid;
			else hi = mid;
		}

		//keyframe 인덱스 — target 이하의 가장 가까운 keyframe byte 로 snap (보간 금지: 비keyframe byte 는 디코드 불가).
		if (m_seek_index_snap)
			return m_seek_index[lo].second;

		int64_t t0 = m_seek_index[lo].first, t1 = m_seek_index[hi].first;
		int64_t b0 = m_seek_index[lo].second, b1 = m_seek_index[hi].second;
		if (t1 == t0)
			return b0;
		double frac = (ms - (double)t0) / (double)(t1 - t0);
		return b0 + (int64_t)(frac * (double)(b1 - b0));
	}

	void CDecoder::close()
	{
		stop();

		//백그라운드 duration 스캐너 취소 + join (독립 context 라 m_fmt 정리와 순서 무관하나 thread 는 먼저 회수).
		m_scan_quit.store(true);
		if (m_scan_thread.joinable())
			m_scan_thread.join();
		m_buffered_frontier_ms.store(-1.0);
		m_scanned_duration_ms.store(-1.0);
		m_unreliable_video_pts = false;
		m_no_seek_index = false;
		{
			std::lock_guard<std::mutex> lk(m_seek_index_mtx);
			m_seek_index.clear();
			m_seek_index_snap = false;
		}
		m_path.clear();

		if (m_video_ctx)
			avcodec_free_context(&m_video_ctx);
		if (m_audio_ctx)
			avcodec_free_context(&m_audio_ctx);

		if (m_hw_device_ctx)
			av_buffer_unref(&m_hw_device_ctx);
		m_hw_pix_fmt = AV_PIX_FMT_NONE;

		if (m_fmt)
			avformat_close_input(&m_fmt);

		//AVFMT_FLAG_CUSTOM_IO 라 avformat_close_input 이 m_fmt->pb 를 정리하지 않음 — caller 책임.
		if (m_avio)
		{
			av_freep(&m_avio->buffer);
			avio_context_free(&m_avio);
		}
		if (m_file_handle)
		{
			::CloseHandle(m_file_handle);
			m_file_handle = nullptr;
		}

		m_video_stream_idx = -1;
		m_probe_w = 0;
		m_probe_h = 0;
		m_audio_stream_idx = -1;
		m_audio_stream_indices.clear();
		m_audio_track_names.clear();
		m_audio_track_current = -1;
		m_subtitle_track_names.clear();
	}

	void CDecoder::start()
	{
		if (!m_fmt || m_thread.joinable())
			return;
		m_quit.store(false);
		m_thread = std::thread(&CDecoder::worker_loop, this);
		//logWrite(_T("[ffi/dec] worker started"));
	}

	void CDecoder::stop()
	{
		if (!m_thread.joinable())
			return;
		m_quit.store(true);
		m_cv_queue.notify_all();
		m_thread.join();

		//queue 의 남은 frame 들 release.
		std::unique_lock<std::mutex> lk(m_mtx_queue);
		while (!m_video_queue.empty())
		{
			AVFrame* f = m_video_queue.front();
			m_video_queue.pop_front();
			av_frame_free(&f);
		}
		while (!m_audio_queue.empty())
		{
			AVFrame* f = m_audio_queue.front();
			m_audio_queue.pop_front();
			av_frame_free(&f);
		}
		//logWrite(_T("[ffi/dec] worker stopped"));
	}

	void CDecoder::seek(double pos_ms, int64_t segment_rt, double prev_emit_ms)
	{
		//Generation 증가는 worker 의 av_seek_frame 처리 후. caller 에서 ++ 하면 worker 가 이미 디코드 중인 frame 들도 stale tag →
		//pop drop → freeze. worker 가 새 위치 처리 후부터 새 generation push 가 정공.
		//m_pending_segment_rt 저장 — worker 가 av_seek_frame 후 m_segment_rt 갱신할 때 사용.
		m_pending_segment_rt = segment_rt;

		{
			std::unique_lock<std::mutex> lk(m_mtx_seek);
			m_pending_seek_ms = pos_ms;
			m_pending_prev_ms = prev_emit_ms;
			m_pending_kf_mode = m_seek_keyframe_mode.load();	//이 seek 의 모드를 호출 시점에 고정 (worker live-read race 차단).
			m_seek_processed = false;
		}

		//seek 요청 시점에 즉시 EOF 해제 (worker 가 av_seek 처리하며 880 에서 다시 false 로 store 하지만 그건 async).
		//이게 없으면: EOS 상태(m_eof=true)에서 seek 시, async worker 가 처리하기 전에 source pin 의 FillBuffer 가
		//재기동돼 is_eof()==true && queue==0 (방금 비움) 을 보고 spurious EndOfStream 을 deliver → 중복 EC_COMPLETE →
		//반복재생이 한 사이클에 2번 발화 + 오디오 렌더러 연속 flush 로 "띡" 글리치. seek = "더 이상 EOF 아님" 이므로 여기서 해제.
		m_eof.store(false);
		m_eof_flushed = false;

		{
			std::unique_lock<std::mutex> lk(m_mtx_queue);
			while (!m_video_queue.empty())
			{
				AVFrame* f = m_video_queue.front();
				m_video_queue.pop_front();
				av_frame_free(&f);
			}
			while (!m_audio_queue.empty())
			{
				AVFrame* f = m_audio_queue.front();
				m_audio_queue.pop_front();
				av_frame_free(&f);
			}
		}

		m_cv_queue.notify_all();   //worker 가 wait 중이면 깨움.
	}

	bool CDecoder::wait_seek_done(int timeout_ms)
	{
		std::unique_lock<std::mutex> lk(m_mtx_seek);
		return m_cv_seek_done.wait_for(lk, std::chrono::milliseconds(timeout_ms),
			[this]() { return m_seek_processed; });
	}

	AVFrame* CDecoder::pop_video_frame()
	{
		int current = m_seek_generation.load();
		std::unique_lock<std::mutex> lk(m_mtx_queue);
		while (!m_video_queue.empty())
		{
			AVFrame* f = m_video_queue.front();
			m_video_queue.pop_front();
			if ((intptr_t)f->opaque == current)
			{
				m_cv_queue.notify_one();
				return f;
			}
			//이전 generation 의 stale frame — skip + free.
			av_frame_free(&f);
		}
		m_cv_queue.notify_one();
		return nullptr;
	}

	AVFrame* CDecoder::pop_audio_frame()
	{
		int current = m_seek_generation.load();
		std::unique_lock<std::mutex> lk(m_mtx_queue);
		while (!m_audio_queue.empty())
		{
			AVFrame* f = m_audio_queue.front();
			m_audio_queue.pop_front();
			if ((intptr_t)f->opaque == current)
			{
				m_cv_queue.notify_one();
				return f;
			}
			av_frame_free(&f);
		}
		m_cv_queue.notify_one();
		return nullptr;
	}

	size_t CDecoder::video_queue_size()
	{
		std::unique_lock<std::mutex> lk(m_mtx_queue);
		return m_video_queue.size();
	}

	size_t CDecoder::audio_queue_size()
	{
		std::unique_lock<std::mutex> lk(m_mtx_queue);
		return m_audio_queue.size();
	}

	int CDecoder::video_width() const
	{
		if (m_probe_w > 0)
			return m_probe_w;
		return m_video_ctx ? m_video_ctx->width : 0;
	}

	int CDecoder::video_height() const
	{
		if (m_probe_h > 0)
			return m_probe_h;
		return m_video_ctx ? m_video_ctx->height : 0;
	}

	double CDecoder::duration_ms() const
	{
		//헤더에 길이가 없던 파일은 백그라운드 스캔 결과를 우선 반환 (스캔 완료 전엔 < 0 → 아래 fallback).
		double scanned = m_scanned_duration_ms.load();
		if (scanned > 0.0)
			return scanned;
		if (!m_fmt || m_fmt->duration == AV_NOPTS_VALUE)
			return 0.0;
		return (double)m_fmt->duration / 1000.0;   //AV_TIME_BASE = 1000000 (μs), /1000 → ms.
	}

	double CDecoder::buffered_ms()
	{
		//frontier probe 스레드가 별도 컨텍스트로 산출한 값. 재생과 무관(=재생을 따라가지 않음).
		return m_buffered_frontier_ms.load();
	}

	double CDecoder::frame_rate() const
	{
		if (!m_fmt || m_video_stream_idx < 0)
			return 0.0;
		AVRational r = m_fmt->streams[m_video_stream_idx]->avg_frame_rate;
		if (r.den == 0)
			return 0.0;
		return (double)r.num / (double)r.den;
	}

	AVRational CDecoder::video_time_base() const
	{
		AVRational zero = { 0, 1 };
		if (!m_fmt || m_video_stream_idx < 0)
			return zero;
		return m_fmt->streams[m_video_stream_idx]->time_base;
	}

	int CDecoder::video_pixel_format() const
	{
		return m_video_ctx ? (int)m_video_ctx->pix_fmt : (int)AV_PIX_FMT_NONE;
	}

	int CDecoder::audio_sample_rate() const
	{
		return m_audio_ctx ? m_audio_ctx->sample_rate : 0;
	}

	int CDecoder::audio_channels() const
	{
		return m_audio_ctx ? m_audio_ctx->ch_layout.nb_channels : 0;
	}

	int CDecoder::audio_sample_format() const
	{
		return m_audio_ctx ? (int)m_audio_ctx->sample_fmt : (int)AV_SAMPLE_FMT_NONE;
	}

	AVRational CDecoder::audio_time_base() const
	{
		AVRational zero = { 0, 1 };
		if (!m_fmt || m_audio_stream_idx < 0)
			return zero;
		return m_fmt->streams[m_audio_stream_idx]->time_base;
	}

	//--- display info — UI 표시용 (PotPlayer Tab OSD 식 codec/format 표시).
	//AVCodecParameters / AVStream 의 정보 직접 query. MediaInfo lib 의존 없음.

	static std::wstring utf8_to_wstring_upper(const char* s)
	{
		std::wstring out;
		if (!s)
			return out;
		while (*s)
		{
			wchar_t c = (wchar_t)(unsigned char)(*s++);
			if (c >= L'a' && c <= L'z')
				c -= 32;
			out += c;
		}
		return out;
	}

	static std::wstring fourcc_to_wstring(uint32_t tag)
	{
		std::wstring out;
		if (tag == 0)
			return out;
		for (int i = 0; i < 4; i++)
		{
			unsigned char c = (tag >> (i * 8)) & 0xFF;
			if (c >= 0x20 && c < 0x7F)
				out += (wchar_t)c;
		}
		return out;
	}

	std::wstring CDecoder::video_codec_name() const
	{
		if (!m_fmt || m_video_stream_idx < 0)
			return L"";
		AVCodecParameters* par = m_fmt->streams[m_video_stream_idx]->codecpar;
		return utf8_to_wstring_upper(avcodec_get_name(par->codec_id));
	}

	std::wstring CDecoder::video_fourcc() const
	{
		if (!m_fmt || m_video_stream_idx < 0)
			return L"";
		AVCodecParameters* par = m_fmt->streams[m_video_stream_idx]->codecpar;
		return fourcc_to_wstring(par->codec_tag);
	}

	int CDecoder::video_bit_depth() const
	{
		if (!m_fmt || m_video_stream_idx < 0)
			return 0;
		AVCodecParameters* par = m_fmt->streams[m_video_stream_idx]->codecpar;
		//per-component depth × component 수 → 8-bit yuv420p = 24, 10-bit = 30 (PotPlayer "N bit" 과 일치).
		const AVPixFmtDescriptor* d = av_pix_fmt_desc_get((AVPixelFormat)par->format);
		if (d && d->nb_components > 0)
			return d->comp[0].depth * d->nb_components;
		if (par->bits_per_raw_sample > 0)
			return par->bits_per_raw_sample;
		if (par->bits_per_coded_sample > 0)
			return par->bits_per_coded_sample;
		return 0;
	}

	int64_t CDecoder::video_bit_rate() const
	{
		if (!m_fmt || m_video_stream_idx < 0)
			return 0;
		int64_t known = stream_known_bit_rate(m_fmt->streams[m_video_stream_idx]);
		if (known > 0)
			return known;
		//codecpar·BPS 둘 다 없는 컨테이너 — 전체 비트레이트(ffmpeg 가 파일크기/재생시간으로 자동 산출)에서 나머지 stream 들의 알려진 비트레이트를 뺀 추정값 (PotPlayer 도 동일).
		if (m_fmt->bit_rate > 0)
		{
			int64_t others = 0;
			for (unsigned int i = 0; i < m_fmt->nb_streams; ++i)
			{
				if ((int)i == m_video_stream_idx)
					continue;
				others += stream_known_bit_rate(m_fmt->streams[i]);
			}
			int64_t est = m_fmt->bit_rate - others;
			if (est > 0)
				return est;
		}
		return 0;
	}

	std::wstring CDecoder::video_aspect_ratio() const
	{
		if (!m_fmt || m_video_stream_idx < 0)
			return L"";
		AVStream* s = m_fmt->streams[m_video_stream_idx];
		AVCodecParameters* par = s->codecpar;
		AVRational sar = s->sample_aspect_ratio;
		if (sar.num == 0 || sar.den == 0)
			sar = par->sample_aspect_ratio;
		int dar_num = par->width;
		int dar_den = par->height;
		if (sar.num > 0 && sar.den > 0)
		{
			dar_num = par->width * sar.num;
			dar_den = par->height * sar.den;
		}
		if (dar_num <= 0 || dar_den <= 0)
			return L"";
		//gcd 로 simplify.
		int a = dar_num, b = dar_den;
		while (b) { int t = a % b; a = b; b = t; }
		if (a > 0)
		{
			dar_num /= a;
			dar_den /= a;
		}
		wchar_t buf[32];
		swprintf_s(buf, 32, L"%d:%d", dar_num, dar_den);
		return buf;
	}

	std::wstring CDecoder::video_pixel_format_name() const
	{
		AVPixelFormat fmt = (AVPixelFormat)video_pixel_format();
		if (fmt == AV_PIX_FMT_NONE)
			return L"";
		const char* name = av_get_pix_fmt_name(fmt);
		if (!name)
			return L"";
		//pixel format 이름은 lower 그대로 (yuv420p, nv12 등 관례).
		std::wstring out;
		while (*name)
			out += (wchar_t)(unsigned char)(*name++);
		return out;
	}

	std::wstring CDecoder::video_chroma_location_name() const
	{
		if (!m_fmt || m_video_stream_idx < 0)
			return L"";
		AVCodecParameters* par = m_fmt->streams[m_video_stream_idx]->codecpar;
		const char* name = av_chroma_location_name(par->chroma_location);
		if (!name)
			return L"";
		std::wstring out;
		while (*name)
			out += (wchar_t)(unsigned char)(*name++);
		if (out == L"unspecified")
			return L"";
		return out;
	}

	std::wstring CDecoder::video_hw_accel_name() const
	{
		if (!m_hw_device_ctx)
			return L"";
		AVHWDeviceContext* hwctx = (AVHWDeviceContext*)m_hw_device_ctx->data;
		const char* name = av_hwdevice_get_type_name(hwctx->type);
		return utf8_to_wstring_upper(name);
	}

	std::wstring CDecoder::audio_codec_name() const
	{
		if (!m_fmt || m_audio_stream_idx < 0)
			return L"";
		AVCodecParameters* par = m_fmt->streams[m_audio_stream_idx]->codecpar;
		return utf8_to_wstring_upper(avcodec_get_name(par->codec_id));
	}

	std::wstring CDecoder::audio_channel_layout_name() const
	{
		if (!m_audio_ctx)
			return L"";
		char buf[64] = { 0 };
		int n = av_channel_layout_describe(&m_audio_ctx->ch_layout, buf, sizeof(buf));
		if (n <= 0)
			return L"";
		std::wstring out;
		for (int i = 0; i < n - 1 && buf[i]; i++)
			out += (wchar_t)(unsigned char)buf[i];
		return out;
	}

	int CDecoder::audio_bit_depth() const
	{
		if (!m_fmt || m_audio_stream_idx < 0)
			return 0;
		AVCodecParameters* par = m_fmt->streams[m_audio_stream_idx]->codecpar;
		return par->bits_per_coded_sample;
	}

	int64_t CDecoder::audio_bit_rate() const
	{
		if (!m_fmt || m_audio_stream_idx < 0)
			return 0;
		AVCodecParameters* par = m_fmt->streams[m_audio_stream_idx]->codecpar;
		return par->bit_rate;
	}

	const std::wstring& CDecoder::audio_track_name(int track_idx) const
	{
		static const std::wstring empty;
		if (track_idx < 0 || track_idx >= (int)m_audio_track_names.size())
			return empty;
		return m_audio_track_names[track_idx];
	}

	const std::wstring& CDecoder::subtitle_track_name(int track_idx) const
	{
		static const std::wstring empty;
		if (track_idx < 0 || track_idx >= (int)m_subtitle_track_names.size())
			return empty;
		return m_subtitle_track_names[track_idx];
	}

	void CDecoder::worker_loop()
	{
		AVPacket* pkt = av_packet_alloc();

		while (!m_quit.load())
		{
			//=== 1) seek 요청 처리 ===
			{
				std::unique_lock<std::mutex> lk(m_mtx_seek);
				if (m_pending_seek_ms >= 0)
				{
					double seek_pos = m_pending_seek_ms;
					double prev_pos = m_pending_prev_ms;
					bool   kf_mode  = m_pending_kf_mode;	//seek() 가 호출 시점에 고정한 스냅샷.
					m_pending_seek_ms = -1.0;
					m_scan_pos_ms.store(-1, std::memory_order_relaxed);	//새 seek — 이전 스캔 위치 잔상 제거.
					m_pos_searching.store(true, std::memory_order_relaxed);	//첫 frame 디코드 전까지 packet 시각으로 트랙바 전진.
					m_video_no_frame_count = 0;		//stall 카운터 리셋.
					m_did_garbage_scan = false;		//이번 seek 의 손상 스캔 여부 리셋.
					lk.unlock();

					//queue flush — 이전 위치 frame 들 모두 버림. video + audio 둘 다.
					{
						std::unique_lock<std::mutex> lkq(m_mtx_queue);
						while (!m_video_queue.empty())
						{
							AVFrame* f = m_video_queue.front();
							m_video_queue.pop_front();
							av_frame_free(&f);
						}
						while (!m_audio_queue.empty())
						{
							AVFrame* f = m_audio_queue.front();
							m_audio_queue.pop_front();
							av_frame_free(&f);
						}
					}

					int64_t target_us = (int64_t)(seek_pos * AV_TIME_BASE / 1000.0);

					LARGE_INTEGER qpc_freq, qpc_t0, qpc_t1;
					::QueryPerformanceFrequency(&qpc_freq);
					::QueryPerformanceCounter(&qpc_t0);

					//stream_index=video 로 명시 + target 을 video stream time_base 단위로 변환.
					//stream_index=-1 (format-level) 은 *어느* stream 의 keyframe 으로 갈지 모호.
					AVRational vtb = m_fmt->streams[m_video_stream_idx]->time_base;
					int64_t target_pts = av_rescale_q(target_us,
						AVRational{1, AV_TIME_BASE}, vtb);

					int hr_seek;
					if (m_unreliable_video_pts || m_no_seek_index)
					{
						//[byte 추정 seek] 인덱스 없는 파일 — 시간 기반 av_seek_frame 이 시작 부근으로 fallback 함(검증됨).
						//파일 크기·추정 길이로 target byte 를 비례 추정 → AVSEEK_FLAG_BYTE 이동, 착지 후 첫 video keyframe 부터
						//emit(clean picture). 위치는 audio pts(연속·정확)가 반영. 정상 파일은 이 분기 안 탐(아래 pts-seek).
						//1순위: seek 인덱스(시간→byte 보간) — 콘텐츠 실제 byte 분포 반영, garbage tail/비균일 bitrate overshoot 방지.
						//인덱스 미생성(스캔 완료 전)이면 linear × filesize fallback (근사, 곧 인덱스로 대체됨).
						int64_t target_byte = byte_for_time_ms(seek_pos);
						const wchar_t* method = L"index";
						if (target_byte < 0)
						{
							double dur = m_scanned_duration_ms.load();
							int64_t file_size = (m_fmt->pb) ? avio_size(m_fmt->pb) : -1;
							if (dur > 0.0 && file_size > 0)
							{
								double ratio = seek_pos / dur;
								if (ratio < 0.0) ratio = 0.0;
								if (ratio > 1.0) ratio = 1.0;
								target_byte = (int64_t)(ratio * (double)file_size);
								method = L"linear";
							}
						}
						if (target_byte >= 0)
						{
							hr_seek = av_seek_frame(m_fmt, -1, target_byte, AVSEEK_FLAG_BYTE);
							m_kf_skip_active  = true;
							m_kf_skip_min_pts = INT64_MIN;	//착지 후 첫 keyframe 부터 디코드.
							m_kf_skip_count   = 0;
							int64_t avio_landed = (m_fmt->pb) ? avio_tell(m_fmt->pb) : -1;
							//logWrite(_T("[ffi/dec/byteseek] seek_pos=%.0fms method=%ls target_byte=%lld hr=%d avio_tell=%lld"),
								//seek_pos, method, (long long)target_byte, hr_seek, (long long)avio_landed);
						}
						else
						{
							hr_seek = av_seek_frame(m_fmt, -1, 0, AVSEEK_FLAG_BYTE);
							m_kf_skip_active = false;
							//logWrite(_T("[ffi/dec/byteseek] no index/duration yet — seek to start"));
						}
					}
					else if (kf_mode)
					{
						//[keyframe 모드] 방향 판정 — prev(직전 위치) 대비 forward 인지 backward 인지.
						m_kf_skip_target_pts = target_pts;
						bool is_forward = (prev_pos < 0) || (seek_pos >= prev_pos);
						if (is_forward)
						{
							//forward: BACKWARD seek 로 target GOP 진입 후 pts>prev 인 첫 keyframe 부터 디코드 — 다음 keyframe 으로 전진(제자리 회귀 방지).
							hr_seek = av_seek_frame(m_fmt, m_video_stream_idx, target_pts, AVSEEK_FLAG_BACKWARD);
							int64_t prev_pts = (prev_pos >= 0)
								? av_rescale_q((int64_t)(prev_pos * AV_TIME_BASE / 1000.0), AVRational{1, AV_TIME_BASE}, vtb)
								: INT64_MIN;
							m_kf_skip_active  = true;
							m_kf_skip_min_pts = prev_pts;	//이 값 초과 첫 keyframe 부터 디코드.
							m_kf_skip_count   = 0;
						}
						else
						{
							//backward: 이 파일은 av_seek_frame/avformat_seek_file 둘 다 target 부근(mid-GOP)에 떨어지고
							//이전 keyframe 으로 snap 하지 않음 → 디코더가 *다음(forward)* keyframe 으로 점프 → 뒤로 안 감(제자리).
							//probe: landing 후 첫 video keyframe 이 target 보다 뒤(overshoot)면 margin 을 늘려가며 재seek →
							//첫 keyframe 이 target 이하가 될 때까지. 디코드/큐 없이 packet flag 만 검사 (오디오 큐 오염 방지).
							int64_t one_sec = av_rescale_q(AV_TIME_BASE, AVRational{1, AV_TIME_BASE}, vtb);
							int64_t seek_ts = target_pts;
							int64_t margin  = (one_sec > 0) ? one_sec : 1;
							hr_seek = avformat_seek_file(m_fmt, m_video_stream_idx, INT64_MIN, seek_ts, seek_ts, AVSEEK_FLAG_BACKWARD);
							AVPacket* probe = av_packet_alloc();
							int probe_attempts = 0;
							for (; probe_attempts < 12; ++probe_attempts)
							{
								int64_t kf = AV_NOPTS_VALUE;
								//손상 구간에서 keyframe 도 EOF 도 안 나오면 무한 루프 → packet 수 상한으로 차단.
								int64_t probe_pkts = 0;
								while (av_read_frame(m_fmt, probe) >= 0)
								{
									bool is_vkey = (probe->stream_index == m_video_stream_idx) && (probe->flags & AV_PKT_FLAG_KEY);
									int64_t ppts = (probe->pts != AV_NOPTS_VALUE) ? probe->pts : probe->dts;
									av_packet_unref(probe);
									if (is_vkey && ppts != AV_NOPTS_VALUE)
									{
										kf = ppts;
										break;
									}
									if (++probe_pkts > kf_skip_limit)
										break;	//garbage — 이 attempt 포기(아래에서 kf==NOPTS 로 루프 종료).
								}
								if (kf == AV_NOPTS_VALUE || kf <= target_pts || seek_ts <= 0)
									break;	 //이전 keyframe 확보 / 파일 앞 / EOF.
								seek_ts -= margin;
								margin  *= 2;
								if (seek_ts < 0)
									seek_ts = 0;
								avformat_seek_file(m_fmt, m_video_stream_idx, INT64_MIN, seek_ts, seek_ts, AVSEEK_FLAG_BACKWARD);
							}
							av_packet_free(&probe);
							//확정 seek_ts 로 복귀 — probe 가 demuxer 를 전진시켰으므로. 그 keyframe 부터 디코드.
							hr_seek = avformat_seek_file(m_fmt, m_video_stream_idx, INT64_MIN, seek_ts, seek_ts, AVSEEK_FLAG_BACKWARD);
							//logWrite(_T("[ffi/dec/kf] backward probe attempts=%d final_seek_ts=%lld target=%lld"),
								//probe_attempts, (long long)seek_ts, (long long)target_pts);
							m_kf_skip_active  = true;
							m_kf_skip_min_pts = INT64_MIN;	//landing keyframe(<=target) 부터 emit.
							m_kf_skip_count   = 0;
						}
					}
					else
					{
						//[정확 모드] av_seek_frame BACKWARD 는 *prefer* 일 뿐 *force* 가 아니라 sparse keyframe 미디어에서
						//target 보다 *뒤* keyframe 으로 fallback 가능. 그러면 source filter 의 pre-target skip(pts<target) 이
						//걸릴 frame 이 없어 그 뒤 keyframe 이 그대로 표시되고 멈춘다 — 뒤로 1프레임(D) 이 무반응/근처 keyframe
						//점프로 보이는 원인 (고정 1초 margin 으론 GOP>1초 미디어에서 여전히 overshoot).
						//avformat_seek_file(max_ts=target) + probe 로 target 이하 keyframe 을 *보장* → source filter 가 거기서
						//target 까지 forward skip 해 정확 프레임 emit. keyframe 모드 backward 분기와 동일한 robust seek.
						int64_t one_sec = av_rescale_q(AV_TIME_BASE, AVRational{1, AV_TIME_BASE}, vtb);
						int64_t seek_ts = target_pts;
						int64_t margin  = (one_sec > 0) ? one_sec : 1;
						hr_seek = avformat_seek_file(m_fmt, m_video_stream_idx, INT64_MIN, seek_ts, seek_ts, AVSEEK_FLAG_BACKWARD);
						AVPacket* probe = av_packet_alloc();
						int probe_attempts = 0;
						for (; probe_attempts < 12; ++probe_attempts)
						{
							int64_t kf = AV_NOPTS_VALUE;
							while (av_read_frame(m_fmt, probe) >= 0)
							{
								bool is_vkey = (probe->stream_index == m_video_stream_idx) && (probe->flags & AV_PKT_FLAG_KEY);
								int64_t ppts = (probe->pts != AV_NOPTS_VALUE) ? probe->pts : probe->dts;
								av_packet_unref(probe);
								if (is_vkey && ppts != AV_NOPTS_VALUE)
								{
									kf = ppts;
									break;
								}
							}
							if (kf == AV_NOPTS_VALUE || kf <= target_pts || seek_ts <= 0)
								break;	 //target 이하 keyframe 확보 / 파일 앞 / EOF.
							seek_ts -= margin;
							margin  *= 2;
							if (seek_ts < 0)
								seek_ts = 0;
							avformat_seek_file(m_fmt, m_video_stream_idx, INT64_MIN, seek_ts, seek_ts, AVSEEK_FLAG_BACKWARD);
						}
						av_packet_free(&probe);
						//확정 seek_ts 로 복귀 — probe 가 demuxer 를 전진시켰으므로.
						hr_seek = avformat_seek_file(m_fmt, m_video_stream_idx, INT64_MIN, seek_ts, seek_ts, AVSEEK_FLAG_BACKWARD);
						logWrite(_T("[seek/diag] exact target_pts=%lld final_seek_ts=%lld probe_attempts=%d hr=%d"),
							(long long)target_pts, (long long)seek_ts, probe_attempts, hr_seek);
						m_kf_skip_active = false;
					}
					avcodec_flush_buffers(m_video_ctx);
					if (m_audio_ctx)
						avcodec_flush_buffers(m_audio_ctx);
					m_eof.store(false);	  //seek 후 새 위치부터 재생 — EOF 상태 해제.
					m_eof_flushed = false;

					::QueryPerformanceCounter(&qpc_t1);
					long long seek_us = (qpc_t1.QuadPart - qpc_t0.QuadPart) * 1000000LL / qpc_freq.QuadPart;

					m_segment_rt.store(m_pending_segment_rt);
					m_video_first_emit_pts_rt.store(LLONG_MIN);	  //다음 video frame push 시 set.
					int new_gen = m_seek_generation.fetch_add(1) + 1;
					//logWrite(_T("[ffi/dec] seek to %.0fms hr=%d gen=%d seg_rt=%lld seek_us=%lld"),
						//seek_pos, hr_seek, new_gen, (long long)m_pending_segment_rt, seek_us);

					//first frame 시간 측정용 — worker 가 다음 frame push 시점.
					m_first_frame_after_seek = false;
					m_seek_done_qpc = qpc_t1.QuadPart;

					//seek 처리 완료 — wait_seek_done() 동기 대기 중인 caller 에게 신호.
					{
						std::unique_lock<std::mutex> lkd(m_mtx_seek);
						m_seek_processed = true;
					}
					m_cv_seek_done.notify_all();
					continue;	//다음 iteration 부터 새 위치 디코드.
				}
			}

			//=== 2) queue 가득 차면 wait ===
			//video 또는 audio 둘 다 max 이상이면 wait. 둘 중 하나 비어 있으면 packet 읽고 그쪽 채움.
			{
				std::unique_lock<std::mutex> lk(m_mtx_queue);
				m_cv_queue.wait(lk, [this]() {
					return m_quit.load() ||
						   ((int)m_video_queue.size() < m_max_queue) ||
						   ((int)m_audio_queue.size() < m_max_audio_queue) ||
						   [this]() {
							   std::unique_lock<std::mutex> lks(m_mtx_seek);
							   return m_pending_seek_ms >= 0;
						   }();
				});
				if (m_quit.load())
					break;
			}

			//pending seek 다시 체크 — wait 중에 seek 가 들어왔으면 우선 처리.
			{
				std::unique_lock<std::mutex> lk(m_mtx_seek);
				if (m_pending_seek_ms >= 0)
					continue;
			}

			//=== 3) packet 읽기 ===
			int hr = av_read_frame(m_fmt, pkt);
			if (hr < 0)
			{
				//[keyframe 모드] forward keyframe 못 찾고 EOF 도달 (target 이 마지막 keyframe 이후) — fallback:
				//target GOP 시작 keyframe 으로 BACKWARD 재seek 후 정상 디코드 (target 직전 keyframe emit).
				//단 unreliable_video_pts(인덱스 없는 파일)는 pts 기반 av_seek 가 무효라 그냥 EOF 처리.
				if (m_kf_skip_active && !m_unreliable_video_pts)
				{
					m_kf_skip_active = false;
					av_seek_frame(m_fmt, m_video_stream_idx, m_kf_skip_target_pts, AVSEEK_FLAG_BACKWARD);
					avcodec_flush_buffers(m_video_ctx);
					if (m_audio_ctx)
						avcodec_flush_buffers(m_audio_ctx);
					m_eof.store(false);
					m_eof_flushed = false;
					//logWrite(_T("[ffi/dec/kf] forward keyframe not found before EOF → backward fallback target_pts=%lld"),
						//(long long)m_kf_skip_target_pts);
					continue;
				}
				//EOF — flag set 전에 디코더 reorder 버퍼를 flush(NULL packet drain)해 남은 끝 프레임을 큐에 push.
				//이게 없으면 HW(D3D11VA) h264 디코더가 들고 있던 마지막 수~수십 프레임이 유실 → 끝부분 재생 안 됨
				//(외부 플레이어는 EOF 시 flush 하므로 풀 재생). flush 는 EOF 당 1회만 (m_eof_flushed 가드, seek 시 리셋).
				if (!m_eof_flushed)
				{
					m_eof_flushed = true;

					if (m_video_ctx)
					{
						avcodec_send_packet(m_video_ctx, NULL);	//flush 신호.
						for (;;)
						{
							AVFrame* frame = av_frame_alloc();
							if (avcodec_receive_frame(m_video_ctx, frame) < 0)
							{
								av_frame_free(&frame);
								break;
							}

							AVFrame* out_frame = frame;
							if (m_hw_pix_fmt != AV_PIX_FMT_NONE && (AVPixelFormat)frame->format == m_hw_pix_fmt)
							{
								AVPixelFormat sw_fmt = AV_PIX_FMT_NV12;
								if (frame->hw_frames_ctx)
								{
									AVHWFramesContext* hwfc = (AVHWFramesContext*)frame->hw_frames_ctx->data;
									if (hwfc && hwfc->sw_format != AV_PIX_FMT_NONE)
										sw_fmt = hwfc->sw_format;
								}
								AVFrame* sw_frame = av_frame_alloc();
								sw_frame->format = sw_fmt;
								sw_frame->width	 = frame->width;
								sw_frame->height = frame->height;
								if (av_frame_get_buffer(sw_frame, 32) < 0)
								{
									av_frame_free(&sw_frame);
									av_frame_free(&frame);
									continue;
								}
								if (av_hwframe_transfer_data(sw_frame, frame, 0) >= 0)
								{
									sw_frame->pts	  = frame->pts;
									sw_frame->pkt_dts = frame->pkt_dts;
									sw_frame->flags	  = frame->flags;
									av_frame_free(&frame);
									out_frame = sw_frame;
								}
								else
								{
									av_frame_free(&sw_frame);
									av_frame_free(&frame);
									continue;
								}
							}

							out_frame->opaque = (void*)(intptr_t)m_seek_generation.load();
							{
								std::unique_lock<std::mutex> lk(m_mtx_queue);
								m_video_queue.push_back(out_frame);
							}
							m_cv_queue.notify_one();
						}
					}

					if (m_audio_ctx)
					{
						avcodec_send_packet(m_audio_ctx, NULL);
						for (;;)
						{
							AVFrame* frame = av_frame_alloc();
							if (avcodec_receive_frame(m_audio_ctx, frame) < 0)
							{
								av_frame_free(&frame);
								break;
							}
							{
								std::unique_lock<std::mutex> lk(m_mtx_queue);
								m_audio_queue.push_back(frame);
							}
							m_cv_queue.notify_one();
						}
					}
				}

				//EOF — flag set. queue 비면 source FillBuffer 가 S_FALSE 반환 → DeliverEndOfStream → renderer EC_COMPLETE 발화.
				//seek 가 오면 av_seek_frame 처리 시 clear.
				m_eof.store(true);
				std::this_thread::sleep_for(std::chrono::milliseconds(30));
				continue;
			}

			//[diag] packet 분포 — seek 후 video/audio 비율 확인. starvation 진단.
			static thread_local int64_t s_pkt_total = 0;
			static thread_local int64_t s_pkt_video = 0;
			static thread_local int64_t s_pkt_audio = 0;
			static thread_local int64_t s_pkt_other = 0;
			++s_pkt_total;
			if (pkt->stream_index == m_video_stream_idx)		++s_pkt_video;
			else if (pkt->stream_index == m_audio_stream_idx)	++s_pkt_audio;
			else												 ++s_pkt_other;
			if ((s_pkt_total % 200) == 0)
			{
				size_t vq, aq;
				{
					std::unique_lock<std::mutex> lk(m_mtx_queue);
					vq = m_video_queue.size();
					aq = m_audio_queue.size();
				}
				//logWrite(_T("[ffi/dec/diag] pkt total=%lld v=%lld a=%lld other=%lld | queue v=%zu a=%zu"),
					//(long long)s_pkt_total, (long long)s_pkt_video, (long long)s_pkt_audio, (long long)s_pkt_other,
					//vq, aq);
			}

			//[위치 스캔] seek 후 첫 frame 디코드 전까지(kf_skip 구간 + bound bail 후 손상 구간 통과 포함) 현재 읽는
			//video packet 시각을 발행 → 트랙바·시간이 복구 지점까지 전진하는 모습(멈춰 보여 freeze 오인 방지).
			//정상 파일은 container pts 가 단조라 전진. unreliable(garbage pts) 파일은 발행 안 함.
			if (m_pos_searching.load(std::memory_order_relaxed) && pkt->stream_index == m_video_stream_idx && !m_unreliable_video_pts)
			{
				int64_t ms_pts = (pkt->pts != AV_NOPTS_VALUE) ? pkt->pts : pkt->dts;
				if (ms_pts != AV_NOPTS_VALUE)
				{
					AVRational vtb = m_fmt->streams[m_video_stream_idx]->time_base;
					m_scan_pos_ms.store((int64_t)((double)ms_pts * av_q2d(vtb) * 1000.0), std::memory_order_relaxed);
				}
			}

			//[keyframe 모드 forward skip] BACKWARD seek 로 간 GOP 시작부터 demux 된 packet 을 디코드 없이 버리고,
			//pts>prev(=min_pts) 인 첫 video keyframe packet 부터 디코드 시작. GOP 전체 디코드(HW transfer 포함) 비용 제거.
			//min_pts = 직전 위치라 결과 keyframe 이 prev 보다 항상 전진 — forward step 제자리 회귀 방지.
			if (m_kf_skip_active)
			{
				//video 는 전진 keyframe 도달 전까지 디코드 없이 skip (GOP 전체 디코드 + HW transfer 비용 제거 = 7caa152 이득).
				if (pkt->stream_index == m_video_stream_idx)
				{
					int64_t ppts = (pkt->pts != AV_NOPTS_VALUE) ? pkt->pts : pkt->dts;
					bool is_ok_kf = (pkt->flags & AV_PKT_FLAG_KEY) && ppts != AV_NOPTS_VALUE && ppts > m_kf_skip_min_pts;
					if (!is_ok_kf)
					{
						//손상 구간 무한 skip 차단 — 정상 GOP 보다 훨씬 많이 skip 했는데도 유효 keyframe 이 없으면
						//(garbage 라 EOF 도 안 옴) skip 포기하고 정상 디코드로 전환. 이후 유효 데이터가 나오면 자연 복구(PotPlayer 식).
						if (++m_kf_skip_count > kf_skip_limit)
						{
							m_kf_skip_active = false;
							m_did_garbage_scan = true;	//손상 구간 확정 — 복구 시 audio decoder 재생성.
							//logWrite(_T("[ffi/dec/kf] skip limit(%lld) 초과 — 손상 구간 판단, 정상 디코드로 전환(무한 skip/freeze 방지)"),
								//(long long)kf_skip_limit);
							//이 packet 은 keyframe 이 아니므로 디코드해도 frame 안 나옴 → 버리고 다음 packet 부터 정상 경로.
							av_packet_unref(pkt);
							continue;
						}
						av_packet_unref(pkt);
						continue;	 //전진 video keyframe 도달 전 — video 만 skip.
					}
					//전진 keyframe 도달 — skip 종료. 이 packet 부터 정상 디코드 (아래로 fall through).
					m_kf_skip_active = false;
					//logWrite(_T("[ffi/dec/kf] keyframe reached pts=%lld min=%lld target=%lld pkt_pos=%lld → decode start"),
						//(long long)((pkt->pts != AV_NOPTS_VALUE) ? pkt->pts : pkt->dts),
						//(long long)m_kf_skip_min_pts, (long long)m_kf_skip_target_pts, (long long)pkt->pos);
				}
				//audio packet 은 skip 하지 않고 정상 디코드로 흘려보낸다 (아래 fall through).
				//[GOP 시작~target] 구간 audio 를 확보해야 audio 첫 emit 의 미디어 시점이 video_first 와 정렬되어
				//rtStart≈0 이 된다. 함께 버리면 audio 가 target 이후로 밀려 rtStart 가 최대 ~1초 → DSound 가
				//그만큼 audio 표시를 미뤄 트랙이동 후 오디오만 무음. video_first 미만 audio 는 FillBuffer 의
				//anchor skip 이 trim 하므로 A/V 정렬은 정확히 유지된다.
			}

			//=== 4) decode (video or audio) ===
			//push 직전 pending seek 체크 — caller seek() 가 큐 flush 한 후 worker 가 stale frame push 하는 race 차단.
			if (pkt->stream_index == m_video_stream_idx)
			{
				//[stall 재진입] searching 종료 후에도 video frame 이 video_stall_limit 만큼 연속 안 나오면(손상 keyframe
				//디코드 후 멈춤 = 시작부 손상 케이스) searching 재진입 → 스캔으로 다음 정상 구간 탐색(OSD + 트랙 전진).
				//정상 재생은 매 packet 마다 frame 이 나와 count 가 작게 유지되므로 발동 안 함(회귀 0).
				if (!m_pos_searching.load(std::memory_order_relaxed) && !m_kf_skip_active &&
					m_video_no_frame_count > video_stall_limit)
				{
					m_pos_searching.store(true, std::memory_order_relaxed);
					m_did_garbage_scan = true;
					m_video_no_frame_count = 0;
					//logWrite(_T("[ffi/dec/kf] 디코드 stall 감지(frame 연속 미생성) — searching 재진입(손상 구간 스캔)"));
				}

				hr = avcodec_send_packet(m_video_ctx, pkt);
				av_packet_unref(pkt);
				if (hr < 0)
					continue;
				++m_video_no_frame_count;	//frame 받으면 아래에서 0 으로 리셋. 연속 미생성만 누적.

				while (true)
				{
					AVFrame* frame = av_frame_alloc();
					hr = avcodec_receive_frame(m_video_ctx, frame);
					if (hr < 0)
					{
						av_frame_free(&frame);
						break;
					}

					bool pending;
					{
						std::unique_lock<std::mutex> lks(m_mtx_seek);
						pending = (m_pending_seek_ms >= 0);
					}
					if (pending)
					{
						av_frame_free(&frame);
						break;	 //다음 iteration 의 seek 처리 단계로.
					}

					//첫 유효 video frame 디코드 — 위치 스캔 종료. 이후 트랙바는 emit frame pts(last_emitted)로 정상 추적.
					m_video_no_frame_count = 0;	//frame 받음 — stall 카운트 리셋.
					bool was_garbage = m_did_garbage_scan;	//이번 seek 스캔이 손상 구간(bound bail / stall)을 거쳤나.
					m_pos_searching.store(false, std::memory_order_relaxed);

					//손상 구간을 길게 스캔한 뒤 복구 — AAC 디코더가 garbage 를 디코드하며 *채널 설정이 깨진다*(ch=29 등 →
					//무음 frame 생성). avcodec_flush_buffers 로는 채널 설정이 안 풀려 복구 후에도 영구 무음 → 컨텍스트를
					//*재생성*(free+alloc+open)해 codecpar(정상 2ch)로 완전 리셋. + 큐의 잔여 무음 frame 제거.
					//(정상 seek 은 m_kf_skip_count 가 작아 이 경로 안 탐 → 회귀 0.)
					if (was_garbage && m_audio_ctx && m_fmt && m_audio_stream_idx >= 0)
					{
						AVCodecParameters* apar = m_fmt->streams[m_audio_stream_idx]->codecpar;
						const AVCodec* acodec = avcodec_find_decoder(apar->codec_id);
						if (acodec)
						{
							avcodec_free_context(&m_audio_ctx);	//free + null
							m_audio_ctx = avcodec_alloc_context3(acodec);
							if (m_audio_ctx)
							{
								avcodec_parameters_to_context(m_audio_ctx, apar);
								if (avcodec_open2(m_audio_ctx, acodec, NULL) < 0)
									avcodec_free_context(&m_audio_ctx);
							}
							//logWrite(_T("[ffi/dec] 손상 후 audio decoder 재생성 (채널설정 복구)"));
						}
						std::unique_lock<std::mutex> lk(m_mtx_queue);
						while (!m_audio_queue.empty())
						{
							AVFrame* af = m_audio_queue.front();
							m_audio_queue.pop_front();
							av_frame_free(&af);
						}
					}
					m_did_garbage_scan = false;	//복구 처리 완료 — 다음 손상 스캔 위해 리셋.

					//HW frame 이면 CPU NV12 로 download. SW frame 이면 그대로.
					AVFrame* out_frame = frame;
					if (m_hw_pix_fmt != AV_PIX_FMT_NONE && (AVPixelFormat)frame->format == m_hw_pix_fmt)
					{
						//명시적 CPU buffer 할당 — device-managed pool 의 buffer 가 아닌 own buffer.
						//이 안 하면 transferred sw_frame->data 가 D3D11 device 관리 상태로 남아 swscale 와 device lock race → FillBuffer stuck.
						//sw_frame->format 을 frame->hw_frames_ctx 의 sw_format 으로 query — codec/미디어 마다 다름 (NV12 / P010 / NV21 등).
						//hard-coded NV12 가 다른 native sw_format 인 미디어 (mkv 일부 H.264 등) 에서 av_hwframe_transfer_data EINVAL fail 원인.
						AVPixelFormat sw_fmt = AV_PIX_FMT_NV12;
						if (frame->hw_frames_ctx)
						{
							AVHWFramesContext* hwfc = (AVHWFramesContext*)frame->hw_frames_ctx->data;
							if (hwfc && hwfc->sw_format != AV_PIX_FMT_NONE)
								sw_fmt = hwfc->sw_format;
						}

						AVFrame* sw_frame = av_frame_alloc();
						sw_frame->format = sw_fmt;
						sw_frame->width	 = frame->width;
						sw_frame->height = frame->height;
						int buf_hr = av_frame_get_buffer(sw_frame, 32);
						if (buf_hr < 0)
						{
							av_frame_free(&sw_frame);
							av_frame_free(&frame);
							continue;
						}

						LARGE_INTEGER qpc_freq, qpc_t0, qpc_t1;
						::QueryPerformanceFrequency(&qpc_freq);
						::QueryPerformanceCounter(&qpc_t0);
						int xfer = av_hwframe_transfer_data(sw_frame, frame, 0);
						::QueryPerformanceCounter(&qpc_t1);
						static thread_local int s_xfer_count = 0;
						++s_xfer_count;
						if (s_xfer_count <= 5 || s_xfer_count % 50 == 0)
						{
							long long us = (qpc_t1.QuadPart - qpc_t0.QuadPart) * 1000000LL / qpc_freq.QuadPart;
							//logWrite(_T("[ffi/dec/hw] transfer#%d xfer=%d us=%lld"), s_xfer_count, xfer, us);
						}
						if (xfer >= 0)
						{
							//pts / flags 보존
							sw_frame->pts = frame->pts;
							sw_frame->pkt_dts = frame->pkt_dts;
							sw_frame->flags = frame->flags;
							av_frame_free(&frame);
							out_frame = sw_frame;
						}
						else
						{
							av_frame_free(&sw_frame);
							av_frame_free(&frame);
							continue;	//skip this frame
						}
					}

					out_frame->opaque = (void*)(intptr_t)m_seek_generation.load();	 //generation tag

					//video_first_emit_pts_rt 는 *worker push 시점이 아닌* CFFiVideoStream::FillBuffer 의 *pre-target skip
					//후 실제 emit 한 첫 frame* 시점에 set 됨 (filter source 에서). worker 의 첫 push frame 은 keyframe
					//일 수 있어 target 이상 frame 과 미디어 시점 차이 (= GOP 길이).

					{
						std::unique_lock<std::mutex> lk(m_mtx_queue);
						m_video_queue.push_back(out_frame);
					}
					m_cv_queue.notify_one();

					//first frame 시간 측정 — seek 후 디코드 시작까지의 wallclock.
					if (!m_first_frame_after_seek)
					{
						LARGE_INTEGER qpc_freq, qpc_now;
						::QueryPerformanceFrequency(&qpc_freq);
						::QueryPerformanceCounter(&qpc_now);
						long long us = (qpc_now.QuadPart - m_seek_done_qpc) * 1000000LL / qpc_freq.QuadPart;
						logWrite(_T("[seek/diag] seek→first_video_frame us=%lld pts=%lld"),
							us, (long long)out_frame->pts);
						m_first_frame_after_seek = true;
					}
				}
			}
			else if (m_audio_ctx && pkt->stream_index == m_audio_stream_idx)
			{
				//packet 의 실제 pts(AVI 인덱스 기반 등) 를 unref 전에 보존. wmapro 등 일부 코덱은 frame->pts 를
				//안 채워(NOPTS) 내보내는데, 그러면 source filter 의 anchor skip(audio→video_anchor 정렬) 과
				//audio_sync 가 동작 못 해 키프레임 모드 backward-seek 시 audio 가 GOP 시작(video_anchor 보다 앞)부터
				//재생 → 위치별 가변 desync. packet 의 진짜 타임스탬프를 NOPTS frame 에 전달해 PTS 미디어와 동일 경로로.
				int64_t a_pkt_pts = (pkt->pts != AV_NOPTS_VALUE) ? pkt->pts : pkt->dts;
				hr = avcodec_send_packet(m_audio_ctx, pkt);
				av_packet_unref(pkt);
				if (hr < 0)
					continue;

				while (true)
				{
					AVFrame* frame = av_frame_alloc();
					hr = avcodec_receive_frame(m_audio_ctx, frame);
					if (hr < 0)
					{
						av_frame_free(&frame);
						break;
					}

					//코덱이 pts 를 안 준 frame 에만 packet pts 전달 (이미 있는 코덱은 절대 덮어쓰지 않음).
					//wmapro 등 AVI 오디오는 보통 packet 당 1 frame 이라 그대로 전달. 정밀도는 anchor skip(42ms 단위) 충분.
					if (frame->pts == AV_NOPTS_VALUE && a_pkt_pts != AV_NOPTS_VALUE)
						frame->pts = a_pkt_pts;

					bool pending;
					{
						std::unique_lock<std::mutex> lks(m_mtx_seek);
						pending = (m_pending_seek_ms >= 0);
					}
					if (pending)
					{
						av_frame_free(&frame);
						break;
					}

					frame->opaque = (void*)(intptr_t)m_seek_generation.load();	 //generation tag
					{
						std::unique_lock<std::mutex> lk(m_mtx_queue);
						m_audio_queue.push_back(frame);
					}
					m_cv_queue.notify_one();
				}
			}
			else
			{
				av_packet_unref(pkt);
			}
		}

		av_packet_free(&pkt);
	}
}
