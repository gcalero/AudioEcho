#include "qtaudio.h"

typedef int16_t AudioSample;

#define VOICE_RECOGNITION "voicerecognition"

#define SAMPLE_SIZE sizeof(AudioSample)
#define SAMPLE_RATE 24000 // network sample rate in HiFi
#define CALLBACK_ACCELERATOR_RATIO 0.5
#define NETWORK_FRAME_BYTES_STEREO  960
#define NETWORK_FRAME_BYTES_PER_CHANNEL NETWORK_FRAME_BYTES_STEREO / 2
#define NETWORK_FRAME_SAMPLES_PER_CHANNEL NETWORK_FRAME_BYTES_PER_CHANNEL / SAMPLE_SIZE
#define OUTPUT_CHANNEL_COUNT 2

#include <mutex>
#include <QDebug>

using Mutex = std::mutex;
using Lock = std::unique_lock<Mutex>;

Mutex _deviceMutex;

QtAudio::QtAudio(int sampleRate, int framesPerBuf) :
    _audioInput (nullptr),
    _inputDevice(nullptr),
    _loopbackOutputDevice(nullptr)
{

    auto inputDevices = QAudioDeviceInfo::availableDevices(QAudio::AudioInput);
    for (auto inputDevice : inputDevices) {
        if (inputDevice.deviceName() == VOICE_RECOGNITION) {
            _inputDeviceInfo = inputDevice;
        }
    }
    assert(_inputDeviceInfo.deviceName() == VOICE_RECOGNITION);

    _inputFormat = _inputDeviceInfo.preferredFormat();
    _inputFormat.setCodec("audio/pcm");
    _inputFormat.setSampleSize(16);
    _inputFormat.setSampleType(QAudioFormat::SignedInt);
    _inputFormat.setByteOrder(QAudioFormat::LittleEndian);

    assert(_inputFormat.channelCount() == 1);


    _outputDeviceInfo = QAudioDeviceInfo::defaultOutputDevice();
    _outputFormat = _outputDeviceInfo.preferredFormat();
    _outputFormat.setCodec("audio/pcm");
    _outputFormat.setSampleSize(16);
    _outputFormat.setSampleType(QAudioFormat::SignedInt);
    _outputFormat.setByteOrder(QAudioFormat::LittleEndian);

    assert(_outputFormat.channelCount() == 2);
    assert(_outputDeviceInfo.isFormatSupported(_outputFormat));
    assert(_outputFormat.sampleRate() * NETWORK_FRAME_SAMPLES_PER_CHANNEL % SAMPLE_RATE == 0);


}


void QtAudio::startPlay() {

    // TODO: I'm changing this to the default thread. Try using a new thread
    _loopbackAudioOutput = new QAudioOutput(_outputDeviceInfo, _outputFormat /*, this*/);
    qDebug() << "[AUDIO-QT] the format to be used for output is " << _outputFormat;

    qDebug() << "[AUDIO-QT] the format to be used for output is " << _inputFormat;

    // TODO: I'm changing this to the default thread. Try using a new thread
    _audioInput = new QAudioInput(_inputDeviceInfo, _inputFormat/*, this*/);
    int numInputCallbackBytes = calculateNumberOfInputCallbackBytes(_inputFormat);
    _audioInput->setBufferSize(numInputCallbackBytes);
    assert (_audioInput->bufferSize() == numInputCallbackBytes);

    _inputDevice = _audioInput->start();

    if (_inputDevice) {
        connect(_inputDevice, SIGNAL(readyRead()), this, SLOT(handleMicAudioInput()));
    } else {
        qDebug() << "Error starting audio input -" <<  _audioInput->error();
        _audioInput->deleteLater();
        _audioInput = NULL;
    }




}



static void channelUpmix(int16_t* source, int16_t* dest, int numSamples, int numExtraChannels) {
    for (int i = 0; i < numSamples/2; i++) {

        // read 2 samples
        int16_t left = *source++;
        int16_t right = *source++;

        // write 2 + N samples
        *dest++ = left;
        *dest++ = right;
        for (int n = 0; n < numExtraChannels; n++) {
            *dest++ = 0;
        }
    }
}

static void channelDownmix(int16_t* source, int16_t* dest, int numSamples) {
    for (int i = 0; i < numSamples/2; i++) {

        // read 2 samples
        int16_t left = *source++;
        int16_t right = *source++;

        // write 1 sample
        *dest++ = (int16_t)((left + right) / 2);
    }
}

