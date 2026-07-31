#pragma once

// APNG(및 정적 PNG) 디코더 — libpng(APNG 지원) 기반 공용 디코더.
// GDI+/Direct2D 에 비의존하도록 결과를 raw straight BGRA 프레임 + delay 로만 돌려준다.
// 이 프레임을 소비 측이 자기 비트맵 타입으로 감싼다:
//   - CSCGdiplusBitmap : BGRA → Gdiplus::Bitmap deque
//   - CSCD2Image       : BGRA → add_frame_from_raw (D2D)
// libpng 는 애니를 재생하지 못하므로(정적 디코드) fcTL offset + dispose/blend 를 여기서 합성한다.
// SC_USE_APNG 정의 + 벤더 libpng/zlib 소스가 빌드에 포함된 프로젝트에서만 링크된다.

#include <vector>
#include <cstdint>
#include <cstddef>

namespace sc_apng
{
	// data/size 의 PNG/APNG 를 디코드해 프레임 시퀀스를 만든다.
	//   w,h        : 캔버스(전체 이미지) 크기.
	//   frames[i]  : 캔버스 전체 straight(비-premultiplied) BGRA, 크기 w*h*4.
	//   delays_ms[i]: 프레임 i 표시시간(ms).
	// 정적 PNG 는 1프레임. 성공 시 true 이고 frames.size() >= 1.
	bool decode(const uint8_t* data, size_t size,
				int& w, int& h,
				std::vector<std::vector<uint8_t>>& frames,
				std::vector<int>& delays_ms);
}
