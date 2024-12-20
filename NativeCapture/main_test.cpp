#include <binder/IPCThreadState.h>
#include <binder/ProcessState.h>
#include <unistd.h>
#include <utils/RefBase.h>

#include "AudioUtil.h"
#include "CameraCapture.h"
#include "YoungTest.h"
#include "YoungTestPlus.h"


using namespace android;

int main() {
    sp<ProcessState> proc(ProcessState::self());
    ProcessState::self()->startThreadPool();
    /*
        YoungTest::getInstance();
        YoungTestPlus::getInstance();
        */
    CameraCapture::getInstance()->startCapture(1920, 1080);

    do {
        char input = getchar();
        printf("%c\n", input);
        if (input == 'q') {
            CameraCapture::getInstance()->stopCapture();
            break;
        }
        usleep(100000);
    } while (1);

    return 0;
}

/*
int main() {
        sp<ProcessState> proc(ProcessState::self());
        ProcessState::self()->startThreadPool();
        AudioUtil::getInstance()->startRecording();
        do {
                char input = getchar();
                printf("%c\n", input);
                if (input == 'q') {
                        break;
                }
                usleep(100000);
        } while (1);

        AudioUtil::getInstance()->startPlaying();
        do {
                char input = getchar();
                printf("%c\n", input);
                if (input == 'q') {
                        break;
                }
                usleep(100000);
        } while (1);

        return 0;
}
*/
