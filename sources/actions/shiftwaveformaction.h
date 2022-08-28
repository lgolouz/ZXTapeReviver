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

#ifndef SHIFTWAVEFORMACTION_H
#define SHIFTWAVEFORMACTION_H

#include "actionbase.h"

struct ShiftWaveFormActionParams {
    QWavVectorType offsetValue;
};

class ShiftWaveFormAction : public ActionBase
{
    const ShiftWaveFormActionParams m_params;

public:
    ShiftWaveFormAction(int channel, const ShiftWaveFormActionParams& params);
    virtual ~ShiftWaveFormAction() = default;

    virtual bool apply() override;
    virtual void undo() override;
};

#endif // SHIFTWAVEFORMACTION_H
