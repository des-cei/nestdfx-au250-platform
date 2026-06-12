// ==============================================================
// Vitis HLS - High-Level Synthesis from C, C++ and OpenCL v2024.2 (64-bit)
// Tool Version Limit: 2024.11
// Copyright 1986-2022 Xilinx, Inc. All Rights Reserved.
// Copyright 2022-2024 Advanced Micro Devices, Inc. All Rights Reserved.
// 
// ==============================================================
/***************************** Include Files *********************************/
#include "xddr_scale_accel.h"

/************************** Function Implementation *************************/
#ifndef __linux__
int XDdr_scale_accel_CfgInitialize(XDdr_scale_accel *InstancePtr, XDdr_scale_accel_Config *ConfigPtr) {
    Xil_AssertNonvoid(InstancePtr != NULL);
    Xil_AssertNonvoid(ConfigPtr != NULL);

    InstancePtr->Control_BaseAddress = ConfigPtr->Control_BaseAddress;
    InstancePtr->IsReady = XIL_COMPONENT_IS_READY;

    return XST_SUCCESS;
}
#endif

void XDdr_scale_accel_Start(XDdr_scale_accel *InstancePtr) {
    u32 Data;

    Xil_AssertVoid(InstancePtr != NULL);
    Xil_AssertVoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    Data = XDdr_scale_accel_ReadReg(InstancePtr->Control_BaseAddress, XDDR_SCALE_ACCEL_CONTROL_ADDR_AP_CTRL) & 0x80;
    XDdr_scale_accel_WriteReg(InstancePtr->Control_BaseAddress, XDDR_SCALE_ACCEL_CONTROL_ADDR_AP_CTRL, Data | 0x01);
}

u32 XDdr_scale_accel_IsDone(XDdr_scale_accel *InstancePtr) {
    u32 Data;

    Xil_AssertNonvoid(InstancePtr != NULL);
    Xil_AssertNonvoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    Data = XDdr_scale_accel_ReadReg(InstancePtr->Control_BaseAddress, XDDR_SCALE_ACCEL_CONTROL_ADDR_AP_CTRL);
    return (Data >> 1) & 0x1;
}

u32 XDdr_scale_accel_IsIdle(XDdr_scale_accel *InstancePtr) {
    u32 Data;

    Xil_AssertNonvoid(InstancePtr != NULL);
    Xil_AssertNonvoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    Data = XDdr_scale_accel_ReadReg(InstancePtr->Control_BaseAddress, XDDR_SCALE_ACCEL_CONTROL_ADDR_AP_CTRL);
    return (Data >> 2) & 0x1;
}

u32 XDdr_scale_accel_IsReady(XDdr_scale_accel *InstancePtr) {
    u32 Data;

    Xil_AssertNonvoid(InstancePtr != NULL);
    Xil_AssertNonvoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    Data = XDdr_scale_accel_ReadReg(InstancePtr->Control_BaseAddress, XDDR_SCALE_ACCEL_CONTROL_ADDR_AP_CTRL);
    // check ap_start to see if the pcore is ready for next input
    return !(Data & 0x1);
}

void XDdr_scale_accel_EnableAutoRestart(XDdr_scale_accel *InstancePtr) {
    Xil_AssertVoid(InstancePtr != NULL);
    Xil_AssertVoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    XDdr_scale_accel_WriteReg(InstancePtr->Control_BaseAddress, XDDR_SCALE_ACCEL_CONTROL_ADDR_AP_CTRL, 0x80);
}

void XDdr_scale_accel_DisableAutoRestart(XDdr_scale_accel *InstancePtr) {
    Xil_AssertVoid(InstancePtr != NULL);
    Xil_AssertVoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    XDdr_scale_accel_WriteReg(InstancePtr->Control_BaseAddress, XDDR_SCALE_ACCEL_CONTROL_ADDR_AP_CTRL, 0);
}

void XDdr_scale_accel_Set_in_ddr(XDdr_scale_accel *InstancePtr, u64 Data) {
    Xil_AssertVoid(InstancePtr != NULL);
    Xil_AssertVoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    XDdr_scale_accel_WriteReg(InstancePtr->Control_BaseAddress, XDDR_SCALE_ACCEL_CONTROL_ADDR_IN_DDR_DATA, (u32)(Data));
    XDdr_scale_accel_WriteReg(InstancePtr->Control_BaseAddress, XDDR_SCALE_ACCEL_CONTROL_ADDR_IN_DDR_DATA + 4, (u32)(Data >> 32));
}

u64 XDdr_scale_accel_Get_in_ddr(XDdr_scale_accel *InstancePtr) {
    u64 Data;

    Xil_AssertNonvoid(InstancePtr != NULL);
    Xil_AssertNonvoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    Data = XDdr_scale_accel_ReadReg(InstancePtr->Control_BaseAddress, XDDR_SCALE_ACCEL_CONTROL_ADDR_IN_DDR_DATA);
    Data += (u64)XDdr_scale_accel_ReadReg(InstancePtr->Control_BaseAddress, XDDR_SCALE_ACCEL_CONTROL_ADDR_IN_DDR_DATA + 4) << 32;
    return Data;
}

void XDdr_scale_accel_Set_out_ddr(XDdr_scale_accel *InstancePtr, u64 Data) {
    Xil_AssertVoid(InstancePtr != NULL);
    Xil_AssertVoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    XDdr_scale_accel_WriteReg(InstancePtr->Control_BaseAddress, XDDR_SCALE_ACCEL_CONTROL_ADDR_OUT_DDR_DATA, (u32)(Data));
    XDdr_scale_accel_WriteReg(InstancePtr->Control_BaseAddress, XDDR_SCALE_ACCEL_CONTROL_ADDR_OUT_DDR_DATA + 4, (u32)(Data >> 32));
}

