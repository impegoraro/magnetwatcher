QT += core gui network

CONFIG += c++11

TARGET = magnet-watcher
CONFIG += console
CONFIG -= app_bundle

TEMPLATE = app

SOURCES += main.cpp \
    transmission.cpp

HEADERS += \
    transmission.h
