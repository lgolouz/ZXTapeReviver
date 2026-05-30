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

#ifndef EXPERIMENTALADAPTIVEPARSER_H
#define EXPERIMENTALADAPTIVEPARSER_H

#include <functional>
#include <QVariantMap>
#include <QVector>
#include "sources/core/parseddata.h"
#include "sources/core/wavreader.h"
#include "sources/models/parsersettingsmodel.h"

class ExperimentalAdaptiveParser
{
public:
    struct Context {
        uint chNum { 0 };
        const QWavVector* channel { nullptr };
        const QVector<ParsedData::WaveformPart>* parsed { nullptr };
        ParsedData* parsedData { nullptr };
        const ParserSettingsModel::ParserSettings* parserSettings { nullptr };
        double sampleRate { 0.0 };
        std::function<bool()> isCancellationRequested;
        std::function<void(size_t, const QString&)> updateProgress;
        std::function<void(size_t, const QVariantMap&, bool)> publishDebug;
        std::function<double(size_t, size_t, double*, double*, double*)> scoreWindow;
    };

    struct Result {
        size_t lastSample { 0 };
        bool stopParsing { false };
    };

    explicit ExperimentalAdaptiveParser(Context context);

    Result parsePayload(qsizetype startPartIndex);

    static QVector<QVariantMap> selectAlternatives(QVector<QVariantMap> candidates,
                                                   ParserSettingsModel::AdaptiveAlternativeMode mode,
                                                   qsizetype maxAlternatives);
    static bool shouldRunBeamInParallel(qsizetype beamSize);

private:
    struct BitCandidate {
        bool valid { false };
        uint8_t bit { 0 };
        size_t begin { 0 };
        size_t end { 0 };
        double score { 0.0 };
        qsizetype firstPartIndex { -1 };
        qsizetype secondPartIndex { -1 };
    };

    struct PilotRun {
        bool found { false };
        size_t begin { 0 };
        size_t end { 0 };
        qsizetype halfWaveCount { 0 };
    };

    struct SignalWindow {
        bool pause { false };
        double range { 0.0 };
        double rms { 0.0 };
        double rangeRatio { 1.0 };
        double rmsRatio { 1.0 };
    };

    bool isSineNormal(const ParsedData::WaveformPart& b, const ParsedData::WaveformPart& e, bool zeroCheck) const;
    bool isPilotHalfFreq(const ParsedData::WaveformPart& part) const;
    PilotRun findNextPilotRun(size_t sample, double referenceRange) const;
    SignalWindow analyzeSignalWindow(size_t sample, double referenceRange) const;
    QVector<QVariantMap> buildHalfWaveCandidates(qsizetype partIndex,
                                                 double candidateSpeedRatio,
                                                 int zeroExpectedLength,
                                                 int oneExpectedLength,
                                                 int classificationBoundary) const;
    QVariantMap buildHalfWaveCandidate(qsizetype partIndex,
                                       double candidateSpeedRatio,
                                       int zeroExpectedLength,
                                       int oneExpectedLength,
                                       int classificationBoundary) const;
    BitCandidate periodCandidateToBitCandidate(const QVariantMap& periodCandidate) const;

    Context m_context;
};

#endif // EXPERIMENTALADAPTIVEPARSER_H
