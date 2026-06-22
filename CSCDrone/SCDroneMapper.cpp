// 이 프로젝트의 Common .cpp 들은 PCH 를 쓰지 않는다(vcxproj 에서 NotUsing). pch.h 를 include 하지 않음.
// SCDrone.h 의 <gdiplus.h> 가 windows 타입을 요구하므로, afxwin/gdiplus 를 끌어오는 SCD2Image.h 를 먼저 include.
#include "../directx/CSCD2Image/SCD2Image.h"
#include "SCDroneMapper.h"

#include <algorithm>
#include <cmath>
#include <unordered_set>

// (x,y) -> Hilbert 곡선 거리 d. 공간 국소성이 보존돼 정렬 후 1:1 매칭하면 모핑 경로가 안 엉킨다.
// n 은 2의 거듭제곱(격자 한 변). (Wikipedia xy2d, rot(s) 변형)
static long long hilbert_xy2d(int n, int x, int y)
{
	long long d = 0;
	for (int s = n / 2; s > 0; s /= 2)
	{
		const int rx = (x & s) > 0 ? 1 : 0;
		const int ry = (y & s) > 0 ? 1 : 0;
		d += static_cast<long long>(s) * s * ((3 * rx) ^ ry);
		if (ry == 0)
		{
			if (rx == 1) { x = s - 1 - x; y = s - 1 - y; }
			const int t = x; x = y; y = t;
		}
	}
	return d;
}

std::vector<CSCDrone> build_drones_from_image(CSCD2Image& img, int target_count, double edge_threshold, float world_extent)
{
	std::vector<CSCDrone> drones;

	const int W = static_cast<int>(img.get_width());
	const int H = static_cast<int>(img.get_height());
	if (W < 3 || H < 3 || target_count < 1)
		return drones;

	// 1) 그레이스케일 버퍼(Sobel 이웃 접근에 재사용).
	std::vector<unsigned char> gray(static_cast<size_t>(W) * H);
	for (int y = 0; y < H; ++y)
		for (int x = 0; x < W; ++x)
		{
			const Gdiplus::Color c = img.get_pixel(x, y);
			gray[static_cast<size_t>(y) * W + x] =
				static_cast<unsigned char>(0.299 * c.GetR() + 0.587 * c.GetG() + 0.114 * c.GetB());
		}

	// 2) Sobel 임계 통과 엣지 픽셀 수집.
	struct edge_px { int x, y; double mag; };
	std::vector<edge_px> edges;
	for (int y = 1; y < H - 1; ++y)
		for (int x = 1; x < W - 1; ++x)
		{
			const int i = y * W + x;
			const int gx = -gray[i - W - 1] - 2 * gray[i - 1] - gray[i + W - 1]
						 +  gray[i - W + 1] + 2 * gray[i + 1] + gray[i + W + 1];
			const int gy = -gray[i - W - 1] - 2 * gray[i - W] - gray[i - W + 1]
						 +  gray[i + W - 1] + 2 * gray[i + W] + gray[i + W + 1];
			const double mag = std::sqrt(static_cast<double>(gx) * gx + static_cast<double>(gy) * gy);
			if (mag >= edge_threshold)
				edges.push_back({ x, y, mag });
		}
	if (edges.empty())
		return drones;

	// 3) 고정 K 를 맞추기 위해 격자 간격(cell)을 이분탐색으로 선택.
	//    cell 이 커지면 격자당 1점만 남으므로 후보 수가 단조 감소 → K 이상 나오는 가장 큰 cell 선택.
	auto count_for = [&](int cell) -> int
	{
		std::unordered_set<long long> s;
		for (const edge_px& e : edges)
			s.insert(static_cast<long long>(e.y / cell) * 1000000LL + (e.x / cell));
		return static_cast<int>(s.size());
	};
	int cell = 2;
	{
		int lo = 2, hi = 60;
		while (lo <= hi)
		{
			const int mid = (lo + hi) / 2;
			if (count_for(mid) >= target_count) { cell = mid; lo = mid + 1; }
			else hi = mid - 1;
		}
	}

	// 4) 선택한 cell 로 격자당 최강점 1개 데시메이션.
	const int cols = (W + cell - 1) / cell;
	const int rows = (H + cell - 1) / cell;
	std::vector<double> best(static_cast<size_t>(cols) * rows, -1.0);
	std::vector<int> best_x(static_cast<size_t>(cols) * rows, 0);
	std::vector<int> best_y(static_cast<size_t>(cols) * rows, 0);
	for (const edge_px& e : edges)
	{
		const int ci = (e.y / cell) * cols + (e.x / cell);
		if (e.mag > best[ci]) { best[ci] = e.mag; best_x[ci] = e.x; best_y[ci] = e.y; }
	}

	// 5) 후보를 Hilbert 순서로 정렬(모핑 매칭 일관성). 좌표는 1024 격자로 정규화해 인덱싱.
	struct cand { int x, y; long long h; };
	std::vector<cand> cands;
	cands.reserve(best.size());
	const int HN = 1024;
	for (size_t k = 0; k < best.size(); ++k)
	{
		if (best[k] < 0.0) continue;
		const int px = best_x[k], py = best_y[k];
		const int hx = (std::min)(HN - 1, px * HN / W);
		const int hy = (std::min)(HN - 1, py * HN / H);
		cands.push_back({ px, py, hilbert_xy2d(HN, hx, hy) });
	}
	std::sort(cands.begin(), cands.end(), [](const cand& a, const cand& b) { return a.h < b.h; });

	// 6) Hilbert 순서를 유지한 채 정확히 K 개로 균등 추출 → 드론 생성.
	//    드론 i 는 모든 이미지에서 동일한 Hilbert 순위 → 이미지간 1:1 매칭이 성립(모핑 기반).
	const float s = 2.0f * world_extent / static_cast<float>((H > W) ? H : W);
	const float cx = W * 0.5f;
	const float cy = H * 0.5f;
	const float radius = cell * s * 0.25f;	// 드론 간격(cell*s)의 약 1/4 → 점이 또렷이 분리되게(이전 검증값)

	const int cnt = static_cast<int>(cands.size());
	drones.reserve(target_count);
	for (int i = 0; i < target_count; ++i)
	{
		int idx = static_cast<int>(static_cast<long long>(i) * cnt / target_count);
		if (idx >= cnt) idx = cnt - 1;
		const cand& c = cands[idx];

		// 원색을 그대로 저장(발광/hue 는 렌더에서 적용).
		const Gdiplus::Color sc = img.get_pixel(c.x, c.y);
		const SCDVec3 pos{ (c.x - cx) * s, -(c.y - cy) * s, 0.0f };	// 화면 y 반전
		drones.emplace_back(pos, Gdiplus::Color(255, sc.GetR(), sc.GetG(), sc.GetB()), radius);
	}

	return drones;
}
