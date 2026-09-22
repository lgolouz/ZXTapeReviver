include(ZXTapeReviver.pro)

TARGET = actions-tests
QT += testlib
CONFIG += console testcase
CONFIG -= app_bundle
SOURCES -= sources/main.cpp
SOURCES += tests/actions_test.cpp
