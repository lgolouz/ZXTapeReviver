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

#include "centerwaveformaction.h"

#include "sources/models/waveformmodel.h"
#include "sources/translations/translations.h"

#include <algorithm>
#include <cmath>

namespace {

QWavVectorType medianOf(QVector<QWavVectorType> values, QWavVectorType fallback = 0.0f)
{
    if (values.empty()) {
        return fallback;
    }

    const auto middle { values.begin() + values.size() / 2 };
    std::nth_element(values.begin(), middle, values.end());
    return *middle;
}

QWavVectorType percentileOf(QVector<QWavVectorType> values, double percentile, QWavVectorType fallback = 0.0f)
{
    if (values.empty()) {
        return fallback;
    }

    const qsizetype index { std::clamp<qsizetype>(static_cast<qsizetype>(std::lround((values.size() - 1) * percentile)), 0, values.size() - 1) };
    const auto it { values.begin() + index };
    std::nth_element(values.begin(), it, values.end());
    return *it;
}

}

CenterWaveformAction::CenterWaveformAction(int channel, const CenterWaveformActionParams& params) :
    ActionBase(channel, Translations::instance()->id_center_waveform_action),
    m_params(params)
{

}

QWavVectorType CenterWaveformAction::axisAt(size_t sample) const
{
    if (m_axisBuckets.empty()) {
        return 0.0f;
    }

    const double bucketPosition { static_cast<double>(sample) / static_cast<double>(m_bucketSize) };
    const auto leftIndex { std::clamp<qsizetype>(static_cast<qsizetype>(std::floor(bucketPosition)), 0, m_axisBuckets.size() - 1) };
    const auto rightIndex { std::min<qsizetype>(leftIndex + 1, m_axisBuckets.size() - 1) };
    const double fraction { bucketPosition - std::floor(bucketPosition) };
    return static_cast<QWavVectorType>(m_axisBuckets.at(leftIndex) * (1.0 - fraction) + m_axisBuckets.at(rightIndex) * fraction);
}

bool CenterWaveformAction::buildAxis(const QWavVector& waveform)
{
    if (waveform.empty() || m_params.sampleRate == 0) {
        return false;
    }

    m_bucketSize = std::max<size_t>(32, static_cast<size_t>(std::lround(m_params.sampleRate / 500.0)));
    const size_t bucketCount { (static_cast<size_t>(waveform.size()) + m_bucketSize - 1) / m_bucketSize };

    QVector<QWavVectorType> medians;
    QVector<QWavVectorType> p35;
    QVector<QWavVectorType> p65;
    medians.reserve(static_cast<qsizetype>(bucketCount));
    p35.reserve(static_cast<qsizetype>(bucketCount));
    p65.reserve(static_cast<qsizetype>(bucketCount));

    for (size_t bucket { 0 }; bucket < bucketCount; ++bucket) {
        const size_t begin { bucket * m_bucketSize };
        const size_t end { std::min<size_t>(static_cast<size_t>(waveform.size()), begin + m_bucketSize) };
        QVector<QWavVectorType> values;
        values.reserve(static_cast<qsizetype>(end - begin));
        for (size_t sample { begin }; sample < end; ++sample) {
            values.append(waveform.at(static_cast<qsizetype>(sample)));
        }

        medians.append(medianOf(values));
        p35.append(percentileOf(values, 0.35));
        p65.append(percentileOf(values, 0.65));
    }

    m_axisBuckets.resize(static_cast<qsizetype>(bucketCount));
    constexpr int c_radius { 2 };
    for (qsizetype bucket { 0 }; bucket < m_axisBuckets.size(); ++bucket) {
        const qsizetype begin { std::max<qsizetype>(0, bucket - c_radius) };
        const qsizetype end { std::min<qsizetype>(m_axisBuckets.size() - 1, bucket + c_radius) };

        switch (m_params.axisMode) {
        case ParserSettingsModel::AdaptiveVirtualAxisPercentile35_65: {
            QWavVectorType lowSum { 0.0f };
            QWavVectorType highSum { 0.0f };
            for (qsizetype index { begin }; index <= end; ++index) {
                lowSum += p35.at(index);
                highSum += p65.at(index);
            }

            const auto count { static_cast<QWavVectorType>(end - begin + 1) };
            m_axisBuckets[bucket] = (lowSum / count + highSum / count) / 2.0f;
            break;
        }

        case ParserSettingsModel::AdaptiveVirtualAxisLowPassMedian:
        case ParserSettingsModel::AdaptiveVirtualAxisMedianWindow: {
            QVector<QWavVectorType> windowValues;
            windowValues.reserve(end - begin + 1);
            for (qsizetype index { begin }; index <= end; ++index) {
                windowValues.append(medians.at(index));
            }
            m_axisBuckets[bucket] = medianOf(windowValues);
            break;
        }
        }
    }

    if (m_params.axisMode == ParserSettingsModel::AdaptiveVirtualAxisLowPassMedian && !m_axisBuckets.empty()) {
        constexpr QWavVectorType c_alpha { 0.18f };
        for (qsizetype index { 1 }; index < m_axisBuckets.size(); ++index) {
            m_axisBuckets[index] = m_axisBuckets.at(index - 1) * (1.0f - c_alpha) + m_axisBuckets.at(index) * c_alpha;
        }
        for (qsizetype index { m_axisBuckets.size() - 2 }; index >= 0; --index) {
            m_axisBuckets[index] = m_axisBuckets.at(index + 1) * (1.0f - c_alpha) + m_axisBuckets.at(index) * c_alpha;
            if (index == 0) {
                break;
            }
        }
    }

    return true;
}

void CenterWaveformAction::applyAxis(QWavVector& waveform, QWavVectorType sign) const
{
    for (size_t sample { 0 }; sample < static_cast<size_t>(waveform.size()); ++sample) {
        waveform[static_cast<qsizetype>(sample)] += sign * axisAt(sample);
    }
}

bool CenterWaveformAction::apply()
{
    auto waveform { WaveFormModel::instance()->getChannel(channel()) };
    const bool valid { isActionValid(waveform) && buildAxis(*waveform) };
    if (valid) {
        applyAxis(*waveform, -1.0f);
    }
    return valid;
}

void CenterWaveformAction::undo()
{
    auto waveform { WaveFormModel::instance()->getChannel(channel()) };
    if (!isActionValid(waveform)) {
        return;
    }

    applyAxis(*waveform, 1.0f);
}
