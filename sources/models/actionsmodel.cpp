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
    QAbstractTableModel(parent)
{

}

void ActionsModel::addAction(QSharedPointer<ActionBase> action) {
    if (action->apply()) {
        m_redoActions.clear();
        const int row { static_cast<int>(m_actions.size()) };
        beginInsertRows({}, row, row);
        m_actions.append(action);
        endInsertRows();
        emit actionsChanged();
    }
}

void ActionsModel::removeAction() {
    if (!m_actions.isEmpty()) {
        const int row { static_cast<int>(m_actions.size() - 1) };
        beginRemoveRows({}, row, row);
        auto action { m_actions.takeLast() };
        action->undo();
        m_redoActions.append(action);
        endRemoveRows();
        emit actionsChanged();
    }
}

void ActionsModel::redoAction() {
    if (m_redoActions.isEmpty()) {
        return;
    }

    auto action { m_redoActions.takeLast() };
    if (action->apply()) {
        const int row { static_cast<int>(m_actions.size()) };
        beginInsertRows({}, row, row);
        m_actions.append(action);
        endInsertRows();
    }
    emit actionsChanged();
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

bool ActionsModel::getCanUndo() const {
    return !m_actions.isEmpty();
}

bool ActionsModel::getCanRedo() const {
    return !m_redoActions.isEmpty();
}

int ActionsModel::rowCount(const QModelIndex& parent) const {
    return parent.isValid() ? 0 : static_cast<int>(m_actions.size());
}

int ActionsModel::columnCount(const QModelIndex& parent) const {
    return parent.isValid() ? 0 : 2;
}

QVariant ActionsModel::data(const QModelIndex& index, int role) const {
    if (!index.isValid() || role != Qt::DisplayRole || index.row() < 0 || index.row() >= m_actions.size()) {
        return {};
    }

    switch (index.column()) {
        case 0:
            return index.row() + 1;

        case 1:
            return m_actions.at(index.row())->actionName();

        default:
            return {};
    }
}

QVariant ActionsModel::headerData(int section, Qt::Orientation orientation, int role) const {
    if (orientation != Qt::Horizontal || role != Qt::DisplayRole) {
        return {};
    }

    return section == 0 ? qtTrId("id_suspicious_point_number") : qtTrId("id_action_name");
}

int ActionsModel::columnWidthProvider(int column) const {
    return column == 0 ? 70 : 180;
}

ActionsModel* ActionsModel::instance() {
    static ActionsModel m;
    return &m;
}
