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

#ifndef PARSERSETTINGSMODEL_H
#define PARSERSETTINGSMODEL_H

#include <QObject>

class ParserSettingsModel : public QObject
{
    Q_OBJECT

    Q_PROPERTY(int pilotHalfFreq READ getPilotHalfFreq WRITE setPilotHalfFreq NOTIFY pilotHalfFreqChanged)
    Q_PROPERTY(int pilotFreq READ getPilotFreq WRITE setPilotFreq NOTIFY pilotFreqChanged)
    Q_PROPERTY(int synchroFirstHalfFreq READ getSynchroFirstHalfFreq WRITE setSynchroFirstHalfFreq NOTIFY synchroFirstHalfFreqChanged)
    Q_PROPERTY(int synchroSecondHalfFreq READ getSynchroSecondHalfFreq WRITE setSynchroSecondHalfFreq NOTIFY synchroSecondHalfFreqChanged)
    Q_PROPERTY(int synchroFreq READ getSynchroFreq WRITE setSynchroFreq NOTIFY synchroFreqChanged)
    Q_PROPERTY(bool preciseSynchroCheck READ getPreciseSynchroCheck WRITE setPreciseSynchroCheck NOTIFY preciseSychroCheckChanged)
    Q_PROPERTY(int zeroHalfFreq READ getZeroHalfFreq WRITE setZeroHalfFreq NOTIFY zeroHalfFreqChanged)
    Q_PROPERTY(int zeroFreq READ getZeroFreq WRITE setZeroFreq NOTIFY zeroFreqChanged)
    Q_PROPERTY(int oneHalfFreq READ getOneHalfFreq WRITE setOneHalfFreq NOTIFY oneHalfFreqChanged)
    Q_PROPERTY(int oneFreq READ getOneFreq WRITE setOneFreq NOTIFY oneFreqChanged)
    Q_PROPERTY(double pilotDelta READ getPilotDelta WRITE setPilotDelta NOTIFY pilotDeltaChanged)
    Q_PROPERTY(double synchroDelta READ getSynchroDelta WRITE setSynchroDelta NOTIFY synchroDeltaChanged)
    Q_PROPERTY(double zeroDelta READ getZeroDelta WRITE setZeroDelta NOTIFY zeroDeltaChanged)
    Q_PROPERTY(double oneDelta READ getOneDelta WRITE setOneDelta NOTIFY oneDeltaChanged)
    Q_PROPERTY(bool checkForAbnormalSine READ getCheckForAbnormalSine WRITE setCheckForAbnormalSine NOTIFY checkForAbnormalSineChanged)
    Q_PROPERTY(double sineCheckTolerance READ getSineCheckTolerance WRITE setSineCheckTolerance NOTIFY sineCheckToleranceChanged)

protected:
    explicit ParserSettingsModel(QObject* parent = nullptr);

public:
    struct ParserSettings {
        int32_t pilotHalfFreq;
        int32_t pilotFreq;
        int32_t synchroFirstHalfFreq;
        int32_t synchroSecondHalfFreq;
        int32_t synchroFreq;
        bool preciseSynchroCheck;
        int32_t zeroHalfFreq;
        int32_t zeroFreq;
        int32_t oneHalfFreq;
        int32_t oneFreq;
        double pilotDelta;
        double synchroDelta;
        double zeroDelta;
        double oneDelta;
        bool checkForAbnormalSine;
        double sineCheckTolerance;
    };

    virtual ~ParserSettingsModel() = default;
    const ParserSettings& getParserSettings() const;

    static ParserSettingsModel* instance();

    Q_INVOKABLE void restoreDefaultSettings();

    //Getters
    int getPilotHalfFreq() const;
    int getPilotFreq() const;
    int getSynchroFirstHalfFreq() const;
    int getSynchroSecondHalfFreq() const;
    int getSynchroFreq() const;
    bool getPreciseSynchroCheck() const;
    int getZeroHalfFreq() const;
    int getZeroFreq() const;
    int getOneHalfFreq() const;
    int getOneFreq() const;
    double getPilotDelta() const;
    double getSynchroDelta() const;
    double getZeroDelta() const;
    double getOneDelta() const;
    bool getCheckForAbnormalSine() const;
    double getSineCheckTolerance() const;

    //Setters
    void setPilotHalfFreq(int freq);
    void setPilotFreq(int freq);
    void setSynchroFirstHalfFreq(int freq);
    void setSynchroSecondHalfFreq(int freq);
    void setSynchroFreq(int freq);
    void setPreciseSynchroCheck(bool precise);
    void setZeroHalfFreq(int freq);
    void setZeroFreq(int freq);
    void setOneHalfFreq(int freq);
    void setOneFreq(int freq);
    void setPilotDelta(double delta);
    void setSynchroDelta(double delta);
    void setZeroDelta(double delta);
    void setOneDelta(double delta);
    void setCheckForAbnormalSine(bool check);
    void setSineCheckTolerance(double value);

signals:
    void pilotHalfFreqChanged();
    void pilotFreqChanged();
    void synchroFirstHalfFreqChanged();
    void synchroSecondHalfFreqChanged();
    void synchroFreqChanged();
    void preciseSychroCheckChanged();
    void zeroHalfFreqChanged();
    void zeroFreqChanged();
    void oneHalfFreqChanged();
    void oneFreqChanged();
    void pilotDeltaChanged();
    void synchroDeltaChanged();
    void zeroDeltaChanged();
    void oneDeltaChanged();
    void checkForAbnormalSineChanged();
    void sineCheckToleranceChanged();

private:
    ParserSettings m_parserSettings;
};

#endif // PARSERSETTINGSMODEL_H
