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

#include "waveformplayermodel.h"
#include "sources/core/wavreader.h"
#include "sources/models/waveformmodel.h"
#include <QAudioDevice>
#include <QAudioFormat>
#include <QDebug>
#include <QMediaDevices>
#include <algorithm>
#include <cmath>
#include <limits>

namespace {
class WaveformAudioDevice : public QIODevice
{
    QSharedPointer<QWavVector> m_channel;
    qsizetype m_currentSample;
    bool m_normalizedFloat;

    static int16_t toAudioSample(QWavVectorType sample, bool normalizedFloat)
    {
        const float scaledSample { normalizedFloat ? sample * 32767.0f : sample };
        return static_cast<int16_t>(std::clamp(std::lround(scaledSample),
                                               static_cast<long>(std::numeric_limits<int16_t>::min()),
                                               static_cast<long>(std::numeric_limits<int16_t>::max())));
    }

    bool detectNormalizedFloat() const
    {
        if (m_channel.isNull() || m_currentSample >= m_channel->size()) {
            return false;
        }

        constexpr qsizetype probeSamples { 4096 };
        const qsizetype endSample { std::min(m_currentSample + probeSamples, m_channel->size()) };
        float maxAbsSample { 0.0f };
        for (qsizetype sample { m_currentSample }; sample < endSample; ++sample) {
            maxAbsSample = std::max(maxAbsSample, std::fabs(m_channel->at(sample)));
        }

        return maxAbsSample > 0.0f && maxAbsSample <= 1.0f;
    }

public:
    WaveformAudioDevice(QSharedPointer<QWavVector> channel, int startSample, QObject* parent = nullptr) :
        QIODevice(parent),
        m_channel(channel),
        m_currentSample(std::max(0, startSample)),
        m_normalizedFloat(false)
    {
        m_normalizedFloat = detectNormalizedFloat();
    }

    qint64 readData(char* data, qint64 maxSize) override
    {
        if (m_channel.isNull() || maxSize < static_cast<qint64>(sizeof(int16_t)) || m_currentSample >= m_channel->size()) {
            return 0;
        }

        const qsizetype requestedSamples { static_cast<qsizetype>(maxSize / sizeof(int16_t)) };
        const qsizetype availableSamples { m_channel->size() - m_currentSample };
        const qsizetype samplesToRead { std::min(requestedSamples, availableSamples) };
        int16_t* out { reinterpret_cast<int16_t*>(data) };
        for (qsizetype i { 0 }; i < samplesToRead; ++i) {
            out[i] = toAudioSample(m_channel->at(m_currentSample + i), m_normalizedFloat);
        }

        m_currentSample += samplesToRead;
        return static_cast<qint64>(samplesToRead * sizeof(int16_t));
    }

    qint64 writeData(const char*, qint64) override
    {
        return -1;
    }

    bool isSequential() const override
    {
        return true;
    }

    qint64 bytesAvailable() const override
    {
        if (m_channel.isNull() || m_currentSample >= m_channel->size()) {
            return QIODevice::bytesAvailable();
        }

        return static_cast<qint64>((m_channel->size() - m_currentSample) * sizeof(int16_t)) + QIODevice::bytesAvailable();
    }

    int currentSample() const
    {
        return static_cast<int>(m_currentSample);
    }
};
}

WaveformPlayerModel::WaveformPlayerModel(QObject* parent) :
    QObject(parent),
    m_playingState(WP_Stopped),
    m_currentSample(-1),
    m_blockTime(0),
    m_processedTime(0),
    m_blockStartTime(0)
{
    m_notifyTimer.setInterval(30);
    connect(&m_notifyTimer, &QTimer::timeout, this, &WaveformPlayerModel::handleAudioOutputNotify);
}

