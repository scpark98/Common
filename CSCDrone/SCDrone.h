#pragma once

#include <cmath>
#include <gdiplus.h>

// 드론 쇼 시뮬레이터의 단일 드론 + 카메라(3D->2D 투영) 정의.
// Direct2D 는 2D 만 그리므로, 드론의 월드 3D 좌표를 매 프레임 회전·투영해 화면 좌표로 바꾼 뒤
// 원(점)으로 렌더한다. 이 헤더는 D2D 에 의존하지 않는다(투영 결과를 평범한 2D 좌표로만 돌려줌).

// 월드 공간 3D 좌표. 드론 위치(이후 시작/목적 좌표)에 사용.
struct SCDVec3
{
	float x = 0.f;
	float y = 0.f;
	float z = 0.f;
};

// 화면(2D) 좌표. 투영 결과.
struct SCDVec2
{
	float x = 0.f;
	float y = 0.f;
};

// 한 대의 드론. 현재 단계에서는 "월드 위치 + 색상" 상태만 가진다.
// 목적지 추종 이동과 드론별 스레드는 이동 단계에서 추가한다(지금은 불필요하므로 넣지 않음).
class CSCDrone
{
public:
	CSCDrone() = default;
	CSCDrone(const SCDVec3& pos, Gdiplus::Color color, float radius = 5.f)
		: m_pos(pos), m_color(color), m_radius(radius) {}

	void			set_position(const SCDVec3& pos) { m_pos = pos; }
	const SCDVec3&	get_position() const { return m_pos; }

	void			set_color(Gdiplus::Color color) { m_color = color; }
	Gdiplus::Color	get_color() const { return m_color; }

	void			set_radius(float r) { m_radius = r; }
	float			get_radius() const { return m_radius; }	// 월드 단위 반지름

	// true = 여분(아웃라인 밖 배경) 드론. 렌더에서 색모드 recolor·그리기 제외(검정 invisible). 총 개수·모핑엔 포함.
	void			set_unused(bool unused) { m_unused = unused; }
	bool			is_unused() const { return m_unused; }

private:
	SCDVec3			m_pos;
	Gdiplus::Color	m_color = Gdiplus::Color(255, 255, 255, 255);
	float			m_radius = 5.f;
	bool			m_unused = false;
};

// Y축(수직) 기준 회전 = 좌우 회전. 포메이션 전체가 제자리에서 도는 효과를 낸다.
inline SCDVec3 rotate_y(const SCDVec3& p, float angle_rad)
{
	const float c = cosf(angle_rad);
	const float s = sinf(angle_rad);
	return SCDVec3{ p.x * c + p.z * s, p.y, -p.x * s + p.z * c };
}

// X축(수평) 기준 회전 = 상하 회전(끄덕임).
inline SCDVec3 rotate_x(const SCDVec3& p, float angle_rad)
{
	const float c = cosf(angle_rad);
	const float s = sinf(angle_rad);
	return SCDVec3{ p.x, p.y * c - p.z * s, p.y * s + p.z * c };
}

// 두 월드 좌표 선형 보간(모핑 경로 계산용).
inline SCDVec3 lerp3(const SCDVec3& a, const SCDVec3& b, float t)
{
	return SCDVec3{ a.x + (b.x - a.x) * t, a.y + (b.y - a.y) * t, a.z + (b.z - a.z) * t };
}

// 간단한 핀홀 원근 카메라. 뷰어는 원점에서 -z 쪽으로 distance 만큼 떨어져 +z 를 바라본다.
// z 가 클수록(멀수록) 작게, 작을수록(가까울수록) 크게 보인다.
struct SCDCamera
{
	// distance 가 작을수록(원점에 가까울수록) 원근이 강해져 z 에 따른 크기차가 뚜렷해진다.
	// 크기차 배율 = (distance + 좌표범위) / (distance - 좌표범위).
	float distance = 300.f;	// 뷰어~원점 거리(월드 단위)
	float focal = 220.f;	// 초점거리(전체 퍼짐 정도. 화면에 맞게 조절)
	float cx = 0.f;			// 화면 중심 x(픽셀)
	float cy = 0.f;			// 화면 중심 y(픽셀)

	// 월드 좌표 p 를 화면 좌표로 투영. out_scale = 깊이에 따른 배율(반지름/정렬에 사용).
	// 반환 false = 카메라 뒤쪽(또는 너무 가까움)이라 투영 불가.
	bool project(const SCDVec3& p, SCDVec2& out_pt, float& out_scale) const
	{
		const float depth = p.z + distance;	// 카메라로부터의 깊이
		if (depth <= 1.f)
			return false;

		const float scale = focal / depth;
		out_pt.x = cx + p.x * scale;
		out_pt.y = cy - p.y * scale;	// 화면 y 는 아래로 증가하므로 부호 반전
		out_scale = scale;
		return true;
	}
};
