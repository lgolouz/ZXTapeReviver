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
#include "sources/translations/translations.h"

namespace {
static QMap<int, QString> blockTypes {
    {0x00, "Program"},
    {0x01, "Number Array"},
    {0x02, "Character Array"},
    {0x03, "Bytes"}
};

static std::vector<ParsedDataModel::ParsedDataModelRoles> columnToRoleMapping {
    ParsedDataModel::ParsedDataModelRoles::BlockNumber,
    ParsedDataModel::ParsedDataModelRoles::BlockType, ParsedDataModel::ParsedDataModelRoles::BlockName,
    ParsedDataModel::ParsedDataModelRoles::BlockSize, ParsedDataModel::ParsedDataModelRoles::BlockStatus };
}

ParsedDataModel::DataItem::DataItem(QSharedPointer<DataBlock> dataBlock, bool updated) :
    m_updated(updated),
    m_checked(false),
    m_dataBlock(dataBlock)
{

}

bool ParsedDataModel::DataItem::operator== (const DataItem& b) const {
    auto mdataBlock = m_dataBlock.lock();
    auto bmdataBlock = b.m_dataBlock.lock();
    if (mdataBlock != nullptr && mdataBlock->waveformData.size() > 0) {
        if (bmdataBlock != nullptr && bmdataBlock->waveformData.size() > 0) {
            return mdataBlock->waveformData.front().begin == bmdataBlock->waveformData.front().begin;
        }
        return false;
    }
    return mdataBlock.isNull() == bmdataBlock.isNull();
}

