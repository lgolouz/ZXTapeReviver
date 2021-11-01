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

#ifndef TRANSLATIONMANAGER_H
#define TRANSLATIONMANAGER_H

#include <QObject>
#include <QTranslator>
#include <QScopedPointer>
#include "sources/util/enummetainfo.h"

class TranslationManager : public QObject, private EnumMetaInfo
{
    Q_OBJECT

    Q_PROPERTY(QString translationChanged READ getTranslationChanged NOTIFY translationChanged)
    Q_PROPERTY(QVariantList languages READ getLanguages CONSTANT)

    QScopedPointer<QTranslator> m_translator;

    QVariantList getAvailableTranslations() const;
    QString getTranslationName() const;
    void installTranslator(bool firstRun = false);

public:
    enum TranslationLanguages {
        COUNTRY_CODES
    };
    Q_ENUM(TranslationLanguages)

    TranslationManager(QObject* parent = nullptr);
    virtual ~TranslationManager() = default;

    TranslationManager(const TranslationManager& other) = delete;
    TranslationManager(TranslationManager&& other) = delete;
    TranslationManager& operator= (const TranslationManager& other) = delete;
    TranslationManager& operator= (TranslationManager&& other) = delete;

    QString getTranslationChanged() const;
    Q_INVOKABLE void setTranslation(TranslationLanguages lng);
    QVariantList getLanguages() const;

    static TranslationManager* instance();

signals:
    void translationChanged();

private:
    TranslationLanguages m_currentLanguage;
};

#endif // TRANSLATIONMANAGER_H
