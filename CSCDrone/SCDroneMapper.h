#pragma once

#include <vector>
#include "SCDrone.h"

class CSCD2Image;

// 이미지를 *정확히 target_count 개* 의 드론으로 변환한다(고정 드론 수 = 모핑의 전제).
//   - Sobel 그래디언트로 엣지 추출(윤곽 + 이목구비 등 특징)
//   - 격자 간격(cell)을 이분탐색으로 골라 후보 수를 K 이상으로 맞춘 뒤 균등 데시메이션
//   - 후보를 Hilbert 곡선 순서로 정렬 후 정확히 K 개로 균등 추출 →
//     드론 i 가 모든 이미지에서 동일한 Hilbert 순위를 가져 이미지간 1:1 매칭이 성립(모핑 기반)
//   - 색은 원색을 그대로 저장(발광/hue 변환은 렌더 시점), z=0 평면에 [-world_extent, world_extent] 정규화
std::vector<CSCDrone> build_drones_from_image(CSCD2Image& img,
											   int target_count = 1500,
											   double edge_threshold = 60.0,
											   float world_extent = 200.f);
