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

#include "suspiciouspointsmodel.h"
#include <QDebug>
#include <QtGlobal>

SuspiciousPointsModel::SuspiciousPointsModel(QObject* parent) : QAbstractTableModel(parent)
{

}

int SuspiciousPointsModel::rowCount(const QModelIndex& parent) const
{
    return parent.isValid() ? 0 : mSuspiciousPoints.size();
}

int SuspiciousPointsModel::columnCount(const QModelIndex& parent) const
{
    return parent.isValid() ? 0 : 2;
}

QVariant SuspiciousPointsModel::data(const QModelIndex& index, int role) const
{
    if (!index.isValid() || role != Qt::DisplayRole || index.row() >= mSuspiciousPoints.size()) {
        return {};
    }

    return index.column() == 0 ? index.row() + 1 : mSuspiciousPoints[index.row()];
}

QVariant SuspiciousPointsModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation != Qt::Horizontal || role != Qt::DisplayRole) {
        return {};
    }

    return section == 0 ? qtTrId("id_suspicious_point_number") : qtTrId("id_suspicious_point_position");
}

bool SuspiciousPointsModel::addSuspiciousPoint(uint idx)
{
    qDebug() << QString("Adding suspicious point: %1").arg(idx);
    const auto it { std::lower_bound(mSuspiciousPoints.begin(), mSuspiciousPoints.end(), idx, [](const QVariant& t1, uint t2) { return t1.toUInt() < t2; }) };
    int row = 0;
    if (it == mSuspiciousPoints.end()) {
        row = mSuspiciousPoints.size();
        beginInsertRows({}, row, row);
        mSuspiciousPoints.append(idx);
    } else if (idx == it->toUInt()) {
        return false;
    } else {
        row = std::distance(mSuspiciousPoints.begin(), it);
        beginInsertRows({}, row, row);
        mSuspiciousPoints.insert(it, idx);
    }

    endInsertRows();
    emit suspiciousPointsChanged();
    emit sizeChanged();
    return true;
}

bool SuspiciousPointsModel::removeSuspiciousPoint(int idx)
{
    qDebug() << QString("Removing suspicious point: %1").arg(idx);
    const auto r = idx >= 0 && mSuspiciousPoints.size() > idx;
    if (r) {
        beginRemoveRows({}, idx, idx);
        mSuspiciousPoints.removeAt(idx);
        endRemoveRows();
        emit suspiciousPointsChanged();
        emit sizeChanged();
    }
    return r;
}

uint SuspiciousPointsModel::getSuspiciousPoint(int idx)
{
    if (mSuspiciousPoints.size() > idx) {
        return mSuspiciousPoints[idx].toUInt();
    }
    return 0;
}

void SuspiciousPointsModel::clearSuspiciousPoints()
{
    beginResetModel();
    mSuspiciousPoints.clear();
    endResetModel();
    emit suspiciousPointsChanged();
    emit sizeChanged();
}

int SuspiciousPointsModel::getSize() const
{
    return mSuspiciousPoints.size();
}

QVariantList SuspiciousPointsModel::getSuspiciousPoints() const
{
    return mSuspiciousPoints;
}

void SuspiciousPointsModel::setSuspiciousPoints(const QVariantList& m)
{
    beginResetModel();
    mSuspiciousPoints = m;
    endResetModel();
    emit suspiciousPointsChanged();
    emit sizeChanged();
}

SuspiciousPointsModel* SuspiciousPointsModel::instance()
{
    static SuspiciousPointsModel m;
    return &m;
}
