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

#ifndef PARSEDDATAMODEL_H
#define PARSEDDATAMODEL_H

#include "sources/models/base/zxtablemodel.h"
#include "sources/models/parsersettingsmodel.h"

class ParsedDataModel : public ZxTableModel
{
public:
    // |------------------------------- 1 - zero '0' data bit
    // | |----------------------------- 1 - one  '1' data bit
    // | | |--------------------------- 1 - pilot tone
    // | | | |------------------------- 1 - synchro signal
    // | | | | |----------------------- 1 - byte bound
    // | | | | | |--------------------- 1 - begin of signal sequence
    // | | | | | | |------------------- 1 - middle of signal sequence
    // | | | | | | | |----------------- 1 - end of signal sequence
    // x x x x x x x x

    static constexpr const uint8_t zeroBit        = 0b10000000; //zero data bit
    static constexpr const uint8_t oneBit         = 0b01000000; //one bit
    static constexpr const uint8_t pilotTone      = 0b00100000; //pilot tone
    static constexpr const uint8_t synchroSignal  = 0b00010000; //synchro signal
    static constexpr const uint8_t byteBound      = 0b00001000; //byte bound
    static constexpr const uint8_t sequenceBegin  = 0b00000100; //begin of signal sequence
    static constexpr const uint8_t sequenceMiddle = 0b00000010; //middle of signal sequence
    static constexpr const uint8_t sequenceEnd    = 0b00000001; //end of signal sequence

    enum WaveformSign { POSITIVE, NEGATIVE };
    enum DataState { OK, R_TAPE_LOADING_ERROR };

    struct WaveformPart
    {
        size_t begin;
        size_t end;
        size_t length;
        WaveformSign sign;
    };

    struct DataBlock
    {
        size_t dataStart;
        size_t dataEnd;
        QVector<uint8_t> data;
        QMap<size_t, uint> dataMapping;
        QVector<WaveformPart> waveformData;
        DataState state;
        uint8_t parityCalculated;
        uint8_t parityAwaited;
    };

    explicit ParsedDataModel(const QStringList& h_header, QObject* parent = nullptr);

    virtual QVariant data(const QModelIndex& index, int role) const override;
    virtual int rowCount(const QModelIndex& index = QModelIndex()) const override;

protected:
    struct DataItem {
        DataItem(DataBlock* dataBlock, bool updated = true);
        virtual ~DataItem() = default;

        bool checked() const;
        void setChecked(bool b);

        bool updated() const;
        void setUpdated(bool b);

        DataBlock* dataBlock() const;
        void setDataBlock(DataBlock* b);

        bool operator== (const DataItem& b) const;
        bool operator< (const DataItem& b) const;

    protected:
        bool m_updated; //flag to know if the item updated (or newly created) or not. All not updated items should be deleted after data parsing pass
        bool m_checked; //flag to know if the item has been checked by user
        DataBlock* m_dataBlock;
    };

    void addData(DataItem* item);

    void invalidateItems();
    void removeOutdatedItems();

    std::list<std::unique_ptr<DataItem>> m_items;
};

#endif // PARSEDDATAMODEL_H
