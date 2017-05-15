#
# Makefile for 	-*-Makefile-*-
#  cree par Jean-Cédric Chappelier (Jean-Cedric.Chappelier@epfl.ch) le  26/11/15 
#
# $Id$
#

#CC       = gcc
CC       = clang
CPPFLAGS =

UNAME_S := $(shell uname -s)
ifeq ($(UNAME_S),Linux)
	CFLAGS += -DLINUX
endif
ifeq ($(UNAME_S),Darwin)
	CFLAGS += -DOSX -I/usr/local/opt/openssl/include/
endif

CFLAGS   += -std=c99
# CFLAGS   += -pedantic -g -Wall -Wextra -Wfloat-equal -Wshadow \
-Wpointer-arith -Wbad-function-cast -Wcast-qual -Wcast-align  \
-Wwrite-strings -Wconversion -Wunreachable-code
CFLAGS   += -fPIC
RM       = /bin/rm -f

TARGETS = libmongoose.so
OBJS = *.o
RUBS = $(OBJS) *~ core

all:: $(TARGETS)
	@echo All done.

libmongoose.so: mongoose.o
	$(CC) -shared -o $@ $^

clean:
	-@$(RM) $(RUBS)
	@echo Cleaned.

new: mrproper all

mrproper: clean
	-@$(RM) $(TARGETS)
