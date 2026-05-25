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

#include "dataplayermodel.h"
#include <QAudioFormat>
#include <QAudioDevice>
#include <QMediaDevices>
#include <QDebug>
#include <QVariantMap>
#include <algorithm>

namespace {
constexpr const char* c_borderIdleColor { "#ffffff" };
constexpr const char* c_borderPilotLowColor { "#00d7d7" };
constexpr const char* c_borderPilotHighColor { "#d70000" };
constexpr const char* c_borderSyncLowColor { "#0000d7" };
constexpr const char* c_borderSyncHighColor { "#d7d700" };
constexpr const char* c_borderZeroLowColor { "#0000d7" };
constexpr const char* c_borderZeroHighColor { "#d7d700" };
constexpr const char* c_borderOneLowColor { "#0000d7" };
constexpr const char* c_borderOneHighColor { "#d7d700" };

int borderStripeHeightFromSamples(int samples, int sampleRate) {
    constexpr int c_spectrumLinesPerSecond { 50 * 312 };
    constexpr int c_roundingDivider { 2 };
    const int screenLines {
        (samples * c_spectrumLinesPerSecond + sampleRate / c_roundingDivider) / sampleRate
    };
    return std::clamp(screenLines, 2, 12);
}
}

DataPlayerModel::DataPlayerModel(QObject* parent) :
    QObject(parent),
    m_playingState(DP_Stopped),
    m_parserData(nullptr),
    m_blockTime(0),
    m_processedTime(0),
    m_blockStartTime(0)
{
    m_notifyTimer.setInterval(30);
    connect(&m_delayTimer, &QTimer::timeout, this, &DataPlayerModel::handleNextDataRecord);
    connect(&m_notifyTimer, &QTimer::timeout, this, &DataPlayerModel::handleAudioOutputNotify);
}

void DataPlayerModel::playParsedData(uint chNum, uint currentBlock) {
    if (m_playingState != DP_Stopped) {
        return;
    }

    QAudioFormat format;
    format.setSampleRate(c_sampleRate);
    format.setChannelCount(1);
    format.setSampleFormat(QAudioFormat::Int16);

    const QAudioDevice info(QMediaDevices::defaultAudioOutput());
    if (info.isNull()) {
        qDebug() << "No audio output device available, cannot play audio.";
        return;
    }

    if (!info.isFormatSupported(format)) {
        qDebug() << "Audio format not supported, cannot play audio:" << format;
        return;
    }

    m_audio.reset(new QAudioSink(info, format));
    connect(m_audio.data(), &QAudioSink::stateChanged, this, &DataPlayerModel::handleAudioOutputStateChanged);

    m_buffer.close();
    m_currentBlock = currentBlock;
    m_data = WaveformParser::instance()->getParsedData(chNum);
    m_parserData = chNum == 0 ? WaveformParser::instance()->getParsedChannel0() : WaveformParser::instance()->getParsedChannel1();
    if (m_currentBlock >= (unsigned) m_data.first.size()) {
        m_audio.reset();
        return;
    }

    m_playingState = DP_Playing;
    emit stoppedChanged();
    emit pausedChanged();
    m_notifyTimer.start();
    handleNextDataRecord();
}

