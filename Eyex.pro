# Copyright (c) 2014 Oliver Lau <ola@ct.de>, Heise Zeitschriften Verlag
# All rights reserved.

QT += core gui multimediawidgets

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = Eyex
TEMPLATE = app

win32:INCLUDEPATH += D:/Developer/tobii-eyex-sdk-cpp/include

LIBS += Tobii.EyeX.Client.lib

SOURCES += main.cpp\
        mainwindow.cpp \
    eyexhost.cpp

HEADERS  += mainwindow.h \
    eyexhost.h

FORMS    += mainwindow.ui
