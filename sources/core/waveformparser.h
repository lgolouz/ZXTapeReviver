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

#ifndef WAVEFORMPARSER_H
#define WAVEFORMPARSER_H

#include <iterator>
#include <QMap>
#include <QVector>
#include <QVariantMap>
#include <QVariantList>
#include "sources/core/wavreader.h"
#include "sources/defines.h"

class WaveformParser : public QObject
{
    Q_OBJECT

    Q_PROPERTY(QVariantList parsedChannel0 READ getParsedChannel0 NOTIFY parsedChannel0Changed)
    Q_PROPERTY(QVariantList parsedChannel1 READ getParsedChannel1 NOTIFY parsedChannel1Changed)

public:
//    enum SignalValue { ZERO, ONE, PILOT, SYNCHRO };
//    struct WaveformData
//    {
//        uint32_t begin;
//        uint32_t end;
//        uint32_t waveBegin;
//        uint32_t waveEnd;
//        SignalValue value;
//    };

// |------------------------------- 1 - zero '0' data bit
// | |----------------------------- 1 - one  '1' data bit
// | | |--------------------------- 1 - pilot tone
// | | | |------------------------- 1 - synchro signal
// | | | | |----------------------- 1 - byte bound
// | | | | | |--------------------- 1 - begin of signal sequence
// | | | | | | |------------------- 1 - middle of signal sequence
// | | | | | | | |----------------- 1 - end of signal sequence
// x x x x x x x x

const uint8_t zeroBit        = 0b10000000; //zero data bit
const uint8_t oneBit         = 0b01000000; //one bit
const uint8_t pilotTone      = 0b00100000; //pilot tone
const uint8_t synchroSignal  = 0b00010000; //synchro signal
const uint8_t byteBound      = 0b00001000; //byte bound
const uint8_t sequenceBegin  = 0b00000100; //begin of signal sequence
const uint8_t sequenceMiddle = 0b00000010; //middle of signal sequence
const uint8_t sequenceEnd    = 0b00000001; //end of signal sequence

private:
    enum WaveformSign { POSITIVE, NEGATIVE };
    enum StateType { SEARCH_OF_PILOT_TONE, PILOT_TONE, SYNCHRO_SIGNAL, DATA_SIGNAL, END_OF_DATA, NO_MORE_DATA };
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
        QVector<WaveformPart> waveformData;
        DataState state;
    };

    template <typename T>
    QVector<WaveformPart> parseChannel(const QVector<T>& ch) {
        QVector<WaveformPart> result;
        if (ch.size() < 1) {
            return result;
        }

        auto it = ch.begin();
        auto val = *it;
        while(it != ch.end()) {
            auto prevIt = it;
            it = std::find_if(it, ch.end(), [&val](const T& i) { return lessThanZero(val) != lessThanZero(i); });
            WaveformPart part;
            part.begin = std::distance(ch.begin(), prevIt);
            part.end = std::distance(ch.begin(), std::prev(it));
            part.length = std::distance(prevIt, it);
            part.sign = lessThanZero(val) ? NEGATIVE : POSITIVE;

            result.append(part);

            if (it != ch.end()) {
                val = *it;
            }
        }

        return result;
    }

    void fillParsedWaveform(uint chNum, const WaveformPart& p, uint8_t val);

    WavReader& mWavReader;
    QMap<uint, QVector<uint8_t>> mParsedWaveform;
    QMap<uint, QVector<DataBlock>> mParsedData;
    mutable QVector<bool> mSelectedBlocks;

protected:
    explicit WaveformParser(QObject* parent = nullptr);
    QVariantList getParsedChannelData(uint chNum) const;

public:
    virtual ~WaveformParser() override = default;

    static WaveformParser* instance();

    void parse(uint chNum);
    void saveTap(uint chNum, const QString& fileName = QString());
    void saveWaveform(uint chNum);
    QVector<uint8_t> getParsedWaveform(uint chNum) const;

    Q_INVOKABLE void toggleBlockSelection(int blockNum);
    Q_INVOKABLE int getBlockDataStart(uint chNum, uint blockNum) const;
    Q_INVOKABLE int getBlockDataEnd(uint chNum, uint blockNum) const;
    Q_INVOKABLE int getPositionByAddress(uint chNum, uint blockNum, uint addr) const;
    //getters
    QVariantList getParsedChannel0() const;
    QVariantList getParsedChannel1() const;

signals:
    void parsedChannel0Changed();
    void parsedChannel1Changed();
};

#endif // WAVEFORMPARSER_H