void DataPlayerModel::handleNextDataRecord() {
    if (m_currentBlock >= (unsigned) m_data.first.size()) {
        return;
    }

    QByteArray array;
    QVector<QString> borderTimeline;
    QVector<qsizetype> borderPulseSamples;
    QVector<int> borderPulseLengths;
    qsizetype generatedSamples { 0 };

    const auto appendSamples = [&array, &borderTimeline, &borderPulseSamples, &borderPulseLengths, &generatedSamples](int16_t val, size_t sampleCount, const QString& borderColor) {
        borderTimeline.append(borderColor);
        borderPulseSamples.append(generatedSamples);
        borderPulseLengths.append(static_cast<int>(sampleCount));
        for (size_t c { 0 }; c < sampleCount; ++c) {
            array.append((char *)&val, sizeof(int16_t));
        }
        generatedSamples += static_cast<qsizetype>(sampleCount);
    };

    const auto c_pilotHalfFreq { SignalFrequencies::PILOT_HALF_FREQ };
    const auto c_synchroFirstHalfFreq { SignalFrequencies::SYNCHRO_FIRST_HALF_FREQ };
    const auto c_synchroSecondHalfFreq { SignalFrequencies::SYNCHRO_SECOND_HALF_FREQ };
    const auto c_zeroHalfFreq { SignalFrequencies::ZERO_HALF_FREQ };
    const auto c_oneHalfFreq { SignalFrequencies::ONE_HALF_FREQ };

    const auto oneFreq { c_oneHalfFreq / 2 };
    const auto oneHalfFreq { oneFreq * 2 };
    const auto zeroFreq { c_zeroHalfFreq / 2 };
    const auto zeroHalfFreq { zeroFreq * 2 };
    const auto synchroFirstHalf { c_synchroFirstHalfFreq };
    const auto synchroSecondHalf { c_synchroSecondHalfFreq };
    const auto pilotFreq { c_pilotHalfFreq / 2 };
    const auto pilotHalfFreq { pilotFreq * 2 };
    const auto pilotLen { 3 };

    //Pilot
    size_t wavlen = c_sampleRate / pilotHalfFreq;
    auto threshold { c_sampleRate * pilotLen / (wavlen * 2) };
    for (size_t i { 0 }; i < threshold; ++i) {
        for (auto p { 0 }; p <= 1; ++p) {
            const int16_t val { int16_t(32760 * (p ? 1 : -1)) };
            appendSamples(val, wavlen, p ? c_borderPilotHighColor : c_borderPilotLowColor);
        }
    }
    //Synchro
    for (auto w { 0 }; w <= 1; ++w) {
        wavlen = c_sampleRate / (w ? synchroSecondHalf : synchroFirstHalf);
        const int16_t val { int16_t(32760 * (w ? 1 : -1)) };
        appendSamples(val, wavlen, w ? c_borderSyncHighColor : c_borderSyncLowColor);
    }
    //Data
    const auto dataBlock { m_data.first.at(m_currentBlock) };
    if (dataBlock.isNull()) {
        prepareNextDataRecord();
        return;
    }

    for (const uint8_t byte: std::as_const(dataBlock->data)) {
        for (int i { 7 }; i >= 0; --i) {
            const uint8_t bit8 = 1 << i;
            const auto bit { byte & bit8 };
            wavlen = c_sampleRate / (bit == 0 ? zeroHalfFreq : oneHalfFreq);
            for (auto b { 0 }; b <= 1; ++b) {
                const int16_t val { int16_t(32760 * (b ? 1 : -1)) };
                appendSamples(val, wavlen, bit == 0
                        ? (b ? c_borderZeroHighColor : c_borderZeroLowColor)
                        : (b ? c_borderOneHighColor : c_borderOneLowColor));
            }
        }
    }
    //Silence for 1 us (prevents R Tape loading error under Linux, ZXTR-48)
    wavlen = c_sampleRate / 1000;
    int16_t val { 0 };
    appendSamples(val, wavlen, c_borderIdleColor);

    m_blockTime = (array.size() / sizeof(int16_t)) / (c_sampleRate / 1000);
    m_processedTime = 0;
    m_romLoaderBorderTimeline = borderTimeline;
    m_romLoaderBorderPulseSamples = borderPulseSamples;
    m_romLoaderBorderPulseLengths = borderPulseLengths;
    emit blockTimeChanged();
    emit processedTimeChanged();
    emit borderTimelineChanged();
    emit currentBlockChanged();
    ++m_currentBlock;

    m_buffer.setData(array);
    m_buffer.open(QIODevice::ReadOnly);
    m_blockStartTime = m_audio ? m_audio->processedUSecs() / 1000 : 0;
    m_audio->start(&m_buffer);
}

void DataPlayerModel::prepareNextDataRecord() {
    const auto& blockData { m_data.first };
    const QVector<bool>& selectionData { m_data.second };
    while (m_currentBlock < (unsigned) blockData.size()) {
        if ((unsigned) selectionData.size() <= m_currentBlock || selectionData.at(m_currentBlock)) {
            break;
        }
        ++m_currentBlock;
    }

    m_buffer.close();
    if (m_currentBlock < (unsigned) blockData.size()) {
        m_romLoaderBorderTimeline.clear();
        m_romLoaderBorderPulseSamples.clear();
        m_romLoaderBorderPulseLengths.clear();
        emit borderTimelineChanged();

        //half a second delay
        m_delayTimer.singleShot(500, this, [this]() {
            //We have to check for state == playing to be sure stop method is not executed previously.
            if (m_playingState == DP_Playing) {
                handleNextDataRecord();
            }
        });
    } else {
        m_notifyTimer.stop();
        if (m_audio) {
            m_audio->stop();
        }
        m_audio.reset();
        m_playingState = DP_Stopped;
        m_romLoaderBorderTimeline.clear();
        m_romLoaderBorderPulseSamples.clear();
        m_romLoaderBorderPulseLengths.clear();
        emit currentBlockChanged();
        emit stoppedChanged();
        emit pausedChanged();
        emit borderTimelineChanged();
    }
}

