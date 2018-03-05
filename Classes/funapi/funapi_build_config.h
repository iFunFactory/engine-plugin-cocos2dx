// Copyright (C) 2013-2018 iFunFactory Inc. All Rights Reserved.
//
// This work is confidential and proprietary to iFunFactory Inc. and
// must not be used, disclosed, copied, or distributed without the prior
// consent of iFunFactory Inc.

#ifndef SRC_FUNAPI_BUILD_CONFIG_H_
#define SRC_FUNAPI_BUILD_CONFIG_H_

#define FUNAPI_COCOS2D

#define FUNAPI_API
#define WITH_FUNAPI 1

#define FUNAPI_HAVE_ZLIB 0
#define FUNAPI_HAVE_ZSTD 0
#define FUNAPI_HAVE_DELAYED_ACK 0
#define FUNAPI_HAVE_TCP_TLS 0
#define FUNAPI_HAVE_RPC 0
#define FUNAPI_HAVE_WEBSOCKET 0
#define FUNAPI_HAVE_SODIUM 1
#define FUNAPI_HAVE_AES128 1

#if defined(_WIN32)
#define FUNAPI_PLATFORM_WINDOWS
#define FUNAPI_COCOS2D_PLATFORM_WINDOWS
#endif

#if COCOS2D_DEBUG > 0
#define DEBUG_LOG
#endif

#ifdef FUNAPI_PLATFORM_WINDOWS
#define NOMINMAX
#endif

#endif  // SRC_FUNAPI_BUILD_CONFIG_H_
