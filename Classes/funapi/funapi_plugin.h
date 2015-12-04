// Copyright (C) 2013-2015 iFunFactory Inc. All Rights Reserved.
//
// This work is confidential and proprietary to iFunFactory Inc. and
// must not be used, disclosed, copied, or distributed without the prior
// consent of iFunFactory Inc.

#pragma once

#if CC_TARGET_PLATFORM == CC_PLATFORM_WIN32 || CC_TARGET_PLATFORM == CC_PLATFORM_IOS || CC_TARGET_PLATFORM == CC_PLATFORM_ANDROID || CC_TARGET_PLATFORM == CC_PLATFORM_MAC
  #ifndef FUNAPI_TARGET_COCOS2D
  #define FUNAPI_TARGET_COCOS2D
  #endif
#endif

#ifdef FUNAPI_TARGET_COCOS2D
#include "cocos2d.h"
#else
#define FUNAPI_TARGET_UE4
#include "Engine.h"
#include "UnrealString.h"
#include "Json.h"
#endif

#if PLATFORM_WINDOWS
#include "AllowWindowsPlatformTypes.h"
#endif

#if CC_TARGET_PLATFORM == CC_PLATFORM_WIN32 || PLATFORM_WINDOWS
#include <process.h>
#include <WinSock2.h>
#include <ws2tcpip.h>
#include <io.h>
#include <direct.h>
#else
#include <netdb.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <assert.h>
#endif

#include <map>
#include <string>
#include <list>
#include <sstream>
#include <functional>
#include <queue>
#include <mutex>
#include <memory>
#include <condition_variable>
#include <thread>

#include "curl/curl.h"

#ifdef FUNAPI_TARGET_COCOS2D
#include "json/stringbuffer.h"
#include "json/writer.h"
#include "json/document.h"
#else
#include "curl/curl.h"
#include "rapidjson/stringbuffer.h"
#include "rapidjson/writer.h"
#include "rapidjson/document.h"
#endif

#if PLATFORM_WINDOWS
#include "HideWindowsPlatformTypes.h"
#endif

namespace fun {

namespace {
  typedef rapidjson::Document Json;
}

  
#ifdef FUNAPI_TARGET_COCOS2D
#define FUNAPI_LOG(fmt, ...)          CCLOG(fmt, ##__VA_ARGS__)
#define FUNAPI_LOG_WARNING(fmt, ...)  CCLOG(fmt, ##__VA_ARGS__)
#define FUNAPI_LOG_ERROR(fmt, ...)    CCLOG(fmt, ##__VA_ARGS__)
#elif FUNAPI_TARGET_UE4
DECLARE_LOG_CATEGORY_EXTERN(LogFunapi, Log, All);
#define FUNAPI_LOG(fmt, ...)          UE_LOG(LogFunapi, Log, TEXT(fmt), ##__VA_ARGS__)
#define FUNAPI_LOG_WARNING(fmt, ...)  UE_LOG(LogFunapi, Warning, TEXT(fmt), ##__VA_ARGS__)
#define FUNAPI_LOG_ERROR(fmt, ...)    UE_LOG(LogFunapi, Error, TEXT(fmt), ##__VA_ARGS__)
#else
#error "target is not defined"
#endif
  

} // namespace fun