bool ParsedDataModel::DataItem::operator< (const DataItem& b) const {
    auto mdataBlock = m_dataBlock.lock();
    auto bmdataBlock = b.m_dataBlock.lock();
    if (mdataBlock != nullptr && mdataBlock->waveformData.size() > 0) {
        if (bmdataBlock != nullptr && bmdataBlock->waveformData.size() > 0) {
            return mdataBlock->waveformData.front().begin < bmdataBlock->waveformData.front().begin;
        }
        return false;
    }
    return bmdataBlock != nullptr;
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

QSharedPointer<ParsedDataModel::DataBlock> ParsedDataModel::DataItem::dataBlock() const {
    return m_dataBlock.lock();
}

void ParsedDataModel::DataItem::setDataBlock(QSharedPointer<DataBlock> b) {
    m_dataBlock = b;
}

ParsedDataModel::ParsedDataModel(const QStringList& h_header, QObject* parent) :
    ZxTableModel{h_header, {40, 80, 120, 60, 60}, parent}
{

}

int ParsedDataModel::rowCount(const QModelIndex& index) const {
    Q_UNUSED(index)
    return m_items.size();
}

QVariant ParsedDataModel::headerData(int section, Qt::Orientation orientation, int role) const {
    if (role != Qt::DisplayRole) {
        return {};
    }

    if (orientation == Qt::Vertical) {
        return section < rowCount() ? QString::number(section + 1) : QVariant();
    }

    switch (section) {
        case 0: return Translations::instance()->id_block_number;
        case 1: return Translations::instance()->id_block_type;
        case 2: return Translations::instance()->id_block_name;
        case 3: return Translations::instance()->id_block_size;
        case 4: return Translations::instance()->id_block_status;
        default: return {};
    }
}

ParsedDataModel::DataItem* ParsedDataModel::at(const size_t idx) const {
    if (idx >= m_items.size()) {
        return nullptr;
    }

    std::decay_t<decltype(idx)> c { 0 };
    auto it { m_items.begin() };
    for (; c < idx && it != m_items.end(); ++c) {
        ++it;
    }

    if (it != m_items.end()) {
        return it->get();
    }
    return nullptr;
};

QMap<int, QVariant> ParsedDataModel::getBlockData(const size_t idx) const {
    const auto* itm = at(idx);
    if (itm == nullptr) {
        return { };
    }
    const auto& i { *itm->dataBlock() };
    std::decay_t<decltype(getBlockData(idx))> m { };

    //m.insert("block", QVariantMap { {"blockSelected", blockNumber < (unsigned) mSelectedBlocks.size() ? mSelectedBlocks[blockNumber] : (mSelectedBlocks.append(true), true)}, {"blockNumber", blockNumber++} });
    m.insert(BlockNumber, idx + 1);
    if (i.data.size() > 0) {
        auto d = i.data.at(0);
        int blockType = -1;
        auto btIt = blockTypes.end();
        QString blockTypeName;
        if (d == 0x00 && i.data.size() > 1) {
            d = i.data.at(1);
            btIt = blockTypes.find(d);
            blockType = btIt == blockTypes.end() ? -1 : d;
            blockTypeName = blockType == -1 ? QString::number(d, 16) : *btIt;
        }
        else {
            blockType = -2;
            blockTypeName = d == 0x00 ? Translations::instance()->id_header : Translations::instance()->id_code;
        }
        m.insert(BlockType, blockTypeName);
        //QString sizeText = QString::number(i.data.size());
        int32_t sizeVal { -1 };
        if (i.data.size() > 13 && btIt != blockTypes.end()) {
            sizeVal = i.data.at(13) * 256 + i.data.at(12);
            //sizeText += QString(" (%1)").arg(i.data.at(13) * 256 + i.data.at(12));
        }
        m.insert(BlockSize, i.data.size());
        m.insert(ExpectedBlockSize, sizeVal < 0 ? QVariant() : QVariant(uint16_t(sizeVal)));
        QString nameText;
        if (blockType >= 0) {
            const auto loopRange { std::min(decltype(i.data.size())(12), i.data.size()) };
            nameText = QByteArray((const char*) &i.data.data()[2], loopRange > 1 ? loopRange - 2 : 0);
        }
        m.insert(BlockName, nameText);
        m.insert(BlockStatus, i.state);// == ParsedDataModel::OK ? id_ok : id_error) + id_parity_message.arg(QString::number(i.parityCalculated, 16).toUpper().rightJustified(2, '0')).arg(QString::number(i.parityAwaited, 16).toUpper().rightJustified(2, '0')));
        m.insert(BlockCheckSum, i.parityCalculated);
        m.insert(ExpectedBlockCheckSum, i.parityAwaited);
    }
    else {
        m.insert(BlockType, Translations::instance()->id_unknown);
        m.insert(BlockName, QString());
        m.insert(BlockSize, QVariant());
        m.insert(BlockStatus, Translations::instance()->id_unknown);
    }

    return m;
}

QVariant ParsedDataModel::data(const QModelIndex& index, int role) const {
    if (!index.isValid()) {
        return { };
    }

    qDebug() << index.column();
    const auto blockData { getBlockData(index.row()) };
    switch (role) {
        case Qt::DisplayRole: {
            switch (index.column()) {
                case 3: {
                    const auto expctBlockSize { blockData.find(ExpectedBlockSize).value() };
                    return expctBlockSize == QVariant()
                        ? blockData.find(BlockSize).value()
                        : QString("%1 (%2)").arg(blockData.find(BlockSize).value().toUInt()).arg(expctBlockSize.toUInt());
                }

                case 4:
                    return QString("%1 %2").arg(blockData.find(BlockStatus).value().toUInt() == ParsedDataModel::OK ? Translations::instance()->id_ok : Translations::instance()->id_error)
                                           .arg(Translations::instance()->id_parity_message.arg(QString::number(blockData.find(BlockCheckSum)->toUInt(), 16).toUpper().rightJustified(2, '0'))
                                                                                           .arg(QString::number(blockData.find(ExpectedBlockCheckSum)->toUInt(), 16).toUpper().rightJustified(2, '0')));

                default:
                    return blockData.find(columnToRoleMapping[index.column()]).value();
            }
        }

        case BlockNumber:
        case BlockType:
        case BlockName:
        case BlockSize:
        case ExpectedBlockSize:
        case BlockStatus:
        case BlockCheckSum:
        case ExpectedBlockCheckSum:
            return blockData.find(role).value();

        default:
            return { };
    }
}

QHash<int, QByteArray> ParsedDataModel::roleNames() const {
    auto names = ZxTableModel::roleNames();
    names.insert(generateRoleNames<ParsedDataModelRoles>());
    return names;
}

void ParsedDataModel::invalidateItems() {
    beginResetModel();

    for (const auto& i: m_items) {
        i->setUpdated(false);
    }

    endResetModel();
}

void ParsedDataModel::removeOutdatedItems() {
    beginResetModel();

    m_items.remove_if([](const auto& i) { return !i->updated(); });

    endResetModel();
}

void ParsedDataModel::addData(QSharedPointer<DataBlock> block) {
    auto item { std::make_unique<DataItem>(block) };
    int row { 0 };
    for (auto it { m_items.begin() }; it != m_items.end(); ++it) {
        if (*it->get() == *item) {
            *it = std::move(item);
            const auto modelIndexBegin { index(row, 0) };
            const auto modelIndexEnd { index(row, columnCount() - 1) };
            emit dataChanged(modelIndexBegin, modelIndexEnd);
            return;
        } else if (*item < *it->get()) {
            beginInsertRows({}, row, row);
            m_items.emplace(it, std::move(item));
            endInsertRows();
            return;
        }
        ++row;
    }

    beginInsertRows({}, row, row);
    m_items.emplace_back(std::move(item));
    endInsertRows();
}
