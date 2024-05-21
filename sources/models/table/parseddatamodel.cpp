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

#include "parseddatamodel.h"

ParsedDataModel::DataItem::DataItem(DataBlock* dataBlock, bool updated) :
    m_updated(updated),
    m_checked(false),
    m_dataBlock(dataBlock)
{

}

bool ParsedDataModel::DataItem::operator== (const DataItem& b) const {
    if (m_dataBlock != nullptr && m_dataBlock->waveformData.size() > 0) {
        if (b.m_dataBlock != nullptr && b.m_dataBlock->waveformData.size() > 0) {
            return m_dataBlock->waveformData.front().begin == b.m_dataBlock->waveformData.front().begin;
        }
        return false;
    }
    return m_dataBlock == b.m_dataBlock;
}

bool ParsedDataModel::DataItem::operator< (const DataItem& b) const {
    if (m_dataBlock != nullptr && m_dataBlock->waveformData.size() > 0) {
        if (b.m_dataBlock != nullptr && b.m_dataBlock->waveformData.size() > 0) {
            return m_dataBlock->waveformData.front().begin < b.m_dataBlock->waveformData.front().begin;
        }
        return false;
    }
    return b.m_dataBlock != nullptr;
}

bool ParsedDataModel::DataItem::checked() const {
    return m_checked;
}

void ParsedDataModel::DataItem::setChecked(bool b) {
    m_checked = b;
}

bool ParsedDataModel::DataItem::updated() const {
    return m_updated;
}

void ParsedDataModel::DataItem::setUpdated(bool b) {
    m_updated = b;
}

ParsedDataModel::DataBlock* ParsedDataModel::DataItem::dataBlock() const {
    return m_dataBlock;
}

void ParsedDataModel::DataItem::setDataBlock(DataBlock* b) {
    m_dataBlock = b;
}

ParsedDataModel::ParsedDataModel(const QStringList& h_header, QObject* parent) :
    ZxTableModel{h_header, parent}
{

}

int ParsedDataModel::rowCount(const QModelIndex& index) const {
    Q_UNUSED(index)
    return m_items.size();
}

QVariant ParsedDataModel::data(const QModelIndex& index, int role) const {
    return { };
}

void ParsedDataModel::invalidateItems() {
    for (const auto& i: m_items) {
        i->setUpdated(false);
    }
}

void ParsedDataModel::removeOutdatedItems() {
    m_items.remove_if([](const auto& i) { return !i->updated(); });
}

void ParsedDataModel::addData(DataItem* item) {
    for (auto it { m_items.begin() }; it != m_items.end(); ++it) {
        if (*it->get() == *item) {
            it->reset(item);
            return;
        } else if (*item < *it->get()) {
            m_items.emplace(it, item);
            return;
        }
    }

    m_items.emplace_back(item);
}
