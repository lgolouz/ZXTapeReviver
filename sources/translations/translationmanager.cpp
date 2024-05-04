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

#include "translationmanager.h"
#include <QCoreApplication>
#include <QUrl>
#include <QDebug>
#include "sources/configuration/configurationmanager.h"

TranslationManager::TranslationManager(QObject* parent) :
    QObject(parent),
    m_translator(new QTranslator()),
    m_currentLanguage(ConfigurationManager::instance()->getApplicationCustomization()->translationLanguage())
{
    installTranslator();
}

QVariantList TranslationManager::getAvailableTranslations() const {
    decltype (getAvailableTranslations()) result;
    const auto translations { QString(AVAILABLE_TRANSLATIONS).split(';', Qt::SkipEmptyParts) };

    for (const auto& t: translations) {
        const auto translationMap { t.split(':', Qt::SkipEmptyParts) };
        const auto language { translationMap.size() > 0 ? translationMap.first() : QString() };
        const auto countryCode { getEnumValue<TranslationManager::TranslationLanguages>(translationMap.size() > 1 ? translationMap.last() : QString()) };
        const auto languageGenerated { QString("id_%1_language").arg(language.toLower()) };
        const auto languageTranslated { qtTrId(languageGenerated.toStdString().c_str()) };

        result.append(QVariantMap({ { "language", languageTranslated == languageGenerated || languageTranslated.isEmpty() ? language : languageTranslated },
                                    { "countryCode", countryCode } }));
    }

    return result;
}

void TranslationManager::installTranslator(bool firstRun) {
    static const QString n_path { ":/translations/translations/" };
    static const QString n_filename { "zxtapereviver_" };

    if (!firstRun) {
        QCoreApplication::removeTranslator(m_translator.data());
    }

    const QString name { n_filename + getTranslationName() };
    if (!m_translator->load(name, n_path)) {
        qDebug() << "Error loading translation!";
        if (!m_translator->load( n_filename + getEnumName(TranslationLanguages::en_US), n_path)) {
            qDebug() << "Error loading default translation!";
        }
    }

    if (!QCoreApplication::installTranslator(m_translator.data())) {
        qDebug() << "Error installing translator!";
    }
}

QString TranslationManager::getTranslationChanged() const {
    return QString();
}

void TranslationManager::setTranslation(TranslationLanguages lng) {
    if (m_currentLanguage != lng) {
        m_currentLanguage = lng;

        auto& cm { *ConfigurationManager::instance() };
        cm.getApplicationCustomization()->setTranslationLanguage(lng);
        cm.writeConfiguration();

        installTranslator();
        emit translationChanged();
    }
}

QString TranslationManager::getTranslationName() const {
    return getEnumName(m_currentLanguage);
}

TranslationManager* TranslationManager::instance() {
    static TranslationManager manager;
    return &manager;
}

QVariantList TranslationManager::getLanguages() const {
    //We have to get available translations every time them asked to support dynamic translation of existing language values
    return getAvailableTranslations();
}
