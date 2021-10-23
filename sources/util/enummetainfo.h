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

#ifndef ENUMMETAINFO_H
#define ENUMMETAINFO_H

#include <QString>
#include <QMetaEnum>

class EnumMetaInfo
{
protected:
    EnumMetaInfo() = default;
    virtual ~EnumMetaInfo() = default;
    EnumMetaInfo(const EnumMetaInfo& other) = delete;
    EnumMetaInfo(EnumMetaInfo&& other) = delete;
    EnumMetaInfo& operator= (const EnumMetaInfo& other) = delete;
    EnumMetaInfo& operator= (EnumMetaInfo&& other) = delete;

    template <typename T, typename O = QString> static O getEnumName(T state) {
        auto s = QMetaEnum::fromType<T>().valueToKey(static_cast<int>(state));
        return s ? s : "Undefined";
    }
};

#endif // ENUMMETAINFO_H
