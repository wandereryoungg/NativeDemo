LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

#TARGET_ARCH_ABI := arm64-v8a
#OPENCV_LIB_TYPE := STATIC
#include $(LOCAL_PATH)/OpenCV-android-sdk/sdk/native/jni/OpenCV.mk

LOCAL_MODULE := NativeCapture

LOCAL_CFLAGS += -Wall -Werror -Wunused -Wunreachable-code -Wno-deprecated-declarations -fexceptions

LOCAL_MULTILIB := 64

LOCAL_SRC_FILES := \
    CameraCapture.cpp \
    AudioUtil.cpp \
    main_test.cpp \
    YoungTest.cpp \
    YoungTestPlus.cpp

LOCAL_SHARED_LIBRARIES := \
    libbase \
    libbinder \
    liblog \
    libutils \
    libmediandk \
    libnativewindow \
    libcamera2ndk \
    libOpenSLES

include $(BUILD_EXECUTABLE)