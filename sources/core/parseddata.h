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

#ifndef PARSEDDATA_H
#define PARSEDDATA_H

#include <QObject>
#include <QSharedPointer>
#include <QMap>
#include "sources/models/table/parseddatamodel.h"

class ParsedData : public ParsedDataModel
{
    Q_OBJECT

public:
    explicit ParsedData(QObject* parent = nullptr);
    virtual ~ParsedData() override = default;

    void beginParse(); // should be called at the very beginning of the parse process
    void endParse(); // should be called just after the end of the parse process

    void storeData(QVector<uint8_t>&& data, QMap<size_t, uint>&& dataMapping, size_t begin, size_t end, QVector<ParsedData::WaveformPart>&& waveformData, uint8_t parity);
    void clear(size_t size = 0);
    void fillParsedWaveform(const ParsedData::WaveformPart& p, uint8_t val);
    void fillParsedWaveform(const ParsedData::WaveformPart& p, uint8_t val, size_t begin, uint8_t begin_val, size_t end, uint8_t end_val);
    void fillParsedWaveform(const ParsedData::WaveformPart& begin, const ParsedData::WaveformPart& end, uint8_t val, uint8_t begin_val, uint8_t end_val);
    Q_ALWAYS_INLINE void setParsedWaveform(size_t pos, uint8_t val) { (*mParsedWaveform)[pos] = val; }
    Q_ALWAYS_INLINE void orParsedWaveform(size_t pos, uint8_t val) { (*mParsedWaveform)[pos] |= val; }

    Q_ALWAYS_INLINE QSharedPointer<QVector<DataBlock>> getParsedData() const { return mParsedData; }
    Q_ALWAYS_INLINE QSharedPointer<QVector<uint8_t>> getParsedWaveform() const { return mParsedWaveform; }

private:
    QSharedPointer<QVector<uint8_t>> mParsedWaveform;
    QSharedPointer<QVector<DataBlock>> mParsedData;
};

#endif // PARSEDDATA_H
