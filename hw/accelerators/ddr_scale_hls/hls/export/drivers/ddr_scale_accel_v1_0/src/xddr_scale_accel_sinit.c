// ==============================================================
// Vitis HLS - High-Level Synthesis from C, C++ and OpenCL v2024.2 (64-bit)
// Tool Version Limit: 2024.11
// Copyright 1986-2022 Xilinx, Inc. All Rights Reserved.
// Copyright 2022-2024 Advanced Micro Devices, Inc. All Rights Reserved.
// 
// ==============================================================
#ifndef __linux__

#include "xstatus.h"
#ifdef SDT
#include "xparameters.h"
#endif
#include "xddr_scale_accel.h"

extern XDdr_scale_accel_Config XDdr_scale_accel_ConfigTable[];

#ifdef SDT
XDdr_scale_accel_Config *XDdr_scale_accel_LookupConfig(UINTPTR BaseAddress) {
	XDdr_scale_accel_Config *ConfigPtr = NULL;

	int Index;

	for (Index = (u32)0x0; XDdr_scale_accel_ConfigTable[Index].Name != NULL; Index++) {
		if (!BaseAddress || XDdr_scale_accel_ConfigTable[Index].Control_BaseAddress == BaseAddress) {
			ConfigPtr = &XDdr_scale_accel_ConfigTable[Index];
			break;
		}
	}

	return ConfigPtr;
}

int XDdr_scale_accel_Initialize(XDdr_scale_accel *InstancePtr, UINTPTR BaseAddress) {
	XDdr_scale_accel_Config *ConfigPtr;

	Xil_AssertNonvoid(InstancePtr != NULL);

	ConfigPtr = XDdr_scale_accel_LookupConfig(BaseAddress);
	if (ConfigPtr == NULL) {
		InstancePtr->IsReady = 0;
		return (XST_DEVICE_NOT_FOUND);
	}

	return XDdr_scale_accel_CfgInitialize(InstancePtr, ConfigPtr);
}
#else
XDdr_scale_accel_Config *XDdr_scale_accel_LookupConfig(u16 DeviceId) {
	XDdr_scale_accel_Config *ConfigPtr = NULL;

	int Index;

	for (Index = 0; Index < XPAR_XDDR_SCALE_ACCEL_NUM_INSTANCES; Index++) {
		if (XDdr_scale_accel_ConfigTable[Index].DeviceId == DeviceId) {
			ConfigPtr = &XDdr_scale_accel_ConfigTable[Index];
			break;
		}
	}

	return ConfigPtr;
}

int XDdr_scale_accel_Initialize(XDdr_scale_accel *InstancePtr, u16 DeviceId) {
	XDdr_scale_accel_Config *ConfigPtr;

	Xil_AssertNonvoid(InstancePtr != NULL);

	ConfigPtr = XDdr_scale_accel_LookupConfig(DeviceId);
	if (ConfigPtr == NULL) {
		InstancePtr->IsReady = 0;
		return (XST_DEVICE_NOT_FOUND);
	}

	return XDdr_scale_accel_CfgInitialize(InstancePtr, ConfigPtr);
}
#endif

#endif

