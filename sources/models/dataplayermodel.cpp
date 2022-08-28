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
#include <QDebug>

DataPlayerModel::DataPlayerModel(QObject* parent) :
    QObject(parent),
    m_playingState(DP_Stopped),
    m_blockTime(0),
    m_processedTime(0)
{
    connect(&m_delayTimer, &QTimer::timeout, this, &DataPlayerModel::handleNextDataRecord);
}

void DataPlayerModel::playParsedData(uint chNum, uint currentBlock) {
    if (m_playingState != DP_Stopped) {
        return;
    }

#if (Q_BYTE_ORDER == Q_BIG_ENDIAN)
    const auto endianness { QAudioFormat::BigEndian };
#else
    const auto endianness { QAudioFormat::LittleEndian };
#endif

    QAudioFormat format;
    // Set up the format
    format.setSampleRate(c_sampleRate);
    format.setChannelCount(1);
    format.setSampleSize(16);
    format.setCodec("audio/pcm");
    format.setByteOrder(endianness);
    format.setSampleType(QAudioFormat::SignedInt);

    const QAudioDeviceInfo info(QAudioDeviceInfo::defaultOutputDevice());
    if (!info.isFormatSupported(format)) {
        qDebug() << "Audio format not supported, cannot play audio.";
        return;
    }

    m_audio.reset(new QAudioOutput(info, format));
    m_audio->setNotifyInterval(30);
    connect(m_audio.data(), &QAudioOutput::stateChanged, this, &DataPlayerModel::handleAudioOutputStateChanged);
    connect(m_audio.data(), &QAudioOutput::notify, this, &DataPlayerModel::handleAudioOutputNotify);

    m_buffer.close();
    m_currentBlock = currentBlock;
    m_data = WaveformParser::instance()->getParsedData(chNum);
    m_parserData = chNum == 0 ? WaveformParser::instance()->getParsedChannel0() : WaveformParser::instance()->getParsedChannel1();
    handleNextDataRecord();
}

void DataPlayerModel::handleNextDataRecord() {
    if (m_currentBlock >= (unsigned) m_data.first.size()) {
        return;
    }

    QByteArray array;

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
            for (size_t c { 0 }; c < wavlen; ++c) {
                array.append((char *)&val, sizeof(int16_t));
            }
        }
    }
    //Synchro
    for (auto w { 0 }; w <= 1; ++w) {
        wavlen = c_sampleRate / (w ? synchroSecondHalf : synchroFirstHalf);
        const int16_t val { int16_t(32760 * (w ? 1 : -1)) };
        for (size_t c { 0 }; c < wavlen; ++c) {
            array.append((char *)&val, sizeof(int16_t));
        }
    }
    //Data
    for (const uint8_t byte: qAsConst(m_data.first[m_currentBlock].data)) {
        for (int i { 7 }; i >= 0; --i) {
            const uint8_t bit8 = 1 << i;
            const auto bit { byte & bit8 };
            wavlen = c_sampleRate / (bit == 0 ? zeroHalfFreq : oneHalfFreq);
            for (auto b { 0 }; b <= 1; ++b) {
                const int16_t val { int16_t(32760 * (b ? 1 : -1)) };
                for (size_t c { 0 }; c < wavlen; ++c) {
                    array.append((char *)&val, sizeof(int16_t));
                }
            }
        }
    }

    emit currentBlockChanged();
    m_blockTime = (array.size() / sizeof(int16_t)) / (c_sampleRate / 1000);
    m_processedTime = 0;
    emit blockTimeChanged();
    emit processedTimeChanged();
    ++m_currentBlock;

    m_buffer.setData(array);
    m_buffer.open(QIODevice::ReadOnly);
    m_audio->start(&m_buffer);
}

void DataPlayerModel::prepareNextDataRecord() {
    const QVector<WaveformParser::DataBlock>& blockData { m_data.first };
    const QVector<bool>& selectionData { m_data.second };
    while (m_currentBlock < (unsigned) blockData.size()) {
        if ((unsigned) selectionData.size() < m_currentBlock || selectionData.at(m_currentBlock)) {
            break;
        }
        ++m_currentBlock;
    }

    m_buffer.close();
    if (m_currentBlock < (unsigned) blockData.size()) {
        //half a second delay
        m_delayTimer.singleShot(500, this, [this]() {
            //We have to check for state == playing to be sure stop method is not executed previously.
            if (m_playingState == DP_Playing) {
                handleNextDataRecord();
            }
        });
    } else {
        m_audio->stop();
        m_audio.reset();
        m_playingState = DP_Stopped;
        emit currentBlockChanged();
        emit stoppedChanged();
    }
}

void DataPlayerModel::handleAudioOutputStateChanged(QAudio::State state) {
    switch (state) {
        case QAudio::IdleState:
            prepareNextDataRecord();
            break;

        case QAudio::StoppedState:
            if (m_audio->error() != QAudio::NoError) {
                qDebug() << "Error playing: " << m_audio->error();
            }
            break;

        case QAudio::ActiveState:
            m_playingState = DP_Playing;
            emit stoppedChanged();
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

void DataPlayerModel::handleAudioOutputNotify() {
    m_processedTime = m_audio->processedUSecs() / 1000;
    emit processedTimeChanged();
}

bool DataPlayerModel::getStopped() const {
    return m_playingState == DP_Stopped;
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
    return cb < 0 ? QVariant() : m_parserData.at(cb);
}

DataPlayerModel::~DataPlayerModel() {
    m_audio.reset();
    qDebug() << "~DataPlayerModel";
}

DataPlayerModel* DataPlayerModel::instance() {
    static DataPlayerModel m;
    return &m;
}
