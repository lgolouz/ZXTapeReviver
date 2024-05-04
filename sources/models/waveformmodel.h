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

#ifndef WAVEFORMMODEL_H
#define WAVEFORMMODEL_H

#include "sources/defines.h"
#include <QSharedPointer>
#include <QVector>

class WaveFormModel final
{
    QVector<QSharedPointer<QWavVector>> m_channels;

private:
    WaveFormModel();

public:
    ~WaveFormModel() = default;

    WaveFormModel(const WaveFormModel& other) = delete;
    WaveFormModel(WaveFormModel&& other) = delete;
    WaveFormModel& operator= (const WaveFormModel& other) = delete;
    WaveFormModel& operator= (WaveFormModel&& other) = delete;

    static WaveFormModel* instance();

    void initialize(QPair<QSharedPointer<QWavVector>, QSharedPointer<QWavVector>> channels);
    QSharedPointer<QWavVector> getChannel(int channel);
};

#endif // WAVEFORMMODEL_H
