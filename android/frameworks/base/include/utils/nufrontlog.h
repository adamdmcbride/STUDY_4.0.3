/*
 * Copyright (C) 2005 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

//
// C/C++ logging functions.  See the logging documentation for API details.
//
// We'd like these to be available from C code (in case we import some from
// somewhere), so this has a C interface.
//
// The output will be correct when the log file is shared between multiple
// threads and/or multiple processes so long as the operating system
// supports O_APPEND.  These calls have mutex-protected data structures
// and so are NOT reentrant.  Do not use LOG in a signal handler.
//
#ifndef _LIBS_UTILS_NULOG_H
#define _LIBS_UTILS_NULOG_H

#include <cutils/log.h>


/*a@nufront start: add for log tool*/

/***********config domain start*********/
#define NULOG_LEVEL    0  /*0: no nulog print; 2: nulogd; 1: nuLoge*/

#define NULOG_ON       1
#define NULOG_OWNER_ON 1
#define NULOG_FILE_ON  0
#define NULOG_LINE_ON  1
#define NULOG_FUNC_ON  1


#define YYLOG_ON       1
#define YYLOG_OWNER_ON 1
#define YYLOG_FILE_ON  0
#define YYLOG_LINE_ON  1
#define YYLOG_FUNC_ON  1

#define ZJFLOG_ON       1
#define ZJFLOG_OWNER_ON 1
#define ZJFLOG_FILE_ON  0
#define ZJFLOG_LINE_ON  1
#define ZJFLOG_FUNC_ON  1

#define SDLOG_ON       1
#define SDLOG_OWNER_ON 1
#define SDLOG_FILE_ON  0
#define SDLOG_LINE_ON  1
#define SDLOG_FUNC_ON  1

#define HPLOG_ON       1
#define HPLOG_OWNER_ON 1
#define HPLOG_FILE_ON  0
#define HPLOG_LINE_ON  1
#define HPLOG_FUNC_ON  1

#define ZXLOG_ON       1
#define ZXLOG_OWNER_ON 1
#define ZXLOG_FILE_ON  0
#define ZXLOG_LINE_ON  1
#define ZXLOG_FUNC_ON  1

#define LDCLOG_ON       1
#define LDCLOG_OWNER_ON 1
#define LDCLOG_FILE_ON  0
#define LDCLOG_LINE_ON  1
#define LDCLOG_FUNC_ON  1

#define LBQLOG_ON       1
#define LBQLOG_OWNER_ON 1
#define LBQLOG_FILE_ON  0
#define LBQLOG_LINE_ON  1
#define LBQLOG_FUNC_ON  1

#define DGYLOG_ON       1
#define DGYLOG_OWNER_ON 1
#define DGYLOG_FILE_ON  0
#define DGYLOG_LINE_ON  1
#define DGYLOG_FUNC_ON  1

#define LYBLOG_ON       1
#define LYBLOG_OWNER_ON 1
#define LYBLOG_FILE_ON  0
#define LYBLOG_LINE_ON  1
#define LYBLOG_FUNC_ON  1

#define LCHLOG_ON       0
#define LCHLOG_OWNER_ON 1
#define LCHLOG_FILE_ON  0
#define LCHLOG_LINE_ON  1
#define LCHLOG_FUNC_ON  1

#define GSLOG_ON       1
#define GSLOG_OWNER_ON 1
#define GSLOG_FILE_ON  0
#define GSLOG_LINE_ON  1
#define GSLOG_FUNC_ON  1

#define DXZLOG_ON       1
#define DXZLOG_OWNER_ON 1
#define DXZLOG_FILE_ON  0
#define DXZLOG_LINE_ON  1
#define DXZLOG_FUNC_ON  1

/***********config domain end*********/
/*************************************/

#define NULOG_LEVEL_LOGD 2
#define NULOG_LEVEL_LOGE 1


#if NULOG_LEVEL && NULOG_ON

#if NULOG_OWNER_ON
#define NULOG_OWNER NULL
#else
#define NULOG_OWNER "nusmart"
#endif

#if NULOG_FILE_ON
#define NULOG_FILE __FILE__
#else
#define NULOG_FILE NULL
#endif

#if NULOG_LINE_ON
#define NULOG_LINE __LINE__
#else
#define NULOG_LINE NULL
#endif

#if NULOG_FUNC_ON
#define NULOG_FUNC __func__
#else
#define NULOG_FUNC NULL
#endif

