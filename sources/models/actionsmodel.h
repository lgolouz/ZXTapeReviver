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

#ifndef ACTIONSMODEL_H
#define ACTIONSMODEL_H

#include <QAbstractTableModel>
#include <QVariantList>
#include <QList>
#include <QSharedPointer>
#include "sources/actions/actionbase.h"

class ActionsModel final : public QAbstractTableModel
{
    Q_OBJECT

    Q_PROPERTY(QVariantList actions READ getActions NOTIFY actionsChanged)
    Q_PROPERTY(bool canUndo READ getCanUndo NOTIFY actionsChanged)
    Q_PROPERTY(bool canRedo READ getCanRedo NOTIFY actionsChanged)

    QList<QSharedPointer<ActionBase>> m_actions;
    QList<QSharedPointer<ActionBase>> m_redoActions;

protected:
    explicit ActionsModel(QObject* parent = nullptr);

public:
    virtual ~ActionsModel() = default;

    ActionsModel(const ActionsModel& other) = delete;
    ActionsModel(ActionsModel&& other) = delete;
    ActionsModel& operator= (const ActionsModel& other) = delete;
    ActionsModel& operator= (ActionsModel&& other) = delete;

    static ActionsModel* instance();

    QVariantList getActions() const;
    bool getCanUndo() const;
    bool getCanRedo() const;
    virtual int rowCount(const QModelIndex& parent = {}) const override;
    virtual int columnCount(const QModelIndex& parent = {}) const override;
    virtual QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    virtual QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;
    Q_INVOKABLE int columnWidthProvider(int column) const;

    void addAction(QSharedPointer<ActionBase> action);
    Q_INVOKABLE void removeAction();
    Q_INVOKABLE void redoAction();
    Q_INVOKABLE void shiftWaveform(double offset);

signals:
    void actionsChanged();
};

#endif // ACTIONSMODEL_H
