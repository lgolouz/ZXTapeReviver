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

#ifndef EDITSAMPLEACTION_H
#define EDITSAMPLEACTION_H

#include "actionbase.h"

struct EditSampleActionParams {
    QWavVectorType previousValue;
    QWavVectorType newValue;
    int sample;
};

class EditSampleAction : public ActionBase
{
    const EditSampleActionParams m_params;

public:
    EditSampleAction(int channel, const EditSampleActionParams& params);
    virtual ~EditSampleAction() = default;

    virtual bool apply() override;
    virtual void undo() override;

private:
    virtual bool isActionValid(const QSharedPointer<QWavVector>& wf) const;
};

#endif // EDITSAMPLEACTION_H
