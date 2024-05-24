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

ZxTableModel::ZxTableModel(const QStringList& horizontalHeader, QObject* parent) :
    QAbstractTableModel(parent),
    m_horizontalHeader(horizontalHeader)
{
    emit headerDataChanged(Qt::Horizontal, 0, horizontalHeader.size() - 1);
}

int ZxTableModel::columnCount(const QModelIndex& index) const {
    Q_UNUSED(index)
    return m_horizontalHeader.size();
}

QVariant ZxTableModel::headerData(int section, Qt::Orientation orientation, int role) const {
    return role == Qt::DisplayRole
               ? orientation == Qt::Horizontal
                     ? section < m_horizontalHeader.size()
                           ? m_horizontalHeader[section]
                           : QVariant()
                     : section < rowCount()
                           ? QString::number(section + 1) // as section begins from 0
                           : QVariant()
               : QVariant();
}
