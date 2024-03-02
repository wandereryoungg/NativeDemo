//
// Created by Charlie on 2024/3/2.
//

#ifndef NATIVECAPTURE_YOUNGTEST_H
#define NATIVECAPTURE_YOUNGTEST_H

#include <android/log.h>

#define ALOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)

namespace android {

class YoungTest {
    static YoungTest* getInstance();
};

}
#endif //NATIVECAPTURE_YOUNGTEST_H
