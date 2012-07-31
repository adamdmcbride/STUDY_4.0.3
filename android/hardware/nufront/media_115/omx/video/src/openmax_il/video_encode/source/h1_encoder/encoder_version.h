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
--  Description :  Encoder version information.
--
------------------------------------------------------------------------------*/

#ifndef HANTRO_ENCODER_VERSION_H
#define HANTRO_ENCODER_VERSION_H

#define COMPONENT_VERSION_MAJOR    1
#define COMPONENT_VERSION_MINOR    1
#define COMPONENT_VERSION_REVISION 1
#define COMPONENT_VERSION_STEP     0


#define VIDEO_COMPONENT_NAME "OMX.hantro.H1.video.encoder"
#define IMAGE_COMPONENT_NAME "OMX.hantro.H1.image.encoder"

/* Roles currently disabled. Standard roles requires support for YUV420Planar
 * format which is not implemeted. */
#define VIDEO_COMPONENT_ROLES 0
#define IMAGE_COMPONENT_ROLES 0


//VIDEO DOMAIN ROLES
#ifdef OMX_ENCODER_VIDEO_DOMAIN
/** Role 1 - H264 Encoder */
#define COMPONENT_NAME_H264  "OMX.hantro.H1.video.encoder.avc"
#define COMPONENT_ROLE_H264  "video_encoder.avc"

#if !defined (ENC8270) && !defined (ENC8290) && !defined (ENCH1)
/** Role 2 - MPEG4 Encoder */
#define COMPONENT_NAME_MPEG4 "OMX.hantro.7280.video.encoder.mpeg4"
#define COMPONENT_ROLE_MPEG4 "video_encoder.mpeg4"
/** Role 3 - H263 Encoder */
#define COMPONENT_NAME_H263  "OMX.hantro.7280.video.encoder.h263"
#define COMPONENT_ROLE_H263  "video_encoder.h263"
#endif
#ifdef ENCH1
/** Role 4 - VP8 Encoder */
#define COMPONENT_NAME_VP8  "OMX.hantro.H1.video.encoder.vp8"
#define COMPONENT_ROLE_VP8  "video_encoder.vp8"
#endif
#endif //~OMX_ENCODER_VIDEO_DOMAIN

//IMAGE DOMAIN ROLES
#ifdef OMX_ENCODER_IMAGE_DOMAIN
/** Role 1 - JPEG Encoder */
#define COMPONENT_NAME_JPEG  "OMX.hantro.H1.image.encoder.jpeg"
#define COMPONENT_ROLE_JPEG  "image_encoder.jpeg"
#ifdef ENCH1
/** Role 2 - WEBP Encoder */
#define COMPONENT_NAME_WEBP  "OMX.hantro.H1.image.encoder.webp"
#define COMPONENT_ROLE_WEBP  "image_encoder.webp"
#endif
#endif //OMX_ENCODER_IMAGE_DOMAIN


#endif // HANTRO_ENCODER_VERSION_H

