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
};

enum SignalFrequencies {
    PILOT_HALF_FREQ = 1620,
    PILOT_FREQ = 810,
    SYNCHRO_FIRST_HALF_FREQ = 4900,
    SYNCHRO_SECOND_HALF_FREQ = 5500,
    SYNCHRO_FREQ = 2600,
    ZERO_HALF_FREQ = 4090,
    ZERO_FIRST_HALF_FREQ = 0,
    ZERO_SECOND_HALF_FREQ = 0,
    ZERO_FREQ = 2050,
    ONE_HALF_FREQ = 2045,
    ONE_FREQ = 1023
};

const bool checkForAbnormalSine = true;

const double pilotDelta = 0.1;
const double synchroDelta = 0.3;
const double zeroDelta = 0.3;//0.3;//0.18;
const double oneDelta = 0.25;//0.25;//0.1;

#endif // DEFINES_H
