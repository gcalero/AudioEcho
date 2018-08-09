#include <QApplication>
#include <QQmlApplicationEngine>
#include "recorder.h"
#include "engine_open_sles.h"
#include <QDebug>
#include "mainwindow.h"

int main(int argc, char *argv[])
{
    QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling);

    QApplication app(argc, argv);

    MainWindow mainWindow;
    mainWindow.show();


    //Recorder obj;
    //int initValue = obj.init();
    //if (initValue != 0)
    //{
    //    return initValue;
    //}

    return app.exec();
}
