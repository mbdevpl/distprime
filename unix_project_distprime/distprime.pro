TEMPLATE = app
CONFIG += console
CONFIG -= app_bundle
CONFIG -= qt

TARGET = distprime

INCLUDEPATH += $$quote(/usr/include/libxml2)

SOURCES += \
	mbdev_unix.c \
	listfunctions.c \
	distprimecommon.c \
	distprimeserver.c \
	distprimeworker.c \
    workerdata.c \
    processdata.c \
    serverdata.c \
    primerange.c

HEADERS += \
	mbdev_unix.h \
	listfunctions.h \
	distprimecommon.h \
    workerdata.h \
    processdata.h \
    serverdata.h \
    defines.h \
    primerange.h

OTHER_FILES += \
	Makefile \
	testxml.xml \
