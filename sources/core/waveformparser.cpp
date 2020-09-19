//*******************************************************************************
// ZX Tape Reviver
//-----------------
//
// Author: Leonid Golouz
// E-mail: lgolouz@list.ru
//*******************************************************************************

#include "waveformparser.h"
#include <QDebug>
#include <QDateTime>
#include <QByteArray>

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
    QVector<WaveformPart> parsed = parseChannel<float>(channel);

    const double sampleRate = mWavReader.getSampleRate();
    auto isFreqFitsInDelta = [&sampleRate](uint32_t length, uint32_t signalFreq, double signalDelta, double deltaDivider = 1.0) -> bool {
        const double freq = (sampleRate / length);
        const double delta = signalFreq * (signalDelta / deltaDivider);
        return freq >= (signalFreq - delta) && freq <= (signalFreq + delta);
    };

    auto& parsedData = mParsedData[chNum];
    parsedData.clear();

    auto& parsedWaveform = mParsedWaveform[chNum];
    parsedWaveform.resize(channel.size());

    auto currentState = SEARCH_OF_PILOT_TONE;
    auto it = parsed.begin();
    QVector<uint8_t> data;
    //WaveformSign signalDirection = POSITIVE;
    uint32_t dataStart = 0;
    int8_t bitIndex = 7;
    uint8_t bit = 0;
    uint8_t parity = 0;

    while (currentState != NO_MORE_DATA) {
        auto prevIt = it;
        switch (currentState) {
        case SEARCH_OF_PILOT_TONE:
            it = std::find_if(it, parsed.end(), [&isFreqFitsInDelta, chNum, this](const WaveformPart& p) {
                fillParsedWaveform(chNum, p, 0);
                return isFreqFitsInDelta(p.length, SignalFrequencies::PILOT_HALF_FREQ, pilotDelta, 2.0);
            });
            if (it != parsed.end()) {
                currentState = PILOT_TONE;
            }
            break;

        case PILOT_TONE:
            it = std::find_if(it, parsed.end(), [&isFreqFitsInDelta, chNum, this](const WaveformPart& p) {
                fillParsedWaveform(chNum, p, pilotTone ^ sequenceMiddle);
                return isFreqFitsInDelta(p.length, SignalFrequencies::SYNCHRO_FIRST_HALF_FREQ, synchroDelta);
            });
            if (it != parsed.end()) {
                auto eIt = std::prev(it);
                fillParsedWaveform(chNum, *eIt, pilotTone ^ sequenceMiddle);
                parsedWaveform[prevIt->begin] = pilotTone ^ sequenceBegin;
                parsedWaveform[eIt->end] = pilotTone ^ sequenceEnd;

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
                if ((isFreqFitsInDelta(it->length, SignalFrequencies::SYNCHRO_SECOND_HALF, synchroDelta)) && (isFreqFitsInDelta(it->length + prevIt->length, SignalFrequencies::SYNCHRO_FREQ, synchroDelta, 2.0))) {
                    fillParsedWaveform(chNum, *prevIt, synchroSignal ^ sequenceMiddle);
                    fillParsedWaveform(chNum, *it, synchroSignal ^ sequenceMiddle);
                    parsedWaveform[prevIt->begin] = synchroSignal ^ sequenceBegin;
                    parsedWaveform[it->end] = synchroSignal ^ sequenceEnd;

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
                    bitIndex = 7;
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
                if (isFreqFitsInDelta(len, SignalFrequencies::ZERO_FREQ, zeroDelta)) { //ZERO
                    fillParsedWaveform(chNum, *prevIt, zeroBit ^ sequenceMiddle);
                    fillParsedWaveform(chNum, *it, zeroBit ^ sequenceMiddle);
                    parsedWaveform[prevIt->begin] = zeroBit ^ sequenceBegin;
                    parsedWaveform[it->end] = zeroBit ^ sequenceEnd;

                    if (bitIndex == 7) {
                        parsedWaveform[prevIt->begin] ^= byteBound;
                    }

//                    WaveformData wd;
//                    wd.begin = std::distance(parsed.begin(), prevIt);
//                    wd.end = std::distance(parsed.begin(), it);
//                    wd.waveBegin = prevIt->begin;
//                    wd.waveEnd = it->end;
//                    wd.value = SignalValue::ZERO;

//                    mParsedWaveform.insert(wd.waveBegin, wd);
                    --bitIndex;
                    if (bitIndex < 0) {
                        parsedWaveform[it->end] ^= byteBound;
                        bitIndex = 7;
                        data.append(bit);
                        parity ^= bit;
                        bit ^= bit;
                    }
                }
                else if (isFreqFitsInDelta(len, SignalFrequencies::ONE_FREQ, oneDelta)) { //ONE
                    fillParsedWaveform(chNum, *prevIt, oneBit ^ sequenceMiddle);
                    fillParsedWaveform(chNum, *it, oneBit ^ sequenceMiddle);
                    parsedWaveform[prevIt->begin] = oneBit ^ sequenceBegin;
                    parsedWaveform[it->end] = oneBit ^ sequenceEnd;

//                    WaveformData wd;
//                    wd.begin = std::distance(parsed.begin(), prevIt);
//                    wd.end = std::distance(parsed.begin(), it);
//                    wd.waveBegin = prevIt->begin;
//                    wd.waveEnd = it->end;
//                    wd.value = SignalValue::ONE;

//                    mParsedWaveform.insert(wd.waveBegin, wd);
                    bit |= 1 << bitIndex--;
                    if (bitIndex < 0) {
                        parsedWaveform[it->end] ^= byteBound;
                        bitIndex = 7;
                        data.append(bit);
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
                        parity ^= data.last(); //Removing parity byte from overal parity check sum
                        db.state = parity == data.last() ? DataState::OK : DataState::R_TAPE_LOADING_ERROR; //Should be checked with checksum
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
                parity ^= data.last(); //Remove parity byte from overal parity check sum
                db.state = parity == data.last() ? DataState::OK : DataState::R_TAPE_LOADING_ERROR; //Should be checked with checksum
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
    f.open(QIODevice::ReadWrite);

    auto& parsedData = mParsedData[chNum];
    for (auto i = 0; i < parsedData.size(); ++i) {
        QByteArray b;
        const uint16_t size = parsedData.at(i).data.size();
        b.append(reinterpret_cast<const char *>(&size), sizeof(size));
        for (uint8_t c: parsedData.at(i).data) {
            b.append(c);
        }
        f.write(b);
    }

    f.close();
}

void WaveformParser::fillParsedWaveform(uint chNum, const WaveformPart& p, uint8_t val)
{
    auto& parsedWaveform = mParsedWaveform[chNum];
    for (auto i = p.begin; i < p.end; ++i) {
        parsedWaveform[i] = val;
    }
}

QVector<uint8_t> WaveformParser::getParsedWaveform(uint chNum) const
{
    return mParsedWaveform[chNum];
}

int WaveformParser::getBlockDataStart(uint chNum, uint blockNum) const
{
    if (chNum < mParsedData.size() && blockNum < mParsedData[chNum].size()) {
        return mParsedData[chNum][blockNum].dataStart;
    }
    return 0;
}

int WaveformParser::getBlockDataEnd(uint chNum, uint blockNum) const
{
    if (chNum < mParsedData.size() && blockNum < mParsedData[chNum].size()) {
        return mParsedData[chNum][blockNum].dataEnd;
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

    QVariantList r;
    const auto& ch = mParsedData[chNum];
    uint blockNumber = 1;

    for (const auto& i: ch) {
        QVariantMap m;

        m.insert("blockNumber", blockNumber++);
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
                blockTypeName = d == 0x00 ? "Header" : "Code";
            }
            m.insert("blockType", blockTypeName);
            QString sizeText = QString::number(i.data.size());
            if (i.data.size() > 13 && btIt != blockTypes.end()) {
                sizeText += QString(" (%1)").arg(i.data.at(13) * 256 + i.data.at(12));
            }
            m.insert("blockSize", sizeText);
            QString nameText;
            if (blockType >= 0) {
                for (auto idx = 2; idx < std::min(12, i.data.size()); ++idx) {
                    nameText += QChar(i.data.at(idx));
                }
            }
            m.insert("blockName", nameText);
            m.insert("blockStatus", i.state == OK ? "Ok" : "Error");
            m.insert("state", i.state);
        }
        else {
            m.insert("blockType", "Unknown");
            m.insert("blockName", QString());
            m.insert("blockSize", 0);
            m.insert("blockStatus", "Unknown");
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
    static WaveformParser p;
    return &p;
}
