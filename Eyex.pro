#-------------------------------------------------
#
# Project created by QtCreator 2014-09-09T15:21:50
#
#-------------------------------------------------

QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = Eyex
TEMPLATE = app

win32:INCLUDEPATH += D:/Developer/tobii-eyex-sdk-cpp/include

LIBS += Tobii.EyeX.Client.lib

SOURCES += main.cpp\
        mainwindow.cpp

HEADERS  += mainwindow.h

FORMS    += mainwindow.ui
