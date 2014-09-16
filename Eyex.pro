# Copyright (c) 2014 Oliver Lau <ola@ct.de>, Heise Zeitschriften Verlag
# All rights reserved.

QT += core gui widgets opengl multimediawidgets

TARGET = Eyex
TEMPLATE = app

DEFINES += FILTER_FADE_TO_BLACK=1
DEFINES += _CRT_SECURE_NO_WARNINGS

SOURCES += main.cpp\
    mainwindow.cpp \
    eyexhost.cpp \
    quiltwidget.cpp \
    videowidgetsurface.cpp \
    videowidget.cpp \
    renderwidget.cpp \
    kernel.cpp \
    decoderthread.cpp \
    semaphores.cpp

HEADERS  += mainwindow.h \
    eyexhost.h \
    quiltwidget.h \
    videowidgetsurface.h \
    videowidget.h \
    renderwidget.h \
    util.h \
    main.h \
    kernel.h \
    decoderthread.h \
    sample.h \
    semaphores.h

FORMS += mainwindow.ui

RESOURCES += \
    eyex.qrc

OTHER_FILES += \
    shaders/hblur.frag \
    shaders/vblur.frag \
    shaders/default.vert \
    shaders/pixelize.frag \
    shaders/fade2black.frag \
    shaders/foo.vert \
    default.frag


### TOBII EYEX ###
TOBII_EYEX_SDK_PATH = $$PWD/tobii-eyex-sdk
QMAKE_LIBDIR += $$TOBII_EYEX_SDK_PATH/lib/x86 $$TOBII_EYEX_SDK_PATH/lib/x64
INCLUDEPATH += $$TOBII_EYEX_SDK_PATH/include
win32 {
LIBS += Tobii.EyeX.Client.lib
}


### FFMPEG ###
QMAKE_LIBDIR += $$PWD/ffmpeg/lib
LIBS += avdevice.lib avutil.lib avcodec.lib avformat.lib swscale.lib
INCLUDEPATH += $$PWD/ffmpeg/include
DEFINES += __STDC_CONSTANT_MACROS
