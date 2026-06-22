// 이 프로젝트의 Common .cpp 들은 PCH 를 쓰지 않는다(vcxproj 에서 NotUsing). pch.h 를 include 하지 않음.
// SCDrone.h 의 <gdiplus.h> 가 windows 타입을 요구하므로, afxwin/gdiplus 를 끌어오는 SCD2Image.h 를 먼저 include.
#include "../directx/CSCD2Image/SCD2Image.h"
#include "SCDroneMapper.h"

#include <algorithm>
#include <cmath>
#include <unordered_map>

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

// 픽셀 좌표 → Hilbert 거리. HN 격자(2의 거듭제곱)로 정규화. 패딩(음수/초과) 좌표는 정렬용으로만 클램프.
static long long hilbert_of(int W, int H, int HN, int px, int py)
{
	int hx = px * HN / W;
	int hy = py * HN / H;
	hx = (std::max)(0, (std::min)(HN - 1, hx));
	hy = (std::max)(0, (std::min)(HN - 1, hy));
	return hilbert_xy2d(HN, hx, hy);
}

std::vector<CSCDrone> build_drones_from_image(CSCD2Image& img, int target_count, double edge_threshold, float world_extent)
{
	std::vector<CSCDrone> drones;

	const int W = static_cast<int>(img.get_width());
	const int H = static_cast<int>(img.get_height());
	if (W < 3 || H < 3 || target_count < 1)
		return drones;

	// 1) 그레이스케일 + 알파 버퍼(Sobel 이웃 접근 / 콘텐츠 판정에 재사용).
	std::vector<unsigned char> gray(static_cast<size_t>(W) * H);
	std::vector<unsigned char> alpha(static_cast<size_t>(W) * H);
	for (int y = 0; y < H; ++y)
		for (int x = 0; x < W; ++x)
		{
			const Gdiplus::Color c = img.get_pixel(x, y);
			const size_t i = static_cast<size_t>(y) * W + x;
			gray[i] = static_cast<unsigned char>(0.299 * c.GetR() + 0.587 * c.GetG() + 0.114 * c.GetB());
			alpha[i] = c.GetA();
		}

	// 2) Sobel 임계 통과 엣지 픽셀 수집(크기 포함).
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

	const int HN = 1024;
	struct cand { int x, y; long long h; bool filler; };

	// 3) 엣지 픽셀 블루노이즈(최소거리 r) 샘플링.
	//    강한 엣지 우선 → 서로 r 이상 떨어진 것만 채택 → 균일 간격(겹침·이중선 제거, 빈 곳 최소).
	std::sort(edges.begin(), edges.end(), [](const edge_px& a, const edge_px& b) { return a.mag > b.mag; });

	auto sample_edges = [&](int r, std::vector<cand>* out) -> int
	{
		const double r2 = static_cast<double>(r) * r;
		const long long stride = 2000003LL;
		std::unordered_map<long long, std::vector<std::pair<int, int>>> hash;
		auto too_close = [&](int x, int y) -> bool
		{
			const int gx = x / r, gy = y / r;
			for (int dy = -1; dy <= 1; ++dy)
				for (int dx = -1; dx <= 1; ++dx)
				{
					auto it = hash.find(static_cast<long long>(gy + dy) * stride + (gx + dx));
					if (it == hash.end())
						continue;
					for (const std::pair<int, int>& p : it->second)
					{
						const double ddx = x - p.first;
						const double ddy = y - p.second;
						if (ddx * ddx + ddy * ddy < r2)
							return true;
					}
				}
			return false;
		};

		int n = 0;
		for (const edge_px& e : edges)
		{
			if (too_close(e.x, e.y))
				continue;
			hash[static_cast<long long>(e.y / r) * stride + (e.x / r)].push_back({ e.x, e.y });
			++n;
			if (out)
				out->push_back({ e.x, e.y, hilbert_of(W, H, HN, e.x, e.y), false });
		}
		return n;
	};

	// 채택 수 ≤ K 중 가장 조밀(가장 작은 r) → 윤곽에 K 예산을 최대한 사용. count(r) 은 r 증가 시 단조 감소.
	int r = 2;
	{
		int lo = 2, hi = 300, best = 300;
		while (lo <= hi)
		{
			const int mid = (lo + hi) / 2;
			if (sample_edges(mid, nullptr) <= target_count) { best = mid; hi = mid - 1; }
			else lo = mid + 1;
		}
		r = best;
	}

	std::vector<cand> reals;
	sample_edges(r, &reals);
	const int real_n = static_cast<int>(reals.size());

	const float s = 2.0f * world_extent / static_cast<float>((H > W) ? H : W);
	const float cx = W * 0.5f;
	const float cy = H * 0.5f;
	const float radius = r * s * 0.25f;	// 최소거리(r) 의 1/4 → 점이 또렷이 분리(겹침 없음)

	auto make_real = [&](const cand& c) -> CSCDrone
	{
		const Gdiplus::Color sc = img.get_pixel(c.x, c.y);
		const SCDVec3 pos{ (c.x - cx) * s, -(c.y - cy) * s, 0.0f };	// 화면 y 반전
		return CSCDrone(pos, Gdiplus::Color(255, sc.GetR(), sc.GetG(), sc.GetB()), radius);
	};

	// 윤곽이 K 를 다 채우면 그대로 K 개. 끝.
	if (real_n >= target_count)
	{
		std::sort(reals.begin(), reals.end(), [](const cand& a, const cand& b) { return a.h < b.h; });
		drones.reserve(target_count);
		for (int i = 0; i < target_count; ++i)
			drones.push_back(make_real(reals[i]));
		return drones;
	}

	// 4) 윤곽이 K 에 미달 → 모자란 만큼(need)을 여분(`unused`) 드론으로 채운다.
	const int need = target_count - real_n;

	// 여분 위치: *실제 사용된 드론(reals)* 에서 바깥으로 다중소스 BFS → 가장 가까운 빈 칸부터 need 개.
	// 콘텐츠 판정에 의존하지 않으므로 이미지 타입(투명/불투명) 불문 일관. 포메이션에 밀착 → 다음 모핑 때
	// 최소거리 이동(실물 드론: 충돌 확률↓, 배터리 절약). (alpha/색 배경추정은 불필요해져 제거.)
	const int ug = (r > 0) ? r : 2;	// 여분 드론 간격 = 윤곽 간격(과밀 방지)
	const int pad = (std::max)(W, H);
	const int gcols = (W + 2 * pad) / ug + 1;
	const int grows = (H + 2 * pad) / ug + 1;

	std::vector<char> visited(static_cast<size_t>(gcols) * grows, 0);
	std::vector<int> bfs;
	bfs.reserve(reals.size() + need + 16);
	for (const cand& rc : reals)	// 다중소스: 모든 사용 드론 셀에서 동시에 바깥으로 확장
	{
		const int gi = (rc.x + pad) / ug;
		const int gj = (rc.y + pad) / ug;
		const int idx = gj * gcols + gi;
		if (!visited[idx]) { visited[idx] = 1; bfs.push_back(idx); }
	}

	std::vector<cand> bg_cands;
	for (size_t head = 0; head < bfs.size() && static_cast<int>(bg_cands.size()) < need; ++head)
	{
		const int cur = bfs[head];
		const int ci = cur % gcols, cj = cur / gcols;
		for (int dy = -1; dy <= 1; ++dy)
			for (int dx = -1; dx <= 1; ++dx)
			{
				if (dx == 0 && dy == 0) continue;
				const int ni = ci + dx, nj = cj + dy;
				if (ni < 0 || ni >= gcols || nj < 0 || nj >= grows) continue;
				const int nidx = nj * gcols + ni;
				if (visited[nidx]) continue;
				visited[nidx] = 1;
				const int px = -pad + ni * ug, py = -pad + nj * ug;	// 사용 드론에 가까운 빈 칸 = 외곽 밀착
				bg_cands.push_back({ px, py, hilbert_of(W, H, HN, px, py), true });
				bfs.push_back(nidx);								// 더 바깥 링으로 BFS 계속
			}
	}

	// 한 칸의 이웃 8개를 한꺼번에 넣어 need 를 최대 7 초과할 수 있음 → 정확히 need 로 잘라 총 K 보장.
	// (이미지마다 개수가 달라지면 모핑 시 cur.size()!=built.size() 로 morph 가 통째 스킵된다.)
	if (static_cast<int>(bg_cands.size()) > need)
		bg_cands.resize(need);

	// 5) 윤곽(전부) + 외곽 밀착 배경(need 개) → 합쳐 Hilbert 정렬 → K 개(배경 부족 시 그 이하, 중복 없음).
	std::vector<cand> all = reals;
	all.insert(all.end(), bg_cands.begin(), bg_cands.end());
	std::sort(all.begin(), all.end(), [](const cand& a, const cand& b) { return a.h < b.h; });

	drones.reserve(all.size());
	for (const cand& c : all)
	{
		if (c.filler)
		{
			const SCDVec3 pos{ (c.x - cx) * s, -(c.y - cy) * s, 0.0f };
			CSCDrone d(pos, Gdiplus::Color(255, 255, 255, 255), radius);	// 흰색·정상 반지름(배치 확인용)
			d.set_unused(true);
			drones.push_back(d);
		}
		else
		{
			drones.push_back(make_real(c));
		}
	}

	return drones;
}
