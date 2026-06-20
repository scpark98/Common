#include "ffi_video_encoder.h"

extern "C" {
#include <libavutil/opt.h>
#include <libavutil/imgutils.h>
}

namespace ffi
{
	CFFiVideoEncoder::~CFFiVideoEncoder()
	{
		close();
	}

	bool CFFiVideoEncoder::open(const char* utf8_path, int width, int height, int fps)
	{
		close();
		if (!utf8_path || width < 2 || height < 2 || fps <= 0)
			return false;

		m_w = width & ~1;	//YUV420P 는 짝수 필요.
		m_h = height & ~1;
		m_fps = fps;

		if (avformat_alloc_output_context2(&m_fmt, NULL, NULL, utf8_path) < 0 || !m_fmt)
			return false;

		//H.264 우선(libx264 등) → 없으면 MPEG4(내장, 항상 가용). mp4 컨테이너는 둘 다 수용.
		const AVCodec* codec = avcodec_find_encoder(AV_CODEC_ID_H264);
		if (!codec)
			codec = avcodec_find_encoder(AV_CODEC_ID_MPEG4);
		if (!codec)
		{
			free_all();
			return false;
		}

		m_st = avformat_new_stream(m_fmt, NULL);
		if (!m_st)
		{
			free_all();
			return false;
		}

		m_cctx = avcodec_alloc_context3(codec);
		if (!m_cctx)
		{
			free_all();
			return false;
		}

		m_cctx->codec_id	= codec->id;
		m_cctx->width		= m_w;
		m_cctx->height		= m_h;
		m_cctx->time_base	= AVRational{ 1, m_fps };
		m_cctx->framerate	= AVRational{ m_fps, 1 };
		m_cctx->pix_fmt		= AV_PIX_FMT_YUV420P;
		m_cctx->gop_size	= 12;
		m_cctx->max_b_frames = 0;	//B-프레임 끔 — pts=dts, 재정렬/음수 dts 없음. 짧은 클립의 재생 호환성·끝부분 멈춤 방지.

		if (codec->id == AV_CODEC_ID_H264)
		{
			av_opt_set(m_cctx->priv_data, "preset", "veryfast", 0);
			av_opt_set(m_cctx->priv_data, "crf", "20", 0);
		}
		else
		{
			m_cctx->bit_rate = (int64_t)m_w * m_h * m_fps / 10;	//MPEG4 대략적 비트레이트.
		}

		//mp4 등 global header 요구 muxer.
		if (m_fmt->oformat->flags & AVFMT_GLOBALHEADER)
			m_cctx->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;

		if (avcodec_open2(m_cctx, codec, NULL) < 0)
		{
			free_all();
			return false;
		}
		if (avcodec_parameters_from_context(m_st->codecpar, m_cctx) < 0)
		{
			free_all();
			return false;
		}
		m_st->time_base = m_cctx->time_base;

		if (!(m_fmt->oformat->flags & AVFMT_NOFILE))
		{
			if (avio_open(&m_fmt->pb, utf8_path, AVIO_FLAG_WRITE) < 0)
			{
				free_all();
				return false;
			}
		}

		if (avformat_write_header(m_fmt, NULL) < 0)
		{
			free_all();
			return false;
		}
		m_header_written = true;

		m_frame = av_frame_alloc();
		if (!m_frame)
		{
			free_all();
			return false;
		}
		m_frame->format = AV_PIX_FMT_YUV420P;
		m_frame->width	= m_w;
		m_frame->height	= m_h;
		if (av_frame_get_buffer(m_frame, 0) < 0)
		{
			free_all();
			return false;
		}

		m_pkt = av_packet_alloc();
		m_next_pts = 0;
		return m_pkt != nullptr;
	}

	bool CFFiVideoEncoder::encode_bgra(const uint8_t* bgra, int stride)
	{
		if (!m_cctx || !m_frame || !bgra)
			return false;

		if (!m_sws)
		{
			m_sws = sws_getContext(m_w, m_h, AV_PIX_FMT_BGRA, m_w, m_h, AV_PIX_FMT_YUV420P,
				SWS_BILINEAR, NULL, NULL, NULL);
			if (!m_sws)
				return false;
		}

		if (av_frame_make_writable(m_frame) < 0)
			return false;

		const uint8_t* src[4]	= { bgra, NULL, NULL, NULL };
		int src_stride[4]		= { stride, 0, 0, 0 };
		sws_scale(m_sws, src, src_stride, 0, m_h, m_frame->data, m_frame->linesize);

		m_frame->pts = m_next_pts++;

		if (avcodec_send_frame(m_cctx, m_frame) < 0)
			return false;
		return drain();
	}

	bool CFFiVideoEncoder::drain()
	{
		for (;;)
		{
			int ret = avcodec_receive_packet(m_cctx, m_pkt);
			if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
				break;
			if (ret < 0)
				return false;

			av_packet_rescale_ts(m_pkt, m_cctx->time_base, m_st->time_base);
			m_pkt->stream_index = m_st->index;
			av_interleaved_write_frame(m_fmt, m_pkt);
			av_packet_unref(m_pkt);
		}
		return true;
	}

	bool CFFiVideoEncoder::close()
	{
		if (m_cctx && m_header_written)
		{
			avcodec_send_frame(m_cctx, NULL);	//flush 신호.
			drain();
		}
		if (m_fmt && m_header_written)
			av_write_trailer(m_fmt);

		free_all();
		return true;
	}

	void CFFiVideoEncoder::free_all()
	{
		if (m_sws)
		{
			sws_freeContext(m_sws);
			m_sws = nullptr;
		}
		if (m_frame)
			av_frame_free(&m_frame);
		if (m_pkt)
			av_packet_free(&m_pkt);
		if (m_cctx)
			avcodec_free_context(&m_cctx);
		if (m_fmt)
		{
			if (m_fmt->pb && !(m_fmt->oformat->flags & AVFMT_NOFILE))
				avio_closep(&m_fmt->pb);
			avformat_free_context(m_fmt);
			m_fmt = nullptr;
		}
		m_st = nullptr;
		m_header_written = false;
		m_next_pts = 0;
	}
}
