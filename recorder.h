#ifndef RECORDER_H
#define RECORDER_H

#include <QObject>
#include <QAudioFormat>
#include <QAudioInput>
#include <QIODevice>
#include <QFile>

class Recorder : public QObject
{
public:
    explicit Recorder(QObject *parent = 0);
    ~Recorder(void);
    int init(void);

public slots:
    void processAudio(void);

private:
    qreal _volume;
    QAudioFormat *_ptrAudioFormat;
    QAudioInput *_ptrAudioInput;
    //QIODevice *_ptrInputDevice;
    int _bufferSize;

    //QDataStream *_debugAudioStream;
    QFile _destinationFile;

};

#endif // RECORDER_H
