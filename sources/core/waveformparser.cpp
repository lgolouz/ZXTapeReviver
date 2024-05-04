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

#include "waveformparser.h"
#include "sources/models/parsersettingsmodel.h"
#include "sources/translations/translations.h"
#include <QDebug>
#include <QDateTime>
#include <QByteArray>
#include <QVariantMap>
#include <algorithm>

#define HARDCODED_DATA_SIGNAL_DELTA 0.75

WaveformParser::WaveformParser(QObject* parent) :
    QObject(parent),
    mWavReader(*WavReader::instance())
{

}

void WaveformParser::repairWaveform2(uint chNum) {
    if (chNum >= mWavReader.getNumberOfChannels()) {
        qDebug() << "Trying to parse channel that exceeds overall number of channels";
        return;
    }

    QWavVector& channel = *(chNum == 0 ? mWavReader.getChannel0() : mWavReader.getChannel1());
    QVector<ParsedData::WaveformPart> parsed = parseChannel<QWavVectorType>(channel);

    const auto& parserSettings = ParserSettingsModel::instance()->getParserSettings();
    const double sampleRate = mWavReader.getSampleRate();
    for (auto it { parsed.begin() }; it != parsed.end();) {
        auto itprev = it++;
        if (it != parsed.end()) {
            bool isZero = isZeroFreqFitsInDelta(sampleRate, (*it).length + (*itprev).length, parserSettings.zeroFreq, parserSettings.zeroDelta, HARDCODED_DATA_SIGNAL_DELTA);
            bool isOne = isOneFreqFitsInDelta(sampleRate, (*it).length + (*itprev).length, parserSettings.oneFreq, HARDCODED_DATA_SIGNAL_DELTA, parserSettings.oneDelta);
            if (isZero || isOne) {
                auto it1 = std::next(channel.begin(), (*itprev).begin);
                auto it2 = std::next(channel.begin(), (*it).end);
                auto itmiddle = std::next(it1, std::distance(it1, it2) / 2);
                const auto min_max = std::minmax_element(it1, it2);
                auto [val1, val2] = (*itprev).sign == ParsedData::WaveformSign::NEGATIVE ? min_max : decltype(min_max) {min_max.second, min_max.first};
                auto itr = it1;
                for (; itr != itmiddle; ++itr) {
                    *itr  = *val1;
                }
                for (; itr != it2; ++itr) {
                    *itr = *val2;
                }
            }
            ++it;
        }
    }
}

inline ParsedData* WaveformParser::getOrCreateParsedDataPtr(uint chNum)
{
    auto it = m_parsedData.find(chNum);
    return it == m_parsedData.end() ? *m_parsedData.insert(chNum, new ParsedData(this)) : *it;
}

inline ParsedData* WaveformParser::getParsedDataPtr(uint chNum) const
{
    auto parsedDataIt { m_parsedData.find(chNum) };
    if (parsedDataIt == m_parsedData.end()) {
        qWarning() << "Unable to access parsed data with requested channel number.";
        return nullptr;
    }
    return *parsedDataIt;
}

QSharedPointer<QVector<ParsedData::DataBlock>> WaveformParser::getParsedDataSharedPtr(uint chNum) const
{
    auto p = getParsedDataPtr(chNum);
    return p == nullptr ? QSharedPointer<QVector<ParsedData::DataBlock>> { } : p->getParsedData();
}

inline bool WaveformParser::isZeroFreqFitsInDelta(uint32_t sampleRate, uint32_t length, uint32_t signalFreq, double signalDeltaBelow, double signalDeltaAbove) const
{
    return isFreqFitsInDelta2(sampleRate, length, signalFreq, signalDeltaBelow, signalDeltaAbove);
}

inline bool WaveformParser::isOneFreqFitsInDelta(uint32_t sampleRate, uint32_t length, uint32_t signalFreq, double signalDeltaBelow, double signalDeltaAbove) const
{
    return isFreqFitsInDelta2(sampleRate, length, signalFreq, signalDeltaBelow, signalDeltaAbove);
}

