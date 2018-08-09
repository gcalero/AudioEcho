#include "engine_open_sles.h"

#include "audio_recorder.h"
#include "audio_player.h"
#include <QDebug>
EngineOpenSLES::EngineOpenSLES(int sampleRate, int framesPerBuf)
{
    SLresult result;

//JNIEXPORT void JNICALL Java_com_google_sample_echo_MainActivity_createSLEngine(
//    JNIEnv *env, jclass type, jint sampleRate, jint framesPerBuf,
//    jlong delayInMs, jfloat decay) {

  fastPathSampleRate_ = static_cast<SLmilliHertz>(sampleRate) * 1000;
  fastPathFramesPerBuf_ = static_cast<uint32_t>(framesPerBuf);
  sampleChannels_ = AUDIO_SAMPLE_CHANNELS;
  bitsPerSample_ = SL_PCMSAMPLEFORMAT_FIXED_16;

  result = slCreateEngine(&slEngineObj_, 0, NULL, 0, NULL, NULL);
  SLASSERT(result);

  result =
      (*slEngineObj_)->Realize(slEngineObj_, SL_BOOLEAN_FALSE);
  SLASSERT(result);

  result = (*slEngineObj_)
               ->GetInterface(slEngineObj_, SL_IID_ENGINE,
                              &slEngineItf_);
  SLASSERT(result);

  // compute the RECOMMENDED fast audio buffer size:
  //   the lower latency required
  //     *) the smaller the buffer should be (adjust it here) AND
  //     *) the less buffering should be before starting player AFTER
  //        receiving the recorder buffer
  //   Adjust the bufSize here to fit your bill [before it busts]
  uint32_t bufSize = fastPathFramesPerBuf_ * sampleChannels_ *
                     bitsPerSample_;
  bufSize = (bufSize + 7) >> 3;  // bits --> byte
  bufCount_ = BUF_COUNT;
  bufs_ = allocateSampleBufs(bufCount_, bufSize);
  assert(bufs_);

  freeBufQueue_ = new AudioQueue(bufCount_);
  recBufQueue_ = new AudioQueue(bufCount_);
  assert(freeBufQueue_ && recBufQueue_);
  for (uint32_t i = 0; i < bufCount_; i++) {
    freeBufQueue_->push(&bufs_[i]);
  }

  delayEffect_ = new AudioDelay(
      fastPathSampleRate_, sampleChannels_, bitsPerSample_,
      0, 0);
  assert(delayEffect_);

}

bool EngineOpenSLES::createSLBufferQueueAudioPlayer() {
  SampleFormat sampleFormat;
  memset(&sampleFormat, 0, sizeof(sampleFormat));
  sampleFormat.pcmFormat_ = (uint16_t)bitsPerSample_;
  sampleFormat.framesPerBuf_ = fastPathFramesPerBuf_;

  // SampleFormat.representation_ = SL_ANDROID_PCM_REPRESENTATION_SIGNED_INT;
  sampleFormat.channels_ = (uint16_t)sampleChannels_;
  sampleFormat.sampleRate_ = fastPathSampleRate_;

  player_ = new AudioPlayer(&sampleFormat, slEngineItf_);
  assert(player_);
  if (player_ == nullptr) return false;

  player_->SetBufQueue(recBufQueue_, freeBufQueue_);
  player_->RegisterEngine(this);

  return true;
}


void EngineOpenSLES::deleteSLBufferQueueAudioPlayer () {
  if (player_) {
    delete player_;
    player_ = nullptr;
  }
}

bool EngineOpenSLES::createAudioRecorder() {
  SampleFormat sampleFormat;
  memset(&sampleFormat, 0, sizeof(sampleFormat));
  sampleFormat.pcmFormat_ = static_cast<uint16_t>(bitsPerSample_);

  // SampleFormat.representation_ = SL_ANDROID_PCM_REPRESENTATION_SIGNED_INT;
  sampleFormat.channels_ = sampleChannels_;
  sampleFormat.sampleRate_ = fastPathSampleRate_;
  sampleFormat.framesPerBuf_ = fastPathFramesPerBuf_;
  recorder_ = new AudioRecorder(&sampleFormat, slEngineItf_);
  if (!recorder_) {
    return false;
  }
  recorder_->SetBufQueues(freeBufQueue_, recBufQueue_);
  recorder_->RegisterEngine(this);
  return true;
}


void EngineOpenSLES::deleteAudioRecorder() {
  if (recorder_) {
      delete recorder_;
  }
  recorder_ = nullptr;
}

void EngineOpenSLES::startPlay() {
  frameCount_ = 0;
  qDebug() << "[DEMO-AUDIO] startPlay()";
  /*
   * start player: make it into waitForData state
   */
  if (SL_BOOLEAN_FALSE == player_->Start()) {
    qDebug() << "[DEMO-AUDIO] ====" <<  __FUNCTION__ << " failed";
    return;
  }
  recorder_->Start();
}

void EngineOpenSLES::stopPlay() {
  recorder_->Stop();
  player_->Stop();

  delete recorder_;
  delete player_;
  recorder_ = NULL;
  player_ = NULL;
}


/*
 * simple message passing for player/recorder to communicate with engine
 */
bool EngineOpenSLES::EngineService(uint32_t msg, void *data) {
  switch (msg) {
    case ENGINE_SERVICE_MSG_RETRIEVE_DUMP_BUFS: {
      //*(static_cast<uint32_t *>(data)) = dbgEngineGetBufCount();
      break;
    }
    case ENGINE_SERVICE_MSG_RECORDED_AUDIO_AVAILABLE: {
      // adding audio delay effect
      sample_buf *buf = static_cast<sample_buf *>(data);
      assert(fastPathFramesPerBuf_ ==
             buf->size_ / sampleChannels_ / (bitsPerSample_ / 8));
      delayEffect_->process(reinterpret_cast<int16_t *>(buf->buf_),
                                   fastPathFramesPerBuf_);
      break;
    }
    default:
      assert(false);
      return false;
  }

  return true;
}
