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

#ifndef TRANSLATIONS_H
#define TRANSLATIONS_H

#include <QString>

#include TRANSLATION_IDS_HEADER

extern const char* ID_TIMELINE_SEC;
extern const char* ID_OK;
extern const char* ID_ERROR;
extern const char* ID_UNKNOWN;
extern const char* ID_HEADER;
extern const char* ID_CODE;
extern const char* ID_EDIT_ACTION;
extern const char* ID_SHIFT_WAVEFORM_ACTION;
extern const char* ID_PARITY_MESSAGE;
extern const char* ID_BLOCK_NUMBER;
extern const char* ID_BLOCK_TYPE;
extern const char* ID_BLOCK_NAME;
extern const char* ID_BLOCK_SIZE;
extern const char* ID_BLOCK_STATUS;

struct Translations final {
    const QString id_header { qtTrId(ID_HEADER) };
    const QString id_code { qtTrId(ID_CODE) };
    const QString id_ok { qtTrId(ID_OK) };
    const QString id_error { qtTrId(ID_ERROR) };
    const QString id_unknown { qtTrId(ID_UNKNOWN) };
    const QString id_timeline_sec { qtTrId(ID_TIMELINE_SEC) };
    const QString id_edit_action { qtTrId(ID_EDIT_ACTION) };
    const QString id_shift_waveform_action { qtTrId(ID_SHIFT_WAVEFORM_ACTION) };
    const QString id_parity_message { qtTrId (ID_PARITY_MESSAGE) };
    const QString id_block_number { qtTrId (ID_BLOCK_NUMBER) };
    const QString id_block_type { qtTrId (ID_BLOCK_TYPE) };
    const QString id_block_name { qtTrId (ID_BLOCK_NAME) };
    const QString id_block_size { qtTrId (ID_BLOCK_SIZE) };
    const QString id_block_status { qtTrId (ID_BLOCK_STATUS) };

    static Translations* instance();

    ~Translations() = default;

protected:
    Translations() = default;

    Translations(const Translations& t) = delete;
    Translations(Translations&& t) = delete;
    Translations& operator= (const Translations& t) = delete;
    Translations& operator= (Translations&& t) = delete;
};

#endif // TRANSLATIONS_H
