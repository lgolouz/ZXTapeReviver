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

#ifndef SIGNALLENGTHRANGES_H
#define SIGNALLENGTHRANGES_H

#include "sources/defines.h"
#include "sources/models/parsersettingsmodel.h"

// Allowed half-wave and period durations (in samples), computed once per parse from the parser settings.
struct SignalLengthRanges {
    LengthRange pilotHalf;
    LengthRange synchroFirstHalf;
    LengthRange synchroSecondHalf;
    LengthRange synchro;
    LengthRange zero;
    LengthRange one;
    LengthRange zeroHalfSine;
    LengthRange oneHalfSine;

    static SignalLengthRanges fromSettings(const ParserSettingsModel::ParserSettings& s, double sampleRate, double dataSignalDelta) {
        SignalLengthRanges r;
        r.pilotHalf = freqToLengthRange(sampleRate, s.pilotHalfFreq, s.pilotDelta);
        r.synchroFirstHalf = freqToLengthRange(sampleRate, s.synchroFirstHalfFreq, s.synchroDelta);
        r.synchroSecondHalf = freqToLengthRange(sampleRate, s.synchroSecondHalfFreq, s.synchroDelta);
        r.synchro = freqToLengthRange(sampleRate, s.synchroFreq, s.synchroDelta);
        r.zero = freqToLengthRange(sampleRate, s.zeroFreq, s.zeroDelta, dataSignalDelta);
        r.one = freqToLengthRange(sampleRate, s.oneFreq, dataSignalDelta, s.oneDelta);
        r.zeroHalfSine = freqToLengthRange(sampleRate, s.zeroHalfFreq, s.zeroDelta / s.sineCheckTolerance);
        r.oneHalfSine = freqToLengthRange(sampleRate, s.oneHalfFreq, s.oneDelta / s.sineCheckTolerance);
        return r;
    }

    const LengthRange& halfSine(bool zeroCheck) const {
        return zeroCheck ? zeroHalfSine : oneHalfSine;
    }
};

#endif // SIGNALLENGTHRANGES_H
