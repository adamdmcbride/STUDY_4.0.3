/*------------------------------------------------------------------------------
--                                                                            --
--       This software is confidential and proprietary and may be used        --
--        only as expressly authorized by a licensing agreement from          --
--                                                                            --
--                            Hantro Products Oy.                             --
--                                                                            --
--                   (C) COPYRIGHT 2006 HANTRO PRODUCTS OY                    --
--                            ALL RIGHTS RESERVED                             --
--                                                                            --
--                 The entire notice above must be reproduced                 --
--                  on all copies and should not be removed.                  --
--                                                                            --
--------------------------------------------------------------------------------
--
--  Abstract : AVS decoder and PP pipeline support
--
--------------------------------------------------------------------------------
--
--  Version control information, please leave untouched.
--
--  $RCSfile: avs_pp_pipeline.h,v $
--  $Date: 2008/07/04 10:38:54 $
--  $Revision: 1.1 $
--
------------------------------------------------------------------------------*/

#ifndef AVS_PP_PIPELINE_H
#define AVS_PP_PIPELINE_H

#include "decppif.h"

i32 avsRegisterPP(const void *decInst, const void *ppInst,
                    void (*PPDecSetup) (const void *, const DecPpInterface *),
                    void (*PPDecPipelineEndCallback) (const void *),
                    void (*PPConfigQuery) (const void *, DecPpQuery *));

i32 avsUnregisterPP(const void *decInst, const void *ppInst);

#endif /* #ifdef AVS_PP_PIPELINE_H */
