//
// Created by Charlie on 2024/3/4.
//
#define LOG_TAG "YoungTestPlus"

#include "YoungTestPlus.h"

#include "Test.h"

// #include <opencv2/opencv.hpp>

namespace android {

YoungTestPlus::YoungTestPlus() {
    ALOGI("%s   %p", __func__, this);

    testSize<double>();

	/*
    int rectWidth = 1920;
    int rectHeight = 1080;

    char* data = new char[rectWidth * rectHeight * 4];
    cv::Mat image(rectHeight, rectWidth, CV_8UC4, data);
    image.setTo(cv::Scalar(0, 0, 0, 0));
    cv::rectangle(image, cv::Rect(0, 0, rectWidth, rectHeight),
                  cv::Scalar(0, 255, 0), 2);
    const char* fileName = "/data/camera/YoungTestPlus.rgb";
    FILE* file = fopen(fileName, "wb+");
    if (file) {
        ALOGI("%s:   write fileName: %s", __FUNCTION__, fileName);
        fwrite(data, 1, rectWidth * rectHeight * 4, file);
        fclose(file);
    } else {
        ALOGI("%s:   fopen fileName: %s error: %s", __FUNCTION__, fileName,
              strerror(errno));
    }
    delete[] data;
    */
}

YoungTestPlus::~YoungTestPlus() { ALOGI("%s   %p", __func__, this); }

YoungTestPlus* YoungTestPlus::getInstance() {
    static YoungTestPlus sInstance;
    return &sInstance;
}

}  // namespace android
