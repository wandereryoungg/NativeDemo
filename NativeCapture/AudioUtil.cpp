//
// Created by Charlie on 2023/11/22.
//

#define LOG_TAG "AudioUtil"

#include <android/log.h>
#include <cassert>
#include "AudioUtil.h"

#define ALOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define ALOGW(...) __android_log_print(ANDROID_LOG_WARN, LOG_TAG, __VA_ARGS__)
#define ALOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

static std::mutex audioLock;

#define RECORDER_FRAMES (32000 * 10 * sizeof(short))
static short recorderBuffer[RECORDER_FRAMES];
static unsigned recorderSize;

namespace android {

AudioUtil::AudioUtil() {
	ALOGI("%s %p", __func__, this);
	createEngine();
	// createBufferQueueAudioPlayer();
	createAudioRecorder();
}

AudioUtil::~AudioUtil() {
	ALOGI("%s %p", __func__, this);
	shutdown();
}

AudioUtil* AudioUtil::getInstance() {
	static AudioUtil audioUtil;
	return &audioUtil;
}

void AudioUtil::createEngine() {
	SLresult result;

	// create engine
	result = slCreateEngine(&engineObject, 0, NULL, 0, NULL, NULL);
	assert(SL_RESULT_SUCCESS == result);
	(void)result;

	// realize the engine
	result = (*engineObject)->Realize(engineObject, SL_BOOLEAN_FALSE);
	assert(SL_RESULT_SUCCESS == result);
	(void)result;

	// get the engine interface, which is needed in order to create other objects
	result = (*engineObject)->GetInterface(engineObject, SL_IID_ENGINE, &engineEngine);
	assert(SL_RESULT_SUCCESS == result);
	(void)result;

	// create output mix, with environmental reverb specified as a non-required interface
	const SLInterfaceID ids[1] = { SL_IID_ENVIRONMENTALREVERB };
	const SLboolean req[1] = { SL_BOOLEAN_FALSE };
	result = (*engineEngine)->CreateOutputMix(engineEngine, &outputMixObject, 1, ids, req);
	assert(SL_RESULT_SUCCESS == result);
	(void)result;

	// realize the output mix
	result = (*outputMixObject)->Realize(outputMixObject, SL_BOOLEAN_FALSE);
	assert(SL_RESULT_SUCCESS == result);
	(void)result;

	// get the environmental reverb interface
	// this could fail if the environmental reverb effect is not available,
	// either because the feature is not present, excessive CPU load, or
	// the required MODIFY_AUDIO_SETTINGS permission was not requested and granted
	result = (*outputMixObject)->GetInterface(outputMixObject, SL_IID_ENVIRONMENTALREVERB,
			&outputMixEnvironmentalReverb);
	if (SL_RESULT_SUCCESS == result)
	{
		result = (*outputMixEnvironmentalReverb)->SetEnvironmentalReverbProperties(
				outputMixEnvironmentalReverb, &reverbSettings);
		(void)result;
	}
	// ignore unsuccessful result codes for environmental reverb, as it is optional for this example

}
/*
static const char android[] =
#include "android_clip.h"
;
*/

// this callback handler is called every time a buffer finishes playing
void AudioUtil::bqPlayerCallback(SLAndroidSimpleBufferQueueItf bq, void* context)
{
    std::unique_lock<std::mutex> lock(audioLock);
    ALOGI("%s:%d", __FUNCTION__, __LINE__);
	assert(bq == bqPlayerBufferQueue);
	assert(NULL == context);
    (void)context;
    /*
    SLresult result = (*bq)->Enqueue(bq, android, sizeof(android));
    // the most likely other result is SL_RESULT_BUFFER_INSUFFICIENT,
    // which for this code example would indicate a programming error
    if (SL_RESULT_SUCCESS != result) {
        ALOGE("%s   error", __func__);
    }
    */

    unsigned i;
    for (i = 0; i < recorderSize; i += 2 * sizeof(short)) {
        recorderBuffer[i >> 2] = recorderBuffer[i >> 1];
    }
    recorderSize >>= 1;
    short* nextBuffer = recorderBuffer;
    unsigned nextSize = recorderSize;
    SLresult result = (*bq)->Enqueue(bq, nextBuffer, nextSize);
    // the most likely other result is SL_RESULT_BUFFER_INSUFFICIENT,
    // which for this code example would indicate a programming error
    if (SL_RESULT_SUCCESS != result) {
        ALOGE("%s:%d   error", __FUNCTION__, __LINE__);
    }

}

void AudioUtil::createBufferQueueAudioPlayer()
{
	SLresult result;

	// configure audio source
	SLDataLocator_AndroidSimpleBufferQueue loc_bufq = { SL_DATALOCATOR_ANDROIDSIMPLEBUFFERQUEUE,
														2 };
	SLDataFormat_PCM format_pcm = { SL_DATAFORMAT_PCM, 1, SL_SAMPLINGRATE_32,
									SL_PCMSAMPLEFORMAT_FIXED_16, SL_PCMSAMPLEFORMAT_FIXED_16,
									SL_SPEAKER_FRONT_CENTER, SL_BYTEORDER_LITTLEENDIAN };
	/*
	 * Enable Fast Audio when possible:  once we set the same rate to be the native, fast audio path
	 * will be triggered
	 */
	if (bqPlayerSampleRate)
	{
		format_pcm.samplesPerSec = bqPlayerSampleRate;       //sample rate in mili second
	}
	SLDataSource audioSrc = { &loc_bufq, &format_pcm };

	// configure audio sink
	SLDataLocator_OutputMix loc_outmix = { SL_DATALOCATOR_OUTPUTMIX, outputMixObject };
	SLDataSink audioSnk = { &loc_outmix, NULL };

	/*
	 * create audio player:
	 *     fast audio does not support when SL_IID_EFFECTSEND is required, skip it
	 *     for fast audio case
	 */
	const SLInterfaceID ids[3] = { SL_IID_BUFFERQUEUE, SL_IID_VOLUME, SL_IID_EFFECTSEND,
			/*SL_IID_MUTESOLO,*/};
	const SLboolean req[3] = { SL_BOOLEAN_TRUE, SL_BOOLEAN_TRUE, SL_BOOLEAN_TRUE,
			/*SL_BOOLEAN_TRUE,*/ };

	result = (*engineEngine)->CreateAudioPlayer(engineEngine, &bqPlayerObject, &audioSrc, &audioSnk,
			bqPlayerSampleRate ? 2 : 3, ids, req);
	assert(SL_RESULT_SUCCESS == result);
	(void)result;
	// realize the player
	result = (*bqPlayerObject)->Realize(bqPlayerObject, SL_BOOLEAN_FALSE);
	assert(SL_RESULT_SUCCESS == result);
	(void)result;

	// get the play interface
	result = (*bqPlayerObject)->GetInterface(bqPlayerObject, SL_IID_PLAY, &bqPlayerPlay);
	assert(SL_RESULT_SUCCESS == result);
	(void)result;

	// get the buffer queue interface
	result = (*bqPlayerObject)->GetInterface(bqPlayerObject, SL_IID_BUFFERQUEUE,
			&bqPlayerBufferQueue);
	assert(SL_RESULT_SUCCESS == result);
	(void)result;

	// register callback on the buffer queue
	result = (*bqPlayerBufferQueue)->RegisterCallback(bqPlayerBufferQueue, bqPlayerCallback, NULL);
	assert(SL_RESULT_SUCCESS == result);
	(void)result;

	// get the effect send interface
	bqPlayerEffectSend = NULL;
	if (0 == bqPlayerSampleRate)
	{
		result = (*bqPlayerObject)->GetInterface(bqPlayerObject, SL_IID_EFFECTSEND,
				&bqPlayerEffectSend);
		assert(SL_RESULT_SUCCESS == result);
		(void)result;
	}

	// get the volume interface
	result = (*bqPlayerObject)->GetInterface(bqPlayerObject, SL_IID_VOLUME, &bqPlayerVolume);
	assert(SL_RESULT_SUCCESS == result);
	(void)result;

	// set the player's state to playing
	result = (*bqPlayerPlay)->SetPlayState(bqPlayerPlay, SL_PLAYSTATE_PLAYING);
	assert(SL_RESULT_SUCCESS == result);
	(void)result;
	ALOGI("%s:%d", __FUNCTION__, __LINE__);
}

// create audio recorder: recorder is not in fast path
// like to avoid excessive re-sampling while playing back from Hello & Android clip
void AudioUtil::createAudioRecorder()
{
    SLresult result;

    // configure audio source
    SLDataLocator_IODevice loc_dev = {SL_DATALOCATOR_IODEVICE, SL_IODEVICE_AUDIOINPUT,
            SL_DEFAULTDEVICEID_AUDIOINPUT, NULL};
    SLDataSource audioSrc = {&loc_dev, NULL};

    // configure audio sink
    SLDataLocator_AndroidSimpleBufferQueue loc_bq = {SL_DATALOCATOR_ANDROIDSIMPLEBUFFERQUEUE, 2};
    SLDataFormat_PCM format_pcm = {SL_DATAFORMAT_PCM, 2, SL_SAMPLINGRATE_32,
        SL_PCMSAMPLEFORMAT_FIXED_16, SL_PCMSAMPLEFORMAT_FIXED_16,
        SL_SPEAKER_FRONT_LEFT | SL_SPEAKER_FRONT_RIGHT, SL_BYTEORDER_LITTLEENDIAN};
    SLDataSink audioSnk = {&loc_bq, &format_pcm};

    // create audio recorder
    // (requires the RECORD_AUDIO permission)
    const SLInterfaceID id[1] = {SL_IID_ANDROIDSIMPLEBUFFERQUEUE};
    const SLboolean req[1] = {SL_BOOLEAN_TRUE};
    result = (*engineEngine)->CreateAudioRecorder(engineEngine, &recorderObject, &audioSrc,
            &audioSnk, 1, id, req);
    assert(SL_RESULT_SUCCESS == result);

    // realize the audio recorder
    result = (*recorderObject)->Realize(recorderObject, SL_BOOLEAN_FALSE);
    assert(SL_RESULT_SUCCESS == result);

    // get the record interface
    result = (*recorderObject)->GetInterface(recorderObject, SL_IID_RECORD, &recorderRecord);
    assert(SL_RESULT_SUCCESS == result);
    (void)result;

    // get the buffer queue interface
    result = (*recorderObject)->GetInterface(recorderObject, SL_IID_ANDROIDSIMPLEBUFFERQUEUE,
            &recorderBufferQueue);
    assert(SL_RESULT_SUCCESS == result);
    (void)result;

    // register callback on the buffer queue
    result = (*recorderBufferQueue)->RegisterCallback(recorderBufferQueue, bqRecorderCallback,
            (void*)recorderRecord);
    assert(SL_RESULT_SUCCESS == result);
    (void)result;
    ALOGI("%s:%d", __FUNCTION__, __LINE__);
}

// this callback handler is called every time a buffer finishes recording
void AudioUtil::bqRecorderCallback(SLAndroidSimpleBufferQueueItf bq, void *context)
{
    std::unique_lock<std::mutex> lock(audioLock);
    assert(bq == recorderBufferQueue);
    (void)bq;
    (void)context;

    SLRecordItf recorder = (SLRecordItf)context;

    SLresult result;
    result = (*recorder)->SetRecordState(recorder, SL_RECORDSTATE_STOPPED);
    if (SL_RESULT_SUCCESS == result) {
        recorderSize = RECORDER_FRAMES;
        FILE* file;
        const char* fileName = "/data/camera/audio_dump";
        file = fopen(fileName, "wb+");
        if (file) {
            ALOGI("%s:%d:   write fileName: %s", __FUNCTION__, __LINE__, fileName);
            fwrite(recorderBuffer, 1, recorderSize, file);
            fclose(file);
        } else {
            ALOGI("%s:%d:   fopen fileName: %s error: %s", __FUNCTION__, __LINE__, fileName, strerror(errno));
        }
    }
}

// set the recording state for the audio recorder
void AudioUtil::startRecording()
{
    SLresult result;

    // in case already recording, stop recording and clear buffer queue
    result = (*recorderRecord)->SetRecordState(recorderRecord, SL_RECORDSTATE_STOPPED);
    assert(SL_RESULT_SUCCESS == result);
    (void)result;
    result = (*recorderBufferQueue)->Clear(recorderBufferQueue);
    assert(SL_RESULT_SUCCESS == result);
    (void)result;

    // the buffer is not valid for playback yet
    recorderSize = 0;
    // enqueue an empty buffer to be filled by the recorder
    // (for streaming recording, we would enqueue at least 2 empty buffers to start things off)
    result = (*recorderBufferQueue)->Enqueue(recorderBufferQueue, recorderBuffer,
            RECORDER_FRAMES * sizeof(short));
    // the most likely other result is SL_RESULT_BUFFER_INSUFFICIENT,
    // which for this code example would indicate a programming error
    assert(SL_RESULT_SUCCESS == result);
    (void)result;

    // start recording
    result = (*recorderRecord)->SetRecordState(recorderRecord, SL_RECORDSTATE_RECORDING);
    assert(SL_RESULT_SUCCESS == result);
    (void)result;
    ALOGI("%s:%d", __FUNCTION__, __LINE__);
}

void AudioUtil::startPlaying() {
    ALOGI("%s:%d", __FUNCTION__, __LINE__);
    bqPlayerCallback(bqPlayerBufferQueue, nullptr);
    ALOGI("%s:%d", __FUNCTION__, __LINE__);
}

// shut down the native audio system
void AudioUtil::shutdown()
{

    // destroy buffer queue audio player object, and invalidate all associated interfaces
    if (bqPlayerObject != NULL) {
        (*bqPlayerObject)->Destroy(bqPlayerObject);
        bqPlayerObject = NULL;
        bqPlayerPlay = NULL;
        bqPlayerBufferQueue = NULL;
        bqPlayerEffectSend = NULL;
        bqPlayerVolume = NULL;
    }

    // destroy audio recorder object, and invalidate all associated interfaces
    if (recorderObject != NULL) {
        (*recorderObject)->Destroy(recorderObject);
        recorderObject = NULL;
        recorderRecord = NULL;
        recorderBufferQueue = NULL;
    }

    // destroy output mix object, and invalidate all associated interfaces
    if (outputMixObject != NULL) {
        (*outputMixObject)->Destroy(outputMixObject);
        outputMixObject = NULL;
        outputMixEnvironmentalReverb = NULL;
    }

    // destroy engine object, and invalidate all associated interfaces
    if (engineObject != NULL) {
        (*engineObject)->Destroy(engineObject);
        engineObject = NULL;
        engineEngine = NULL;
    }

}

}