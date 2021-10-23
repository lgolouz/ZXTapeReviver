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

#include "waveformcustomization.h"
#include <QFileInfo>
#include <QSettings>

//INI Value helper classes
WaveformCustomization::INIQColorValue::INIQColorValue(QColor& val) : INIValueTypedBase(val)
{

}

QVariant WaveformCustomization::INIQColorValue::getValue() const {
    return m_val.name(m_val.alpha() == UINT8_MAX ? QColor::HexRgb : QColor::HexArgb).toUpper();
}

void WaveformCustomization::INIQColorValue::setValue(const QVariant& val) {
    m_val = val.toString();
}

WaveformCustomization::INIUIntValue::INIUIntValue(unsigned& val) : INIValueTypedBase(val)
{

}

void WaveformCustomization::INIUIntValue::setValue(const QVariant& val) {
    m_val = val.toUInt();
}

WaveformCustomization::INIBoolValue::INIBoolValue(bool& val) : INIValueTypedBase(val)
{

}

void WaveformCustomization::INIBoolValue::setValue(const QVariant& val) {
    m_val = val.toBool();
}

//Waveform customization class
WaveformCustomization::WaveformCustomization(QObject* parent) :
    QObject(parent),
    m_operationModeBgColor(7, 7, 36),
    m_selectionModeBgColor(37, 37, 37),
    m_measurementModeBgColor(0, 17, 17),
    m_rangeSelectionColor(7, 7, 137, 128),
    m_xAxisColor(11, 60, 0),
    m_yAxisColor(11, 60, 0),
    m_blockStartColor(0, 0, 250),
    m_blockMarkerColor(0, 0, 250),
    m_blockEndColor(255, 242, 0),
    m_wavePositiveColor(50, 150, 0),
    m_waveNegativeColor(200, 0 , 0),
    m_textColor(255, 255, 255),
    m_waveLineThickness(1),
    m_circleRadius(2),
    m_checkVerticalRange(true),
    m_ini({
            { INISections::COLOR, {
                  qMakePair(INIKeys::operationModeBgColor, std::make_shared<INIQColorValue>(m_operationModeBgColor)),
                  qMakePair(INIKeys::selectionModeBgColor, std::make_shared<INIQColorValue>(m_selectionModeBgColor)),
                  qMakePair(INIKeys::measurementModeBgColor, std::make_shared<INIQColorValue>(m_measurementModeBgColor)),
                  qMakePair(INIKeys::rangeSelectionColor, std::make_shared<INIQColorValue>(m_rangeSelectionColor)),
                  qMakePair(INIKeys::xAxisColor, std::make_shared<INIQColorValue>(m_xAxisColor)),
                  qMakePair(INIKeys::yAxisColor, std::make_shared<INIQColorValue>(m_yAxisColor)),
                  qMakePair(INIKeys::blockStartColor, std::make_shared<INIQColorValue>(m_blockStartColor)),
                  qMakePair(INIKeys::blockMarkerColor, std::make_shared<INIQColorValue>(m_blockMarkerColor)),
                  qMakePair(INIKeys::blockEndColor, std::make_shared<INIQColorValue>(m_blockEndColor)),
                  qMakePair(INIKeys::wavePositiveColor, std::make_shared<INIQColorValue>(m_wavePositiveColor)),
                  qMakePair(INIKeys::waveNegativeColor, std::make_shared<INIQColorValue>(m_waveNegativeColor)),
                  qMakePair(INIKeys::textColor, std::make_shared<INIQColorValue>(m_textColor))
            } },
            { INISections::STYLE, {
                  qMakePair(INIKeys::waveLineThickness, std::make_shared<INIUIntValue>(m_waveLineThickness)),
                  qMakePair(INIKeys::circleRadius, std::make_shared<INIUIntValue>(m_circleRadius))
            } },
            { INISections::BEHAVIOR, {
                  qMakePair(INIKeys::checkVerticalRange, std::make_shared<INIBoolValue>(m_checkVerticalRange))
            } }
          })
{
    // UI settings
    QFileInfo ini_file("config.ini");
    QSettings ini(ini_file.fileName(), QSettings::IniFormat);

    if (!ini_file.exists() || !ini_file.isFile()) {
        // If no INI file - create new one
        for (auto it = m_ini.constKeyValueBegin(); it != m_ini.constKeyValueEnd(); ++it) {
            for (const auto& key: it->second) {
                ini.setValue(getSettingKey(it->first, key.first), key.second->getValue());
            }
        }
    } else {
        // Read settings from INI file
        for (auto it = m_ini.constKeyValueBegin(); it != m_ini.constKeyValueEnd(); ++it) {
            for (auto& key: it->second) {
                key.second->setValue(ini.value(getSettingKey(it->first, key.first)));
            }
        }
    }
}

QString WaveformCustomization::getSettingKey(INISections section, INIKeys key) const {
    return getEnumName(section) + "/" + getEnumName(key);
}


const QColor& WaveformCustomization::operationModeBgColor() const {
    return m_operationModeBgColor;
}

const QColor& WaveformCustomization::selectioModeBgColor() const {
    return m_selectionModeBgColor;
}

const QColor& WaveformCustomization::measurementModeBgColor() const {
    return m_measurementModeBgColor;
}

const QColor& WaveformCustomization::rangeSelectionColor() const {
    return m_rangeSelectionColor;
}

const QColor& WaveformCustomization::xAxisColor() const {
    return m_xAxisColor;
}

const QColor& WaveformCustomization::yAxisColor() const {
    return m_yAxisColor;
}

const QColor& WaveformCustomization::blockStartColor() const {
    return m_blockStartColor;
}

const QColor& WaveformCustomization::blockMarkerColor() const {
    return m_blockMarkerColor;
}

const QColor& WaveformCustomization::blockEndColor() const {
    return m_blockEndColor;
}

const QColor& WaveformCustomization::textColor() const {
    return m_textColor;
}

const QColor& WaveformCustomization::wavePositiveColor() const {
    return m_wavePositiveColor;
}

const QColor& WaveformCustomization::waveNegativeColor() const {
    return m_waveNegativeColor;
}

unsigned WaveformCustomization::waveLineThickness() const {
    return m_waveLineThickness;
}

unsigned WaveformCustomization::circleRadius() const {
    return m_circleRadius;
}

bool WaveformCustomization::checkVerticalRange() const {
    return m_checkVerticalRange;
}
