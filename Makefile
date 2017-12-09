#
# Copyright (C) 2017 by DXO
# $Id: Makefile Sat Sep 2 18:30:04 GMT+07:00 2017 ctdao $
# Contact: DXO Team
#

DEBUG=0

DIRS := .
SRCDIR := $(DIRS)/src
TESTDIR := $(DIRS)/test
CONFIGDIR := $(DIRS)/config
BUILDDIR := $(DIRS)/build
DEPS := $(DIRS)/deps
SOURCES := $(shell find $(SRCDIR) -name "*.c")
PROG := $(BUILDDIR)/racing_proxy
SRCTEST := $(shell find $(TESTDIR) -name "udpBroadcastServer.c")
CONFIG := $(shell find $(SRCDIR) -name "parse_config.c")
UTIL := $(shell find $(SRCDIR) -name "util.c")
CONFIGOBJ := $(BUILDDIR)/config.o
UTILOBJ := $(BUILDDIR)/util.o
LOGGEROBJ := $(DEPS)/logger/logger.o
LIBCONFIG := $(BUILDDIR)/libconfig.a
PROGTEST := $(BUILDDIR)/test_racing_proxy

ifeq ($(DEBUG),1)
CXXFLAGS_EXTRA=-pg -g -O3
else
CXXFLAGS_EXTRA=-O3
endif

CC := gcc
AR := ar rcs
MD := mkdir -p
RM := rm -rf
CD := cd
CP := cp
INCL := -Iincl -Ideps/libuv-v1.14.1/include/ -Ideps/logger
CFLAGS := -g -Wall
CXXFLAGS := $(CFLAGS) $(CXXFLAGS_EXTRA) $(INCL)
LDFLAGS := -Ldeps/libuv-v1.14.1 -luv \
	-Ldeps/logger -llogger -lpthread -pthread
LDFLAGS_EXTRA := -Lbuild -lconfig


$(PROG): $(SOURCES) libuv.a liblogger.a
	$(MD) $(BUILDDIR)
	$(CC) $(SOURCES) $(CXXFLAGS) -o $@ $(LDFLAGS)
	$(CP) $(CONFIGDIR)/config.conf $(BUILDDIR)

libuv.a:
	$(CD) deps/libuv-v1.14.1 && ./autogen.sh && ./configure && make
	$(CP) deps/libuv-v1.14.1/.libs/libuv.a $(DEPS)/libuv-v1.14.1

liblogger.a:
	$(CD) deps/logger && make liblogger.a

$(PROGTEST): $(SRCTEST) libuv.a liblogger.a $(LIBCONFIG)
	$(CC) $(SRCTEST) $(CXXFLAGS) -o $@ $(LDFLAGS) $(LDFLAGS_EXTRA)
	$(CP) $(CONFIGDIR)/config.conf $(BUILDDIR)

$(LIBCONFIG): $(CONFIGOBJ) $(UTILOBJ)
	$(AR) $@ $^ $(LOGGEROBJ)

$(CONFIGOBJ):
	$(MD) $(BUILDDIR)
	$(CC) -O -c $(CONFIG) -o $@ $(INCL)

$(UTILOBJ):
	$(CC) -O -c $(UTIL) -o $@ $(INCL)

.PHONY: all test clean

all: $(PROG) $(PROGTEST)

test: $(PROGTEST)

clean:
	$(RM) $(BUILDDIR)
