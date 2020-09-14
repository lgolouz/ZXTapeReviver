//*******************************************************************************
// ZX Tape Reviver
//-----------------
//
// Author: Leonid Golouz
// E-mail: lgolouz@list.ru
//*******************************************************************************

#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include "sources/controls/waveformcontrol.h"
#include "sources/core/waveformparser.h"
#include "sources/models/fileworkermodel.h"

void registerTypes()
{
    qmlRegisterUncreatableType<FileWorkerModel>("com.enums.zxtapereviver", 1, 0, "FileWorkerResults", QString());
    qmlRegisterUncreatableType<WavReader>("com.enums.zxtapereviver", 1, 0, "ErrorCodesEnum", QString());

    qmlRegisterType<WaveformControl>("WaveformControl", 1, 0, "WaveformControl");
    qmlRegisterSingletonType<FileWorkerModel>("com.models.zxtapereviver", 1, 0, "FileWorkerModel", [](QQmlEngine* engine, QJSEngine* scriptEngine) -> QObject* {
        Q_UNUSED(engine)
        Q_UNUSED(scriptEngine)

        return new FileWorkerModel();
    });
}

int main(int argc, char *argv[])
{
    Q_UNUSED(argc)
    Q_UNUSED(argv)

    QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling);

    QGuiApplication app(argc, argv);

    registerTypes();

    //auto& r = *WavReader::instance();
    //r.setFileName("D:/123/Кассеты/ZZZ-1/тест2-2.wav");
    //r.setFileName("D:/123/Кассеты/ZZZ-1/04 - SABOTEUR 1.wav");
    //r.setFileName("D:/123/Кассеты/ZZZ-1/track-2.wav");
    //r.setFileName("D:/Tapes/Agfa Fe-I 90/Side1-track1-spd.wav");
    //r.setFileName("D:/Tapes/ZZZ-90/track-1-side-B-other.wav");

    //r.open();
    //r.read();

    QQmlApplicationEngine engine;
    const QUrl url(QStringLiteral("qrc:/main.qml"));
    QObject::connect(&engine, &QQmlApplicationEngine::objectCreated, &app, [url](QObject *obj, const QUrl &objUrl) {
        if (!obj && url == objUrl) {
            QCoreApplication::exit(-1);
        }
    }, Qt::QueuedConnection);
    engine.load(url);

    //auto& p = *WaveformParser::instance();
    //p.parse(0);

    return app.exec();
}
