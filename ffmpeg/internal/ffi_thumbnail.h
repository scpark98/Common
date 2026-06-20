#pragma once

/*
* ffi::CFFiThumbnail — 독립 단일-프레임 썸네일 추출기.
*
* 메인 재생 그래프(렌더러·디코더)와 *무관하게* 같은 파일을 별도로 열어, 임의 위치의
* 직전 키프레임 1장을 SW 디코드 → RGB 비트맵으로 반환한다. 트랙 hover 프리뷰용.
*
* 설계 의도:
*  - 메인 재생 경로를 전혀 건드리지 않아 격리적 (회귀 위험 낮음).
*  - DirectShow 렌더러(VMR9/MPC-VR)를 안 쓰고 메모리로만 디코드 → 과거 2nd-graph 방식의
*    cross-thread D3D 검정 프레임 문제 자체가 없음.
*  - 키프레임만 디코드(forward decode 없음)라 빠름. 작은 썸네일이라 저화질 OK.
*  - SW 디코드 — 메인 재생의 HW 디코더와 contention 회피.
*
* lifecycle: open → grab(여러 번) → close. grab 은 호출 스레드에서 동기 디코드하므로
*            UI 멈춤 방지를 위해 worker 스레드 + hover throttle 와 함께 쓴다(호출측 책임).
*/

#include "ffmpeg_internal.h"

class CSCGdiplusBitmap;

namespace ffi
{
	class CFFiThumbnail
	{
	public:
		CFFiThumbnail() = default;
		~CFFiThumbnail();

		//파일 열기 — video stream 탐색 + SW decoder open. 성공 시 true. 이미 열려 있으면 먼저 close.
		bool	open(const wchar_t* utf16_path);
		void	close();
		bool	is_opened() const { return m_fmt != nullptr; }

		//미디어 길이(ms). 모르면 0.
		double	duration_ms() const;

		//time_ms 위치 프레임 1장을 디코드해 out 에 채운다. target_width = 결과 폭(종횡비 유지, 짝수 정렬).
		//성공 시 true, 실패 시 false(out 미변경).
		//exact=false(기본): 직전 키프레임 1장만 디코드 → 빠르나 long-GOP 에선 *근처* 프레임(프리뷰 hover 용).
		//exact=true: 키프레임 seek 후 목표 pts 까지 forward decode → 정확 시각 프레임(멀티캡처/내보내기용, 느림).
		bool	grab(double time_ms, int target_width, CSCGdiplusBitmap& out, bool exact = false);

		//순차 추출 — 조밀·연속 시각(예: 비디오 내보내기의 fps 프레임)을 빠르게. seek_sequential 1회 후
		//target_ms 를 증가시키며 next_frame 반복. 각 소스 프레임을 1회만 디코드(매번 seek 하는 grab(exact) 보다 훨씬 빠름).
		bool	seek_sequential(double start_ms);
		bool	next_frame(double target_ms, int target_width, CSCGdiplusBitmap& out);

	private:
		void	free_sws();
		bool	scale_frame(AVFrame* frame, int target_width, CSCGdiplusBitmap& out);	//AVFrame → BGRA DIB(out).

		AVFormatContext*	m_fmt		= nullptr;
		AVCodecContext*		m_vctx		= nullptr;
		int					m_video_idx = -1;

		//순차 디코드(next_frame) 상태 — 호출 간 유지되는 packet/frame + EOF 플래그.
		AVPacket*			m_seq_pkt	= nullptr;
		AVFrame*			m_seq_frame	= nullptr;
		bool				m_seq_eof	= false;

		//sws context 캐시 — 같은 src/dst 조합이면 재사용 (hover 연속 호출 시 매번 alloc/free 회피).
		SwsContext*			m_sws		 = nullptr;
		int					m_sws_src_w	 = 0;
		int					m_sws_src_h	 = 0;
		int					m_sws_src_fmt = -1;
		int					m_sws_dst_w	 = 0;
		int					m_sws_dst_h	 = 0;
	};
}
