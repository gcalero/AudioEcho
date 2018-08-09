#include <QtWidgets>

#include "mainwindow.h"
#include <QtAndroidExtras/QAndroidJniObject>
#include <QtAndroidExtras/QtAndroid>
#define STR_START_ECHO_SL "Start echo SL"
#define STR_STOP_ECHO_SL "Stop echo"
#define STR_START_ECHO_QT "Start echo Qt"
#define STR_STOP_ECHO_QT "Stop echo"

#define FAST_SAMPLE_RATE 48000
#define FAST_BUFFER_SIZE 192 // depends on the device
MainWindow::MainWindow() :
    root(new QWidget()),
    isPlaying(false)
{
    setCentralWidget(root);
    echoSLButton = new QPushButton(root);
    echoSLButton->setText(tr(STR_START_ECHO_SL));
    connect(echoSLButton, SIGNAL(clicked()), this, SLOT(startEchoSL()));

    echoQtButton = new QPushButton(root);
    echoQtButton->move(0, 200);
    echoQtButton->setText(tr(STR_START_ECHO_QT));
    connect(echoQtButton, SIGNAL(clicked()), this, SLOT(startEchoQt()));
}

void MainWindow::startEchoSL()
{
    qDebug() << "SL : isPlaying" << isPlaying;
    if (!isPlaying) {
        engineSL = new EngineOpenSLES(FAST_SAMPLE_RATE, FAST_BUFFER_SIZE);
        if(!engineSL->createSLBufferQueueAudioPlayer()) {
            qDebug() << "[DEMO-AUDIO] Error in createSLBufferQueueAudioPlayer";
            return;
        }
        qDebug() << "[DEMO-AUDIO] createSLBufferQueueAudioPlayer ok";
        if(!engineSL->createAudioRecorder()) {
            engineSL->deleteSLBufferQueueAudioPlayer();
            qDebug() << "Error in createAudioRecorder";
            return;
        }
        qDebug() << "[DEMO-AUDIO] createAudioRecorder ok";
        engineSL->startPlay();   // startPlay() triggers startRecording()
        //statusView.setText(getString(R.string.echoing_status_msg));
    } else {
        engineSL->stopPlay();  // stopPlay() triggers stopRecording()
        //updateNativeAudioUI();
        engineSL->deleteAudioRecorder();
        engineSL->deleteSLBufferQueueAudioPlayer();
    }
    isPlaying = !isPlaying;
    echoSLButton->setText(isPlaying ? tr(STR_STOP_ECHO_SL) : tr(STR_START_ECHO_SL));
    echoQtButton->setEnabled(!isPlaying);
}

void MainWindow::startEchoQt() {
    if (!isPlaying) {
        audioQt = new QtAudio(FAST_SAMPLE_RATE, FAST_BUFFER_SIZE);
        qDebug() << "[AUDIO-QT] created";
        audioQt->startPlay();
        qDebug() << "[AUDIO-QT] started";

    } else {
        audioQt->stopPlay();
    }
    isPlaying = !isPlaying;
    echoQtButton->setText(isPlaying ? tr(STR_STOP_ECHO_QT) : tr(STR_START_ECHO_QT));
    echoSLButton->setEnabled(!isPlaying);
}
