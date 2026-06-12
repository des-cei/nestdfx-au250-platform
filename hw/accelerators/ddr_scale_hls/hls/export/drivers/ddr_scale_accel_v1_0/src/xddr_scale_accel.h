// ==============================================================
// Vitis HLS - High-Level Synthesis from C, C++ and OpenCL v2024.2 (64-bit)
// Tool Version Limit: 2024.11
// Copyright 1986-2022 Xilinx, Inc. All Rights Reserved.
// Copyright 2022-2024 Advanced Micro Devices, Inc. All Rights Reserved.
// 
// ==============================================================
#ifndef XDDR_SCALE_ACCEL_H
#define XDDR_SCALE_ACCEL_H

#ifdef __cplusplus
extern "C" {
#endif

/***************************** Include Files *********************************/
#ifndef __linux__
#include "xil_types.h"
#include "xil_assert.h"
#include "xstatus.h"
#include "xil_io.h"
#else
#include <stdint.h>
#include <assert.h>
#include <dirent.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>
#include <stddef.h>
#endif
#include "xddr_scale_accel_hw.h"

/**************************** Type Definitions ******************************/
#ifdef __linux__
typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;
#else
typedef struct {
#ifdef SDT
    char *Name;
#else
    u16 DeviceId;
#endif
    u64 Control_BaseAddress;
} XDdr_scale_accel_Config;
#endif

typedef struct {
    u64 Control_BaseAddress;
    u32 IsReady;
} XDdr_scale_accel;

typedef u32 word_type;

/***************** Macros (Inline Functions) Definitions *********************/
#ifndef __linux__
#define XDdr_scale_accel_WriteReg(BaseAddress, RegOffset, Data) \
    Xil_Out32((BaseAddress) + (RegOffset), (u32)(Data))
#define XDdr_scale_accel_ReadReg(BaseAddress, RegOffset) \
    Xil_In32((BaseAddress) + (RegOffset))
#else
#define XDdr_scale_accel_WriteReg(BaseAddress, RegOffset, Data) \
    *(volatile u32*)((BaseAddress) + (RegOffset)) = (u32)(Data)
#define XDdr_scale_accel_ReadReg(BaseAddress, RegOffset) \
    *(volatile u32*)((BaseAddress) + (RegOffset))

#define Xil_AssertVoid(expr)    assert(expr)
#define Xil_AssertNonvoid(expr) assert(expr)

#define XST_SUCCESS             0
#define XST_DEVICE_NOT_FOUND    2
#define XST_OPEN_DEVICE_FAILED  3
#define XIL_COMPONENT_IS_READY  1
#endif

/************************** Function Prototypes *****************************/
#ifndef __linux__
#ifdef SDT
int XDdr_scale_accel_Initialize(XDdr_scale_accel *InstancePtr, UINTPTR BaseAddress);
XDdr_scale_accel_Config* XDdr_scale_accel_LookupConfig(UINTPTR BaseAddress);
#else
int XDdr_scale_accel_Initialize(XDdr_scale_accel *InstancePtr, u16 DeviceId);
XDdr_scale_accel_Config* XDdr_scale_accel_LookupConfig(u16 DeviceId);
#endif
int XDdr_scale_accel_CfgInitialize(XDdr_scale_accel *InstancePtr, XDdr_scale_accel_Config *ConfigPtr);
#else
int XDdr_scale_accel_Initialize(XDdr_scale_accel *InstancePtr, const char* InstanceName);
int XDdr_scale_accel_Release(XDdr_scale_accel *InstancePtr);
#endif

void XDdr_scale_accel_Start(XDdr_scale_accel *InstancePtr);
u32 XDdr_scale_accel_IsDone(XDdr_scale_accel *InstancePtr);
u32 XDdr_scale_accel_IsIdle(XDdr_scale_accel *InstancePtr);
u32 XDdr_scale_accel_IsReady(XDdr_scale_accel *InstancePtr);
void XDdr_scale_accel_EnableAutoRestart(XDdr_scale_accel *InstancePtr);
void XDdr_scale_accel_DisableAutoRestart(XDdr_scale_accel *InstancePtr);

void XDdr_scale_accel_Set_in_ddr(XDdr_scale_accel *InstancePtr, u64 Data);
u64 XDdr_scale_accel_Get_in_ddr(XDdr_scale_accel *InstancePtr);
void XDdr_scale_accel_Set_out_ddr(XDdr_scale_accel *InstancePtr, u64 Data);
u64 XDdr_scale_accel_Get_out_ddr(XDdr_scale_accel *InstancePtr);
void XDdr_scale_accel_Set_length_r(XDdr_scale_accel *InstancePtr, u32 Data);
u32 XDdr_scale_accel_Get_length_r(XDdr_scale_accel *InstancePtr);
void XDdr_scale_accel_Set_scale(XDdr_scale_accel *InstancePtr, u32 Data);
u32 XDdr_scale_accel_Get_scale(XDdr_scale_accel *InstancePtr);

void XDdr_scale_accel_InterruptGlobalEnable(XDdr_scale_accel *InstancePtr);
void XDdr_scale_accel_InterruptGlobalDisable(XDdr_scale_accel *InstancePtr);
void XDdr_scale_accel_InterruptEnable(XDdr_scale_accel *InstancePtr, u32 Mask);
void XDdr_scale_accel_InterruptDisable(XDdr_scale_accel *InstancePtr, u32 Mask);
void XDdr_scale_accel_InterruptClear(XDdr_scale_accel *InstancePtr, u32 Mask);
u32 XDdr_scale_accel_InterruptGetEnabled(XDdr_scale_accel *InstancePtr);
u32 XDdr_scale_accel_InterruptGetStatus(XDdr_scale_accel *InstancePtr);

#ifdef __cplusplus
}
#endif

#endif
