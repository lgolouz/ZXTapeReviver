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

SOURCES += \
        sources/controls/waveformcustomization.cpp \
        sources/main.cpp \
        sources/models/fileworkermodel.cpp \
        sources/controls/waveformcontrol.cpp \
        sources/core/waveformparser.cpp \
        sources/core/wavreader.cpp \
        sources/models/parsersettingsmodel.cpp \
        sources/models/suspiciouspointsmodel.cpp \
        sources/util/enummetainfo.cpp

HEADERS += \
    sources/controls/waveformcustomization.h \
    sources/defines.h \
    sources/models/fileworkermodel.h \
    sources/controls/waveformcontrol.h \
    sources/core/waveformparser.h \
    sources/core/wavreader.h \
    sources/models/parsersettingsmodel.h \
    sources/models/suspiciouspointsmodel.h \
    sources/util/enummetainfo.h

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
