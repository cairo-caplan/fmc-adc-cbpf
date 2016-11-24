TEMPLATE = app
CONFIG += console c++11
CONFIG -= app_bundle
CONFIG -= qt

SOURCES += main.c

#LIBADC = ../lib/
#ZIO ?= ../zio

#CFLAGS = -Wall -g -ggdb -I$(LIBADC) -I$(ZIO)/include -I../kernel $(EXTRACFLAGS)
#LDFLAGS = -L$(LIBADC) -lfmcadc -lpthread -lrt

INCLUDEPATH += ../../../zio/include
INCLUDEPATH += ../../../lib/include
INCLUDEPATH += ../../../kernel

LIBS += -L"../../../lib/" -L"../../../zio/" -lfmcadc -lpthread -lrt

