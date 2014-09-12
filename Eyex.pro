# Copyright (c) 2014 Oliver Lau <ola@ct.de>, Heise Zeitschriften Verlag
# All rights reserved.

QT += core gui widgets opengl multimediawidgets

TARGET = Eyex
TEMPLATE = app

DEFINES += FILTER_FADE_TO_BLACK=1
DEFINES += _CRT_SECURE_NO_WARNINGS

TOBII_EYEX_SDK_PATH = $$PWD/tobii-eyex-sdk

QMAKE_LIBDIR += $$TOBII_EYEX_SDK_PATH/lib/x86 $$TOBII_EYEX_SDK_PATH/lib/x64
INCLUDEPATH += $$TOBII_EYEX_SDK_PATH/include

win32 {
LIBS += Tobii.EyeX.Client.lib
}

SOURCES += main.cpp\
    mainwindow.cpp \
    eyexhost.cpp \
    quiltwidget.cpp \
    videowidgetsurface.cpp \
    videowidget.cpp \
    renderwidget.cpp \
    kernel.cpp \
    decoderthread.cpp

HEADERS  += mainwindow.h \
    eyexhost.h \
    quiltwidget.h \
    videowidgetsurface.h \
    videowidget.h \
    renderwidget.h \
    util.h \
    main.h \
    kernel.h \
    decoderthread.h

FORMS    += mainwindow.ui

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


QTFFMPEGWRAPPER_SOURCE_PATH = $$PWD/qtffmpegwrapper
FFMPEG_LIBRARY_PATH = $$PWD/qtffmpegwrapper/lib/x86
FFMPEG_INCLUDE_PATH = $$QTFFMPEGWRAPPER_SOURCE_PATH
SOURCES += $$QTFFMPEGWRAPPER_SOURCE_PATH/QVideoEncoder.cpp \
    $$QTFFMPEGWRAPPER_SOURCE_PATH/QVideoDecoder.cpp
HEADERS += $$QTFFMPEGWRAPPER_SOURCE_PATH/QVideoEncoder.h \
    $$QTFFMPEGWRAPPER_SOURCE_PATH/QVideoDecoder.h
LIBS += -lavutil -lavcodec -lavformat -lswscale
LIBS += -L$$FFMPEG_LIBRARY_PATH
INCLUDEPATH += QVideoEncoder
INCLUDEPATH += $$FFMPEG_INCLUDE_PATH
DEFINES += __STDC_CONSTANT_MACROS
