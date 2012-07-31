/*
 * This confidential and proprietary software may be used only as
 * authorised by a licensing agreement from ARM Limited
 * (C) COPYRIGHT 2010, 2012 ARM Limited
 * ALL RIGHTS RESERVED
 * The entire notice above must be reproduced on all authorised
 * copies and copies may only be made to the extent permitted
 * by a licensing agreement from ARM Limited.
 */

/**
 * @file mali_config.h
 *
 * This file is contains misc configurations parameters for the Mali driver
 *
 * Some Mali DDK build systems (eg for Symbian OS) have a global
 * config file which is used to select which definitions apply. Most
 * therefore have #ifndef #endif wrapped around them
 */

#ifndef _MALI_CONFIG_H_
#define _MALI_CONFIG_H_

/*
 * MALI_OPENVG_TX_ALPHA_CLAMP_DEFAULT
 *
 * The default value for the environment variable
 * MALI_OPENVG_TEXTURE_ALPHA_CLAMP.
 *
 * 1: Apply strict interpretation of OpenVG spec, and clamp colour
 * channel values of pre multiplied pixel formats in the range [0,
 * alpha].
 *
 * 0: Clamp colour values only on rendered output. This means that any
 * texture uploaded with out of range values and immediately read back
 * will still be out of range.
 */
#ifndef MALI_OPENVG_TX_ALPHA_CLAMP_DEFAULT
#define MALI_OPENVG_TX_ALPHA_CLAMP_DEFAULT 1
#endif

/*
 * MALI_OPENVG_TX_LAYOUT_LINEAR_DEFAULT
 *
 * The default value for the environment variable
 * MALI_OPENVG_TEXTURE_LAYOUT_LINEAR
 *
 * 0: Use a block-interleaved format. Best for rendering speed.
 *
 * 1: Use a linear format. Best for upload speed.
 */
#ifndef MALI_OPENVG_TX_LAYOUT_LINEAR_DEFAULT
#define MALI_OPENVG_TX_LAYOUT_LINEAR_DEFAULT 0
#endif

#endif /*_MALI_CONFIG_H_ */
