QT += core gui
QT -= widgets
CONFIG += console c++11
CONFIG -= app_bundle
TARGET = kgstv-offline
INCLUDEPATH += ../../src/kgstv
SOURCES += main.cpp \
           ../../src/kgstv/kgstvprotocol.cpp \
           ../../src/kgstv/kgstvcontroller.cpp \
           ../../src/kgstv/kgstvaudio.cpp \
           ../../src/kgstv/kgstvimage.cpp \
           ../../src/kgstv/kgstvtransfer.cpp
HEADERS += ../../src/kgstv/kgstvprotocol.h \
           ../../src/kgstv/kgstvcontroller.h \
           ../../src/kgstv/kgstvaudio.h \
           ../../src/kgstv/kgstvimage.h \
           ../../src/kgstv/kgstvtransfer.h \
           ../../src/kgstv/kgstvtables.h
