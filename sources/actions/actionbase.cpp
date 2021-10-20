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

ActionBase::ActionBase(const QString& actionName, QObject* parent) :
    QObject(parent),
    m_actionName(actionName)
{

}

const QString& ActionBase::name() const {
    return m_actionName;
}