#if NULOG_LEVEL < NULOG_LEVEL_LOGD
#define NULOGD(...)
#else
#define NULOGD(...) ((void)NULOG(LOG_DEBUG, LOG_TAG, NULOG_OWNER, NULOG_FILE, NULOG_FUNC, NULOG_LINE, __VA_ARGS__))
#endif

#if NULOG_LEVEL < NULOG_LEVEL_LOGE
#define NULOGE(...)
#else
#define NULOGE(...) ((void)NULOG(LOG_ERROR, LOG_TAG, NULOG_OWNER, NULOG_FILE, NULOG_FUNC, NULOG_LINE, __VA_ARGS__))
#endif
#else
#define NULOGD(...)
#define NULOGE(...)
#endif




#if (NULOG_LEVEL && ZJFLOG_ON)

#if ZJFLOG_OWNER_ON
#define ZJFLOG_OWNER "zjf"
#else
#define ZJFLOG_OWNER NULL
#endif

#if ZJFLOG_FILE_ON
#define ZJFLOG_FILE __FILE__
#else
#define ZJFLOG_FILE NULL
#endif

#if ZJFLOG_LINE_ON
#define ZJFLOG_LINE __LINE__
#else
#define ZJFLOG_LINE NULL
#endif

#if ZJFLOG_FUNC_ON
#define ZJFLOG_FUNC __func__
#else
#define ZJFLOG_FUNC NULL
#endif

#if NULOG_LEVEL < NULOG_LEVEL_LOGD
#define ZJFLOGD(...)
#else
#define ZJFLOGD(...) ((void)NULOG(LOG_DEBUG, LOG_TAG, ZJFLOG_OWNER, ZJFLOG_FILE, ZJFLOG_FUNC, ZJFLOG_LINE, __VA_ARGS__))
#endif

#if NULOG_LEVEL < NULOG_LEVEL_LOGE
#define ZJFLOGE(...)
#else
#define ZJFLOGE(...) ((void)NULOG(LOG_ERROR, LOG_TAG, ZJFLOG_OWNER, ZJFLOG_FILE, ZJFLOG_FUNC, ZJFLOG_LINE, __VA_ARGS__))
#endif
#else
#define ZJFLOGD(...)
#define ZJFLOGE(...)
#endif

#if (NULOG_LEVEL && YYLOG_ON)

#if YYLOG_OWNER_ON
#define YYLOG_OWNER "yy"
#else
#define NULOG_OWNER NULL
#endif

#if YYLOG_FILE_ON
#define YYLOG_FILE __FILE__
#else
#define YYLOG_FILE NULL
#endif

#if YYLOG_LINE_ON
#define YYLOG_LINE __LINE__
#else
#define YYLOG_LINE NULL
#endif

#if YYLOG_FUNC_ON
#define YYLOG_FUNC __func__
#else
#define YYLOG_FUNC NULL
#endif

#if ((NULOG_LEVEL) < (NULOG_LEVEL_LOGD))
#define YYLOGD(...)
#else
#define YYLOGD(...) ((void)NULOG(LOG_DEBUG, LOG_TAG, YYLOG_OWNER, YYLOG_FILE, YYLOG_FUNC, YYLOG_LINE, __VA_ARGS__))
#endif

#if ((NULOG_LEVEL) < (NULOG_LEVEL_LOGE))
#define YYLOGE(...)
#else
#define YYLOGE(...) ((void)NULOG(LOG_ERROR, LOG_TAG, YYLOG_OWNER, YYLOG_FILE, YYLOG_FUNC, YYLOG_LINE, __VA_ARGS__))
#endif
#else
#define YYLOGD(...)
#define YYLOGE(...)
#endif

#if (NULOG_LEVEL && SDLOG_ON)

#if SDLOG_OWNER_ON
#define SDLOG_OWNER "sd"
#else
#define SDLOG_OWNER NULL
#endif

#if SDLOG_FILE_ON
#define SDLOG_FILE __FILE__
#else
#define SDLOG_FILE NULL
#endif

#if SDLOG_LINE_ON
#define SDLOG_LINE __LINE__
#else
#define SDLOG_LINE NULL
#endif

#if SDLOG_FUNC_ON
#define SDLOG_FUNC __func__
#else
#define SDLOG_FUNC NULL
#endif

