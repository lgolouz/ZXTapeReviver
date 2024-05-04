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

#include "sources/defines.h"
#include <QString>
#include <QSharedPointer>

class ActionBase
{
    const int m_channel;
    const QString m_actionName;

public:
    ActionBase(int channel, const QString& name = { });
    virtual ~ActionBase() = default;

    int channel() const;
    const QString& actionName() const;

    virtual bool apply() = 0;
    virtual void undo() = 0;

protected:
    virtual bool isActionValid(const QSharedPointer<QWavVector>& wf) const;
};

#endif // ACTIONBASE_H
