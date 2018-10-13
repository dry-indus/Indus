#-------------------------------------------------
#
# Project created by QtCreator 2015-03-16T14:11:36
#
#-------------------------------------------------

QT       += core gui sql concurrent

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = multiDatabase
TEMPLATE = app

CONFIG += c++11
CONFIG += console

SOURCES += main.cpp \
    multiDatabase/multiDatabase.cpp

HEADERS  += \
    multiDatabase/multiDatabase.h

FORMS    +=