void DataPlayerModel::handleAudioOutputStateChanged(QAudio::State state) {
    switch (state) {
        case QAudio::IdleState:
            if (m_playingState == DP_Playing) {
                prepareNextDataRecord();
            }
            break;

        case QAudio::StoppedState:
            if (m_audio && m_audio->error() != QAudio::NoError) {
                qDebug() << "Error playing: " << m_audio->error();
            }
            break;

        case QAudio::ActiveState:
            break;

        default:
            break;
    }
}

void DataPlayerModel::stop() {
    if (m_playingState != DP_Stopped) {
        m_currentBlock = m_data.first.size();
        prepareNextDataRecord();
    }
}

void DataPlayerModel::pause() {
    if (m_playingState == DP_Playing && m_audio) {
        m_audio->suspend();
        m_notifyTimer.stop();
        m_playingState = DP_Paused;
        emit pausedChanged();
    }
}

void DataPlayerModel::resume() {
    if (m_playingState == DP_Paused && m_audio) {
        m_audio->resume();
        m_notifyTimer.start();
        m_playingState = DP_Playing;
        emit pausedChanged();
    }
}

void DataPlayerModel::handleAudioOutputNotify() {
    if (m_audio) {
        m_processedTime = std::max<qint64>(0, m_audio->processedUSecs() / 1000 - m_blockStartTime);
    }
    emit processedTimeChanged();
}

bool DataPlayerModel::getStopped() const {
    return m_playingState == DP_Stopped;
}

bool DataPlayerModel::getPaused() const {
    return m_playingState == DP_Paused;
}

int DataPlayerModel::getCurrentBlock() const {
    return m_currentBlock < (unsigned) m_data.first.size() ? m_currentBlock : -1;
}

int DataPlayerModel::getBlockTime() const {
    return m_blockTime;
}

int DataPlayerModel::getProcessedTime() const {
    return m_processedTime;
}

QVariant DataPlayerModel::getBlockData() const {
    const auto cb { getCurrentBlock() };
    if (cb < 0 || m_parserData == nullptr) {
        return {};
    }

    return QVariantMap {
        { "block", QVariantMap { { "blockNumber", cb } } },
        { "blockType", m_parserData->data(m_parserData->index(cb, 1), Qt::DisplayRole) },
        { "blockName", m_parserData->data(m_parserData->index(cb, 2), Qt::DisplayRole) },
    };
}

QVariant DataPlayerModel::getRomLoaderBorderStripe(int timeMs, int stripeIndex) const {
    QVariantMap stripe {
        { "color", c_borderIdleColor },
        { "height", 4 },
    };

    if (timeMs < 0 || m_romLoaderBorderTimeline.empty() || m_romLoaderBorderPulseSamples.empty()
            || m_romLoaderBorderPulseLengths.empty()) {
        return stripe;
    }

    const qsizetype sample { static_cast<qsizetype>(timeMs) * c_sampleRate / 1000 };
    const auto firstAfterSample {
        std::upper_bound(m_romLoaderBorderPulseSamples.cbegin(), m_romLoaderBorderPulseSamples.cend(), sample)
    };
    if (firstAfterSample == m_romLoaderBorderPulseSamples.cbegin()) {
        return stripe;
    }

    const auto currentPulseIndex { static_cast<int>(std::distance(m_romLoaderBorderPulseSamples.cbegin(), firstAfterSample) - 1) };
    const int timelineSize { static_cast<int>(m_romLoaderBorderTimeline.size()) };
    const int index { (currentPulseIndex + stripeIndex) % timelineSize };

    const auto& color { m_romLoaderBorderTimeline.at(index) };
    stripe["color"] = color.isEmpty() ? QString(c_borderIdleColor) : color;
    stripe["height"] = borderStripeHeightFromSamples(m_romLoaderBorderPulseLengths.at(index), c_sampleRate);
    return stripe;
}

DataPlayerModel::~DataPlayerModel() {
    m_audio.reset();
    qDebug() << "~DataPlayerModel";
}

DataPlayerModel* DataPlayerModel::instance() {
    static DataPlayerModel m;
    return &m;
}
