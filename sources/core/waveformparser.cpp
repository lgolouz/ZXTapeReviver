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

    mParsedData.clear();
    mParsedWaveform.resize(channel.size());

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
            it = std::find_if(it, parsed.end(), [&isFreqFitsInDelta, this](const WaveformPart& p) {
                fillParsedWaveform(p, 0);
                return isFreqFitsInDelta(p.length, SignalFrequencies::PILOT_HALF_FREQ, pilotDelta, 2.0);
            });
            if (it != parsed.end()) {
                currentState = PILOT_TONE;
            }
            break;

        case PILOT_TONE:
            it = std::find_if(it, parsed.end(), [&isFreqFitsInDelta, this](const WaveformPart& p) {
                fillParsedWaveform(p, pilotTone ^ sequenceMiddle);
                return isFreqFitsInDelta(p.length, SignalFrequencies::SYNCHRO_FIRST_HALF_FREQ, synchroDelta);
            });
            if (it != parsed.end()) {
                auto eIt = std::prev(it);
                fillParsedWaveform(*eIt, pilotTone ^ sequenceMiddle);
                mParsedWaveform[prevIt->begin] = pilotTone ^ sequenceBegin;
                mParsedWaveform[eIt->end] = pilotTone ^ sequenceEnd;

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
                    fillParsedWaveform(*prevIt, synchroSignal ^ sequenceMiddle);
                    fillParsedWaveform(*it, synchroSignal ^ sequenceMiddle);
                    mParsedWaveform[prevIt->begin] = synchroSignal ^ sequenceBegin;
                    mParsedWaveform[it->end] = synchroSignal ^ sequenceEnd;

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
                    fillParsedWaveform(*prevIt, zeroBit ^ sequenceMiddle);
                    fillParsedWaveform(*it, zeroBit ^ sequenceMiddle);
                    mParsedWaveform[prevIt->begin] = zeroBit ^ sequenceBegin;
                    mParsedWaveform[it->end] = zeroBit ^ sequenceEnd;

                    if (bitIndex == 7) {
                        mParsedWaveform[prevIt->begin] ^= byteBound;
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
                        mParsedWaveform[it->end] ^= byteBound;
                        bitIndex = 7;
                        data.append(bit);
                        parity ^= bit;
                        bit ^= bit;
                    }
                }
                else if (isFreqFitsInDelta(len, SignalFrequencies::ONE_FREQ, oneDelta)) { //ONE
                    fillParsedWaveform(*prevIt, oneBit ^ sequenceMiddle);
                    fillParsedWaveform(*it, oneBit ^ sequenceMiddle);
                    mParsedWaveform[prevIt->begin] = oneBit ^ sequenceBegin;
                    mParsedWaveform[it->end] = oneBit ^ sequenceEnd;

//                    WaveformData wd;
//                    wd.begin = std::distance(parsed.begin(), prevIt);
//                    wd.end = std::distance(parsed.begin(), it);
//                    wd.waveBegin = prevIt->begin;
//                    wd.waveEnd = it->end;
//                    wd.value = SignalValue::ONE;

//                    mParsedWaveform.insert(wd.waveBegin, wd);
                    bit |= 1 << bitIndex--;
                    if (bitIndex < 0) {
                        mParsedWaveform[it->end] ^= byteBound;
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

                        mParsedData.append(db);
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

                mParsedData.append(db);
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

void WaveformParser::saveTap()
{
    QFile f(QString("tape_%1.tap").arg(QDateTime::currentDateTime().toString("dd.MM.yyyy hh-mm-ss.zzz")));
    f.open(QIODevice::ReadWrite);

    for (auto i = 0; i < mParsedData.size(); ++i) {
        QByteArray b;
        const uint16_t size = mParsedData.at(i).data.size();
        b.append(reinterpret_cast<const char *>(&size), sizeof(size));
        for (uint8_t c: mParsedData.at(i).data) {
            b.append(c);
        }
        f.write(b);
    }

    f.close();
}

void WaveformParser::fillParsedWaveform(const WaveformPart& p, uint8_t val)
{
    for (auto i = p.begin; i < p.end; ++i) {
        mParsedWaveform[i] = val;
    }
}

const QVector<uint8_t>& WaveformParser::getParsedWaveform() const
{
    return mParsedWaveform;
}

WaveformParser* WaveformParser::instance()
{
    static WaveformParser p;
    return &p;
}
