//*******************************************************************************
// ZX Tape Reviver
//-----------------
//
// Author: Leonid Golouz
// E-mail: lgolouz@list.ru
//*******************************************************************************

#ifndef FILEWORKERMODEL_H
#define FILEWORKERMODEL_H

#include <QObject>
#include "sources/core/wavreader.h"

class FileWorkerModel : public QObject
{
    Q_OBJECT

    Q_PROPERTY(QString wavFileName READ getWavFileName NOTIFY wavFileNameChanged)

public:
    enum FileWorkerResults {
        FW_OK,
        FW_ERR
    };
    Q_ENUM(FileWorkerResults)

    explicit FileWorkerModel(QObject* parent = nullptr);
    ~FileWorkerModel();
    //getters
    QString getWavFileName() const;

    //setters

    //QML invokable members
    Q_INVOKABLE /*WavReader::ErrorCodesEnum*/ int openWavFileByUrl(const QString& fileNameUrl);
    Q_INVOKABLE /*WavReader::ErrorCodesEnum*/ int openWavFile(const QString& fileName);
    Q_INVOKABLE /*WavReader::ErrorCodesEnum*/ int openWaveformFileByUrl(const QString& fileNameUrl);
    Q_INVOKABLE /*WavReader::ErrorCodesEnum*/ int openWaveformFile(const QString& fileName);
    Q_INVOKABLE /*WavReader::ErrorCodesEnum*/ int saveWaveformFileByUrl(const QString& fileNameUrl);
    Q_INVOKABLE /*WavReader::ErrorCodesEnum*/ int saveWaveformFile(const QString& fileName);

signals:
    void wavFileNameChanged();

private:
    QString m_wavFileName;
};

#endif // FILEWORKERMODEL_H
