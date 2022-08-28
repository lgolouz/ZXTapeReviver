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

#include "actionbase.h"

ActionBase::ActionBase(int channel, const QString& name) :
    m_channel(channel),
    m_actionName(name)
{

}

int ActionBase::channel() const {
    return m_channel;
}

const QString& ActionBase::actionName() const {
    return m_actionName;
}

bool ActionBase::isActionValid(const QSharedPointer<QWavVector>& wf) const {
    return !wf.isNull();
}
