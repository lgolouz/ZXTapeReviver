//*******************************************************************************
// ZX Tape Reviver
//-----------------
//
// Author: Leonid Golouz
// E-mail: lgolouz@list.ru
//*******************************************************************************

#include "fileworkermodel.h"

FileWorkerModel::FileWorkerModel(QObject* parent) : QObject(parent)
{

}

FileWorkerModel::FileWorkerResults FileWorkerModel::openWavFile(const QString& fileName)
{
    return FileWorkerResults::FW_ERR;
}

QString FileWorkerModel::getWavFileName() const
{
    return { };
}
