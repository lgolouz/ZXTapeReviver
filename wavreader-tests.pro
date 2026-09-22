include(ZXTapeReviver.pro)

TARGET = wavreader-tests
QT += testlib
CONFIG += console testcase
CONFIG -= app_bundle
SOURCES -= sources/main.cpp
SOURCES += tests/wavreader_test.cpp
