#ifndef AUDIO_MAIN_H
#define AUDIO_MAIN_H

#include "audio_effect.h"
#include "audio_common.h"

class AudioPlayer;
class AudioRecorder;

class EngineOpenSLES
{
public:
    EngineOpenSLES(int sampleRate, int framesPerBuf);
    bool createSLBufferQueueAudioPlayer();
    void deleteSLBufferQueueAudioPlayer();

    bool createAudioRecorder();
    void deleteAudioRecorder();

    void startPlay();
    void stopPlay();

    bool EngineService(uint32_t msg, void *data);

private:
      SLmilliHertz fastPathSampleRate_;
      uint32_t fastPathFramesPerBuf_;
      uint16_t sampleChannels_;
      uint16_t bitsPerSample_;

      SLObjectItf slEngineObj_;
      SLEngineItf slEngineItf_;

      AudioRecorder *recorder_;
      AudioPlayer *player_;
      AudioQueue *freeBufQueue_;  // Owner of the queue
      AudioQueue *recBufQueue_;   // Owner of the queue

      sample_buf *bufs_;
      uint32_t bufCount_;
      uint32_t frameCount_;
      AudioDelay *delayEffect_;

};

#endif // AUDIO_MAIN_H
