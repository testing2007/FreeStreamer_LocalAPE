//
//  player_debug.h
//  KWCore
//
//  Created by 魏志强 on 16/2/4.
//  Copyright © 2016年 yeelion. All rights reserved.
//

#ifndef player_debug_h
#define player_debug_h

//audio_queue
//#define AQ_DEBUG 1

#if !defined (AQ_DEBUG)
#define AQ_TRACE(...) do {} while (0)
#define AQ_ASSERT(...) do {} while (0)
#else
#include <cassert>
#include <pthread.h>
#define AQ_TRACE(...) printf("[audio_queue.cpp:%i thread %x] ", __LINE__, pthread_mach_thread_np(pthread_self())); printf(__VA_ARGS__)
#define AQ_ASSERT(...) assert(__VA_ARGS__)
#endif

//audio_stream
//#define AS_DEBUG 1

#if !defined (AS_DEBUG)
#define AS_TRACE(...) do {} while (0)
#else
#include <pthread.h>
#define AS_TRACE(...) printf("[audio_stream.cpp:%i thread %x] ", __LINE__, pthread_mach_thread_np(pthread_self())); printf(__VA_ARGS__)
#endif

//http_stream
//#define HS_DEBUG 1

#if !defined (HS_DEBUG)
#define HS_TRACE(...) do {} while (0)
#define HS_TRACE_CFSTRING(X) do {} while (0)
#else
#define HS_TRACE(...) printf(__VA_ARGS__)
#define HS_TRACE_CFSTRING(X) HS_TRACE("%s\n", CFStringGetCStringPtr(X, kCFStringEncodingMacRoman))
#endif

//file_stream
//#define FS_DEBUG 1

#if !defined (FS_DEBUG)
#define FS_TRACE(...) do {} while (0)
#else
#include <pthread.h>
#define FS_TRACE(...) printf("[file_stream.cpp:%i thread %x] ", __LINE__, pthread_mach_thread_np(pthread_self())); printf(__VA_ARGS__)
#endif

//caching_stream
//#define CS_DEBUG 1

#if !defined (CS_DEBUG)
#define CS_TRACE(...) do {} while (0)
#define CS_TRACE_CFSTRING(X) do {} while (0)
#define CS_TRACE_CFURL(X) do {} while (0)
#else
#define CS_TRACE(...) printf(__VA_ARGS__)
#define CS_TRACE_CFSTRING(X) CS_TRACE("%s\n", CFStringGetCStringPtr(X, kCFStringEncodingMacRoman))
#define CS_TRACE_CFURL(X) CS_TRACE_CFSTRING(CFURLGetString(X))
#endif








#endif /* player_debug_h */
