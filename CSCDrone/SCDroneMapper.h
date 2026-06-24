#pragma once

#include <vector>
#include "SCDrone.h"

class CSCD2Image;

// 이미지 → 드론 포메이션 변환 알고리즘. 피사체(인물/사물·크기/매끈함)에 따라 적합한 방식이 달라
// scene 편집에서 고를 수 있어야 한다. .scn 에 선택을 저장(추후)하고, 미선택 시 edge 가 기본.
enum class drone_map_algorithm
{
	edge = 0,		// 엣지(윤곽) 기반 블루노이즈. 사물/단순·큰 형상에 적합. 기본값.
	tone_fill = 1,	// 엣지 + 톤 면채움(전경 한정). 인물 등 매끈한 면이 중요한 피사체에 적합.
};

// 이미지를 *정확히 target_count 개* 의 드론으로 변환한다(고정 드론 수 = 모핑의 전제).
//   - 두 알고리즘 공통: 후보를 Hilbert 곡선 순서로 정렬해 이미지간 1:1 매칭(모핑) 성립,
//     색은 원색 저장(발광/hue 는 렌더 시점), z=0 평면 [-world_extent, world_extent] 정규화,
//     실 점이 K 미달이면 외곽 밀착 `unused` 드론으로 패딩.
//   - drone_map_algorithm::edge      : Sobel 엣지 블루노이즈(윤곽 위주).
//   - drone_map_algorithm::tone_fill : 윤곽(예산 상한) + 전경 톤 면채움(매끈한 면도 음영으로 채움).
std::vector<CSCDrone> build_drones_from_image(CSCD2Image& img,
											   drone_map_algorithm algorithm = drone_map_algorithm::edge,
											   int target_count = 1500,
											   double edge_threshold = 60.0,
											   float world_extent = 200.f);
