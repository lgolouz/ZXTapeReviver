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


INCLUDEPATH += $$PWD/dll
LIBS += -L$$PWD/dll -llibfftw3f-3
DEFINES += DLL_IMPORT
