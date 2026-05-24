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
    QString id_header;
    QString id_code;
    QString id_ok;
    QString id_error;
    QString id_unknown;
    QString id_timeline_sec;
    QString id_edit_action;
    QString id_shift_waveform_action;
    QString id_parity_message;
    QString id_block_number;
    QString id_block_type;
    QString id_block_name;
    QString id_block_size;
    QString id_block_status;

    static Translations* instance();
    void retranslate();

    ~Translations() = default;

protected:
    Translations();

    Translations(const Translations& t) = delete;
    Translations(Translations&& t) = delete;
    Translations& operator= (const Translations& t) = delete;
    Translations& operator= (Translations&& t) = delete;
};

#endif // TRANSLATIONS_H
