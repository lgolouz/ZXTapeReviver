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

#ifndef CENTERWAVEFORMACTION_H
#define CENTERWAVEFORMACTION_H

#include "actionbase.h"
#include "sources/models/parsersettingsmodel.h"

#include <QVector>

struct CenterWaveformActionParams {
    uint32_t sampleRate;
    ParserSettingsModel::AdaptiveVirtualAxisMode axisMode;
};

class CenterWaveformAction : public ActionBase
{
    const CenterWaveformActionParams m_params;
    size_t m_bucketSize { 1 };
    QVector<QWavVectorType> m_axisBuckets;

    QWavVectorType axisAt(size_t sample) const;
    bool buildAxis(const QWavVector& waveform);
    void applyAxis(QWavVector& waveform, QWavVectorType sign) const;

public:
    CenterWaveformAction(int channel, const CenterWaveformActionParams& params);
    virtual ~CenterWaveformAction() = default;

    virtual bool apply() override;
    virtual void undo() override;
};

#endif // CENTERWAVEFORMACTION_H
