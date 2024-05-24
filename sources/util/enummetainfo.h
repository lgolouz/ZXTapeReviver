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

    template <typename T> static QString getEnumName(T state) {
        const auto* s = QMetaEnum::fromType<T>().valueToKey(static_cast<int>(state));
        return s ? s : "Undefined";
    }

    template <typename T> static QString getEnumRoleName(T state) {
        const auto* s = QMetaEnum::fromType<T>().valueToKey(static_cast<int>(state));
        return s ? enumNameToRoleName(s) : QString { "invalidRoleName" };
    }

    template <typename T> T static getEnumValue(const QString& name, T def = T { }) {
        bool ok;
        auto v = static_cast<T>(QMetaEnum::fromType<T>().keyToValue(name.toStdString().c_str(), &ok));
        return ok ? v : def;
    }

    static QString enumNameToRoleName(const char* n);
};

#endif // ENUMMETAINFO_H
