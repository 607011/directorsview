# Copyright (c) 2014 Oliver Lau <ola@ct.de>, Heise Zeitschriften Verlag
# All rights reserved.

QT += core gui widgets opengl multimediawidgets

TARGET = Eyex
TEMPLATE = app

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
    renderwidget.cpp

HEADERS  += mainwindow.h \
    eyexhost.h \
    quiltwidget.h \
    videowidgetsurface.h \
    videowidget.h \
    renderwidget.h \
    util.h \
    main.h

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
