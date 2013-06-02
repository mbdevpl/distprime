TEMPLATE = app
CONFIG += console
CONFIG -= app_bundle
CONFIG -= qt

TARGET = distprime

SOURCES += \
	distprimeserver.c \
	distprimeclient.c \
	listfunctions.c \
	mbdev_unix.c \

HEADERS += \
	mbdev_unix.h \
    listfunctions.h

OTHER_FILES += Makefile
