/*
  *Copyright (C), 2009~2019, NUFRONT. Co., Ltd.
  *File name:    OMX_VideoDec_VPU_PP.c
  *Author: Sudao Version: v1.0 Date: 2011-09-25
  *Description: VPU post-processor operations.
*/

#ifndef VPU_PP_H
#define VPU_PP_H
#include "OMX_VideoDec_VPU.h"
#include "OMX_VideoDec_Utils.h"

int vpu_pp_init(VIDDEC_COMPONENT_PRIVATE* pComponentPrivate, u32 decType);

int vpu_pp_end(VIDDEC_COMPONENT_PRIVATE* pComponentPrivate);

int vpupp_setup_multibuf(VIDDEC_COMPONENT_PRIVATE* pComponentPrivate,
    u32 amount, u32 bytes, u32 picSize) ;

int vpupp_free_multibuf(VIDDEC_COMPONENT_PRIVATE* pComponentPrivate);

int vpupp_config(VIDDEC_COMPONENT_PRIVATE* pComponentPrivate);

int vpupp_get_output(VIDDEC_COMPONENT_PRIVATE* pComponentPrivate,
    u32 width, u32 height, u32 pixBytes);

int vpupp_get_output2(VIDDEC_COMPONENT_PRIVATE* pComponentPrivate,
    u32 inWidth, u32 inHeight, u32 outWidth, u32 outHeight,
    u32 busAddress, u32 busCbAddress);
#endif

