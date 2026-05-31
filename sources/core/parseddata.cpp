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

#include "parseddata.h"
#include "sources/translations/translations.h"
#include <algorithm>

ParsedData::ParsedData(QObject* parent) :
    ParsedDataModel({
        Translations::instance()->id_block_number,
        Translations::instance()->id_block_type, Translations::instance()->id_block_name,
        Translations::instance()->id_block_size, Translations::instance()->id_block_status }, parent)
{
    clear();
}

void ParsedData::beginParse() {
    invalidateItems();
}

void ParsedData::endParse() {
    removeOutdatedItems();
}

void ParsedData::clear(size_t size)
{
    mParsedWaveform.reset(new QVector<uint8_t>(size));
    mParsedData.reset(new QVector<QSharedPointer<DataBlock>>());
}

void ParsedData::fillParsedWaveform(const ParsedData::WaveformPart& p, uint8_t val)
{
    for (auto i = p.begin; i <= p.end; ++i) {
        setParsedWaveform(i, val);
    }
}

void ParsedData::fillParsedWaveform(const ParsedData::WaveformPart& p, uint8_t val, size_t begin, uint8_t begin_val, size_t end, uint8_t end_val)
{
    fillParsedWaveform(p, val);
    setParsedWaveform(begin, begin_val);
    setParsedWaveform(end, end_val);
}

void ParsedData::fillParsedWaveform(const ParsedData::WaveformPart& begin, const ParsedData::WaveformPart& end, uint8_t val, uint8_t begin_val, uint8_t end_val)
{
    fillParsedWaveform(begin, val);
    fillParsedWaveform(end, val);
    setParsedWaveform(begin.begin, begin_val);
    setParsedWaveform(end.end, end_val);
}

void ParsedData::storeData(QVector<uint8_t>&& data, QMap<size_t, uint>&& dataMapping, size_t begin, size_t end, QVector<ParsedData::WaveformPart>&& waveformData, uint8_t parity)
{
    if (data.empty()) {
        return;
    }

    DataBlock db;
    db.dataStart = begin;
    db.dataEnd = end;
    db.dataMapping = std::move(dataMapping);
    db.waveformData = std::move(waveformData);
    //Storing parity data
    db.parityAwaited = data.last();
    db.parityCalculated = parity;
    db.state = parity == db.parityAwaited ? DataState::OK : DataState::R_TAPE_LOADING_ERROR; //Should be checked with checksum
    //Storing parsed data block
    db.data = std::move(data);

    storeDataBlock(std::move(db));
}

void ParsedData::updateDataSnapshot(const QVector<uint8_t>& data, const QMap<size_t, uint>& dataMapping, size_t begin, size_t end, const QVector<ParsedData::WaveformPart>& waveformData, uint8_t parity)
{
    if (data.empty()) {
        return;
    }

    DataBlock db;
    db.dataStart = begin;
    db.dataEnd = end;
    db.dataMapping = dataMapping;
    db.waveformData = waveformData;
    db.parityAwaited = data.last();
    db.parityCalculated = parity;
    db.state = parity == db.parityAwaited ? DataState::OK : DataState::R_TAPE_LOADING_ERROR;
    db.data = data;

    storeDataBlock(std::move(db));
}

void ParsedData::storeDataBlock(DataBlock&& dataBlock)
{
    auto existingBlockIt { std::find_if(mParsedData->begin(), mParsedData->end(), [&dataBlock](const QSharedPointer<DataBlock>& block) {
        return !block.isNull() && block->dataStart == dataBlock.dataStart;
    }) };

    if (existingBlockIt != mParsedData->end()) {
        **existingBlockIt = std::move(dataBlock);
        addData(*existingBlockIt);
        return;
    }

    mParsedData->emplace_back(QSharedPointer<DataBlock>::create(std::move(dataBlock)));
    addData(mParsedData->last());
}
