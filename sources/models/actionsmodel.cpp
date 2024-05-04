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

#include "actionsmodel.h"
#include <QVariantMap>
#include "sources/actions/shiftwaveformaction.h"

ActionsModel::ActionsModel(QObject* parent) :
    QObject(parent)
{

}

void ActionsModel::addAction(QSharedPointer<ActionBase> action) {
    if (action->apply()) {
        m_actions.append(action);
        emit actionsChanged();
    }
}

void ActionsModel::removeAction() {
    if (!m_actions.isEmpty()) {
        m_actions.takeLast()->undo();
        emit actionsChanged();
    }
}

void ActionsModel::shiftWaveform(double offset) {
    addAction(QSharedPointer<ShiftWaveFormAction>::create(0, ShiftWaveFormActionParams { static_cast<QWavVectorType>(offset) }));
}


QVariantList ActionsModel::getActions() const {
    QVariantList result;
    for (const auto& a: m_actions) {
        result.append(QVariantMap { { "name", a->actionName() } });
    }
    return result;
}

ActionsModel* ActionsModel::instance() {
    static ActionsModel m;
    return &m;
}
