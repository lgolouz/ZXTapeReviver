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
#include "sources/core/parseddata.h"
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

private:
    enum StateType { SEARCH_OF_PILOT_TONE, PILOT_TONE, SYNCHRO_SIGNAL, DATA_SIGNAL, END_OF_DATA, NO_MORE_DATA };

    template <typename T>
    QVector<ParsedData::WaveformPart> parseChannel(const QVector<T>& ch) {
        decltype(parseChannel(ch)) result;
        if (ch.size() < 1) {
            return result;
        }

        auto it = ch.begin();
        auto val = *it;
        while (it != ch.end()) {
            auto prevIt = it;
            it = std::find_if(it, ch.end(), [&val](const T& i) { return lessThanZero(val) != lessThanZero(i); });
            typename std::remove_reference<decltype(*result.begin())>::type part;
            part.begin = std::distance(ch.begin(), prevIt);
            part.end = std::distance(ch.begin(), std::prev(it));
            part.length = std::distance(prevIt, it);
            part.sign = lessThanZero(val) ? ParsedData::NEGATIVE : ParsedData::POSITIVE;

            result.append(part);

            if (it != ch.end()) {
                val = *it;
            }
        }

        return result;
    }

    //Helper methods intended to use in case of change we can made them only once
    __attribute__((always_inline)) inline bool isZeroFreqFitsInDelta(uint32_t sampleRate, uint32_t length, uint32_t signalFreq, double signalDeltaBelow, double signalDeltaAbove) const;
    __attribute__((always_inline)) inline bool isOneFreqFitsInDelta(uint32_t sampleRate, uint32_t length, uint32_t signalFreq, double signalDeltaBelow, double signalDeltaAbove) const;

    WavReader& mWavReader;
    QMap<uint, ParsedData*> m_parsedData;
    // QMap<uint, QVector<uint8_t>> mParsedWaveform;
    // QMap<uint, QVector<DataBlock>> mParsedData;
    mutable QVector<bool> mSelectedBlocks;

protected:
    explicit WaveformParser(QObject* parent = nullptr);
    QVariantList getParsedChannelData(uint chNum) const;
    __attribute__((always_inline)) inline ParsedData* getOrCreateParsedDataPtr(uint chNum);
    __attribute__((always_inline)) inline ParsedData* getParsedDataPtr(uint chNum) const;

public:
    virtual ~WaveformParser() override = default;

    static WaveformParser* instance();

    void parse(uint chNum);
    void saveTap(uint chNum, const QString& fileName = QString());
    void saveWaveform(uint chNum);
    QVector<uint8_t> getParsedWaveform(uint chNum) const;
    QPair<QVector<ParsedData::DataBlock>, QVector<bool>> getParsedData(uint chNum) const;
    QSharedPointer<QVector<ParsedData::DataBlock>> getParsedDataSharedPtr(uint chNum) const;

    void repairWaveform2(uint chNum);

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
