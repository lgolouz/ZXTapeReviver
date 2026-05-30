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

#include "parsersettingsmodel.h"
#include "sources/defines.h"
#include <QScopedPointer>
#include <algorithm>

namespace {
constexpr int c_defaultAdaptiveBaseDepth { 12 };
constexpr int c_defaultAdaptiveUncertainDepth { 32 };
constexpr int c_defaultAdaptiveMaxDepth { 64 };
constexpr int c_defaultAdaptiveBeamWidth { 12 };
constexpr double c_defaultAdaptiveTimingStabilityPenalty { 0.35 };
constexpr ParserSettingsModel::AdaptiveAlternativeMode c_defaultAdaptiveAlternativeMode { ParserSettingsModel::AdaptiveSmartAlternatives };
}

ParserSettingsModel::ParserSettingsModel(QObject* parent) :
    QObject(parent),
    m_parserSettings {
        SignalFrequencies::PILOT_HALF_FREQ,
        SignalFrequencies::PILOT_FREQ,
        SignalFrequencies::SYNCHRO_FIRST_HALF_FREQ,
        SignalFrequencies::SYNCHRO_SECOND_HALF_FREQ,
        SignalFrequencies::SYNCHRO_FREQ,
        preciseSynchroCheck,
        SignalFrequencies::ZERO_HALF_FREQ,
        SignalFrequencies::ZERO_FREQ,
        SignalFrequencies::ONE_HALF_FREQ,
        SignalFrequencies::ONE_FREQ,
        pilotDelta,
        synchroDelta,
        zeroDelta,
        oneDelta,
        checkForAbnormalSine,
        sineCheckTolerance,
        StandardParser,
        c_defaultAdaptiveBaseDepth,
        c_defaultAdaptiveUncertainDepth,
        c_defaultAdaptiveMaxDepth,
        c_defaultAdaptiveBeamWidth,
        c_defaultAdaptiveTimingStabilityPenalty,
        c_defaultAdaptiveAlternativeMode }
{

}

void ParserSettingsModel::restoreDefaultSettings()
{
    setPilotHalfFreq(SignalFrequencies::PILOT_HALF_FREQ);
    setPilotFreq(SignalFrequencies::PILOT_FREQ);
    setSynchroFirstHalfFreq(SignalFrequencies::SYNCHRO_FIRST_HALF_FREQ);
    setSynchroSecondHalfFreq(SignalFrequencies::SYNCHRO_SECOND_HALF_FREQ);
    setSynchroFreq(SignalFrequencies::SYNCHRO_FREQ);
    setPreciseSynchroCheck(preciseSynchroCheck);
    setZeroHalfFreq(SignalFrequencies::ZERO_HALF_FREQ);
    setZeroFreq(SignalFrequencies::ZERO_FREQ);
    setOneHalfFreq(SignalFrequencies::ONE_HALF_FREQ);
    setOneFreq(SignalFrequencies::ONE_FREQ);
    setPilotDelta(pilotDelta);
    setSynchroDelta(synchroDelta);
    setZeroDelta(zeroDelta);
    setOneDelta(oneDelta);
    setCheckForAbnormalSine(checkForAbnormalSine);
    setSineCheckTolerance(sineCheckTolerance);
    setParserMode(StandardParser);
    setAdaptiveBaseDepth(c_defaultAdaptiveBaseDepth);
    setAdaptiveUncertainDepth(c_defaultAdaptiveUncertainDepth);
    setAdaptiveMaxDepth(c_defaultAdaptiveMaxDepth);
    setAdaptiveBeamWidth(c_defaultAdaptiveBeamWidth);
    setAdaptiveTimingStabilityPenalty(c_defaultAdaptiveTimingStabilityPenalty);
    setAdaptiveAlternativeMode(c_defaultAdaptiveAlternativeMode);
}

