#pragma once

/*
* ffi::CFFiVideoEncoder — BGRA 프레임 시퀀스를 H.264(없으면 MPEG4)로 인코딩하여 MP4 로 muxing.
*
* 내장 FFmpeg(libavcodec/libavformat/libswscale) 사용. Direct2D/GDI 불필요 — 순수 메모리→파일이라
* 호출 스레드(워커)에서 동기 인코딩 가능. 비디오 내보내기(mp4)에서 사용.
*
* lifecycle: open → encode_bgra(여러 번) → close.
*/

#include "ffmpeg_internal.h"
#include <cstdint>

namespace ffi
{
	class CFFiVideoEncoder
	{
	public:
		CFFiVideoEncoder() = default;
		~CFFiVideoEncoder();

		//utf8_path = 출력 .mp4 경로(확장자로 muxer 추론). width/height 는 짝수로 정렬됨. fps>0.
		//H.264 인코더 우선, 없으면 MPEG4. 둘 다 없으면 false.
		bool	open(const char* utf8_path, int width, int height, int fps);
		bool	is_opened() const { return m_fmt != nullptr; }

		//BGRA(32bpp, top-down) 한 프레임 인코딩. stride = 행당 바이트. 성공 true.
		bool	encode_bgra(const uint8_t* bgra, int stride);

		//flush + trailer 작성 + 자원 해제. 성공 true.
		bool	close();

	private:
		AVFormatContext*	m_fmt = nullptr;
		AVCodecContext*		m_cctx = nullptr;
		AVStream*			m_st = nullptr;
		SwsContext*			m_sws = nullptr;
		AVFrame*			m_frame = nullptr;	//YUV420P
		AVPacket*			m_pkt = nullptr;
		int					m_w = 0;
		int					m_h = 0;
		int					m_fps = 0;
		int64_t				m_next_pts = 0;
		bool				m_header_written = false;

		bool	drain();			//인코더에서 패킷을 빼 muxer 로 write.
		void	free_all();
	};
}
