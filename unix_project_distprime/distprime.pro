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

HEADERS += \
	mbdev_unix.h \
	listfunctions.h \
	distprimecommon.h \

OTHER_FILES += \
	Makefile \
	testxml.xml \