void ParserSettingsModel::applyAdaptiveParserPreset(AdaptiveParserPreset preset)
{
    switch (preset) {
    case AdaptiveBasicPreset:
        setAdaptiveBaseDepth(10);
        setAdaptiveUncertainDepth(10);
        setAdaptiveMaxDepth(10);
        setAdaptiveBeamWidth(12);
        setAdaptiveTimingStabilityPenalty(0.35);
        break;

    case AdaptiveFastPreset:
        setAdaptiveBaseDepth(8);
        setAdaptiveUncertainDepth(20);
        setAdaptiveMaxDepth(32);
        setAdaptiveBeamWidth(8);
        setAdaptiveTimingStabilityPenalty(0.45);
        break;

    case AdaptiveAccuratePreset:
        setAdaptiveBaseDepth(c_defaultAdaptiveBaseDepth);
        setAdaptiveUncertainDepth(c_defaultAdaptiveUncertainDepth);
        setAdaptiveMaxDepth(c_defaultAdaptiveMaxDepth);
        setAdaptiveBeamWidth(c_defaultAdaptiveBeamWidth);
        setAdaptiveTimingStabilityPenalty(c_defaultAdaptiveTimingStabilityPenalty);
        break;

    case AdaptiveMaximumPreset:
        setAdaptiveBaseDepth(16);
        setAdaptiveUncertainDepth(64);
        setAdaptiveMaxDepth(128);
        setAdaptiveBeamWidth(24);
        setAdaptiveTimingStabilityPenalty(0.25);
        break;
    }
}

const ParserSettingsModel::ParserSettings& ParserSettingsModel::getParserSettings() const
{
    return m_parserSettings;
}

int ParserSettingsModel::getPilotHalfFreq() const
{
    return m_parserSettings.pilotHalfFreq;
}

int ParserSettingsModel::getPilotFreq() const
{
    return m_parserSettings.pilotFreq;
}

int ParserSettingsModel::getSynchroFirstHalfFreq() const
{
    return m_parserSettings.synchroFirstHalfFreq;
}

int ParserSettingsModel::getSynchroSecondHalfFreq() const
{
    return m_parserSettings.synchroSecondHalfFreq;
}

int ParserSettingsModel::getSynchroFreq() const
{
    return m_parserSettings.synchroFreq;
}

bool ParserSettingsModel::getPreciseSynchroCheck() const
{
    return m_parserSettings.preciseSynchroCheck;
}

int ParserSettingsModel::getZeroHalfFreq() const
{
    return m_parserSettings.zeroHalfFreq;
}

int ParserSettingsModel::getZeroFreq() const
{
    return m_parserSettings.zeroFreq;
}

int ParserSettingsModel::getOneHalfFreq() const
{
    return m_parserSettings.oneHalfFreq;
}

int ParserSettingsModel::getOneFreq() const
{
    return m_parserSettings.oneFreq;
}

double ParserSettingsModel::getPilotDelta() const
{
    return m_parserSettings.pilotDelta;
}

double ParserSettingsModel::getSynchroDelta() const
{
    return m_parserSettings.synchroDelta;
}

double ParserSettingsModel::getZeroDelta() const
{
    return m_parserSettings.zeroDelta;
}

double ParserSettingsModel::getOneDelta() const
{
    return m_parserSettings.oneDelta;
}

bool ParserSettingsModel::getCheckForAbnormalSine() const
{
    return m_parserSettings.checkForAbnormalSine;
}

double ParserSettingsModel::getSineCheckTolerance() const {
    return m_parserSettings.sineCheckTolerance;
}

ParserSettingsModel::ParserMode ParserSettingsModel::getParserMode() const
{
    return m_parserSettings.parserMode;
}

int ParserSettingsModel::getAdaptiveBaseDepth() const
{
    return m_parserSettings.adaptiveBaseDepth;
}

int ParserSettingsModel::getAdaptiveUncertainDepth() const
{
    return m_parserSettings.adaptiveUncertainDepth;
}

