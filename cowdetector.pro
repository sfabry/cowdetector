QT += core sql serialport
QT -= gui

CONFIG += c++11

TARGET = cowdetector
CONFIG -= app_bundle

TEMPLATE = app

SOURCES += main.cpp \
    gpiointerface.cpp \
    detectorinterface.cpp \
    debugdetector.cpp \
    cowbox.cpp \
    innovationreader.cpp \
    cowdetector.cpp

HEADERS += \
    gpiointerface.h \
    detectorinterface.h \
    debugdetector.h \
    cowbox.h \
    innovationreader.h \
    cowdetector.h

win32 {
QT += quick qml
RESOURCES += \
    debug.qrc
SOURCES += \
    debuggpio.cpp
HEADERS += \
    debuggpio.h
}

!win32 {
CONFIG += console
SOURCES += \
    gertboard/gb_common.c \
    rpigpio.cpp \

HEADERS += \
    gertboard/gb_common.h \
    rpigpio.h \
}

DISTFILES += \
    cowdetector.json \
    main.qml \
    Led.qml

