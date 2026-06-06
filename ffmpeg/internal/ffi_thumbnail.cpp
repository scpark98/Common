#include "ffi_thumbnail.h"

#include "../../SCGdiplusBitmap.h"
#include "../../log/SCLog/SCLog.h"

#include <windows.h>
#include <vector>

namespace ffi
{
	CFFiThumbnail::~CFFiThumbnail()
	{
		close();
	}

	bool CFFiThumbnail::open(const wchar_t* utf16_path)
	{
		close();

		//avformat 는 UTF-8 경로. ffi_decoder 는 custom AVIO 를 쓰지만 thumbnail 은 단순 동기 read 라 표준 file protocol 로 충분.
		char path_utf8[1024] = { 0 };
		if (WideCharToMultiByte(CP_UTF8, 0, utf16_path, -1, path_utf8, sizeof(path_utf8), NULL, NULL) == 0)
			return false;

		int hr = avformat_open_input(&m_fmt, path_utf8, NULL, NULL);
		if (hr < 0)
		{
			char buf[256];
			logWrite(_T("[ffi/thumb] open fail hr=%d (%hs)"), hr, err_str(hr, buf, sizeof(buf)));
			m_fmt = nullptr;
			return false;
		}

		if (avformat_find_stream_info(m_fmt, NULL) < 0)
		{
			close();
			return false;
		}

		m_video_idx = av_find_best_stream(m_fmt, AVMEDIA_TYPE_VIDEO, -1, -1, NULL, 0);
		if (m_video_idx < 0)
		{
			close();
			return false;
		}

		AVCodecParameters* par = m_fmt->streams[m_video_idx]->codecpar;
		const AVCodec* dec = avcodec_find_decoder(par->codec_id);
		if (!dec)
		{
			close();
			return false;
		}

		m_vctx = avcodec_alloc_context3(dec);
		if (!m_vctx)
		{
			close();
			return false;
		}
		avcodec_parameters_to_context(m_vctx, par);
		m_vctx->thread_count = 1;	//단일 프레임 — 멀티스레드 디코드 불필요(셋업 오버헤드만 늘어남).
		//썸네일 가속 — 작은 프리뷰라 화질 영향 미미: deblock loop filter 생략 + 비규격 허용 가속.
		m_vctx->skip_loop_filter = AVDISCARD_ALL;
		m_vctx->flags2 |= AV_CODEC_FLAG2_FAST;

		if (avcodec_open2(m_vctx, dec, NULL) < 0)
		{
			close();
			return false;
		}

		logWrite(_T("[ffi/thumb] open OK %dx%d codec=%hs dur=%.0fms"),
			m_vctx->width, m_vctx->height, dec->name, duration_ms());
		return true;
	}

	void CFFiThumbnail::close()
	{
		free_sws();
		if (m_vctx)
			avcodec_free_context(&m_vctx);
		if (m_fmt)
			avformat_close_input(&m_fmt);
		m_video_idx = -1;
	}

	double CFFiThumbnail::duration_ms() const
	{
		if (!m_fmt || m_fmt->duration == AV_NOPTS_VALUE)
			return 0.0;
		return (double)m_fmt->duration / AV_TIME_BASE * 1000.0;
	}

	bool CFFiThumbnail::grab(double time_ms, int target_width, CSCGdiplusBitmap& out)
	{
		if (!m_fmt || !m_vctx || m_video_idx < 0 || target_width < 2)
			return false;

		AVStream* st = m_fmt->streams[m_video_idx];
		int64_t ts = av_rescale_q((int64_t)(time_ms * 1000.0),
			AVRational{ 1, AV_TIME_BASE }, st->time_base);

		//직전 키프레임으로 seek 후 codec flush. forward decode 없이 그 키프레임 1장만 받음.
		if (av_seek_frame(m_fmt, m_video_idx, ts, AVSEEK_FLAG_BACKWARD) < 0)
			return false;
		avcodec_flush_buffers(m_vctx);

		AVPacket* pkt = av_packet_alloc();
		AVFrame*  frame = av_frame_alloc();
		if (!pkt || !frame)
		{
			av_frame_free(&frame);
			av_packet_free(&pkt);
			return false;
		}

		//hover 프리뷰는 속도 우선 — seek 한 직전 키프레임(<=ts) 1장만 디코드해 즉시 반환.
		//forward decode 로 정확 시각까지 진행하면 long-GOP 미디어에서 hover 마다 수백 프레임 디코드 → 느림.
		//keyframe 1장만 받으면 sparse keyframe 미디어에서 최대 GOP 길이만큼 어긋날 수 있으나 프리뷰 용도엔 충분.
		bool got = false;
		while (!got && av_read_frame(m_fmt, pkt) >= 0)
		{
			if (pkt->stream_index == m_video_idx && avcodec_send_packet(m_vctx, pkt) >= 0)
			{
				if (avcodec_receive_frame(m_vctx, frame) >= 0)
					got = true;	//첫 디코드 프레임(= 키프레임) 사용.
			}
			av_packet_unref(pkt);
		}

		bool ok = false;
		if (got && frame->width > 0 && frame->height > 0)
		{
			int dw = target_width & ~1;
			int dh = (int)((int64_t)dw * frame->height / frame->width) & ~1;
			if (dh < 2)
				dh = 2;

			if (!m_sws || m_sws_src_w != frame->width || m_sws_src_h != frame->height ||
				m_sws_src_fmt != frame->format || m_sws_dst_w != dw || m_sws_dst_h != dh)
			{
				free_sws();
				m_sws = sws_getContext(frame->width, frame->height, (AVPixelFormat)frame->format,
					dw, dh, AV_PIX_FMT_BGRA, SWS_FAST_BILINEAR, NULL, NULL, NULL);
				m_sws_src_w = frame->width;
				m_sws_src_h = frame->height;
				m_sws_src_fmt = frame->format;
				m_sws_dst_w = dw;
				m_sws_dst_h = dh;
			}

			if (m_sws)
			{
				int stride = dw * 4;
				std::vector<uint8_t> rgb((size_t)stride * dh);
				uint8_t* dst[4] = { rgb.data(), NULL, NULL, NULL };
				int dst_stride[4] = { stride, 0, 0, 0 };
				sws_scale(m_sws, frame->data, frame->linesize, 0, frame->height, dst, dst_stride);

				//top-down DIB(biHeight<0) + 32bpp BGRA → create_from_dib 가 Clone 으로 자체 픽셀 복사.
				BITMAPINFO bmi;
				ZeroMemory(&bmi, sizeof(bmi));
				bmi.bmiHeader.biSize		= sizeof(BITMAPINFOHEADER);
				bmi.bmiHeader.biWidth		= dw;
				bmi.bmiHeader.biHeight		= -dh;
				bmi.bmiHeader.biPlanes		= 1;
				bmi.bmiHeader.biBitCount	= 32;
				bmi.bmiHeader.biCompression = BI_RGB;
				ok = out.create_from_dib(&bmi, rgb.data());
			}
		}

		av_frame_free(&frame);
		av_packet_free(&pkt);
		return ok;
	}

	void CFFiThumbnail::free_sws()
	{
		if (m_sws)
		{
			sws_freeContext(m_sws);
			m_sws = nullptr;
		}
		m_sws_src_w = 0;
		m_sws_src_h = 0;
		m_sws_src_fmt = -1;
		m_sws_dst_w = 0;
		m_sws_dst_h = 0;
	}
}