void WaveformParser::parse(uint chNum)
{
    if (chNum >= mWavReader.getNumberOfChannels()) {
        qDebug() << "Trying to parse channel that exceeds overall number of channels";
        return;
    }

    QWavVector& channel = *(chNum == 0 ? mWavReader.getChannel0() : mWavReader.getChannel1());
    QVector<ParsedData::WaveformPart> parsed = parseChannel<QWavVectorType>(channel);

    const double sampleRate = mWavReader.getSampleRate();
    auto& parsedData = *getOrCreateParsedDataPtr(chNum);
    parsedData.clear(channel.size());

    auto currentState = SEARCH_OF_PILOT_TONE;
    auto it = parsed.begin();
    QVector<uint8_t> data;
    QVector<ParsedData::WaveformPart> waveformData;
    QMap<size_t, uint> data_mapping;
    //WaveformSign signalDirection = POSITIVE;
    uint32_t dataStart = 0;
    uint8_t bitIndex = 0;
    uint8_t bit = 0;
    uint8_t parity = 0;

    const auto& parserSettings = ParserSettingsModel::instance()->getParserSettings();
    auto isSineNormal = [&parserSettings, sampleRate](const ParsedData::WaveformPart& b, const ParsedData::WaveformPart& e, bool zeroCheck) -> bool {
        if (parserSettings.checkForAbnormalSine) {
            return isFreqFitsInDelta(sampleRate, b.length, zeroCheck ? parserSettings.zeroHalfFreq : parserSettings.oneHalfFreq, zeroCheck ? parserSettings.zeroDelta : parserSettings.oneDelta, parserSettings.sineCheckTolerance) &&
                   isFreqFitsInDelta(sampleRate, e.length, zeroCheck ? parserSettings.zeroHalfFreq : parserSettings.oneHalfFreq, zeroCheck ? parserSettings.zeroDelta : parserSettings.oneDelta, parserSettings.sineCheckTolerance);
        }
        return true;
    };

    const auto isPilotHalfFreq = [&parserSettings, sampleRate](const ParsedData::WaveformPart& p) -> bool {
        return isFreqFitsInDelta(sampleRate, p.length, parserSettings.pilotHalfFreq, parserSettings.pilotDelta, 1.0);
    };
    const auto isSynchroFirstHalfFreq = [&parserSettings, sampleRate](const ParsedData::WaveformPart& p, double deltaDivider = 1.0) -> bool {
        return isFreqFitsInDelta(sampleRate, p.length, parserSettings.synchroFirstHalfFreq, parserSettings.synchroDelta, deltaDivider);
    };
    const auto isSynchroSecondHalfFreq = [&parserSettings, sampleRate](const ParsedData::WaveformPart& p, double deltaDivider = 1.0) -> bool {
        return isFreqFitsInDelta(sampleRate, p.length, parserSettings.synchroSecondHalfFreq, parserSettings.synchroDelta, deltaDivider);
    };

    while (currentState != NO_MORE_DATA) {
        auto prevIt = it;
        switch (currentState) {
        case SEARCH_OF_PILOT_TONE:
            it = std::find_if(it, parsed.end(), [&isPilotHalfFreq, &parsedData, this](const ParsedData::WaveformPart& p) {
                parsedData.fillParsedWaveform(p, 0);
                return isPilotHalfFreq(p);
            });
            if (it != parsed.end()) {
                currentState = PILOT_TONE;
            }
            break;

        case PILOT_TONE:
            for (; it != parsed.end() && isPilotHalfFreq(*it); ++it) {
                //Mark parsed waveform as pilot-tone
                parsedData.fillParsedWaveform(*it, ParsedData::pilotTone | ParsedData::sequenceMiddle);
            };

            //Found the first half of SYNCHRO signal
            if (it != parsed.end()) {
                if (auto itnext { std::next(it) };
                    (parserSettings.preciseSynchroCheck && isSynchroFirstHalfFreq(*it)) ||
                    (!parserSettings.preciseSynchroCheck &&
                     (itnext != parsed.end() &&
                      isFreqFitsInDelta(sampleRate, it->length + itnext->length, parserSettings.synchroFreq, parserSettings.synchroDelta, 1.0))))
                {
                    auto eIt = std::prev(it);
                    //Mark parsed waveform as pilot-tone and sets the begin and end bounds
                    parsedData.fillParsedWaveform(*eIt, ParsedData::pilotTone | ParsedData::sequenceMiddle,
                                                  prevIt->begin, ParsedData::pilotTone | ParsedData::sequenceBegin,
                                                  eIt->end, ParsedData::pilotTone | ParsedData::sequenceEnd);
                    currentState = SYNCHRO_SIGNAL;
                }
                else {
                    currentState = SEARCH_OF_PILOT_TONE;
                }
            }
            else {
                currentState = SEARCH_OF_PILOT_TONE;
            }
            break;

        case SYNCHRO_SIGNAL:
            it = std::next(it);
            if (it != parsed.end()) {
                //Check for second half of SYNCHRO signal or if `preciseSynchroCheck` option is off - assume there is synchro, because we did the check on the previous step
                if (!parserSettings.preciseSynchroCheck || isSynchroSecondHalfFreq(*it)) {
                    //Mark parsed waveform as syncro signal and sets the begin and end bounds
                    parsedData.fillParsedWaveform(*prevIt, *it, ParsedData::synchroSignal | ParsedData::sequenceMiddle,
                                                  ParsedData::synchroSignal | ParsedData::sequenceBegin,
                                                  ParsedData::synchroSignal | ParsedData::sequenceEnd);

                    //Initializing the currently parsing data block
                    currentState = DATA_SIGNAL;

                    it = std::next(it);
                    dataStart = std::distance(parsed.begin(), it);
                    data.clear();
                    waveformData.clear();
                    data_mapping.clear();
                    bitIndex ^= bitIndex;
                    bit ^= bit;
                }
                else {
                    //Got the synchro error
                    currentState = SEARCH_OF_PILOT_TONE;
                }
            }
            break;

        case DATA_SIGNAL:
            it = std::next(it);
            if (it != parsed.end()) {
                const auto len = it->length + prevIt->length;
                const auto storeParsedByte = [&parsedData, &bitIndex, &data, &waveformData, &parity, &bit, it]() {
                    //Store parsed byte in data buffer
                    bitIndex ^= bitIndex;
                    data.append(bit);
                    waveformData.append(*it);
                    parity ^= bit;
                    bit ^= bit;
                };

                //"0" - ZERO
                if (isZeroFreqFitsInDelta(sampleRate, len, parserSettings.zeroFreq, parserSettings.zeroDelta, HARDCODED_DATA_SIGNAL_DELTA) && isSineNormal(*prevIt, *it, true)) {
                    //Mark parsed waveform as "0"-bit and sets the begin and end bounds
                    parsedData.fillParsedWaveform(*prevIt, *it, ParsedData::zeroBit | ParsedData::sequenceMiddle,
                                                  ParsedData::zeroBit | ParsedData::sequenceBegin | (bitIndex == 0 ? ParsedData::byteBound : 0),
                                                  ParsedData::zeroBit | ParsedData::sequenceEnd | (bitIndex == 7 ? ParsedData::byteBound : 0));

                    if (bitIndex == 0) {
                        data_mapping.insert((*prevIt).begin, data.size());
                    }

                    if (bitIndex++ == 7) {
                        data_mapping.insert((*it).end, data.size());
                        storeParsedByte();
                        // //Update byte bound mark
                        // parsedData.orParsedWaveform(it->end, ParsedData::byteBound);
                        // //Store parsed byte in data buffer
                        // bitIndex ^= bitIndex;
                        // data.append(bit);
                        // waveformData.append(*it);
                        // parity ^= bit;
                        // bit ^= bit;
                    }
                }
                // "1" - ONE
                else if (isOneFreqFitsInDelta(sampleRate, len, parserSettings.oneFreq, HARDCODED_DATA_SIGNAL_DELTA, parserSettings.oneDelta) && isSineNormal(*prevIt, *it, false)) {
                    //Mark parsed waveform as "1"-bit and sets the begin and end bounds
                    parsedData.fillParsedWaveform(*prevIt, *it, ParsedData::oneBit | ParsedData::sequenceMiddle,
                                                  ParsedData::oneBit | ParsedData::sequenceBegin | (bitIndex == 0 ? ParsedData::byteBound : 0),
                                                  ParsedData::oneBit | ParsedData::sequenceEnd | (bitIndex == 7 ? ParsedData::byteBound : 0));

                    if (bitIndex == 0) {
                        data_mapping.insert((*prevIt).begin, data.size());
                    }

                    //Set the currently parsed bit
                    bit |= 1 << (7 - bitIndex);
                    if (bitIndex++ == 7) {
                        data_mapping.insert((*it).end, data.size());
                        storeParsedByte();
                        // //Update byte bound mark
                        // parsedData.orParsedWaveform(it->end, ParsedData::byteBound);
                        // //Store parsed byte in data buffer
                        // bitIndex ^= bitIndex;
                        // data.append(bit);
                        // waveformData.append(*it);
                        // parity ^= bit;
                        // bit ^= bit;
                    }
                }
                else {
                    currentState = END_OF_DATA;
                    if (!data.empty()) {
                        parity ^= data.last(); //Removing parity byte from overal parity check sum
                        //Storing parsed data
                        parsedData.storeData(std::move(data), std::move(data_mapping), parsed.at(dataStart).begin, parsed.at(std::distance(parsed.begin(), it)).end, std::move(waveformData), parity);
                        parity ^= parity; //Zeroing parity byte
                    }
                }
                it = std::next(it);
            }
            else if (!data.empty()) {
                parity ^= data.last(); //Remove parity byte from overal parity check sum
                //Storing parsed data
                parsedData.storeData(std::move(data), std::move(data_mapping), parsed.at(dataStart).begin, parsed.at(parsed.size() - 1).end, std::move(waveformData), parity);
                parity ^= parity; //Zeroing parity byte
            }
            break;

        case END_OF_DATA:
            currentState = SEARCH_OF_PILOT_TONE;
            break;

        default:
            break;
        }

        if (it == parsed.end()) {
            currentState = NO_MORE_DATA;
        }
    }

    if (chNum == 0) {
        emit parsedChannel0Changed();
    }
    else {
        emit parsedChannel1Changed();
    }
}