#if NULOG_LEVEL < NULOG_LEVEL_LOGD
#define SDLOGD(...)
#else
#define SDLOGD(...) ((void)NULOG(LOG_DEBUG, LOG_TAG, SDLOG_OWNER, SDLOG_FILE, SDLOG_FUNC, SDLOG_LINE, __VA_ARGS__))
#endif

#if NULOG_LEVEL < NULOG_LEVEL_LOGE
#define SDLOGE(...)
#else
#define SDLOGE(...) ((void)NULOG(LOG_ERROR, LOG_TAG, SDLOG_OWNER, SDLOG_FILE, SDLOG_FUNC, SDLOG_LINE, __VA_ARGS__))
#endif
#else
#define SDLOGD(...)
#define SDLOGE(...)
#endif

#if (NULOG_LEVEL && HPLOG_ON)

#if HPLOG_OWNER_ON
#define HPLOG_OWNER "hp"
#else
#define HPLOG_OWNER NULL
#endif

#if HPLOG_FILE_ON
#define HPLOG_FILE __FILE__
#else
#define HPLOG_FILE NULL
#endif

#if HPLOG_LINE_ON
#define HPLOG_LINE __LINE__
#else
#define HPLOG_LINE NULL
#endif

#if HPLOG_FUNC_ON
#define HPLOG_FUNC __func__
#else
#define HPLOG_FUNC NULL
#endif

#if NULOG_LEVEL < NULOG_LEVEL_LOGD
#define HPLOGD(...)
#else
#define HPLOGD(...) ((void)NULOG(LOG_DEBUG, LOG_TAG, HPLOG_OWNER, HPLOG_FILE, HPLOG_FUNC, HPLOG_LINE, __VA_ARGS__))
#endif

#if NULOG_LEVEL < NULOG_LEVEL_LOGE
#define HPLOGE(...)
#else
#define HPLOGE(...) ((void)NULOG(LOG_ERROR, LOG_TAG, HPLOG_OWNER, HPLOG_FILE, HPLOG_FUNC, HPLOG_LINE, __VA_ARGS__))
#endif
#else
#define HPLOGD(...)
#define HPLOGE(...)
#endif

#if (NULOG_LEVEL && ZXLOG_ON)

#if ZXLOG_OWNER_ON
#define ZXLOG_OWNER "zx"
#else
#define ZXLOG_OWNER NULL
#endif

#if ZXLOG_FILE_ON
#define ZXLOG_FILE __FILE__
#else
#define ZXLOG_FILE NULL
#endif

#if ZXLOG_LINE_ON
#define ZXLOG_LINE __LINE__
#else
#define ZXLOG_LINE NULL
#endif

#if ZXLOG_FUNC_ON
#define ZXLOG_FUNC __func__
#else
#define ZXLOG_FUNC NULL
#endif

#if NULOG_LEVEL < NULOG_LEVEL_LOGD
#define ZXLOGD(...)
#else
#define ZXLOGD(...) ((void)NULOG(LOG_DEBUG, LOG_TAG, ZXLOG_OWNER, ZXLOG_FILE, ZXLOG_FUNC, ZXLOG_LINE, __VA_ARGS__))
#endif

#if NULOG_LEVEL < NULOG_LEVEL_LOGE
#define ZXLOGE(...)
#else
#define ZXLOGE(...) ((void)ZXLOG(LOG_ERROR, LOG_TAG, ZXLOG_OWNER, ZXLOG_FILE, ZXLOG_FUNC, ZXLOG_LINE, __VA_ARGS__))
#endif
#else
#define ZXLOGD(...)
#define ZXLOGE(...)
#endif

#if (NULOG_LEVEL && LDCLOG_ON)

#if LDCLOG_OWNER_ON
#define LDCLOG_OWNER "ldc"
#else
#define LDCLOG_OWNER NULL
#endif

#if LDCLOG_FILE_ON
#define LDCLOG_FILE __FILE__
#else
#define LDCLOG_FILE NULL
#endif

#if LDCLOG_LINE_ON
#define LDCLOG_LINE __LINE__
#else
#define LDCLOG_LINE NULL
#endif

#if LDCLOG_FUNC_ON
#define LDCLOG_FUNC __func__
#else
#define LDCLOG_FUNC NULL
#endif

#if NULOG_LEVEL < NULOG_LEVEL_LOGD
#define LDCLOGD(...)
#else
#define LDCLOGD(...) ((void)NULOG(LOG_DEBUG, LOG_TAG, LDCLOG_OWNER, LDCLOG_FILE, LDCLOG_FUNC, LDCLOG_LINE, __VA_ARGS__))
#endif

