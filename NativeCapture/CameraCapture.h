//
// Created by Charlie on 2023/11/10.
//

#ifndef USB_DEVICE_CAMERACAPTURE_H
#define USB_DEVICE_CAMERACAPTURE_H

#include <camera/NdkCameraCaptureSession.h>
#include <camera/NdkCameraDevice.h>
#include <camera/NdkCameraError.h>
#include <camera/NdkCameraManager.h>
#include <camera/NdkCaptureRequest.h>
#include <media/NdkImage.h>
#include <media/NdkImageReader.h>
#include <stdint.h>

#include <mutex>


namespace android {

static constexpr uint32_t kDefaultImageWidth = 1920;
static constexpr uint32_t kDefaultImageHeight = 1080;
static constexpr uint32_t kDefaultImageFormat = AIMAGE_FORMAT_YUV_420_888;
static constexpr uint64_t kDefaultImageUsage =
    AHARDWAREBUFFER_USAGE_CPU_READ_OFTEN;
static constexpr uint32_t kDefaultImageCount = 32;

template <typename T>
class A {
    friend T;
};
using AA = A<int>;

class B;
typedef B BB;

class CameraCapture {
public:
    CameraCapture();

    ~CameraCapture();

    static CameraCapture* getInstance();

    int startCapture(int32_t width = kDefaultImageWidth,
                     int32_t height = kDefaultImageHeight,
                     int32_t format = kDefaultImageFormat,
                     uint64_t usage = kDefaultImageUsage,
                     int32_t maxImages = kDefaultImageCount);

    int stopCapture();

private:
    int32_t mWidth;
    int32_t mHeight;
    int32_t mFormat;
    uint64_t mUsage;
    uint32_t mMaxImages;

    size_t mAcquiredImageCount{0};

    std::mutex mImageMutex;

    AImageReader* mImgReader{nullptr};

    ANativeWindow* mImgReaderAnw{nullptr};

    int initImageReader();

    static void onImageAvailable(void* obj, AImageReader* reader);

    AImageReader_ImageListener mReaderAvailableCb{this, onImageAvailable};

    void handleImageAvailable(AImageReader* reader);

    static void onDeviceDisconnected(void* /*obj*/, ACameraDevice* /*device*/) {
    }
    static void onDeviceError(void* /*obj*/, ACameraDevice* /*device*/,
                              int /*errorCode*/) {}

    static void onSessionClosed(void* /*obj*/,
                                ACameraCaptureSession* /*session*/) {}
    static void onSessionReady(void* /*obj*/,
                               ACameraCaptureSession* /*session*/) {}
    static void onSessionActive(void* /*obj*/,
                                ACameraCaptureSession* /*session*/) {}

    ACameraDevice_StateCallbacks mDeviceCb{this, onDeviceDisconnected,
                                           onDeviceError};
    ACameraCaptureSession_stateCallbacks mSessionCb{
        this, onSessionClosed, onSessionReady, onSessionActive};

    const char* mCameraId{nullptr};
    // Camera manager
    ACameraManager* mCameraManager{nullptr};
    ACameraIdList* mCameraIdList{nullptr};
    // Camera device
    ACameraMetadata* mCameraMetadata{nullptr};
    ACameraDevice* mDevice{nullptr};
    // Capture session
    ACaptureSessionOutputContainer* mOutputs{nullptr};
    ACaptureSessionOutput* mImgReaderOutput{nullptr};
    ACameraCaptureSession* mSession{nullptr};
    // Capture request
    ACaptureRequest* mCaptureRequest{nullptr};
    ACameraOutputTarget* mReqImgReaderOutput{nullptr};

    int initCamera();
};

}  // namespace android

#endif  // USB_DEVICE_CAMERACAPTURE_H
