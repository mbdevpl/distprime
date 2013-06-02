TEMPLATE = app
CONFIG += console
CONFIG -= app_bundle
CONFIG -= qt

TARGET = distprime

SOURCES += \
	mbdev_unix.c \
	listfunctions.c \
	distprimeserver.c \
    distprimeworker.c \
    distprimecommon.c

HEADERS += \
	mbdev_unix.h \
	listfunctions.h \
    distprimecommon.h

OTHER_FILES += Makefile
