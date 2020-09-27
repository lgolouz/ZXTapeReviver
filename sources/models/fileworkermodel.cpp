//*******************************************************************************
// ZX Tape Reviver
//-----------------
//
// Author: Leonid Golouz
// E-mail: lgolouz@list.ru
//*******************************************************************************

#include "fileworkermodel.h"
#include "sources/core/waveformparser.h"
#include <QUrl>
#include <QDebug>

FileWorkerModel::FileWorkerModel(QObject* parent) :
    QObject(parent),
    m_wavFileName(QString())
{

}

/*WavReader::ErrorCodesEnum*/ int FileWorkerModel::openWavFileByUrl(const QString& fileNameUrl)
{
    QUrl u(fileNameUrl);
    return openWavFile(u.toLocalFile());
}

/*WavReader::ErrorCodesEnum*/ int FileWorkerModel::openWavFile(const QString& fileName)
{
    auto& r = *WavReader::instance();
    r.close();

    auto result = r.setFileName(fileName);
    result = r.open();
    if (result == WavReader::Ok) {
        result = r.read();
        if (result == WavReader::Ok) {
            m_wavFileName = fileName;
            emit wavFileNameChanged();
        }
    }

    return result;
}

/*WavReader::ErrorCodesEnum*/ int FileWorkerModel::openWaveformFileByUrl(const QString& fileNameUrl)
{
    QUrl u(fileNameUrl);
    return openWaveformFile(u.toLocalFile());
}

/*WavReader::ErrorCodesEnum*/ int FileWorkerModel::openWaveformFile(const QString& fileName)
{
    auto& r = *WavReader::instance();
    r.close();

    r.loadWaveform(fileName);
    m_wavFileName = fileName;
    emit wavFileNameChanged();
    return WavReader::Ok;
}

/*WavReader::ErrorCodesEnum*/ int FileWorkerModel::saveWaveformFileByUrl(const QString& fileNameUrl)
{
    QUrl u(fileNameUrl);
    return saveWaveformFile(u.toLocalFile());
}

/*WavReader::ErrorCodesEnum*/ int FileWorkerModel::saveWaveformFile(const QString& fileName)
{
    auto& r = *WavReader::instance();
    r.saveWaveform(fileName);
    return WavReader::Ok;
}

QString FileWorkerModel::getWavFileName() const
{
    return m_wavFileName;
}

FileWorkerModel::~FileWorkerModel()
{
    qDebug() << "~FileWorkerModel";
}