void WaveformParser::saveTap(uint chNum, const QString& fileName)
{
    auto parsedDataPtr { getParsedDataPtr(chNum) };
    if (parsedDataPtr == nullptr) {
        return;
    }

    QFile f(fileName.isEmpty() ? QString("tape_%1_%2.tap").arg(QDateTime::currentDateTime().toString("dd.MM.yyyy hh-mm-ss.zzz")).arg(chNum ? "R" : "L") : fileName);
    f.remove(); //Remove file if exists
    f.open(QIODevice::WriteOnly);

    auto parsedDataSPtr { parsedDataPtr->getParsedData() };
    auto& parsedData = *parsedDataSPtr.data();
    for (auto i = 0; i < parsedData.size(); ++i) {
        if (i < mSelectedBlocks.size() && !mSelectedBlocks[i]) {
            continue;
        }

        QByteArray b;
        const auto& data { parsedData.at(i).data };
        const uint16_t size = data.size();
        b.append(reinterpret_cast<const char *>(&size), sizeof(size));
        b.append(reinterpret_cast<const char *>(data.data()), size);
        f.write(b);
    }

    f.close();
}

QVector<uint8_t> WaveformParser::getParsedWaveform(uint chNum) const {
    auto parsedDataPtr { getParsedDataPtr(chNum) };
    if (parsedDataPtr == nullptr) {
        return {};
    }
    return *parsedDataPtr->getParsedWaveform();// mParsedWaveform[chNum];
}

