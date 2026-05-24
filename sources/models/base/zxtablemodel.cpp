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

ZxTableModel::ZxTableModel(const QStringList& horizontalHeader, const QVector<qsizetype>& columnsWidth, QObject* parent) :
    QAbstractTableModel(parent),
    m_horizontalHeader(horizontalHeader),
    m_columnsWidth(calculateColumnsWidth(columnsWidth))
{

}

ZxTableModel::ZxTableModel(const QStringList& horizontalHeader, QObject* parent) : ZxTableModel(horizontalHeader, { }, parent)
{

}

QVector<qsizetype> ZxTableModel::calculateColumnsWidth(const QVector<qsizetype>& other, qsizetype implicitWidth) const {
    const auto th { other.size() };
    auto idx { qMax(m_horizontalHeader.size(), th) };
    decltype(calculateColumnsWidth()) res(idx);
    while (idx-- > 0) {
        res[idx] = idx < th ? other[idx] : implicitWidth;
    }

    return res;
}

int ZxTableModel::columnCount(const QModelIndex& index) const {
    Q_UNUSED(index)
    return m_horizontalHeader.size();
}

QVariant ZxTableModel::headerData(int section, Qt::Orientation orientation, int role) const {
    qDebug() << "headerData";
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

Qt::ItemFlags ZxTableModel::flags(const QModelIndex &index) const {
    Q_UNUSED(index)
    return Qt::ItemIsSelectable | Qt::ItemIsEnabled;// | Qt::ItemIsEditable;
}

int ZxTableModel::columnWidthProvider(int column) const {
    if (column < 0 || column >= columnCount()) {
        return 0;
    }

    return m_columnsWidth[column];
}
