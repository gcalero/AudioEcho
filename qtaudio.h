#ifndef QTAUDIO_H
#define QTAUDIO_H

#include <QObject>
#include <QtMultimedia/QAudioInput>
#include <QtMultimedia/QAudioOutput>

class QtAudio : QObject
{
Q_OBJECT
public:
    QtAudio(int sampleRate, int framesPerBuf);

    void startPlay();
    void stopPlay();

public slots:
    void handleMicAudioInput();

private:
    QAudioDeviceInfo _inputDeviceInfo;
    QAudioFormat _inputFormat;
    QAudioInput* _audioInput;
    QIODevice* _inputDevice;

    QAudioDeviceInfo _outputDeviceInfo;
    QAudioFormat _outputFormat;
    QAudioOutput* _loopbackAudioOutput;
    QIODevice* _loopbackOutputDevice;

    int calculateNumberOfInputCallbackBytes(const QAudioFormat& format) const;
    bool sampleChannelConversion(const int16_t* sourceSamples, int16_t* destinationSamples, unsigned int numSourceSamples,
                                 const int sourceChannelCount, const int destinationChannelCount);

};

#endif // QTAUDIO_H
