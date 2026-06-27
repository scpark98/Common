#pragma once

#include <vector>
#include "SCDrone.h"

class CSCD2Image;

// 이미지 → 드론 포메이션 변환 알고리즘. 피사체(인물/사물·크기/매끈함)에 따라 적합한 방식이 달라
// scene 편집에서 고를 수 있어야 한다. .scn 에 선택을 저장(추후)하고, 미선택 시 edge 가 기본.
enum class drone_map_algorithm
{
	edge       = 0,	// 엣지(윤곽) 기반 블루노이즈. 사물/단순·큰 형상에 적합. 기본값.
	tone_fill  = 1,	// 엣지 + 톤 면채움(전경 한정). 인물 등 매끈한 면이 중요한 피사체에 적합.
	halftone   = 2,	// 정사각 격자 + 톤→점 크기. LED 패널 느낌. *드론 실 구현에는 크기가변이 부자연*.
	// 아래 5종은 드론 물리(크기 고정 + 색·밝기만 가변) 에 맞춘 시뮬레이션 알고리즘.
	bright_mod = 3,	// 균일 그리드 + 픽셀 색 sample + 명도 → 알파 변조. 실제 LED 드론쇼 표현에 가장 가까움.
	soft_edge  = 4,	// 균일 그리드 + 전경 마스크의 neighborhood 비율 → 부드러운 윤곽 fade(AA 원리).
	stipple    = 5,	// 명도가 가장 강한 top-N 픽셀에 점을 배치. 점 밀도가 형상.
	layered    = 6,	// 전경 마스크 안의 균일 그리드. 크기·알파 균일, 색만 픽셀 sample.
	hybrid     = 7,	// 엣지 contour(외곽 강조) + 내부 sparse 그리드(색 채움). edge + tone_fill 융합.
};

// 이미지를 *정확히 target_count 개* 의 드론으로 변환한다(고정 드론 수 = 모핑의 전제).
//   - 두 알고리즘 공통: 후보를 Hilbert 곡선 순서로 정렬해 이미지간 1:1 매칭(모핑) 성립,
//     색은 원색 저장(발광/hue 는 렌더 시점), z=0 평면 [-world_extent, world_extent] 정규화,
//     실 점이 K 미달이면 외곽 밀착 `unused` 드론으로 패딩.
//   - drone_map_algorithm::edge      : Sobel 엣지 블루노이즈(윤곽 위주).
//   - drone_map_algorithm::tone_fill : 윤곽(예산 상한) + 전경 톤 면채움(매끈한 면도 음영으로 채움).
//   - drone_map_algorithm::halftone  : 정사각 격자에 톤→점 크기 매핑(전경 한정, 규칙 격자 패널).
std::vector<CSCDrone> build_drones_from_image(CSCD2Image& img,
											   drone_map_algorithm algorithm = drone_map_algorithm::edge,
											   int target_count = 1500,
											   double edge_threshold = 60.0,
											   float world_extent = 200.f);
