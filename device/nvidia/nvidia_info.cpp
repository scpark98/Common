#include "nvidia_info.h"
#include <afxwin.h>

CNVidiaInfo::CNVidiaInfo()
{
	HMODULE hmod = LoadLibrary(_T("nvapi64.dll"));
	if (hmod == NULL)
	{
		AfxMessageBox(_T("Couldn't find nvapi.dll. NVidia device only."));
		return;
	}


	// nvapi_QueryInterface is a function used to retrieve other internal functions in nvapi.dll
	NvAPI_QueryInterface = (NvAPI_QueryInterface_t)GetProcAddress(hmod, "nvapi_QueryInterface");

	// some useful internal functions that aren't exported by nvapi.dll
	NvAPI_Initialize = (NvAPI_Initialize_t)(*NvAPI_QueryInterface)(0x0150E828);
	NvAPI_EnumPhysicalGPUs = (NvAPI_EnumPhysicalGPUs_t)(*NvAPI_QueryInterface)(0xE5AC921F);
	NvAPI_GPU_GetUsages = (NvAPI_GPU_GetUsages_t)(*NvAPI_QueryInterface)(0x189A1FDF);
	NvAPI_GPU_GetThermalSettings = (NvAPI_GPU_GetThermalSettings_t)(*NvAPI_QueryInterface)(0xE3640A56);

	if (NvAPI_Initialize == NULL || NvAPI_EnumPhysicalGPUs == NULL ||
		NvAPI_EnumPhysicalGPUs == NULL || NvAPI_GPU_GetUsages == NULL)
	{
		std::cerr << "Couldn't get functions in nvapi.dll" << std::endl;
		return;
	}

	// initialize NvAPI library, call it once before calling any other NvAPI functions
	(*NvAPI_Initialize)();

	// gpuUsages[0] must be this value, otherwise NvAPI_GPU_GetUsages won't work
	gpuUsages[0] = (NVAPI_MAX_USAGES_PER_GPU * 4) | 0x10000;

	(*NvAPI_EnumPhysicalGPUs)(gpuHandles, &gpuCount);
	nvgts.version = sizeof(NV_GPU_THERMAL_SETTINGS) | (1 << 16);
	nvgts.count = 0;
	nvgts.sensor[0].controller = -1;
	nvgts.sensor[0].target = NVAPI_THERMAL_TARGET_GPU;
}

int CNVidiaInfo::get_usage(int index)
{
	if (index >= nvgts.count)
		return -1;

	(*NvAPI_GPU_GetUsages)(gpuHandles[index], gpuUsages);
	return gpuUsages[3];
}

int CNVidiaInfo::get_temperature(int index)
{
	if (index >= nvgts.count)
		return -1;

	(*NvAPI_GPU_GetThermalSettings)(gpuHandles[index], 0, &nvgts);
	return nvgts.sensor[index].currentTemp;
}
