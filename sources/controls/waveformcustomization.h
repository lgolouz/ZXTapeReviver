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

#ifndef WAVEFORMCUSTOMIZATION_H
#define WAVEFORMCUSTOMIZATION_H

#include <QColor>
#include <QObject>
#include <QList>
#include <QPair>
#include <memory>
#include "sources/util/enummetainfo.h"

class WaveformCustomization final : public QObject, protected EnumMetaInfo {
    Q_OBJECT

public:
    enum class INISections {
        COLOR,
        STYLE,
        BEHAVIOR
    };
    Q_ENUM(INISections)

    enum class INIKeys {
        operationModeBgColor,
        selectionModeBgColor,
        measurementModeBgColor,
        rangeSelectionColor,
        xAxisColor,
        yAxisColor,
        blockStartColor,
        blockMarkerColor,
        blockEndColor,
        wavePositiveColor,
        waveNegativeColor,
        textColor,
        waveLineThickness,
        circleRadius,
        checkVerticalRange
    };
    Q_ENUM(INIKeys)

private:
    QColor m_operationModeBgColor;
    QColor m_selectionModeBgColor;
    QColor m_measurementModeBgColor;
    QColor m_rangeSelectionColor;
    QColor m_xAxisColor;
    QColor m_yAxisColor;
    QColor m_blockStartColor;
    QColor m_blockMarkerColor;
    QColor m_blockEndColor;
    QColor m_wavePositiveColor;
    QColor m_waveNegativeColor;
    QColor m_textColor;
    unsigned m_waveLineThickness;
    unsigned m_circleRadius;
    bool m_checkVerticalRange;

    class INIValueBase {
    protected:
        INIValueBase() = default;

    public:
        INIValueBase(const INIValueBase& other) = delete;
        INIValueBase(INIValueBase&& other) = delete;
        INIValueBase& operator= (const INIValueBase& other) = delete;
        INIValueBase& operator= (INIValueBase&& other) = delete;
        virtual ~INIValueBase() = default;

        virtual QVariant getValue() const = 0;
        virtual void setValue(const QVariant& val) = 0;
    };

    template<typename T>
    class INIValueTypedBase : public INIValueBase {
    protected:
        T& m_val;

        INIValueTypedBase(T& t) : m_val(t) { }

    public:
        virtual QVariant getValue() const override {
            return { m_val };
        }
    };

    class INIQColorValue : public INIValueTypedBase<QColor> {
    public:
        INIQColorValue(QColor& val);
        virtual QVariant getValue() const override;
        virtual void setValue(const QVariant& val) override;
    };

    class INIUIntValue : public INIValueTypedBase<unsigned> {
    public:
        INIUIntValue(unsigned& val);
        virtual void setValue(const QVariant& val) override;
    };

    class INIBoolValue : public INIValueTypedBase<bool> {
    public:
        INIBoolValue(bool& val);
        virtual void setValue(const QVariant& val) override;
    };

    QString getSettingKey(INISections section, INIKeys key) const;

    QMap<INISections, QList<QPair<INIKeys, std::shared_ptr<INIValueBase>>>> m_ini;

public:
    WaveformCustomization(QObject* parent = nullptr);
    WaveformCustomization(WaveformCustomization&& other) = delete;
    WaveformCustomization(const WaveformCustomization& other) = delete;

    WaveformCustomization& operator= (const WaveformCustomization& other) = delete;
    WaveformCustomization& operator= (WaveformCustomization&& other) = delete;

    ~WaveformCustomization() = default;

    const QColor& operationModeBgColor() const;
    const QColor& selectioModeBgColor() const;
    const QColor& measurementModeBgColor() const;
    const QColor& rangeSelectionColor() const;
    const QColor& xAxisColor() const;
    const QColor& yAxisColor() const;
    const QColor& blockStartColor() const;
    const QColor& blockMarkerColor() const;
    const QColor& blockEndColor() const;
    const QColor& textColor() const;
    const QColor& wavePositiveColor() const;
    const QColor& waveNegativeColor() const;

    unsigned waveLineThickness() const;
    unsigned circleRadius() const;
    bool checkVerticalRange() const;
};

#endif // WAVEFORMCUSTOMIZATION_H
