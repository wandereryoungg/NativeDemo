//
// Created by Charlie on 2023/11/22.
//

#ifndef NATIVECAPTURE_AUDIOUTIL_H
#define NATIVECAPTURE_AUDIOUTIL_H

#include <SLES/OpenSLES.h>
#include <SLES/OpenSLES_Android.h>
#include <mutex>

namespace android {

class AudioUtil {
public:
	AudioUtil();

	~AudioUtil();

	static AudioUtil* getInstance();

	void createEngine();

	void createBufferQueueAudioPlayer();

	static void bqPlayerCallback(SLAndroidSimpleBufferQueueItf bq, void* context);

	void createAudioRecorder();

	static void bqRecorderCallback(SLAndroidSimpleBufferQueueItf bq, void* context);

	void startRecording();

	void startPlaying();

private:
	void shutdown();

	SLObjectItf engineObject;

	SLEngineItf engineEngine;

	SLObjectItf outputMixObject;

	SLEnvironmentalReverbItf outputMixEnvironmentalReverb;

	const SLEnvironmentalReverbSettings reverbSettings = SL_I3DL2_ENVIRONMENT_PRESET_STONECORRIDOR;

	// buffer queue player interfaces
	SLObjectItf bqPlayerObject;

	SLPlayItf bqPlayerPlay;

	SLAndroidSimpleBufferQueueItf bqPlayerBufferQueue;

	SLEffectSendItf bqPlayerEffectSend;

	SLVolumeItf bqPlayerVolume;

	SLmilliHertz bqPlayerSampleRate = SL_SAMPLINGRATE_32;

	// recorder interfaces
	SLObjectItf recorderObject;

	SLRecordItf recorderRecord;

	SLAndroidSimpleBufferQueueItf recorderBufferQueue;

};

}

#endif //NATIVECAPTURE_AUDIOUTIL_H