QPair<QVector<ParsedData::DataBlock>, QVector<bool>> WaveformParser::getParsedData(uint chNum) const {
    auto parsedDataPtr { getParsedDataPtr(chNum) };
    if (parsedDataPtr == nullptr) {
        return {};
    }
    return { *parsedDataPtr->getParsedData(), mSelectedBlocks };
}

void WaveformParser::toggleBlockSelection(int blockNum) {
    if (blockNum < mSelectedBlocks.size()) {
        auto& blk = mSelectedBlocks[blockNum];
        blk = !blk;

        emit parsedChannel0Changed();
        emit parsedChannel1Changed();
    }
}

int WaveformParser::getBlockDataStart(uint chNum, uint blockNum) const
{
    auto parsedDataPtr { getParsedDataPtr(chNum) };
    if (parsedDataPtr == nullptr) {
        return 0;
    }
    auto parsedDataSPtr { parsedDataPtr->getParsedData() };
    auto& parsedData { *parsedDataSPtr };

    if (blockNum < (unsigned) parsedData.size()) {
        return parsedData[blockNum].dataStart;
    }
    return 0;
}

int WaveformParser::getBlockDataEnd(uint chNum, uint blockNum) const
{
    auto parsedDataPtr { getParsedDataPtr(chNum) };
    if (parsedDataPtr == nullptr) {
        return 0;
    }
    auto parsedDataSPtr { parsedDataPtr->getParsedData() };
    auto& parsedData { *parsedDataSPtr };

    if (blockNum < (unsigned) parsedData.size()) {
        return parsedData[blockNum].dataEnd;
    }
    return 0;
}