u64 XDdr_scale_accel_Get_out_ddr(XDdr_scale_accel *InstancePtr) {
    u64 Data;

    Xil_AssertNonvoid(InstancePtr != NULL);
    Xil_AssertNonvoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    Data = XDdr_scale_accel_ReadReg(InstancePtr->Control_BaseAddress, XDDR_SCALE_ACCEL_CONTROL_ADDR_OUT_DDR_DATA);
    Data += (u64)XDdr_scale_accel_ReadReg(InstancePtr->Control_BaseAddress, XDDR_SCALE_ACCEL_CONTROL_ADDR_OUT_DDR_DATA + 4) << 32;
    return Data;
}

void XDdr_scale_accel_Set_length_r(XDdr_scale_accel *InstancePtr, u32 Data) {
    Xil_AssertVoid(InstancePtr != NULL);
    Xil_AssertVoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    XDdr_scale_accel_WriteReg(InstancePtr->Control_BaseAddress, XDDR_SCALE_ACCEL_CONTROL_ADDR_LENGTH_R_DATA, Data);
}

u32 XDdr_scale_accel_Get_length_r(XDdr_scale_accel *InstancePtr) {
    u32 Data;

    Xil_AssertNonvoid(InstancePtr != NULL);
    Xil_AssertNonvoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    Data = XDdr_scale_accel_ReadReg(InstancePtr->Control_BaseAddress, XDDR_SCALE_ACCEL_CONTROL_ADDR_LENGTH_R_DATA);
    return Data;
}

void XDdr_scale_accel_Set_scale(XDdr_scale_accel *InstancePtr, u32 Data) {
    Xil_AssertVoid(InstancePtr != NULL);
    Xil_AssertVoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    XDdr_scale_accel_WriteReg(InstancePtr->Control_BaseAddress, XDDR_SCALE_ACCEL_CONTROL_ADDR_SCALE_DATA, Data);
}

u32 XDdr_scale_accel_Get_scale(XDdr_scale_accel *InstancePtr) {
    u32 Data;

    Xil_AssertNonvoid(InstancePtr != NULL);
    Xil_AssertNonvoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    Data = XDdr_scale_accel_ReadReg(InstancePtr->Control_BaseAddress, XDDR_SCALE_ACCEL_CONTROL_ADDR_SCALE_DATA);
    return Data;
}

void XDdr_scale_accel_InterruptGlobalEnable(XDdr_scale_accel *InstancePtr) {
    Xil_AssertVoid(InstancePtr != NULL);
    Xil_AssertVoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    XDdr_scale_accel_WriteReg(InstancePtr->Control_BaseAddress, XDDR_SCALE_ACCEL_CONTROL_ADDR_GIE, 1);
}

void XDdr_scale_accel_InterruptGlobalDisable(XDdr_scale_accel *InstancePtr) {
    Xil_AssertVoid(InstancePtr != NULL);
    Xil_AssertVoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    XDdr_scale_accel_WriteReg(InstancePtr->Control_BaseAddress, XDDR_SCALE_ACCEL_CONTROL_ADDR_GIE, 0);
}

void XDdr_scale_accel_InterruptEnable(XDdr_scale_accel *InstancePtr, u32 Mask) {
    u32 Register;

    Xil_AssertVoid(InstancePtr != NULL);
    Xil_AssertVoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    Register =  XDdr_scale_accel_ReadReg(InstancePtr->Control_BaseAddress, XDDR_SCALE_ACCEL_CONTROL_ADDR_IER);
    XDdr_scale_accel_WriteReg(InstancePtr->Control_BaseAddress, XDDR_SCALE_ACCEL_CONTROL_ADDR_IER, Register | Mask);
}

void XDdr_scale_accel_InterruptDisable(XDdr_scale_accel *InstancePtr, u32 Mask) {
    u32 Register;

    Xil_AssertVoid(InstancePtr != NULL);
    Xil_AssertVoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    Register =  XDdr_scale_accel_ReadReg(InstancePtr->Control_BaseAddress, XDDR_SCALE_ACCEL_CONTROL_ADDR_IER);
    XDdr_scale_accel_WriteReg(InstancePtr->Control_BaseAddress, XDDR_SCALE_ACCEL_CONTROL_ADDR_IER, Register & (~Mask));
}

void XDdr_scale_accel_InterruptClear(XDdr_scale_accel *InstancePtr, u32 Mask) {
    Xil_AssertVoid(InstancePtr != NULL);
    Xil_AssertVoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    XDdr_scale_accel_WriteReg(InstancePtr->Control_BaseAddress, XDDR_SCALE_ACCEL_CONTROL_ADDR_ISR, Mask);
}

u32 XDdr_scale_accel_InterruptGetEnabled(XDdr_scale_accel *InstancePtr) {
    Xil_AssertNonvoid(InstancePtr != NULL);
    Xil_AssertNonvoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    return XDdr_scale_accel_ReadReg(InstancePtr->Control_BaseAddress, XDDR_SCALE_ACCEL_CONTROL_ADDR_IER);
}

u32 XDdr_scale_accel_InterruptGetStatus(XDdr_scale_accel *InstancePtr) {
    Xil_AssertNonvoid(InstancePtr != NULL);
    Xil_AssertNonvoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    return XDdr_scale_accel_ReadReg(InstancePtr->Control_BaseAddress, XDDR_SCALE_ACCEL_CONTROL_ADDR_ISR);
}