bool WaveformPlayerModel::playChannelFromSample(uint chNum, int startSample)
{
    if (m_playingState != WP_Stopped) {
        return false;
    }

    const auto channel { WaveFormModel::instance()->getChannel(chNum) };
    if (channel.isNull() || channel->empty()) {
        qDebug() << "No waveform channel data available for audio playback:" << chNum;
        return false;
    }

    const int clampedStartSample { std::clamp(startSample, 0, static_cast<int>(channel->size() - 1)) };
    const auto sampleRate { WavReader::instance()->getSampleRate() };
    if (sampleRate == 0) {
        qDebug() << "Invalid sample rate, cannot play waveform.";
        return false;
    }

    QAudioFormat format;
    format.setSampleRate(static_cast<int>(sampleRate));
    format.setChannelCount(1);
    format.setSampleFormat(QAudioFormat::Int16);

    const QAudioDevice info(QMediaDevices::defaultAudioOutput());
    if (info.isNull()) {
        qDebug() << "No audio output device available, cannot play waveform.";
        return false;
    }

    if (!info.isFormatSupported(format)) {
        qDebug() << "Audio format not supported, cannot play waveform:" << format;
        return false;
    }

    m_audio.reset(new QAudioSink(info, format));
    connect(m_audio.data(), &QAudioSink::stateChanged, this, &WaveformPlayerModel::handleAudioOutputStateChanged);

    m_audioDevice.reset(new WaveformAudioDevice(channel, clampedStartSample, this));
    m_audioDevice->open(QIODevice::ReadOnly);

    m_playingState = WP_Playing;
    m_blockTime = static_cast<int>((static_cast<qint64>(channel->size() - clampedStartSample) * 1000) / sampleRate);
    m_processedTime = 0;
    m_currentSample = clampedStartSample;
    m_blockStartTime = 0;

    emit stoppedChanged();
    emit pausedChanged();
    emit blockTimeChanged();
    emit processedTimeChanged();
    emit currentSampleChanged();

    m_notifyTimer.start();
    m_audio->start(m_audioDevice.data());
    return true;
}

void WaveformPlayerModel::handleAudioOutputStateChanged(QAudio::State state)
{
    switch (state) {
        case QAudio::IdleState:
            if (m_playingState == WP_Playing) {
                stop();
            }
            break;

        case QAudio::StoppedState:
            if (m_audio && m_audio->error() != QAudio::NoError) {
                qDebug() << "Error playing waveform: " << m_audio->error();
            }
            break;

        case QAudio::ActiveState:
            break;

        default:
            break;
    }
}

void WaveformPlayerModel::stop()
{
    if (m_playingState == WP_Stopped) {
        return;
    }

    m_notifyTimer.stop();
    if (auto audioDevice { dynamic_cast<WaveformAudioDevice*>(m_audioDevice.data()) }) {
        const int currentSample { audioDevice->currentSample() };
        if (m_currentSample != currentSample) {
            m_currentSample = currentSample;
            emit currentSampleChanged();
        }
    }

    if (m_audio) {
        m_audio->stop();
    }
    m_audio.reset();
    m_audioDevice.reset();
    m_playingState = WP_Stopped;
    m_blockStartTime = 0;
    emit stoppedChanged();
    emit pausedChanged();
}

void WaveformPlayerModel::pause()
{
    if (m_playingState == WP_Playing && m_audio) {
        m_audio->suspend();
        m_notifyTimer.stop();
        m_playingState = WP_Paused;
        emit pausedChanged();
    }
}

void WaveformPlayerModel::resume()
{
    if (m_playingState == WP_Paused && m_audio) {
        m_audio->resume();
        m_notifyTimer.start();
        m_playingState = WP_Playing;
        emit pausedChanged();
    }
}

void WaveformPlayerModel::handleAudioOutputNotify()
{
    if (m_audio) {
        m_processedTime = std::max<qint64>(0, m_audio->processedUSecs() / 1000 - m_blockStartTime);
        if (auto audioDevice { dynamic_cast<WaveformAudioDevice*>(m_audioDevice.data()) }) {
            const int currentSample { audioDevice->currentSample() };
            if (m_currentSample != currentSample) {
                m_currentSample = currentSample;
                emit currentSampleChanged();
            }
        }
    }
    emit processedTimeChanged();
}

bool WaveformPlayerModel::getStopped() const
{
    return m_playingState == WP_Stopped;
}

bool WaveformPlayerModel::getPaused() const
{
    return m_playingState == WP_Paused;
}

int WaveformPlayerModel::getCurrentSample() const
{
    return m_currentSample;
}

int WaveformPlayerModel::getBlockTime() const
{
    return m_blockTime;
}

int WaveformPlayerModel::getProcessedTime() const
{
    return m_processedTime;
}

WaveformPlayerModel::~WaveformPlayerModel()
{
    m_audio.reset();
    m_audioDevice.reset();
    qDebug() << "~WaveformPlayerModel";
}

WaveformPlayerModel* WaveformPlayerModel::instance()
{
    static WaveformPlayerModel m;
    return &m;
}
