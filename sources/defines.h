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
#include <limits>

// Internal amplitude uses PCM16 units; float preserves fractional high-resolution samples.
using QWavVectorType = float;
constexpr QWavVectorType waveformFullScale { 32768.0f };
using QWavVector = QVector<QWavVectorType>;

template<typename T>
inline bool lessThanZero(T t) {
    return t < 0;
}

// Allowed duration of a signal part, in samples.
struct LengthRange {
    double min { std::numeric_limits<double>::infinity() };
    double max { 0.0 };

    Q_ALWAYS_INLINE bool contains(double length) const {
        return length >= min && length <= max;
    }
};

// Frequency range [signalFreq * (1 - deltaBelow), signalFreq * (1 + deltaAbove)] converted to durations:
// the higher frequency bound gives the shorter length. A non-positive lower frequency bound leaves the
// length unbounded from above; a non-positive signal frequency gives an empty range.
// Bounds are widened by a relative epsilon so a length that lands exactly on a bound stays inside it.
inline LengthRange freqToLengthRange(double sampleRate, double signalFreq, double deltaBelow, double deltaAbove) {
    constexpr double boundEpsilon { 1e-9 };
    const double highestFreq { signalFreq * (1.0 + deltaAbove) };
    const double lowestFreq { signalFreq * (1.0 - deltaBelow) };
    if (signalFreq <= 0.0 || highestFreq <= 0.0) {
        return { };
    }
    return {
        sampleRate / highestFreq * (1.0 - boundEpsilon),
        lowestFreq > 0.0 ? sampleRate / lowestFreq * (1.0 + boundEpsilon) : std::numeric_limits<double>::infinity()
    };
}

inline LengthRange freqToLengthRange(double sampleRate, double signalFreq, double delta) {
    return freqToLengthRange(sampleRate, signalFreq, delta, delta);
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
