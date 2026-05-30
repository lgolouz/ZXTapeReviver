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

#include "experimentaladaptiveparser.h"
#include "sources/defines.h"
#include "sources/models/suspiciouspointsmodel.h"
#include <algorithm>
#include <atomic>
#include <cmath>
#include <future>
#include <limits>

#define HARDCODED_DATA_SIGNAL_DELTA 0.75

ExperimentalAdaptiveParser::ExperimentalAdaptiveParser(Context context) :
    m_context(std::move(context))
{
}

bool ExperimentalAdaptiveParser::isSineNormal(const ParsedData::WaveformPart& b, const ParsedData::WaveformPart& e, bool zeroCheck) const
{
    const auto& parserSettings { *m_context.parserSettings };
    if (parserSettings.checkForAbnormalSine) {
        return isFreqFitsInDelta(m_context.sampleRate, b.length, zeroCheck ? parserSettings.zeroHalfFreq : parserSettings.oneHalfFreq, zeroCheck ? parserSettings.zeroDelta : parserSettings.oneDelta, parserSettings.sineCheckTolerance) &&
               isFreqFitsInDelta(m_context.sampleRate, e.length, zeroCheck ? parserSettings.zeroHalfFreq : parserSettings.oneHalfFreq, zeroCheck ? parserSettings.zeroDelta : parserSettings.oneDelta, parserSettings.sineCheckTolerance);
    }

    return true;
}

bool ExperimentalAdaptiveParser::isPilotHalfFreq(const ParsedData::WaveformPart& part) const
{
    const auto& parserSettings { *m_context.parserSettings };
    return isFreqFitsInDelta(m_context.sampleRate, part.length, parserSettings.pilotHalfFreq, parserSettings.pilotDelta, 1.0);
}

ExperimentalAdaptiveParser::BitCandidate ExperimentalAdaptiveParser::periodCandidateToBitCandidate(const QVariantMap& periodCandidate) const
{
    BitCandidate result;
    if (periodCandidate["valid"].toBool()) {
        result.valid = true;
        result.bit = static_cast<uint8_t>(periodCandidate["bit"].toInt());
        result.begin = static_cast<size_t>(periodCandidate["begin"].toInt());
        result.end = static_cast<size_t>(periodCandidate["end"].toInt());
        result.score = periodCandidate["score"].toDouble();
        result.firstPartIndex = periodCandidate.value("firstPartIndex", -1).toInt();
        result.secondPartIndex = periodCandidate.value("secondPartIndex", -1).toInt();
    }
    return result;
}

ExperimentalAdaptiveParser::PilotRun ExperimentalAdaptiveParser::findNextPilotRun(size_t sample, double referenceRange) const
{
    const auto& parsed { *m_context.parsed };
    const auto& channel { *m_context.channel };
    constexpr qsizetype c_minPilotHalfWaves { 96 };
    constexpr double c_minPauseSecondsBeforePilot { 0.008 };
    const size_t minPauseSamples { static_cast<size_t>(m_context.sampleRate * c_minPauseSecondsBeforePilot) };
    const size_t maxLookAheadSamples { static_cast<size_t>(m_context.sampleRate * 0.45) };
    auto it { std::lower_bound(parsed.begin(), parsed.end(), sample, [](const ParsedData::WaveformPart& part, size_t value) {
        return part.end < value;
    }) };
    const size_t lookAheadEnd { sample + maxLookAheadSamples };

    const auto hasPauseBeforePilot = [&channel, referenceRange, minPauseSamples, sample](size_t pilotBegin) {
        if (referenceRange <= 0.0 || pilotBegin <= sample || pilotBegin - sample < minPauseSamples) {
            return false;
        }

        const size_t windowBegin { pilotBegin - minPauseSamples };
        double minValue { channel.at(static_cast<qsizetype>(windowBegin)) };
        double maxValue { minValue };
        double sum { 0.0 };
        for (size_t pos { windowBegin }; pos < pilotBegin; ++pos) {
            const double value { channel.at(static_cast<qsizetype>(pos)) };
            minValue = std::min(minValue, value);
            maxValue = std::max(maxValue, value);
            sum += value;
        }

        const double mean { sum / static_cast<double>(minPauseSamples) };
        double squareSum { 0.0 };
        for (size_t pos { windowBegin }; pos < pilotBegin; ++pos) {
            const double diff { channel.at(static_cast<qsizetype>(pos)) - mean };
            squareSum += diff * diff;
        }

        const double rangeRatio { (maxValue - minValue) / referenceRange };
        const double rmsRatio { std::sqrt(squareSum / static_cast<double>(minPauseSamples)) / referenceRange };
        return rangeRatio < 0.30 && rmsRatio < 0.11;
    };

    while (it != parsed.end() && it->begin <= lookAheadEnd) {
        it = std::find_if(it, parsed.end(), [this, lookAheadEnd](const ParsedData::WaveformPart& part) {
            return part.begin <= lookAheadEnd && isPilotHalfFreq(part);
        });
        if (it == parsed.end() || it->begin > lookAheadEnd) {
            break;
        }

        const auto pilotBeginIt { it };
        auto pilotEndIt { it };
        for (; pilotEndIt != parsed.end() && isPilotHalfFreq(*pilotEndIt); ++pilotEndIt) {
        }

        const qsizetype halfWaveCount { std::distance(pilotBeginIt, pilotEndIt) };
        if (halfWaveCount >= c_minPilotHalfWaves && hasPauseBeforePilot(pilotBeginIt->begin)) {
            return PilotRun {
                true,
                pilotBeginIt->begin,
                std::prev(pilotEndIt)->end,
                halfWaveCount
            };
        }

        it = pilotEndIt;
    }

    return {};
}

