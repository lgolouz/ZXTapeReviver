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

#include "waveformmodel.h"

WaveFormModel::WaveFormModel()
{

}

void WaveFormModel::initialize(QPair<QSharedPointer<QWavVector>, QSharedPointer<QWavVector>> channels) {
    m_channels = QVector<QSharedPointer<QWavVector>>({ channels.first, channels.second });
}

QSharedPointer<QWavVector> WaveFormModel::getChannel(int channel) {
    return channel < m_channels.size() ? m_channels.at(channel) : QSharedPointer<QWavVector>::create();
}

WaveFormModel* WaveFormModel::instance() {
    static WaveFormModel m;
    return &m;
}
