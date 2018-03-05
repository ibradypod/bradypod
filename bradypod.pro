TEMPLATE = subdirs
CONFIG += ordered

linux {
    SUBDIRS += $$PWD/src/qt-qpa-platform-plugin/bradypod-qpa.pro
}

SUBDIRS += $$PWD/src/bradypod.pro

linux {
    bradypod.depends = bradypod-qpa
}
