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


INCLUDEPATH += $$PWD/wavelib/src \
               $$PWD/wavelib/header
                  
SOURCES += \
    $$PWD/wavelib/src/conv.c \
    $$PWD/wavelib/src/cwt.c \
    $$PWD/wavelib/src/cwtmath.c \
    $$PWD/wavelib/src/hsfft.c \
    $$PWD/wavelib/src/real.c \
    $$PWD/wavelib/src/wavefilt.c \
    $$PWD/wavelib/src/wavefunc.c \
    $$PWD/wavelib/src/wavelib.c \
    $$PWD/wavelib/src/wtmath.c

HEADERS += \
    $$PWD/wavelib/src/conv.h \
    $$PWD/wavelib/src/cwt.h \
    $$PWD/wavelib/src/cwtmath.h \
    $$PWD/wavelib/src/hsfft.h \
    $$PWD/wavelib/src/real.h \
    $$PWD/wavelib/src/wavefilt.h \
    $$PWD/wavelib/src/wavefunc.h \
    $$PWD/wavelib/src/wtmath.h \
    $$PWD/wavelib/header/wavelib.h