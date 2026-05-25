#pragma once

//CPU 온도 측정 — CoreTemp Shared Memory (https://www.alcpu.com/CoreTemp/developers.html).
//CoreTemp 가 백그라운드 실행 중일 때 "Local\CoreTempMappingObject" mapping 으로 코어별 온도 + TjMax + Delta mode 읽기.
//추가 DLL / driver 의존 없음 — OpenFileMapping + MapViewOfFile 표준 Win32 API 만.
//CoreTemp 미실행 또는 측정 실패 시 -1.
class CCpuTemp
{
public:
	CCpuTemp() = default;
	~CCpuTemp() = default;

	int get_temp();		//°C. 실패 -1. 전 코어 평균 + EMA 시간평활 (초당 잡음 완화).

private:
	double m_ema = -1.0;	//EMA 상태 (이전 표시값). -1 = 미초기화/측정실패.
};
