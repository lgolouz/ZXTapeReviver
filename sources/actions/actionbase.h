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

#ifndef ACTIONBASE_H
#define ACTIONBASE_H

#include <QObject>

class ActionBase : public QObject
{
    Q_OBJECT

    ActionBase(const QString& actionName, QObject* parent = nullptr);

private:
    const QString m_actionName;

public:
    ActionBase(const ActionBase& other) = delete;
    ActionBase& operator=(const ActionBase& other) = delete;

    virtual ~ActionBase() override = default;

    const QString& name() const;
};

#endif // ACTIONBASE_H