int ParserSettingsModel::getAdaptiveMaxDepth() const
{
    return m_parserSettings.adaptiveMaxDepth;
}

int ParserSettingsModel::getAdaptiveBeamWidth() const
{
    return m_parserSettings.adaptiveBeamWidth;
}

double ParserSettingsModel::getAdaptiveTimingStabilityPenalty() const
{
    return m_parserSettings.adaptiveTimingStabilityPenalty;
}

ParserSettingsModel::AdaptiveAlternativeMode ParserSettingsModel::getAdaptiveAlternativeMode() const
{
    return m_parserSettings.adaptiveAlternativeMode;
}

void ParserSettingsModel::setPilotHalfFreq(int freq)
{
    if (m_parserSettings.pilotHalfFreq != freq) {
        m_parserSettings.pilotHalfFreq = freq;
        emit pilotHalfFreqChanged();
    }
}

void ParserSettingsModel::setPilotFreq(int freq)
{
    if (m_parserSettings.pilotFreq != freq) {
        m_parserSettings.pilotFreq = freq;
        emit pilotFreqChanged();
    }
}

void ParserSettingsModel::setSynchroFirstHalfFreq(int freq)
{
    if (m_parserSettings.synchroFirstHalfFreq != freq) {
        m_parserSettings.synchroFirstHalfFreq = freq;
        emit synchroFirstHalfFreqChanged();
    }
}

void ParserSettingsModel::setSynchroSecondHalfFreq(int freq)
{
    if (m_parserSettings.synchroSecondHalfFreq != freq) {
        m_parserSettings.synchroSecondHalfFreq = freq;
        emit synchroSecondHalfFreqChanged();
    }
}

void ParserSettingsModel::setSynchroFreq(int freq)
{
    if (m_parserSettings.synchroFreq != freq) {
        m_parserSettings.synchroFreq = freq;
        emit synchroFreqChanged();
    }
}

void ParserSettingsModel::setPreciseSynchroCheck(bool precise)
{
    if (m_parserSettings.preciseSynchroCheck != precise) {
        m_parserSettings.preciseSynchroCheck = precise;
        emit preciseSychroCheckChanged();
    }
}

void ParserSettingsModel::setZeroHalfFreq(int freq)
{
    if (m_parserSettings.zeroHalfFreq != freq) {
        m_parserSettings.zeroHalfFreq = freq;
        emit zeroHalfFreqChanged();
    }
}

void ParserSettingsModel::setZeroFreq(int freq)
{
    if (m_parserSettings.zeroFreq != freq) {
        m_parserSettings.zeroFreq = freq;
        emit zeroFreqChanged();
    }
}

void ParserSettingsModel::setOneHalfFreq(int freq)
{
    if (m_parserSettings.oneHalfFreq != freq) {
        m_parserSettings.oneHalfFreq = freq;
        emit oneHalfFreqChanged();
    }
}

void ParserSettingsModel::setOneFreq(int freq)
{
    if (m_parserSettings.oneFreq != freq) {
        m_parserSettings.oneFreq = freq;
        emit oneFreqChanged();
    }
}

void ParserSettingsModel::setPilotDelta(double delta)
{
    if (m_parserSettings.pilotDelta != delta) {
        m_parserSettings.pilotDelta = delta;
        emit pilotDeltaChanged();
    }
}

void ParserSettingsModel::setSynchroDelta(double delta)
{
    if (m_parserSettings.synchroDelta != delta) {
        m_parserSettings.synchroDelta = delta;
        emit synchroDeltaChanged();
    }
}

void ParserSettingsModel::setZeroDelta(double delta)
{
    if (m_parserSettings.zeroDelta != delta) {
        m_parserSettings.zeroDelta = delta;
        emit zeroDeltaChanged();
    }
}

void ParserSettingsModel::setOneDelta(double delta)
{
    if (m_parserSettings.oneDelta != delta) {
        m_parserSettings.oneDelta = delta;
        emit oneDeltaChanged();
    }
}