#if NULOG_LEVEL < NULOG_LEVEL_LOGE
#define LDCLOGE(...)
#else
#define LDCLOGE(...) ((void)NULOG(LOG_ERROR, LOG_TAG, LDCLOG_OWNER, LDCLOG_FILE, LDCLOG_FUNC, LDCLOG_LINE, __VA_ARGS__))
#endif
#else
#define LDCLOGD(...)
#define LDCLOGE(...)
#endif

#if (NULOG_LEVEL && LBQLOG_ON)

#if LBQLOG_OWNER_ON
#define LBQLOG_OWNER "lbq"
#else
#define LBQLOG_OWNER NULL
#endif

#if LBQLOG_FILE_ON
#define LBQLOG_FILE __FILE__
#else
#define LBQLOG_FILE NULL
#endif

#if LBQLOG_LINE_ON
#define LBQLOG_LINE __LINE__
#else
#define LBQLOG_LINE NULL
#endif

#if LBQLOG_FUNC_ON
#define LBQLOG_FUNC __func__
#else
#define LBQLOG_FUNC NULL
#endif

#if NULOG_LEVEL < NULOG_LEVEL_LOGD
#define LBQLOGD(...)
#else
#define LBQLOGD(...) ((void)NULOG(LOG_DEBUG, LOG_TAG, LBQLOG_OWNER, LBQLOG_FILE, LBQLOG_FUNC, LBQLOG_LINE, __VA_ARGS__))
#endif

#if NULOG_LEVEL < NULOG_LEVEL_LOGE
#define LBQLOGE(...)
#else
#define LBQLOGE(...) ((void)NULOG(LOG_ERROR, LOG_TAG, LBQLOG_OWNER, LBQLOG_FILE, LBQLOG_FUNC, LBQLOG_LINE, __VA_ARGS__))
#endif
#else
#define LBQLOGD(...)
#define LBQLOGE(...)
#endif

#if (NULOG_LEVEL && DGYLOG_ON)

#if DGYLOG_OWNER_ON
#define DGYLOG_OWNER "dgy"
#else
#define DGYLOG_OWNER NULL
#endif

#if DGYLOG_FILE_ON
#define DGYLOG_FILE __FILE__
#else
#define DGYLOG_FILE NULL
#endif

#if DGYLOG_LINE_ON
#define DGYLOG_LINE __LINE__
#else
#define DGYLOG_LINE NULL
#endif

#if DGYLOG_FUNC_ON
#define DGYLOG_FUNC __func__
#else
#define DGYLOG_FUNC NULL
#endif

#if NULOG_LEVEL < NULOG_LEVEL_LOGD
#define DGYLOGD(...)
#else
#define DGYLOGD(...) ((void)NULOG(LOG_DEBUG, LOG_TAG, DGYLOG_OWNER, DGYLOG_FILE, DGYLOG_FUNC, DGYLOG_LINE, __VA_ARGS__))
#endif

#if NULOG_LEVEL < NULOG_LEVEL_LOGE
#define DGYLOGE(...)
#else
#define DGYLOGE(...) ((void)NULOG(LOG_ERROR, LOG_TAG, DGYLOG_OWNER, DGYLOG_FILE, DGYLOG_FUNC, DGYLOG_LINE, __VA_ARGS__))
#endif
#else
#define DGYLOGD(...)
#define DGYLOGE(...)
#endif

#if (NULOG_LEVEL && LYBLOG_ON)

#if LYBLOG_OWNER_ON
#define LYBLOG_OWNER "lyb"
#else
#define LYBLOG_OWNER NULL
#endif

#if LYBLOG_FILE_ON
#define LYBLOG_FILE __FILE__
#else
#define LYBLOG_FILE NULL
#endif

#if LYBLOG_LINE_ON
#define LYBLOG_LINE __LINE__
#else
#define LYBLOG_LINE NULL
#endif

#if LYBLOG_FUNC_ON
#define LYBLOG_FUNC __func__
#else
#define LYBLOG_FUNC NULL
#endif

#if NULOG_LEVEL < NULOG_LEVEL_LOGD
#define LYBLOGD(...)
#else
#define LYBLOGD(...) ((void)NULOG(LOG_DEBUG, LOG_TAG, LYBLOG_OWNER, LYBLOG_FILE, LYBLOG_FUNC, LYBLOG_LINE, __VA_ARGS__))
#endif

