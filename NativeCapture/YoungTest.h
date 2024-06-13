//
// Created by Charlie on 2024/3/2.
//

#ifndef NATIVECAPTURE_YOUNGTEST_H
#define NATIVECAPTURE_YOUNGTEST_H

namespace android {

class YoungTest {
public:
    YoungTest();

    ~YoungTest();

    static YoungTest* getInstance();
};

}  // namespace android
#endif  // NATIVECAPTURE_YOUNGTEST_H
