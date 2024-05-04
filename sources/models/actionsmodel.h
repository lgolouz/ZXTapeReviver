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

#include <QObject>
#include <QVariantList>
#include <QList>
#include <QSharedPointer>
#include "sources/actions/actionbase.h"

class ActionsModel final : public QObject
{
    Q_OBJECT

    Q_PROPERTY(QVariantList actions READ getActions NOTIFY actionsChanged)

    QList<QSharedPointer<ActionBase>> m_actions;

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

    void addAction(QSharedPointer<ActionBase> action);
    Q_INVOKABLE void removeAction();
    Q_INVOKABLE void shiftWaveform(double offset);

signals:
    void actionsChanged();
};

#endif // ACTIONSMODEL_H
