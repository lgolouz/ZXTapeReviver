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
    ~FileWorkerModel() = default;

    //getters
    QString getWavFileName() const;

    //setters

    //QML invocable members
    Q_INVOKABLE FileWorkerResults openWavFile(const QString& fileName);

signals:
    void wavFileNameChanged();
};

#endif // FILEWORKERMODEL_H
