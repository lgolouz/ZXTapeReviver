//*******************************************************************************
// ZX Tape Reviver
//-----------------
//
// Author: Leonid Golouz
// E-mail: lgolouz@list.ru
// YouTube channel: https://www.youtube.com/channel/UCz_ktTqWVekT0P4zVW8Xgcg
// YouTube channel e-mail: computerenthusiasttips@mail.ru
//
// Code modification and distribution of any kind is not allowed without direct
// permission of the Author.
//*******************************************************************************

#include "zxtablemodel.h"
#include <QVariantMap>

ZxTableModel::ZxTableModel(QObject* parent) :
    QAbstractTableModel(parent)
{

}

int ZxTableModel::rowCount(const QModelIndex& index) const {
    Q_UNUSED(index)
    return 0;
}

int ZxTableModel::columnCount(const QModelIndex& index) const {
    Q_UNUSED(index)
    return 0;
}

QVariant ZxTableModel::data(const QModelIndex& index, int role) const {
    return {};
}

QHash<int, QByteArray> ZxTableModel::roleNames() const {
    return { {Qt::DisplayRole, "display"} };
}

void ZxTableModel::appendRow() {

}

QVariantList ZxTableModel::getVerticalHeader() const {
    return {};
}

QVariantList ZxTableModel::getHorizontalHeader() const {
    return {};
}
