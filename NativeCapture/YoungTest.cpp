//
// Created by Charlie on 2024/3/2.
//

#include "YoungTest.h"
#include <thread>

namespace android {

int nums1[10240][10240];
int nums2[10240][10240];

char* mallocArray(int num) {
    ALOGI("%s num: %d start", __func__, num);
    assert (num > 0);
    ALOGI("%s num: %d end", __func__, num);
    return new char[num];
}

template <typename T, typename U> void bitCopy(T& t, U& u) {
    static_assert(sizeof(t) == sizeof(u), "copy error");

}

void Throw() {
    throw 1;
}

void noBlockThrow() {
    Throw();
}

void blockThrow() noexcept {
    Throw();
}

void testStr(int a = 0, int b = 1) {
    ALOGI("%s   a: %d b: %d", __func__, a, b);
}

void test() {

    testSize<double>();

    testStr(3);

    std::string str("hello");

    try {
        Throw();
    } catch (...) {
        ALOGI("%s %d found exception.", __func__, __LINE__);
    }
    try {
        noBlockThrow();
    } catch (...) {
        ALOGI("%s %d found exception.", __func__, __LINE__);
    }
    try {
        blockThrow();
    } catch (...) {
        ALOGI("%s %d found exception.", __func__, __LINE__);
    }


	thread thread1([&]{
        ALOGI("youngyoung thread1 start");
        for (int i = 0; i < 10240; i++) {
            for (int j = 0; j < 10240; j++) {
                nums1[i][j] = 10240;
            }
        }
        ALOGI("youngyoung thread1 end");
	});
	thread1.detach();

	thread thread2([&]{
        ALOGI("youngyoung thread2 start");
        for (int i = 0; i < 10240; i++) {
            for (int j = 0; j < 10240; j++) {
                nums2[j][i] = 10240;
            }
        }
        ALOGI("youngyoung thread2 end");
	});
	thread2.detach();

    ALOGI("%s   youngyoung:  %ld", __func__, __cplusplus);
#if __cplusplus < 201103L
    #error "should use c++11 implementation"
#endif

    char* nums = mallocArray(0);
    delete nums;

    // int num1;
    // double num2;
    // bitCopy(num1, num2);

}

YoungTest::YoungTest() {
	ALOGI("%s %p", __func__, this);

    test();
}

YoungTest::~YoungTest() {
	ALOGI("%s %p", __func__, this);
}

YoungTest* YoungTest::getInstance() {
    static YoungTest sInstance;
    return &sInstance;
}


}