ExperimentalAdaptiveParser::SignalWindow ExperimentalAdaptiveParser::analyzeSignalWindow(size_t sample, double referenceRange) const
{
    const auto& channel { *m_context.channel };
    if (referenceRange <= 0.0 || sample >= static_cast<size_t>(channel.size())) {
        return {};
    }

    const size_t windowLength {
        std::min<size_t>(
            static_cast<size_t>(std::max(16.0, m_context.sampleRate * 0.002)),
            static_cast<size_t>(channel.size()) - sample)
    };
    if (windowLength < 16) {
        return {};
    }

    double minValue { channel.at(static_cast<qsizetype>(sample)) };
    double maxValue { minValue };
    double sum { 0.0 };
    for (size_t offset { 0 }; offset < windowLength; ++offset) {
        const double value { channel.at(static_cast<qsizetype>(sample + offset)) };
        minValue = std::min(minValue, value);
        maxValue = std::max(maxValue, value);
        sum += value;
    }

    const double mean { sum / static_cast<double>(windowLength) };
    double squareSum { 0.0 };
    for (size_t offset { 0 }; offset < windowLength; ++offset) {
        const double value { channel.at(static_cast<qsizetype>(sample + offset)) - mean };
        squareSum += value * value;
    }

    SignalWindow result;
    result.range = maxValue - minValue;
    result.rms = std::sqrt(squareSum / static_cast<double>(windowLength));
    result.rangeRatio = result.range / referenceRange;
    result.rmsRatio = result.rms / referenceRange;
    result.pause = result.rangeRatio < 0.22 && result.rmsRatio < 0.08;
    return result;
}

QVector<QVariantMap> ExperimentalAdaptiveParser::selectAlternatives(QVector<QVariantMap> candidates,
                                                                     ParserSettingsModel::AdaptiveAlternativeMode mode,
                                                                     qsizetype maxAlternatives)
{
    std::sort(candidates.begin(), candidates.end(), [](const QVariantMap& left, const QVariantMap& right) {
        return left.value("score").toDouble() > right.value("score").toDouble();
    });

    if (mode == ParserSettingsModel::AdaptiveFullAlternatives) {
        if (candidates.size() > maxAlternatives) {
            candidates.resize(maxAlternatives);
        }
        return candidates;
    }

    QVector<QVariantMap> result;
    const auto appendUnique = [&result, maxAlternatives](const QVariantMap& candidate) {
        if (result.size() >= maxAlternatives) {
            return;
        }

        for (const auto& existingCandidate: result) {
            if (existingCandidate.value("bit").toInt() == candidate.value("bit").toInt() &&
                    existingCandidate.value("secondPartIndex").toInt() == candidate.value("secondPartIndex").toInt() &&
                    existingCandidate.value("splitPartIndex").toInt() == candidate.value("splitPartIndex").toInt()) {
                return;
            }
        }

        result.append(candidate);
    };

    const auto appendBestBy = [&candidates, &appendUnique](auto predicate) {
        auto it { std::max_element(candidates.cbegin(), candidates.cend(), [&predicate](const QVariantMap& left, const QVariantMap& right) {
            const double leftScore { predicate(left) };
            const double rightScore { predicate(right) };
            return leftScore < rightScore;
        }) };
        if (it != candidates.cend() && predicate(*it) > -1.0e9) {
            appendUnique(*it);
        }
    };

    appendBestBy([](const QVariantMap& candidate) {
        return candidate.value("bit").toInt() == 0 ? candidate.value("score").toDouble() : -1.0e10;
    });
    appendBestBy([](const QVariantMap& candidate) {
        return candidate.value("bit").toInt() == 1 ? candidate.value("score").toDouble() : -1.0e10;
    });
    appendBestBy([](const QVariantMap& candidate) {
        return -candidate.value("length").toDouble() + candidate.value("score").toDouble() * 12.0;
    });
    appendBestBy([](const QVariantMap& candidate) {
        return candidate.value("length").toDouble() + candidate.value("score").toDouble() * 12.0;
    });

    for (const auto& candidate: candidates) {
        appendUnique(candidate);
    }

    return result;
}

bool ExperimentalAdaptiveParser::shouldRunBeamInParallel(qsizetype beamSize)
{
    return beamSize >= 4;
}
