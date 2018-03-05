TARGET = bradypod-qpa
CONFIG += static

PLUGIN_TYPE = platforms
PLUGIN_CLASS_NAME = BradypodIntegrationPlugin
!equals(TARGET, $$QT_DEFAULT_QPA_PLUGIN): PLUGIN_EXTENDS = -
load(qt_plugin)

SOURCES += $$PWD/main.cpp

include(bradypod-qpa.pri)
