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

#ifndef WAVEFORMPARSER_H
#define WAVEFORMPARSER_H

#include <iterator>
#include <QMap>
#include <QSet>
#include <QVariantList>
#include <QVector>
#include <QVariantMap>
#include "sources/core/parseddata.h"
#include "sources/core/wavreader.h"
#include "sources/defines.h"
#include "sources/models/parsersettingsmodel.h"

class WaveformParser : public QObject
{
    Q_OBJECT

    Q_PROPERTY(ZxTableModel* parsedChannel0 READ getParsedChannel0 NOTIFY parsedChannel0Changed)
    Q_PROPERTY(ZxTableModel* parsedChannel1 READ getParsedChannel1 NOTIFY parsedChannel1Changed)
    Q_PROPERTY(bool parsingActive READ getParsingActive NOTIFY parsingProgressChanged)
    Q_PROPERTY(int parsingProgress READ getParsingProgress NOTIFY parsingProgressChanged)
    Q_PROPERTY(QString parsingStatus READ getParsingStatus NOTIFY parsingProgressChanged)
    Q_PROPERTY(bool parsingCancellationRequested READ getParsingCancellationRequested NOTIFY parsingProgressChanged)

public:
//    enum SignalValue { ZERO, ONE, PILOT, SYNCHRO };
//    struct WaveformData
//    {
//        uint32_t begin;
//        uint32_t end;
//        uint32_t waveBegin;
//        uint32_t waveEnd;
//        SignalValue value;
//    };

private:
    enum StateType { SEARCH_OF_PILOT_TONE, PILOT_TONE, SYNCHRO_SIGNAL, DATA_SIGNAL, END_OF_DATA, NO_MORE_DATA };

    template <typename T>
    QVector<ParsedData::WaveformPart> parseChannel(const QVector<T>& ch) {
        decltype(parseChannel(ch)) result;
        if (ch.size() < 1) {
            return result;
        }

        auto it = ch.begin();
        auto val = *it;
        while (it != ch.end()) {
            auto prevIt = it;
            it = std::find_if(it, ch.end(), [&val](const T& i) { return lessThanZero(val) != lessThanZero(i); });
            typename std::remove_reference<decltype(*result.begin())>::type part;
            part.begin = std::distance(ch.begin(), prevIt);
            part.end = std::distance(ch.begin(), std::prev(it));
            part.length = std::distance(prevIt, it);
            part.sign = lessThanZero(val) ? ParsedData::NEGATIVE : ParsedData::POSITIVE;

            result.append(part);

            if (it != ch.end()) {
                val = *it;
            }
        }

        return result;
    }

    //Helper methods intended to use in case of change we can made them only once
    Q_ALWAYS_INLINE bool isZeroFreqFitsInDelta(uint32_t sampleRate, uint32_t length, uint32_t signalFreq, double signalDeltaBelow, double signalDeltaAbove) const;
    Q_ALWAYS_INLINE bool isOneFreqFitsInDelta(uint32_t sampleRate, uint32_t length, uint32_t signalFreq, double signalDeltaBelow, double signalDeltaAbove) const;

    WavReader& mWavReader;
    QMap<uint, ParsedData*> m_parsedData;
    // QMap<uint, QVector<uint8_t>> mParsedWaveform;
    // QMap<uint, QVector<DataBlock>> mParsedData;
    QMap<uint, QSet<int>> mSelectedBlocks;
    QVector<ParsedData::WaveformPart> m_experimentalDebugParsed;
    uint m_experimentalDebugChannel;
    bool m_experimentalDebugActive;
    bool m_experimentalDebugManualInspection;
    size_t m_experimentalDebugSample;
    QVariantMap m_experimentalDebugState;
    bool m_parsingActive;
    bool m_parsingCancellationRequested;
    int m_parsingProgress;
    QString m_parsingStatus;

    double scoreExperimentalWindow(const QWavVector& channel, size_t begin, size_t length, double* axis = nullptr, double* upperLevel = nullptr, double* lowerLevel = nullptr) const;
    QVariantMap findExperimentalBitCandidate(const QWavVector& channel, size_t expectedBegin, uint8_t bit, const ParserSettingsModel::ParserSettings& parserSettings, double sampleRate) const;
    QVariantMap findExperimentalPeriodCandidate(const QWavVector& channel, size_t expectedBegin, const ParserSettingsModel::ParserSettings& parserSettings, double sampleRate) const;
    void setExperimentalDebugInactive(uint chNum, const QString& message = QString());
    void setParsingProgress(bool active, int progress, const QString& status);

protected:
    explicit WaveformParser(QObject* parent = nullptr);
    QPointer<ParsedDataModel> getParsedChannelData(uint chNum) const;
    Q_ALWAYS_INLINE ParsedData* getOrCreateParsedDataPtr(uint chNum);
    Q_ALWAYS_INLINE ParsedData* getParsedDataPtr(uint chNum) const;

public:
    enum class SaveTapResultCode {
        Success,
        NoParsedData,
        CannotRemoveExistingFile,
        CannotOpenFile
    };
    Q_ENUM(SaveTapResultCode)

    struct SaveTapResult {
        SaveTapResultCode code { SaveTapResultCode::Success };
        QString details;

        bool succeeded() const { return code == SaveTapResultCode::Success; }
    };

    static WaveformParser* instance();

    void parse(uint chNum);
    SaveTapResult saveTap(uint chNum, const QString& fileName = QString());
    void saveWaveform(uint chNum);
    QVector<uint8_t> getParsedWaveform(uint chNum) const;
    QPair<QVector<QSharedPointer<ParsedData::DataBlock>>, QVector<bool>> getParsedData(uint chNum) const;
    QSharedPointer<QVector<QSharedPointer<ParsedData::DataBlock>>> getParsedDataSharedPtr(uint chNum) const;

    void repairWaveform3(uint chNum);

    Q_INVOKABLE bool isBlockSelected(uint chNum, int blockNum) const;
    Q_INVOKABLE bool isBlockParseError(uint chNum, int blockNum) const;
    Q_INVOKABLE void setBlockSelected(uint chNum, int blockNum, bool selected);
    Q_INVOKABLE void toggleBlockSelection(uint chNum, int blockNum);
    Q_INVOKABLE void clearBlockSelection(uint chNum);
    Q_INVOKABLE QVariantList selectedBlocks(uint chNum) const;
    Q_INVOKABLE int getBlockDataStart(uint chNum, uint blockNum) const;
    Q_INVOKABLE int getBlockDataEnd(uint chNum, uint blockNum) const;
    Q_INVOKABLE int getPositionByAddress(uint chNum, uint blockNum, uint addr) const;
    Q_INVOKABLE bool startExperimentalDebug(uint chNum);
    Q_INVOKABLE bool nextExperimentalDebugStep();
    Q_INVOKABLE bool inspectExperimentalDebugAt(uint chNum, int sample);
    Q_INVOKABLE void stopExperimentalDebug();
    Q_INVOKABLE QVariantMap experimentalDebugState(uint chNum) const;
    Q_INVOKABLE void cancelParsing();
    Q_INVOKABLE void clearParsingCancellation();
    //getters
    ParsedDataModel* getParsedChannel0() const;
    ParsedDataModel* getParsedChannel1() const;
    bool getParsingActive() const;
    bool getParsingCancellationRequested() const;
    int getParsingProgress() const;
    QString getParsingStatus() const;

signals:
    void parsedChannel0Changed();
    void parsedChannel1Changed();
    void blockSelectionChanged(uint chNum);
    void experimentalDebugChanged(uint chNum);
    void parsingProgressChanged();
};

#endif // WAVEFORMPARSER_H
