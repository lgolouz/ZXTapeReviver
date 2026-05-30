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

#include <QVariantMap>
#include <QVector>
#include "sources/models/parsersettingsmodel.h"

class ExperimentalAdaptiveParser
{
public:
    static QVector<QVariantMap> selectAlternatives(QVector<QVariantMap> candidates,
                                                   ParserSettingsModel::AdaptiveAlternativeMode mode,
                                                   qsizetype maxAlternatives);
    static bool shouldRunBeamInParallel(qsizetype beamSize);
};

#endif // EXPERIMENTALADAPTIVEPARSER_H