int WaveformParser::getPositionByAddress(uint chNum, uint blockNum, uint addr) const
{
    auto parsedDataPtr { getParsedDataPtr(chNum) };
    if (parsedDataPtr == nullptr) {
        return 0;
    }
    auto parsedDataSPtr { parsedDataPtr->getParsedData() };
    auto& parsedData { *parsedDataSPtr };

    if (blockNum < (unsigned) parsedData.size() && addr < (unsigned) parsedData[blockNum].waveformData.size()) {
        return parsedData[blockNum].waveformData[addr].begin;
    }
    return 0;
}

QVariantList WaveformParser::getParsedChannelData(uint chNum) const
{
    auto parsedDataPtr { getParsedDataPtr(chNum) };
    if (parsedDataPtr == nullptr) {
        return {};
    }
    auto parsedDataSPtr { parsedDataPtr->getParsedData() };
    auto& parsedData { *parsedDataSPtr };

    static QMap<int, QString> blockTypes {
        {0x00, "Program"},
        {0x01, "Number Array"},
        {0x02, "Character Array"},
        {0x03, "Bytes"}
    };
    const QString id_header { qtTrId(ID_HEADER) };
    const QString id_code { qtTrId(ID_CODE) };
    const QString id_ok { qtTrId(ID_OK) };
    const QString id_error { qtTrId(ID_ERROR) };
    const QString id_unknown { qtTrId(ID_UNKNOWN) };

    QVariantList r;
    uint blockNumber = 0;

    for (const auto& i: parsedData) {
        QVariantMap m;

        m.insert("block", QVariantMap { {"blockSelected", blockNumber < (unsigned) mSelectedBlocks.size() ? mSelectedBlocks[blockNumber] : (mSelectedBlocks.append(true), true)}, {"blockNumber", blockNumber++} });
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
                blockTypeName = d == 0x00 ? id_header : id_code;
            }
            m.insert("blockType", blockTypeName);
            QString sizeText = QString::number(i.data.size());
            if (i.data.size() > 13 && btIt != blockTypes.end()) {
                sizeText += QString(" (%1)").arg(i.data.at(13) * 256 + i.data.at(12));
            }
            m.insert("blockSize", sizeText);
            QString nameText;
            if (blockType >= 0) {
                const auto loopRange { std::min(decltype(i.data.size())(12), i.data.size()) };
                nameText = QByteArray((const char*) &i.data.data()[2], loopRange > 1 ? loopRange - 2 : 0);
            }
            m.insert("blockName", nameText);
            m.insert("blockStatus", (i.state == ParsedData::OK ? id_ok : id_error) + qtTrId(ID_PARITY_MESSAGE).arg(QString::number(i.parityCalculated, 16).toUpper().rightJustified(2, '0')).arg(QString::number(i.parityAwaited, 16).toUpper().rightJustified(2, '0')));
            m.insert("state", i.state);
        }
        else {
            m.insert("blockType", id_unknown);
            m.insert("blockName", QString());
            m.insert("blockSize", 0);
            m.insert("blockStatus", id_unknown);
        }
        r.append(m);
    }

    return r;
}

QVariantList WaveformParser::getParsedChannel0() const
{
    return getParsedChannelData(0);
}

QVariantList WaveformParser::getParsedChannel1() const
{
    return getParsedChannelData(1);
}

WaveformParser* WaveformParser::instance()
{
    static QScopedPointer<WaveformParser> p { new WaveformParser() };
    return p.get();
}
