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
	processdata.c \
	workerdata.c \
	serverdata.c \
	primerange.c \
	distprimeserver.c \
	distprimeworker.c \

HEADERS += \
	mbdev_unix.h \
	listfunctions.h \
	defines.h \
	distprimecommon.h \
	processdata.h \
	workerdata.h \
	serverdata.h \
	primerange.h \

OTHER_FILES += \
	Makefile \
	readme.html \
	testxml.xml \
