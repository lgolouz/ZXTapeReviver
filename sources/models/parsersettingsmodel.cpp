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

ParserSettingsModel::ParserSettingsModel(QObject* parent) :
    QObject(parent),
    m_parserSettings {
        SignalFrequencies::PILOT_HALF_FREQ,
        SignalFrequencies::PILOT_FREQ,
        SignalFrequencies::SYNCHRO_FIRST_HALF_FREQ,
        SignalFrequencies::SYNCHRO_SECOND_HALF_FREQ,
        SignalFrequencies::SYNCHRO_FREQ,
        SignalFrequencies::ZERO_HALF_FREQ,
        SignalFrequencies::ZERO_FREQ,
        SignalFrequencies::ONE_HALF_FREQ,
        SignalFrequencies::ONE_FREQ,
        pilotDelta,
        synchroDelta,
        zeroDelta,
        oneDelta,
        checkForAbnormalSine,
        sineCheckTolerance }
{

}

void ParserSettingsModel::restoreDefaultSettings()
{
    setPilotHalfFreq(SignalFrequencies::PILOT_HALF_FREQ);
    setPilotFreq(SignalFrequencies::PILOT_FREQ);
    setSynchroFirstHalfFreq(SignalFrequencies::SYNCHRO_FIRST_HALF_FREQ);
    setSynchroSecondHalfFreq(SignalFrequencies::SYNCHRO_SECOND_HALF_FREQ);
    setSynchroFreq(SignalFrequencies::SYNCHRO_FREQ);
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

ParserSettingsModel* ParserSettingsModel::instance()
{
    static QScopedPointer<ParserSettingsModel> m { new ParserSettingsModel() };
    return m.get();
}
