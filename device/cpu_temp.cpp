#include "cpu_temp.h"

#include <windows.h>

//CoreTemp Shared Memory SDK 의 공식 struct 정의.
//https://www.alcpu.com/CoreTemp/developers.html — public domain SDK.
#pragma pack(push, 1)
struct CORE_TEMP_SHARED_DATA
{
	unsigned int   uiLoad[256];          //per-core load %
	unsigned int   uiTjMax[128];         //per-CPU TjMax (°C)
	unsigned int   uiCoreCnt;            //total cores (across all CPUs)
	unsigned int   uiCPUCnt;             //CPU count
	float          fTemp[256];           //per-core temperature
	float          fVID;
	float          fCPUSpeed;
	float          fFSBSpeed;
	float          fMultiplier;
	char           sCPUName[100];
	unsigned char  ucFahrenheit;         //1: fTemp 가 °F, 0: °C
	unsigned char  ucDeltaToTjMax;       //1: fTemp 가 TjMax 와의 차이, 0: absolute
};
#pragma pack(pop)

int CCpuTemp::get_temp()
{
	HANDLE h = OpenFileMappingW(FILE_MAP_READ, FALSE, L"Local\\CoreTempMappingObject");
	if (!h)
		return -1;	//CoreTemp 미실행.

	CORE_TEMP_SHARED_DATA* p = (CORE_TEMP_SHARED_DATA*)
		MapViewOfFile(h, FILE_MAP_READ, 0, 0, sizeof(CORE_TEMP_SHARED_DATA));
	if (!p)
	{
		CloseHandle(h);
		return -1;
	}

	double sum = 0.0;
	int count = 0;
	if (p->uiCoreCnt > 0 && p->uiCoreCnt <= 256 && p->uiCPUCnt > 0 && p->uiCPUCnt <= 128)
	{
		unsigned int cores_per_cpu = p->uiCoreCnt / p->uiCPUCnt;
		if (cores_per_cpu == 0)
			cores_per_cpu = 1;

		for (unsigned int i = 0; i < p->uiCoreCnt; i++)
		{
			unsigned int cpu_idx = i / cores_per_cpu;
			if (cpu_idx >= p->uiCPUCnt)
				cpu_idx = p->uiCPUCnt - 1;

			float actual = p->fTemp[i];
			if (p->ucDeltaToTjMax)
				actual = (float)p->uiTjMax[cpu_idx] - actual;
			if (p->ucFahrenheit)
				actual = (actual - 32.0f) * 5.0f / 9.0f;

			if (actual > 0.0f && actual < 150.0f)
			{
				sum += actual;
				count++;
			}
		}
	}

	UnmapViewOfFile(p);
	CloseHandle(h);

	int raw = count > 0 ? (int)(sum / count + 0.5) : -1;
	if (raw < 0)
	{
		m_ema = -1.0;
		return -1;
	}

	//온도 출렁임은 코어 간 차이가 아니라 시간축 — 8 코어가 매 순간 동일값을 보고하고(평균=그 값),
	//그 값 자체가 초당 41~65 처럼 튄다(순간 부하 DTS). → EMA 시간평활로 안정화.
	//표시값 = 이전*0.85 + 새값*0.15 (get_temp 가 ~1초 주기 호출 전제). 스파이크 완화, 평균 추종.
	m_ema = (m_ema < 0.0) ? raw : (m_ema * 0.85 + raw * 0.15);
	return (int)(m_ema + 0.5);
}
