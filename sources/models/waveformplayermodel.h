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

#ifndef WAVEFORMPLAYERMODEL_H
#define WAVEFORMPLAYERMODEL_H

#include <QAudioSink>
#include <QIODevice>
#include <QScopedPointer>
#include <QTimer>
#include <QObject>

class WaveformPlayerModel : public QObject
{
    Q_OBJECT

    enum PlayingState {
        WP_Stopped = 0,
        WP_Playing,
        WP_Paused
    };

    Q_PROPERTY(bool stopped READ getStopped NOTIFY stoppedChanged)
    Q_PROPERTY(bool paused READ getPaused NOTIFY pausedChanged)
    Q_PROPERTY(int currentSample READ getCurrentSample NOTIFY currentSampleChanged)
    Q_PROPERTY(int blockTime READ getBlockTime NOTIFY blockTimeChanged)
    Q_PROPERTY(int processedTime READ getProcessedTime NOTIFY processedTimeChanged)

    PlayingState m_playingState;
    QScopedPointer<QAudioSink> m_audio;
    QScopedPointer<QIODevice> m_audioDevice;
    QTimer m_notifyTimer;
    int m_currentSample;
    int m_blockTime;
    int m_processedTime;
    qint64 m_blockStartTime;

protected slots:
    void handleAudioOutputStateChanged(QAudio::State state);
    void handleAudioOutputNotify();

protected:
    explicit WaveformPlayerModel(QObject* parent = nullptr);

public:
    virtual ~WaveformPlayerModel() override;

    bool getStopped() const;
    bool getPaused() const;
    int getCurrentSample() const;
    int getBlockTime() const;
    int getProcessedTime() const;

    Q_INVOKABLE bool playChannelFromSample(uint chNum, int startSample);
    Q_INVOKABLE void stop();
    Q_INVOKABLE void pause();
    Q_INVOKABLE void resume();

    static WaveformPlayerModel* instance();

signals:
    void stoppedChanged();
    void pausedChanged();
    void currentSampleChanged();
    void blockTimeChanged();
    void processedTimeChanged();
};

#endif // WAVEFORMPLAYERMODEL_H
