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

WaveformParser::WaveformParser(QObject* parent) :
    QObject(parent),
    mWavReader(*WavReader::instance())
{

}

void WaveformParser::parse(uint chNum)
{
    if (chNum >= mWavReader.getNumberOfChannels()) {
        qDebug() << "Trying to parse channel that exceeds overall number of channels";
        return;
    }

    QWavVector& channel = *(chNum == 0 ? mWavReader.getChannel0() : mWavReader.getChannel1());
    QVector<WaveformPart> parsed = parseChannel<QWavVectorType>(channel);

    const double sampleRate = mWavReader.getSampleRate();
    auto& parsedData = mParsedData[chNum];
    parsedData.clear();

    auto& parsedWaveform = mParsedWaveform[chNum];
    parsedWaveform.resize(channel.size());

    auto currentState = SEARCH_OF_PILOT_TONE;
    auto it = parsed.begin();
    QVector<uint8_t> data;
    QVector<WaveformPart> waveformData;
    //WaveformSign signalDirection = POSITIVE;
    uint32_t dataStart = 0;
    uint8_t bitIndex = 0;
    uint8_t bit = 0;
    uint8_t parity = 0;

    const auto& parserSettings = ParserSettingsModel::instance()->getParserSettings();
    auto isSineNormal = [&parserSettings, sampleRate](const WaveformPart& b, const WaveformPart& e, bool zeroCheck) -> bool {
        if (parserSettings.checkForAbnormalSine) {
            return isFreqFitsInDelta(sampleRate, b.length, zeroCheck ? parserSettings.zeroHalfFreq : parserSettings.oneHalfFreq, zeroCheck ? parserSettings.zeroDelta : parserSettings.oneDelta, parserSettings.sineCheckTolerance) &&
                   isFreqFitsInDelta(sampleRate, e.length, zeroCheck ? parserSettings.zeroHalfFreq : parserSettings.oneHalfFreq, zeroCheck ? parserSettings.zeroDelta : parserSettings.oneDelta, parserSettings.sineCheckTolerance);
        }
        return true;
    };

    while (currentState != NO_MORE_DATA) {
        auto prevIt = it;
        switch (currentState) {
        case SEARCH_OF_PILOT_TONE:
            it = std::find_if(it, parsed.end(), [&parserSettings, sampleRate, chNum, this](const WaveformPart& p) {
                fillParsedWaveform(chNum, p, 0);
                return isFreqFitsInDelta(sampleRate, p.length, parserSettings.pilotHalfFreq, parserSettings.pilotDelta, 2.0);
            });
            if (it != parsed.end()) {
                currentState = PILOT_TONE;
            }
            break;

        case PILOT_TONE:
            it = std::find_if(it, parsed.end(), [&parserSettings, sampleRate, chNum, this](const WaveformPart& p) {
                fillParsedWaveform(chNum, p, pilotTone | sequenceMiddle);
                return isFreqFitsInDelta(sampleRate, p.length, parserSettings.synchroFirstHalfFreq, parserSettings.synchroDelta);
            });
            if (it != parsed.end()) {
                auto eIt = std::prev(it);
                fillParsedWaveform(chNum, *eIt, pilotTone | sequenceMiddle);
                parsedWaveform[prevIt->begin] = pilotTone | sequenceBegin;
                parsedWaveform[eIt->end] = pilotTone | sequenceEnd;

                currentState = SYNCHRO_SIGNAL;
//                WaveformData wd;
//                wd.begin = std::distance(parsed.begin(), prevIt);
//                wd.end = std::distance(parsed.begin(), std::prev(it));
//                wd.waveBegin = prevIt->begin;
//                wd.waveEnd = it->end;
//                wd.value = SYNCHRO;

//                mParsedWaveform.insert(wd.waveBegin, wd);
            }
            break;

        case SYNCHRO_SIGNAL:
            it = std::next(it);
            if (it != parsed.end()) {
                if ((isFreqFitsInDelta(sampleRate, it->length, parserSettings.synchroSecondHalfFreq, parserSettings.synchroDelta)) &&
                    (isFreqFitsInDelta(sampleRate, it->length + prevIt->length, parserSettings.synchroFreq, parserSettings.synchroDelta, 2.0))) {
                    fillParsedWaveform(chNum, *prevIt, synchroSignal | sequenceMiddle);
                    fillParsedWaveform(chNum, *it, synchroSignal | sequenceMiddle);
                    parsedWaveform[prevIt->begin] = synchroSignal | sequenceBegin;
                    parsedWaveform[it->end] = synchroSignal | sequenceEnd;

                    currentState = DATA_SIGNAL;
                    //signalDirection = prevIt->sign;

//                    WaveformData wd;
//                    wd.begin = std::distance(parsed.begin(), prevIt);
//                    wd.end = std::distance(parsed.begin(), it);
//                    wd.waveBegin = prevIt->begin;
//                    wd.waveEnd = it->end;
//                    wd.value = SYNCHRO;

//                    mParsedWaveform.insert(wd.waveBegin, wd);
                    it = std::next(it);
                    dataStart = std::distance(parsed.begin(), it) + 1;
                    data.clear();
                    waveformData.clear();
                    bitIndex ^= bitIndex;
                    bit ^= bit;
                }
                else {
                    currentState = SEARCH_OF_PILOT_TONE;
                }
            }
            break;

        case DATA_SIGNAL:
            it = std::next(it);
            if (it != parsed.end()) {
                const auto len = it->length + prevIt->length;
                if (isFreqFitsInDelta2(sampleRate, len, parserSettings.zeroFreq, parserSettings.zeroDelta, 0.75) && isSineNormal(*prevIt, *it, true)) { //ZERO
                    fillParsedWaveform(chNum, *prevIt, zeroBit | sequenceMiddle);
                    fillParsedWaveform(chNum, *it, zeroBit | sequenceMiddle);
                    parsedWaveform[prevIt->begin] = zeroBit | sequenceBegin;
                    parsedWaveform[it->end] = zeroBit | sequenceEnd;

                    if (bitIndex == 0) {
                        parsedWaveform[prevIt->begin] |= byteBound;
                    }

//                    WaveformData wd;
//                    wd.begin = std::distance(parsed.begin(), prevIt);
//                    wd.end = std::distance(parsed.begin(), it);
//                    wd.waveBegin = prevIt->begin;
//                    wd.waveEnd = it->end;
//                    wd.value = SignalValue::ZERO;

//                    mParsedWaveform.insert(wd.waveBegin, wd);
                    if (bitIndex++ == 7) {
                        parsedWaveform[it->end] |= byteBound;
                        bitIndex ^= bitIndex;
                        data.append(bit);
                        waveformData.append(*it);
                        parity ^= bit;
                        bit ^= bit;
                    }
                }
                else if (isFreqFitsInDelta2(sampleRate, len, parserSettings.oneFreq, 0.75, parserSettings.oneDelta) && isSineNormal(*prevIt, *it, false)) { //ONE
                    fillParsedWaveform(chNum, *prevIt, oneBit | sequenceMiddle);
                    fillParsedWaveform(chNum, *it, oneBit | sequenceMiddle);
                    parsedWaveform[prevIt->begin] = oneBit | sequenceBegin;
                    parsedWaveform[it->end] = oneBit | sequenceEnd;

//                    WaveformData wd;
//                    wd.begin = std::distance(parsed.begin(), prevIt);
//                    wd.end = std::distance(parsed.begin(), it);
//                    wd.waveBegin = prevIt->begin;
//                    wd.waveEnd = it->end;
//                    wd.value = SignalValue::ONE;

//                    mParsedWaveform.insert(wd.waveBegin, wd);
                    bit |= 1 << (7 - bitIndex);
                    if (bitIndex++ == 7) {
                        parsedWaveform[it->end] |= byteBound;
                        bitIndex ^= bitIndex;
                        data.append(bit);
                        waveformData.append(*it);
                        parity ^= bit;
                        bit ^= bit;
                    }
                }
                else {
                    currentState = END_OF_DATA;
                    if (!data.empty()) {
                        DataBlock db;
                        db.dataStart = parsed.at(dataStart).begin;
                        db.dataEnd = parsed.at(std::distance(parsed.begin(), it)).end;
                        db.data = data;
                        db.waveformData = waveformData;
                        parity ^= data.last(); //Removing parity byte from overal parity check sum
                        //Storing parity data
                        db.parityAwaited = data.last();
                        db.parityCalculated = parity;
                        db.state = parity == db.parityAwaited ? DataState::OK : DataState::R_TAPE_LOADING_ERROR; //Should be checked with checksum
                        parity ^= parity; //Zeroing parity byte
                        parsedData.append(db);
                    }
                }
                it = std::next(it);
            }
            else if (!data.empty()) {
                DataBlock db;
                db.dataStart = parsed.at(dataStart).begin;
                db.dataEnd = parsed.at(parsed.size() - 1).end;
                db.data = data;
                db.waveformData = waveformData;
                parity ^= data.last(); //Remove parity byte from overal parity check sum
                //Storing parity data
                db.parityAwaited = data.last();
                db.parityCalculated = parity;
                db.state = parity == db.parityAwaited ? DataState::OK : DataState::R_TAPE_LOADING_ERROR; //Should be checked with checksum
                parity ^= parity; //Zeroing parity byte

                parsedData.append(db);
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
    QFile f(fileName.isEmpty() ? QString("tape_%1_%2.tap").arg(QDateTime::currentDateTime().toString("dd.MM.yyyy hh-mm-ss.zzz")).arg(chNum ? "R" : "L") : fileName);
    f.remove(); //Remove file if exists
    f.open(QIODevice::WriteOnly);

    auto& parsedData = mParsedData[chNum];
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

void WaveformParser::fillParsedWaveform(uint chNum, const WaveformPart& p, uint8_t val)
{
    auto& parsedWaveform = mParsedWaveform[chNum];
    for (auto i = p.begin; i <= p.end; ++i) {
        parsedWaveform[i] = val;
    }
}

QVector<uint8_t> WaveformParser::getParsedWaveform(uint chNum) const {
    return mParsedWaveform[chNum];
}

QPair<QVector<WaveformParser::DataBlock>, QVector<bool>> WaveformParser::getParsedData(uint chNum) const {
    return { mParsedData[chNum], mSelectedBlocks };
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
    if (chNum < (unsigned) mParsedData.size() && blockNum < (unsigned) mParsedData[chNum].size()) {
        return mParsedData[chNum][blockNum].dataStart;
    }
    return 0;
}

int WaveformParser::getBlockDataEnd(uint chNum, uint blockNum) const
{
    if (chNum < (unsigned) mParsedData.size() && blockNum < (unsigned) mParsedData[chNum].size()) {
        return mParsedData[chNum][blockNum].dataEnd;
    }
    return 0;
}

int WaveformParser::getPositionByAddress(uint chNum, uint blockNum, uint addr) const
{
    if (chNum < (unsigned) mParsedData.size() && blockNum < (unsigned) mParsedData[chNum].size() && addr < (unsigned) mParsedData[chNum][blockNum].waveformData.size()) {
        return mParsedData[chNum][blockNum].waveformData.at(addr).end;
    }
    return 0;
}

QVariantList WaveformParser::getParsedChannelData(uint chNum) const
{
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
    const auto& ch = mParsedData[chNum];
    uint blockNumber = 0;

    for (const auto& i: ch) {
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
            m.insert("blockStatus", (i.state == OK ? id_ok : id_error) + qtTrId(ID_PARITY_MESSAGE).arg(QString::number(i.parityCalculated, 16).toUpper().rightJustified(2, '0')).arg(QString::number(i.parityAwaited, 16).toUpper().rightJustified(2, '0')));
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
