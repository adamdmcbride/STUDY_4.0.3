/*
 * This confidential and proprietary software may be used only as
 * authorised by a licensing agreement from ARM Limited
 * (C) COPYRIGHT 2006-2012 ARM Limited
 * ALL RIGHTS RESERVED
 * The entire notice above must be reproduced on all authorised
 * copies and copies may only be made to the extent permitted
 * by a licensing agreement from ARM Limited.
 */

#ifndef _MALI_GL2EXT_H_
#define _MALI_GL2EXT_H_


/*
 * This is the Mali gl wrapper, for use in driver development only.  All
 * applications should be built with the stock gl2.h and gl2ext.h
 */

/* current khronos distributed gl.h, must be on include path */
#include <GLES2/gl2ext.h>

/* driver specific contents can be defined here */

/** ETC */
#define GL_OES_compressed_ETC1_RGB8_texture 1
#define GL_ETC1_RGB8_OES           0x8D64

/** video controls extensions */
#ifdef EXTENSION_VIDEO_CONTROLS_ARM_ENABLE
#define GL_TEXTURE_BRIGHTNESS_ARM           0x6001
#define GL_TEXTURE_CONTRAST_ARM             0x6002
#define GL_TEXTURE_SATURATION_ARM           0x6003
#endif

#if defined(__SYMBIAN32__)
/**
 * Defined in 3rdparty\include\khronos\GLES2\gl2ext.h
 * We are using \epoc32\include\GLES2\gl2ext.h
 */

/* GL_OES_standard_derivatives */
#ifndef GL_FRAGMENT_SHADER_DERIVATIVE_HINT_OES
#define GL_FRAGMENT_SHADER_DERIVATIVE_HINT_OES                  0x8B8B
#endif

/* GL_OES_packed_depth_stencil */
#ifndef GL_OES_packed_depth_stencil
#define GL_DEPTH_STENCIL_OES                                    0x84F9
#define GL_UNSIGNED_INT_24_8_OES                                0x84FA
#define GL_DEPTH24_STENCIL8_OES                                 0x88F0
#endif

#ifndef GL_BGRA_EXT
#define GL_BGRA_EXT                                             0x80E1
#endif

/* GL_EXT_blend_minmax */
#ifndef GL_MIN_EXT
#define GL_MIN_EXT                                              0x8007
#endif
#ifndef GL_MAX_EXT
#define GL_MAX_EXT                                              0x8008
#endif

/* GL_ARM_mali_shader_binary */
#ifndef GL_ARM_mali_shader_binary
#define GL_MALI_SHADER_BINARY_ARM                               0x8F60
#endif

/* GL_OES_EGL_image_external */
#ifndef GL_OES_EGL_image_external
/* GLeglImageOES defined in GL_OES_EGL_image already. */
#define GL_TEXTURE_EXTERNAL_OES                                 0x8D65
#define GL_SAMPLER_EXTERNAL_OES                                 0x8D66
#define GL_TEXTURE_BINDING_EXTERNAL_OES                         0x8D67
#define GL_REQUIRED_TEXTURE_IMAGE_UNITS_OES                     0x8D68
#endif

#endif  /* __SYMBIAN32__ */

#ifndef GL_EXT_MULTISAMPLED_RENDER_TO_TEXTURE
#define GL_EXT_MULTISAMPLED_RENDER_TO_TEXTURE

#define GL_RENDERBUFFER_SAMPLES_EXT                0x9133
#define GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE_EXT  0x9134
#define GL_MAX_SAMPLES_EXT                         0x9135
#define GL_TEXTURE_SAMPLES_EXT                     0x9136

GL_APICALL void GL_APIENTRY glRenderbufferStorageMultisampleEXT( GLenum target, GLsizei samples, GLenum internalformat, GLsizei width, GLsizei height);
GL_APICALL void GL_APIENTRY glFramebufferTexture2DMultisampleEXT( GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level, GLsizei samples);

#endif

/* GL_EXT_discard_framebuffer */
#ifndef GL_EXT_DISCARD_FRAMEBUFFER
#define GL_EXT_DISCARD_FRAMEBUFFER

#define GL_COLOR_EXT                               0x1800
#define GL_DEPTH_EXT                               0x1801
#define GL_STENCIL_EXT                             0x1802

GL_APICALL void GL_APIENTRY glDiscardFramebufferEXT( GLenum target, GLsizei numAttachments, const GLenum *attachments );

#endif

#endif /* _MALI_GL2EXT_H_ */
