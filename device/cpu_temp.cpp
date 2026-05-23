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

	int max_celsius = -1;
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

			int c = (int)(actual + 0.5f);
			if (c > max_celsius && c > 0 && c < 150)
				max_celsius = c;
		}
	}

	UnmapViewOfFile(p);
	CloseHandle(h);
	return max_celsius;
}
