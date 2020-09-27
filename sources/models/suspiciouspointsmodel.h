//*******************************************************************************
// ZX Tape Reviver
//-----------------
//
// Author: Leonid Golouz
// E-mail: lgolouz@list.ru
//*******************************************************************************

#ifndef SUSPICIOUSPOINTSMODEL_H
#define SUSPICIOUSPOINTSMODEL_H

#include <QVariantList>

class SuspiciousPointsModel : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QVariantList suspiciousPoints READ getSuspiciousPoints NOTIFY suspiciousPointsChanged)
    Q_PROPERTY(int size READ getSize NOTIFY sizeChanged)

    QVariantList mSuspiciousPoints;

protected:
    explicit SuspiciousPointsModel(QObject* parent = nullptr);

public:
    virtual ~SuspiciousPointsModel() = default;
    static SuspiciousPointsModel* instance();

    int getSize() const;
    QVariantList getSuspiciousPoints() const;
    void setSuspiciousPoints(const QVariantList& m);

    Q_INVOKABLE bool addSuspiciousPoint(uint idx);
    Q_INVOKABLE bool removeSuspiciousPoint(int idx);
    Q_INVOKABLE uint getSuspiciousPoint(int idx);

signals:
    void suspiciousPointsChanged();
    void sizeChanged();
};

#endif // SUSPICIOUSPOINTSMODEL_H
