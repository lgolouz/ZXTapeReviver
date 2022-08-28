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

#include "shiftwaveformaction.h"
#include "sources/models/waveformmodel.h"
#include "sources/translations/translations.h"

ShiftWaveFormAction::ShiftWaveFormAction(int channel, const ShiftWaveFormActionParams& params) :
    ActionBase(channel, qtTrId(ID_SHIFT_WAVEFORM_ACTION)),
    m_params(params)
{

}

bool ShiftWaveFormAction::apply() {
    auto wf { WaveFormModel::instance()->getChannel(channel()) };
    const bool valid { isActionValid(wf) };
    if (valid) {
        std::for_each(wf->begin(), wf->end(), [this](QWavVectorType& itm) {
            itm += m_params.offsetValue;
        });
    }
    return valid;
}

void ShiftWaveFormAction::undo() {
    auto wf { WaveFormModel::instance()->getChannel(channel()) };
    std::for_each(wf->begin(), wf->end(), [this](QWavVectorType& itm) {
        itm -= m_params.offsetValue;
    });
}
