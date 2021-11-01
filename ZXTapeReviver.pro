#*******************************************************************************
# ZX Tape Reviver
#-----------------
#
# Author: Leonid Golouz
# E-mail: lgolouz@list.ru
# YouTube channel: https://www.youtube.com/channel/UCz_ktTqWVekT0P4zVW8Xgcg
# YouTube channel e-mail: computerenthusiasttips@mail.ru
#
# Code modification and distribution of any kind is not allowed without direct
# permission of the Author.
#*******************************************************************************

QT += quick gui quickcontrols2

# The following define makes your compiler emit warnings if you use
# any Qt feature that has been marked deprecated (the exact warnings
# depend on your compiler). Refer to the documentation for the
# deprecated API to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

# You can also make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0
CONFIG += c++14

#Dynamically generate translation names and country codes based on AVAILABLE_TRANSLATIONS variable
AVAILABLE_TRANSLATIONS = English:en_US Russian:ru_RU

TRANSLATION_FILENAME = zxtapereviver_
TRANSLATIONS_PATH = ./qml/translations/

for (T, AVAILABLE_TRANSLATIONS) {
    S = $$split(T, ":")
    TRANSLATION_LANGUAGE = $$take_first(S)
    COUNTRY_CODE = $$take_last(S)

    TRANSLATION_ID = ID_$${TRANSLATION_LANGUAGE}_LANGUAGE
    TRANSLATION_ID_UPPER = $$upper($${TRANSLATION_ID})
    TRANSLATION_ID_LOWER = $$lower($${TRANSLATION_ID})
    TRANSLATION_ID_HEADER = "extern const char* $${TRANSLATION_ID_UPPER};"
    TRANSLATION_ID_CODE = "const char* $${TRANSLATION_ID_UPPER}=QT_TRID_NOOP(\"$${TRANSLATION_ID_LOWER}\");"

    isEmpty(DEFINED_TRANSLATIONS) {
        COUNTRY_CODES = $${COUNTRY_CODE}
        DEFINED_TRANSLATIONS = $${T}
    }
    else {
        COUNTRY_CODES = $${COUNTRY_CODES},$${COUNTRY_CODE}
        DEFINED_TRANSLATIONS = $${DEFINED_TRANSLATIONS};$${T}
    }
    TRANSLATION_IDS_HEADER = $${TRANSLATION_IDS_HEADER} $${TRANSLATION_ID_HEADER}
    TRANSLATION_IDS_CODE = $${TRANSLATION_IDS_CODE} $${TRANSLATION_ID_CODE}

    system($$[QT_INSTALL_BINS]/lrelease -idbased $${TRANSLATIONS_PATH}/$${TRANSLATION_FILENAME}$${COUNTRY_CODE}.xlf -qm $${TRANSLATIONS_PATH}/$${TRANSLATION_FILENAME}$${COUNTRY_CODE}.qm)
}
DEFINES += AVAILABLE_TRANSLATIONS=\\\"$${DEFINED_TRANSLATIONS}\\\" \
           COUNTRY_CODES=$${COUNTRY_CODES}

TRANSLATIONS_GENERATED_FILENAME = $${PWD}/generated/translations_generated
TRANSLATIONS_GENERATED_FILENAME_H = $${TRANSLATIONS_GENERATED_FILENAME}.h
TRANSLATIONS_GENERATED_FILENAME_CPP = $${TRANSLATIONS_GENERATED_FILENAME}.cpp

write_file($${TRANSLATIONS_GENERATED_FILENAME_H}, TRANSLATION_IDS_HEADER)
write_file($${TRANSLATIONS_GENERATED_FILENAME_CPP}, TRANSLATION_IDS_CODE)


DEFINES += TRANSLATION_IDS_HEADER="\\\"$${TRANSLATIONS_GENERATED_FILENAME_H}\\\"" \
           TRANSLATION_IDS_CODE="\\\"$${TRANSLATIONS_GENERATED_FILENAME_CPP}\\\""


SOURCES += \
        sources/main.cpp \
        sources/models/fileworkermodel.cpp \
        sources/controls/waveformcontrol.cpp \
        sources/core/waveformparser.cpp \
        sources/core/wavreader.cpp \
        sources/models/parsersettingsmodel.cpp \
        sources/models/suspiciouspointsmodel.cpp \
        sources/translations/translationmanager.cpp \
        sources/translations/translations.cpp \
        sources/util/enummetainfo.cpp \
        sources/configuration/configurationmanager.cpp

HEADERS += \
    sources/defines.h \
    sources/models/fileworkermodel.h \
    sources/controls/waveformcontrol.h \
    sources/core/waveformparser.h \
    sources/core/wavreader.h \
    sources/models/parsersettingsmodel.h \
    sources/models/suspiciouspointsmodel.h \
    sources/translations/translationmanager.h \
    sources/translations/translations.h \
    sources/util/enummetainfo.h \
    sources/configuration/configurationmanager.h

RESOURCES += qml/qml.qrc

RC_ICONS = icon.ico

# Additional import path used to resolve QML modules in Qt Creator's code model
QML_IMPORT_PATH =

# Additional import path used to resolve QML modules just for Qt Quick Designer
QML_DESIGNER_IMPORT_PATH =

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target
