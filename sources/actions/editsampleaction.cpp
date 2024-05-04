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

#include "editsampleaction.h"
#include "sources/models/waveformmodel.h"
#include "sources/translations/translations.h"

EditSampleAction::EditSampleAction(int channel, const EditSampleActionParams& params) :
    ActionBase(channel, qtTrId(ID_EDIT_ACTION)),
    m_params(params)
{

}

bool EditSampleAction::apply() {
    auto wf { WaveFormModel::instance()->getChannel(channel()) };
    const bool valid { isActionValid(wf) };
    if (valid) {
        wf->operator[](m_params.sample) = m_params.newValue;
    }

    return valid;
}

void EditSampleAction::undo() {
    auto wf { WaveFormModel::instance()->getChannel(channel()) };
    wf->operator[](m_params.sample) = m_params.previousValue;
}

bool EditSampleAction::isActionValid(const QSharedPointer<QWavVector>& wf) const {
    return ActionBase::isActionValid(wf) && m_params.sample >= 0 && m_params.sample < wf->size();
}
