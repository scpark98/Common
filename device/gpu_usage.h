#pragma once

#include <windows.h>
#include <pdh.h>

// PDH 기반 GPU 사용률. 작업관리자와 동일한 소스(\GPU Engine(*)\Utilization Percentage).
// 엔진 타입별(3D/Compute/Copy/VideoDecode/VideoEncode)로 프로세스 값을 합산한 뒤
// 그 중 최댓값을 반환 — 작업관리자 그래프와 일치하는 집계 방식.
class CGpuUsage
{
public:
	CGpuUsage();
	~CGpuUsage();

	int get_usage();	// 0~100, 실패 시 -1

private:
	PDH_HQUERY   m_query = NULL;
	PDH_HCOUNTER m_counter = NULL;
	bool         m_ready = false;
};
