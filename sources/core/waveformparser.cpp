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
    WavReader::QVectorBase& channel = *(chNum == 0 ? mWavReader.getChannel0() : mWavReader.getChannel1());
    const auto bytesPerSample = mWavReader.getBytesPerSample();

    QVector<WaveformPart> parsed = bytesPerSample == 1 ? parseChannel<uint8_t>(static_cast<WavReader::QWavVector<uint8_t>&>(channel)) : parseChannel<int16_t>(static_cast<WavReader::QWavVector<int16_t>&>(channel));

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
                        db.dataStart = dataStart;
                        db.dataEnd = std::distance(parsed.begin(), it);
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
                db.dataStart = dataStart;
                db.dataEnd = parsed.size() - 1;
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

    bit = 0;
}

void WaveformParser::saveTap(uint chNum)
{
    QFile f(QString("tape_%1_%2.tap").arg(QDateTime::currentDateTime().toString("dd.MM.yyyy hh-mm-ss.zzz")).arg(chNum ? "R" : "L"));
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

QVariantList WaveformParser::getParsedChannel0() const
{
//    QVariantList r;
//    const auto& ch = mParsedData[0];
//    for (const auto& i: ch) {
//        QVariantMap m;

//        m.insert("blockType", i.data.size() > 0 ? i.data.at(1) : 0xff);
//        m.insert("blockName", i.)
//    }

    return { };
}

QVariantList WaveformParser::getParsedChannel1() const
{
    return  { };
}

WaveformParser* WaveformParser::instance()
{
    static WaveformParser p;
    return &p;
}
