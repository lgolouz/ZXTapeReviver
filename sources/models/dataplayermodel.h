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

#ifndef DATAPLAYERMODEL_H
#define DATAPLAYERMODEL_H

#include <QTimer>
#include <QBuffer>
#include <QAudioOutput>
#include "sources/core/waveformparser.h"

class DataPlayerModel : public QObject
{
    Q_OBJECT

    enum PlayingState {
        DP_Stopped = 0,
        DP_Playing,
        DP_Paused
    };

    Q_PROPERTY(bool stopped READ getStopped NOTIFY stoppedChanged)
    Q_PROPERTY(int currentBlock READ getCurrentBlock NOTIFY currentBlockChanged)
    Q_PROPERTY(int blockTime READ getBlockTime NOTIFY blockTimeChanged)
    Q_PROPERTY(int processedTime READ getProcessedTime NOTIFY processedTimeChanged)
    Q_PROPERTY(QVariant blockData READ getBlockData NOTIFY currentBlockChanged)

    PlayingState m_playingState;
    QScopedPointer<QAudioOutput> m_audio;
    QPair<QVector<WaveformParser::DataBlock>, QVector<bool>> m_data;
    QVariantList m_parserData;
    unsigned m_currentBlock;
    QTimer m_delayTimer;
    QBuffer m_buffer;
    const unsigned c_sampleRate { 44100 };
    int m_blockTime;
    int m_processedTime;

protected slots:
    void handleAudioOutputStateChanged(QAudio::State state);
    void handleAudioOutputNotify();
    void handleNextDataRecord();

protected:
    explicit DataPlayerModel(QObject* parent = nullptr);
    void prepareNextDataRecord();

public:
    virtual ~DataPlayerModel() override;

    bool getStopped() const;
    int getCurrentBlock() const;
    int getBlockTime() const;
    int getProcessedTime() const;
    QVariant getBlockData() const;

    Q_INVOKABLE void playParsedData(uint chNum, uint currentBlock = 0);
    Q_INVOKABLE void stop();
    //Q_INVOKABLE void pause();
    //Q_INVOKABLE void resume();

    static DataPlayerModel* instance();

signals:
    void stoppedChanged();
    void currentBlockChanged();
    void blockTimeChanged();
    void processedTimeChanged();
};

#endif // DATAPLAYERMODEL_H
