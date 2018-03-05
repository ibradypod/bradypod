#-------------------------------------------------
#
# Project created by QtCreator 2017-06-05T11:19:59
#
#-------------------------------------------------

QT     += core core-private xml webkitwidgets webkitwidgets-private webkit webkit-private network network-private

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = bradypod
TEMPLATE = app

CONFIG += app_bundle
CONFIG += console c++11
CONFIG += staticlib

DESTDIR = ../bin

#CONFIG(static) {
#    WEB_INSPECTOR_RESOURCES_DIR = $$(WEB_INSPECTOR_RESOURCES_DIR)
#    isEmpty(WEB_INSPECTOR_RESOURCES_DIR): {
#        error("You must set the environment variable WEB_INSPECTOR_RESOURCES_DIR to generated Web Inspector resources")
#    }
#
#    RESOURCES += \
#        $(WEB_INSPECTOR_RESOURCES_DIR)/WebInspector.qrc
#    message("Using Web Inspector resources from $(WEB_INSPECTOR_RESOURCES_DIR)")
#}

SOURCES += main.cpp\
    cookiejar.cpp \
    networkaccessmanager.cpp \
    webpage.cpp \
    config.cpp \
    bradypod.cpp \
    utils.cpp \
    crashdump.cpp \
    encoding.cpp \
    env.cpp \
    filesystem.cpp \
    system.cpp \
    terminal.cpp \
    qcommandline.cpp \
    callback.cpp \
    domparser.cpp \
    htmlloader.cpp \
    qwebviewaccessible.cpp

HEADERS  += \
    cookiejar.h \
    networkaccessmanager.h \
    webpage.h \
    config.h \
    consts.h \
    bradypod.h \
    utils.h \
    crashdump.h \
    encoding.h \
    env.h \
    filesystem.h \
    system.h \
    terminal.h \
    qcommandline.h \
    callback.h \
    domparser.h \
    htmlloader.h

RESOURCES += \
    bradypod.qrc

DISTFILES += \
    test_uri.txt

win32 {
   RC_FILE = bradypod.rc
}

mac {
    QMAKE_CXXFLAGS += -fvisibility=hidden
    QMAKE_LFLAGS += '-sectcreate __TEXT __info_plist Info_mac.plist'
    CONFIG -= app_bundle
    ICON = bradypod.icns
    QMAKE_INFO_PLIST = Info_mac.plist
}

win32-msvc* {
    DEFINES += NOMINMAX \
        WIN32_LEAN_AND_MEAN \
        _CRT_SECURE_NO_WARNINGS
    # ingore warnings:
    # 4049 - locally defined symbol 'symbol' imported
    QMAKE_LFLAGS += /ignore:4049 /LARGEADDRESSAWARE
    CONFIG(static) {
        DEFINES += STATIC_BUILD
    }
    LIBS += -licudt
}

linux {
    # include($$PWD/qt-qpa-platform-plugin/bradypod-qpa.pri)

    CONFIG += c++11
     QTPLUGIN.platforms = bradypod-qpa
    # LIBS += -L$$PWD/../qt-qpa-platform-plugin/plugins/platforms
    # LIBS += -L$$PWD/qt-qpa-platform-plugin/plugins/platforms
    # LIBS += -L$$PWD/qt/qtbase/plugins/platforms
    # LIBS += -lbradypod-qpa
}

openbsd* {
    LIBS += -L/usr/X11R6/lib
}

#unix:!macx: {
#    LIBS += /usr/lib64/libxslt.a /usr/local/lib/libsqlite3.a /usr/lib64/libxml2.a
#}
