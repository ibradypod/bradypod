QT += core-private gui-private \
    service_support-private theme_support-private \
    eventdispatcher_support-private fontdatabase_support-private

qtHaveModule(linuxaccessibility_support-private): \
    QT += linuxaccessibility_support-private

SOURCES += $$PWD/bradypodintegration.cpp \
           $$PWD/bradypodbackingstore.cpp

HEADERS += $$PWD/bradypodintegration.h \
           $$PWD/bradypodbackingstore.h

QMAKE_LFLAGS += $$QMAKE_LFLAGS_NOUNDEF

INCLUDEPATH += $$PWD

CONFIG += qpa/genericunixfontdatabase

OTHER_FILES += $$PWD/bradypod.json
