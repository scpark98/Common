#pragma once

#include <windows.h>

//시스템 전체 CPU 사용률 — GetSystemTimes 기반. 호출 간격이 1초 정도면 합리적, 그 미만이면 정밀도 ↓.
//gpu_usage / cpu_temp / nvidia_info 와 짝.
class CCpuUsage
{
public:
	CCpuUsage();

	int get_usage();	//0~100, 미초기화·실패 시 -1

private:
	ULONGLONG m_last_idle = 0;
	ULONGLONG m_last_kernel = 0;
	ULONGLONG m_last_user = 0;
	bool      m_ready = false;
};
