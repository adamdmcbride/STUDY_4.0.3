/*
 * This confidential and proprietary software may be used only as
 * authorised by a licensing agreement from ARM Limited
 * (C) COPYRIGHT 2005-2012 ARM Limited
 * ALL RIGHTS RESERVED
 * The entire notice above must be reproduced on all authorised
 * copies and copies may only be made to the extent permitted
 * by a licensing agreement from ARM Limited.
 */

#ifndef __gl_ext_h_
#define __gl_ext_h_

/* current khronos distributed glext.h, must be on include path */
#include <GLES/gl.h>
#include <GLES/glext.h>
#include <GLES/glplatform.h>

#ifdef __cplusplus
extern "C" {
#endif

#if defined(__SYMBIAN32__)
/**
 * Defined in 3rdparty\include\khronos\GLES\glext.h
 * We are using \epoc32\include\GLES\glext.h
 */

/* GL_OES_element_index_uint */
#ifndef GL_UNSIGNED_INT
#define GL_UNSIGNED_INT                   0x1405
#endif

#ifndef GL_BGRA_EXT
#define GL_BGRA_EXT                       0x80E1
#endif

#ifndef GL_DEPTH_STENCIL_OES
#define GL_DEPTH_STENCIL_OES              0x84F9
#endif

#ifndef GL_UNSIGNED_INT_24_8_OES
#define GL_UNSIGNED_INT_24_8_OES          0x84FA
#endif

#ifndef GL_DEPTH24_STENCIL8_OES
#define GL_DEPTH24_STENCIL8_OES           0x88F0
#endif

#endif /* #if defined(__SYMBIAN32__) */


#ifndef GL_EXT_MULTISAMPLED_RENDER_TO_TEXTURE
#define GL_EXT_MULTISAMPLED_RENDER_TO_TEXTURE

#define GL_RENDERBUFFER_SAMPLES_EXT                0x9133
#define GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE_EXT  0x9134
#define GL_MAX_SAMPLES_EXT                         0x9135
#define GL_TEXTURE_SAMPLES_EXT                     0x9136

GL_API void GL_APIENTRY glRenderbufferStorageMultisampleEXT( GLenum target, GLsizei samples, GLenum internalformat, GLsizei width, GLsizei height);
GL_API void GL_APIENTRY glFramebufferTexture2DMultisampleEXT( GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level, GLsizei samples);

#endif

/* GL_EXT_discard_framebuffer */
#ifndef GL_EXT_DISCARD_FRAMEBUFFER
#define GL_EXT_DISCARD_FRAMEBUFFER

#define GL_COLOR_EXT                               0x1800
#define GL_DEPTH_EXT                               0x1801
#define GL_STENCIL_EXT                             0x1802

GL_API void GL_APIENTRY glDiscardFramebufferEXT( GLenum target, GLsizei numAttachments, const GLenum *attachments );

#endif

#ifdef __cplusplus
}
#endif

#endif /* __gl_ext_h_ */
