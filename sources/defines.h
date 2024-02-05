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

#ifndef DEFINES_H
#define DEFINES_H

#include <QVector>

using QWavVectorType = float;
using QWavVector = QVector<QWavVectorType>;

template<typename T>
inline bool lessThanZero(T t) {
    return t < 0;
}

inline bool isFreqFitsInDelta(uint32_t sampleRate, uint32_t length, uint32_t signalFreq, double signalDelta, double deltaDivider = 1.0) {
    const double freq = sampleRate / length;
    const double delta = signalFreq * (signalDelta / deltaDivider);
    return freq >= (signalFreq - delta) && freq <= (signalFreq + delta);
}

inline bool isFreqFitsInDelta2(uint32_t sampleRate, uint32_t length, uint32_t signalFreq, double signalDeltaBelow, double signalDeltaAbove) {
    const double freq = sampleRate / length;
    const double deltaB = signalFreq * signalDeltaBelow;
    const double deltaA = signalFreq * signalDeltaAbove;
    return freq >= (signalFreq - deltaB) && freq <= (signalFreq + deltaA);
}

enum SignalFrequencies {
    PILOT_HALF_FREQ = 1660,
    PILOT_FREQ = 830,
    SYNCHRO_FIRST_HALF_FREQ = 6300,
    SYNCHRO_SECOND_HALF_FREQ = 5500,
    SYNCHRO_FREQ = 2950,
    ZERO_HALF_FREQ = 4200,
    ZERO_FIRST_HALF_FREQ = 0,
    ZERO_SECOND_HALF_FREQ = 0,
    ZERO_FREQ = 2100,
    ONE_HALF_FREQ = 2100,
    ONE_FREQ = 1050
};

constexpr const bool preciseSynchroCheck = false;
constexpr const bool checkForAbnormalSine = true;

constexpr const double pilotDelta = 0.1;
constexpr const double synchroDelta = 0.3;
constexpr const double zeroDelta = 0.3;//0.3;//0.18;
constexpr const double oneDelta = 0.25;//0.25;//0.1;
constexpr const double sineCheckTolerance = 0.5;

#endif // DEFINES_H
