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

#include <QStringList>
#include <QAbstractTableModel>

class ZxTableModel : public QAbstractTableModel
{
    Q_OBJECT

public:
    virtual ~ZxTableModel() = default;

    explicit ZxTableModel(const QStringList& horizontalHeader, QObject* parent = nullptr);

    ZxTableModel(const ZxTableModel& other) = delete;
    ZxTableModel(ZxTableModel&& other) = delete;
    ZxTableModel& operator= (const ZxTableModel& other) = delete;
    ZxTableModel& operator= (ZxTableModel&& other) = delete;

    //these methods should be implemented in sub-classes [https://doc.qt.io/qt-6/qabstracttablemodel.html]
    //int rowCount(const QModelIndex& index = QModelIndex()) const override;
    //QVariant data(const QModelIndex& index, int role) const override;

    //default roleNames method returns pre-defined names, so we have to extend them, if needed [https://doc.qt.io/qt-6/qabstractitemmodel.html#roleNames]
    //QHash<int, QByteArray> roleNames() const override;

    virtual int columnCount(const QModelIndex& index = QModelIndex()) const override;
    virtual QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;

private:
    QStringList m_horizontalHeader;
};

#endif // ZXTABLEMODEL_H
