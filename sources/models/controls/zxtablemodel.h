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

#ifndef ZXTABLEMODEL_H
#define ZXTABLEMODEL_H

#include <QtQml>
#include <QBitArray>
#include <QAbstractTableModel>

class ZxTableModel final : public QAbstractTableModel
{
    Q_OBJECT
    Q_PROPERTY(QVariantList verticalHeader READ getVerticalHeader NOTIFY verticalHeaderChanged)
    Q_PROPERTY(QVariantList horizontalHeader READ getHorizontalHeader NOTIFY horizontalHeaderChanged)

public:
    virtual ~ZxTableModel() = default;

    explicit ZxTableModel(QObject* parent = nullptr);

    ZxTableModel(const ZxTableModel& other) = delete;
    ZxTableModel(ZxTableModel&& other) = delete;
    ZxTableModel& operator= (const ZxTableModel& other) = delete;
    ZxTableModel& operator= (ZxTableModel&& other) = delete;

    int rowCount(const QModelIndex& index = QModelIndex()) const override;
    int columnCount(const QModelIndex& index = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

    QVariantList getVerticalHeader() const;
    QVariantList getHorizontalHeader() const;

    Q_INVOKABLE void appendRow();

signals:
    void verticalHeaderChanged();
    void horizontalHeaderChanged();

private:
    QBitArray m_verticalHeader;
    QVariantList m_horizontalHeader;
};

#endif // ZXTABLEMODEL_H
