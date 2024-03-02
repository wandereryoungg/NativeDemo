//
// Created by Charlie on 2024/3/2.
//

#ifndef NATIVECAPTURE_TEST_H
#define NATIVECAPTURE_TEST_H

#include <android/log.h>

#define ALOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)

template <typename T> void testSize() {
	ALOGI("%s   size: %lu", __func__, sizeof(T));
}

#endif //NATIVECAPTURE_TEST_H
