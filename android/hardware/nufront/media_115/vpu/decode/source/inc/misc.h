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
--  Description : 
--          1) add decoder reference buffer to delay releasing the used buffer.
--          2) TODO
--------------------------------------------------------------------------------
--
--  Version control information, please leave untouched.
--
--  $RCSfile: misc.h,v $
--  $Revision: 1.0 $
--  $Date: 2012/01/15 11:07:49 $
--
------------------------------------------------------------------------------*/

#ifndef __MISC_H__
#define __MISC_H__

/*no pp and only decoder output, using 2D to do yuv2rgb/scaling/crop etc */
#define DEC_ONLY

#define ADD_BUFF  7 

#endif /* __MISC_H__ */
