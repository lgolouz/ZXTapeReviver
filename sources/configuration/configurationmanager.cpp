#include "configurationmanager.h"
#include <QFileInfo>
#include <QSettings>

//INI Value helper classes
ConfigurationManager::INIQColorValue::INIQColorValue(QColor& val) : INIValueTypedBase(val)
{

}

QVariant ConfigurationManager::INIQColorValue::getValue() const {
    return m_val.name(m_val.alpha() == UINT8_MAX ? QColor::HexRgb : QColor::HexArgb).toUpper();
}

void ConfigurationManager::INIQColorValue::setValue(const QVariant& val) {
    m_val = val.toString();
}

ConfigurationManager::INIUIntValue::INIUIntValue(unsigned& val) : INIValueTypedBase(val)
{

}

void ConfigurationManager::INIUIntValue::setValue(const QVariant& val) {
    m_val = val.toUInt();
}

ConfigurationManager::INIBoolValue::INIBoolValue(bool& val) : INIValueTypedBase(val)
{

}

void ConfigurationManager::INIBoolValue::setValue(const QVariant& val) {
    m_val = val.toBool();
}

ConfigurationManager::INITranslationLanguageValue::INITranslationLanguageValue(TranslationManager::TranslationLanguages& val) : INIValueTypedBase(val)
{

}

QVariant ConfigurationManager::INITranslationLanguageValue::getValue() const {
    return getEnumName(m_val);
}

void ConfigurationManager::INITranslationLanguageValue::setValue(const QVariant& val) {
    m_val = getEnumValue<TranslationManager::TranslationLanguages>(val.toString(), TranslationManager::TranslationLanguages::en_US);
}


ConfigurationManager::CustomizationBase::CustomizationBase(const QMap<INISections, QList<QPair<INIKeys, std::shared_ptr<INIValueBase>>>>& sections) :
    m_ini(sections)
{

}

const QMap<ConfigurationManager::INISections, QList<QPair<ConfigurationManager::INIKeys, std::shared_ptr<ConfigurationManager::INIValueBase>>>>&  ConfigurationManager::CustomizationBase::getSections() const {
    return m_ini;
}

ConfigurationManager::WaveformCustomization::WaveformCustomization() :
    ConfigurationManager::CustomizationBase(m_waveformini),
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
    m_waveformini({
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

}

const QColor& ConfigurationManager::WaveformCustomization::operationModeBgColor() const {
    return m_operationModeBgColor;
}

const QColor& ConfigurationManager::WaveformCustomization::selectioModeBgColor() const {
    return m_selectionModeBgColor;
}

const QColor& ConfigurationManager::WaveformCustomization::measurementModeBgColor() const {
    return m_measurementModeBgColor;
}

const QColor& ConfigurationManager::WaveformCustomization::rangeSelectionColor() const {
    return m_rangeSelectionColor;
}

const QColor& ConfigurationManager::WaveformCustomization::xAxisColor() const {
    return m_xAxisColor;
}

const QColor& ConfigurationManager::WaveformCustomization::yAxisColor() const {
    return m_yAxisColor;
}

const QColor& ConfigurationManager::WaveformCustomization::blockStartColor() const {
    return m_blockStartColor;
}

const QColor& ConfigurationManager::WaveformCustomization::blockMarkerColor() const {
    return m_blockMarkerColor;
}

const QColor& ConfigurationManager::WaveformCustomization::blockEndColor() const {
    return m_blockEndColor;
}

const QColor& ConfigurationManager::WaveformCustomization::textColor() const {
    return m_textColor;
}

const QColor& ConfigurationManager::WaveformCustomization::wavePositiveColor() const {
    return m_wavePositiveColor;
}

const QColor& ConfigurationManager::WaveformCustomization::waveNegativeColor() const {
    return m_waveNegativeColor;
}

unsigned ConfigurationManager::WaveformCustomization::waveLineThickness() const {
    return m_waveLineThickness;
}

unsigned ConfigurationManager::WaveformCustomization::circleRadius() const {
    return m_circleRadius;
}

bool ConfigurationManager::WaveformCustomization::checkVerticalRange() const {
    return m_checkVerticalRange;
}


ConfigurationManager::ApplicationCustomization::ApplicationCustomization() :
    ConfigurationManager::CustomizationBase(m_applicationini),
    m_translationLanguage(TranslationManager::TranslationLanguages::en_US),
    m_applicationini({
        { INISections::TRANSLATION, {
              qMakePair(INIKeys::language, std::make_shared<INITranslationLanguageValue>(m_translationLanguage))
        } }
    })
{

}

TranslationManager::TranslationLanguages ConfigurationManager::ApplicationCustomization::translationLanguage() const {
    return m_translationLanguage;
}

bool ConfigurationManager::ApplicationCustomization::setTranslationLanguage(TranslationManager::TranslationLanguages lng) {
    m_translationLanguage = lng;
    return true;
}


//ConfigurationManager class
ConfigurationManager::ConfigurationManager(QObject* parent) :
    QObject(parent),
    m_customizations({ &m_waveformCustomization, &m_applicationCustomization }),
    m_configurationFile("config.ini")
{
    // UI settings
    QFileInfo ini_file(m_configurationFile);

    if (!ini_file.exists() || !ini_file.isFile()) {
        // If no INI file - create new one
        writeConfiguration();
    } else {
        QSettings ini(ini_file.fileName(), QSettings::IniFormat);
        // Read settings from INI file
        for (const auto* customization: m_customizations) {
            for (auto it = customization->getSections().constKeyValueBegin(); it != customization->getSections().constKeyValueEnd(); ++it) {
                for (auto& key: it->second) {
                    key.second->setValue(ini.value(getSettingKey(it->first, key.first)));
                }
            }
        }
    }
}

void ConfigurationManager::writeConfiguration() {
    QSettings ini(m_configurationFile, QSettings::IniFormat);

    for (const auto* customization: m_customizations) {
        for (auto it = customization->getSections().constKeyValueBegin(); it != customization->getSections().constKeyValueEnd(); ++it) {
            for (const auto& key: it->second) {
                ini.setValue(getSettingKey(it->first, key.first), key.second->getValue());
            }
        }
    }
}

ConfigurationManager::~ConfigurationManager()
{
    writeConfiguration();
}

QString ConfigurationManager::getZxTapeReviverVersion() const {
    const auto split { getZxTapeReviverBuildTag().split('_', Qt::SkipEmptyParts) };
    return split.isEmpty() ? QString() : split.last();
}

QString ConfigurationManager::getZxTapeReviverBuildTag() const {
    return ZXTAPEREVIVER_VERSION;
}

QString ConfigurationManager::getSettingKey(INISections section, INIKeys key) const {
    return getEnumName(section) + "/" + getEnumName(key);
}

ConfigurationManager::WaveformCustomization* ConfigurationManager::getWaveformCustomization() {
    return &m_waveformCustomization;
}

ConfigurationManager::ApplicationCustomization* ConfigurationManager::getApplicationCustomization() {
    return &m_applicationCustomization;
}

ConfigurationManager* ConfigurationManager::instance() {
    static ConfigurationManager manager;
    return &manager;
}
