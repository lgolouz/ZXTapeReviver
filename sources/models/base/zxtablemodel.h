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

#include <QDebug>

#include <QStringList>
#include <QAbstractTableModel>
#include <type_traits>
#include "sources/util/enummetainfo.h"

class ZxTableModel : public QAbstractTableModel, protected EnumMetaInfo
{
    Q_OBJECT

public:
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

protected:
    template<typename T>
    const QHash<int, QByteArray> generateRoleNames() const {
        const auto qMetaEnum { QMetaEnum::fromType<T>() };
        std::decay_t<decltype(generateRoleNames<T>())> res;
        const auto keyCount = qMetaEnum.keyCount();
        for (decltype(qMetaEnum.keyCount()) counter { 0 }; counter < keyCount; ++counter) {
            auto val = qMetaEnum.value(counter);
            res.emplace(val, enumNameToRoleName(qMetaEnum.key(counter)).toUtf8());
        }
        return res;
    }

private:
    QStringList m_horizontalHeader;
};

#endif // ZXTABLEMODEL_H
