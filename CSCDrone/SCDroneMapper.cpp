// 이 프로젝트의 Common .cpp 들은 PCH 를 쓰지 않는다(vcxproj 에서 NotUsing). pch.h 를 include 하지 않음.
// SCDrone.h 의 <gdiplus.h> 가 windows 타입을 요구하므로, afxwin/gdiplus 를 끌어오는 SCD2Image.h 를 먼저 include.
#include "../directx/CSCD2Image/SCD2Image.h"
#include "SCDroneMapper.h"

#include <algorithm>
#include <cmath>
#include <unordered_map>
#include <utility>

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

// 두 알고리즘 공통 후보. filler=true 면 외곽 밀착 `unused` 패딩 점.
struct map_cand { int x, y; long long h; bool filler; };

// 실 점(reals) 미달 시 외곽 밀착 BFS 로 need 개의 `unused` 패딩을 만들어 정확히 K 를 맞춘다.
// 실 점 셀에서 다중소스로 바깥 확장 → 포메이션에 밀착(모핑 이동 최소). 이미지 타입 불문 일관.
static std::vector<map_cand> make_filler_padding(const std::vector<map_cand>& reals, int need,
												  int W, int H, int HN, int ug)
{
	std::vector<map_cand> bg_cands;
	if (need <= 0)
		return bg_cands;
	if (ug <= 0)
		ug = 2;
	const int pad = (std::max)(W, H);
	const int gcols = (W + 2 * pad) / ug + 1;
	const int grows = (H + 2 * pad) / ug + 1;

	std::vector<char> visited(static_cast<size_t>(gcols) * grows, 0);
	std::vector<int> bfs;
	bfs.reserve(reals.size() + need + 16);
	for (const map_cand& rc : reals)	// 다중소스: 모든 사용 드론 셀에서 동시에 바깥으로 확장
	{
		const int gi = (rc.x + pad) / ug;
		const int gj = (rc.y + pad) / ug;
		const int idx = gj * gcols + gi;
		if (!visited[idx]) { visited[idx] = 1; bfs.push_back(idx); }
	}

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
	if (static_cast<int>(bg_cands.size()) > need)
		bg_cands.resize(need);
	return bg_cands;
}

