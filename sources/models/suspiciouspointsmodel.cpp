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

SuspiciousPointsModel::SuspiciousPointsModel(QObject* parent) : QObject(parent)
{

}

bool SuspiciousPointsModel::addSuspiciousPoint(uint idx)
{
    qDebug() << QString("Adding suspicious point: %1").arg(idx);
    const auto it { std::lower_bound(mSuspiciousPoints.begin(), mSuspiciousPoints.end(), idx, [](const QVariant& t1, uint t2) { return t1.toUInt() < t2; }) };
    if (it == mSuspiciousPoints.end()) {
        mSuspiciousPoints.append(idx);
    } else if (idx == it->toUInt()) {
        return false;
    } else {
        mSuspiciousPoints.insert(it, idx);
    }

    emit suspiciousPointsChanged();
    emit sizeChanged();
    return true;
}

bool SuspiciousPointsModel::removeSuspiciousPoint(int idx)
{
    qDebug() << QString("Removing suspicious point: %1").arg(idx);
    const auto r = mSuspiciousPoints.size() > idx;
    if (r) {
        mSuspiciousPoints.removeAt(idx);
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
    mSuspiciousPoints.clear();
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
    mSuspiciousPoints = m;
    emit suspiciousPointsChanged();
    emit sizeChanged();
}

SuspiciousPointsModel* SuspiciousPointsModel::instance()
{
    static QScopedPointer<SuspiciousPointsModel> m { new SuspiciousPointsModel() };
    return m.get();
}