void ParserSettingsModel::setCheckForAbnormalSine(bool check)
{
    if (m_parserSettings.checkForAbnormalSine != check) {
        m_parserSettings.checkForAbnormalSine = check;
        emit checkForAbnormalSineChanged();
    }
}

void ParserSettingsModel::setSineCheckTolerance(double value) {
    if (m_parserSettings.sineCheckTolerance != value) {
        m_parserSettings.sineCheckTolerance = value;
        emit sineCheckToleranceChanged();
    }
}

void ParserSettingsModel::setParserMode(ParserMode mode)
{
    if (m_parserSettings.parserMode != mode) {
        m_parserSettings.parserMode = mode;
        emit parserModeChanged();
    }
}

void ParserSettingsModel::setAdaptiveBaseDepth(int depth)
{
    const int clampedDepth { std::clamp(depth, 2, 128) };
    if (m_parserSettings.adaptiveBaseDepth != clampedDepth) {
        m_parserSettings.adaptiveBaseDepth = clampedDepth;
        if (m_parserSettings.adaptiveUncertainDepth < clampedDepth) {
            m_parserSettings.adaptiveUncertainDepth = clampedDepth;
            emit adaptiveUncertainDepthChanged();
        }
        if (m_parserSettings.adaptiveMaxDepth < m_parserSettings.adaptiveUncertainDepth) {
            m_parserSettings.adaptiveMaxDepth = m_parserSettings.adaptiveUncertainDepth;
            emit adaptiveMaxDepthChanged();
        }
        emit adaptiveBaseDepthChanged();
    }
}

void ParserSettingsModel::setAdaptiveUncertainDepth(int depth)
{
    const int clampedDepth { std::clamp(depth, m_parserSettings.adaptiveBaseDepth, 128) };
    if (m_parserSettings.adaptiveUncertainDepth != clampedDepth) {
        m_parserSettings.adaptiveUncertainDepth = clampedDepth;
        if (m_parserSettings.adaptiveMaxDepth < clampedDepth) {
            m_parserSettings.adaptiveMaxDepth = clampedDepth;
            emit adaptiveMaxDepthChanged();
        }
        emit adaptiveUncertainDepthChanged();
    }
}

void ParserSettingsModel::setAdaptiveMaxDepth(int depth)
{
    const int clampedDepth { std::clamp(depth, m_parserSettings.adaptiveUncertainDepth, 128) };
    if (m_parserSettings.adaptiveMaxDepth != clampedDepth) {
        m_parserSettings.adaptiveMaxDepth = clampedDepth;
        emit adaptiveMaxDepthChanged();
    }
}

void ParserSettingsModel::setAdaptiveBeamWidth(int width)
{
    const int clampedWidth { std::clamp(width, 2, 64) };
    if (m_parserSettings.adaptiveBeamWidth != clampedWidth) {
        m_parserSettings.adaptiveBeamWidth = clampedWidth;
        emit adaptiveBeamWidthChanged();
    }
}

void ParserSettingsModel::setAdaptiveTimingStabilityPenalty(double penalty)
{
    const double clampedPenalty { std::clamp(penalty, 0.0, 2.0) };
    if (m_parserSettings.adaptiveTimingStabilityPenalty != clampedPenalty) {
        m_parserSettings.adaptiveTimingStabilityPenalty = clampedPenalty;
        emit adaptiveTimingStabilityPenaltyChanged();
    }
}

void ParserSettingsModel::setAdaptiveAlternativeMode(AdaptiveAlternativeMode mode)
{
    if (m_parserSettings.adaptiveAlternativeMode != mode) {
        m_parserSettings.adaptiveAlternativeMode = mode;
        emit adaptiveAlternativeModeChanged();
    }
}

ParserSettingsModel* ParserSettingsModel::instance()
{
    static ParserSettingsModel m;
    return &m;
}
