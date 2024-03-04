//
// Created by Charlie on 2024/3/4.
//
#define LOG_TAG "YoungTestPlus"

#include "YoungTestPlus.h"
#include "Test.h"

namespace android {

YoungTestPlus::YoungTestPlus() {
	ALOGI("%s   %p", __func__, this);

	testSize<double>();
}


YoungTestPlus::~YoungTestPlus() {
	ALOGI("%s   %p", __func__, this);
}

YoungTestPlus* YoungTestPlus::getInstance() {
	static YoungTestPlus sInstance;
	return &sInstance;
}

}
