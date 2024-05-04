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

class ParsedData : public QObject
{
    Q_OBJECT

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
        uint32_t begin;
        uint32_t end;
        uint32_t length;
        WaveformSign sign;
    };

    struct DataBlock
    {
        uint32_t dataStart;
        uint32_t dataEnd;
        QVector<uint8_t> data;
        QMap<size_t, uint> dataMapping;
        QVector<WaveformPart> waveformData;
        DataState state;
        uint8_t parityCalculated;
        uint8_t parityAwaited;
    };

    explicit ParsedData(QObject* parent = nullptr);
    virtual ~ParsedData() override = default;

    void storeData(QVector<uint8_t>&& data, QMap<size_t, uint>&& dataMapping, size_t begin, size_t end, QVector<ParsedData::WaveformPart>&& waveformData, uint8_t parity);
    void clear(size_t size = 0);
    void fillParsedWaveform(const ParsedData::WaveformPart& p, uint8_t val);
    void fillParsedWaveform(const ParsedData::WaveformPart& p, uint8_t val, size_t begin, uint8_t begin_val, size_t end, uint8_t end_val);
    void fillParsedWaveform(const ParsedData::WaveformPart& begin, const ParsedData::WaveformPart& end, uint8_t val, uint8_t begin_val, uint8_t end_val);
    __attribute__((always_inline)) inline void setParsedWaveform(size_t pos, uint8_t val) {
        mParsedWaveform.get()->operator [](pos) = val;
    }
    __attribute__((always_inline)) inline void orParsedWaveform(size_t pos, uint8_t val) {
        mParsedWaveform.get()->operator [](pos) |= val;
    }

    __attribute__((always_inline)) inline QSharedPointer<QVector<DataBlock>> getParsedData() const {
        return mParsedData;
    }
    __attribute__((always_inline)) inline QSharedPointer<QVector<uint8_t>> getParsedWaveform() const {
        return mParsedWaveform;
    }

private:
    QSharedPointer<QVector<uint8_t>> mParsedWaveform;
    QSharedPointer<QVector<DataBlock>> mParsedData;

};

#endif // PARSEDDATA_H
