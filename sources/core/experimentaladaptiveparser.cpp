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
#include <algorithm>

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