#if NULOG_LEVEL < NULOG_LEVEL_LOGE
#define LYBLOGE(...)
#else
#define LYBLOGE(...) ((void)NULOG(LOG_ERROR, LOG_TAG, LYBLOG_OWNER, LYBLOG_FILE, LYBLOG_FUNC, LYBLOG_LINE, __VA_ARGS__))
#endif
#else
#define LYBLOGD(...)
#define LYBLOGE(...)
#endif

#if (NULOG_LEVEL && LCHLOG_ON)

#if LCHLOG_OWNER_ON
#define LCHLOG_OWNER "LCH"
#else
#define LCHLOG_OWNER NULL
#endif

#if LCHLOG_FILE_ON
#define LCHLOG_FILE __FILE__
#else
#define LCHLOG_FILE NULL
#endif

#if LCHLOG_LINE_ON
#define LCHLOG_LINE __LINE__
#else
#define LCHLOG_LINE NULL
#endif

#if LCHLOG_FUNC_ON
#define LCHLOG_FUNC __func__
#else
#define LCHLOG_FUNC NULL
#endif

#if NULOG_LEVEL < NULOG_LEVEL_LOGD
#define LCHLOGV(...)
#define LCHLOGE(...)
#define LCHLOGD(...)
#define LCHLOGD1(...)
#define LCHLOGD2(...)
#define LCHLOGV1(...)
#define LCHLOGV2(...)
#else
#define LCHDEBUG(...)           ((void)NULOG(LOG_DEBUG, LOG_TAG, NULL, NULL, NULL, NULL, __VA_ARGS__))
#define LCHLOGV(fmt, argv...) LCHDEBUG("[SeanLiu -- %s:%s:%d]"fmt, __FILE__,__FUNCTION__,__LINE__,##argv)
#define LCHLOGE(fmt, argv...) LCHDEBUG("[SeanLiu -- %s:%s:%d]"fmt, __FILE__,__FUNCTION__,__LINE__,##argv)
#define LCHLOGD(fmt, argv...) LCHDEBUG("[SeanLiu -- %s:%s:%d]"fmt, __FILE__,__FUNCTION__,__LINE__,##argv)
#define LCHLOGD1(fmt, argv...) LCHDEBUG("[SeanLiu -- %s:%d]"fmt, __FUNCTION__,__LINE__,##argv)
#define LCHLOGV1(fmt, argv...) LCHDEBUG("[SeanLiu -- %s:%d]"fmt, __FUNCTION__,__LINE__,##argv)
#define LCHLOGD2(fmt, argv...) LCHDEBUG("[SeanLiu -- ]"fmt, ##argv)
#define LCHLOGV2(fmt, argv...) LCHDEBUG("[SeanLiu -- ]"fmt, ##argv)
#endif

#if NULOG_LEVEL < NULOG_LEVEL_LOGE
#define LCHLOGE(...)
#else
#define LCHLOGE(...) ((void)NULOG(LOG_ERROR, LOG_TAG, LCHLOG_OWNER, LCHLOG_FILE, LCHLOG_FUNC, LCHLOG_LINE, __VA_ARGS__))
#endif
#else
#if 1
#define LCHLOGV(...)
#define LCHLOGE(...)
#define LCHLOGD(...)
#define LCHLOGD1(...)
#define LCHLOGD2(...)
#define LCHLOGV1(...)
#define LCHLOGV2(...)
#else
#define LCHDEBUG(...)   ((void)NULOG(LOG_DEBUG, LOG_TAG, NULL, NULL, NULL, NULL, __VA_ARGS__))
#define LCHLOGV(fmt, argv...) LCHDEBUG("[SeanLiu -- %s:%s:%d]"fmt, __FILE__,__FUNCTION__,__LINE__,##argv)
#define LCHLOGE(fmt, argv...) LCHDEBUG("[SeanLiu -- %s:%s:%d]"fmt, __FILE__,__FUNCTION__,__LINE__,##argv)
#define LCHLOGD(fmt, argv...) LCHDEBUG("[SeanLiu -- %s:%s:%d]"fmt, __FILE__,__FUNCTION__,__LINE__,##argv)
#define LCHLOGD1(fmt, argv...) LCHDEBUG("[SeanLiu -- %s:%d]"fmt, __FUNCTION__,__LINE__,##argv)
#define LCHLOGV1(fmt, argv...) LCHDEBUG("[SeanLiu -- %s:%d]"fmt, __FUNCTION__,__LINE__,##argv)
#define LCHLOGD2(fmt, argv...) LCHDEBUG("[SeanLiu -- ]"fmt, ##argv)
#define LCHLOGV2(fmt, argv...) LCHDEBUG("[SeanLiu -- ]"fmt, ##argv)
#endif
#endif

