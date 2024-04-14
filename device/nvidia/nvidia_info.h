#pragma once

#include <iostream>

#define NVAPI_MAX_THERMAL_SENSORS_PER_GPU 3

typedef enum
{
    NVAPI_THERMAL_TARGET_NONE = 0,
    NVAPI_THERMAL_TARGET_GPU = 1,     //!< GPU core temperature requires NvPhysicalGpuHandle
    NVAPI_THERMAL_TARGET_MEMORY = 2,     //!< GPU memory temperature requires NvPhysicalGpuHandle
    NVAPI_THERMAL_TARGET_POWER_SUPPLY = 4,     //!< GPU power supply temperature requires NvPhysicalGpuHandle
    NVAPI_THERMAL_TARGET_BOARD = 8,     //!< GPU board ambient temperature requires NvPhysicalGpuHandle
    NVAPI_THERMAL_TARGET_VCD_BOARD = 9,     //!< Visual Computing Device Board temperature requires NvVisualComputingDeviceHandle
    NVAPI_THERMAL_TARGET_VCD_INLET = 10,    //!< Visual Computing Device Inlet temperature requires NvVisualComputingDeviceHandle
    NVAPI_THERMAL_TARGET_VCD_OUTLET = 11,    //!< Visual Computing Device Outlet temperature requires NvVisualComputingDeviceHandle

    NVAPI_THERMAL_TARGET_ALL = 15,
    NVAPI_THERMAL_TARGET_UNKNOWN = -1,
} NV_THERMAL_TARGET;

typedef struct
{
    int   version;                //!< structure version 
    int   count;                  //!< number of associated thermal sensors
    struct
    {
        int       controller;        //!< internal, ADM1032, MAX6649...
        int                       defaultMinTemp;    //!< The min default temperature value of the thermal sensor in degree Celsius 
        int                       defaultMaxTemp;    //!< The max default temperature value of the thermal sensor in degree Celsius 
        int                       currentTemp;       //!< The current temperature value of the thermal sensor in degree Celsius 
        NV_THERMAL_TARGET           target;            //!< Thermal sensor targeted @ GPU, memory, chipset, powersupply, Visual Computing Device, etc.
    } sensor[NVAPI_MAX_THERMAL_SENSORS_PER_GPU];

} NV_GPU_THERMAL_SETTINGS;

// magic numbers, do not change them
#define NVAPI_MAX_PHYSICAL_GPUS   64
#define NVAPI_MAX_USAGES_PER_GPU  34

// function pointer types
typedef int* (*NvAPI_QueryInterface_t)(unsigned int offset);
typedef int (*NvAPI_Initialize_t)();
typedef int (*NvAPI_EnumPhysicalGPUs_t)(int** handles, int* count);
typedef int (*NvAPI_GPU_GetUsages_t)(int* handle, unsigned int* usages);
typedef int (*NvAPI_GPU_GetThermalSettings_t)(int* handle, int sensorIndex, NV_GPU_THERMAL_SETTINGS* temp);


class CNVidiaInfo
{
public :
    CNVidiaInfo::CNVidiaInfo();

	//bool	init();
    int     get_usage(int index);
    int     get_temperature(int index);

protected :
    // nvapi.dll internal function pointers
    NvAPI_QueryInterface_t      NvAPI_QueryInterface = NULL;
    NvAPI_Initialize_t          NvAPI_Initialize = NULL;
    NvAPI_EnumPhysicalGPUs_t    NvAPI_EnumPhysicalGPUs = NULL;
    NvAPI_GPU_GetUsages_t       NvAPI_GPU_GetUsages = NULL;
    NvAPI_GPU_GetThermalSettings_t	NvAPI_GPU_GetThermalSettings = NULL;

    NV_GPU_THERMAL_SETTINGS 		 nvgts;
    int          gpuCount = 0;
    int 		 gpuTemp;
    int* gpuHandles[NVAPI_MAX_PHYSICAL_GPUS] = { NULL };
    unsigned int gpuUsages[NVAPI_MAX_USAGES_PER_GPU] = { 0 };

};