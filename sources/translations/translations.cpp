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

#include "translations.h"
#include <QtGlobal>

#include TRANSLATION_IDS_CODE

const char* ID_TIMELINE_SEC          = QT_TRID_NOOP("id_timeline_sec");
const char* ID_OK                    = QT_TRID_NOOP("id_ok");
const char* ID_ERROR                 = QT_TRID_NOOP("id_error");
const char* ID_UNKNOWN               = QT_TRID_NOOP("id_unknown");
const char* ID_HEADER                = QT_TRID_NOOP("id_header");
const char* ID_CODE                  = QT_TRID_NOOP("id_code");
const char* ID_EDIT_ACTION           = QT_TRID_NOOP("id_edit_action");
const char* ID_SHIFT_WAVEFORM_ACTION = QT_TRID_NOOP("id_shift_waveform_action");
const char* ID_PARITY_MESSAGE        = QT_TRID_NOOP("id_parity_message");
const char* ID_BLOCK_NUMBER          = QT_TRID_NOOP("id_block_number");
const char* ID_BLOCK_TYPE            = QT_TRID_NOOP("id_block_type");
const char* ID_BLOCK_NAME            = QT_TRID_NOOP("id_block_name");
const char* ID_BLOCK_SIZE            = QT_TRID_NOOP("id_block_size");
const char* ID_BLOCK_STATUS          = QT_TRID_NOOP("id_block_status");

Translations* Translations::instance() {
    static Translations t;
    return &t;
}

Translations::Translations()
{
    retranslate();
}

void Translations::retranslate()
{
    id_header = qtTrId(ID_HEADER);
    id_code = qtTrId(ID_CODE);
    id_ok = qtTrId(ID_OK);
    id_error = qtTrId(ID_ERROR);
    id_unknown = qtTrId(ID_UNKNOWN);
    id_timeline_sec = qtTrId(ID_TIMELINE_SEC);
    id_edit_action = qtTrId(ID_EDIT_ACTION);
    id_shift_waveform_action = qtTrId(ID_SHIFT_WAVEFORM_ACTION);
    id_parity_message = qtTrId(ID_PARITY_MESSAGE);
    id_block_number = qtTrId(ID_BLOCK_NUMBER);
    id_block_type = qtTrId(ID_BLOCK_TYPE);
    id_block_name = qtTrId(ID_BLOCK_NAME);
    id_block_size = qtTrId(ID_BLOCK_SIZE);
    id_block_status = qtTrId(ID_BLOCK_STATUS);
}