void QtAudio::handleMicAudioInput() {
    if (!_inputDevice) {
        qDebug() << "[MIC-INPUT] NO READ";
        return;
    }

    QByteArray inputByteArray = _inputDevice->readAll();

    if (_inputFormat.sampleRate() != _outputFormat.sampleRate()) {
        return;
    }

    if (!_loopbackOutputDevice && _loopbackAudioOutput) {
        // we didn't have the loopback output device going so set that up now

        // NOTE: device start() uses the Qt internal device list
        Lock lock(_deviceMutex);
        _loopbackOutputDevice = _loopbackAudioOutput->start();
        lock.unlock();

        if (!_loopbackOutputDevice) {
            return;
        }
    }

    static QByteArray loopBackByteArray;

    int numInputSamples = inputByteArray.size() / SAMPLE_SIZE;
    int numLoopbackSamples = (numInputSamples * OUTPUT_CHANNEL_COUNT) / _inputFormat.channelCount();

    loopBackByteArray.resize(numLoopbackSamples * SAMPLE_SIZE);

    int16_t* inputSamples = reinterpret_cast<int16_t*>(inputByteArray.data());
    int16_t* loopbackSamples = reinterpret_cast<int16_t*>(loopBackByteArray.data());

    // upmix mono to stereo
    if (!sampleChannelConversion(inputSamples, loopbackSamples, numInputSamples, _inputFormat.channelCount(), OUTPUT_CHANNEL_COUNT)) {
        // no conversion, just copy the samples
        memcpy(loopbackSamples, inputSamples, numInputSamples * SAMPLE_SIZE);
    }

    // apply stereo reverb at the source, to the loopback audio
//    if (!_shouldEchoLocally && hasReverb) {
//        updateReverbOptions();
//        _sourceReverb.render(loopbackSamples, loopbackSamples, numLoopbackSamples/2);
//    }

    // if required, upmix or downmix to deviceChannelCount
    int deviceChannelCount = _outputFormat.channelCount();
    if (deviceChannelCount == OUTPUT_CHANNEL_COUNT) {
        qDebug() << "loopbackOutputDevice.write";
        if (_loopbackOutputDevice != nullptr) {
            _loopbackOutputDevice->write(loopBackByteArray);

        }

    } else {
        qDebug() << "loopbackOutputDevice else";

        static QByteArray deviceByteArray;

        int numDeviceSamples = (numLoopbackSamples * deviceChannelCount) / OUTPUT_CHANNEL_COUNT;

        deviceByteArray.resize(numDeviceSamples * SAMPLE_SIZE);

        int16_t* deviceSamples = reinterpret_cast<int16_t*>(deviceByteArray.data());

        if (deviceChannelCount > OUTPUT_CHANNEL_COUNT) {
            channelUpmix(loopbackSamples, deviceSamples, numLoopbackSamples, deviceChannelCount - OUTPUT_CHANNEL_COUNT);
        } else {
            channelDownmix(loopbackSamples, deviceSamples, numLoopbackSamples);
        }
        _loopbackOutputDevice->write(deviceByteArray);
    }
}

void QtAudio::stopPlay() {
    _audioInput->stop();
    _inputDevice = NULL;
    _audioInput->deleteLater();
    _audioInput = NULL;

    _loopbackOutputDevice = NULL;
    _loopbackAudioOutput->deleteLater();
    _loopbackAudioOutput = NULL;
}

int QtAudio::calculateNumberOfInputCallbackBytes(const QAudioFormat& format) const {
    int numInputCallbackBytes = (int)(((NETWORK_FRAME_BYTES_PER_CHANNEL
        * format.channelCount()
        * ((float) format.sampleRate() / SAMPLE_RATE))
        / CALLBACK_ACCELERATOR_RATIO) + 0.5f);

    return numInputCallbackBytes;
}

bool QtAudio::sampleChannelConversion(const int16_t* sourceSamples, int16_t* destinationSamples, unsigned int numSourceSamples,
                             const int sourceChannelCount, const int destinationChannelCount) {
    if (sourceChannelCount == 2 && destinationChannelCount == 1) {
        // loop through the stereo input audio samples and average every two samples
        for (uint i = 0; i < numSourceSamples; i += 2) {
            destinationSamples[i / 2] = (sourceSamples[i] / 2) + (sourceSamples[i + 1] / 2);
        }

        return true;
    } else if (sourceChannelCount == 1 && destinationChannelCount == 2) {

        // loop through the mono input audio and repeat each sample twice
        for (uint i = 0; i < numSourceSamples; ++i) {
            destinationSamples[i * 2] = destinationSamples[(i * 2) + 1] = sourceSamples[i];
        }

        return true;
    }

    return false;
}