//LCH: add to replace assert()
#define BacktraceAssert(exp)   do { if (!(exp)) { LOGE("ASSERT FAILURE: (%s) at %s:%s:%d)", #exp, __FILE__, __FUNCTION__, __LINE__); } } while (0)
#define LCHISEXIST(sp)  ((sp == NULL) ? 0x0 : 0x1)

#if (NULOG_LEVEL && GSLOG_ON)

#if GSLOG_OWNER_ON
#define GSLOG_OWNER "GS"
#else
#define GSLOG_OWNER NULL
#endif

#if GSLOG_FILE_ON
#define GSLOG_FILE __FILE__
#else
#define GSLOG_FILE NULL
#endif

#if GSLOG_LINE_ON
#define GSLOG_LINE __LINE__
#else
#define GSLOG_LINE NULL
#endif

#if GSLOG_FUNC_ON
#define GSLOG_FUNC __func__
#else
#define GSLOG_FUNC NULL
#endif

#if NULOG_LEVEL < NULOG_LEVEL_LOGD
#define GSLOGD(...)
#else
#define GSLOGD(...) ((void)NULOG(LOG_DEBUG, LOG_TAG, GSLOG_OWNER, GSLOG_FILE, GSLOG_FUNC, GSLOG_LINE, __VA_ARGS__))
#endif

#if NULOG_LEVEL < NULOG_LEVEL_LOGE
#define GSLOGE(...)
#else
#define GSLOGE(...) ((void)NULOG(LOG_ERROR, LOG_TAG, GSLOG_OWNER, GSLOG_FILE, GSLOG_FUNC, GSLOG_LINE, __VA_ARGS__))
#endif
#else
#define GSLOGD(...)
#define GSLOGE(...)
#endif

#if (NULOG_LEVEL && DXZLOG_ON)

#if DXZLOG_OWNER_ON
#define DXZLOG_OWNER "WIKI"
#else
#define DXZLOG_OWNER NULL
#endif

#if DXZLOG_FILE_ON
#define DXZLOG_FILE __FILE__
#else
#define DXZLOG_FILE NULL
#endif

#if DXZLOG_LINE_ON
#define DXZLOG_LINE __LINE__
#else
#define DXZLOG_LINE NULL
#endif

#if DXZLOG_FUNC_ON
#define DXZLOG_FUNC __func__
#else
#define DXZLOG_FUNC NULL
#endif

#if NULOG_LEVEL < NULOG_LEVEL_LOGD
#define DXZLOGD(...)
#else
#define DXZLOGD(...) ((void)NULOG(LOG_DEBUG, LOG_TAG, DXZLOG_OWNER, DXZLOG_FILE, DXZLOG_FUNC, DXZLOG_LINE, __VA_ARGS__))
#endif

#if NULOG_LEVEL < NULOG_LEVEL_LOGE
#define DXZLOGE(...)
#else
#define DXZLOGE(...) ((void)NULOG(LOG_ERROR, LOG_TAG, DXZLOG_OWNER, DXZLOG_FILE, DXZLOG_FUNC, DXZLOG_LINE, __VA_ARGS__))
#endif
#else
#define DXZLOGD(...)
#define DXZLOGE(...)

#endif

#ifndef LITERAL_TO_STRING_INTERNAL(x)
#define LITERAL_TO_STRING_INTERNAL(x)    #x
#endif

#ifndef LITERAL_TO_STRING(x)
#define LITERAL_TO_STRING(x) LITERAL_TO_STRING_INTERNAL(x)
#endif

#ifndef CHECK_EQ(x,y)
#define CHECK_EQ(x,y)                                                   \
    LOG_ALWAYS_FATAL_IF(                                                \
            (x) != (y),                                                 \
            __FILE__ ":" LITERAL_TO_STRING(__LINE__) " " #x " != " #y)

#endif

#ifndef CHECK(x)
#define CHECK(x)                                                        \
    LOG_ALWAYS_FATAL_IF(                                                \
            !(x),                                                       \
            __FILE__ ":" LITERAL_TO_STRING(__LINE__) " " #x)

#endif
#endif // _LIBS_UTILS_NULOG_H
