#include "recorder.h"
#include <QDebug>
#include <QApplication>
#include <QDateTime>
#include <QDataStream>

Recorder::Recorder(QObject *parent) :
    QObject(parent),
    _volume(100.0),
    _ptrAudioFormat(0),
    _ptrAudioInput(0),
    //_ptrInputDevice(0),
    _bufferSize(32 * 1024)
    //_debugAudioStream(NULL)
{
    qDebug() << "[RECORDER-DEMO] constructor";
}

int Recorder::init()
{
    qDebug() << "[RECORDER-DEMO] init";
    _ptrAudioFormat = new QAudioFormat();
    _ptrAudioFormat->setChannelCount(1);
    _ptrAudioFormat->setSampleRate(24000);
    _ptrAudioFormat->setSampleSize(16);
    _ptrAudioFormat->setSampleType(QAudioFormat::SignedInt);
    _ptrAudioFormat->setByteOrder(QAudioFormat::LittleEndian);
    _ptrAudioFormat->setCodec("audio/pcm");

    _ptrAudioInput = new QAudioInput(*_ptrAudioFormat);
    _ptrAudioInput->setBufferSize(_bufferSize);
    _ptrAudioInput->setVolume(_volume);

    _destinationFile.setFileName(QString("/sdcard/audio-recording-%1.raw").arg(QDateTime::currentMSecsSinceEpoch()));
    _destinationFile.open(QIODevice::WriteOnly);
    _ptrAudioInput->start(&_destinationFile);
    //_ptrInputDevice = _ptrAudioInput->start();

    /*if (_ptrInputDevice == 0)
    {
        qDebug() << _ptrAudioInput->error();
        return -1;
    }
    else
    {
        _debugAudioStream = new QDataStream(&_destinationFile);

        connect(_ptrInputDevice, &QIODevice::readyRead, this, &Recorder::processAudio);
    }
*/
    return 0;
}

Recorder::~Recorder()
{
    //_ptrInputDevice = 0;

    if (_ptrAudioInput)
    {
        _ptrAudioInput->stop();
        _destinationFile.close();
        delete _ptrAudioInput; _ptrAudioInput = 0;
    }

    if (_ptrAudioFormat)
    {
        delete _ptrAudioFormat; _ptrAudioFormat = 0;
    }
}

void Recorder::processAudio()
{
    /*
        char *byteBuffer = new char[_bufferSize];
        short *sampleBuffer = new short[_bufferSize / sizeof(short)];

        _ptrAudioInput->setVolume(_volume);

        int bytesRead = static_cast<int>(_ptrInputDevice->read(byteBuffer, _bufferSize));
        memcpy(sampleBuffer, byteBuffer, bytesRead);
        int samplesRead = bytesRead / sizeof(short);
        int maxValue = 0; int minValue = 0;
        for (int s = 0; s < samplesRead; s ++)
        {
            int value = static_cast<int>(sampleBuffer[s]);
            if (minValue > value) { minValue = value; }
            if (maxValue < value) { maxValue = value; }
        }

        if (_debugAudioStream != NULL) {
            qDebug() << "[RECORDER-DEMO] writing " << bytesRead << " bytes";
            *_debugAudioStream << byteBuffer;
            _destinationFile.flush();
        }

        qDebug() << "volume set: " << _volume
                 << " volume returned: " << _ptrAudioInput->volume()
                 << " bytes read: " << bytesRead
                 << " min value: " << minValue
                 << " max value: " << maxValue;

        delete [] sampleBuffer; sampleBuffer = 0;
        delete [] byteBuffer; byteBuffer = 0;

        if (_volume > 0.9)
        {
            //QApplication::exit(0);
        }
        else
        {
            //_volume += 0.1;
        }
        */
}
