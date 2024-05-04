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

ParsedData::ParsedData(QObject* parent) :
    QObject(parent)
{
    clear();
}

void ParsedData::clear(size_t size)
{
    mParsedWaveform.reset(new QVector<uint8_t>(size));
    mParsedData.reset(new QVector<DataBlock>());
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

    mParsedData->append(db);
}
