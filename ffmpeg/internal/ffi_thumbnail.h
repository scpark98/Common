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

		//time_ms 의 직전 키프레임 1장을 디코드해 out 에 채운다. target_width = 결과 폭(종횡비 유지, 짝수 정렬).
		//성공 시 true, 실패 시 false(out 미변경). 키프레임만 디코드라 정확 위치가 아닌 *근처* 프레임.
		bool	grab(double time_ms, int target_width, CSCGdiplusBitmap& out);

	private:
		void	free_sws();

		AVFormatContext*	m_fmt		= nullptr;
		AVCodecContext*		m_vctx		= nullptr;
		int					m_video_idx = -1;

		//sws context 캐시 — 같은 src/dst 조합이면 재사용 (hover 연속 호출 시 매번 alloc/free 회피).
		SwsContext*			m_sws		 = nullptr;
		int					m_sws_src_w	 = 0;
		int					m_sws_src_h	 = 0;
		int					m_sws_src_fmt = -1;
		int					m_sws_dst_w	 = 0;
		int					m_sws_dst_h	 = 0;
	};
}
