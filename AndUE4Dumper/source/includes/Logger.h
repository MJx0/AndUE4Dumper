#pragma once

#ifdef _EXECUTABLE

#include <cstdio>

#define LOGV(fmt, ...) printf("V : " fmt "\n", ##__VA_ARGS__)
#define LOGI(fmt, ...) printf("I : " fmt "\n", ##__VA_ARGS__)
#define LOGW(fmt, ...) printf("W : " fmt "\n", ##__VA_ARGS__)
#define LOGE(fmt, ...) printf("E : " fmt "\n", ##__VA_ARGS__)
#define LOGF(fmt, ...) printf("F : " fmt "\n", ##__VA_ARGS__)

#ifndef NDEBUG
#define LOGD(fmt, ...) printf("D : " fmt "\n", ##__VA_ARGS__)
#else
#define LOGD(...)
#endif

#else

#include <android/log.h>

#define LOG_TAG "UE4Dump3r"

#define LOGV(...) ((void)__android_log_print(ANDROID_LOG_VERBOSE, LOG_TAG, __VA_ARGS__))
#define LOGI(...) ((void)__android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__))
#define LOGW(...) ((void)__android_log_print(ANDROID_LOG_WARN, LOG_TAG, __VA_ARGS__))
#define LOGE(...) ((void)__android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__))
#define LOGF(...) ((void)__android_log_print(ANDROID_LOG_FATAL, LOG_TAG, __VA_ARGS__))

#ifndef NDEBUG
#define LOGD(...) ((void)__android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__))
#else
#define LOGD(...)
#endif

#endif