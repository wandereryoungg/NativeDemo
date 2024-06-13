//
// Created by Charlie on 2024/3/4.
//

#ifndef NATIVECAPTURE_YOUNGTESTPLUS_H
#define NATIVECAPTURE_YOUNGTESTPLUS_H

namespace android {

class YoungTestPlus {
public:
    YoungTestPlus();

    ~YoungTestPlus();

    static YoungTestPlus* getInstance();
};

}  // namespace android

#endif  // NATIVECAPTURE_YOUNGTESTPLUS_H
