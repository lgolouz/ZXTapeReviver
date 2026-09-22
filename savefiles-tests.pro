include(ZXTapeReviver.pro)

TARGET = savefiles-tests
QT += testlib
CONFIG += console testcase
CONFIG -= app_bundle
SOURCES -= sources/main.cpp
SOURCES += tests/savefiles_test.cpp