// ============================================================================
// 알고리즘 1: 엣지(윤곽) 기반 블루노이즈. (원본 구현 — 사물/단순·큰 형상에 적합)
// ============================================================================
static std::vector<CSCDrone> build_edge(CSCD2Image& img, int target_count, double edge_threshold, float world_extent)
{
	std::vector<CSCDrone> drones;

	const int W = static_cast<int>(img.get_width());
	const int H = static_cast<int>(img.get_height());
	if (W < 3 || H < 3 || target_count < 1)
		return drones;

	// 1) 그레이스케일 버퍼(Sobel 이웃 접근).
	std::vector<unsigned char> gray(static_cast<size_t>(W) * H);
	for (int y = 0; y < H; ++y)
		for (int x = 0; x < W; ++x)
		{
			const Gdiplus::Color c = img.get_pixel(x, y);
			const size_t i = static_cast<size_t>(y) * W + x;
			gray[i] = static_cast<unsigned char>(0.299 * c.GetR() + 0.587 * c.GetG() + 0.114 * c.GetB());
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

	// 3) 엣지 픽셀 블루노이즈(최소거리 r) 샘플링.
	//    강한 엣지 우선 → 서로 r 이상 떨어진 것만 채택 → 균일 간격(겹침·이중선 제거, 빈 곳 최소).
	std::sort(edges.begin(), edges.end(), [](const edge_px& a, const edge_px& b) { return a.mag > b.mag; });

	auto sample_edges = [&](int r, std::vector<map_cand>* out) -> int
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

	std::vector<map_cand> reals;
	sample_edges(r, &reals);
	const int real_n = static_cast<int>(reals.size());

	const float s = 2.0f * world_extent / static_cast<float>((H > W) ? H : W);
	const float cx = W * 0.5f;
	const float cy = H * 0.5f;
	const float radius = r * s * 0.25f;	// 최소거리(r) 의 1/4 → 점이 또렷이 분리(겹침 없음)

	auto make_real = [&](const map_cand& c) -> CSCDrone
	{
		const Gdiplus::Color sc = img.get_pixel(c.x, c.y);
		const SCDVec3 pos{ (c.x - cx) * s, -(c.y - cy) * s, 0.0f };	// 화면 y 반전
		return CSCDrone(pos, Gdiplus::Color(255, sc.GetR(), sc.GetG(), sc.GetB()), radius);
	};

	// 윤곽이 K 를 다 채우면 그대로 K 개. 끝.
	if (real_n >= target_count)
	{
		std::sort(reals.begin(), reals.end(), [](const map_cand& a, const map_cand& b) { return a.h < b.h; });
		drones.reserve(target_count);
		for (int i = 0; i < target_count; ++i)
			drones.push_back(make_real(reals[i]));
		return drones;
	}

	// 4) 윤곽이 K 에 미달 → 모자란 만큼을 여분(`unused`) 드론으로 채운다.
	const std::vector<map_cand> bg_cands = make_filler_padding(reals, target_count - real_n, W, H, HN, r);

	// 5) 윤곽(전부) + 외곽 밀착 배경 → Hilbert 정렬 → K 개(배경 부족 시 그 이하, 중복 없음).
	std::vector<map_cand> all = reals;
	all.insert(all.end(), bg_cands.begin(), bg_cands.end());
	std::sort(all.begin(), all.end(), [](const map_cand& a, const map_cand& b) { return a.h < b.h; });

	drones.reserve(all.size());
	for (const map_cand& c : all)
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

// ============================================================================
// 알고리즘 2: 엣지(예산 상한) + 톤 면채움. (인물 등 매끈한 면이 중요한 피사체에 적합)
// ============================================================================

// 분리형 박스 블러(반경 r). 슬라이딩 합으로 O(N). 머리카락 등 미세 텍스처를 뭉개 엣지가 과도하게
// 잡히는 것을 막는 용도. 경계는 클램프.
static std::vector<unsigned char> box_blur(const std::vector<unsigned char>& src, int W, int H, int r)
{
	if (r < 1)
		return src;
	const int win = 2 * r + 1;
	std::vector<unsigned char> tmp(src.size()), dst(src.size());

	for (int y = 0; y < H; ++y)
	{
		const int row = y * W;
		int sum = 0;
		for (int x = -r; x <= r; ++x)
			sum += src[row + (std::max)(0, (std::min)(W - 1, x))];
		for (int x = 0; x < W; ++x)
		{
			tmp[row + x] = static_cast<unsigned char>(sum / win);
			const int add = (std::max)(0, (std::min)(W - 1, x + r + 1));
			const int sub = (std::max)(0, (std::min)(W - 1, x - r));
			sum += src[row + add] - src[row + sub];
		}
	}
	for (int x = 0; x < W; ++x)
	{
		int sum = 0;
		for (int y = -r; y <= r; ++y)
			sum += tmp[(std::max)(0, (std::min)(H - 1, y)) * W + x];
		for (int y = 0; y < H; ++y)
		{
			dst[y * W + x] = static_cast<unsigned char>(sum / win);
			const int add = (std::max)(0, (std::min)(H - 1, y + r + 1));
			const int sub = (std::max)(0, (std::min)(H - 1, y - r));
			sum += tmp[add * W + x] - tmp[sub * W + x];
		}
	}
	return dst;
}

// 가변 반지름 Poisson-disk 용 공간 해시. is_free 로 반지름 내 기존 점 유무를 보고, add 로 채택점을 등록.
// cell 은 질의 최소 반지름에 맞춘다(질의 반지름 q 면 ceil(q/cell) 링까지 스캔).
struct poisson_grid
{
	double cell;
	long long stride;
	std::unordered_map<long long, std::vector<std::pair<int, int>>> grid;

	explicit poisson_grid(double c) : cell(c < 1.0 ? 1.0 : c), stride(2000003LL) {}

	long long key(int gx, int gy) const { return static_cast<long long>(gy) * stride + gx; }

	bool is_free(int x, int y, double radius) const
	{
		const double r2 = radius * radius;
		const int gx = static_cast<int>(std::floor(x / cell));
		const int gy = static_cast<int>(std::floor(y / cell));
		const int rad = static_cast<int>(std::ceil(radius / cell));
		for (int dy = -rad; dy <= rad; ++dy)
			for (int dx = -rad; dx <= rad; ++dx)
			{
				auto it = grid.find(key(gx + dx, gy + dy));
				if (it == grid.end())
					continue;
				for (const std::pair<int, int>& p : it->second)
				{
					const double ddx = x - p.first;
					const double ddy = y - p.second;
					if (ddx * ddx + ddy * ddy < r2)
						return false;
				}
			}
		return true;
	}

	void add(int x, int y)
	{
		const int gx = static_cast<int>(std::floor(x / cell));
		const int gy = static_cast<int>(std::floor(y / cell));
		grid[key(gx, gy)].push_back({ x, y });
	}
};

static std::vector<CSCDrone> build_tone_fill(CSCD2Image& img, int target_count, double edge_threshold, float world_extent)
{
	std::vector<CSCDrone> drones;

	const int W = static_cast<int>(img.get_width());
	const int H = static_cast<int>(img.get_height());
	if (W < 3 || H < 3 || target_count < 1)
		return drones;
	const size_t N = static_cast<size_t>(W) * H;

	// 1) 그레이/알파/RGB 버퍼. 전경 판정·톤·색을 위해 RGB 도 보관.
	std::vector<unsigned char> gray(N), alpha(N), rbuf(N), gbuf(N), bbuf(N);
	bool has_alpha = false;
	for (int y = 0; y < H; ++y)
		for (int x = 0; x < W; ++x)
		{
			const Gdiplus::Color c = img.get_pixel(x, y);
			const size_t i = static_cast<size_t>(y) * W + x;
			rbuf[i] = c.GetR(); gbuf[i] = c.GetG(); bbuf[i] = c.GetB();
			gray[i] = static_cast<unsigned char>(0.299 * c.GetR() + 0.587 * c.GetG() + 0.114 * c.GetB());
			alpha[i] = c.GetA();
			if (c.GetA() < 250)
				has_alpha = true;
		}

	// 2) 엣지 계산용으로 그레이를 블러 → 머리카락 미세 텍스처 억제. 톤(면 채움)은 원본 gray 사용.
	const std::vector<unsigned char> blur = box_blur(gray, W, H, 2);

	// 3) 전경 마스크. 알파가 있으면 알파, 없으면 테두리 평균색(배경)과의 색거리로 판정.
	//    드론쇼는 피사체만 표현 → 매끈한 배경에 면 채움 점이 깔리지 않도록 전경으로 한정.
	std::vector<unsigned char> fg(N, 1);
	if (has_alpha)
	{
		for (size_t i = 0; i < N; ++i)
			fg[i] = alpha[i] > 127 ? 1 : 0;
	}
	else
	{
		double sr = 0, sg = 0, sb = 0; long bc = 0;
		auto acc = [&](int x, int y) { const size_t i = static_cast<size_t>(y) * W + x; sr += rbuf[i]; sg += gbuf[i]; sb += bbuf[i]; ++bc; };
		for (int x = 0; x < W; ++x) { acc(x, 0); acc(x, H - 1); }
		for (int y = 1; y < H - 1; ++y) { acc(0, y); acc(W - 1, y); }
		const double bgR = sr / bc, bgG = sg / bc, bgB = sb / bc;
		const double thr2 = 48.0 * 48.0;	// 배경색과 이 정도 떨어지면 전경
		for (size_t i = 0; i < N; ++i)
		{
			const double dr = rbuf[i] - bgR, dg = gbuf[i] - bgG, db = bbuf[i] - bgB;
			fg[i] = (dr * dr + dg * dg + db * db) > thr2 ? 1 : 0;
		}
	}

	const int HN = 1024;

	// ---- Pool A: 윤곽(엣지) ----------------------------------------------------
	// 블러 그레이에 Sobel. 임계 통과 + (전경 또는 전경 인접=실루엣)인 픽셀만 → 배경 텍스처 배제.
	struct edge_px { int x, y; double mag; };
	std::vector<edge_px> edges;
	for (int y = 1; y < H - 1; ++y)
		for (int x = 1; x < W - 1; ++x)
		{
			const int i = y * W + x;
			const int gx = -blur[i - W - 1] - 2 * blur[i - 1] - blur[i + W - 1]
						 +  blur[i - W + 1] + 2 * blur[i + 1] + blur[i + W + 1];
			const int gy = -blur[i - W - 1] - 2 * blur[i - W] - blur[i - W + 1]
						 +  blur[i + W - 1] + 2 * blur[i + W] + blur[i + W + 1];
			const double mag = std::sqrt(static_cast<double>(gx) * gx + static_cast<double>(gy) * gy);
			if (mag < edge_threshold)
				continue;
			const bool near_fg = fg[i] || fg[i - 1] || fg[i + 1] || fg[i - W] || fg[i + W];
			if (near_fg)
				edges.push_back({ x, y, mag });
		}
	std::sort(edges.begin(), edges.end(), [](const edge_px& a, const edge_px& b) { return a.mag > b.mag; });

	// 윤곽이 예산을 독식하지 못하게 K_edge 로 상한. 남는 예산은 얼굴 등 면 채움(Pool B)으로.
	const int K_edge = static_cast<int>(std::lround(target_count * 0.40));

	auto count_edge = [&](int r) -> int
	{
		poisson_grid g(r);
		int n = 0;
		for (const edge_px& e : edges)
			if (g.is_free(e.x, e.y, r)) { g.add(e.x, e.y); ++n; }
		return n;
	};

	int r_edge = 2;
	if (!edges.empty())
	{
		int lo = 2, hi = 300, best = 300;
		while (lo <= hi)
		{
			const int mid = (lo + hi) / 2;
			if (count_edge(mid) <= K_edge) { best = mid; hi = mid - 1; }
			else lo = mid + 1;
		}
		r_edge = best;
	}

	std::vector<map_cand> reals;
	std::vector<std::pair<int, int>> edge_pts;	// Pool B 가 윤곽과 겹치지 않게 제외용
	{
		poisson_grid g(r_edge);
		for (const edge_px& e : edges)
			if (g.is_free(e.x, e.y, r_edge))
			{
				g.add(e.x, e.y);
				reals.push_back({ e.x, e.y, hilbert_of(W, H, HN, e.x, e.y), false });
				edge_pts.push_back({ e.x, e.y });
			}
	}
	const int edge_n = static_cast<int>(reals.size());

	// ---- Pool B: 톤 기반 면 채움 ---------------------------------------------
	// 전경 픽셀을 밝기(톤)에 비례한 밀도로 채운다. 매끈한 얼굴도 톤만큼 점이 깔려 음영이 산다.
	// 가변 반지름 r_fill(톤) = f·(R_LO + (R_HI-R_LO)·(1-tone)). 밝을수록 조밀.
	const int K_fill = target_count - edge_n;	// 윤곽이 쓴 만큼 빼고 나머지 실 예산 전부

	struct fcand { int x, y; double tone; };
	std::vector<fcand> fills;
	if (K_fill > 0)
	{
		const int STEP = 2;	// 후보 데시메이션(최소 간격 ≥ 2 라 손실 없음). 탐색 비용 절감.
		for (int y = 0; y < H; y += STEP)
			for (int x = 0; x < W; x += STEP)
			{
				const size_t i = static_cast<size_t>(y) * W + x;
				if (!fg[i])
					continue;
				const double tone = std::pow(gray[i] / 255.0, 0.85);	// 미드톤을 살짝 끌어올림
				fills.push_back({ x, y, tone });
			}
		std::sort(fills.begin(), fills.end(), [](const fcand& a, const fcand& b) { return a.tone > b.tone; });
	}

	const double R_LO = 2.0, R_HI = 6.0;
	auto fill_radius = [&](double f, double tone) { return f * (R_LO + (R_HI - R_LO) * (1.0 - tone)); };

	auto count_fill = [&](double f) -> int
	{
		poisson_grid g(f * R_LO);
		for (const std::pair<int, int>& p : edge_pts)	// 윤곽 점 먼저 등록 → 면 채움이 윤곽을 피함
			g.add(p.first, p.second);
		int n = 0;
		for (const fcand& c : fills)
		{
			const double rad = fill_radius(f, c.tone);
			if (g.is_free(c.x, c.y, rad)) { g.add(c.x, c.y); ++n; }
		}
		return n;
	};

	if (!fills.empty() && K_fill > 0)
	{
		double lo = 0.2, hi = 40.0;
		for (int it = 0; it < 24; ++it)	// count(f) 단조 감소 → count ≤ K_fill 되는 최소 f
		{
			const double mid = (lo + hi) * 0.5;
			if (count_fill(mid) <= K_fill) hi = mid;
			else lo = mid;
		}
		const double f = hi;

		poisson_grid g(f * R_LO);
		for (const std::pair<int, int>& p : edge_pts)
			g.add(p.first, p.second);
		for (const fcand& c : fills)
		{
			const double rad = fill_radius(f, c.tone);
			if (g.is_free(c.x, c.y, rad))
			{
				g.add(c.x, c.y);
				reals.push_back({ c.x, c.y, hilbert_of(W, H, HN, c.x, c.y), false });
			}
		}
	}

	const int real_n = static_cast<int>(reals.size());
	if (real_n == 0)
		return drones;

	const float s = 2.0f * world_extent / static_cast<float>((H > W) ? H : W);
	const float cx = W * 0.5f;
	const float cy = H * 0.5f;
	const float radius = r_edge * s * 0.25f;	// 윤곽 간격(r_edge)의 1/4 → 점이 또렷이 분리

	auto make_real = [&](const map_cand& c) -> CSCDrone
	{
		const size_t i = static_cast<size_t>(c.y) * W + c.x;
		const SCDVec3 pos{ (c.x - cx) * s, -(c.y - cy) * s, 0.0f };	// 화면 y 반전
		return CSCDrone(pos, Gdiplus::Color(255, rbuf[i], gbuf[i], bbuf[i]), radius);
	};

	// 실 점이 K 를 넘으면(반올림/탐색 오차) Hilbert 균등 데시메이션으로 정확히 K.
	if (real_n >= target_count)
	{
		std::sort(reals.begin(), reals.end(), [](const map_cand& a, const map_cand& b) { return a.h < b.h; });
		drones.reserve(target_count);
		for (int i = 0; i < target_count; ++i)
			drones.push_back(make_real(reals[static_cast<size_t>(i) * real_n / target_count]));
		return drones;
	}

	// 실 점이 K 미달 → 모자란 만큼을 외곽 밀착 `unused` 드론으로 채운다.
	const std::vector<map_cand> bg_cands = make_filler_padding(reals, target_count - real_n, W, H, HN, r_edge);

	std::vector<map_cand> all = reals;
	all.insert(all.end(), bg_cands.begin(), bg_cands.end());
	std::sort(all.begin(), all.end(), [](const map_cand& a, const map_cand& b) { return a.h < b.h; });

	drones.reserve(all.size());
	for (const map_cand& c : all)
	{
		if (c.filler)
		{
			const SCDVec3 pos{ (c.x - cx) * s, -(c.y - cy) * s, 0.0f };
			CSCDrone d(pos, Gdiplus::Color(255, 255, 255, 255), radius);
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

// ============================================================================
// 알고리즘 3: 하프톤(정사각 격자 + 톤→점 크기). (LED 패널 느낌, 모핑 시 점이 거의 안 움직임)
// ============================================================================
static std::vector<CSCDrone> build_halftone(CSCD2Image& img, int target_count, double /*edge_threshold*/, float world_extent)
{
	std::vector<CSCDrone> drones;

	const int W = static_cast<int>(img.get_width());
	const int H = static_cast<int>(img.get_height());
	if (W < 3 || H < 3 || target_count < 1)
		return drones;
	const size_t N = static_cast<size_t>(W) * H;

	// 1) 그레이/알파/RGB 버퍼.
	std::vector<unsigned char> gray(N), alpha(N), rbuf(N), gbuf(N), bbuf(N);
	bool has_alpha = false;
	for (int y = 0; y < H; ++y)
		for (int x = 0; x < W; ++x)
		{
			const Gdiplus::Color c = img.get_pixel(x, y);
			const size_t i = static_cast<size_t>(y) * W + x;
			rbuf[i] = c.GetR(); gbuf[i] = c.GetG(); bbuf[i] = c.GetB();
			gray[i] = static_cast<unsigned char>(0.299 * c.GetR() + 0.587 * c.GetG() + 0.114 * c.GetB());
			alpha[i] = c.GetA();
			if (c.GetA() < 250)
				has_alpha = true;
		}

	// 2) 전경 마스크(tone_fill 과 동일). 배경 격자에는 점을 두지 않는다.
	std::vector<unsigned char> fg(N, 1);
	if (has_alpha)
	{
		for (size_t i = 0; i < N; ++i)
			fg[i] = alpha[i] > 127 ? 1 : 0;
	}
	else
	{
		double sr = 0, sg = 0, sb = 0; long bc = 0;
		auto acc = [&](int x, int y) { const size_t i = static_cast<size_t>(y) * W + x; sr += rbuf[i]; sg += gbuf[i]; sb += bbuf[i]; ++bc; };
		for (int x = 0; x < W; ++x) { acc(x, 0); acc(x, H - 1); }
		for (int y = 1; y < H - 1; ++y) { acc(0, y); acc(W - 1, y); }
		const double bgR = sr / bc, bgG = sg / bc, bgB = sb / bc;
		const double thr2 = 48.0 * 48.0;
		for (size_t i = 0; i < N; ++i)
		{
			const double dr = rbuf[i] - bgR, dg = gbuf[i] - bgG, db = bbuf[i] - bgB;
			fg[i] = (dr * dr + dg * dg + db * db) > thr2 ? 1 : 0;
		}
	}

	const int HN = 1024;

	// 3) 격자 간격 g(정사각). 전경 셀(셀 중심이 전경) 수 ≤ K 가 되는 최소 g = 가장 조밀한 해상도.
	auto count_cells = [&](int g) -> int
	{
		int n = 0;
		for (int cy = g / 2; cy < H; cy += g)
			for (int cx = g / 2; cx < W; cx += g)
				if (fg[static_cast<size_t>(cy) * W + cx])
					++n;
		return n;
	};

	int g = 4;
	{
		int lo = 2, hi = 400, best = 400;
		while (lo <= hi)
		{
			const int mid = (lo + hi) / 2;
			if (count_cells(mid) <= target_count) { best = mid; hi = mid - 1; }
			else lo = mid + 1;
		}
		g = best;
	}

	const float s = 2.0f * world_extent / static_cast<float>((H > W) ? H : W);
	const float cx = W * 0.5f;
	const float cy = H * 0.5f;
	const float r_max = g * s * 0.5f;	// 격자 한 칸의 절반 = 점이 거의 맞닿는 최대 반지름

	// 4) 각 전경 셀: 셀 평균 톤→점 크기, 셀 평균색. 점 위치는 격자 중심(이미지가 바뀌어도 격자가 같으면 이동 0).
	struct hd { long long h; CSCDrone d; };
	std::vector<hd> reals;
	std::vector<map_cand> reals_xy;	// 외곽 패딩 BFS 시드용
	const int half = g / 2;
	for (int yy = half; yy < H; yy += g)
		for (int xx = half; xx < W; xx += g)
		{
			if (!fg[static_cast<size_t>(yy) * W + xx])
				continue;

			long sr = 0, sg = 0, sb = 0, sl = 0, cnt = 0;
			for (int dy = -half; dy <= half; ++dy)
				for (int dx = -half; dx <= half; ++dx)
				{
					const int px = xx + dx, py = yy + dy;
					if (px < 0 || px >= W || py < 0 || py >= H)
						continue;
					const size_t pi = static_cast<size_t>(py) * W + px;
					if (!fg[pi])
						continue;
					sr += rbuf[pi]; sg += gbuf[pi]; sb += bbuf[pi]; sl += gray[pi]; ++cnt;
				}
			if (cnt == 0)
				continue;

			const double tone = std::pow((sl / static_cast<double>(cnt)) / 255.0, 0.85);	// 밝을수록 큰 점
			const float radius = r_max * static_cast<float>(0.20 + 0.80 * tone);
			const Gdiplus::Color col(255,
				static_cast<BYTE>(sr / cnt), static_cast<BYTE>(sg / cnt), static_cast<BYTE>(sb / cnt));
			const SCDVec3 pos{ (xx - cx) * s, -(yy - cy) * s, 0.0f };
			const long long h = hilbert_of(W, H, HN, xx, yy);
			reals.push_back({ h, CSCDrone(pos, col, radius) });
			reals_xy.push_back({ xx, yy, h, false });
		}

	if (reals.empty())
		return drones;

	// 5) K 미달 → 외곽 밀착 `unused` 패딩(간격 = 격자 g). 작은 흰 점(소등 상태).
	const std::vector<map_cand> bg_cands =
		make_filler_padding(reals_xy, target_count - static_cast<int>(reals.size()), W, H, HN, g);

	std::vector<hd> all = std::move(reals);
	for (const map_cand& c : bg_cands)
	{
		const SCDVec3 pos{ (c.x - cx) * s, -(c.y - cy) * s, 0.0f };
		CSCDrone d(pos, Gdiplus::Color(255, 255, 255, 255), r_max * 0.3f);
		d.set_unused(true);
		all.push_back({ c.h, d });
	}
	std::sort(all.begin(), all.end(), [](const hd& a, const hd& b) { return a.h < b.h; });

	drones.reserve(all.size());
	for (const hd& e : all)
		drones.push_back(e.d);
	return drones;
}

// ============================================================================
// 드론 물리(크기 고정 + 색·밝기 가변) 친화 알고리즘 5종 (build_halftone 패턴 변형).
// 공통 helper(hilbert_of / make_filler_padding / map_cand) 재사용.
// ============================================================================

// 픽셀 → 전경 마스크 + 그레이/RGB 버퍼 계산. halftone 등에서 쓰는 패턴을 함수로 분리하지 않고 인라인.
// (코드 중복 최소화 위해 lambda 로 캡슐화하는 패턴은 추후 리팩토링 — 일단 prototype.)

// 1) bright_mod — 균일 그리드 + 픽셀 색 + 명도 → 알파.
//    halftone 베이스, radius 균일(r_max*0.7), color.a 가 셀 평균 명도에 비례.
//    실제 LED 드론쇼에서 *드론 크기 고정·발광 강도 가변* 표현과 1:1.
static std::vector<CSCDrone> build_bright_mod(CSCD2Image& img, int target_count, double /*edge_threshold*/, float world_extent)
{
	std::vector<CSCDrone> drones;
	const int W = (int)img.get_width(), H = (int)img.get_height();
	if (W < 3 || H < 3 || target_count < 1) return drones;
	const size_t N = (size_t)W * H;

	std::vector<unsigned char> gray(N), rbuf(N), gbuf(N), bbuf(N), alpha(N), fg(N, 1);
	bool has_alpha = false;
	for (int y = 0; y < H; ++y)
		for (int x = 0; x < W; ++x)
		{
			const Gdiplus::Color c = img.get_pixel(x, y);
			const size_t i = (size_t)y * W + x;
			rbuf[i] = c.GetR(); gbuf[i] = c.GetG(); bbuf[i] = c.GetB();
			gray[i] = (unsigned char)(0.299 * c.GetR() + 0.587 * c.GetG() + 0.114 * c.GetB());
			alpha[i] = c.GetA();
			if (c.GetA() < 250) has_alpha = true;
		}
	// 전경 마스크 (alpha 우선, 없으면 배경색 추정).
	if (has_alpha)
	{
		for (size_t i = 0; i < N; ++i) fg[i] = alpha[i] > 127 ? 1 : 0;
	}
	else
	{
		double sr=0, sg=0, sb=0; long bc=0;
		auto acc=[&](int x,int y){ const size_t i=(size_t)y*W+x; sr+=rbuf[i]; sg+=gbuf[i]; sb+=bbuf[i]; ++bc; };
		for (int x=0;x<W;++x){ acc(x,0); acc(x,H-1); }
		for (int y=1;y<H-1;++y){ acc(0,y); acc(W-1,y); }
		const double bgR=sr/bc, bgG=sg/bc, bgB=sb/bc;
		const double thr2=48.0*48.0;
		for (size_t i=0;i<N;++i){ const double dr=rbuf[i]-bgR, dg=gbuf[i]-bgG, db=bbuf[i]-bgB; fg[i] = (dr*dr+dg*dg+db*db) > thr2 ? 1 : 0; }
	}

	const int HN = 1024;
	// 격자 간격 — halftone 과 동일 알고리즘으로 K 안에 들어가는 최소 g.
	auto count_cells = [&](int g) { int n=0; for (int cy=g/2; cy<H; cy+=g) for (int cx=g/2; cx<W; cx+=g) if (fg[(size_t)cy*W+cx]) ++n; return n; };
	int g = 4;
	{ int lo=2, hi=400, best=400; while (lo<=hi){ const int mid=(lo+hi)/2; if (count_cells(mid)<=target_count){ best=mid; hi=mid-1; } else lo=mid+1; } g=best; }

	const float s = 2.0f * world_extent / (float)((H>W)?H:W);
	const float cxf = W*0.5f, cyf = H*0.5f;
	const float r_max = g * s * 0.5f;
	const float radius_fixed = r_max * 0.7f;	// 균일 크기(드론 물리)

	struct hd { long long h; CSCDrone d; };
	std::vector<hd> reals;
	std::vector<map_cand> reals_xy;
	const int half = g / 2;
	for (int yy = half; yy < H; yy += g)
		for (int xx = half; xx < W; xx += g)
		{
			if (!fg[(size_t)yy*W+xx]) continue;
			long sr=0, sg=0, sb=0, sl=0, cnt=0;
			for (int dy=-half; dy<=half; ++dy) for (int dx=-half; dx<=half; ++dx)
			{
				const int px=xx+dx, py=yy+dy;
				if (px<0||px>=W||py<0||py>=H) continue;
				const size_t pi=(size_t)py*W+px;
				if (!fg[pi]) continue;
				sr+=rbuf[pi]; sg+=gbuf[pi]; sb+=bbuf[pi]; sl+=gray[pi]; ++cnt;
			}
			if (cnt==0) continue;
			const double brightness = (sl / (double)cnt) / 255.0;	// 0~1
			// 알파 = brightness, 단 최저 0.15 (너무 어두워도 약간 발광 — 형상 보존).
			const BYTE a = (BYTE)(255 * (std::max)(0.15, brightness));
			const Gdiplus::Color col(a, (BYTE)(sr/cnt), (BYTE)(sg/cnt), (BYTE)(sb/cnt));
			const SCDVec3 pos{ (xx-cxf)*s, -(yy-cyf)*s, 0.0f };
			const long long h = hilbert_of(W, H, HN, xx, yy);
			reals.push_back({ h, CSCDrone(pos, col, radius_fixed) });
			reals_xy.push_back({ xx, yy, h, false });
		}
	if (reals.empty()) return drones;

	const std::vector<map_cand> bg_cands = make_filler_padding(reals_xy, target_count - (int)reals.size(), W, H, HN, g);
	std::vector<hd> all = std::move(reals);
	for (const map_cand& c : bg_cands)
	{
		const SCDVec3 pos{ (c.x-cxf)*s, -(c.y-cyf)*s, 0.0f };
		CSCDrone d(pos, Gdiplus::Color(255, 255, 255, 255), radius_fixed * 0.5f);
		d.set_unused(true);
		all.push_back({ c.h, d });
	}
	std::sort(all.begin(), all.end(), [](const hd& a, const hd& b){ return a.h < b.h; });
	drones.reserve(all.size());
	for (const hd& e : all) drones.push_back(e.d);
	return drones;
}

// 2) soft_edge — 균일 그리드 + neighborhood fg ratio → 알파(부드러운 윤곽).
//    fg 0/1 binary 대신 5x5 neighborhood 의 fg 비율로 알파 결정.
//    경계 셀 = 비율 ~0.5 → 알파 ~0.5 → fade. AA 의 sub-pixel coverage 원리.
static std::vector<CSCDrone> build_soft_edge(CSCD2Image& img, int target_count, double /*edge_threshold*/, float world_extent)
{
	std::vector<CSCDrone> drones;
	const int W = (int)img.get_width(), H = (int)img.get_height();
	if (W < 3 || H < 3 || target_count < 1) return drones;
	const size_t N = (size_t)W * H;

	std::vector<unsigned char> rbuf(N), gbuf(N), bbuf(N), alpha(N), fg(N, 1);
	bool has_alpha = false;
	for (int y = 0; y < H; ++y)
		for (int x = 0; x < W; ++x)
		{
			const Gdiplus::Color c = img.get_pixel(x, y);
			const size_t i = (size_t)y*W+x;
			rbuf[i]=c.GetR(); gbuf[i]=c.GetG(); bbuf[i]=c.GetB(); alpha[i]=c.GetA();
			if (c.GetA()<250) has_alpha=true;
		}
	if (has_alpha) { for (size_t i=0;i<N;++i) fg[i] = alpha[i]>127?1:0; }
	else
	{
		double sr=0,sg=0,sb=0; long bc=0;
		auto acc=[&](int x,int y){ const size_t i=(size_t)y*W+x; sr+=rbuf[i]; sg+=gbuf[i]; sb+=bbuf[i]; ++bc; };
		for (int x=0;x<W;++x){ acc(x,0); acc(x,H-1); }
		for (int y=1;y<H-1;++y){ acc(0,y); acc(W-1,y); }
		const double bgR=sr/bc, bgG=sg/bc, bgB=sb/bc;
		const double thr2=48.0*48.0;
		for (size_t i=0;i<N;++i){ const double dr=rbuf[i]-bgR, dg=gbuf[i]-bgG, db=bbuf[i]-bgB; fg[i]=(dr*dr+dg*dg+db*db)>thr2?1:0; }
	}

	const int HN = 1024;
	// 격자 — soft_edge 는 외부 fade 까지 그리므로 fg 셀 + neighborhood 점수 > 0 셀 모두 카운트.
	auto count_cells = [&](int g) {
		int n=0;
		for (int cy=g/2; cy<H; cy+=g) for (int cx=g/2; cx<W; cx+=g)
		{
			int fc=0, tc=0;
			for (int dy=-2; dy<=2; ++dy) for (int dx=-2; dx<=2; ++dx) {
				const int px=cx+dx, py=cy+dy;
				if (px<0||px>=W||py<0||py>=H) continue;
				if (fg[(size_t)py*W+px]) ++fc; ++tc;
			}
			if (fc > 0) ++n;
		}
		return n;
	};
	int g = 4;
	{ int lo=2, hi=400, best=400; while (lo<=hi){ const int mid=(lo+hi)/2; if (count_cells(mid)<=target_count){ best=mid; hi=mid-1; } else lo=mid+1; } g=best; }

	const float s = 2.0f * world_extent / (float)((H>W)?H:W);
	const float cxf = W*0.5f, cyf = H*0.5f;
	const float r_max = g * s * 0.5f;
	const float radius_fixed = r_max * 0.7f;

	struct hd { long long h; CSCDrone d; };
	std::vector<hd> reals;
	std::vector<map_cand> reals_xy;
	const int half = g/2;
	for (int yy=half; yy<H; yy+=g) for (int xx=half; xx<W; xx+=g)
	{
		int fc=0, tc=0;
		long sr=0, sg=0, sb=0, cnt=0;
		for (int dy=-2; dy<=2; ++dy) for (int dx=-2; dx<=2; ++dx)
		{
			const int px=xx+dx, py=yy+dy;
			if (px<0||px>=W||py<0||py>=H) continue;
			const size_t pi=(size_t)py*W+px;
			if (fg[pi]) { ++fc; sr+=rbuf[pi]; sg+=gbuf[pi]; sb+=bbuf[pi]; ++cnt; }
			++tc;
		}
		if (fc == 0 || cnt == 0) continue;
		const double soft = (double)fc / tc;	// 0~1, 경계는 ~0.5, 깊은 안쪽은 1.0
		const BYTE a = (BYTE)(255 * soft);
		const Gdiplus::Color col(a, (BYTE)(sr/cnt), (BYTE)(sg/cnt), (BYTE)(sb/cnt));
		const SCDVec3 pos{ (xx-cxf)*s, -(yy-cyf)*s, 0.0f };
		const long long h = hilbert_of(W, H, HN, xx, yy);
		reals.push_back({ h, CSCDrone(pos, col, radius_fixed) });
		reals_xy.push_back({ xx, yy, h, false });
	}
	if (reals.empty()) return drones;

	const std::vector<map_cand> bg_cands = make_filler_padding(reals_xy, target_count - (int)reals.size(), W, H, HN, g);
	std::vector<hd> all = std::move(reals);
	for (const map_cand& c : bg_cands)
	{
		const SCDVec3 pos{ (c.x-cxf)*s, -(c.y-cyf)*s, 0.0f };
		CSCDrone d(pos, Gdiplus::Color(255, 255, 255, 255), radius_fixed*0.5f);
		d.set_unused(true);
		all.push_back({ c.h, d });
	}
	std::sort(all.begin(), all.end(), [](const hd& a, const hd& b){ return a.h < b.h; });
	drones.reserve(all.size());
	for (const hd& e : all) drones.push_back(e.d);
	return drones;
}

// 3) stipple — 명도 기반 상위 K 픽셀 선택(점 밀도가 형상).
//    프로토타입: gray 값으로 정렬, 상위 target_count 픽셀에 점 배치.
//    정식 dithering (Floyd-Steinberg) 은 후속 — 일단 단순 buckets / random sample.
static std::vector<CSCDrone> build_stipple(CSCD2Image& img, int target_count, double /*edge_threshold*/, float world_extent)
{
	std::vector<CSCDrone> drones;
	const int W = (int)img.get_width(), H = (int)img.get_height();
	if (W < 3 || H < 3 || target_count < 1) return drones;
	const size_t N = (size_t)W * H;

	std::vector<unsigned char> gray(N), rbuf(N), gbuf(N), bbuf(N);
	for (int y=0; y<H; ++y) for (int x=0; x<W; ++x)
	{
		const Gdiplus::Color c = img.get_pixel(x, y);
		const size_t i = (size_t)y*W+x;
		rbuf[i]=c.GetR(); gbuf[i]=c.GetG(); bbuf[i]=c.GetB();
		gray[i] = (unsigned char)(0.299*c.GetR() + 0.587*c.GetG() + 0.114*c.GetB());
	}

	// stride 로 픽셀 sub-sample → 그 중 명도 높은 상위 K. 모든 픽셀 정렬은 비용 큼.
	const int stride = (std::max)(1, (int)std::sqrt((double)N / (target_count * 4)));	// 후보 ~4K
	std::vector<std::pair<int, int>> cand;	// (gray, idx)
	cand.reserve((W/stride) * (H/stride) + 16);
	for (int y=0; y<H; y+=stride) for (int x=0; x<W; x+=stride)
		cand.push_back({ -(int)gray[(size_t)y*W+x], (int)((size_t)y*W+x) });	// 음수 → 큰 명도 먼저
	std::sort(cand.begin(), cand.end());
	const int take = (std::min)(target_count, (int)cand.size());

	const int HN = 1024;
	const float s = 2.0f * world_extent / (float)((H>W)?H:W);
	const float cxf = W*0.5f, cyf = H*0.5f;
	const float radius_fixed = (stride * s) * 0.4f;

	struct hd { long long h; CSCDrone d; };
	std::vector<hd> reals;
	std::vector<map_cand> reals_xy;
	for (int k=0; k<take; ++k)
	{
		const int idx = cand[k].second;
		const int xx = idx % W, yy = idx / W;
		const size_t pi = (size_t)idx;
		const Gdiplus::Color col(255, rbuf[pi], gbuf[pi], bbuf[pi]);
		const SCDVec3 pos{ (xx-cxf)*s, -(yy-cyf)*s, 0.0f };
		const long long h = hilbert_of(W, H, HN, xx, yy);
		reals.push_back({ h, CSCDrone(pos, col, radius_fixed) });
		reals_xy.push_back({ xx, yy, h, false });
	}
	if (reals.empty()) return drones;

	const std::vector<map_cand> bg_cands = make_filler_padding(reals_xy, target_count - (int)reals.size(), W, H, HN, stride);
	std::vector<hd> all = std::move(reals);
	for (const map_cand& c : bg_cands)
	{
		const SCDVec3 pos{ (c.x-cxf)*s, -(c.y-cyf)*s, 0.0f };
		CSCDrone d(pos, Gdiplus::Color(255, 255, 255, 255), radius_fixed*0.5f);
		d.set_unused(true);
		all.push_back({ c.h, d });
	}
	std::sort(all.begin(), all.end(), [](const hd& a, const hd& b){ return a.h < b.h; });
	drones.reserve(all.size());
	for (const hd& e : all) drones.push_back(e.d);
	return drones;
}

// 4) layered — 전경 마스크 안 균일 그리드. radius·alpha 균일, 색만 픽셀 sample.
//    bright_mod 의 *알파 변조 없는* 단순 on/off 버전.
static std::vector<CSCDrone> build_layered(CSCD2Image& img, int target_count, double /*edge_threshold*/, float world_extent)
{
	std::vector<CSCDrone> drones;
	const int W = (int)img.get_width(), H = (int)img.get_height();
	if (W < 3 || H < 3 || target_count < 1) return drones;
	const size_t N = (size_t)W*H;

	std::vector<unsigned char> rbuf(N), gbuf(N), bbuf(N), alpha(N), fg(N, 1);
	bool has_alpha = false;
	for (int y=0; y<H; ++y) for (int x=0; x<W; ++x)
	{
		const Gdiplus::Color c = img.get_pixel(x, y);
		const size_t i = (size_t)y*W+x;
		rbuf[i]=c.GetR(); gbuf[i]=c.GetG(); bbuf[i]=c.GetB(); alpha[i]=c.GetA();
		if (c.GetA()<250) has_alpha=true;
	}
	if (has_alpha) { for (size_t i=0;i<N;++i) fg[i] = alpha[i]>127?1:0; }
	else
	{
		double sr=0,sg=0,sb=0; long bc=0;
		auto acc=[&](int x,int y){ const size_t i=(size_t)y*W+x; sr+=rbuf[i]; sg+=gbuf[i]; sb+=bbuf[i]; ++bc; };
		for (int x=0;x<W;++x){ acc(x,0); acc(x,H-1); }
		for (int y=1;y<H-1;++y){ acc(0,y); acc(W-1,y); }
		const double bgR=sr/bc, bgG=sg/bc, bgB=sb/bc;
		const double thr2=48.0*48.0;
		for (size_t i=0;i<N;++i){ const double dr=rbuf[i]-bgR, dg=gbuf[i]-bgG, db=bbuf[i]-bgB; fg[i]=(dr*dr+dg*dg+db*db)>thr2?1:0; }
	}

	const int HN = 1024;
	auto count_cells = [&](int g) { int n=0; for (int cy=g/2; cy<H; cy+=g) for (int cx=g/2; cx<W; cx+=g) if (fg[(size_t)cy*W+cx]) ++n; return n; };
	int g = 4;
	{ int lo=2, hi=400, best=400; while (lo<=hi){ const int mid=(lo+hi)/2; if (count_cells(mid)<=target_count){ best=mid; hi=mid-1; } else lo=mid+1; } g=best; }

	const float s = 2.0f * world_extent / (float)((H>W)?H:W);
	const float cxf = W*0.5f, cyf = H*0.5f;
	const float r_max = g * s * 0.5f;
	const float radius_fixed = r_max * 0.7f;

	struct hd { long long h; CSCDrone d; };
	std::vector<hd> reals;
	std::vector<map_cand> reals_xy;
	const int half = g/2;
	for (int yy=half; yy<H; yy+=g) for (int xx=half; xx<W; xx+=g)
	{
		if (!fg[(size_t)yy*W+xx]) continue;
		long sr=0,sg=0,sb=0, cnt=0;
		for (int dy=-half; dy<=half; ++dy) for (int dx=-half; dx<=half; ++dx)
		{
			const int px=xx+dx, py=yy+dy;
			if (px<0||px>=W||py<0||py>=H) continue;
			const size_t pi=(size_t)py*W+px;
			if (!fg[pi]) continue;
			sr+=rbuf[pi]; sg+=gbuf[pi]; sb+=bbuf[pi]; ++cnt;
		}
		if (cnt==0) continue;
		const Gdiplus::Color col(255, (BYTE)(sr/cnt), (BYTE)(sg/cnt), (BYTE)(sb/cnt));	// 알파 풀, on/off 만
		const SCDVec3 pos{ (xx-cxf)*s, -(yy-cyf)*s, 0.0f };
		const long long h = hilbert_of(W, H, HN, xx, yy);
		reals.push_back({ h, CSCDrone(pos, col, radius_fixed) });
		reals_xy.push_back({ xx, yy, h, false });
	}
	if (reals.empty()) return drones;

	const std::vector<map_cand> bg_cands = make_filler_padding(reals_xy, target_count - (int)reals.size(), W, H, HN, g);
	std::vector<hd> all = std::move(reals);
	for (const map_cand& c : bg_cands)
	{
		const SCDVec3 pos{ (c.x-cxf)*s, -(c.y-cyf)*s, 0.0f };
		CSCDrone d(pos, Gdiplus::Color(255, 255, 255, 255), radius_fixed*0.5f);
		d.set_unused(true);
		all.push_back({ c.h, d });
	}
	std::sort(all.begin(), all.end(), [](const hd& a, const hd& b){ return a.h < b.h; });
	drones.reserve(all.size());
	for (const hd& e : all) drones.push_back(e.d);
	return drones;
}

// 5) hybrid — edge contour(외곽) + 내부 sparse 그리드(색 채움).
//    프로토타입: edge 결과 70% + 내부 그리드 30% mix. 정확한 비율은 사용자 결과 보고 조율.
static std::vector<CSCDrone> build_hybrid(CSCD2Image& img, int target_count, double edge_threshold, float world_extent)
{
	const int edge_budget = (int)(target_count * 0.6);
	const int fill_budget = target_count - edge_budget;
	// edge 점들 (forward declaration 없이 같은 .cpp 안이라 직접 호출 가능).
	std::vector<CSCDrone> edge_drones = build_edge(img, edge_budget, edge_threshold, world_extent);
	std::vector<CSCDrone> fill_drones = build_layered(img, fill_budget, edge_threshold, world_extent);

	// 단순 concat — 두 결과를 합치고 Hilbert 재정렬은 생략(prototype). 모핑 품질은 sub-optimal 일 수 있음.
	std::vector<CSCDrone> all;
	all.reserve(edge_drones.size() + fill_drones.size());
	for (const auto& d : edge_drones) all.push_back(d);
	for (const auto& d : fill_drones) all.push_back(d);
	return all;
}

// ============================================================================
// 디스패처
// ============================================================================
std::vector<CSCDrone> build_drones_from_image(CSCD2Image& img, drone_map_algorithm algorithm,
											   int target_count, double edge_threshold, float world_extent)
{
	switch (algorithm)
	{
	case drone_map_algorithm::tone_fill:  return build_tone_fill (img, target_count, edge_threshold, world_extent);
	case drone_map_algorithm::halftone:   return build_halftone  (img, target_count, edge_threshold, world_extent);
	case drone_map_algorithm::bright_mod: return build_bright_mod(img, target_count, edge_threshold, world_extent);
	case drone_map_algorithm::soft_edge:  return build_soft_edge (img, target_count, edge_threshold, world_extent);
	case drone_map_algorithm::stipple:    return build_stipple   (img, target_count, edge_threshold, world_extent);
	case drone_map_algorithm::layered:    return build_layered   (img, target_count, edge_threshold, world_extent);
	case drone_map_algorithm::hybrid:     return build_hybrid    (img, target_count, edge_threshold, world_extent);
	case drone_map_algorithm::edge:
	default:                              return build_edge      (img, target_count, edge_threshold, world_extent);
	}
}
