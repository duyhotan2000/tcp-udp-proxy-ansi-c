#
# Copyright (C) 2017 by DXO
# $Id: Makefile Sat Sep 2 18:30:04 GMT+07:00 2017 ctdao $
# Contact: DXO Team
#

DEBUG=0

DIRS := .
SRCDIR := $(DIRS)/src
BUILDDIR := $(DIRS)/build
INCL := -Iincl
SOURCES := $(shell find $(SRCDIR) -name "*.c")
PROG := $(BUILDDIR)/racing_proxy

ifeq ($(DEBUG),1)
CXXFLAGS_EXTRA=-pg -g -O3
else
CXXFLAGS_EXTRA=-O3
endif

CC := gcc
MD := mkdir -p
RM := rm -rf
CFLAGS := -g -Wall -Werror
CXXFLAGS := $(CFLAGS) $(CXXFLAGS_EXTRA) $(INCL)
LDFLAGS := -luv


$(PROG): $(SOURCES)
	$(MD) $(BUILDDIR)
	$(CC) $^ $(CXXFLAGS) -o $@ $(LDFLAGS)

.PHONY: all clean

all: $(PROG)

clean:
	$(RM) $(BUILDDIR)

