//
// Created by Charlie on 2023/11/10.
//

#define LOG_TAG "CameraCapture"

#include "CameraCapture.h"

#include <android/log.h>
#include <vector>
#include <unistd.h>

#define ALOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define ALOGW(...) __android_log_print(ANDROID_LOG_WARN, LOG_TAG, __VA_ARGS__)
#define ALOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

using namespace std;

namespace android {

CameraCapture::CameraCapture() {
	ALOGI("%s %p", __func__, this);
}

CameraCapture::~CameraCapture() {
	ALOGI("%s %p", __func__, this);
}

CameraCapture* CameraCapture::getInstance() {
	static CameraCapture cameraCapture;
	return &cameraCapture;
}

int CameraCapture::startCapture(int32_t width, int32_t height, int32_t format, uint64_t usage, int32_t maxImages) {
	mWidth = width;
	mHeight = height;
	mFormat = format;
	mUsage = usage;
	mMaxImages = maxImages;
	ALOGI("%s   mWidth: %d   mHeight: %d   mFormat: %d   mUsage: %" PRIu64 "   mMaxImages: %d", __func__, mWidth, mHeight, mFormat, mUsage, mMaxImages);
	int ret = initImageReader();
	if (ret) {
		return ret;
	}
	ret = initCamera();
	return ret;
}

int CameraCapture::stopCapture() {
	ALOGI("%s", __func__);
	ACameraCaptureSession_stopRepeating(mSession);
	ACaptureSessionOutputContainer_remove(mOutputs, mImgReaderOutput);
	ACaptureRequest_removeTarget(mCaptureRequest, mReqImgReaderOutput);

	if (mReqImgReaderOutput) {
		ACameraOutputTarget_free(mReqImgReaderOutput);
		mReqImgReaderOutput = nullptr;
	}

	if (mCaptureRequest) {
		ACaptureRequest_free(mCaptureRequest);
		mCaptureRequest = nullptr;
	}

	if (mImgReaderOutput) {
		ACaptureSessionOutput_free(mImgReaderOutput);
		mImgReaderOutput = nullptr;
	}

	if (mOutputs) {
		ACaptureSessionOutputContainer_free(mOutputs);
		mOutputs = nullptr;
	}

	if (mCameraMetadata) {
		ACameraMetadata_free(mCameraMetadata);
		mCameraMetadata = nullptr;
	}

	if (mDevice) {
		ACameraDevice_close(mDevice);
		mDevice = nullptr;
	}

	if (mCameraIdList) {
		ACameraManager_deleteCameraIdList(mCameraIdList);
		mCameraIdList = nullptr;
	}

	if (mImgReader) {
		AImageReader_delete(mImgReader);
		mImgReader = nullptr;
		mImgReaderAnw = nullptr;
	}

	if (mCameraManager) {
		ACameraManager_delete(mCameraManager);
		mCameraManager = nullptr;
	}

	return ACAMERA_OK;
}

int CameraCapture::initImageReader() {
	if (mImgReader != nullptr || mImgReaderAnw != nullptr) {
		ALOGE("Cannot re-init image reader, mImgReader=%p, mImgReaderAnw=%p", mImgReader, mImgReaderAnw);
		return -1;
	}
	ALOGI("%s   mUsage: %" PRIu64 "", __func__, mUsage);
	int ret = AImageReader_newWithUsage(mWidth, mHeight, mFormat, mUsage, mMaxImages, &mImgReader);
	if (ret != AMEDIA_OK || mImgReader == nullptr) {
		ALOGE("Failed to create new AImageReader, ret=%d, mImgReader=%p", ret, mImgReader);
		return -1;
	}

	ret = AImageReader_setImageListener(mImgReader, &mReaderAvailableCb);
	if (ret != AMEDIA_OK) {
		ALOGE("Failed to set image available listener, ret=%d.", ret);
		return ret;
	}

	ret = AImageReader_getWindow(mImgReader, &mImgReaderAnw);
	if (ret != AMEDIA_OK || mImgReaderAnw == nullptr) {
		ALOGE("Failed to get ANativeWindow from AImageReader, ret=%d, mImgReaderAnw=%p.", ret, mImgReaderAnw);
		return -1;
	}

	return 0;
}

void CameraCapture::onImageAvailable(void* obj, AImageReader* reader) {
	CameraCapture* thiz = reinterpret_cast<CameraCapture*>(obj);
	thiz->handleImageAvailable(reader);
}

void CameraCapture::handleImageAvailable(AImageReader* reader) {
	std::lock_guard<std::mutex> lock(mImageMutex);

	usleep(100000);

	AImage* outImage = nullptr;
	media_status_t ret;
	// Make sure AImage will be deleted automatically when it goes out of scope.
	auto imageDeleter = [](AImage* img) {
		AImage_delete(img);
	};
	std::unique_ptr<AImage, decltype(imageDeleter)> img(nullptr, imageDeleter);

	ret = AImageReader_acquireNextImage(reader, &outImage);
	if (ret != AMEDIA_OK || outImage == nullptr) {
		ALOGE("Failed to acquire image, ret=%d, outImage=%p.", ret, outImage);
		return;
	}
	img.reset(outImage);

	AHardwareBuffer* outBuffer = nullptr;
	ret = AImage_getHardwareBuffer(img.get(), &outBuffer);
	if (ret != AMEDIA_OK || outBuffer == nullptr) {
		ALOGE("Failed to get hardware buffer, ret=%d, outBuffer=%p.", ret, outBuffer);
		return;
	}

	// No need to release AHardwareBuffer, since we don't acquire additional reference to it.
	AHardwareBuffer_Desc outDesc;
	AHardwareBuffer_describe(outBuffer, &outDesc);
	int32_t imageWidth = 0;
	int32_t imageHeight = 0;
	int32_t bufferWidth = static_cast<int32_t>(outDesc.width);
	int32_t bufferHeight = static_cast<int32_t>(outDesc.height);

	if (mFormat == AIMAGE_FORMAT_RGBA_8888 ||
		mFormat == AIMAGE_FORMAT_RGBX_8888 ||
		mFormat == AIMAGE_FORMAT_RGB_888 ||
		mFormat == AIMAGE_FORMAT_RGB_565 ||
		mFormat == AIMAGE_FORMAT_RGBA_FP16 ||
		mFormat == AIMAGE_FORMAT_YUV_420_888 ||
		mFormat == AIMAGE_FORMAT_Y8) {
		// Check output buffer dimension for certain formats. Don't do this for blob based formats.
		if (bufferWidth != mWidth || bufferHeight != mHeight) {
			ALOGE("Mismatched output buffer dimension: expected=%dx%d, actual=%dx%d", mWidth, mHeight, bufferWidth, bufferHeight);
			return;
		}
	}

	AImage_getWidth(outImage, &imageWidth);
	AImage_getHeight(outImage, &imageHeight);
	if (imageWidth != mWidth || imageHeight != mHeight) {
		ALOGE("Mismatched output image dimension: expected=%dx%d, actual=%dx%d", mWidth, mHeight, imageWidth, imageHeight);
		return;
	}

	if ((outDesc.usage & mUsage) != mUsage) {
		ALOGE("Mismatched output buffer usage: actual (%" PRIu64 "), expected (%" PRIu64 ")", outDesc.usage, mUsage);
		return;
	}

	uint8_t* data = nullptr;
	int dataLength = 0;
	ret = AImage_getPlaneData(img.get(), 0, &data, &dataLength);
	if (mUsage & AHARDWAREBUFFER_USAGE_CPU_READ_OFTEN) {
		// When we have CPU_READ_OFTEN usage bits, we can lock the image.
		if (ret != AMEDIA_OK || data == nullptr || dataLength < 0) {
			ALOGE("Failed to access CPU data, ret=%d, data=%p, dataLength=%d", ret, data, dataLength);
			return;
		}
	} else {
		if (ret != AMEDIA_IMGREADER_CANNOT_LOCK_IMAGE || data != nullptr || dataLength != 0) {
			ALOGE("Shouldn't be able to access CPU data, ret=%d, data=%p, dataLength=%d", ret, data, dataLength);
			return;
		}
	}
	// Only increase mAcquiredImageCount if all checks pass.
	mAcquiredImageCount++;
	ALOGI("%s   mAcquiredImageCount: %zu   dataLength: %d", __func__, mAcquiredImageCount, dataLength);
	if (mAcquiredImageCount == 100) {
		FILE* file;
		char fileName[128];
		sprintf(fileName, "/data/camera/camera_dump_%zu.yuv", mAcquiredImageCount);
		file = fopen(fileName, "wb+");
		if (file) {
			ALOGI("%s:   write fileName: %s", __FUNCTION__, fileName);
			fwrite(data, 1, mWidth * mHeight * 3 / 2, file);
			fclose(file);
		} else {
			ALOGI("%s:   fopen fileName: %s error: %s", __FUNCTION__, fileName, strerror(errno));
		}
	}

}


int CameraCapture::initCamera() {
	if (mImgReaderAnw == nullptr) {
		ALOGE("Cannot initialize camera before image reader get initialized.");
		return -1;
	}

	mCameraManager = ACameraManager_create();
	if (mCameraManager == nullptr) {
		ALOGE("Failed to create ACameraManager.");
		return -1;
	}

	int ret = ACameraManager_getCameraIdList(mCameraManager, &mCameraIdList);
	if (ret != AMEDIA_OK) {
		ALOGE("Failed to get cameraIdList: ret=%d", ret);
		return ret;
	}
	if (mCameraIdList->numCameras < 1) {
		ALOGW("Device has no NDK compatible camera.");
		return 0;
	}
	ALOGI("Found %d camera(s).", mCameraIdList->numCameras);

	// We always use the first camera.
	mCameraId = mCameraIdList->cameraIds[0];
	if (mCameraId == nullptr) {
		ALOGE("Failed to get cameraId.");
		return -1;
	}

	ret = ACameraManager_openCamera(mCameraManager, mCameraId, &mDeviceCb,
		&mDevice);
	if (ret != AMEDIA_OK || mDevice == nullptr) {
		ALOGE("Failed to open camera, ret=%d, mDevice=%p.", ret, mDevice);
		return -1;
	}

	ret = ACameraManager_getCameraCharacteristics(mCameraManager, mCameraId,
		&mCameraMetadata);
	if (ret != ACAMERA_OK || mCameraMetadata == nullptr) {
		ALOGE("Get camera %s characteristics failure. ret %d, metadata %p",
			mCameraId, ret, mCameraMetadata);
		return -1;
	}

	// Create capture session
	ret = ACaptureSessionOutputContainer_create(&mOutputs);
	if (ret != AMEDIA_OK) {
		ALOGE("ACaptureSessionOutputContainer_create failed, ret=%d", ret);
		return ret;
	}
	ret = ACaptureSessionOutput_create(mImgReaderAnw, &mImgReaderOutput);
	if (ret != AMEDIA_OK) {
		ALOGE("ACaptureSessionOutput_create failed, ret=%d", ret);
		return ret;
	}
	ret = ACaptureSessionOutputContainer_add(mOutputs, mImgReaderOutput);
	if (ret != AMEDIA_OK) {
		ALOGE("ACaptureSessionOutputContainer_add failed, ret=%d", ret);
		return ret;
	}
	ret = ACameraDevice_createCaptureSession(mDevice, mOutputs, &mSessionCb,
		&mSession);
	if (ret != AMEDIA_OK) {
		ALOGE("ACameraDevice_createCaptureSession failed, ret=%d", ret);
		return ret;
	}

	// Create capture request
	ret = ACameraDevice_createCaptureRequest(mDevice, TEMPLATE_RECORD,
		&mCaptureRequest);
	if (ret != AMEDIA_OK) {
		ALOGE("ACameraDevice_createCaptureRequest failed, ret=%d", ret);
		return ret;
	}

	std::vector<int32_t> fpsRanges = { 60, 60 };
	ret = ACaptureRequest_setEntry_i32(mCaptureRequest, ACAMERA_CONTROL_AE_TARGET_FPS_RANGE, fpsRanges.size(), fpsRanges.data());
	if (ret != AMEDIA_OK) {
		ALOGE("ACaptureRequest_setEntry_i32 failed, ret=%d", ret);
		return ret;
	}

	ret = ACameraOutputTarget_create(mImgReaderAnw, &mReqImgReaderOutput);
	if (ret != AMEDIA_OK) {
		ALOGE("ACameraOutputTarget_create failed, ret=%d", ret);
		return ret;
	}
	ret = ACaptureRequest_addTarget(mCaptureRequest, mReqImgReaderOutput);
	if (ret != AMEDIA_OK) {
		ALOGE("ACaptureRequest_addTarget failed, ret=%d", ret);
		return ret;
	}

	ACameraCaptureSession_captureCallbacks mCaptureCallbacks = {
		this, // context
		nullptr, // onCaptureStarted
		nullptr, // onCaptureProgressed
		[](void* , ACameraCaptureSession*, ACaptureRequest*,
										  const ACameraMetadata*) {
			ALOGI("%s", __func__);
		},
		nullptr, // onCaptureFailed
		nullptr, // onCaptureSequenceCompleted
		nullptr, // onCaptureSequenceAborted
		nullptr, // onCaptureBufferLost
	};

	int reqSeq;
	ACameraCaptureSession_setRepeatingRequest(mSession, &mCaptureCallbacks, 1, &mCaptureRequest, &reqSeq);

	return 0;
}

}